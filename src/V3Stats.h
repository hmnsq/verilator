// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Collect and print statistics
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3STATS_H_
#define VERILATOR_V3STATS_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    ದ್ವಿಗುಣ m_d = 0.0;  ///< Count of occurrences/ value
ಸಾರ್ವಜನಿಕ:
    // METHODS
    VDouble0() = ಸ್ವಭಾವ;
    ~VDouble0() = ಸ್ವಭಾವ;

    // Implicit conversion operators:
    explicit VDouble0(ಧ್ರುವ uint64_t v)
        : m_d{static_cast<ದ್ವಿಗುಣ>(v)} {}
    operator ದ್ವಿಗುಣ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        ಪ್ರತ್ಯಾಗಮ *this;
    }
    VDouble0 operator++(ಪೂರ್ಣಾಙ್ಕ) {  // postfix
        ಧ್ರುವ VDouble0 old = *this;
        ++m_d;
        ಪ್ರತ್ಯಾಗಮ old;
    }
    VDouble0& operator=(ಧ್ರುವ ದ್ವಿಗುಣ v) {
        m_d = v;
        ಪ್ರತ್ಯಾಗಮ *this;
    }
    VDouble0& operator+=(ಧ್ರುವ ದ್ವಿಗುಣ v) {
        m_d += v;
        ಪ್ರತ್ಯಾಗಮ *this;
    }
    VDouble0& operator-=(ಧ್ರುವ ದ್ವಿಗುಣ v) {
        m_d -= v;
        ಪ್ರತ್ಯಾಗಮ *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ಧ್ರುವ string m_name;  ///< Name of this statistic
    ದ್ವಿಗುಣ m_ಮೌಲ್ಯ;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    ಧ್ರುವ string m_stage;  ///< Runtime stage
    ಧ್ರುವ bool m_sumit;  ///< Do summation of similar stats
    ಧ್ರುವ bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
ಸಾರ್ವಜನಿಕ:
    // METHODS
    string stage() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_stage; }
    string name() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ದ್ವಿಗುಣ ಮೌಲ್ಯ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ; }
    unsigned precision() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_precision; }
    bool sumit() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_sumit; }
    bool perf() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_perf; }
    bool printit() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_printit; }
    ಆಭಾಸ ಶೂನ್ಯ dump(std::ofstream& os) ಧ್ರುವ VL_MT_SAFE;
    ಶೂನ್ಯ combineWith(V3Statistic* otherp) {
        m_ಮೌಲ್ಯ += otherp->ಮೌಲ್ಯ();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ಧ್ರುವ string& stage, ಧ್ರುವ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_ಮೌಲ್ಯ{ಮೌಲ್ಯ}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    ಆಭಾಸ ~V3Statistic() = ಸ್ವಭಾವ;
};

//============================================================================

class V3Stats final {
    ಸ್ಥಿರ V3Mutex s_mutex;  // Protects accesses

ಸಾರ್ವಜನಿಕ:
    // Symbolic names for some statistics that are later read by summaryReport()
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_CPP_CHARS = "Output, C++ bytes written";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_CPP_FILES = "Output, C++ files written";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_CPUTIME = "CPU time, Total (sec)";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_WALLTIME = "Wall time, Total (sec)";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ಸ್ಥಿರ ಶೂನ್ಯ addStat(ಧ್ರುವ V3Statistic&);
    ಸ್ಥಿರ ಶೂನ್ಯ addStat(ಧ್ರುವ string& stage, ಧ್ರುವ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, ಮೌಲ್ಯ, precision, false, false});
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addStat(ಧ್ರುವ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, ಮೌಲ್ಯ, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ಸ್ಥಿರ ಶೂನ್ಯ addStatSum(ಧ್ರುವ ವರ್ಣ* name, ದ್ವಿಗುಣ ಎಣಿಕೆ) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        ಯದಿ (ಎಣಿಕೆ == 0.0) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, ಎಣಿಕೆ, 0, true, false});
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addStatSum(ಧ್ರುವ std::string& name, ದ್ವಿಗುಣ ಎಣಿಕೆ) {
        addStatSum(name.c_str(), ಎಣಿಕೆ);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addStatPerf(ಧ್ರುವ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        addStat(V3Statistic{"*", name, ಮೌಲ್ಯ, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ಸ್ಥಿರ ದ್ವಿಗುಣ getStatSum(ಧ್ರುವ string& name);
    ಸ್ಥಿರ uint64_t getStatSumQ(ಧ್ರುವ string& name) {
        ಪ್ರತ್ಯಾಗಮ static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    ಸ್ಥಿರ ಶೂನ್ಯ statsStage(ಧ್ರುವ string& name);
    /// Called by the top level, and error path, to collect statistics
    ಸ್ಥಿರ ಶೂನ್ಯ statsStageAll(AstNetlist* nodep, ಧ್ರುವ string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ಸ್ಥಿರ ಶೂನ್ಯ statsReport();
    /// Called by debug dumps
    ಸ್ಥಿರ ಶೂನ್ಯ infoHeader(std::ofstream& os, ಧ್ರುವ string& prefix);
    /// Called for final build report
    ಸ್ಥಿರ ಶೂನ್ಯ summaryReport();
};

#endif  // Guard
