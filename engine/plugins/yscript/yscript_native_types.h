#pragma once

#include "yscript.h"
#include "core/logger.h"
#include "core/ystring.h"
#include "math/ymath.h"
#include "yscript_native_logger_hooks.h"

#define INT_NAME "int"
#define INT64_NAME "int64"
#define FLOAT_NAME "float"
#define FLOAT64_NAME "float64"
#define VECTOR2_NAME "Vector2"
#define VECTOR3_NAME "Vector3"
#define VECTOR4_NAME "Vector4"
#define MATRIX4_NAME "Matrix4x4"
#define STRING_NAME "string"
#define BOOL_NAME "bool"
#define NULL_NAME "NULL"
#define ARRAY_NAME "Array"
#define FUNCTION_TYPE_NAME "function"
#define FUNCTION_NATIVE_TYPE_NAME "native_function"

void* i64_plus(void* left_value, void* right_value) {
    i64 calculated_value = *((i64*)left_value) + *((i64*)right_value);

    void* return_value = yallocate(sizeof(i64), MEMORY_TAG_SCRIPT);
    *(i64*)return_value = calculated_value;

    return return_value;
}

void* i64_minus(void* left_value, void* right_value) {
    i64 calculated_value = *((i64*)left_value) - *((i64*)right_value);

    void* return_value = yallocate(sizeof(i64), MEMORY_TAG_SCRIPT);
    *(i64*)return_value = calculated_value;

    return return_value;
}

void* i64_multiply(void* left_value, void* right_value) {
    i64 calculated_value = *((i64*)left_value) * *((i64*)right_value);

    void* return_value = yallocate(sizeof(i64), MEMORY_TAG_SCRIPT);
    *(i64*)return_value = calculated_value;

    return return_value;
}

void* i64_divide(void* left_value, void* right_value) {
    if (*((i64*)right_value) == 0) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    i64 calculated_value = *((i64*)left_value) / *((i64*)right_value);

    void* return_value = yallocate(sizeof(i64), MEMORY_TAG_SCRIPT);
    *(i64*)return_value = calculated_value;

    return return_value;
}

void* i32_plus(void* left_value, void* right_value) {
    i32 calculated_value = *((i32*)left_value) + *((i32*)right_value);

    void* return_value = yallocate(sizeof(i32), MEMORY_TAG_SCRIPT);
    *(i32*)return_value = calculated_value;

    return return_value;
}

void* i32_minus(void* left_value, void* right_value) {
    i32 calculated_value = *((i32*)left_value) - *((i32*)right_value);

    void* return_value = yallocate(sizeof(i32), MEMORY_TAG_SCRIPT);
    *(i32*)return_value = calculated_value;

    return return_value;
}

void* i32_multiply(void* left_value, void* right_value) {
    i32 calculated_value = *((i32*)left_value) * *((i32*)right_value);

    void* return_value = yallocate(sizeof(i32), MEMORY_TAG_SCRIPT);
    *(i32*)return_value = calculated_value;

    return return_value;
}

void* i32_divide(void* left_value, void* right_value) {
    if (*((i32*)right_value) == 0) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    i32 calculated_value = *((i32*)left_value) / *((i32*)right_value);

    void* return_value = yallocate(sizeof(i32), MEMORY_TAG_SCRIPT);
    *(i32*)return_value = calculated_value;

    return return_value;
}

void* f32_plus(void* left_value, void* right_value) {
    f32 calculated_value = *((f32*)left_value) + *((f32*)right_value);

    void* return_value = yallocate(sizeof(f32), MEMORY_TAG_SCRIPT);
    *(f32*)return_value = calculated_value;

    return return_value;
}

void* f32_minus(void* left_value, void* right_value) {
    f32 calculated_value = *((f32*)left_value) - *((f32*)right_value);

    void* return_value = yallocate(sizeof(f32), MEMORY_TAG_SCRIPT);
    *(f32*)return_value = calculated_value;

    return return_value;
}

void* f32_multiply(void* left_value, void* right_value) {
    f32 calculated_value = *((f32*)left_value) * *((f32*)right_value);

    void* return_value = yallocate(sizeof(f32), MEMORY_TAG_SCRIPT);
    *(f32*)return_value = calculated_value;

    return return_value;
}

void* f32_divide(void* left_value, void* right_value) {
    if (*((f32*)right_value) == 0) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    f32 calculated_value = *((f32*)left_value) / *((f32*)right_value);

    void* return_value = yallocate(sizeof(f32), MEMORY_TAG_SCRIPT);
    *(f32*)return_value = calculated_value;

    return return_value;
}

void* f64_plus(void* left_value, void* right_value) {
    f64 calculated_value = *((f64*)left_value) + *((f64*)right_value);

    void* return_value = yallocate(sizeof(f64), MEMORY_TAG_SCRIPT);
    *(f64*)return_value = calculated_value;

    return return_value;
}

void* f64_minus(void* left_value, void* right_value) {
    f64 calculated_value = *((f64*)left_value) - *((f64*)right_value);

    void* return_value = yallocate(sizeof(f64), MEMORY_TAG_SCRIPT);
    *(f64*)return_value = calculated_value;

    return return_value;
}

void* f64_multiply(void* left_value, void* right_value) {
    f64 calculated_value = *((f64*)left_value) * *((f64*)right_value);

    void* return_value = yallocate(sizeof(f64), MEMORY_TAG_SCRIPT);
    *(f64*)return_value = calculated_value;

    return return_value;
}

void* f64_divide(void* left_value, void* right_value) {
    if (*((f64*)right_value) == 0) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    f64 calculated_value = *((f64*)left_value) / *((f64*)right_value);

    void* return_value = yallocate(sizeof(f64), MEMORY_TAG_SCRIPT);
    *(f64*)return_value = calculated_value;

    return return_value;
}

void* Vector2_plus(void* left_value, void* right_value) {
    Vector2 calculated_value = Vector2_add(*((Vector2*)left_value), *((Vector2*)right_value));

    void* return_value = yallocate(sizeof(Vector2), MEMORY_TAG_SCRIPT);
    *(Vector2*)return_value = calculated_value;

    return return_value;
}

void* Vector2_minus(void* left_value, void* right_value) {
    Vector2 calculated_value = Vector2_sub(*((Vector2*)left_value), *((Vector2*)right_value));

    void* return_value = yallocate(sizeof(Vector2), MEMORY_TAG_SCRIPT);
    *(Vector2*)return_value = calculated_value;

    return return_value;
}

void* Vector2_mul(void* left_value, void* right_value) {
    Vector2 calculated_value = Vector2_multiply(*((Vector2*)left_value), *((Vector2*)right_value));

    void* return_value = yallocate(sizeof(Vector2), MEMORY_TAG_SCRIPT);
    *(Vector2*)return_value = calculated_value;

    return return_value;
}

void* Vector2_divide(void* left_value, void* right_value) {
    if ((*((Vector2*)right_value)).x == 0.0f || \
        (*((Vector2*)right_value)).y == 0.0f) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    Vector2 calculated_value = Vector2_div(*((Vector2*)left_value), *((Vector2*)right_value));

    void* return_value = yallocate(sizeof(Vector2), MEMORY_TAG_SCRIPT);
    *(Vector2*)return_value = calculated_value;

    return return_value;
}

void* Vector3_plus(void* left_value, void* right_value) {
    Vector3 calculated_value = Vector3_add(*((Vector3*)left_value), *((Vector3*)right_value));

    void* return_value = yallocate(sizeof(Vector3), MEMORY_TAG_SCRIPT);
    *(Vector3*)return_value = calculated_value;

    return return_value;
}

void* Vector3_minus(void* left_value, void* right_value) {
    Vector3 calculated_value = Vector3_sub(*((Vector3*)left_value), *((Vector3*)right_value));

    void* return_value = yallocate(sizeof(Vector3), MEMORY_TAG_SCRIPT);
    *(Vector3*)return_value = calculated_value;

    return return_value;
}

void* Vector3_mul(void* left_value, void* right_value) {
    Vector3 calculated_value = Vector3_multiply(*((Vector3*)left_value), *((Vector3*)right_value));

    void* return_value = yallocate(sizeof(Vector3), MEMORY_TAG_SCRIPT);
    *(Vector3*)return_value = calculated_value;

    return return_value;
}

void* Vector3_divide(void* left_value, void* right_value) {
    if ((*((Vector3*)right_value)).x == 0.0f || \
        (*((Vector3*)right_value)).y == 0.0f || \
        (*((Vector3*)right_value)).z == 0.0f) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    Vector3 calculated_value = Vector3_div(*((Vector3*)left_value), *((Vector3*)right_value));

    void* return_value = yallocate(sizeof(Vector3), MEMORY_TAG_SCRIPT);
    *(Vector3*)return_value = calculated_value;

    return return_value;
}

void* Vector4_plus(void* left_value, void* right_value) {
    Vector4 calculated_value = Vector4_add(*((Vector4*)left_value), *((Vector4*)right_value));

    void* return_value = yallocate(sizeof(Vector4), MEMORY_TAG_SCRIPT);
    *(Vector4*)return_value = calculated_value;

    return return_value;
}

void* Vector4_minus(void* left_value, void* right_value) {
    Vector4 calculated_value = Vector4_sub(*((Vector4*)left_value), *((Vector4*)right_value));

    void* return_value = yallocate(sizeof(Vector4), MEMORY_TAG_SCRIPT);
    *(Vector4*)return_value = calculated_value;

    return return_value;
}

void* Vector4_mul(void* left_value, void* right_value) {
    Vector4 calculated_value = Vector4_multiply(*((Vector4*)left_value), *((Vector4*)right_value));

    void* return_value = yallocate(sizeof(Vector4), MEMORY_TAG_SCRIPT);
    *(Vector4*)return_value = calculated_value;

    return return_value;
}

void* Vector4_divide(void* left_value, void* right_value) {
    if ((*((Vector4*)right_value)).x == 0.0f || \
        (*((Vector4*)right_value)).y == 0.0f || \
        (*((Vector4*)right_value)).z == 0.0f || \
        (*((Vector4*)right_value)).w == 0.0f) {
        // we should find a way to report it
        //PRINT_ERROR("Division by zero");
        //parser->had_error = true;
        //value_release(result);
        return NULL;
    }
    Vector4 calculated_value = Vector4_div(*((Vector4*)left_value), *((Vector4*)right_value));

    void* return_value = yallocate(sizeof(Vector4), MEMORY_TAG_SCRIPT);
    *(Vector4*)return_value = calculated_value;

    return return_value;
}

void* Matrix4_mul(void* left_value, void* right_value) {
    Matrice4 calculated_value = Matrice4_multiply(*((Matrice4*)left_value), *((Matrice4*)right_value));

    void* return_value = yallocate(sizeof(Matrice4), MEMORY_TAG_SCRIPT);
    *(Matrice4*)return_value = calculated_value;

    return return_value;
}

void* string_plus(void* left_value, void* right_value) {
    RopeNode* left = (RopeNode*)left_value;
    RopeNode* right = (RopeNode*)right_value;
    RopeNode* calculated_value = rope_concat(left, right);
    return calculated_value;
}

b8 string_to_int32(char* str, void* out_value){
    if (!str) {
        return false;
    }
    i32* i = (i32*)out_value;
    *i = 0;

    i32 result = sscanf(str, "int(%i)", i);
    return result != -1;
}

b8 string_to_int64(char* str, void* out_value){
    if (!str) {
        return false;
    }
    i64* i = (i64*)out_value;
    *i = 0;
    i32 result = sscanf(str, "int64(%lli)", i);
    return result != -1;
}

b8 string_to_float32(char* str, void* out_value){
    if (!str) {
        return false;
    }
    f32* f = (f32*)out_value; 
    *f = 0;
    i32 result = sscanf(str, "float(%f)", f);
    return result != -1;
}

b8 string_to_float64(char* str, void* out_value){
    if (!str) {
        return false;
    }

    f64* f = (f64*)out_value;
    i32 result = sscanf(str, "float64(%lf)", f);
    return result != -1;
}

RopeNode* i32_to_string(void* value){
    char string[16] = {0};
    string_format(string, "%i\0", *(i32*)value);
    return rope_new(string);
}

RopeNode* i64_to_string(void* value){
    char string[32] = {0};
    string_format(string, "%lli\0", *(i64*)value);
    return rope_new(string);
}

RopeNode* f32_to_string(void* value){
    char string[16] = {0};
    string_format(string, "%f\0", *(f32*)value);
    return rope_new(string);
}

RopeNode* f64_to_string(void* value){
    char string[32] = {0};
    string_format(string, "%.20f\0", *(f64*)value);
    return rope_new(string);
}

RopeNode* string_to_string(void* value){
    return rope_duplicate((RopeNode*)value);
}

RopeNode* bool_to_string(void* value){
    char string[6] = {0};
    string_format(string, "%s\0", *(b8*)value ? "true" : "false");
    return rope_new(string);
}

RopeNode* vector2_to_string(void* value){
    Vector2* vector = (Vector2*)value;
    char string[128] = {0};
    string_format(string, "Vector2(%f, %f)\0", vector->x, vector->y);
    return rope_new(string);
}

RopeNode* vector3_to_string(void* value){
    Vector3* vector = (Vector3*)value;
    char string[128] = {0};
    string_format(string, "Vector3(%f, %f, %f)\0", vector->x, vector->y, vector->z);
    return rope_new(string);
}

RopeNode* vector4_to_string(void* value){
    Vector4* vector = (Vector4*)value;
    char string[128] = {0};
    string_format(string, "Vector4(%f, %f, %f, %f)\0", vector->x, vector->y, vector->z, vector->w);
    return rope_new(string);
}

RopeNode* matrix4_to_string(void* value){
    Matrice4* matrix4_value = (Matrice4*)value;
    char string[128] = {0};
    string_format(string, "Matrix4x4(\
                    %f, %f, %f, %f,\
                    %f, %f, %f, %f,\
                    %f, %f, %f, %f,\
                    %f, %f, %f, %f)\0",
                    matrix4_value->data[0], matrix4_value->data[1], matrix4_value->data[2], matrix4_value->data[3],
                    matrix4_value->data[4], matrix4_value->data[5], matrix4_value->data[6], matrix4_value->data[7],
                    matrix4_value->data[8], matrix4_value->data[9], matrix4_value->data[10], matrix4_value->data[11],
                    matrix4_value->data[12], matrix4_value->data[13], matrix4_value->data[14], matrix4_value->data[15]
                );
    return rope_new(string);
}

RopeNode* array_to_string(void* value){
    yscript_array* array = (yscript_array*)value;
    u64 array_size = array->length;
    RopeNode* array_string = rope_new("[ ");
    RopeNode* comma_string = rope_new(", ");
    comma_string->ref_count--;
    for (u64 i=0; i < array_size; i++){
        yscript_value* array_field = array->values[i];
        if (array_field->type->cast_to_string){
            array_string->ref_count--;
            RopeNode* field_rope = array_field->type->cast_to_string(array_field->internal_data);
            field_rope->ref_count--;
            array_string = rope_concat(array_string, field_rope);
        }
        if (i < array_size-1){
            array_string->ref_count--;
            array_string = rope_concat(array_string, comma_string);
        }
    }
    RopeNode* closing = rope_new(" ]");
    closing->ref_count--;
    array_string->ref_count--;
    array_string = rope_concat(array_string, closing);
    return array_string;
}

void rope_string_free(void* internal_data){
    if (!internal_data){
        return;
    }
    RopeNode* rope = ((RopeNode*)internal_data);
    rope_release(rope);
}

void array_free(void* internal_data){
    if (!internal_data){
        return;
    }
    yscript_array* values_array = (yscript_array*)internal_data;

    u64 array_size = values_array->length;
    for (u32 i=0; i < array_size; i++){
        yscript_value* array_field = NULL;
        array_field = values_array->values[i];
        value_release(array_field);
    }
    yfree(values_array->values);
    yfree(values_array);
}

void non_pointer_free(void* internal_data){
    if (!internal_data){
        return;
    }
    yfree(internal_data);
}
