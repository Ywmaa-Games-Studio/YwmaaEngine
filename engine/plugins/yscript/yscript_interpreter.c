/* YSCRIPT_INTERPRETER.c
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.21 -04:28
 * Last edited:
 *   <l3152AMle>
 * Auto updated?
 *   Yes
 *
 * Description:
 *   YScript Interpreter Implementation
**/

#include "yscript_interpreter.h"
#include "yscript_native_logger_hooks.h"
#include "yscript_native_types.h"
#include "core/logger.h"
#include "core/ystring.h"
#include "core/ymemory.h"

#include <ctype.h> // isdigit, isalpha

#define DEBUG_YSCRIPT
#undef DEBUG_YSCRIPT

static yscript_type** registered_types;
static u32 types_count = 0;

// Add helper function for variable lookup
static yscript_variable* yscript_lookup_variable(yscript_context* context, const char* name, u32 name_length) {
    yscript_context* ctx = context;
    while (ctx) {
        for (u32 i = 0; i < ctx->variable_count; i++) {
            yscript_variable* var = ctx->variables[i];
            if (strings_nequal(var->name, name, name_length) && var->name[name_length] == 0) {
                return var;
            }
        }
        ctx = ctx->parent;
    }
    return NULL;
}

// helper function for type lookup
static yscript_type* yscript_lookup_type(const char* name, u32 name_length) {
    //yscript_context* ctx = context;
    //while (ctx) {
        for (u32 i = 0; i < types_count; i++) {
            yscript_type* type = registered_types[i];
            if (strings_nequal(type->type_name, name, name_length)) {
                return type;
            }
        }
    //    ctx = ctx->parent;
    //}
    return NULL;
}

// Initialize a new value with reference counting
static yscript_value* value_create(yscript_type* type) {
    yscript_value* value = yallocate_aligned(sizeof(yscript_value), 8, MEMORY_TAG_SCRIPT);
    value->type = type;
    value->ref_count = 1;
    if (!value->type->is_pointer){
        value->internal_data = yallocate_aligned(value->type->size, 8, MEMORY_TAG_SCRIPT);
    } else {
        value->internal_data = NULL;
    }
    return value;
}

// if move is true, it clears the value after coping it if it is not a pointer
static void value_set(yscript_value* target, void* value, b8 move) {
    if (target->type->is_pointer){
        if (target->internal_data && target->type->free){
            target->type->free(target->internal_data);
        }
        target->internal_data = value;
    } else {
        ycopy_memory(target->internal_data, value, target->type->size);
        if (target->type->free && move) {
            target->type->free(value);
        }
    }
}

yscript_type* yscript_type_create(const char* name, b8 is_pointer, u32 size, u8 members_count, u16* members_strides, char** member_names,\
    void* (*plus)(void* left_value, void* right_value),\
    void* (*minus)(void* left_value, void* right_value),\
    void* (*multiply)(void* left_value, void* right_value),\
    void* (*divide)(void* left_value, void* right_value),\
    b8 (*cast_string)(char* str, void* out_value),\
    RopeNode* (*cast_to_string)(void* value),\
    void* (*allocation)(void),\
    void (*free)(void* internal_data)){

    yscript_type* type = (yscript_type*)yallocate_aligned(sizeof(yscript_type), 8, MEMORY_TAG_SCRIPT);
    type->type_name = string_duplicate(name);
    type->is_pointer = is_pointer;
    type->size = size;
    type->plus = plus;
    type->minus = minus;
    type->multiply = multiply;
    type->divide = divide;
    type->cast_string = cast_string;
    type->cast_to_string = cast_to_string;
    type->allocation = allocation;
    type->free = free;

    return type;
}

void yscript_type_destroy(yscript_type* type){
    yfree(type->type_name);
    yfree(type);
}

// Increment reference count
static void value_retain(yscript_value* value) {
    if (value) {
        value->ref_count++;
    }
}

// Decrement reference count and free if zero
static void value_release(yscript_value* value) {
    if (value) {
        value->ref_count--;
        //PRINT_ERROR("releasing ref count %i", value->ref_count)
        if (value->ref_count == 0) {
            if (value->type->free) {
                value->type->free(value->internal_data);
            }
            yfree(value);
        }
    }
}

// Initialize lexer
static void lexer_init(yscript_lexer* lexer, char* source) {
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
}

// Get next token
static yscript_token lexer_next_token(yscript_lexer* lexer) {
    yscript_token token;
    token.start = lexer->current;
    token.line = lexer->line;

    char c = *lexer->current++;
    
    switch (c) {
        case '\0':
            token.type = TOKEN_EOF;
            break;
        case ' ':
        case '\r':
        case '\t':
            // Skip whitespace
            return lexer_next_token(lexer);
        case '\n':
            lexer->line++;
            return lexer_next_token(lexer);
        case '#':
            token.type = TOKEN_COMMENT;
            token.start = lexer->current - 1; // Include the opening comment
            while (*lexer->current != '\n' && *lexer->current != '\0') {
                lexer->current++;
            }
            if (*lexer->current == '\n') lexer->line++;
            token.length = (u32)(lexer->current - token.start);
            break;
        case '(':
            token.type = TOKEN_LEFT_PAREN;
            break;
        case ')':
            token.type = TOKEN_RIGHT_PAREN;
            break;
        case '{':
            token.type = TOKEN_LEFT_BRACE;
            break;
        case '}':
            token.type = TOKEN_RIGHT_BRACE;
            break;
        case '[':
            token.type = TOKEN_LEFT_BRACKET;
            break;
        case ']':
            token.type = TOKEN_RIGHT_BRACKET;
            break;
        case ';':
            token.type = TOKEN_SEMICOLON;
            break;
        case ':':
            token.type = TOKEN_COLON;
            break;
        case ',':
            token.type = TOKEN_COMMA;
            break;
        case '<':
            token.type = TOKEN_LESS_THAN;
            break;
        case '>':
            token.type = TOKEN_GREATER_THAN;
            break;
        case '-':
            if (*lexer->current == '>') {
                lexer->current++;
                token.type = TOKEN_ARROW;
            } else {
                token.type = TOKEN_MINUS;
            }
            break;
        case '+':
            token.type = TOKEN_PLUS;
            break;
        case '*':
            token.type = TOKEN_ASTERISK;
            break;
        case '/':
            token.type = TOKEN_SLASH;
            break;
        case '=':
            if (*lexer->current == '=') {
                lexer->current++;
                token.type = TOKEN_EQUAL;
            } else {
                token.type = TOKEN_ASSIGN;
            }
            break;
        case '"':
            // String literal
            token.type = TOKEN_STRING;
            token.start = lexer->current - 1; // Include the opening quote
            while (*lexer->current != '"' && *lexer->current != '\0') {
                if (*lexer->current == '\n') lexer->line++;
                lexer->current++;
            }
            if (*lexer->current == '"') {
                lexer->current++; // Include the closing quote
            }
            token.length = (u32)(lexer->current - token.start);
            break;
        default:
            if (isalpha(c)) {
                // Identifier or keyword
                while (isalnum(*lexer->current) || *lexer->current == '_') {
                    lexer->current++;
                }
                // it is probably an identifier unless the checks below says otherwise
                token.type = TOKEN_IDENTIFIER;
                
                if (strings_nequal((lexer->current-4), "true", 4)) {
                    token.type = TOKEN_TRUE;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-5), "false", 5)) {
                    token.type = TOKEN_FALSE;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-3), "var", 3)) {
                    token.type = TOKEN_VAR;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-5), "const", 5)) {
                    token.type = TOKEN_CONST;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-4), "func", 4)) {
                    token.type = TOKEN_FUNC;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-6), "struct", 6)) {
                    token.type = TOKEN_STRUCT;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-4), "enum", 4)) {
                    token.type = TOKEN_ENUM;
                    token.start -= 1;
                    break;
                } else if (strings_nequal((lexer->current-6), "return", 6)) {
                    token.type = TOKEN_RETURN;
                    token.start -= 1;
                    break;
                } else
                {
                    // get next letter directly after the identifier
                    // this is to check if it is a function call
                    char* next_letter_pointer = lexer->current;
                    while (isspace(*next_letter_pointer)) {
                        next_letter_pointer++;
                    }

                    if (strings_nequal(next_letter_pointer, "(", 1)) {
                        char* name_start_character = lexer->current;
                        for (u8 i=0; i < lexer->current-token.start; i++){
                            name_start_character--;
                        }
                        // custom types are almost initialized like a function call (aka constructor)
                        if (yscript_lookup_type(name_start_character, lexer->current-token.start) != NULL) {
                            token.type = TOKEN_CUSTOM_TYPE_NAME;
                            //advance till the end of the type
                            while (*lexer->current != ')') {
                                lexer->current++;
                            }
                            // include the closing parenthesis
                            lexer->current++;
                            //token.start -= 1;
                        // Check if this is a function declaration by looking at previous tokens
                        // If the previous token was 'func', this is a declaration, not a call
                        } else if (token.start > lexer->source && 
                            strings_nequal(token.start - 5, "func", 4) == false) {
                            token.type = TOKEN_FUNC_CALL;
                        }
                    }
                }
                break;
            } else if (isdigit(c)) {
                token.type = TOKEN_INT;
                // Number
                while (isdigit(*lexer->current)) {
                    lexer->current++;
                }
                if (*lexer->current == '.') {
                    lexer->current++;
                    while (isdigit(*lexer->current)) {
                        lexer->current++;
                    }
                    token.type = TOKEN_FLOAT;
                }
            } else {
                token.type = TOKEN_ERROR;
            }
            break;
    }

    if (token.type != TOKEN_STRING 
    && token.type != TOKEN_COMMENT) {
        token.length = (u32)(lexer->current - token.start);
    }
    
    // Debug print the token
    char token_text[token.length + 1];
    string_ncopy(token_text, token.start, token.length);
    token_text[token.length] = '\0';
#ifdef DEBUG_YSCRIPT
    PRINT_TRACE("Lexer produced token: type=%s, text='%s'", token_type_strings[token.type], token_text);
#endif
    return token;
}

// Initialize parser
static void parser_init(yscript_parser* parser, char* source) {
    lexer_init(&parser->lexer, source);
    parser->current = lexer_next_token(&parser->lexer);
    parser->previous = parser->current;
    parser->had_error = false;
}

// Advance to next token
static void parser_advance(yscript_parser* parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(&parser->lexer);
}

// Add new function to evaluate binary expressions
static yscript_value* evaluate_binary_expression(yscript_parser* parser, yscript_context* context, yscript_value* left, yscript_token_type op, yscript_value* right) {
    if (left->type != right->type) {
        PRINT_ERROR("Type mismatch in binary operation");
        parser->had_error = true;
        return NULL;
    }
    yscript_value* result = NULL;
    void* result_value = NULL;
    switch (op) {
        case TOKEN_PLUS:
            if (left->type->plus){
                result = value_create(left->type);
                result_value = result->type->plus(left->internal_data, right->internal_data);
                value_set(result, result_value, true);
            } else{
                PRINT_ERROR("Unsupported binary operation");
                parser->had_error = true;
            }
            break;
        case TOKEN_MINUS:
            if (left->type->minus){
                result = value_create(left->type);
                result_value = result->type->minus(left->internal_data, right->internal_data);
                value_set(result, result_value, true);
            } else{
                PRINT_ERROR("Unsupported binary operation");
                parser->had_error = true;
            }
            break;
        case TOKEN_ASTERISK:
            if (left->type->multiply){
                result = value_create(left->type);
                result_value = result->type->multiply(left->internal_data, right->internal_data);
                value_set(result, result_value, true);
            } else{
                PRINT_ERROR("Unsupported binary operation");
                parser->had_error = true;
            }
            break;
        case TOKEN_SLASH:
            if (left->type->divide){
                result = value_create(left->type);
                result_value = result->type->divide(left->internal_data, right->internal_data);
                value_set(result, result_value, true);
            } else{
                PRINT_ERROR("Unsupported binary operation");
                parser->had_error = true;
            }
            break;
        default:
            PRINT_ERROR("Unsupported binary operation");
            parser->had_error = true;
            return NULL;

    }
    value_release(left);
    value_release(right);

    return result;
}
// Helper function to get operator precedence
static u32 get_precedence(yscript_token_type type) {
    switch (type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 1;
        case TOKEN_ASTERISK:
        case TOKEN_SLASH:
            return 2;
        default:
            return 0;
    }
}

int count_decimal_digits(const char *str) {
    const char *dot = string_first_occurrence(str, '.');
    if (!dot) return 0;
    return string_span(dot + 1, "0123456789");
}

#define INT32_MIN -2147483648
#define INT32_MAX 2147483647

static yscript_value* parse_array(yscript_parser* parser, yscript_context* context, int precedence, yscript_value* parsed_array){
    yscript_value* return_value = NULL;
    parser_advance(parser);
    if (parser->current.type == TOKEN_LEFT_BRACKET){
        // advance to array number
        parser_advance(parser);
        yscript_value* index_value = NULL;
        index_value = parse_value(parser, context, precedence);
        if (!index_value) {
            PRINT_ERROR("Failed to parse value in array access brackets");
            parser->had_error = true;
            return NULL;
        }
        if (index_value->type == yscript_lookup_type(INT64_NAME, sizeof(INT64_NAME)) || index_value->type == yscript_lookup_type(INT_NAME, sizeof(INT_NAME))){
            i32* index = ((i32*)index_value->internal_data);
            value_release(index_value);
            yscript_value* array_field = NULL;
            yscript_array* array = (yscript_array*)parsed_array->internal_data;
            u64 array_size = array->length;
            if (*index > array_size-1){
                PRINT_ERROR("index %i out of bounds for Array %s with size %llu, hint: max_index is: %llu", *index, "", array_size, array_size-1);
                parser->had_error = true;
                return NULL;
            }
            array_field = array->values[*index];
            if (array_field->type == yscript_lookup_type(ARRAY_NAME, sizeof(ARRAY_NAME))){
                return_value = parse_array(parser, context, precedence, array_field);
            } else {
                parser_advance(parser); // advance the right bracket
                value_retain(array_field);
                return_value =  array_field;
            }
        } else {
            value_release(index_value);
            PRINT_ERROR("The array access index must be an integer type");
            parser->had_error = true;
            return NULL;
        }
    } else {
        value_retain(parsed_array);
        return_value = parsed_array;
    }
    return return_value;
}

static yscript_value* parse_value(yscript_parser* parser, yscript_context* context, int precedence) {
    yscript_value* left = NULL;
    // check if prevous token is a variable declaration
    switch (parser->current.type) {
        case TOKEN_INT: {
            i64 value;
            string_to_i64(parser->current.start, &value);
            if (value > INT32_MIN && value < INT32_MAX) {
                left = value_create(yscript_lookup_type(INT_NAME, sizeof(INT_NAME)));
                *(i32*)left->internal_data = value;
            } else {
                left = value_create(yscript_lookup_type(INT64_NAME, sizeof(INT64_NAME)));
                *(i64*)left->internal_data = value;
            }
            parser_advance(parser);
            break;
        }
        case TOKEN_FLOAT: {
            int decimal_digits = count_decimal_digits(parser->current.start);
            if (decimal_digits > 6) {
                left = value_create(yscript_lookup_type(FLOAT64_NAME, sizeof(FLOAT64_NAME)));
                string_to_f64(parser->current.start, left->internal_data);
            } else {
                left = value_create(yscript_lookup_type(FLOAT_NAME, sizeof(FLOAT_NAME)));
                string_to_f32(parser->current.start, left->internal_data);
            }
            parser_advance(parser);
            break;
        }
        case TOKEN_CUSTOM_TYPE_NAME:
            {
                char* next_letter_pointer = parser->current.start;
                while (isalnum(*next_letter_pointer)) {
                    next_letter_pointer++;
                }
                if (strings_nequal(next_letter_pointer, "(", 1)) {
                    
                    left = value_create(yscript_lookup_type(parser->current.start, next_letter_pointer-parser->current.start));
                    if (left->type->cast_string != NULL) {
                        left->type->cast_string(parser->current.start, left->internal_data);
                    }
                    
                    parser_advance(parser);
                }
                break;
            }
        case TOKEN_STRING: {
#ifdef DEBUG_YSCRIPT
            PRINT_DEBUG("Parsing string value");
#endif
            left = value_create(yscript_lookup_type(STRING_NAME, sizeof(STRING_NAME)));
            const char* start = parser->current.start + 1;
            const char* end = parser->current.start + parser->current.length - 1;
            u32 length = end - start;
            left->internal_data = rope_new_leaf(start, length);
#ifdef DEBUG_YSCRIPT
            RopeNode* node = (RopeNode*)left->internal_data;
            PRINT_DEBUG("Parsed string value: %s ref count set to %i", rope_flatten(node), node->ref_count );
#endif
            parser_advance(parser);
            break;
        }
        case TOKEN_TRUE: {
            left = value_create(yscript_lookup_type(BOOL_NAME, sizeof(BOOL_NAME)));
            *((b8*)left->internal_data) = true;
            parser_advance(parser);
            break;
        }
        case TOKEN_FALSE: {
            left = value_create(yscript_lookup_type(BOOL_NAME, sizeof(BOOL_NAME)));
            *((b8*)left->internal_data) = false;
            parser_advance(parser);
            break;
        }
        case TOKEN_IDENTIFIER: {
            // It's a variable reference
            yscript_variable* var = yscript_lookup_variable(context, parser->current.start, parser->current.length);
            if (!var) {
                PRINT_ERROR("Undefined variable: %.*s", parser->current.length, parser->current.start);
                parser->had_error = true;
                return NULL;
            }
            if (var->type == yscript_lookup_type(ARRAY_NAME, sizeof(ARRAY_NAME))){
                left = parse_array(parser, context, precedence, var->value);
            } else {
                left = var->value;
                value_retain(var->value);
                parser_advance(parser);
            }
            break;
        } case TOKEN_FUNC_CALL: {
            left = evaluate_function_call(parser, context, parser->current.start);
            break;
        } case TOKEN_LEFT_BRACKET: {
            // this is an array we need to parse its values
            yscript_type* array_type = yscript_lookup_type(ARRAY_NAME, sizeof(ARRAY_NAME));
            left = value_create(array_type);
            yscript_array* values_array = ((yscript_array*)left->internal_data);
            values_array->length = 0;
            parser_advance(parser);
            while (true){
                if (parser->current.type == TOKEN_RIGHT_BRACKET){
                    parser_advance(parser);
                    break;
                } else if (parser->current.type == TOKEN_COMMA) {
                    parser_advance(parser);
                    continue;
                } else {
                    yscript_value* value = NULL;
                    value = parse_value(parser, context, precedence);
                    if (!value) {
                        PRINT_ERROR("Failed to parse value in array");
                        parser->had_error = true;
                        return NULL;
                    }
                    values_array->values = yreallocate_aligned(values_array->values, 
                        sizeof(yscript_value*) * values_array->length,
                        sizeof(yscript_value*) * (values_array->length + 1),
                        8,
                        MEMORY_TAG_SCRIPT);
                    values_array->values[values_array->length++] = value;
                }
            }
            break;
        }
        default:
            PRINT_ERROR("Unexpected token type in parse_value: %s", token_type_strings[parser->current.type]);
            parser->had_error = true;
            return NULL;
    }
    while (precedence < get_precedence(parser->current.type)) {
        yscript_token_type op = parser->current.type;
        parser_advance(parser);
        yscript_value* right = parse_value(parser, context, get_precedence(op));
        if (!right) {
            value_release(left);
            return NULL;
        }
        yscript_value* new_left = evaluate_binary_expression(parser, context, left, op, right);
        if (!new_left) {
            return NULL;
        }
        left = new_left;
    }
    return left;
}

// Parse a variable declaration
static void parse_variable_declaration(yscript_parser* parser, yscript_context* context) {
    b8 is_const = false;
    if (parser->current.type == TOKEN_CONST) {
        is_const = true;
    }
    parser_advance(parser);
    
    if (parser->current.type != TOKEN_IDENTIFIER) {
        PRINT_ERROR("Expected variable name");
        parser->had_error = true;
        return;
    }
    char* name  = yallocate(parser->current.length+1, MEMORY_TAG_STRING);
    ycopy_memory(name, parser->current.start, parser->current.length);
    name[parser->current.length+1] = 0;
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Parsing variable: %s", name);
#endif
    parser_advance(parser);

    yscript_type* type = NULL; // Default type
    if (parser->current.type == TOKEN_COLON) {
        parser_advance(parser);
        if (parser->current.type == TOKEN_IDENTIFIER) {
            type = yscript_lookup_type(parser->current.start, parser->current.length);
            if (type == NULL) {
                PRINT_ERROR("Unknown type: %.*s", parser->current.length, parser->current.start);
                parser->had_error = true;
                return;
            }
            parser_advance(parser);
        } else {
            PRINT_ERROR("Expected type name after ':'");
            parser->had_error = true;
            return;
        }
    }
    yscript_value* value = NULL;
    if (parser->current.type == TOKEN_ASSIGN && 
        *parser->current.start == '=') {
        parser_advance(parser);
        value = parse_value(parser, context, 0);
        if (!value) {
            PRINT_ERROR("Failed to parse value for variable %s", name);
            parser->had_error = true;
            return;
        }
    }
    if (value == NULL){
        value = value_create(type);
    }
    
    // Add variable to context
    yscript_variable* var = yallocate_aligned(sizeof(yscript_variable), 8, MEMORY_TAG_SCRIPT);
    var->name = name;
    var->is_const = is_const;
    var->type = type;
    var->value = value;
    
    // Add to context's variables array
    context->variables = yreallocate_aligned(context->variables, 
        sizeof(yscript_variable*) * context->variable_count,
        sizeof(yscript_variable*) * (context->variable_count + 1),
        8,
        MEMORY_TAG_SCRIPT);
    context->variable_count++;
    context->variables[context->variable_count-1] = var;
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Added variable %s to context", name);
#endif

    if (parser->current.type == TOKEN_SEMICOLON) {
        parser_advance(parser);
    } else {
        PRINT_ERROR("Expected ';' after variable declaration");
        parser->had_error = true;
        return;
    }
}

// Parse a function declaration
static void parse_function_declaration(yscript_parser* parser, yscript_context* context) {
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Parsing function declaration");
#endif
    // Skip 'func' keyword
    parser_advance(parser);
    
    if (parser->current.type != TOKEN_IDENTIFIER) {
        PRINT_ERROR("Expected function name after 'func'");
        parser->had_error = true;
        return;
    }
    
    char* name = yallocate_aligned(parser->current.length+1, 8, MEMORY_TAG_STRING);
    ycopy_memory(name, parser->current.start, parser->current.length);
    name[parser->current.length+1] = 0;
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Parsing function: %s", name);
#endif
    parser_advance(parser);
    
    if (parser->current.type != TOKEN_LEFT_PAREN) {
        PRINT_ERROR("Expected '(' after function name");
        parser->had_error = true;
        return;
    }
    parser_advance(parser);
    
    // Parse parameters
    yscript_function* func = yallocate_aligned(sizeof(yscript_function), 8, MEMORY_TAG_SCRIPT);
    func->name = name;
    func->parameter_count = 0;
    func->parameter_names = NULL;
    func->parameter_types = NULL;
    
    while (parser->current.type != TOKEN_RIGHT_PAREN) {
        if (parser->current.type == TOKEN_COMMA) {
            parser_advance(parser);
        }
        
        if (parser->current.type != TOKEN_IDENTIFIER) {
            PRINT_ERROR("Expected parameter name");
            parser->had_error = true;
            return;
        }
        
        // Add parameter name
        func->parameter_names = yreallocate_aligned(func->parameter_names,
            sizeof(char*) * func->parameter_count,
            sizeof(char*) * (func->parameter_count + 1),
            8,
            MEMORY_TAG_SCRIPT);
        func->parameter_names[func->parameter_count] = yallocate(parser->current.length+1, MEMORY_TAG_STRING);
        ycopy_memory(func->parameter_names[func->parameter_count], parser->current.start, parser->current.length);
        func->parameter_names[func->parameter_count][parser->current.length+1] = 0;
#ifdef DEBUG_YSCRIPT
        PRINT_DEBUG("Found parameter: %s", func->parameter_names[func->parameter_count]);
#endif
        parser_advance(parser);
        
        if (parser->current.type != TOKEN_COLON) {
            PRINT_ERROR("Expected ':' after parameter name");
            parser->had_error = true;
            return;
        }
        parser_advance(parser);
        
        if (parser->current.type != TOKEN_IDENTIFIER) {
            PRINT_ERROR("Expected parameter type");
            parser->had_error = true;
            return;
        }
        
        // Add parameter type
        func->parameter_types = yreallocate_aligned(func->parameter_types,
            sizeof(yscript_type*) * func->parameter_count,
            sizeof(yscript_type*) * (func->parameter_count + 1),
            8,
            MEMORY_TAG_SCRIPT);
        func->parameter_types[func->parameter_count] = yscript_lookup_type(parser->current.start, parser->current.length);
        if (func->parameter_types[func->parameter_count] == NULL) {
            PRINT_ERROR("Unknown parameter type: %.*s", parser->current.length, parser->current.start);
            parser->had_error = true;
            return;
        }
#ifdef DEBUG_YSCRIPT
        PRINT_DEBUG("Parameter %s has type %d", func->parameter_names[func->parameter_count], func->parameter_types[func->parameter_count]);
#endif
        func->parameter_count++;
        parser_advance(parser);
    }
    parser_advance(parser); // Skip ')'
    
    // Parse return type
    if (parser->current.type == TOKEN_ARROW) {
        parser_advance(parser);
        if (parser->current.type != TOKEN_IDENTIFIER) {
            PRINT_ERROR("Expected return type after '->'");
            parser->had_error = true;
            return;
        }
        func->return_type = yscript_lookup_type(parser->current.start, parser->current.length);
        if (func->return_type == NULL) {
            PRINT_ERROR("Unknown return type: %.*s", parser->current.length, parser->current.start);
            parser->had_error = true;
            return;
        }
#ifdef DEBUG_YSCRIPT
        PRINT_DEBUG("Function returns type %d", func->return_type);
#endif
        parser_advance(parser);
    } else {
        func->return_type = yscript_lookup_type(NULL_NAME, sizeof(NULL_NAME));
    }
    
    // Parse function body
    if (parser->current.type != TOKEN_LEFT_BRACE) {
        PRINT_ERROR("Expected '{' for function body");
        parser->had_error = true;
        return;
    }
    parser_advance(parser);
    
    // Store the function body start
    const char* body_start = parser->current.start;
    u32 brace_count = 1;
    
    // Find the matching closing brace
    while (brace_count > 0 && parser->current.type != TOKEN_EOF) {
        if (parser->current.type == TOKEN_LEFT_BRACE) {
            brace_count++;
        } else if (parser->current.type == TOKEN_RIGHT_BRACE) {
            brace_count--;
        }
        parser_advance(parser);
    }
    
    if (brace_count != 0) {
        PRINT_ERROR("Unmatched braces in function body");
        parser->had_error = true;
        return;
    }
    
    // Store the function body
    func->body = string_duplicate(body_start);
    
    // Add function to context
    context->functions = yreallocate_aligned(context->functions,
        sizeof(yscript_function*) * context->function_count,
        sizeof(yscript_function*) * (context->function_count + 1),
        8,
        MEMORY_TAG_SCRIPT);
    context->functions[context->function_count++] = func;
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Added function %s to context with %d parameters", name, func->parameter_count);
#endif
}

// Update evaluate_function_call to handle declared functions
static yscript_value* evaluate_function_call(yscript_parser* parser, yscript_context* context, const char* func_name) {
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Evaluating function call: %.*s", parser->current.length, func_name);
#endif
    const char* func_name_start = func_name;
    u32 func_name_length = parser->current.length;
    parser_advance(parser); // Skip the function name
    parser_advance(parser); // Skip the TOKEN_LEFT_PAREN '('
    
    u32 arg_count = 0;
    yscript_value** args = NULL;
    
    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        do {
            if (parser->current.type == TOKEN_COMMA) {
                parser_advance(parser);
            }
            yscript_value* arg = parse_value(parser, context, 0);
            if (!arg) {
                PRINT_ERROR("Failed to parse argument %d", arg_count);
                parser->had_error = true;
                return NULL;
            }
            
            args = yreallocate_aligned(args, 
                sizeof(yscript_value*) * arg_count,
                sizeof(yscript_value*) * (arg_count + 1),
                8,
                MEMORY_TAG_SCRIPT);
            args[arg_count++] = arg;
            
        } while (parser->current.type == TOKEN_COMMA);
    }
    
    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        PRINT_ERROR("Expected ')' after arguments");
        parser->had_error = true;
        return NULL;
    }
    parser_advance(parser);
    
    // Check for declared functions first
    for (u32 i = 0; i < context->function_count; i++) {
        yscript_function* func = context->functions[i];
        if (strings_nequal(func->name, func_name_start, func_name_length)) {
            if (arg_count != func->parameter_count) {
                PRINT_ERROR("Function %.*s expects %d arguments, got %d", func_name_length, func_name_start, func->parameter_count, arg_count);
                parser->had_error = true;
                for (u32 j = 0; j < arg_count; j++) {
                    value_release(args[j]);
                }
                yfree(args);
                return NULL;
            }
            
            // Create a new context for the function
            yscript_context* func_context = yscript_context_create();
            func_context->parent = context;
            
            // Add parameters as variables in the function context
            for (u32 j = 0; j < func->parameter_count; j++) {
                yscript_variable* param = yallocate_aligned(sizeof(yscript_variable), 8, MEMORY_TAG_SCRIPT);
                param->name = string_duplicate(func->parameter_names[j]);
                param->is_const = true;
                param->type = func->parameter_types[j];
                
                // Check if argument type matches parameter type
                if (args[j]->type != func->parameter_types[j]) {
                    PRINT_ERROR("Type mismatch in argument %d for function %.*s. Expected %s, got %s", 
                        j, func_name_length, func_name_start,
                        func->parameter_types[j]->type_name,
                        args[j]->type->type_name);
                    parser->had_error = true;
                    yscript_context_destroy(func_context);
                    yfree(args);
                    return NULL;
                }
                
                param->value = args[j];
                func_context->variables = yreallocate_aligned(func_context->variables, 
                    sizeof(yscript_variable*) * func_context->variable_count, 
                    sizeof(yscript_variable*) * (func_context->variable_count + 1), 
                    8, 
                    MEMORY_TAG_SCRIPT);
#ifdef DEBUG_YSCRIPT
                PRINT_DEBUG("Adding parameter %s to function context", param->name);
#endif
                func_context->variables[func_context->variable_count++] = param;
            }
            
            // Execute the function body
            yscript_execute_string(func_context, func->body);
            
            // Get the return value if any
            yscript_value* result = NULL;
            if (func_context->return_value) {
                result = func_context->return_value;
                func_context->return_value = NULL; // Prevent double free
            } else if (func->return_type != yscript_lookup_type(NULL_NAME, sizeof(NULL_NAME))) {
                result = value_create(func->return_type);
            }
            
            yscript_context_destroy(func_context);
            yfree(args);
            return result;
        }
    }
    
    // Then, check for native functions
    for (u32 i = 0; i < context->variable_count; i++) {
        yscript_variable* var = context->variables[i];
        if (strings_nequal(var->name, func_name_start, func_name_length) && 
            var->value->type == yscript_lookup_type(FUNCTION_NATIVE_TYPE_NAME, sizeof(FUNCTION_NATIVE_TYPE_NAME))) {
            yscript_native_function* native = var->value->internal_data;
            yscript_value* result = native->function(context, args, arg_count);
            for (u32 j = 0; j < arg_count; j++) {
                value_release(args[j]);
            }
            yfree(args);
            return result;
        }
    }

    PRINT_ERROR("Function '%.*s' not found", func_name_length, func_name_start);
    parser->had_error = true;
    for (u32 j = 0; j < arg_count; j++) {
        value_release(args[j]);
    }
    yfree(args);
    return NULL;
}

// Execute YScript code
b8 yscript_execute_string(yscript_context* context, char* code) {
#ifdef DEBUG_YSCRIPT
    PRINT_DEBUG("Executing YScript code: %s", code);
#endif
    yscript_parser parser;
    parser_init(&parser, code);

    while (parser.current.type != TOKEN_EOF && !parser.had_error) {
        char token_text[parser.current.length + 1];
        string_ncopy(token_text, parser.current.start, parser.current.length);
        token_text[parser.current.length] = '\0';
#ifdef DEBUG_YSCRIPT
        PRINT_TRACE("Parser processing token: type=%s, text='%s'", token_type_strings[parser.current.type], token_text);
#endif
        if (parser.current.type == TOKEN_VAR || parser.current.type == TOKEN_CONST) {
#ifdef DEBUG_YSCRIPT
            PRINT_DEBUG("Parsing variable declaration");
#endif
            parse_variable_declaration(&parser, context);
        } else if (parser.current.type == TOKEN_FUNC){
#ifdef DEBUG_YSCRIPT
            PRINT_DEBUG("Found function declaration");
#endif
            parse_function_declaration(&parser, context);
        } else if (parser.current.type == TOKEN_FUNC_CALL) {
#ifdef DEBUG_YSCRIPT
            PRINT_DEBUG("Found function call");
#endif
            yscript_value* result = evaluate_function_call(&parser, context, parser.current.start);
            if (result) {
                value_release(result);
            }
            // check for semicolon
            if (parser.current.type == TOKEN_SEMICOLON) {
                parser_advance(&parser);
            } else {
                PRINT_ERROR("Expected ';' after function call");
                parser.had_error = true;
            }
        } else if (parser.current.type == TOKEN_RETURN) {
#ifdef DEBUG_YSCRIPT
            PRINT_DEBUG("Found return statement");
#endif
            parser_advance(&parser); // Skip 'return'
            
            // Parse the return value
            yscript_value* return_value = parse_value(&parser, context, 0);
            if (return_value) {
                // Store the return value in the context
                if (context->return_value) {
                    value_release(context->return_value);
                }
                context->return_value = return_value;
            }
            
            // Check for semicolon
            if (parser.current.type == TOKEN_SEMICOLON) {
                parser_advance(&parser);
            } else {
                PRINT_ERROR("Expected ';' after return statement");
                parser.had_error = true;
            }
            
            // Stop execution after return
            break;
        } else if (parser.current.type == TOKEN_COMMENT) {
            // Skip comments
#ifdef DEBUG_YSCRIPT
            PRINT_DEBUG("Parser Skipping comment");
#endif
            parser_advance(&parser);
        }
    }

    if (parser.had_error) {
        PRINT_ERROR("Parser encountered an error");
    }

    return !parser.had_error;
}

// Register a native function
void yscript_register_native_function(yscript_context* context, const char* name, yscript_native_fn function, 
    u32 parameter_count, yscript_type** parameter_types, yscript_type* return_type) {
    
    yscript_native_function* native_func = yallocate_aligned(sizeof(yscript_native_function), 8, MEMORY_TAG_SCRIPT);
    native_func->name = string_duplicate(name);
    native_func->function = function;
    native_func->parameter_count = parameter_count;
    
    // Copy parameter types
    native_func->parameter_types = yallocate_aligned(sizeof(yscript_type*) * parameter_count, 8, MEMORY_TAG_SCRIPT);
    for (u32 i = 0; i < parameter_count; i++) {
        native_func->parameter_types[i] = parameter_types[i];
    }
    
    native_func->return_type = return_type;

    yscript_value* value = value_create(yscript_lookup_type(FUNCTION_NATIVE_TYPE_NAME, sizeof(FUNCTION_NATIVE_TYPE_NAME)));
    value->internal_data = native_func;

    // Add to context's variables
    yscript_variable* var = yallocate_aligned(sizeof(yscript_variable), 8, MEMORY_TAG_SCRIPT);
    var->name = string_duplicate(name);
    var->is_const = true;
    var->type = yscript_lookup_type(FUNCTION_NATIVE_TYPE_NAME, sizeof(FUNCTION_NATIVE_TYPE_NAME));
    var->value = value;

    context->variables = yreallocate_aligned(context->variables, 
        sizeof(yscript_variable*) * context->variable_count,
        sizeof(yscript_variable*) * (context->variable_count + 1),
        8,
        MEMORY_TAG_SCRIPT);
    context->variable_count++;
    context->variables[context->variable_count-1] = var;
}


void native_function_free(void* internal_data){
    if (!internal_data){
        return;
    }
    yscript_native_function* native_function = ((yscript_native_function*)internal_data);
    yfree(native_function->name);
    yfree(native_function->parameter_types);
    yfree(native_function);
}


// Register built-in native functions
void yscript_register_builtin_functions(yscript_context* context) {
    // Register logger functions
    //E_YSCRIPT_VALUE_TYPE string_type = YSCRIPT_VALUE_TYPE_STRING;
    yscript_type* null_type = yscript_lookup_type(NULL_NAME, sizeof(NULL_NAME));
    yscript_type* any_type = yscript_lookup_type(NULL_NAME, sizeof(NULL_NAME));

    yscript_register_native_function(context, "print", native_print, 1, &any_type, null_type);
    yscript_register_native_function(context, "print_error", native_print_error, 1, &any_type, null_type);
    yscript_register_native_function(context, "print_warning", native_print_warning, 1, &any_type, null_type);
    yscript_register_native_function(context, "print_info", native_print_info, 1, &any_type, null_type);
    yscript_register_native_function(context, "print_debug", native_print_debug, 1, &any_type, null_type);
    yscript_register_native_function(context, "print_trace", native_print_trace, 1, &any_type, null_type);
}

void yscript_register_type(yscript_type* type) {
    // Add to types array
    registered_types = yreallocate_aligned(registered_types, 
        sizeof(yscript_type*) * types_count,
        sizeof(yscript_type*) * (types_count + 1),
        8,
        MEMORY_TAG_SCRIPT);
    types_count++;
    registered_types[types_count-1] = type;
}
static b8 registered_native_types = false;
// Register built-in native functions
void yscript_register_builtin_types(void) {
    if (registered_native_types){
        return;
    }
    registered_native_types = true;
    // Register types
    yscript_register_type(yscript_type_create(NULL_NAME, true, sizeof(u64*), 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    yscript_register_type(yscript_type_create(INT_NAME, false, sizeof(i32), 0, 0, 0, i32_plus, i32_minus, i32_multiply, i32_divide, string_to_int32, i32_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(INT64_NAME, false, sizeof(i64), 0, 0, 0, i64_plus, i64_minus, i64_multiply, i64_divide, string_to_int64, i64_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(FLOAT_NAME, false, sizeof(f32), 0, 0, 0, f32_plus, f32_minus, f32_multiply, f32_divide, string_to_float32, f32_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(FLOAT64_NAME, false, sizeof(f64), 0, 0, 0, f64_plus, f64_minus, f64_multiply, f64_divide, string_to_float64, f64_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(VECTOR2_NAME, false, sizeof(Vector2), 0, 0, 0, Vector2_plus, Vector2_minus, Vector2_mul, Vector2_divide, string_to_vector2, vector2_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(VECTOR3_NAME, false, sizeof(Vector3), 0, 0, 0, Vector3_plus, Vector3_minus, Vector3_mul, Vector3_divide, string_to_vector3, vector3_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(VECTOR4_NAME, false, sizeof(Vector4), 0, 0, 0, Vector4_plus, Vector4_minus, Vector4_mul, Vector4_divide, string_to_vector4, vector4_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(MATRIX4_NAME, false, sizeof(Matrice4), 0, 0, 0, NULL, NULL, Matrix4_mul, NULL, NULL, matrix4_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(STRING_NAME, true, sizeof(RopeNode*), 0, 0, 0, string_plus, NULL, NULL, NULL, NULL, string_to_string, NULL, rope_string_free));
    yscript_register_type(yscript_type_create(BOOL_NAME, false, sizeof(b8), 0, 0, 0, NULL, NULL, NULL, NULL, NULL, bool_to_string, NULL, non_pointer_free));
    yscript_register_type(yscript_type_create(ARRAY_NAME, false, sizeof(yscript_array), 0, 0, 0, NULL, NULL, NULL, NULL, NULL, array_to_string, NULL, array_free));
    yscript_register_type(yscript_type_create(FUNCTION_NATIVE_TYPE_NAME, true, sizeof(yscript_native_function*), 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, native_function_free));
}

void yscript_clear_types(void){
    for (u8 i = 0; i < types_count; i++){
        yscript_type_destroy(registered_types[i]);
    }
    yfree(registered_types);
}
// Create a new script context
yscript_context* yscript_context_create(void) {
    yscript_context* context = yallocate_aligned(sizeof(yscript_context), 8, MEMORY_TAG_SCRIPT);
    context->variables = NULL;
    context->variable_count = 0;
    context->functions = NULL;
    context->function_count = 0;
    context->parent = NULL;
    context->return_value = NULL;

    yscript_register_builtin_types();
    // Register built-in functions
    yscript_register_builtin_functions(context);

    return context;
}

// Destroy a script context
void yscript_context_destroy(yscript_context* context) {
    if (context) {
        if(context->variables){
            // Free variables
            for (u32 i = 0; i < context->variable_count; i++) {
                yscript_variable* var = context->variables[i];
                if (var->value) {
                    //PRINT_INFO("free variable %s", var->name)
                    value_release(var->value);
                }

                if (var->name){
                    yfree(var->name);
                }
                yfree(var);
            }
            yfree(context->variables);
        }
        if (context->functions){
            // Free functions
            for (u32 i = 0; i < context->function_count; i++) {
                yscript_function* func = context->functions[i];
                yfree(func->name);
                for (u32 j = 0; j < func->parameter_count; j++) {
                    yfree(func->parameter_names[j]);
                }
                yfree(func->parameter_names);
                yfree(func->parameter_types);
                yfree(func->body);
                yfree(func);
            }
            yfree(context->functions);
        }
        
        value_release(context->return_value);
        yfree(context);
    }
} 
