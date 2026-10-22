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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"

class AstNetlist;

//============================================================================

class VDouble0 final {
    // Double counter, initializes to zero for easy use
    இரட்டை m_d = 0.0;  ///< Count of occurrences/ value
பொது:
    // METHODS
    VDouble0() = இயல்பு;
    ~VDouble0() = இயல்பு;

    // Implicit conversion operators:
    explicit VDouble0(மாறிலி uint64_t v)
        : m_d{static_cast<இரட்டை>(v)} {}
    operator இரட்டை() மாறிலி { திரும்பு m_d; }

    // Explicit operators:
    VDouble0& operator++() {  // prefix
        ++m_d;
        திரும்பு *this;
    }
    VDouble0 operator++(முழு) {  // postfix
        மாறிலி VDouble0 old = *this;
        ++m_d;
        திரும்பு old;
    }
    VDouble0& operator=(மாறிலி இரட்டை v) {
        m_d = v;
        திரும்பு *this;
    }
    VDouble0& operator+=(மாறிலி இரட்டை v) {
        m_d += v;
        திரும்பு *this;
    }
    VDouble0& operator-=(மாறிலி இரட்டை v) {
        m_d -= v;
        திரும்பு *this;
    }
};

//============================================================================

class V3Statistic final {
    // A statistical entry we want published into the database
    மாறிலி string m_name;  ///< Name of this statistic
    இரட்டை m_மதிப்பு;  ///< Value of statistic (count, ratio, etc.)
    unsigned m_precision;  ///< Precision to print with (number of fractional digits)
    மாறிலி string m_stage;  ///< Runtime stage
    மாறிலி bool m_sumit;  ///< Do summation of similar stats
    மாறிலி bool m_perf;  ///< Performance section
    bool m_printit = true;  ///< Print the results
பொது:
    // METHODS
    string stage() மாறிலி VL_MT_SAFE { திரும்பு m_stage; }
    string name() மாறிலி VL_MT_SAFE { திரும்பு m_name; }
    இரட்டை மதிப்பு() மாறிலி VL_MT_SAFE { திரும்பு m_மதிப்பு; }
    unsigned precision() மாறிலி VL_MT_SAFE { திரும்பு m_precision; }
    bool sumit() மாறிலி VL_MT_SAFE { திரும்பு m_sumit; }
    bool perf() மாறிலி VL_MT_SAFE { திரும்பு m_perf; }
    bool printit() மாறிலி VL_MT_SAFE { திரும்பு m_printit; }
    மெய்நிகர் வெறுமை dump(std::ofstream& os) மாறிலி VL_MT_SAFE;
    வெறுமை combineWith(V3Statistic* otherp) {
        m_மதிப்பு += otherp->மதிப்பு();
        otherp->m_printit = false;
    }
    // CONSTRUCTORS
    V3Statistic(மாறிலி string& stage, மாறிலி string& name, இரட்டை மதிப்பு, unsigned precision,
                bool sumit, bool perf)
        : m_name{name}
        , m_மதிப்பு{மதிப்பு}
        , m_precision{precision}
        , m_stage{stage}
        , m_sumit{sumit}
        , m_perf{perf} {}
    மெய்நிகர் ~V3Statistic() = இயல்பு;
};

//============================================================================

class V3Stats final {
    நிலையான V3Mutex s_mutex;  // Protects accesses

பொது:
    // Symbolic names for some statistics that are later read by summaryReport()
    நிலையான constexpr மாறிலி எழுத்து* STAT_CPP_CHARS = "Output, C++ bytes written";
    நிலையான constexpr மாறிலி எழுத்து* STAT_CPP_FILES = "Output, C++ files written";
    நிலையான constexpr மாறிலி எழுத்து* STAT_CPUTIME = "CPU time, Total (sec)";
    நிலையான constexpr மாறிலி எழுத்து* STAT_MODEL_SIZE = "Size prediction, Model total (bytes)";
    நிலையான constexpr மாறிலி எழுத்து* STAT_SOURCE_CHARS = "Input, Verilog bytes read";
    நிலையான constexpr மாறிலி எழுத்து* STAT_SOURCE_MODULES = "Input, Verilog modules read";
    நிலையான constexpr மாறிலி எழுத்து* STAT_WALLTIME = "Wall time, Total (sec)";
    நிலையான constexpr மாறிலி எழுத்து* STAT_WALLTIME_BUILD = "Wall time, Build (sec)";
    நிலையான constexpr மாறிலி எழுத்து* STAT_WALLTIME_CVT = "Wall time, Conversion (sec)";
    நிலையான constexpr மாறிலி எழுத்து* STAT_WALLTIME_ELAB = "Wall time, Elaboration (sec)";

    நிலையான வெறுமை addStat(மாறிலி V3Statistic&);
    நிலையான வெறுமை addStat(மாறிலி string& stage, மாறிலி string& name, இரட்டை மதிப்பு,
                        unsigned precision = 0) {
        addStat(V3Statistic{stage, name, மதிப்பு, precision, false, false});
    }
    நிலையான வெறுமை addStat(மாறிலி string& name, இரட்டை மதிப்பு, unsigned precision = 0) {
        addStat(V3Statistic{"*", name, மதிப்பு, precision, false, false});
    }
    // Add summary statistic - Threadsafe _unlike most other functions here_
    நிலையான வெறுமை addStatSum(மாறிலி எழுத்து* name, இரட்டை மொத்தம்) VL_MT_SAFE_EXCLUDES(s_mutex) {
        // Avoid memory blow-up when called frequently with zero adds,
        // e.g. from V3Const invoked on individual expressions.
        எனில் (மொத்தம் == 0.0) திரும்பு;
        மாறிலி V3LockGuard lock{s_mutex};
        addStat(V3Statistic{"*", name, மொத்தம், 0, true, false});
    }
    நிலையான வெறுமை addStatSum(மாறிலி std::string& name, இரட்டை மொத்தம்) {
        addStatSum(name.c_str(), மொத்தம்);
    }
    நிலையான வெறுமை addStatPerf(மாறிலி string& name, இரட்டை மதிப்பு) {
        addStat(V3Statistic{"*", name, மதிப்பு, 6, true, true});
    }
    /// Return value of statistic, or zero if not found
    நிலையான இரட்டை getStatSum(மாறிலி string& name);
    நிலையான uint64_t getStatSumQ(மாறிலி string& name) {
        திரும்பு static_cast<uint64_t>(getStatSum(name));
    }
    /// Called each stage
    நிலையான வெறுமை statsStage(மாறிலி string& name);
    /// Called by the top level, and error path, to collect statistics
    நிலையான வெறுமை statsStageAll(AstNetlist* nodep, மாறிலி string& stage, bool fastOnly = false);
    /// Called by the top level to dump the statistics
    நிலையான வெறுமை statsReport();
    /// Called by debug dumps
    நிலையான வெறுமை infoHeader(std::ofstream& os, மாறிலி string& prefix);
    /// Called for final build report
    நிலையான வெறுமை summaryReport();
};

#endif  // Guard
