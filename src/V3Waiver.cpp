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

#شامل "verilatedos.h"

#شامل "V3Waiver.h"

#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Options.h"

#شامل <memory>
#شامل <sstream>

تهی V3Waiver::addEntry(V3ErrorCode errorCode, ثابت std::string& filename, ثابت std::string& پیام)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    اگر (filename == V3Options::getStdPackagePath()) بازگشت;
    اگر (filename == V3Options::getStdWaiverPath()) بازگشت;
    ثابت V3LockGuard lock{s_mutex};

    string trimmsg = پیام;
    اگر (!v3Global.opt.waiverMultiline()) {
        ثابت size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        اگر (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        برای (ثابت نویسه* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            وقتی (*cp == ' ' || isdigit(*cp)) ++cp;
            اگر (*cp == '|') ++cp;
            // ^~~~~
            وقتی (*cp == ' ' || *cp == '^') ++cp;
            وقتی (*cp == '~') ++cp;
            وقتی (*cp && *cp != '\n') result += *cp++;
            وقتی (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        برای (ثابت نویسه& c : trimmsg) {
            اگر (c == '*' || !std::isprint(c)) {
                add = "*";
            } وگرنه اگر (c == ' ') {
                اگر (add != "*") add += c;
            } وگرنه {
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
    s_waiverList.افزودن_back(entry.str());
}

تهی V3Waiver::اكتب(ثابت std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ثابت V3LockGuard lock{s_mutex};

    اگر (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    برای (ثابت auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
