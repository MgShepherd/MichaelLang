#include "llvm.h"
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
} Variable;

// TODO: Aware that using a dynamic array for storing variables is not ideal and hashmap should be used, but for not
// that is not implemented
typedef struct {
  Variable *variables;
  size_t count;
  size_t capacity;
} VariableArray;

typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  VariableArray variable_arr;
} IRState;

unsigned char init_ir_state(IRState *state);

unsigned char build_function(IRState *state, const Function *func);
LLVMTypeRef get_type(const IRState *state, const Token *token);

unsigned char build_statement(IRState *state, const Statement *statement);
unsigned char build_return_statement(const IRState *state, const ReturnStatement *ret);
unsigned char build_declaration_statement(IRState *state, const DeclarationStatement *dec);

unsigned char generate_object_file(const IRState *state, const char *file_name);

void dispose_ir_state(const IRState *state);

// Returns NULL in case of variable being undefined
LLVMValueRef *load_variable(const VariableArray *variable_arr, const char *identifier);
unsigned char insert_variable(VariableArray *variable_arr, Variable *variable);

unsigned char program_to_object_file(const Program *program, const char *file_name) {
  assert(program != NULL);

  IRState state;
  if (init_ir_state(&state) != 0) {
    fprintf(stderr, "Failed to initialise IR state\n");
    return 1;
  }

  for (size_t i = 0; i < program->function_arr.count; i++) {
    if (build_function(&state, &program->function_arr.functions[i]) != 0) {
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
  Variable *variables = malloc(VARIBLE_LEN_ESTIMATE * sizeof(Variable));
  if (variables == NULL) {
    fprintf(stderr, "Failed to allocate enough memory for storing variable map\n");
    return 1;
  }
  state->variable_arr.variables = variables;
  state->variable_arr.count = 0;
  state->variable_arr.capacity = VARIBLE_LEN_ESTIMATE;

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
  LLVMValueRef llvm_func = LLVMAddFunction(state->module, func->identifier->item, func_type);

  LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(state->context, llvm_func, func->identifier->item);
  LLVMPositionBuilderAtEnd(state->builder, block);

  for (size_t i = 0; i < func->statement_arr.count; i++) {
    if (build_statement(state, &func->statement_arr.statements[i]) != 0) {
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
LLVMTypeRef get_type(const IRState *state, const Token *token) {
  assert(state != NULL && token != NULL && token->t_type == T_KEYWORD);

  if (strcmp(token->item, "i32") == 0) {
    return LLVMInt32TypeInContext(state->context);
  }

  fprintf(stderr, "Attempted to get LLVM type from token that is not valid type: %s, this should be unreachable\n",
          token->item);
  unreachable();
}

unsigned char build_statement(IRState *state, const Statement *statement) {
  assert(state != NULL && statement != NULL);
  switch (statement->s_type) {
  case S_RETURN:
    if (build_return_statement(state, &statement->s_union.ret) != 0) {
      fprintf(stderr, "Failed to build return statement\n");
      return 1;
    }
    break;
  case S_DECLARATION:
    if (build_declaration_statement(state, &statement->s_union.dec) != 0) {
      fprintf(stderr, "Failed to build declaration statement\n");
      return 1;
    }
    break;
  default:
    fprintf(stderr, "Unimplemented statement type in ir: %s\n", s_type_to_string(statement->s_type));
    return 1;
  }

  return 0;
}

// TODO: Add support for identifiers as the return value
unsigned char build_return_statement(const IRState *state, const ReturnStatement *ret) {
  assert(ret != NULL);

  // TODO: Currently we only support integers as numeric literals, we should support floats etc in future
  switch (ret->expr->t_type) {
  case T_NUMERIC_LIT:
    // TODO: Need to work out the return statement type by pulling from the function definition
    LLVMTypeRef return_type = LLVMInt32TypeInContext(state->context);

    long long int_val = strtoll(ret->expr->item, NULL, INT_BASE);
    if (int_val == LONG_MIN || int_val == LONG_MAX) {
      fprintf(stderr, "Failed to convert return value into integer: %s\n", ret->expr->item);
      return 1;
    }

    // TODO: Check what happens for negative return values, as think this may have an issue
    LLVMValueRef ret_val = LLVMConstInt(return_type, int_val, false);
    LLVMBuildRet(state->builder, ret_val);
    break;
  case T_IDENTIFIER:
    const LLVMTypeRef var_type = LLVMInt32TypeInContext(state->context);
    const LLVMValueRef *var_ptr = load_variable(&state->variable_arr, ret->expr->item);
    if (var_ptr == NULL) {
      fprintf(stderr, "Undefined variable: %s\n", ret->expr->item);
      return 1;
    }

    LLVMValueRef ret_var = LLVMBuildLoad2(state->builder, var_type, *var_ptr, ret->expr->item);
    LLVMBuildRet(state->builder, ret_var);
    break;
  default:
    fprintf(stderr, "Unexpected expression type for return statement: %s\n", t_type_to_string(ret->expr->t_type));
    return 1;
  }
  return 0;
}

unsigned char build_declaration_statement(IRState *state, const DeclarationStatement *dec) {
  assert(dec != NULL);

  // TODO: Type is currently hardcoded to int32 - should be worked out based on the declaration statement
  LLVMTypeRef var_type = LLVMInt32TypeInContext(state->context);
  LLVMValueRef var_ptr = LLVMBuildAlloca(state->builder, var_type, dec->identifier->item);

  long long int_val = strtoll(dec->expr->item, NULL, INT_BASE);
  if (int_val == LONG_MIN || int_val == LONG_MAX) {
    fprintf(stderr, "Failed to convert declaration value into integer: %s\n", dec->expr->item);
    return 1;
  }

  // TODO: Currently only supporting declaration statements with single constant values - should properly support
  // expressions
  LLVMValueRef dec_val = LLVMConstInt(var_type, int_val, false);
  LLVMBuildStore(state->builder, dec_val, var_ptr);

  Variable var = {.name = dec->identifier->item, .ptr = var_ptr};
  if (insert_variable(&state->variable_arr, &var) != 0) {
    fprintf(stderr, "Failed to insert variable %s into variable array\n", dec->identifier->item);
    return 1;
  }

  return 0;
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

void dispose_ir_state(const IRState *state) {
  if (state->variable_arr.variables != NULL) {
    free(state->variable_arr.variables);
  }

  LLVMDisposeBuilder(state->builder);
  LLVMDisposeModule(state->module);
  LLVMContextDispose(state->context);
}

LLVMValueRef *load_variable(const VariableArray *variable_arr, const char *identifier) {
  for (size_t i = 0; i < variable_arr->count; i++) {
    if (strcmp(variable_arr->variables[i].name, identifier) == 0) {
      return &variable_arr->variables[i].ptr;
    }
  }

  return NULL;
}

unsigned char insert_variable(VariableArray *variable_arr, Variable *variable) {
  assert(variable_arr->variables != NULL);

  if (variable_arr->count >= variable_arr->capacity) {
    variable_arr->capacity = variable_arr->capacity * ARRAY_REALLOC_FACTOR;
    Variable *new_variables = realloc(variable_arr->variables, variable_arr->capacity * sizeof(Variable));
    if (new_variables == NULL) {
      fprintf(stderr, "Failed to allocate additional required space for variables array\n");
      return 1;
    }
    variable_arr->variables = new_variables;
  }

  variable_arr->variables[variable_arr->count++] = *variable;

  return 0;
}
