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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    द्विगुण m_d = 0.0;  ///< Count of occurrences/ value
सार्वजनिक:
    // METHODS
    VDouble0() = पूर्वनिर्धारित;
    ~VDouble0() = पूर्वनिर्धारित;

    // Implicit conversion operators:
    explicit VDouble0(स्थिरांक uint64_t v)
        : m_d{static_cast<द्विगुण>(v)} {}
    चालक द्विगुण() स्थिरांक { परत m_d; }

    // Explicit operators:
    VDouble0& चालक++() {  // prefix
        ++m_d;
        परत *this;
    }
    VDouble0 चालक++(पूर्णांक) {  // postfix
        स्थिरांक VDouble0 old = *this;
        ++m_d;
        परत old;
    }
    VDouble0& चालक=(स्थिरांक द्विगुण v) {
        m_d = v;
        परत *this;
    }
    VDouble0& चालक+=(स्थिरांक द्विगुण v) {
        m_d += v;
        परत *this;
    }
    VDouble0& चालक-=(स्थिरांक द्विगुण v) {
        m_d -= v;
        परत *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    स्थिरांक string m_name;  ///< Name of this statistic
    द्विगुण m_मूल्य;  ///< Value of statistic (count, ratio, etc.)
    अचिन्हित m_precision;  ///< Precision to print with (number of fractional digits)
    स्थिरांक string m_stage;  ///< Runtime stage
    स्थिरांक bool m_sumit;  ///< Do summation of similar stats
    स्थिरांक bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
सार्वजनिक:
    // METHODS
    string stage() स्थिरांक VL_MT_SAFE { परत m_stage; }
    string name() स्थिरांक VL_MT_SAFE { परत m_name; }
    द्विगुण मूल्य() स्थिरांक VL_MT_SAFE { परत m_मूल्य; }
    अचिन्हित precision() स्थिरांक VL_MT_SAFE { परत m_precision; }
    bool sumit() स्थिरांक VL_MT_SAFE { परत m_sumit; }
    bool perf() स्थिरांक VL_MT_SAFE { परत m_perf; }
    bool printit() स्थिरांक VL_MT_SAFE { परत m_printit; }
    आभासी शून्य dump(std::ofstream& os) स्थिरांक VL_MT_SAFE;
    शून्य combineWith(V3Statistic* otherp) {
        m_मूल्य += otherp->मूल्य();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(स्थिरांक string& stage, स्थिरांक string& name, द्विगुण मूल्य, अचिन्हित precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_मूल्य{मूल्य}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    आभासी ~V3Statistic() = पूर्वनिर्धारित;
};

//============================================================================

class V3Stats final {
    स्थिर V3Mutex s_mutex;  // Protects accesses

सार्वजनिक:
    // Symbolic names for some statistics that are later read by summaryReport()
    स्थिर constexpr स्थिरांक अक्षर* STAT_CPP_CHARS = "Output, C++ bytes written";
    स्थिर constexpr स्थिरांक अक्षर* STAT_CPP_FILES = "Output, C++ files written";
    स्थिर constexpr स्थिरांक अक्षर* STAT_CPUTIME = "CPU time, Total (sec)";
    स्थिर constexpr स्थिरांक अक्षर* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    स्थिर constexpr स्थिरांक अक्षर* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    स्थिर constexpr स्थिरांक अक्षर* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    स्थिर constexpr स्थिरांक अक्षर* STAT_WALLTIME = "Wall time, Total (sec)";
    स्थिर constexpr स्थिरांक अक्षर* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    स्थिर constexpr स्थिरांक अक्षर* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    स्थिर constexpr स्थिरांक अक्षर* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    स्थिर शून्य addStat(स्थिरांक V3Statistic&);
    स्थिर शून्य addStat(स्थिरांक string& stage, स्थिरांक string& name, द्विगुण मूल्य,
                        अचिन्हित precision = 0) {
        addStat(V3Statistic{stage, name, मूल्य, precision, false, false});
    }
    स्थिर शून्य addStat(स्थिरांक string& name, द्विगुण मूल्य, अचिन्हित precision = 0) {
        addStat(V3Statistic{"*", name, मूल्य, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    स्थिर शून्य addStatSum(स्थिरांक अक्षर* name, द्विगुण मोजणी) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        जर (मोजणी == 0.0) परत;
        स्थिरांक V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, मोजणी, 0, true, false});
    }
    स्थिर शून्य addStatSum(स्थिरांक std::string& name, द्विगुण मोजणी) {
        addStatSum(name.c_str(), मोजणी);
    }
    स्थिर शून्य addStatPerf(स्थिरांक string& name, द्विगुण मूल्य) {
        addStat(V3Statistic{"*", name, मूल्य, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    स्थिर द्विगुण getStatSum(स्थिरांक string& name);
    स्थिर uint64_t getStatSumQ(स्थिरांक string& name) {
        परत static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    स्थिर शून्य statsStage(स्थिरांक string& name);
    /// Called by the top level, and error path, to collect statistics
    स्थिर शून्य statsStageAll(AstNetlist* nodep, स्थिरांक string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    स्थिर शून्य statsReport();
    /// Called by debug dumps
    स्थिर शून्य infoHeader(std::ofstream& os, स्थिरांक string& prefix);
    /// Called for final build report
    स्थिर शून्य summaryReport();
};

#endif  // Guard
