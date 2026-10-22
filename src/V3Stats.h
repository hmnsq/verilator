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

#جے_بنا_تعریف VERILATOR_V3STATS_H_
#تعریف_کرو VERILATOR_V3STATS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    مزدوج m_d = 0.0;  ///< Count of occurrences/ value
عام:
    // METHODS
    VDouble0() = افتراضي;
    ~VDouble0() = افتراضي;

    // Implicit conversion operators:
    explicit VDouble0(ثابت uint64_t v)
        : m_d{static_cast<مزدوج>(v)} {}
    عامل مزدوج() ثابت { أرجع m_d; }

    // Explicit operators:
    VDouble0& عامل++() {  //اگیتر
        ++m_d;
        أرجع *this;
    }
    VDouble0 عامل++(صحيح) {  // postfix
        ثابت VDouble0 old = *this;
        ++m_d;
        أرجع old;
    }
    VDouble0& عامل=(ثابت مزدوج v) {
        m_d = v;
        أرجع *this;
    }
    VDouble0& عامل+=(ثابت مزدوج v) {
        m_d += v;
        أرجع *this;
    }
    VDouble0& عامل-=(ثابت مزدوج v) {
        m_d -= v;
        أرجع *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ثابت string m_name;  ///< Name of this statistic
    مزدوج m_قدر;  ///< Value of statistic (count, ratio, etc.)
    بنانشانی m_precision;  ///< Precision to print with (number of fractional digits)
    ثابت string m_stage;  ///< Runtime stage
    ثابت bool m_sumit;  ///< Do summation of similar stats
    ثابت bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
عام:
    // METHODS
    string stage() ثابت VL_MT_محفوظ { أرجع m_stage; }
    string name() ثابت VL_MT_محفوظ { أرجع m_name; }
    مزدوج قدر() ثابت VL_MT_محفوظ { أرجع m_قدر; }
    بنانشانی precision() ثابت VL_MT_محفوظ { أرجع m_precision; }
    bool sumit() ثابت VL_MT_محفوظ { أرجع m_sumit; }
    bool perf() ثابت VL_MT_محفوظ { أرجع m_perf; }
    bool printit() ثابت VL_MT_محفوظ { أرجع m_printit; }
    مجازی فراغ dump(std::ofstream& os) ثابت VL_MT_محفوظ;
    فراغ combineWith(V3Statistic* otherp) {
        m_قدر += otherp->قدر();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ثابت string& stage, ثابت string& name, مزدوج قدر, بنانشانی precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_قدر{قدر}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    مجازی ~V3Statistic() = افتراضي;
};

//============================================================================

class V3Stats final {
    ساكن V3Mutex s_mutex;  // Protects accesses

عام:
    // Symbolic names for some statistics that are later read by summaryReport()
    ساكن constexpr ثابت حرف* STAT_CPP_CHARS = "Output, C++ bytes written";
    ساكن constexpr ثابت حرف* STAT_CPP_FILES = "Output, C++ files written";
    ساكن constexpr ثابت حرف* STAT_CPUTIME = "CPU time, Total (sec)";
    ساكن constexpr ثابت حرف* STAT_MODEL_ناپ = "Size prediction, Model total (bytes)";
    ساكن constexpr ثابت حرف* STAT_سروت_CHARS = "Input, Verilog bytes read";
    ساكن constexpr ثابت حرف* STAT_سروت_MODULES = "Input, Verilog modules read";
    ساكن constexpr ثابت حرف* STAT_WALLTIME = "Wall time, Total (sec)";
    ساكن constexpr ثابت حرف* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ساكن constexpr ثابت حرف* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ساكن constexpr ثابت حرف* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ساكن فراغ جوڑو_Stat(ثابت V3Statistic&);
    ساكن فراغ جوڑو_Stat(ثابت string& stage, ثابت string& name, مزدوج قدر,
                        بنانشانی precision = 0) {
        جوڑو_Stat(V3Statistic{stage, name, قدر, precision, false, false});
    }
    ساكن فراغ جوڑو_Stat(ثابت string& name, مزدوج قدر, بنانشانی precision = 0) {
        جوڑو_Stat(V3Statistic{"*", name, قدر, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ساكن فراغ جوڑو_Stat_جوڑ(ثابت حرف* name, مزدوج گنتی) VL_MT_محفوظ_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        إذا (گنتی == 0.0) أرجع;
        ثابت V3_جندرا_Guard جندرا_2{s_mutex};
        جوڑو_Stat(V3Statistic{"*", name, گنتی, 0, true, false});
    }
    ساكن فراغ جوڑو_Stat_جوڑ(ثابت std::string& name, مزدوج گنتی) {
        جوڑو_Stat_جوڑ(name.c_str(), گنتی);
    }
    ساكن فراغ جوڑو_Stat_Perf(ثابت string& name, مزدوج قدر) {
        جوڑو_Stat(V3Statistic{"*", name, قدر, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ساكن مزدوج get_Stat_جوڑ(ثابت string& name);
    ساكن uint64_t get_Stat_جوڑ_Q(ثابت string& name) {
        أرجع static_cast<uint64_t>(get_Stat_جوڑ(name));
    }
    /// Called each stage
    ساكن فراغ statsStage(ثابت string& name);
    /// Called by the top level, and error path, to collect statistics
    ساكن فراغ statsStageAll(AstNetlist* nodep, ثابت string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ساكن فراغ statsReport();
    /// Called by debug dumps
    ساكن فراغ info_سرخی(std::ofstream& os, ثابت string& اگیتر);
    /// Called for final build report
    ساكن فراغ summaryReport();
};

#جے_انت  // Guard
