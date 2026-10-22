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
#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#నిర్వచితం_కాకపోతే HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#అయితే_ముగింపు
// clang-format on

#అన్తర్భావ "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
#నిర్వచించు V3ERROR_NO_GLOBAL_
#అయితే_ముగింపు
#అన్తర్భావ "V3Error.h"
స్థిర పూర్ణాంకం debug() { తిరిగి V3Error::debugDefault(); }
#అన్తర్భావ "V3Error.cpp"
#అన్తర్భావ "V3String.cpp"
#నిర్వచించు V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#అన్తర్భావ "V3OptionParser.cpp"
#అన్తర్భావ "V3Os.cpp"
#అన్తర్భావ "VlcTop.cpp"
// clang-format on

#అన్తర్భావ "VlcOptions.h"
#అన్తర్భావ "VlcTop.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <fstream>

//######################################################################
// VlcOptions

శూన్య VlcOptions::కూడు_Read_File(స్థిరరాశి string& filename) { m_readFiles.చొప్పించు(filename); }

string VlcOptions::సంస్కరణ() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_గమ్యం_Str(DTVERSION_rev);
    తిరిగి ver;
}

శూన్య VlcOptions::parseReportOption() {
    ఒకవేళ (m_report.ఖాళీలు()) తిరిగి;
    string::కొలత_type ప్రారంభం = 0;
    అయితే (నిజం) {
        స్థిరరాశి string::కొలత_type comma = m_report.కనుగొను(',', ప్రారంభం);
        స్థిరరాశి string రకం = m_report.substr(ప్రారంభం, comma - ప్రారంభం);
        ఒకవేళ (రకం == "summary") {
            m_reportSummary = నిజం;
        } లేకపోతే ఒకవేళ (రకం == "hier" || రకం == "hierarchy") {
            m_reportHierarchy = నిజం;
        } లేకపోతే {
            v3fatal("Invalid --report option: " << m_report);
        }
        ఒకవేళ (comma == string::npos) ఆపు;
        ప్రారంభం = comma + 1;
    }
}

శూన్య VlcOptions::parseOptsList(పూర్ణాంకం argc, అక్షరం** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ట్యాగులు;

    DECL_OPTION("-annotate", సమితి_2, &m_annotate_బహిర్గతం);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", సమితి_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](పూర్ణాంకం v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", సమితి_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", సమితి_2, &m_report_స్థాయిలు);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", సమితి_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_సంస్కరణ(నిజం);
        std::నిష్క్రమించు(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_సంస్కరణ(అబద్ధం);
        std::నిష్క్రమించు(0);
    });
    DECL_OPTION("-write", సమితి_2, &m_writeFile);
    DECL_OPTION("-write-info", సమితి_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    కొరకు (పూర్ణాంకం i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        ఒకవేళ (argv[i][0] == '-') {
            ఒకవేళ (స్థిరరాశి పూర్ణాంకం consumed = parser.parse(i, argc, argv).మొదటి) {
                i += consumed;
            } లేకపోతే {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } లేకపోతే {
            కూడు_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

శూన్య VlcOptions::show_సంస్కరణ(తార్కిక verbose) {
    std::బయటకు << సంస్కరణ();
    std::బయటకు << "\n";
    ఒకవేళ (!verbose) తిరిగి;

    std::బయటకు << "\n";
    std::బయటకు << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::బయటకు << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::బయటకు << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::బయటకు << "License Version 2.0.\n";

    std::బయటకు << "\n";
    std::బయటకు << "See https://verilator.org for documentation\n";
}

//######################################################################

పూర్ణాంకం ప్రధాన(పూర్ణాంకం argc, అక్షరం** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_శిఖరం శిఖరం;

    // Command option parsing
    శిఖరం.opt.parseOptsList(argc - 1, argv + 1);

    ఒకవేళ (శిఖరం.opt.readFiles().ఖాళీలు()) శిఖరం.opt.కూడు_Read_File("vlt_coverage.dat");

    {
        స్థిరరాశి Vl_String_సమితి& readFiles = శిఖరం.opt.readFiles();
        కొరకు (స్థిరరాశి స్వయంచాలక& filename : readFiles) శిఖరం.readCoverage(filename);
    }

    ఒకవేళ (debug() >= 9) {
        శిఖరం.tests().dump(నిజం);
        శిఖరం.points().dump();
    }

    స్థిరరాశి తార్కిక defaultReport = !శిఖరం.opt.reportSpecified() && !శిఖరం.opt.rank()
                               && శిఖరం.opt.writeFile().ఖాళీలు() && శిఖరం.opt.writeInfoFile().ఖాళీలు();
    ఒకవేళ (శిఖరం.opt.reportSpecified()) {
        ఒకవేళ (శిఖరం.opt.reportSummary()) శిఖరం.printTypeSummary();
        ఒకవేళ (శిఖరం.opt.reportHierarchy()) శిఖరం.printHierarchyReport();
    } లేకపోతే ఒకవేళ (defaultReport) {
        శిఖరం.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    ఒకవేళ (!శిఖరం.opt.annotate_బహిర్గతం().ఖాళీలు()) శిఖరం.annotate(శిఖరం.opt.annotate_బహిర్గతం());

    ఒకవేళ (శిఖరం.opt.rank()) {
        శిఖరం.rank();
        శిఖరం.tests().dump(అబద్ధం);
    }

    ఒకవేళ (!శిఖరం.opt.writeFile().ఖాళీలు() || !శిఖరం.opt.writeInfoFile().ఖాళీలు()) {
        ఒకవేళ (!శిఖరం.opt.writeFile().ఖాళీలు()) శిఖరం.writeCoverage(శిఖరం.opt.writeFile());
        ఒకవేళ (!శిఖరం.opt.writeInfoFile().ఖాళీలు()) శిఖరం.writeInfo(శిఖరం.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        ఒకవేళ (శిఖరం.opt.unlink()) {
            స్థిరరాశి Vl_String_సమితి& readFiles = శిఖరం.opt.readFiles();
            కొరకు (స్థిరరాశి స్వయంచాలక& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
