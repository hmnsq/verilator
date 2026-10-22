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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "VlcPoint.h"
#ଅନ୍ତର୍ଭାଵ "config_rev.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

ବ୍ୟକ୍ତିଗତ:
    // METHODS
    ଶୂନ୍ୟ parseReportOption() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ showVersion(bool verbose) VL_MT_DISABLED;

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VlcOptions() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VlcOptions() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    ଶୂନ୍ୟ parseOptsList(ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) VL_MT_DISABLED;
    ଶୂନ୍ୟ addReadFile(ଅଚଳ string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ଅଚଳ VlStringSet& readFiles() ଅଚଳ { ଫେରନ୍ତୁ m_readFiles; }
    string annotateOut() ଅଚଳ { ଫେରନ୍ତୁ m_annotateOut; }
    bool annotateAll() ଅଚଳ { ଫେରନ୍ତୁ m_annotateAll; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା annotateMin() ଅଚଳ { ଫେରନ୍ତୁ m_annotateMin; }
    bool ଗଣନା_Ok(uint64_t ଗଣନା_2) ଅଚଳ { ଫେରନ୍ତୁ ଗଣନା_2 >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ଅଚଳ { ଫେରନ୍ତୁ m_annotatePoints; }
    bool includeResetArcs() ଅଚଳ { ଫେରନ୍ତୁ m_includeResetArcs; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା reportLevels() ଅଚଳ { ଫେରନ୍ତୁ m_reportLevels; }
    bool reportSpecified() ଅଚଳ { ଫେରନ୍ତୁ !m_report.empty(); }
    bool reportSummary() ଅଚଳ { ଫେରନ୍ତୁ m_reportSummary; }
    bool reportHierarchy() ଅଚଳ { ଫେରନ୍ତୁ m_reportHierarchy; }
    bool rank() ଅଚଳ { ଫେରନ୍ତୁ m_rank; }
    bool unlink() ଅଚଳ { ଫେରନ୍ତୁ m_unlink; }
    string writeFile() ଅଚଳ { ଫେରନ୍ତୁ m_writeFile; }
    string writeInfoFile() ଅଚଳ { ଫେରନ୍ତୁ m_writeInfoFile; }
    bool isTypeMatch(ଅଚଳ ଅକ୍ଷର* name) ଅଚଳ {
        ଫେରନ୍ତୁ VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    ସ୍ଥିର string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
