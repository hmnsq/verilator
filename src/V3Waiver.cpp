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

فراغ V3Waiver::جمع_کړه_ثبت(V3_Error_کوډ error_کوډ, ثابت std::string& filename, ثابت std::string& پیغام_2)
    VL_MT_خوندي_EXCLUDES(s_mutex) {
    إذا (filename == V3Options::get_Std_Package_لار()) أرجع;
    إذا (filename == V3Options::get_Std_Waiver_لار()) أرجع;
    ثابت V3_قلف_Guard قلف_2{s_mutex};

    string trimmsg = پیغام_2;
    إذا (!v3Global.opt.waiverMultiline()) {
        ثابت اندازه_t pos = trimmsg.پیدا_کړه('\n');
        trimmsg.resize(pos);
        إذا (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string پایله;
        لكل (ثابت حرف* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            طالما (*cp == ' ' || که_عدد(*cp)) ++cp;
            إذا (*cp == '|') ++cp;
            // ^~~~~
            طالما (*cp == ' ' || *cp == '^') ++cp;
            طالما (*cp == '~') ++cp;
            طالما (*cp && *cp != '\n') پایله += *cp++;
            طالما (*cp == '\n') پایله += *cp++;
        }
        trimmsg = پایله;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string پایله;
        string جمع_کړه;
        پایله.reserve(trimmsg.اندازه());
        لكل (ثابت حرف& c : trimmsg) {
            إذا (c == '*' || !std::isprint(c)) {
                جمع_کړه = "*";
            } وإلا إذا (c == ' ') {
                إذا (جمع_کړه != "*") جمع_کړه += c;
            } وإلا {
                پایله += جمع_کړه + c;
                جمع_کړه = "";
            }
        }
        پایله += جمع_کړه;
        trimmsg = پایله;
    }

    std::stringstream ثبت;
    ثبت << "lint_off -rule " << error_کوډ.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ټېل_شا(ثبت.str());
}

فراغ V3Waiver::اكتب(ثابت std::string& filename) VL_MT_خوندي_EXCLUDES(s_mutex) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ثابت V3_قلف_Guard قلف_2{s_mutex};

    إذا (s_waiverList.تشه()) *ofp << "// No waivers needed - great!\n";

    لكل (ثابت خپلکاری& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
