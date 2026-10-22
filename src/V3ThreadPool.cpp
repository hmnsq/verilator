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

#અન્તર્ભાવ "V3ThreadPool.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(પૂર્ણાંક અંક_Threads) {
    અંક_Threads = std::max(અંક_Threads, 1);
    જો (અંક_Threads == 1) પાછા;
    માટે (પૂર્ણાંક i = 0; i < અંક_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, આ);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        અચલ V3LockGuard lock{m_mutex};
        m_shutdown = સાચું;
    }
    m_cv.notify_all();
    wait();
}

શૂન્ય V3ThreadPool::enqueue(std::function<શૂન્ય()>&& f) {
    જો (m_workers.empty()) {
        f();
    } નહીંતર {
        {
            અચલ V3LockGuard lock{m_mutex};
            m_queue.ધકેલો(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

શૂન્ય V3ThreadPool::wait() {
    જ્યારે (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    જો (m_shutdown) {
        માટે (auto& worker : m_workers) worker.join();
    }
}

શૂન્ય V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

શૂન્ય V3ThreadPool::workerJobLoop() {
    જ્યારે (સાચું) {
        std::function<શૂન્ય()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            અચલ V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { પાછા !m_queue.empty() || m_shutdown; });
            જો (m_shutdown) પાછા;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            જો (m_queue.empty()) ચાલુ;
            job = std::move(m_queue.front());
            m_queue.કાઢો();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

શૂન્ય V3ThreadPool::selfTestMtDisabled() {
    // empty
}

શૂન્ય V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    પૂર્ણાંક common_કિંમત{0};

    auto firstJob = [&](પૂર્ણાંક સુષુપ્તિ) -> શૂન્ય {
        std::this_thread::sleep_for(std::chrono::milliseconds{સુષુપ્તિ});
        અચલ V3LockGuard lock{commonMutex};
        common_કિંમત = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{સુષુપ્તિ + 10});
        UASSERT(common_કિંમત == 10, "unexpected commonValue = " << common_કિંમત);
    };
    auto secondJob = [&](પૂર્ણાંક સુષુપ્તિ) -> શૂન્ય {
        commonMutex.lock();
        commonMutex.unlock();
        અચલ V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{સુષુપ્તિ});
        common_કિંમત = 1000;
    };
    auto thirdJob = [&](પૂર્ણાંક સુષુપ્તિ) -> શૂન્ય {
        {
            અચલ V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{સુષુપ્તિ});
        }
        firstJob(સુષુપ્તિ);
        અચલ V3LockGuard lock{commonMutex};
        common_કિંમત = 100;
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

        UASSERT(common_કિંમત == 1000 || common_કિંમત == 10,
                "unexpected common value = " << common_કિંમત);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_કિંમત == 100, "unexpected common value = " << common_કિંમત);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_કિંમત == 10, "unexpected common value = " << common_કિંમત);

    {
        પૂર્ણાંક result = 0;
        auto forthJob = [&]() -> શૂન્ય { result = 1234; };

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

શૂન્ય V3ThreadScope::enqueue(std::function<શૂન્ય()>&& f) { m_pool->enqueue(std::move(f)); }

શૂન્ય V3ThreadScope::wait() { m_pool->wait(); }
