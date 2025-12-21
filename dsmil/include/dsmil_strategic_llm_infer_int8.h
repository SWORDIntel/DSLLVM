#ifndef _DSMIL_STRATEGIC_LLM_INFER_INT8_H
#define _DSMIL_STRATEGIC_LLM_INFER_INT8_H

#include <linux/types.h>
#include "dsmil_model_apis.h"

/* LLM inference mode definitions */
typedef enum {
    DSMIL_LLM_MODE_DEFENSIVE = 0,           /* Strategic defensive analysis */
    DSMIL_LLM_MODE_ACTIVE_MEASURES = 1,     /* Offensive disinformation operations */
    DSMIL_LLM_MODE_CODE_GENERATION = 2,     /* Malware/exploit code synthesis */
    DSMIL_LLM_MODE_INTELLIGENCE_ANALYSIS = 3 /* Threat intelligence assessment */
} dsmil_llm_inference_mode_t;

/* Security classification levels */
typedef enum {
    DSMIL_CLASS_UNCLASSIFIED = 0,
    DSMIL_CLASS_CONFIDENTIAL = 1,
    DSMIL_CLASS_SECRET = 2,
    DSMIL_CLASS_TOP_SECRET = 3,
    DSMIL_CLASS_TOP_SECRET_SCI = 4
} dsmil_security_classification_t;

/* Maximum number of operational recommendations */
#define DSMIL_MAX_RECOMMENDATIONS 10

/* Additional constants */
#define DSMIL_MAX_INTELLIGENCE_RESULTS 50
#define DSMIL_MAX_FEED_ALERTS 20
#define DSMIL_MAX_FEED_SOURCES 10

/* Strategic LLM result structure */
typedef struct {
    char *response_text;                    /* Generated analysis or code */
    size_t response_length;                 /* Length of response in bytes */
    float confidence_score;                 /* Analysis confidence (0.0-1.0) */
    struct {
        uint32_t recommendation_count;      /* Number of recommendations */
        struct {
            char *action_description;       /* Recommended action */
            uint8_t priority_level;         /* Priority (1-5, 5=highest) */
            uint8_t resource_requirement;   /* Resource needs (1-5, 5=high) */
            uint32_t estimated_timeline_days; /* Implementation time */
            float success_probability;      /* Estimated success rate */
        } recommendations[DSMIL_MAX_RECOMMENDATIONS];
        uint8_t risk_assessment;            /* Overall risk level (1-10) */
        uint8_t operational_feasibility;    /* Implementation difficulty (1-10) */
        char *strategic_rationale;          /* Reasoning for recommendations */
    } recommendations;                      /* Operational recommendations */
    dsmil_security_classification_t classification; /* Content classification */
    uint32_t processing_time_ms;            /* Inference processing time */
    struct {
        uint32_t tokens_generated;          /* Number of tokens generated */
        uint32_t tokens_processed;          /* Number of tokens processed */
        float perplexity_score;             /* Model perplexity score */
        uint8_t mode_used;                  /* Inference mode used */
    } metadata;                             /* Analysis metadata */
} dsmil_strategic_llm_result_t;

#ifdef __KERNEL__
/* Kernel-space API */

/**
 * Initialize strategic LLM runtime
 * @return 0 on success, negative error code on failure
 */
int dsmil_strategic_llm_init_runtime(void);

/**
 * Cleanup strategic LLM runtime
 */
void dsmil_strategic_llm_cleanup_runtime(void);

/**
 * Strategic LLM inference with INT8 optimization
 * @param model_handle Handle to the loaded INT8 strategic LLM model
 * @param input_prompt Strategic analysis query or operational scenario
 * @param prompt_length Length of input prompt in bytes
 * @param mode Inference mode (defensive, active measures, code generation, analysis)
 * @param result Structured result containing analysis or generated content
 * @param flags Inference optimization and security flags
 * @return 0 on success, negative error code on failure
 */
int dsmil_strategic_llm_infer_int8(
    dsmil_model_handle_t model_handle,
    const char *input_prompt,
    size_t prompt_length,
    dsmil_llm_inference_mode_t mode,
    dsmil_strategic_llm_result_t *result,
    dsmil_inference_flags_t flags
);

#else
/* User-space API */

#include <stdint.h>
#include <stddef.h>

/**
 * Strategic LLM inference API for user-space applications
 * @param model_handle Handle to the loaded INT8 strategic LLM model
 * @param input_prompt Strategic analysis query or operational scenario
 * @param prompt_length Length of input prompt in bytes
 * @param mode Inference mode (defensive, active measures, code generation, analysis)
 * @param result Structured result containing analysis or generated content
 * @param flags Inference optimization and security flags
 * @return 0 on success, negative error code on failure
 */
static inline int dsmil_strategic_llm_infer_int8(
    dsmil_model_handle_t model_handle,
    const char *input_prompt,
    size_t prompt_length,
    dsmil_llm_inference_mode_t mode,
    dsmil_strategic_llm_result_t *result,
    dsmil_inference_flags_t flags
) {
    /* User-space stub - actual implementation in DSLLVM runtime */
    (void)model_handle;
    (void)input_prompt;
    (void)prompt_length;
    (void)mode;
    (void)result;
    (void)flags;

    return -ENOSYS; /* Not implemented in user-space */
}

#endif /* __KERNEL__ */

/* Additional handle types for connection points */
typedef struct dsmil_llm_stream_handle_t *dsmil_llm_stream_handle_t;
typedef struct dsmil_llm_conversation_handle_t *dsmil_llm_conversation_handle_t;
typedef struct dsmil_intelligence_db_handle_t *dsmil_intelligence_db_handle_t;
typedef struct dsmil_feed_processor_handle_t *dsmil_feed_processor_handle_t;
typedef struct dsmil_session_handle_t *dsmil_session_handle_t;
typedef struct dsmil_fine_tune_handle_t *dsmil_fine_tune_handle_t;
typedef struct dsmil_cluster_handle_t *dsmil_cluster_handle_t;
typedef struct dsmil_secure_session_handle_t *dsmil_secure_session_handle_t;
typedef struct dsmil_feed_source_handle_t *dsmil_feed_source_handle_t;

/* Additional data structures for connection points */
typedef struct {
    uint32_t max_turns;
    uint32_t context_window_size;
    uint8_t enable_memory_compression;
    uint8_t auto_summarize;
    char *conversation_topic;
    dsmil_security_classification_t security_level;
} dsmil_conversation_config_t;

typedef enum {
    DSMIL_DB_FLAG_READ_ONLY = 0x01,
    DSMIL_DB_FLAG_REAL_TIME_SYNC = 0x02,
    DSMIL_DB_FLAG_OFFLINE_CACHE = 0x04,
    DSMIL_DB_FLAG_ENCRYPTED_QUERY = 0x08,
    DSMIL_DB_FLAG_AUDIT_LOGGING = 0x10
} dsmil_db_connection_flags_t;

typedef struct {
    uint32_t result_count;
    struct {
        char *intelligence_snippet;
        float relevance_score;
        dsmil_security_classification_t classification;
        char *source_reference;
        uint32_t timestamp;
    } results[50];
    uint8_t query_confidence;
    char *query_summary;
} dsmil_intelligence_results_t;

typedef struct {
    uint32_t feed_buffer_size;
    uint32_t analysis_interval_ms;
    uint8_t real_time_processing;
    uint8_t anomaly_detection;
    char *feed_source_type;
    dsmil_security_classification_t feed_classification;
} dsmil_feed_config_t;

typedef struct {
    uint8_t anomaly_detected;
    float anomaly_confidence;
    char *analysis_summary;
    struct {
        uint32_t alert_count;
        struct {
            char *alert_description;
            uint8_t severity_level;
            char *recommended_action;
        } alerts[20];
    } alerts;
    uint32_t processed_feed_chunks;
    uint32_t analysis_timestamp;
} dsmil_feed_analysis_result_t;

typedef struct {
    uint32_t max_analysts;
    uint32_t session_timeout_minutes;
    uint8_t real_time_collaboration;
    uint8_t consensus_required;
    char *session_objective;
    dsmil_security_classification_t session_classification;
} dsmil_session_config_t;

typedef struct {
    uint32_t analyst_id;
    char *analyst_name;
    void *clearance;
    char *specialization_area;
    uint8_t real_time_participation;
} dsmil_analyst_id_t;

typedef enum {
    DSMIL_SESSION_PERM_READ = 0x01,
    DSMIL_SESSION_PERM_WRITE = 0x02,
    DSMIL_SESSION_PERM_ADMIN = 0x04,
    DSMIL_SESSION_PERM_MODERATE = 0x08,
    DSMIL_SESSION_PERM_ANALYZE = 0x10
} dsmil_session_permissions_t;

#endif /* _DSMIL_STRATEGIC_LLM_INFER_INT8_H */
