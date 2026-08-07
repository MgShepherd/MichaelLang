#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *path) {
  FILE *f_ptr = fopen(path, "r");
  if (f_ptr == NULL) {
    fprintf(stderr, "Failed to read file at path: %s\n", path);
    return NULL;
  }

  if (fseek(f_ptr, 0, SEEK_END) != 0) {
    fprintf(stderr, "Failed to jump to end of file in order to get size\n");
    fclose(f_ptr);
    return NULL;
  }

  const long file_size = ftell(f_ptr);
  if (file_size <= 0) {
    fprintf(stderr, "Failed to get the size of the input file\n");
    fclose(f_ptr);
    return NULL;
  }

  if (fseek(f_ptr, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to jump back to start of file after reading size\n");
    fclose(f_ptr);
    return NULL;
  }

  char *data = malloc(file_size + 1);
  if (data == NULL) {
    fprintf(stderr, "Failed to allocate required memory for data buffer\n");
    fclose(f_ptr);
    return NULL;
  }

  size_t read_bytes = fread(data, 1, file_size, f_ptr);
  if (read_bytes != (size_t)file_size) {
    fprintf(stderr, "Failed to copy file data to output buffer, read bytes %zu, but expected %zu\n", read_bytes,
            file_size);
    free(data);
    fclose(f_ptr);
    return NULL;
  }

  data[file_size] = '\0';

  fclose(f_ptr);
  return data;
}

char *string_slice(const char *input, size_t slice_start, size_t slice_end) {
  assert(input != NULL);
  assert(slice_start < slice_end);

  char *output = malloc(slice_end - slice_start + 1);
  if (output == NULL) {
    fprintf(stderr, "Failed to allocate required space for string slice");
    return NULL;
  }

  for (size_t i = slice_start; i < slice_end; i++) {
    output[i - slice_start] = input[i];
  }

  output[slice_end - slice_start] = '\0';
  return output;
}
