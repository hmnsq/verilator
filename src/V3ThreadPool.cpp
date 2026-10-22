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

#అన్తర్భావ "V3ThreadPool.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Mutex.h"

V3ThreadPool::V3ThreadPool(పూర్ణాంకం అంకె_దారాలు) {
    అంకె_దారాలు = std::max(అంకె_దారాలు, 1);
    ఒకవేళ (అంకె_దారాలు == 1) తిరిగి;
    కొరకు (పూర్ణాంకం i = 0; i < అంకె_దారాలు; ++i) {
        m_workers.emplace_వెనుక(&V3ThreadPool::ప్రారంభం_కార్మికుడు, ఇది);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        m_shutdown = నిజం;
    }
    m_cv.notify_all();
    wait();
}

శూన్య V3ThreadPool::వరుసలో_చేర్చు(std::function<శూన్య()>&& f) {
    ఒకవేళ (m_workers.ఖాళీలు()) {
        f();
    } లేకపోతే {
        {
            స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
            m_వరుస.నెట్టు(std::move(f));
        }
        m_pendingJobs.fetch_కూడు(1, std::జ్ఞాపకం_క్రమం_release);
        m_cv.notify_one();
    }
}

శూన్య V3ThreadPool::wait() {
    అయితే (m_pendingJobs.ఎక్కించు(std::జ్ఞాపకం_క్రమం_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    ఒకవేళ (m_shutdown) {
        కొరకు (స్వయంచాలక& కార్మికుడు : m_workers) కార్మికుడు.join();
    }
}

శూన్య V3ThreadPool::ప్రారంభం_కార్మికుడు(V3ThreadPool* selfThreadp) { selfThreadp->కార్మికుడు_పని_Loop(); }

శూన్య V3ThreadPool::కార్మికుడు_పని_Loop() {
    అయితే (నిజం) {
        std::function<శూన్య()> పని;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
            m_cv.wait(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { తిరిగి !m_వరుస.ఖాళీలు() || m_shutdown; });
            ఒకవేళ (m_shutdown) తిరిగి;  // Terminate if requested
            UASSERT(!m_వరుస.ఖాళీలు(), "Job should be available");
            ఒకవేళ (m_వరుస.ఖాళీలు()) కొనసాగు;
            పని = std::move(m_వరుస.front());
            m_వరుస.తీయి();
        }
        పని();
        m_pendingJobs.fetch_sub(1, std::జ్ఞాపకం_క్రమం_release);
    }
}

శూన్య V3ThreadPool::selfTestMtDisabled() {
    // empty
}

శూన్య V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    పూర్ణాంకం common_విలువ{0};

    స్వయంచాలక మొదటి_పని = [&](పూర్ణాంకం సుషుప్తి) -> శూన్య {
        std::this_thread::sleep_for(std::chrono::milliseconds{సుషుప్తి});
        స్థిరరాశి V3_తాళం_Guard తాళం_2{commonMutex};
        common_విలువ = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{సుషుప్తి + 10});
        UASSERT(common_విలువ == 10, "unexpected commonValue = " << common_విలువ);
    };
    స్వయంచాలక రెండవ_పని = [&](పూర్ణాంకం సుషుప్తి) -> శూన్య {
        commonMutex.తాళం_2();
        commonMutex.unlock();
        స్థిరరాశి V3_తాళం_Guard తాళం_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{సుషుప్తి});
        common_విలువ = 1000;
    };
    స్వయంచాలక మూడవ_పని = [&](పూర్ణాంకం సుషుప్తి) -> శూన్య {
        {
            స్థిరరాశి V3_తాళం_Guard తాళం_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{సుషుప్తి});
        }
        మొదటి_పని(సుషుప్తి);
        స్థిరరాశి V3_తాళం_Guard తాళం_2{commonMutex};
        common_విలువ = 100;
    };
    {
        V3ThreadScope scope;

        scope.వరుసలో_చేర్చు(std::bind(మొదటి_పని, 100));
        scope.వరుసలో_చేర్చు(std::bind(రెండవ_పని, 100));
        scope.వరుసలో_చేర్చు(std::bind(మొదటి_పని, 100));
        scope.వరుసలో_చేర్చు(std::bind(రెండవ_పని, 100));
        scope.వరుసలో_చేర్చు(std::bind(రెండవ_పని, 200));
        scope.వరుసలో_చేర్చు(std::bind(మొదటి_పని, 200));
        scope.వరుసలో_చేర్చు(std::bind(మొదటి_పని, 300));
        scope.wait();

        UASSERT(common_విలువ == 1000 || common_విలువ == 10,
                "unexpected common value = " << common_విలువ);

        scope.వరుసలో_చేర్చు(std::bind(మూడవ_పని, 100));
        scope.వరుసలో_చేర్చు(std::bind(మూడవ_పని, 100));
    }

    UASSERT(common_విలువ == 100, "unexpected common value = " << common_విలువ);

    {
        V3ThreadScope scope;
        scope.వరుసలో_చేర్చు(std::bind(మొదటి_పని, 100));
    }

    UASSERT(common_విలువ == 10, "unexpected common value = " << common_విలువ);

    {
        పూర్ణాంకం ఫలితం = 0;
        స్వయంచాలక forth_పని = [&]() -> శూన్య { ఫలితం = 1234; };

        V3ThreadScope scope;
        scope.వరుసలో_చేర్చు(forth_పని);
        scope.wait();
        UASSERT(ఫలితం == 1234, "unexpected job result = " << ఫలితం);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    wait();
}

శూన్య V3ThreadScope::వరుసలో_చేర్చు(std::function<శూన్య()>&& f) { m_pool->వరుసలో_చేర్చు(std::move(f)); }

శూన్య V3ThreadScope::wait() { m_pool->wait(); }
