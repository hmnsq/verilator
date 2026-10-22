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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "VlcPoint.h"
#അന്തര്ഭാവ "config_rev.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    പൂർണ്ണസംഖ്യ m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    പൂർണ്ണസംഖ്യ m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

സ്വകാര്യ:
    // METHODS
    ശൂന്യം parseReportOption() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം showVersion(bool verbose) VL_MT_DISABLED;

പൊതു:
    // CONSTRUCTORS
    VlcOptions() = സ്ഥിരസ്ഥിതി;
    ~VlcOptions() = സ്ഥിരസ്ഥിതി;

    // METHODS
    ശൂന്യം parseOptsList(പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) VL_MT_DISABLED;
    ശൂന്യം addReadFile(സ്ഥിരം string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    സ്ഥിരം VlStringSet& readFiles() സ്ഥിരം { തിരികെ m_readFiles; }
    string annotateOut() സ്ഥിരം { തിരികെ m_annotateOut; }
    bool annotateAll() സ്ഥിരം { തിരികെ m_annotateAll; }
    പൂർണ്ണസംഖ്യ annotateMin() സ്ഥിരം { തിരികെ m_annotateMin; }
    bool എണ്ണം_Ok(uint64_t എണ്ണം) സ്ഥിരം { തിരികെ എണ്ണം >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() സ്ഥിരം { തിരികെ m_annotatePoints; }
    bool includeResetArcs() സ്ഥിരം { തിരികെ m_includeResetArcs; }
    പൂർണ്ണസംഖ്യ reportLevels() സ്ഥിരം { തിരികെ m_reportLevels; }
    bool reportSpecified() സ്ഥിരം { തിരികെ !m_report.empty(); }
    bool reportSummary() സ്ഥിരം { തിരികെ m_reportSummary; }
    bool reportHierarchy() സ്ഥിരം { തിരികെ m_reportHierarchy; }
    bool rank() സ്ഥിരം { തിരികെ m_rank; }
    bool unlink() സ്ഥിരം { തിരികെ m_unlink; }
    string writeFile() സ്ഥിരം { തിരികെ m_writeFile; }
    string writeInfoFile() സ്ഥിരം { തിരികെ m_writeInfoFile; }
    bool isTypeMatch(സ്ഥിരം അക്ഷരം* name) സ്ഥിരം {
        തിരികെ VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    സ്ഥിതം string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
