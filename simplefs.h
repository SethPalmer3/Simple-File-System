#ifndef _SIMPLEFS_H_
#define _SIMPLEFS_H_

/*
 * Copyright (c) 2022, University of Oregon
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the University of Oregon nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * interface definition for simple file system ADT
 */

#include "ADTs/ADTdefs.h"

typedef struct simplefs SimpleFS;        /* forward reference */

/*
 * this function signature is provided as the default constructor for
 * any SimpleFS implementation
 *
 * maps the contents of `filename', which is a file system image, into memory;
 * the file system image is manipulated by the ADT methods; since the image
 * is mapped into memory, when the destroy() method is invoked, all changes
 * are permanently reflected in `filename'
 *
 * returns a pointer to the simplefs instance, or NULL if errors
 */
const SimpleFS *SimpleFS_create(char *filename);

/*
 * dispatch table for a generic Queue
 */
struct simplefs {
/*
 * the private data of the simple file system
 */
    void *self;

/*
 * destroys the SimpleFS;
 * unmaps the file from memory
 * the storage associated with the SimpleFS is then returned to the heap
 */
    void (*destroy)(const SimpleFS *fs);

/*
 * initializes the SimpleFS
 *
 * upon return, the file system is empty
 */
    void (*init)(const SimpleFS *fs);

/*
 * create a new filename in the file system
 *
 * returns true if successful, false if unsuccesful (name already exists,
 * name is longer than 5 characters, no more free elements in the master
 * directory)
 */
    bool (*create)(const SimpleFS *fs, char *name);

/*
 * removes file from the file system
 *
 * returns true if successful, false if unsuccessful (name not in file system)
 */
    bool (*remove)(const SimpleFS *fs, char *name);

/*
 * writes `content' into `name'; if `name' previously had data, that data is
 * overwritten; 
 *
 * return true if successful, false if not (name not in file system, or content
 * has more than 2048 characters)
 */
    bool (*write)(const SimpleFS *fs, char *name, char *content);

/*
 * returns the contents of `name' in `contenta; if `name' exists but has no
 * content, content[0] == '\0'
 *
 * returns true if successful, false if not (name not in file system);
 */
    bool (*read)(const SimpleFS *fs, char *name, char *content);

/*
 * returns the list of file names in `filenames', with a comma between each
 * pair of file names
 */
    bool (*list)(const SimpleFS *fs, char *filenames);

/*
 * returns information about the file system in `information'
 */
    bool (*info)(const SimpleFS *fs, char *information);

/*
 * returns information about file headers and the directory in `dumpinfo'
 */
    bool (*dump)(const SimpleFS *fs, char *dumpinfo);

/*
 * returns the contents of the `number'-th block in `blockinfo'
 */
    bool (*block)(const SimpleFS *fs, int block, char *blockinfo);
};

#endif /* _SIMPLEFS_H_ */
