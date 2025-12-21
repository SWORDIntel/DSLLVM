#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/atomic.h>

#include "dsmil_device255_crypto.h"
#include "dsmil_int8_model_load_runtime.h"
#include "dsmil_model_infer_int8_runtime.h"
#include "dsmil_vehicle_detector_infer_int8.h"

/* Vehicle detection runtime context */
struct dsmil_vehicle_detector_ctx {
    dsmil_model_handle_t model_handle;
    atomic_t active_inferences;
    struct mutex inference_lock;
    uint32_t max_concurrent_inferences;
    uint32_t inference_timeout_ms;
    dsmil_vehicle_detector_config_t config;
};

/* Vehicle detection result structure */
typedef struct {
    uint32_t vehicle_class;
    float confidence;
    struct {
        float x_min, y_min, x_max, y_max;
        float width, height;
    } bbox;
    uint32_t tracking_id;
    struct {
        uint8_t direction;
        float speed_kmh;
        uint8_t occlusion;
        uint8_t lighting;
        uint8_t weather;
    } attributes;
} dsmil_vehicle_detection_result_int_t;

/* Vehicle detection configuration */
typedef struct {
    uint32_t input_width;
    uint32_t input_height;
    uint32_t max_detections;
    float confidence_threshold;
    uint8_t enable_tracking;
    uint8_t enable_attributes;
    uint8_t hardware_acceleration;
} dsmil_vehicle_detector_config_t;

/* Global vehicle detector context */
static struct dsmil_vehicle_detector_ctx *g_vehicle_detector_ctx = NULL;
static DEFINE_MUTEX(g_vehicle_detector_init_lock);

/* Forward declarations */
static int dsmil_vehicle_detector_init_internal(void);
static int dsmil_vehicle_detector_inference_int8_internal(
    const uint8_t *input_image,
    size_t input_size,
    dsmil_vehicle_detection_result_int_t *results,
    size_t max_results,
    float confidence_threshold
);
static void dsmil_vehicle_detector_cleanup_internal(void);

/**
 * Initialize vehicle detector runtime
 */
int dsmil_vehicle_detector_init_runtime(void)
{
    int ret;

    mutex_lock(&g_vehicle_detector_init_lock);

    if (g_vehicle_detector_ctx) {
        mutex_unlock(&g_vehicle_detector_init_lock);
        return 0; /* Already initialized */
    }

    ret = dsmil_vehicle_detector_init_internal();
    if (ret) {
        pr_err("dsmil: Failed to initialize vehicle detector runtime: %d\n", ret);
        mutex_unlock(&g_vehicle_detector_init_lock);
        return ret;
    }

    pr_info("dsmil: Vehicle detector runtime initialized successfully\n");
    mutex_unlock(&g_vehicle_detector_init_lock);

    return 0;
}

/**
 * Cleanup vehicle detector runtime
 */
void dsmil_vehicle_detector_cleanup_runtime(void)
{
    mutex_lock(&g_vehicle_detector_init_lock);

    if (g_vehicle_detector_ctx) {
        dsmil_vehicle_detector_cleanup_internal();
        g_vehicle_detector_ctx = NULL;
    }

    mutex_unlock(&g_vehicle_detector_init_lock);
}

/**
 * Internal initialization function
 */
static int dsmil_vehicle_detector_init_internal(void)
{
    int ret;

    g_vehicle_detector_ctx = kzalloc(sizeof(*g_vehicle_detector_ctx), GFP_KERNEL);
    if (!g_vehicle_detector_ctx) {
        return -ENOMEM;
    }

    /* Initialize context */
    atomic_set(&g_vehicle_detector_ctx->active_inferences, 0);
    mutex_init(&g_vehicle_detector_ctx->inference_lock);
    g_vehicle_detector_ctx->max_concurrent_inferences = 4;
    g_vehicle_detector_ctx->inference_timeout_ms = 5000;

    /* Configure vehicle detector */
    g_vehicle_detector_ctx->config.input_width = 640;
    g_vehicle_detector_ctx->config.input_height = 640;
    g_vehicle_detector_ctx->config.max_detections = 100;
    g_vehicle_detector_ctx->config.confidence_threshold = 0.5f;
    g_vehicle_detector_ctx->config.enable_tracking = 1;
    g_vehicle_detector_ctx->config.enable_attributes = 1;
    g_vehicle_detector_ctx->config.hardware_acceleration = 1;

    /* Load vehicle detection model */
    ret = dsmil_int8_model_load_runtime("vehicle_detector_yolov8_int8.onnx",
                                      &g_vehicle_detector_ctx->model_handle);
    if (ret) {
        pr_err("dsmil: Failed to load vehicle detector model: %d\n", ret);
        kfree(g_vehicle_detector_ctx);
        g_vehicle_detector_ctx = NULL;
        return ret;
    }

    pr_info("dsmil: Vehicle detector model loaded successfully\n");
    return 0;
}

/**
 * Internal cleanup function
 */
static void dsmil_vehicle_detector_cleanup_internal(void)
{
    if (!g_vehicle_detector_ctx) {
        return;
    }

    /* Wait for active inferences to complete */
    while (atomic_read(&g_vehicle_detector_ctx->active_inferences) > 0) {
        msleep(100);
    }

    /* Unload model */
    if (g_vehicle_detector_ctx->model_handle) {
        dsmil_int8_model_unload_runtime(g_vehicle_detector_ctx->model_handle);
    }

    /* Cleanup context */
    mutex_destroy(&g_vehicle_detector_ctx->inference_lock);
    kfree(g_vehicle_detector_ctx);
}

/**
 * Vehicle detector inference with INT8 optimization
 */
int dsmil_vehicle_detector_infer_int8(
    dsmil_model_handle_t model_handle,
    const uint8_t *input_image,
    size_t input_size,
    dsmil_vehicle_detection_result_t *results,
    size_t max_results,
    float confidence_threshold,
    dsmil_inference_flags_t flags
)
{
    dsmil_vehicle_detection_result_int_t *internal_results;
    size_t num_detections = 0;
    int ret;

    if (!g_vehicle_detector_ctx) {
        return -EINVAL;
    }

    if (!input_image || !results || max_results == 0) {
        return -EINVAL;
    }

    /* Check concurrent inference limits */
    if (atomic_read(&g_vehicle_detector_ctx->active_inferences) >=
        g_vehicle_detector_ctx->max_concurrent_inferences) {
        return -EBUSY;
    }

    /* Allocate internal results buffer */
    internal_results = kzalloc(sizeof(*internal_results) * max_results, GFP_KERNEL);
    if (!internal_results) {
        return -ENOMEM;
    }

    atomic_inc(&g_vehicle_detector_ctx->active_inferences);
    mutex_lock(&g_vehicle_detector_ctx->inference_lock);

    /* Perform inference */
    ret = dsmil_vehicle_detector_inference_int8_internal(
        input_image,
        input_size,
        internal_results,
        max_results,
        confidence_threshold
    );

    mutex_unlock(&g_vehicle_detector_ctx->inference_lock);
    atomic_dec(&g_vehicle_detector_ctx->active_inferences);

    if (ret == 0) {
        /* Convert internal results to API format */
        for (size_t i = 0; i < max_results; i++) {
            if (internal_results[i].confidence < confidence_threshold) {
                break;
            }

            results[i].vehicle_class = internal_results[i].vehicle_class;
            results[i].confidence = internal_results[i].confidence;
            results[i].bbox.x_min = internal_results[i].bbox.x_min;
            results[i].bbox.y_min = internal_results[i].bbox.y_min;
            results[i].bbox.x_max = internal_results[i].bbox.x_max;
            results[i].bbox.y_max = internal_results[i].bbox.y_max;
            results[i].bbox.width = internal_results[i].bbox.width;
            results[i].bbox.height = internal_results[i].bbox.height;
            results[i].tracking_id = internal_results[i].tracking_id;
            results[i].attrs.direction = internal_results[i].attributes.direction;
            results[i].attrs.speed_kmh = internal_results[i].attributes.speed_kmh;
            results[i].attrs.occlusion = internal_results[i].attributes.occlusion;
            results[i].attrs.lighting = internal_results[i].attributes.lighting;
            results[i].attrs.weather = internal_results[i].attributes.weather;

            num_detections++;
        }
    }

    kfree(internal_results);
    return ret;
}

/**
 * Internal inference implementation
 */
static int dsmil_vehicle_detector_inference_int8_internal(
    const uint8_t *input_image,
    size_t input_size,
    dsmil_vehicle_detection_result_int_t *results,
    size_t max_results,
    float confidence_threshold
)
{
    uint8_t *processed_input = NULL;
    float *model_output = NULL;
    size_t output_size;
    int ret;

    if (!g_vehicle_detector_ctx || !g_vehicle_detector_ctx->model_handle) {
        return -EINVAL;
    }

    /* Validate input image size */
    size_t expected_size = g_vehicle_detector_ctx->config.input_width *
                          g_vehicle_detector_ctx->config.input_height * 3;
    if (input_size != expected_size) {
        pr_err("dsmil: Invalid input image size: %zu (expected %zu)\n",
               input_size, expected_size);
        return -EINVAL;
    }

    /* Preprocess input image */
    processed_input = kzalloc(input_size, GFP_KERNEL);
    if (!processed_input) {
        return -ENOMEM;
    }

    /* Copy and preprocess image data */
    memcpy(processed_input, input_image, input_size);

    /* Apply image preprocessing (normalization, etc.) */
    ret = dsmil_vehicle_detector_preprocess_image(
        processed_input,
        g_vehicle_detector_ctx->config.input_width,
        g_vehicle_detector_ctx->config.input_height
    );
    if (ret) {
        kfree(processed_input);
        return ret;
    }

    /* Allocate model output buffer */
    output_size = g_vehicle_detector_ctx->config.max_detections *
                 sizeof(float) * 6; /* bbox + class + confidence */
    model_output = kzalloc(output_size, GFP_KERNEL);
    if (!model_output) {
        kfree(processed_input);
        return -ENOMEM;
    }

    /* Perform model inference */
    ret = dsmil_model_infer_int8_runtime(
        g_vehicle_detector_ctx->model_handle,
        processed_input,
        input_size,
        model_output,
        &output_size
    );

    kfree(processed_input);

    if (ret) {
        kfree(model_output);
        pr_err("dsmil: Vehicle detector inference failed: %d\n", ret);
        return ret;
    }

    /* Post-process results */
    ret = dsmil_vehicle_detector_postprocess_results(
        model_output,
        output_size,
        results,
        max_results,
        confidence_threshold,
        g_vehicle_detector_ctx->config.input_width,
        g_vehicle_detector_ctx->config.input_height
    );

    kfree(model_output);
    return ret;
}

/**
 * Image preprocessing function
 */
static int dsmil_vehicle_detector_preprocess_image(
    uint8_t *image_data,
    uint32_t width,
    uint32_t height
)
{
    /* RGB to model input format conversion */
    /* Normalization and channel ordering */
    size_t total_pixels = width * height;

    /* Convert RGB to BGR and normalize to 0-1 range */
    for (size_t i = 0; i < total_pixels; i++) {
        uint8_t r = image_data[i * 3 + 0];
        uint8_t g = image_data[i * 3 + 1];
        uint8_t b = image_data[i * 3 + 2];

        /* BGR conversion and normalization */
        image_data[i * 3 + 0] = (uint8_t)(b * 0.00784313725490196f); /* B / 127.5 */
        image_data[i * 3 + 1] = (uint8_t)(g * 0.00784313725490196f); /* G / 127.5 */
        image_data[i * 3 + 2] = (uint8_t)(r * 0.00784313725490196f); /* R / 127.5 */
    }

    return 0;
}

/**
 * Result post-processing function
 */
static int dsmil_vehicle_detector_postprocess_results(
    const float *model_output,
    size_t output_size,
    dsmil_vehicle_detection_result_int_t *results,
    size_t max_results,
    float confidence_threshold,
    uint32_t input_width,
    uint32_t input_height
)
{
    size_t num_detections = output_size / (sizeof(float) * 6);
    size_t valid_detections = 0;

    if (num_detections > max_results) {
        num_detections = max_results;
    }

    /* Process each detection */
    for (size_t i = 0; i < num_detections; i++) {
        const float *detection = &model_output[i * 6];

        float x_center = detection[0];
        float y_center = detection[1];
        float width = detection[2];
        float height = detection[3];
        float confidence = detection[4];
        uint32_t class_id = (uint32_t)detection[5];

        /* Filter by confidence */
        if (confidence < confidence_threshold) {
            continue;
        }

        /* Convert to bounding box format */
        float x_min = (x_center - width / 2.0f) / input_width;
        float y_min = (y_center - height / 2.0f) / input_height;
        float x_max = (x_center + width / 2.0f) / input_width;
        float y_max = (y_center + height / 2.0f) / input_height;

        /* Clamp coordinates */
        x_min = max(0.0f, min(1.0f, x_min));
        y_min = max(0.0f, min(1.0f, y_min));
        x_max = max(0.0f, min(1.0f, x_max));
        y_max = max(0.0f, min(1.0f, y_max));

        /* Store result */
        results[valid_detections].vehicle_class = class_id;
        results[valid_detections].confidence = confidence;
        results[valid_detections].bbox.x_min = x_min;
        results[valid_detections].bbox.y_min = y_min;
        results[valid_detections].bbox.x_max = x_max;
        results[valid_detections].bbox.y_max = y_max;
        results[valid_detections].bbox.width = (x_max - x_min) * input_width;
        results[valid_detections].bbox.height = (y_max - y_min) * input_height;
        results[valid_detections].tracking_id = valid_detections; /* Simple tracking */

        /* Generate vehicle attributes */
        results[valid_detections].attributes.direction = 0; /* Placeholder */
        results[valid_detections].attributes.speed_kmh = 0.0f; /* Placeholder */
        results[valid_detections].attributes.occlusion = 0; /* Placeholder */
        results[valid_detections].attributes.lighting = 128; /* Neutral */
        results[valid_detections].attributes.weather = 128; /* Neutral */

        valid_detections++;
    }

    pr_debug("dsmil: Vehicle detector found %zu valid detections\n", valid_detections);
    return 0;
}

/* Runtime registration */
static int __init dsmil_vehicle_detector_runtime_init(void)
{
    pr_info("dsmil: Vehicle detector runtime module loaded\n");
    return 0;
}

static void __exit dsmil_vehicle_detector_runtime_exit(void)
{
    dsmil_vehicle_detector_cleanup_runtime();
    pr_info("dsmil: Vehicle detector runtime module unloaded\n");
}

module_init(dsmil_vehicle_detector_runtime_init);
module_exit(dsmil_vehicle_detector_runtime_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSMIL Development Team");
MODULE_DESCRIPTION("Vehicle Detection INT8 Runtime for DSLLVM");
MODULE_VERSION("1.0.0");
