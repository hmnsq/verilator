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

خالی V3Waiver::جوڑو_اندراج(V3_Error_رمز error_رمز, ثابت std::string& filename, ثابت std::string& پیغام_2)
    VL_MT_محفوظ_EXCLUDES(s_mutex) {
    اگر (filename == V3Options::get_Std_Package_راستہ()) واپس;
    اگر (filename == V3Options::get_Std_Waiver_راستہ()) واپس;
    ثابت V3_تالا_Guard تالا_2{s_mutex};

    string trimmsg = پیغام_2;
    اگر (!v3Global.opt.waiverMultiline()) {
        ثابت سائز_t pos = trimmsg.ڈھونڈو('\n');
        trimmsg.resize(pos);
        اگر (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string نتیجہ;
        برائے (ثابت حرف* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            جب (*cp == ' ' || ہندسہ_ہے(*cp)) ++cp;
            اگر (*cp == '|') ++cp;
            // ^~~~~
            جب (*cp == ' ' || *cp == '^') ++cp;
            جب (*cp == '~') ++cp;
            جب (*cp && *cp != '\n') نتیجہ += *cp++;
            جب (*cp == '\n') نتیجہ += *cp++;
        }
        trimmsg = نتیجہ;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string نتیجہ;
        string جوڑو;
        نتیجہ.reserve(trimmsg.سائز());
        برائے (ثابت حرف& c : trimmsg) {
            اگر (c == '*' || !std::isprint(c)) {
                جوڑو = "*";
            } ورنہ اگر (c == ' ') {
                اگر (جوڑو != "*") جوڑو += c;
            } ورنہ {
                نتیجہ += جوڑو + c;
                جوڑو = "";
            }
        }
        نتیجہ += جوڑو;
        trimmsg = نتیجہ;
    }

    std::stringstream اندراج_2;
    اندراج_2 << "lint_off -rule " << error_رمز.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ڈالو_عقب(اندراج_2.str());
}

خالی V3Waiver::اكتب(ثابت std::string& filename) VL_MT_محفوظ_EXCLUDES(s_mutex) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ثابت V3_تالا_Guard تالا_2{s_mutex};

    اگر (s_waiverList.خالی_خانے()) *ofp << "// No waivers needed - great!\n";

    برائے (ثابت خودکار& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
