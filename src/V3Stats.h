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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    દ્વિગુણ m_d = 0.0;  ///< Count of occurrences/ value
જાહેર:
    // METHODS
    VDouble0() = મૂળભૂત;
    ~VDouble0() = મૂળભૂત;

    // Implicit conversion operators:
    explicit VDouble0(અચલ uint64_t v)
        : m_d{static_cast<દ્વિગુણ>(v)} {}
    operator દ્વિગુણ() અચલ { પાછા m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        પાછા *this;
    }
    VDouble0 operator++(પૂર્ણાંક) {  // postfix
        અચલ VDouble0 old = *this;
        ++m_d;
        પાછા old;
    }
    VDouble0& operator=(અચલ દ્વિગુણ v) {
        m_d = v;
        પાછા *this;
    }
    VDouble0& operator+=(અચલ દ્વિગુણ v) {
        m_d += v;
        પાછા *this;
    }
    VDouble0& operator-=(અચલ દ્વિગુણ v) {
        m_d -= v;
        પાછા *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    અચલ string m_name;  ///< Name of this statistic
    દ્વિગુણ m_કિંમત;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    અચલ string m_stage;  ///< Runtime stage
    અચલ bool m_sumit;  ///< Do summation of similar stats
    અચલ bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
જાહેર:
    // METHODS
    string stage() અચલ VL_MT_SAFE { પાછા m_stage; }
    string name() અચલ VL_MT_SAFE { પાછા m_name; }
    દ્વિગુણ કિંમત() અચલ VL_MT_SAFE { પાછા m_કિંમત; }
    unsigned precision() અચલ VL_MT_SAFE { પાછા m_precision; }
    bool sumit() અચલ VL_MT_SAFE { પાછા m_sumit; }
    bool perf() અચલ VL_MT_SAFE { પાછા m_perf; }
    bool printit() અચલ VL_MT_SAFE { પાછા m_printit; }
    આભાસી શૂન્ય dump(std::ofstream& os) અચલ VL_MT_SAFE;
    શૂન્ય combineWith(V3Statistic* otherp) {
        m_કિંમત += otherp->કિંમત();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(અચલ string& stage, અચલ string& name, દ્વિગુણ કિંમત, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_કિંમત{કિંમત}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    આભાસી ~V3Statistic() = મૂળભૂત;
};

//============================================================================

class V3Stats final {
    સ્થિર V3Mutex s_mutex;  // Protects accesses

જાહેર:
    // Symbolic names for some statistics that are later read by summaryReport()
    સ્થિર constexpr અચલ અક્ષર* STAT_CPP_CHARS = "Output, C++ bytes written";
    સ્થિર constexpr અચલ અક્ષર* STAT_CPP_FILES = "Output, C++ files written";
    સ્થિર constexpr અચલ અક્ષર* STAT_CPUTIME = "CPU time, Total (sec)";
    સ્થિર constexpr અચલ અક્ષર* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    સ્થિર constexpr અચલ અક્ષર* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    સ્થિર constexpr અચલ અક્ષર* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    સ્થિર constexpr અચલ અક્ષર* STAT_WALLTIME = "Wall time, Total (sec)";
    સ્થિર constexpr અચલ અક્ષર* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    સ્થિર constexpr અચલ અક્ષર* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    સ્થિર constexpr અચલ અક્ષર* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    સ્થિર શૂન્ય addStat(અચલ V3Statistic&);
    સ્થિર શૂન્ય addStat(અચલ string& stage, અચલ string& name, દ્વિગુણ કિંમત,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, કિંમત, precision, false, false});
    }
    સ્થિર શૂન્ય addStat(અચલ string& name, દ્વિગુણ કિંમત, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, કિંમત, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    સ્થિર શૂન્ય addStatSum(અચલ અક્ષર* name, દ્વિગુણ ગણતરી) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        જો (ગણતરી == 0.0) પાછા;
        અચલ V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, ગણતરી, 0, true, false});
    }
    સ્થિર શૂન્ય addStatSum(અચલ std::string& name, દ્વિગુણ ગણતરી) {
        addStatSum(name.c_str(), ગણતરી);
    }
    સ્થિર શૂન્ય addStatPerf(અચલ string& name, દ્વિગુણ કિંમત) {
        addStat(V3Statistic{"*", name, કિંમત, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    સ્થિર દ્વિગુણ getStatSum(અચલ string& name);
    સ્થિર uint64_t getStatSumQ(અચલ string& name) {
        પાછા static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    સ્થિર શૂન્ય statsStage(અચલ string& name);
    /// Called by the top level, and error path, to collect statistics
    સ્થિર શૂન્ય statsStageAll(AstNetlist* nodep, અચલ string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    સ્થિર શૂન્ય statsReport();
    /// Called by debug dumps
    સ્થિર શૂન્ય infoHeader(std::ofstream& os, અચલ string& prefix);
    /// Called for final build report
    સ્થિર શૂન્ય summaryReport();
};

#endif  // Guard
