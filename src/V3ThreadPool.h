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

#నిర్వచితం_కాకపోతే _V3THREADPOOL_H_
#నిర్వచించు _V3THREADPOOL_H_ 1

#నిర్వచితమైతే VL_MT_DISABLED_సంకేతం_UNIT
#దోషం "Source file has been declared as MT_DISABLED, threads use is prohibited."
#అయితే_ముగింపు

#అన్తర్భావ "V3Mutex.h"

#అన్తర్భావ <atomic>
#అన్తర్భావ <condition_variable>
#అన్తర్భావ <functional>
#అన్తర్భావ <queue>
#అన్తర్భావ <thread>

//============================================================================

class V3ThreadPool final {
    // MEMBERS
    std::సదిశ<std::thread> m_workers;  // Worker threads
    std::వరుస<std::function<శూన్య()>> m_వరుస VL_GUARDED_BY(m_mutex);  // Job queue
    std::condition_variable_any m_cv;  // Conditions to wake up workers
    std::atomic<bool> m_shutdown{false};  // Termination pending
    std::atomic<కొలత_t> m_pendingJobs{0};  // Number of started and not yet finished jobs
    V3Mutex m_mutex;  // Mutex for use by m_queue

బహిరంగ:
    // CONSTRUCTORS
    explicit V3ThreadPool(పూర్ణాంకం అంకె_దారాలు);
    ~V3ThreadPool() VL_EXCLUDES(m_mutex);
    VL_UNCOPYABLE(V3ThreadPool);
    VL_UNMOVABLE(V3ThreadPool);

    స్థిర శూన్య selfTest();
    స్థిర శూన్య selfTestMtDisabled() VL_MT_DISABLED;

అంతరంగ:
    // METHODS
    // Enqueue a job for asynchronous execution
    // Due to missing support for lambda annotations in c++11,
    // `clang_check_attributes` script assumes that if
    // function takes `std::function` as argument, it
    // will call it. `VL_MT_START` here indicates that
    // every function call inside this `std::function` requires
    // annotations.
    శూన్య వరుసలో_చేర్చు(std::function<శూన్య()>&& f) VL_MT_ప్రారంభం VL_EXCLUDES(m_mutex);

    // Wait for all enqueued jobs to finish
    శూన్య wait() VL_MT_సురక్షిత;

    // Job execution loop
    // Each worker wait for available job and executes it when it is available.
    శూన్య కార్మికుడు_పని_Loop() VL_MT_సురక్షిత VL_EXCLUDES(m_mutex);

    // Start worker thread
    స్థిర శూన్య ప్రారంభం_కార్మికుడు(V3ThreadPool* selfThreadp) VL_MT_సురక్షిత VL_EXCLUDES(m_mutex);

    // For access to enqueue() and wait()
    మిత్ర class V3ThreadScope;
};

// The actual interface for submitting jobs and ensuring their completion.
// Ensures that jobs are started and completed within a given scope.
class V3ThreadScope final {
    // MEMBERS
    V3ThreadPool* m_pool = nullptr;  // Global thread pool instance

బహిరంగ:
    // CONSTRUCTORS
    V3ThreadScope() VL_MT_సురక్షిత VL_ACQUIRE(VlOs::MtScopeMutex::s_haveThreadScope);
    ~V3ThreadScope() VL_MT_సురక్షిత VL_RELEASE(VlOs::MtScopeMutex::s_haveThreadScope) { wait(); }
    VL_UNCOPYABLE(V3ThreadScope);
    VL_UNMOVABLE(V3ThreadScope);

    // METHODS
    // Submit job to the thread pool instance
    శూన్య వరుసలో_చేర్చు(std::function<శూన్య()>&& f) VL_MT_ప్రారంభం;
    // Wait for thread pool's jobs completion
    శూన్య wait() VL_MT_సురక్షిత VL_REQUIRES(VlOs::MtScopeMutex::s_haveThreadScope);
};

#అయితే_ముగింపు  // Guard
