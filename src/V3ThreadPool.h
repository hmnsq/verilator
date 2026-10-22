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

#अगर_अघोषित _V3THREADPOOL_H_
#घोषणा _V3THREADPOOL_H_ 1

#अगर_घोषित VL_MT_DISABLED_CODE_UNIT
#त्रुटि "Source file has been declared as MT_DISABLED, threads use is prohibited."
#पूर्ण_अगर

#समावेश "V3Mutex.h"

#समावेश <atomic>
#समावेश <condition_variable>
#समावेश <functional>
#समावेश <queue>
#समावेश <thread>

//============================================================================

class V3ThreadPool final {
    // MEMBERS
    std::vector<std::thread> m_workers;  // Worker threads
    std::queue<std::function<व्योम()>> m_queue VL_GUARDED_BY(m_mutex);  // Job queue
    std::condition_variable_any m_cv;  // Conditions to wake up workers
    std::atomic<bool> m_shutdown{false};  // Termination pending
    std::atomic<माप_प्रकार> m_pendingJobs{0};  // Number of started and not yet finished jobs
    V3Mutex m_mutex;  // Mutex for use by m_queue

खुला:
    // CONSTRUCTORS
    explicit V3ThreadPool(पूर्णांक अंक_Threads);
    ~V3ThreadPool() VL_EXCLUDES(m_mutex);
    VL_UNCOPYABLE(V3ThreadPool);
    VL_UNMOVABLE(V3ThreadPool);

    अटल व्योम selfTest();
    अटल व्योम selfTestMtDisabled() VL_MT_DISABLED;

निजी:
    // METHODS
    // Enqueue a job for asynchronous execution
    // Due to missing support for lambda annotations in c++11,
    // `clang_check_attributes` script assumes that if
    // function takes `std::function` as argument, it
    // will call it. `VL_MT_START` here indicates that
    // every function call inside this `std::function` requires
    // annotations.
    व्योम enqueue(std::function<व्योम()>&& f) VL_MT_START VL_EXCLUDES(m_mutex);

    // Wait for all enqueued jobs to finish
    व्योम रुको() VL_MT_SAFE;

    // Job execution loop
    // Each worker wait for available job and executes it when it is available.
    व्योम workerJobLoop() VL_MT_SAFE VL_EXCLUDES(m_mutex);

    // Start worker thread
    अटल व्योम startWorker(V3ThreadPool* selfThreadp) VL_MT_SAFE VL_EXCLUDES(m_mutex);

    // For access to enqueue() and wait()
    मित्र class V3ThreadScope;
};

// The actual interface for submitting jobs and ensuring their completion.
// Ensures that jobs are started and completed within a given scope.
class V3ThreadScope final {
    // MEMBERS
    V3ThreadPool* m_pool = nullptr;  // Global thread pool instance

खुला:
    // CONSTRUCTORS
    V3ThreadScope() VL_MT_SAFE VL_ACQUIRE(VlOs::MtScopeMutex::s_haveThreadScope);
    ~V3ThreadScope() VL_MT_SAFE VL_RELEASE(VlOs::MtScopeMutex::s_haveThreadScope) { रुको(); }
    VL_UNCOPYABLE(V3ThreadScope);
    VL_UNMOVABLE(V3ThreadScope);

    // METHODS
    // Submit job to the thread pool instance
    व्योम enqueue(std::function<व्योम()>&& f) VL_MT_START;
    // Wait for thread pool's jobs completion
    व्योम रुको() VL_MT_SAFE VL_REQUIRES(VlOs::MtScopeMutex::s_haveThreadScope);
};

#पूर्ण_अगर  // Guard
