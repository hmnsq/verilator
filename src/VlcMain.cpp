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
#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "config_build.h"
#که_نه_تعریف_شوی HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#د_که_پای
// clang-format on

#ضمن "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
#تعریف_کړه V3ERROR_NO_GLOBAL_
#د_که_پای
#ضمن "V3Error.h"
ساكن صحيح debug() { أرجع V3Error::debugDefault(); }
#ضمن "V3Error.cpp"
#ضمن "V3String.cpp"
#تعریف_کړه V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#ضمن "V3OptionParser.cpp"
#ضمن "V3Os.cpp"
#ضمن "VlcTop.cpp"
// clang-format on

#ضمن "VlcOptions.h"
#ضمن "VlcTop.h"

#ضمن <algorithm>
#ضمن <fstream>

//######################################################################
// VlcOptions

فراغ VlcOptions::جمع_کړه_Read_File(ثابت string& filename) { m_readFiles.ورننباسه(filename); }

string VlcOptions::نسخه() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_منزل_Str(DTVERSION_rev);
    أرجع ver;
}

فراغ VlcOptions::parseReportOption() {
    إذا (m_report.تشه()) أرجع;
    string::اندازه_type پیل = 0;
    طالما (ریښتیا) {
        ثابت string::اندازه_type comma = m_report.پیدا_کړه(',', پیل);
        ثابت string ډول = m_report.substr(پیل, comma - پیل);
        إذا (ډول == "summary") {
            m_reportSummary = ریښتیا;
        } وإلا إذا (ډول == "hier" || ډول == "hierarchy") {
            m_reportHierarchy = ریښتیا;
        } وإلا {
            v3fatal("Invalid --report option: " << m_report);
        }
        إذا (comma == string::npos) اكسر;
        پیل = comma + 1;
    }
}

فراغ VlcOptions::parseOptsList(صحيح argc, حرف** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_نښانونه;

    DECL_OPTION("-annotate", سېټ_2, &m_annotate_بهر);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", سېټ_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحيح v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", سېټ_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", سېټ_2, &m_report_پوړونه);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", سېټ_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_نسخه(ریښتیا);
        std::وتل(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_نسخه(خطأ);
        std::وتل(0);
    });
    DECL_OPTION("-write", سېټ_2, &m_writeFile);
    DECL_OPTION("-write-info", سېټ_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        إذا (argv[i][0] == '-') {
            إذا (ثابت صحيح consumed = parser.parse(i, argc, argv).لومړی) {
                i += consumed;
            } وإلا {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } وإلا {
            جمع_کړه_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

فراغ VlcOptions::show_نسخه(منطقي verbose) {
    std::وتنه << نسخه();
    std::وتنه << "\n";
    إذا (!verbose) أرجع;

    std::وتنه << "\n";
    std::وتنه << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::وتنه << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::وتنه << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::وتنه << "License Version 2.0.\n";

    std::وتنه << "\n";
    std::وتنه << "See https://verilator.org for documentation\n";
}

//######################################################################

صحيح اصلي(صحيح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_سر سر;

    // Command option parsing
    سر.opt.parseOptsList(argc - 1, argv + 1);

    إذا (سر.opt.readFiles().تشه()) سر.opt.جمع_کړه_Read_File("vlt_coverage.dat");

    {
        ثابت Vl_String_سېټ& readFiles = سر.opt.readFiles();
        لكل (ثابت خپلکاری& filename : readFiles) سر.readCoverage(filename);
    }

    إذا (debug() >= 9) {
        سر.tests().dump(ریښتیا);
        سر.points().dump();
    }

    ثابت منطقي defaultReport = !سر.opt.reportSpecified() && !سر.opt.rank()
                               && سر.opt.writeFile().تشه() && سر.opt.writeInfoFile().تشه();
    إذا (سر.opt.reportSpecified()) {
        إذا (سر.opt.reportSummary()) سر.printTypeSummary();
        إذا (سر.opt.reportHierarchy()) سر.printHierarchyReport();
    } وإلا إذا (defaultReport) {
        سر.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    إذا (!سر.opt.annotate_بهر().تشه()) سر.annotate(سر.opt.annotate_بهر());

    إذا (سر.opt.rank()) {
        سر.rank();
        سر.tests().dump(خطأ);
    }

    إذا (!سر.opt.writeFile().تشه() || !سر.opt.writeInfoFile().تشه()) {
        إذا (!سر.opt.writeFile().تشه()) سر.writeCoverage(سر.opt.writeFile());
        إذا (!سر.opt.writeInfoFile().تشه()) سر.writeInfo(سر.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        إذا (سر.opt.unlink()) {
            ثابت Vl_String_سېټ& readFiles = سر.opt.readFiles();
            لكل (ثابت خپلکاری& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
