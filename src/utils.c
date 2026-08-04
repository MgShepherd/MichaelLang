#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

// TODO: Look into whether we should use errno values from here to make better error messages

unsigned char read_file(FileData *file_data, const char *path) {
  FILE *f_ptr = fopen(path, "r");
  if (f_ptr == NULL) {
    fprintf(stderr, "Failed to read file at path: %s\n", path);
    return 1;
  }

  if (fseek(f_ptr, 0, SEEK_END) != 0) {
    fprintf(stderr, "Failed to jump to end of file in order to get size\n");
    fclose(f_ptr);
    return 1;
  }

  const long file_size = ftell(f_ptr);
  if (file_size <= 0) {
    fprintf(stderr, "Failed to get the size of the input file\n");
    fclose(f_ptr);
    return 1;
  }

  if (fseek(f_ptr, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to jump back to start of file after reading size\n");
    fclose(f_ptr);
    return 1;
  }

  // Allocate extra character to account for null byte
  file_data->count = file_size + 1;
  file_data->data = malloc(file_size + 1);
  if (file_data->data == NULL) {
    fprintf(stderr, "Failed to allocate required memory for data buffer\n");
    fclose(f_ptr);
    return 1;
  }

  size_t read_bytes = fread(file_data->data, 1, file_size, f_ptr);
  if (read_bytes != (size_t)file_size) {
    fprintf(stderr, "Failed to copy file data to output buffer, read bytes %zu, but expected %zu\n", read_bytes,
            file_size);
    file_data_free(file_data);
    fclose(f_ptr);
    return 1;
  }

  file_data->data[file_size] = '\0';

  fclose(f_ptr);
  return 0;
}

void file_data_free(FileData *file_data) {
  if (file_data->data != NULL) {
    free(file_data->data);
  }
  file_data->data = NULL;
  file_data->count = 0;
}
