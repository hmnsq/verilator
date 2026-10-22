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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "VlcPoint.h"
#अन्तर्भाव "config_rev.h"

#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    पूर्णांक m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    पूर्णांक m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

खाजगी:
    // METHODS
    शून्य parseReportOption() VL_MT_DISABLED;
    स्थिर शून्य showVersion(bool verbose) VL_MT_DISABLED;

सार्वजनिक:
    // CONSTRUCTORS
    VlcOptions() = पूर्वनिर्धारित;
    ~VlcOptions() = पूर्वनिर्धारित;

    // METHODS
    शून्य parseOptsList(पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED;
    शून्य addReadFile(स्थिरांक string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    स्थिरांक VlStringSet& readFiles() स्थिरांक { परत m_readFiles; }
    string annotateOut() स्थिरांक { परत m_annotateOut; }
    bool annotateAll() स्थिरांक { परत m_annotateAll; }
    पूर्णांक annotateMin() स्थिरांक { परत m_annotateMin; }
    bool मोजणी_Ok(uint64_t मोजणी) स्थिरांक { परत मोजणी >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() स्थिरांक { परत m_annotatePoints; }
    bool includeResetArcs() स्थिरांक { परत m_includeResetArcs; }
    पूर्णांक reportLevels() स्थिरांक { परत m_reportLevels; }
    bool reportSpecified() स्थिरांक { परत !m_report.empty(); }
    bool reportSummary() स्थिरांक { परत m_reportSummary; }
    bool reportHierarchy() स्थिरांक { परत m_reportHierarchy; }
    bool rank() स्थिरांक { परत m_rank; }
    bool unlink() स्थिरांक { परत m_unlink; }
    string writeFile() स्थिरांक { परत m_writeFile; }
    string writeInfoFile() स्थिरांक { परत m_writeInfoFile; }
    bool isTypeMatch(स्थिरांक अक्षर* name) स्थिरांक {
        परत VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    स्थिर string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
