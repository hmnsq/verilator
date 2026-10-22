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
#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

#ضمن "config_build.h"
#جے_بنا_تعریف HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#جے_انت
// clang-format on

#ضمن "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
#تعریف_کرو V3ERROR_NO_GLOBAL_
#جے_انت
#ضمن "V3Error.h"
ساكن صحيح debug() { أرجع V3Error::debugDefault(); }
#ضمن "V3Error.cpp"
#ضمن "V3String.cpp"
#تعریف_کرو V3OPTION_PARSER_NO_VOPTION_BOOL
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

فراغ VlcOptions::جوڑو_Read_File(ثابت string& filename) { m_readFiles.داخل_کرو(filename); }

string VlcOptions::نسخہ() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_منزل_Str(DTVERSION_rev);
    أرجع ver;
}

فراغ VlcOptions::parseReportOption() {
    إذا (m_report.خالی_تھاواں()) أرجع;
    string::ناپ_type شروع = 0;
    طالما (سچ) {
        ثابت string::ناپ_type comma = m_report.لبھو(',', شروع);
        ثابت string قسم = m_report.substr(شروع, comma - شروع);
        إذا (قسم == "summary") {
            m_reportSummary = سچ;
        } وإلا إذا (قسم == "hier" || قسم == "hierarchy") {
            m_reportHierarchy = سچ;
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
    V3_OPTION_PARSER_DECL_ٹیگ_سوچی;

    DECL_OPTION("-annotate", سیٹ_2, &m_annotate_باہرلا);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", سیٹ_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحيح v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", سیٹ_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", سیٹ_2, &m_report_پدھراں);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", سیٹ_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_نسخہ(سچ);
        std::باہر_نکلو(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_نسخہ(خطأ);
        std::باہر_نکلو(0);
    });
    DECL_OPTION("-write", سیٹ_2, &m_writeFile);
    DECL_OPTION("-write-info", سیٹ_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        إذا (argv[i][0] == '-') {
            إذا (ثابت صحيح consumed = parser.parse(i, argc, argv).پہلا) {
                i += consumed;
            } وإلا {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } وإلا {
            جوڑو_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

فراغ VlcOptions::show_نسخہ(منطقي verbose) {
    std::باہر << نسخہ();
    std::باہر << "\n";
    إذا (!verbose) أرجع;

    std::باہر << "\n";
    std::باہر << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::باہر << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::باہر << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::باہر << "License Version 2.0.\n";

    std::باہر << "\n";
    std::باہر << "See https://verilator.org for documentation\n";
}

//######################################################################

صحيح مکھ(صحيح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_چوٹی چوٹی;

    // Command option parsing
    چوٹی.opt.parseOptsList(argc - 1, argv + 1);

    إذا (چوٹی.opt.readFiles().خالی_تھاواں()) چوٹی.opt.جوڑو_Read_File("vlt_coverage.dat");

    {
        ثابت Vl_String_سیٹ& readFiles = چوٹی.opt.readFiles();
        لكل (ثابت خودکار& filename : readFiles) چوٹی.readCoverage(filename);
    }

    إذا (debug() >= 9) {
        چوٹی.tests().dump(سچ);
        چوٹی.points().dump();
    }

    ثابت منطقي defaultReport = !چوٹی.opt.reportSpecified() && !چوٹی.opt.rank()
                               && چوٹی.opt.writeFile().خالی_تھاواں() && چوٹی.opt.writeInfoFile().خالی_تھاواں();
    إذا (چوٹی.opt.reportSpecified()) {
        إذا (چوٹی.opt.reportSummary()) چوٹی.printTypeSummary();
        إذا (چوٹی.opt.reportHierarchy()) چوٹی.printHierarchyReport();
    } وإلا إذا (defaultReport) {
        چوٹی.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    إذا (!چوٹی.opt.annotate_باہرلا().خالی_تھاواں()) چوٹی.annotate(چوٹی.opt.annotate_باہرلا());

    إذا (چوٹی.opt.rank()) {
        چوٹی.rank();
        چوٹی.tests().dump(خطأ);
    }

    إذا (!چوٹی.opt.writeFile().خالی_تھاواں() || !چوٹی.opt.writeInfoFile().خالی_تھاواں()) {
        إذا (!چوٹی.opt.writeFile().خالی_تھاواں()) چوٹی.writeCoverage(چوٹی.opt.writeFile());
        إذا (!چوٹی.opt.writeInfoFile().خالی_تھاواں()) چوٹی.writeInfo(چوٹی.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        إذا (چوٹی.opt.unlink()) {
            ثابت Vl_String_سیٹ& readFiles = چوٹی.opt.readFiles();
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
