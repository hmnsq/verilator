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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ThreadPool.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(ਪੂਰਨ ਅੰਕ_ਧਾਗੇ) {
    ਅੰਕ_ਧਾਗੇ = std::max(ਅੰਕ_ਧਾਗੇ, 1);
    ਜੇ (ਅੰਕ_ਧਾਗੇ == 1) ਵਾਪਸ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਅੰਕ_ਧਾਗੇ; ++i) {
        m_workers.emplace_ਪਿੱਠ(&V3ThreadPool::ਸ਼ੁਰੂ_ਕਾਮਾ, ਇਹ);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        m_shutdown = ਸੱਚ;
    }
    m_cv.notify_all();
    wait();
}

ਖਾਲੀ V3ThreadPool::ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::function<ਖਾਲੀ()>&& f) {
    ਜੇ (m_workers.ਖਾਲੀ_ਥਾਵਾਂ()) {
        f();
    } ਨਹੀਂਤਾਂ {
        {
            ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
            m_ਕਤਾਰ.ਧੱਕੋ(std::move(f));
        }
        m_pendingJobs.fetch_ਜੋੜੋ(1, std::ਸਮ੍ਰਿਤੀ_ਕ੍ਰਮ_release);
        m_cv.notify_one();
    }
}

ਖਾਲੀ V3ThreadPool::wait() {
    ਜਦੋਂ (m_pendingJobs.ਲੱਦੋ(std::ਸਮ੍ਰਿਤੀ_ਕ੍ਰਮ_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    ਜੇ (m_shutdown) {
        ਲਈ (ਸਵੈਚਾਲਿਤ& ਕਾਮਾ : m_workers) ਕਾਮਾ.join();
    }
}

ਖਾਲੀ V3ThreadPool::ਸ਼ੁਰੂ_ਕਾਮਾ(V3ThreadPool* selfThreadp) { selfThreadp->ਕਾਮਾ_ਕੰਮ_Loop(); }

ਖਾਲੀ V3ThreadPool::ਕਾਮਾ_ਕੰਮ_Loop() {
    ਜਦੋਂ (ਸੱਚ) {
        std::function<ਖਾਲੀ()> ਕੰਮ;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { ਵਾਪਸ !m_ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ() || m_shutdown; });
            ਜੇ (m_shutdown) ਵਾਪਸ;  // Terminate if requested
            UASSERT(!m_ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ(), "Job should be available");
            ਜੇ (m_ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਕੰਮ = std::move(m_ਕਤਾਰ.front());
            m_ਕਤਾਰ.ਕੱਢੋ();
        }
        ਕੰਮ();
        m_pendingJobs.fetch_sub(1, std::ਸਮ੍ਰਿਤੀ_ਕ੍ਰਮ_release);
    }
}

ਖਾਲੀ V3ThreadPool::selfTestMtDisabled() {
    // empty
}

ਖਾਲੀ V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    ਪੂਰਨ common_ਮੁੱਲ{0};

    ਸਵੈਚਾਲਿਤ ਪਹਿਲਾ_ਕੰਮ = [&](ਪੂਰਨ ਸੁਸ਼ੁਪ੍ਤਿ) -> ਖਾਲੀ {
        std::this_thread::sleep_for(std::chrono::milliseconds{ਸੁਸ਼ੁਪ੍ਤਿ});
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{commonMutex};
        common_ਮੁੱਲ = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{ਸੁਸ਼ੁਪ੍ਤਿ + 10});
        UASSERT(common_ਮੁੱਲ == 10, "unexpected commonValue = " << common_ਮੁੱਲ);
    };
    ਸਵੈਚਾਲਿਤ ਦੂਜਾ_ਕੰਮ = [&](ਪੂਰਨ ਸੁਸ਼ੁਪ੍ਤਿ) -> ਖਾਲੀ {
        commonMutex.ਤਾਲਾ_2();
        commonMutex.unlock();
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{ਸੁਸ਼ੁਪ੍ਤਿ});
        common_ਮੁੱਲ = 1000;
    };
    ਸਵੈਚਾਲਿਤ ਤੀਜਾ_ਕੰਮ = [&](ਪੂਰਨ ਸੁਸ਼ੁਪ੍ਤਿ) -> ਖਾਲੀ {
        {
            ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{ਸੁਸ਼ੁਪ੍ਤਿ});
        }
        ਪਹਿਲਾ_ਕੰਮ(ਸੁਸ਼ੁਪ੍ਤਿ);
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{commonMutex};
        common_ਮੁੱਲ = 100;
    };
    {
        V3ThreadScope scope;

        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਪਹਿਲਾ_ਕੰਮ, 100));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਦੂਜਾ_ਕੰਮ, 100));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਪਹਿਲਾ_ਕੰਮ, 100));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਦੂਜਾ_ਕੰਮ, 100));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਦੂਜਾ_ਕੰਮ, 200));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਪਹਿਲਾ_ਕੰਮ, 200));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਪਹਿਲਾ_ਕੰਮ, 300));
        scope.wait();

        UASSERT(common_ਮੁੱਲ == 1000 || common_ਮੁੱਲ == 10,
                "unexpected common value = " << common_ਮੁੱਲ);

        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਤੀਜਾ_ਕੰਮ, 100));
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਤੀਜਾ_ਕੰਮ, 100));
    }

    UASSERT(common_ਮੁੱਲ == 100, "unexpected common value = " << common_ਮੁੱਲ);

    {
        V3ThreadScope scope;
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::bind(ਪਹਿਲਾ_ਕੰਮ, 100));
    }

    UASSERT(common_ਮੁੱਲ == 10, "unexpected common value = " << common_ਮੁੱਲ);

    {
        ਪੂਰਨ ਨਤੀਜਾ = 0;
        ਸਵੈਚਾਲਿਤ forth_ਕੰਮ = [&]() -> ਖਾਲੀ { ਨਤੀਜਾ = 1234; };

        V3ThreadScope scope;
        scope.ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(forth_ਕੰਮ);
        scope.wait();
        UASSERT(ਨਤੀਜਾ == 1234, "unexpected job result = " << ਨਤੀਜਾ);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

ਖਾਲੀ V3ThreadScope::ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::function<ਖਾਲੀ()>&& f) { m_pool->ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::move(f)); }

ਖਾਲੀ V3ThreadScope::wait() { m_pool->wait(); }
