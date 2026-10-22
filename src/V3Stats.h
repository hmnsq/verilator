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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    দ্ৱিগুণ m_d = 0.0;  ///< Count of occurrences/ value
সার্ৱজনিক:
    // METHODS
    VDouble0() = স্ৱভাৱ;
    ~VDouble0() = স্ৱভাৱ;

    // Implicit conversion operators:
    explicit VDouble0(ধ্রুৱ uint64_t v)
        : m_d{static_cast<দ্ৱিগুণ>(v)} {}
    operator দ্ৱিগুণ() ধ্রুৱ { ঘূৰাই m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        ঘূৰাই *this;
    }
    VDouble0 operator++(পূর্ণাঙ্ক) {  // postfix
        ধ্রুৱ VDouble0 old = *this;
        ++m_d;
        ঘূৰাই old;
    }
    VDouble0& operator=(ধ্রুৱ দ্ৱিগুণ v) {
        m_d = v;
        ঘূৰাই *this;
    }
    VDouble0& operator+=(ধ্রুৱ দ্ৱিগুণ v) {
        m_d += v;
        ঘূৰাই *this;
    }
    VDouble0& operator-=(ধ্রুৱ দ্ৱিগুণ v) {
        m_d -= v;
        ঘূৰাই *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ধ্রুৱ string m_name;  ///< Name of this statistic
    দ্ৱিগুণ m_মান;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    ধ্রুৱ string m_stage;  ///< Runtime stage
    ধ্রুৱ bool m_sumit;  ///< Do summation of similar stats
    ধ্রুৱ bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
সার্ৱজনিক:
    // METHODS
    string stage() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_stage; }
    string name() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_name; }
    দ্ৱিগুণ মান() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_মান; }
    unsigned precision() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_precision; }
    bool sumit() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_sumit; }
    bool perf() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_perf; }
    bool printit() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_printit; }
    আভাস শূন্য dump(std::ofstream& os) ধ্রুৱ VL_MT_SAFE;
    শূন্য combineWith(V3Statistic* otherp) {
        m_মান += otherp->মান();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ধ্রুৱ string& stage, ধ্রুৱ string& name, দ্ৱিগুণ মান, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_মান{মান}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    আভাস ~V3Statistic() = স্ৱভাৱ;
};

//============================================================================

class V3Stats final {
    স্থির V3Mutex s_mutex;  // Protects accesses

সার্ৱজনিক:
    // Symbolic names for some statistics that are later read by summaryReport()
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_CPP_CHARS = "Output, C++ bytes written";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_CPP_FILES = "Output, C++ files written";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_CPUTIME = "CPU time, Total (sec)";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_WALLTIME = "Wall time, Total (sec)";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    স্থির constexpr ধ্রুৱ ৱর্ণ* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    স্থির শূন্য addStat(ধ্রুৱ V3Statistic&);
    স্থির শূন্য addStat(ধ্রুৱ string& stage, ধ্রুৱ string& name, দ্ৱিগুণ মান,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, মান, precision, false, false});
    }
    স্থির শূন্য addStat(ধ্রুৱ string& name, দ্ৱিগুণ মান, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, মান, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    স্থির শূন্য addStatSum(ধ্রুৱ ৱর্ণ* name, দ্ৱিগুণ গুনতি) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        যদি (গুনতি == 0.0) ঘূৰাই;
        ধ্রুৱ V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, গুনতি, 0, true, false});
    }
    স্থির শূন্য addStatSum(ধ্রুৱ std::string& name, দ্ৱিগুণ গুনতি) {
        addStatSum(name.c_str(), গুনতি);
    }
    স্থির শূন্য addStatPerf(ধ্রুৱ string& name, দ্ৱিগুণ মান) {
        addStat(V3Statistic{"*", name, মান, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    স্থির দ্ৱিগুণ getStatSum(ধ্রুৱ string& name);
    স্থির uint64_t getStatSumQ(ধ্রুৱ string& name) {
        ঘূৰাই static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    স্থির শূন্য statsStage(ধ্রুৱ string& name);
    /// Called by the top level, and error path, to collect statistics
    স্থির শূন্য statsStageAll(AstNetlist* nodep, ধ্রুৱ string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    স্থির শূন্য statsReport();
    /// Called by debug dumps
    স্থির শূন্য infoHeader(std::ofstream& os, ধ্রুৱ string& prefix);
    /// Called for final build report
    স্থির শূন্য summaryReport();
};

#endif  // Guard
