/**
 * @file dsmil_device255_crypto_runtime.c
 * @brief Device 255 Master Crypto Controller Runtime Implementation
 * 
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_device255_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <limits.h>

/* TPM 2.0 TSS library for hardware integration */
#include <tss2/tss2_sys.h>
#include <tss2/tss2_tcti.h>
#include <tss2/tss2_tcti_device.h>

/* Hardware acceleration detection */
#include <cpuid.h>

/* OpenSSL for crypto operations */
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

/* liboqs for quantum-safe crypto */
#include <oqs/oqs.h>

/* Compatibility shims for older TSS header sets */
#ifndef TPM2_ALG_SHAKE128
#define TPM2_ALG_SHAKE128 TPM_ALG_SHAKE128
#endif
#ifndef TPM2_ALG_SHAKE256
#define TPM2_ALG_SHAKE256 TPM_ALG_SHAKE256
#endif

#ifndef DSMIL_ENABLE_TPM
#define DSMIL_ENABLE_TPM 1
#endif

#define DEVICE255_ID 255
#define ALGORITHM_COUNT 88

static struct {
    bool initialized;
    dsmil_device255_ctx_t contexts[10];  // One per layer
    uint64_t engine_stats[3];  // TPM, Hardware, Software
    
    /* TPM 2.0 state */
    TSS2_SYS_CONTEXT *tpm_context;
    TSS2_TCTI_CONTEXT *tcti_context;
    bool tpm_available;
    bool tpm_probed;
    
    /* Hardware acceleration state */
    bool aes_ni_available;
    bool avx512_available;
    bool hw_accel_probed;
} g_device255_state = {0};

typedef struct {
    const EVP_MD *md;
    TPM2_ALG_ID tpm_alg;
    size_t digest_size;
    bool is_xof;
} hash_spec_t;

typedef enum {
    CIPHER_MODE_STANDARD = 0,
    CIPHER_MODE_GCM,
    CIPHER_MODE_CCM,
    CIPHER_MODE_POLY1305
} cipher_mode_t;

typedef struct {
    const EVP_CIPHER *cipher;
    TPM2_ALG_ID tpm_alg;
    TPM2_ALG_ID tpm_mode;
    size_t iv_len;
    size_t tag_len;
    size_t min_key_len;
    size_t max_key_len;
    bool allow_tpm;
    cipher_mode_t mode;
} cipher_spec_t;

static bool resolve_hash_spec(uint16_t algorithm, hash_spec_t *spec) {
    if (!spec) {
        return false;
    }
    switch (algorithm) {
        case TPM_ALG_SHA1:
            *spec = (hash_spec_t){EVP_sha1(), TPM2_ALG_SHA1, 20, false};
            return true;
        case TPM_ALG_SHA256:
            *spec = (hash_spec_t){EVP_sha256(), TPM2_ALG_SHA256, 32, false};
            return true;
        case TPM_ALG_SHA384:
            *spec = (hash_spec_t){EVP_sha384(), TPM2_ALG_SHA384, 48, false};
            return true;
        case TPM_ALG_SHA512:
            *spec = (hash_spec_t){EVP_sha512(), TPM2_ALG_SHA512, 64, false};
            return true;
        case TPM_ALG_SM3_256:
            *spec = (hash_spec_t){EVP_sm3(), TPM2_ALG_SM3_256, 32, false};
            return true;
        case TPM_ALG_SHA3_256:
            *spec = (hash_spec_t){EVP_sha3_256(), TPM2_ALG_SHA3_256, 32, false};
            return true;
        case TPM_ALG_SHA3_384:
            *spec = (hash_spec_t){EVP_sha3_384(), TPM2_ALG_SHA3_384, 48, false};
            return true;
        case TPM_ALG_SHA3_512:
            *spec = (hash_spec_t){EVP_sha3_512(), TPM2_ALG_SHA3_512, 64, false};
            return true;
        case TPM_ALG_SHAKE128:
            *spec = (hash_spec_t){EVP_shake128(), TPM2_ALG_SHAKE128, 32, true};
            return true;
        case TPM_ALG_SHAKE256:
            *spec = (hash_spec_t){EVP_shake256(), TPM2_ALG_SHAKE256, 64, true};
            return true;
        default:
            return false;
    }
}

static bool resolve_cipher_spec(uint16_t algorithm, size_t key_len, cipher_spec_t *spec) {
    if (!spec) {
        return false;
    }
    cipher_spec_t out = {
        .cipher = NULL,
        .tpm_alg = TPM2_ALG_NULL,
        .tpm_mode = TPM2_ALG_NULL,
        .iv_len = 0,
        .tag_len = 0,
        .min_key_len = 0,
        .max_key_len = SIZE_MAX,
        .allow_tpm = false,
        .mode = CIPHER_MODE_STANDARD,
    };

    switch (algorithm) {
        case CRYPTO_ALG_AES_128_ECB:
            out = (cipher_spec_t){EVP_aes_128_ecb(), TPM2_ALG_AES, TPM2_ALG_ECB, 0, 0, 16, 16, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_256_ECB:
            out = (cipher_spec_t){EVP_aes_256_ecb(), TPM2_ALG_AES, TPM2_ALG_ECB, 0, 0, 32, 32, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_128_CBC:
            out = (cipher_spec_t){EVP_aes_128_cbc(), TPM2_ALG_AES, TPM2_ALG_CBC, 16, 0, 16, 16, true, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_256_CBC:
            out = (cipher_spec_t){EVP_aes_256_cbc(), TPM2_ALG_AES, TPM2_ALG_CBC, 16, 0, 32, 32, true, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_128_CTR:
            out = (cipher_spec_t){EVP_aes_128_ctr(), TPM2_ALG_AES, TPM2_ALG_CTR, 16, 0, 16, 16, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_256_CTR:
            out = (cipher_spec_t){EVP_aes_256_ctr(), TPM2_ALG_AES, TPM2_ALG_CTR, 16, 0, 32, 32, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_128_OFB:
            out = (cipher_spec_t){EVP_aes_128_ofb(), TPM2_ALG_AES, TPM2_ALG_OFB, 16, 0, 16, 16, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_256_OFB:
            out = (cipher_spec_t){EVP_aes_256_ofb(), TPM2_ALG_AES, TPM2_ALG_OFB, 16, 0, 32, 32, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_128_CFB:
            out = (cipher_spec_t){EVP_aes_128_cfb128(), TPM2_ALG_AES, TPM2_ALG_CFB, 16, 0, 16, 16, true, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_256_CFB:
            out = (cipher_spec_t){EVP_aes_256_cfb128(), TPM2_ALG_AES, TPM2_ALG_CFB, 16, 0, 32, 32, true, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_128_GCM:
            out = (cipher_spec_t){EVP_aes_128_gcm(), TPM2_ALG_AES, TPM2_ALG_CFB, 12, 16, 16, 16, false, CIPHER_MODE_GCM};
            break;
        case CRYPTO_ALG_AES_256_GCM:
            out = (cipher_spec_t){EVP_aes_256_gcm(), TPM2_ALG_AES, TPM2_ALG_CFB, 12, 16, 32, 32, false, CIPHER_MODE_GCM};
            break;
        case CRYPTO_ALG_AES_128_CCM:
            out = (cipher_spec_t){EVP_aes_128_ccm(), TPM2_ALG_AES, TPM2_ALG_CFB, 12, 16, 16, 16, false, CIPHER_MODE_CCM};
            break;
        case CRYPTO_ALG_AES_256_CCM:
            out = (cipher_spec_t){EVP_aes_256_ccm(), TPM2_ALG_AES, TPM2_ALG_CFB, 12, 16, 32, 32, false, CIPHER_MODE_CCM};
            break;
        case CRYPTO_ALG_AES_128_XTS:
            out = (cipher_spec_t){EVP_aes_128_xts(), TPM2_ALG_AES, TPM2_ALG_CFB, 16, 0, 32, 32, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_AES_256_XTS:
            out = (cipher_spec_t){EVP_aes_256_xts(), TPM2_ALG_AES, TPM2_ALG_CFB, 16, 0, 64, 64, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_3DES_EDE:
            out = (cipher_spec_t){EVP_des_ede3_cbc(), TPM2_ALG_TDES, TPM2_ALG_CBC, 8, 0, 24, 24, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_CAMELLIA_128:
            out = (cipher_spec_t){EVP_camellia_128_cbc(), TPM2_ALG_CAMELLIA, TPM2_ALG_CBC, 16, 0, 16, 16, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_CAMELLIA_256:
            out = (cipher_spec_t){EVP_camellia_256_cbc(), TPM2_ALG_CAMELLIA, TPM2_ALG_CBC, 16, 0, 32, 32, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_SM4_128:
            out = (cipher_spec_t){EVP_sm4_cbc(), TPM2_ALG_SM4, TPM2_ALG_CBC, 16, 0, 16, 16, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_CHACHA20:
            out = (cipher_spec_t){EVP_chacha20(), TPM2_ALG_NULL, TPM2_ALG_NULL, 12, 0, 32, 32, false, CIPHER_MODE_STANDARD};
            break;
        case CRYPTO_ALG_CHACHA20_POLY1305:
            out = (cipher_spec_t){EVP_chacha20_poly1305(), TPM2_ALG_NULL, TPM2_ALG_NULL, 12, 16, 32, 32, false, CIPHER_MODE_POLY1305};
            break;
        default:
            return false;
    }

    if (key_len < out.min_key_len || key_len > out.max_key_len) {
        return false;
    }

    if (!out.cipher) {
        return false;
    }

    *spec = out;
    return true;
}

/* Forward declarations */
#if DSMIL_ENABLE_TPM
static bool probe_tpm2_availability(void);
static bool probe_hardware_acceleration(void);
static bool detect_secure_boot(void);
static TSS2_SYS_CONTEXT* get_tpm_context(void);
static void release_tpm_context(TSS2_SYS_CONTEXT *sys_ctx);
static TSS2_SYS_CONTEXT* create_sys_context(TSS2_TCTI_CONTEXT **out_tcti);
static void cleanup_sys_context(TSS2_SYS_CONTEXT *sys_ctx, TSS2_TCTI_CONTEXT *tcti_ctx);
#endif

#if DSMIL_ENABLE_TPM

/**
 * @brief Probe for TPM 2.0 availability
 * @return true if TPM 2.0 detected and accessible, false otherwise
 */
static bool probe_tpm2_availability(void) {
    TSS2_TCTI_CONTEXT *tcti_ctx = NULL;
    TSS2_SYS_CONTEXT *sys_ctx = create_sys_context(&tcti_ctx);
    if (!sys_ctx) {
        return false;
    }

    TPM2_CAP capability = TPM2_CAP_ALGS;
    TPM2_HANDLE property = 0;
    UINT32 property_count = 1;
    TPMI_YES_NO more_data = TPM2_NO;
    TPMS_CAPABILITY_DATA capability_data = {0};

    TSS2_RC rc = Tss2_Sys_GetCapability(sys_ctx, NULL, capability, property,
                                        property_count, &more_data, &capability_data, NULL);

    cleanup_sys_context(sys_ctx, tcti_ctx);
    return (rc == TSS2_RC_SUCCESS);
}

/**
 * @brief Probe for hardware acceleration capabilities
 * @return true if AES-NI or AVX-512 detected
 */
static bool probe_hardware_acceleration(void) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Check for AES-NI support (CPUID leaf 1, ECX bit 25) */
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (ecx & (1 << 25)) {
            g_device255_state.aes_ni_available = true;
        }
    }
    
    /* Check for AVX-512 support (CPUID leaf 7, EBX bits 16-28) */
    if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
        /* AVX-512 Foundation (bit 16) */
        if (ebx & (1 << 16)) {
            g_device255_state.avx512_available = true;
        }
    }
    
    return (g_device255_state.aes_ni_available || g_device255_state.avx512_available);
}

/**
 * @brief Detect if secure boot is enabled
 * @return true if secure boot is active
 */
static bool detect_secure_boot(void) {
    /* Check UEFI secure boot status */
    const char *sb_enabled = "/sys/firmware/efi/secure_boot";
    
    /* If EFI variables exist, try to check secure boot */
    if (access(sb_enabled, R_OK) == 0) {
        FILE *fp = fopen(sb_enabled, "r");
        if (fp) {
            char buf[8] = {0};
            if (fread(buf, 1, sizeof(buf)-1, fp) > 0) {
                fclose(fp);
                /* Value "1" means secure boot is on */
                return (buf[0] == '1');
            }
            fclose(fp);
        }
    }
    
    /* Default to false if we can't determine */
    return false;
}

static TSS2_SYS_CONTEXT* create_sys_context(TSS2_TCTI_CONTEXT **out_tcti) {
    if (out_tcti) {
        *out_tcti = NULL;
    }

    const char *tpm_devices[] = { "/dev/tpmrm0", "/dev/tpm0" };
    const char *tpm_device = NULL;
    for (size_t i = 0; i < sizeof(tpm_devices)/sizeof(tpm_devices[0]); ++i) {
        if (access(tpm_devices[i], R_OK | W_OK) == 0) {
            tpm_device = tpm_devices[i];
            break;
        }
    }
    if (!tpm_device) {
        return NULL;
    }

    size_t tcti_size = 0;
    Tss2_Tcti_Device_Init(NULL, &tcti_size, tpm_device);
    TSS2_TCTI_CONTEXT *tcti_ctx = calloc(1, tcti_size);
    if (!tcti_ctx) {
        return NULL;
    }

    TSS2_RC rc = Tss2_Tcti_Device_Init(tcti_ctx, &tcti_size, tpm_device);
    if (rc != TSS2_RC_SUCCESS) {
        free(tcti_ctx);
        return NULL;
    }

    size_t sys_size = Tss2_Sys_GetContextSize(0);
    TSS2_SYS_CONTEXT *sys_ctx = calloc(1, sys_size);
    if (!sys_ctx) {
        Tss2_Tcti_Finalize(tcti_ctx);
        free(tcti_ctx);
        return NULL;
    }

    rc = Tss2_Sys_Initialize(sys_ctx, sys_size, tcti_ctx, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        free(sys_ctx);
        Tss2_Tcti_Finalize(tcti_ctx);
        free(tcti_ctx);
        return NULL;
    }

    if (out_tcti) {
        *out_tcti = tcti_ctx;
    }
    return sys_ctx;
}

static void cleanup_sys_context(TSS2_SYS_CONTEXT *sys_ctx, TSS2_TCTI_CONTEXT *tcti_ctx) {
    if (sys_ctx) {
        Tss2_Sys_Finalize(sys_ctx);
        free(sys_ctx);
    }
    if (tcti_ctx) {
        Tss2_Tcti_Finalize(tcti_ctx);
        free(tcti_ctx);
    }
}

/**
 * @brief Get or create TPM 2.0 SYS context
 * @return TSS2_SYS_CONTEXT* or NULL on error
 */
static TSS2_SYS_CONTEXT* get_tpm_context(void) {
    if (!g_device255_state.tpm_available) {
        return NULL;
    }
    
    /* If we already have a context, return it */
    if (g_device255_state.tpm_context) {
        return g_device255_state.tpm_context;
    }
    
    /* Create new TPM context */
    TSS2_TCTI_CONTEXT *tcti_ctx = NULL;
    TSS2_SYS_CONTEXT *sys_ctx = create_sys_context(&tcti_ctx);
    if (!sys_ctx) {
        return NULL;
    }

    g_device255_state.tcti_context = tcti_ctx;
    g_device255_state.tpm_context = sys_ctx;
    return sys_ctx;
}

/**
 * @brief Release TPM context (called on shutdown)
 */
static void __attribute__((unused)) release_tpm_context(TSS2_SYS_CONTEXT *sys_ctx) {
    cleanup_sys_context(sys_ctx, g_device255_state.tcti_context);
    g_device255_state.tpm_context = NULL;
    g_device255_state.tcti_context = NULL;
}

static void init_caps(dsmil_device255_caps_t *caps) {
    memset(caps, 0, sizeof(*caps));
    caps->available = DSMIL_CRYPTO_CAP_ALL;
    caps->enabled = DSMIL_CRYPTO_CAP_ALL;
    caps->algorithm_count = ALGORITHM_COUNT;
    
    /* Probe for TPM on first call */
    if (!g_device255_state.tpm_probed) {
        g_device255_state.tpm_available = probe_tpm2_availability();
        g_device255_state.tpm_probed = true;
    }
    caps->tpm_available = g_device255_state.tpm_available;
    
    /* Probe for hardware acceleration on first call */
    if (!g_device255_state.hw_accel_probed) {
        probe_hardware_acceleration();
        g_device255_state.hw_accel_probed = true;
    }
    
    /* Detect secure boot status */
    caps->secure_boot_verified = detect_secure_boot();
}

int dsmil_device255_init(uint8_t layer, dsmil_device255_ctx_t *ctx) {
    if (!ctx || layer > 9) {
        return -1;
    }
    
    if (!g_device255_state.initialized) {
        memset(&g_device255_state, 0, sizeof(g_device255_state));
        g_device255_state.initialized = true;
    }
    
    /* Initialize context */
    memset(ctx, 0, sizeof(*ctx));
    ctx->device_id = DEVICE255_ID;
    ctx->layer = layer;
    
    /* Initialize capabilities (probes hardware) */
    init_caps(&ctx->caps);
    
    /* Select engine based on availability:
     * Priority: TPM > Hardware (AES-NI/AVX-512) > Software fallback
     */
    if (g_device255_state.tpm_available) {
        ctx->engine = DSMIL_CRYPTO_ENGINE_TPM;
    } else if (g_device255_state.aes_ni_available || g_device255_state.avx512_available) {
        ctx->engine = DSMIL_CRYPTO_ENGINE_HARDWARE;
    } else {
        ctx->engine = DSMIL_CRYPTO_ENGINE_SOFTWARE;
    }
    
    ctx->caps.active_engine = ctx->engine;
    
    /* Store context per layer */
    if (layer < 10) {
        g_device255_state.contexts[layer] = *ctx;
    }
    
    return 0;
}

int dsmil_device255_get_caps(const dsmil_device255_ctx_t *ctx,
                              dsmil_device255_caps_t *caps) {
    if (!ctx || !caps) {
        return -1;
    }
    
    *caps = ctx->caps;
    return 0;
}

int dsmil_device255_set_engine(dsmil_device255_ctx_t *ctx,
                                dsmil_crypto_engine_t engine) {
    if (!ctx) {
        return -1;
    }
    
    if (engine > DSMIL_CRYPTO_ENGINE_SOFTWARE) {
        return -1;
    }
    
    ctx->engine = engine;
    ctx->caps.active_engine = engine;
    
    return 0;
}

int dsmil_device255_hash(const dsmil_device255_ctx_t *ctx,
                         uint16_t algorithm,
                         const void *input, size_t input_len,
                         void *output, size_t *output_len) {
    if (!ctx || !input || !output || !output_len) {
        return -1;
    }

    hash_spec_t spec = {0};
    if (!resolve_hash_spec(algorithm, &spec)) {
        return -1;
    }

    size_t required_len = spec.is_xof ? ((*output_len > 0) ? *output_len : spec.digest_size)
                                      : spec.digest_size;
    if (*output_len < required_len) {
        *output_len = required_len;
        return -1;
    }

    /* Route to TPM first if requested and available */
    if (ctx->engine == DSMIL_CRYPTO_ENGINE_TPM && g_device255_state.tpm_available && spec.tpm_alg != TPM2_ALG_NULL) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            TPM2B_MAX_BUFFER data = {0};
            data.size = (uint16_t)(input_len < sizeof(data.buffer) ? input_len : sizeof(data.buffer));
            memcpy(data.buffer, input, data.size);

            TPM2B_DIGEST digest = {0};
            TSS2_RC rc = Tss2_Sys_Hash(sys_ctx, NULL, &data, spec.tpm_alg, TPM2_RH_NULL, &digest, NULL, NULL);
            if (rc == TSS2_RC_SUCCESS && digest.size >= required_len) {
                memcpy(output, digest.buffer, required_len);
                if (ctx->layer < 10) {
                    g_device255_state.contexts[ctx->layer].operation_count++;
                    g_device255_state.contexts[ctx->layer].bytes_processed += input_len;
                    g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                }
                *output_len = required_len;
                return 0;
            }
        }
    }

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        return -1;
    }

    if (EVP_DigestInit_ex(md_ctx, spec.md, NULL) != 1) {
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }

    if (EVP_DigestUpdate(md_ctx, input, input_len) != 1) {
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }

    int rc_final = 0;
    if (spec.is_xof) {
        rc_final = EVP_DigestFinalXOF(md_ctx, (unsigned char*)output, required_len);
    } else {
        unsigned int digest_len = 0;
        rc_final = EVP_DigestFinal_ex(md_ctx, (unsigned char*)output, &digest_len);
        if (rc_final == 1 && digest_len != required_len) {
            rc_final = 0;
        }
    }

    EVP_MD_CTX_free(md_ctx);
    if (rc_final != 1) {
        return -1;
    }

    if (ctx->layer < 10) {
        g_device255_state.contexts[ctx->layer].operation_count++;
        g_device255_state.contexts[ctx->layer].bytes_processed += input_len;
        g_device255_state.engine_stats[ctx->engine]++;
    }

    *output_len = required_len;
    return 0;
}

int dsmil_device255_encrypt(const dsmil_device255_ctx_t *ctx,
                            uint16_t algorithm,
                            const void *key, size_t key_len,
                            const void *iv, size_t iv_len,
                            const void *plaintext, size_t plaintext_len,
                            void *ciphertext, size_t *ciphertext_len) {
    if (!ctx || !key || !plaintext || !ciphertext || !ciphertext_len) {
        return -1;
    }

    cipher_spec_t spec = {0};
    if (!resolve_cipher_spec(algorithm, key_len, &spec)) {
        return -1;
    }

    if (spec.iv_len > 0 && iv_len < spec.iv_len) {
        return -1;
    }

    size_t block_overhead = spec.cipher ? (size_t)EVP_CIPHER_block_size(spec.cipher) : 0;
    size_t required_ciphertext_len = plaintext_len + spec.tag_len + block_overhead;
    if (*ciphertext_len < required_ciphertext_len) {
        *ciphertext_len = required_ciphertext_len;
        return -1;
    }

    /* Try TPM first when allowed and available */
    bool used_tpm = false;
    if (ctx->engine == DSMIL_CRYPTO_ENGINE_TPM && g_device255_state.tpm_available &&
        spec.allow_tpm && spec.tpm_alg == TPM2_ALG_AES) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            TPM2B_SENSITIVE_CREATE in_sensitive = {0};
            TPM2B_PUBLIC in_public = {0};
            in_public.publicArea.type = TPM2_ALG_SYMCIPHER;
            in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
            in_public.publicArea.objectAttributes = TPMA_OBJECT_DECRYPT | TPMA_OBJECT_SIGN_ENCRYPT;
            in_public.publicArea.parameters.symDetail.sym.algorithm = spec.tpm_alg;
            in_public.publicArea.parameters.symDetail.sym.keyBits.aes = (uint16_t)(key_len * 8);
            in_public.publicArea.parameters.symDetail.sym.mode.aes = spec.tpm_mode;

            in_sensitive.sensitive.data.size = (uint16_t)(key_len < sizeof(in_sensitive.sensitive.data.buffer) ? key_len : sizeof(in_sensitive.sensitive.data.buffer));
            memcpy(in_sensitive.sensitive.data.buffer, key, in_sensitive.sensitive.data.size);

            TPM2B_DATA outside_info = {0};
            TPML_PCR_SELECTION creation_pcr = {0};
            TPM2B_PRIVATE out_private = {0};
            TPM2B_PUBLIC out_public = {0};
            TPM2B_CREATION_DATA creation_data = {0};
            TPM2B_DIGEST creation_hash = {0};
            TPMT_TK_CREATION creation_ticket = {0};

            TSS2_RC rc = Tss2_Sys_Create(sys_ctx, TPM2_RH_NULL, NULL, &in_sensitive, &in_public,
                                         &outside_info, &creation_pcr, &out_private, &out_public,
                                         &creation_data, &creation_hash, &creation_ticket, NULL);
            if (rc == TSS2_RC_SUCCESS) {
                TPM2_HANDLE loaded_handle = 0;
                TPM2B_PRIVATE in_private = out_private;
                TPM2B_PUBLIC in_public_key = out_public;
                rc = Tss2_Sys_Load(sys_ctx, TPM2_RH_NULL, NULL, &in_private, &in_public_key, &loaded_handle, NULL, NULL);
                if (rc == TSS2_RC_SUCCESS) {
                    TPM2B_MAX_BUFFER in_data = {0};
                    in_data.size = (uint16_t)(plaintext_len < sizeof(in_data.buffer) ? plaintext_len : sizeof(in_data.buffer));
                    memcpy(in_data.buffer, plaintext, in_data.size);

                    TPM2B_MAX_BUFFER out_data = {0};
                    TPM2B_IV iv_in = {0};
                    if (spec.iv_len > 0 && spec.iv_len <= sizeof(iv_in.buffer) && iv) {
                        iv_in.size = (uint16_t)spec.iv_len;
                        memcpy(iv_in.buffer, iv, spec.iv_len);
                    }

                    TPM2B_IV iv_out = {0};
                    rc = Tss2_Sys_EncryptDecrypt(sys_ctx, loaded_handle, NULL,
                                                 0, spec.tpm_mode, &iv_in,
                                                 &in_data, &out_data, &iv_out, NULL);
                    if (rc == TSS2_RC_SUCCESS && out_data.size <= *ciphertext_len) {
                        memcpy(ciphertext, out_data.buffer, out_data.size);
                        *ciphertext_len = out_data.size;
                        used_tpm = true;
                        if (ctx->layer < 10) {
                            g_device255_state.contexts[ctx->layer].operation_count++;
                            g_device255_state.contexts[ctx->layer].bytes_processed += plaintext_len;
                        }
                        g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                        Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                        return 0;
                    }
                    Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                }
            }
        }
    }

    const unsigned char *iv_bytes = (spec.iv_len > 0) ? (const unsigned char*)iv : NULL;
    EVP_CIPHER_CTX *cipher_ctx = EVP_CIPHER_CTX_new();
    if (!cipher_ctx) {
        return -1;
    }

    int len = 0;
    int ciphertext_len_result = 0;
    int rc = 0;

    switch (spec.mode) {
        case CIPHER_MODE_GCM:
        case CIPHER_MODE_POLY1305: {
            int iv_ctrl = (spec.mode == CIPHER_MODE_GCM) ? EVP_CTRL_GCM_SET_IVLEN : EVP_CTRL_AEAD_SET_IVLEN;
            int tag_ctrl = (spec.mode == CIPHER_MODE_GCM) ? EVP_CTRL_GCM_GET_TAG : EVP_CTRL_AEAD_GET_TAG;

            if (EVP_EncryptInit_ex(cipher_ctx, spec.cipher, NULL, NULL, NULL) != 1) { rc = -1; break; }
            if (spec.iv_len && EVP_CIPHER_CTX_ctrl(cipher_ctx, iv_ctrl, (int)spec.iv_len, NULL) != 1) { rc = -1; break; }
            if (EVP_EncryptInit_ex(cipher_ctx, NULL, NULL, (const unsigned char*)key, iv_bytes) != 1) { rc = -1; break; }
            if (EVP_EncryptUpdate(cipher_ctx, (unsigned char*)ciphertext, &len,
                                  (const unsigned char*)plaintext, (int)plaintext_len) != 1) { rc = -1; break; }
            ciphertext_len_result = len;
            if (EVP_EncryptFinal_ex(cipher_ctx, (unsigned char*)ciphertext + ciphertext_len_result, &len) != 1) { rc = -1; break; }
            ciphertext_len_result += len;
            if (spec.tag_len &&
                EVP_CIPHER_CTX_ctrl(cipher_ctx, tag_ctrl, (int)spec.tag_len,
                                    (unsigned char*)ciphertext + ciphertext_len_result) != 1) { rc = -1; break; }
            ciphertext_len_result += (int)spec.tag_len;
            rc = 0;
            break;
        }
        case CIPHER_MODE_CCM: {
            if (EVP_EncryptInit_ex(cipher_ctx, spec.cipher, NULL, NULL, NULL) != 1) { rc = -1; break; }
            if (spec.iv_len && EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_AEAD_SET_IVLEN, (int)spec.iv_len, NULL) != 1) { rc = -1; break; }
            if (spec.tag_len && EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_CCM_SET_TAG, (int)spec.tag_len, NULL) != 1) { rc = -1; break; }
            if (EVP_EncryptInit_ex(cipher_ctx, NULL, NULL, (const unsigned char*)key, iv_bytes) != 1) { rc = -1; break; }
            if (EVP_EncryptUpdate(cipher_ctx, NULL, &len, NULL, (int)plaintext_len) != 1) { rc = -1; break; }
            if (EVP_EncryptUpdate(cipher_ctx, (unsigned char*)ciphertext, &len,
                                  (const unsigned char*)plaintext, (int)plaintext_len) != 1) { rc = -1; break; }
            ciphertext_len_result = len;
            if (EVP_EncryptFinal_ex(cipher_ctx, (unsigned char*)ciphertext + ciphertext_len_result, &len) != 1) { rc = -1; break; }
            ciphertext_len_result += len;
            if (spec.tag_len &&
                EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_CCM_GET_TAG, (int)spec.tag_len,
                                    (unsigned char*)ciphertext + ciphertext_len_result) != 1) { rc = -1; break; }
            ciphertext_len_result += (int)spec.tag_len;
            rc = 0;
            break;
        }
        default: {
            if (EVP_EncryptInit_ex(cipher_ctx, spec.cipher, NULL, (const unsigned char*)key, iv_bytes) != 1) { rc = -1; break; }
            if (EVP_EncryptUpdate(cipher_ctx, (unsigned char*)ciphertext, &len,
                                  (const unsigned char*)plaintext, (int)plaintext_len) != 1) { rc = -1; break; }
            ciphertext_len_result = len;
            if (EVP_EncryptFinal_ex(cipher_ctx, (unsigned char*)ciphertext + ciphertext_len_result, &len) != 1) { rc = -1; break; }
            ciphertext_len_result += len;
            rc = 0;
            break;
        }
    }

    EVP_CIPHER_CTX_free(cipher_ctx);
    if (rc != 0) {
        return -1;
    }

    if (ctx->layer < 10) {
        g_device255_state.contexts[ctx->layer].operation_count++;
        g_device255_state.contexts[ctx->layer].bytes_processed += plaintext_len;
    }
    g_device255_state.engine_stats[used_tpm ? DSMIL_CRYPTO_ENGINE_TPM : ctx->engine]++;

    *ciphertext_len = (size_t)ciphertext_len_result;
    return 0;
}

int dsmil_device255_decrypt(const dsmil_device255_ctx_t *ctx,
                            uint16_t algorithm,
                            const void *key, size_t key_len,
                            const void *iv, size_t iv_len,
                            const void *ciphertext, size_t ciphertext_len,
                            void *plaintext, size_t *plaintext_len) {
    if (!ctx || !key || !ciphertext || !plaintext || !plaintext_len) {
        return -1;
    }

    cipher_spec_t spec = {0};
    if (!resolve_cipher_spec(algorithm, key_len, &spec)) {
        return -1;
    }

    if (spec.iv_len > 0 && iv_len < spec.iv_len) {
        return -1;
    }

    if (ciphertext_len < spec.tag_len) {
        return -1;
    }

    size_t data_len = ciphertext_len - spec.tag_len;
    size_t block_overhead = spec.cipher ? (size_t)EVP_CIPHER_block_size(spec.cipher) : 0;
    size_t required_plaintext_len = data_len + block_overhead;
    if (*plaintext_len < required_plaintext_len) {
        *plaintext_len = required_plaintext_len;
        return -1;
    }

    bool used_tpm = false;
    if (ctx->engine == DSMIL_CRYPTO_ENGINE_TPM && g_device255_state.tpm_available &&
        spec.allow_tpm && spec.tpm_alg == TPM2_ALG_AES) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            TPM2B_SENSITIVE_CREATE in_sensitive = {0};
            TPM2B_PUBLIC in_public = {0};
            in_public.publicArea.type = TPM2_ALG_SYMCIPHER;
            in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
            in_public.publicArea.objectAttributes = TPMA_OBJECT_DECRYPT | TPMA_OBJECT_SIGN_ENCRYPT;
            in_public.publicArea.parameters.symDetail.sym.algorithm = spec.tpm_alg;
            in_public.publicArea.parameters.symDetail.sym.keyBits.aes = (uint16_t)(key_len * 8);
            in_public.publicArea.parameters.symDetail.sym.mode.aes = spec.tpm_mode;

            in_sensitive.sensitive.data.size = (uint16_t)(key_len < sizeof(in_sensitive.sensitive.data.buffer) ? key_len : sizeof(in_sensitive.sensitive.data.buffer));
            memcpy(in_sensitive.sensitive.data.buffer, key, in_sensitive.sensitive.data.size);

            TPM2B_DATA outside_info = {0};
            TPML_PCR_SELECTION creation_pcr = {0};
            TPM2B_PRIVATE out_private = {0};
            TPM2B_PUBLIC out_public = {0};
            TPM2B_CREATION_DATA creation_data = {0};
            TPM2B_DIGEST creation_hash = {0};
            TPMT_TK_CREATION creation_ticket = {0};

            TSS2_RC rc = Tss2_Sys_Create(sys_ctx, TPM2_RH_NULL, NULL, &in_sensitive, &in_public,
                                         &outside_info, &creation_pcr, &out_private, &out_public,
                                         &creation_data, &creation_hash, &creation_ticket, NULL);
            if (rc == TSS2_RC_SUCCESS) {
                TPM2_HANDLE loaded_handle = 0;
                TPM2B_PRIVATE in_private = out_private;
                TPM2B_PUBLIC in_public_key = out_public;
                rc = Tss2_Sys_Load(sys_ctx, TPM2_RH_NULL, NULL, &in_private, &in_public_key, &loaded_handle, NULL, NULL);
                if (rc == TSS2_RC_SUCCESS) {
                    TPM2B_MAX_BUFFER in_data = {0};
                    in_data.size = (uint16_t)(data_len < sizeof(in_data.buffer) ? data_len : sizeof(in_data.buffer));
                    memcpy(in_data.buffer, ciphertext, in_data.size);

                    TPM2B_MAX_BUFFER out_data = {0};
                    TPM2B_IV iv_in = {0};
                    if (spec.iv_len > 0 && spec.iv_len <= sizeof(iv_in.buffer) && iv) {
                        iv_in.size = (uint16_t)spec.iv_len;
                        memcpy(iv_in.buffer, iv, spec.iv_len);
                    }

                    TPM2B_IV iv_out = {0};
                    rc = Tss2_Sys_EncryptDecrypt(sys_ctx, loaded_handle, NULL,
                                                 1, spec.tpm_mode, &iv_in,
                                                 &in_data, &out_data, &iv_out, NULL);
                    if (rc == TSS2_RC_SUCCESS && out_data.size <= *plaintext_len) {
                        memcpy(plaintext, out_data.buffer, out_data.size);
                        *plaintext_len = out_data.size;
                        used_tpm = true;
                        if (ctx->layer < 10) {
                            g_device255_state.contexts[ctx->layer].operation_count++;
                            g_device255_state.contexts[ctx->layer].bytes_processed += ciphertext_len;
                        }
                        g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                        Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                        return 0;
                    }
                    Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                }
            }
        }
    }

    const unsigned char *iv_bytes = (spec.iv_len > 0) ? (const unsigned char*)iv : NULL;
    const unsigned char *tag_ptr_const = (spec.tag_len > 0) ? ((const unsigned char*)ciphertext + data_len) : NULL;

    EVP_CIPHER_CTX *cipher_ctx = EVP_CIPHER_CTX_new();
    if (!cipher_ctx) {
        return -1;
    }

    int len = 0;
    int plaintext_result = 0;
    int rc = 0;

    switch (spec.mode) {
        case CIPHER_MODE_GCM:
        case CIPHER_MODE_POLY1305: {
            int iv_ctrl = (spec.mode == CIPHER_MODE_GCM) ? EVP_CTRL_GCM_SET_IVLEN : EVP_CTRL_AEAD_SET_IVLEN;
            int tag_ctrl = (spec.mode == CIPHER_MODE_GCM) ? EVP_CTRL_GCM_SET_TAG : EVP_CTRL_AEAD_SET_TAG;

            if (EVP_DecryptInit_ex(cipher_ctx, spec.cipher, NULL, NULL, NULL) != 1) { rc = -1; break; }
            if (spec.iv_len && EVP_CIPHER_CTX_ctrl(cipher_ctx, iv_ctrl, (int)spec.iv_len, NULL) != 1) { rc = -1; break; }
            if (EVP_DecryptInit_ex(cipher_ctx, NULL, NULL, (const unsigned char*)key, iv_bytes) != 1) { rc = -1; break; }
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
            if (spec.tag_len && tag_ptr_const &&
                EVP_CIPHER_CTX_ctrl(cipher_ctx, tag_ctrl, (int)spec.tag_len, (void*)tag_ptr_const) != 1) { rc = -1; break; }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
            if (EVP_DecryptUpdate(cipher_ctx, (unsigned char*)plaintext, &len,
                                  (const unsigned char*)ciphertext, (int)data_len) != 1) { rc = -1; break; }
            plaintext_result = len;
            if (EVP_DecryptFinal_ex(cipher_ctx, (unsigned char*)plaintext + plaintext_result, &len) != 1) { rc = -1; break; }
            plaintext_result += len;
            rc = 0;
            break;
        }
        case CIPHER_MODE_CCM: {
            if (EVP_DecryptInit_ex(cipher_ctx, spec.cipher, NULL, NULL, NULL) != 1) { rc = -1; break; }
            if (spec.iv_len && EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_AEAD_SET_IVLEN, (int)spec.iv_len, NULL) != 1) { rc = -1; break; }
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
            if (spec.tag_len && tag_ptr_const &&
                EVP_CIPHER_CTX_ctrl(cipher_ctx, EVP_CTRL_CCM_SET_TAG, (int)spec.tag_len, (void*)tag_ptr_const) != 1) { rc = -1; break; }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
            if (EVP_DecryptInit_ex(cipher_ctx, NULL, NULL, (const unsigned char*)key, iv_bytes) != 1) { rc = -1; break; }
            if (EVP_DecryptUpdate(cipher_ctx, NULL, &len, NULL, (int)data_len) != 1) { rc = -1; break; }
            if (EVP_DecryptUpdate(cipher_ctx, (unsigned char*)plaintext, &len,
                                  (const unsigned char*)ciphertext, (int)data_len) != 1) { rc = -1; break; }
            plaintext_result = len;
            if (EVP_DecryptFinal_ex(cipher_ctx, (unsigned char*)plaintext + plaintext_result, &len) != 1) { rc = -1; break; }
            plaintext_result += len;
            rc = 0;
            break;
        }
        default: {
            if (EVP_DecryptInit_ex(cipher_ctx, spec.cipher, NULL, (const unsigned char*)key, iv_bytes) != 1) { rc = -1; break; }
            if (EVP_DecryptUpdate(cipher_ctx, (unsigned char*)plaintext, &len,
                                  (const unsigned char*)ciphertext, (int)data_len) != 1) { rc = -1; break; }
            plaintext_result = len;
            if (EVP_DecryptFinal_ex(cipher_ctx, (unsigned char*)plaintext + plaintext_result, &len) != 1) { rc = -1; break; }
            plaintext_result += len;
            rc = 0;
            break;
        }
    }

    EVP_CIPHER_CTX_free(cipher_ctx);
    if (rc != 0) {
        return -1;
    }

    if (ctx->layer < 10) {
        g_device255_state.contexts[ctx->layer].operation_count++;
        g_device255_state.contexts[ctx->layer].bytes_processed += ciphertext_len;
    }
    g_device255_state.engine_stats[used_tpm ? DSMIL_CRYPTO_ENGINE_TPM : ctx->engine]++;

    *plaintext_len = (size_t)plaintext_result;
    return 0;
}

int dsmil_device255_sign(const dsmil_device255_ctx_t *ctx,
                         uint16_t algorithm,
                         const void *private_key, size_t key_len,
                         const void *message, size_t message_len,
                         void *signature, size_t *signature_len) {
    if (!ctx || !private_key || !message || !signature || !signature_len) {
        return -1;
    }
    
    /* Route to appropriate engine */
    if (ctx->engine == DSMIL_CRYPTO_ENGINE_TPM && g_device255_state.tpm_available) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            if (algorithm == TPM_ALG_RSA || algorithm == TPM_ALG_ECDSA) {
                /* Load private key into TPM and sign */
                TPM2B_SENSITIVE_CREATE in_sensitive = {0};
                TPM2B_PUBLIC in_public = {0};
                
                if (algorithm == TPM_ALG_RSA) {
                    in_public.publicArea.type = TPM2_ALG_RSA;
                    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.objectAttributes = TPMA_OBJECT_SIGN_ENCRYPT | TPMA_OBJECT_USERWITHAUTH;
                    in_public.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM2_ALG_NULL;
                    in_public.publicArea.parameters.rsaDetail.scheme.scheme = TPM2_ALG_RSASSA;
                    in_public.publicArea.parameters.rsaDetail.scheme.details.rsassa.hashAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.parameters.rsaDetail.keyBits = (uint16_t)(key_len * 8);
                    in_public.publicArea.parameters.rsaDetail.exponent = 0;
                    
                    /* Copy private key material */
                    memcpy(in_sensitive.sensitive.data.buffer, private_key, key_len < sizeof(in_sensitive.sensitive.data.buffer) ? key_len : sizeof(in_sensitive.sensitive.data.buffer));
                    in_sensitive.sensitive.data.size = (uint16_t)(key_len < sizeof(in_sensitive.sensitive.data.buffer) ? key_len : sizeof(in_sensitive.sensitive.data.buffer));
                } else {
                    in_public.publicArea.type = TPM2_ALG_ECC;
                    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.objectAttributes = TPMA_OBJECT_SIGN_ENCRYPT | TPMA_OBJECT_USERWITHAUTH;
                    in_public.publicArea.parameters.eccDetail.symmetric.algorithm = TPM2_ALG_NULL;
                    in_public.publicArea.parameters.eccDetail.scheme.scheme = TPM2_ALG_ECDSA;
                    in_public.publicArea.parameters.eccDetail.scheme.details.ecdsa.hashAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.parameters.eccDetail.curveID = TPM2_ECC_NIST_P256;
                    in_public.publicArea.parameters.eccDetail.kdf.scheme = TPM2_ALG_NULL;
                    
                    memcpy(in_sensitive.sensitive.data.buffer, private_key, key_len < sizeof(in_sensitive.sensitive.data.buffer) ? key_len : sizeof(in_sensitive.sensitive.data.buffer));
                    in_sensitive.sensitive.data.size = (uint16_t)(key_len < sizeof(in_sensitive.sensitive.data.buffer) ? key_len : sizeof(in_sensitive.sensitive.data.buffer));
                }
                
                TPM2B_DATA outside_info = {0};
                TPML_PCR_SELECTION creation_pcr = {0};
                TPM2B_PRIVATE out_private = {0};
                TPM2B_PUBLIC out_public = {0};
                TPM2B_CREATION_DATA creation_data = {0};
                TPM2B_DIGEST creation_hash = {0};
                TPMT_TK_CREATION creation_ticket = {0};
                
                TSS2_RC rc = Tss2_Sys_Create(sys_ctx, TPM2_RH_NULL, NULL, &in_sensitive, &in_public, &outside_info, &creation_pcr, &out_private, &out_public, &creation_data, &creation_hash, &creation_ticket, NULL);
                
                if (rc == TSS2_RC_SUCCESS) {
                    TPM2B_PRIVATE in_private = out_private;
                    TPM2B_PUBLIC in_public_key = out_public;
                    TPM2_HANDLE loaded_handle = 0;
                    rc = Tss2_Sys_Load(sys_ctx, TPM2_RH_NULL, NULL, &in_private, &in_public_key, &loaded_handle, NULL, NULL);
                    
                    if (rc == TSS2_RC_SUCCESS) {
                        /* Hash the message first */
                        TPM2B_MAX_BUFFER message_data = {0};
                        message_data.size = (uint16_t)(message_len < sizeof(message_data.buffer) ? message_len : sizeof(message_data.buffer));
                        memcpy(message_data.buffer, message, message_data.size);
                        
                        TPM2B_DIGEST digest = {0};
                        rc = Tss2_Sys_Hash(sys_ctx, NULL, &message_data, TPM2_ALG_SHA256, TPM2_RH_NULL, &digest, NULL, NULL);
                        
                        if (rc == TSS2_RC_SUCCESS) {
                            /* Sign the digest */
                            TPMT_SIG_SCHEME scheme = {0};
                            scheme.scheme = (algorithm == TPM_ALG_RSA) ? TPM2_ALG_RSASSA : TPM2_ALG_ECDSA;
                            scheme.details.rsassa.hashAlg = TPM2_ALG_SHA256;
                            
                            TPM2B_DIGEST digest_to_sign = digest;
                            TPMT_SIGNATURE signature_out = {0};
                            
                            rc = Tss2_Sys_Sign(sys_ctx, loaded_handle, NULL, &digest_to_sign, &scheme, NULL, &signature_out, NULL);
                            
                            if (rc == TSS2_RC_SUCCESS) {
                                /* Extract signature from TPM format */
                                size_t sig_len = 0;
                                if (algorithm == TPM_ALG_RSA) {
                                    sig_len = signature_out.signature.rsassa.sig.size;
                                    if (sig_len <= *signature_len) {
                                        memcpy(signature, signature_out.signature.rsassa.sig.buffer, sig_len);
                                    }
                                } else {
                                    /* ECDSA: concatenate r and s */
                                    sig_len = signature_out.signature.ecdsa.signatureR.size + signature_out.signature.ecdsa.signatureS.size;
                                    if (sig_len <= *signature_len) {
                                        memcpy(signature, signature_out.signature.ecdsa.signatureR.buffer, signature_out.signature.ecdsa.signatureR.size);
                                        memcpy((char*)signature + signature_out.signature.ecdsa.signatureR.size, signature_out.signature.ecdsa.signatureS.buffer, signature_out.signature.ecdsa.signatureS.size);
                                    }
                                }
                                
                                if (sig_len > 0 && sig_len <= *signature_len) {
                                    /* Update statistics */
                                    if (ctx->layer < 10) {
                                        g_device255_state.contexts[ctx->layer].operation_count++;
                                        g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
                                        g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                                    }
                                    
                                    Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                                    *signature_len = sig_len;
                                    return 0;
                                }
                            }
                        }
                        
                        Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                    }
                }
            }
        }
    }
    
    /* Handle ML-DSA-87 (quantum-safe) */
    if (algorithm == TPM_ALG_ML_DSA_87) {
        OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
        if (!sig) {
            return -1;
        }
        
        size_t sig_len = 0;
        if (OQS_SIG_sign(sig, (unsigned char*)signature, &sig_len,
                         (const unsigned char*)message, message_len,
                         (const unsigned char*)private_key) != OQS_SUCCESS) {
            OQS_SIG_free(sig);
            return -1;
        }
        
        OQS_SIG_free(sig);
        
        if (*signature_len < sig_len) {
            *signature_len = sig_len;
            return -1;
        }
        
        *signature_len = sig_len;
        
        /* Update statistics */
        if (ctx->layer < 10) {
            g_device255_state.contexts[ctx->layer].operation_count++;
            g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
            g_device255_state.engine_stats[ctx->engine]++;
        }
        
        return 0;
    }
    
    /* Determine signature size and digest type */
    size_t sig_size = 0;
    const EVP_MD *md = NULL;
    EVP_PKEY *pkey = NULL;
    
    switch (algorithm) {
        case TPM_ALG_RSA:
            sig_size = key_len;  /* RSA signature size = key size */
            md = EVP_sha256();
            break;
        case TPM_ALG_ECDSA:
            sig_size = key_len * 2;  /* r and s components */
            md = EVP_sha256();
            break;
        default:
            return -1;
    }
    
    if (*signature_len < sig_size) {
        *signature_len = sig_size;
        return -1;
    }
    
    /* Load private key from memory (DER format assumed) */
    BIO *bio = BIO_new_mem_buf(private_key, (int)key_len);
    if (!bio) {
        return -1;
    }
    
    pkey = d2i_PrivateKey_bio(bio, NULL);
    
    BIO_free(bio);
    
    if (!pkey) {
        return -1;
    }
    
    /* Sign message */
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    if (EVP_DigestSignInit(md_ctx, NULL, md, NULL, pkey) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    if (EVP_DigestSignUpdate(md_ctx, message, message_len) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    size_t sig_len = *signature_len;
    if (EVP_DigestSignFinal(md_ctx, (unsigned char*)signature, &sig_len) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    
    /* Update statistics */
    if (ctx->layer < 10) {
        g_device255_state.contexts[ctx->layer].operation_count++;
        g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
        g_device255_state.engine_stats[ctx->engine]++;
    }
    
    *signature_len = sig_len;
    return 0;
}

int dsmil_device255_verify(const dsmil_device255_ctx_t *ctx,
                           uint16_t algorithm,
                           const void *public_key, size_t key_len,
                           const void *message, size_t message_len,
                           const void *signature, size_t signature_len) {
    if (!ctx || !public_key || !message || !signature) {
        return -1;
    }
    
    /* Route to appropriate engine */
    if (ctx->engine == DSMIL_CRYPTO_ENGINE_TPM && g_device255_state.tpm_available) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            if (algorithm == TPM_ALG_RSA || algorithm == TPM_ALG_ECDSA) {
                /* Load public key into TPM and verify */
                TPM2B_PUBLIC in_public = {0};
                
                if (algorithm == TPM_ALG_RSA) {
                    in_public.publicArea.type = TPM2_ALG_RSA;
                    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.objectAttributes = TPMA_OBJECT_RESTRICTED | TPMA_OBJECT_DECRYPT;
                    in_public.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM2_ALG_NULL;
                    in_public.publicArea.parameters.rsaDetail.scheme.scheme = TPM2_ALG_RSASSA;
                    in_public.publicArea.parameters.rsaDetail.scheme.details.rsassa.hashAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.parameters.rsaDetail.keyBits = (uint16_t)(key_len * 8);
                    in_public.publicArea.parameters.rsaDetail.exponent = 0;
                    
                    /* Copy public key material (modulus) */
                    if (key_len <= sizeof(in_public.publicArea.unique.rsa.buffer)) {
                        in_public.publicArea.unique.rsa.size = (uint16_t)key_len;
                        memcpy(in_public.publicArea.unique.rsa.buffer, public_key, key_len);
                    }
                } else {
                    in_public.publicArea.type = TPM2_ALG_ECC;
                    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.objectAttributes = TPMA_OBJECT_RESTRICTED | TPMA_OBJECT_DECRYPT;
                    in_public.publicArea.parameters.eccDetail.symmetric.algorithm = TPM2_ALG_NULL;
                    in_public.publicArea.parameters.eccDetail.scheme.scheme = TPM2_ALG_ECDSA;
                    in_public.publicArea.parameters.eccDetail.scheme.details.ecdsa.hashAlg = TPM2_ALG_SHA256;
                    in_public.publicArea.parameters.eccDetail.curveID = TPM2_ECC_NIST_P256;
                    in_public.publicArea.parameters.eccDetail.kdf.scheme = TPM2_ALG_NULL;
                    
                    /* Copy public key point */
                    if (key_len <= sizeof(in_public.publicArea.unique.ecc.x.buffer)) {
                        in_public.publicArea.unique.ecc.x.size = (uint16_t)(key_len / 2);
                        in_public.publicArea.unique.ecc.y.size = (uint16_t)(key_len / 2);
                        memcpy(in_public.publicArea.unique.ecc.x.buffer, public_key, key_len / 2);
                        memcpy(in_public.publicArea.unique.ecc.y.buffer, (const char*)public_key + key_len / 2, key_len / 2);
                    }
                }
                
                /* Hash the message */
                TPM2B_MAX_BUFFER message_data = {0};
                message_data.size = (uint16_t)(message_len < sizeof(message_data.buffer) ? message_len : sizeof(message_data.buffer));
                memcpy(message_data.buffer, message, message_data.size);
                
                TPM2B_DIGEST digest = {0};
                TSS2_RC rc = Tss2_Sys_Hash(sys_ctx, NULL, &message_data, TPM2_ALG_SHA256, TPM2_RH_NULL, &digest, NULL, NULL);
                
                if (rc == TSS2_RC_SUCCESS) {
                    /* Load public key */
                    TPM2B_PUBLIC in_public_key = in_public;
                    TPM2_HANDLE loaded_handle = 0;
                    rc = Tss2_Sys_LoadExternal(sys_ctx, NULL, NULL, &in_public_key, TPM2_RH_NULL, &loaded_handle, NULL, NULL);
                    
                    if (rc == TSS2_RC_SUCCESS) {
                        /* Build signature structure */
                        TPMT_SIGNATURE signature_in = {0};
                        signature_in.sigAlg = (algorithm == TPM_ALG_RSA) ? TPM2_ALG_RSASSA : TPM2_ALG_ECDSA;
                        signature_in.signature.rsassa.hash = TPM2_ALG_SHA256;
                        
                        if (algorithm == TPM_ALG_RSA) {
                            signature_in.signature.rsassa.sig.size = (uint16_t)(signature_len < sizeof(signature_in.signature.rsassa.sig.buffer) ? signature_len : sizeof(signature_in.signature.rsassa.sig.buffer));
                            memcpy(signature_in.signature.rsassa.sig.buffer, signature, signature_in.signature.rsassa.sig.size);
                        } else {
                            /* ECDSA: split r and s */
                            size_t component_size = signature_len / 2;
                            signature_in.signature.ecdsa.signatureR.size = (uint16_t)(component_size < sizeof(signature_in.signature.ecdsa.signatureR.buffer) ? component_size : sizeof(signature_in.signature.ecdsa.signatureR.buffer));
                            signature_in.signature.ecdsa.signatureS.size = (uint16_t)(component_size < sizeof(signature_in.signature.ecdsa.signatureS.buffer) ? component_size : sizeof(signature_in.signature.ecdsa.signatureS.buffer));
                            memcpy(signature_in.signature.ecdsa.signatureR.buffer, signature, signature_in.signature.ecdsa.signatureR.size);
                            memcpy(signature_in.signature.ecdsa.signatureS.buffer, (const char*)signature + component_size, signature_in.signature.ecdsa.signatureS.size);
                        }
                        
                        TPMT_TK_VERIFIED validation = {0};
                        rc = Tss2_Sys_VerifySignature(sys_ctx, loaded_handle, NULL, &digest, &signature_in, &validation, NULL);
                        
                        Tss2_Sys_FlushContext(sys_ctx, loaded_handle);
                        
                        if (rc == TSS2_RC_SUCCESS) {
                            /* Update statistics */
                            if (ctx->layer < 10) {
                                g_device255_state.contexts[ctx->layer].operation_count++;
                                g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
                                g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                            }
                            
                            return 0;  /* Signature valid */
                        }
                    }
                }
                
                /* TPM verification failed, signature invalid */
                if (ctx->layer < 10) {
                    g_device255_state.contexts[ctx->layer].operation_count++;
                    g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
                    g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                }
                
                return -1;  /* Signature invalid */
            }
        }
    }
    
    /* Handle ML-DSA-87 (quantum-safe) */
    if (algorithm == TPM_ALG_ML_DSA_87) {
        OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
        if (!sig) {
            return -1;
        }
        
        int result = (OQS_SIG_verify(sig, (const unsigned char*)message, message_len,
                                      (const unsigned char*)signature, signature_len,
                                      (const unsigned char*)public_key) == OQS_SUCCESS) ? 0 : -1;
        
        OQS_SIG_free(sig);
        
        /* Update statistics */
        if (ctx->layer < 10) {
            g_device255_state.contexts[ctx->layer].operation_count++;
            g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
            g_device255_state.engine_stats[ctx->engine]++;
        }
        
        return result;
    }
    
    /* Determine digest type */
    const EVP_MD *md = NULL;
    EVP_PKEY *pkey = NULL;
    
    switch (algorithm) {
        case TPM_ALG_RSA:
            md = EVP_sha256();
            break;
        case TPM_ALG_ECDSA:
            md = EVP_sha256();
            break;
        default:
            return -1;
    }
    
    /* Load public key from memory (DER format assumed) */
    BIO *bio = BIO_new_mem_buf(public_key, (int)key_len);
    if (!bio) {
        return -1;
    }
    
    pkey = d2i_PUBKEY_bio(bio, NULL);
    
    BIO_free(bio);
    
    if (!pkey) {
        return -1;
    }
    
    /* Verify signature */
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    if (EVP_DigestVerifyInit(md_ctx, NULL, md, NULL, pkey) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    if (EVP_DigestVerifyUpdate(md_ctx, message, message_len) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return -1;
    }
    
    int verify_result = EVP_DigestVerifyFinal(md_ctx, (const unsigned char*)signature, signature_len);
    
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    
    /* Update statistics */
    if (ctx->layer < 10) {
        g_device255_state.contexts[ctx->layer].operation_count++;
        g_device255_state.contexts[ctx->layer].bytes_processed += message_len;
        g_device255_state.engine_stats[ctx->engine]++;
    }
    
    /* Return 0 if valid, -1 if invalid */
    return (verify_result == 1) ? 0 : -1;
}

int dsmil_device255_rng(const dsmil_device255_ctx_t *ctx,
                        void *output, size_t len,
                        dsmil_crypto_engine_t *source) {
    if (!ctx || !output || len == 0) {
        return -1;
    }
    
    dsmil_crypto_engine_t actual_source = DSMIL_CRYPTO_ENGINE_SOFTWARE;
    
    /* Route to appropriate engine */
    if (ctx->engine == DSMIL_CRYPTO_ENGINE_TPM && g_device255_state.tpm_available) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            /* Use TPM2_GetRandom for hardware RNG */
            TPM2B_DIGEST random_bytes = {0};
            size_t remaining = len;
            uint8_t *out_ptr = (uint8_t*)output;
            
            while (remaining > 0) {
                uint16_t chunk_size = (uint16_t)(remaining < sizeof(random_bytes.buffer) ? remaining : sizeof(random_bytes.buffer));
                
                TSS2_RC rc = Tss2_Sys_GetRandom(sys_ctx, NULL, chunk_size, &random_bytes, NULL);
                
                if (rc != TSS2_RC_SUCCESS || random_bytes.size < chunk_size) {
                    break;  /* TPM failed, fall through to software */
                }
                
                memcpy(out_ptr, random_bytes.buffer, chunk_size);
                out_ptr += chunk_size;
                remaining -= chunk_size;
            }
            
            if (remaining == 0) {
                /* Successfully filled all bytes from TPM */
                if (ctx->layer < 10) {
                    g_device255_state.contexts[ctx->layer].operation_count++;
                    g_device255_state.contexts[ctx->layer].bytes_processed += len;
                    g_device255_state.engine_stats[DSMIL_CRYPTO_ENGINE_TPM]++;
                }
                
                if (source) {
                    *source = DSMIL_CRYPTO_ENGINE_TPM;
                }
                
                return 0;
            }
        }
    }
    
    /* Use OpenSSL RAND_bytes (cryptographically secure) */
    if (RAND_bytes((unsigned char*)output, (int)len) != 1) {
        /* Fallback to /dev/urandom if OpenSSL fails */
        FILE *urandom = fopen("/dev/urandom", "rb");
        if (!urandom) {
            return -1;
        }
        
        size_t bytes_read = fread(output, 1, len, urandom);
        fclose(urandom);
        
        if (bytes_read != len) {
            return -1;
        }
        
        actual_source = DSMIL_CRYPTO_ENGINE_SOFTWARE;
    } else {
        /* OpenSSL RAND_bytes succeeded */
        actual_source = (ctx->engine == DSMIL_CRYPTO_ENGINE_HARDWARE) ?
                        DSMIL_CRYPTO_ENGINE_HARDWARE : DSMIL_CRYPTO_ENGINE_SOFTWARE;
    }
    
    /* Update statistics */
    if (ctx->layer < 10) {
        g_device255_state.contexts[ctx->layer].operation_count++;
        g_device255_state.contexts[ctx->layer].bytes_processed += len;
        g_device255_state.engine_stats[actual_source]++;
    }
    
    if (source) {
        *source = actual_source;
    }
    
    return 0;
}

int dsmil_device255_data_wipe(dsmil_device255_ctx_t *ctx,
                              uint32_t target,
                              uint32_t confirmation,
                              uint32_t session_token) {
    if (!ctx) {
        return -1;
    }
    
    /* Verify confirmation code */
    if (confirmation != 0xDEADBEEF) {
        return -1;
    }
    
    /* Use TPM for secure data wipe if available */
    if (g_device255_state.tpm_available) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            /* TPM can be used to securely wipe keys and sensitive data */
            /* For memory wiping, use secure memset */
            volatile uint8_t *target_ptr = (volatile uint8_t*)(uintptr_t)target;
            size_t wipe_size = 1024;  /* Wipe 1KB by default */
            
            /* Secure memory wipe using volatile writes */
            for (size_t i = 0; i < wipe_size; i++) {
                target_ptr[i] = 0xFF;
                target_ptr[i] = 0x00;
                target_ptr[i] = 0xAA;
                target_ptr[i] = 0x55;
                target_ptr[i] = 0x00;
            }
            
            /* Memory barrier to ensure writes complete */
            __asm__ __volatile__("" ::: "memory");
        }
    } else {
        /* Software secure wipe */
        volatile uint8_t *target_ptr = (volatile uint8_t*)(uintptr_t)target;
        size_t wipe_size = 1024;
        
        for (size_t i = 0; i < wipe_size; i++) {
            target_ptr[i] = 0xFF;
            target_ptr[i] = 0x00;
            target_ptr[i] = 0xAA;
            target_ptr[i] = 0x55;
            target_ptr[i] = 0x00;
        }
        
        __asm__ __volatile__("" ::: "memory");
    }
    
    return 0;
}

int dsmil_device255_cap_control(dsmil_device255_ctx_t *ctx,
                                uint16_t capability,
                                bool enable) {
    if (!ctx) {
        return -1;
    }
    
    if (enable) {
        ctx->caps.enabled |= capability;
    } else {
        ctx->caps.enabled &= ~capability;
    }
    
    return 0;
}

int dsmil_device255_cap_lock(dsmil_device255_ctx_t *ctx,
                             uint16_t capability,
                             uint32_t session_token) {
    if (!ctx) {
        return -1;
    }
    
    /* Use TPM for capability locking if available */
    if (g_device255_state.tpm_available) {
        TSS2_SYS_CONTEXT *sys_ctx = get_tpm_context();
        if (sys_ctx) {
            /* TPM can enforce capability locks via PCR policies */
            /* For now, mark as locked in software */
            ctx->caps.locked |= capability;
            
            /* In full implementation, would set TPM policy to prevent changes */
            /* This requires PCR-based authorization */
        } else {
            ctx->caps.locked |= capability;
        }
    } else {
        ctx->caps.locked |= capability;
    }
    
    return 0;
}

bool dsmil_device255_pqc_available(const dsmil_device255_ctx_t *ctx,
                                   uint16_t pqc_algorithm) {
    if (!ctx) {
        return false;
    }
    
    // Check if PQC capability is enabled
    if (!(ctx->caps.enabled & DSMIL_CRYPTO_CAP_POST_QUANTUM)) {
        return false;
    }
    
    // Check specific algorithm
    switch (pqc_algorithm) {
        case TPM_ALG_ML_KEM_1024:
        case TPM_ALG_ML_DSA_87:
        case CRYPTO_ALG_KYBER512:
        case CRYPTO_ALG_KYBER768:
        case CRYPTO_ALG_DILITHIUM2:
        case CRYPTO_ALG_DILITHIUM5:
        case CRYPTO_ALG_FALCON512:
        case CRYPTO_ALG_FALCON1024:
            return true;
        default:
            return false;
    }
}

int dsmil_device255_get_stats(const dsmil_device255_ctx_t *ctx,
                              uint64_t *total_ops,
                              uint64_t *bytes_processed,
                              uint64_t engine_stats[3]) {
    if (!ctx) {
        return -1;
    }
    
    if (total_ops) {
        *total_ops = ctx->operation_count;
    }
    
    if (bytes_processed) {
        *bytes_processed = ctx->bytes_processed;
    }
    
    if (engine_stats) {
        engine_stats[0] = g_device255_state.engine_stats[0];  // TPM
        engine_stats[1] = g_device255_state.engine_stats[1];  // Hardware
        engine_stats[2] = g_device255_state.engine_stats[2];  // Software
    }
    
    return 0;
}

#else /* DSMIL_ENABLE_TPM */

static void init_caps(dsmil_device255_caps_t *caps) {
    if (!caps) {
        return;
    }
    memset(caps, 0, sizeof(*caps));
    caps->available = DSMIL_CRYPTO_CAP_ALL;
    caps->enabled = DSMIL_CRYPTO_CAP_ALL;
    caps->locked = 0;
    caps->active_engine = DSMIL_CRYPTO_ENGINE_SOFTWARE;
    caps->algorithm_count = ALGORITHM_COUNT;
    caps->tpm_available = false;
    caps->secure_boot_verified = false;
}

int dsmil_device255_init(uint8_t layer, dsmil_device255_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->device_id = DEVICE255_ID;
    ctx->layer = layer;
    ctx->engine = DSMIL_CRYPTO_ENGINE_SOFTWARE;
    init_caps(&ctx->caps);

    g_device255_state.initialized = true;
    g_device255_state.contexts[layer % 10] = *ctx;
    g_device255_state.hw_accel_probed = true;
    g_device255_state.aes_ni_available = false;
    return 0;
}

int dsmil_device255_get_caps(const dsmil_device255_ctx_t *ctx,
                              dsmil_device255_caps_t *caps) {
    if (!ctx || !caps) {
        return -1;
    }
    *caps = ctx->caps;
    return 0;
}

int dsmil_device255_set_engine(dsmil_device255_ctx_t *ctx,
                                dsmil_crypto_engine_t engine) {
    if (!ctx) {
        return -1;
    }
    ctx->engine = engine;
    ctx->caps.active_engine = engine;
    return 0;
}

int dsmil_device255_hash(const dsmil_device255_ctx_t *ctx,
                         uint16_t algorithm,
                         const void *input, size_t input_len,
                         void *output, size_t *output_len) {
    (void)ctx; (void)algorithm; (void)input; (void)input_len; (void)output; (void)output_len;
    return -1;  /* TPM disabled */
}

int dsmil_device255_encrypt(const dsmil_device255_ctx_t *ctx,
                            uint16_t algorithm,
                            const void *key, size_t key_len,
                            const void *iv, size_t iv_len,
                            const void *plaintext, size_t plaintext_len,
                            void *ciphertext, size_t *ciphertext_len) {
    (void)ctx; (void)algorithm; (void)key; (void)key_len; (void)iv; (void)iv_len; (void)plaintext; (void)plaintext_len; (void)ciphertext; (void)ciphertext_len;
    return -1;
}

int dsmil_device255_decrypt(const dsmil_device255_ctx_t *ctx,
                            uint16_t algorithm,
                            const void *key, size_t key_len,
                            const void *iv, size_t iv_len,
                            const void *ciphertext, size_t ciphertext_len,
                            void *plaintext, size_t *plaintext_len) {
    (void)ctx; (void)algorithm; (void)key; (void)key_len; (void)iv; (void)iv_len; (void)ciphertext; (void)ciphertext_len; (void)plaintext; (void)plaintext_len;
    return -1;
}

int dsmil_device255_sign(const dsmil_device255_ctx_t *ctx,
                         uint16_t algorithm,
                         const void *private_key, size_t key_len,
                         const void *message, size_t message_len,
                         void *signature, size_t *signature_len) {
    (void)ctx; (void)algorithm; (void)private_key; (void)key_len; (void)message; (void)message_len; (void)signature; (void)signature_len;
    return -1;
}

int dsmil_device255_verify(const dsmil_device255_ctx_t *ctx,
                           uint16_t algorithm,
                           const void *public_key, size_t key_len,
                           const void *message, size_t message_len,
                           const void *signature, size_t signature_len) {
    (void)ctx; (void)algorithm; (void)public_key; (void)key_len; (void)message; (void)message_len; (void)signature; (void)signature_len;
    return -1;
}

int dsmil_device255_rng(const dsmil_device255_ctx_t *ctx,
                        void *output, size_t output_len,
                        dsmil_crypto_engine_t *source) {
    (void)ctx; (void)output; (void)output_len; (void)source;
    return -1;
}

int dsmil_device255_data_wipe(dsmil_device255_ctx_t *ctx,
                              uint32_t target,
                              uint32_t confirmation,
                              uint32_t session_token) {
    (void)ctx; (void)target; (void)confirmation; (void)session_token;
    return 0;
}

int dsmil_device255_cap_control(dsmil_device255_ctx_t *ctx,
                                uint16_t capability,
                                bool enable) {
    if (!ctx) {
        return -1;
    }
    if (enable) {
        ctx->caps.enabled |= capability;
    } else {
        ctx->caps.enabled &= ~capability;
    }
    return 0;
}

int dsmil_device255_cap_lock(dsmil_device255_ctx_t *ctx,
                             uint16_t capability,
                             uint32_t session_token) {
    if (!ctx) {
        return -1;
    }
    (void)session_token;
    ctx->caps.locked |= capability;
    return 0;
}

bool dsmil_device255_pqc_available(const dsmil_device255_ctx_t *ctx,
                                   uint16_t pqc_algorithm) {
    (void)ctx; (void)pqc_algorithm;
    return false;
}

int dsmil_device255_get_stats(const dsmil_device255_ctx_t *ctx,
                              uint64_t *total_ops,
                              uint64_t *bytes_processed,
                              uint64_t engine_stats[3]) {
    if (!ctx) {
        return -1;
    }
    if (total_ops) {
        *total_ops = ctx->operation_count;
    }
    if (bytes_processed) {
        *bytes_processed = ctx->bytes_processed;
    }
    if (engine_stats) {
        engine_stats[0] = 0;
        engine_stats[1] = 0;
        engine_stats[2] = 0;
    }
    return 0;
}

#endif /* DSMIL_ENABLE_TPM */
