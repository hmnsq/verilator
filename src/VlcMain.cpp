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
#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "config_build.h"
#اگر_غیر_تعریف_شدہ HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#اگر_ختم
// clang-format on

#شامل "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
#تعریف_کرو V3ERROR_NO_GLOBAL_
#اگر_ختم
#شامل "V3Error.h"
ساکن صحیح debug() { واپس V3Error::debugDefault(); }
#شامل "V3Error.cpp"
#شامل "V3String.cpp"
#تعریف_کرو V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#شامل "V3OptionParser.cpp"
#شامل "V3Os.cpp"
#شامل "VlcTop.cpp"
// clang-format on

#شامل "VlcOptions.h"
#شامل "VlcTop.h"

#شامل <algorithm>
#شامل <fstream>

//######################################################################
// VlcOptions

خالی VlcOptions::جوڑو_Read_File(ثابت string& filename) { m_readFiles.داخل_کرو(filename); }

string VlcOptions::نسخہ() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_مقصد_Str(DTVERSION_rev);
    واپس ver;
}

خالی VlcOptions::parseReportOption() {
    اگر (m_report.خالی_خانے()) واپس;
    string::سائز_type آغاز = 0;
    جب (سچ) {
        ثابت string::سائز_type comma = m_report.ڈھونڈو(',', آغاز);
        ثابت string قسم = m_report.substr(آغاز, comma - آغاز);
        اگر (قسم == "summary") {
            m_reportSummary = سچ;
        } ورنہ اگر (قسم == "hier" || قسم == "hierarchy") {
            m_reportHierarchy = سچ;
        } ورنہ {
            v3fatal("Invalid --report option: " << m_report);
        }
        اگر (comma == string::npos) توڑو;
        آغاز = comma + 1;
    }
}

خالی VlcOptions::parseOptsList(صحیح argc, حرف** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_لیبل_فہرست;

    DECL_OPTION("-annotate", سیٹ_2, &m_annotate_برآمد);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", سیٹ_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحیح v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", سیٹ_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", سیٹ_2, &m_report_سطحیں);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", سیٹ_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_نسخہ(سچ);
        std::باہر_نکلو(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_نسخہ(جھوٹ);
        std::باہر_نکلو(0);
    });
    DECL_OPTION("-write", سیٹ_2, &m_writeFile);
    DECL_OPTION("-write-info", سیٹ_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    برائے (صحیح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        اگر (argv[i][0] == '-') {
            اگر (ثابت صحیح consumed = parser.parse(i, argc, argv).پہلا) {
                i += consumed;
            } ورنہ {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } ورنہ {
            جوڑو_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

خالی VlcOptions::show_نسخہ(منطقی verbose) {
    std::خارج << نسخہ();
    std::خارج << "\n";
    اگر (!verbose) واپس;

    std::خارج << "\n";
    std::خارج << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::خارج << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::خارج << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::خارج << "License Version 2.0.\n";

    std::خارج << "\n";
    std::خارج << "See https://verilator.org for documentation\n";
}

//######################################################################

صحیح مین(صحیح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_چوٹی چوٹی;

    // Command option parsing
    چوٹی.opt.parseOptsList(argc - 1, argv + 1);

    اگر (چوٹی.opt.readFiles().خالی_خانے()) چوٹی.opt.جوڑو_Read_File("vlt_coverage.dat");

    {
        ثابت Vl_String_سیٹ& readFiles = چوٹی.opt.readFiles();
        برائے (ثابت خودکار& filename : readFiles) چوٹی.readCoverage(filename);
    }

    اگر (debug() >= 9) {
        چوٹی.tests().dump(سچ);
        چوٹی.points().dump();
    }

    ثابت منطقی defaultReport = !چوٹی.opt.reportSpecified() && !چوٹی.opt.rank()
                               && چوٹی.opt.writeFile().خالی_خانے() && چوٹی.opt.writeInfoFile().خالی_خانے();
    اگر (چوٹی.opt.reportSpecified()) {
        اگر (چوٹی.opt.reportSummary()) چوٹی.printTypeSummary();
        اگر (چوٹی.opt.reportHierarchy()) چوٹی.printHierarchyReport();
    } ورنہ اگر (defaultReport) {
        چوٹی.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    اگر (!چوٹی.opt.annotate_برآمد().خالی_خانے()) چوٹی.annotate(چوٹی.opt.annotate_برآمد());

    اگر (چوٹی.opt.rank()) {
        چوٹی.rank();
        چوٹی.tests().dump(جھوٹ);
    }

    اگر (!چوٹی.opt.writeFile().خالی_خانے() || !چوٹی.opt.writeInfoFile().خالی_خانے()) {
        اگر (!چوٹی.opt.writeFile().خالی_خانے()) چوٹی.writeCoverage(چوٹی.opt.writeFile());
        اگر (!چوٹی.opt.writeInfoFile().خالی_خانے()) چوٹی.writeInfo(چوٹی.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        اگر (چوٹی.opt.unlink()) {
            ثابت Vl_String_سیٹ& readFiles = چوٹی.opt.readFiles();
            برائے (ثابت خودکار& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
