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

فراغ V3Waiver::جوڑو_اندراج(V3_Error_کوڈ error_کوڈ, ثابت std::string& filename, ثابت std::string& پیغام)
    VL_MT_محفوظ_EXCLUDES(s_mutex) {
    إذا (filename == V3Options::get_Std_Package_رستہ()) أرجع;
    إذا (filename == V3Options::get_Std_Waiver_رستہ()) أرجع;
    ثابت V3_جندرا_Guard جندرا_2{s_mutex};

    string trimmsg = پیغام;
    إذا (!v3Global.opt.waiverMultiline()) {
        ثابت ناپ_t pos = trimmsg.لبھو('\n');
        trimmsg.resize(pos);
        إذا (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string نتیجہ;
        لكل (ثابت حرف* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            طالما (*cp == ' ' || ہندسہ_اے(*cp)) ++cp;
            إذا (*cp == '|') ++cp;
            // ^~~~~
            طالما (*cp == ' ' || *cp == '^') ++cp;
            طالما (*cp == '~') ++cp;
            طالما (*cp && *cp != '\n') نتیجہ += *cp++;
            طالما (*cp == '\n') نتیجہ += *cp++;
        }
        trimmsg = نتیجہ;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string نتیجہ;
        string جوڑو;
        نتیجہ.reserve(trimmsg.ناپ());
        لكل (ثابت حرف& c : trimmsg) {
            إذا (c == '*' || !std::isprint(c)) {
                جوڑو = "*";
            } وإلا إذا (c == ' ') {
                إذا (جوڑو != "*") جوڑو += c;
            } وإلا {
                نتیجہ += جوڑو + c;
                جوڑو = "";
            }
        }
        نتیجہ += جوڑو;
        trimmsg = نتیجہ;
    }

    std::stringstream اندراج;
    اندراج << "lint_off -rule " << error_کوڈ.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ڈالو_پٹھ(اندراج.str());
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

    ثابت V3_جندرا_Guard جندرا_2{s_mutex};

    إذا (s_waiverList.خالی_تھاواں()) *ofp << "// No waivers needed - great!\n";

    لكل (ثابت خودکار& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
