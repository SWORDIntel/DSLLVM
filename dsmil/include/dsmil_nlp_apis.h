/*
 * DSMIL NLP APIs Header
 *
 * This header defines the NLP (Natural Language Processing) API functions
 * for tokenization and detokenization operations.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#ifndef DSMIL_NLP_APIS_H
#define DSMIL_NLP_APIS_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * NLP TOKENIZE API
 * ============================================================================ */

/**
 * @brief Tokenize text for NLP models
 *
 * This function tokenizes input text into token IDs suitable for NLP models.
 * Used in IOC extraction and incident classification systems.
 *
 * @param text Input text to tokenize
 * @param text_len Length of input text in bytes
 * @param tokens Output array for token IDs
 * @param token_count Input: maximum tokens, Output: actual token count
 * @param max_tokens Maximum number of tokens to extract
 * @return 0 on success, negative error code on failure
 */
int dsmil_nlp_tokenize(const char *text,
                      size_t text_len,
                      int *tokens,
                      size_t *token_count,
                      size_t max_tokens);

/* ============================================================================
 * NLP DETOKENIZE API
 * ============================================================================ */

/**
 * @brief Convert tokens back to text
 *
 * This function converts token IDs back to human-readable text.
 * Inverse operation of tokenization.
 *
 * @param tokens Input array of token IDs
 * @param token_count Number of tokens in the array
 * @param text Output text buffer
 * @param text_len Input: buffer size, Output: actual text length
 * @return 0 on success, negative error code on failure
 */
int dsmil_nlp_detokenize(const int *tokens,
                        size_t token_count,
                        char *text,
                        size_t *text_len);

/* ============================================================================
 * NLP API UTILITIES
 * ============================================================================ */

/**
 * @brief Check if NLP subsystem is available
 *
 * @return 1 if NLP is available, 0 otherwise
 */
int dsmil_nlp_available(void);

/**
 * @brief Initialize NLP subsystem
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_nlp_initialize(void);

/**
 * @brief Cleanup NLP subsystem resources
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_nlp_cleanup(void);

/**
 * @brief Get maximum supported tokens
 *
 * @return Maximum number of tokens supported
 */
size_t dsmil_nlp_get_max_tokens(void);

/* ============================================================================
 * NLP CONSTANTS
 * ============================================================================ */

/* Tokenization constants */
#define NLP_MAX_TEXT_LENGTH 8192    /* Maximum input text length */
#define NLP_MAX_TOKENS 512          /* Maximum tokens per text */
#define NLP_TOKEN_UNKNOWN -1        /* Unknown token ID */
#define NLP_TOKEN_PAD 0             /* Padding token ID */
#define NLP_TOKEN_START 1           /* Start of sequence token */
#define NLP_TOKEN_END 2             /* End of sequence token */

#endif /* DSMIL_NLP_APIS_H */
