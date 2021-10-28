##
# @file firmware_creator.py
#
# @brief Script to generate firmware images accepted by the CryptBoot bootloader.
#
# @copyright SPDX-FileCopyrightText: Copyright 2021 by Michal Protasowicki
# 
# @license SPDX-License-Identifier: MIT
# 

import argparse
import datetime
import os
import string
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path, PurePosixPath

CONTROL_DATA_SIZE = 64
MAC_FIELD_SIZE = 16
IV_FIELD_SIZE = 16
FIRMWARE_AT_ADDR = (0x08 * 0x100) - CONTROL_DATA_SIZE

U32_S = 4
M32 = 0xFFFFFFFF

AES_IV_SIZE = 16

XTEA_BLOCK_SIZE = 8
XTEA_BLOCK_SIZE_U32 = XTEA_BLOCK_SIZE // U32_S
XTEA_IV_SIZE = XTEA_BLOCK_SIZE
IV_SIZE = XTEA_IV_SIZE
KEY_SIZE = 16
KEY_SIZE_U32 = KEY_SIZE // U32_S

@dataclass
class XteaCtx:
    key:            list    = field(default_factory = lambda: [None] * KEY_SIZE)
    secondKey:      list    = field(default_factory = lambda: [None] * KEY_SIZE)
    iv:             list    = field(default_factory = lambda: [None] * IV_SIZE)
    data:           list    = field(default_factory = lambda: [None] * XTEA_BLOCK_SIZE)
    dataLength:     int     = 0
    rounds:         int     = 32

@dataclass
class FirmwareCtx:
    firmwareMac:    list    = field(default_factory = lambda: [0xFF] * MAC_FIELD_SIZE)          # uint8_t [16] 
    version:        int     = 0x10                                                              # uint8_t
    mode:           int     = 0x00                                                              # uint8_t
    cipherRounds:   int     = 32                                                                # uint8_t
    macRounds:      int     = 32                                                                # uint8_t
    timeStamp:      list    = field(default_factory = lambda: timeStamp())                      # uint32_t
    firmwareSize:   int     = 0                                                                 # uint32_t
    cipherIv:       list    = field(default_factory = lambda: [0xFF] * IV_FIELD_SIZE)           # uint8_t [16]
    rfu:            list    = field(default_factory = lambda: [0xFF] * 4)                       # uint8_t [4]
    newKey:         list    = field(default_factory = lambda: [0xFF] * KEY_SIZE)                # uint8_t [16]
                                                                                                # -------------
                                                                                                # 64 bytes
    firmware:       list    = field(default_factory = list)

    def setEncryption(self, cipher: str):
        self.mode &= 0xFC
        match cipher:
            case 'NONE':
                pass
            case 'XTEA':
                self.mode |= 0x01
            case 'AES':
                raise SystemExit("ERROR: This mode is currently not allowed!!!")
                self.mode |= 0x02
            case _:
                raise SystemExit("ERROR: Unsupported encryption mode!!!")
        return
    def ivLoad(self, iv: list):
        if not((len(iv) == XTEA_IV_SIZE) or (len(iv) == AES_IV_SIZE)):
            raise SystemExit("ERROR: Length IV should be %s bytes for XTEA or %s bytes for AES-128, and there are: %s bytes." % (XTEA_IV_SIZE, AES_IV_SIZE, str(len(iv))))
        self.cipherIv = iv.copy()
        if len(self.cipherIv) != IV_FIELD_SIZE:
            self.cipherIv += [0xFF] * (IV_FIELD_SIZE - len(self.cipherIv))
        return
    def newKeyLoad(self, key: list, cipher: str):
        self.newKey = key.copy()
        self.mode &= 0xF3
        match cipher:
            case 'XTEA':
                self.mode |= 0x04
            case 'AES':
                raise SystemExit("ERROR: This mode is currently not allowed!!!")
                self.mode |= 0x08
            case _:
                raise SystemExit("ERROR: Unsupported encryption mode!!!")
        return
    def firmwareMacLoad(self, mac: list):
        _macLen: int = -1
        match (self.mode >> 4) & 0x03:
            case 0x00:
                _macLen = 8
            case 0x01:
                raise SystemExit("ERROR: This size is currently not allowed")
                _macLen = 12
            case 0x02:
                raise SystemExit("ERROR: This size is currently not allowed")
                _macLen = MAC_FIELD_SIZE
        if len(mac) != _macLen:
            raise SystemExit("ERROR: Invalid MAC size! Should be: %s bytes, and there are: %s bytes." % (str(_macLen), str(len(mac))))
        self.firmwareMac = mac.copy()
        if len(self.firmwareMac) != MAC_FIELD_SIZE:
            self.firmwareMac += [0xFF] * (MAC_FIELD_SIZE - len(self.firmwareMac))
        return
    def getDescrData(self):
        _descr = []
        _descr.append(self.version)
        _descr.append(self.mode)
        _descr.append(self.cipherRounds)
        _descr.append(self.macRounds)
        _descr += self.timeStamp
        _descr += int32ToInt8(self.firmwareSize, 'little')
        _descr += self.cipherIv
        _descr += self.rfu
        _descr += self.newKey
        if len(_descr) != (CONTROL_DATA_SIZE - MAC_FIELD_SIZE):
            raise SystemExit("ERROR: Firmware description w/o MAC should be 48 bytes in size, and there are: %s bytes." % str(len(_descr)))
        return _descr
    def serialize(self):
        _firmware = self.firmwareMac.copy()
        _firmware += self.getDescrData()
        if len(_firmware) != CONTROL_DATA_SIZE:
            raise SystemExit("ERROR: Firmware description should be %s bytes in size, and there are: %s bytes." % (CONTROL_DATA_SIZE, str(len(_firmware))))
        if self.firmwareSize != len(self.firmware):
            raise SystemExit("ERROR: Firmware size does not match!")
        _firmware += self.firmware
        return _firmware

# // mode
# //   ---------------------------------------------------------
# //   Bit Number   Contents
# //   ----------   --------------------------------------------
# //   7 ... 6      MAC type
# //                    00 - CFB-MAC [XTEA]
# //                    01 - CFB-MAC [AES-128]
# //                    10 - HMAC
# //                    11 - RFU (Reserved for Future Use)
# //   5 ... 4      MAC size
# //                    00 - 8
# //                    01 - 12
# //                    10 - 16
# //                    11 - RFU
# //   3 ... 2      new key cipher type
# //                    00 - no new key
# //                    01 - XTEA
# //                    10 - AES-128
# //                    11 - RFU
# //   1 ... 0      firmware cipher type
# //                    00 - none
# //                    01 - XTEA
# //                    10 - AES-128
# //                    11 - RFU
# //   ---------------------------------------------------------

# // timeStamp
# //   ---------------------------------------------------------
# //   Bit Number   Contents
# //   ----------  --------------------------------------------
# //   31 ... 20    YYYY
# //   19 ... 16    MM
# //   15 ... 11    DD
# //   10 ... 06    HH
# //   05 ... 00    MM
# //   ---------------------------------------------------------

def m32(n: int):
    return n & M32

def mAdd(a: int, b: int):
    return m32(a + b)

def mLs(a: int, b: int):
    return m32(a << b)

def mRs(a: int, b: int):
    return m32(a >> b)

def int32ToInt8(n: int, endianness: str):
    _mask = (1 << 8) - 1
    _range = range(0, 32, 8) if (sys.byteorder == endianness) else reversed(range(0, 32, 8))
    return [((n >> k) & _mask) for k in _range]

def xteaEcbEncrypt(key: list, data: list, rounds: int):
    _key: list = [None] * KEY_SIZE_U32
    _data: list = [None] * XTEA_BLOCK_SIZE_U32
    for idx in range(KEY_SIZE_U32):
        _key[idx] = m32(int.from_bytes(key[(idx * U32_S):((idx * U32_S) + U32_S)], byteorder = 'big', signed = False))
    for idx in range(XTEA_BLOCK_SIZE_U32):
        _data[idx] = m32(int.from_bytes(data[(idx * U32_S):((idx * U32_S) + U32_S)], byteorder = 'big', signed = False))
    _v0 = _data[0]
    _v1 = _data[1]
    _sum = m32(0)
    _delta = m32(0x9E3779B9)
    for idx in range(rounds):
        _v00 = mLs(_v1, 4) ^ mRs(_v1, 5)
        _v01 = mAdd(_sum, _key[_sum & 3])
        _v0 = mAdd(_v0, (mAdd(_v00, _v1) ^ _v01))
        _sum = mAdd(_sum, _delta)
        _v10 = mLs(_v0, 4) ^ mRs(_v0, 5)
        _v11 = mAdd(_sum, _key[mRs(_sum, 11) & 3])
        _v1 = mAdd(_v1, mAdd(_v10, _v0) ^ _v11)
    return int32ToInt8(_v0, 'big') + int32ToInt8(_v1, 'big')

def xteaCfbEncryptBlock(ctx: XteaCtx):
    _iv = xteaEcbEncrypt(ctx.key, ctx.iv, ctx.rounds)
    ctx.data = [(ctx.data[idx] ^ _iv[idx]) for idx in range(XTEA_BLOCK_SIZE)]
    ctx.iv = ctx.data.copy()
    return ctx

def xteaCfbInit(ctx: XteaCtx, key: list, iv: list, rounds: int = 32):
    ctx.key = key.copy()
    ctx.iv = iv.copy()
    ctx.rounds = rounds
    ctx.dataLength = 0x00
    return ctx

def xteaCfbEncrypt(ctx: XteaCtx, data: list):
    _ciphertext: list = []
    ctx.dataLength = min(len(data), XTEA_BLOCK_SIZE)
    _start_pos: int = 0
    while ctx.dataLength > 0:
        ctx.data = data[_start_pos:(_start_pos + ctx.dataLength)]
        if len(ctx.data) != XTEA_BLOCK_SIZE:
            ctx.data += [0x00] * (XTEA_BLOCK_SIZE - len(ctx.data))
        ctx = xteaCfbEncryptBlock(ctx)
        _ciphertext += ctx.data[:ctx.dataLength]
        _start_pos += ctx.dataLength
        _remaining_bytes = len(data) - _start_pos
        if _remaining_bytes < XTEA_BLOCK_SIZE:
            ctx.dataLength = _remaining_bytes
    return ctx, _ciphertext

def xteaCfbMacInit(ctx: XteaCtx, key: list, rounds: int = 32):
    ctx.rounds = rounds
    ctx.dataLength = 0x00
    ctx.key = [(key[idx] ^ 0x36) for idx in range(KEY_SIZE)]        # ipad
    ctx.secondKey = [(key[idx] ^ 0x5C) for idx in range(KEY_SIZE)]  # opad
    ctx.iv = [0x00] * IV_SIZE
    return ctx

def xteaCfbMacUpdate(ctx: XteaCtx, data: list):
    for idx in range(len(data)):
        ctx.data[ctx.dataLength] = data[idx]
        ctx.dataLength += 1
        if (XTEA_BLOCK_SIZE == ctx.dataLength):
            ctx = xteaCfbEncryptBlock(ctx)
            ctx.dataLength = 0x00
    return ctx

def xteaCfbMacFinish(ctx: XteaCtx):
    _idx: int = ctx.dataLength
    ctx.data[_idx] = 0x80
    _idx += 1
    while (_idx < XTEA_BLOCK_SIZE):
        ctx.data[_idx] = 0x00
        _idx += 1
    ctx = xteaCfbEncryptBlock(ctx)
    ctx.key = ctx.secondKey.copy()
    ctx = xteaCfbEncryptBlock(ctx)
    return ctx

def xteaCfbMacGet(ctx: XteaCtx):
    return ctx.iv.copy()

def timeStamp():
    _date = datetime.now()
    _timestamp = mLs((_date.year & 0x00000FFF), 20)
    _timestamp |= mLs((_date.month & 0x0000000F), 16)
    _timestamp |= mLs((_date.day & 0x0000001F), 11)
    _timestamp |= mLs((_date.hour & 0x0000001F), 6)
    _timestamp |= (_date.minute & 0x0000003F)
    return int32ToInt8(_timestamp, 'little')


# --------------------------------------------------------------------------------------------------------
def checkRoundsRange(value):
    inValue = int(value)
    if not(20 <= inValue <= 255):
        raise SystemExit("ERROR: %s is outside the allowable range for the 'rounds' parameter 20-255" % value)
    return inValue

def checkKeyValue(value):
    if not(all(char in string.hexdigits for char in value)):
        raise SystemExit("ERROR: key value is not a hexadecimal string")
    if len(value) != (2 * KEY_SIZE):
        raise SystemExit("ERROR: Correct key length is 16 bytes, and there are: %s bytes." % str(len(value) / 2))
    return str(value)

def checkCipherType(value):
    inValue = str(value.upper())
    allowedValues = ['NONE', 'XTEA']
    if not(inValue in allowedValues):
        raise SystemExit("ERROR: %s is unsupported encryption mode!!!" % inValue)
    return inValue

def checkFileType(value):
    inFile = Path(value)
    if inFile.is_file():
        if not(str(inFile).endswith('.bin')):
            raise SystemExit("ERROR: Specified file is NOT a '*.bin' type file: %s" % value)
    else:
        raise SystemExit("ERROR: Specified file does not exist: %s" % value)
    return str(value)

parser = argparse.ArgumentParser()
parser.add_argument("--cipher", type = checkCipherType, required = False, nargs = '?', const = 1, default = 'NONE', help = "firmware encryption algorithm: [NONE, XTEA]")
parser.add_argument("--newKey", type = checkKeyValue, required = False, help = "NEW [XTEA] cryptographic key to be included in the firmware image [32 hex characters -> 16 bytes]")
parser.add_argument("--macRounds", type = checkRoundsRange, required = False, nargs = '?', const = 1, default = 32, help = "number of XTEA rounds for computing MAC code [20-255]")
parser.add_argument("--cipherRounds", type = checkRoundsRange, required = False, nargs = '?', const = 1, default = 32, help = "number of XTEA rounds for encryption [20-255]")
parser.add_argument("--key", type = checkKeyValue, required = True, help = "current encryption/MAC key [32 hex characters -> 16 bytes]")
parser.add_argument("--file", type = checkFileType, required = True, help = "firmware file to be processed")
args = parser.parse_args()

cipherKey: list = list(bytearray.fromhex(args.key))
fwCtx = FirmwareCtx()
fwCtx.ivLoad(list(bytearray(os.urandom(IV_SIZE))))
fwCtx.setEncryption(args.cipher)
fwCtx.cipherRounds = args.cipherRounds
fwCtx.macRounds = args.macRounds

if args.newKey:
    _temp: list = list(bytearray.fromhex(args.newKey))
    fwCtx.newKeyLoad(_temp, 'XTEA')

try:
    inFile = open(args.file, "rb")
    fwCtx.firmware = list(inFile.read())
    fwCtx.firmwareSize = len(fwCtx.firmware)
    inFile.close()
except Exception as err:
    raise SystemExit("ERROR: %s while trying to read from file: %s" % (repr(err), args.file))

ctx = XteaCtx()
ctx = xteaCfbInit(ctx, cipherKey, fwCtx.cipherIv, fwCtx.cipherRounds)
match ((fwCtx.mode >> 2) & 0x03):
    case 0x00:
        pass
    case 0x01:
        ctx, fwCtx.newKey = xteaCfbEncrypt(ctx, fwCtx.newKey)
    case _:
        raise SystemExit("ERROR: This mode is currently not allowed!!!")
if len(fwCtx.firmware) != 0:
    match (fwCtx.mode & 0x03):
        case 0x00:
            pass
        case 0x01:
            ctx, fwCtx.firmware = xteaCfbEncrypt(ctx, fwCtx.firmware)
        case _:
            raise SystemExit("ERROR: This mode is currently not allowed!!!")
if not(fwCtx.mode & 0x40):
    ctx = xteaCfbMacInit(ctx, cipherKey, fwCtx.macRounds)
    ctx = xteaCfbMacUpdate(ctx, fwCtx.getDescrData())
    if len(fwCtx.firmware) != 0:
        ctx = xteaCfbMacUpdate(ctx, fwCtx.firmware)
    ctx = xteaCfbMacFinish(ctx)
    fwCtx.firmwareMacLoad(xteaCfbMacGet(ctx))
else:
    raise SystemExit("ERROR: This mode is currently not allowed!!!")

firmware: list = fwCtx.serialize()

filePath = str(PurePosixPath(args.file).stem)

try:
    outFilePath = filePath + '.crypted.bin'
    outFile = open(outFilePath, "wb")
    outFile.write(bytearray(firmware))
    outFile.close()
except Exception as err:
    raise SystemExit("ERROR: %s while trying to write to file: %s" % (repr(err), outFilePath))

alignedFirmware = [0xFF] * FIRMWARE_AT_ADDR
alignedFirmware += firmware

try:
    outFilePath = filePath + '.aligned.bin'
    outFile = open(outFilePath, "wb")
    outFile.write(bytearray(alignedFirmware))
    outFile.close()
except Exception as err:
    raise SystemExit("ERROR: %s while trying to write to file: %s" % (repr(err), outFilePath))
