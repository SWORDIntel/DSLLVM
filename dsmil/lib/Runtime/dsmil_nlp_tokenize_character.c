/**
 * @file dsmil_nlp_tokenize_character.c
 * @brief Character-Level Tokenizer
 * 
 * Implements character-level tokenization where each character (or Unicode
 * codepoint) becomes a token.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <string.h>
#include <stdint.h>

// UTF-8 decoding with full multi-byte support
static size_t utf8_decode_char_simple(const char *text, size_t text_len, size_t *pos, uint32_t *codepoint) {
    if (!text || !pos || *pos >= text_len || !codepoint) return 0;
    const unsigned char *bytes = (const unsigned char *)text;
    
    if (bytes[*pos] < 0x80) {
        // ASCII character
        *codepoint = bytes[*pos];
        (*pos)++;
        return 1;
    } else if ((bytes[*pos] & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (*pos + 1 >= text_len) return 0;
        *codepoint = ((bytes[*pos] & 0x1F) << 6) | (bytes[*pos + 1] & 0x3F);
        *pos += 2;
        return 2;
    } else if ((bytes[*pos] & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (*pos + 2 >= text_len) return 0;
        *codepoint = ((bytes[*pos] & 0x0F) << 12) | 
                     ((bytes[*pos + 1] & 0x3F) << 6) | 
                     (bytes[*pos + 2] & 0x3F);
        *pos += 3;
        return 3;
    } else if ((bytes[*pos] & 0xF8) == 0xF0) {
        // 4-byte sequence
        if (*pos + 3 >= text_len) return 0;
        *codepoint = ((bytes[*pos] & 0x07) << 18) | 
                     ((bytes[*pos + 1] & 0x3F) << 12) | 
                     ((bytes[*pos + 2] & 0x3F) << 6) | 
                     (bytes[*pos + 3] & 0x3F);
        *pos += 4;
        return 4;
    }
    return 0;  // Invalid UTF-8
}

/**
 * @brief Tokenize using character-level algorithm
 */
int dsmil_tokenize_character(const char *text, size_t text_len,
                             int *tokens, size_t *token_count,
                             size_t max_tokens,
                             dsmil_token_info_t *token_info,
                             bool handle_unicode) {
    if (!text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    size_t pos = 0;
    size_t count = 0;
    
    while (pos < text_len && count < max_tokens) {
        uint32_t codepoint = 0;
        size_t consumed = 0;
        size_t char_start = pos;
        
        if (handle_unicode) {
            consumed = utf8_decode_char_simple(text, text_len, &pos, &codepoint);
        } else {
            // ASCII only
            codepoint = (unsigned char)text[pos];
            consumed = 1;
            pos++;
        }
        
        if (consumed == 0) {
            break;
        }
        
        // Use codepoint as token ID (with offset to avoid special tokens)
        tokens[count] = (int)(codepoint + 10);  // Offset to avoid special tokens 0-9
        
        if (token_info) {
            token_info[count].token_id = tokens[count];
            token_info[count].start_pos = char_start;
            token_info[count].end_pos = pos;
        }
        
        count++;
    }
    
    *token_count = count;
    return 0;
}

