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

#अन्तर्भाव "V3ThreadPool.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Mutex.h"

V3ThreadPool::V3ThreadPool(पूर्णाङ्क अङ्कः_तन्तवः) {
    अङ्कः_तन्तवः = std::max(अङ्कः_तन्तवः, 1);
    यदि (अङ्कः_तन्तवः == 1) प्रत्यागम;
    प्रति (पूर्णाङ्क i = 0; i < अङ्कः_तन्तवः; ++i) {
        m_workers.emplace_पृष्ठभागः(&V3ThreadPool::start_कार्यकर्ता, एतत्);
    }
}

V3ThreadPool::~V3ThreadPool() {
    {
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        m_shutdown = सत्य;
    }
    m_cv.notify_all();
    प्रतीक्षस्व();
}

शून्य V3ThreadPool::पङ्क्तौ_निक्षिप(std::function<शून्य()>&& f) {
    यदि (m_workers.रिक्तानि()) {
        f();
    } अन्यथा {
        {
            ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
            m_queue.प्रक्षिप(std::move(f));
        }
        m_pendingJobs.fetch_सङ्कलय(1, std::स्मृतिः_क्रमः_release);
        m_cv.notify_one();
    }
}

शून्य V3ThreadPool::प्रतीक्षस्व() {
    यावत् (m_pendingJobs.आरोपय(std::स्मृतिः_क्रमः_acquire) > 0 && !m_shutdown) {
        std::this_thread::yield();
    }
    यदि (m_shutdown) {
        प्रति (स्वचालित& कार्यकर्ता : m_workers) कार्यकर्ता.join();
    }
}

शून्य V3ThreadPool::start_कार्यकर्ता(V3ThreadPool* selfThreadp) { selfThreadp->कार्यकर्ता_Job_Loop(); }

शून्य V3ThreadPool::कार्यकर्ता_Job_Loop() {
    यावत् (सत्य) {
        std::function<शून्य()> job;
        {
            // Locking before `condition_variable::wait` is required to ensure that the
            // `m_cv` condition will be executed under a lock. Taking a lock
            // before `condition_variable::wait` may lead to missed
            // `condition_variable::notify_all` notification (when a thread waits at the lock
            // before) but, according to C++ standard, the `condition_variable::wait` first checks
            // the condition and then waits for the notification, thus even when notification is
            // missed, the condition still will be checked.
            ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
            m_cv.प्रतीक्षस्व(m_mutex,
                      [&]() VL_REQUIRES(m_mutex) { प्रत्यागम !m_queue.रिक्तानि() || m_shutdown; });
            यदि (m_shutdown) प्रत्यागम;  // Terminate if requested
            UASSERT(!m_queue.रिक्तानि(), "Job should be available");
            यदि (m_queue.रिक्तानि()) अनुवर्त;
            job = std::move(m_queue.front());
            m_queue.निष्कासय();
        }
        job();
        m_pendingJobs.fetch_sub(1, std::स्मृतिः_क्रमः_release);
    }
}

शून्य V3ThreadPool::selfTestMtDisabled() {
    // empty
}

शून्य V3ThreadPool::selfTest() {
    V3Mutex commonMutex;
    पूर्णाङ्क common_मानम्{0};

    स्वचालित firstJob = [&](पूर्णाङ्क सुषुप्ति) -> शून्य {
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        ध्रुव V3_तालकम्_Guard तालकम्_2{commonMutex};
        common_मानम् = 10;
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति + 10});
        UASSERT(common_मानम् == 10, "unexpected commonValue = " << common_मानम्);
    };
    स्वचालित secondJob = [&](पूर्णाङ्क सुषुप्ति) -> शून्य {
        commonMutex.तालकम्_2();
        commonMutex.unlock();
        ध्रुव V3_तालकम्_Guard तालकम्_2{commonMutex};
        std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        common_मानम् = 1000;
    };
    स्वचालित thirdJob = [&](पूर्णाङ्क सुषुप्ति) -> शून्य {
        {
            ध्रुव V3_तालकम्_Guard तालकम्_2{commonMutex};
            std::this_thread::sleep_for(std::chrono::milliseconds{सुषुप्ति});
        }
        firstJob(सुषुप्ति);
        ध्रुव V3_तालकम्_Guard तालकम्_2{commonMutex};
        common_मानम् = 100;
    };
    {
        V3ThreadScope scope;

        scope.पङ्क्तौ_निक्षिप(std::bind(firstJob, 100));
        scope.पङ्क्तौ_निक्षिप(std::bind(secondJob, 100));
        scope.पङ्क्तौ_निक्षिप(std::bind(firstJob, 100));
        scope.पङ्क्तौ_निक्षिप(std::bind(secondJob, 100));
        scope.पङ्क्तौ_निक्षिप(std::bind(secondJob, 200));
        scope.पङ्क्तौ_निक्षिप(std::bind(firstJob, 200));
        scope.पङ्क्तौ_निक्षिप(std::bind(firstJob, 300));
        scope.प्रतीक्षस्व();

        UASSERT(common_मानम् == 1000 || common_मानम् == 10,
                "unexpected common value = " << common_मानम्);

        scope.पङ्क्तौ_निक्षिप(std::bind(thirdJob, 100));
        scope.पङ्क्तौ_निक्षिप(std::bind(thirdJob, 100));
    }

    UASSERT(common_मानम् == 100, "unexpected common value = " << common_मानम्);

    {
        V3ThreadScope scope;
        scope.पङ्क्तौ_निक्षिप(std::bind(firstJob, 100));
    }

    UASSERT(common_मानम् == 10, "unexpected common value = " << common_मानम्);

    {
        पूर्णाङ्क परिणाम = 0;
        स्वचालित forthJob = [&]() -> शून्य { परिणाम = 1234; };

        V3ThreadScope scope;
        scope.पङ्क्तौ_निक्षिप(forthJob);
        scope.प्रतीक्षस्व();
        UASSERT(परिणाम == 1234, "unexpected job result = " << परिणाम);
    }
    selfTestMtDisabled();
}

V3ThreadScope::V3ThreadScope() {
    UASSERT(v3Global.threadPoolp(), "ThreadPool must be initialized before ThreadScope.");
    m_pool = v3Global.threadPoolp();
    प्रतीक्षस्व();
}

शून्य V3ThreadScope::पङ्क्तौ_निक्षिप(std::function<शून्य()>&& f) { m_pool->पङ्क्तौ_निक्षिप(std::move(f)); }

शून्य V3ThreadScope::प्रतीक्षस्व() { m_pool->प्रतीक्षस्व(); }
