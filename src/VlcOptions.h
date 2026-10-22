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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VLCOPTIONS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VLCOPTIONS_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "VlcPoint.h"
#ಅನ್ತರ್ಭಾವ "config_rev.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

//######################################################################
// V3Options - Command line options

using Vl_String_ಗಣ = std::ಗಣ<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotate_ಹೊರಹೋಗು;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    ಪೂರ್ಣಾಂಕ m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    Vl_String_ಗಣ m_readFiles;    // main switch: --read
    ಪೂರ್ಣಾಂಕ m_report_ಮಟ್ಟಗಳು = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

ಖಾಸಗಿ:
    // METHODS
    ಶೂನ್ಯ parseReportOption() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ show_ಆವೃತ್ತಿ(bool verbose) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VlcOptions() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VlcOptions() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    ಶೂನ್ಯ parseOptsList(ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) VL_MT_DISABLED;
    ಶೂನ್ಯ ಕೂಡಿಸು_Read_File(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ಸ್ಥಿರಾಂಕ Vl_String_ಗಣ& readFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_readFiles; }
    string annotate_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_annotate_ಹೊರಹೋಗು; }
    bool annotateAll() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_annotateAll; }
    ಪೂರ್ಣಾಂಕ annotateMin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_annotateMin; }
    bool ಎಣಿಕೆ_Ok(uint64_t ಎಣಿಕೆ_2) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2 >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_annotatePoints; }
    bool includeResetArcs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_includeResetArcs; }
    ಪೂರ್ಣಾಂಕ report_ಮಟ್ಟಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_report_ಮಟ್ಟಗಳು; }
    bool reportSpecified() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_report.ಖಾಲಿಗಳು(); }
    bool reportSummary() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_reportSummary; }
    bool reportHierarchy() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_reportHierarchy; }
    bool rank() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rank; }
    bool unlink() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unlink; }
    string writeFile() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_writeFile; }
    string writeInfoFile() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_writeInfoFile; }
    bool is_Type_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ VString::wildmatch(Vlc_ಬಿಂದು::type_ಹೊರತೆಗೆ(name), m_filterType);
    }

    // METHODS (from main)
    ಸ್ಥಿರ string ಆವೃತ್ತಿ() VL_MT_DISABLED;
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
