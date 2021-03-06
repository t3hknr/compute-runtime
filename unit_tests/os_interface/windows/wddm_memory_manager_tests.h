/*
 * Copyright (c) 2017, Intel Corporation
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

#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "test.h"

#include "unit_tests/fixtures/memory_management_fixture.h"
#include "unit_tests/mocks/mock_context.h"
#include "unit_tests/mocks/mock_gmm_page_table_mngr.h"
#include "unit_tests/mocks/mock_gmm.h"
#include "unit_tests/os_interface/windows/wddm_fixture.h"
#include "unit_tests/os_interface/windows/mock_gdi_interface.h"
#include "unit_tests/os_interface/windows/mock_wddm_memory_manager.h"

using namespace OCLRT;
using namespace ::testing;

class WddmMemoryManagerFixture : public MemoryManagementFixture, public WddmFixture {
  public:
    WddmMemoryManager *mm = nullptr;

    virtual void SetUp();

    template <typename FamiltyType>
    void SetUpMm() {
        WddmMock *mockWddm = static_cast<WddmMock *>(wddm);
        EXPECT_TRUE(wddm->init<FamiltyType>());
        uint64_t heap32Base = (uint64_t)(0x800000000000);
        if (sizeof(uintptr_t) == 4) {
            heap32Base = 0x1000;
        }
        mockWddm->setHeap32(heap32Base, 1000 * MemoryConstants::pageSize - 1);
        mm = new (std::nothrow) WddmMemoryManager(false, wddm);
        //assert we have memory manager
        ASSERT_NE(nullptr, mm);
    }

    virtual void TearDown() {
        delete mm;
        this->wddm = nullptr;
        WddmFixture::TearDown();
        MemoryManagementFixture::TearDown();
    }
};

typedef ::Test<WddmMemoryManagerFixture> WddmMemoryManagerTest;

class MockWddmMemoryManagerFixture : public WddmFixture {
  public:
    MockWddmMemoryManager *mm = nullptr;

    virtual void SetUp() {
        WddmFixture::SetUp(&gdi);
        ASSERT_NE(nullptr, wddm);
    }

    template <typename FamiltyType>
    void SetUpMm() {
        WddmMock *mockWddm = static_cast<WddmMock *>(wddm);
        EXPECT_TRUE(wddm->init<FamiltyType>());
        uint64_t heap32Base = (uint64_t)(0x800000000000);
        if (sizeof(uintptr_t) == 4) {
            heap32Base = 0x1000;
        }
        mockWddm->setHeap32(heap32Base, 1000 * MemoryConstants::pageSize - 1);
        mm = new (std::nothrow) MockWddmMemoryManager(wddm);
        //assert we have memory manager
        ASSERT_NE(nullptr, mm);
    }

    virtual void TearDown() {
        delete mm;
        this->wddm = nullptr;
        WddmFixture::TearDown();
    }
    MockGdi gdi;
};

typedef ::Test<MockWddmMemoryManagerFixture> WddmMemoryManagerResidencyTest;

class GmockWddm : public Wddm {
  public:
    using Wddm::device;

    GmockWddm() {
    }
    ~GmockWddm() = default;

    MOCK_METHOD4(makeResident, bool(D3DKMT_HANDLE *handles, uint32_t count, bool cantTrimFurther, uint64_t *numberOfBytesToTrim));
};

class WddmMemoryManagerFixtureWithGmockWddm {
  public:
    MockWddmMemoryManager *mm = nullptr;

    void SetUp() {
        // wddm is deleted by memory manager
        wddm = new GmockWddm;
        ASSERT_NE(nullptr, wddm);
    }

    template <typename FamiltyType>
    void SetUpMm() {
        wddm->init<FamiltyType>();
        mm = new (std::nothrow) MockWddmMemoryManager(wddm);
        //assert we have memory manager
        ASSERT_NE(nullptr, mm);
    }
    void TearDown() {
        delete mm;
        wddm = nullptr;
    }

    GmockWddm *wddm;
};

typedef ::Test<WddmMemoryManagerFixtureWithGmockWddm> WddmMemoryManagerTest2;

class BufferWithWddmMemory : public ::testing::Test,
                             public WddmMemoryManagerFixture {
  public:
  protected:
    void SetUp() {
        WddmMemoryManagerFixture::SetUp();
        tmp = context.getMemoryManager();
    }

    template <typename FamiltyType>
    void SetUpMm() {
        WddmMock *mockWddm = static_cast<WddmMock *>(wddm);
        EXPECT_TRUE(wddm->init<FamiltyType>());
        uint64_t heap32Base = (uint64_t)(0x800000000000);
        if (sizeof(uintptr_t) == 4) {
            heap32Base = 0x1000;
        }
        mockWddm->setHeap32(heap32Base, 1000 * MemoryConstants::pageSize - 1);
        mm = new (std::nothrow) WddmMemoryManager(false, wddm);
        //assert we have memory manager
        ASSERT_NE(nullptr, mm);
        context.setMemoryManager(mm);
        flags = 0;
    }

    void TearDown() {
        context.setMemoryManager(tmp);
        WddmMemoryManagerFixture::TearDown();
    }

    MemoryManager *tmp;
    MockContext context;
    cl_mem_flags flags;
    cl_int retVal;
};

typedef ::testing::Test MockWddmMemoryManagerTest;

typedef MockWddmMemoryManagerTest OsAgnosticMemoryManagerUsingWddmTest;
