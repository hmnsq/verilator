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

#அந்தர்பாவ "V3ThreadPool.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(முழு இலக்கம்_Threads) {
    இலக்கம்_Threads = std::max(இலக்கம்_Threads, 1);
    எனில் (இலக்கம்_Threads == 1) திரும்பு;
    க்கு (முழு i = 0; i < இலக்கம்_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, இது);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        மாறிலி V3LockGuard lock{m_mutex};
        m_shutdown = உண்மை;
    }
    m_cv.notify_all();
    wait();
}

வெறுமை V3ThreadPool::enqueue(std::function<வெறுமை()>&& f) {
    எனில் (m_workers.empty()) {
        f();
    } இல்லையெனில் {
        {
            மாறிலி V3LockGuard lock{m_mutex};
            m_queue.தள்ளு(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

வெறுமை V3ThreadPool::wait() {
    போது (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    எனில் (m_shutdown) {
        க்கு (auto& worker : m_workers) worker.join();
    }
}

வெறுமை V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

வெறுமை V3ThreadPool::workerJobLoop() {
    போது (உண்மை) {
        std::function<வெறுமை()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            மாறிலி V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { திரும்பு !m_queue.empty() || m_shutdown; });
            எனில் (m_shutdown) திரும்பு;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            எனில் (m_queue.empty()) தொடர்;
            job = std::move(m_queue.front());
            m_queue.எடு();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

வெறுமை V3ThreadPool::selfTestMtDisabled() {
    // empty
}

வெறுமை V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    முழு common_மதிப்பு{0};

    auto firstJob = [&](முழு ஸுஷுப்தி) -> வெறுமை {
        std::this_thread::sleep_for(std::chrono::milliseconds{ஸுஷுப்தி});
        மாறிலி V3LockGuard lock{commonMutex};
        common_மதிப்பு = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{ஸுஷுப்தி + 10});
        UASSERT(common_மதிப்பு == 10, "unexpected commonValue = " << common_மதிப்பு);
    };
    auto secondJob = [&](முழு ஸுஷுப்தி) -> வெறுமை {
        commonMutex.lock();
        commonMutex.unlock();
        மாறிலி V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{ஸுஷுப்தி});
        common_மதிப்பு = 1000;
    };
    auto thirdJob = [&](முழு ஸுஷுப்தி) -> வெறுமை {
        {
            மாறிலி V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{ஸுஷுப்தி});
        }
        firstJob(ஸுஷுப்தி);
        மாறிலி V3LockGuard lock{commonMutex};
        common_மதிப்பு = 100;
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

        UASSERT(common_மதிப்பு == 1000 || common_மதிப்பு == 10,
                "unexpected common value = " << common_மதிப்பு);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_மதிப்பு == 100, "unexpected common value = " << common_மதிப்பு);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_மதிப்பு == 10, "unexpected common value = " << common_மதிப்பு);

    {
        முழு result = 0;
        auto forthJob = [&]() -> வெறுமை { result = 1234; };

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

வெறுமை V3ThreadScope::enqueue(std::function<வெறுமை()>&& f) { m_pool->enqueue(std::move(f)); }

வெறுமை V3ThreadScope::wait() { m_pool->wait(); }
