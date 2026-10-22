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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    ଦ୍ୱିଗୁଣ m_d = 0.0;  ///< Count of occurrences/ value
ସର୍ବସାଧାରଣ:
    // METHODS
    VDouble0() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VDouble0() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // Implicit conversion operators:
    explicit VDouble0(ଅଚଳ uint64_t v)
        : m_d{static_cast<ଦ୍ୱିଗୁଣ>(v)} {}
    operator ଦ୍ୱିଗୁଣ() ଅଚଳ { ଫେରନ୍ତୁ m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        ଫେରନ୍ତୁ *this;
    }
    VDouble0 operator++(ପୂର୍ଣ୍ଣସଂଖ୍ୟା) {  // postfix
        ଅଚଳ VDouble0 old = *this;
        ++m_d;
        ଫେରନ୍ତୁ old;
    }
    VDouble0& operator=(ଅଚଳ ଦ୍ୱିଗୁଣ v) {
        m_d = v;
        ଫେରନ୍ତୁ *this;
    }
    VDouble0& operator+=(ଅଚଳ ଦ୍ୱିଗୁଣ v) {
        m_d += v;
        ଫେରନ୍ତୁ *this;
    }
    VDouble0& operator-=(ଅଚଳ ଦ୍ୱିଗୁଣ v) {
        m_d -= v;
        ଫେରନ୍ତୁ *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ଅଚଳ string m_name;  ///< Name of this statistic
    ଦ୍ୱିଗୁଣ m_ମୂଲ୍ୟ;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    ଅଚଳ string m_stage;  ///< Runtime stage
    ଅଚଳ bool m_sumit;  ///< Do summation of similar stats
    ଅଚଳ bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
ସର୍ବସାଧାରଣ:
    // METHODS
    string stage() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_stage; }
    string name() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_name; }
    ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ; }
    unsigned precision() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_precision; }
    bool sumit() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_sumit; }
    bool perf() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_perf; }
    bool printit() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_printit; }
    ଆଭାସୀ ଶୂନ୍ୟ dump(std::ofstream& os) ଅଚଳ VL_MT_SAFE;
    ଶୂନ୍ୟ combineWith(V3Statistic* otherp) {
        m_ମୂଲ୍ୟ += otherp->ମୂଲ୍ୟ();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ଅଚଳ string& stage, ଅଚଳ string& name, ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_ମୂଲ୍ୟ{ମୂଲ୍ୟ}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    ଆଭାସୀ ~V3Statistic() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//============================================================================

class V3Stats final {
    ସ୍ଥିର V3Mutex s_mutex;  // Protects accesses

ସର୍ବସାଧାରଣ:
    // Symbolic names for some statistics that are later read by summaryReport()
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_CPP_CHARS = "Output, C++ bytes written";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_CPP_FILES = "Output, C++ files written";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_CPUTIME = "CPU time, Total (sec)";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_WALLTIME = "Wall time, Total (sec)";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ସ୍ଥିର ଶୂନ୍ୟ addStat(ଅଚଳ V3Statistic&);
    ସ୍ଥିର ଶୂନ୍ୟ addStat(ଅଚଳ string& stage, ଅଚଳ string& name, ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, ମୂଲ୍ୟ, precision, false, false});
    }
    ସ୍ଥିର ଶୂନ୍ୟ addStat(ଅଚଳ string& name, ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, ମୂଲ୍ୟ, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ସ୍ଥିର ଶୂନ୍ୟ addStatSum(ଅଚଳ ଅକ୍ଷର* name, ଦ୍ୱିଗୁଣ ଗଣନା_2) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        ଯଦି (ଗଣନା_2 == 0.0) ଫେରନ୍ତୁ;
        ଅଚଳ V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, ଗଣନା_2, 0, true, false});
    }
    ସ୍ଥିର ଶୂନ୍ୟ addStatSum(ଅଚଳ std::string& name, ଦ୍ୱିଗୁଣ ଗଣନା_2) {
        addStatSum(name.c_str(), ଗଣନା_2);
    }
    ସ୍ଥିର ଶୂନ୍ୟ addStatPerf(ଅଚଳ string& name, ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ) {
        addStat(V3Statistic{"*", name, ମୂଲ୍ୟ, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ସ୍ଥିର ଦ୍ୱିଗୁଣ getStatSum(ଅଚଳ string& name);
    ସ୍ଥିର uint64_t getStatSumQ(ଅଚଳ string& name) {
        ଫେରନ୍ତୁ static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    ସ୍ଥିର ଶୂନ୍ୟ statsStage(ଅଚଳ string& name);
    /// Called by the top level, and error path, to collect statistics
    ସ୍ଥିର ଶୂନ୍ୟ statsStageAll(AstNetlist* nodep, ଅଚଳ string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ସ୍ଥିର ଶୂନ୍ୟ statsReport();
    /// Called by debug dumps
    ସ୍ଥିର ଶୂନ୍ୟ infoHeader(std::ofstream& os, ଅଚଳ string& prefix);
    /// Called for final build report
    ସ୍ଥିର ଶୂନ୍ୟ summaryReport();
};

#endif  // Guard
