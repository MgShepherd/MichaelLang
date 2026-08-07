#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>

typedef struct {
  char *data;
  size_t count;
} FileData;

/*
 * Loads all the content of a file and return as a null terminated string
 * Allocates memory on the heap which will need to be freed
 * Returns NULL on failure
 */
char *read_file(const char *path);

/*
 * Returns a slice from within an existing string - slice_start inclusive, slice_end exclusive
 * Will ensure the slice is null terminated
 * Will allocate the string slice on the heap so this must be freed once finished with
 * Does not perform bound checks on the length of the string, so ensure slice_start and slice_end are within string
 * bounds - always assumes start is less than end
 * Will return NULL if unable to create the string slice
 */
char *string_slice(const char *input, size_t slice_start, size_t slice_end);

#endif // _UTILS_H_
