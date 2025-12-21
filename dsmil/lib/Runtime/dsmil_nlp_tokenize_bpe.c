/**
 * @file dsmil_nlp_tokenize_bpe.c
 * @brief Byte Pair Encoding (BPE) Tokenizer
 * 
 * Implements BPE algorithm for subword tokenization. BPE iteratively merges
 * the most frequent pairs of bytes/subwords until a desired vocabulary size.
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

/**
 * @brief BPE merge rule
 */
typedef struct {
    char *pair;
    size_t pair_len;
    char *merged;
    size_t merged_len;
    uint32_t priority;  // Lower = higher priority
} bpe_merge_rule_t;

/**
 * @brief BPE context
 */
typedef struct {
    bpe_merge_rule_t *rules;
    size_t num_rules;
    size_t capacity;
} bpe_context_t;

/**
 * @brief BPE subword segment
 */
typedef struct {
    char *text;
    size_t len;
    struct bpe_segment *next;
} bpe_segment_t;

/**
 * @brief Apply BPE merges to word
 */
static int bpe_apply_merges(const char *word, size_t word_len,
                           bpe_context_t *bpe_ctx,
                           char *output, size_t *output_len) {
    if (!word || word_len == 0 || !output || !output_len) {
        return -EINVAL;
    }
    
    // Initialize word as sequence of characters (subwords)
    bpe_segment_t *segments = NULL;
    bpe_segment_t *last = NULL;
    
    // Create initial segments (one per character)
    for (size_t i = 0; i < word_len; i++) {
        bpe_segment_t *seg = calloc(1, sizeof(bpe_segment_t));
        if (!seg) {
            // Cleanup on error
            while (segments) {
                bpe_segment_t *next = segments->next;
                free(segments->text);
                free(segments);
                segments = next;
            }
            return -ENOMEM;
        }
        seg->text = malloc(2);
        if (!seg->text) {
            free(seg);
            while (segments) {
                bpe_segment_t *next = segments->next;
                free(segments->text);
                free(segments);
                segments = next;
            }
            return -ENOMEM;
        }
        seg->text[0] = word[i];
        seg->text[1] = '\0';
        seg->len = 1;
        
        if (last) {
            last->next = seg;
        } else {
            segments = seg;
        }
        last = seg;
    }
    
    // Apply merge rules in priority order
    if (bpe_ctx && bpe_ctx->rules) {
        for (size_t r = 0; r < bpe_ctx->num_rules; r++) {
            bpe_merge_rule_t *rule = &bpe_ctx->rules[r];
            
            // Find all occurrences of the pair and merge them
            bpe_segment_t *prev = NULL;
            bpe_segment_t *curr = segments;
            
            while (curr && curr->next) {
                // Check if current segment + next segment matches the pair
                size_t pair_len = curr->len + curr->next->len;
                if (pair_len == rule->pair_len) {
                    char *combined = malloc(pair_len + 1);
                    if (combined) {
                        memcpy(combined, curr->text, curr->len);
                        memcpy(combined + curr->len, curr->next->text, curr->next->len);
                        combined[pair_len] = '\0';
                        
                        if (memcmp(combined, rule->pair, rule->pair_len) == 0) {
                            // Merge segments
                            free(combined);
                            free(curr->text);
                            free(curr->next->text);
                            
                            curr->text = malloc(rule->merged_len + 1);
                            if (curr->text) {
                                memcpy(curr->text, rule->merged, rule->merged_len);
                                curr->text[rule->merged_len] = '\0';
                                curr->len = rule->merged_len;
                                
                                // Remove next segment
                                bpe_segment_t *to_remove = curr->next;
                                curr->next = to_remove->next;
                                if (to_remove == last) {
                                    last = curr;
                                }
                                free(to_remove);
                                continue;
                            }
                        } else {
                            free(combined);
                        }
                    }
                }
                
                prev = curr;
                curr = curr->next;
            }
        }
    }
    
    // Convert segments to output string with spaces between subwords
    size_t out_pos = 0;
    bpe_segment_t *seg = segments;
    bool first = true;
    while (seg && out_pos < *output_len - 1) {
        if (!first && out_pos < *output_len - 1) {
            output[out_pos++] = ' ';  // Add space between subwords
        }
        first = false;
        
        size_t copy_len = (seg->len < *output_len - out_pos - 1) ? seg->len : *output_len - out_pos - 1;
        memcpy(output + out_pos, seg->text, copy_len);
        out_pos += copy_len;
        seg = seg->next;
    }
    output[out_pos] = '\0';
    *output_len = out_pos;
    
    // Cleanup segments
    while (segments) {
        bpe_segment_t *next = segments->next;
        free(segments->text);
        free(segments);
        segments = next;
    }
    
    return 0;
}

/**
 * @brief Tokenize using BPE algorithm
 */
int dsmil_tokenize_bpe(const char *text, size_t text_len,
                       int *tokens, size_t *token_count,
                       size_t max_tokens,
                       dsmil_token_info_t *token_info,
                       bpe_context_t *bpe_ctx,
                       void *vocab_ctx) {
    if (!text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    // First, split into words using whitespace
    size_t pos = 0;
    size_t count = 0;
    size_t word_start = 0;
    bool in_word = false;
    
    while (pos < text_len && count < max_tokens) {
        char c = text[pos];
        bool is_ws = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        
        if (is_ws) {
            if (in_word) {
                // Process word with BPE
                size_t word_len = pos - word_start;
                char word_buf[256];
                size_t word_buf_len = sizeof(word_buf);
                
                if (word_len < sizeof(word_buf)) {
                    memcpy(word_buf, text + word_start, word_len);
                    word_buf[word_len] = '\0';
                    
                    // Apply BPE merges
                    char bpe_output[256];
                    size_t bpe_len = sizeof(bpe_output);
                    bpe_apply_merges(word_buf, word_len, bpe_ctx, bpe_output, &bpe_len);
                    
                    // Generate token IDs from BPE subwords
                    // Split BPE output into subwords and tokenize each
                    size_t subword_start = 0;
                    while (subword_start < bpe_len && count < max_tokens) {
                        // Find subword boundary (space or end)
                        size_t subword_end = subword_start;
                        while (subword_end < bpe_len && bpe_output[subword_end] != ' ' && 
                               bpe_output[subword_end] != '\0') {
                            subword_end++;
                        }
                        
                        if (subword_end > subword_start) {
                            // Generate token ID for this subword
                            uint32_t hash = 5381;
                            for (size_t i = subword_start; i < subword_end; i++) {
                                hash = ((hash << 5) + hash) + (unsigned char)bpe_output[i];
                            }
                            
                            tokens[count] = (int)hash;
                            if (token_info) {
                                token_info[count].token_id = (int)hash;
                                token_info[count].start_pos = word_start + subword_start;
                                token_info[count].end_pos = word_start + subword_end;
                            }
                            count++;
                        }
                        
                        subword_start = subword_end;
                        if (subword_start < bpe_len && bpe_output[subword_start] == ' ') {
                            subword_start++;  // Skip space
                        }
                    }
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
        if (word_len < 256) {
            char word_buf[256];
            memcpy(word_buf, text + word_start, word_len);
            word_buf[word_len] = '\0';
            
            char bpe_output[256];
            size_t bpe_len = sizeof(bpe_output);
            bpe_apply_merges(word_buf, word_len, bpe_ctx, bpe_output, &bpe_len);
            
            // Split BPE output into subwords and tokenize each
            size_t subword_start = 0;
            while (subword_start < bpe_len && count < max_tokens) {
                size_t subword_end = subword_start;
                while (subword_end < bpe_len && bpe_output[subword_end] != ' ' && 
                       bpe_output[subword_end] != '\0') {
                    subword_end++;
                }
                
                if (subword_end > subword_start) {
                    uint32_t hash = 5381;
                    for (size_t i = subword_start; i < subword_end; i++) {
                        hash = ((hash << 5) + hash) + (unsigned char)bpe_output[i];
                    }
                    
                    tokens[count] = (int)hash;
                    if (token_info) {
                        token_info[count].token_id = (int)hash;
                        token_info[count].start_pos = word_start + subword_start;
                        token_info[count].end_pos = word_start + subword_end;
                    }
                    count++;
                }
                
                subword_start = subword_end;
                if (subword_start < bpe_len && bpe_output[subword_start] == ' ') {
                    subword_start++;
                }
            }
        }
    }
    
    *token_count = count;
    return 0;
}

