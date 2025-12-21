/*
 * DSMIL RL Runtime Implementation
 *
 * This file implements the RL policy inference API for incident response
 * automation in the DSMIL security runtime.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_model_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

/**
 * @brief Validate RL policy inference parameters
 */
static int validate_rl_params(void *model_handle,
                             const float *state_vector,
                             size_t state_size,
                             float *action_scores,
                             size_t num_actions)
{
    if (!model_handle || !state_vector || !action_scores) {
        pr_err("dsmil: RL infer: Invalid parameters\n");
        return -EINVAL;
    }

    if (state_size != RL_STATE_SIZE) {
        pr_err("dsmil: RL infer: Invalid state size: %zu (expected %d)\n",
               state_size, RL_STATE_SIZE);
        return -EINVAL;
    }

    if (num_actions != RL_NUM_ACTIONS) {
        pr_err("dsmil: RL infer: Invalid action count: %zu (expected %d)\n",
               num_actions, RL_NUM_ACTIONS);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Post-process RL model output
 *
 * Applies softmax to convert raw Q-values into action probabilities,
 * and identifies the best action.
 */
static int process_rl_output(const float *raw_scores,
                            size_t num_actions,
                            float *action_scores)
{
    float sum = 0.0f;
    float max_score = raw_scores[0];
    size_t i;

    /* Find maximum score for numerical stability */
    for (i = 1; i < num_actions; i++) {
        if (raw_scores[i] > max_score) {
            max_score = raw_scores[i];
        }
    }

    /* Apply softmax: exp(x - max) / sum(exp(x - max)) */
    for (i = 0; i < num_actions; i++) {
        action_scores[i] = expf(raw_scores[i] - max_score);
        sum += action_scores[i];
    }

    /* Normalize to probabilities */
    if (sum > 0.0f) {
        for (i = 0; i < num_actions; i++) {
            action_scores[i] /= sum;
        }
    }

    return 0;
}

/**
 * @brief Get recommended action from policy
 *
 * @param action_scores Action probability distribution
 * @param num_actions Number of actions
 * @return Index of recommended action
 */
static int get_recommended_action(const float *action_scores, size_t num_actions)
{
    float max_prob = action_scores[0];
    int best_action = 0;
    size_t i;

    for (i = 1; i < num_actions; i++) {
        if (action_scores[i] > max_prob) {
            max_prob = action_scores[i];
            best_action = i;
        }
    }

    return best_action;
}

/**
 * @brief RL policy inference for incident response implementation
 */
int dsmil_rl_policy_infer_int8(void *model_handle,
                              const float *state_vector,
                              size_t state_size,
                              float *action_scores,
                              size_t num_actions)
{
    float *raw_scores;
    int ret;

    /* Validate parameters */
    ret = validate_rl_params(model_handle, state_vector, state_size,
                            action_scores, num_actions);
    if (ret != 0)
        return ret;

    /* Allocate buffer for raw model output */
    raw_scores = kzalloc(num_actions * sizeof(float), GFP_KERNEL);
    if (!raw_scores) {
        pr_err("dsmil: RL infer: Failed to allocate scores buffer\n");
        return -ENOMEM;
    }

    /* Run RL model inference */
    ret = dsmil_model_infer_int8(model_handle, state_vector, state_size,
                                raw_scores, num_actions);
    if (ret != 0) {
        pr_err("dsmil: RL infer: Model inference failed: %d\n", ret);
        kfree(raw_scores);
        return ret;
    }

    /* Process model output into action probabilities */
    ret = process_rl_output(raw_scores, num_actions, action_scores);
    if (ret != 0) {
        kfree(raw_scores);
        return ret;
    }

    /* Get recommended action for logging */
    int recommended_action = get_recommended_action(action_scores, num_actions);

    kfree(raw_scores);

    pr_debug("dsmil: RL infer: Recommended action %d (prob=%.3f)\n",
             recommended_action, action_scores[recommended_action]);

    return 0;
}

/*
 * RL Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
