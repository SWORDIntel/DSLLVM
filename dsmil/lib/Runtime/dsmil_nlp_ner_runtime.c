/*
 * DSMIL NLP NER Runtime Implementation
 *
 * This file implements the NLP NER inference API for IOC extraction
 * in the DSMIL security runtime.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_model_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

/* NER tag definitions (BIO format) */
#define NER_TAG_O  0   /* Outside - not an entity */
#define NER_TAG_B_IP 1   /* Beginning of IP address */
#define NER_TAG_I_IP 2   /* Inside of IP address */
#define NER_TAG_B_DOMAIN 3   /* Beginning of domain */
#define NER_TAG_I_DOMAIN 4   /* Inside of domain */
#define NER_TAG_B_HASH 5     /* Beginning of file hash */
#define NER_TAG_I_HASH 6     /* Inside of file hash */
#define NER_TAG_B_URL 7      /* Beginning of URL */
#define NER_TAG_I_URL 8      /* Inside of URL */

#define NUM_NER_TAGS 9

/* Entity type names */
static const char *ner_tag_names[] = {
    "O", "B-IP", "I-IP", "B-DOMAIN", "I-DOMAIN",
    "B-HASH", "I-HASH", "B-URL", "I-URL"
};

/**
 * @brief Validate NLP NER parameters
 */
static int validate_ner_params(void *model_handle,
                              const int *input_tokens,
                              size_t token_count,
                              int *entity_tags,
                              size_t *tag_count)
{
    if (!model_handle || !input_tokens || !entity_tags || !tag_count) {
        pr_err("dsmil: NLP NER: Invalid parameters\n");
        return -EINVAL;
    }

    if (token_count == 0 || token_count > NER_MAX_TOKENS) {
        pr_err("dsmil: NLP NER: Invalid token count: %zu\n", token_count);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Preprocess input tokens for NER model
 *
 * Converts variable-length token sequence to fixed-size input
 * expected by the NER model.
 */
static int preprocess_ner_tokens(const int *input_tokens,
                                size_t token_count,
                                float *model_input,
                                size_t input_size)
{
    size_t i;
    size_t max_tokens = input_size / sizeof(float);

    /* Copy tokens, padding with zeros if needed */
    for (i = 0; i < max_tokens; i++) {
        if (i < token_count) {
            model_input[i] = (float)input_tokens[i];
        } else {
            model_input[i] = 0.0f; /* Padding token */
        }
    }

    return 0;
}

/**
 * @brief Post-process NER model output
 *
 * Converts raw logits to NER tags using argmax for each token.
 */
static int postprocess_ner_output(const float *raw_logits,
                                 size_t output_size,
                                 size_t token_count,
                                 int *entity_tags,
                                 size_t *tag_count)
{
    size_t tokens_processed = output_size / NUM_NER_TAGS;
    size_t i, j;

    /* Ensure we don't exceed input token count */
    if (tokens_processed > token_count) {
        tokens_processed = token_count;
    }

    /* Convert logits to tags for each token */
    for (i = 0; i < tokens_processed; i++) {
        const float *token_logits = &raw_logits[i * NUM_NER_TAGS];

        /* Find highest probability tag (argmax) */
        float max_prob = token_logits[0];
        int best_tag = 0;

        for (j = 1; j < NUM_NER_TAGS; j++) {
            if (token_logits[j] > max_prob) {
                max_prob = token_logits[j];
                best_tag = j;
            }
        }

        entity_tags[i] = best_tag;
    }

    /* Set remaining tags to O (outside) if any */
    for (i = tokens_processed; i < token_count; i++) {
        entity_tags[i] = NER_TAG_O;
    }

    *tag_count = token_count;

    return 0;
}

/**
 * @brief Extract entities from NER tags
 *
 * Groups consecutive B-* and I-* tags into complete entities.
 */
static int extract_entities(const int *entity_tags,
                           size_t tag_count,
                           const int *input_tokens,
                           size_t token_count)
{
    size_t i;
    int current_entity_type = -1;
    size_t entity_start = 0;

    for (i = 0; i < tag_count && i < token_count; i++) {
        int tag = entity_tags[i];

        if (tag == NER_TAG_O) {
            /* End of any current entity */
            if (current_entity_type != -1) {
                /* Log completed entity */
                pr_debug("dsmil: NER entity: type=%s, tokens %zu-%zu\n",
                        ner_tag_names[current_entity_type], entity_start, i-1);
                current_entity_type = -1;
            }
        } else if (tag >= NER_TAG_B_IP && tag <= NER_TAG_B_URL) {
            /* Beginning of new entity */
            if (current_entity_type != -1) {
                /* End previous entity */
                pr_debug("dsmil: NER entity: type=%s, tokens %zu-%zu\n",
                        ner_tag_names[current_entity_type], entity_start, i-1);
            }

            current_entity_type = tag;
            entity_start = i;
        } else if (tag >= NER_TAG_I_IP && tag <= NER_TAG_I_URL) {
            /* Continuation of current entity */
            if (current_entity_type != -1) {
                /* Check if continuation matches expected type */
                int expected_continuation = current_entity_type + 1; /* B_* + 1 = I_* */
                if (tag != expected_continuation) {
                    /* Type mismatch - end previous entity */
                    pr_debug("dsmil: NER entity: type=%s, tokens %zu-%zu\n",
                            ner_tag_names[current_entity_type], entity_start, i-1);
                    current_entity_type = tag - 1; /* Start new entity with B_* type */
                    entity_start = i;
                }
            } else {
                /* Unexpected continuation - start new entity */
                current_entity_type = tag - 1; /* Convert I_* to B_* */
                entity_start = i;
            }
        }
    }

    /* End any remaining entity */
    if (current_entity_type != -1) {
        pr_debug("dsmil: NER entity: type=%s, tokens %zu-%zu\n",
                ner_tag_names[current_entity_type], entity_start, i-1);
    }

    return 0;
}

/**
 * @brief NLP NER inference for IOC extraction implementation
 */
int dsmil_nlp_ner_infer_int8(void *model_handle,
                            const int *input_tokens,
                            size_t token_count,
                            int *entity_tags,
                            size_t *tag_count)
{
    float *model_input;
    float *model_output;
    size_t input_size, output_size;
    int ret;

    /* Validate parameters */
    ret = validate_ner_params(model_handle, input_tokens, token_count,
                             entity_tags, tag_count);
    if (ret != 0)
        return ret;

    /* Allocate input buffer */
    input_size = NER_MAX_TOKENS;
    model_input = kzalloc(input_size * sizeof(float), GFP_KERNEL);
    if (!model_input) {
        pr_err("dsmil: NLP NER: Failed to allocate input buffer\n");
        return -ENOMEM;
    }

    /* Allocate output buffer (one logit per tag per token) */
    output_size = NER_MAX_TOKENS * NUM_NER_TAGS;
    model_output = kzalloc(output_size * sizeof(float), GFP_KERNEL);
    if (!model_output) {
        kfree(model_input);
        pr_err("dsmil: NLP NER: Failed to allocate output buffer\n");
        return -ENOMEM;
    }

    /* Preprocess input tokens */
    ret = preprocess_ner_tokens(input_tokens, token_count, model_input, input_size);
    if (ret != 0) {
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Run NER inference */
    ret = dsmil_model_infer_int8(model_handle, model_input, input_size,
                                model_output, output_size);
    if (ret != 0) {
        pr_err("dsmil: NLP NER: Model inference failed: %d\n", ret);
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Post-process output into entity tags */
    ret = postprocess_ner_output(model_output, output_size, token_count,
                                entity_tags, tag_count);
    if (ret != 0) {
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Extract entities for logging/debugging */
    ret = extract_entities(entity_tags, *tag_count, input_tokens, token_count);
    if (ret != 0) {
        pr_warn("dsmil: NLP NER: Entity extraction failed: %d\n", ret);
        /* Don't fail the whole operation for extraction issues */
    }

    kfree(model_input);
    kfree(model_output);

    pr_debug("dsmil: NLP NER: Processed %zu tokens, extracted %zu tags\n",
             token_count, *tag_count);

    return 0;
}

/*
 * NLP NER Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
