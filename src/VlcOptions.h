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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcPoint.h"
#شامل "config_rev.h"

#شامل <map>
#شامل <set>
#شامل <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    صحیح m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    صحیح m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

خصوصی:
    // METHODS
    تهی parseReportOption() VL_MT_DISABLED;
    ایستا تهی showVersion(bool verbose) VL_MT_DISABLED;

عمومی:
    // CONSTRUCTORS
    VlcOptions() = پیشفرض;
    ~VlcOptions() = پیشفرض;

    // METHODS
    تهی parseOptsList(صحیح argc, نویسه** argv) VL_MT_DISABLED;
    تهی addReadFile(ثابت string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ثابت VlStringSet& readFiles() ثابت { بازگشت m_readFiles; }
    string annotateOut() ثابت { بازگشت m_annotateOut; }
    bool annotateAll() ثابت { بازگشت m_annotateAll; }
    صحیح annotateMin() ثابت { بازگشت m_annotateMin; }
    bool شمار_Ok(uint64_t شمار) ثابت { بازگشت شمار >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ثابت { بازگشت m_annotatePoints; }
    bool includeResetArcs() ثابت { بازگشت m_includeResetArcs; }
    صحیح reportLevels() ثابت { بازگشت m_reportLevels; }
    bool reportSpecified() ثابت { بازگشت !m_report.empty(); }
    bool reportSummary() ثابت { بازگشت m_reportSummary; }
    bool reportHierarchy() ثابت { بازگشت m_reportHierarchy; }
    bool rank() ثابت { بازگشت m_rank; }
    bool unlink() ثابت { بازگشت m_unlink; }
    string writeFile() ثابت { بازگشت m_writeFile; }
    string writeInfoFile() ثابت { بازگشت m_writeInfoFile; }
    bool isTypeMatch(ثابت نویسه* name) ثابت {
        بازگشت VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    ایستا string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
