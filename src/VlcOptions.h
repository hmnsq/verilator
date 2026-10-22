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

#ifndef VERILATOR_VLCOPTIONS_H_
#define VERILATOR_VLCOPTIONS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcPoint.h"
#ضمن "config_rev.h"

#ضمن <map>
#ضمن <set>
#ضمن <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    صحيح m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    صحيح m_reportLevels = -1;    // main switch: --levels, negative means all depths
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
    ساكن فراغ showVersion(bool verbose) VL_MT_DISABLED;

عام:
    // CONSTRUCTORS
    VlcOptions() = افتراضي;
    ~VlcOptions() = افتراضي;

    // METHODS
    فراغ parseOptsList(صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ addReadFile(ثابت string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ثابت VlStringSet& readFiles() ثابت { أرجع m_readFiles; }
    string annotateOut() ثابت { أرجع m_annotateOut; }
    bool annotateAll() ثابت { أرجع m_annotateAll; }
    صحيح annotateMin() ثابت { أرجع m_annotateMin; }
    bool گنتی_Ok(uint64_t گنتی) ثابت { أرجع گنتی >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ثابت { أرجع m_annotatePoints; }
    bool includeResetArcs() ثابت { أرجع m_includeResetArcs; }
    صحيح reportLevels() ثابت { أرجع m_reportLevels; }
    bool reportSpecified() ثابت { أرجع !m_report.empty(); }
    bool reportSummary() ثابت { أرجع m_reportSummary; }
    bool reportHierarchy() ثابت { أرجع m_reportHierarchy; }
    bool rank() ثابت { أرجع m_rank; }
    bool unlink() ثابت { أرجع m_unlink; }
    string writeFile() ثابت { أرجع m_writeFile; }
    string writeInfoFile() ثابت { أرجع m_writeInfoFile; }
    bool isTypeMatch(ثابت حرف* name) ثابت {
        أرجع VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    ساكن string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
