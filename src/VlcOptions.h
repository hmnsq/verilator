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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "VlcPoint.h"
#অন্তর্ভাৱ "config_rev.h"

#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    পূর্ণাঙ্ক m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    পূর্ণাঙ্ক m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

গুপ্ত:
    // METHODS
    শূন্য parseReportOption() VL_MT_DISABLED;
    স্থির শূন্য showVersion(bool verbose) VL_MT_DISABLED;

সার্ৱজনিক:
    // CONSTRUCTORS
    VlcOptions() = স্ৱভাৱ;
    ~VlcOptions() = স্ৱভাৱ;

    // METHODS
    শূন্য parseOptsList(পূর্ণাঙ্ক argc, ৱর্ণ** argv) VL_MT_DISABLED;
    শূন্য addReadFile(ধ্রুৱ string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ধ্রুৱ VlStringSet& readFiles() ধ্রুৱ { ঘূৰাই m_readFiles; }
    string annotateOut() ধ্রুৱ { ঘূৰাই m_annotateOut; }
    bool annotateAll() ধ্রুৱ { ঘূৰাই m_annotateAll; }
    পূর্ণাঙ্ক annotateMin() ধ্রুৱ { ঘূৰাই m_annotateMin; }
    bool গুনতি_Ok(uint64_t গুনতি) ধ্রুৱ { ঘূৰাই গুনতি >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ধ্রুৱ { ঘূৰাই m_annotatePoints; }
    bool includeResetArcs() ধ্রুৱ { ঘূৰাই m_includeResetArcs; }
    পূর্ণাঙ্ক reportLevels() ধ্রুৱ { ঘূৰাই m_reportLevels; }
    bool reportSpecified() ধ্রুৱ { ঘূৰাই !m_report.empty(); }
    bool reportSummary() ধ্রুৱ { ঘূৰাই m_reportSummary; }
    bool reportHierarchy() ধ্রুৱ { ঘূৰাই m_reportHierarchy; }
    bool rank() ধ্রুৱ { ঘূৰাই m_rank; }
    bool unlink() ধ্রুৱ { ঘূৰাই m_unlink; }
    string writeFile() ধ্রুৱ { ঘূৰাই m_writeFile; }
    string writeInfoFile() ধ্রুৱ { ঘূৰাই m_writeInfoFile; }
    bool isTypeMatch(ধ্রুৱ ৱর্ণ* name) ধ্রুৱ {
        ঘূৰাই VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    স্থির string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
