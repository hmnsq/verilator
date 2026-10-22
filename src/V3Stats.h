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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    দ্বিগুণ m_d = 0.0;  ///< Count of occurrences/ value
সার্বজনিক:
    // METHODS
    VDouble0() = স্বভাব;
    ~VDouble0() = স্বভাব;

    // Implicit conversion operators:
    explicit VDouble0(ধ্রুব uint64_t v)
        : m_d{static_cast<দ্বিগুণ>(v)} {}
    operator দ্বিগুণ() ধ্রুব { প্রত্যাগম m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        প্রত্যাগম *this;
    }
    VDouble0 operator++(পূর্ণাঙ্ক) {  // postfix
        ধ্রুব VDouble0 old = *this;
        ++m_d;
        প্রত্যাগম old;
    }
    VDouble0& operator=(ধ্রুব দ্বিগুণ v) {
        m_d = v;
        প্রত্যাগম *this;
    }
    VDouble0& operator+=(ধ্রুব দ্বিগুণ v) {
        m_d += v;
        প্রত্যাগম *this;
    }
    VDouble0& operator-=(ধ্রুব দ্বিগুণ v) {
        m_d -= v;
        প্রত্যাগম *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ধ্রুব string m_name;  ///< Name of this statistic
    দ্বিগুণ m_মান;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    ধ্রুব string m_stage;  ///< Runtime stage
    ধ্রুব bool m_sumit;  ///< Do summation of similar stats
    ধ্রুব bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
সার্বজনিক:
    // METHODS
    string stage() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_stage; }
    string name() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_name; }
    দ্বিগুণ মান() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_মান; }
    unsigned precision() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_precision; }
    bool sumit() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_sumit; }
    bool perf() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_perf; }
    bool printit() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_printit; }
    আভাস শূন্য dump(std::ofstream& os) ধ্রুব VL_MT_SAFE;
    শূন্য combineWith(V3Statistic* otherp) {
        m_মান += otherp->মান();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ধ্রুব string& stage, ধ্রুব string& name, দ্বিগুণ মান, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_মান{মান}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    আভাস ~V3Statistic() = স্বভাব;
};

//============================================================================

class V3Stats final {
    স্থির V3Mutex s_mutex;  // Protects accesses

সার্বজনিক:
    // Symbolic names for some statistics that are later read by summaryReport()
    স্থির constexpr ধ্রুব বর্ণ* STAT_CPP_CHARS = "Output, C++ bytes written";
    স্থির constexpr ধ্রুব বর্ণ* STAT_CPP_FILES = "Output, C++ files written";
    স্থির constexpr ধ্রুব বর্ণ* STAT_CPUTIME = "CPU time, Total (sec)";
    স্থির constexpr ধ্রুব বর্ণ* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    স্থির constexpr ধ্রুব বর্ণ* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    স্থির constexpr ধ্রুব বর্ণ* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    স্থির constexpr ধ্রুব বর্ণ* STAT_WALLTIME = "Wall time, Total (sec)";
    স্থির constexpr ধ্রুব বর্ণ* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    স্থির constexpr ধ্রুব বর্ণ* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    স্থির constexpr ধ্রুব বর্ণ* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    স্থির শূন্য addStat(ধ্রুব V3Statistic&);
    স্থির শূন্য addStat(ধ্রুব string& stage, ধ্রুব string& name, দ্বিগুণ মান,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, মান, precision, false, false});
    }
    স্থির শূন্য addStat(ধ্রুব string& name, দ্বিগুণ মান, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, মান, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    স্থির শূন্য addStatSum(ধ্রুব বর্ণ* name, দ্বিগুণ গুনতি) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        যদি (গুনতি == 0.0) প্রত্যাগম;
        ধ্রুব V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, গুনতি, 0, true, false});
    }
    স্থির শূন্য addStatSum(ধ্রুব std::string& name, দ্বিগুণ গুনতি) {
        addStatSum(name.c_str(), গুনতি);
    }
    স্থির শূন্য addStatPerf(ধ্রুব string& name, দ্বিগুণ মান) {
        addStat(V3Statistic{"*", name, মান, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    স্থির দ্বিগুণ getStatSum(ধ্রুব string& name);
    স্থির uint64_t getStatSumQ(ধ্রুব string& name) {
        প্রত্যাগম static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    স্থির শূন্য statsStage(ধ্রুব string& name);
    /// Called by the top level, and error path, to collect statistics
    স্থির শূন্য statsStageAll(AstNetlist* nodep, ধ্রুব string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    স্থির শূন্য statsReport();
    /// Called by debug dumps
    স্থির শূন্য infoHeader(std::ofstream& os, ধ্রুব string& prefix);
    /// Called for final build report
    স্থির শূন্য summaryReport();
};

#endif  // Guard
