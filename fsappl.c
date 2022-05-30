#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void printHelp(void) {
    printf("Legal commands:\n");
    printf("  ? - this list of commands\n");
    printf("  dump\n");
    printf("  exit\n");
    printf("  info\n");
    printf("  init\n");
    printf("  list\n");
    printf("  block <number>\n");
    printf("  create <filename>\n");
    printf("  remove <filename>\n");
    printf("  read <filename>\n");
    printf("  write <filename>\n");
}

int main(int argc, char *argv[]) {
    const SimpleFS *fs;
    char cmd[1024];

    if (argc != 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
	return EXIT_FAILURE;
    }
    if ((fs = SimpleFS_create(argv[1])) == NULL) {
        printf("%s: error mapping %s\n", argv[0], argv[1]);
	return EXIT_FAILURE;
    }
    while (fgets(cmd, sizeof cmd, stdin) != NULL) {
        char buf[10240];
        char *p = strrchr(cmd, '\n');
	if (p != NULL)
            *p = '\0';
        if (strlen(cmd) == 0)         /* empty line */
            continue;
        if (strcmp(cmd, "exit") == 0)
            break;
	if (strcmp(cmd, "?") == 0)
            printHelp();
	else if (strcmp(cmd, "init") == 0)
            fs->init(fs);
        else if (strcmp(cmd, "list") == 0) {
            fs->list(fs, buf);
	    fputs(buf, stdout);
        } else if (strcmp(cmd, "info") == 0) {
            fs->info(fs, buf);
	    fputs(buf, stdout);
	} else if (strcmp(cmd, "dump") == 0) {
            fs->dump(fs, buf);
	    fputs(buf, stdout);
	} else {
            char extra[1024];
	    char *q = strchr(cmd, ' ');
	    if (q != NULL)
	        *q++ = '\0';
	    else
                q = cmd + strlen(cmd);
	    while (*q == ' ')
                q++;
	    strcpy(extra, q);
	    if (strcmp(cmd, "create") == 0) {
                if (! fs->create(fs, extra))
                    fprintf(stderr, "%s: create %s failed!\n", argv[0], extra);
	    } else if (strcmp(cmd, "remove") == 0) {
                if (! fs->remove(fs, extra))
                    fprintf(stderr, "%s: remove %s failed!\n", argv[0], extra);
	    } else if (strcmp(cmd, "read") == 0) {
                buf[0] = '\0';
                if (! fs->read(fs, extra, buf))
                    fprintf(stderr, "%s: read %s failed!\n", argv[0], extra);
		fputs(buf, stdout);
	    } else if (strcmp(cmd, "block") == 0) {
                int n = atoi(extra);
                if (! fs->block(fs, n, buf))
                    fprintf(stderr, "%s: block %s failed!\n", argv[0], extra);
		fputs(buf, stdout);
	    } else if (strcmp(cmd, "write") == 0) {
                char line[BUFSIZ];
                // first collect input up to a line consisting of ".\n"
		buf[0] = '\0';
		while (fgets(line, sizeof line, stdin) != NULL) {
                    if (strcmp(line, ".\n") == 0)
                        break;
                    strcat(buf, line);
		}
                if (! fs->write(fs, extra, buf))
                    fprintf(stderr, "%s: write %s failed!\n", argv[0], extra);
	    } else {
                fprintf(stderr, "%s: illegal command - %s\n", argv[0], cmd);
	    }
	}
    }
    fs->destroy(fs);
    return EXIT_SUCCESS;
}
