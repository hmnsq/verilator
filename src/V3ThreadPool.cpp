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

V3ThreadPool::V3ThreadPool(صحيح عدد_تارونه) {
    عدد_تارونه = std::max(عدد_تارونه, 1);
    إذا (عدد_تارونه == 1) أرجع;
    لكل (صحيح i = 0; i < عدد_تارونه; ++i) {
        m_workers.emplace_شا(&V3ThreadPool::پیل_کارګر, هذا);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        m_shutdown = ریښتیا;
    }
    m_cv.notify_all();
    wait();
}

فراغ V3ThreadPool::کتار_ته_ورزیاتول(std::function<فراغ()>&& f) {
    إذا (m_workers.تشه()) {
        f();
    } وإلا {
        {
            ثابت V3_قلف_Guard قلف_2{m_mutex};
            m_کتار.ټېل(std::move(f));
        }
        m_pendingJobs.fetch_جمع_کړه(1, std::حافظه_ترتیب_release);
        m_cv.notify_one();
    }
}

فراغ V3ThreadPool::wait() {
    طالما (m_pendingJobs.بارول(std::حافظه_ترتیب_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    إذا (m_shutdown) {
        لكل (خپلکاری& کارګر : m_workers) کارګر.join();
    }
}

فراغ V3ThreadPool::پیل_کارګر(V3ThreadPool* selfThreadp) { selfThreadp->کارګر_دنده_Loop(); }

فراغ V3ThreadPool::کارګر_دنده_Loop() {
    طالما (ریښتیا) {
        std::function<فراغ()> دنده;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ثابت V3_قلف_Guard قلف_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { أرجع !m_کتار.تشه() || m_shutdown; });
            إذا (m_shutdown) أرجع;  // Terminate if requested
            UASSERT(!m_کتار.تشه(), "Job should be available");
            إذا (m_کتار.تشه()) استمر;
            دنده = std::move(m_کتار.front());
            m_کتار.راوباسه();
        }
        دنده();
        m_pendingJobs.fetch_sub(1, std::حافظه_ترتیب_release);
    }
}

فراغ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

فراغ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    صحيح common_ارزښت{0};

    خپلکاری لومړی_دنده = [&](صحيح نوم) -> فراغ {
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        ثابت V3_قلف_Guard قلف_2{commonMutex};
        common_ارزښت = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم + 10});
        UASSERT(common_ارزښت == 10, "unexpected commonValue = " << common_ارزښت);
    };
    خپلکاری دوهم_دنده = [&](صحيح نوم) -> فراغ {
        commonMutex.قلف_2();
        commonMutex.unlock();
        ثابت V3_قلف_Guard قلف_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        common_ارزښت = 1000;
    };
    خپلکاری دریم_دنده = [&](صحيح نوم) -> فراغ {
        {
            ثابت V3_قلف_Guard قلف_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{نوم});
        }
        لومړی_دنده(نوم);
        ثابت V3_قلف_Guard قلف_2{commonMutex};
        common_ارزښت = 100;
    };
    {
        V3ThreadScope scope;

        scope.کتار_ته_ورزیاتول(std::bind(لومړی_دنده, 100));
        scope.کتار_ته_ورزیاتول(std::bind(دوهم_دنده, 100));
        scope.کتار_ته_ورزیاتول(std::bind(لومړی_دنده, 100));
        scope.کتار_ته_ورزیاتول(std::bind(دوهم_دنده, 100));
        scope.کتار_ته_ورزیاتول(std::bind(دوهم_دنده, 200));
        scope.کتار_ته_ورزیاتول(std::bind(لومړی_دنده, 200));
        scope.کتار_ته_ورزیاتول(std::bind(لومړی_دنده, 300));
        scope.wait();

        UASSERT(common_ارزښت == 1000 || common_ارزښت == 10,
                "unexpected common value = " << common_ارزښت);

        scope.کتار_ته_ورزیاتول(std::bind(دریم_دنده, 100));
        scope.کتار_ته_ورزیاتول(std::bind(دریم_دنده, 100));
    }

    UASSERT(common_ارزښت == 100, "unexpected common value = " << common_ارزښت);

    {
        V3ThreadScope scope;
        scope.کتار_ته_ورزیاتول(std::bind(لومړی_دنده, 100));
    }

    UASSERT(common_ارزښت == 10, "unexpected common value = " << common_ارزښت);

    {
        صحيح پایله = 0;
        خپلکاری forth_دنده = [&]() -> فراغ { پایله = 1234; };

        V3ThreadScope scope;
        scope.کتار_ته_ورزیاتول(forth_دنده);
        scope.wait();
        UASSERT(پایله == 1234, "unexpected job result = " << پایله);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

فراغ V3ThreadScope::کتار_ته_ورزیاتول(std::function<فراغ()>&& f) { m_pool->کتار_ته_ورزیاتول(std::move(f)); }

فراغ V3ThreadScope::wait() { m_pool->wait(); }
