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

V3ThreadPool::V3ThreadPool(ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Threads) {
    ಅಂಕೆ_Threads = std::max(ಅಂಕೆ_Threads, 1);
    ಯದಿ (ಅಂಕೆ_Threads == 1) ಪ್ರತ್ಯಾಗಮ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಅಂಕೆ_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, ಏತತ್);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        m_shutdown = ಸತ್ಯ;
    }
    m_cv.notify_all();
    wait();
}

ಶೂನ್ಯ V3ThreadPool::enqueue(std::function<ಶೂನ್ಯ()>&& f) {
    ಯದಿ (m_workers.empty()) {
        f();
    } ಅನ್ಯಥಾ {
        {
            ಧ್ರುವ V3LockGuard lock{m_mutex};
            m_queue.ತಳ್ಳು(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

ಶೂನ್ಯ V3ThreadPool::wait() {
    ಯಾವತ್ (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    ಯದಿ (m_shutdown) {
        ಪ್ರತಿ (auto& worker : m_workers) worker.join();
    }
}

ಶೂನ್ಯ V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

ಶೂನ್ಯ V3ThreadPool::workerJobLoop() {
    ಯಾವತ್ (ಸತ್ಯ) {
        std::function<ಶೂನ್ಯ()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ಧ್ರುವ V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ !m_queue.empty() || m_shutdown; });
            ಯದಿ (m_shutdown) ಪ್ರತ್ಯಾಗಮ;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            ಯದಿ (m_queue.empty()) ಅನುವರ್ತ;
            job = std::move(m_queue.front());
            m_queue.ತೆಗೆ();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

ಶೂನ್ಯ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

ಶೂನ್ಯ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    ಪೂರ್ಣಾಙ್ಕ common_ಮೌಲ್ಯ{0};

    auto firstJob = [&](ಪೂರ್ಣಾಙ್ಕ ಸುಷುಪ್ತಿ) -> ಶೂನ್ಯ {
        std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ});
        ಧ್ರುವ V3LockGuard lock{commonMutex};
        common_ಮೌಲ್ಯ = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ + 10});
        UASSERT(common_ಮೌಲ್ಯ == 10, "unexpected commonValue = " << common_ಮೌಲ್ಯ);
    };
    auto secondJob = [&](ಪೂರ್ಣಾಙ್ಕ ಸುಷುಪ್ತಿ) -> ಶೂನ್ಯ {
        commonMutex.lock();
        commonMutex.unlock();
        ಧ್ರುವ V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ});
        common_ಮೌಲ್ಯ = 1000;
    };
    auto thirdJob = [&](ಪೂರ್ಣಾಙ್ಕ ಸುಷುಪ್ತಿ) -> ಶೂನ್ಯ {
        {
            ಧ್ರುವ V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{ಸುಷುಪ್ತಿ});
        }
        firstJob(ಸುಷುಪ್ತಿ);
        ಧ್ರುವ V3LockGuard lock{commonMutex};
        common_ಮೌಲ್ಯ = 100;
    };
    {
        V3ThreadScope scope;

        scope.enqueue(std::bind(firstJob, 100));
        scope.enqueue(std::bind(secondJob, 100));
        scope.enqueue(std::bind(firstJob, 100));
        scope.enqueue(std::bind(secondJob, 100));
        scope.enqueue(std::bind(secondJob, 200));
        scope.enqueue(std::bind(firstJob, 200));
        scope.enqueue(std::bind(firstJob, 300));
        scope.wait();

        UASSERT(common_ಮೌಲ್ಯ == 1000 || common_ಮೌಲ್ಯ == 10,
                "unexpected common value = " << common_ಮೌಲ್ಯ);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_ಮೌಲ್ಯ == 100, "unexpected common value = " << common_ಮೌಲ್ಯ);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_ಮೌಲ್ಯ == 10, "unexpected common value = " << common_ಮೌಲ್ಯ);

    {
        ಪೂರ್ಣಾಙ್ಕ result = 0;
        auto forthJob = [&]() -> ಶೂನ್ಯ { result = 1234; };

        V3ThreadScope scope;
        scope.enqueue(forthJob);
        scope.wait();
        UASSERT(result == 1234, "unexpected job result = " << result);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

ಶೂನ್ಯ V3ThreadScope::enqueue(std::function<ಶೂನ್ಯ()>&& f) { m_pool->enqueue(std::move(f)); }

ಶೂನ್ಯ V3ThreadScope::wait() { m_pool->wait(); }
