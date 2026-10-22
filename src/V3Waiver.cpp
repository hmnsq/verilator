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

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Waiver.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Options.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>

ਖਾਲੀ V3Waiver::ਜੋੜੋ_ਇੰਦਰਾਜ(V3_Error_ਕੋਡ error_ਕੋਡ, ਅਚਲ std::string& filename, ਅਚਲ std::string& ਸੁਨੇਹਾ_2)
    VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s_mutex) {
    ਜੇ (filename == V3Options::get_Std_Package_ਰਸਤਾ()) ਵਾਪਸ;
    ਜੇ (filename == V3Options::get_Std_Waiver_ਰਸਤਾ()) ਵਾਪਸ;
    ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_mutex};

    string trimmsg = ਸੁਨੇਹਾ_2;
    ਜੇ (!v3Global.opt.waiverMultiline()) {
        ਅਚਲ ਮਾਪ_t pos = trimmsg.ਲੱਭੋ('\n');
        trimmsg.resize(pos);
        ਜੇ (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string ਨਤੀਜਾ;
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            ਜਦੋਂ (*cp == ' ' || ਅੰਕ_ਹੈ(*cp)) ++cp;
            ਜੇ (*cp == '|') ++cp;
            // ^~~~~
            ਜਦੋਂ (*cp == ' ' || *cp == '^') ++cp;
            ਜਦੋਂ (*cp == '~') ++cp;
            ਜਦੋਂ (*cp && *cp != '\n') ਨਤੀਜਾ += *cp++;
            ਜਦੋਂ (*cp == '\n') ਨਤੀਜਾ += *cp++;
        }
        trimmsg = ਨਤੀਜਾ;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string ਨਤੀਜਾ;
        string ਜੋੜੋ;
        ਨਤੀਜਾ.reserve(trimmsg.ਮਾਪ());
        ਲਈ (ਅਚਲ ਅੱਖਰ& c : trimmsg) {
            ਜੇ (c == '*' || !std::isprint(c)) {
                ਜੋੜੋ = "*";
            } ਨਹੀਂਤਾਂ ਜੇ (c == ' ') {
                ਜੇ (ਜੋੜੋ != "*") ਜੋੜੋ += c;
            } ਨਹੀਂਤਾਂ {
                ਨਤੀਜਾ += ਜੋੜੋ + c;
                ਜੋੜੋ = "";
            }
        }
        ਨਤੀਜਾ += ਜੋੜੋ;
        trimmsg = ਨਤੀਜਾ;
    }

    std::stringstream ਇੰਦਰਾਜ;
    ਇੰਦਰਾਜ << "lint_off -rule " << error_ਕੋਡ.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.ਧੱਕੋ_ਪਿੱਠ(ਇੰਦਰਾਜ.str());
}

ਖਾਲੀ V3Waiver::ਲਿਖ(ਅਚਲ std::string& filename) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s_mutex) {
    ਅਚਲ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ਜੇ (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_mutex};

    ਜੇ (s_waiverList.ਖਾਲੀ_ਥਾਵਾਂ()) *ofp << "// No waivers needed - great!\n";

    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
