#include "simplefs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
/*
 * any other includes you require
 */

/*
 * defined constants and structure declarations you need
 */
const int block_size = 64;
const int max_blocks = 256;
const u_int16_t boot_block_id = 0;
const u_int16_t fh_block_start = 1;
const u_int16_t num_fh_blocks = 16;
const u_int16_t master_dir_index_block = 17;
const u_int16_t directory_data_start = 18;
const u_int16_t num_dd_blocks = 31;
const u_int16_t size_fh = 4;

// converts str to an int by len number of characters
int atoni(const char*str, int len){

	int i;
	int ret=0;
	for (i = 0; i < len; i++) {
		ret = ret * 10 + (str[i] - '0');
	}
	return ret;
}

typedef struct block{
	char *data;
} Block;

typedef struct self {
    /* members you need in the instance-specific structure */
	char *filename;
	Block **file_contents;
	bool formatted;
	u_int16_t first_free;
	int num_written_blocks;
} Self;

void simplefs_free_self(Self *s){
	free(s->filename);
	for (int i = 0; i < s->num_written_blocks; i++) {
		free(s->file_contents[i]);
	}
	free(s->file_contents);

}

/*
 * helper functions needed in your method implementations
 */

static void sf_destroy(const SimpleFS *fs) {
	// write all changes into file in linux system
	// free adt
}

static void sf_init(const SimpleFS *fs) {
	// initializes the linux file for our filesystem
	
	// the file system should be empty
}

static bool sf_create(const SimpleFS *fs, char *name) {
	// this creates a file in the file system
	// possibly allocating a data block and page header for `name`
    return false;
}

static bool sf_remove(const SimpleFS *fs, char *name) {
	// removes file `name` from file system
	// putting all its allocated blocks back into the free blocks linked list
    return false;
}

static bool sf_write(const SimpleFS *fs, char *name, char *content) {
	// write `content` to file `name`
    return false;
}

static bool sf_read(const SimpleFS *fs, char *name, char *content) {
	// read all data from file `name` and put on standard output
    return false;
}

static bool sf_list(const SimpleFS *fs, char *filenames) {
    return false;
}

static bool sf_info(const SimpleFS *fs, char *information) {
    return false;
}

static bool sf_dump(const SimpleFS *fs, char *dumpinfo) {
    return false;
}

static bool sf_block(const SimpleFS *fs, int block, char *blockinfo) {
    return false;
}

static const SimpleFS template = {
    NULL, sf_destroy, sf_init, sf_create, sf_remove, sf_write, sf_read,
    sf_list, sf_info, sf_dump, sf_block
};

static const Self blank = {NULL, NULL, 0, 0, 0};

const SimpleFS *SimpleFS_create(char *filename) {
	u_int16_t first_free;
	// We need to open this file if it exists
	// set up the adt
	SimpleFS *fs = (SimpleFS *)malloc(sizeof(SimpleFS));
	*fs = template;
	if (fs == NULL) {
		return fs;
	}
	
	Self *s = (Self *)malloc(sizeof(Self));
	if (s == NULL) {
		free((void*)fs);
		return NULL;
	}
	*s = blank;
	s->filename = strdup(filename);
	fs->self = (void*)s;
	// map contents of `filename`
	FILE *f = fopen(filename, "r");
	if (f == NULL) {
		printf("Could not open file %s", filename);
		simplefs_free_self(s);
		free(fs);
		exit(EXIT_FAILURE);
	}
	char *buf = (char*)malloc(block_size);
	Block *new_block;
	s->file_contents = (Block **)malloc(sizeof(Block*) * max_blocks);
	int fcts_ind = 0;
	while (fgets(buf, block_size, f) > 0) {
		new_block = (Block*)malloc(sizeof(Block));
		new_block->data = strdup(buf);
		s->file_contents[fcts_ind++] = new_block;
	}
	s->num_written_blocks = fcts_ind;
	s->formatted = atoni(s->file_contents[boot_block_id]->data, sizeof(u_int16_t)) == 12345; // check if file is formatted
	s->first_free = s->formatted ? atoni(s->file_contents[boot_block_id]->data+sizeof(u_int16_t), sizeof(u_int16_t)) : 50; // indicates first free block
	// return the adt
	fclose(f);
    return fs;
}
