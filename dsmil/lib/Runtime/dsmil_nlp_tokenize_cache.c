/**
 * @file dsmil_nlp_tokenize_cache.c
 * @brief LRU Cache for Tokenization
 * 
 * Implements LRU (Least Recently Used) cache for tokenization results
 * to improve performance on repeated text.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/**
 * @brief Cache entry
 */
typedef struct cache_entry {
    char *text_key;
    size_t text_key_len;
    int *tokens;
    size_t token_count;
    struct cache_entry *prev;
    struct cache_entry *next;
    uint64_t access_time;
} cache_entry_t;

/**
 * @brief Cache structure
 */
typedef struct {
    cache_entry_t *head;
    cache_entry_t *tail;
    cache_entry_t **hash_table;
    size_t hash_table_size;
    size_t max_size;
    size_t current_size;
    uint64_t access_counter;
} tokenize_cache_t;

/**
 * @brief Hash function for cache keys
 */
static uint32_t cache_hash(const char *key, size_t key_len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < key_len; i++) {
        hash = ((hash << 5) + hash) + (unsigned char)key[i];
    }
    return hash;
}

/**
 * @brief Create cache
 */
tokenize_cache_t *tokenize_cache_create(size_t max_size) {
    tokenize_cache_t *cache = calloc(1, sizeof(tokenize_cache_t));
    if (!cache) {
        return NULL;
    }
    
    cache->max_size = max_size;
    cache->hash_table_size = max_size * 2;
    cache->hash_table = calloc(cache->hash_table_size, sizeof(cache_entry_t*));
    if (!cache->hash_table) {
        free(cache);
        return NULL;
    }
    
    return cache;
}

/**
 * @brief Lookup in cache
 */
int tokenize_cache_lookup(tokenize_cache_t *cache, const char *text, size_t text_len,
                          int **tokens, size_t *token_count) {
    if (!cache || !text || !tokens || !token_count) {
        return -EINVAL;
    }
    
    uint32_t hash = cache_hash(text, text_len);
    uint32_t index = hash % cache->hash_table_size;
    
    // Search hash table chain
    cache_entry_t *entry = cache->hash_table[index];
    
    while (entry) {
        if (entry->text_key_len == text_len &&
            memcmp(entry->text_key, text, text_len) == 0) {
            // Cache hit - move to front of LRU list
            // First, remove from current position in LRU list
            if (entry->prev) {
                entry->prev->next = entry->next;
            } else {
                cache->head = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            } else {
                cache->tail = entry->prev;
            }
            
            // Move to front of LRU list
            entry->prev = NULL;
            entry->next = cache->head;
            if (cache->head) {
                cache->head->prev = entry;
            }
            cache->head = entry;
            if (!cache->tail) {
                cache->tail = entry;
            }
            
            entry->access_time = ++cache->access_counter;
            *tokens = entry->tokens;
            *token_count = entry->token_count;
            return 0;
        }
        entry = entry->hash_next;
    }
    
    return -ENOENT;  // Cache miss
}

/**
 * @brief Insert into cache
 */
int tokenize_cache_insert(tokenize_cache_t *cache, const char *text, size_t text_len,
                          const int *tokens, size_t token_count) {
    if (!cache || !text || !tokens) {
        return -EINVAL;
    }
    
    // Evict if at capacity
    while (cache->current_size >= cache->max_size && cache->tail) {
        cache_entry_t *to_remove = cache->tail;
        
        // Remove from hash table chain
        uint32_t hash = cache_hash(to_remove->text_key, to_remove->text_key_len);
        uint32_t index = hash % cache->hash_table_size;
        
        cache_entry_t *chain_entry = cache->hash_table[index];
        cache_entry_t *chain_prev = NULL;
        
        while (chain_entry) {
            if (chain_entry == to_remove) {
                if (chain_prev) {
                    chain_prev->hash_next = to_remove->hash_next;
                } else {
                    cache->hash_table[index] = to_remove->hash_next;
                }
                break;
            }
            chain_prev = chain_entry;
            chain_entry = chain_entry->hash_next;
        }
        
        // Remove from list
        if (to_remove->prev) {
            to_remove->prev->next = to_remove->next;
        } else {
            cache->head = to_remove->next;
        }
        if (to_remove->next) {
            to_remove->next->prev = to_remove->prev;
        } else {
            cache->tail = to_remove->prev;
        }
        
        free(to_remove->text_key);
        free(to_remove->tokens);
        free(to_remove);
        cache->current_size--;
    }
    
    // Create new entry
    cache_entry_t *entry = calloc(1, sizeof(cache_entry_t));
    if (!entry) {
        return -ENOMEM;
    }
    
    entry->text_key = malloc(text_len + 1);
    if (!entry->text_key) {
        free(entry);
        return -ENOMEM;
    }
    memcpy(entry->text_key, text, text_len);
    entry->text_key[text_len] = '\0';
    entry->text_key_len = text_len;
    
    entry->tokens = malloc(token_count * sizeof(int));
    if (!entry->tokens) {
        free(entry->text_key);
        free(entry);
        return -ENOMEM;
    }
    memcpy(entry->tokens, tokens, token_count * sizeof(int));
    entry->token_count = token_count;
    entry->access_time = ++cache->access_counter;
    
    // Add to hash table with proper chaining
    uint32_t hash = cache_hash(text, text_len);
    uint32_t index = hash % cache->hash_table_size;
    entry->hash_next = cache->hash_table[index];
    cache->hash_table[index] = entry;
    
    // Add to front of list
    entry->next = cache->head;
    if (cache->head) {
        cache->head->prev = entry;
    }
    cache->head = entry;
    if (!cache->tail) {
        cache->tail = entry;
    }
    
    cache->current_size++;
    return 0;
}

/**
 * @brief Clear cache
 */
void tokenize_cache_clear(tokenize_cache_t *cache) {
    if (!cache) {
        return;
    }
    
    cache_entry_t *entry = cache->head;
    while (entry) {
        cache_entry_t *next = entry->next;
        free(entry->text_key);
        free(entry->tokens);
        free(entry);
        entry = next;
    }
    
    memset(cache->hash_table, 0, cache->hash_table_size * sizeof(cache_entry_t*));
    cache->head = NULL;
    cache->tail = NULL;
    cache->current_size = 0;
}

/**
 * @brief Destroy cache
 */
void tokenize_cache_destroy(tokenize_cache_t *cache) {
    if (!cache) {
        return;
    }
    
    tokenize_cache_clear(cache);
    free(cache->hash_table);
    free(cache);
}

