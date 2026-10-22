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

#నిర్వచితం_కాకపోతే VERILATOR_VLCOPTIONS_H_
#నిర్వచించు VERILATOR_VLCOPTIONS_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "VlcPoint.h"
#అన్తర్భావ "config_rev.h"

#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <vector>

//######################################################################
// V3Options - Command line options

using Vl_String_సమితి = std::సమితి<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotate_బహిర్గతం;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    పూర్ణాంకం m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    Vl_String_సమితి m_readFiles;    // main switch: --read
    పూర్ణాంకం m_report_స్థాయిలు = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

అంతరంగ:
    // METHODS
    శూన్య parseReportOption() VL_MT_DISABLED;
    స్థిర శూన్య show_సంస్కరణ(bool verbose) VL_MT_DISABLED;

బహిరంగ:
    // CONSTRUCTORS
    VlcOptions() = అప్రమేయ;
    ~VlcOptions() = అప్రమేయ;

    // METHODS
    శూన్య parseOptsList(పూర్ణాంకం argc, అక్షరం** argv) VL_MT_DISABLED;
    శూన్య కూడు_Read_File(స్థిరరాశి string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    స్థిరరాశి Vl_String_సమితి& readFiles() స్థిరరాశి { తిరిగి m_readFiles; }
    string annotate_బహిర్గతం() స్థిరరాశి { తిరిగి m_annotate_బహిర్గతం; }
    bool annotateAll() స్థిరరాశి { తిరిగి m_annotateAll; }
    పూర్ణాంకం annotateMin() స్థిరరాశి { తిరిగి m_annotateMin; }
    bool లెక్క_Ok(uint64_t లెక్క) స్థిరరాశి { తిరిగి లెక్క >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() స్థిరరాశి { తిరిగి m_annotatePoints; }
    bool includeResetArcs() స్థిరరాశి { తిరిగి m_includeResetArcs; }
    పూర్ణాంకం report_స్థాయిలు() స్థిరరాశి { తిరిగి m_report_స్థాయిలు; }
    bool reportSpecified() స్థిరరాశి { తిరిగి !m_report.ఖాళీలు(); }
    bool reportSummary() స్థిరరాశి { తిరిగి m_reportSummary; }
    bool reportHierarchy() స్థిరరాశి { తిరిగి m_reportHierarchy; }
    bool rank() స్థిరరాశి { తిరిగి m_rank; }
    bool unlink() స్థిరరాశి { తిరిగి m_unlink; }
    string writeFile() స్థిరరాశి { తిరిగి m_writeFile; }
    string writeInfoFile() స్థిరరాశి { తిరిగి m_writeInfoFile; }
    bool is_Type_సరిపోలిక(స్థిరరాశి అక్షరం* name) స్థిరరాశి {
        తిరిగి VString::wildmatch(Vlc_బిందువు::type_వెలికితీయి(name), m_filterType);
    }

    // METHODS (from main)
    స్థిర string సంస్కరణ() VL_MT_DISABLED;
};

//######################################################################

#అయితే_ముగింపు  // guard
