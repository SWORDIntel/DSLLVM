/**
 * @file dsmil_nlp_tokenize.h
 * @brief Advanced NLP Tokenization System for DSMIL
 * 
 * Production-grade tokenization with multiple algorithms, vocabulary
 * management, caching, and Unicode support.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_NLP_TOKENIZE_H
#define DSMIL_NLP_TOKENIZE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup DSMIL_NLP_TOKENIZE NLP Tokenization System
 * @{
 */

/**
 * @brief Tokenization algorithm types
 */
typedef enum {
    DSMIL_TOKENIZE_WHITESPACE,      // Simple whitespace splitting
    DSMIL_TOKENIZE_BPE,             // Byte Pair Encoding
    DSMIL_TOKENIZE_WORDPIECE,        // WordPiece (BERT-style)
    DSMIL_TOKENIZE_SENTENCEPIECE,   // SentencePiece (T5-style)
    DSMIL_TOKENIZE_CHARACTER,       // Character-level
    DSMIL_TOKENIZE_CUSTOM,          // Custom regex-based
    DSMIL_TOKENIZE_AUTO             // Auto-select based on vocabulary
} dsmil_tokenize_algorithm_t;

/**
 * @brief Special token IDs
 */
#define DSMIL_TOKEN_PAD     0
#define DSMIL_TOKEN_UNK     1
#define DSMIL_TOKEN_BOS     2
#define DSMIL_TOKEN_EOS     3
#define DSMIL_TOKEN_CLS     4
#define DSMIL_TOKEN_SEP     5
#define DSMIL_TOKEN_MASK    6

/**
 * @brief Tokenization context (manages vocabulary, cache, etc.)
 */
typedef struct dsmil_tokenizer_ctx dsmil_tokenizer_ctx_t;

/**
 * @brief Tokenization options
 */
typedef struct {
    dsmil_tokenize_algorithm_t algorithm;
    bool lowercase;                 // Lowercase tokens
    bool remove_punctuation;        // Remove punctuation
    bool handle_unicode;            // Full Unicode support
    bool add_special_tokens;        // Add BOS/EOS tokens
    size_t max_tokens;              // Maximum tokens to return
    size_t max_token_length;        // Maximum characters per token
    const char *vocab_path;         // Path to vocabulary file
    bool use_cache;                 // Enable token caching
    size_t cache_size;              // Cache size (number of entries)
} dsmil_tokenize_options_t;

/**
 * @brief Token information
 */
typedef struct {
    int token_id;                   // Token ID from vocabulary
    size_t start_pos;               // Start position in original text
    size_t end_pos;                 // End position in original text
    const char *token_text;         // Token text (if available)
    size_t token_text_len;          // Token text length
} dsmil_token_info_t;

/**
 * @brief Initialize tokenization context
 * 
 * @param vocab_path Path to vocabulary file (NULL for default)
 * @param options Tokenization options (NULL for defaults)
 * @param ctx Output context pointer
 * @return 0 on success, negative on error
 */
int dsmil_tokenizer_init(const char *vocab_path,
                        const dsmil_tokenize_options_t *options,
                        dsmil_tokenizer_ctx_t **ctx);

/**
 * @brief Tokenize text
 * 
 * @param ctx Tokenizer context
 * @param text Input text (UTF-8)
 * @param text_len Text length in bytes
 * @param tokens Output token IDs array
 * @param token_count Output number of tokens
 * @param max_tokens Maximum tokens to return
 * @param token_info Optional token information array
 * @return 0 on success, negative on error
 */
int dsmil_nlp_tokenize(dsmil_tokenizer_ctx_t *ctx,
                       const char *text,
                       size_t text_len,
                       int *tokens,
                       size_t *token_count,
                       size_t max_tokens,
                       dsmil_token_info_t *token_info);

/**
 * @brief Detokenize (convert tokens back to text)
 * 
 * @param ctx Tokenizer context
 * @param tokens Token IDs array
 * @param token_count Number of tokens
 * @param text Output text buffer
 * @param text_len Input/output text length
 * @param add_spaces Add spaces between tokens
 * @return 0 on success, negative on error
 */
int dsmil_nlp_detokenize(dsmil_tokenizer_ctx_t *ctx,
                        const int *tokens,
                        size_t token_count,
                        char *text,
                        size_t *text_len,
                        bool add_spaces);

/**
 * @brief Load vocabulary from file
 * 
 * @param ctx Tokenizer context
 * @param vocab_path Path to vocabulary file
 * @param format Vocabulary format (json, txt, binary)
 * @return 0 on success, negative on error
 */
int dsmil_tokenizer_load_vocab(dsmil_tokenizer_ctx_t *ctx,
                              const char *vocab_path,
                              const char *format);

/**
 * @brief Add token to vocabulary
 * 
 * @param ctx Tokenizer context
 * @param token Token text
 * @param token_len Token length
 * @param token_id Output token ID
 * @return 0 on success, negative on error
 */
int dsmil_tokenizer_add_token(dsmil_tokenizer_ctx_t *ctx,
                              const char *token,
                              size_t token_len,
                              int *token_id);

/**
 * @brief Get token text from ID
 * 
 * @param ctx Tokenizer context
 * @param token_id Token ID
 * @param token_text Output token text
 * @param token_text_len Output token text length
 * @return 0 on success, negative on error
 */
int dsmil_tokenizer_get_token_text(dsmil_tokenizer_ctx_t *ctx,
                                  int token_id,
                                  const char **token_text,
                                  size_t *token_text_len);

/**
 * @brief Clear tokenization cache
 * 
 * @param ctx Tokenizer context
 */
void dsmil_tokenizer_clear_cache(dsmil_tokenizer_ctx_t *ctx);

/**
 * @brief Get tokenization statistics
 * 
 * @param ctx Tokenizer context
 * @param total_tokens Output total tokens processed
 * @param cache_hits Output cache hit count
 * @param cache_misses Output cache miss count
 */
void dsmil_tokenizer_get_stats(dsmil_tokenizer_ctx_t *ctx,
                               uint64_t *total_tokens,
                               uint64_t *cache_hits,
                               uint64_t *cache_misses);

/**
 * @brief Cleanup tokenization context
 * 
 * @param ctx Tokenizer context
 */
void dsmil_tokenizer_cleanup(dsmil_tokenizer_ctx_t *ctx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* DSMIL_NLP_TOKENIZE_H */

