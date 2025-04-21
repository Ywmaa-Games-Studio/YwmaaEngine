/* YSCRIPT_INTERPRETER.h
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.21 -04:28
 * Last edited:
 *   <l1833PMle>
 * Auto updated?
 *   Yes
 *
 * Description:
 *   YScript Interpreter Implementation Header
**/

#pragma once

#include "yscript.h"
#include "data_structures/rope.h"
#include "data_structures/hashtable.h"
#include "math/math_types.h"

struct yscript_context;
typedef struct yscript_type {
    char* type_name;
    b8 is_pointer;
    u32 size;

    // maximum members of 255, also if set to 0 then it is not a struct
    u8 members_count;
    // variable member >= 8KB not supported
    u16* members_strides;
    // we use it to get the index of the member using its name
    // so this maps like this name => index
    HASHTABLE* member_names_hash;

    // operators for this type
    void* (*plus)(void* left_value, void* right_value);
    void* (*minus)(void* left_value, void* right_value);
    void* (*multiply)(void* left_value, void* right_value);
    void* (*divide)(void* left_value, void* right_value);
    
    // casting to native types if available
    i32 (*cast_to_int32)(void* value);
    f32 (*cast_to_float32)(void* value);
    i64 (*cast_to_int64)(void* value);
    f64 (*cast_to_float64)(void* value);
    RopeNode* (*cast_to_string)(void* value);
    b8 (*cast_to_bool)(void* value);
    Vector2 (*cast_to_vector2)(void* value);
    Vector3 (*cast_to_vector3)(void* value);
    Vector4 (*cast_to_vector4)(void* value);

    b8 (*cast_string)(char* str, void* out_value);

    void* (*allocation)(void);
    void (*free)(void* internal_data);
} yscript_type;

// Reference counted value structure
typedef struct yscript_value {
    // it will be a pointer to one of the registered types
    yscript_type* type;
    // pointer to the internally allocated data
    void* internal_data;
    // reference count
    u32 ref_count;
} yscript_value;

// Variable/Constant definition
typedef struct yscript_variable {
    char* name;
    // it will be a pointer to one of the registered types
    yscript_type* type;
    yscript_value* value;
    b8 is_const;
} yscript_variable;

// Array
typedef struct yscript_array {
    // it will be a pointer to one of the registered types
    u32 length;
    yscript_value** values;
} yscript_array;

// Function definition
typedef struct yscript_function {
    char* name;
    char* body;
    yscript_type* return_type;
    char** parameter_names;
    yscript_type** parameter_types;
    u16 parameter_count; // limited to 255 parameters
} yscript_function;
// Script context containing all state
typedef struct yscript_context {
    yscript_variable** variables;
    u32 variable_count;
    yscript_function** functions;
    u32 function_count;
    struct yscript_context* parent; // For nested scopes
    yscript_value* return_value;    // For handling return statements
} yscript_context;

// Native function callback type
typedef struct yscript_value* (*yscript_native_fn)(struct yscript_context* context, struct yscript_value** args, u32 arg_count);

// Native function definition
typedef struct yscript_native_function {
    char* name;
    yscript_native_fn function;
    u32 parameter_count;
    yscript_type** parameter_types;
    yscript_type* return_type;
} yscript_native_function;

// Token types for lexing
typedef enum yscript_token_type {
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_IDENTIFIER,
    TOKEN_FUNC_CALL,
    TOKEN_VAR,
    TOKEN_CONST,
    TOKEN_FUNC,
    TOKEN_STRUCT,
    TOKEN_ENUM,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CUSTOM_TYPE_NAME,
    TOKEN_KEYWORD,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_ASTERISK,
    TOKEN_EQUAL,
    TOKEN_GREATER_THAN,
    TOKEN_LESS_THAN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_ARROW,
    TOKEN_COMMENT,
    TOKEN_RETURN
} yscript_token_type;

// Token type string array for debugging
static const char* token_type_strings[] = {
    "TOKEN_EOF",
    "TOKEN_ERROR",
    "TOKEN_IDENTIFIER",
    "TOKEN_FUNC_CALL",
    "TOKEN_VAR",
    "TOKEN_CONST",
    "TOKEN_FUNC",
    "TOKEN_STRUCT",
    "TOKEN_ENUM",
    "TOKEN_INT",
    "TOKEN_FLOAT",
    "TOKEN_STRING",
    "TOKEN_CUSTOM_TYPE_NAME",
    "TOKEN_KEYWORD",
    "TOKEN_ASSIGN",
    "TOKEN_PLUS",
    "TOKEN_MINUS",
    "TOKEN_SLASH",
    "TOKEN_ASTERISK",
    "TOKEN_EQUAL",
    "TOKEN_GREATER_THAN",
    "TOKEN_LESS_THAN",
    "TOKEN_TRUE",
    "TOKEN_FALSE",
    "TOKEN_IF",
    "TOKEN_ELSE",
    "TOKEN_SEMICOLON",
    "TOKEN_COLON",
    "TOKEN_COMMA",
    "TOKEN_LEFT_PAREN",
    "TOKEN_RIGHT_PAREN",
    "TOKEN_LEFT_BRACE",
    "TOKEN_RIGHT_BRACE",
    "TOKEN_LEFT_BRACKET",
    "TOKEN_RIGHT_BRACKET",
    "TOKEN_ARROW",
    "TOKEN_COMMENT",
    "TOKEN_RETURN"
};

// Token structure
typedef struct yscript_token {
    yscript_token_type type;
    char* start;
    u32 length;
    u32 line;
} yscript_token;

// Lexer state
typedef struct yscript_lexer {
    char* source;
    char* current;
    u32 line;
} yscript_lexer;

// Parser state
typedef struct yscript_parser {
    yscript_lexer lexer;
    yscript_token current;
    yscript_token previous;
    b8 had_error;
} yscript_parser;

void yscript_clear_types(void);

static yscript_type* yscript_lookup_type(const char* name, u32 name_length);

// Initialize a new value with reference counting
static yscript_value* value_create(yscript_type* type);

// Increment reference count
static void value_retain(yscript_value* value);

// Decrement reference count and free if zero
static void value_release(yscript_value* value);

// Initialize lexer
static void lexer_init(yscript_lexer* lexer, char* source);

// Get next token
static yscript_token lexer_next_token(yscript_lexer* lexer);

// Initialize parser
static void parser_init(yscript_parser* parser, char* source);

// Advance to next token
static void parser_advance(yscript_parser* parser);

// Parse a value
static yscript_value* parse_value(yscript_parser* parser, yscript_context* context, int precedence);

// Parse a variable declaration
static void parse_variable_declaration(yscript_parser* parser, yscript_context* context);

// Parse a function declaration
static void parse_function_declaration(yscript_parser* parser, yscript_context* context);

static yscript_value* evaluate_function_call(yscript_parser* parser, yscript_context* context, const char* func_name);

// Execute YScript code
b8 yscript_execute_string(yscript_context* context, char* code);

// Create a new script context
yscript_context* yscript_context_create(void);

// Destroy a script context
void yscript_context_destroy(yscript_context* context);

// Register a native function
void yscript_register_native_function(yscript_context* context, const char* name, yscript_native_fn function, 
    u32 parameter_count, yscript_type** parameter_types, yscript_type* return_type);

// Register built-in native functions (like logger)
void yscript_register_builtin_functions(yscript_context* context); 
