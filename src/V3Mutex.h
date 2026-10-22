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

#अगर_अघोषित VERILATOR_V3MUTEX_H_
#घोषणा VERILATOR_V3MUTEX_H_ 1

#समावेश "verilatedos.h"

#समावेश <cstdlib>
#समावेश <functional>
#समावेश <mutex>

#घोषणा VL_LOCK_SPINS 50000  /// Number of times to spin for a mutex before yielding

// MutexConfig class that allows to configure how mutex and lockguards behave
// once configured and locked, it cannot be changed. Configuration and lock needs to be
// done before starting any additional threads.
class V3MutexConfig final {
    // Allows to disable mutexes and lockguards.
    // Use carefully as it can cause undefined behavior when used inappropriately.
    // All mutexes needs to be unlocked.
    bool m_enable = false;  // Allows locking on mutexes, default don't lock mutexes
    bool m_lockConfig = false;  // After set, configuration cannot be changed

    V3MutexConfig() = शेष;
    ~V3MutexConfig() = शेष;

खुला:
    अटल V3MutexConfig& s() VL_MT_SAFE {
        अटल V3MutexConfig s{};
        वापस s;
    }

    // configures class
    व्योम configure(bool enable) VL_MT_UNSAFE {
        अगर (!s().m_lockConfig) {
            s().m_enable = enable;
            s().m_lockConfig = true;
        } अन्यथा {
            // requires <iostream>
            // avoided to reduce compile time
            // std::cerr << "%Error: V3Mutex already configured." << std::endl;
            std::पात();  // LCOV_EXCL_LINE
        }
    }
    bool enable() स्थिर VL_MT_SAFE { वापस m_enable; }
};

/// Mutex, wrapped to allow -fthread_safety checks
ढाँचा <typename T>
class VL_CAPABILITY("mutex") V3MutexImp final {
    T m_mutex;  // Mutex

खुला:
    /// Construct mutex (without locking it)
    V3MutexImp() = शेष;
    ~V3MutexImp() = शेष;
    VL_UNCOPYABLE(V3MutexImp);
    स्थिर V3MutexImp& चालक!() स्थिर { वापस *this; }  // For -fthread_safety
    /// Acquire/lock mutex
    व्योम lock() VL_ACQUIRE() VL_MT_SAFE {
        अगर (V3MutexConfig::s().enable()) {
            // Try to acquire the lock by spinning.  If the wait is short,
            // avoids a trap to the OS plus OS scheduler overhead.
            अगर (VL_LIKELY(try_lock())) वापस;  // Short circuit loop
            क्रम (पूर्णांक i = 0; i < VL_LOCK_SPINS; ++i) {
                अगर (VL_LIKELY(try_lock())) वापस;
                VL_CPU_RELAX();
            }
            // Spinning hasn't worked, pay the cost of blocking.
            m_mutex.lock();
        }
    }
    /// Release/unlock mutex
    व्योम unlock() VL_RELEASE() VL_MT_SAFE {
        अगर (V3MutexConfig::s().enable()) m_mutex.unlock();
    }
    /// Try to acquire mutex.  Returns true on success, and false on failure.
    bool try_lock() VL_TRY_ACQUIRE(true) VL_MT_SAFE {
        वापस V3MutexConfig::s().enable() ? m_mutex.try_lock() : true;
    }
    /// Assume that the mutex is already held. Purely for Clang thread safety analyzer.
    व्योम assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_SAFE {}
    /// Pretend that the mutex is being unlocked. Purely for Clang thread safety analyzer.
    व्योम pretendUnlock() VL_RELEASE() VL_MT_SAFE {}
};

/// Lock guard for mutex (ala std::unique_lock), wrapped to allow -fthread_safety checks
ढाँचा <typename T>
class VL_SCOPED_CAPABILITY V3LockGuardImp final {
    VL_UNCOPYABLE(V3LockGuardImp);

निजी:
    T& m_mutexr;

खुला:
    /// Lock given mutex and hold it for the object lifetime.
    explicit V3LockGuardImp(T& mutexr) VL_ACQUIRE(mutexr) VL_MT_SAFE
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
        mutexr.lock();
    }
    /// Take already locked mutex, and and hold the lock for the object lifetime.
    explicit V3LockGuardImp(T& mutexr, std::adopt_lock_t) VL_REQUIRES(mutexr) VL_MT_SAFE
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
    }

    /// Unlock the mutex
    ~V3LockGuardImp() VL_RELEASE() { m_mutexr.unlock(); }
};

using V3Mutex = V3MutexImp<std::mutex>;
using V3RecursiveMutex = V3MutexImp<std::recursive_mutex>;
using V3LockGuard = V3LockGuardImp<V3Mutex>;
using V3RecursiveLockGuard = V3LockGuardImp<V3RecursiveMutex>;

#पूर्ण_अगर  // guard
