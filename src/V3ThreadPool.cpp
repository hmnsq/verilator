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

#ଅନ୍ତର୍ଭାଵ "V3ThreadPool.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Threads) {
    ଅଙ୍କ_Threads = std::max(ଅଙ୍କ_Threads, 1);
    ଯଦି (ଅଙ୍କ_Threads == 1) ଫେରନ୍ତୁ;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଅଙ୍କ_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, ଏହି);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ଅଚଳ V3LockGuard lock{m_mutex};
        m_shutdown = ସତ୍ୟ;
    }
    m_cv.notify_all();
    wait();
}

ଶୂନ୍ୟ V3ThreadPool::enqueue(std::function<ଶୂନ୍ୟ()>&& f) {
    ଯଦି (m_workers.empty()) {
        f();
    } ଅନ୍ୟଥା {
        {
            ଅଚଳ V3LockGuard lock{m_mutex};
            m_queue.ଠେଲ(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

ଶୂନ୍ୟ V3ThreadPool::wait() {
    ଯେତେବେଳେ (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    ଯଦି (m_shutdown) {
        ପାଇଁ (auto& worker : m_workers) worker.join();
    }
}

ଶୂନ୍ୟ V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

ଶୂନ୍ୟ V3ThreadPool::workerJobLoop() {
    ଯେତେବେଳେ (ସତ୍ୟ) {
        std::function<ଶୂନ୍ୟ()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ଅଚଳ V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ !m_queue.empty() || m_shutdown; });
            ଯଦି (m_shutdown) ଫେରନ୍ତୁ;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            ଯଦି (m_queue.empty()) ଜାରି;
            job = std::move(m_queue.front());
            m_queue.କାଢ଼();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

ଶୂନ୍ୟ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

ଶୂନ୍ୟ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା common_ମୂଲ୍ୟ{0};

    auto firstJob = [&](ପୂର୍ଣ୍ଣସଂଖ୍ୟା ସୁଷୁପ୍ତି) -> ଶୂନ୍ୟ {
        std::this_thread::sleep_for(std::chrono::milliseconds{ସୁଷୁପ୍ତି});
        ଅଚଳ V3LockGuard lock{commonMutex};
        common_ମୂଲ୍ୟ = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{ସୁଷୁପ୍ତି + 10});
        UASSERT(common_ମୂଲ୍ୟ == 10, "unexpected commonValue = " << common_ମୂଲ୍ୟ);
    };
    auto secondJob = [&](ପୂର୍ଣ୍ଣସଂଖ୍ୟା ସୁଷୁପ୍ତି) -> ଶୂନ୍ୟ {
        commonMutex.lock();
        commonMutex.unlock();
        ଅଚଳ V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{ସୁଷୁପ୍ତି});
        common_ମୂଲ୍ୟ = 1000;
    };
    auto thirdJob = [&](ପୂର୍ଣ୍ଣସଂଖ୍ୟା ସୁଷୁପ୍ତି) -> ଶୂନ୍ୟ {
        {
            ଅଚଳ V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{ସୁଷୁପ୍ତି});
        }
        firstJob(ସୁଷୁପ୍ତି);
        ଅଚଳ V3LockGuard lock{commonMutex};
        common_ମୂଲ୍ୟ = 100;
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

        UASSERT(common_ମୂଲ୍ୟ == 1000 || common_ମୂଲ୍ୟ == 10,
                "unexpected common value = " << common_ମୂଲ୍ୟ);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_ମୂଲ୍ୟ == 100, "unexpected common value = " << common_ମୂଲ୍ୟ);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_ମୂଲ୍ୟ == 10, "unexpected common value = " << common_ମୂଲ୍ୟ);

    {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା result = 0;
        auto forthJob = [&]() -> ଶୂନ୍ୟ { result = 1234; };

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

ଶୂନ୍ୟ V3ThreadScope::enqueue(std::function<ଶୂନ୍ୟ()>&& f) { m_pool->enqueue(std::move(f)); }

ଶୂନ୍ୟ V3ThreadScope::wait() { m_pool->wait(); }
