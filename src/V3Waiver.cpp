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

#ضمن "verilatedos.h"

#ضمن "V3Waiver.h"

#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Options.h"

#ضمن <memory>
#ضمن <sstream>

فراغ V3Waiver::جوڙيو_داخلا_2(V3_Error_ڪوڊ error_ڪوڊ, ثابت std::string& filename, ثابت std::string& نياپو)
    VL_MT_محفوظ_EXCLUDES(s_mutex) {
    إذا (filename == V3Options::get_Std_Package_رستو()) أرجع;
    إذا (filename == V3Options::get_Std_Waiver_رستو()) أرجع;
    ثابت V3_تالو_Guard تالو_2{s_mutex};

    string trimmsg = نياپو;
    إذا (!v3Global.opt.waiverMultiline()) {
        ثابت سائيز_t pos = trimmsg.ڳوليو('\n');
        trimmsg.resize(pos);
        إذا (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string نتيجو;
        لكل (ثابت حرف* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            طالما (*cp == ' ' || انگ_آهي(*cp)) ++cp;
            إذا (*cp == '|') ++cp;
            // ^~~~~
            طالما (*cp == ' ' || *cp == '^') ++cp;
            طالما (*cp == '~') ++cp;
            طالما (*cp && *cp != '\n') نتيجو += *cp++;
            طالما (*cp == '\n') نتيجو += *cp++;
        }
        trimmsg = نتيجو;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string نتيجو;
        string جوڙيو;
        نتيجو.reserve(trimmsg.سائيز());
        لكل (ثابت حرف& c : trimmsg) {
            إذا (c == '*' || !std::isprint(c)) {
                جوڙيو = "*";
            } وإلا إذا (c == ' ') {
                إذا (جوڙيو != "*") جوڙيو += c;
            } وإلا {
                نتيجو += جوڙيو + c;
                جوڙيو = "";
            }
        }
        نتيجو += جوڙيو;
        trimmsg = نتيجو;
    }

    std::stringstream داخلا_2;
    داخلا_2 << "lint_off -rule " << error_ڪوڊ.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ڌڪو_پٺ(داخلا_2.str());
}

فراغ V3Waiver::اكتب(ثابت std::string& filename) VL_MT_محفوظ_EXCLUDES(s_mutex) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ثابت V3_تالو_Guard تالو_2{s_mutex};

    إذا (s_waiverList.خالي_جايون()) *ofp << "// No waivers needed - great!\n";

    لكل (ثابت خودڪار& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
