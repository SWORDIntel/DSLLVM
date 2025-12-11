// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "dsmil_device255_crypto.h"

#include <oqs/oqs.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int passed;
    int failed;
    int skipped;
} test_results_t;

static void record_pass(test_results_t *r, const char *name) {
    r->passed++;
    printf("[PASS] %s\n", name);
}

static void record_fail(test_results_t *r, const char *name, const char *msg) {
    r->failed++;
    printf("[FAIL] %s: %s\n", name, msg);
}

static void record_skip(test_results_t *r, const char *name, const char *msg) {
    r->skipped++;
    printf("[SKIP] %s: %s\n", name, msg);
}

static bool digest_expected(const EVP_MD *md, const uint8_t *msg, size_t msg_len,
                            uint8_t *out, size_t *out_len) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return false;
    }
    unsigned int len = 0;
    bool ok = EVP_DigestInit_ex(ctx, md, NULL) == 1 &&
              EVP_DigestUpdate(ctx, msg, msg_len) == 1 &&
              EVP_DigestFinal_ex(ctx, out, &len) == 1;
    if (ok) {
        *out_len = len;
    }
    EVP_MD_CTX_free(ctx);
    return ok;
}

static bool buffers_equal(const uint8_t *a, const uint8_t *b, size_t len) {
    return memcmp(a, b, len) == 0;
}

static int run_tests(void) {
    test_results_t res = {0, 0, 0};
    dsmil_device255_ctx_t ctx = {0};
    if (dsmil_device255_init(0, &ctx) != 0) {
        printf("[FAIL] init: unable to initialize device255 context\n");
        return 1;
    }

    /* Hash tests */
    {
        const uint8_t msg[] = "hello world";
        uint8_t out[64] = {0};
        size_t out_len = sizeof(out);
        if (dsmil_device255_hash(&ctx, TPM_ALG_SHA256, msg, sizeof(msg) - 1, out, &out_len) == 0 && out_len == 32) {
            uint8_t exp[64] = {0};
            size_t exp_len = 0;
            if (digest_expected(EVP_sha256(), msg, sizeof(msg) - 1, exp, &exp_len) && exp_len == 32 &&
                buffers_equal(out, exp, 32)) {
                record_pass(&res, "hash_sha256");
            } else {
                record_fail(&res, "hash_sha256", "digest mismatch");
            }
        } else {
            record_fail(&res, "hash_sha256", "call failed");
        }
    }
    {
        const uint8_t msg[] = "device255-sha3-512";
        uint8_t out[64] = {0};
        size_t out_len = sizeof(out);
        if (dsmil_device255_hash(&ctx, TPM_ALG_SHA3_512, msg, sizeof(msg) - 1, out, &out_len) == 0 && out_len == 64) {
            uint8_t exp[64] = {0};
            size_t exp_len = 0;
            if (digest_expected(EVP_sha3_512(), msg, sizeof(msg) - 1, exp, &exp_len) && exp_len == 64 &&
                buffers_equal(out, exp, 64)) {
                record_pass(&res, "hash_sha3_512");
            } else {
                record_fail(&res, "hash_sha3_512", "digest mismatch");
            }
        } else {
            record_fail(&res, "hash_sha3_512", "call failed");
        }
    }

    /* Symmetric AEAD tests */
    dsmil_device255_set_engine(&ctx, DSMIL_CRYPTO_ENGINE_SOFTWARE);
    {
        const uint8_t key[32] = {0};
        const uint8_t iv[12] = {1,2,3,4,5,6,7,8,9,10,11,12};
        const uint8_t pt[] = "gcm-roundtrip";
        uint8_t ct[128] = {0};
        size_t ct_len = sizeof(ct);
        if (dsmil_device255_encrypt(&ctx, CRYPTO_ALG_AES_256_GCM, key, sizeof(key),
                                    iv, sizeof(iv), pt, sizeof(pt) - 1, ct, &ct_len) == 0) {
            uint8_t dec[128] = {0};
            size_t dec_len = sizeof(dec);
            if (dsmil_device255_decrypt(&ctx, CRYPTO_ALG_AES_256_GCM, key, sizeof(key),
                                        iv, sizeof(iv), ct, ct_len, dec, &dec_len) == 0 &&
                dec_len == sizeof(pt) - 1 && buffers_equal(pt, dec, dec_len)) {
                record_pass(&res, "aes_256_gcm_roundtrip");
            } else {
                record_fail(&res, "aes_256_gcm_roundtrip", "decrypt mismatch");
            }
        } else {
            record_fail(&res, "aes_256_gcm_roundtrip", "encrypt failed");
        }
    }
    {
        const uint8_t key[16] = {0};
        const uint8_t iv[12] = {9,8,7,6,5,4,3,2,1,0,1,2};
        const uint8_t pt[] = "ccm-roundtrip";
        uint8_t ct[128] = {0};
        size_t ct_len = sizeof(ct);
        if (dsmil_device255_encrypt(&ctx, CRYPTO_ALG_AES_128_CCM, key, sizeof(key),
                                    iv, sizeof(iv), pt, sizeof(pt) - 1, ct, &ct_len) == 0) {
            uint8_t dec[128] = {0};
            size_t dec_len = sizeof(dec);
            if (dsmil_device255_decrypt(&ctx, CRYPTO_ALG_AES_128_CCM, key, sizeof(key),
                                        iv, sizeof(iv), ct, ct_len, dec, &dec_len) == 0 &&
                dec_len == sizeof(pt) - 1 && buffers_equal(pt, dec, dec_len)) {
                record_pass(&res, "aes_128_ccm_roundtrip");
            } else {
                record_fail(&res, "aes_128_ccm_roundtrip", "decrypt mismatch");
            }
        } else {
            record_fail(&res, "aes_128_ccm_roundtrip", "encrypt failed");
        }
    }

    /* Stream/other modes */
    {
        const uint8_t key[32] = {0};
        const uint8_t iv[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
        const uint8_t pt[] = "chacha20-poly1305";
        uint8_t ct[128] = {0};
        size_t ct_len = sizeof(ct);
        if (dsmil_device255_encrypt(&ctx, CRYPTO_ALG_CHACHA20_POLY1305, key, sizeof(key),
                                    iv, sizeof(iv), pt, sizeof(pt) - 1, ct, &ct_len) == 0) {
            uint8_t dec[128] = {0};
            size_t dec_len = sizeof(dec);
            if (dsmil_device255_decrypt(&ctx, CRYPTO_ALG_CHACHA20_POLY1305, key, sizeof(key),
                                        iv, sizeof(iv), ct, ct_len, dec, &dec_len) == 0 &&
                dec_len == sizeof(pt) - 1 && buffers_equal(pt, dec, dec_len)) {
                record_pass(&res, "chacha20_poly1305_roundtrip");
            } else {
                record_fail(&res, "chacha20_poly1305_roundtrip", "decrypt mismatch");
            }
        } else {
            record_fail(&res, "chacha20_poly1305_roundtrip", "encrypt failed");
        }
    }
    {
        const uint8_t key[32] = {0};
        const uint8_t iv[16] = {0};
        const uint8_t pt[] = "aes-cbc-padding-check";
        uint8_t ct[128] = {0};
        size_t ct_len = sizeof(ct);
        if (dsmil_device255_encrypt(&ctx, CRYPTO_ALG_AES_256_CBC, key, sizeof(key),
                                    iv, sizeof(iv), pt, sizeof(pt) - 1, ct, &ct_len) == 0) {
            uint8_t dec[128] = {0};
            size_t dec_len = sizeof(dec);
            if (dsmil_device255_decrypt(&ctx, CRYPTO_ALG_AES_256_CBC, key, sizeof(key),
                                        iv, sizeof(iv), ct, ct_len, dec, &dec_len) == 0 &&
                dec_len == sizeof(pt) - 1 && buffers_equal(pt, dec, dec_len)) {
                record_pass(&res, "aes_256_cbc_roundtrip");
            } else {
                record_fail(&res, "aes_256_cbc_roundtrip", "decrypt mismatch");
            }
        } else {
            record_fail(&res, "aes_256_cbc_roundtrip", "encrypt failed");
        }
    }

    /* TPM-path tests (skip if TPM unavailable) */
    dsmil_device255_caps_t caps = {0};
    dsmil_device255_get_caps(&ctx, &caps);
    bool tpm_ok = caps.tpm_available;
    if (tpm_ok) {
        dsmil_device255_set_engine(&ctx, DSMIL_CRYPTO_ENGINE_TPM);
        const uint8_t msg[] = "tpm-hash";
        uint8_t out[64] = {0};
        size_t out_len = sizeof(out);
        int rc = dsmil_device255_hash(&ctx, TPM_ALG_SHA1, msg, sizeof(msg) - 1, out, &out_len);
        if (rc == 0 && out_len == 20) {
            record_pass(&res, "tpm_hash_sha1");
        } else {
            record_fail(&res, "tpm_hash_sha1", "TPM hash failed");
        }

        const uint8_t key[32] = {1};
        const uint8_t iv[16] = {2};
        const uint8_t pt[] = "tpm-aes-cfb";
        uint8_t ct[128] = {0};
        size_t ct_len = sizeof(ct);
        rc = dsmil_device255_encrypt(&ctx, CRYPTO_ALG_AES_256_CFB, key, sizeof(key),
                                     iv, sizeof(iv), pt, sizeof(pt) - 1, ct, &ct_len);
        if (rc == 0) {
            uint8_t dec[128] = {0};
            size_t dec_len = sizeof(dec);
            if (dsmil_device255_decrypt(&ctx, CRYPTO_ALG_AES_256_CFB, key, sizeof(key),
                                        iv, sizeof(iv), ct, ct_len, dec, &dec_len) == 0 &&
                dec_len == sizeof(pt) - 1 && buffers_equal(pt, dec, dec_len)) {
                record_pass(&res, "tpm_aes_256_cfb_roundtrip");
            } else {
                record_fail(&res, "tpm_aes_256_cfb_roundtrip", "decrypt mismatch");
            }
        } else {
            record_fail(&res, "tpm_aes_256_cfb_roundtrip", "encrypt failed");
        }
    } else {
        record_skip(&res, "tpm_hash_sha1", "TPM not available");
        record_skip(&res, "tpm_aes_256_cfb_roundtrip", "TPM not available");
    }

    /* PQC tests */
    dsmil_device255_set_engine(&ctx, DSMIL_CRYPTO_ENGINE_SOFTWARE);
    if (dsmil_device255_pqc_available(&ctx, CRYPTO_ALG_KYBER1024) &&
        dsmil_device255_pqc_available(&ctx, CRYPTO_ALG_DILITHIUM3)) {
        record_pass(&res, "pqc_capability_kyber_dilithium");
    } else {
        record_fail(&res, "pqc_capability_kyber_dilithium", "capability flags missing");
    }

    {
        OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
        if (!sig) {
            record_fail(&res, "pqc_ml_dsa_sign_verify", "sig init failed");
        } else {
            uint8_t *pk = malloc(sig->length_public_key);
            uint8_t *sk = malloc(sig->length_secret_key);
            if (!pk || !sk) {
                record_fail(&res, "pqc_ml_dsa_sign_verify", "key alloc failed");
            } else if (OQS_SIG_keypair(sig, pk, sk) != OQS_SUCCESS) {
                record_fail(&res, "pqc_ml_dsa_sign_verify", "keygen failed");
            } else {
                const uint8_t msg[] = "pqc-ml-dsa-message";
                size_t sig_buf_len = sig->length_signature;
                uint8_t *sig_buf = malloc(sig_buf_len);
                if (!sig_buf) {
                    record_fail(&res, "pqc_ml_dsa_sign_verify", "sig alloc failed");
                } else {
                    size_t sig_len = sig_buf_len;
                    if (dsmil_device255_sign(&ctx, TPM_ALG_ML_DSA_87, sk, sig->length_secret_key,
                                             msg, sizeof(msg) - 1, sig_buf, &sig_len) == 0) {
                        if (dsmil_device255_verify(&ctx, TPM_ALG_ML_DSA_87, pk, sig->length_public_key,
                                                   msg, sizeof(msg) - 1, sig_buf, sig_len) == 0) {
                            record_pass(&res, "pqc_ml_dsa_sign_verify");
                        } else {
                            record_fail(&res, "pqc_ml_dsa_sign_verify", "verify failed");
                        }
                    } else {
                        record_fail(&res, "pqc_ml_dsa_sign_verify", "sign failed");
                    }
                    free(sig_buf);
                }
            }
            free(pk);
            free(sk);
            OQS_SIG_free(sig);
        }
    }

    printf("\nSummary: %d passed, %d failed, %d skipped\n", res.passed, res.failed, res.skipped);
    return res.failed == 0 ? 0 : 1;
}

int main(void) {
    return run_tests();
}

