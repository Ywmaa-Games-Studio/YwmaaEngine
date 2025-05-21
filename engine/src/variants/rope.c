#include "rope.h"
#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"

u64 rope_codepoint_to_byte_offset(const char* data, u64 length, u64 cp_index) {
    u64 byte_offset = 0;
    u64 cp_count = 0;

    while (byte_offset < length && cp_count < cp_index) {
        unsigned char byte = (unsigned char)data[byte_offset];
        u64 advance = 1;
        if ((byte & 0x80) == 0x00) advance = 1;
        else if ((byte & 0xE0) == 0xC0 && byte_offset + 1 < length) advance = 2;
        else if ((byte & 0xF0) == 0xE0 && byte_offset + 2 < length) advance = 3;
        else if ((byte & 0xF8) == 0xF0 && byte_offset + 3 < length) advance = 4;
        else break; // invalid UTF-8

        //if (byte_offset + advance > length) break; // would overrun
        byte_offset += advance;
        cp_count++;
    }
    return byte_offset > length ? length : byte_offset;
}

RopeNode* rope_new_leaf(const char* data, u64 length) {
    b8 is_empty = (data == NULL || length == 0);
    RopeNode* node = (RopeNode*)yallocate_aligned(sizeof(RopeNode), 8, MEMORY_TAG_ROPE);
    if (!node) return NULL;

    node->type = ROPE_LEAF;
    node->leaf.data = (char*)yallocate(is_empty ? 1 : length, MEMORY_TAG_ROPE);
    if (!node->leaf.data) {
        yfree(node, MEMORY_TAG_ROPE);
        return NULL;
    }
    ycopy_memory(node->leaf.data, data, length);
    node->leaf.length = length;
    return node;
}

RopeNode* rope_new(const char* data) {
    u64 length = string_length(data);
    return rope_new_leaf(data, length);
}

RopeNode* rope_from_list(const char** strings, const u64* lengths, u64 count) {
    if (count == 0) return NULL;
    RopeNode** nodes = (RopeNode**)yallocate_aligned(count * sizeof(RopeNode*), 8, MEMORY_TAG_ROPE);
    for (u64 i = 0; i < count; ++i) {
        nodes[i] = rope_new_leaf(strings[i], lengths[i]);
    }
    RopeNode* balanced = rope_balance(nodes, count);
    yfree(nodes, MEMORY_TAG_ROPE);
    return balanced;
}


RopeNode* rope_concat(RopeNode* left, RopeNode* right) {
    if (!left) return right;
    if (!right) return left;
    if (!left && !right) return NULL;
    RopeNode* node = (RopeNode*)yallocate_aligned(sizeof(RopeNode), 8, MEMORY_TAG_ROPE);
    if (!node) return NULL;

    node->type = ROPE_CONCAT;
    node->concat.left = left;
    node->concat.right = right;
    node->concat.weight = rope_byte_length(left);
    node->concat.flattened_cache = NULL;
    return node;
}

RopeNode* rope_duplicate(RopeNode* node){
    if (!node) return NULL;

    RopeNode* new_node = (RopeNode*)yallocate_aligned(sizeof(RopeNode), 8, MEMORY_TAG_ROPE);
    if (!new_node) return NULL;

    new_node->type = node->type;
    if (node->type == ROPE_LEAF) {
        new_node->leaf.data = (char*)yallocate(node->leaf.length, MEMORY_TAG_ROPE);
        ycopy_memory(new_node->leaf.data, node->leaf.data, node->leaf.length);
        new_node->leaf.length = node->leaf.length;
    } else {
        new_node->concat.left = rope_duplicate(node->concat.left);
        new_node->concat.right = rope_duplicate(node->concat.right);
        new_node->concat.weight = node->concat.weight;
        new_node->concat.flattened_cache = NULL;
    }
    return new_node;
}

b8 rope_is_empty(RopeNode* node){
    if (!node) return true;
    if (node->type == ROPE_LEAF) return node->leaf.length == 0;
    return rope_is_empty(node->concat.left) && rope_is_empty(node->concat.right);
}


b8 rope_are_equal(RopeNode* node1, RopeNode* node2) {
    if (!node1 && !node2) return true;
    if (!node1 || !node2) return false;

    if (node1->type != node2->type) return false;

    if (node1->type == ROPE_LEAF) {
        if (node1->leaf.length != node2->leaf.length) return false;
        return strings_equal(node1->leaf.data, node2->leaf.data);
    } else {
        return rope_are_equal(node1->concat.left, node2->concat.left) &&
               rope_are_equal(node1->concat.right, node2->concat.right);
    }
}

// Returns the length of the rope in bytes
u64 rope_byte_length(const RopeNode* node) {
    if (!node) return 0;
    if (node->type == ROPE_LEAF) return node->leaf.length;
    return node->concat.weight + rope_byte_length(node->concat.right);
}

// Supports UTF-8 returns the number of codepoints
u64 rope_length(RopeNode* node) {
    char* data = rope_flatten(node);
    u64 length = rope_byte_length(node);
    u64 count = 0;
    for (u64 i = 0; i < length;) {
        unsigned char byte = (unsigned char)data[i];
        if ((byte & 0x80) == 0) i += 1;
        else if ((byte & 0xE0) == 0xC0) i += 2;
        else if ((byte & 0xF0) == 0xE0) i += 3;
        else if ((byte & 0xF8) == 0xF0) i += 4;
        else return 0; // invalid utf-8
        count++;
    }
    return count;
}
// Results from here are null-terminated
// Returns a pointer to the flattened string. The caller is responsible for freeing it.
// The flattened string is cached in the node for performance.
char* rope_flatten(RopeNode* node) {
    if (!node) return NULL;

    if (node->type == ROPE_LEAF) {
        return node->leaf.data;
    }

    if (node->concat.flattened_cache) {
        return node->concat.flattened_cache;
    }

    u64 length_left = rope_byte_length(node->concat.left);
    u64 length_right = rope_byte_length(node->concat.right);
    u64 total = length_left + length_left;

    char* result = (char*)yallocate(total, MEMORY_TAG_STRING);
    if (!result) return NULL;

    const char* left_flat = rope_flatten(node->concat.left);
    const char* right_flat = rope_flatten(node->concat.right);

    ycopy_memory(result, left_flat, length_left);
    ycopy_memory(result + length_left, right_flat, length_right);
    node->concat.flattened_cache = result;
    
    return result;
}

void rope_free(RopeNode* node) {
    if (!node) return;

    switch (node->type) {
        case ROPE_LEAF:
            yfree(node->leaf.data, MEMORY_TAG_ROPE);
            break;
        case ROPE_CONCAT:
            rope_free(node->concat.left);
            rope_free(node->concat.right);
            if (node->concat.flattened_cache) {
                yfree(node->concat.flattened_cache, MEMORY_TAG_ROPE);
            }
            break;
    }

    yfree(node, MEMORY_TAG_ROPE);
}

RopeNode* rope_insert_at(RopeNode* root, u64 index, const char* insert_data) {
    u64 insert_length = string_length((char*)insert_data);
    char* flat = rope_flatten(root);
    u64 length = rope_byte_length(root);
    u64 byte_offset = rope_codepoint_to_byte_offset(flat, length, index);

    if (byte_offset > length) byte_offset = length;

    if (byte_offset != 0) {
        RopeNode* left = rope_new_leaf(flat, byte_offset);
        RopeNode* right = rope_new_leaf(flat + byte_offset, length - byte_offset);
        RopeNode* mid = rope_new_leaf(insert_data, insert_length);
        return rope_concat(rope_concat(left, mid), right);
    } else {
        RopeNode* mid = rope_new_leaf(insert_data, insert_length);
        RopeNode* right = rope_new_leaf(flat + byte_offset, length - byte_offset);
        return rope_concat(mid, right);
    }
}

RopeNode* rope_delete_at(RopeNode* root, u64 index, u64 delete_length) {
    char* flat = rope_flatten(root);
    u64 length = rope_byte_length(root);
    u64 start = rope_codepoint_to_byte_offset(flat, length, index);
    u64 end = rope_codepoint_to_byte_offset(flat, length, index + delete_length);
    if (start > length) start = length;
    if (end > length) end = length;

    if (start != 0){
        RopeNode* left = rope_new_leaf(flat, start);
        RopeNode* right = rope_new_leaf(flat + end, length - end);
        return rope_concat(left, right);
    } else {
        RopeNode* right = rope_new_leaf(flat + end, length - end);
        return right;
    }
}

RopeNode* rope_substring(RopeNode* root, u64 start, u64 end) {
    if (start >= end) return rope_new_leaf(NULL, 0);
    char* flat = rope_flatten(root);
    u64 length = rope_length(root);
    if (start > length) start = length;
    if (end > length) end = length;
    return rope_new_leaf(flat + start, end - start);
}

i64 rope_find(RopeNode* root, const char* needle) {
    u64 needle_length = string_length(needle);
    char* haystack = rope_flatten(root);
    u64 haystack_len = rope_length(root);
    for (u64 i = 0; i + needle_length <= haystack_len; ++i) {
        if (ycopy_memory(haystack + i, needle, needle_length) == 0)
            return (i64)i;
    }
    return -1; // not found
}

void rope_split_at(RopeNode* root, u64 index, RopeNode** left_out, RopeNode** right_out) {
    char* flat = rope_flatten(root);
    u64 length = rope_length(root);
    if (index > length) index = length;

    *left_out = rope_new_leaf(flat, index);
    *right_out = rope_new_leaf(flat + index, length - index);
}

RopeNode* rope_replace_at(RopeNode* root, u64 index, u64 length, const char* replacement) {
    RopeNode* temp = rope_delete_at(root, index, length);
    RopeNode* result = rope_insert_at(temp, index, replacement);
    rope_free(temp);
    return result;
}

RopeNode* rope_balance(RopeNode** nodes, u64 count) {
    if (count == 0) return NULL;
    if (count == 1) return nodes[0];
    u64 mid = count / 2;
    RopeNode* left = rope_balance(nodes, mid);
    RopeNode* right = rope_balance(nodes + mid, count - mid);
    return rope_concat(left, right);
}
