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

#अगर_अघोषित VERILATOR_V3STATS_H_
#घोषणा VERILATOR_V3STATS_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    द्विगुन m_d = 0.0;  ///< Count of occurrences/ value
खुला:
    // METHODS
    VDouble0() = शेष;
    ~VDouble0() = शेष;

    // Implicit conversion operators:
    explicit VDouble0(स्थिर uint64_t v)
        : m_d{static_cast<द्विगुन>(v)} {}
    चालक द्विगुन() स्थिर { वापस m_d; }

    // Explicit operators:
    VDouble0& चालक++() {  // prefix
        ++m_d;
        वापस *this;
    }
    VDouble0 चालक++(पूर्णांक) {  // postfix
        स्थिर VDouble0 old = *this;
        ++m_d;
        वापस old;
    }
    VDouble0& चालक=(स्थिर द्विगुन v) {
        m_d = v;
        वापस *this;
    }
    VDouble0& चालक+=(स्थिर द्विगुन v) {
        m_d += v;
        वापस *this;
    }
    VDouble0& चालक-=(स्थिर द्विगुन v) {
        m_d -= v;
        वापस *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    स्थिर string m_name;  ///< Name of this statistic
    द्विगुन m_मूल्य;  ///< Value of statistic (count, ratio, etc.)
    अचिन्हित m_precision;  ///< Precision to print with (number of fractional digits)
    स्थिर string m_stage;  ///< Runtime stage
    स्थिर bool m_sumit;  ///< Do summation of similar stats
    स्थिर bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
खुला:
    // METHODS
    string stage() स्थिर VL_MT_SAFE { वापस m_stage; }
    string name() स्थिर VL_MT_SAFE { वापस m_name; }
    द्विगुन मूल्य() स्थिर VL_MT_SAFE { वापस m_मूल्य; }
    अचिन्हित precision() स्थिर VL_MT_SAFE { वापस m_precision; }
    bool sumit() स्थिर VL_MT_SAFE { वापस m_sumit; }
    bool perf() स्थिर VL_MT_SAFE { वापस m_perf; }
    bool printit() स्थिर VL_MT_SAFE { वापस m_printit; }
    भव व्योम dump(std::ofstream& os) स्थिर VL_MT_SAFE;
    व्योम combineWith(V3Statistic* otherp) {
        m_मूल्य += otherp->मूल्य();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(स्थिर string& stage, स्थिर string& name, द्विगुन मूल्य, अचिन्हित precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_मूल्य{मूल्य}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    भव ~V3Statistic() = शेष;
};

//============================================================================

class V3Stats final {
    अटल V3Mutex s_mutex;  // Protects accesses

खुला:
    // Symbolic names for some statistics that are later read by summaryReport()
    अटल constexpr स्थिर अक्षर* STAT_CPP_CHARS = "Output, C++ bytes written";
    अटल constexpr स्थिर अक्षर* STAT_CPP_FILES = "Output, C++ files written";
    अटल constexpr स्थिर अक्षर* STAT_CPUTIME = "CPU time, Total (sec)";
    अटल constexpr स्थिर अक्षर* STAT_MODEL_आकार = "Size prediction, Model total (bytes)";
    अटल constexpr स्थिर अक्षर* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    अटल constexpr स्थिर अक्षर* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    अटल constexpr स्थिर अक्षर* STAT_WALLTIME = "Wall time, Total (sec)";
    अटल constexpr स्थिर अक्षर* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    अटल constexpr स्थिर अक्षर* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    अटल constexpr स्थिर अक्षर* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    अटल व्योम addStat(स्थिर V3Statistic&);
    अटल व्योम addStat(स्थिर string& stage, स्थिर string& name, द्विगुन मूल्य,
                        अचिन्हित precision = 0) {
        addStat(V3Statistic{stage, name, मूल्य, precision, false, false});
    }
    अटल व्योम addStat(स्थिर string& name, द्विगुन मूल्य, अचिन्हित precision = 0) {
        addStat(V3Statistic{"*", name, मूल्य, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    अटल व्योम add_Stat_योग(स्थिर अक्षर* name, द्विगुन गिनती) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        अगर (गिनती == 0.0) वापस;
        स्थिर V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, गिनती, 0, true, false});
    }
    अटल व्योम add_Stat_योग(स्थिर std::string& name, द्विगुन गिनती) {
        add_Stat_योग(name.c_str(), गिनती);
    }
    अटल व्योम addStatPerf(स्थिर string& name, द्विगुन मूल्य) {
        addStat(V3Statistic{"*", name, मूल्य, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    अटल द्विगुन get_Stat_योग(स्थिर string& name);
    अटल uint64_t get_Stat_योग_Q(स्थिर string& name) {
        वापस static_cast<uint64_t>(get_Stat_योग(name));
    }
    /// Called each stage
    अटल व्योम statsStage(स्थिर string& name);
    /// Called by the top level, and error path, to collect statistics
    अटल व्योम statsStageAll(AstNetlist* nodep, स्थिर string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    अटल व्योम statsReport();
    /// Called by debug dumps
    अटल व्योम infoHeader(std::ofstream& os, स्थिर string& prefix);
    /// Called for final build report
    अटल व्योम summaryReport();
};

#पूर्ण_अगर  // Guard
