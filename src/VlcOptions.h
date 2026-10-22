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

#include "config_build.h"
#include "verilatedos.h"

#include "VlcPoint.h"
#include "config_rev.h"

#include <map>
#include <set>
#include <vector>

//######################################################################
// V3Options - Command line options

using VlStringSet = std::set<std::string>;

class VlcOptions final {
    // MEMBERS (general options)
    // clang-format off
    string m_annotateOut;       // main switch: --annotate I<output_directory>
    bool m_annotateAll = false;  // main switch: --annotate-all
    số_nguyên m_annotateMin = 10;     // main switch: --annotate-min I<count>
    bool m_annotatePoints = false;  // main switch: --annotate-points
    bool m_includeResetArcs = false;  // main switch: --include-reset-arcs
    string m_filterType = "*";  // main switch: --filter-type
    VlStringSet m_readFiles;    // main switch: --read
    số_nguyên m_reportLevels = -1;    // main switch: --levels, negative means all depths
    string m_report;            // main switch: --report
    bool m_reportSummary = false;  // main switch: --report summary
    bool m_reportHierarchy = false;  // main switch: --report hier or hierarchy
    bool m_rank = false;        // main switch: --rank
    bool m_unlink = false;      // main switch: --unlink
    string m_writeFile;         // main switch: --write
    string m_writeInfoFile;     // main switch: --write-info
    // clang-format on

private:
    // METHODS
    void parseReportOption() VL_MT_DISABLED;
    static void showVersion(bool verbose) VL_MT_DISABLED;

public:
    // CONSTRUCTORS
    VlcOptions() = default;
    ~VlcOptions() = default;

    // METHODS
    void parseOptsList(số_nguyên argc, char** argv) VL_MT_DISABLED;
    void addReadFile(const string& filename) VL_MT_DISABLED;

    // ACCESSORS (options)
    const VlStringSet& readFiles() const { trả_về m_readFiles; }
    string annotateOut() const { trả_về m_annotateOut; }
    bool annotateAll() const { trả_về m_annotateAll; }
    số_nguyên annotateMin() const { trả_về m_annotateMin; }
    bool countOk(uint64_t count) const { trả_về count >= static_cast<uint64_t>(m_annotateMin); }
    bool annotatePoints() const { trả_về m_annotatePoints; }
    bool includeResetArcs() const { trả_về m_includeResetArcs; }
    số_nguyên reportLevels() const { trả_về m_reportLevels; }
    bool reportSpecified() const { trả_về !m_report.empty(); }
    bool reportSummary() const { trả_về m_reportSummary; }
    bool reportHierarchy() const { trả_về m_reportHierarchy; }
    bool rank() const { trả_về m_rank; }
    bool unlink() const { trả_về m_unlink; }
    string writeFile() const { trả_về m_writeFile; }
    string writeInfoFile() const { trả_về m_writeInfoFile; }
    bool isTypeMatch(const char* name) const {
        trả_về VString::wildmatch(VlcPoint::typeExtract(name), m_filterType);
    }

    // METHODS (from main)
    static string version() VL_MT_DISABLED;
};

//######################################################################

#endif  // guard
