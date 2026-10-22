// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit waivers into a config file
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2020-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Waiver.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <sstream>

ಶೂನ್ಯ V3Waiver::ಕೂಡಿಸು_ನಮೂದು(V3_Error_ಸಂಕೇತ error_ಸಂಕೇತ, ಸ್ಥಿರಾಂಕ std::string& filename, ಸ್ಥಿರಾಂಕ std::string& ಸಂದೇಶ_2)
    VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s_mutex) {
    ಆದರೆ (filename == V3Options::get_Std_Package_ಮಾರ್ಗ()) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (filename == V3Options::get_Std_Waiver_ಮಾರ್ಗ()) ಹಿಂತಿರುಗಿ;
    ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_mutex};

    string trimmsg = ಸಂದೇಶ_2;
    ಆದರೆ (!v3Global.opt.waiverMultiline()) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pos = trimmsg.ಕಂಡುಹಿಡಿ('\n');
        trimmsg.resize(pos);
        ಆದರೆ (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string ಫಲಿತಾಂಶ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            ಆಗ (*cp == ' ' || ಅಂಕಿಯೇ(*cp)) ++cp;
            ಆದರೆ (*cp == '|') ++cp;
            // ^~~~~
            ಆಗ (*cp == ' ' || *cp == '^') ++cp;
            ಆಗ (*cp == '~') ++cp;
            ಆಗ (*cp && *cp != '\n') ಫಲಿತಾಂಶ += *cp++;
            ಆಗ (*cp == '\n') ಫಲಿತಾಂಶ += *cp++;
        }
        trimmsg = ಫಲಿತಾಂಶ;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string ಫಲಿತಾಂಶ;
        string ಕೂಡಿಸು;
        ಫಲಿತಾಂಶ.reserve(trimmsg.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ& c : trimmsg) {
            ಆದರೆ (c == '*' || !std::isprint(c)) {
                ಕೂಡಿಸು = "*";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == ' ') {
                ಆದರೆ (ಕೂಡಿಸು != "*") ಕೂಡಿಸು += c;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಫಲಿತಾಂಶ += ಕೂಡಿಸು + c;
                ಕೂಡಿಸು = "";
            }
        }
        ಫಲಿತಾಂಶ += ಕೂಡಿಸು;
        trimmsg = ಫಲಿತಾಂಶ;
    }

    std::stringstream ನಮೂದು;
    ನಮೂದು << "lint_off -rule " << error_ಸಂಕೇತ.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ತಳ್ಳು_ಹಿಂಭಾಗ(ನಮೂದು.str());
}

ಶೂನ್ಯ V3Waiver::ಲಿಖ(ಸ್ಥಿರಾಂಕ std::string& filename) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s_mutex) {
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ಆದರೆ (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_mutex};

    ಆದರೆ (s_waiverList.ಖಾಲಿಗಳು()) *ofp << "// No waivers needed - great!\n";

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
