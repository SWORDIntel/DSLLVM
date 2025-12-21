/*
 * DSMIL GNN Runtime Implementation
 *
 * This file implements the GNN inference API for event correlation
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

/**
 * @brief Validate GNN inference parameters
 */
static int validate_gnn_params(void *model_handle,
                              float **node_features,
                              size_t num_nodes,
                              float **edge_features,
                              size_t num_edges,
                              void *adjacency_matrix,
                              float **node_embeddings,
                              int *cluster_assignments)
{
    if (!model_handle || !node_features || !edge_features ||
        !adjacency_matrix || !node_embeddings || !cluster_assignments) {
        pr_err("dsmil: GNN infer: Invalid parameters\n");
        return -EINVAL;
    }

    if (num_nodes == 0 || num_nodes > GNN_MAX_NODES) {
        pr_err("dsmil: GNN infer: Invalid node count: %zu\n", num_nodes);
        return -EINVAL;
    }

    if (num_edges == 0 || num_edges > (num_nodes * num_nodes)) {
        pr_err("dsmil: GNN infer: Invalid edge count: %zu\n", num_edges);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Convert graph data to model input format
 *
 * Prepares the graph data (node features, edge features, adjacency matrix)
 * into the format expected by the GNN model.
 */
static int prepare_gnn_input(float **node_features,
                            size_t num_nodes,
                            float **edge_features,
                            size_t num_edges,
                            void *adjacency_matrix,
                            float *model_input,
                            size_t *input_size)
{
    size_t total_features = 0;
    size_t i, j;

    /* Calculate total input size */
    *input_size = num_nodes * GNN_NODE_FEATURES;

    if (*input_size > GNN_MAX_NODES * GNN_NODE_FEATURES) {
        pr_err("dsmil: GNN prepare: Input size too large: %zu\n", *input_size);
        return -EINVAL;
    }

    /* Flatten node features into model input */
    for (i = 0; i < num_nodes; i++) {
        if (!node_features[i]) {
            pr_err("dsmil: GNN prepare: NULL node features at index %zu\n", i);
            return -EINVAL;
        }

        memcpy(&model_input[i * GNN_NODE_FEATURES],
               node_features[i],
               GNN_NODE_FEATURES * sizeof(float));
    }

    /* Note: In a full implementation, edge features and adjacency matrix
     * would also be incorporated into the model input. For this basic
     * implementation, we only use node features. */

    return 0;
}

/**
 * @brief Process GNN model output
 *
 * Converts the raw model output into node embeddings and cluster assignments.
 */
static int process_gnn_output(const float *model_output,
                             size_t output_size,
                             size_t num_nodes,
                             float **node_embeddings,
                             int *cluster_assignments)
{
    size_t i, j;
    size_t embedding_size = GNN_EMBEDDING_SIZE;

    /* Extract node embeddings from model output */
    for (i = 0; i < num_nodes; i++) {
        if (!node_embeddings[i]) {
            pr_err("dsmil: GNN process: NULL embedding buffer at index %zu\n", i);
            return -EINVAL;
        }

        /* Copy embedding for this node */
        memcpy(node_embeddings[i],
               &model_output[i * embedding_size],
               embedding_size * sizeof(float));

        /* Determine cluster assignment (simple argmax on embedding) */
        float max_val = node_embeddings[i][0];
        int max_idx = 0;

        for (j = 1; j < embedding_size; j++) {
            if (node_embeddings[i][j] > max_val) {
                max_val = node_embeddings[i][j];
                max_idx = j;
            }
        }

        cluster_assignments[i] = max_idx;
    }

    return 0;
}

/**
 * @brief GNN inference for event correlation implementation
 */
int dsmil_gnn_infer_int8(void *model_handle,
                        float **node_features,
                        size_t num_nodes,
                        float **edge_features,
                        size_t num_edges,
                        void *adjacency_matrix,
                        float **node_embeddings,
                        int *cluster_assignments)
{
    float *model_input;
    float *model_output;
    size_t input_size, output_size;
    int ret;

    /* Validate parameters */
    ret = validate_gnn_params(model_handle, node_features, num_nodes,
                             edge_features, num_edges, adjacency_matrix,
                             node_embeddings, cluster_assignments);
    if (ret != 0)
        return ret;

    /* Allocate model input buffer */
    input_size = num_nodes * GNN_NODE_FEATURES;
    model_input = kzalloc(input_size * sizeof(float), GFP_KERNEL);
    if (!model_input) {
        pr_err("dsmil: GNN infer: Failed to allocate input buffer\n");
        return -ENOMEM;
    }

    /* Allocate model output buffer */
    output_size = num_nodes * GNN_EMBEDDING_SIZE;
    model_output = kzalloc(output_size * sizeof(float), GFP_KERNEL);
    if (!model_output) {
        kfree(model_input);
        pr_err("dsmil: GNN infer: Failed to allocate output buffer\n");
        return -ENOMEM;
    }

    /* Prepare input data */
    ret = prepare_gnn_input(node_features, num_nodes, edge_features, num_edges,
                           adjacency_matrix, model_input, &input_size);
    if (ret != 0) {
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Run GNN inference */
    ret = dsmil_model_infer_int8(model_handle, model_input, input_size,
                                model_output, output_size);
    if (ret != 0) {
        pr_err("dsmil: GNN infer: Model inference failed: %d\n", ret);
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Process output data */
    ret = process_gnn_output(model_output, output_size, num_nodes,
                            node_embeddings, cluster_assignments);
    if (ret != 0) {
        kfree(model_input);
        kfree(model_output);
        return ret;
    }

    /* Cleanup */
    kfree(model_input);
    kfree(model_output);

    pr_debug("dsmil: GNN infer: Processed %zu nodes successfully\n", num_nodes);
    return 0;
}

/*
 * GNN Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
