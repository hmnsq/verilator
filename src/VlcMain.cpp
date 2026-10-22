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
static ຈຳນວນເຕັມ debug() { ສົ່ງຄືນ V3Error::debugDefault(); }
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
    ສົ່ງຄືນ ver;
}

void VlcOptions::parseReportOption() {
    ຖ້າ (m_report.empty()) ສົ່ງຄືນ;
    string::size_type start = 0;
    ໃນຂະນະທີ່ (true) {
        const string::size_type comma = m_report.find(',', start);
        const string kind = m_report.substr(start, comma - start);
        ຖ້າ (kind == "summary") {
            m_reportSummary = true;
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = true;
        } ບໍ່ດັ່ງນັ້ນ {
            v3fatal("Invalid --report option: " << m_report);
        }
        ຖ້າ (comma == string::npos) bha_nga;
        start = comma + 1;
    }
}

void VlcOptions::parseOptsList(ຈຳນວນເຕັມ argc, var_na** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](ຈຳນວນເຕັມ v) { V3Error::debugDefault(v); });
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
    ສຳລັບ (ຈຳນວນເຕັມ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ຖ້າ (argv[i][0] == '-') {
            ຖ້າ (const ຈຳນວນເຕັມ consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } ບໍ່ດັ່ງນັ້ນ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ບໍ່ດັ່ງນັ້ນ {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

void VlcOptions::showVersion(bool verbose) {
    std::cout << version();
    std::cout << "\n";
    ຖ້າ (!verbose) ສົ່ງຄືນ;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

ຈຳນວນເຕັມ main(ຈຳນວນເຕັມ argc, var_na** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    VlcTop top;

    // Command option parsing
    top.opt.parseOptsList(argc - 1, argv + 1);

    ຖ້າ (top.opt.readFiles().empty()) top.opt.addReadFile("vlt_coverage.dat");

    {
        const VlStringSet& readFiles = top.opt.readFiles();
        ສຳລັບ (const auto& filename : readFiles) top.readCoverage(filename);
    }

    ຖ້າ (debug() >= 9) {
        top.tests().dump(true);
        top.points().dump();
    }

    const bool defaultReport = !top.opt.reportSpecified() && !top.opt.rank()
                               && top.opt.writeFile().empty() && top.opt.writeInfoFile().empty();
    ຖ້າ (top.opt.reportSpecified()) {
        ຖ້າ (top.opt.reportSummary()) top.printTypeSummary();
        ຖ້າ (top.opt.reportHierarchy()) top.printHierarchyReport();
    } ບໍ່ດັ່ງນັ້ນ ຖ້າ (defaultReport) {
        top.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ຖ້າ (!top.opt.annotateOut().empty()) top.annotate(top.opt.annotateOut());

    ຖ້າ (top.opt.rank()) {
        top.rank();
        top.tests().dump(false);
    }

    ຖ້າ (!top.opt.writeFile().empty() || !top.opt.writeInfoFile().empty()) {
        ຖ້າ (!top.opt.writeFile().empty()) top.writeCoverage(top.opt.writeFile());
        ຖ້າ (!top.opt.writeInfoFile().empty()) top.writeInfo(top.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ຖ້າ (top.opt.unlink()) {
            const VlStringSet& readFiles = top.opt.readFiles();
            ສຳລັບ (const auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
