#pragma once

#include "defines.h"

typedef enum E_ROPE_TYPE {
    ROPE_LEAF,
    ROPE_CONCAT
} E_ROPE_TYPE;

typedef struct RopeNode {
    E_ROPE_TYPE type;
    union {
        struct {
            char* data;
            u64 length;
        } leaf;

        struct {
            struct RopeNode* left;
            struct RopeNode* right;
            u64 weight;         // length of left
            char* flattened_cache; // lazily built
        } concat;
    };
} RopeNode;
// manualy specified length
RopeNode* rope_new_leaf(const char* data, u64 length);
RopeNode* rope_new(const char* data);
RopeNode* rope_from_list(const char** strings, const u64* lengths, u64 count);
RopeNode* rope_concat(RopeNode* left, RopeNode* right);
RopeNode* rope_duplicate(RopeNode* node);
// convert to string
char* rope_flatten(RopeNode* node);
// Returns the length of the rope in bytes
u64 rope_byte_length(const RopeNode* node);
u64 rope_length(RopeNode* node);
b8 rope_is_empty(RopeNode* node);
b8 rope_are_equal(RopeNode* node1, RopeNode* node2);
void rope_free(RopeNode* node);
RopeNode* rope_insert_at(RopeNode* root, u64 index, const char* insert_data);
RopeNode* rope_delete_at(RopeNode* root, u64 index, u64 delete_length);
RopeNode* rope_replace_at(RopeNode* root, u64 index, u64 length, const char* replacement);
RopeNode* rope_substring(RopeNode* root, u64 start, u64 end);
i64 rope_find(RopeNode* root, const char* needle);
void rope_split_at(RopeNode* root, u64 index, RopeNode** left_out, RopeNode** right_out);
RopeNode* rope_balance(RopeNode** nodes, u64 count);
