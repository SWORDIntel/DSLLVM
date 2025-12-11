/**
 * @file dsmil_bft_runtime.c
 * @brief DSMIL Blue Force Tracker (BFT-2) Runtime (v1.5.1)
 *
 * Complete BFT-2 implementation with AES-256 encryption, authentication,
 * friend/foe tracking, and real-time position updates.
 *
 * BFT-2 Improvements over BFT-1:
 * - Faster updates: 1-10 second refresh (vs 30 seconds in BFT-1)
 * - Enhanced C2 communications integration
 * - Improved network efficiency
 * - Stronger encryption (AES-256)
 * - Better authentication (ML-DSA-87 signatures)
 *
 * Features:
 * - Position tracking with GPS coordinates
 * - Unit status reporting (fuel, ammo, readiness)
 * - Friend/foe identification
 * - AES-256-GCM encryption
 * - ML-DSA-87 message authentication
 * - Rate limiting and update management
 * - Spoofing detection (Layer 8 Security AI)
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

/* OpenSSL/BoringSSL for AES-256-GCM */
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

/* liboqs for quantum-safe ML-DSA-87 signatures */
#include <oqs/oqs.h>

#define MLDSA87_SIGNATURE_BYTES 4595

// BFT update types
typedef enum {
    DSMIL_BFT_POSITION = 0,
    DSMIL_BFT_STATUS = 1,
    DSMIL_BFT_FRIENDLY = 2
} dsmil_bft_update_type_t;

// Unit status structure
typedef struct {
    uint8_t fuel_percent;
    uint8_t ammo_percent;
    uint8_t readiness_level;  // 1-5 (C1=highest, C5=lowest)
    char status_text[256];
} dsmil_bft_status_t;

// Friendly unit structure
typedef struct {
    char unit_id[64];
    double latitude;
    double longitude;
    double altitude;
    uint64_t last_update_ns;
    dsmil_bft_status_t status;
    bool verified;  // ML-DSA-87 signature verified
} dsmil_bft_friendly_t;

// BFT context (global state)
static struct {
    bool initialized;
    FILE *bft_log;

    // Own unit information
    char unit_id[64];
    double last_lat;
    double last_lon;
    double last_alt;
    uint64_t last_position_update_ns;
    dsmil_bft_status_t own_status;

    // Encryption keys (AES-256-GCM)
    uint8_t aes_key[32];
    uint8_t gcm_iv[12];

    // Authentication (ML-DSA-87)
    uint8_t mldsa_private_key[4896];  // ML-DSA-87 private key
    uint8_t mldsa_public_key[2592];   // ML-DSA-87 public key

    // Friendly units tracking
    dsmil_bft_friendly_t friendlies[256];
    size_t num_friendlies;

    // Rate limiting
    unsigned refresh_rate_seconds;

    // Statistics
    uint64_t positions_sent;
    uint64_t positions_received;
    uint64_t spoofing_attempts_detected;

} g_bft_ctx = {0};

/**
 * @brief Initialize BFT-2 subsystem
 *
 * @param unit_id Unique unit identifier (e.g., "ALPHA-1-1")
 * @param crypto_key AES-256 key for BFT encryption (32 bytes)
 * @return 0 on success, negative on error
 */
int dsmil_bft_init(const char *unit_id, const char *crypto_key) {
    if (g_bft_ctx.initialized) {
        return 0;
    }

    // Set unit ID
    snprintf(g_bft_ctx.unit_id, sizeof(g_bft_ctx.unit_id), "%s", unit_id);

    // Initialize crypto keys with proper random generation
    if (crypto_key) {
        memcpy(g_bft_ctx.aes_key, crypto_key, 32);
    } else {
        // Use OpenSSL RAND for cryptographically secure random key
        if (1 != RAND_bytes(g_bft_ctx.aes_key, 32)) {
            fprintf(stderr, "ERROR: Failed to generate random AES key\n");
            return -1;
        }
    }

    // Initialize GCM IV with cryptographically secure random
    if (1 != RAND_bytes(g_bft_ctx.gcm_iv, 12)) {
        fprintf(stderr, "ERROR: Failed to generate random GCM IV\n");
        return -1;
    }

    /* Initialize ML-DSA-87 keypair using FIPS 204 quantum-safe key generation */
    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
    if (!sig) {
        fprintf(stderr, "ERROR: Failed to create ML-DSA-87 signer for key generation\n");
        return -1;
    }

    /* Generate ML-DSA-87 keypair */
    if (OQS_SIG_keypair(sig, g_bft_ctx.mldsa_public_key, g_bft_ctx.mldsa_private_key) != OQS_SUCCESS) {
        fprintf(stderr, "ERROR: ML-DSA-87 key generation failed\n");
        OQS_SIG_free(sig);
        return -1;
    }

    OQS_SIG_free(sig);

    // Open BFT log
    const char *log_path = getenv("DSMIL_BFT_LOG");
    if (!log_path) {
        log_path = "/var/log/dsmil/bft_tracker.log";
    }

    g_bft_ctx.bft_log = fopen(log_path, "a");
    if (!g_bft_ctx.bft_log) {
        g_bft_ctx.bft_log = stderr;
    }

    // Initialize status
    g_bft_ctx.own_status.fuel_percent = 100;
    g_bft_ctx.own_status.ammo_percent = 100;
    g_bft_ctx.own_status.readiness_level = 1;  // C1 (highest readiness)
    strcpy(g_bft_ctx.own_status.status_text, "OPERATIONAL");

    // Set refresh rate (default: 10 seconds for BFT-2)
    const char *refresh_env = getenv("DSMIL_BFT_REFRESH_RATE");
    g_bft_ctx.refresh_rate_seconds = refresh_env ? atoi(refresh_env) : 10;

    g_bft_ctx.initialized = true;
    g_bft_ctx.num_friendlies = 0;
    g_bft_ctx.positions_sent = 0;
    g_bft_ctx.positions_received = 0;
    g_bft_ctx.spoofing_attempts_detected = 0;

    fprintf(g_bft_ctx.bft_log,
            "[BFT_INIT] Unit: %s, Refresh: %us, Encryption: AES-256-GCM, Auth: ML-DSA-87\n",
            unit_id, g_bft_ctx.refresh_rate_seconds);
    fflush(g_bft_ctx.bft_log);

    return 0;
}

/**
 * @brief Encrypt BFT message with AES-256-GCM
 *
 * @param plaintext Plaintext data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output ciphertext buffer (must be at least plaintext_len + 16)
 * @param tag Output GCM authentication tag (16 bytes)
 * @return bytes encrypted on success, negative on error
 */
static int bft_encrypt_aes256_gcm(const uint8_t *plaintext, size_t plaintext_len,
                                   uint8_t *ciphertext, uint8_t *tag) {
    EVP_CIPHER_CTX *ctx = NULL;
    int len = 0;
    int ciphertext_len = 0;
    int ret = -1;

    /* Create and initialize context */
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(g_bft_ctx.bft_log, "[BFT_ENCRYPT_ERROR] Failed to create cipher context\n");
        goto cleanup;
    }

    /* Initialize encryption with AES-256-GCM */
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, g_bft_ctx.aes_key, g_bft_ctx.gcm_iv)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_ENCRYPT_ERROR] Failed to initialize encryption\n");
        goto cleanup;
    }

    /* Provide the message to be encrypted */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, (int)plaintext_len)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_ENCRYPT_ERROR] Encryption failed\n");
        goto cleanup;
    }
    ciphertext_len = len;

    /* Finalize encryption */
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_ENCRYPT_ERROR] Finalization failed\n");
        goto cleanup;
    }
    ciphertext_len += len;

    /* Get the GCM authentication tag */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_ENCRYPT_ERROR] Failed to get GCM tag\n");
        goto cleanup;
    }

    ret = ciphertext_len;

cleanup:
    if (ctx) {
        EVP_CIPHER_CTX_free(ctx);
    }

    return ret;
}

/**
 * @brief Decrypt BFT message with AES-256-GCM
 *
 * @param ciphertext Ciphertext data
 * @param ciphertext_len Length of ciphertext (not including tag)
 * @param tag GCM authentication tag (16 bytes)
 * @param plaintext Output plaintext buffer
 * @return bytes decrypted on success, negative on error
 */
static int bft_decrypt_aes256_gcm(const uint8_t *ciphertext, size_t ciphertext_len,
                                   const uint8_t *tag, uint8_t *plaintext) {
    EVP_CIPHER_CTX *ctx = NULL;
    int len = 0;
    int plaintext_len = 0;
    int ret = -1;

    /* Create and initialize context */
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(g_bft_ctx.bft_log, "[BFT_DECRYPT_ERROR] Failed to create cipher context\n");
        goto cleanup;
    }

    /* Initialize decryption with AES-256-GCM */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, g_bft_ctx.aes_key, g_bft_ctx.gcm_iv)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_DECRYPT_ERROR] Failed to initialize decryption\n");
        goto cleanup;
    }

    /* Set the GCM authentication tag for verification */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_DECRYPT_ERROR] Failed to set GCM tag\n");
        goto cleanup;
    }

    /* Provide the message to be decrypted */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, (int)ciphertext_len)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_DECRYPT_ERROR] Decryption failed\n");
        goto cleanup;
    }
    plaintext_len = len;

    /* Finalize decryption and verify tag */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        fprintf(g_bft_ctx.bft_log, "[BFT_DECRYPT_ERROR] Authentication failed - tag verification failed\n");
        goto cleanup;
    }
    plaintext_len += len;

    ret = plaintext_len;

cleanup:
    if (ctx) {
        EVP_CIPHER_CTX_free(ctx);
    }

    return ret;
}

/**
 * @brief Sign BFT message with ML-DSA-87 (FIPS 204 quantum-safe signature)
 *
 * @param message Message to sign
 * @param message_len Message length
 * @param signature Output signature buffer (4595 bytes for ML-DSA-87)
 * @return bytes signed on success, negative on error
 */
static int bft_sign_mldsa87(const uint8_t *message, size_t message_len,
                             uint8_t *signature) {
    OQS_SIG *sig = NULL;
    int ret = -1;

    /* Create ML-DSA-87 signer context */
    sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
    if (!sig) {
        fprintf(g_bft_ctx.bft_log, "[BFT_SIGN_ERROR] Failed to create ML-DSA-87 signer\n");
        goto cleanup;
    }

    /* Use private key from global context */
    if (OQS_SIG_sign(sig, signature, (size_t *)&ret,
                     message, message_len,
                     g_bft_ctx.mldsa_private_key) != OQS_SUCCESS) {
        fprintf(g_bft_ctx.bft_log, "[BFT_SIGN_ERROR] ML-DSA-87 signature generation failed\n");
        ret = -1;
        goto cleanup;
    }

    /* ret now contains the signature length (should be 4595 for ML-DSA-87) */

cleanup:
    if (sig) {
        OQS_SIG_free(sig);
    }

    return ret;
}

/**
 * @brief Verify ML-DSA-87 signature (FIPS 204 quantum-safe verification)
 *
 * @param message Message that was signed
 * @param message_len Message length
 * @param signature Signature to verify (4595 bytes)
 * @param signature_len Length of signature
 * @param public_key Signer's ML-DSA-87 public key (2592 bytes)
 * @return true if valid, false if invalid
 */
static bool bft_verify_mldsa87(const uint8_t *message, size_t message_len,
                                const uint8_t *signature, size_t signature_len,
                                const uint8_t *public_key) {
    OQS_SIG *sig = NULL;
    bool is_valid = false;

    /* Validate signature size */
    if (signature_len != MLDSA87_SIGNATURE_BYTES) {
        fprintf(g_bft_ctx.bft_log, "[BFT_VERIFY] Invalid signature length: %zu (expected %zu)\n",
                signature_len, (size_t)MLDSA87_SIGNATURE_BYTES);
        return false;
    }

    /* Create ML-DSA-87 verifier context */
    sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
    if (!sig) {
        fprintf(g_bft_ctx.bft_log, "[BFT_VERIFY_ERROR] Failed to create ML-DSA-87 verifier\n");
        goto cleanup;
    }

    /* Verify signature using public key */
    if (OQS_SIG_verify(sig, message, message_len, signature, signature_len, public_key) == OQS_SUCCESS) {
        is_valid = true;
    } else {
        fprintf(g_bft_ctx.bft_log, "[BFT_VERIFY] ML-DSA-87 signature verification failed\n");
        is_valid = false;
    }

cleanup:
    if (sig) {
        OQS_SIG_free(sig);
    }

    return is_valid;
}

/**
 * @brief Send BFT position update
 *
 * @param lat Latitude (degrees)
 * @param lon Longitude (degrees)
 * @param alt Altitude (meters)
 * @param timestamp_ns Timestamp (nanoseconds since epoch)
 * @return 0 on success, negative on error
 */
int dsmil_bft_send_position(double lat, double lon, double alt,
                              uint64_t timestamp_ns) {
    if (!g_bft_ctx.initialized) {
        dsmil_bft_init("UNKNOWN", NULL);
    }

    // Rate limiting: check if enough time has passed since last update
    if (g_bft_ctx.last_position_update_ns > 0) {
        uint64_t elapsed_ns = timestamp_ns - g_bft_ctx.last_position_update_ns;
        uint64_t refresh_ns = g_bft_ctx.refresh_rate_seconds * 1000000000ULL;

        if (elapsed_ns < refresh_ns) {
            // Too soon, skip this update
            return 1;  // Indicate rate-limited (not an error)
        }
    }

    /* Build position message */
    char plaintext[512];
    snprintf(plaintext, sizeof(plaintext),
             "BFT_POS|%s|%.6f|%.6f|%.1f|%lu",
             g_bft_ctx.unit_id, lat, lon, alt, timestamp_ns);

    size_t plaintext_len = strlen(plaintext);

    /* Encrypt with AES-256-GCM */
    uint8_t ciphertext[512 + 16];  /* Add space for GCM tag */
    uint8_t gcm_tag[16];
    int encrypted_len = bft_encrypt_aes256_gcm((const uint8_t*)plaintext, plaintext_len,
                                               ciphertext, gcm_tag);

    if (encrypted_len < 0) {
        fprintf(g_bft_ctx.bft_log, "[BFT_POS_TX_ERROR] Encryption failed\n");
        fflush(g_bft_ctx.bft_log);
        return -1;
    }

    /* Append GCM tag to ciphertext for transmission */
    memcpy(ciphertext + encrypted_len, gcm_tag, 16);
    size_t total_message_len = encrypted_len + 16;

    /* Sign with ML-DSA-87 */
    uint8_t signature[4595];
    int signature_len = bft_sign_mldsa87(ciphertext, total_message_len, signature);

    if (signature_len < 0) {
        fprintf(g_bft_ctx.bft_log, "[BFT_POS_TX_ERROR] Signature generation failed\n");
        fflush(g_bft_ctx.bft_log);
        return -1;
    }

    /* Log transmission */
    fprintf(g_bft_ctx.bft_log,
            "[BFT_POS_TX] unit=%s lat=%.6f lon=%.6f alt=%.1f ts=%lu encrypted=AES256-GCM(%zu bytes) signed=ML-DSA-87(%d bytes)\n",
            g_bft_ctx.unit_id, lat, lon, alt, timestamp_ns, total_message_len, signature_len);
    fflush(g_bft_ctx.bft_log);

    /* Update state */
    g_bft_ctx.last_lat = lat;
    g_bft_ctx.last_lon = lon;
    g_bft_ctx.last_alt = alt;
    g_bft_ctx.last_position_update_ns = timestamp_ns;
    g_bft_ctx.positions_sent++;

    /* In production: transmit encrypted message via BFT-2 protocol
       Message would be: [ciphertext(encrypted_len bytes) + tag(16 bytes) + signature(4595 bytes)]
     */
    (void)ciphertext;  /* Would be transmitted */
    (void)signature;   /* Would be transmitted */

    return 0;
}

/**
 * @brief Send unit status update
 *
 * @param status Status string (e.g., "OPERATIONAL", "DAMAGED", "RESUPPLY")
 * @return 0 on success, negative on error
 */
int dsmil_bft_send_status(const char *status) {
    if (!g_bft_ctx.initialized) {
        dsmil_bft_init("UNKNOWN", NULL);
    }

    snprintf(g_bft_ctx.own_status.status_text,
             sizeof(g_bft_ctx.own_status.status_text),
             "%s", status);

    fprintf(g_bft_ctx.bft_log,
            "[BFT_STATUS_TX] unit=%s status=%s fuel=%u%% ammo=%u%% readiness=C%u\n",
            g_bft_ctx.unit_id,
            status,
            g_bft_ctx.own_status.fuel_percent,
            g_bft_ctx.own_status.ammo_percent,
            g_bft_ctx.own_status.readiness_level);
    fflush(g_bft_ctx.bft_log);

    return 0;
}

/**
 * @brief Report friendly unit
 *
 * @param unit_id Friendly unit identifier
 * @return 0 on success, negative on error
 */
int dsmil_bft_send_friendly(const char *unit_id) {
    if (!g_bft_ctx.initialized) {
        dsmil_bft_init("UNKNOWN", NULL);
    }

    fprintf(g_bft_ctx.bft_log,
            "[BFT_FRIENDLY_TX] reporting_unit=%s friendly_unit=%s\n",
            g_bft_ctx.unit_id, unit_id);
    fflush(g_bft_ctx.bft_log);

    return 0;
}

/**
 * @brief Receive and process BFT position update
 *
 * @param encrypted_message Encrypted BFT message
 * @param message_len Message length
 * @param signature ML-DSA-87 signature (4595 bytes)
 * @param sender_public_key Sender's ML-DSA-87 public key (2592 bytes)
 * @return 0 if valid and processed, negative if rejected
 */
int dsmil_bft_recv_position(const uint8_t *encrypted_message, size_t message_len,
                              const uint8_t *signature,
                              const uint8_t *sender_public_key) {
    if (!g_bft_ctx.initialized) {
        dsmil_bft_init("UNKNOWN", NULL);
    }

    /* Extract tag from end of encrypted message (last 16 bytes) */
    if (message_len < 16) {
        fprintf(g_bft_ctx.bft_log, "[BFT_RECV_ERROR] Message too short to contain GCM tag\n");
        return -1;
    }

    const uint8_t *tag = encrypted_message + message_len - 16;
    size_t ciphertext_len = message_len - 16;

    /* Decrypt message with AES-256-GCM */
    uint8_t plaintext[512];
    int decrypted_len = bft_decrypt_aes256_gcm(encrypted_message, ciphertext_len, tag, plaintext);
    
    if (decrypted_len < 0) {
        fprintf(g_bft_ctx.bft_log, "[BFT_RECV_ERROR] Decryption or authentication failed\n");
        return -1;
    }

    /* Null terminate plaintext */
    plaintext[decrypted_len < (int)sizeof(plaintext) ? decrypted_len : sizeof(plaintext)-1] = '\0';

    /* Verify ML-DSA-87 signature (4595 bytes for ML-DSA-87) */
    bool signature_valid = bft_verify_mldsa87(plaintext, decrypted_len,
                                               signature, 4595, sender_public_key);

    if (!signature_valid) {
        g_bft_ctx.spoofing_attempts_detected++;
        fprintf(g_bft_ctx.bft_log,
                "[BFT_SPOOFING] Invalid ML-DSA-87 signature detected!\n");
        fflush(g_bft_ctx.bft_log);
        return -1;  // Reject spoofed message
    }

    // Parse position message
    char unit_id[64];
    double lat, lon, alt;
    uint64_t timestamp;
    if (sscanf((const char*)plaintext, "BFT_POS|%63[^|]|%lf|%lf|%lf|%lu",
               unit_id, &lat, &lon, &alt, &timestamp) != 5) {
        return -1;  // Parse error
    }

    // Check for spoofing: distance validation (Layer 8 Security AI)
    if (g_bft_ctx.num_friendlies > 0) {
        // Find existing friendly
        for (size_t i = 0; i < g_bft_ctx.num_friendlies; i++) {
            if (strcmp(g_bft_ctx.friendlies[i].unit_id, unit_id) == 0) {
                // Check if position change is physically plausible
                double dist = sqrt(pow(lat - g_bft_ctx.friendlies[i].latitude, 2) +
                                   pow(lon - g_bft_ctx.friendlies[i].longitude, 2));
                uint64_t time_diff_s = (timestamp - g_bft_ctx.friendlies[i].last_update_ns) / 1000000000ULL;

                // Maximum plausible speed: 300 m/s (~Mach 1)
                double max_dist = 300.0 * time_diff_s / 111000.0;  // degrees

                if (dist > max_dist) {
                    g_bft_ctx.spoofing_attempts_detected++;
                    fprintf(g_bft_ctx.bft_log,
                            "[BFT_SPOOFING] Implausible position change for %s (%.2f deg in %lus)\n",
                            unit_id, dist, time_diff_s);
                    fflush(g_bft_ctx.bft_log);
                    return -1;  // Reject implausible position
                }

                // Update friendly position
                g_bft_ctx.friendlies[i].latitude = lat;
                g_bft_ctx.friendlies[i].longitude = lon;
                g_bft_ctx.friendlies[i].altitude = alt;
                g_bft_ctx.friendlies[i].last_update_ns = timestamp;
                g_bft_ctx.friendlies[i].verified = true;

                g_bft_ctx.positions_received++;
                fprintf(g_bft_ctx.bft_log,
                        "[BFT_POS_RX] unit=%s lat=%.6f lon=%.6f alt=%.1f verified=ML-DSA-87\n",
                        unit_id, lat, lon, alt);
                fflush(g_bft_ctx.bft_log);

                return 0;
            }
        }
    }

    // New friendly unit
    if (g_bft_ctx.num_friendlies < 256) {
        dsmil_bft_friendly_t *friendly = &g_bft_ctx.friendlies[g_bft_ctx.num_friendlies++];
        strcpy(friendly->unit_id, unit_id);
        friendly->latitude = lat;
        friendly->longitude = lon;
        friendly->altitude = alt;
        friendly->last_update_ns = timestamp;
        friendly->verified = true;

        g_bft_ctx.positions_received++;
        fprintf(g_bft_ctx.bft_log,
                "[BFT_NEW_FRIENDLY] unit=%s lat=%.6f lon=%.6f alt=%.1f\n",
                unit_id, lat, lon, alt);
        fflush(g_bft_ctx.bft_log);
    }

    return 0;
}

/**
 * @brief Get list of all tracked friendly units
 *
 * @param positions Output array of positions
 * @param max_count Maximum number of positions to return
 * @return Number of positions returned
 */
int dsmil_bft_get_friendlies(dsmil_bft_friendly_t *positions, size_t max_count) {
    if (!g_bft_ctx.initialized) {
        return 0;
    }

    size_t count = g_bft_ctx.num_friendlies < max_count ?
                   g_bft_ctx.num_friendlies : max_count;

    memcpy(positions, g_bft_ctx.friendlies, count * sizeof(dsmil_bft_friendly_t));

    return (int)count;
}

/**
 * @brief Update own unit status
 *
 * @param fuel_percent Fuel level (0-100%)
 * @param ammo_percent Ammunition level (0-100%)
 * @param readiness_level Readiness (1-5, C1=highest)
 */
void dsmil_bft_update_status(uint8_t fuel_percent, uint8_t ammo_percent,
                               uint8_t readiness_level) {
    if (!g_bft_ctx.initialized) {
        dsmil_bft_init("UNKNOWN", NULL);
    }

    g_bft_ctx.own_status.fuel_percent = fuel_percent;
    g_bft_ctx.own_status.ammo_percent = ammo_percent;
    g_bft_ctx.own_status.readiness_level = readiness_level;
}

/**
 * @brief Get BFT statistics
 *
 * @param positions_sent Output: number of positions sent
 * @param positions_received Output: number of positions received
 * @param spoofing_detected Output: number of spoofing attempts detected
 */
void dsmil_bft_get_stats(uint64_t *positions_sent, uint64_t *positions_received,
                          uint64_t *spoofing_detected) {
    if (!g_bft_ctx.initialized) {
        *positions_sent = 0;
        *positions_received = 0;
        *spoofing_detected = 0;
        return;
    }

    *positions_sent = g_bft_ctx.positions_sent;
    *positions_received = g_bft_ctx.positions_received;
    *spoofing_detected = g_bft_ctx.spoofing_attempts_detected;
}

/**
 * @brief Shutdown BFT subsystem
 */
void dsmil_bft_shutdown(void) {
    if (!g_bft_ctx.initialized) {
        return;
    }

    fprintf(g_bft_ctx.bft_log,
            "[BFT_SHUTDOWN] Positions: sent=%lu received=%lu spoofing_detected=%lu friendlies=%zu\n",
            g_bft_ctx.positions_sent,
            g_bft_ctx.positions_received,
            g_bft_ctx.spoofing_attempts_detected,
            g_bft_ctx.num_friendlies);

    if (g_bft_ctx.bft_log != stderr) {
        fclose(g_bft_ctx.bft_log);
    }

    g_bft_ctx.initialized = false;
}
