// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2008 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "barcode.h"
#include "include/BC_QRCoder.h"
#include "include/BC_QRCoderEncoder.h"
#include "include/BC_CommonByteArray.h"
#include "include/BC_QRCoderMode.h"
#include "include/BC_QRCoderEncoder.h"
#include "include/BC_QRCoderECBlocks.h"
#include "include/BC_QRCoderVersion.h"
#include "include/BC_QRCoderBlockPair.h"
#include "include/BC_QRCoderMaskUtil.h"
#include "include/BC_QRCoderMatrixUtil.h"
#include "include/BC_ReedSolomon.h"
#include "include/BC_CommonByteMatrix.h"
#include "include/BC_ReedSolomonGF256.h"
#include "include/BC_UtilCodingConvert.h"
#include "include/BC_QRCoderBitVector.h"
const FX_INT32 CBC_QRCoderEncoder::m_alphaNumbericTable[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    36, -1, -1, -1, 37, 38, -1, -1, -1, -1, 39, 40, -1, 41, 42, 43,
    0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 44, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1
};
CBC_QRCoderEncoder::CBC_QRCoderEncoder()
{
}
CBC_QRCoderEncoder::~CBC_QRCoderEncoder()
{
}
class Make_Pair : public CFX_Object
{
public:
    CBC_QRCoderMode* m_mode;
    CFX_ByteString m_string;
private:
    Make_Pair(const Make_Pair &mode_string) {}
    Make_Pair &operator = (Make_Pair &mode_string)
    {
        if (this == &mode_string) {
            return *this;
        }
        m_mode = mode_string.m_mode;
        m_string = mode_string.m_string;
        return *this;
    }
public:
    Make_Pair(CBC_QRCoderMode *mode, const CFX_ByteString &str): m_mode(mode), m_string(str) {}
    ~Make_Pair() {}
};
void CBC_QRCoderEncoder::Encode(const CFX_ByteString &content, CBC_QRCoderErrorCorrectionLevel* ecLevel,
                                CBC_QRCoder *qrCode, FX_INT32 &e, FX_INT32 versionSpecify)
{
    if(versionSpecify == 0) {
        EncodeWithAutoVersion(content, ecLevel, qrCode, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e)
    } else if(versionSpecify > 0 && versionSpecify <= 40) {
        EncodeWithSpecifyVersion(content, ecLevel, qrCode, versionSpecify, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    } else {
        e = BCExceptionVersionMust1_40;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::AppendECI(CBC_QRCoderBitVector* bits)
{
}
void CBC_QRCoderEncoder::AppendDataModeLenghInfo(CFX_PtrArray &splitResult, CBC_QRCoderBitVector &headerAndDataBits,
        CBC_QRCoderMode *tempMode, CBC_QRCoder *qrCode, CFX_ByteString &encoding, FX_INT32 &e)
{
    for(FX_INT32 i = 0; i < splitResult.GetSize(); i++) {
        tempMode = ((Make_Pair*)splitResult[i])->m_mode;
        if(tempMode == CBC_QRCoderMode::sGBK) {
            AppendModeInfo(tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendLengthInfo(((Make_Pair*)splitResult[i])->m_string.GetLength(), qrCode->GetVersion(), tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendBytes(((Make_Pair*)splitResult[i])->m_string, tempMode, &headerAndDataBits, encoding, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        } else if(tempMode == CBC_QRCoderMode::sBYTE) {
            CFX_ByteArray bytes;
            CBC_UtilCodingConvert::LocaleToUtf8(((Make_Pair*)splitResult[i])->m_string, bytes);
            AppendModeInfo(tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendLengthInfo(bytes.GetSize(), qrCode->GetVersion(), tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            Append8BitBytes(bytes, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        } else if(tempMode == CBC_QRCoderMode::sALPHANUMERIC) {
            AppendModeInfo(tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendLengthInfo(((Make_Pair*)splitResult[i])->m_string.GetLength(), qrCode->GetVersion(), tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendBytes(((Make_Pair*)splitResult[i])->m_string, tempMode, &headerAndDataBits, encoding, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        } else if(tempMode == CBC_QRCoderMode::sNUMERIC) {
            AppendModeInfo(tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendLengthInfo(((Make_Pair*)splitResult[i])->m_string.GetLength(), qrCode->GetVersion(), tempMode, &headerAndDataBits, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            AppendBytes(((Make_Pair*)splitResult[i])->m_string, tempMode, &headerAndDataBits, encoding, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        } else {
            e = BCExceptionUnknown;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
    }
}
void CBC_QRCoderEncoder::SplitString(const CFX_ByteString &content, CFX_PtrArray &result)
{
    FX_INT32 index = 0, flag = 0;
    while((((FX_BYTE)content[index] >= 0xA1 && (FX_BYTE)content[index] <= 0xAA) ||
            ((FX_BYTE)content[index] >= 0xB0 && (FX_BYTE)content[index] <= 0xFA)) && (index < content.GetLength())) {
        index += 2;
    }
    if(index != flag) {
        result.Add(FX_NEW Make_Pair(CBC_QRCoderMode::sGBK, content.Mid(flag, index - flag)));
    }
    flag = index;
    if(index >= content.GetLength()) {
        return;
    }
    while(GetAlphaNumericCode((FX_BYTE)content[index]) == -1
            && !(((FX_BYTE)content[index] >= 0xA1 && (FX_BYTE)content[index] <= 0xAA) ||
                 ((FX_BYTE)content[index] >= 0xB0 && (FX_BYTE)content[index] <= 0xFA))
            && (index < content.GetLength())) {
#if _FXM_PLATFORM_ == _FXM_PLATFORM_WINDOWS_
        if(IsDBCSLeadByte((FX_BYTE)content[index]))
#else
        if((FX_BYTE)content[index] > 127)
#endif
        {
            index += 2;
        } else {
            index++;
        }
    }
    if(index != flag) {
        result.Add(FX_NEW Make_Pair(CBC_QRCoderMode::sBYTE, content.Mid(flag, index - flag)));
    }
    flag = index;
    if(index >= content.GetLength()) {
        return;
    }
    while(FXSYS_Isdigit((FX_BYTE)content[index]) && (index < content.GetLength())) {
        index++;
    }
    if(index != flag) {
        result.Add(FX_NEW Make_Pair(CBC_QRCoderMode::sNUMERIC, content.Mid(flag, index - flag)));
    }
    flag = index;
    if(index >= content.GetLength()) {
        return;
    }
    while(GetAlphaNumericCode((FX_BYTE)content[index]) != -1  && (index < content.GetLength())) {
        index++;
    }
    if(index != flag) {
        result.Add(FX_NEW Make_Pair(CBC_QRCoderMode::sALPHANUMERIC, content.Mid(flag, index - flag)));
    }
    flag = index;
    if(index >= content.GetLength()) {
        return;
    }
    SplitString(content.Mid(index, content.GetLength() - index), result);
}
FX_INT32 CBC_QRCoderEncoder::GetSpanByVersion(CBC_QRCoderMode *modeFirst, CBC_QRCoderMode *modeSecond, FX_INT32 versionNum, FX_INT32 &e)
{
    if(versionNum == 0) {
        return 0;
    }
    if((modeFirst == CBC_QRCoderMode::sALPHANUMERIC)
            && (modeSecond == CBC_QRCoderMode::sBYTE)) {
        if(versionNum >= 1 && versionNum <= 9) {
            return 11;
        } else if(versionNum >= 10 && versionNum <= 26) {
            return 15;
        } else if(versionNum >= 27 && versionNum <= 40) {
            return 16;
        } else {
            e = BCExceptionNoSuchVersion;
            BC_EXCEPTION_CHECK_ReturnValue(e, 0);
        }
    } else if((modeSecond == CBC_QRCoderMode::sALPHANUMERIC)
              && (modeFirst == CBC_QRCoderMode::sNUMERIC)) {
        if(versionNum >= 1 && versionNum <= 9) {
            return 13;
        } else if(versionNum >= 10 && versionNum <= 26) {
            return 15;
        } else if(versionNum >= 27 && versionNum <= 40) {
            return 17;
        } else {
            e = BCExceptionNoSuchVersion;
            BC_EXCEPTION_CHECK_ReturnValue(e, 0);
        }
    } else if((modeSecond == CBC_QRCoderMode::sBYTE)
              && (modeFirst == CBC_QRCoderMode::sNUMERIC)) {
        if(versionNum >= 1 && versionNum <= 9) {
            return 6;
        } else if(versionNum >= 10 && versionNum <= 26) {
            return 8;
        } else if(versionNum >= 27 && versionNum <= 40) {
            return 9;
        } else {
            e = BCExceptionNoSuchVersion;
            BC_EXCEPTION_CHECK_ReturnValue(e, 0);
        }
    }
    return -1;
}
void CBC_QRCoderEncoder::MergeString(CFX_PtrArray &result, FX_INT32 versionNum, FX_INT32 &e)
{
    Make_Pair *first = NULL;
    Make_Pair *second = NULL;
    size_t mergeNum = 0;
    FX_INT32 i;
    for(i = 0; ((i < result.GetSize()) && (i + 1 < result.GetSize())); i++) {
        first = (Make_Pair*)result[i];
        second = (Make_Pair*)result[i + 1];
        if(first->m_mode == CBC_QRCoderMode::sALPHANUMERIC) {
            FX_INT32 tmp = GetSpanByVersion(CBC_QRCoderMode::sALPHANUMERIC, CBC_QRCoderMode::sBYTE, versionNum, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            if((second->m_mode == CBC_QRCoderMode::sBYTE)
                    && (first->m_string.GetLength() < tmp)) {
                CFX_ByteString str = first->m_string + second->m_string;
                second->m_string = str;
                delete first;
                result.RemoveAt(i);
                i--;
                mergeNum++;
            }
        } else if(first->m_mode == CBC_QRCoderMode::sBYTE) {
            if(second->m_mode == CBC_QRCoderMode::sBYTE) {
                first->m_string += second->m_string;
                delete second;
                result.RemoveAt(i + 1);
                i--;
                mergeNum++;
            }
        } else if(first->m_mode == CBC_QRCoderMode::sNUMERIC) {
            FX_INT32 tmp = GetSpanByVersion(CBC_QRCoderMode::sNUMERIC, CBC_QRCoderMode::sBYTE, versionNum, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            if((second->m_mode == CBC_QRCoderMode::sBYTE)
                    && (first->m_string.GetLength() < tmp)) {
                CFX_ByteString str = first->m_string + second->m_string;
                second->m_string = str;
                delete first;
                result.RemoveAt(i);
                i--;
                mergeNum++;
            }
            tmp = GetSpanByVersion(CBC_QRCoderMode::sNUMERIC, CBC_QRCoderMode::sALPHANUMERIC, versionNum, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            if((second->m_mode == CBC_QRCoderMode::sALPHANUMERIC)
                    && (first->m_string.GetLength() < tmp)) {
                CFX_ByteString str = first->m_string + second->m_string;
                second->m_string = str;
                delete first;
                result.RemoveAt(i);
                i--;
                mergeNum++;
            }
        }
    }
    if(mergeNum == 0) {
        return;
    }
    MergeString(result, versionNum, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
}
void CBC_QRCoderEncoder::InitQRCode(FX_INT32 numInputBytes, FX_INT32 versionNumber,
                                    CBC_QRCoderErrorCorrectionLevel* ecLevel, CBC_QRCoderMode* mode, CBC_QRCoder* qrCode, FX_INT32 &e)
{
    qrCode->SetECLevel(ecLevel);
    qrCode->SetMode(mode);
    CBC_QRCoderVersion* version = CBC_QRCoderVersion::GetVersionForNumber(versionNumber, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    FX_INT32 numBytes = version->GetTotalCodeWords();
    CBC_QRCoderECBlocks* ecBlocks = version->GetECBlocksForLevel(ecLevel);
    FX_INT32 numEcBytes = ecBlocks->GetTotalECCodeWords();
    FX_INT32 numRSBlocks = ecBlocks->GetNumBlocks();
    FX_INT32 numDataBytes = numBytes - numEcBytes;
    if(numDataBytes >= numInputBytes + 3) {
        qrCode->SetVersion(versionNumber);
        qrCode->SetNumTotalBytes(numBytes);
        qrCode->SetNumDataBytes(numDataBytes);
        qrCode->SetNumRSBlocks(numRSBlocks);
        qrCode->SetNumECBytes(numEcBytes);
        qrCode->SetMatrixWidth(version->GetDimensionForVersion());
        return;
    }
    e = BCExceptionCannotFindBlockInfo;
    BC_EXCEPTION_CHECK_ReturnVoid(e);
}
void CBC_QRCoderEncoder::EncodeWithSpecifyVersion(const CFX_ByteString &content, CBC_QRCoderErrorCorrectionLevel* ecLevel,
        CBC_QRCoder *qrCode, FX_INT32 versionSpecify, FX_INT32 &e)
{
    CFX_ByteString encoding = "utf8";
    CBC_QRCoderMode *mode = CBC_QRCoderMode::sBYTE;
    CFX_PtrArray splitResult;
    CBC_QRCoderBitVector dataBits;
    dataBits.Init();
    SplitString(content, splitResult);
    MergeString(splitResult, versionSpecify, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e)
    CBC_QRCoderMode *tempMode = NULL;
    for(FX_INT32 i = 0; i < splitResult.GetSize(); i++) {
        AppendBytes(((Make_Pair*)splitResult[i])->m_string, ((Make_Pair*)splitResult[i])->m_mode, &dataBits, encoding, e);
        if(e != BCExceptionNO) {
            for(FX_INT32 y = 0; y < splitResult.GetSize(); y++) {
                delete (Make_Pair*)splitResult[y];
            }
            splitResult.RemoveAll();
            return;
        }
    }
    FX_INT32 numInputBytes = dataBits.sizeInBytes();
    CBC_QRCoderBitVector headerAndDataBits;
    headerAndDataBits.Init();
    InitQRCode(numInputBytes, versionSpecify, ecLevel, mode, qrCode, e);
    if(e != BCExceptionNO) {
        for(FX_INT32 k = 0; k < splitResult.GetSize(); k++) {
            delete (Make_Pair*)splitResult[k];
        }
        splitResult.RemoveAll();
        return ;
    }
    AppendDataModeLenghInfo(splitResult, headerAndDataBits, tempMode, qrCode, encoding, e);
    if(e != BCExceptionNO) {
        for(FX_INT32 k = 0; k < splitResult.GetSize(); k++) {
            delete (Make_Pair*)splitResult[k];
        }
        splitResult.RemoveAll();
        return ;
    }
    numInputBytes = headerAndDataBits.sizeInBytes();
    TerminateBits(qrCode->GetNumDataBytes(), &headerAndDataBits, e);
    if(e != BCExceptionNO) {
        for(FX_INT32 k = 0; k < splitResult.GetSize(); k++) {
            delete (Make_Pair*)splitResult[k];
        }
        splitResult.RemoveAll();
        return ;
    }
    for(FX_INT32 j = 0; j < splitResult.GetSize(); j++) {
        delete (Make_Pair*)splitResult[j];
    }
    splitResult.RemoveAll();
    CBC_QRCoderBitVector finalBits ;
    finalBits.Init();
    InterleaveWithECBytes(&headerAndDataBits, qrCode->GetNumTotalBytes(), qrCode->GetNumDataBytes(),
                          qrCode->GetNumRSBlocks(), &finalBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_CommonByteMatrix* pDecoder = FX_NEW CBC_CommonByteMatrix(qrCode->GetMatrixWidth(), qrCode->GetMatrixWidth());
    pDecoder->Init();
    CBC_AutoPtr<CBC_CommonByteMatrix> matrix(pDecoder);
    FX_INT32 maskPattern = ChooseMaskPattern(&finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), matrix.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    qrCode->SetMaskPattern(maskPattern);
    CBC_QRCoderMatrixUtil::BuildMatrix(&finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), qrCode->GetMaskPattern(), matrix.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    qrCode->SetMatrix(matrix.release());
    if(!qrCode->IsValid()) {
        e = BCExceptionInvalidQRCode;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::EncodeWithAutoVersion(const CFX_ByteString &content, CBC_QRCoderErrorCorrectionLevel* ecLevel, CBC_QRCoder *qrCode, FX_INT32 &e)
{
    CFX_ByteString encoding = "utf8";
    CBC_QRCoderMode *mode = CBC_QRCoderMode::sBYTE;
    CFX_PtrArray splitResult;
    CBC_QRCoderBitVector dataBits;
    dataBits.Init();
    SplitString(content, splitResult);
    MergeString(splitResult, 8, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_QRCoderMode *tempMode = NULL;
    for(FX_INT32 i = 0; i < splitResult.GetSize(); i++) {
        AppendBytes(((Make_Pair*)splitResult[i])->m_string, ((Make_Pair*)splitResult[i])->m_mode, &dataBits, encoding, e);
        if(e != BCExceptionNO) {
            for(FX_INT32 l = 0; l < splitResult.GetSize(); l++) {
                delete (Make_Pair*)splitResult[l];
            }
            splitResult.RemoveAll();
            return;
        }
    }
    FX_INT32 numInputBytes = dataBits.sizeInBytes();
    InitQRCode(numInputBytes, ecLevel, mode, qrCode, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e)
    CBC_QRCoderBitVector headerAndDataBits;
    headerAndDataBits.Init();
    tempMode = NULL;
    FX_INT32 versionNum = qrCode->GetVersion();
sign:
    AppendDataModeLenghInfo(splitResult, headerAndDataBits, tempMode, qrCode, encoding, e);
    if (e != BCExceptionNO) {
        goto catchException;
    }
    numInputBytes = headerAndDataBits.sizeInBytes();
    TerminateBits(qrCode->GetNumDataBytes(), &headerAndDataBits, e);
    if (e != BCExceptionNO) {
        goto catchException;
    }
catchException:
    if (e != BCExceptionNO) {
        FX_INT32 e1 = BCExceptionNO;
        InitQRCode(numInputBytes, ecLevel, mode, qrCode, e1);
        if (e1 != BCExceptionNO) {
            e = e1;
            return;
        }
        versionNum++;
        if (versionNum <= 40) {
            headerAndDataBits.Clear();
            e = BCExceptionNO;
            goto sign;
        } else {
            for (FX_INT32 j = 0; j < splitResult.GetSize(); j++) {
                delete (Make_Pair*)splitResult[j];
            }
            splitResult.RemoveAll();
            return;
        }
    }
    for (FX_INT32 k = 0; k < splitResult.GetSize(); k++) {
        delete (Make_Pair*)splitResult[k];
    }
    splitResult.RemoveAll();
    CBC_QRCoderBitVector finalBits ;
    finalBits.Init();
    InterleaveWithECBytes(&headerAndDataBits, qrCode->GetNumTotalBytes(), qrCode->GetNumDataBytes(),
                          qrCode->GetNumRSBlocks(), &finalBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_CommonByteMatrix* pDecoder = FX_NEW CBC_CommonByteMatrix(qrCode->GetMatrixWidth(), qrCode->GetMatrixWidth());
    pDecoder->Init();
    CBC_AutoPtr<CBC_CommonByteMatrix> matrix(pDecoder);
    FX_INT32 maskPattern = ChooseMaskPattern(&finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), matrix.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    qrCode->SetMaskPattern(maskPattern);
    CBC_QRCoderMatrixUtil::BuildMatrix(&finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), qrCode->GetMaskPattern(), matrix.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e)
    qrCode->SetMatrix(matrix.release());
    if(!qrCode->IsValid()) {
        e = BCExceptionInvalidQRCode;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::Encode(const CFX_WideString &content, CBC_QRCoderErrorCorrectionLevel* ecLevel, CBC_QRCoder *qrCode, FX_INT32 &e)
{
    CFX_ByteString encoding = "utf8";
    CFX_ByteString utf8Data;
    CBC_UtilCodingConvert::UnicodeToUTF8(content, utf8Data);
    CBC_QRCoderMode* mode = ChooseMode(utf8Data, encoding);
    CBC_QRCoderBitVector dataBits;
    dataBits.Init();
    AppendBytes(utf8Data, mode, &dataBits, encoding, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    FX_INT32 numInputBytes = dataBits.sizeInBytes();
    InitQRCode(numInputBytes, ecLevel, mode, qrCode, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_QRCoderBitVector headerAndDataBits;
    headerAndDataBits.Init();
    AppendModeInfo(mode, &headerAndDataBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    FX_INT32 numLetters = mode == CBC_QRCoderMode::sBYTE ? dataBits.sizeInBytes() : content.GetLength();
    AppendLengthInfo(numLetters, qrCode->GetVersion(), mode, &headerAndDataBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    headerAndDataBits.AppendBitVector(&dataBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e)
    TerminateBits(qrCode->GetNumDataBytes(), &headerAndDataBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_QRCoderBitVector finalBits ;
    finalBits.Init();
    InterleaveWithECBytes(&headerAndDataBits, qrCode->GetNumTotalBytes(), qrCode->GetNumDataBytes(),
                          qrCode->GetNumRSBlocks(), &finalBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_CommonByteMatrix* pDecoder = FX_NEW CBC_CommonByteMatrix(qrCode->GetMatrixWidth(), qrCode->GetMatrixWidth());
    pDecoder->Init();
    CBC_AutoPtr<CBC_CommonByteMatrix> matrix(pDecoder);
    FX_INT32 maskPattern = ChooseMaskPattern(&finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), matrix.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    qrCode->SetMaskPattern(maskPattern);
    CBC_QRCoderMatrixUtil::BuildMatrix(&finalBits, qrCode->GetECLevel(), qrCode->GetVersion(), qrCode->GetMaskPattern(), matrix.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e)
    qrCode->SetMatrix(matrix.release());
    if(!qrCode->IsValid()) {
        e = BCExceptionInvalidQRCode;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::TerminateBits(FX_INT32 numDataBytes, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    FX_INT32 capacity = numDataBytes << 3;
    if(bits->Size() > capacity) {
        e = BCExceptionDataTooMany;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
    for (FX_INT32 i = 0; i < 4 && bits->Size() < capacity; ++i) {
        bits->AppendBit(0, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
    FX_INT32 numBitsInLastByte = bits->Size() % 8;
    if (numBitsInLastByte > 0) {
        FX_INT32 numPaddingBits = 8 - numBitsInLastByte;
        for (FX_INT32 j = 0; j < numPaddingBits; ++j) {
            bits->AppendBit(0, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e)
        }
    }
    if (bits->Size() % 8 != 0) {
        e = BCExceptionDigitLengthMustBe8;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
    FX_INT32 numPaddingBytes = numDataBytes - bits->sizeInBytes();
    for (FX_INT32 k = 0; k < numPaddingBytes; ++k) {
        if (k % 2 == 0) {
            bits->AppendBits(0xec, 8, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        } else {
            bits->AppendBits(0x11, 8, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
    }
    if (bits->Size() != capacity) {
        e = BCExceptionBitsNotEqualCacity;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
FX_INT32 CBC_QRCoderEncoder::ChooseMaskPattern(CBC_QRCoderBitVector* bits, CBC_QRCoderErrorCorrectionLevel* ecLevel, FX_INT32 version, CBC_CommonByteMatrix* matrix, FX_INT32 &e)
{
    FX_INT32 minPenalty = 65535;
    FX_INT32 bestMaskPattern = -1;
    for(FX_INT32 maskPattern = 0; maskPattern < CBC_QRCoder::NUM_MASK_PATTERNS; maskPattern++) {
        CBC_QRCoderMatrixUtil::BuildMatrix(bits, ecLevel, version, maskPattern, matrix, e);
        BC_EXCEPTION_CHECK_ReturnValue(e, 0);
        FX_INT32 penalty = CalculateMaskPenalty(matrix);
        if(penalty < minPenalty) {
            minPenalty = penalty;
            bestMaskPattern = maskPattern;
        }
    }
    return bestMaskPattern;
}
FX_INT32 CBC_QRCoderEncoder::CalculateMaskPenalty(CBC_CommonByteMatrix* matrix)
{
    FX_INT32 penalty = 0;
    penalty += CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule1(matrix);
    penalty += CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule2(matrix);
    penalty += CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule3(matrix);
    penalty += CBC_QRCoderMaskUtil::ApplyMaskPenaltyRule4(matrix);
    return penalty;
}
CBC_QRCoderMode *CBC_QRCoderEncoder::ChooseMode(const CFX_ByteString &content, CFX_ByteString encoding)
{
    if(encoding.Compare("SHIFT_JIS") == 0) {
        return CBC_QRCoderMode::sKANJI;
    }
    FX_BOOL hasNumeric = FALSE;
    FX_BOOL hasAlphaNumeric = FALSE;
    for(FX_INT32 i = 0; i < content.GetLength(); i++) {
        if(isdigit((FX_BYTE)content[i])) {
            hasNumeric = TRUE;
        } else if(GetAlphaNumericCode((FX_BYTE)content[i]) != -1) {
            hasAlphaNumeric = TRUE;
        } else {
            return CBC_QRCoderMode::sBYTE;
        }
    }
    if(hasAlphaNumeric) {
        return CBC_QRCoderMode::sALPHANUMERIC;
    } else if(hasNumeric) {
        return CBC_QRCoderMode::sNUMERIC;
    }
    return CBC_QRCoderMode::sBYTE;
}
FX_INT32 CBC_QRCoderEncoder::GetAlphaNumericCode(FX_INT32 code)
{
    if(code < 96 && code >= 0) {
        return m_alphaNumbericTable[code];
    }
    return -1;
}
void CBC_QRCoderEncoder::AppendBytes(const CFX_ByteString &content, CBC_QRCoderMode* mode, CBC_QRCoderBitVector* bits, CFX_ByteString encoding, FX_INT32 &e)
{
    if(mode == CBC_QRCoderMode::sNUMERIC) {
        AppendNumericBytes(content, bits, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    } else if(mode == CBC_QRCoderMode::sALPHANUMERIC) {
        AppendAlphaNumericBytes(content, bits, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    } else if(mode == CBC_QRCoderMode::sBYTE) {
        Append8BitBytes(content, bits, encoding, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    } else if(mode == CBC_QRCoderMode::sKANJI) {
        AppendKanjiBytes(content, bits, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    } else if(mode == CBC_QRCoderMode::sGBK) {
        AppendGBKBytes(content, bits, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    } else {
        e = BCExceptionUnsupportedMode;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::AppendNumericBytes(const CFX_ByteString &content, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    FX_INT32 length = content.GetLength();
    FX_INT32 i = 0;
    while(i < length) {
        FX_INT32 num1 = content[i] - '0';
        if(i + 2 < length) {
            FX_INT32 num2 = content[i + 1] - '0';
            FX_INT32 num3 = content[i + 2] - '0';
            bits->AppendBits(num1 * 100 + num2 * 10 + num3, 10, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e)
            i += 3;
        } else if(i + 1 < length) {
            FX_INT32 num2 = content[i + 1] - '0';
            bits->AppendBits(num1 * 10 + num2, 7, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e)
            i += 2;
        } else {
            bits->AppendBits(num1, 4, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            i++;
        }
    }
}
void CBC_QRCoderEncoder::AppendAlphaNumericBytes(const CFX_ByteString &content, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    FX_INT32 length = content.GetLength();
    FX_INT32 i = 0;
    while(i < length) {
        FX_INT32 code1 = GetAlphaNumericCode(content[i]);
        if(code1 == -1) {
            e = BCExceptionInvalidateCharacter;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
        if(i + 1 < length) {
            FX_INT32 code2 = GetAlphaNumericCode(content[i + 1]);
            if(code2 == -1) {
                e = BCExceptionInvalidateCharacter;
                BC_EXCEPTION_CHECK_ReturnVoid(e);
            }
            bits->AppendBits(code1 * 45 + code2, 11, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            i += 2;
        } else {
            bits->AppendBits(code1, 6, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e)
            i++;
        }
    }
}
void CBC_QRCoderEncoder::AppendGBKBytes(const CFX_ByteString &content, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    FX_INT32 length = content.GetLength();
    FX_DWORD value = 0;
    for(FX_INT32 i = 0; i < length; i += 2) {
        value = (FX_DWORD)((FX_BYTE)content[i] << 8 | (FX_BYTE)content[i + 1]);
        if(value <= 0xAAFE && value >= 0xA1A1) {
            value -= 0xA1A1;
        } else if(value <= 0xFAFE && value >= 0xB0A1) {
            value -= 0xA6A1;
        } else {
            e = BCExceptionInvalidateCharacter;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
        value = (FX_DWORD)((value >> 8 ) * 0x60) + (FX_DWORD)(value & 0xff);
        bits->AppendBits(value, 13, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::Append8BitBytes(const CFX_ByteString &content, CBC_QRCoderBitVector* bits, CFX_ByteString encoding, FX_INT32 &e)
{
    for(FX_INT32 i = 0; i < content.GetLength(); i++) {
        bits->AppendBits(content[i], 8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::Append8BitBytes(CFX_ByteArray &bytes, CBC_QRCoderBitVector *bits, FX_INT32 &e)
{
    for(FX_INT32 i = 0; i < bytes.GetSize(); i++) {
        bits->AppendBits(bytes[i], 8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::AppendKanjiBytes(const CFX_ByteString &content, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    CFX_ByteArray bytes;
    FX_DWORD value = 0, h = 0;
    for(FX_INT32 i = 0; i < bytes.GetSize(); i += 2) {
        value = (FX_DWORD)((FX_BYTE)(content[i] << 8) | (FX_BYTE)content[i + 1]);
        if(value <= 0x9ffc && value >= 0x8140) {
            value -= 0x8140;
        } else if(value <= 0xebbf && value >= 0xe040) {
            value -= 0xc140;
        } else {
            e = BCExceptionInvalidateCharacter;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
        value = (FX_DWORD)((value >> 8 ) * 0xc0) + (FX_DWORD)(value & 0xff);
        bits->AppendBits(value, 13, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::InitQRCode(FX_INT32 numInputBytes, CBC_QRCoderErrorCorrectionLevel* ecLevel,
                                    CBC_QRCoderMode* mode, CBC_QRCoder* qrCode, FX_INT32 &e)
{
    qrCode->SetECLevel(ecLevel);
    qrCode->SetMode(mode);
    for(FX_INT32 versionNum = 1; versionNum <= 40; versionNum++) {
        CBC_QRCoderVersion* version = CBC_QRCoderVersion::GetVersionForNumber(versionNum, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        FX_INT32 numBytes = version->GetTotalCodeWords();
        CBC_QRCoderECBlocks* ecBlocks = version->GetECBlocksForLevel(ecLevel);
        FX_INT32 numEcBytes = ecBlocks->GetTotalECCodeWords();
        FX_INT32 numRSBlocks = ecBlocks->GetNumBlocks();
        FX_INT32 numDataBytes = numBytes - numEcBytes;
        if(numDataBytes >= numInputBytes + 3) {
            qrCode->SetVersion(versionNum);
            qrCode->SetNumTotalBytes(numBytes);
            qrCode->SetNumDataBytes(numDataBytes);
            qrCode->SetNumRSBlocks(numRSBlocks);
            qrCode->SetNumECBytes(numEcBytes);
            qrCode->SetMatrixWidth(version->GetDimensionForVersion());
            return;
        }
    }
    e = BCExceptionCannotFindBlockInfo;
    BC_EXCEPTION_CHECK_ReturnVoid(e);
}
void CBC_QRCoderEncoder::AppendModeInfo(CBC_QRCoderMode* mode, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    bits->AppendBits(mode->GetBits(), 4, e);
    if(mode == CBC_QRCoderMode::sGBK) {
        bits->AppendBits(1, 4, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::AppendLengthInfo(FX_INT32 numLetters, FX_INT32 version, CBC_QRCoderMode* mode, CBC_QRCoderBitVector* bits, FX_INT32 &e)
{
    CBC_QRCoderVersion* qcv = CBC_QRCoderVersion::GetVersionForNumber(version, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    FX_INT32 numBits = mode->GetCharacterCountBits(qcv, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    if(numBits > ((1 << numBits) - 1)) {
        return;
    }
    if(mode == CBC_QRCoderMode::sGBK) {
        bits->AppendBits(numLetters / 2, numBits, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
    bits->AppendBits(numLetters, numBits, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
}
void CBC_QRCoderEncoder::InterleaveWithECBytes(CBC_QRCoderBitVector* bits, FX_INT32 numTotalBytes, FX_INT32 numDataBytes, FX_INT32 numRSBlocks, CBC_QRCoderBitVector* result, FX_INT32 &e)
{
    if(bits->sizeInBytes() != numDataBytes) {
        e = BCExceptionBitsBytesNotMatch;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
    FX_INT32 dataBytesOffset = 0;
    FX_INT32 maxNumDataBytes = 0;
    FX_INT32 maxNumEcBytes = 0;
    CFX_PtrArray blocks;
    FX_INT32 i;
    for(i = 0; i < numRSBlocks; i++) {
        FX_INT32 numDataBytesInBlock;
        FX_INT32 numEcBytesInBlosk;
        GetNumDataBytesAndNumECBytesForBlockID(numTotalBytes, numDataBytes, numRSBlocks, i,
                                               numDataBytesInBlock, numEcBytesInBlosk);
        CBC_CommonByteArray* dataBytes = FX_NEW CBC_CommonByteArray;
        dataBytes->Set(bits->GetArray(), dataBytesOffset, numDataBytesInBlock);
        CBC_CommonByteArray* ecBytes = GenerateECBytes(dataBytes, numEcBytesInBlosk, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        blocks.Add(FX_NEW CBC_QRCoderBlockPair(dataBytes, ecBytes));
        maxNumDataBytes = FX_MAX(maxNumDataBytes, dataBytes->Size());
        maxNumEcBytes = FX_MAX(maxNumEcBytes, ecBytes->Size());
        dataBytesOffset += numDataBytesInBlock;
    }
    if(numDataBytes != dataBytesOffset) {
        e = BCExceptionBytesNotMatchOffset;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
    for(FX_INT32 x = 0; x < maxNumDataBytes; x++) {
        for(FX_INT32 j = 0; j < blocks.GetSize(); j++) {
            CBC_CommonByteArray* dataBytes = ((CBC_QRCoderBlockPair*)blocks[j])->GetDataBytes();
            if(x < dataBytes->Size()) {
                result->AppendBits(dataBytes->At(x), 8, e);
                BC_EXCEPTION_CHECK_ReturnVoid(e);
            }
        }
    }
    for(FX_INT32 y = 0; y < maxNumEcBytes; y++) {
        for(FX_INT32 l = 0; l < blocks.GetSize(); l++) {
            CBC_CommonByteArray* ecBytes = ((CBC_QRCoderBlockPair*)blocks[l])->GetErrorCorrectionBytes();
            if(y < ecBytes->Size()) {
                result->AppendBits(ecBytes->At(y), 8, e);
                BC_EXCEPTION_CHECK_ReturnVoid(e);
            }
        }
    }
    for(FX_INT32 k = 0; k < blocks.GetSize(); k++) {
        delete (CBC_QRCoderBlockPair*)blocks[k];
    }
    if(numTotalBytes != result->sizeInBytes()) {
        e = BCExceptionSizeInBytesDiffer;
        BC_EXCEPTION_CHECK_ReturnVoid(e);
    }
}
void CBC_QRCoderEncoder::GetNumDataBytesAndNumECBytesForBlockID(FX_INT32 numTotalBytes, FX_INT32 numDataBytes,
        FX_INT32 numRSBlocks, FX_INT32 blockID,
        FX_INT32 &numDataBytesInBlock, FX_INT32& numECBytesInBlock)
{
    if(blockID >= numRSBlocks) {
        return;
    }
    FX_INT32 numRsBlocksInGroup2 = numTotalBytes % numRSBlocks;
    FX_INT32 numRsBlocksInGroup1 = numRSBlocks - numRsBlocksInGroup2;
    FX_INT32 numTotalBytesInGroup1 = numTotalBytes / numRSBlocks;
    FX_INT32 numTotalBytesInGroup2 = numTotalBytesInGroup1 + 1;
    FX_INT32 numDataBytesInGroup1 = numDataBytes / numRSBlocks;
    FX_INT32 numDataBytesInGroup2 = numDataBytesInGroup1 + 1;
    FX_INT32 numEcBytesInGroup1 = numTotalBytesInGroup1 - numDataBytesInGroup1;
    FX_INT32 numEcBytesInGroup2 = numTotalBytesInGroup2 - numDataBytesInGroup2;
    if (blockID < numRsBlocksInGroup1) {
        numDataBytesInBlock = numDataBytesInGroup1;
        numECBytesInBlock = numEcBytesInGroup1;
    } else {
        numDataBytesInBlock = numDataBytesInGroup2;
        numECBytesInBlock = numEcBytesInGroup2;
    }
}
CBC_CommonByteArray* CBC_QRCoderEncoder::GenerateECBytes(CBC_CommonByteArray* dataBytes, FX_INT32 numEcBytesInBlock, FX_INT32 &e)
{
    FX_INT32 numDataBytes = dataBytes->Size();
    CFX_Int32Array toEncode;
    toEncode.SetSize(numDataBytes + numEcBytesInBlock);
    for(FX_INT32 i = 0; i < numDataBytes; i++) {
        toEncode[i] = (dataBytes->At(i));
    }
    CBC_ReedSolomonEncoder encode(CBC_ReedSolomonGF256::QRCodeFild);
    encode.Init();
    encode.Encode(&toEncode, numEcBytesInBlock, e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_CommonByteArray* ecBytes = FX_NEW CBC_CommonByteArray(numEcBytesInBlock);
    for(FX_INT32 j = 0; j < numEcBytesInBlock; j++) {
        ecBytes->Set(j, toEncode[numDataBytes + j]);
    }
    return ecBytes;
}