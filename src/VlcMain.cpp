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

#ضمن "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#ضمن "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ضمن "V3Error.h"
ساكن صحيح debug() { أرجع V3Error::debugDefault(); }
#ضمن "V3Error.cpp"
#ضمن "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#ضمن "V3OptionParser.cpp"
#ضمن "V3Os.cpp"
#ضمن "VlcTop.cpp"
// clang-format on

#ضمن "VlcOptions.h"
#ضمن "VlcTop.h"

#ضمن <algorithm>
#ضمن <fstream>

//######################################################################
// VlcOptions

فراغ VlcOptions::addReadFile(ثابت string& filename) { m_readFiles.سيسيڤ(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    أرجع ver;
}

فراغ VlcOptions::parseReportOption() {
    إذا (m_report.empty()) أرجع;
    string::size_type start = 0;
    طالما (true) {
        ثابت string::size_type comma = m_report.find(',', start);
        ثابت string kind = m_report.substr(start, comma - start);
        إذا (kind == "summary") {
            m_reportSummary = true;
        } وإلا إذا (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = true;
        } وإلا {
            v3fatal("Invalid --report option: " << m_report);
        }
        إذا (comma == string::npos) اكسر;
        start = comma + 1;
    }
}

فراغ VlcOptions::parseOptsList(صحيح argc, حرف** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحيح v) { V3Error::debugDefault(v); });
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
        showVersion(خطأ);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        إذا (argv[i][0] == '-') {
            إذا (ثابت صحيح consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } وإلا {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } وإلا {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

فراغ VlcOptions::showVersion(منطقي verbose) {
    std::cout << version();
    std::cout << "\n";
    إذا (!verbose) أرجع;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

صحيح main(صحيح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_اتس اتس;

    // Command option parsing
    اتس.opt.parseOptsList(argc - 1, argv + 1);

    إذا (اتس.opt.readFiles().empty()) اتس.opt.addReadFile("vlt_coverage.dat");

    {
        ثابت VlStringSet& readFiles = اتس.opt.readFiles();
        لكل (ثابت auto& filename : readFiles) اتس.readCoverage(filename);
    }

    إذا (debug() >= 9) {
        اتس.tests().dump(true);
        اتس.points().dump();
    }

    ثابت منطقي defaultReport = !اتس.opt.reportSpecified() && !اتس.opt.rank()
                               && اتس.opt.writeFile().empty() && اتس.opt.writeInfoFile().empty();
    إذا (اتس.opt.reportSpecified()) {
        إذا (اتس.opt.reportSummary()) اتس.printTypeSummary();
        إذا (اتس.opt.reportHierarchy()) اتس.printHierarchyReport();
    } وإلا إذا (defaultReport) {
        اتس.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    إذا (!اتس.opt.annotateOut().empty()) اتس.annotate(اتس.opt.annotateOut());

    إذا (اتس.opt.rank()) {
        اتس.rank();
        اتس.tests().dump(خطأ);
    }

    إذا (!اتس.opt.writeFile().empty() || !اتس.opt.writeInfoFile().empty()) {
        إذا (!اتس.opt.writeFile().empty()) اتس.writeCoverage(اتس.opt.writeFile());
        إذا (!اتس.opt.writeInfoFile().empty()) اتس.writeInfo(اتس.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        إذا (اتس.opt.unlink()) {
            ثابت VlStringSet& readFiles = اتس.opt.readFiles();
            لكل (ثابت auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
