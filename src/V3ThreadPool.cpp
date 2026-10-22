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

#অন্তর্ভাৱ "V3ThreadPool.h"

#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(পূর্ণাঙ্ক অঙ্ক_Threads) {
    অঙ্ক_Threads = std::max(অঙ্ক_Threads, 1);
    যদি (অঙ্ক_Threads == 1) ঘূৰাই;
    বাবে (পূর্ণাঙ্ক i = 0; i < অঙ্ক_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, এতত্);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ধ্রুৱ V3LockGuard lock{m_mutex};
        m_shutdown = সঁচা;
    }
    m_cv.notify_all();
    wait();
}

শূন্য V3ThreadPool::enqueue(std::function<শূন্য()>&& f) {
    যদি (m_workers.empty()) {
        f();
    } অন্যথা {
        {
            ধ্রুৱ V3LockGuard lock{m_mutex};
            m_queue.ঠেলো(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

শূন্য V3ThreadPool::wait() {
    যেতিয়ালৈ (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    যদি (m_shutdown) {
        বাবে (auto& worker : m_workers) worker.join();
    }
}

শূন্য V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

শূন্য V3ThreadPool::workerJobLoop() {
    যেতিয়ালৈ (সঁচা) {
        std::function<শূন্য()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ধ্রুৱ V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { ঘূৰাই !m_queue.empty() || m_shutdown; });
            যদি (m_shutdown) ঘূৰাই;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            যদি (m_queue.empty()) অনুৱর্ত;
            job = std::move(m_queue.front());
            m_queue.বেৰ_কৰো();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

শূন্য V3ThreadPool::selfTestMtDisabled() {
    // empty
}

শূন্য V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    পূর্ণাঙ্ক common_মান{0};

    auto firstJob = [&](পূর্ণাঙ্ক সুষুপ্তি) -> শূন্য {
        std::this_thread::sleep_for(std::chrono::milliseconds{সুষুপ্তি});
        ধ্রুৱ V3LockGuard lock{commonMutex};
        common_মান = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{সুষুপ্তি + 10});
        UASSERT(common_মান == 10, "unexpected commonValue = " << common_মান);
    };
    auto secondJob = [&](পূর্ণাঙ্ক সুষুপ্তি) -> শূন্য {
        commonMutex.lock();
        commonMutex.unlock();
        ধ্রুৱ V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{সুষুপ্তি});
        common_মান = 1000;
    };
    auto thirdJob = [&](পূর্ণাঙ্ক সুষুপ্তি) -> শূন্য {
        {
            ধ্রুৱ V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{সুষুপ্তি});
        }
        firstJob(সুষুপ্তি);
        ধ্রুৱ V3LockGuard lock{commonMutex};
        common_মান = 100;
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

        UASSERT(common_মান == 1000 || common_মান == 10,
                "unexpected common value = " << common_মান);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_মান == 100, "unexpected common value = " << common_মান);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_মান == 10, "unexpected common value = " << common_মান);

    {
        পূর্ণাঙ্ক result = 0;
        auto forthJob = [&]() -> শূন্য { result = 1234; };

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

শূন্য V3ThreadScope::enqueue(std::function<শূন্য()>&& f) { m_pool->enqueue(std::move(f)); }

শূন্য V3ThreadScope::wait() { m_pool->wait(); }
