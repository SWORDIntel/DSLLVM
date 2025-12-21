/**
 * @file dsmil_nlp_tokenize_whitespace.c
 * @brief Advanced Whitespace Tokenizer
 * 
 * Implements sophisticated whitespace-based tokenization with punctuation
 * handling, number detection, and position tracking.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

// Unicode functions - inline implementations for tokenization
static size_t utf8_decode_char_simple(const char *text, size_t text_len, size_t *pos, uint32_t *codepoint) {
    if (!text || !pos || *pos >= text_len || !codepoint) return 0;
    const unsigned char *bytes = (const unsigned char *)text;
    if (bytes[*pos] < 0x80) {
        *codepoint = bytes[*pos];
        (*pos)++;
        return 1;
    }
    // Full UTF-8 multi-byte support
    if (bytes[*pos] < 0x80) {
        *codepoint = bytes[*pos];
        (*pos)++;
        return 1;
    } else if ((bytes[*pos] & 0xE0) == 0xC0 && *pos + 1 < text_len) {
        *codepoint = ((bytes[*pos] & 0x1F) << 6) | (bytes[*pos + 1] & 0x3F);
        *pos += 2;
        return 2;
    } else if ((bytes[*pos] & 0xF0) == 0xE0 && *pos + 2 < text_len) {
        *codepoint = ((bytes[*pos] & 0x0F) << 12) | 
                     ((bytes[*pos + 1] & 0x3F) << 6) | 
                     (bytes[*pos + 2] & 0x3F);
        *pos += 3;
        return 3;
    } else if ((bytes[*pos] & 0xF8) == 0xF0 && *pos + 3 < text_len) {
        *codepoint = ((bytes[*pos] & 0x07) << 18) | 
                     ((bytes[*pos + 1] & 0x3F) << 12) | 
                     ((bytes[*pos + 2] & 0x3F) << 6) | 
                     (bytes[*pos + 3] & 0x3F);
        *pos += 4;
        return 4;
    }
    return 0;  // Invalid UTF-8
}

static bool is_whitespace_simple(uint32_t cp) {
    return (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r');
}

static bool is_punctuation_simple(uint32_t cp) {
    return ((cp >= '!' && cp <= '/') || (cp >= ':' && cp <= '@') ||
            (cp >= '[' && cp <= '`') || (cp >= '{' && cp <= '~'));
}

static bool is_letter_simple(uint32_t cp) {
    return ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z'));
}

static bool is_digit_simple(uint32_t cp) {
    return (cp >= '0' && cp <= '9');
}

/**
 * @brief Tokenize using whitespace algorithm
 */
int dsmil_tokenize_whitespace(const char *text, size_t text_len,
                              int *tokens, size_t *token_count,
                              size_t max_tokens,
                              dsmil_token_info_t *token_info,
                              bool lowercase, bool remove_punctuation,
                              bool handle_unicode) {
    if (!text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    size_t pos = 0;
    size_t count = 0;
    size_t token_start = 0;
    bool in_token = false;
    
    // Hash function for token ID generation
    uint32_t token_hash = 5381;
    
    while (pos < text_len && count < max_tokens) {
        uint32_t codepoint = 0;
        size_t consumed = 0;
        
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
        
        bool is_ws = is_whitespace_simple(codepoint);
        bool is_punct = is_punctuation_simple(codepoint);
        bool is_alpha = is_letter_simple(codepoint);
        bool is_digit = is_digit_simple(codepoint);
        
        if (is_ws) {
            if (in_token) {
                // End of token
                if (count < max_tokens) {
                    tokens[count] = (int)token_hash;
                    if (token_info) {
                        token_info[count].token_id = (int)token_hash;
                        token_info[count].start_pos = token_start;
                        token_info[count].end_pos = pos - consumed;
                    }
                    count++;
                }
                in_token = false;
                token_hash = 5381;
            }
        } else if (remove_punctuation && is_punct) {
            // Skip punctuation
            if (in_token) {
                // End current token
                if (count < max_tokens) {
                    tokens[count] = (int)token_hash;
                    if (token_info) {
                        token_info[count].token_id = (int)token_hash;
                        token_info[count].start_pos = token_start;
                        token_info[count].end_pos = pos - consumed;
                    }
                    count++;
                }
                in_token = false;
                token_hash = 5381;
            }
        } else {
            // Token character
            if (!in_token) {
                token_start = pos - consumed;
                in_token = true;
            }
            
            // Update hash
            uint32_t char_val = codepoint;
            if (lowercase && is_alpha && char_val >= 'A' && char_val <= 'Z') {
                char_val = char_val - 'A' + 'a';
            }
            token_hash = ((token_hash << 5) + token_hash) + char_val;
        }
    }
    
    // Handle final token
    if (in_token && count < max_tokens) {
        tokens[count] = (int)token_hash;
        if (token_info) {
            token_info[count].token_id = (int)token_hash;
            token_info[count].start_pos = token_start;
            token_info[count].end_pos = pos;
        }
        count++;
    }
    
    *token_count = count;
    return 0;
}

