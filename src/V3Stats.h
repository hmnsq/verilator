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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3STATS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3STATS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    ਦੁੱਗਣਾ m_d = 0.0;  ///< Count of occurrences/ value
ਜਨਤਕ:
    // METHODS
    VDouble0() = ਮੂਲ;
    ~VDouble0() = ਮੂਲ;

    // Implicit conversion operators:
    explicit VDouble0(ਅਚਲ uint64_t v)
        : m_d{static_cast<ਦੁੱਗਣਾ>(v)} {}
    ਸੰਚਾਲਕ ਦੁੱਗਣਾ() ਅਚਲ { ਵਾਪਸ m_d; }

    // Explicit operators:
    VDouble0& ਸੰਚਾਲਕ++() {  //ਅਗੇਤਰ
        ++m_d;
        ਵਾਪਸ *this;
    }
    VDouble0 ਸੰਚਾਲਕ++(ਪੂਰਨ) {  // postfix
        ਅਚਲ VDouble0 old = *this;
        ++m_d;
        ਵਾਪਸ old;
    }
    VDouble0& ਸੰਚਾਲਕ=(ਅਚਲ ਦੁੱਗਣਾ v) {
        m_d = v;
        ਵਾਪਸ *this;
    }
    VDouble0& ਸੰਚਾਲਕ+=(ਅਚਲ ਦੁੱਗਣਾ v) {
        m_d += v;
        ਵਾਪਸ *this;
    }
    VDouble0& ਸੰਚਾਲਕ-=(ਅਚਲ ਦੁੱਗਣਾ v) {
        m_d -= v;
        ਵਾਪਸ *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ਅਚਲ string m_name;  ///< Name of this statistic
    ਦੁੱਗਣਾ m_ਮੁੱਲ;  ///< Value of statistic (count, ratio, etc.)
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_precision;  ///< Precision to print with (number of fractional digits)
    ਅਚਲ string m_stage;  ///< Runtime stage
    ਅਚਲ bool m_sumit;  ///< Do summation of similar stats
    ਅਚਲ bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
ਜਨਤਕ:
    // METHODS
    string stage() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_stage; }
    string name() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_name; }
    ਦੁੱਗਣਾ ਮੁੱਲ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਮੁੱਲ; }
    ਬਿਨਾਨਿਸ਼ਾਨੀ precision() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_precision; }
    bool sumit() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_sumit; }
    bool perf() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_perf; }
    bool printit() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_printit; }
    ਆਭਾਸੀ ਖਾਲੀ dump(std::ofstream& os) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਖਾਲੀ combineWith(V3Statistic* otherp) {
        m_ਮੁੱਲ += otherp->ਮੁੱਲ();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ਅਚਲ string& stage, ਅਚਲ string& name, ਦੁੱਗਣਾ ਮੁੱਲ, ਬਿਨਾਨਿਸ਼ਾਨੀ precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_ਮੁੱਲ{ਮੁੱਲ}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    ਆਭਾਸੀ ~V3Statistic() = ਮੂਲ;
};

//============================================================================

class V3Stats final {
    ਸਥਿਰ V3Mutex s_mutex;  // Protects accesses

ਜਨਤਕ:
    // Symbolic names for some statistics that are later read by summaryReport()
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_CPP_CHARS = "Output, C++ bytes written";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_CPP_FILES = "Output, C++ files written";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_CPUTIME = "CPU time, Total (sec)";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_MODEL_ਮਾਪ = "Size prediction, Model total (bytes)";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_ਸਰੋਤ_CHARS = "Input, Verilog bytes read";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_ਸਰੋਤ_MODULES = "Input, Verilog modules read";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_WALLTIME = "Wall time, Total (sec)";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat(ਅਚਲ V3Statistic&);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat(ਅਚਲ string& stage, ਅਚਲ string& name, ਦੁੱਗਣਾ ਮੁੱਲ,
                        ਬਿਨਾਨਿਸ਼ਾਨੀ precision = 0) {
        ਜੋੜੋ_Stat(V3Statistic{stage, name, ਮੁੱਲ, precision, false, false});
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat(ਅਚਲ string& name, ਦੁੱਗਣਾ ਮੁੱਲ, ਬਿਨਾਨਿਸ਼ਾਨੀ precision = 0) {
        ਜੋੜੋ_Stat(V3Statistic{"*", name, ਮੁੱਲ, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat_ਜੋੜ(ਅਚਲ ਅੱਖਰ* name, ਦੁੱਗਣਾ ਗਿਣਤੀ_2) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        ਜੇ (ਗਿਣਤੀ_2 == 0.0) ਵਾਪਸ;
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_mutex};
        ਜੋੜੋ_Stat(V3Statistic{"*", name, ਗਿਣਤੀ_2, 0, true, false});
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat_ਜੋੜ(ਅਚਲ std::string& name, ਦੁੱਗਣਾ ਗਿਣਤੀ_2) {
        ਜੋੜੋ_Stat_ਜੋੜ(name.c_str(), ਗਿਣਤੀ_2);
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat_Perf(ਅਚਲ string& name, ਦੁੱਗਣਾ ਮੁੱਲ) {
        ਜੋੜੋ_Stat(V3Statistic{"*", name, ਮੁੱਲ, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ਸਥਿਰ ਦੁੱਗਣਾ get_Stat_ਜੋੜ(ਅਚਲ string& name);
    ਸਥਿਰ uint64_t get_Stat_ਜੋੜ_Q(ਅਚਲ string& name) {
        ਵਾਪਸ static_cast<uint64_t>(get_Stat_ਜੋੜ(name));
    }
    /// Called each stage
    ਸਥਿਰ ਖਾਲੀ statsStage(ਅਚਲ string& name);
    /// Called by the top level, and error path, to collect statistics
    ਸਥਿਰ ਖਾਲੀ statsStageAll(AstNetlist* nodep, ਅਚਲ string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ਸਥਿਰ ਖਾਲੀ statsReport();
    /// Called by debug dumps
    ਸਥਿਰ ਖਾਲੀ info_ਸਿਰਲੇਖ(std::ofstream& os, ਅਚਲ string& ਅਗੇਤਰ);
    /// Called for final build report
    ਸਥਿਰ ਖਾਲੀ summaryReport();
};

#ਜੇ_ਅੰਤ  // Guard
