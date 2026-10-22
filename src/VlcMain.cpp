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
#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#अगर_अघोषित HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#पूर्ण_अगर
// clang-format on

#समावेश "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#अगर_अघोषित V3ERROR_NO_GLOBAL_
#घोषणा V3ERROR_NO_GLOBAL_
#पूर्ण_अगर
#समावेश "V3Error.h"
अटल पूर्णांक debug() { वापस V3Error::debugDefault(); }
#समावेश "V3Error.cpp"
#समावेश "V3String.cpp"
#घोषणा V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#समावेश "V3OptionParser.cpp"
#समावेश "V3Os.cpp"
#समावेश "VlcTop.cpp"
// clang-format on

#समावेश "VlcOptions.h"
#समावेश "VlcTop.h"

#समावेश <algorithm>
#समावेश <fstream>

//######################################################################
// VlcOptions

व्योम VlcOptions::addReadFile(स्थिर string& filename) { m_readFiles.जोड़ो(filename); }

string VlcOptions::version() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    वापस ver;
}

व्योम VlcOptions::parseReportOption() {
    अगर (m_report.empty()) वापस;
    string::आकार_type start = 0;
    जबतक (true) {
        स्थिर string::आकार_type comma = m_report.find(',', start);
        स्थिर string kind = m_report.substr(start, comma - start);
        अगर (kind == "summary") {
            m_reportSummary = true;
        } अन्यथा अगर (kind == "hier" || kind == "hierarchy") {
            m_reportHierarchy = true;
        } अन्यथा {
            v3fatal("Invalid --report option: " << m_report);
        }
        अगर (comma == string::npos) अवरोध;
        start = comma + 1;
    }
}

व्योम VlcOptions::parseOptsList(पूर्णांक argc, अक्षर** argv) {
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
        showVersion(true);
        std::निकास(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        showVersion(false);
        std::निकास(0);
    });
    DECL_OPTION("-write", Set, &m_writeFile);
    DECL_OPTION("-write-info", Set, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    क्रम (पूर्णांक i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        अगर (argv[i][0] == '-') {
            अगर (स्थिर पूर्णांक consumed = parser.parse(i, argc, argv).first) {
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

व्योम VlcOptions::showVersion(bool verbose) {
    std::अ_बाहर << version();
    std::अ_बाहर << "\n";
    अगर (!verbose) वापस;

    std::अ_बाहर << "\n";
    std::अ_बाहर << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::अ_बाहर << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::अ_बाहर << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::अ_बाहर << "License Version 2.0.\n";

    std::अ_बाहर << "\n";
    std::अ_बाहर << "See https://verilator.org for documentation\n";
}

//######################################################################

पूर्णांक मुख्य(पूर्णांक argc, अक्षर** argv) {
    // General initialization
    std::पनप::sync_with_stdio();

    Vlc_शीर्ष शीर्ष;

    // Command option parsing
    शीर्ष.opt.parseOptsList(argc - 1, argv + 1);

    अगर (शीर्ष.opt.readFiles().empty()) शीर्ष.opt.addReadFile("vlt_coverage.dat");

    {
        स्थिर VlStringSet& readFiles = शीर्ष.opt.readFiles();
        क्रम (स्थिर स्वतः& filename : readFiles) शीर्ष.readCoverage(filename);
    }

    अगर (debug() >= 9) {
        शीर्ष.tests().dump(true);
        शीर्ष.points().dump();
    }

    स्थिर bool defaultReport = !शीर्ष.opt.reportSpecified() && !शीर्ष.opt.rank()
                               && शीर्ष.opt.writeFile().empty() && शीर्ष.opt.writeInfoFile().empty();
    अगर (शीर्ष.opt.reportSpecified()) {
        अगर (शीर्ष.opt.reportSummary()) शीर्ष.printTypeSummary();
        अगर (शीर्ष.opt.reportHierarchy()) शीर्ष.printHierarchyReport();
    } अन्यथा अगर (defaultReport) {
        शीर्ष.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    अगर (!शीर्ष.opt.annotateOut().empty()) शीर्ष.annotate(शीर्ष.opt.annotateOut());

    अगर (शीर्ष.opt.rank()) {
        शीर्ष.rank();
        शीर्ष.tests().dump(false);
    }

    अगर (!शीर्ष.opt.writeFile().empty() || !शीर्ष.opt.writeInfoFile().empty()) {
        अगर (!शीर्ष.opt.writeFile().empty()) शीर्ष.writeCoverage(शीर्ष.opt.writeFile());
        अगर (!शीर्ष.opt.writeInfoFile().empty()) शीर्ष.writeInfo(शीर्ष.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        अगर (शीर्ष.opt.unlink()) {
            स्थिर VlStringSet& readFiles = शीर्ष.opt.readFiles();
            क्रम (स्थिर स्वतः& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
