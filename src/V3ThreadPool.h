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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ _V3THREADPOOL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _V3THREADPOOL_H_ 1

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_MT_DISABLED_ਕੋਡ_UNIT
#ਗਲਤੀ "Source file has been declared as MT_DISABLED, threads use is prohibited."
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <atomic>
#ਅਨ੍ਤਰ੍ਭਾਵ <condition_variable>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <queue>
#ਅਨ੍ਤਰ੍ਭਾਵ <thread>

//============================================================================

class V3ThreadPool final {
    // MEMBERS
    std::ਸਦਿਸ਼<std::thread> m_workers;  // Worker threads
    std::ਕਤਾਰ<std::function<ਖਾਲੀ()>> m_ਕਤਾਰ VL_GUARDED_BY(m_mutex);  // Job queue
    std::condition_variable_any m_cv;  // Conditions to wake up workers
    std::atomic<bool> m_shutdown{false};  // Termination pending
    std::atomic<ਮਾਪ_t> m_pendingJobs{0};  // Number of started and not yet finished jobs
    V3Mutex m_mutex;  // Mutex for use by m_queue

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit V3ThreadPool(ਪੂਰਨ ਅੰਕ_ਧਾਗੇ);
    ~V3ThreadPool() VL_EXCLUDES(m_mutex);
    VL_UNCOPYABLE(V3ThreadPool);
    VL_UNMOVABLE(V3ThreadPool);

    ਸਥਿਰ ਖਾਲੀ selfTest();
    ਸਥਿਰ ਖਾਲੀ selfTestMtDisabled() VL_MT_DISABLED;

ਨਿਜੀ:
    // METHODS
    // Enqueue a job for asynchronous execution
    // Due to missing support for lambda annotations in c++11,
    // `clang_check_attributes` script assumes that if
    // function takes `std::function` as argument, it
    // will call it. `VL_MT_START` here indicates that
    // every function call inside this `std::function` requires
    // annotations.
    ਖਾਲੀ ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::function<ਖਾਲੀ()>&& f) VL_MT_ਸ਼ੁਰੂ VL_EXCLUDES(m_mutex);

    // Wait for all enqueued jobs to finish
    ਖਾਲੀ wait() VL_MT_ਸੁਰੱਖਿਅਤ;

    // Job execution loop
    // Each worker wait for available job and executes it when it is available.
    ਖਾਲੀ ਕਾਮਾ_ਕੰਮ_Loop() VL_MT_ਸੁਰੱਖਿਅਤ VL_EXCLUDES(m_mutex);

    // Start worker thread
    ਸਥਿਰ ਖਾਲੀ ਸ਼ੁਰੂ_ਕਾਮਾ(V3ThreadPool* selfThreadp) VL_MT_ਸੁਰੱਖਿਅਤ VL_EXCLUDES(m_mutex);

    // For access to enqueue() and wait()
    ਮਿੱਤਰ class V3ThreadScope;
};

// The actual interface for submitting jobs and ensuring their completion.
// Ensures that jobs are started and completed within a given scope.
class V3ThreadScope final {
    // MEMBERS
    V3ThreadPool* m_pool = nullptr;  // Global thread pool instance

ਜਨਤਕ:
    // CONSTRUCTORS
    V3ThreadScope() VL_MT_ਸੁਰੱਖਿਅਤ VL_ACQUIRE(VlOs::MtScopeMutex::s_haveThreadScope);
    ~V3ThreadScope() VL_MT_ਸੁਰੱਖਿਅਤ VL_RELEASE(VlOs::MtScopeMutex::s_haveThreadScope) { wait(); }
    VL_UNCOPYABLE(V3ThreadScope);
    VL_UNMOVABLE(V3ThreadScope);

    // METHODS
    // Submit job to the thread pool instance
    ਖਾਲੀ ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(std::function<ਖਾਲੀ()>&& f) VL_MT_ਸ਼ੁਰੂ;
    // Wait for thread pool's jobs completion
    ਖਾਲੀ wait() VL_MT_ਸੁਰੱਖਿਅਤ VL_REQUIRES(VlOs::MtScopeMutex::s_haveThreadScope);
};

#ਜੇ_ਅੰਤ  // Guard
