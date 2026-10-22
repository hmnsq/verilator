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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    ഇരട്ട m_d = 0.0;  ///< Count of occurrences/ value
പൊതു:
    // METHODS
    VDouble0() = സ്ഥിരസ്ഥിതി;
    ~VDouble0() = സ്ഥിരസ്ഥിതി;

    // Implicit conversion operators:
    explicit VDouble0(സ്ഥിരം uint64_t v)
        : m_d{static_cast<ഇരട്ട>(v)} {}
    operator ഇരട്ട() സ്ഥിരം { തിരികെ m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        തിരികെ *this;
    }
    VDouble0 operator++(പൂർണ്ണസംഖ്യ) {  // postfix
        സ്ഥിരം VDouble0 old = *this;
        ++m_d;
        തിരികെ old;
    }
    VDouble0& operator=(സ്ഥിരം ഇരട്ട v) {
        m_d = v;
        തിരികെ *this;
    }
    VDouble0& operator+=(സ്ഥിരം ഇരട്ട v) {
        m_d += v;
        തിരികെ *this;
    }
    VDouble0& operator-=(സ്ഥിരം ഇരട്ട v) {
        m_d -= v;
        തിരികെ *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    സ്ഥിരം string m_name;  ///< Name of this statistic
    ഇരട്ട m_മൂല്യം;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    സ്ഥിരം string m_stage;  ///< Runtime stage
    സ്ഥിരം bool m_sumit;  ///< Do summation of similar stats
    സ്ഥിരം bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
പൊതു:
    // METHODS
    string stage() സ്ഥിരം VL_MT_SAFE { തിരികെ m_stage; }
    string name() സ്ഥിരം VL_MT_SAFE { തിരികെ m_name; }
    ഇരട്ട മൂല്യം() സ്ഥിരം VL_MT_SAFE { തിരികെ m_മൂല്യം; }
    unsigned precision() സ്ഥിരം VL_MT_SAFE { തിരികെ m_precision; }
    bool sumit() സ്ഥിരം VL_MT_SAFE { തിരികെ m_sumit; }
    bool perf() സ്ഥിരം VL_MT_SAFE { തിരികെ m_perf; }
    bool printit() സ്ഥിരം VL_MT_SAFE { തിരികെ m_printit; }
    മായാ ശൂന്യം dump(std::ofstream& os) സ്ഥിരം VL_MT_SAFE;
    ശൂന്യം combineWith(V3Statistic* otherp) {
        m_മൂല്യം += otherp->മൂല്യം();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(സ്ഥിരം string& stage, സ്ഥിരം string& name, ഇരട്ട മൂല്യം, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_മൂല്യം{മൂല്യം}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    മായാ ~V3Statistic() = സ്ഥിരസ്ഥിതി;
};

//============================================================================

class V3Stats final {
    സ്ഥിതം V3Mutex s_mutex;  // Protects accesses

പൊതു:
    // Symbolic names for some statistics that are later read by summaryReport()
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_CPP_CHARS = "Output, C++ bytes written";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_CPP_FILES = "Output, C++ files written";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_CPUTIME = "CPU time, Total (sec)";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_WALLTIME = "Wall time, Total (sec)";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    സ്ഥിതം ശൂന്യം addStat(സ്ഥിരം V3Statistic&);
    സ്ഥിതം ശൂന്യം addStat(സ്ഥിരം string& stage, സ്ഥിരം string& name, ഇരട്ട മൂല്യം,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, മൂല്യം, precision, false, false});
    }
    സ്ഥിതം ശൂന്യം addStat(സ്ഥിരം string& name, ഇരട്ട മൂല്യം, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, മൂല്യം, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    സ്ഥിതം ശൂന്യം addStatSum(സ്ഥിരം അക്ഷരം* name, ഇരട്ട എണ്ണം) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        എങ്കിൽ (എണ്ണം == 0.0) തിരികെ;
        സ്ഥിരം V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, എണ്ണം, 0, true, false});
    }
    സ്ഥിതം ശൂന്യം addStatSum(സ്ഥിരം std::string& name, ഇരട്ട എണ്ണം) {
        addStatSum(name.c_str(), എണ്ണം);
    }
    സ്ഥിതം ശൂന്യം addStatPerf(സ്ഥിരം string& name, ഇരട്ട മൂല്യം) {
        addStat(V3Statistic{"*", name, മൂല്യം, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    സ്ഥിതം ഇരട്ട getStatSum(സ്ഥിരം string& name);
    സ്ഥിതം uint64_t getStatSumQ(സ്ഥിരം string& name) {
        തിരികെ static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    സ്ഥിതം ശൂന്യം statsStage(സ്ഥിരം string& name);
    /// Called by the top level, and error path, to collect statistics
    സ്ഥിതം ശൂന്യം statsStageAll(AstNetlist* nodep, സ്ഥിരം string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    സ്ഥിതം ശൂന്യം statsReport();
    /// Called by debug dumps
    സ്ഥിതം ശൂന്യം infoHeader(std::ofstream& os, സ്ഥിരം string& prefix);
    /// Called for final build report
    സ്ഥിതം ശൂന്യം summaryReport();
};

#endif  // Guard
