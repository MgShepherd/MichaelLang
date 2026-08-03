#ifndef _UTILS_H_
#define _UTILS_H_

/*
 * Loads all the content of a file at the specified path into a char array
 * Allocates memory on the heap which will need to be freed
 * Returns NULL if there is an error reading the file
 */
char *read_file(const char *path);

#endif // _UTILS_H_
