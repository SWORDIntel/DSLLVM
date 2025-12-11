# Layer 8 Adversarial Defense Training Specification

## Overview

The Adversarial Defense Training system uses Device 52 (Adversarial ML Defense, 30 TOPS INT8) to harden ML models against adversarial attacks through GAN-based adversarial example generation and adversarial training.

## Architecture

### Components

1. **GAN Generator**: INT8 quantized generator network for adversarial example generation
2. **Target Model**: The ML model to be hardened (any model architecture)
3. **Adversarial Training Loop**: Min-max optimization with mixed clean/adversarial batches
4. **Robustness Evaluator**: Tests against FGSM, PGD, and C&W attacks

## GAN-Based Adversarial Generation

### Generator Architecture

- **Input**: Noise vector (100 dimensions) + original sample
- **Output**: Adversarial perturbation vector (128 dimensions)
- **Architecture**: Generator network (INT8 quantized)
- **Perturbation Budget**: L∞ norm ≤ ε (ε = 0.03 for images, 0.1 for features)
- **Device**: Device 52 (30 TOPS INT8)

### Generation Process

1. **Input Preparation**:
   - Extract original sample features
   - Generate random noise vector z ~ Uniform(-1, 1)
   - Concatenate: [original_sample, noise_vector]

2. **GAN Inference**:
   - Run INT8 quantized generator forward pass
   - Output: perturbation vector δ
   - Clip perturbation: δ = clip(δ, -ε, ε)

3. **Adversarial Sample Creation**:
   - x_adv = x_original + δ
   - Verify: ||x_adv - x_original||_∞ ≤ ε

## Adversarial Training Pipeline

### Training Configuration

- **Batch Size**: 32 (16 clean + 16 adversarial)
- **Optimizer**: Adam with learning rate scheduling
- **Initial Learning Rate**: 0.001
- **Learning Rate Decay**: Halve every 3 epochs
- **Max Epochs**: 10 (with early stopping)
- **Loss Function**: Standard loss + adversarial loss
- **Quantization**: INT8 quantized training on NPU/GPU

### Training Loop

```
For each epoch:
    1. Shuffle training data
    2. For each batch:
       a. Generate adversarial examples for batch samples
       b. Mix clean and adversarial samples (50/50)
       c. Forward pass through model
       d. Compute loss (standard + adversarial)
       e. Backward pass (INT8 quantized gradients)
       f. Update model weights
    3. Validate on validation set
    4. Evaluate robust accuracy periodically
    5. Decay learning rate if needed
    6. Early stopping if robust accuracy plateaus
```

### Loss Function

```
L_total = L_clean + λ * L_adversarial

Where:
- L_clean: Standard cross-entropy loss on clean samples
- L_adversarial: Cross-entropy loss on adversarial samples
- λ: Adversarial loss weight (typically 0.5-1.0)
```

## Robustness Evaluation

### Attack Methods

1. **FGSM (Fast Gradient Sign Method)**:
   - Single-step attack
   - δ = ε * sign(∇_x L(x, y))
   - Typically easiest to defend against

2. **PGD (Projected Gradient Descent)**:
   - Multi-step iterative attack
   - Stronger than FGSM
   - Projected onto L∞ ball

3. **C&W (Carlini & Wagner)**:
   - Optimization-based attack
   - Strongest attack method
   - Most challenging to defend against

### Evaluation Metrics

- **Clean Accuracy**: Accuracy on clean (unperturbed) test set
- **Robust Accuracy**: Accuracy on adversarial test set
- **Robustness Score**: Average accuracy across all attack methods
- **Robustness Gap**: Difference between clean and robust accuracy

### Target Metrics

- **Robust Accuracy**: >85% (target)
- **Robustness Gap**: <5% (target)
- **Clean Accuracy Retention**: >90% (should not degrade significantly)

## Model Export

### Export Format

- **Format**: ONNX-INT8 or TensorFlow Lite INT8
- **Includes**:
  - Model weights (INT8 quantized)
  - Model architecture
  - Quantization parameters
  - Robustness metrics (clean accuracy, robust accuracy, robustness score)
  - Training metadata (epochs, learning rate schedule)

### Model Metadata

```json
{
  "model_type": "adversarial_defense",
  "version": "1.0.0",
  "clean_accuracy": 0.92,
  "robust_accuracy": 0.85,
  "robustness_score": 0.85,
  "training_epochs": 5,
  "quantization": "INT8",
  "device": "Device 52 (30 TOPS INT8)",
  "perturbation_epsilon": 0.03,
  "attack_methods": ["FGSM", "PGD", "C&W"]
}
```

## Integration

### API Usage

```c
// Train adversarial defense
int result = dsmil_layer8_train_adversarial_defense(
    "model.onnx",                    // Input model path
    adversarial_samples,              // Adversarial samples buffer
    num_samples,                      // Number of samples
    "hardened_model.onnx"            // Output hardened model path
);
```

### Training Context

The training context (`adversarial_training_ctx_t`) tracks:
- Model and GAN generator handles
- Generated adversarial samples
- Training/validation loss history
- Clean and robust accuracy metrics
- Robustness scores

## Performance Requirements

- **Training Time**: <1 hour for typical models (Device 52, 30 TOPS INT8)
- **Memory Usage**: <100 MB for model + GAN + samples
- **Throughput**: >1000 samples/second adversarial generation
- **Robustness**: >85% robust accuracy, <5% robustness gap

## References

- Device 52 Specification: 30 TOPS INT8 capacity
- INT8 Quantization: See `dsmil_int8_quantization.h`
- GAN Architecture: Generator network for perturbation generation
- Adversarial Training: Min-max optimization framework

