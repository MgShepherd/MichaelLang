#include "llvm_ir.h"

#include <assert.h>
#include <llvm-c/Core.h>
#include <stdio.h>

unsigned char llvm_ir_from_program(const Program *program) {
  assert(program != NULL);
  // Test code to ensure that LLVM has been included and linked correctly
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("MyModule", context);

  char *module_info = LLVMPrintModuleToString(module);
  printf("LLVM Module: %s\n", module_info);
  LLVMDisposeMessage(module_info);

  LLVMDisposeModule(module);
  LLVMContextDispose(context);
  return 0;
}
