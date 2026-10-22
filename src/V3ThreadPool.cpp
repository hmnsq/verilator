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

#अन्तर्भाव "V3ThreadPool.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Mutex.h"

V3ThreadPool::V3ThreadPool(पूर्णांक अंक_Threads) {
    अंक_Threads = std::max(अंक_Threads, 1);
    जर (अंक_Threads == 1) परत;
    साठी (पूर्णांक i = 0; i < अंक_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, हे);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        स्थिरांक V3LockGuard lock{m_mutex};
        m_shutdown = सत्य;
    }
    m_cv.notify_all();
    रुको();
}

शून्य V3ThreadPool::enqueue(std::function<शून्य()>&& f) {
    जर (m_workers.empty()) {
        f();
    } अन्यथा {
        {
            स्थिरांक V3LockGuard lock{m_mutex};
            m_queue.ढकला(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

शून्य V3ThreadPool::रुको() {
    पर्यंत (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    जर (m_shutdown) {
        साठी (स्वतः& worker : m_workers) worker.join();
    }
}

शून्य V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

शून्य V3ThreadPool::workerJobLoop() {
    पर्यंत (सत्य) {
        std::function<शून्य()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            स्थिरांक V3LockGuard lock{m_mutex};
            m_cv.रुको(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { परत !m_queue.empty() || m_shutdown; });
            जर (m_shutdown) परत;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            जर (m_queue.empty()) सुरूठेवा;
            job = std::move(m_queue.front());
            m_queue.काढा();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

शून्य V3ThreadPool::selfTestMtDisabled() {
    // empty
}

शून्य V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    पूर्णांक common_मूल्य{0};

    स्वतः firstJob = [&](पूर्णांक सुषुप्ति) -> शून्य {
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        स्थिरांक V3LockGuard lock{commonMutex};
        common_मूल्य = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति + 10});
        UASSERT(common_मूल्य == 10, "unexpected commonValue = " << common_मूल्य);
    };
    स्वतः secondJob = [&](पूर्णांक सुषुप्ति) -> शून्य {
        commonMutex.lock();
        commonMutex.unlock();
        स्थिरांक V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        common_मूल्य = 1000;
    };
    स्वतः thirdJob = [&](पूर्णांक सुषुप्ति) -> शून्य {
        {
            स्थिरांक V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        }
        firstJob(सुषुप्ति);
        स्थिरांक V3LockGuard lock{commonMutex};
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
        पूर्णांक result = 0;
        स्वतः forthJob = [&]() -> शून्य { result = 1234; };

        V3ThreadScope scope;
        scope.enqueue(forthJob);
        scope.रुको();
        UASSERT(result == 1234, "unexpected job result = " << result);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    रुको();
}

शून्य V3ThreadScope::enqueue(std::function<शून्य()>&& f) { m_pool->enqueue(std::move(f)); }

शून्य V3ThreadScope::रुको() { m_pool->रुको(); }
