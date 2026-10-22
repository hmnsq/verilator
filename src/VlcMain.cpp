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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ಅನ್ತರ್ಭಾವ "V3Error.h"
ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ debug() { ಪ್ರತ್ಯಾಗಮ V3Error::debugDefault(); }
#ಅನ್ತರ್ಭಾವ "V3Error.cpp"
#ಅನ್ತರ್ಭಾವ "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#ಅನ್ತರ್ಭಾವ "V3OptionParser.cpp"
#ಅನ್ತರ್ಭಾವ "V3Os.cpp"
#ಅನ್ತರ್ಭಾವ "VlcTop.cpp"
// clang-format on

#ಅನ್ತರ್ಭಾವ "VlcOptions.h"
#ಅನ್ತರ್ಭಾವ "VlcTop.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <fstream>

//######################################################################
// VlcOptions

ಶೂನ್ಯ VlcOptions::addReadFile(ಧ್ರುವ string& filename) { m_readFiles.ಸೇರಿಸು(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ಪ್ರತ್ಯಾಗಮ ver;
}

ಶೂನ್ಯ VlcOptions::parseReportOption() {
    ಯದಿ (m_report.empty()) ಪ್ರತ್ಯಾಗಮ;
    string::size_type start = 0;
    ಯಾವತ್ (ಸತ್ಯ) {
        ಧ್ರುವ string::size_type comma = m_report.find(',', start);
        ಧ್ರುವ string kind = m_report.substr(start, comma - start);
        ಯದಿ (kind == "summary") {
            m_reportSummary = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            v3fatal("Invalid --report option: " << m_report);
        }
        ಯದಿ (comma == string::npos) ಭಙ್ಗ;
        start = comma + 1;
    }
}

ಶೂನ್ಯ VlcOptions::parseOptsList(ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](ಪೂರ್ಣಾಙ್ಕ v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(ಸತ್ಯ);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(ಅಸತ್ಯ);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ಯದಿ (argv[i][0] == '-') {
            ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } ಅನ್ಯಥಾ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ಅನ್ಯಥಾ {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

ಶೂನ್ಯ VlcOptions::showVersion(ತಾರ್ಕಿಕ verbose) {
    std::cout << version();
    std::cout << "\n";
    ಯದಿ (!verbose) ಪ್ರತ್ಯಾಗಮ;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

ಪೂರ್ಣಾಙ್ಕ main(ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_ತುದಿ ತುದಿ;

    // Command option parsing
    ತುದಿ.opt.parseOptsList(argc - 1, argv + 1);

    ಯದಿ (ತುದಿ.opt.readFiles().empty()) ತುದಿ.opt.addReadFile("vlt_coverage.dat");

    {
        ಧ್ರುವ VlStringSet& readFiles = ತುದಿ.opt.readFiles();
        ಪ್ರತಿ (ಧ್ರುವ auto& filename : readFiles) ತುದಿ.readCoverage(filename);
    }

    ಯದಿ (debug() >= 9) {
        ತುದಿ.tests().dump(ಸತ್ಯ);
        ತುದಿ.points().dump();
    }

    ಧ್ರುವ ತಾರ್ಕಿಕ defaultReport = !ತುದಿ.opt.reportSpecified() && !ತುದಿ.opt.rank()
                               && ತುದಿ.opt.writeFile().empty() && ತುದಿ.opt.writeInfoFile().empty();
    ಯದಿ (ತುದಿ.opt.reportSpecified()) {
        ಯದಿ (ತುದಿ.opt.reportSummary()) ತುದಿ.printTypeSummary();
        ಯದಿ (ತುದಿ.opt.reportHierarchy()) ತುದಿ.printHierarchyReport();
    } ಅನ್ಯಥಾ ಯದಿ (defaultReport) {
        ತುದಿ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ಯದಿ (!ತುದಿ.opt.annotateOut().empty()) ತುದಿ.annotate(ತುದಿ.opt.annotateOut());

    ಯದಿ (ತುದಿ.opt.rank()) {
        ತುದಿ.rank();
        ತುದಿ.tests().dump(ಅಸತ್ಯ);
    }

    ಯದಿ (!ತುದಿ.opt.writeFile().empty() || !ತುದಿ.opt.writeInfoFile().empty()) {
        ಯದಿ (!ತುದಿ.opt.writeFile().empty()) ತುದಿ.writeCoverage(ತುದಿ.opt.writeFile());
        ಯದಿ (!ತುದಿ.opt.writeInfoFile().empty()) ತುದಿ.writeInfo(ತುದಿ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ಯದಿ (ತುದಿ.opt.unlink()) {
            ಧ್ರುವ VlStringSet& readFiles = ತುದಿ.opt.readFiles();
            ಪ್ರತಿ (ಧ್ರುವ auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
