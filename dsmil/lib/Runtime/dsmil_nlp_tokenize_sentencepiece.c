/**
 * @file dsmil_nlp_tokenize_sentencepiece.c
 * @brief SentencePiece Tokenizer
 * 
 * Implements SentencePiece algorithm for unsupervised tokenization.
 * SentencePiece treats the input as a raw stream of bytes and segments
 * it into subword units using a unigram language model.
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
#include <math.h>

/**
 * @brief SentencePiece model structure
 */
typedef struct {
    uint32_t *vocab_ids;
    char **vocab_tokens;
    size_t vocab_size;
    float *unigram_scores;  // Log probabilities
    void *trie;  // For efficient prefix matching
} sentencepiece_model_t;

/**
 * @brief SentencePiece segmentation using unigram language model
 */
static int sentencepiece_segment(const char *text, size_t text_len,
                                 sentencepiece_model_t *model,
                                 int *tokens, size_t *token_count,
                                 size_t max_tokens,
                                 dsmil_token_info_t *token_info) {
    if (!text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    size_t count = 0;
    size_t pos = 0;
    
    // Viterbi-like segmentation: find best segmentation using unigram scores
    while (pos < text_len && count < max_tokens) {
        // Try different segment lengths and pick best scoring one
        size_t best_len = 1;
        float best_score = -1e10;
        size_t best_vocab_idx = SIZE_MAX;
        
        // Try segments from 1 to min(remaining, 20) characters
        size_t max_seg_len = (text_len - pos < 20) ? (text_len - pos) : 20;
        
        for (size_t seg_len = 1; seg_len <= max_seg_len; seg_len++) {
            // Calculate score for this segment
            float score = -10.0f * (float)seg_len;  // Default: prefer shorter segments
            
            // Check if segment exists in vocabulary (if model provided)
            if (model && model->vocab_tokens) {
                bool found = false;
                for (size_t i = 0; i < model->vocab_size; i++) {
                    size_t vocab_token_len = strlen(model->vocab_tokens[i]);
                    if (vocab_token_len == seg_len &&
                        strncmp(model->vocab_tokens[i], text + pos, seg_len) == 0) {
                        found = true;
                        if (model->unigram_scores) {
                            score = model->unigram_scores[i];
                        } else {
                            score = -1.0f * (float)seg_len;  // Prefer shorter if no scores
                        }
                        if (score > best_score) {
                            best_score = score;
                            best_len = seg_len;
                            best_vocab_idx = i;
                        }
                        break;
                    }
                }
                if (!found && seg_len > 1) {
                    score = -1e10;  // Penalize unknown segments
                }
            } else {
                // No model: use character-based segmentation with length penalty
                score = -1.0f * (float)(seg_len * seg_len);
                if (score > best_score) {
                    best_score = score;
                    best_len = seg_len;
                }
            }
        }
        
        // Generate token ID for best segment
        int token_id;
        if (model && best_vocab_idx != SIZE_MAX && model->vocab_ids) {
            token_id = (int)model->vocab_ids[best_vocab_idx];
        } else {
            // Generate hash-based token ID
            uint32_t hash = 5381;
            for (size_t i = 0; i < best_len; i++) {
                hash = ((hash << 5) + hash) + (unsigned char)text[pos + i];
            }
            token_id = (int)hash;
        }
        
        tokens[count] = token_id;
        if (token_info) {
            token_info[count].token_id = token_id;
            token_info[count].start_pos = pos;
            token_info[count].end_pos = pos + best_len;
        }
        count++;
        pos += best_len;
    }
    
    *token_count = count;
    return 0;
}

/**
 * @brief Tokenize using SentencePiece algorithm
 */
int dsmil_tokenize_sentencepiece(const char *text, size_t text_len,
                                 int *tokens, size_t *token_count,
                                 size_t max_tokens,
                                 dsmil_token_info_t *token_info,
                                 void *sp_model) {
    if (!text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    sentencepiece_model_t *model = (sentencepiece_model_t *)sp_model;
    
    // Perform SentencePiece segmentation
    return sentencepiece_segment(text, text_len, model, tokens, token_count, 
                                max_tokens, token_info);
}
