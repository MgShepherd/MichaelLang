#ifndef _LLVM_H_
#define _LLVM_H_

#include "parser.h"

/*
 * program_to_object_file takes a built program AST and will generate a native obj file via LLVM
 * Generated obj will be output to provided file name
 * All LLVM state is cleaned up as part of this function, no additional cleanup neccessary
 * Returns 0 on success, 1 on error
 */
unsigned char program_to_object_file(const Program *program, const char *file_name);

#endif // _LLVM_H_
