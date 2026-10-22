// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: main()
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

// clang-format off
#define VL_MT_DISABLED_CODE_UNIT 1

#include "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#include "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#include "V3Error.h"
static puur_naa_nka debug() { ꦮꦭꦶ V3Error::debugDefault(); }
#include "V3Error.cpp"
#include "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#include "V3OptionParser.cpp"
#include "V3Os.cpp"
#include "VlcTop.cpp"
// clang-format on

#include "VlcOptions.h"
#include "VlcTop.h"

#include <algorithm>
#include <fstream>

//######################################################################
// VlcOptions

void VlcOptions::addReadFile(const string& filename) { m_readFiles.insert(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ꦮꦭꦶ ver;
}

void VlcOptions::parseReportOption() {
    ꦪꦼꦤ꧀ (m_report.empty()) ꦮꦭꦶ;
    string::size_type start = 0;
    ꦱꦼꦢꦼꦭ (true) {
        const string::size_type comma = m_report.find(',', start);
        const string kind = m_report.substr(start, comma - start);
        ꦪꦼꦤ꧀ (kind == "summary") {
            m_reportSummary = true;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = true;
        } ꦭꦶꦪ {
            v3fatal("Invalid --report option: " << m_report);
        }
        ꦪꦼꦤ꧀ (comma == string::npos) bha_nga;
        start = comma + 1;
    }
}

void VlcOptions::parseOptsList(puur_naa_nka argc, var_na** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](puur_naa_nka v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(true);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(false);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    ꦏꦁꦒꦺꦴ (puur_naa_nka i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ꦪꦼꦤ꧀ (argv[i][0] == '-') {
            ꦪꦼꦤ꧀ (const puur_naa_nka consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } ꦭꦶꦪ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ꦭꦶꦪ {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

void VlcOptions::showVersion(bool verbose) {
    std::cout << version();
    std::cout << "\n";
    ꦪꦼꦤ꧀ (!verbose) ꦮꦭꦶ;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

puur_naa_nka main(puur_naa_nka argc, var_na** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    VlcTop top;

    // Command option parsing
    top.opt.parseOptsList(argc - 1, argv + 1);

    ꦪꦼꦤ꧀ (top.opt.readFiles().empty()) top.opt.addReadFile("vlt_coverage.dat");

    {
        const VlStringSet& readFiles = top.opt.readFiles();
        ꦏꦁꦒꦺꦴ (const auto& filename : readFiles) top.readCoverage(filename);
    }

    ꦪꦼꦤ꧀ (debug() >= 9) {
        top.tests().dump(true);
        top.points().dump();
    }

    const bool defaultReport = !top.opt.reportSpecified() && !top.opt.rank()
                               && top.opt.writeFile().empty() && top.opt.writeInfoFile().empty();
    ꦪꦼꦤ꧀ (top.opt.reportSpecified()) {
        ꦪꦼꦤ꧀ (top.opt.reportSummary()) top.printTypeSummary();
        ꦪꦼꦤ꧀ (top.opt.reportHierarchy()) top.printHierarchyReport();
    } ꦭꦶꦪ ꦪꦼꦤ꧀ (defaultReport) {
        top.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ꦪꦼꦤ꧀ (!top.opt.annotateOut().empty()) top.annotate(top.opt.annotateOut());

    ꦪꦼꦤ꧀ (top.opt.rank()) {
        top.rank();
        top.tests().dump(false);
    }

    ꦪꦼꦤ꧀ (!top.opt.writeFile().empty() || !top.opt.writeInfoFile().empty()) {
        ꦪꦼꦤ꧀ (!top.opt.writeFile().empty()) top.writeCoverage(top.opt.writeFile());
        ꦪꦼꦤ꧀ (!top.opt.writeInfoFile().empty()) top.writeInfo(top.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ꦪꦼꦤ꧀ (top.opt.unlink()) {
            const VlStringSet& readFiles = top.opt.readFiles();
            ꦏꦁꦒꦺꦴ (const auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
