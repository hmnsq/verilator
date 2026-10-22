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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "VlcPoint.h"
#અન્તર્ભાવ "config_rev.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    પૂર્ણાંક m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    પૂર્ણાંક m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

ખાનગી:
    // METHODS
    શૂન્ય parseReportOption() VL_MT_DISABLED;
    સ્થિર શૂન્ય showVersion(bool verbose) VL_MT_DISABLED;

જાહેર:
    // CONSTRUCTORS
    VlcOptions() = મૂળભૂત;
    ~VlcOptions() = મૂળભૂત;

    // METHODS
    શૂન્ય parseOptsList(પૂર્ણાંક argc, અક્ષર** argv) VL_MT_DISABLED;
    શૂન્ય addReadFile(અચલ string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    અચલ VlStringSet& readFiles() અચલ { પાછા m_readFiles; }
    string annotateOut() અચલ { પાછા m_annotateOut; }
    bool annotateAll() અચલ { પાછા m_annotateAll; }
    પૂર્ણાંક annotateMin() અચલ { પાછા m_annotateMin; }
    bool ગણતરી_Ok(uint64_t ગણતરી) અચલ { પાછા ગણતરી >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() અચલ { પાછા m_annotatePoints; }
    bool includeResetArcs() અચલ { પાછા m_includeResetArcs; }
    પૂર્ણાંક reportLevels() અચલ { પાછા m_reportLevels; }
    bool reportSpecified() અચલ { પાછા !m_report.empty(); }
    bool reportSummary() અચલ { પાછા m_reportSummary; }
    bool reportHierarchy() અચલ { પાછા m_reportHierarchy; }
    bool rank() અચલ { પાછા m_rank; }
    bool unlink() અચલ { પાછા m_unlink; }
    string writeFile() અચલ { પાછા m_writeFile; }
    string writeInfoFile() અચલ { પાછા m_writeInfoFile; }
    bool isTypeMatch(અચલ અક્ષર* name) અચલ {
        પાછા VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    સ્થિર string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
