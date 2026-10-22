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

#അന്തര്ഭാവ "V3ThreadPool.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(പൂർണ്ണസംഖ്യ അക്കം_Threads) {
    അക്കം_Threads = std::max(അക്കം_Threads, 1);
    എങ്കിൽ (അക്കം_Threads == 1) തിരികെ;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < അക്കം_Threads; ++i) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, ഇത്);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        സ്ഥിരം V3LockGuard lock{m_mutex};
        m_shutdown = സത്യം;
    }
    m_cv.notify_all();
    wait();
}

ശൂന്യം V3ThreadPool::enqueue(std::function<ശൂന്യം()>&& f) {
    എങ്കിൽ (m_workers.empty()) {
        f();
    } അല്ലെങ്കിൽ {
        {
            സ്ഥിരം V3LockGuard lock{m_mutex};
            m_queue.തള്ളുക(std::move(f));
        }
        m_pendingJobs.fetch_add(1, std::memory_order_release);
        m_cv.notify_one();
    }
}

ശൂന്യം V3ThreadPool::wait() {
    ആകുമ്പോൾ (m_pendingJobs.load(std::memory_order_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    എങ്കിൽ (m_shutdown) {
        വേണ്ടി (auto& worker : m_workers) worker.join();
    }
}

ശൂന്യം V3ThreadPool::startWorker(V3ThreadPool* selfThreadp) { selfThreadp->workerJobLoop(); }

ശൂന്യം V3ThreadPool::workerJobLoop() {
    ആകുമ്പോൾ (സത്യം) {
        std::function<ശൂന്യം()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            സ്ഥിരം V3LockGuard lock{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { തിരികെ !m_queue.empty() || m_shutdown; });
            എങ്കിൽ (m_shutdown) തിരികെ;  // Terminate if requested
            UASSERT(!m_queue.empty(), "Job should be available");
            എങ്കിൽ (m_queue.empty()) തുടരുക;
            job = std::move(m_queue.front());
            m_queue.എടുക്കുക();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::memory_order_release);
    }
}

ശൂന്യം V3ThreadPool::selfTestMtDisabled() {
    // empty
}

ശൂന്യം V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    പൂർണ്ണസംഖ്യ common_മൂല്യം{0};

    auto firstJob = [&](പൂർണ്ണസംഖ്യ സുഷുപ്തി) -> ശൂന്യം {
        std::this_thread::sleep_for(std::chrono::milliseconds{സുഷുപ്തി});
        സ്ഥിരം V3LockGuard lock{commonMutex};
        common_മൂല്യം = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{സുഷുപ്തി + 10});
        UASSERT(common_മൂല്യം == 10, "unexpected commonValue = " << common_മൂല്യം);
    };
    auto secondJob = [&](പൂർണ്ണസംഖ്യ സുഷുപ്തി) -> ശൂന്യം {
        commonMutex.lock();
        commonMutex.unlock();
        സ്ഥിരം V3LockGuard lock{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{സുഷുപ്തി});
        common_മൂല്യം = 1000;
    };
    auto thirdJob = [&](പൂർണ്ണസംഖ്യ സുഷുപ്തി) -> ശൂന്യം {
        {
            സ്ഥിരം V3LockGuard lock{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{സുഷുപ്തി});
        }
        firstJob(സുഷുപ്തി);
        സ്ഥിരം V3LockGuard lock{commonMutex};
        common_മൂല്യം = 100;
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

        UASSERT(common_മൂല്യം == 1000 || common_മൂല്യം == 10,
                "unexpected common value = " << common_മൂല്യം);

        scope.enqueue(std::bind(thirdJob, 100));
        scope.enqueue(std::bind(thirdJob, 100));
    }

    UASSERT(common_മൂല്യം == 100, "unexpected common value = " << common_മൂല്യം);

    {
        V3ThreadScope scope;
        scope.enqueue(std::bind(firstJob, 100));
    }

    UASSERT(common_മൂല്യം == 10, "unexpected common value = " << common_മൂല്യം);

    {
        പൂർണ്ണസംഖ്യ result = 0;
        auto forthJob = [&]() -> ശൂന്യം { result = 1234; };

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

ശൂന്യം V3ThreadScope::enqueue(std::function<ശൂന്യം()>&& f) { m_pool->enqueue(std::move(f)); }

ശൂന്യം V3ThreadScope::wait() { m_pool->wait(); }
