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
#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

#ضمن "config_build.h"
#جيڪڏهن_وصف_نه_ٿيل HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#جيڪڏهن_پورو
// clang-format on

#ضمن "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
#وصف_ڪريو V3ERROR_NO_GLOBAL_
#جيڪڏهن_پورو
#ضمن "V3Error.h"
ساكن صحيح debug() { أرجع V3Error::debugDefault(); }
#ضمن "V3Error.cpp"
#ضمن "V3String.cpp"
#وصف_ڪريو V3OPTION_PARSER_NO_VOPTION_BOOL
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

فراغ VlcOptions::جوڙيو_Read_File(ثابت string& filename) { m_readFiles.وجهو(filename); }

string VlcOptions::نسخو() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_منزل_Str(DTVERSION_rev);
    أرجع ver;
}

فراغ VlcOptions::parseReportOption() {
    إذا (m_report.خالي_جايون()) أرجع;
    string::سائيز_type شروعات = 0;
    طالما (سچ) {
        ثابت string::سائيز_type comma = m_report.ڳوليو(',', شروعات);
        ثابت string قسم = m_report.substr(شروعات, comma - شروعات);
        إذا (قسم == "summary") {
            m_reportSummary = سچ;
        } وإلا إذا (قسم == "hier" || قسم == "hierarchy") {
            m_reportHierarchy = سچ;
        } وإلا {
            v3fatal("Invalid --report option: " << m_report);
        }
        إذا (comma == string::npos) اكسر;
        شروعات = comma + 1;
    }
}

فراغ VlcOptions::parseOptsList(صحيح argc, حرف** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ٽيگ_فهرست;

    DECL_OPTION("-annotate", سيٽ_2, &m_annotate_ٻاهريون);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", سيٽ_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](صحيح v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", سيٽ_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", سيٽ_2, &m_report_سطحون);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", سيٽ_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_نسخو(سچ);
        std::ٻاهر_نڪرو(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_نسخو(خطأ);
        std::ٻاهر_نڪرو(0);
    });
    DECL_OPTION("-write", سيٽ_2, &m_writeFile);
    DECL_OPTION("-write-info", سيٽ_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        إذا (argv[i][0] == '-') {
            إذا (ثابت صحيح consumed = parser.parse(i, argc, argv).پهريون) {
                i += consumed;
            } وإلا {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } وإلا {
            جوڙيو_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

فراغ VlcOptions::show_نسخو(منطقي verbose) {
    std::ٻاهر << نسخو();
    std::ٻاهر << "\n";
    إذا (!verbose) أرجع;

    std::ٻاهر << "\n";
    std::ٻاهر << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::ٻاهر << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::ٻاهر << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::ٻاهر << "License Version 2.0.\n";

    std::ٻاهر << "\n";
    std::ٻاهر << "See https://verilator.org for documentation\n";
}

//######################################################################

صحيح مکيه(صحيح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_چوٽي چوٽي;

    // Command option parsing
    چوٽي.opt.parseOptsList(argc - 1, argv + 1);

    إذا (چوٽي.opt.readFiles().خالي_جايون()) چوٽي.opt.جوڙيو_Read_File("vlt_coverage.dat");

    {
        ثابت Vl_String_سيٽ& readFiles = چوٽي.opt.readFiles();
        لكل (ثابت خودڪار& filename : readFiles) چوٽي.readCoverage(filename);
    }

    إذا (debug() >= 9) {
        چوٽي.tests().dump(سچ);
        چوٽي.points().dump();
    }

    ثابت منطقي defaultReport = !چوٽي.opt.reportSpecified() && !چوٽي.opt.rank()
                               && چوٽي.opt.writeFile().خالي_جايون() && چوٽي.opt.writeInfoFile().خالي_جايون();
    إذا (چوٽي.opt.reportSpecified()) {
        إذا (چوٽي.opt.reportSummary()) چوٽي.printTypeSummary();
        إذا (چوٽي.opt.reportHierarchy()) چوٽي.printHierarchyReport();
    } وإلا إذا (defaultReport) {
        چوٽي.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    إذا (!چوٽي.opt.annotate_ٻاهريون().خالي_جايون()) چوٽي.annotate(چوٽي.opt.annotate_ٻاهريون());

    إذا (چوٽي.opt.rank()) {
        چوٽي.rank();
        چوٽي.tests().dump(خطأ);
    }

    إذا (!چوٽي.opt.writeFile().خالي_جايون() || !چوٽي.opt.writeInfoFile().خالي_جايون()) {
        إذا (!چوٽي.opt.writeFile().خالي_جايون()) چوٽي.writeCoverage(چوٽي.opt.writeFile());
        إذا (!چوٽي.opt.writeInfoFile().خالي_جايون()) چوٽي.writeInfo(چوٽي.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        إذا (چوٽي.opt.unlink()) {
            ثابت Vl_String_سيٽ& readFiles = چوٽي.opt.readFiles();
            لكل (ثابت خودڪار& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
