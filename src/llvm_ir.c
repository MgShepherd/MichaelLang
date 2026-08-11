#include "llvm_ir.h"
#include "lexer.h"
#include "parser.h"
#include "llvm-c/Types.h"

#include <assert.h>
#include <limits.h>
#include <llvm-c/Core.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "main"
#define INT_BASE 10

typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} IRState;

void build_function(const IRState *state, const FunctionStatement *func);
LLVMTypeRef get_type(const IRState *state, const Token *token);

void build_statement(const IRState *state, const Statement *statement);
void build_return_statement(const IRState *state, const ReturnStatement *ret);

unsigned char llvm_ir_from_program(const Program *program) {
  assert(program != NULL);

  // TODO: This creation logic will need updating when supporting multiple source files
  IRState state;
  state.context = LLVMContextCreate();
  state.module = LLVMModuleCreateWithNameInContext(MODULE_NAME, state.context);
  state.builder = LLVMCreateBuilderInContext(state.context);

  // TODO: For now we are hardcoding a main function which doesn't actually exist
  // This should be updated to actually look for the main function in the program and insert instructions there
  // Requires some extra parsing checks to stop top level executable instructions for now
  const Token returnTypeToken = {.t_type = T_KEYWORD, .item = "i32"};
  const Token identifierToken = {.t_type = T_IDENTIFIER, .item = "main"};
  FunctionStatement func = {
      .identifier = &identifierToken,
      .return_type = &returnTypeToken,
      .statement_arr = program->statement_arr,
  };
  build_function(&state, &func);

  char *module_info = LLVMPrintModuleToString(state.module);
  printf("LLVM Module: %s\n", module_info);
  LLVMDisposeMessage(module_info);

  LLVMDisposeModule(state.module);
  LLVMContextDispose(state.context);
  return 0;
}

void build_function(const IRState *state, const FunctionStatement *func) {
  LLVMTypeRef return_type = get_type(state, func->return_type);
  // TODO: Will need updating when we support function parameters
  LLVMTypeRef func_type = LLVMFunctionType(return_type, NULL, 0, false);
  LLVMValueRef llvm_func = LLVMAddFunction(state->module, func->identifier->item, func_type);

  LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(state->context, llvm_func, func->identifier->item);
  LLVMPositionBuilderAtEnd(state->builder, block);

  for (size_t i = 0; i < func->statement_arr.count; i++) {
    build_statement(state, &func->statement_arr.statements[i]);
  }
}

// TODO: Question: We are calling get_type a lot which may be inefficient, can we maybe use some kind of type map
LLVMTypeRef get_type(const IRState *state, const Token *token) {
  assert(state != NULL && token != NULL && token->t_type == T_KEYWORD);

  if (strcmp(token->item, "i32") == 0) {
    return LLVMInt32TypeInContext(state->context);
  }

  fprintf(stderr, "Attempted to get LLVM type from token that is not valid type: %s, this should be unreachable\n",
          token->item);
  unreachable();
}

void build_statement(const IRState *state, const Statement *statement) {
  assert(state != NULL && statement != NULL);
  switch (statement->s_type) {
  case S_RETURN:
    build_return_statement(state, &statement->s_union.ret);
    break;
  default:
    fprintf(stderr, "Unimplemented statement type in ir: %s\n", s_type_to_string(statement->s_type));
  }
}

// TODO: Add support for identifiers as the return value
void build_return_statement(const IRState *state, const ReturnStatement *ret) {
  // TODO: Currently we only support integers as numeric literals, we should support floats etc in future
  switch (ret->expr->t_type) {
  case T_NUMERIC_LIT:
    // Need to work out the return statement type by pulling from the function definition - need to check they match
    LLVMTypeRef return_type = LLVMInt32TypeInContext(state->context);

    long long int_val = strtoll(ret->expr->item, NULL, INT_BASE);
    if (int_val == LONG_MIN || int_val == LONG_MAX) {
      fprintf(stderr, "Failed to convert return value into integer: %s\n", ret->expr->item);
      return;
    }

    // TODO: Check what happens for negative return values, as think this may have an issue
    LLVMValueRef ret_val = LLVMConstInt(return_type, int_val, false);
    LLVMBuildRet(state->builder, ret_val);
    break;
  default:
    fprintf(stderr, "Unexpected expression type for return statement: %s\n", t_type_to_string(ret->expr->t_type));
  }
}
