#include "simplefs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
/*
 * any other includes you require
 */

/*
 * defined constants and structure declarations you need
 */
#define BLOCK_SIZE 64
#define MAX_BLOCKS (1<<16)
#define BOOT_INDEX 0
#define FH_INDEX 1
#define MST_INDEX_BLOCK 17
#define MST_DIR_ENTRIES 18
#define FH_PER_BLOCK 16
#define ENTRY_PER_BLOCK 8

typedef struct master{
	u_int16_t isinit;
	u_int16_t first_free;
	unsigned char extra [BLOCK_SIZE - sizeof(u_int16_t) * 2];
} Master;

typedef struct freeblock{
	u_int16_t next_block;
	unsigned char extra[BLOCK_SIZE - sizeof(u_int16_t)];
} FreeBlock;

typedef struct fileheader{
	u_int16_t index_block;
	u_int16_t size;
} FileHeader;

typedef struct direntry{
	char name[6];
	u_int16_t fileid;
} DirEntry;


typedef union block {
	unsigned char raw[BLOCK_SIZE];
	FreeBlock fbl;
	Master mst;
	FileHeader fhs[BLOCK_SIZE/sizeof(FileHeader)];
	DirEntry des[BLOCK_SIZE/sizeof(DirEntry)];
	u_int16_t ind[BLOCK_SIZE/sizeof(u_int16_t)];
} Block;


void set_char_arr(void *arr, int off, void *val, unsigned int len){
	memcpy((char*)arr + off, val, len);
}

void get_char_arr(void *arr, int off, void *val, unsigned int len){
	memcpy(val, (char*)arr + off, len);
}


typedef struct self {
    /* members you need in the instance-specific structure */
	Block *blocks; // an array of blocks
	unsigned long file_size;
	u_int16_t next_free;
	char *filename;
} Self;

void get_entry(Self *s, unsigned long index, unsigned int section_off, void*entry, unsigned int entry_size){
	// Gets an entry from any section of the file
	unsigned int entry_per_block = BLOCK_SIZE / entry_size;
	unsigned int block = index / entry_per_block;
	unsigned int row = (index - (block * entry_per_block)) * entry_size;
	get_char_arr(s->blocks[block + section_off].raw, row, entry, entry_size);
}

/*
 * helper functions needed in your method implementations
 */
Self *return_self(const SimpleFS *fs){
	return (Self *)fs->self;
}

static void sf_destroy(const SimpleFS *fs) {
	// write all changes into file in linux system
	Self *s = return_self(fs);
	int fd = open(s->filename, O_WRONLY);
	if(write(fd, s->blocks, s->file_size) < (long int)s->file_size){
	   printf("Could not write all the data: %ld\n", s->file_size);	// write all data
	}
	close(fd);
	munmap(s->blocks, s->file_size);
	// free adt
	free(s->filename);
	free(s);
	free((void*)fs);
}

static void sf_init(const SimpleFS *fs) {
	// initializes the linux file for our filesystem
	// format boot block
	Self *s = return_self(fs);
	s->blocks[BOOT_INDEX].mst.isinit = 12345; // indicate that file system is initialized
	s->blocks[BOOT_INDEX].mst.first_free = 50; // indicates first free block
	s->next_free = 50;
	for (int i = FH_INDEX; i < MST_INDEX_BLOCK; i++) {
		memset(s->blocks[i].raw, 0, BLOCK_SIZE); // clears out fileheaders
	} // master fileheader already set 
	// set master directory entry
	DirEntry master = {"mstdir", 0};
	s->blocks[MST_DIR_ENTRIES].des[0] = master;
	s->blocks[1].fhs[0].index_block = MST_INDEX_BLOCK;	
	// link blocks
	for (unsigned long i = 50; i < (s->file_size/BLOCK_SIZE) - 1; i++) {
		s->blocks[i].fbl.next_block = i + 1;
	}
	
}

u_int16_t get_next_fbl(Self *s){
	// Get the next free block avaliable to the file system
	u_int16_t next_block = s->next_free;
	s->next_free = s->blocks[s->next_free].fbl.next_block;
	return next_block;
}

void fileid_to_blckrw(u_int16_t fileid, unsigned int entry_per_block, unsigned int *block, unsigned int *row){
	// given a fileid retrieve which relative block and entry in block it would be in
	*block = fileid / entry_per_block;
	*row = fileid - (*block * entry_per_block);
}

void blckrw_to_fileid(u_int16_t *fileid, unsigned int entry_per_block, unsigned int block, unsigned int row){
	// given a relative block number and a row into that block return it's fileid
	*fileid = (block * entry_per_block) + row;
}

FileHeader *find_empty_fh(Self*s, u_int16_t *fileid){
	Block *fh_b;
	for (int entry_block = FH_INDEX; entry_block < MST_INDEX_BLOCK; entry_block++) {
		fh_b = s->blocks + entry_block;
		for (int entry_row = 0; entry_row < FH_PER_BLOCK; entry_row++) {
			if (fh_b->fhs[entry_row].index_block == 0) {
				if (fileid != NULL) {*fileid = ((entry_block - FH_INDEX) * FH_PER_BLOCK) + entry_row;}
				return fh_b->fhs + entry_row;
			}
		}
	}
	return NULL;
}

DirEntry *find_empty_entry(Self*s, u_int16_t *fileid){
	Block *fh_b;
	for (int entry_block = MST_DIR_ENTRIES; entry_block < 50; entry_block++) {
		fh_b = s->blocks + entry_block;
		for (int entry_row = 0; entry_row < ENTRY_PER_BLOCK; entry_row++) {
			if (fh_b->des[entry_row].name[0] == 0) {
				if (fileid != NULL) {*fileid = ((entry_block - MST_DIR_ENTRIES) * ENTRY_PER_BLOCK) + entry_row;}
				return fh_b->des + entry_row;
			}
		}
	}
	return NULL;
}
static bool sf_create(const SimpleFS *fs, char *name) {
	// this creates a file in the file system
	Self *s = return_self(fs);
	FileHeader *fh;
	DirEntry *de;
	u_int16_t dir_fileid;
	u_int16_t fh_fileid;
	// find the next file header to allocate
	fh = find_empty_fh(s, &fh_fileid);
	de = find_empty_entry(s, &dir_fileid);
	if (fh_fileid != dir_fileid) {
		return false;
	}
	fh->index_block = get_next_fbl(s); // assign index block
	de->fileid = fh_fileid; // set fileid in directory entry
	memset(s->blocks + fh->index_block, 0, BLOCK_SIZE); // clear index block
	memcpy(de->name, name, 6); // set name of directory entry
	
    return true;
}

DirEntry *find_entry(Self *s, char *name, u_int16_t *fileid){

	Block *b;
	
	for (int i = MST_DIR_ENTRIES; i < 50; i++) {
		b = s->blocks + i;
		for (int j = 0; j < ENTRY_PER_BLOCK; j++) {
			if (strncmp(b->des[j].name, name, 6) == 0) {
				*fileid = b->des[j].fileid;
				return b->des + j;
			}
		}
	}
	return NULL;
}

FileHeader *find_fh(Self *s, u_int16_t fileid){
	u_int16_t blockid = fileid / BLOCK_SIZE;

	FileHeader *fh = s->blocks[blockid + FH_INDEX].fhs + fileid - (blockid * FH_PER_BLOCK);
	if (fh->index_block==0) {
		return NULL;
	}
	return fh;
}

void return_to_freeblocks(Self *s, u_int16_t block_num){
	Block *new_free = s->blocks + block_num;
	memset(new_free, 0, BLOCK_SIZE); // clear out data
	new_free->fbl.next_block = s->next_free; // sticking newly freed block to the head of the free block linked list
	s->next_free = block_num; // setting new next free block
}

void clear_index_block(Self *s, Block *index){
	unsigned int row = 0;
	while (index->ind[row] != 0) {
		return_to_freeblocks(s, index->ind[row++]);
	}
	memset(index, 0, BLOCK_SIZE);
}

static bool sf_remove(const SimpleFS *fs, char *name) {
	Self *s = return_self(fs);
	u_int16_t fileid;
	DirEntry *de;
	FileHeader *fh;
	// removes file `name` from file system
	if ((de = find_entry(s, name, &fileid))==NULL) {
		return false;
	}
	unsigned int block = fileid / FH_PER_BLOCK;
	fh = s->blocks[block + FH_INDEX].fhs + (fileid - (block * FH_PER_BLOCK));
	// putting all its allocated blocks back into the free blocks linked list
	clear_index_block(s, s->blocks + fh->index_block);
	memset(fh, 0, sizeof(FileHeader)); // clear filheader
	memset(de, 0, sizeof(DirEntry)); // clear entry
    return true;
}

static bool sf_write(const SimpleFS *fs, char *name, char *content) {
	// write `content` to file `name`
	Self *s = return_self(fs);
	DirEntry *en;
	FileHeader *fh;
	u_int16_t fileid;
	unsigned long cpysize;
	if((en = find_entry(s, name, &fileid)) == NULL){ // get the directory entry
		return false;
	}
	if ((fh = find_fh(s, fileid)) == NULL) { // get fileheader
		return false;
	}
	Block *ind = s->blocks + fh->index_block;
	Block *data;
	u_int16_t new_data_id;	
	
	char *end = strchr(content, '\0');

	for (int b = 0; b <= (end - content)/BLOCK_SIZE; b++) {
		if (ind->ind[b] == 0) {
			new_data_id = get_next_fbl(s); // get a new data block
			ind->ind[b] = new_data_id; // add the new data block location to the index block
		}
		data = s->blocks + ind->ind[b]; // get the data block
		cpysize = strlen(content + (b*BLOCK_SIZE)) < BLOCK_SIZE ? strlen(content + (b*BLOCK_SIZE)) : BLOCK_SIZE; // get the size of data to copy
		memcpy(data->raw, content, cpysize); // copy the data into the block
		fh->size = cpysize; // change file size
	}	
    return true;
}

static bool sf_read(const SimpleFS *fs, char *name, char *content) {
	// read all data from file `name` and put on standard output
	DirEntry *en;
	FileHeader *fh;
	Block *ind;
	Block *data;
	char ret[BUFSIZ];
	u_int16_t fileid;
	long int cpysize;
	Self *s = return_self(fs);
	if ((en = find_entry(s, name, &fileid)) == NULL) {
		return false;
	}
	if ((fh = find_fh(s, fileid)) == NULL) {
		return false;
	}
	ind = s->blocks + fh->index_block;
	ret[0] = '\0';
	for (int b = 0; b <= fh->size/BLOCK_SIZE; b++) {
		data = s->blocks + ind->ind[b];
		cpysize = (fh->size - (b*BLOCK_SIZE)) < 64 ? fh->size - (b*BLOCK_SIZE) : BLOCK_SIZE;
		strncat(ret, (char *)data->raw, cpysize);
	}
	strcpy(content, ret);
    return true;
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

const SimpleFS *SimpleFS_create(char *filename) {
	SimpleFS *fs = (SimpleFS *)malloc(sizeof(SimpleFS));
	if(fs == NULL){
		printf("Failed to allocated file system\n");
		exit(EXIT_FAILURE);
	}
	*fs = template;
	Self *s = (Self *)malloc(sizeof(Self));
	if (s == NULL) {
		free(fs);
		printf("Failed to allocate file system data\n");
		exit(EXIT_FAILURE);
	}
	int f = open(filename, O_RDWR);
	struct stat st;
	if (f < 0) {
		printf("Could not open file %s\n", filename);
		free(s);
		free(fs);
		exit(EXIT_FAILURE);
	}
	fstat(f, &st);

	s->blocks = (Block *)mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, f, 0);
	s->file_size = st.st_size;
	s->next_free = s->blocks[0].mst.first_free;
	s->filename = strdup(filename);
	fs->self = s;
	close(f);
	return fs;

}
