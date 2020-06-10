/*
 * Copyright (C) 2017-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/helpers/constants.h"
#include "shared/source/helpers/flat_batch_buffer_helper_hw.inl"
#include "shared/source/helpers/hw_helper_bdw_plus.inl"

namespace NEO {
typedef BDWFamily Family;

static uint32_t slmSizeId[] = {0, 1, 2, 4, 4, 8, 8, 8, 8, 16, 16, 16, 16, 16, 16, 16};

template <>
uint32_t HwHelperHw<Family>::alignSlmSize(uint32_t slmSize) {
    if (slmSize == 0u) {
        return 0u;
    }
    slmSize = std::max(slmSize, 4096u);
    slmSize = Math::nextPowerOfTwo(slmSize);
    return slmSize;
}

template <>
uint32_t HwHelperHw<Family>::computeSlmValues(uint32_t slmSize) {
    slmSize += (4 * KB - 1);
    slmSize = slmSize >> 12;
    slmSize = std::min(slmSize, 15u);
    slmSize = slmSizeId[slmSize];
    return slmSize;
}

template <>
size_t HwHelperHw<Family>::getMaxBarrierRegisterPerSlice() const {
    return 16;
}

template <>
void HwHelperHw<Family>::setupHardwareCapabilities(HardwareCapabilities *caps, const HardwareInfo &hwInfo) {
    caps->image3DMaxHeight = 2048;
    caps->image3DMaxWidth = 2048;
    caps->maxMemAllocSize = 2 * MemoryConstants::gigaByte - 8 * MemoryConstants::megaByte;
    caps->isStatelesToStatefullWithOffsetSupported = false;
}

template <>
void MemorySynchronizationCommands<Family>::addPipeControl(LinearStream &commandStream, PipeControlArgs &args) {
    Family::PIPE_CONTROL cmd = Family::cmdInitPipeControl;
    args.dcFlushEnable = true;
    MemorySynchronizationCommands<Family>::setPipeControl(cmd, args);
    Family::PIPE_CONTROL *cmdBuffer = commandStream.getSpaceForCmd<Family::PIPE_CONTROL>();
    *cmdBuffer = cmd;
}

template class HwHelperHw<Family>;
template class FlatBatchBufferHelperHw<Family>;
template struct MemorySynchronizationCommands<Family>;
template struct LriHelper<Family>;
} // namespace NEO