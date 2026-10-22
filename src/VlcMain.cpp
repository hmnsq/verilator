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
#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

#ضمن "config_build.h"
#اگر_تعریف_نشده HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#پایان_اگر
// clang-format on

#ضمن "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
#تعریف_کن V3ERROR_NO_GLOBAL_
#پایان_اگر
#ضمن "V3Error.h"
ساكن صحيح debug() { أرجع V3Error::debugDefault(); }
#ضمن "V3Error.cpp"
#ضمن "V3String.cpp"
#تعریف_کن V3OPTION_PARSER_NO_VOPTION_BOOL
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

فراغ VlcOptions::جمع_کن_Read_File(ثابت string& filename) { m_readFiles.درج(filename); }

string VlcOptions::نسخه() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_مقصد_Str(DTVERSION_rev);
    أرجع ver;
}

فراغ VlcOptions::parseReportOption() {
    إذا (m_report.خالی()) أرجع;
    string::اندازه_type شروع = 0;
    طالما (درست) {
        ثابت string::اندازه_type comma = m_report.پیدا_کن(',', شروع);
        ثابت string نوع = m_report.substr(شروع, comma - شروع);
        إذا (نوع == "summary") {
            m_reportSummary = درست;
        } وإلا إذا (نوع == "hier" || نوع == "hierarchy") {
            m_reportHierarchy = درست;
        } وإلا {
            v3fatal("Invalid --report option: " << m_report);
        }
        إذا (comma == string::npos) اكسر;
        شروع = comma + 1;
    }
}

فراغ VlcOptions::parseOptsList(صحيح argc, حرف** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_برچسبها;

    DECL_OPTION("-annotate", مجموعه_2, &m_annotate_بیرون);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", مجموعه_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحيح v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", مجموعه_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", مجموعه_2, &m_report_سطوح);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", مجموعه_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_نسخه(درست);
        std::خروج(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_نسخه(خطأ);
        std::خروج(0);
    });
    DECL_OPTION("-write", مجموعه_2, &m_writeFile);
    DECL_OPTION("-write-info", مجموعه_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        إذا (argv[i][0] == '-') {
            إذا (ثابت صحيح consumed = parser.parse(i, argc, argv).اولی) {
                i += consumed;
            } وإلا {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } وإلا {
            جمع_کن_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

فراغ VlcOptions::show_نسخه(منطقي verbose) {
    std::خروجی << نسخه();
    std::خروجی << "\n";
    إذا (!verbose) أرجع;

    std::خروجی << "\n";
    std::خروجی << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::خروجی << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::خروجی << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::خروجی << "License Version 2.0.\n";

    std::خروجی << "\n";
    std::خروجی << "See https://verilator.org for documentation\n";
}

//######################################################################

صحيح اصلی(صحيح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_بالاترین بالاترین;

    // Command option parsing
    بالاترین.opt.parseOptsList(argc - 1, argv + 1);

    إذا (بالاترین.opt.readFiles().خالی()) بالاترین.opt.جمع_کن_Read_File("vlt_coverage.dat");

    {
        ثابت Vl_String_مجموعه& readFiles = بالاترین.opt.readFiles();
        لكل (ثابت خودکار& filename : readFiles) بالاترین.readCoverage(filename);
    }

    إذا (debug() >= 9) {
        بالاترین.tests().dump(درست);
        بالاترین.points().dump();
    }

    ثابت منطقي defaultReport = !بالاترین.opt.reportSpecified() && !بالاترین.opt.rank()
                               && بالاترین.opt.writeFile().خالی() && بالاترین.opt.writeInfoFile().خالی();
    إذا (بالاترین.opt.reportSpecified()) {
        إذا (بالاترین.opt.reportSummary()) بالاترین.printTypeSummary();
        إذا (بالاترین.opt.reportHierarchy()) بالاترین.printHierarchyReport();
    } وإلا إذا (defaultReport) {
        بالاترین.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    إذا (!بالاترین.opt.annotate_بیرون().خالی()) بالاترین.annotate(بالاترین.opt.annotate_بیرون());

    إذا (بالاترین.opt.rank()) {
        بالاترین.rank();
        بالاترین.tests().dump(خطأ);
    }

    إذا (!بالاترین.opt.writeFile().خالی() || !بالاترین.opt.writeInfoFile().خالی()) {
        إذا (!بالاترین.opt.writeFile().خالی()) بالاترین.writeCoverage(بالاترین.opt.writeFile());
        إذا (!بالاترین.opt.writeInfoFile().خالی()) بالاترین.writeInfo(بالاترین.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        إذا (بالاترین.opt.unlink()) {
            ثابت Vl_String_مجموعه& readFiles = بالاترین.opt.readFiles();
            لكل (ثابت خودکار& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
