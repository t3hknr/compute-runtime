/*
 * Copyright (c) 2018, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "runtime/command_stream/create_command_stream_impl.h"
#include "runtime/command_stream/aub_command_stream_receiver.h"
#include "runtime/command_stream/tbx_command_stream_receiver.h"
#include "runtime/command_stream/command_stream_receiver_with_aub_dump.h"
#include "runtime/command_stream/device_command_stream.h"
#include "runtime/helpers/debug_helpers.h"
#include "runtime/gmm_helper/gmm_helper.h"
#include "runtime/os_interface/device_factory.h"
#include "runtime/helpers/options.h"
#include "runtime/helpers/hw_info.h"

namespace OCLRT {

CommandStreamReceiver *createCommandStream(const HardwareInfo *pHwInfo) {
    return createCommandStreamImpl(pHwInfo);
}

bool getDevices(HardwareInfo **hwInfo, size_t &numDevicesReturned) {
    bool result;
    int32_t csr = DebugManager.flags.SetCommandStreamReceiver.get();
    if (csr) {
        auto productFamily = DebugManager.flags.ProductFamilyOverride.get();
        auto hwInfoConst = *platformDevices;

        for (int j = 0; j < IGFX_MAX_PRODUCT; j++) {
            if (hardwarePrefix[j] == nullptr)
                continue;
            if (strcmp(hardwarePrefix[j], productFamily.c_str()) == 0) {
                hwInfoConst = hardwareInfoTable[j];
                break;
            }
        }

        *hwInfo = const_cast<HardwareInfo *>(hwInfoConst);
        hardwareInfoSetupGt[hwInfoConst->pPlatform->eProductFamily](const_cast<GT_SYSTEM_INFO *>(hwInfo[0]->pSysInfo));

        numDevicesReturned = 1;
        return true;
    }
    result = DeviceFactory::getDevices(hwInfo, numDevicesReturned);
    if (result) {
        DEBUG_BREAK_IF(hwInfo == nullptr);
        // For now only one device should be present
        DEBUG_BREAK_IF(numDevicesReturned != 1);
    }
    return result;
}
} // namespace OCLRT
