// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Command line options
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#اگر_غیر_تعریف_شدہ VERILATOR_VLCOPTIONS_H_
#تعریف_کرو VERILATOR_VLCOPTIONS_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcPoint.h"
#شامل "config_rev.h"

#شامل <map>
#شامل <set>
#شامل <vector>

//######################################################################
// V3Options - Command line options

using Vl_String_سیٹ = std::سیٹ<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotate_برآمد;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    صحیح m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    Vl_String_سیٹ m_readFiles;    // main switch: --read
    صحیح m_report_سطحیں = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

نجی:
    // METHODS
    خالی parseReportOption() VL_MT_DISABLED;
    ساکن خالی show_نسخہ(bool verbose) VL_MT_DISABLED;

عوامی:
    // CONSTRUCTORS
    VlcOptions() = طےشدہ;
    ~VlcOptions() = طےشدہ;

    // METHODS
    خالی parseOptsList(صحیح argc, حرف** argv) VL_MT_DISABLED;
    خالی جوڑو_Read_File(ثابت string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ثابت Vl_String_سیٹ& readFiles() ثابت { واپس m_readFiles; }
    string annotate_برآمد() ثابت { واپس m_annotate_برآمد; }
    bool annotateAll() ثابت { واپس m_annotateAll; }
    صحیح annotateMin() ثابت { واپس m_annotateMin; }
    bool گنتی_Ok(uint64_t گنتی) ثابت { واپس گنتی >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ثابت { واپس m_annotatePoints; }
    bool includeResetArcs() ثابت { واپس m_includeResetArcs; }
    صحیح report_سطحیں() ثابت { واپس m_report_سطحیں; }
    bool reportSpecified() ثابت { واپس !m_report.خالی_خانے(); }
    bool reportSummary() ثابت { واپس m_reportSummary; }
    bool reportHierarchy() ثابت { واپس m_reportHierarchy; }
    bool rank() ثابت { واپس m_rank; }
    bool unlink() ثابت { واپس m_unlink; }
    string writeFile() ثابت { واپس m_writeFile; }
    string writeInfoFile() ثابت { واپس m_writeInfoFile; }
    bool is_Type_مطابقت(ثابت حرف* name) ثابت {
        واپس VString::wildmatch(Vlc_نقطہ::type_نکالو(name), m_filterType);
    }

    // METHODS (from main)
    ساکن string نسخہ() VL_MT_DISABLED;
};

//######################################################################

#اگر_ختم  // guard
