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

#که_نه_تعریف_شوی VERILATOR_V3MUTEX_H_
#تعریف_کړه VERILATOR_V3MUTEX_H_ 1

#ضمن "verilatedos.h"

#ضمن <cstdlib>
#ضمن <functional>
#ضمن <mutex>

#تعریف_کړه VL_قلف_SPINS 50000  /// Number of times to spin for a mutex before yielding

// MutexConfig class that allows to configure how mutex and lockguards behave
// once configured and locked, it cannot be changed. Configuration and lock needs to be
// done before starting any additional threads.
class V3MutexConfig final {
    // Allows to disable mutexes and lockguards.
    // Use carefully as it can cause undefined behavior when used inappropriately.
    // All mutexes needs to be unlocked.
    bool m_enable = false;  // Allows locking on mutexes, default don't lock mutexes
    bool m_قلف_Config = false;  // After set, configuration cannot be changed

    V3MutexConfig() = افتراضي;
    ~V3MutexConfig() = افتراضي;

عام:
    ساكن V3MutexConfig& s() VL_MT_خوندي {
        ساكن V3MutexConfig s{};
        أرجع s;
    }

    // configures class
    فراغ configure(bool enable) VL_MT_UNSAFE {
        إذا (!s().m_قلف_Config) {
            s().m_enable = enable;
            s().m_قلف_Config = true;
        } وإلا {
            // requires <iostream>
            // avoided to reduce compile time
            // std::cerr << "%Error: V3Mutex already configured." << std::endl;
            std::abort();  // LCOV_EXCL_LINE
        }
    }
    bool enable() ثابت VL_MT_خوندي { أرجع m_enable; }
};

/// Mutex, wrapped to allow -fthread_safety checks
کينډۍ <typename T>
class VL_CAPABILITY("mutex") V3MutexImp final {
    T m_mutex;  // Mutex

عام:
    /// Construct mutex (without locking it)
    V3MutexImp() = افتراضي;
    ~V3MutexImp() = افتراضي;
    VL_UNCOPYABLE(V3MutexImp);
    ثابت V3MutexImp& چلوونکی!() ثابت { أرجع *this; }  // For -fthread_safety
    /// Acquire/lock mutex
    فراغ قلف_2() VL_ACQUIRE() VL_MT_خوندي {
        إذا (V3MutexConfig::s().enable()) {
            // Try to acquire the lock by spinning.  If the wait is short,
            // avoids a trap to the OS plus OS scheduler overhead.
            إذا (VL_LIKELY(try_قلف())) أرجع;  // Short circuit loop
            لكل (صحيح i = 0; i < VL_قلف_SPINS; ++i) {
                إذا (VL_LIKELY(try_قلف())) أرجع;
                VL_CPU_RELAX();
            }
            // Spinning hasn't worked, pay the cost of blocking.
            m_mutex.قلف_2();
        }
    }
    /// Release/unlock mutex
    فراغ unlock() VL_RELEASE() VL_MT_خوندي {
        إذا (V3MutexConfig::s().enable()) m_mutex.unlock();
    }
    /// Try to acquire mutex.  Returns true on success, and false on failure.
    bool try_قلف() VL_TRY_ACQUIRE(true) VL_MT_خوندي {
        أرجع V3MutexConfig::s().enable() ? m_mutex.try_قلف() : true;
    }
    /// Assume that the mutex is already held. Purely for Clang thread safety analyzer.
    فراغ assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_خوندي {}
    /// Pretend that the mutex is being unlocked. Purely for Clang thread safety analyzer.
    فراغ pretendUnlock() VL_RELEASE() VL_MT_خوندي {}
};

/// Lock guard for mutex (ala std::unique_lock), wrapped to allow -fthread_safety checks
کينډۍ <typename T>
class VL_SCOPED_CAPABILITY V3_قلف_Guard_Imp final {
    VL_UNCOPYABLE(V3_قلف_Guard_Imp);

خاص:
    T& m_mutexr;

عام:
    /// Lock given mutex and hold it for the object lifetime.
    explicit V3_قلف_Guard_Imp(T& mutexr) VL_ACQUIRE(mutexr) VL_MT_خوندي
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
        mutexr.قلف_2();
    }
    /// Take already locked mutex, and and hold the lock for the object lifetime.
    explicit V3_قلف_Guard_Imp(T& mutexr, std::adopt_قلف_t) VL_REQUIRES(mutexr) VL_MT_خوندي
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
    }

    /// Unlock the mutex
    ~V3_قلف_Guard_Imp() VL_RELEASE() { m_mutexr.unlock(); }
};

using V3Mutex = V3MutexImp<std::mutex>;
using V3RecursiveMutex = V3MutexImp<std::recursive_mutex>;
using V3_قلف_Guard = V3_قلف_Guard_Imp<V3Mutex>;
using V3_Recursive_قلف_Guard = V3_قلف_Guard_Imp<V3RecursiveMutex>;

#د_که_پای  // guard
