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

فراغ V3Waiver::addEntry(V3ErrorCode errorCode, ثابت std::string& filename, ثابت std::string& msg)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    𐤀𐤌 (filename == V3Options::getStdPackagePath()) أرجع;
    𐤀𐤌 (filename == V3Options::getStdWaiverPath()) أرجع;
    ثابت V3LockGuard lock{s_mutex};

    string trimmsg = msg;
    𐤀𐤌 (!v3Global.opt.waiverMultiline()) {
        ثابت size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        𐤀𐤌 (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        لكل (ثابت حرف* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            طالما (*cp == ' ' || isdigit(*cp)) ++cp;
            𐤀𐤌 (*cp == '|') ++cp;
            // ^~~~~
            طالما (*cp == ' ' || *cp == '^') ++cp;
            طالما (*cp == '~') ++cp;
            طالما (*cp && *cp != '\n') result += *cp++;
            طالما (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        لكل (ثابت حرف& c : trimmsg) {
            𐤀𐤌 (c == '*' || !std::isprint(c)) {
                add = "*";
            } وإلا 𐤀𐤌 (c == ' ') {
                𐤀𐤌 (add != "*") add += c;
            } وإلا {
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
    s_waiverList.push_back(entry.str());
}

فراغ V3Waiver::اكتب(ثابت std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    𐤀𐤌 (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ثابت V3LockGuard lock{s_mutex};

    𐤀𐤌 (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    لكل (ثابت auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
