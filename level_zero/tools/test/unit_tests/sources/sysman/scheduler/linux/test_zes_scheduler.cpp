/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/tools/test/unit_tests/sources/sysman/linux/mock_sysman_fixture.h"

#include "mock_sysfs_scheduler.h"

namespace L0 {
namespace ult {
constexpr uint32_t handleComponentCount = 1u;
constexpr uint64_t convertMilliToMicro = 1000u;
constexpr uint64_t defaultTimeoutMilliSecs = 650u;
constexpr uint64_t defaultTimesliceMilliSecs = 1u;
constexpr uint64_t defaultHeartbeatMilliSecs = 3000u;
constexpr uint64_t timeoutMilliSecs = 640u;
constexpr uint64_t timesliceMilliSecs = 1u;
constexpr uint64_t heartbeatMilliSecs = 2500u;
constexpr uint64_t expectedDefaultHeartbeatTimeoutMicroSecs = defaultHeartbeatMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedDefaultTimeoutMicroSecs = defaultTimeoutMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedDefaultTimesliceMicroSecs = defaultTimesliceMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedHeartbeatTimeoutMicroSecs = heartbeatMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedTimeoutMicroSecs = timeoutMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedTimesliceMicroSecs = timesliceMilliSecs * convertMilliToMicro;
class SysmanDeviceSchedulerFixture : public SysmanDeviceFixture {

  protected:
    std::unique_ptr<Mock<SchedulerSysfsAccess>> pSysfsAccess;
    SysfsAccess *pSysfsAccessOld = nullptr;

    void SetUp() override {
        SysmanDeviceFixture::SetUp();
        pSysfsAccessOld = pLinuxSysmanImp->pSysfsAccess;
        pSysfsAccess = std::make_unique<NiceMock<Mock<SchedulerSysfsAccess>>>();
        pLinuxSysmanImp->pSysfsAccess = pSysfsAccess.get();
        pSysfsAccess->setVal(defaultPreemptTimeoutMilliSecs, defaultTimeoutMilliSecs);
        pSysfsAccess->setVal(defaultTimesliceDurationMilliSecs, defaultTimesliceMilliSecs);
        pSysfsAccess->setVal(defaultHeartbeatIntervalMilliSecs, defaultHeartbeatMilliSecs);
        pSysfsAccess->setVal(preemptTimeoutMilliSecs, timeoutMilliSecs);
        pSysfsAccess->setVal(timesliceDurationMilliSecs, timesliceMilliSecs);
        pSysfsAccess->setVal(heartbeatIntervalMilliSecs, heartbeatMilliSecs);

        ON_CALL(*pSysfsAccess.get(), read(_, _))
            .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::getVal));
        ON_CALL(*pSysfsAccess.get(), write(_, _))
            .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::setVal));
        ON_CALL(*pSysfsAccess.get(), canRead(_))
            .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::getCanReadStatus));

        pSysmanDeviceImp->pSchedulerHandleContext->init();
    }

    void TearDown() override {
        SysmanDeviceFixture::TearDown();
        pLinuxSysmanImp->pSysfsAccess = pSysfsAccessOld;
    }

    std::vector<zes_sched_handle_t> get_sched_handles(uint32_t count) {
        std::vector<zes_sched_handle_t> handles(count, nullptr);
        EXPECT_EQ(zesDeviceEnumSchedulers(device->toHandle(), &count, handles.data()), ZE_RESULT_SUCCESS);
        return handles;
    }

    zes_sched_mode_t fixtureGetCurrentMode(zes_sched_handle_t hScheduler) {
        zes_sched_mode_t mode;
        ze_result_t result = zesSchedulerGetCurrentMode(hScheduler, &mode);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        return mode;
    }

    zes_sched_timeout_properties_t fixtureGetTimeoutModeProperties(zes_sched_handle_t hScheduler, ze_bool_t getDefaults) {
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetTimeoutModeProperties(hScheduler, getDefaults, &config);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        return config;
    }

    zes_sched_timeslice_properties_t fixtureGetTimesliceModeProperties(zes_sched_handle_t hScheduler, ze_bool_t getDefaults) {
        zes_sched_timeslice_properties_t config;
        ze_result_t result = zesSchedulerGetTimesliceModeProperties(hScheduler, getDefaults, &config);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        return config;
    }
};

TEST_F(SysmanDeviceSchedulerFixture, GivenComponentCountZeroWhenCallingzesDeviceEnumSchedulersAndSysfsCanReadReturnsErrorThenZeroCountIsReturned) {
    ON_CALL(*pSysfsAccess.get(), canRead(_))
        .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::getCanReadStatusReturnError));

    auto pSchedulerHandleContextTest = std::make_unique<SchedulerHandleContext>(pOsSysman);
    pSchedulerHandleContextTest->init();
    EXPECT_EQ(0u, static_cast<uint32_t>(pSchedulerHandleContextTest->handleList.size()));
}

TEST_F(SysmanDeviceSchedulerFixture, GivenComponentCountZeroWhenCallingzesDeviceEnumSchedulersThenNonZeroCountIsReturnedAndVerifyCallSucceeds) {
    uint32_t count = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumSchedulers(device->toHandle(), &count, NULL));
    EXPECT_EQ(count, handleComponentCount);

    uint32_t testcount = count + 1;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumSchedulers(device->toHandle(), &testcount, NULL));
    EXPECT_EQ(testcount, count);

    count = 0;
    std::vector<zes_sched_handle_t> handles(count, nullptr);
    EXPECT_EQ(zesDeviceEnumSchedulers(device->toHandle(), &count, handles.data()), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, handleComponentCount);
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetCurrentModeThenVerifyzesSchedulerGetCurrentModeCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_TIMESLICE);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesThenVerifyzesSchedulerGetTimeoutModePropertiesCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimeoutModeProperties(handle, false);
        EXPECT_EQ(config.watchdogTimeout, expectedHeartbeatTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesWithDefaultsThenVerifyzesSchedulerGetTimeoutModePropertiesCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimeoutModeProperties(handle, true);
        EXPECT_EQ(config.watchdogTimeout, expectedDefaultHeartbeatTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimesliceModePropertiesThenVerifyzesSchedulerGetTimesliceModePropertiesCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimesliceModeProperties(handle, false);
        EXPECT_EQ(config.interval, expectedTimesliceMicroSecs);
        EXPECT_EQ(config.yieldTimeout, expectedTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimesliceModePropertiesWithDefaultsThenVerifyzesSchedulerGetTimesliceModePropertiesCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimesliceModeProperties(handle, true);
        EXPECT_EQ(config.interval, expectedDefaultTimesliceMicroSecs);
        EXPECT_EQ(config.yieldTimeout, expectedDefaultTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeThenVerifyzesSchedulerSetTimeoutModeCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setConfig;
        setConfig.watchdogTimeout = 10000u;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_FALSE(needReboot);
        auto getConfig = fixtureGetTimeoutModeProperties(handle, false);
        EXPECT_EQ(getConfig.watchdogTimeout, setConfig.watchdogTimeout);
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_TIMEOUT);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeThenVerifyzesSchedulerSetTimesliceModeCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = 1000u;
        setConfig.yieldTimeout = 1000u;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_FALSE(needReboot);
        auto getConfig = fixtureGetTimesliceModeProperties(handle, false);
        EXPECT_EQ(getConfig.interval, setConfig.interval);
        EXPECT_EQ(getConfig.yieldTimeout, setConfig.yieldTimeout);
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_TIMESLICE);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetExclusiveModeThenVerifyzesSchedulerSetExclusiveModeCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        ze_result_t result = zesSchedulerSetExclusiveMode(handle, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_FALSE(needReboot);
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_EXCLUSIVE);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetCurrentModeWhenSysfsNodeIsAbsentThenFailureIsReturned) {
    ON_CALL(*pSysfsAccess.get(), read(_, _))
        .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::getValForError));
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_mode_t mode;
        ze_result_t result = zesSchedulerGetCurrentMode(handle, &mode);
        EXPECT_EQ(ZE_RESULT_ERROR_NOT_AVAILABLE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesWithDefaultsWhenSysfsNodeIsAbsentThenFailureIsReturned) {
    ON_CALL(*pSysfsAccess.get(), read(_, _))
        .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::getValForError));
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetTimeoutModeProperties(handle, true, &config);
        EXPECT_EQ(ZE_RESULT_ERROR_NOT_AVAILABLE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimesliceModePropertiesWithDefaultsWhenSysfsNodeIsAbsentThenFailureIsReturned) {
    ON_CALL(*pSysfsAccess.get(), read(_, _))
        .WillByDefault(::testing::Invoke(pSysfsAccess.get(), &Mock<SchedulerSysfsAccess>::getValForError));
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_timeslice_properties_t config;
        ze_result_t result = zesSchedulerGetTimesliceModeProperties(handle, true, &config);
        EXPECT_EQ(ZE_RESULT_ERROR_NOT_AVAILABLE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetPropertiesThenVerifyzesSchedulerGetPropertiesCallSucceeds) {
    auto handles = get_sched_handles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_TRUE(properties.canControl);
        EXPECT_EQ(properties.engines, ZES_ENGINE_TYPE_FLAG_COMPUTE);
        EXPECT_EQ(properties.supportedModes, static_cast<uint32_t>((1 << ZES_SCHED_MODE_TIMEOUT) | (1 << ZES_SCHED_MODE_TIMESLICE) | (1 << ZES_SCHED_MODE_EXCLUSIVE)));
    }
}

} // namespace ult
} // namespace L0