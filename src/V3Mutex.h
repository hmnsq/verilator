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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3MUTEX_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3MUTEX_H_ 1

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstdlib>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <mutex>

#ವ್ಯಾಖ್ಯಾನಿಸು VL_ಬೀಗ_SPINS 50000  /// Number of times to spin for a mutex before yielding

// MutexConfig class that allows to configure how mutex and lockguards behave
// once configured and locked, it cannot be changed. Configuration and lock needs to be
// done before starting any additional threads.
class V3MutexConfig final {
    // Allows to disable mutexes and lockguards.
    // Use carefully as it can cause undefined behavior when used inappropriately.
    // All mutexes needs to be unlocked.
    bool m_enable = false;  // Allows locking on mutexes, default don't lock mutexes
    bool m_ಬೀಗ_Config = false;  // After set, configuration cannot be changed

    V3MutexConfig() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3MutexConfig() = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3MutexConfig& s() VL_MT_ಸುರಕ್ಷಿತ {
        ಸ್ಥಿರ V3MutexConfig s{};
        ಹಿಂತಿರುಗಿ s;
    }

    // configures class
    ಶೂನ್ಯ configure(bool enable) VL_MT_UNSAFE {
        ಆದರೆ (!s().m_ಬೀಗ_Config) {
            s().m_enable = enable;
            s().m_ಬೀಗ_Config = true;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // requires <iostream>
            // avoided to reduce compile time
            // std::cerr << "%Error: V3Mutex already configured." << std::endl;
            std::abort();  // LCOV_EXCL_LINE
        }
    }
    bool enable() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_enable; }
};

/// Mutex, wrapped to allow -fthread_safety checks
ಮಾದರಿ <typename T>
class VL_CAPABILITY("mutex") V3MutexImp final {
    T m_mutex;  // Mutex

ಸಾರ್ವಜನಿಕ:
    /// Construct mutex (without locking it)
    V3MutexImp() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3MutexImp() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(V3MutexImp);
    ಸ್ಥಿರಾಂಕ V3MutexImp& ಕ್ರಿಯಾಕಾರಕ!() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *this; }  // For -fthread_safety
    /// Acquire/lock mutex
    ಶೂನ್ಯ ಬೀಗ_2() VL_ACQUIRE() VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (V3MutexConfig::s().enable()) {
            // Try to acquire the lock by spinning.  If the wait is short,
            // avoids a trap to the OS plus OS scheduler overhead.
            ಆದರೆ (VL_LIKELY(try_ಬೀಗ())) ಹಿಂತಿರುಗಿ;  // Short circuit loop
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VL_ಬೀಗ_SPINS; ++i) {
                ಆದರೆ (VL_LIKELY(try_ಬೀಗ())) ಹಿಂತಿರುಗಿ;
                VL_CPU_RELAX();
            }
            // Spinning hasn't worked, pay the cost of blocking.
            m_mutex.ಬೀಗ_2();
        }
    }
    /// Release/unlock mutex
    ಶೂನ್ಯ unlock() VL_RELEASE() VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (V3MutexConfig::s().enable()) m_mutex.unlock();
    }
    /// Try to acquire mutex.  Returns true on success, and false on failure.
    bool try_ಬೀಗ() VL_TRY_ACQUIRE(true) VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ V3MutexConfig::s().enable() ? m_mutex.try_ಬೀಗ() : true;
    }
    /// Assume that the mutex is already held. Purely for Clang thread safety analyzer.
    ಶೂನ್ಯ assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_ಸುರಕ್ಷಿತ {}
    /// Pretend that the mutex is being unlocked. Purely for Clang thread safety analyzer.
    ಶೂನ್ಯ pretendUnlock() VL_RELEASE() VL_MT_ಸುರಕ್ಷಿತ {}
};

/// Lock guard for mutex (ala std::unique_lock), wrapped to allow -fthread_safety checks
ಮಾದರಿ <typename T>
class VL_SCOPED_CAPABILITY V3_ಬೀಗ_Guard_Imp final {
    VL_UNCOPYABLE(V3_ಬೀಗ_Guard_Imp);

ಖಾಸಗಿ:
    T& m_mutexr;

ಸಾರ್ವಜನಿಕ:
    /// Lock given mutex and hold it for the object lifetime.
    explicit V3_ಬೀಗ_Guard_Imp(T& mutexr) VL_ACQUIRE(mutexr) VL_MT_ಸುರಕ್ಷಿತ
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
        mutexr.ಬೀಗ_2();
    }
    /// Take already locked mutex, and and hold the lock for the object lifetime.
    explicit V3_ಬೀಗ_Guard_Imp(T& mutexr, std::adopt_ಬೀಗ_t) VL_REQUIRES(mutexr) VL_MT_ಸುರಕ್ಷಿತ
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
    }

    /// Unlock the mutex
    ~V3_ಬೀಗ_Guard_Imp() VL_RELEASE() { m_mutexr.unlock(); }
};

using V3Mutex = V3MutexImp<std::mutex>;
using V3RecursiveMutex = V3MutexImp<std::recursive_mutex>;
using V3_ಬೀಗ_Guard = V3_ಬೀಗ_Guard_Imp<V3Mutex>;
using V3_Recursive_ಬೀಗ_Guard = V3_ಬೀಗ_Guard_Imp<V3RecursiveMutex>;

#ಆದರೆ_ಅಂತ್ಯ  // guard
