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

#ifndef _V3THREADPOOL_H_
#define _V3THREADPOOL_H_ 1

#ifdef VL_MT_DISABLED_CODE_UNIT
#error "Source file has been declared as MT_DISABLED, threads use is prohibited."
#endif

#അന്തര്ഭാവ "V3Mutex.h"

#അന്തര്ഭാവ <atomic>
#അന്തര്ഭാവ <condition_variable>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <queue>
#അന്തര്ഭാവ <thread>

//============================================================================

class V3ThreadPool final {
    // MEMBERS
    std::vector<std::thread> m_workers;  // Worker threads
    std::queue<std::function<ശൂന്യം()>> m_queue VL_GUARDED_BY(m_mutex);  // Job queue
    std::condition_variable_any m_cv;  // Conditions to wake up workers
    std::atomic<bool> m_shutdown{false};  // Termination pending
    std::atomic<size_t> m_pendingJobs{0};  // Number of started and not yet finished jobs
    V3Mutex m_mutex;  // Mutex for use by m_queue

പൊതു:
    // CONSTRUCTORS
    explicit V3ThreadPool(പൂർണ്ണസംഖ്യ അക്കം_Threads);
    ~V3ThreadPool() VL_EXCLUDES(m_mutex);
    VL_UNCOPYABLE(V3ThreadPool);
    VL_UNMOVABLE(V3ThreadPool);

    സ്ഥിതം ശൂന്യം selfTest();
    സ്ഥിതം ശൂന്യം selfTestMtDisabled() VL_MT_DISABLED;

സ്വകാര്യ:
    // METHODS
    // Enqueue a job for asynchronous execution
    // Due to missing support for lambda annotations in c++11,
    // `clang_check_attributes` script assumes that if
    // function takes `std::function` as argument, it
    // will call it. `VL_MT_START` here indicates that
    // every function call inside this `std::function` requires
    // annotations.
    ശൂന്യം enqueue(std::function<ശൂന്യം()>&& f) VL_MT_START VL_EXCLUDES(m_mutex);

    // Wait for all enqueued jobs to finish
    ശൂന്യം wait() VL_MT_SAFE;

    // Job execution loop
    // Each worker wait for available job and executes it when it is available.
    ശൂന്യം workerJobLoop() VL_MT_SAFE VL_EXCLUDES(m_mutex);

    // Start worker thread
    സ്ഥിതം ശൂന്യം startWorker(V3ThreadPool* selfThreadp) VL_MT_SAFE VL_EXCLUDES(m_mutex);

    // For access to enqueue() and wait()
    friend class V3ThreadScope;
};

// The actual interface for submitting jobs and ensuring their completion.
// Ensures that jobs are started and completed within a given scope.
class V3ThreadScope final {
    // MEMBERS
    V3ThreadPool* m_pool = nullptr;  // Global thread pool instance

പൊതു:
    // CONSTRUCTORS
    V3ThreadScope() VL_MT_SAFE VL_ACQUIRE(VlOs::MtScopeMutex::s_haveThreadScope);
    ~V3ThreadScope() VL_MT_SAFE VL_RELEASE(VlOs::MtScopeMutex::s_haveThreadScope) { wait(); }
    VL_UNCOPYABLE(V3ThreadScope);
    VL_UNMOVABLE(V3ThreadScope);

    // METHODS
    // Submit job to the thread pool instance
    ശൂന്യം enqueue(std::function<ശൂന്യം()>&& f) VL_MT_START;
    // Wait for thread pool's jobs completion
    ശൂന്യം wait() VL_MT_SAFE VL_REQUIRES(VlOs::MtScopeMutex::s_haveThreadScope);
};

#endif  // Guard
