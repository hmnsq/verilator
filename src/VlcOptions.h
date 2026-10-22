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

#अगर_अघोषित VERILATOR_VLCOPTIONS_H_
#घोषणा VERILATOR_VLCOPTIONS_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "VlcPoint.h"
#समावेश "config_rev.h"

#समावेश <map>
#समावेश <set>
#समावेश <vector>

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

निजी:
    // METHODS
    व्योम parseReportOption() VL_MT_DISABLED;
    अटल व्योम showVersion(bool verbose) VL_MT_DISABLED;

खुला:
    // CONSTRUCTORS
    VlcOptions() = शेष;
    ~VlcOptions() = शेष;

    // METHODS
    व्योम parseOptsList(पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED;
    व्योम addReadFile(स्थिर string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    स्थिर VlStringSet& readFiles() स्थिर { वापस m_readFiles; }
    string annotateOut() स्थिर { वापस m_annotateOut; }
    bool annotateAll() स्थिर { वापस m_annotateAll; }
    पूर्णांक annotateMin() स्थिर { वापस m_annotateMin; }
    bool गिनती_Ok(uint64_t गिनती) स्थिर { वापस गिनती >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() स्थिर { वापस m_annotatePoints; }
    bool includeResetArcs() स्थिर { वापस m_includeResetArcs; }
    पूर्णांक reportLevels() स्थिर { वापस m_reportLevels; }
    bool reportSpecified() स्थिर { वापस !m_report.empty(); }
    bool reportSummary() स्थिर { वापस m_reportSummary; }
    bool reportHierarchy() स्थिर { वापस m_reportHierarchy; }
    bool rank() स्थिर { वापस m_rank; }
    bool unlink() स्थिर { वापस m_unlink; }
    string writeFile() स्थिर { वापस m_writeFile; }
    string writeInfoFile() स्थिर { वापस m_writeInfoFile; }
    bool isTypeMatch(स्थिर अक्षर* name) स्थिर {
        वापस VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    अटल string version() VL_MT_DISABLED;
};

//######################################################################

#पूर्ण_अगर  // guard
