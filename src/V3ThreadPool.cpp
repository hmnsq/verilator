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

#ಅನ್ತರ್ಭಾವ "V3ThreadPool.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಎಳೆಗಳು) {
    ಅಂಕೆ_ಎಳೆಗಳು = std::max(ಅಂಕೆ_ಎಳೆಗಳು, 1);
    ಆದರೆ (ಅಂಕೆ_ಎಳೆಗಳು == 1) ಹಿಂತಿರುಗಿ;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಅಂಕೆ_ಎಳೆಗಳು; ++i) {
        m_workers.emplace_ಹಿಂಭಾಗ(&V3ThreadPool::ಆರಂಭ_ಕೆಲಸಗಾರ, ಇದು);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        m_shutdown = ನಿಜ;
    }
    m_cv.notify_all();
    wait();
}

ಶೂನ್ಯ V3ThreadPool::ಸರತಿಗೆ_ಸೇರಿಸು(std::function<ಶೂನ್ಯ()>&& f) {
    ಆದರೆ (m_workers.ಖಾಲಿಗಳು()) {
        f();
    } ಇಲ್ಲದಿದ್ದರೆ {
        {
            ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
            m_ಸರತಿ.ತಳ್ಳು(std::move(f));
        }
        m_pendingJobs.fetch_ಕೂಡಿಸು(1, std::ಸ್ಮರಣೆ_ಕ್ರಮ_release);
        m_cv.notify_one();
    }
}

ಶೂನ್ಯ V3ThreadPool::wait() {
    ಆಗ (m_pendingJobs.ತುಂಬು(std::ಸ್ಮರಣೆ_ಕ್ರಮ_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    ಆದರೆ (m_shutdown) {
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& ಕೆಲಸಗಾರ : m_workers) ಕೆಲಸಗಾರ.join();
    }
}

ಶೂನ್ಯ V3ThreadPool::ಆರಂಭ_ಕೆಲಸಗಾರ(V3ThreadPool* selfThreadp) { selfThreadp->ಕೆಲಸಗಾರ_ಕೆಲಸ_Loop(); }

ಶೂನ್ಯ V3ThreadPool::ಕೆಲಸಗಾರ_ಕೆಲಸ_Loop() {
    ಆಗ (ನಿಜ) {
        std::function<ಶೂನ್ಯ()> ಕೆಲಸ;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ !m_ಸರತಿ.ಖಾಲಿಗಳು() || m_shutdown; });
            ಆದರೆ (m_shutdown) ಹಿಂತಿರುಗಿ;  // Terminate if requested
            UASSERT(!m_ಸರತಿ.ಖಾಲಿಗಳು(), "Job should be available");
            ಆದರೆ (m_ಸರತಿ.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಕೆಲಸ = std::move(m_ಸರತಿ.front());
            m_ಸರತಿ.ತೆಗೆ_2();
        }
        ಕೆಲಸ();
        m_pendingJobs.fetch_sub(1, std::ಸ್ಮರಣೆ_ಕ್ರಮ_release);
    }
}

ಶೂನ್ಯ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

ಶೂನ್ಯ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    ಪೂರ್ಣಾಂಕ common_ಮೌಲ್ಯ{0};

    ಸ್ವಯಂಚಾಲಿತ ಮೊದಲ_ಕೆಲಸ = [&](ಪೂರ್ಣಾಂಕ ಸುಷುಪ್ತಿ) -> ಶೂನ್ಯ {
        std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ});
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{commonMutex};
        common_ಮೌಲ್ಯ = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ + 10});
        UASSERT(common_ಮೌಲ್ಯ == 10, "unexpected commonValue = " << common_ಮೌಲ್ಯ);
    };
    ಸ್ವಯಂಚಾಲಿತ ಎರಡನೇ_ಕೆಲಸ = [&](ಪೂರ್ಣಾಂಕ ಸುಷುಪ್ತಿ) -> ಶೂನ್ಯ {
        commonMutex.ಬೀಗ_2();
        commonMutex.unlock();
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ});
        common_ಮೌಲ್ಯ = 1000;
    };
    ಸ್ವಯಂಚಾಲಿತ ಮೂರನೇ_ಕೆಲಸ = [&](ಪೂರ್ಣಾಂಕ ಸುಷುಪ್ತಿ) -> ಶೂನ್ಯ {
        {
            ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ});
        }
        ಮೊದಲ_ಕೆಲಸ(ಸುಷುಪ್ತಿ);
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{commonMutex};
        common_ಮೌಲ್ಯ = 100;
    };
    {
        V3ThreadScope scope;

        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೊದಲ_ಕೆಲಸ, 100));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಎರಡನೇ_ಕೆಲಸ, 100));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೊದಲ_ಕೆಲಸ, 100));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಎರಡನೇ_ಕೆಲಸ, 100));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಎರಡನೇ_ಕೆಲಸ, 200));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೊದಲ_ಕೆಲಸ, 200));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೊದಲ_ಕೆಲಸ, 300));
        scope.wait();

        UASSERT(common_ಮೌಲ್ಯ == 1000 || common_ಮೌಲ್ಯ == 10,
                "unexpected common value = " << common_ಮೌಲ್ಯ);

        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೂರನೇ_ಕೆಲಸ, 100));
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೂರನೇ_ಕೆಲಸ, 100));
    }

    UASSERT(common_ಮೌಲ್ಯ == 100, "unexpected common value = " << common_ಮೌಲ್ಯ);

    {
        V3ThreadScope scope;
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(std::bind(ಮೊದಲ_ಕೆಲಸ, 100));
    }

    UASSERT(common_ಮೌಲ್ಯ == 10, "unexpected common value = " << common_ಮೌಲ್ಯ);

    {
        ಪೂರ್ಣಾಂಕ ಫಲಿತಾಂಶ = 0;
        ಸ್ವಯಂಚಾಲಿತ forth_ಕೆಲಸ = [&]() -> ಶೂನ್ಯ { ಫಲಿತಾಂಶ = 1234; };

        V3ThreadScope scope;
        scope.ಸರತಿಗೆ_ಸೇರಿಸು(forth_ಕೆಲಸ);
        scope.wait();
        UASSERT(ಫಲಿತಾಂಶ == 1234, "unexpected job result = " << ಫಲಿತಾಂಶ);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

ಶೂನ್ಯ V3ThreadScope::ಸರತಿಗೆ_ಸೇರಿಸು(std::function<ಶೂನ್ಯ()>&& f) { m_pool->ಸರತಿಗೆ_ಸೇರಿಸು(std::move(f)); }

ಶೂನ್ಯ V3ThreadScope::wait() { m_pool->wait(); }
