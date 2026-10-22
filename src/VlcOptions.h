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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "VlcPoint.h"
#ಅನ್ತರ್ಭಾವ "config_rev.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    ಪೂರ್ಣಾಙ್ಕ m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    ಪೂರ್ಣಾಙ್ಕ m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

ಗುಪ್ತ:
    // METHODS
    ಶೂನ್ಯ parseReportOption() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ showVersion(bool verbose) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VlcOptions() = ಸ್ವಭಾವ;
    ~VlcOptions() = ಸ್ವಭಾವ;

    // METHODS
    ಶೂನ್ಯ parseOptsList(ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) VL_MT_DISABLED;
    ಶೂನ್ಯ addReadFile(ಧ್ರುವ string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ಧ್ರುವ VlStringSet& readFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_readFiles; }
    string annotateOut() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_annotateOut; }
    bool annotateAll() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_annotateAll; }
    ಪೂರ್ಣಾಙ್ಕ annotateMin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_annotateMin; }
    bool ಎಣಿಕೆ_Ok(uint64_t ಎಣಿಕೆ) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_annotatePoints; }
    bool includeResetArcs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_includeResetArcs; }
    ಪೂರ್ಣಾಙ್ಕ reportLevels() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reportLevels; }
    bool reportSpecified() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_report.empty(); }
    bool reportSummary() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reportSummary; }
    bool reportHierarchy() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reportHierarchy; }
    bool rank() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rank; }
    bool unlink() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unlink; }
    string writeFile() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_writeFile; }
    string writeInfoFile() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_writeInfoFile; }
    bool isTypeMatch(ಧ್ರುವ ವರ್ಣ* name) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    ಸ್ಥಿರ string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
