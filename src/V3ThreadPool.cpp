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

#समावेश "V3ThreadPool.h"

#समावेश "V3Error.h"
#समावेश "V3Global.h"
#समावेश "V3Mutex.h"

V3ThreadPool::V3ThreadPool(पूर्णांक अंक_Threads) {
    अंक_Threads = std::max(अंक_Threads, 1);
    अगर (अंक_Threads == 1) वापस;
    क्रम (पूर्णांक i = 0; i < अंक_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, यह);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        स्थिर V3LockGuard lock{m_mutex};
        m_shutdown = true;
    }
    m_cv.notify_all();
    रुको();
}

व्योम V3ThreadPool::enqueue(std::function<व्योम()>&& f) {
    अगर (m_workers.empty()) {
        f();
    } अन्यथा {
        {
            स्थिर V3LockGuard lock{m_mutex};
            m_queue.धकेलो(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

व्योम V3ThreadPool::रुको() {
    जबतक (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    अगर (m_shutdown) {
        क्रम (स्वतः& worker : m_workers) worker.join();
    }
}

व्योम V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

व्योम V3ThreadPool::workerJobLoop() {
    जबतक (true) {
        std::function<व्योम()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            स्थिर V3LockGuard lock{m_mutex};
            m_cv.रुको(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { वापस !m_queue.empty() || m_shutdown; });
            अगर (m_shutdown) वापस;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            अगर (m_queue.empty()) जारी;
            job = std::move(m_queue.front());
            m_queue.निकालो();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

व्योम V3ThreadPool::selfTestMtDisabled() {
    // empty
}

व्योम V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    पूर्णांक common_मूल्य{0};

    स्वतः firstJob = [&](पूर्णांक sleep) -> व्योम {
        std::this_thread::sleep_for(std::chrono::milliseconds{sleep});
        स्थिर V3LockGuard lock{commonMutex};
        common_मूल्य = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{sleep + 10});
        UASSERT(common_मूल्य == 10, "unexpected commonValue = " << common_मूल्य);
    };
    स्वतः secondJob = [&](पूर्णांक sleep) -> व्योम {
        commonMutex.lock();
        commonMutex.unlock();
        स्थिर V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{sleep});
        common_मूल्य = 1000;
    };
    स्वतः thirdJob = [&](पूर्णांक sleep) -> व्योम {
        {
            स्थिर V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{sleep});
        }
        firstJob(sleep);
        स्थिर V3LockGuard lock{commonMutex};
        common_मूल्य = 100;
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
        scope.रुको();

        UASSERT(common_मूल्य == 1000 || common_मूल्य == 10,
                "unexpected common value = " << common_मूल्य);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_मूल्य == 100, "unexpected common value = " << common_मूल्य);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_मूल्य == 10, "unexpected common value = " << common_मूल्य);

    {
        पूर्णांक परिणाम = 0;
        स्वतः forthJob = [&]() -> व्योम { परिणाम = 1234; };

        V3ThreadScope scope;
        scope.enqueue(forthJob);
        scope.रुको();
        UASSERT(परिणाम == 1234, "unexpected job result = " << परिणाम);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    रुको();
}

व्योम V3ThreadScope::enqueue(std::function<व्योम()>&& f) { m_pool->enqueue(std::move(f)); }

व्योम V3ThreadScope::रुको() { m_pool->रुको(); }
