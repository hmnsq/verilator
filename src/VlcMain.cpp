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

#अन्तर्भाव "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#अन्तर्भाव "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#अन्तर्भाव "V3Error.h"
स्थिर पूर्णांक debug() { परत V3Error::debugDefault(); }
#अन्तर्भाव "V3Error.cpp"
#अन्तर्भाव "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#अन्तर्भाव "V3OptionParser.cpp"
#अन्तर्भाव "V3Os.cpp"
#अन्तर्भाव "VlcTop.cpp"
// clang-format on

#अन्तर्भाव "VlcOptions.h"
#अन्तर्भाव "VlcTop.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <fstream>

//######################################################################
// VlcOptions

शून्य VlcOptions::addReadFile(स्थिरांक string& filename) { m_readFiles.घाला(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    परत ver;
}

शून्य VlcOptions::parseReportOption() {
    जर (m_report.empty()) परत;
    string::size_type start = 0;
    पर्यंत (सत्य) {
        स्थिरांक string::size_type comma = m_report.find(',', start);
        स्थिरांक string kind = m_report.substr(start, comma - start);
        जर (kind == "summary") {
            m_reportSummary = सत्य;
        } अन्यथा जर (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = सत्य;
        } अन्यथा {
            v3fatal("Invalid --report option: " << m_report);
        }
        जर (comma == string::npos) थांबा;
        start = comma + 1;
    }
}

शून्य VlcOptions::parseOptsList(पूर्णांक argc, अक्षर** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](पूर्णांक v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(सत्य);
        std::निकास(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(असत्य);
        std::निकास(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    साठी (पूर्णांक i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        जर (argv[i][0] == '-') {
            जर (स्थिरांक पूर्णांक consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } अन्यथा {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } अन्यथा {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

शून्य VlcOptions::showVersion(तार्किक verbose) {
    std::cout << version();
    std::cout << "\n";
    जर (!verbose) परत;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

पूर्णांक main(पूर्णांक argc, अक्षर** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_शीर्ष शीर्ष;

    // Command option parsing
    शीर्ष.opt.parseOptsList(argc - 1, argv + 1);

    जर (शीर्ष.opt.readFiles().empty()) शीर्ष.opt.addReadFile("vlt_coverage.dat");

    {
        स्थिरांक VlStringSet& readFiles = शीर्ष.opt.readFiles();
        साठी (स्थिरांक स्वतः& filename : readFiles) शीर्ष.readCoverage(filename);
    }

    जर (debug() >= 9) {
        शीर्ष.tests().dump(सत्य);
        शीर्ष.points().dump();
    }

    स्थिरांक तार्किक defaultReport = !शीर्ष.opt.reportSpecified() && !शीर्ष.opt.rank()
                               && शीर्ष.opt.writeFile().empty() && शीर्ष.opt.writeInfoFile().empty();
    जर (शीर्ष.opt.reportSpecified()) {
        जर (शीर्ष.opt.reportSummary()) शीर्ष.printTypeSummary();
        जर (शीर्ष.opt.reportHierarchy()) शीर्ष.printHierarchyReport();
    } अन्यथा जर (defaultReport) {
        शीर्ष.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    जर (!शीर्ष.opt.annotateOut().empty()) शीर्ष.annotate(शीर्ष.opt.annotateOut());

    जर (शीर्ष.opt.rank()) {
        शीर्ष.rank();
        शीर्ष.tests().dump(असत्य);
    }

    जर (!शीर्ष.opt.writeFile().empty() || !शीर्ष.opt.writeInfoFile().empty()) {
        जर (!शीर्ष.opt.writeFile().empty()) शीर्ष.writeCoverage(शीर्ष.opt.writeFile());
        जर (!शीर्ष.opt.writeInfoFile().empty()) शीर्ष.writeInfo(शीर्ष.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        जर (शीर्ष.opt.unlink()) {
            स्थिरांक VlStringSet& readFiles = शीर्ष.opt.readFiles();
            साठी (स्थिरांक स्वतः& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
