/*
 * DSMIL GAN Runtime Implementation
 *
 * This file implements the GAN generator inference API for adversarial
 * robustness testing in the DSMIL security runtime.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_model_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/random.h>

/**
 * @brief Validate GAN generator parameters
 */
static int validate_gan_params(void *model_handle,
                              const float *noise_input,
                              size_t noise_size,
                              float *generated_output,
                              size_t output_size)
{
    if (!model_handle || !noise_input || !generated_output) {
        pr_err("dsmil: GAN infer: Invalid parameters\n");
        return -EINVAL;
    }

    if (noise_size != GAN_NOISE_SIZE) {
        pr_err("dsmil: GAN infer: Invalid noise size: %zu (expected %d)\n",
               noise_size, GAN_NOISE_SIZE);
        return -EINVAL;
    }

    if (output_size != GAN_OUTPUT_SIZE) {
        pr_err("dsmil: GAN infer: Invalid output size: %zu (expected %d)\n",
               output_size, GAN_OUTPUT_SIZE);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Generate random noise for GAN input
 *
 * Creates random noise vector from normal distribution for GAN input.
 */
static int generate_random_noise(float *noise, size_t noise_size)
{
    size_t i;
    uint32_t random_val;

    /* Generate pseudo-random noise using kernel random functions */
    for (i = 0; i < noise_size; i++) {
        /* Get random 32-bit value and convert to float in [-1, 1] range */
        get_random_bytes(&random_val, sizeof(random_val));
        noise[i] = ((float)random_val / (float)UINT32_MAX) * 2.0f - 1.0f;
    }

    return 0;
}

/**
 * @brief Post-process GAN output
 *
 * Applies activation function and normalization to GAN output.
 * For image generation, applies tanh activation and scales to [0, 255].
 */
static int postprocess_gan_output(float *generated_output, size_t output_size)
{
    size_t i;

    /* Apply tanh activation and scale to [0, 255] range for image generation */
    for (i = 0; i < output_size; i++) {
        /* Tanh activation */
        generated_output[i] = tanhf(generated_output[i]);

        /* Scale from [-1, 1] to [0, 255] */
        generated_output[i] = (generated_output[i] + 1.0f) * 127.5f;

        /* Clamp to valid range */
        if (generated_output[i] < 0.0f) generated_output[i] = 0.0f;
        if (generated_output[i] > 255.0f) generated_output[i] = 255.0f;
    }

    return 0;
}

/**
 * @brief Validate generated output quality
 *
 * Performs basic quality checks on GAN-generated content.
 */
static int validate_generated_output(const float *generated_output, size_t output_size)
{
    size_t i;
    float mean = 0.0f;
    float variance = 0.0f;

    /* Calculate basic statistics */
    for (i = 0; i < output_size; i++) {
        mean += generated_output[i];
    }
    mean /= (float)output_size;

    for (i = 0; i < output_size; i++) {
        float diff = generated_output[i] - mean;
        variance += diff * diff;
    }
    variance /= (float)output_size;

    /* Check for degenerate output (all same values) */
    if (variance < 1.0f) {
        pr_warn("dsmil: GAN: Low variance in generated output (%.3f), possible degenerate generation\n", variance);
    }

    pr_debug("dsmil: GAN: Generated output stats - mean=%.1f, variance=%.1f\n", mean, variance);

    return 0;
}

/**
 * @brief GAN generator inference implementation
 */
int dsmil_gan_generator_infer_int8(void *model_handle,
                                  const float *noise_input,
                                  size_t noise_size,
                                  float *generated_output,
                                  size_t output_size)
{
    float *processed_noise;
    int ret;

    /* Validate parameters */
    ret = validate_gan_params(model_handle, noise_input, noise_size,
                             generated_output, output_size);
    if (ret != 0)
        return ret;

    /* If no noise provided, generate random noise */
    if (!noise_input) {
        processed_noise = kzalloc(noise_size * sizeof(float), GFP_KERNEL);
        if (!processed_noise) {
            pr_err("dsmil: GAN infer: Failed to allocate noise buffer\n");
            return -ENOMEM;
        }

        ret = generate_random_noise(processed_noise, noise_size);
        if (ret != 0) {
            kfree(processed_noise);
            return ret;
        }
    } else {
        /* Use provided noise */
        processed_noise = (float *)noise_input;
    }

    /* Run GAN generator inference */
    ret = dsmil_model_infer_int8(model_handle, processed_noise, noise_size,
                                generated_output, output_size);
    if (ret != 0) {
        pr_err("dsmil: GAN infer: Model inference failed: %d\n", ret);
        if (!noise_input) kfree(processed_noise);
        return ret;
    }

    /* Post-process generated output */
    ret = postprocess_gan_output(generated_output, output_size);
    if (ret != 0) {
        if (!noise_input) kfree(processed_noise);
        return ret;
    }

    /* Validate output quality */
    ret = validate_generated_output(generated_output, output_size);
    if (ret != 0) {
        pr_warn("dsmil: GAN: Output validation failed: %d\n", ret);
        /* Don't fail the operation for validation issues */
    }

    if (!noise_input) kfree(processed_noise);

    pr_debug("dsmil: GAN infer: Generated %zu samples successfully\n", output_size);

    return 0;
}

/*
 * GAN Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
