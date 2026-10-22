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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3STATS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3STATS_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    ದ್ವಿಗುಣ m_d = 0.0;  ///< Count of occurrences/ value
ಸಾರ್ವಜನಿಕ:
    // METHODS
    VDouble0() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VDouble0() = ಪೂರ್ವನಿಯೋಜಿತ;

    // Implicit conversion operators:
    explicit VDouble0(ಸ್ಥಿರಾಂಕ uint64_t v)
        : m_d{static_cast<ದ್ವಿಗುಣ>(v)} {}
    ಕ್ರಿಯಾಕಾರಕ ದ್ವಿಗುಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_d; }

    // Explicit operators:
    VDouble0& ಕ್ರಿಯಾಕಾರಕ++() {  //ಪೂರ್ವಪ್ರತ್ಯಯ
        ++m_d;
        ಹಿಂತಿರುಗಿ *this;
    }
    VDouble0 ಕ್ರಿಯಾಕಾರಕ++(ಪೂರ್ಣಾಂಕ) {  // postfix
        ಸ್ಥಿರಾಂಕ VDouble0 old = *this;
        ++m_d;
        ಹಿಂತಿರುಗಿ old;
    }
    VDouble0& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ v) {
        m_d = v;
        ಹಿಂತಿರುಗಿ *this;
    }
    VDouble0& ಕ್ರಿಯಾಕಾರಕ+=(ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ v) {
        m_d += v;
        ಹಿಂತಿರುಗಿ *this;
    }
    VDouble0& ಕ್ರಿಯಾಕಾರಕ-=(ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ v) {
        m_d -= v;
        ಹಿಂತಿರುಗಿ *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    ಸ್ಥಿರಾಂಕ string m_name;  ///< Name of this statistic
    ದ್ವಿಗುಣ m_ಮೌಲ್ಯ;  ///< Value of statistic (count, ratio, etc.)
    ಚಿಹ್ನೆರಹಿತ m_precision;  ///< Precision to print with (number of fractional digits)
    ಸ್ಥಿರಾಂಕ string m_stage;  ///< Runtime stage
    ಸ್ಥಿರಾಂಕ bool m_sumit;  ///< Do summation of similar stats
    ಸ್ಥಿರಾಂಕ bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
ಸಾರ್ವಜನಿಕ:
    // METHODS
    string stage() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_stage; }
    string name() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_name; }
    ದ್ವಿಗುಣ ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ; }
    ಚಿಹ್ನೆರಹಿತ precision() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_precision; }
    bool sumit() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_sumit; }
    bool perf() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_perf; }
    bool printit() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_printit; }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ dump(std::ofstream& os) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಶೂನ್ಯ combineWith(V3Statistic* otherp) {
        m_ಮೌಲ್ಯ += otherp->ಮೌಲ್ಯ();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(ಸ್ಥಿರಾಂಕ string& stage, ಸ್ಥಿರಾಂಕ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ, ಚಿಹ್ನೆರಹಿತ precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_ಮೌಲ್ಯ{ಮೌಲ್ಯ}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    ವಾಸ್ತವಿಕ ~V3Statistic() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//============================================================================

class V3Stats final {
    ಸ್ಥಿರ V3Mutex s_mutex;  // Protects accesses

ಸಾರ್ವಜನಿಕ:
    // Symbolic names for some statistics that are later read by summaryReport()
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_CPP_CHARS = "Output, C++ bytes written";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_CPP_FILES = "Output, C++ files written";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_CPUTIME = "CPU time, Total (sec)";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_MODEL_ಗಾತ್ರ = "Size prediction, Model total (bytes)";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_ಮೂಲ_CHARS = "Input, Verilog bytes read";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_ಮೂಲ_MODULES = "Input, Verilog modules read";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_WALLTIME = "Wall time, Total (sec)";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat(ಸ್ಥಿರಾಂಕ V3Statistic&);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat(ಸ್ಥಿರಾಂಕ string& stage, ಸ್ಥಿರಾಂಕ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ,
                        ಚಿಹ್ನೆರಹಿತ precision = 0) {
        ಕೂಡಿಸು_Stat(V3Statistic{stage, name, ಮೌಲ್ಯ, precision, false, false});
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat(ಸ್ಥಿರಾಂಕ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ, ಚಿಹ್ನೆರಹಿತ precision = 0) {
        ಕೂಡಿಸು_Stat(V3Statistic{"*", name, ಮೌಲ್ಯ, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat_ಮೊತ್ತ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name, ದ್ವಿಗುಣ ಎಣಿಕೆ_2) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        ಆದರೆ (ಎಣಿಕೆ_2 == 0.0) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_mutex};
        ಕೂಡಿಸು_Stat(V3Statistic{"*", name, ಎಣಿಕೆ_2, 0, true, false});
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat_ಮೊತ್ತ(ಸ್ಥಿರಾಂಕ std::string& name, ದ್ವಿಗುಣ ಎಣಿಕೆ_2) {
        ಕೂಡಿಸು_Stat_ಮೊತ್ತ(name.c_str(), ಎಣಿಕೆ_2);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat_Perf(ಸ್ಥಿರಾಂಕ string& name, ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        ಕೂಡಿಸು_Stat(V3Statistic{"*", name, ಮೌಲ್ಯ, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    ಸ್ಥಿರ ದ್ವಿಗುಣ get_Stat_ಮೊತ್ತ(ಸ್ಥಿರಾಂಕ string& name);
    ಸ್ಥಿರ uint64_t get_Stat_ಮೊತ್ತ_Q(ಸ್ಥಿರಾಂಕ string& name) {
        ಹಿಂತಿರುಗಿ static_cast<uint64_t>(get_Stat_ಮೊತ್ತ(name));
    }
    /// Called each stage
    ಸ್ಥಿರ ಶೂನ್ಯ statsStage(ಸ್ಥಿರಾಂಕ string& name);
    /// Called by the top level, and error path, to collect statistics
    ಸ್ಥಿರ ಶೂನ್ಯ statsStageAll(AstNetlist* nodep, ಸ್ಥಿರಾಂಕ string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    ಸ್ಥಿರ ಶೂನ್ಯ statsReport();
    /// Called by debug dumps
    ಸ್ಥಿರ ಶೂನ್ಯ info_ಶೀರ್ಷಿಕೆ(std::ofstream& os, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ);
    /// Called for final build report
    ಸ್ಥಿರ ಶೂನ್ಯ summaryReport();
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
