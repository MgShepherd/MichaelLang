#include "llvm.h"
#include "dynamic_array.h"
#include "lexer.h"
#include "parser.h"
#include "llvm-c/Types.h"

#include <assert.h>
#include <limits.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "main"
#define INT_BASE 10
#define VARIBLE_LEN_ESTIMATE 20
#define ARRAY_REALLOC_FACTOR 2

typedef struct {
  char *name;
  LLVMValueRef ptr;
  bool variable;
} ValueRef;

typedef struct {
  ValueRef *elements;
  size_t count;
  size_t capacity;
} ValueRefs;

typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  ValueRefs values;
} IRState;

unsigned char init_ir_state(IRState *state);

unsigned char build_function(IRState *state, const Function *func);
LLVMTypeRef get_type(const IRState *state, DataType d_type);

unsigned char build_statement(IRState *state, const Statement *statement);
// Declaration statement can only error due to failing to add ValueRef into array
unsigned char build_declaration_statement(IRState *state, const DeclarationStatement *dec);
void build_assignment_statement(IRState *state, const AssignmentStatement *assign);
void build_return_statement(const IRState *state, const ReturnStatement *ret);

/*
 * build_expression will build the required statements in order to get a single output
 * value which can be used in statements
 * Will produce output value as return value
 * Functions should never error assuming that all parser checks worked successfully
 */
LLVMValueRef build_expression(const IRState *state, const Expression *expr);
LLVMValueRef build_terminal_expr(const IRState *state, const TerminalExpr *term);
LLVMValueRef build_arithmetic_expr(const IRState *state, const ArithmeticExpr *arith);

unsigned char generate_object_file(const IRState *state, const char *file_name);

void dispose_ir_state(IRState *state);

LLVMValueRef load_identifier(const ValueRefs *values, const char *identifier);

unsigned char program_to_object_file(const Program *program, const char *file_name) {
  assert(program != NULL);

  IRState state;
  if (init_ir_state(&state) != 0) {
    fprintf(stderr, "Failed to initialise IR state\n");
    return 1;
  }

  for (size_t i = 0; i < program->functions.count; i++) {
    if (build_function(&state, &program->functions.elements[i]) != 0) {
      dispose_ir_state(&state);
      return 1;
    }
  }

  char *message;
  if (LLVMVerifyModule(state.module, LLVMReturnStatusAction, &message) != 0) {
    fprintf(stderr, "Failed to build function due to: %s\n", message);
    LLVMDisposeMessage(message);
    dispose_ir_state(&state);
    return 1;
  }

  if (generate_object_file(&state, file_name) != 0) {
    fprintf(stderr, "Failed to generated object file from LLVM IR\n");
    dispose_ir_state(&state);
    return 1;
  }

  dispose_ir_state(&state);
  return 0;
}

unsigned char init_ir_state(IRState *state) {
  dyn_array_init(&state->values, sizeof(ValueRef), VARIBLE_LEN_ESTIMATE);

  // TODO: This creation logic will need updating when supporting multiple source files
  state->context = LLVMContextCreate();
  state->module = LLVMModuleCreateWithNameInContext(MODULE_NAME, state->context);
  state->builder = LLVMCreateBuilderInContext(state->context);

  return 0;
}

unsigned char build_function(IRState *state, const Function *func) {
  LLVMTypeRef return_type = get_type(state, func->return_type);
  // TODO: Will need updating when we support function parameters
  LLVMTypeRef func_type = LLVMFunctionType(return_type, NULL, 0, false);
  LLVMValueRef llvm_func = LLVMAddFunction(state->module, func->name, func_type);

  LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(state->context, llvm_func, func->name);
  LLVMPositionBuilderAtEnd(state->builder, block);

  for (size_t i = 0; i < func->statements.count; i++) {
    if (build_statement(state, &func->statements.elements[i]) != 0) {
      fprintf(stderr, "Failed to build statement\n");
      return 1;
    }
  }

  if (LLVMVerifyFunction(llvm_func, LLVMPrintMessageAction) != 0) {
    fprintf(stderr, "Failed to build function\n");
    return 1;
  }
  return 0;
}

// TODO: Question: We are calling get_type a lot which may be inefficient, can we maybe use some kind of type map
LLVMTypeRef get_type(const IRState *state, DataType d_type) {
  assert(state != NULL);

  if (d_type == D_I32) {
    return LLVMInt32TypeInContext(state->context);
  }

  abort();
  unreachable();
}

unsigned char build_statement(IRState *state, const Statement *statement) {
  assert(state != NULL && statement != NULL);
  switch (statement->s_type) {
  case S_RETURN:
    build_return_statement(state, &statement->s_union.ret);
    break;
  case S_DECLARATION:
    if (build_declaration_statement(state, &statement->s_union.dec) != 0) {
      return 1;
    }
    break;
  case S_ASSIGNMENT:
    build_assignment_statement(state, &statement->s_union.assign);
    break;
  default:
    abort();
    unreachable();
  }

  return 0;
}

void build_return_statement(const IRState *state, const ReturnStatement *ret) {
  assert(ret != NULL);

  LLVMValueRef expr_output = build_expression(state, &ret->expr);
  assert(expr_output != NULL);
  LLVMBuildRet(state->builder, expr_output);
}

unsigned char build_declaration_statement(IRState *state, const DeclarationStatement *dec) {
  assert(dec != NULL);

  // TODO: Type is currently hardcoded to int32 - should be worked out based on the declaration statement
  LLVMTypeRef var_type = LLVMInt32TypeInContext(state->context);
  LLVMValueRef var_ptr = LLVMBuildAlloca(state->builder, var_type, dec->identifier->name);

  LLVMValueRef expr_output = build_expression(state, &dec->expr);
  assert(expr_output != NULL);
  LLVMBuildStore(state->builder, expr_output, var_ptr);

  ValueRef var = {
      .name = dec->identifier->name,
      .ptr = var_ptr,
      .variable = dec->identifier->variable,
  };
  dyn_array_insert(&state->values, var);

  return 0;
}

void build_assignment_statement(IRState *state, const AssignmentStatement *assign) {
  assert(assign != NULL);

  LLVMValueRef expr_output = build_expression(state, &assign->expr);
  assert(expr_output != NULL);
  LLVMValueRef assign_var = load_identifier(&state->values, assign->identifier->name);
  assert(assign_var != NULL);

  LLVMBuildStore(state->builder, expr_output, assign_var);
}

LLVMValueRef build_expression(const IRState *state, const Expression *expr) {
  switch (expr->e_type) {
  case E_TERM:
    return build_terminal_expr(state, &expr->e_union.terminal);
  case E_ARITHMETIC:
    return build_arithmetic_expr(state, &expr->e_union.arithmetic);
  default:
    abort();
    unreachable();
  }
}

LLVMValueRef build_terminal_expr(const IRState *state, const TerminalExpr *term) {
  assert(term != NULL);

  // TODO: Currently we only support integers as numeric literals, we should support floats etc in future
  LLVMValueRef processed;
  switch (term->tok->t_type) {
  case T_NUMERIC_LIT:
    // TODO: Need to work out the literal type dynamically, rather than hardcoding to int
    LLVMTypeRef lit_type = LLVMInt32TypeInContext(state->context);

    // TODO: Need to handle the case of 0 being returned from strtoll with errno set - this happens for invalid
    // conversion - This check should be moved to parser as part of type checking
    long long int_val = strtoll(term->tok->item, NULL, INT_BASE);
    if (int_val == LLONG_MIN || int_val == LLONG_MAX) {
      fprintf(stderr, "Failed to convert return value into integer: %s\n", term->tok->item);
      return NULL;
    }

    processed = LLVMConstInt(lit_type, int_val, false);
    break;
  case T_IDENTIFIER:
    const LLVMTypeRef var_type = LLVMInt32TypeInContext(state->context);
    const LLVMValueRef value_ref = load_identifier(&state->values, term->tok->item);
    assert(value_ref != NULL);

    processed = LLVMBuildLoad2(state->builder, var_type, value_ref, term->tok->item);
    break;
  default:
    abort();
    unreachable();
  }

  if (term->sign == SIGN_NEGATIVE) {
    return LLVMBuildNeg(state->builder, processed, term->tok->item);
  }

  return processed;
}

LLVMValueRef build_arithmetic_expr(const IRState *state, const ArithmeticExpr *arith) {
  assert(arith != NULL && arith->op->t_type == T_ARITHMETIC);

  LLVMValueRef lhs = build_terminal_expr(state, &arith->lhs);
  assert(lhs != NULL);
  LLVMValueRef rhs = build_expression(state, arith->rhs);
  assert(rhs != NULL);

  switch (arith->op->item[0]) {
  case '+':
    return LLVMBuildAdd(state->builder, lhs, rhs, arith->lhs.tok->item);
  case '-':
    return LLVMBuildSub(state->builder, lhs, rhs, arith->lhs.tok->item);
  default:
    abort();
    unreachable();
  }
}

unsigned char generate_object_file(const IRState *state, const char *file_name) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();
  LLVMInitializeNativeAsmPrinter();

  char *target_triple = LLVMGetDefaultTargetTriple();

  LLVMTargetRef target;
  char *error_message;
  if (LLVMGetTargetFromTriple(target_triple, &target, &error_message) != 0) {
    fprintf(stderr, "Failed to get target machine, error: %s\n", error_message);
    LLVMDisposeMessage(error_message);
    LLVMDisposeMessage(target_triple);
    return 1;
  }

  LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
      target, target_triple, "generic", "", LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
  LLVMTargetDataRef target_data = LLVMCreateTargetDataLayout(target_machine);
  LLVMSetModuleDataLayout(state->module, target_data);

  unsigned char status = 0;
  if (LLVMTargetMachineEmitToFile(target_machine, state->module, file_name, LLVMObjectFile, &error_message) != 0) {
    fprintf(stderr, "Failed to generate object file for machine, error: %s\n", error_message);
    LLVMDisposeMessage(error_message);
    status = 1;
  }

  LLVMDisposeMessage(target_triple);
  LLVMDisposeTargetData(target_data);
  LLVMDisposeTargetMachine(target_machine);
  return status;
}

void dispose_ir_state(IRState *state) {
  dyn_array_free(&state->values);

  LLVMDisposeBuilder(state->builder);
  LLVMDisposeModule(state->module);
  LLVMContextDispose(state->context);
}

LLVMValueRef load_identifier(const ValueRefs *values, const char *identifier) {
  for (size_t i = 0; i < values->count; i++) {
    if (strcmp(values->elements[i].name, identifier) == 0) {
      return values->elements[i].ptr;
    }
  }

  return NULL;
}
