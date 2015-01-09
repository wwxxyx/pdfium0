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
#include "include/BC_CommonByteArray.h"
#include "include/BC_QRCoderBlockPair.h"
CBC_QRCoderBlockPair::CBC_QRCoderBlockPair(CBC_CommonByteArray* data, CBC_CommonByteArray* errorCorrection)
{
    m_dataBytes = data;
    m_errorCorrectionBytes = errorCorrection;
}
CBC_QRCoderBlockPair::~CBC_QRCoderBlockPair()
{
    if(m_dataBytes != NULL) {
        delete m_dataBytes;
        m_dataBytes = NULL;
    }
    if(m_errorCorrectionBytes != NULL) {
        delete m_errorCorrectionBytes;
        m_errorCorrectionBytes = NULL;
    }
}
CBC_CommonByteArray* CBC_QRCoderBlockPair::GetDataBytes()
{
    return m_dataBytes;
}
CBC_CommonByteArray* CBC_QRCoderBlockPair::GetErrorCorrectionBytes()
{
    return m_errorCorrectionBytes;
}