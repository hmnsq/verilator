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

#که_نه_تعریف_شوی VERILATOR_VLCOPTIONS_H_
#تعریف_کړه VERILATOR_VLCOPTIONS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcPoint.h"
#ضمن "config_rev.h"

#ضمن <map>
#ضمن <set>
#ضمن <vector>

//######################################################################
// V3Options - Command line options

using Vl_String_سېټ = std::سېټ<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotate_بهر;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    صحيح m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    Vl_String_سېټ m_readFiles;    // main switch: --read
    صحيح m_report_پوړونه = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

خاص:
    // METHODS
    فراغ parseReportOption() VL_MT_DISABLED;
    ساكن فراغ show_نسخه(bool verbose) VL_MT_DISABLED;

عام:
    // CONSTRUCTORS
    VlcOptions() = افتراضي;
    ~VlcOptions() = افتراضي;

    // METHODS
    فراغ parseOptsList(صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ جمع_کړه_Read_File(ثابت string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ثابت Vl_String_سېټ& readFiles() ثابت { أرجع m_readFiles; }
    string annotate_بهر() ثابت { أرجع m_annotate_بهر; }
    bool annotateAll() ثابت { أرجع m_annotateAll; }
    صحيح annotateMin() ثابت { أرجع m_annotateMin; }
    bool شمېر_Ok(uint64_t شمېر) ثابت { أرجع شمېر >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ثابت { أرجع m_annotatePoints; }
    bool includeResetArcs() ثابت { أرجع m_includeResetArcs; }
    صحيح report_پوړونه() ثابت { أرجع m_report_پوړونه; }
    bool reportSpecified() ثابت { أرجع !m_report.تشه(); }
    bool reportSummary() ثابت { أرجع m_reportSummary; }
    bool reportHierarchy() ثابت { أرجع m_reportHierarchy; }
    bool rank() ثابت { أرجع m_rank; }
    bool unlink() ثابت { أرجع m_unlink; }
    string writeFile() ثابت { أرجع m_writeFile; }
    string writeInfoFile() ثابت { أرجع m_writeInfoFile; }
    bool is_Type_سمون(ثابت حرف* name) ثابت {
        أرجع VString::wildmatch(Vlc_ټکی::type_ایستل(name), m_filterType);
    }

    // METHODS (from main)
    ساكن string نسخه() VL_MT_DISABLED;
};

//######################################################################

#د_که_پای  // guard
