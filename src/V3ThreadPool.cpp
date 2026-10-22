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

#شامل "V3ThreadPool.h"

#شامل "V3Error.h"
#شامل "V3Global.h"
#شامل "V3Mutex.h"

V3ThreadPool::V3ThreadPool(صحیح عدد_دھاگے) {
    عدد_دھاگے = std::max(عدد_دھاگے, 1);
    اگر (عدد_دھاگے == 1) واپس;
    برائے (صحیح i = 0; i < عدد_دھاگے; ++i) {
        m_workers.emplace_عقب(&V3ThreadPool::آغاز_کارکن, یہ);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        m_shutdown = سچ;
    }
    m_cv.notify_all();
    wait();
}

خالی V3ThreadPool::قطار_میں_ڈالو(std::function<خالی()>&& f) {
    اگر (m_workers.خالی_خانے()) {
        f();
    } ورنہ {
        {
            ثابت V3_تالا_Guard تالا_2{m_mutex};
            m_قطار.ڈالو(std::move(f));
        }
        m_pendingJobs.fetch_جوڑو(1, std::حافظہ_ترتیب_release);
        m_cv.notify_one();
    }
}

خالی V3ThreadPool::wait() {
    جب (m_pendingJobs.بھرو(std::حافظہ_ترتیب_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    اگر (m_shutdown) {
        برائے (خودکار& کارکن : m_workers) کارکن.join();
    }
}

خالی V3ThreadPool::آغاز_کارکن(V3ThreadPool* selfThreadp) { selfThreadp->کارکن_کام_Loop(); }

خالی V3ThreadPool::کارکن_کام_Loop() {
    جب (سچ) {
        std::function<خالی()> کام;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ثابت V3_تالا_Guard تالا_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { واپس !m_قطار.خالی_خانے() || m_shutdown; });
            اگر (m_shutdown) واپس;  // Terminate if requested
            UASSERT(!m_قطار.خالی_خانے(), "Job should be available");
            اگر (m_قطار.خالی_خانے()) جاری;
            کام = std::move(m_قطار.front());
            m_قطار.نکالو();
        }
        کام();
        m_pendingJobs.fetch_sub(1, std::حافظہ_ترتیب_release);
    }
}

خالی V3ThreadPool::selfTestMtDisabled() {
    // empty
}

خالی V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    صحیح common_قدر{0};

    خودکار پہلا_کام = [&](صحیح نوم) -> خالی {
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        ثابت V3_تالا_Guard تالا_2{commonMutex};
        common_قدر = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم + 10});
        UASSERT(common_قدر == 10, "unexpected commonValue = " << common_قدر);
    };
    خودکار دوسرا_کام = [&](صحیح نوم) -> خالی {
        commonMutex.تالا_2();
        commonMutex.unlock();
        ثابت V3_تالا_Guard تالا_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        common_قدر = 1000;
    };
    خودکار تیسرا_کام = [&](صحیح نوم) -> خالی {
        {
            ثابت V3_تالا_Guard تالا_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        }
        پہلا_کام(نوم);
        ثابت V3_تالا_Guard تالا_2{commonMutex};
        common_قدر = 100;
    };
    {
        V3ThreadScope scope;

        scope.قطار_میں_ڈالو(std::bind(پہلا_کام, 100));
        scope.قطار_میں_ڈالو(std::bind(دوسرا_کام, 100));
        scope.قطار_میں_ڈالو(std::bind(پہلا_کام, 100));
        scope.قطار_میں_ڈالو(std::bind(دوسرا_کام, 100));
        scope.قطار_میں_ڈالو(std::bind(دوسرا_کام, 200));
        scope.قطار_میں_ڈالو(std::bind(پہلا_کام, 200));
        scope.قطار_میں_ڈالو(std::bind(پہلا_کام, 300));
        scope.wait();

        UASSERT(common_قدر == 1000 || common_قدر == 10,
                "unexpected common value = " << common_قدر);

        scope.قطار_میں_ڈالو(std::bind(تیسرا_کام, 100));
        scope.قطار_میں_ڈالو(std::bind(تیسرا_کام, 100));
    }

    UASSERT(common_قدر == 100, "unexpected common value = " << common_قدر);

    {
        V3ThreadScope scope;
        scope.قطار_میں_ڈالو(std::bind(پہلا_کام, 100));
    }

    UASSERT(common_قدر == 10, "unexpected common value = " << common_قدر);

    {
        صحیح نتیجہ = 0;
        خودکار forth_کام = [&]() -> خالی { نتیجہ = 1234; };

        V3ThreadScope scope;
        scope.قطار_میں_ڈالو(forth_کام);
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

خالی V3ThreadScope::قطار_میں_ڈالو(std::function<خالی()>&& f) { m_pool->قطار_میں_ڈالو(std::move(f)); }

خالی V3ThreadScope::wait() { m_pool->wait(); }
