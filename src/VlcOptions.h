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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "VlcPoint.h"
#அந்தர்பாவ "config_rev.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    முழு m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    முழு m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

தனிப்பட்ட:
    // METHODS
    வெறுமை parseReportOption() VL_MT_DISABLED;
    நிலையான வெறுமை showVersion(bool verbose) VL_MT_DISABLED;

பொது:
    // CONSTRUCTORS
    VlcOptions() = இயல்பு;
    ~VlcOptions() = இயல்பு;

    // METHODS
    வெறுமை parseOptsList(முழு argc, எழுத்து** argv) VL_MT_DISABLED;
    வெறுமை addReadFile(மாறிலி string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    மாறிலி VlStringSet& readFiles() மாறிலி { திரும்பு m_readFiles; }
    string annotateOut() மாறிலி { திரும்பு m_annotateOut; }
    bool annotateAll() மாறிலி { திரும்பு m_annotateAll; }
    முழு annotateMin() மாறிலி { திரும்பு m_annotateMin; }
    bool மொத்தம்_Ok(uint64_t மொத்தம்) மாறிலி { திரும்பு மொத்தம் >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() மாறிலி { திரும்பு m_annotatePoints; }
    bool includeResetArcs() மாறிலி { திரும்பு m_includeResetArcs; }
    முழு reportLevels() மாறிலி { திரும்பு m_reportLevels; }
    bool reportSpecified() மாறிலி { திரும்பு !m_report.empty(); }
    bool reportSummary() மாறிலி { திரும்பு m_reportSummary; }
    bool reportHierarchy() மாறிலி { திரும்பு m_reportHierarchy; }
    bool rank() மாறிலி { திரும்பு m_rank; }
    bool unlink() மாறிலி { திரும்பு m_unlink; }
    string writeFile() மாறிலி { திரும்பு m_writeFile; }
    string writeInfoFile() மாறிலி { திரும்பு m_writeInfoFile; }
    bool isTypeMatch(மாறிலி எழுத்து* name) மாறிலி {
        திரும்பு VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    நிலையான string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
