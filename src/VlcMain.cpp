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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା debug() { ଫେରନ୍ତୁ V3Error::debugDefault(); }
#ଅନ୍ତର୍ଭାଵ "V3Error.cpp"
#ଅନ୍ତର୍ଭାଵ "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#ଅନ୍ତର୍ଭାଵ "V3OptionParser.cpp"
#ଅନ୍ତର୍ଭାଵ "V3Os.cpp"
#ଅନ୍ତର୍ଭାଵ "VlcTop.cpp"
// clang-format on

#ଅନ୍ତର୍ଭାଵ "VlcOptions.h"
#ଅନ୍ତର୍ଭାଵ "VlcTop.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <fstream>

//######################################################################
// VlcOptions

ଶୂନ୍ୟ VlcOptions::addReadFile(ଅଚଳ string& filename) { m_readFiles.ଭର୍ତ୍ତି(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ଫେରନ୍ତୁ ver;
}

ଶୂନ୍ୟ VlcOptions::parseReportOption() {
    ଯଦି (m_report.empty()) ଫେରନ୍ତୁ;
    string::size_type start = 0;
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଅଚଳ string::size_type comma = m_report.find(',', start);
        ଅଚଳ string kind = m_report.substr(start, comma - start);
        ଯଦି (kind == "summary") {
            m_reportSummary = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            v3fatal("Invalid --report option: " << m_report);
        }
        ଯଦି (comma == string::npos) ଭାଙ୍ଗ;
        start = comma + 1;
    }
}

ଶୂନ୍ୟ VlcOptions::parseOptsList(ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](ପୂର୍ଣ୍ଣସଂଖ୍ୟା v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(ସତ୍ୟ);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(ମିଥ୍ୟା);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ଯଦି (argv[i][0] == '-') {
            ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } ଅନ୍ୟଥା {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ଅନ୍ୟଥା {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

ଶୂନ୍ୟ VlcOptions::showVersion(ତାର୍କିକ verbose) {
    std::cout << version();
    std::cout << "\n";
    ଯଦି (!verbose) ଫେରନ୍ତୁ;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୁଖ୍ୟ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_ଶୀର୍ଷ ଶୀର୍ଷ;

    // Command option parsing
    ଶୀର୍ଷ.opt.parseOptsList(argc - 1, argv + 1);

    ଯଦି (ଶୀର୍ଷ.opt.readFiles().empty()) ଶୀର୍ଷ.opt.addReadFile("vlt_coverage.dat");

    {
        ଅଚଳ VlStringSet& readFiles = ଶୀର୍ଷ.opt.readFiles();
        ପାଇଁ (ଅଚଳ auto& filename : readFiles) ଶୀର୍ଷ.readCoverage(filename);
    }

    ଯଦି (debug() >= 9) {
        ଶୀର୍ଷ.tests().dump(ସତ୍ୟ);
        ଶୀର୍ଷ.points().dump();
    }

    ଅଚଳ ତାର୍କିକ defaultReport = !ଶୀର୍ଷ.opt.reportSpecified() && !ଶୀର୍ଷ.opt.rank()
                               && ଶୀର୍ଷ.opt.writeFile().empty() && ଶୀର୍ଷ.opt.writeInfoFile().empty();
    ଯଦି (ଶୀର୍ଷ.opt.reportSpecified()) {
        ଯଦି (ଶୀର୍ଷ.opt.reportSummary()) ଶୀର୍ଷ.printTypeSummary();
        ଯଦି (ଶୀର୍ଷ.opt.reportHierarchy()) ଶୀର୍ଷ.printHierarchyReport();
    } ଅନ୍ୟଥା ଯଦି (defaultReport) {
        ଶୀର୍ଷ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ଯଦି (!ଶୀର୍ଷ.opt.annotateOut().empty()) ଶୀର୍ଷ.annotate(ଶୀର୍ଷ.opt.annotateOut());

    ଯଦି (ଶୀର୍ଷ.opt.rank()) {
        ଶୀର୍ଷ.rank();
        ଶୀର୍ଷ.tests().dump(ମିଥ୍ୟା);
    }

    ଯଦି (!ଶୀର୍ଷ.opt.writeFile().empty() || !ଶୀର୍ଷ.opt.writeInfoFile().empty()) {
        ଯଦି (!ଶୀର୍ଷ.opt.writeFile().empty()) ଶୀର୍ଷ.writeCoverage(ଶୀର୍ଷ.opt.writeFile());
        ଯଦି (!ଶୀର୍ଷ.opt.writeInfoFile().empty()) ଶୀର୍ଷ.writeInfo(ଶୀର୍ଷ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ଯଦି (ଶୀର୍ଷ.opt.unlink()) {
            ଅଚଳ VlStringSet& readFiles = ଶୀର୍ଷ.opt.readFiles();
            ପାଇଁ (ଅଚଳ auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
