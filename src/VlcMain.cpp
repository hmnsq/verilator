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
स्थिर पूर्णाङ्क debug() { पटिनिवत्त V3Error::debugDefault(); }
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

शून्य VlcOptions::addReadFile(ध्रुव string& filename) { m_readFiles.निवेशय(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    पटिनिवत्त ver;
}

शून्य VlcOptions::parseReportOption() {
    सचे (m_report.empty()) पटिनिवत्त;
    string::size_type start = 0;
    याव (सच्च) {
        ध्रुव string::size_type comma = m_report.find(',', start);
        ध्रुव string kind = m_report.substr(start, comma - start);
        सचे (kind == "summary") {
            m_reportSummary = सच्च;
        } अञ्ञथा सचे (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = सच्च;
        } अञ्ञथा {
            v3fatal("Invalid --report option: " << m_report);
        }
        सचे (comma == string::npos) भङ्ग;
        start = comma + 1;
    }
}

शून्य VlcOptions::parseOptsList(पूर्णाङ्क argc, वर्ण** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](पूर्णाङ्क v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(सच्च);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(मिच्छा);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    अत्थाय (पूर्णाङ्क i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        सचे (argv[i][0] == '-') {
            सचे (ध्रुव पूर्णाङ्क consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } अञ्ञथा {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } अञ्ञथा {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

शून्य VlcOptions::showVersion(तार्किक verbose) {
    std::cout << version();
    std::cout << "\n";
    सचे (!verbose) पटिनिवत्त;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

पूर्णाङ्क main(पूर्णाङ्क argc, वर्ण** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_शिखरम् शिखरम्;

    // Command option parsing
    शिखरम्.opt.parseOptsList(argc - 1, argv + 1);

    सचे (शिखरम्.opt.readFiles().empty()) शिखरम्.opt.addReadFile("vlt_coverage.dat");

    {
        ध्रुव VlStringSet& readFiles = शिखरम्.opt.readFiles();
        अत्थाय (ध्रुव auto& filename : readFiles) शिखरम्.readCoverage(filename);
    }

    सचे (debug() >= 9) {
        शिखरम्.tests().dump(सच्च);
        शिखरम्.points().dump();
    }

    ध्रुव तार्किक defaultReport = !शिखरम्.opt.reportSpecified() && !शिखरम्.opt.rank()
                               && शिखरम्.opt.writeFile().empty() && शिखरम्.opt.writeInfoFile().empty();
    सचे (शिखरम्.opt.reportSpecified()) {
        सचे (शिखरम्.opt.reportSummary()) शिखरम्.printTypeSummary();
        सचे (शिखरम्.opt.reportHierarchy()) शिखरम्.printHierarchyReport();
    } अञ्ञथा सचे (defaultReport) {
        शिखरम्.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    सचे (!शिखरम्.opt.annotateOut().empty()) शिखरम्.annotate(शिखरम्.opt.annotateOut());

    सचे (शिखरम्.opt.rank()) {
        शिखरम्.rank();
        शिखरम्.tests().dump(मिच्छा);
    }

    सचे (!शिखरम्.opt.writeFile().empty() || !शिखरम्.opt.writeInfoFile().empty()) {
        सचे (!शिखरम्.opt.writeFile().empty()) शिखरम्.writeCoverage(शिखरम्.opt.writeFile());
        सचे (!शिखरम्.opt.writeInfoFile().empty()) शिखरम्.writeInfo(शिखरम्.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        सचे (शिखरम्.opt.unlink()) {
            ध्रुव VlStringSet& readFiles = शिखरम्.opt.readFiles();
            अत्थाय (ध्रुव auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
