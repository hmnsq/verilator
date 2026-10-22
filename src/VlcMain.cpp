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
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#ਜੇ_ਅੰਤ
// clang-format on

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3ERROR_NO_GLOBAL_
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
ਸਥਿਰ ਪੂਰਨ debug() { ਵਾਪਸ V3Error::debugDefault(); }
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.cpp"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.cpp"
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OptionParser.cpp"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.cpp"
#ਅਨ੍ਤਰ੍ਭਾਵ "VlcTop.cpp"
// clang-format on

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcOptions.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "VlcTop.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>

//######################################################################
// VlcOptions

ਖਾਲੀ VlcOptions::ਜੋੜੋ_Read_File(ਅਚਲ string& filename) { m_readFiles.ਪਾਓ(filename); }

string VlcOptions::ਸੰਸਕਰਣ() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_ਮੰਜ਼ਿਲ_Str(DTVERSION_rev);
    ਵਾਪਸ ver;
}

ਖਾਲੀ VlcOptions::parseReportOption() {
    ਜੇ (m_report.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
    string::ਮਾਪ_type ਸ਼ੁਰੂ = 0;
    ਜਦੋਂ (ਸੱਚ) {
        ਅਚਲ string::ਮਾਪ_type comma = m_report.ਲੱਭੋ(',', ਸ਼ੁਰੂ);
        ਅਚਲ string ਕਿਸਮ = m_report.substr(ਸ਼ੁਰੂ, comma - ਸ਼ੁਰੂ);
        ਜੇ (ਕਿਸਮ == "summary") {
            m_reportSummary = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਕਿਸਮ == "hier" || ਕਿਸਮ == "hierarchy") {
            m_reportHierarchy = ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            v3fatal("Invalid --report option: " << m_report);
        }
        ਜੇ (comma == string::npos) ਰੋਕੋ;
        ਸ਼ੁਰੂ = comma + 1;
    }
}

ਖਾਲੀ VlcOptions::parseOptsList(ਪੂਰਨ argc, ਅੱਖਰ** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ਟੈਗ_ਸੂਚੀ;

    DECL_OPTION("-annotate", ਸੈੱਟ_2, &m_annotate_ਬਾਹਰਲਾ);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", ਸੈੱਟ_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](ਪੂਰਨ v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", ਸੈੱਟ_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", ਸੈੱਟ_2, &m_report_ਪੱਧਰ);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", ਸੈੱਟ_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_ਸੰਸਕਰਣ(ਸੱਚ);
        std::ਬਾਹਰ_ਨਿਕਲੋ(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_ਸੰਸਕਰਣ(ਝੂਠ);
        std::ਬਾਹਰ_ਨਿਕਲੋ(0);
    });
    DECL_OPTION("-write", ਸੈੱਟ_2, &m_writeFile);
    DECL_OPTION("-write-info", ਸੈੱਟ_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    ਲਈ (ਪੂਰਨ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ਜੇ (argv[i][0] == '-') {
            ਜੇ (ਅਚਲ ਪੂਰਨ consumed = parser.parse(i, argc, argv).ਪਹਿਲਾ) {
                i += consumed;
            } ਨਹੀਂਤਾਂ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

ਖਾਲੀ VlcOptions::show_ਸੰਸਕਰਣ(ਤਾਰਕਿਕ verbose) {
    std::ਬਾਹਰ << ਸੰਸਕਰਣ();
    std::ਬਾਹਰ << "\n";
    ਜੇ (!verbose) ਵਾਪਸ;

    std::ਬਾਹਰ << "\n";
    std::ਬਾਹਰ << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::ਬਾਹਰ << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::ਬਾਹਰ << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::ਬਾਹਰ << "License Version 2.0.\n";

    std::ਬਾਹਰ << "\n";
    std::ਬਾਹਰ << "See https://verilator.org for documentation\n";
}

//######################################################################

ਪੂਰਨ ਮੁੱਖ(ਪੂਰਨ argc, ਅੱਖਰ** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_ਸਿਖਰ ਸਿਖਰ;

    // Command option parsing
    ਸਿਖਰ.opt.parseOptsList(argc - 1, argv + 1);

    ਜੇ (ਸਿਖਰ.opt.readFiles().ਖਾਲੀ_ਥਾਵਾਂ()) ਸਿਖਰ.opt.ਜੋੜੋ_Read_File("vlt_coverage.dat");

    {
        ਅਚਲ Vl_String_ਸੈੱਟ& readFiles = ਸਿਖਰ.opt.readFiles();
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& filename : readFiles) ਸਿਖਰ.readCoverage(filename);
    }

    ਜੇ (debug() >= 9) {
        ਸਿਖਰ.tests().dump(ਸੱਚ);
        ਸਿਖਰ.points().dump();
    }

    ਅਚਲ ਤਾਰਕਿਕ defaultReport = !ਸਿਖਰ.opt.reportSpecified() && !ਸਿਖਰ.opt.rank()
                               && ਸਿਖਰ.opt.writeFile().ਖਾਲੀ_ਥਾਵਾਂ() && ਸਿਖਰ.opt.writeInfoFile().ਖਾਲੀ_ਥਾਵਾਂ();
    ਜੇ (ਸਿਖਰ.opt.reportSpecified()) {
        ਜੇ (ਸਿਖਰ.opt.reportSummary()) ਸਿਖਰ.printTypeSummary();
        ਜੇ (ਸਿਖਰ.opt.reportHierarchy()) ਸਿਖਰ.printHierarchyReport();
    } ਨਹੀਂਤਾਂ ਜੇ (defaultReport) {
        ਸਿਖਰ.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ਜੇ (!ਸਿਖਰ.opt.annotate_ਬਾਹਰਲਾ().ਖਾਲੀ_ਥਾਵਾਂ()) ਸਿਖਰ.annotate(ਸਿਖਰ.opt.annotate_ਬਾਹਰਲਾ());

    ਜੇ (ਸਿਖਰ.opt.rank()) {
        ਸਿਖਰ.rank();
        ਸਿਖਰ.tests().dump(ਝੂਠ);
    }

    ਜੇ (!ਸਿਖਰ.opt.writeFile().ਖਾਲੀ_ਥਾਵਾਂ() || !ਸਿਖਰ.opt.writeInfoFile().ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (!ਸਿਖਰ.opt.writeFile().ਖਾਲੀ_ਥਾਵਾਂ()) ਸਿਖਰ.writeCoverage(ਸਿਖਰ.opt.writeFile());
        ਜੇ (!ਸਿਖਰ.opt.writeInfoFile().ਖਾਲੀ_ਥਾਵਾਂ()) ਸਿਖਰ.writeInfo(ਸਿਖਰ.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ਜੇ (ਸਿਖਰ.opt.unlink()) {
            ਅਚਲ Vl_String_ਸੈੱਟ& readFiles = ਸਿਖਰ.opt.readFiles();
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
