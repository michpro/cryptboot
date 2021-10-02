/**
 * \file xtea.c
 * \brief XTEA cipher library, size-optimized.
 *
 * \copyright SPDX-FileCopyrightText: Copyright 2020-2021 Michal Protasowicki
 *
 * \license SPDX-License-Identifier: MIT
 *
 */

#include "xtea.h"

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
void *xteaEcbEncrypt(const u32_u8_union_t key[XTEA_KEY_SIZE / sizeof(uint32_t)],
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
        v0.u32  += (((v1.u32 << 4) ^ (v1.u32 >> 5)) + v1.u32) ^ (sum + key[sum & 3].u32);
        sum     += delta;
        v1.u32  += (((v0.u32 << 4) ^ (v0.u32 >> 5)) + v0.u32) ^ (sum + key[(sum >> 11) & 3].u32);
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
 * \brief Base function to decrypt a data block in the ECB mode.
 *
 * \param[in]   key     128-bit [16 bytes] XTEA cipher key.
 * \param[in]   input   64-bit [8 bytes] data block to decrypt.
 * \param[out]  output  64-bit [8 bytes] block of decrypted data.
 * \param[in]   rounds  Number of internal rounds of decryption.
 *
 * \return Processed data is returned by the 'output' parameter.
 */
void *xteaEcbDecrypt(const u32_u8_union_t key[XTEA_KEY_SIZE / sizeof(uint32_t)],
                     const uint8_t input[XTEA_BLOCK_SIZE],
                     uint8_t output[XTEA_BLOCK_SIZE],
                     uint_fast8_t rounds)
{
    const uint32_t          delta   = 0x9E3779B9;
    uint32_t                sum     = delta * rounds;

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
        v1.u32  -= (((v0.u32 << 4) ^ (v0.u32 >> 5)) + v0.u32) ^ (sum + key[(sum >> 11) & 3].u32);
        sum     -= delta;
        v0.u32  -= (((v1.u32 << 4) ^ (v1.u32 >> 5)) + v1.u32) ^ (sum + key[sum & 3].u32);
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
 * \brief A function that sets type of cipher operation.
 *
 * \param[in]   ctx         XTEA ECB cipher context.
 * \param[in]   operation   Type of operation to be performed.
 *
 * \return nothing
 */
inline void xteaSetOperation(xteaEcbCtx_t *ctx, const xteaOperation_t operation)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->operation = operation;
}

/**
 * \brief A function that loads a key into an XTEA ECB context.
 *
 * \param[in]   ctx     XTEA ECB context.
 * \param[in]   key     128-bit [16 bytes] XTEA cipher key.
 *
 * \return nothing
 */
void xteaSetKey(xteaEcbCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE])
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
inline void xteaSetIv(xteaCipherCtx_t *ctx, const uint8_t iv[XTEA_IV_SIZE])
{
    if (ctx == NULL)
    {
        return;
    }

    memcpy(ctx->iv, iv, XTEA_IV_SIZE);
}

/**
 * \brief   A function that initializes the XTEA ECB context by the passed data.
 *          The encryption operation is set by default.
 *
 * \param[in]   ctx     XTEA ECB context.
 * \param[in]   key     128-bit [16 bytes] XTEA cipher key.
 * \param[in]   rounds  Number of internal rounds of cipher operation.
 *
 * \return nothing
 */
inline void xteaInitEcb(xteaEcbCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint_fast8_t rounds)
{
    if (ctx == NULL)
    {
        return;
    }

    xteaSetKey(ctx, key);
    ctx->rounds = rounds;
    ctx->operation = xteaEncrypt;
}

/**
 * \brief   A function that initializes the XTEA cipher context by the passed data.
 *          The encryption operation is set by default.
 *
 * \param[in]   ctx     XTEA cipher context.
 * \param[in]   key     128-bit [16 bytes] XTEA cipher key.
 * \param[in]   iv      64-bit [8 bytes] random initialization vector, a.k.a. nonce - number used once.
 * \param[in]   rounds  Number of internal rounds of cipher operation.
 *
 * \return nothing
 */
void xteaInit(xteaCipherCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint8_t iv[XTEA_IV_SIZE], const uint_fast8_t rounds)
{
    xteaInitEcb(&(ctx->base), key, rounds);
    xteaSetIv(ctx, iv);
}

/**
 * \brief Function that encrypts/decrypts data block in ECB mode.
 *
 * \param[in]       ctx     XTEA cipher context.
 * \param[in,out]   data    Block of 64-bit [8 bytes] data processed by the function.
 *
 * \return Processed data is returned by the 'data' parameter.
 */
void xteaEcbBlock(xteaEcbCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE])
{
    if (ctx == NULL)
    {
        return;
    }

    if (xteaEncrypt == ctx->operation)
    {
        xteaEcbEncrypt(ctx->key, data, data, ctx->rounds);
    } else
    {
        xteaEcbDecrypt(ctx->key, data, data, ctx->rounds);
    }
}

/**
 * \brief Function that encrypts/decrypts data block in CFB mode.
 *
 * \param[in]       ctx     XTEA cipher context.
 * \param[in,out]   data    Block of 64-bit [8 bytes] data processed by the function.
 *
 * \return Processed data is returned by the 'data' parameter.
 */
void xteaCfbBlock(xteaCipherCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE])
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

/**
 * \brief Function that encrypts/decrypts data block in OFB mode.
 *
 * \param[in]       ctx     XTEA cipher context.
 * \param[in,out]   data    Block of 64-bit [8 bytes] data processed by the function.
 *
 * \return Processed data is returned by the 'data' parameter.
 */
void xteaOfbBlock(xteaCipherCtx_t *ctx, uint8_t data[XTEA_BLOCK_SIZE])
{
    if (ctx == NULL)
    {
        return;
    }

    register uint_fast8_t idx = XTEA_BLOCK_SIZE;

    xteaEcbEncrypt(ctx->base.key, ctx->iv, ctx->iv, ctx->base.rounds);

    while (idx--)
    {
        data[idx] ^= ctx->iv[idx];
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
void xteaCfbMacInit(xteaCtx_t *ctx, const uint8_t key[XTEA_KEY_SIZE], const uint_fast8_t rounds)
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
void xteaCfbMacUpdate(xteaCtx_t *ctx, const uint8_t data[], const uint32_t length)
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
void xteaCfbMacFinish(xteaCtx_t *ctx)
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
 * \brief   Function extracts computed MAC code from the XTEA context
 *          and rewrites it to the 'mac' parameter.
 *
 * \param[in]   ctx XTEA context.
 * \param[out]  mac Calculated MAC code.
 *
 * \return Values are returned by the 'mac' parameter.
 */
inline void xteaCfbMacGet(xteaCtx_t *ctx, uint8_t mac[XTEA_BLOCK_SIZE])
{
    if (ctx == NULL)
    {
        return;
    }

    memcpy(mac, ctx->data, XTEA_BLOCK_SIZE);
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
inline bool xteaCfbMacCmp(xteaCtx_t *ctx, const uint8_t mac[XTEA_BLOCK_SIZE])
{
    if (ctx == NULL)
    {
        return false;
    }

    return (0 == memcmp(mac, ctx->data, XTEA_BLOCK_SIZE));
}
