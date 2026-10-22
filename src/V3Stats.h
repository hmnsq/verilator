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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    دوگانه m_d = 0.0;  ///< Count of occurrences/ value
عمومی:
    // METHODS
    VDouble0() = پیشفرض;
    ~VDouble0() = پیشفرض;

    // Implicit conversion operators:
    explicit VDouble0(ثابت uint64_t v)
        : m_d{static_cast<دوگانه>(v)} {}
    operator دوگانه() ثابت { بازگشت m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        بازگشت *this;
    }
    VDouble0 operator++(صحیح) {  // postfix
        ثابت VDouble0 old = *this;
        ++m_d;
        بازگشت old;
    }
    VDouble0& operator=(ثابت دوگانه v) {
        m_d = v;
        بازگشت *this;
    }
    VDouble0& operator+=(ثابت دوگانه v) {
        m_d += v;
        بازگشت *this;
    }
    VDouble0& operator-=(ثابت دوگانه v) {
        m_d -= v;
        بازگشت *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ثابت string m_name;  ///< Name of this statistic
    دوگانه m_مقدار;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    ثابت string m_stage;  ///< Runtime stage
    ثابت bool m_sumit;  ///< Do summation of similar stats
    ثابت bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
عمومی:
    // METHODS
    string stage() ثابت VL_MT_SAFE { بازگشت m_stage; }
    string name() ثابت VL_MT_SAFE { بازگشت m_name; }
    دوگانه مقدار() ثابت VL_MT_SAFE { بازگشت m_مقدار; }
    unsigned precision() ثابت VL_MT_SAFE { بازگشت m_precision; }
    bool sumit() ثابت VL_MT_SAFE { بازگشت m_sumit; }
    bool perf() ثابت VL_MT_SAFE { بازگشت m_perf; }
    bool printit() ثابت VL_MT_SAFE { بازگشت m_printit; }
    مجازی تهی dump(std::ofstream& os) ثابت VL_MT_SAFE;
    تهی combineWith(V3Statistic* otherp) {
        m_مقدار += otherp->مقدار();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ثابت string& stage, ثابت string& name, دوگانه مقدار, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_مقدار{مقدار}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    مجازی ~V3Statistic() = پیشفرض;
};

//============================================================================

class V3Stats final {
    ایستا V3Mutex s_mutex;  // Protects accesses

عمومی:
    // Symbolic names for some statistics that are later read by summaryReport()
    ایستا constexpr ثابت نویسه* STAT_CPP_CHARS = "Output, C++ bytes written";
    ایستا constexpr ثابت نویسه* STAT_CPP_FILES = "Output, C++ files written";
    ایستا constexpr ثابت نویسه* STAT_CPUTIME = "CPU time, Total (sec)";
    ایستا constexpr ثابت نویسه* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    ایستا constexpr ثابت نویسه* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    ایستا constexpr ثابت نویسه* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    ایستا constexpr ثابت نویسه* STAT_WALLTIME = "Wall time, Total (sec)";
    ایستا constexpr ثابت نویسه* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ایستا constexpr ثابت نویسه* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ایستا constexpr ثابت نویسه* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ایستا تهی addStat(ثابت V3Statistic&);
    ایستا تهی addStat(ثابت string& stage, ثابت string& name, دوگانه مقدار,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, مقدار, precision, false, false});
    }
    ایستا تهی addStat(ثابت string& name, دوگانه مقدار, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, مقدار, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ایستا تهی addStatSum(ثابت نویسه* name, دوگانه شمار) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        اگر (شمار == 0.0) بازگشت;
        ثابت V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, شمار, 0, true, false});
    }
    ایستا تهی addStatSum(ثابت std::string& name, دوگانه شمار) {
        addStatSum(name.c_str(), شمار);
    }
    ایستا تهی addStatPerf(ثابت string& name, دوگانه مقدار) {
        addStat(V3Statistic{"*", name, مقدار, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ایستا دوگانه getStatSum(ثابت string& name);
    ایستا uint64_t getStatSumQ(ثابت string& name) {
        بازگشت static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    ایستا تهی statsStage(ثابت string& name);
    /// Called by the top level, and error path, to collect statistics
    ایستا تهی statsStageAll(AstNetlist* nodep, ثابت string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ایستا تهی statsReport();
    /// Called by debug dumps
    ایستا تهی infoHeader(std::ofstream& os, ثابت string& prefix);
    /// Called for final build report
    ایستا تهی summaryReport();
};

#endif  // Guard
