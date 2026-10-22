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

فراغ V3Waiver::جمع_کن_مدخل(V3_Error_کد error_کد, ثابت std::string& filename, ثابت std::string& پیام_2)
    VL_MT_امن_EXCLUDES(s_mutex) {
    إذا (filename == V3Options::get_Std_Package_مسیر()) أرجع;
    إذا (filename == V3Options::get_Std_Waiver_مسیر()) أرجع;
    ثابت V3_قفل_Guard قفل_2{s_mutex};

    string trimmsg = پیام_2;
    إذا (!v3Global.opt.waiverMultiline()) {
        ثابت اندازه_t pos = trimmsg.پیدا_کن('\n');
        trimmsg.resize(pos);
        إذا (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string نتیجه;
        لكل (ثابت حرف* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            طالما (*cp == ' ' || آیا_رقم(*cp)) ++cp;
            إذا (*cp == '|') ++cp;
            // ^~~~~
            طالما (*cp == ' ' || *cp == '^') ++cp;
            طالما (*cp == '~') ++cp;
            طالما (*cp && *cp != '\n') نتیجه += *cp++;
            طالما (*cp == '\n') نتیجه += *cp++;
        }
        trimmsg = نتیجه;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string نتیجه;
        string جمع_کن;
        نتیجه.reserve(trimmsg.اندازه());
        لكل (ثابت حرف& c : trimmsg) {
            إذا (c == '*' || !std::isprint(c)) {
                جمع_کن = "*";
            } وإلا إذا (c == ' ') {
                إذا (جمع_کن != "*") جمع_کن += c;
            } وإلا {
                نتیجه += جمع_کن + c;
                جمع_کن = "";
            }
        }
        نتیجه += جمع_کن;
        trimmsg = نتیجه;
    }

    std::stringstream مدخل;
    مدخل << "lint_off -rule " << error_کد.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.افزودن_پشت(مدخل.str());
}

فراغ V3Waiver::اكتب(ثابت std::string& filename) VL_MT_امن_EXCLUDES(s_mutex) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ثابت V3_قفل_Guard قفل_2{s_mutex};

    إذا (s_waiverList.خالی()) *ofp << "// No waivers needed - great!\n";

    لكل (ثابت خودکار& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
