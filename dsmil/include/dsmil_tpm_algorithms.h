/**
 * @file dsmil_tpm_algorithms.h
 * @brief Shared TPM 2.0 algorithm identifiers for Device 255 (88 total)
 *
 * These values mirror the kernel-side `dsmil_tpm_algorithms.h` definitions so
 * user space and toolchains stay aligned. The categories add up to 88:
 *  - Hash: 10
 *  - Symmetric: 22
 *  - Asymmetric: 5
 *  - ECC: 12
 *  - KDF: 11
 *  - HMAC: 5
 *  - Signatures: 8
 *  - Key agreement: 3
 *  - Mask generation: 4
 *  - Post-quantum: 8
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_TPM_ALGORITHMS_H
#define DSMIL_TPM_ALGORITHMS_H

#include <stdint.h>

/* TPM 2.0 base algorithms (hash) */
#ifndef TPM_ALG_NULL
#define TPM_ALG_NULL           0x0010
#endif
#ifndef TPM_ALG_SHA1
#define TPM_ALG_SHA1           0x0004
#endif
#ifndef TPM_ALG_SHA256
#define TPM_ALG_SHA256         0x000B
#endif
#ifndef TPM_ALG_SHA384
#define TPM_ALG_SHA384         0x000C
#endif
#ifndef TPM_ALG_SHA512
#define TPM_ALG_SHA512         0x000D
#endif
#ifndef TPM_ALG_SM3_256
#define TPM_ALG_SM3_256        0x0012
#endif
#ifndef TPM_ALG_SHA3_256
#define TPM_ALG_SHA3_256       0x0027
#endif
#ifndef TPM_ALG_SHA3_384
#define TPM_ALG_SHA3_384       0x0028
#endif
#ifndef TPM_ALG_SHA3_512
#define TPM_ALG_SHA3_512       0x0029
#endif
#define TPM_ALG_SHAKE128       0x002A
#define TPM_ALG_SHAKE256       0x002B

/* Symmetric AES modes (extended IDs for runtime selection) */
#define CRYPTO_ALG_AES_128_ECB  0x1000
#define CRYPTO_ALG_AES_256_ECB  0x1001
#define CRYPTO_ALG_AES_128_CBC  0x1002
#define CRYPTO_ALG_AES_256_CBC  0x1003
#define CRYPTO_ALG_AES_128_CTR  0x1004
#define CRYPTO_ALG_AES_256_CTR  0x1005
#define CRYPTO_ALG_AES_128_OFB  0x1006
#define CRYPTO_ALG_AES_256_OFB  0x1007
#define CRYPTO_ALG_AES_128_CFB  0x1008
#define CRYPTO_ALG_AES_256_CFB  0x1009
#define CRYPTO_ALG_AES_128_GCM  0x100A
#define CRYPTO_ALG_AES_256_GCM  0x100B
#define CRYPTO_ALG_AES_128_CCM  0x100C
#define CRYPTO_ALG_AES_256_CCM  0x100D
#define CRYPTO_ALG_AES_128_XTS  0x100E
#define CRYPTO_ALG_AES_256_XTS  0x100F

/* Other symmetric ciphers */
#define CRYPTO_ALG_3DES_EDE     0x1010
#define CRYPTO_ALG_CAMELLIA_128 0x1011
#define CRYPTO_ALG_CAMELLIA_256 0x1012
#define CRYPTO_ALG_SM4_128      0x1013
#define CRYPTO_ALG_CHACHA20     0x1014
#define CRYPTO_ALG_CHACHA20_POLY1305 0x1015

/* Standard TPM symmetric algorithm IDs */
#ifndef TPM_ALG_AES
#define TPM_ALG_AES            0x0006
#endif
#ifndef TPM_ALG_CAMELLIA
#define TPM_ALG_CAMELLIA       0x0026
#endif
#ifndef TPM_ALG_SM4
#define TPM_ALG_SM4            0x0013
#endif
#ifndef TPM_ALG_XOR
#define TPM_ALG_XOR            0x000A
#endif
#ifndef TPM_ALG_TDES
#define TPM_ALG_TDES           0x0003
#endif

/* RSA key size variants */
#define CRYPTO_ALG_RSA_1024     0x2000
#define CRYPTO_ALG_RSA_2048     0x2001
#define CRYPTO_ALG_RSA_3072     0x2002
#define CRYPTO_ALG_RSA_4096     0x2003
#define CRYPTO_ALG_RSA_8192     0x2004

/* Base asymmetric IDs */
#ifndef TPM_ALG_RSA
#define TPM_ALG_RSA            0x0001
#endif
#ifndef TPM_ALG_ECC
#define TPM_ALG_ECC            0x0023
#endif
#ifndef TPM_ALG_SM2
#define TPM_ALG_SM2            0x001B
#endif

/* ECC curves */
#define CRYPTO_ALG_ECC_P192       0x3000
#define CRYPTO_ALG_ECC_P224       0x3001
#define CRYPTO_ALG_ECC_P256       0x3002
#define CRYPTO_ALG_ECC_P384       0x3003
#define CRYPTO_ALG_ECC_P521       0x3004
#define CRYPTO_ALG_ECC_SM2_P256   0x3005
#define CRYPTO_ALG_ECC_BN_P256    0x3006
#define CRYPTO_ALG_ECC_BN_P638    0x3007
#define CRYPTO_ALG_ECC_CURVE25519 0x3008
#define CRYPTO_ALG_ECC_CURVE448   0x3009
#define CRYPTO_ALG_ECC_ED25519    0x300A
#define CRYPTO_ALG_ECC_ED448      0x300B

/* ECC schemes */
#ifndef TPM_ALG_ECDSA
#define TPM_ALG_ECDSA          0x0018
#endif
#ifndef TPM_ALG_ECDH
#define TPM_ALG_ECDH           0x0019
#endif
#ifndef TPM_ALG_ECDAA
#define TPM_ALG_ECDAA          0x001A
#endif
#ifndef TPM_ALG_ECSCHNORR
#define TPM_ALG_ECSCHNORR      0x001C
#endif
#ifndef TPM_ALG_ECMQV
#define TPM_ALG_ECMQV          0x001D
#endif

/* HMAC */
#define CRYPTO_ALG_HMAC_SHA1    0x4000
#define CRYPTO_ALG_HMAC_SHA256  0x4001
#define CRYPTO_ALG_HMAC_SHA384  0x4002
#define CRYPTO_ALG_HMAC_SHA512  0x4003
#define CRYPTO_ALG_HMAC_SM3     0x4004

#ifndef TPM_ALG_HMAC
#define TPM_ALG_HMAC           0x0005
#endif
#ifndef TPM_ALG_KEYEDHASH
#define TPM_ALG_KEYEDHASH      0x0008
#endif

/* Key derivation */
#define CRYPTO_ALG_KDF_SP800_108 0x5000
#define CRYPTO_ALG_KDF_SP800_56A 0x5001
#define CRYPTO_ALG_HKDF_SHA256   0x5002
#define CRYPTO_ALG_HKDF_SHA384   0x5003
#define CRYPTO_ALG_HKDF_SHA512   0x5004
#define CRYPTO_ALG_PBKDF2_SHA256 0x5005
#define CRYPTO_ALG_PBKDF2_SHA512 0x5006
#define CRYPTO_ALG_SCRYPT        0x5007
#define CRYPTO_ALG_ARGON2I       0x5008
#define CRYPTO_ALG_ARGON2D       0x5009
#define CRYPTO_ALG_ARGON2ID      0x500A

#ifndef TPM_ALG_KDF1_SP800_56A
#define TPM_ALG_KDF1_SP800_56A 0x0020
#endif
#ifndef TPM_ALG_KDF2
#define TPM_ALG_KDF2           0x0021
#endif
#ifndef TPM_ALG_KDF1_SP800_108
#define TPM_ALG_KDF1_SP800_108 0x0022
#endif

/* Signature schemes */
#define CRYPTO_ALG_RSA_SSA_PKCS1V15 0x6000
#define CRYPTO_ALG_RSA_PSS      0x6001
#define CRYPTO_ALG_ECDSA_SHA256 0x6002
#define CRYPTO_ALG_ECDSA_SHA384 0x6003
#define CRYPTO_ALG_ECDSA_SHA512 0x6004
#define CRYPTO_ALG_SCHNORR      0x6005
#define CRYPTO_ALG_SM2_SIGN     0x6006
#define CRYPTO_ALG_ECDAA_SIG    0x6007

#ifndef TPM_ALG_RSASSA
#define TPM_ALG_RSASSA         0x0014
#endif
#ifndef TPM_ALG_RSAPSS
#define TPM_ALG_RSAPSS         0x0016
#endif
#ifndef TPM_ALG_RSAES
#define TPM_ALG_RSAES          0x0015
#endif
#ifndef TPM_ALG_OAEP
#define TPM_ALG_OAEP           0x0017
#endif

/* Key agreement */
#define CRYPTO_ALG_ECDH_KA      0x7000
#define CRYPTO_ALG_ECMQV_KA     0x7001
#define CRYPTO_ALG_DH           0x7002

/* Mask generation */
#define CRYPTO_ALG_MGF1_SHA1    0x8000
#define CRYPTO_ALG_MGF1_SHA256  0x8001
#define CRYPTO_ALG_MGF1_SHA384  0x8002
#define CRYPTO_ALG_MGF1_SHA512  0x8003

#ifndef TPM_ALG_MGF1
#define TPM_ALG_MGF1           0x0007
#endif

/* Post-quantum */
#define CRYPTO_ALG_KYBER512     0x9000
#define CRYPTO_ALG_KYBER768     0x9001
#define CRYPTO_ALG_KYBER1024    0x9002
#define CRYPTO_ALG_DILITHIUM2   0x9003
#define CRYPTO_ALG_DILITHIUM3   0x9004
#define CRYPTO_ALG_DILITHIUM5   0x9005
#define CRYPTO_ALG_FALCON512    0x9006
#define CRYPTO_ALG_FALCON1024   0x9007

/* Encryption modes */
#ifndef TPM_ALG_CTR
#define TPM_ALG_CTR            0x0040
#endif
#ifndef TPM_ALG_OFB
#define TPM_ALG_OFB            0x0041
#endif
#ifndef TPM_ALG_CBC
#define TPM_ALG_CBC            0x0042
#endif
#ifndef TPM_ALG_CFB
#define TPM_ALG_CFB            0x0043
#endif
#ifndef TPM_ALG_ECB
#define TPM_ALG_ECB            0x0044
#endif

/* Compatibility aliases for legacy names used in the runtime */
#ifndef TPM_ALG_AES_128_GCM
#define TPM_ALG_AES_128_GCM CRYPTO_ALG_AES_128_GCM
#endif
#ifndef TPM_ALG_AES_256_GCM
#define TPM_ALG_AES_256_GCM CRYPTO_ALG_AES_256_GCM
#endif
#ifndef TPM_ALG_AES_256_CBC
#define TPM_ALG_AES_256_CBC CRYPTO_ALG_AES_256_CBC
#endif
#ifndef TPM_ALG_AES_128_CBC
#define TPM_ALG_AES_128_CBC CRYPTO_ALG_AES_128_CBC
#endif
#ifndef TPM_ALG_AES_256_CFB
#define TPM_ALG_AES_256_CFB CRYPTO_ALG_AES_256_CFB
#endif
#ifndef TPM_ALG_AES_128_CFB
#define TPM_ALG_AES_128_CFB CRYPTO_ALG_AES_128_CFB
#endif

#ifndef TPM_ALG_ML_KEM_1024
#define TPM_ALG_ML_KEM_1024 CRYPTO_ALG_KYBER1024
#endif
#ifndef TPM_ALG_ML_DSA_87
#define TPM_ALG_ML_DSA_87   CRYPTO_ALG_DILITHIUM3
#endif

#endif /* DSMIL_TPM_ALGORITHMS_H */

