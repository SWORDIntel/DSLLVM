/**
 * @file dsmil_nlp_vocab_runtime.c
 * @brief Vocabulary Management System for NLP Tokenization
 * 
 * Provides comprehensive vocabulary management with support for multiple
 * file formats, efficient lookup, and dynamic token management.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

/**
 * @brief Vocabulary entry structure
 */
typedef struct vocab_entry {
    char *token;
    size_t token_len;
    int token_id;
    uint32_t frequency;  // For BPE/WordPiece frequency-based operations
    struct vocab_entry *next;  // For hash table chaining
} vocab_entry_t;

/**
 * @brief Vocabulary structure
 */
typedef struct {
    vocab_entry_t *entries;
    size_t size;
    size_t capacity;
    vocab_entry_t **hash_table;
    size_t hash_table_size;
    int next_token_id;
} vocabulary_t;

/**
 * @brief Hash function for token lookup
 */
static uint32_t vocab_hash(const char *token, size_t token_len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < token_len; i++) {
        hash = ((hash << 5) + hash) + (uint32_t)(unsigned char)token[i];
    }
    return hash;
}

/**
 * @brief Create vocabulary
 */
vocabulary_t *vocab_create(void) {
    vocabulary_t *vocab = calloc(1, sizeof(vocabulary_t));
    if (!vocab) {
        return NULL;
    }
    
    vocab->capacity = 1024;
    vocab->entries = calloc(vocab->capacity, sizeof(vocab_entry_t));
    if (!vocab->entries) {
        free(vocab);
        return NULL;
    }
    
    vocab->hash_table_size = 4096;
    vocab->hash_table = calloc(vocab->hash_table_size, sizeof(vocab_entry_t*));
    if (!vocab->hash_table) {
        free(vocab->entries);
        free(vocab);
        return NULL;
    }
    
    vocab->next_token_id = 10;  // Start after special tokens (0-9)
    
    return vocab;
}

/**
 * @brief Add token to vocabulary
 */
int vocab_add_token(vocabulary_t *vocab, const char *token, size_t token_len, int *token_id) {
    if (!vocab || !token || token_len == 0) {
        return -EINVAL;
    }
    
    // Check if token already exists
    uint32_t hash = vocab_hash(token, token_len);
    uint32_t index = hash % vocab->hash_table_size;
    
    vocab_entry_t *entry = vocab->hash_table[index];
    while (entry) {
        if (entry->token_len == token_len && 
            memcmp(entry->token, token, token_len) == 0) {
            if (token_id) {
                *token_id = entry->token_id;
            }
            return 0;  // Already exists
        }
        entry = entry->next;
    }
    
    // Expand if needed
    if (vocab->size >= vocab->capacity) {
        size_t new_capacity = vocab->capacity * 2;
        vocab_entry_t *new_entries = realloc(vocab->entries, 
                                            new_capacity * sizeof(vocab_entry_t));
        if (!new_entries) {
            return -ENOMEM;
        }
        vocab->entries = new_entries;
        vocab->capacity = new_capacity;
    }
    
    // Add new entry
    vocab_entry_t *new_entry = &vocab->entries[vocab->size];
    new_entry->token = malloc(token_len + 1);
    if (!new_entry->token) {
        return -ENOMEM;
    }
    memcpy(new_entry->token, token, token_len);
    new_entry->token[token_len] = '\0';
    new_entry->token_len = token_len;
    new_entry->token_id = vocab->next_token_id++;
    new_entry->frequency = 1;
    new_entry->next = NULL;
    
    // Add to hash table with proper chaining
    if (vocab->hash_table[index]) {
        new_entry->next = vocab->hash_table[index];
    }
    vocab->hash_table[index] = new_entry;
    
    vocab->size++;
    
    if (token_id) {
        *token_id = new_entry->token_id;
    }
    
    return 0;
}

/**
 * @brief Find token in vocabulary
 */
vocab_entry_t *vocab_find_token(vocabulary_t *vocab, const char *token, size_t token_len) {
    if (!vocab || !token || token_len == 0) {
        return NULL;
    }
    
    uint32_t hash = vocab_hash(token, token_len);
    uint32_t index = hash % vocab->hash_table_size;
    
    vocab_entry_t *entry = vocab->hash_table[index];
    while (entry) {
        if (entry->token_len == token_len && 
            memcmp(entry->token, token, token_len) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief Load vocabulary from text file (one token per line)
 */
int vocab_load_text(vocabulary_t *vocab, const char *vocab_path) {
    FILE *fp = fopen(vocab_path, "r");
    if (!fp) {
        return -errno;
    }
    
    char *line = NULL;
    size_t line_len = 0;
    ssize_t read;
    int token_id = 10;  // Start after special tokens
    
    while ((read = getline(&line, &line_len, fp)) != -1) {
        // Remove newline
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }
        
        if (read > 0) {
            int id;
            int ret = vocab_add_token(vocab, line, read, &id);
            if (ret == 0 && id == token_id) {
                token_id++;
            }
        }
    }
    
    free(line);
    fclose(fp);
    
    return 0;
}

/**
 * @brief Load vocabulary from JSON file
 */
int vocab_load_json(vocabulary_t *vocab, const char *vocab_path) {
    FILE *fp = fopen(vocab_path, "r");
    if (!fp) {
        return -errno;
    }
    
    // Simple JSON parser for vocabulary files
    // Format: {"token1": id1, "token2": id2, ...}
    char buffer[4096];
    size_t total_read = 0;
    
    while (fgets(buffer + total_read, sizeof(buffer) - total_read, fp)) {
        total_read = strlen(buffer);
        if (total_read >= sizeof(buffer) - 1) {
            break;
        }
    }
    
    fclose(fp);
    
    // Parse JSON vocabulary file format
    const char *p = buffer;
    while (*p) {
        // Skip whitespace
        while (*p && isspace(*p)) p++;
        if (*p == '"') {
            p++;  // Skip opening quote
            const char *token_start = p;
            while (*p && *p != '"') p++;
            if (*p == '"') {
                size_t token_len = p - token_start;
                p++;  // Skip closing quote
                
                // Skip to colon
                while (*p && *p != ':') p++;
                if (*p == ':') {
                    p++;
                    // Skip whitespace
                    while (*p && isspace(*p)) p++;
                    
                    // Parse token ID
                    int token_id = 0;
                    while (*p && isdigit(*p)) {
                        token_id = token_id * 10 + (*p - '0');
                        p++;
                    }
                    
                    // Add token
                    char *token = malloc(token_len + 1);
                    if (token) {
                        memcpy(token, token_start, token_len);
                        token[token_len] = '\0';
                        vocab_add_token(vocab, token, token_len, NULL);
                        free(token);
                    }
                }
            }
        }
        if (*p) p++;
    }
    
    return 0;
}

/**
 * @brief Destroy vocabulary
 */
void vocab_destroy(vocabulary_t *vocab) {
    if (!vocab) {
        return;
    }
    
    if (vocab->entries) {
        for (size_t i = 0; i < vocab->size; i++) {
            free(vocab->entries[i].token);
        }
        free(vocab->entries);
    }
    
    free(vocab->hash_table);
    free(vocab);
}

// Public API implementations will be in main tokenization file
// This file provides vocabulary management functions

