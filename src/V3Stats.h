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
    VDouble0() = स्वभाव;
    ~VDouble0() = स्वभाव;

    // Implicit conversion operators:
    explicit VDouble0(अचर uint64_t v)
        : m_d{static_cast<द्विगुण>(v)} {}
    चालक द्विगुण() अचर { फर्काऊ m_d; }

    // Explicit operators:
    VDouble0& चालक++() {  // prefix
        ++m_d;
        फर्काऊ *this;
    }
    VDouble0 चालक++(पूर्णाङ्क) {  // postfix
        अचर VDouble0 old = *this;
        ++m_d;
        फर्काऊ old;
    }
    VDouble0& चालक=(अचर द्विगुण v) {
        m_d = v;
        फर्काऊ *this;
    }
    VDouble0& चालक+=(अचर द्विगुण v) {
        m_d += v;
        फर्काऊ *this;
    }
    VDouble0& चालक-=(अचर द्विगुण v) {
        m_d -= v;
        फर्काऊ *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    अचर string m_name;  ///< Name of this statistic
    द्विगुण m_मान;  ///< Value of statistic (count, ratio, etc.)
    अचिन्हित m_precision;  ///< Precision to print with (number of fractional digits)
    अचर string m_stage;  ///< Runtime stage
    अचर bool m_sumit;  ///< Do summation of similar stats
    अचर bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
सार्वजनिक:
    // METHODS
    string stage() अचर VL_MT_SAFE { फर्काऊ m_stage; }
    string name() अचर VL_MT_SAFE { फर्काऊ m_name; }
    द्विगुण मान() अचर VL_MT_SAFE { फर्काऊ m_मान; }
    अचिन्हित precision() अचर VL_MT_SAFE { फर्काऊ m_precision; }
    bool sumit() अचर VL_MT_SAFE { फर्काऊ m_sumit; }
    bool perf() अचर VL_MT_SAFE { फर्काऊ m_perf; }
    bool printit() अचर VL_MT_SAFE { फर्काऊ m_printit; }
    आभास शून्य dump(std::ofstream& os) अचर VL_MT_SAFE;
    शून्य combineWith(V3Statistic* otherp) {
        m_मान += otherp->मान();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(अचर string& stage, अचर string& name, द्विगुण मान, अचिन्हित precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_मान{मान}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    आभास ~V3Statistic() = स्वभाव;
};

//============================================================================

class V3Stats final {
    स्थिर V3Mutex s_mutex;  // Protects accesses

सार्वजनिक:
    // Symbolic names for some statistics that are later read by summaryReport()
    स्थिर constexpr अचर वर्ण* STAT_CPP_CHARS = "Output, C++ bytes written";
    स्थिर constexpr अचर वर्ण* STAT_CPP_FILES = "Output, C++ files written";
    स्थिर constexpr अचर वर्ण* STAT_CPUTIME = "CPU time, Total (sec)";
    स्थिर constexpr अचर वर्ण* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    स्थिर constexpr अचर वर्ण* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    स्थिर constexpr अचर वर्ण* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    स्थिर constexpr अचर वर्ण* STAT_WALLTIME = "Wall time, Total (sec)";
    स्थिर constexpr अचर वर्ण* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    स्थिर constexpr अचर वर्ण* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    स्थिर constexpr अचर वर्ण* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    स्थिर शून्य addStat(अचर V3Statistic&);
    स्थिर शून्य addStat(अचर string& stage, अचर string& name, द्विगुण मान,
                        अचिन्हित precision = 0) {
        addStat(V3Statistic{stage, name, मान, precision, false, false});
    }
    स्थिर शून्य addStat(अचर string& name, द्विगुण मान, अचिन्हित precision = 0) {
        addStat(V3Statistic{"*", name, मान, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    स्थिर शून्य addStatSum(अचर वर्ण* name, द्विगुण गन्ती) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        यदि (गन्ती == 0.0) फर्काऊ;
        अचर V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, गन्ती, 0, true, false});
    }
    स्थिर शून्य addStatSum(अचर std::string& name, द्विगुण गन्ती) {
        addStatSum(name.c_str(), गन्ती);
    }
    स्थिर शून्य addStatPerf(अचर string& name, द्विगुण मान) {
        addStat(V3Statistic{"*", name, मान, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    स्थिर द्विगुण getStatSum(अचर string& name);
    स्थिर uint64_t getStatSumQ(अचर string& name) {
        फर्काऊ static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    स्थिर शून्य statsStage(अचर string& name);
    /// Called by the top level, and error path, to collect statistics
    स्थिर शून्य statsStageAll(AstNetlist* nodep, अचर string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    स्थिर शून्य statsReport();
    /// Called by debug dumps
    स्थिर शून्य infoHeader(std::ofstream& os, अचर string& prefix);
    /// Called for final build report
    स्थिर शून्य summaryReport();
};

#endif  // Guard
