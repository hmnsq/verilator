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
#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
#ವ್ಯಾಖ್ಯಾನಿಸು V3ERROR_NO_GLOBAL_
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ "V3Error.h"
ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ debug() { ಹಿಂತಿರುಗಿ V3Error::debugDefault(); }
#ಅನ್ತರ್ಭಾವ "V3Error.cpp"
#ಅನ್ತರ್ಭಾವ "V3String.cpp"
#ವ್ಯಾಖ್ಯಾನಿಸು V3OPTION_PARSER_NO_VOPTION_BOOL
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

ಶೂನ್ಯ VlcOptions::ಕೂಡಿಸು_Read_File(ಸ್ಥಿರಾಂಕ string& filename) { m_readFiles.ಸೇರಿಸು(filename); }

string VlcOptions::ಆವೃತ್ತಿ() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_ಗಮ್ಯ_Str(DTVERSION_rev);
    ಹಿಂತಿರುಗಿ ver;
}

ಶೂನ್ಯ VlcOptions::parseReportOption() {
    ಆದರೆ (m_report.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
    string::ಗಾತ್ರ_type ಆರಂಭ = 0;
    ಆಗ (ನಿಜ) {
        ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type comma = m_report.ಕಂಡುಹಿಡಿ(',', ಆರಂಭ);
        ಸ್ಥಿರಾಂಕ string ಬಗೆ = m_report.substr(ಆರಂಭ, comma - ಆರಂಭ);
        ಆದರೆ (ಬಗೆ == "summary") {
            m_reportSummary = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಗೆ == "hier" || ಬಗೆ == "hierarchy") {
            m_reportHierarchy = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            v3fatal("Invalid --report option: " << m_report);
        }
        ಆದರೆ (comma == string::npos) ನಿಲ್ಲಿಸು;
        ಆರಂಭ = comma + 1;
    }
}

ಶೂನ್ಯ VlcOptions::parseOptsList(ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ಟ್ಯಾಗುಗಳು;

    DECL_OPTION("-annotate", ಗಣ_2, &m_annotate_ಹೊರಹೋಗು);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", ಗಣ_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](ಪೂರ್ಣಾಂಕ v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", ಗಣ_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", ಗಣ_2, &m_report_ಮಟ್ಟಗಳು);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", ಗಣ_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_ಆವೃತ್ತಿ(ನಿಜ);
        std::ನಿರ್ಗಮಿಸು(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_ಆವೃತ್ತಿ(ಸುಳ್ಳು);
        std::ನಿರ್ಗಮಿಸು(0);
    });
    DECL_OPTION("-write", ಗಣ_2, &m_writeFile);
    DECL_OPTION("-write-info", ಗಣ_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ಆದರೆ (argv[i][0] == '-') {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ consumed = parser.parse(i, argc, argv).ಮೊದಲ) {
                i += consumed;
            } ಇಲ್ಲದಿದ್ದರೆ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

ಶೂನ್ಯ VlcOptions::show_ಆವೃತ್ತಿ(ತಾರ್ಕಿಕ verbose) {
    std::ಹೊರಗೆ << ಆವೃತ್ತಿ();
    std::ಹೊರಗೆ << "\n";
    ಆದರೆ (!verbose) ಹಿಂತಿರುಗಿ;

    std::ಹೊರಗೆ << "\n";
    std::ಹೊರಗೆ << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::ಹೊರಗೆ << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::ಹೊರಗೆ << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::ಹೊರಗೆ << "License Version 2.0.\n";

    std::ಹೊರಗೆ << "\n";
    std::ಹೊರಗೆ << "See https://verilator.org for documentation\n";
}

//######################################################################

ಪೂರ್ಣಾಂಕ ಮುಖ್ಯ(ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_ತುದಿ ತುದಿ;

    // Command option parsing
    ತುದಿ.opt.parseOptsList(argc - 1, argv + 1);

    ಆದರೆ (ತುದಿ.opt.readFiles().ಖಾಲಿಗಳು()) ತುದಿ.opt.ಕೂಡಿಸು_Read_File("vlt_coverage.dat");

    {
        ಸ್ಥಿರಾಂಕ Vl_String_ಗಣ& readFiles = ತುದಿ.opt.readFiles();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& filename : readFiles) ತುದಿ.readCoverage(filename);
    }

    ಆದರೆ (debug() >= 9) {
        ತುದಿ.tests().dump(ನಿಜ);
        ತುದಿ.points().dump();
    }

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ defaultReport = !ತುದಿ.opt.reportSpecified() && !ತುದಿ.opt.rank()
                               && ತುದಿ.opt.writeFile().ಖಾಲಿಗಳು() && ತುದಿ.opt.writeInfoFile().ಖಾಲಿಗಳು();
    ಆದರೆ (ತುದಿ.opt.reportSpecified()) {
        ಆದರೆ (ತುದಿ.opt.reportSummary()) ತುದಿ.printTypeSummary();
        ಆದರೆ (ತುದಿ.opt.reportHierarchy()) ತುದಿ.printHierarchyReport();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (defaultReport) {
        ತುದಿ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ಆದರೆ (!ತುದಿ.opt.annotate_ಹೊರಹೋಗು().ಖಾಲಿಗಳು()) ತುದಿ.annotate(ತುದಿ.opt.annotate_ಹೊರಹೋಗು());

    ಆದರೆ (ತುದಿ.opt.rank()) {
        ತುದಿ.rank();
        ತುದಿ.tests().dump(ಸುಳ್ಳು);
    }

    ಆದರೆ (!ತುದಿ.opt.writeFile().ಖಾಲಿಗಳು() || !ತುದಿ.opt.writeInfoFile().ಖಾಲಿಗಳು()) {
        ಆದರೆ (!ತುದಿ.opt.writeFile().ಖಾಲಿಗಳು()) ತುದಿ.writeCoverage(ತುದಿ.opt.writeFile());
        ಆದರೆ (!ತುದಿ.opt.writeInfoFile().ಖಾಲಿಗಳು()) ತುದಿ.writeInfo(ತುದಿ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ಆದರೆ (ತುದಿ.opt.unlink()) {
            ಸ್ಥಿರಾಂಕ Vl_String_ಗಣ& readFiles = ತುದಿ.opt.readFiles();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
