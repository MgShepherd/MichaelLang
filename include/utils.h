#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>

typedef struct {
  char *data;
  size_t count;
} FileData;

/*
 * Loads all the content of a file at the specified path into file_data parameter
 * Assumes that the file_data parameter is empty - will overwrite any existing data if not
 * Allocates memory on the heap which will need to be freed - use file_data_free for this
 * Returns 0 on success, 1 on failure
 */
unsigned char read_file(FileData *file_data, const char *path);

/*
 * Frees the memory associated with the file_data object
 */
void file_data_free(FileData *file_data);

#endif // _UTILS_H_
