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

ಶೂನ್ಯ V3Waiver::addEntry(V3ErrorCode errorCode, ಧ್ರುವ std::string& filename, ಧ್ರುವ std::string& ಸಂದೇಶ)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    ಯದಿ (filename == V3Options::getStdPackagePath()) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (filename == V3Options::getStdWaiverPath()) ಪ್ರತ್ಯಾಗಮ;
    ಧ್ರುವ V3LockGuard lock{s_mutex};

    string trimmsg = ಸಂದೇಶ;
    ಯದಿ (!v3Global.opt.waiverMultiline()) {
        ಧ್ರುವ size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        ಯದಿ (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            ಯಾವತ್ (*cp == ' ' || isdigit(*cp)) ++cp;
            ಯದಿ (*cp == '|') ++cp;
            // ^~~~~
            ಯಾವತ್ (*cp == ' ' || *cp == '^') ++cp;
            ಯಾವತ್ (*cp == '~') ++cp;
            ಯಾವತ್ (*cp && *cp != '\n') result += *cp++;
            ಯಾವತ್ (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ& c : trimmsg) {
            ಯದಿ (c == '*' || !std::isprint(c)) {
                add = "*";
            } ಅನ್ಯಥಾ ಯದಿ (c == ' ') {
                ಯದಿ (add != "*") add += c;
            } ಅನ್ಯಥಾ {
                result += add + c;
                add = "";
            }
        }
        result += add;
        trimmsg = result;
    }

    std::stringstream entry;
    entry << "lint_off -rule " << errorCode.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ತಳ್ಳು_back(entry.str());
}

ಶೂನ್ಯ V3Waiver::ಲಿಖ(ಧ್ರುವ std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    ಧ್ರುವ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ಯದಿ (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ಧ್ರುವ V3LockGuard lock{s_mutex};

    ಯದಿ (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    ಪ್ರತಿ (ಧ್ರುವ auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
