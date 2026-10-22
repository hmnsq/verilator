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

#અન્તર્ભાવ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#અન્તર્ભાવ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#અન્તર્ભાવ "V3Error.h"
સ્થિર પૂર્ણાંક debug() { પાછા V3Error::debugDefault(); }
#અન્તર્ભાવ "V3Error.cpp"
#અન્તર્ભાવ "V3String.cpp"
#define V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#અન્તર્ભાવ "V3OptionParser.cpp"
#અન્તર્ભાવ "V3Os.cpp"
#અન્તર્ભાવ "VlcTop.cpp"
// clang-format on

#અન્તર્ભાવ "VlcOptions.h"
#અન્તર્ભાવ "VlcTop.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <fstream>

//######################################################################
// VlcOptions

શૂન્ય VlcOptions::addReadFile(અચલ string& filename) { m_readFiles.ઉમેરો(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    પાછા ver;
}

શૂન્ય VlcOptions::parseReportOption() {
    જો (m_report.empty()) પાછા;
    string::size_type start = 0;
    જ્યારે (સાચું) {
        અચલ string::size_type comma = m_report.find(',', start);
        અચલ string kind = m_report.substr(start, comma - start);
        જો (kind == "summary") {
            m_reportSummary = સાચું;
        } નહીંતર જો (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = સાચું;
        } નહીંતર {
            v3fatal("Invalid --report option: " << m_report);
        }
        જો (comma == string::npos) તોડો;
        start = comma + 1;
    }
}

શૂન્ય VlcOptions::parseOptsList(પૂર્ણાંક argc, અક્ષર** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-annotate", Set, &m_annotateOut);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", Set, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](પૂર્ણાંક v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", Set, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", Set, &m_reportLevels);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", Set, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        showVersion(સાચું);
        std::exit(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(ખોટું);
        std::exit(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    માટે (પૂર્ણાંક i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        જો (argv[i][0] == '-') {
            જો (અચલ પૂર્ણાંક consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } નહીંતર {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } નહીંતર {
            addReadFile(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

શૂન્ય VlcOptions::showVersion(તાર્કિક verbose) {
    std::cout << version();
    std::cout << "\n";
    જો (!verbose) પાછા;

    std::cout << "\n";
    std::cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::cout << "License Version 2.0.\n";

    std::cout << "\n";
    std::cout << "See https://verilator.org for documentation\n";
}

//######################################################################

પૂર્ણાંક મુખ્ય(પૂર્ણાંક argc, અક્ષર** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_ટોચ ટોચ;

    // Command option parsing
    ટોચ.opt.parseOptsList(argc - 1, argv + 1);

    જો (ટોચ.opt.readFiles().empty()) ટોચ.opt.addReadFile("vlt_coverage.dat");

    {
        અચલ VlStringSet& readFiles = ટોચ.opt.readFiles();
        માટે (અચલ auto& filename : readFiles) ટોચ.readCoverage(filename);
    }

    જો (debug() >= 9) {
        ટોચ.tests().dump(સાચું);
        ટોચ.points().dump();
    }

    અચલ તાર્કિક defaultReport = !ટોચ.opt.reportSpecified() && !ટોચ.opt.rank()
                               && ટોચ.opt.writeFile().empty() && ટોચ.opt.writeInfoFile().empty();
    જો (ટોચ.opt.reportSpecified()) {
        જો (ટોચ.opt.reportSummary()) ટોચ.printTypeSummary();
        જો (ટોચ.opt.reportHierarchy()) ટોચ.printHierarchyReport();
    } નહીંતર જો (defaultReport) {
        ટોચ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    જો (!ટોચ.opt.annotateOut().empty()) ટોચ.annotate(ટોચ.opt.annotateOut());

    જો (ટોચ.opt.rank()) {
        ટોચ.rank();
        ટોચ.tests().dump(ખોટું);
    }

    જો (!ટોચ.opt.writeFile().empty() || !ટોચ.opt.writeInfoFile().empty()) {
        જો (!ટોચ.opt.writeFile().empty()) ટોચ.writeCoverage(ટોચ.opt.writeFile());
        જો (!ટોચ.opt.writeInfoFile().empty()) ટોચ.writeInfo(ટોચ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        જો (ટોચ.opt.unlink()) {
            અચલ VlStringSet& readFiles = ટોચ.opt.readFiles();
            માટે (અચલ auto& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
