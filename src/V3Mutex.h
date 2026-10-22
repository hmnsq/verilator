// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Thread pool for Verilator itself
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
///
/// \file
/// \brief Verilator Mutex and LockGuard used only in verilator.
///
/// This file defines Mutex and LockGuard that is used in verilator
/// source code. It shouldn't be used by verilated code.
/// In contrast to VerilatedMutex that is used by verilated code,
/// this mutex can be configured and disabled when verilation is using
/// single thread.
///
/// This implementation also allows using different base mutex class in
/// wrapper V3Mutex class.
///
//*************************************************************************

#اگر_غیر_تعریف_شدہ VERILATOR_V3MUTEX_H_
#تعریف_کرو VERILATOR_V3MUTEX_H_ 1

#شامل "verilatedos.h"

#شامل <cstdlib>
#شامل <functional>
#شامل <mutex>

#تعریف_کرو VL_تالا_SPINS 50000  /// Number of times to spin for a mutex before yielding

// MutexConfig class that allows to configure how mutex and lockguards behave
// once configured and locked, it cannot be changed. Configuration and lock needs to be
// done before starting any additional threads.
class V3MutexConfig final {
    // Allows to disable mutexes and lockguards.
    // Use carefully as it can cause undefined behavior when used inappropriately.
    // All mutexes needs to be unlocked.
    bool m_enable = false;  // Allows locking on mutexes, default don't lock mutexes
    bool m_تالا_Config = false;  // After set, configuration cannot be changed

    V3MutexConfig() = طےشدہ;
    ~V3MutexConfig() = طےشدہ;

عوامی:
    ساکن V3MutexConfig& s() VL_MT_محفوظ {
        ساکن V3MutexConfig s{};
        واپس s;
    }

    // configures class
    خالی configure(bool enable) VL_MT_UNSAFE {
        اگر (!s().m_تالا_Config) {
            s().m_enable = enable;
            s().m_تالا_Config = true;
        } ورنہ {
            // requires <iostream>
            // avoided to reduce compile time
            // std::cerr << "%Error: V3Mutex already configured." << std::endl;
            std::abort();  // LCOV_EXCL_LINE
        }
    }
    bool enable() ثابت VL_MT_محفوظ { واپس m_enable; }
};

/// Mutex, wrapped to allow -fthread_safety checks
سانچہ <typename T>
class VL_CAPABILITY("mutex") V3MutexImp final {
    T m_mutex;  // Mutex

عوامی:
    /// Construct mutex (without locking it)
    V3MutexImp() = طےشدہ;
    ~V3MutexImp() = طےشدہ;
    VL_UNCOPYABLE(V3MutexImp);
    ثابت V3MutexImp& عامل!() ثابت { واپس *this; }  // For -fthread_safety
    /// Acquire/lock mutex
    خالی تالا_2() VL_ACQUIRE() VL_MT_محفوظ {
        اگر (V3MutexConfig::s().enable()) {
            // Try to acquire the lock by spinning.  If the wait is short,
            // avoids a trap to the OS plus OS scheduler overhead.
            اگر (VL_LIKELY(try_تالا())) واپس;  // Short circuit loop
            برائے (صحیح i = 0; i < VL_تالا_SPINS; ++i) {
                اگر (VL_LIKELY(try_تالا())) واپس;
                VL_CPU_RELAX();
            }
            // Spinning hasn't worked, pay the cost of blocking.
            m_mutex.تالا_2();
        }
    }
    /// Release/unlock mutex
    خالی unlock() VL_RELEASE() VL_MT_محفوظ {
        اگر (V3MutexConfig::s().enable()) m_mutex.unlock();
    }
    /// Try to acquire mutex.  Returns true on success, and false on failure.
    bool try_تالا() VL_TRY_ACQUIRE(true) VL_MT_محفوظ {
        واپس V3MutexConfig::s().enable() ? m_mutex.try_تالا() : true;
    }
    /// Assume that the mutex is already held. Purely for Clang thread safety analyzer.
    خالی assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_محفوظ {}
    /// Pretend that the mutex is being unlocked. Purely for Clang thread safety analyzer.
    خالی pretendUnlock() VL_RELEASE() VL_MT_محفوظ {}
};

/// Lock guard for mutex (ala std::unique_lock), wrapped to allow -fthread_safety checks
سانچہ <typename T>
class VL_SCOPED_CAPABILITY V3_تالا_Guard_Imp final {
    VL_UNCOPYABLE(V3_تالا_Guard_Imp);

نجی:
    T& m_mutexr;

عوامی:
    /// Lock given mutex and hold it for the object lifetime.
    explicit V3_تالا_Guard_Imp(T& mutexr) VL_ACQUIRE(mutexr) VL_MT_محفوظ
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
        mutexr.تالا_2();
    }
    /// Take already locked mutex, and and hold the lock for the object lifetime.
    explicit V3_تالا_Guard_Imp(T& mutexr, std::adopt_تالا_t) VL_REQUIRES(mutexr) VL_MT_محفوظ
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
    }

    /// Unlock the mutex
    ~V3_تالا_Guard_Imp() VL_RELEASE() { m_mutexr.unlock(); }
};

using V3Mutex = V3MutexImp<std::mutex>;
using V3RecursiveMutex = V3MutexImp<std::recursive_mutex>;
using V3_تالا_Guard = V3_تالا_Guard_Imp<V3Mutex>;
using V3_Recursive_تالا_Guard = V3_تالا_Guard_Imp<V3RecursiveMutex>;

#اگر_ختم  // guard
