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

V3ThreadPool::V3ThreadPool(पूर्णाङ्क अङ्क_Threads) {
    अङ्क_Threads = std::max(अङ्क_Threads, 1);
    यदि (अङ्क_Threads == 1) फर्काऊ;
    लागि (पूर्णाङ्क i = 0; i < अङ्क_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, एतत्);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        अचर V3LockGuard lock{m_mutex};
        m_shutdown = सत्य;
    }
    m_cv.notify_all();
    रुको();
}

शून्य V3ThreadPool::enqueue(std::function<शून्य()>&& f) {
    यदि (m_workers.empty()) {
        f();
    } अन्यथा {
        {
            अचर V3LockGuard lock{m_mutex};
            m_queue.थप(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

शून्य V3ThreadPool::रुको() {
    जबसम्म (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    यदि (m_shutdown) {
        लागि (स्वतः& worker : m_workers) worker.join();
    }
}

शून्य V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

शून्य V3ThreadPool::workerJobLoop() {
    जबसम्म (सत्य) {
        std::function<शून्य()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            अचर V3LockGuard lock{m_mutex};
            m_cv.रुको(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { फर्काऊ !m_queue.empty() || m_shutdown; });
            यदि (m_shutdown) फर्काऊ;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            यदि (m_queue.empty()) जारी;
            job = std::move(m_queue.front());
            m_queue.झिक();
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
    पूर्णाङ्क common_मान{0};

    स्वतः firstJob = [&](पूर्णाङ्क सुषुप्ति) -> शून्य {
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        अचर V3LockGuard lock{commonMutex};
        common_मान = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति + 10});
        UASSERT(common_मान == 10, "unexpected commonValue = " << common_मान);
    };
    स्वतः secondJob = [&](पूर्णाङ्क सुषुप्ति) -> शून्य {
        commonMutex.lock();
        commonMutex.unlock();
        अचर V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        common_मान = 1000;
    };
    स्वतः thirdJob = [&](पूर्णाङ्क सुषुप्ति) -> शून्य {
        {
            अचर V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        }
        firstJob(सुषुप्ति);
        अचर V3LockGuard lock{commonMutex};
        common_मान = 100;
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

        UASSERT(common_मान == 1000 || common_मान == 10,
                "unexpected common value = " << common_मान);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_मान == 100, "unexpected common value = " << common_मान);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_मान == 10, "unexpected common value = " << common_मान);

    {
        पूर्णाङ्क result = 0;
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
