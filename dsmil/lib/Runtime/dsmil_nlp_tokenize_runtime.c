/**
 * @file dsmil_nlp_tokenize_runtime.c
 * @brief Main NLP Tokenization Implementation
 * 
 * Implements the main tokenization API, dispatches to appropriate algorithms,
 * and manages context, vocabulary, and caching.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declarations from tokenization modules
extern int dsmil_tokenize_whitespace(const char *text, size_t text_len,
                                     int *tokens, size_t *token_count,
                                     size_t max_tokens,
                                     dsmil_token_info_t *token_info,
                                     bool lowercase, bool remove_punctuation,
                                     bool handle_unicode);

extern int dsmil_tokenize_character(const char *text, size_t text_len,
                                    int *tokens, size_t *token_count,
                                    size_t max_tokens,
                                    dsmil_token_info_t *token_info,
                                    bool handle_unicode);

extern int dsmil_tokenize_bpe(const char *text, size_t text_len,
                               int *tokens, size_t *token_count,
                               size_t max_tokens,
                               dsmil_token_info_t *token_info,
                               void *bpe_ctx, void *vocab_ctx);

extern int dsmil_tokenize_wordpiece(const char *text, size_t text_len,
                                     int *tokens, size_t *token_count,
                                     size_t max_tokens,
                                     dsmil_token_info_t *token_info,
                                     void *vocab_ctx, const char *subword_prefix);

extern int dsmil_tokenize_sentencepiece(const char *text, size_t text_len,
                                         int *tokens, size_t *token_count,
                                         size_t max_tokens,
                                         dsmil_token_info_t *token_info,
                                         void *sp_model);

// Forward declarations from cache module
struct tokenize_cache;
typedef struct tokenize_cache tokenize_cache_t;
extern tokenize_cache_t *tokenize_cache_create(size_t max_size);
extern int tokenize_cache_lookup(tokenize_cache_t *cache, const char *text, size_t text_len,
                                 int **tokens, size_t *token_count);
extern int tokenize_cache_insert(tokenize_cache_t *cache, const char *text, size_t text_len,
                                 const int *tokens, size_t token_count);
extern void tokenize_cache_clear(tokenize_cache_t *cache);
extern void tokenize_cache_destroy(tokenize_cache_t *cache);

// Vocabulary functions - defined in vocab_runtime.c
typedef struct vocabulary vocabulary_t;
typedef struct {
    char *token;
    size_t token_len;
    int token_id;
    uint32_t frequency;
} vocab_entry_t;
extern vocabulary_t *vocab_create(void);
extern int vocab_load_text(vocabulary_t *vocab, const char *vocab_path);
extern int vocab_load_json(vocabulary_t *vocab, const char *vocab_path);
extern int vocab_add_token(vocabulary_t *vocab, const char *token, size_t token_len, int *token_id);
extern vocab_entry_t *vocab_find_token(vocabulary_t *vocab, const char *token, size_t token_len);
extern void vocab_destroy(vocabulary_t *vocab);

/**
 * @brief Tokenizer context structure
 */
struct dsmil_tokenizer_ctx {
    dsmil_tokenize_options_t options;
    vocabulary_t *vocab;
    tokenize_cache_t *cache;
    void *algorithm_ctx;  // Algorithm-specific context (BPE, WordPiece, etc.)
    uint64_t total_tokens;
    uint64_t cache_hits;
    uint64_t cache_misses;
};

/**
 * @brief Initialize tokenization context
 */
int dsmil_tokenizer_init(const char *vocab_path,
                        const dsmil_tokenize_options_t *options,
                        dsmil_tokenizer_ctx_t **ctx) {
    if (!ctx) {
        return -EINVAL;
    }
    
    dsmil_tokenizer_ctx_t *new_ctx = calloc(1, sizeof(dsmil_tokenizer_ctx_t));
    if (!new_ctx) {
        return -ENOMEM;
    }
    
    // Set default options
    if (options) {
        new_ctx->options = *options;
    } else {
        new_ctx->options.algorithm = DSMIL_TOKENIZE_WHITESPACE;
        new_ctx->options.lowercase = false;
        new_ctx->options.remove_punctuation = false;
        new_ctx->options.handle_unicode = true;
        new_ctx->options.add_special_tokens = false;
        new_ctx->options.max_tokens = 1024;
        new_ctx->options.max_token_length = 256;
        new_ctx->options.use_cache = true;
        new_ctx->options.cache_size = 1000;
    }
    
    // Create vocabulary if path provided
    if (vocab_path || (options && options->vocab_path)) {
        new_ctx->vocab = vocab_create();
        if (new_ctx->vocab) {
            const char *path = vocab_path ? vocab_path : (options ? options->vocab_path : NULL);
            if (path) {
                // Try to load vocabulary (assume text format)
                vocab_load_text(new_ctx->vocab, path);
            }
        }
    } else {
        new_ctx->vocab = NULL;
    }
    
    // Create cache if enabled
    if (new_ctx->options.use_cache) {
        new_ctx->cache = tokenize_cache_create(new_ctx->options.cache_size);
    }
    
    *ctx = new_ctx;
    return 0;
}

/**
 * @brief Tokenize text
 */
int dsmil_nlp_tokenize(dsmil_tokenizer_ctx_t *ctx,
                       const char *text,
                       size_t text_len,
                       int *tokens,
                       size_t *token_count,
                       size_t max_tokens,
                       dsmil_token_info_t *token_info) {
    if (!ctx || !text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    if (text_len == 0) {
        *token_count = 0;
        return 0;
    }
    
    // Check cache first
    if (ctx->cache) {
        int *cached_tokens = NULL;
        size_t cached_count = 0;
        if (tokenize_cache_lookup(ctx->cache, text, text_len, &cached_tokens, &cached_count) == 0) {
            // Cache hit
            size_t copy_count = (cached_count < max_tokens) ? cached_count : max_tokens;
            memcpy(tokens, cached_tokens, copy_count * sizeof(int));
            *token_count = copy_count;
            ctx->cache_hits++;
            ctx->total_tokens += copy_count;
            return 0;
        }
        ctx->cache_misses++;
    }
    
    // Determine actual max tokens
    size_t actual_max = max_tokens;
    if (ctx->options.max_tokens > 0 && ctx->options.max_tokens < max_tokens) {
        actual_max = ctx->options.max_tokens;
    }
    
    // Dispatch to appropriate algorithm
    int ret = -ENOSYS;
    size_t count = 0;
    
    switch (ctx->options.algorithm) {
        case DSMIL_TOKENIZE_WHITESPACE:
            ret = dsmil_tokenize_whitespace(text, text_len, tokens, &count, actual_max,
                                           token_info, ctx->options.lowercase,
                                           ctx->options.remove_punctuation,
                                           ctx->options.handle_unicode);
            break;
            
        case DSMIL_TOKENIZE_CHARACTER:
            ret = dsmil_tokenize_character(text, text_len, tokens, &count, actual_max,
                                          token_info, ctx->options.handle_unicode);
            break;
            
        case DSMIL_TOKENIZE_BPE:
            ret = dsmil_tokenize_bpe(text, text_len, tokens, &count, actual_max,
                                    token_info, ctx->algorithm_ctx, ctx->vocab);
            break;
            
        case DSMIL_TOKENIZE_WORDPIECE:
            ret = dsmil_tokenize_wordpiece(text, text_len, tokens, &count, actual_max,
                                          token_info, ctx->vocab, "##");
            break;
            
        case DSMIL_TOKENIZE_SENTENCEPIECE:
            ret = dsmil_tokenize_sentencepiece(text, text_len, tokens, &count, actual_max,
                                               token_info, ctx->algorithm_ctx);
            break;
            
        default:
            // Fallback to whitespace
            ret = dsmil_tokenize_whitespace(text, text_len, tokens, &count, actual_max,
                                           token_info, ctx->options.lowercase,
                                           ctx->options.remove_punctuation,
                                           ctx->options.handle_unicode);
            break;
    }
    
    if (ret != 0) {
        *token_count = 0;
        return ret;
    }
    
    // Add special tokens if requested
    if (ctx->options.add_special_tokens && count < actual_max) {
        // Shift tokens to make room for BOS
        if (count > 0) {
            memmove(tokens + 1, tokens, count * sizeof(int));
            if (token_info) {
                memmove(token_info + 1, token_info, count * sizeof(dsmil_token_info_t));
            }
        }
        tokens[0] = DSMIL_TOKEN_BOS;
        if (token_info) {
            token_info[0].token_id = DSMIL_TOKEN_BOS;
            token_info[0].start_pos = 0;
            token_info[0].end_pos = 0;
        }
        count++;
        
        // Add EOS if space available
        if (count < actual_max) {
            tokens[count] = DSMIL_TOKEN_EOS;
            if (token_info) {
                token_info[count].token_id = DSMIL_TOKEN_EOS;
                token_info[count].start_pos = text_len;
                token_info[count].end_pos = text_len;
            }
            count++;
        }
    }
    
    *token_count = count;
    ctx->total_tokens += count;
    
    // Cache result
    if (ctx->cache && count > 0) {
        tokenize_cache_insert(ctx->cache, text, text_len, tokens, count);
    }
    
    return 0;
}

/**
 * @brief Detokenize (convert tokens back to text)
 */
int dsmil_nlp_detokenize(dsmil_tokenizer_ctx_t *ctx,
                        const int *tokens,
                        size_t token_count,
                        char *text,
                        size_t *text_len,
                        bool add_spaces) {
    if (!ctx || !tokens || !text || !text_len) {
        return -EINVAL;
    }
    
    size_t pos = 0;
    
    for (size_t i = 0; i < token_count && pos < *text_len - 1; i++) {
        // Skip special tokens
        if (tokens[i] == DSMIL_TOKEN_BOS || tokens[i] == DSMIL_TOKEN_EOS ||
            tokens[i] == DSMIL_TOKEN_PAD || tokens[i] == DSMIL_TOKEN_CLS ||
            tokens[i] == DSMIL_TOKEN_SEP) {
            continue;
        }
        
        // Get token text from vocabulary
        const char *token_text = NULL;
        size_t token_text_len = 0;
        
        if (ctx->vocab) {
            // Look up token in vocabulary by ID
            vocabulary_t *vocab = (vocabulary_t *)ctx->vocab;
            for (size_t j = 0; j < vocab->size; j++) {
                vocab_entry_t *entry = &vocab->entries[j];
                if (entry->token_id == tokens[i]) {
                    token_text = entry->token;
                    token_text_len = entry->token_len;
                    break;
                }
            }
        }
        
        // Fallback: use token ID as character if not found in vocabulary
        if (!token_text && tokens[i] > 10) {  // Skip special tokens
            static char c;
            c = (char)(tokens[i] - 10);
            if (c >= 32 && c < 127) {  // Printable ASCII
                token_text = &c;
                token_text_len = 1;
            }
        }
        
        if (token_text && token_text_len > 0) {
            if (add_spaces && pos > 0 && text[pos - 1] != ' ') {
                if (pos < *text_len - 1) {
                    text[pos++] = ' ';
                }
            }
            
            size_t copy_len = (token_text_len < *text_len - pos) ? token_text_len : *text_len - pos - 1;
            memcpy(text + pos, token_text, copy_len);
            pos += copy_len;
        }
    }
    
    text[pos] = '\0';
    *text_len = pos;
    
    return 0;
}

/**
 * @brief Load vocabulary from file
 */
int dsmil_tokenizer_load_vocab(dsmil_tokenizer_ctx_t *ctx,
                              const char *vocab_path,
                              const char *format) {
    if (!ctx || !vocab_path) {
        return -EINVAL;
    }
    
    if (!ctx->vocab) {
        ctx->vocab = vocab_create();
        if (!ctx->vocab) {
            return -ENOMEM;
        }
    }
    
    if (!format || strcmp(format, "txt") == 0) {
        return vocab_load_text(ctx->vocab, vocab_path);
    } else if (strcmp(format, "json") == 0) {
        return vocab_load_json(ctx->vocab, vocab_path);
    }
    
    return -EINVAL;
}

/**
 * @brief Add token to vocabulary
 */
int dsmil_tokenizer_add_token(dsmil_tokenizer_ctx_t *ctx,
                              const char *token,
                              size_t token_len,
                              int *token_id) {
    if (!ctx || !token || token_len == 0) {
        return -EINVAL;
    }
    
    if (!ctx->vocab) {
        ctx->vocab = vocab_create();
        if (!ctx->vocab) {
            return -ENOMEM;
        }
    }
    
    return vocab_add_token(ctx->vocab, token, token_len, token_id);
}

/**
 * @brief Get token text from ID
 */
int dsmil_tokenizer_get_token_text(dsmil_tokenizer_ctx_t *ctx,
                                  int token_id,
                                  const char **token_text,
                                  size_t *token_text_len) {
    if (!ctx || !token_text || !token_text_len) {
        return -EINVAL;
    }
    
    // Look up token in vocabulary by ID
    if (ctx->vocab) {
        vocabulary_t *vocab = (vocabulary_t *)ctx->vocab;
        for (size_t i = 0; i < vocab->size; i++) {
            vocab_entry_t *entry = &vocab->entries[i];
            if (entry->token_id == token_id) {
                *token_text = entry->token;
                *token_text_len = entry->token_len;
                return 0;
            }
        }
    }
    
    // Fallback: use token ID as character for special tokens
    if (token_id >= 0 && token_id <= 9) {
        // Special tokens
        static const char *special_tokens[] = {
            "[PAD]", "[UNK]", "[BOS]", "[EOS]", "[CLS]", "[SEP]", "[MASK]"
        };
        if (token_id < 7) {
            *token_text = special_tokens[token_id];
            *token_text_len = strlen(special_tokens[token_id]);
            return 0;
        }
    }
    
    return -ENOENT;
}

/**
 * @brief Clear tokenization cache
 */
void dsmil_tokenizer_clear_cache(dsmil_tokenizer_ctx_t *ctx) {
    if (ctx && ctx->cache) {
        tokenize_cache_clear(ctx->cache);
    }
}

/**
 * @brief Get tokenization statistics
 */
void dsmil_tokenizer_get_stats(dsmil_tokenizer_ctx_t *ctx,
                               uint64_t *total_tokens,
                               uint64_t *cache_hits,
                               uint64_t *cache_misses) {
    if (!ctx) {
        return;
    }
    
    if (total_tokens) {
        *total_tokens = ctx->total_tokens;
    }
    if (cache_hits) {
        *cache_hits = ctx->cache_hits;
    }
    if (cache_misses) {
        *cache_misses = ctx->cache_misses;
    }
}

/**
 * @brief Cleanup tokenization context
 */
void dsmil_tokenizer_cleanup(dsmil_tokenizer_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->vocab) {
        vocab_destroy(ctx->vocab);
    }
    
    if (ctx->cache) {
        tokenize_cache_destroy(ctx->cache);
    }
    
    if (ctx->algorithm_ctx) {
        free(ctx->algorithm_ctx);
    }
    
    free(ctx);
}

