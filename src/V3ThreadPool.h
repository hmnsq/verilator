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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ _V3THREADPOOL_H_
#ವ್ಯಾಖ್ಯಾನಿಸು _V3THREADPOOL_H_ 1

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_MT_DISABLED_ಸಂಕೇತ_UNIT
#ದೋಷ "Source file has been declared as MT_DISABLED, threads use is prohibited."
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

#ಅನ್ತರ್ಭಾವ <atomic>
#ಅನ್ತರ್ಭಾವ <condition_variable>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <queue>
#ಅನ್ತರ್ಭಾವ <thread>

//============================================================================

class V3ThreadPool final {
    // MEMBERS
    std::ಸದಿಶ<std::thread> m_workers;  // Worker threads
    std::ಸರತಿ<std::function<ಶೂನ್ಯ()>> m_ಸರತಿ VL_GUARDED_BY(m_mutex);  // Job queue
    std::condition_variable_any m_cv;  // Conditions to wake up workers
    std::atomic<bool> m_shutdown{false};  // Termination pending
    std::atomic<ಗಾತ್ರ_t> m_pendingJobs{0};  // Number of started and not yet finished jobs
    V3Mutex m_mutex;  // Mutex for use by m_queue

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit V3ThreadPool(ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಎಳೆಗಳು);
    ~V3ThreadPool() VL_EXCLUDES(m_mutex);
    VL_UNCOPYABLE(V3ThreadPool);
    VL_UNMOVABLE(V3ThreadPool);

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestMtDisabled() VL_MT_DISABLED;

ಖಾಸಗಿ:
    // METHODS
    // Enqueue a job for asynchronous execution
    // Due to missing support for lambda annotations in c++11,
    // `clang_check_attributes` script assumes that if
    // function takes `std::function` as argument, it
    // will call it. `VL_MT_START` here indicates that
    // every function call inside this `std::function` requires
    // annotations.
    ಶೂನ್ಯ ಸರತಿಗೆ_ಸೇರಿಸು(std::function<ಶೂನ್ಯ()>&& f) VL_MT_ಆರಂಭ VL_EXCLUDES(m_mutex);

    // Wait for all enqueued jobs to finish
    ಶೂನ್ಯ wait() VL_MT_ಸುರಕ್ಷಿತ;

    // Job execution loop
    // Each worker wait for available job and executes it when it is available.
    ಶೂನ್ಯ ಕೆಲಸಗಾರ_ಕೆಲಸ_Loop() VL_MT_ಸುರಕ್ಷಿತ VL_EXCLUDES(m_mutex);

    // Start worker thread
    ಸ್ಥಿರ ಶೂನ್ಯ ಆರಂಭ_ಕೆಲಸಗಾರ(V3ThreadPool* selfThreadp) VL_MT_ಸುರಕ್ಷಿತ VL_EXCLUDES(m_mutex);

    // For access to enqueue() and wait()
    ಮಿತ್ರ class V3ThreadScope;
};

// The actual interface for submitting jobs and ensuring their completion.
// Ensures that jobs are started and completed within a given scope.
class V3ThreadScope final {
    // MEMBERS
    V3ThreadPool* m_pool = nullptr;  // Global thread pool instance

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3ThreadScope() VL_MT_ಸುರಕ್ಷಿತ VL_ACQUIRE(VlOs::MtScopeMutex::s_haveThreadScope);
    ~V3ThreadScope() VL_MT_ಸುರಕ್ಷಿತ VL_RELEASE(VlOs::MtScopeMutex::s_haveThreadScope) { wait(); }
    VL_UNCOPYABLE(V3ThreadScope);
    VL_UNMOVABLE(V3ThreadScope);

    // METHODS
    // Submit job to the thread pool instance
    ಶೂನ್ಯ ಸರತಿಗೆ_ಸೇರಿಸು(std::function<ಶೂನ್ಯ()>&& f) VL_MT_ಆರಂಭ;
    // Wait for thread pool's jobs completion
    ಶೂನ್ಯ wait() VL_MT_ಸುರಕ್ಷಿತ VL_REQUIRES(VlOs::MtScopeMutex::s_haveThreadScope);
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
