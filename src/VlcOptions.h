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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VLCOPTIONS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VLCOPTIONS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcPoint.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "config_rev.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//######################################################################
// V3Options - Command line options

using Vl_String_ਸੈੱਟ = std::ਸੈੱਟ<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotate_ਬਾਹਰਲਾ;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    ਪੂਰਨ m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    Vl_String_ਸੈੱਟ m_readFiles;    // main switch: --read
    ਪੂਰਨ m_report_ਪੱਧਰ = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

ਨਿਜੀ:
    // METHODS
    ਖਾਲੀ parseReportOption() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ show_ਸੰਸਕਰਣ(bool verbose) VL_MT_DISABLED;

ਜਨਤਕ:
    // CONSTRUCTORS
    VlcOptions() = ਮੂਲ;
    ~VlcOptions() = ਮੂਲ;

    // METHODS
    ਖਾਲੀ parseOptsList(ਪੂਰਨ argc, ਅੱਖਰ** argv) VL_MT_DISABLED;
    ਖਾਲੀ ਜੋੜੋ_Read_File(ਅਚਲ string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    ਅਚਲ Vl_String_ਸੈੱਟ& readFiles() ਅਚਲ { ਵਾਪਸ m_readFiles; }
    string annotate_ਬਾਹਰਲਾ() ਅਚਲ { ਵਾਪਸ m_annotate_ਬਾਹਰਲਾ; }
    bool annotateAll() ਅਚਲ { ਵਾਪਸ m_annotateAll; }
    ਪੂਰਨ annotateMin() ਅਚਲ { ਵਾਪਸ m_annotateMin; }
    bool ਗਿਣਤੀ_Ok(uint64_t ਗਿਣਤੀ_2) ਅਚਲ { ਵਾਪਸ ਗਿਣਤੀ_2 >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() ਅਚਲ { ਵਾਪਸ m_annotatePoints; }
    bool includeResetArcs() ਅਚਲ { ਵਾਪਸ m_includeResetArcs; }
    ਪੂਰਨ report_ਪੱਧਰ() ਅਚਲ { ਵਾਪਸ m_report_ਪੱਧਰ; }
    bool reportSpecified() ਅਚਲ { ਵਾਪਸ !m_report.ਖਾਲੀ_ਥਾਵਾਂ(); }
    bool reportSummary() ਅਚਲ { ਵਾਪਸ m_reportSummary; }
    bool reportHierarchy() ਅਚਲ { ਵਾਪਸ m_reportHierarchy; }
    bool rank() ਅਚਲ { ਵਾਪਸ m_rank; }
    bool unlink() ਅਚਲ { ਵਾਪਸ m_unlink; }
    string writeFile() ਅਚਲ { ਵਾਪਸ m_writeFile; }
    string writeInfoFile() ਅਚਲ { ਵਾਪਸ m_writeInfoFile; }
    bool is_Type_ਮੇਲ(ਅਚਲ ਅੱਖਰ* name) ਅਚਲ {
        ਵਾਪਸ VString::wildmatch(Vlc_ਬਿੰਦੂ::type_ਕੱਢੋ(name), m_filterType);
    }

    // METHODS (from main)
    ਸਥਿਰ string ਸੰਸਕਰਣ() VL_MT_DISABLED;
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
