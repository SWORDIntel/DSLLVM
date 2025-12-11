# Layer 8 Anomaly Detection Model

**Version**: 1.0.0  
**Device**: Device 51 (Enhanced Security Framework)  
**Layer**: Layer 8 (ENHANCED_SEC)  
**Status**: Integrated with AI Intelligence Flow System

---

## Overview

The Layer 8 Anomaly Detection Model provides real-time anomaly detection for system behavior data using INT8 quantized machine learning models running on Device 51's 15 TOPS INT8 capacity. The model is integrated into the DSMIL intelligence flow system as a pipeline hook, automatically processing behavior data events from lower layers and publishing security intelligence upward to Layer 9 (Executive).

## Model Architecture

### Architecture Choice: 1D CNN Autoencoder

**Model Type**: Lightweight 1D Convolutional Neural Network Autoencoder  
**Purpose**: Anomaly detection via reconstruction error  
**Quantization**: INT8 post-training quantization  
**Model Size**: ~50K parameters (fits Device 51's 15 TOPS INT8 capacity)

### Architecture Details

```
Input: 262 features
  ↓
Encoder:
  - Conv1D(262 → 128) + ReLU
  - Conv1D(128 → 64) + ReLU
  - Conv1D(64 → 32) + ReLU
  ↓
Bottleneck:
  - Dense(32 → 16)
  ↓
Decoder:
  - Dense(16 → 32)
  - Conv1D(32 → 64) + ReLU
  - Conv1D(64 → 128) + ReLU
  - Conv1D(128 → 262) + ReLU
  ↓
Output: Reconstruction error (anomaly score: 0.0-1.0)
```

### Training Pipeline

1. **Data Preparation**
   - Collect normal behavior data from Layers 3-7
   - Extract features using defined schema (see Feature Schema below)
   - Create training dataset with normal behavior patterns

2. **Normalization**
   - Z-score normalization per feature
   - Store normalization parameters (mean, std) for inference

3. **Baseline Fitting**
   - Train autoencoder on normal data only
   - Minimize reconstruction error on normal patterns
   - Validate on held-out normal data

4. **Quantization**
   - Post-training INT8 quantization
   - Calibration dataset: representative normal behavior samples
   - Quantization-aware fine-tuning (optional)

5. **Export**
   - Format: ONNX-INT8 or TensorFlow Lite INT8
   - Model file: `anomaly_detector_int8.onnx` or `anomaly_detector_int8.tflite`
   - Include normalization parameters in model metadata

## Feature Schema

### Input Features (262 total)

**Statistical Features (6)**:
- `mean`: Mean value of data window
- `std_dev`: Standard deviation
- `min`: Minimum value
- `max`: Maximum value
- `range`: Value range (max - min)
- `entropy`: Shannon entropy of byte distribution

**Distribution Features (256)**:
- `histogram[0..255]`: Byte value histogram (256 bins)

### Feature Extraction

**Window Size**: First 10KB of behavior data (capped for performance)  
**Preprocessing**:
1. Read up to 10KB from `behavior_data`
2. Calculate statistical features (mean, std_dev, min, max, range)
3. Build 256-bin histogram of byte values
4. Calculate entropy from histogram
5. Normalize features (Z-score using training statistics)

### Baseline Statistics

These are learned from training data and used for:
- Statistical heuristic fallback (when model unavailable)
- Feature normalization
- Threshold calibration

**Default Baselines** (placeholder - should be learned):
- `baseline_mean`: 128.0
- `baseline_std_dev`: 50.0
- `baseline_range`: 200.0
- `expected_entropy`: 7.5

## Inference Path

### Model Loading

**Location**: Model should be loaded at Layer 8 initialization  
**API** (placeholder):
```c
// In dsmil_layer8_security_init():
dsmil_model_load("anomaly_detector_int8.onnx", &g_anomaly_model);
```

**Fallback**: If model unavailable, uses statistical heuristics

### Inference Flow

1. **Feature Extraction**
   - Extract 262 features from `behavior_data`
   - Normalize features using stored normalization parameters

2. **Model Inference**
   - Run INT8 quantized model inference
   - Get reconstruction error as anomaly score
   - Score range: [0.0, 1.0] where higher = more anomalous

3. **Score Interpretation**
   - `score < 0.1`: Normal behavior (no action)
   - `0.1 ≤ score < 0.4`: Low anomaly (log, monitor)
   - `0.4 ≤ score < 0.7`: Medium anomaly (alert, investigate)
   - `score ≥ 0.7`: High anomaly (critical alert, immediate action)

4. **Risk Score Calculation**
   - `overall_risk = anomaly_score`
   - `threat_probability = anomaly_score * 0.9`
   - `impact_score = anomaly_score * 0.8`
   - `confidence`: Based on data quality and anomaly strength

## AI System Integration

### Intelligence Flow Integration

The anomaly detection model is integrated as a **pipeline hook** in the DSMIL intelligence flow system:

**Subscription**:
- Subscribes to `DSMIL_INTEL_RAW_DATA` events (Layer 3)
- Subscribes to `DSMIL_INTEL_DOMAIN_ANALYTICS` events (Layer 3)
- Processes events via `dsmil_layer8_anomaly_event_handler()` callback

**Event Processing**:
1. Receive behavior data event from lower layer
2. Extract payload (behavior_data)
3. Run anomaly detection
4. If anomaly score ≥ 0.1, publish security event

**Publishing**:
- Event type: `DSMIL_INTEL_SECURITY`
- Source: Layer 8, Device 51
- Target: Layer 9 (Executive)
- Payload: `dsmil_security_risk_t` structure with anomaly results
- Clearance: Layer 8 security clearance (mask: 0x8)

### Event Flow Diagram

```
Lower Layers (3-7)
  ↓ Publish RAW_DATA/DOMAIN_ANALYTICS events
Intelligence Flow Bus
  ↓ Event callback
Layer 8 Event Handler
  ↓ Process event
Anomaly Detection Model
  ↓ Calculate anomaly score
Risk Assessment
  ↓ If score ≥ 0.1
Publish DSMIL_INTEL_SECURITY event
  ↓
Layer 9 (Executive)
```

## Resource Constraints

### Device 51 Constraints

- **TOPS Capacity**: 15 TOPS INT8
- **Memory Budget**: 8 GB (shared Layer 8 pool)
- **Model Size**: ~50K parameters (~200 KB INT8)
- **Inference Latency**: < 5ms target (15 TOPS allows ~75K ops)

### Memory Budget Management

- Check memory budget before model execution
- Track memory usage: `ctx.memory_used_bytes += data_size`
- Fail gracefully if budget exceeded (return error, don't crash)

### Performance Optimizations

- **Window Capping**: Process only first 10KB of data
- **Feature Caching**: Cache normalization parameters
- **Batch Processing**: Process multiple events in batch (future enhancement)

## API Reference

### Function: `dsmil_layer8_detect_anomaly()`

**Signature**:
```c
int dsmil_layer8_detect_anomaly(const void *behavior_data, 
                                 size_t data_size,
                                 dsmil_security_risk_t *risk);
```

**Parameters**:
- `behavior_data`: Pointer to behavior data (bytes)
- `data_size`: Size of behavior data in bytes
- `risk`: Output risk score structure

**Returns**:
- `0`: Success
- `-1`: Error (invalid parameters, memory budget exceeded, etc.)

**Output**:
- `risk->overall_risk`: Anomaly score [0.0, 1.0]
- `risk->threat_probability`: Threat probability [0.0, 1.0]
- `risk->impact_score`: Impact score [0.0, 1.0]
- `risk->threat_type`: `DSMIL_THREAT_ANOMALY`
- `risk->confidence`: Confidence level [0, 100]
- `risk->threat_description`: Human-readable description

### Event Handler: `dsmil_layer8_anomaly_event_handler()`

**Type**: `dsmil_intelligence_callback_t`  
**Purpose**: Process intelligence events and run anomaly detection  
**Registration**: Automatically subscribed during Layer 8 initialization

## Model Contract

### Input Contract

**Format**: Raw bytes (`uint8_t*`)  
**Size**: Variable (up to 10KB processed)  
**Preprocessing**: Feature extraction (statistical + histogram)  
**Normalization**: Z-score normalization per feature

### Output Contract

**Format**: `dsmil_security_risk_t` structure  
**Score Range**: [0.0, 1.0]  
**Interpretation**:
- 0.0-0.1: Normal (no action)
- 0.1-0.4: Low anomaly (monitor)
- 0.4-0.7: Medium anomaly (alert)
- 0.7-1.0: High anomaly (critical)

### Preprocessing Contract

1. **Window Selection**: First 10KB of input data
2. **Feature Extraction**: 
   - Statistical: mean, std_dev, min, max, range, entropy
   - Histogram: 256-bin byte distribution
3. **Normalization**: Z-score normalization using training statistics
4. **Feature Vector**: [mean, std_dev, min, max, range, entropy, histogram[0..255]]

## Implementation Status

### Completed ✅

- [x] Feature schema definition
- [x] Statistical heuristic implementation (fallback)
- [x] Intelligence flow integration
- [x] Event handler callback
- [x] Subscription setup
- [x] Result publishing
- [x] Memory budget guards
- [x] Resource constraint handling

### Pending 🔜

- [ ] INT8 model training pipeline
- [ ] Model loading infrastructure (`dsmil_model_load()`)
- [ ] INT8 inference runtime (`dsmil_model_infer_int8()`)
- [ ] Model calibration dataset collection
- [ ] Production model deployment

## Usage Examples

### Direct API Call

```c
uint8_t behavior_data[1024] = { /* ... */ };
dsmil_security_risk_t risk;

if (dsmil_layer8_detect_anomaly(behavior_data, sizeof(behavior_data), &risk) == 0) {
    if (risk.overall_risk > 0.4f) {
        printf("Anomaly detected: %s (score: %.2f)\n", 
               risk.threat_description, risk.overall_risk);
    }
}
```

### Event-Driven (Automatic)

The model automatically processes events via intelligence flow:

1. Lower layer publishes behavior data event
2. Event handler receives event
3. Anomaly detection runs automatically
4. Results published to Layer 9 if significant

No manual API calls needed for event-driven processing.

## Troubleshooting

### Model Not Loading

**Symptom**: Always uses statistical heuristics  
**Check**: 
- Model file exists: `anomaly_detector_int8.onnx`
- Model loading API implemented
- Memory available for model

### High False Positive Rate

**Symptom**: Too many anomalies detected  
**Solution**: 
- Retrain model with more diverse normal data
- Adjust anomaly threshold (currently 0.1)
- Review baseline statistics

### Memory Budget Exceeded

**Symptom**: Function returns error with "Memory budget exceeded"  
**Solution**:
- Reduce feature window size
- Process data in smaller chunks
- Increase Layer 8 memory budget

## References

- Device 51 Specification: `dsmil/include/dsmil_layer8_security.h`
- Intelligence Flow API: `dsmil/include/dsmil_intelligence_flow.h`
- Runtime Implementation: `dsmil/lib/Runtime/dsmil_layer8_security_runtime.c`
- Security Risk Types: `dsmil/include/dsmil_layer8_security.h` (dsmil_threat_type_t)

---

**Last Updated**: 2025-12-10  
**Maintainer**: DSMIL Kernel Team

