/*
 * DSMIL Text Classifier Runtime Implementation
 *
 * This file implements the text classifier inference API for incident
 * classification in the DSMIL security runtime.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_model_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

/* Incident classification categories */
static const char *incident_classes[] = {
    "normal",           /* 0: Normal activity */
    "malware",          /* 1: Malware infection */
    "intrusion",        /* 2: Network intrusion */
    "exfiltration",     /* 3: Data exfiltration */
    "ddos",            /* 4: DDoS attack */
    "unauthorized"     /* 5: Unauthorized access */
};

#define NUM_INCIDENT_CLASSES (sizeof(incident_classes) / sizeof(incident_classes[0]))

/**
 * @brief Validate text classifier parameters
 */
static int validate_text_classifier_params(void *model_handle,
                                          const int *input_tokens,
                                          size_t token_count,
                                          float *probabilities,
                                          size_t num_classes)
{
    if (!model_handle || !input_tokens || !probabilities) {
        pr_err("dsmil: Text classifier: Invalid parameters\n");
        return -EINVAL;
    }

    if (token_count == 0 || token_count > TEXT_CLASSIFIER_MAX_TOKENS) {
        pr_err("dsmil: Text classifier: Invalid token count: %zu\n", token_count);
        return -EINVAL;
    }

    if (num_classes != TEXT_CLASSIFIER_NUM_CLASSES) {
        pr_err("dsmil: Text classifier: Invalid class count: %zu (expected %d)\n",
               num_classes, TEXT_CLASSIFIER_NUM_CLASSES);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Preprocess input tokens for model
 *
 * Converts variable-length token sequence to fixed-size input
 * expected by the text classification model.
 */
static int preprocess_tokens(const int *input_tokens,
                            size_t token_count,
                            float *model_input,
                            size_t input_size)
{
    size_t i;
    size_t max_tokens = input_size / sizeof(float); /* Assume 1 token per float */

    /* Copy tokens, padding with zeros if needed */
    for (i = 0; i < max_tokens; i++) {
        if (i < token_count) {
            model_input[i] = (float)input_tokens[i];
        } else {
            model_input[i] = 0.0f; /* Padding */
        }
    }

    return 0;
}

/**
 * @brief Post-process model output
 *
 * Applies softmax to convert logits to probabilities and
 * determines the most likely classification.
 */
static int postprocess_output(const float *raw_logits,
                             size_t num_classes,
                             float *probabilities)
{
    float sum = 0.0f;
    float max_logit = raw_logits[0];
    size_t i;

    /* Find maximum logit for numerical stability */
    for (i = 1; i < num_classes; i++) {
        if (raw_logits[i] > max_logit) {
            max_logit = raw_logits[i];
        }
    }

    /* Apply softmax: exp(x - max) / sum(exp(x - max)) */
    for (i = 0; i < num_classes; i++) {
        probabilities[i] = expf(raw_logits[i] - max_logit);
        sum += probabilities[i];
    }

    /* Normalize to probabilities */
    if (sum > 0.0f) {
        for (i = 0; i < num_classes; i++) {
            probabilities[i] /= sum;
        }
    }

    return 0;
}

/**
 * @brief Get predicted class from probabilities
 *
 * @param probabilities Class probability distribution
 * @param num_classes Number of classes
 * @return Index of most likely class
 */
static int get_predicted_class(const float *probabilities, size_t num_classes)
{
    float max_prob = probabilities[0];
    int best_class = 0;
    size_t i;

    for (i = 1; i < num_classes; i++) {
        if (probabilities[i] > max_prob) {
            max_prob = probabilities[i];
            best_class = i;
        }
    }

    return best_class;
}

/**
 * @brief Text classifier inference implementation
 */
int dsmil_text_classifier_infer_int8(void *model_handle,
                                    const int *input_tokens,
                                    size_t token_count,
                                    float *probabilities,
                                    size_t num_classes)
{
    float *model_input;
    float *model_output;
    size_t input_size;
    int ret;

    /* Validate parameters */
    ret = validate_text_classifier_params(model_handle, input_tokens,
                                         token_count, probabilities, num_classes);
    if (ret != 0)
        return ret;

    /* Allocate input buffer (fixed size for model) */
    input_size = TEXT_CLASSIFIER_MAX_TOKENS;
    model_input = kzalloc(input_size * sizeof(float), GFP_KERNEL);
    if (!model_input) {
        pr_err("dsmil: Text classifier: Failed to allocate input buffer\n");
        return -ENOMEM;
    }

    /* Allocate output buffer for raw logits */
    model_output = kzalloc(num_classes * sizeof(float), GFP_KERNEL);
    if (!model_output) {
        kfree(model_input);
        pr_err("dsmil: Text classifier: Failed to allocate output buffer\n");
        return -ENOMEM;
    }

    /* Preprocess input tokens */
    ret = preprocess_tokens(input_tokens, token_count, model_input, input_size);
    if (ret != 0) {
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Run text classification inference */
    ret = dsmil_model_infer_int8(model_handle, model_input, input_size,
                                model_output, num_classes);
    if (ret != 0) {
        pr_err("dsmil: Text classifier: Model inference failed: %d\n", ret);
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Post-process output into probabilities */
    ret = postprocess_output(model_output, num_classes, probabilities);
    if (ret != 0) {
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Get predicted class for logging */
    int predicted_class = get_predicted_class(probabilities, num_classes);

    kfree(model_input);
    kfree(model_output);

    pr_debug("dsmil: Text classifier: Predicted class %d (%s, prob=%.3f)\n",
             predicted_class, incident_classes[predicted_class],
             probabilities[predicted_class]);

    return 0;
}

/*
 * Text Classifier Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
