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

#اگر_غیر_تعریف_شدہ VERILATOR_V3STATS_H_
#تعریف_کرو VERILATOR_V3STATS_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    دوہرا m_d = 0.0;  ///< Count of occurrences/ value
عوامی:
    // METHODS
    VDouble0() = طےشدہ;
    ~VDouble0() = طےشدہ;

    // Implicit conversion operators:
    explicit VDouble0(ثابت uint64_t v)
        : m_d{static_cast<دوہرا>(v)} {}
    عامل دوہرا() ثابت { واپس m_d; }

    // Explicit operators:
    VDouble0& عامل++() {  //سابقہ
        ++m_d;
        واپس *this;
    }
    VDouble0 عامل++(صحیح) {  // postfix
        ثابت VDouble0 old = *this;
        ++m_d;
        واپس old;
    }
    VDouble0& عامل=(ثابت دوہرا v) {
        m_d = v;
        واپس *this;
    }
    VDouble0& عامل+=(ثابت دوہرا v) {
        m_d += v;
        واپس *this;
    }
    VDouble0& عامل-=(ثابت دوہرا v) {
        m_d -= v;
        واپس *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ثابت string m_name;  ///< Name of this statistic
    دوہرا m_قدر;  ///< Value of statistic (count, ratio, etc.)
    غیرعلامتی m_precision;  ///< Precision to print with (number of fractional digits)
    ثابت string m_stage;  ///< Runtime stage
    ثابت bool m_sumit;  ///< Do summation of similar stats
    ثابت bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
عوامی:
    // METHODS
    string stage() ثابت VL_MT_محفوظ { واپس m_stage; }
    string name() ثابت VL_MT_محفوظ { واپس m_name; }
    دوہرا قدر() ثابت VL_MT_محفوظ { واپس m_قدر; }
    غیرعلامتی precision() ثابت VL_MT_محفوظ { واپس m_precision; }
    bool sumit() ثابت VL_MT_محفوظ { واپس m_sumit; }
    bool perf() ثابت VL_MT_محفوظ { واپس m_perf; }
    bool printit() ثابت VL_MT_محفوظ { واپس m_printit; }
    مجازی خالی dump(std::ofstream& os) ثابت VL_MT_محفوظ;
    خالی combineWith(V3Statistic* otherp) {
        m_قدر += otherp->قدر();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ثابت string& stage, ثابت string& name, دوہرا قدر, غیرعلامتی precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_قدر{قدر}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    مجازی ~V3Statistic() = طےشدہ;
};

//============================================================================

class V3Stats final {
    ساکن V3Mutex s_mutex;  // Protects accesses

عوامی:
    // Symbolic names for some statistics that are later read by summaryReport()
    ساکن constexpr ثابت حرف* STAT_CPP_CHARS = "Output, C++ bytes written";
    ساکن constexpr ثابت حرف* STAT_CPP_FILES = "Output, C++ files written";
    ساکن constexpr ثابت حرف* STAT_CPUTIME = "CPU time, Total (sec)";
    ساکن constexpr ثابت حرف* STAT_MODEL_سائز = "Size prediction, Model total (bytes)";
    ساکن constexpr ثابت حرف* STAT_ماخذ_CHARS = "Input, Verilog bytes read";
    ساکن constexpr ثابت حرف* STAT_ماخذ_MODULES = "Input, Verilog modules read";
    ساکن constexpr ثابت حرف* STAT_WALLTIME = "Wall time, Total (sec)";
    ساکن constexpr ثابت حرف* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ساکن constexpr ثابت حرف* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ساکن constexpr ثابت حرف* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ساکن خالی جوڑو_Stat(ثابت V3Statistic&);
    ساکن خالی جوڑو_Stat(ثابت string& stage, ثابت string& name, دوہرا قدر,
                        غیرعلامتی precision = 0) {
        جوڑو_Stat(V3Statistic{stage, name, قدر, precision, false, false});
    }
    ساکن خالی جوڑو_Stat(ثابت string& name, دوہرا قدر, غیرعلامتی precision = 0) {
        جوڑو_Stat(V3Statistic{"*", name, قدر, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ساکن خالی جوڑو_Stat_مجموع(ثابت حرف* name, دوہرا گنتی) VL_MT_محفوظ_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        اگر (گنتی == 0.0) واپس;
        ثابت V3_تالا_Guard تالا_2{s_mutex};
        جوڑو_Stat(V3Statistic{"*", name, گنتی, 0, true, false});
    }
    ساکن خالی جوڑو_Stat_مجموع(ثابت std::string& name, دوہرا گنتی) {
        جوڑو_Stat_مجموع(name.c_str(), گنتی);
    }
    ساکن خالی جوڑو_Stat_Perf(ثابت string& name, دوہرا قدر) {
        جوڑو_Stat(V3Statistic{"*", name, قدر, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ساکن دوہرا get_Stat_مجموع(ثابت string& name);
    ساکن uint64_t get_Stat_مجموع_Q(ثابت string& name) {
        واپس static_cast<uint64_t>(get_Stat_مجموع(name));
    }
    /// Called each stage
    ساکن خالی statsStage(ثابت string& name);
    /// Called by the top level, and error path, to collect statistics
    ساکن خالی statsStageAll(AstNetlist* nodep, ثابت string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ساکن خالی statsReport();
    /// Called by debug dumps
    ساکن خالی info_سرخی(std::ofstream& os, ثابت string& سابقہ);
    /// Called for final build report
    ساکن خالی summaryReport();
};

#اگر_ختم  // Guard
