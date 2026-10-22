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

#அந்தர்பாவ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#அந்தர்பாவ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#அந்தர்பாவ "V3Error.h"
நிலையான முழு debug() { திரும்பு V3Error::debugDefault(); }
#அந்தர்பாவ "V3Error.cpp"
#அந்தர்பாவ "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#அந்தர்பாவ "V3OptionParser.cpp"
#அந்தர்பாவ "V3Os.cpp"
#அந்தர்பாவ "VlcTop.cpp"
// clang-format on

#அந்தர்பாவ "VlcOptions.h"
#அந்தர்பாவ "VlcTop.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <fstream>

//######################################################################
// VlcOptions

வெறுமை VlcOptions::addReadFile(மாறிலி string& filename) { m_readFiles.செருகு(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    திரும்பு ver;
}

வெறுமை VlcOptions::parseReportOption() {
    எனில் (m_report.empty()) திரும்பு;
    string::size_type start = 0;
    போது (உண்மை) {
        மாறிலி string::size_type comma = m_report.find(',', start);
        மாறிலி string kind = m_report.substr(start, comma - start);
        எனில் (kind == "summary") {
            m_reportSummary = உண்மை;
        } இல்லையெனில் எனில் (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = உண்மை;
        } இல்லையெனில் {
            v3fatal("Invalid --report option: " << m_report);
        }
        எனில் (comma == string::npos) நிறுத்து;
        start = comma + 1;
    }
}

வெறுமை VlcOptions::parseOptsList(முழு argc, எழுத்து** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](முழு v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(உண்மை);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(பொய்);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    க்கு (முழு i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        எனில் (argv[i][0] == '-') {
            எனில் (மாறிலி முழு consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } இல்லையெனில் {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } இல்லையெனில் {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

வெறுமை VlcOptions::showVersion(தருக்க verbose) {
    std::cout << version();
    std::cout << "\n";
    எனில் (!verbose) திரும்பு;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

முழு முதன்மை(முழு argc, எழுத்து** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_உச்சி உச்சி;

    // Command option parsing
    உச்சி.opt.parseOptsList(argc - 1, argv + 1);

    எனில் (உச்சி.opt.readFiles().empty()) உச்சி.opt.addReadFile("vlt_coverage.dat");

    {
        மாறிலி VlStringSet& readFiles = உச்சி.opt.readFiles();
        க்கு (மாறிலி auto& filename : readFiles) உச்சி.readCoverage(filename);
    }

    எனில் (debug() >= 9) {
        உச்சி.tests().dump(உண்மை);
        உச்சி.points().dump();
    }

    மாறிலி தருக்க defaultReport = !உச்சி.opt.reportSpecified() && !உச்சி.opt.rank()
                               && உச்சி.opt.writeFile().empty() && உச்சி.opt.writeInfoFile().empty();
    எனில் (உச்சி.opt.reportSpecified()) {
        எனில் (உச்சி.opt.reportSummary()) உச்சி.printTypeSummary();
        எனில் (உச்சி.opt.reportHierarchy()) உச்சி.printHierarchyReport();
    } இல்லையெனில் எனில் (defaultReport) {
        உச்சி.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    எனில் (!உச்சி.opt.annotateOut().empty()) உச்சி.annotate(உச்சி.opt.annotateOut());

    எனில் (உச்சி.opt.rank()) {
        உச்சி.rank();
        உச்சி.tests().dump(பொய்);
    }

    எனில் (!உச்சி.opt.writeFile().empty() || !உச்சி.opt.writeInfoFile().empty()) {
        எனில் (!உச்சி.opt.writeFile().empty()) உச்சி.writeCoverage(உச்சி.opt.writeFile());
        எனில் (!உச்சி.opt.writeInfoFile().empty()) உச்சி.writeInfo(உச்சி.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        எனில் (உச்சி.opt.unlink()) {
            மாறிலி VlStringSet& readFiles = உச்சி.opt.readFiles();
            க்கு (மாறிலி auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
