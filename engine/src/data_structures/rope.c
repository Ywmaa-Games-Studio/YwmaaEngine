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
    if (is_empty) return NULL;
    RopeNode* node = (RopeNode*)yallocate_aligned(sizeof(RopeNode), 8, MEMORY_TAG_ROPE);
    if (!node) return NULL;

    node->type = ROPE_LEAF;
    node->ref_count = 1;
    node->leaf.data = (char*)yallocate(length, MEMORY_TAG_ROPE);
    if (!node->leaf.data) {
        yfree(node);
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
    yfree(nodes);
    return balanced;
}


RopeNode* rope_concat(RopeNode* left, RopeNode* right) {
    if (!left) return right;
    if (!right) return left;
    if (!left && !right) return NULL;
    RopeNode* node = (RopeNode*)yallocate_aligned(sizeof(RopeNode), 8, MEMORY_TAG_ROPE);
    if (!node) return NULL;
    node->ref_count = 1;
    node->type = ROPE_CONCAT;
    rope_retain(left);
    node->concat.left = left;
    rope_retain(right);
    node->concat.right = right;
    node->concat.weight = rope_byte_length(left);
    return node;
}

RopeNode* rope_duplicate(RopeNode* node){
    if (!node) return NULL;

    RopeNode* new_node = (RopeNode*)yallocate_aligned(sizeof(RopeNode), 8, MEMORY_TAG_ROPE);
    if (!new_node) return NULL;

    new_node->ref_count = 1;
    new_node->type = node->type;
    if (node->type == ROPE_LEAF) {
        if (!node->leaf.data) return NULL;
        new_node->leaf.data = (char*)yallocate(node->leaf.length, MEMORY_TAG_ROPE);
        ycopy_memory(new_node->leaf.data, node->leaf.data, node->leaf.length);
        new_node->leaf.length = node->leaf.length;
    } else {
        new_node->concat.left = rope_duplicate(node->concat.left);
        new_node->concat.right = rope_duplicate(node->concat.right);
        new_node->concat.weight = node->concat.weight;
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
/* u64 rope_length(RopeNode* node) {
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
} */

// Results from here are null-terminated
// Returns a pointer to the a copied string. The caller is responsible for freeing it.
char* rope_null_terminated_string(RopeNode* node){
    u64 length = rope_byte_length(node);
    char* buffer = (char*)yallocate(length + 1, MEMORY_TAG_ROPE);
    u64 index = 0;
    rope_flatten(node, buffer, &index);
    buffer[length] = '\0';
    return buffer;
}

void rope_flatten(RopeNode* node, char* out, u64* index) {
    if (!node) return;
    switch (node->type) {
        case ROPE_LEAF:
            ycopy_memory(out + *index, node->leaf.data, node->leaf.length);
            *index += node->leaf.length;
            break;
        case ROPE_CONCAT:
            rope_flatten(node->concat.left, out, index);
            rope_flatten(node->concat.right, out, index);
            break;
    }
}

void rope_retain(RopeNode* node){
    if (node) node->ref_count++;
}

void rope_release(RopeNode* node) {
    if (!node) return;
    node->ref_count--;
    if (node->ref_count > 0) return;

    switch (node->type) {
        case ROPE_LEAF:
            yfree(node->leaf.data);
            break;
        case ROPE_CONCAT:
            rope_release(node->concat.left);
            rope_release(node->concat.right);
            break;
    }

    yfree(node);
}

RopeNode* rope_insert_at(RopeNode* rope, u64 index, RopeNode* to_insert) {
    if (!to_insert) return rope_retain(rope), rope;

    if (!rope || index >= rope_byte_length(rope)) {
        return rope_concat(rope, to_insert);
    }

    RopeNode *left = NULL, *right = NULL;
    rope_split_at(rope, index, &left, &right);

    RopeNode* combined = rope_concat(left, to_insert);
    RopeNode* result = rope_concat(combined, right);

    rope_release(left);
    rope_release(right);
    rope_release(combined);

    return result;
}

RopeNode* rope_delete_at(RopeNode* rope, u64 start, u64 delete_length) {
    if (!rope || delete_length == 0 || start >= rope_byte_length(rope))
        return rope;

    RopeNode* left_part = NULL;
    RopeNode* right_part = NULL;

    rope_split_at(rope, start, &left_part, &right_part);

    RopeNode* rest_after_cut = NULL;
    if (right_part) {
        RopeNode* dummy = NULL;
        rope_split_at(right_part, delete_length, &dummy, &rest_after_cut);
        rope_release(dummy);
    }

    RopeNode* result = NULL;
    if (left_part && rest_after_cut) {
        result = rope_concat(left_part, rest_after_cut);
        rope_release(left_part);
        rope_release(rest_after_cut);
    } else if (left_part) {
        result = left_part;
    } else if (rest_after_cut) {
        result = rest_after_cut;
    }

    rope_release(right_part);
    return result;
}

RopeNode* rope_substring(RopeNode* rope, u64 start, u64 length) {
    if (!rope || length == 0 || start >= rope_byte_length(rope))
        return NULL;

    if (rope->type == ROPE_LEAF) {
        u64 rlen = rope->leaf.length;
        if (start >= rlen) return NULL;

        u64 extract_len = (start + length > rlen) ? (rlen - start) : length;
        return rope_new_leaf(rope->leaf.data + start, extract_len);
    }

    u64 left_weight = rope->concat.weight;

    if (start + length <= left_weight) {
        return rope_substring(rope->concat.left, start, length);
    } else if (start >= left_weight) {
        return rope_substring(rope->concat.right, start - left_weight, length);
    } else {
        // straddles left/right
        u64 left_len = left_weight - start;
        RopeNode* left_part = rope_substring(rope->concat.left, start, left_len);
        RopeNode* right_part = rope_substring(rope->concat.right, 0, length - left_len);
        RopeNode* result = rope_concat(left_part, right_part);
        rope_release(left_part);
        rope_release(right_part);
        return result;
    }
}

i64 rope_find(RopeNode* rope, const char* needle) {
    if (!rope || !needle) return -1;
    u64 needle_length = string_length(needle);
    u64 rope_length = rope_byte_length(rope);
    if (needle_length > rope_length) return -1;

    char* flat = rope_null_terminated_string(rope);
    if (!flat) return -1;

    for (u64 i = 0; i <= rope_length - needle_length; ++i) {
        if (strings_nequali(flat + i, needle, needle_length) == 0) {
            yfree(flat);
            return (i64)i;
        }
    }

    yfree(flat);
    return -1;
}

void rope_split_at(RopeNode* node, u64 index, RopeNode** left_out, RopeNode** right_out) {
   if (!node) {
        *left_out = NULL;
        *right_out = NULL;
        return;
    }

    if (node->type == ROPE_LEAF) {
        u64 length = node->leaf.length;
        const char* str = node->leaf.data;

        if (index >= length) {
            *left_out = rope_new_leaf(str, length);
            *right_out = NULL;
        } else if (index == 0) {
            *left_out = NULL;
            *right_out = rope_new_leaf(str, length);
        } else {
            *left_out = rope_new_leaf(str, index);
            *right_out = rope_new_leaf(str + index, length - index);
        }
        return;
    }

    // ROPE_CONCAT
    u64 left_weight = node->concat.weight;

    if (index < left_weight) {
        RopeNode *ll = NULL, *lr = NULL;
        rope_split_at(node->concat.left, index, &ll, &lr);

        RopeNode* new_right = NULL;
        if (lr && node->concat.right) {
            new_right = rope_concat(lr, node->concat.right);
            rope_release(lr);
        } else if (node->concat.right) {
            rope_retain(node->concat.right);
            new_right = node->concat.right;
        }

        *left_out = ll;
        *right_out = new_right;

    } else {
        RopeNode *rl = NULL, *rr = NULL;
        rope_split_at(node->concat.right, index - left_weight, &rl, &rr);

        RopeNode* new_left = NULL;
        if (node->concat.left && rl) {
            new_left = rope_concat(node->concat.left, rl);
            rope_release(rl);
        } else if (node->concat.left) {
            rope_retain(node->concat.left);
            new_left = node->concat.left;
        }

        *left_out = new_left;
        *right_out = rr;
    }
}

RopeNode* rope_replace_at(RopeNode* rope, u64 start, u64 length, RopeNode* replacement) {
    RopeNode* deleted = rope_delete_at(rope, start, length);
    RopeNode* replaced = rope_insert_at(deleted, start, replacement);
    rope_release(deleted);
    return replaced;
}

RopeNode* rope_balance(RopeNode** nodes, u64 count) {
    if (count == 0) return NULL;
    if (count == 1) return nodes[0];
    u64 mid = count / 2;
    RopeNode* left = rope_balance(nodes, mid);
    RopeNode* right = rope_balance(nodes + mid, count - mid);
    return rope_concat(left, right);
}
