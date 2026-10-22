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

#ضمن "V3ThreadPool.h"

#ضمن "V3Error.h"
#ضمن "V3Global.h"
#ضمن "V3Mutex.h"

V3ThreadPool::V3ThreadPool(صحيح رقم_Threads) {
    رقم_Threads = std::max(رقم_Threads, 1);
    إذا (رقم_Threads == 1) أرجع;
    لكل (صحيح i = 0; i < رقم_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, هذا);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ثابت V3LockGuard lock{m_mutex};
        m_shutdown = true;
    }
    m_cv.notify_all();
    wait();
}

فراغ V3ThreadPool::enqueue(std::function<فراغ()>&& f) {
    إذا (m_workers.empty()) {
        f();
    } وإلا {
        {
            ثابت V3LockGuard lock{m_mutex};
            m_queue.ادفع(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

فراغ V3ThreadPool::wait() {
    طالما (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    إذا (m_shutdown) {
        لكل (auto& worker : m_workers) worker.join();
    }
}

فراغ V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

فراغ V3ThreadPool::workerJobLoop() {
    طالما (true) {
        std::function<فراغ()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ثابت V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { أرجع !m_queue.empty() || m_shutdown; });
            إذا (m_shutdown) أرجع;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            إذا (m_queue.empty()) استمر;
            job = std::move(m_queue.front());
            m_queue.اسحب();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

فراغ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

فراغ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    صحيح common_قيمة{0};

    auto firstJob = [&](صحيح نوم) -> فراغ {
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        ثابت V3LockGuard lock{commonMutex};
        common_قيمة = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم + 10});
        UASSERT(common_قيمة == 10, "unexpected commonValue = " << common_قيمة);
    };
    auto secondJob = [&](صحيح نوم) -> فراغ {
        commonMutex.lock();
        commonMutex.unlock();
        ثابت V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        common_قيمة = 1000;
    };
    auto thirdJob = [&](صحيح نوم) -> فراغ {
        {
            ثابت V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        }
        firstJob(نوم);
        ثابت V3LockGuard lock{commonMutex};
        common_قيمة = 100;
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

        UASSERT(common_قيمة == 1000 || common_قيمة == 10,
                "unexpected common value = " << common_قيمة);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_قيمة == 100, "unexpected common value = " << common_قيمة);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_قيمة == 10, "unexpected common value = " << common_قيمة);

    {
        صحيح result = 0;
        auto forthJob = [&]() -> فراغ { result = 1234; };

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

فراغ V3ThreadScope::enqueue(std::function<فراغ()>&& f) { m_pool->enqueue(std::move(f)); }

فراغ V3ThreadScope::wait() { m_pool->wait(); }
