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

#اگر_غیر_تعریف_شدہ _V3THREADPOOL_H_
#تعریف_کرو _V3THREADPOOL_H_ 1

#اگر_تعریف_شدہ VL_MT_DISABLED_رمز_UNIT
#غلطی "Source file has been declared as MT_DISABLED, threads use is prohibited."
#اگر_ختم

#شامل "V3Mutex.h"

#شامل <atomic>
#شامل <condition_variable>
#شامل <functional>
#شامل <queue>
#شامل <thread>

//============================================================================

class V3ThreadPool final {
    // MEMBERS
    std::سمتیہ<std::thread> m_workers;  // Worker threads
    std::قطار<std::function<خالی()>> m_قطار VL_GUARDED_BY(m_mutex);  // Job queue
    std::condition_variable_any m_cv;  // Conditions to wake up workers
    std::atomic<bool> m_shutdown{false};  // Termination pending
    std::atomic<سائز_t> m_pendingJobs{0};  // Number of started and not yet finished jobs
    V3Mutex m_mutex;  // Mutex for use by m_queue

عوامی:
    // CONSTRUCTORS
    explicit V3ThreadPool(صحیح عدد_دھاگے);
    ~V3ThreadPool() VL_EXCLUDES(m_mutex);
    VL_UNCOPYABLE(V3ThreadPool);
    VL_UNMOVABLE(V3ThreadPool);

    ساکن خالی selfTest();
    ساکن خالی selfTestMtDisabled() VL_MT_DISABLED;

نجی:
    // METHODS
    // Enqueue a job for asynchronous execution
    // Due to missing support for lambda annotations in c++11,
    // `clang_check_attributes` script assumes that if
    // function takes `std::function` as argument, it
    // will call it. `VL_MT_START` here indicates that
    // every function call inside this `std::function` requires
    // annotations.
    خالی قطار_میں_ڈالو(std::function<خالی()>&& f) VL_MT_آغاز VL_EXCLUDES(m_mutex);

    // Wait for all enqueued jobs to finish
    خالی wait() VL_MT_محفوظ;

    // Job execution loop
    // Each worker wait for available job and executes it when it is available.
    خالی کارکن_کام_Loop() VL_MT_محفوظ VL_EXCLUDES(m_mutex);

    // Start worker thread
    ساکن خالی آغاز_کارکن(V3ThreadPool* selfThreadp) VL_MT_محفوظ VL_EXCLUDES(m_mutex);

    // For access to enqueue() and wait()
    دوست class V3ThreadScope;
};

// The actual interface for submitting jobs and ensuring their completion.
// Ensures that jobs are started and completed within a given scope.
class V3ThreadScope final {
    // MEMBERS
    V3ThreadPool* m_pool = nullptr;  // Global thread pool instance

عوامی:
    // CONSTRUCTORS
    V3ThreadScope() VL_MT_محفوظ VL_ACQUIRE(VlOs::MtScopeMutex::s_haveThreadScope);
    ~V3ThreadScope() VL_MT_محفوظ VL_RELEASE(VlOs::MtScopeMutex::s_haveThreadScope) { wait(); }
    VL_UNCOPYABLE(V3ThreadScope);
    VL_UNMOVABLE(V3ThreadScope);

    // METHODS
    // Submit job to the thread pool instance
    خالی قطار_میں_ڈالو(std::function<خالی()>&& f) VL_MT_آغاز;
    // Wait for thread pool's jobs completion
    خالی wait() VL_MT_محفوظ VL_REQUIRES(VlOs::MtScopeMutex::s_haveThreadScope);
};

#اگر_ختم  // Guard
