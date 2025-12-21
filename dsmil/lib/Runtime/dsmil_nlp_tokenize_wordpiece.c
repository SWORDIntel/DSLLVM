/**
 * @file dsmil_nlp_tokenize_wordpiece.c
 * @brief WordPiece Tokenizer
 * 
 * Implements WordPiece algorithm used by BERT and similar models. WordPiece
 * uses longest-match-first tokenization with subword prefixes.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

// Forward declaration
typedef struct vocabulary vocabulary_t;
typedef struct vocab_entry vocab_entry_t;
extern vocab_entry_t *vocab_find_token(vocabulary_t *vocab, const char *token, size_t token_len);

/**
 * @brief Check if token exists in vocabulary
 */
static bool wordpiece_vocab_contains(void *vocab_ctx, const char *token, size_t token_len) {
    if (!vocab_ctx || !token || token_len == 0) {
        return false;
    }
    
    vocabulary_t *vocab = (vocabulary_t *)vocab_ctx;
    vocab_entry_t *entry = vocab_find_token(vocab, token, token_len);
    return (entry != NULL);
}

/**
 * @brief Find longest matching token from vocabulary
 */
static size_t wordpiece_find_longest_match(void *vocab_ctx, const char *text, 
                                           size_t text_len, size_t start_pos,
                                           size_t *match_len) {
    if (!text || !match_len || start_pos >= text_len) {
        return 0;
    }
    
    // Longest-match-first algorithm: try progressively shorter substrings
    size_t max_len = text_len - start_pos;
    if (max_len > 100) max_len = 100;  // Limit search
    
    for (size_t len = max_len; len > 0; len--) {
        if (wordpiece_vocab_contains(vocab_ctx, text + start_pos, len)) {
            *match_len = len;
            return start_pos;
        }
    }
    
    // No match found, return single character
    *match_len = 1;
    return start_pos;
}

/**
 * @brief Tokenize using WordPiece algorithm
 */
int dsmil_tokenize_wordpiece(const char *text, size_t text_len,
                             int *tokens, size_t *token_count,
                             size_t max_tokens,
                             dsmil_token_info_t *token_info,
                             void *vocab_ctx,
                             const char *subword_prefix) {
    if (!text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    size_t pos = 0;
    size_t count = 0;
    
    // First, split into words
    size_t word_start = 0;
    bool in_word = false;
    
    while (pos < text_len && count < max_tokens) {
        char c = text[pos];
        bool is_ws = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        
        if (is_ws) {
            if (in_word) {
                // Tokenize word using WordPiece
                size_t word_len = pos - word_start;
                size_t word_pos = 0;
                
                while (word_pos < word_len && count < max_tokens) {
                    size_t match_len = 0;
                    size_t match_start = wordpiece_find_longest_match(vocab_ctx, 
                                                                      text + word_start,
                                                                      word_len, 
                                                                      word_pos,
                                                                      &match_len);
                    
                    // Generate token ID
                    uint32_t hash = 5381;
                    for (size_t i = 0; i < match_len; i++) {
                        hash = ((hash << 5) + hash) + (unsigned char)text[word_start + match_start + i];
                    }
                    
                    tokens[count] = (int)hash;
                    if (token_info) {
                        token_info[count].token_id = (int)hash;
                        token_info[count].start_pos = word_start + match_start;
                        token_info[count].end_pos = word_start + match_start + match_len;
                    }
                    count++;
                    
                    word_pos += match_len;
                }
                
                in_word = false;
            }
        } else {
            if (!in_word) {
                word_start = pos;
                in_word = true;
            }
        }
        
        pos++;
    }
    
    // Handle final word
    if (in_word && count < max_tokens) {
        size_t word_len = pos - word_start;
        size_t word_pos = 0;
        
        while (word_pos < word_len && count < max_tokens) {
            size_t match_len = 0;
            size_t match_start = wordpiece_find_longest_match(vocab_ctx,
                                                              text + word_start,
                                                              word_len,
                                                              word_pos,
                                                              &match_len);
            
            uint32_t hash = 5381;
            for (size_t i = 0; i < match_len; i++) {
                hash = ((hash << 5) + hash) + (unsigned char)text[word_start + match_start + i];
            }
            
            tokens[count] = (int)hash;
            if (token_info) {
                token_info[count].token_id = (int)hash;
                token_info[count].start_pos = word_start + match_start;
                token_info[count].end_pos = word_start + match_start + match_len;
            }
            count++;
            
            word_pos += match_len;
        }
    }
    
    *token_count = count;
    return 0;
}

