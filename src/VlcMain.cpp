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

#അന്തര്ഭാവ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#അന്തര്ഭാവ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#അന്തര്ഭാവ "V3Error.h"
സ്ഥിതം പൂർണ്ണസംഖ്യ debug() { തിരികെ V3Error::debugDefault(); }
#അന്തര്ഭാവ "V3Error.cpp"
#അന്തര്ഭാവ "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#അന്തര്ഭാവ "V3OptionParser.cpp"
#അന്തര്ഭാവ "V3Os.cpp"
#അന്തര്ഭാവ "VlcTop.cpp"
// clang-format on

#അന്തര്ഭാവ "VlcOptions.h"
#അന്തര്ഭാവ "VlcTop.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <fstream>

//######################################################################
// VlcOptions

ശൂന്യം VlcOptions::addReadFile(സ്ഥിരം string& filename) { m_readFiles.തിരുകുക(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    തിരികെ ver;
}

ശൂന്യം VlcOptions::parseReportOption() {
    എങ്കിൽ (m_report.empty()) തിരികെ;
    string::size_type start = 0;
    ആകുമ്പോൾ (സത്യം) {
        സ്ഥിരം string::size_type comma = m_report.find(',', start);
        സ്ഥിരം string kind = m_report.substr(start, comma - start);
        എങ്കിൽ (kind == "summary") {
            m_reportSummary = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = സത്യം;
        } അല്ലെങ്കിൽ {
            v3fatal("Invalid --report option: " << m_report);
        }
        എങ്കിൽ (comma == string::npos) നിർത്തുക;
        start = comma + 1;
    }
}

ശൂന്യം VlcOptions::parseOptsList(പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](പൂർണ്ണസംഖ്യ v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(സത്യം);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(അസത്യം);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        എങ്കിൽ (argv[i][0] == '-') {
            എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } അല്ലെങ്കിൽ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } അല്ലെങ്കിൽ {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

ശൂന്യം VlcOptions::showVersion(തർക്കം verbose) {
    std::cout << version();
    std::cout << "\n";
    എങ്കിൽ (!verbose) തിരികെ;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

പൂർണ്ണസംഖ്യ പ്രധാന(പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_മുകൾ മുകൾ;

    // Command option parsing
    മുകൾ.opt.parseOptsList(argc - 1, argv + 1);

    എങ്കിൽ (മുകൾ.opt.readFiles().empty()) മുകൾ.opt.addReadFile("vlt_coverage.dat");

    {
        സ്ഥിരം VlStringSet& readFiles = മുകൾ.opt.readFiles();
        വേണ്ടി (സ്ഥിരം auto& filename : readFiles) മുകൾ.readCoverage(filename);
    }

    എങ്കിൽ (debug() >= 9) {
        മുകൾ.tests().dump(സത്യം);
        മുകൾ.points().dump();
    }

    സ്ഥിരം തർക്കം defaultReport = !മുകൾ.opt.reportSpecified() && !മുകൾ.opt.rank()
                               && മുകൾ.opt.writeFile().empty() && മുകൾ.opt.writeInfoFile().empty();
    എങ്കിൽ (മുകൾ.opt.reportSpecified()) {
        എങ്കിൽ (മുകൾ.opt.reportSummary()) മുകൾ.printTypeSummary();
        എങ്കിൽ (മുകൾ.opt.reportHierarchy()) മുകൾ.printHierarchyReport();
    } അല്ലെങ്കിൽ എങ്കിൽ (defaultReport) {
        മുകൾ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    എങ്കിൽ (!മുകൾ.opt.annotateOut().empty()) മുകൾ.annotate(മുകൾ.opt.annotateOut());

    എങ്കിൽ (മുകൾ.opt.rank()) {
        മുകൾ.rank();
        മുകൾ.tests().dump(അസത്യം);
    }

    എങ്കിൽ (!മുകൾ.opt.writeFile().empty() || !മുകൾ.opt.writeInfoFile().empty()) {
        എങ്കിൽ (!മുകൾ.opt.writeFile().empty()) മുകൾ.writeCoverage(മുകൾ.opt.writeFile());
        എങ്കിൽ (!മുകൾ.opt.writeInfoFile().empty()) മുകൾ.writeInfo(മുകൾ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        എങ്കിൽ (മുകൾ.opt.unlink()) {
            സ്ഥിരം VlStringSet& readFiles = മുകൾ.opt.readFiles();
            വേണ്ടി (സ്ഥിരം auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
