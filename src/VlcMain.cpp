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

#অন্তর্ভাৱ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#অন্তর্ভাৱ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#অন্তর্ভাৱ "V3Error.h"
স্থির পূর্ণাঙ্ক debug() { ঘূৰাই V3Error::debugDefault(); }
#অন্তর্ভাৱ "V3Error.cpp"
#অন্তর্ভাৱ "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#অন্তর্ভাৱ "V3OptionParser.cpp"
#অন্তর্ভাৱ "V3Os.cpp"
#অন্তর্ভাৱ "VlcTop.cpp"
// clang-format on

#অন্তর্ভাৱ "VlcOptions.h"
#অন্তর্ভাৱ "VlcTop.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <fstream>

//######################################################################
// VlcOptions

শূন্য VlcOptions::addReadFile(ধ্রুৱ string& filename) { m_readFiles.ঢোকাও(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ঘূৰাই ver;
}

শূন্য VlcOptions::parseReportOption() {
    যদি (m_report.empty()) ঘূৰাই;
    string::size_type start = 0;
    যেতিয়ালৈ (সঁচা) {
        ধ্রুৱ string::size_type comma = m_report.find(',', start);
        ধ্রুৱ string kind = m_report.substr(start, comma - start);
        যদি (kind == "summary") {
            m_reportSummary = সঁচা;
        } অন্যথা যদি (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = সঁচা;
        } অন্যথা {
            v3fatal("Invalid --report option: " << m_report);
        }
        যদি (comma == string::npos) ভঙ্গ;
        start = comma + 1;
    }
}

শূন্য VlcOptions::parseOptsList(পূর্ণাঙ্ক argc, ৱর্ণ** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](পূর্ণাঙ্ক v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(সঁচা);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(মিছা);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    বাবে (পূর্ণাঙ্ক i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        যদি (argv[i][0] == '-') {
            যদি (ধ্রুৱ পূর্ণাঙ্ক consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } অন্যথা {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } অন্যথা {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

শূন্য VlcOptions::showVersion(তার্কিক verbose) {
    std::cout << version();
    std::cout << "\n";
    যদি (!verbose) ঘূৰাই;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

পূর্ণাঙ্ক main(পূর্ণাঙ্ক argc, ৱর্ণ** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_শীৰ্ষ শীৰ্ষ;

    // Command option parsing
    শীৰ্ষ.opt.parseOptsList(argc - 1, argv + 1);

    যদি (শীৰ্ষ.opt.readFiles().empty()) শীৰ্ষ.opt.addReadFile("vlt_coverage.dat");

    {
        ধ্রুৱ VlStringSet& readFiles = শীৰ্ষ.opt.readFiles();
        বাবে (ধ্রুৱ auto& filename : readFiles) শীৰ্ষ.readCoverage(filename);
    }

    যদি (debug() >= 9) {
        শীৰ্ষ.tests().dump(সঁচা);
        শীৰ্ষ.points().dump();
    }

    ধ্রুৱ তার্কিক defaultReport = !শীৰ্ষ.opt.reportSpecified() && !শীৰ্ষ.opt.rank()
                               && শীৰ্ষ.opt.writeFile().empty() && শীৰ্ষ.opt.writeInfoFile().empty();
    যদি (শীৰ্ষ.opt.reportSpecified()) {
        যদি (শীৰ্ষ.opt.reportSummary()) শীৰ্ষ.printTypeSummary();
        যদি (শীৰ্ষ.opt.reportHierarchy()) শীৰ্ষ.printHierarchyReport();
    } অন্যথা যদি (defaultReport) {
        শীৰ্ষ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    যদি (!শীৰ্ষ.opt.annotateOut().empty()) শীৰ্ষ.annotate(শীৰ্ষ.opt.annotateOut());

    যদি (শীৰ্ষ.opt.rank()) {
        শীৰ্ষ.rank();
        শীৰ্ষ.tests().dump(মিছা);
    }

    যদি (!শীৰ্ষ.opt.writeFile().empty() || !শীৰ্ষ.opt.writeInfoFile().empty()) {
        যদি (!শীৰ্ষ.opt.writeFile().empty()) শীৰ্ষ.writeCoverage(শীৰ্ষ.opt.writeFile());
        যদি (!শীৰ্ষ.opt.writeInfoFile().empty()) শীৰ্ষ.writeInfo(শীৰ্ষ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        যদি (শীৰ্ষ.opt.unlink()) {
            ধ্রুৱ VlStringSet& readFiles = শীৰ্ষ.opt.readFiles();
            বাবে (ধ্রুৱ auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
