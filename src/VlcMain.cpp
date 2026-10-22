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
#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

#अन्तर्भाव "config_build.h"
#यदि_अपरिभाषितम् HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#यदि_अन्तः
// clang-format on

#अन्तर्भाव "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
#परिभाषय V3ERROR_NO_GLOBAL_
#यदि_अन्तः
#अन्तर्भाव "V3Error.h"
स्थिर पूर्णाङ्क debug() { प्रत्यागम V3Error::debugDefault(); }
#अन्तर्भाव "V3Error.cpp"
#अन्तर्भाव "V3String.cpp"
#परिभाषय V3OPTION_PARSER_NO_VOPTION_BOOL
// clang-format off
#अन्तर्भाव "V3OptionParser.cpp"
#अन्तर्भाव "V3Os.cpp"
#अन्तर्भाव "VlcTop.cpp"
// clang-format on

#अन्तर्भाव "VlcOptions.h"
#अन्तर्भाव "VlcTop.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <fstream>

//######################################################################
// VlcOptions

शून्य VlcOptions::सङ्कलय_Read_File(ध्रुव string& filename) { m_readFiles.निवेशय(filename); }

string VlcOptions::संस्करणम्() {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_गन्तव्यम्_Str(DTVERSION_rev);
    प्रत्यागम ver;
}

शून्य VlcOptions::parseReportOption() {
    यदि (m_report.रिक्तानि()) प्रत्यागम;
    string::आकार_type start = 0;
    यावत् (सत्य) {
        ध्रुव string::आकार_type comma = m_report.find(',', start);
        ध्रुव string विधा = m_report.substr(start, comma - start);
        यदि (विधा == "summary") {
            m_reportSummary = सत्य;
        } अन्यथा यदि (विधा == "hier" || विधा == "hierarchy") {
            m_reportHierarchy = सत्य;
        } अन्यथा {
            v3fatal("Invalid --report option: " << m_report);
        }
        यदि (comma == string::npos) भङ्ग;
        start = comma + 1;
    }
}

शून्य VlcOptions::parseOptsList(पूर्णाङ्क argc, वर्ण** argv) {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_चिह्नकानि;

    DECL_OPTION("-annotate", समुच्चयः_2, &m_annotate_बहिर्गमनम्);
    DECL_OPTION("-annotate-all", OnOff, &m_annotateAll);
    DECL_OPTION("-annotate-min", समुच्चयः_2, &m_annotateMin);
    DECL_OPTION("-annotate-points", OnOff, &m_annotatePoints);
    DECL_OPTION("-debug", CbCall, []() { V3Error::debugDefault(3); });
    DECL_OPTION("-debugi", CbVal, [](पूर्णाङ्क v) { V3Error::debugDefault(v); });
    DECL_OPTION("-filter-type", समुच्चयः_2, &m_filterType);
    DECL_OPTION("-include-reset-arcs", OnOff, &m_includeResetArcs);
    DECL_OPTION("-levels", समुच्चयः_2, &m_report_स्तराः);
    DECL_OPTION("-rank", OnOff, &m_rank);
    DECL_OPTION("-report", समुच्चयः_2, &m_report);
    DECL_OPTION("-unlink", OnOff, &m_unlink);
    DECL_OPTION("-V", CbCall, []() {
        show_संस्करणम्(सत्य);
        std::निर्गच्छ(0);
    });
    DECL_OPTION("-version", CbCall, []() {
        show_संस्करणम्(असत्य);
        std::निर्गच्छ(0);
    });
    DECL_OPTION("-write", समुच्चयः_2, &m_writeFile);
    DECL_OPTION("-write-info", समुच्चयः_2, &m_writeInfoFile);
    parser.finalize();

    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.
    प्रति (पूर्णाङ्क i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);
        यदि (argv[i][0] == '-') {
            यदि (ध्रुव पूर्णाङ्क consumed = parser.parse(i, argc, argv).first) {
                i += consumed;
            } अन्यथा {
                v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                ++i;  // LCOV_EXCL_LINE
            }
        } अन्यथा {
            सङ्कलय_Read_File(argv[i]);
            ++i;
        }
    }
    parseReportOption();
}

शून्य VlcOptions::show_संस्करणम्(तार्किक verbose) {
    std::निर्गमः << संस्करणम्();
    std::निर्गमः << "\n";
    यदि (!verbose) प्रत्यागम;

    std::निर्गमः << "\n";
    std::निर्गमः << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    std::निर्गमः << "redistribute it and/or modify the Verilator internals under the terms of\n";
    std::निर्गमः << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    std::निर्गमः << "License Version 2.0.\n";

    std::निर्गमः << "\n";
    std::निर्गमः << "See https://verilator.org for documentation\n";
}

//######################################################################

पूर्णाङ्क मुख्यम्(पूर्णाङ्क argc, वर्ण** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Vlc_शिखरम् शिखरम्;

    // Command option parsing
    शिखरम्.opt.parseOptsList(argc - 1, argv + 1);

    यदि (शिखरम्.opt.readFiles().रिक्तानि()) शिखरम्.opt.सङ्कलय_Read_File("vlt_coverage.dat");

    {
        ध्रुव Vl_String_समुच्चयः& readFiles = शिखरम्.opt.readFiles();
        प्रति (ध्रुव स्वचालित& filename : readFiles) शिखरम्.readCoverage(filename);
    }

    यदि (debug() >= 9) {
        शिखरम्.tests().dump(सत्य);
        शिखरम्.points().dump();
    }

    ध्रुव तार्किक defaultReport = !शिखरम्.opt.reportSpecified() && !शिखरम्.opt.rank()
                               && शिखरम्.opt.writeFile().रिक्तानि() && शिखरम्.opt.writeInfoFile().रिक्तानि();
    यदि (शिखरम्.opt.reportSpecified()) {
        यदि (शिखरम्.opt.reportSummary()) शिखरम्.printTypeSummary();
        यदि (शिखरम्.opt.reportHierarchy()) शिखरम्.printHierarchyReport();
    } अन्यथा यदि (defaultReport) {
        शिखरम्.printTypeSummary();
    }

    V3Error::abortIfWarnings();
    यदि (!शिखरम्.opt.annotate_बहिर्गमनम्().रिक्तानि()) शिखरम्.annotate(शिखरम्.opt.annotate_बहिर्गमनम्());

    यदि (शिखरम्.opt.rank()) {
        शिखरम्.rank();
        शिखरम्.tests().dump(असत्य);
    }

    यदि (!शिखरम्.opt.writeFile().रिक्तानि() || !शिखरम्.opt.writeInfoFile().रिक्तानि()) {
        यदि (!शिखरम्.opt.writeFile().रिक्तानि()) शिखरम्.writeCoverage(शिखरम्.opt.writeFile());
        यदि (!शिखरम्.opt.writeInfoFile().रिक्तानि()) शिखरम्.writeInfo(शिखरम्.opt.writeInfoFile());
        V3Error::abortIfWarnings();
        यदि (शिखरम्.opt.unlink()) {
            ध्रुव Vl_String_समुच्चयः& readFiles = शिखरम्.opt.readFiles();
            प्रति (ध्रुव स्वचालित& filename : readFiles) unlink(filename.c_str());
        }
    }

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    UINFO(1, "Done, Exiting...");
}

// Local Variables:
// compile-command: "v4make bin/verilator_coverage --debugi 9 test_regress/t/t_vlcov_data_*.dat"
// End:
