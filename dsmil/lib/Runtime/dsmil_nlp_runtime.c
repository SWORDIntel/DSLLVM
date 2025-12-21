/*
 * DSMIL NLP Runtime Implementation
 *
 * This file implements the NLP tokenization and detokenization
 * functionality as specified in the model APIs.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_nlp_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>

/* Simple vocabulary for basic tokenization */
static const char *basic_vocab[] = {
    "[PAD]", "[UNK]", "[CLS]", "[SEP]", "[MASK]",  /* Special tokens */
    "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for",
    "of", "with", "by", "from", "up", "about", "into", "through", "during",
    "before", "after", "above", "below", "between", "among", "within",
    "without", "against", "along", "around", "behind", "beside", "near",
    "next", "over", "under", "upon", "like", "unlike", "as", "than",
    "that", "this", "these", "those", "i", "me", "my", "myself", "we",
    "our", "ours", "ourselves", "you", "your", "yours", "yourself",
    "yourselves", "he", "him", "his", "himself", "she", "her", "hers",
    "herself", "it", "its", "itself", "they", "them", "their", "theirs",
    "themselves", "what", "which", "who", "whom", "whose", "this", "that",
    "these", "those", "am", "is", "are", "was", "were", "be", "been",
    "being", "have", "has", "had", "having", "do", "does", "did", "doing",
    "will", "would", "could", "should", "may", "might", "must", "shall",
    "can", "ought", "dare", "need", "used", "not", "no", "nor", "none",
    "never", "neither", "nobody", "nothing", "nowhere", "noone", "none",
    "system", "network", "security", "attack", "malware", "virus", "trojan",
    "ransomware", "exploit", "vulnerability", "threat", "incident", "breach",
    "compromise", "intrusion", "suspicious", "anomaly", "detection", "alert",
    "response", "mitigation", "prevention", "analysis", "forensic", "log",
    "traffic", "packet", "connection", "port", "ip", "address", "domain",
    "url", "file", "process", "service", "server", "client", "user", "admin",
    "root", "access", "permission", "credential", "password", "key", "certificate"
};

#define VOCAB_SIZE (sizeof(basic_vocab) / sizeof(basic_vocab[0]))

/* Reverse mapping for detokenization */
static char *vocab_text[VOCAB_SIZE];

/**
 * @brief Initialize vocabulary text mappings
 */
static void init_vocab_text(void)
{
    int i;
    for (i = 0; i < VOCAB_SIZE; i++) {
        vocab_text[i] = (char *)basic_vocab[i];
    }
}

/**
 * @brief Find token ID for a word
 *
 * @param word Word to look up
 * @param word_len Length of the word
 * @return Token ID or NLP_TOKEN_UNKNOWN if not found
 */
static int find_token_id(const char *word, size_t word_len)
{
    int i;
    char *word_copy;
    int token_id = NLP_TOKEN_UNKNOWN;

    /* Create null-terminated copy for comparison */
    word_copy = kzalloc(word_len + 1, GFP_KERNEL);
    if (!word_copy)
        return NLP_TOKEN_UNKNOWN;

    memcpy(word_copy, word, word_len);
    word_copy[word_len] = '\0';

    /* Convert to lowercase for case-insensitive matching */
    for (i = 0; word_copy[i]; i++) {
        word_copy[i] = tolower(word_copy[i]);
    }

    /* Search vocabulary */
    for (i = 0; i < VOCAB_SIZE; i++) {
        if (strcmp(word_copy, basic_vocab[i]) == 0) {
            token_id = i;
            break;
        }
    }

    kfree(word_copy);
    return token_id;
}

/**
 * @brief Extract next word from text
 *
 * @param text Input text
 * @param text_len Text length
 * @param pos Current position (updated)
 * @param word Output word buffer
 * @param word_len Maximum word length
 * @return Length of extracted word, 0 if no more words
 */
static size_t extract_next_word(const char *text, size_t text_len,
                               size_t *pos, char *word, size_t word_len)
{
    size_t start = *pos;
    size_t end;
    size_t len;

    /* Skip whitespace */
    while (start < text_len && isspace(text[start])) {
        start++;
    }

    if (start >= text_len) {
        *pos = start;
        return 0; /* No more words */
    }

    /* Find word end */
    end = start;
    while (end < text_len && !isspace(text[end]) &&
           !ispunct(text[end])) {
        end++;
    }

    /* Extract word */
    len = end - start;
    if (len >= word_len) {
        len = word_len - 1; /* Leave space for null terminator */
    }

    memcpy(word, &text[start], len);
    word[len] = '\0';

    *pos = end;
    return len;
}

/**
 * @brief Validate tokenization parameters
 */
static int validate_tokenize_params(const char *text, size_t text_len,
                                   int *tokens, size_t *token_count,
                                   size_t max_tokens)
{
    if (!text || !tokens || !token_count) {
        pr_err("dsmil: NLP tokenize: Invalid parameters\n");
        return -EINVAL;
    }

    if (text_len == 0 || text_len > NLP_MAX_TEXT_LENGTH) {
        pr_err("dsmil: NLP tokenize: Invalid text length: %zu\n", text_len);
        return -EINVAL;
    }

    if (max_tokens == 0 || max_tokens > NLP_MAX_TOKENS) {
        pr_err("dsmil: NLP tokenize: Invalid max tokens: %zu\n", max_tokens);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief NLP Tokenization implementation
 */
int dsmil_nlp_tokenize(const char *text, size_t text_len,
                      int *tokens, size_t *token_count, size_t max_tokens)
{
    size_t pos = 0;
    size_t token_idx = 0;
    char word[256];
    size_t word_len;
    int token_id;
    int ret;

    /* Validate parameters */
    ret = validate_tokenize_params(text, text_len, tokens, token_count, max_tokens);
    if (ret != 0)
        return ret;

    /* Initialize vocabulary if needed */
    static bool vocab_initialized = false;
    if (!vocab_initialized) {
        init_vocab_text();
        vocab_initialized = true;
    }

    /* Extract words and tokenize */
    while ((word_len = extract_next_word(text, text_len, &pos, word, sizeof(word))) > 0) {
        if (token_idx >= max_tokens)
            break;

        /* Find token ID */
        token_id = find_token_id(word, word_len);
        tokens[token_idx++] = token_id;

        /* Stop if we hit maximum tokens */
        if (token_idx >= max_tokens)
            break;
    }

    *token_count = token_idx;
    return 0;
}

/**
 * @brief Validate detokenization parameters
 */
static int validate_detokenize_params(const int *tokens, size_t token_count,
                                     char *text, size_t *text_len)
{
    if (!tokens || !text || !text_len) {
        pr_err("dsmil: NLP detokenize: Invalid parameters\n");
        return -EINVAL;
    }

    if (token_count == 0 || token_count > NLP_MAX_TOKENS) {
        pr_err("dsmil: NLP detokenize: Invalid token count: %zu\n", token_count);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief NLP Detokenization implementation
 */
int dsmil_nlp_detokenize(const int *tokens, size_t token_count,
                        char *text, size_t *text_len)
{
    size_t pos = 0;
    size_t i;
    int token_id;
    const char *word;
    size_t word_len;
    int ret;

    /* Validate parameters */
    ret = validate_detokenize_params(tokens, token_count, text, text_len);
    if (ret != 0)
        return ret;

    /* Convert tokens back to text */
    for (i = 0; i < token_count; i++) {
        token_id = tokens[i];

        /* Handle special tokens */
        if (token_id == NLP_TOKEN_PAD)
            continue; /* Skip padding tokens */

        if (token_id < 0 || token_id >= VOCAB_SIZE) {
            word = "[UNK]";
            word_len = 5;
        } else {
            word = vocab_text[token_id];
            word_len = strlen(word);
        }

        /* Check if we have space */
        if (pos + word_len + 1 >= *text_len) { /* +1 for space/null */
            *text_len = pos + word_len + 1;
            return -ENOBUFS;
        }

        /* Add space before word (except first word) */
        if (pos > 0) {
            text[pos++] = ' ';
        }

        /* Copy word */
        memcpy(&text[pos], word, word_len);
        pos += word_len;
    }

    /* Null terminate */
    text[pos] = '\0';
    *text_len = pos;

    return 0;
}

/**
 * @brief Check if NLP subsystem is available
 */
int dsmil_nlp_available(void)
{
    /* Basic NLP tokenization is always available as it's software-based */
    return 1;
}

/**
 * @brief Initialize NLP subsystem
 */
int dsmil_nlp_initialize(void)
{
    /* Initialize vocabulary mappings */
    init_vocab_text();

    pr_info("dsmil: NLP subsystem initialized\n");
    return 0;
}

/**
 * @brief Cleanup NLP subsystem resources
 */
int dsmil_nlp_cleanup(void)
{
    /* No dynamic resources to cleanup */
    pr_info("dsmil: NLP subsystem cleanup completed\n");
    return 0;
}

/**
 * @brief Get maximum supported tokens
 */
size_t dsmil_nlp_get_max_tokens(void)
{
    return NLP_MAX_TOKENS;
}

/*
 * NLP Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
