/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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

#include "runtime/command_stream/command_stream_receiver.h"
#include "runtime/command_stream/device_command_stream.h"
#include "hw_cmds.h"
#include "runtime/device/device.h"
#include "runtime/device/device_vector.h"
#include "runtime/helpers/debug_helpers.h"
#include "runtime/helpers/options.h"
#include "runtime/memory_manager/memory_manager.h"
#include "runtime/os_interface/os_time.h"
#include "runtime/device/driver_info.h"
#include <cstring>
#include <map>

namespace OCLRT {

decltype(&PerformanceCounters::create) Device::createPerformanceCountersFunc = PerformanceCounters::create;

DeviceVector::DeviceVector(const cl_device_id *devices,
                           cl_uint numDevices) {
    for (cl_uint i = 0; i < numDevices; i++) {
        this->push_back(castToObject<Device>(devices[i]));
    }
}

void DeviceVector::toDeviceIDs(std::vector<cl_device_id> &devIDs) {
    int i = 0;
    devIDs.resize(this->size());

    for (auto &it : *this) {
        devIDs[i] = it;
        i++;
    }
}

CommandStreamReceiver *createCommandStream(const HardwareInfo *pHwInfo);

// Global table of hardware prefixes
const char *hardwarePrefix[IGFX_MAX_PRODUCT] = {
    nullptr,
};
// Global table of family names
const char *familyName[IGFX_MAX_CORE] = {
    nullptr,
};
// Global table of family names
bool familyEnabled[IGFX_MAX_CORE] = {
    false,
};

Device::Device(const HardwareInfo &hwInfo,
               bool isRootDevice)
    : memoryManager(nullptr), enabledClVersion(false), hwInfo(hwInfo), isRoot(isRootDevice),
      commandStreamReceiver(nullptr), tagAddress(nullptr), tagAllocation(nullptr), preemptionAllocation(nullptr),
      osTime(nullptr), slmWindowStartAddress(nullptr) {
    memset(&deviceInfo, 0, sizeof(deviceInfo));
    deviceExtensions.reserve(1000);
    preemptionMode = DebugManager.flags.ForcePreemptionMode.get() == 0
                         ? hwInfo.capabilityTable.defaultPreemptionMode
                         : (PreemptionMode)DebugManager.flags.ForcePreemptionMode.get();
}

Device::~Device() {
    DEBUG_BREAK_IF(nullptr == memoryManager);
    if (performanceCounters) {
        performanceCounters->shutdown();
    }
    delete commandStreamReceiver;
    commandStreamReceiver = nullptr;
    if (memoryManager) {
        if (preemptionAllocation) {
            memoryManager->freeGraphicsMemory(preemptionAllocation);
            preemptionAllocation = nullptr;
        }
        memoryManager->waitForDeletions();

        memoryManager->freeGraphicsMemory(tagAllocation);
        alignedFree(this->slmWindowStartAddress);
    }
    tagAllocation = nullptr;
    delete memoryManager;
    memoryManager = nullptr;
}

bool Device::createDeviceImpl(const HardwareInfo *pHwInfo,
                              bool isRootDevice, Device &outDevice) {
    CommandStreamReceiver *commandStreamReceiver = createCommandStream(pHwInfo);
    if (!commandStreamReceiver) {
        return false;
    }

    outDevice.commandStreamReceiver = commandStreamReceiver;

    if (!outDevice.memoryManager) {
        outDevice.memoryManager = commandStreamReceiver->createMemoryManager(outDevice.deviceInfo.enabled64kbPages);
    } else {
        commandStreamReceiver->setMemoryManager(outDevice.memoryManager);
    }

    DEBUG_BREAK_IF(nullptr == outDevice.memoryManager);

    outDevice.memoryManager->csr = commandStreamReceiver;

    auto pTagAllocation = outDevice.memoryManager->allocateGraphicsMemory(
        sizeof(uint32_t), sizeof(uint32_t));
    if (!pTagAllocation) {
        return false;
    }
    auto pTagMemory = reinterpret_cast<uint32_t *>(pTagAllocation->getUnderlyingBuffer());
    // Initialize HW tag to a known value
    *pTagMemory = DebugManager.flags.EnableNullHardware.get() ? -1 : initialHardwareTag;

    commandStreamReceiver->setTagAllocation(pTagAllocation);

    auto pDevice = &outDevice;

    if (!pDevice->osTime) {
        pDevice->osTime = OSTime::create(commandStreamReceiver->getOSInterface());
    }
    pDevice->driverInfo.reset(DriverInfo::create(commandStreamReceiver->getOSInterface()));
    pDevice->memoryManager = outDevice.memoryManager;
    pDevice->tagAddress = pTagMemory;

    pDevice->initializeCaps();
    pDevice->tagAllocation = pTagAllocation;

    if (pDevice->osTime->getOSInterface()) {
        if (pHwInfo->capabilityTable.instrumentationEnabled) {
            pDevice->performanceCounters = createPerformanceCountersFunc(pDevice->osTime.get());
            pDevice->performanceCounters->initialize(pHwInfo);
        }
    }

    outDevice.memoryManager->setForce32BitAllocations(pDevice->getDeviceInfo().force32BitAddressess);
    outDevice.memoryManager->device = pDevice;

    if (pDevice->preemptionMode == PreemptionMode::MidThread) {
        size_t requiredSize = pHwInfo->pSysInfo->CsrSizeInMb * MemoryConstants::megaByte;
        size_t alignment = 256 * MemoryConstants::kiloByte;
        bool uncacheable = pDevice->getWaTable()->waCSRUncachable;
        pDevice->preemptionAllocation = outDevice.memoryManager->allocateGraphicsMemory(requiredSize, alignment, false, uncacheable);
        if (!pDevice->preemptionAllocation) {
            return false;
        }
        commandStreamReceiver->setPreemptionCsrAllocation(pDevice->preemptionAllocation);
    }

    return true;
}

const HardwareInfo *Device::getDeviceInitHwInfo(const HardwareInfo *pHwInfoIn) {
    return pHwInfoIn ? pHwInfoIn : platformDevices[0];
}

const HardwareInfo &Device::getHardwareInfo() const { return hwInfo; }

EngineType Device::getEngineType() const {
    return hwInfo.capabilityTable.defaultEngineType;
}

const WorkaroundTable *Device::getWaTable() const { return hwInfo.pWaTable; }

const DeviceInfo &Device::getDeviceInfo() const {
    return deviceInfo;
}

DeviceInfo *Device::getMutableDeviceInfo() {
    return &deviceInfo;
}

void *Device::getSLMWindowStartAddress() {
    prepareSLMWindow();
    return this->slmWindowStartAddress;
}

void Device::prepareSLMWindow() {
    if (this->slmWindowStartAddress == nullptr) {
        this->slmWindowStartAddress = memoryManager->allocateSystemMemory(MemoryConstants::slmWindowSize, MemoryConstants::slmWindowAlignment);
    }
}

const char *Device::getProductAbbrev() const {
    return hardwarePrefix[hwInfo.pPlatform->eProductFamily];
}

double Device::getProfilingTimerResolution() {
    return osTime->getDynamicDeviceTimerResolution(hwInfo);
}

unsigned int Device::getSupportedClVersion() const {
    return hwInfo.capabilityTable.clVersionSupport;
}

/* We hide the retain and release function of BaseObject. */
void Device::retain() {
    DEBUG_BREAK_IF(!isValid());

    /* According to CL spec, root devices are always available with
       1 reference. Only subdevices need reference. */
    if (!isRoot) {
        BaseObject<_cl_device_id>::retain();
    }
}

unique_ptr_if_unused<Device> Device::release() {
    DEBUG_BREAK_IF(!isValid());

    /* According to CL spec, root devices are always avaible with
       1 reference. Only subdevices need reference. */
    if (!isRoot) {
        return BaseObject<_cl_device_id>::release();
    }

    return unique_ptr_if_unused<Device>(this, false);
}

bool Device::isSimulation() {
    return hwInfo.capabilityTable.isSimulation(hwInfo.pPlatform->usDeviceID);
}

double Device::getPlatformHostTimerResolution() const {
    if (osTime.get())
        return osTime->getHostTimerResolution();
    return 0.0;
}
GFXCORE_FAMILY Device::getRenderCoreFamily() const {
    return this->getHardwareInfo().pPlatform->eRenderCoreFamily;
}
} // namespace OCLRT
