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

V3ThreadPool::V3ThreadPool(صحيح عدد_دھاگے) {
    عدد_دھاگے = std::max(عدد_دھاگے, 1);
    إذا (عدد_دھاگے == 1) أرجع;
    لكل (صحيح i = 0; i < عدد_دھاگے; ++i) {
        m_workers.emplace_پٹھ(&V3ThreadPool::شروع_کامہ, هذا);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        m_shutdown = سچ;
    }
    m_cv.notify_all();
    wait();
}

فراغ V3ThreadPool::قطار_وچ_پاؤ(std::function<فراغ()>&& f) {
    إذا (m_workers.خالی_تھاواں()) {
        f();
    } وإلا {
        {
            ثابت V3_جندرا_Guard جندرا_2{m_mutex};
            m_قطار.ڈالو(std::move(f));
        }
        m_pendingJobs.fetch_جوڑو(1, std::حافظہ_ترتیب_release);
        m_cv.notify_one();
    }
}

فراغ V3ThreadPool::wait() {
    طالما (m_pendingJobs.لدو(std::حافظہ_ترتیب_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    إذا (m_shutdown) {
        لكل (خودکار& کامہ : m_workers) کامہ.join();
    }
}

فراغ V3ThreadPool::شروع_کامہ(V3ThreadPool* selfThreadp) { selfThreadp->کامہ_کم_Loop(); }

فراغ V3ThreadPool::کامہ_کم_Loop() {
    طالما (سچ) {
        std::function<فراغ()> کم_2;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ثابت V3_جندرا_Guard جندرا_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { أرجع !m_قطار.خالی_تھاواں() || m_shutdown; });
            إذا (m_shutdown) أرجع;  // Terminate if requested
            UASSERT(!m_قطار.خالی_تھاواں(), "Job should be available");
            إذا (m_قطار.خالی_تھاواں()) استمر;
            کم_2 = std::move(m_قطار.front());
            m_قطار.نکالو();
        }
        کم_2();
        m_pendingJobs.fetch_sub(1, std::حافظہ_ترتیب_release);
    }
}

فراغ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

فراغ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    صحيح common_قدر{0};

    خودکار پہلا_کم = [&](صحيح نوم) -> فراغ {
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        ثابت V3_جندرا_Guard جندرا_2{commonMutex};
        common_قدر = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم + 10});
        UASSERT(common_قدر == 10, "unexpected commonValue = " << common_قدر);
    };
    خودکار دوجا_کم = [&](صحيح نوم) -> فراغ {
        commonMutex.جندرا_2();
        commonMutex.unlock();
        ثابت V3_جندرا_Guard جندرا_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        common_قدر = 1000;
    };
    خودکار تیجا_کم = [&](صحيح نوم) -> فراغ {
        {
            ثابت V3_جندرا_Guard جندرا_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        }
        پہلا_کم(نوم);
        ثابت V3_جندرا_Guard جندرا_2{commonMutex};
        common_قدر = 100;
    };
    {
        V3ThreadScope scope;

        scope.قطار_وچ_پاؤ(std::bind(پہلا_کم, 100));
        scope.قطار_وچ_پاؤ(std::bind(دوجا_کم, 100));
        scope.قطار_وچ_پاؤ(std::bind(پہلا_کم, 100));
        scope.قطار_وچ_پاؤ(std::bind(دوجا_کم, 100));
        scope.قطار_وچ_پاؤ(std::bind(دوجا_کم, 200));
        scope.قطار_وچ_پاؤ(std::bind(پہلا_کم, 200));
        scope.قطار_وچ_پاؤ(std::bind(پہلا_کم, 300));
        scope.wait();

        UASSERT(common_قدر == 1000 || common_قدر == 10,
                "unexpected common value = " << common_قدر);

        scope.قطار_وچ_پاؤ(std::bind(تیجا_کم, 100));
        scope.قطار_وچ_پاؤ(std::bind(تیجا_کم, 100));
    }

    UASSERT(common_قدر == 100, "unexpected common value = " << common_قدر);

    {
        V3ThreadScope scope;
        scope.قطار_وچ_پاؤ(std::bind(پہلا_کم, 100));
    }

    UASSERT(common_قدر == 10, "unexpected common value = " << common_قدر);

    {
        صحيح نتیجہ = 0;
        خودکار forth_کم = [&]() -> فراغ { نتیجہ = 1234; };

        V3ThreadScope scope;
        scope.قطار_وچ_پاؤ(forth_کم);
        scope.wait();
        UASSERT(نتیجہ == 1234, "unexpected job result = " << نتیجہ);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

فراغ V3ThreadScope::قطار_وچ_پاؤ(std::function<فراغ()>&& f) { m_pool->قطار_وچ_پاؤ(std::move(f)); }

فراغ V3ThreadScope::wait() { m_pool->wait(); }
