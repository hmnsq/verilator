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

#شامل "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#شامل "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#شامل "V3Error.h"
ایستا صحیح debug() { بازگشت V3Error::debugDefault(); }
#شامل "V3Error.cpp"
#شامل "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#شامل "V3OptionParser.cpp"
#شامل "V3Os.cpp"
#شامل "VlcTop.cpp"
// clang-format on

#شامل "VlcOptions.h"
#شامل "VlcTop.h"

#شامل <algorithm>
#شامل <fstream>

//######################################################################
// VlcOptions

تهی VlcOptions::addReadFile(ثابت string& filename) { m_readFiles.درج(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    بازگشت ver;
}

تهی VlcOptions::parseReportOption() {
    اگر (m_report.empty()) بازگشت;
    string::size_type start = 0;
    وقتی (درست) {
        ثابت string::size_type comma = m_report.find(',', start);
        ثابت string kind = m_report.substr(start, comma - start);
        اگر (kind == "summary") {
            m_reportSummary = درست;
        } وگرنه اگر (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = درست;
        } وگرنه {
            v3fatal("Invalid --report option: " << m_report);
        }
        اگر (comma == string::npos) بشکن;
        start = comma + 1;
    }
}

تهی VlcOptions::parseOptsList(صحیح argc, نویسه** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحیح v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(درست);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(نادرست);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    برای (صحیح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        اگر (argv[i][0] == '-') {
            اگر (ثابت صحیح consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } وگرنه {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } وگرنه {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

تهی VlcOptions::showVersion(منطقی verbose) {
    std::cout << version();
    std::cout << "\n";
    اگر (!verbose) بازگشت;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

صحیح اصلی(صحیح argc, نویسه** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_بالاترین بالاترین;

    // Command option parsing
    بالاترین.opt.parseOptsList(argc - 1, argv + 1);

    اگر (بالاترین.opt.readFiles().empty()) بالاترین.opt.addReadFile("vlt_coverage.dat");

    {
        ثابت VlStringSet& readFiles = بالاترین.opt.readFiles();
        برای (ثابت auto& filename : readFiles) بالاترین.readCoverage(filename);
    }

    اگر (debug() >= 9) {
        بالاترین.tests().dump(درست);
        بالاترین.points().dump();
    }

    ثابت منطقی defaultReport = !بالاترین.opt.reportSpecified() && !بالاترین.opt.rank()
                               && بالاترین.opt.writeFile().empty() && بالاترین.opt.writeInfoFile().empty();
    اگر (بالاترین.opt.reportSpecified()) {
        اگر (بالاترین.opt.reportSummary()) بالاترین.printTypeSummary();
        اگر (بالاترین.opt.reportHierarchy()) بالاترین.printHierarchyReport();
    } وگرنه اگر (defaultReport) {
        بالاترین.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    اگر (!بالاترین.opt.annotateOut().empty()) بالاترین.annotate(بالاترین.opt.annotateOut());

    اگر (بالاترین.opt.rank()) {
        بالاترین.rank();
        بالاترین.tests().dump(نادرست);
    }

    اگر (!بالاترین.opt.writeFile().empty() || !بالاترین.opt.writeInfoFile().empty()) {
        اگر (!بالاترین.opt.writeFile().empty()) بالاترین.writeCoverage(بالاترین.opt.writeFile());
        اگر (!بالاترین.opt.writeInfoFile().empty()) بالاترین.writeInfo(بالاترین.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        اگر (بالاترین.opt.unlink()) {
            ثابت VlStringSet& readFiles = بالاترین.opt.readFiles();
            برای (ثابت auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
