#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*
 * set_bytes - puts n bytes of data of v into characters in s
 */
void set_bytes(void *s, int off, int n, unsigned long v){
	char *ns = (char*)s;

	char nibble;
	for (int i = 0; i < n; i++) {
		nibble = v>>(i*8) & 255;
		ns[i+off] = nibble;
	}
}

/*
 * get_bytes - copy n characters from s and put them, inorder, into v
 */
void *get_bytes(void **s, int off, int n, void *v){
	char *ns = (char*)*s;
	char nibble;
	unsigned long new_v = 0;
	for (int i = 0; i < n; i++) {
		nibble = *(ns+i+off);
		new_v = new_v | nibble << (i*8);
	}
	*(unsigned long *)v = new_v;
	*s = (void*)ns;
	return s;
}

int main(int argc, char **argv){
	unsigned char x[64] = {};
	u_int16_t t = 12345;
	memcpy(x, &t, sizeof(u_int16_t));
	return 0;
}
