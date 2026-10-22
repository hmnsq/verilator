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

#यदि_अपरिभाषितम् VERILATOR_V3STATS_H_
#परिभाषय VERILATOR_V3STATS_H_

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
    explicit VDouble0(ध्रुव uint64_t v)
        : m_d{static_cast<द्विगुण>(v)} {}
    संक्रियकः द्विगुण() ध्रुव { प्रत्यागम m_d; }

    // Explicit operators:
    VDouble0& संक्रियकः++() {  //उपसर्गः
        ++m_d;
        प्रत्यागम *this;
    }
    VDouble0 संक्रियकः++(पूर्णाङ्क) {  // postfix
        ध्रुव VDouble0 old = *this;
        ++m_d;
        प्रत्यागम old;
    }
    VDouble0& संक्रियकः=(ध्रुव द्विगुण v) {
        m_d = v;
        प्रत्यागम *this;
    }
    VDouble0& संक्रियकः+=(ध्रुव द्विगुण v) {
        m_d += v;
        प्रत्यागम *this;
    }
    VDouble0& संक्रियकः-=(ध्रुव द्विगुण v) {
        m_d -= v;
        प्रत्यागम *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ध्रुव string m_name;  ///< Name of this statistic
    द्विगुण m_मानम्;  ///< Value of statistic (count, ratio, etc.)
    अचिह्नित m_precision;  ///< Precision to print with (number of fractional digits)
    ध्रुव string m_stage;  ///< Runtime stage
    ध्रुव bool m_sumit;  ///< Do summation of similar stats
    ध्रुव bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
सार्वजनिक:
    // METHODS
    string stage() ध्रुव VL_MT_SAFE { प्रत्यागम m_stage; }
    string name() ध्रुव VL_MT_SAFE { प्रत्यागम m_name; }
    द्विगुण मानम्() ध्रुव VL_MT_SAFE { प्रत्यागम m_मानम्; }
    अचिह्नित precision() ध्रुव VL_MT_SAFE { प्रत्यागम m_precision; }
    bool sumit() ध्रुव VL_MT_SAFE { प्रत्यागम m_sumit; }
    bool perf() ध्रुव VL_MT_SAFE { प्रत्यागम m_perf; }
    bool printit() ध्रुव VL_MT_SAFE { प्रत्यागम m_printit; }
    आभास शून्य dump(std::ofstream& os) ध्रुव VL_MT_SAFE;
    शून्य combineWith(V3Statistic* otherp) {
        m_मानम् += otherp->मानम्();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ध्रुव string& stage, ध्रुव string& name, द्विगुण मानम्, अचिह्नित precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_मानम्{मानम्}
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
    स्थिर constexpr ध्रुव वर्ण* STAT_CPP_CHARS = "Output, C++ bytes written";
    स्थिर constexpr ध्रुव वर्ण* STAT_CPP_FILES = "Output, C++ files written";
    स्थिर constexpr ध्रुव वर्ण* STAT_CPUTIME = "CPU time, Total (sec)";
    स्थिर constexpr ध्रुव वर्ण* STAT_MODEL_आकार = "Size prediction, Model total (bytes)";
    स्थिर constexpr ध्रुव वर्ण* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    स्थिर constexpr ध्रुव वर्ण* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    स्थिर constexpr ध्रुव वर्ण* STAT_WALLTIME = "Wall time, Total (sec)";
    स्थिर constexpr ध्रुव वर्ण* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    स्थिर constexpr ध्रुव वर्ण* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    स्थिर constexpr ध्रुव वर्ण* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    स्थिर शून्य सङ्कलय_Stat(ध्रुव V3Statistic&);
    स्थिर शून्य सङ्कलय_Stat(ध्रुव string& stage, ध्रुव string& name, द्विगुण मानम्,
                        अचिह्नित precision = 0) {
        सङ्कलय_Stat(V3Statistic{stage, name, मानम्, precision, false, false});
    }
    स्थिर शून्य सङ्कलय_Stat(ध्रुव string& name, द्विगुण मानम्, अचिह्नित precision = 0) {
        सङ्कलय_Stat(V3Statistic{"*", name, मानम्, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    स्थिर शून्य सङ्कलय_Stat_योग(ध्रुव वर्ण* name, द्विगुण गणनम्) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        यदि (गणनम् == 0.0) प्रत्यागम;
        ध्रुव V3_तालकम्_Guard तालकम्_2{s_mutex};
        सङ्कलय_Stat(V3Statistic{"*", name, गणनम्, 0, true, false});
    }
    स्थिर शून्य सङ्कलय_Stat_योग(ध्रुव std::string& name, द्विगुण गणनम्) {
        सङ्कलय_Stat_योग(name.c_str(), गणनम्);
    }
    स्थिर शून्य सङ्कलय_Stat_Perf(ध्रुव string& name, द्विगुण मानम्) {
        सङ्कलय_Stat(V3Statistic{"*", name, मानम्, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    स्थिर द्विगुण get_Stat_योग(ध्रुव string& name);
    स्थिर uint64_t get_Stat_योग_Q(ध्रुव string& name) {
        प्रत्यागम static_cast<uint64_t>(get_Stat_योग(name));
    }
    /// Called each stage
    स्थिर शून्य statsStage(ध्रुव string& name);
    /// Called by the top level, and error path, to collect statistics
    स्थिर शून्य statsStageAll(AstNetlist* nodep, ध्रुव string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    स्थिर शून्य statsReport();
    /// Called by debug dumps
    स्थिर शून्य info_शीर्षकम्(std::ofstream& os, ध्रुव string& उपसर्गः);
    /// Called for final build report
    स्थिर शून्य summaryReport();
};

#यदि_अन्तः  // Guard
