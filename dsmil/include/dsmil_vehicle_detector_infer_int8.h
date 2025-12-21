#ifndef _DSMIL_VEHICLE_DETECTOR_INFER_INT8_H
#define _DSMIL_VEHICLE_DETECTOR_INFER_INT8_H

#include <linux/types.h>
#include "dsmil_model_apis.h"

/* Vehicle detection result structure */
typedef struct {
    uint32_t vehicle_class;           /* Vehicle type classification */
    float confidence;                 /* Detection confidence (0.0-1.0) */
    struct {
        float x_min, y_min;           /* Top-left coordinates (normalized 0.0-1.0) */
        float x_max, y_max;           /* Bottom-right coordinates (normalized 0.0-1.0) */
        float width, height;          /* Bounding box dimensions in pixels */
    } bbox;
    uint32_t tracking_id;             /* Optional tracking identifier */
    struct {
        uint8_t direction;            /* Movement direction (0-359 degrees) */
        float speed_kmh;              /* Estimated speed in km/h */
        uint8_t occlusion;            /* Occlusion level (0-100%) */
        uint8_t lighting;             /* Lighting conditions confidence (0-255) */
        uint8_t weather;              /* Weather impact assessment (0-255) */
    } attrs;                          /* Vehicle-specific attributes */
} dsmil_vehicle_detection_result_t;

/* Vehicle class definitions */
#define DSMIL_VEHICLE_CLASS_CAR          0
#define DSMIL_VEHICLE_CLASS_TRUCK        1
#define DSMIL_VEHICLE_CLASS_BUS          2
#define DSMIL_VEHICLE_CLASS_MOTORCYCLE   3
#define DSMIL_VEHICLE_CLASS_BICYCLE      4
#define DSMIL_VEHICLE_CLASS_UNKNOWN      255

/* Maximum number of detections */
#define DSMIL_MAX_VEHICLE_DETECTIONS     100

#ifdef __KERNEL__
/* Kernel-space API */

/**
 * Initialize vehicle detector runtime
 * @return 0 on success, negative error code on failure
 */
int dsmil_vehicle_detector_init_runtime(void);

/**
 * Cleanup vehicle detector runtime
 */
void dsmil_vehicle_detector_cleanup_runtime(void);

/**
 * Vehicle detector inference with INT8 optimization
 * @param model_handle Handle to the loaded INT8 vehicle detection model
 * @param input_image Pointer to RGB image data
 * @param input_size Size of input image in bytes
 * @param results Array to store detection results
 * @param max_results Maximum number of results to return
 * @param confidence_threshold Minimum confidence for detections
 * @param flags Inference optimization flags
 * @return 0 on success, negative error code on failure
 */
int dsmil_vehicle_detector_infer_int8(
    dsmil_model_handle_t model_handle,
    const uint8_t *input_image,
    size_t input_size,
    dsmil_vehicle_detection_result_t *results,
    size_t max_results,
    float confidence_threshold,
    dsmil_inference_flags_t flags
);

#else
/* User-space API */

#include <stdint.h>
#include <stddef.h>

/**
 * Vehicle detector inference API for user-space applications
 * @param model_handle Handle to the loaded INT8 vehicle detection model
 * @param input_image Pointer to RGB image data
 * @param input_size Size of input image in bytes
 * @param results Array to store detection results
 * @param max_results Maximum number of results to return
 * @param confidence_threshold Minimum confidence for detections
 * @param flags Inference optimization flags
 * @return 0 on success, negative error code on failure
 */
static inline int dsmil_vehicle_detector_infer_int8(
    dsmil_model_handle_t model_handle,
    const uint8_t *input_image,
    size_t input_size,
    dsmil_vehicle_detection_result_t *results,
    size_t max_results,
    float confidence_threshold,
    dsmil_inference_flags_t flags
) {
    /* User-space stub - actual implementation in DSLLVM runtime */
    (void)model_handle;
    (void)input_image;
    (void)input_size;
    (void)results;
    (void)max_results;
    (void)confidence_threshold;
    (void)flags;

    return -ENOSYS; /* Not implemented in user-space */
}

#endif /* __KERNEL__ */

#endif /* _DSMIL_VEHICLE_DETECTOR_INFER_INT8_H */
