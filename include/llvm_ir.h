#ifndef _LLVM_IR_H_
#define _LLVM_IR_H_

#include "parser.h"

/*
 * llvm_ir_from_program takes a built program ASt and will generate the LLVM code for this
 * Currently just outputs the generate code to stdout, but will eventually be to an obj file
 * All LLVM state is cleaned up as part of this function, no additional cleanup neccessary
 * Returns 0 on success, 1 on error
 */
unsigned char llvm_ir_from_program(const Program *program);

#endif // _LLVM_IR_H_
