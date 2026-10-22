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

V3ThreadPool::V3ThreadPool(صحيح انگ_ڌاڳا) {
    انگ_ڌاڳا = std::max(انگ_ڌاڳا, 1);
    إذا (انگ_ڌاڳا == 1) أرجع;
    لكل (صحيح i = 0; i < انگ_ڌاڳا; ++i) {
        m_workers.emplace_پٺ(&V3ThreadPool::شروعات_ڪم_ڪندڙ, هذا);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        m_shutdown = سچ;
    }
    m_cv.notify_all();
    wait();
}

فراغ V3ThreadPool::قطار_اندر_وجهو(std::function<فراغ()>&& f) {
    إذا (m_workers.خالي_جايون()) {
        f();
    } وإلا {
        {
            ثابت V3_تالو_Guard تالو_2{m_mutex};
            m_قطار.ڌڪو(std::move(f));
        }
        m_pendingJobs.fetch_جوڙيو(1, std::يادگيري_ترتيب_release);
        m_cv.notify_one();
    }
}

فراغ V3ThreadPool::wait() {
    طالما (m_pendingJobs.ڀريو(std::يادگيري_ترتيب_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    إذا (m_shutdown) {
        لكل (خودڪار& ڪم_ڪندڙ : m_workers) ڪم_ڪندڙ.join();
    }
}

فراغ V3ThreadPool::شروعات_ڪم_ڪندڙ(V3ThreadPool* selfThreadp) { selfThreadp->ڪم_ڪندڙ_ڪم_Loop(); }

فراغ V3ThreadPool::ڪم_ڪندڙ_ڪم_Loop() {
    طالما (سچ) {
        std::function<فراغ()> ڪم;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ثابت V3_تالو_Guard تالو_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { أرجع !m_قطار.خالي_جايون() || m_shutdown; });
            إذا (m_shutdown) أرجع;  // Terminate if requested
            UASSERT(!m_قطار.خالي_جايون(), "Job should be available");
            إذا (m_قطار.خالي_جايون()) استمر;
            ڪم = std::move(m_قطار.front());
            m_قطار.ڪڍو();
        }
        ڪم();
        m_pendingJobs.fetch_sub(1, std::يادگيري_ترتيب_release);
    }
}

فراغ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

فراغ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    صحيح common_قدر{0};

    خودڪار پهريون_ڪم = [&](صحيح نوم) -> فراغ {
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        ثابت V3_تالو_Guard تالو_2{commonMutex};
        common_قدر = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم + 10});
        UASSERT(common_قدر == 10, "unexpected commonValue = " << common_قدر);
    };
    خودڪار ٻيو_ڪم = [&](صحيح نوم) -> فراغ {
        commonMutex.تالو_2();
        commonMutex.unlock();
        ثابت V3_تالو_Guard تالو_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        common_قدر = 1000;
    };
    خودڪار ٽيون_ڪم = [&](صحيح نوم) -> فراغ {
        {
            ثابت V3_تالو_Guard تالو_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        }
        پهريون_ڪم(نوم);
        ثابت V3_تالو_Guard تالو_2{commonMutex};
        common_قدر = 100;
    };
    {
        V3ThreadScope scope;

        scope.قطار_اندر_وجهو(std::bind(پهريون_ڪم, 100));
        scope.قطار_اندر_وجهو(std::bind(ٻيو_ڪم, 100));
        scope.قطار_اندر_وجهو(std::bind(پهريون_ڪم, 100));
        scope.قطار_اندر_وجهو(std::bind(ٻيو_ڪم, 100));
        scope.قطار_اندر_وجهو(std::bind(ٻيو_ڪم, 200));
        scope.قطار_اندر_وجهو(std::bind(پهريون_ڪم, 200));
        scope.قطار_اندر_وجهو(std::bind(پهريون_ڪم, 300));
        scope.wait();

        UASSERT(common_قدر == 1000 || common_قدر == 10,
                "unexpected common value = " << common_قدر);

        scope.قطار_اندر_وجهو(std::bind(ٽيون_ڪم, 100));
        scope.قطار_اندر_وجهو(std::bind(ٽيون_ڪم, 100));
    }

    UASSERT(common_قدر == 100, "unexpected common value = " << common_قدر);

    {
        V3ThreadScope scope;
        scope.قطار_اندر_وجهو(std::bind(پهريون_ڪم, 100));
    }

    UASSERT(common_قدر == 10, "unexpected common value = " << common_قدر);

    {
        صحيح نتيجو = 0;
        خودڪار forth_ڪم = [&]() -> فراغ { نتيجو = 1234; };

        V3ThreadScope scope;
        scope.قطار_اندر_وجهو(forth_ڪم);
        scope.wait();
        UASSERT(نتيجو == 1234, "unexpected job result = " << نتيجو);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

فراغ V3ThreadScope::قطار_اندر_وجهو(std::function<فراغ()>&& f) { m_pool->قطار_اندر_وجهو(std::move(f)); }

فراغ V3ThreadScope::wait() { m_pool->wait(); }
