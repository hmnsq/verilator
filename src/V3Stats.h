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

#నిర్వచితం_కాకపోతే VERILATOR_V3STATS_H_
#నిర్వచించు VERILATOR_V3STATS_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    ద్విగుణ m_d = 0.0;  ///< Count of occurrences/ value
బహిరంగ:
    // METHODS
    VDouble0() = అప్రమేయ;
    ~VDouble0() = అప్రమేయ;

    // Implicit conversion operators:
    explicit VDouble0(స్థిరరాశి uint64_t v)
        : m_d{static_cast<ద్విగుణ>(v)} {}
    క్రియాకారకం ద్విగుణ() స్థిరరాశి { తిరిగి m_d; }

    // Explicit operators:
    VDouble0& క్రియాకారకం++() {  //ఉపసర్గ
        ++m_d;
        తిరిగి *this;
    }
    VDouble0 క్రియాకారకం++(పూర్ణాంకం) {  // postfix
        స్థిరరాశి VDouble0 old = *this;
        ++m_d;
        తిరిగి old;
    }
    VDouble0& క్రియాకారకం=(స్థిరరాశి ద్విగుణ v) {
        m_d = v;
        తిరిగి *this;
    }
    VDouble0& క్రియాకారకం+=(స్థిరరాశి ద్విగుణ v) {
        m_d += v;
        తిరిగి *this;
    }
    VDouble0& క్రియాకారకం-=(స్థిరరాశి ద్విగుణ v) {
        m_d -= v;
        తిరిగి *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    స్థిరరాశి string m_name;  ///< Name of this statistic
    ద్విగుణ m_విలువ;  ///< Value of statistic (count, ratio, etc.)
    గుర్తులేని m_precision;  ///< Precision to print with (number of fractional digits)
    స్థిరరాశి string m_stage;  ///< Runtime stage
    స్థిరరాశి bool m_sumit;  ///< Do summation of similar stats
    స్థిరరాశి bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
బహిరంగ:
    // METHODS
    string stage() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_stage; }
    string name() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_name; }
    ద్విగుణ విలువ() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_విలువ; }
    గుర్తులేని precision() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_precision; }
    bool sumit() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_sumit; }
    bool perf() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_perf; }
    bool printit() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_printit; }
    మాయా శూన్య dump(std::ofstream& os) స్థిరరాశి VL_MT_సురక్షిత;
    శూన్య combineWith(V3Statistic* otherp) {
        m_విలువ += otherp->విలువ();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(స్థిరరాశి string& stage, స్థిరరాశి string& name, ద్విగుణ విలువ, గుర్తులేని precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_విలువ{విలువ}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    మాయా ~V3Statistic() = అప్రమేయ;
};

//============================================================================

class V3Stats final {
    స్థిర V3Mutex s_mutex;  // Protects accesses

బహిరంగ:
    // Symbolic names for some statistics that are later read by summaryReport()
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_CPP_CHARS = "Output, C++ bytes written";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_CPP_FILES = "Output, C++ files written";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_CPUTIME = "CPU time, Total (sec)";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_MODEL_కొలత = "Size prediction, Model total (bytes)";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_ఆధారం_CHARS = "Input, Verilog bytes read";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_ఆధారం_MODULES = "Input, Verilog modules read";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_WALLTIME = "Wall time, Total (sec)";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    స్థిర constexpr స్థిరరాశి అక్షరం* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    స్థిర శూన్య కూడు_Stat(స్థిరరాశి V3Statistic&);
    స్థిర శూన్య కూడు_Stat(స్థిరరాశి string& stage, స్థిరరాశి string& name, ద్విగుణ విలువ,
                        గుర్తులేని precision = 0) {
        కూడు_Stat(V3Statistic{stage, name, విలువ, precision, false, false});
    }
    స్థిర శూన్య కూడు_Stat(స్థిరరాశి string& name, ద్విగుణ విలువ, గుర్తులేని precision = 0) {
        కూడు_Stat(V3Statistic{"*", name, విలువ, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    స్థిర శూన్య కూడు_Stat_కూడిక(స్థిరరాశి అక్షరం* name, ద్విగుణ లెక్క) VL_MT_సురక్షిత_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        ఒకవేళ (లెక్క == 0.0) తిరిగి;
        స్థిరరాశి V3_తాళం_Guard తాళం_2{s_mutex};
        కూడు_Stat(V3Statistic{"*", name, లెక్క, 0, true, false});
    }
    స్థిర శూన్య కూడు_Stat_కూడిక(స్థిరరాశి std::string& name, ద్విగుణ లెక్క) {
        కూడు_Stat_కూడిక(name.c_str(), లెక్క);
    }
    స్థిర శూన్య కూడు_Stat_Perf(స్థిరరాశి string& name, ద్విగుణ విలువ) {
        కూడు_Stat(V3Statistic{"*", name, విలువ, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    స్థిర ద్విగుణ get_Stat_కూడిక(స్థిరరాశి string& name);
    స్థిర uint64_t get_Stat_కూడిక_Q(స్థిరరాశి string& name) {
        తిరిగి static_cast<uint64_t>(get_Stat_కూడిక(name));
    }
    /// Called each stage
    స్థిర శూన్య statsStage(స్థిరరాశి string& name);
    /// Called by the top level, and error path, to collect statistics
    స్థిర శూన్య statsStageAll(AstNetlist* nodep, స్థిరరాశి string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    స్థిర శూన్య statsReport();
    /// Called by debug dumps
    స్థిర శూన్య info_శీర్షిక(std::ofstream& os, స్థిరరాశి string& ఉపసర్గ);
    /// Called for final build report
    స్థిర శూన్య summaryReport();
};

#అయితే_ముగింపు  // Guard
