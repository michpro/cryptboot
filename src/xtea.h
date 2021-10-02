/**
 * \file xtea.h
 * \brief XTEA cipher library, size-optimized.
 *
 * \copyright SPDX-FileCopyrightText: Copyright 2020-2021 Michal Protasowicki
 *
 * \license SPDX-License-Identifier: MIT
 *
 */

#ifndef XTEA_H_
#define XTEA_H_

#include <stdbool.h>

#ifdef ARDUINO
#define __null ((void *)0)
#endif

#ifdef __cplusplus
    #ifdef bool
        #undef bool
    #endif
    #define bool        boolean
    #ifndef true
        #define true	1
    #endif
    #ifndef false
        #define false   0
    #endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define XTEA_BLOCK_SIZE     8
#define XTEA_IV_SIZE        XTEA_BLOCK_SIZE
#define XTEA_KEY_SIZE       16

// actual number of Feistel rounds during cipher is 2 * XTEA_ROUNDS
// known attack is on 36 Feistel rounds (18 XTEA_ROUNDS)
#ifndef XTEA_ROUNDS
#define XTEA_ROUNDS         32
#endif

// actual number of Feistel rounds during MAC calculation is 2 * XTEA_MAC_ROUNDS
// known attack is on 36 Feistel rounds (18 XTEA_MAC_ROUNDS)
#ifndef XTEA_MAC_ROUNDS
#define XTEA_MAC_ROUNDS     32
#endif

/**
 *  \brief Cipher operation type.
 */
typedef enum xteaOperation
{
    xteaEncrypt = 0x00,
    xteaDecrypt = 0x01
} xteaOperation_t;

/**
 * \brief Auxiliary type to easy convert between uint32_t and uint8_t[]
 */
typedef union u32_u8_union
{
    uint32_t            u32;
    uint8_t             u8[sizeof(uint32_t)];
} u32_u8_union_t;

/**
 * \brief XTEA ECB context.
 */
typedef struct xteaEcbCtx
{
    /// 128-bit XTEA cipher key.
    u32_u8_union_t      key[XTEA_KEY_SIZE / sizeof(uint32_t)];
    /// Number of internal rounds of cipher operation.
    uint_fast8_t        rounds;
    /// Type of operation to be performed by cipher.
    xteaOperation_t     operation;
} xteaEcbCtx_t;

/**
 * \brief XTEA cipher context.
 */
typedef struct xteaCipherCtx
{
    /// XTEA ECB context.
    xteaEcbCtx_t        base;
    /// 64-bit random initialization vector (IV), a.k.a. nonce - number used once.
    uint8_t             iv[XTEA_IV_SIZE];
} xteaCipherCtx_t;

/**
 * \brief XTEA (MAC) context.
 */
typedef struct xteaCtx
{
    /// XTEA cipher context.
    xteaCipherCtx_t     cipher;
    /// Key used when completing MAC code computation.
    u32_u8_union_t      secondKey[XTEA_KEY_SIZE / sizeof(uint32_t)];
    /// buffer for auxiliary data / or computed MAC code.
    uint8_t             data[XTEA_BLOCK_SIZE];
    /// amount of data in the variable 'data'
    uint_fast8_t        dataLength;
} xteaCtx_t;

#ifdef __cplusplus
extern "C"
{
#endif

void *xteaEcbEncrypt    (const u32_u8_union_t key[XTEA_KEY_SIZE / sizeof(uint32_t)],
                         const uint8_t input[XTEA_BLOCK_SIZE],
                         uint8_t output[XTEA_BLOCK_SIZE],
                         uint_fast8_t rounds);
void *xteaEcbDecrypt    (const u32_u8_union_t key[XTEA_KEY_SIZE / sizeof(uint32_t)],
                         const uint8_t input[XTEA_BLOCK_SIZE],
                         uint8_t output[XTEA_BLOCK_SIZE],
                         uint_fast8_t rounds);
void xteaSetOperation   (xteaEcbCtx_t *ctx, const xteaOperation_t operation);
void xteaSetKey         (xteaEcbCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE]);
void xteaSetIv          (xteaCipherCtx_t *ctx, const uint8_t iv[XTEA_IV_SIZE]);
void xteaInitEcb        (xteaEcbCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint_fast8_t rounds);
void xteaInit           (xteaCipherCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint8_t iv[XTEA_IV_SIZE], const uint_fast8_t rounds);
void xteaEcbBlock       (xteaEcbCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE]);
void xteaCfbBlock       (xteaCipherCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE]);
void xteaOfbBlock       (xteaCipherCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE]);
void xteaCfbMacInit     (xteaCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint_fast8_t rounds);
void xteaCfbMacUpdate   (xteaCtx_t *ctx, const uint8_t data[], const uint32_t length);
void xteaCfbMacFinish   (xteaCtx_t *ctx);
void xteaCfbMacGet      (xteaCtx_t *ctx, uint8_t mac[XTEA_BLOCK_SIZE]);
bool xteaCfbMacCmp      (xteaCtx_t *ctx, const uint8_t mac[XTEA_BLOCK_SIZE]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // XTEA_H_
