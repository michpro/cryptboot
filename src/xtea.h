/**
 * \file xtea.h
 * \brief XTEA cipher library, size-optimized.
 *
 * \copyright SPDX-FileCopyrightText: Copyright 2020-2021 by Michal Protasowicki
 *
 * \license SPDX-License-Identifier: MIT
 *
 */

#ifndef XTEA_H_
#define XTEA_H_

#include <stdbool.h>

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

static void *xteaEcbEncrypt    (const u32_u8_union_t key[XTEA_KEY_SIZE / sizeof(uint32_t)],
                                const uint8_t input[XTEA_BLOCK_SIZE],
                                uint8_t output[XTEA_BLOCK_SIZE],
                                uint_fast8_t rounds);
static void xteaSetKey         (xteaEcbCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE]);
static void xteaSetIv          (xteaCipherCtx_t *ctx, const uint8_t iv[XTEA_IV_SIZE]);
static void xteaCfbBlock       (xteaCipherCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE]);
static void xteaCfbMacInit     (xteaCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint_fast8_t rounds);
static void xteaCfbMacUpdate   (xteaCtx_t *ctx, const uint8_t data[], const uint32_t length);
static void xteaCfbMacFinish   (xteaCtx_t *ctx);
static bool xteaCfbMacCmp      (xteaCtx_t *ctx, const uint8_t mac[XTEA_BLOCK_SIZE]);

/**
 * \brief Base function to encrypt a data block in the ECB mode.
 *
 * \param[in]   key     128-bit [16 bytes] XTEA cipher key.
 * \param[in]   input   64-bit [8 bytes] data block to encrypt.
 * \param[out]  output  64-bit [8 bytes] block of encrypted data.
 * \param[in]   rounds  Number of internal rounds of encryption.
 *
 * \return Processed data is returned by the 'output' parameter.
 */
static void *xteaEcbEncrypt(const u32_u8_union_t key[XTEA_KEY_SIZE / sizeof(uint32_t)],
                     const uint8_t input[XTEA_BLOCK_SIZE],
                     uint8_t output[XTEA_BLOCK_SIZE],
                     uint_fast8_t rounds)
{
    const uint32_t          delta   = 0x9E3779B9;
    uint32_t                sum     = 0x00000000;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    u32_u8_union_t          v0      = { .u8 = {input[3], input[2], input[1], input[0]} };
    u32_u8_union_t          v1      = { .u8 = {input[7], input[6], input[5], input[4]} };
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    u32_u8_union_t          v0      = { .u8 = {input[0], input[1], input[2], input[3]} };
    u32_u8_union_t          v1      = { .u8 = {input[4], input[5], input[6], input[7]} };
#else
    #error "Unsupported hardware !!!"
#endif

    while(rounds--)
    {
        v0.u32  += ((((v1.u32 << 4) & 0xFFFFFFF0) ^ ((v1.u32 >> 5) & 0x07FFFFFF)) + v1.u32) ^ (sum + key[sum & 3].u32);
        sum     += delta;
        v1.u32  += ((((v0.u32 << 4) & 0xFFFFFFF0) ^ ((v0.u32 >> 5) & 0x07FFFFFF)) + v0.u32) ^ (sum + key[((sum >> 11) & 0x001FFFFF) & 3].u32);
    }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    output[0] = v0.u8[3];
    output[1] = v0.u8[2];
    output[2] = v0.u8[1];
    output[3] = v0.u8[0];
    output[4] = v1.u8[3];
    output[5] = v1.u8[2];
    output[6] = v1.u8[1];
    output[7] = v1.u8[0];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    output[0] = v0.u8[0];
    output[1] = v0.u8[1];
    output[2] = v0.u8[2];
    output[3] = v0.u8[3];
    output[4] = v1.u8[0];
    output[5] = v1.u8[1];
    output[6] = v1.u8[2];
    output[7] = v1.u8[3];
#else
    #error "Unsupported hardware !!!"
#endif

    return output;
}

/**
 * \brief A function that loads a key into an XTEA ECB context.
 *
 * \param[in]   ctx     XTEA ECB context.
 * \param[in]   key     128-bit [16 bytes] XTEA cipher key.
 *
 * \return nothing
 */
static void xteaSetKey(xteaEcbCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE])
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    register uint_fast8_t idx = (uint_fast8_t)(XTEA_KEY_SIZE / sizeof(uint32_t));
    register uint_fast8_t shift = (idx - 1) * sizeof(uint32_t);
    while (idx--)
    {
        *((uint8_t *)(ctx->key + idx) + 3) = *((uint8_t *)key + shift + 0);
        *((uint8_t *)(ctx->key + idx) + 2) = *((uint8_t *)key + shift + 1);
        *((uint8_t *)(ctx->key + idx) + 1) = *((uint8_t *)key + shift + 2);
        *((uint8_t *)(ctx->key + idx) + 0) = *((uint8_t *)key + shift + 3);
        shift -= sizeof(uint32_t);
    }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    memcpy((uint8_t *)ctx->key, (uint8_t *)key, XTEA_KEY_SIZE);
#else
    #error "Unsupported hardware !!!"
#endif
}

/**
 * \brief A function that loads an initialization vector into an XTEA cipher context.
 *
 * \param[in]   ctx     XTEA cipher context.
 * \param[in]   iv      64-bit [8 bytes] random initialization vector, a.k.a. nonce - number used once.
 *
 * \return nothing
 */
static inline void xteaSetIv(xteaCipherCtx_t *ctx, const uint8_t iv[XTEA_IV_SIZE])
{
    if (ctx == NULL)
    {
        return;
    }

    memcpy(ctx->iv, iv, XTEA_IV_SIZE);
}

/**
 * \brief Function that encrypts/decrypts data block in CFB mode.
 *
 * \param[in]       ctx     XTEA cipher context.
 * \param[in,out]   data    Block of 64-bit [8 bytes] data processed by the function.
 *
 * \return Processed data is returned by the 'data' parameter.
 */
static void xteaCfbBlock(xteaCipherCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE])
{
    if (ctx == NULL)
    {
        return;
    }

    register uint_fast8_t idx   = XTEA_BLOCK_SIZE;
    register uint_fast8_t vTmp;

    xteaEcbEncrypt(ctx->base.key, ctx->iv, ctx->iv, ctx->base.rounds);

    while (idx--)
    {
        vTmp = data[idx];
        data[idx] ^= ctx->iv[idx];
        ctx->iv[idx] = (xteaEncrypt == ctx->base.operation) ? data[idx] : vTmp;
    }
}

// ----------------------------------------------------------------
// |                         XTEA CFB-MAC                         |
// ----------------------------------------------------------------

/**
 * \brief   A function that initializes the XTEA context by the passed data,
 *          for MAC code calculation operation (MAC - message authentication code).
 *          Two dependent keys (with large Hamming distance) are
 *          internally generated from passed key, which allows the same key
 *          to be used for both encryption and MAC code calculation operations.
 *
 * \param[in]   ctx     XTEA context.
 * \param[in]   key     128-bit [16 bytes] XTEA key.
 * \param[in]   rounds  Number of internal rounds of cipher operation.
 *
 * \return nothing
 */
static void xteaCfbMacInit(xteaCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint_fast8_t rounds)
{
    if (ctx == NULL)
    {
        return;
    }

    uint_fast8_t    idx = XTEA_KEY_SIZE;

    xteaSetKey(&(ctx->cipher.base), key);
    ctx->cipher.base.rounds = rounds;
    ctx->cipher.base.operation = xteaEncrypt;
    ctx->dataLength = 0x00;

    uint8_t * firstKeyPtr   = (uint8_t *)&(ctx->cipher.base.key);
    uint8_t * secondKeyPtr  = (uint8_t *)&(ctx->secondKey);

    while(idx--)
    {
        *(secondKeyPtr + idx) = *(firstKeyPtr + idx) ^ 0x5C;    // opad
        *(firstKeyPtr + idx) ^= 0x36;                           // ipad
        if(idx < XTEA_BLOCK_SIZE)
        {
            ctx->cipher.iv[idx] = 0x00;
        }
    }
}

/**
 * \brief Add data to an initialized MAC calculation.
 *
 * \param[in]   ctx     XTEA context.
 * \param[in]   data    Data to be added.
 * \param[in]   length  Size of the data to be added in bytes.
 *
 * \return nothing
 */
static void xteaCfbMacUpdate(xteaCtx_t *ctx, const uint8_t data[], const uint32_t length)
{
    if (ctx == NULL)
    {
        return;
    }

    for (uint32_t idx = 0; idx < length; idx++) {
        ctx->data[ctx->dataLength++] = data[idx];
        if (XTEA_BLOCK_SIZE == ctx->dataLength)
        {
            xteaCfbBlock(&(ctx->cipher), ctx->data);
            ctx->dataLength = 0x00;
        }
    }
}

/**
 * \brief Finish a MAC operation returning the MAC value.
 *
 * \param[in]  ctx XTEA context.
 *
 * \return Computed MAC code is in 'data' field of XTEA context.
 */
static void xteaCfbMacFinish(xteaCtx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    register uint_fast8_t idx = ctx->dataLength;

    // Pad whatever data is left in the buffer.
    ctx->data[idx++] = 0x80;
    while (idx < XTEA_BLOCK_SIZE)
    {
        ctx->data[idx++] = 0x00;
    }
    xteaCfbBlock(&(ctx->cipher), ctx->data);

    memcpy(&(ctx->cipher.base.key), &(ctx->secondKey), XTEA_KEY_SIZE);
    xteaCfbBlock(&(ctx->cipher), ctx->data);
}

/**
 * \brief   A function that compares the indicated MAC code with
 *          code computed previously from passed data to check if they match.
 *
 * \param[in]   ctx XTEA context.
 * \param[in]   mac MAC code to compare with code stored in XTEA context.
 *
 * \return true - if MAC codes match, otherwise returns false.
 */
static inline bool xteaCfbMacCmp(xteaCtx_t *ctx, const uint8_t mac[XTEA_BLOCK_SIZE])
{
    if (ctx == NULL)
    {
        return false;
    }

    return (0 == memcmp(mac, ctx->data, XTEA_BLOCK_SIZE));
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // XTEA_H_
