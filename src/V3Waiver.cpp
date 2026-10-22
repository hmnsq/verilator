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

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Waiver.h"

#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Options.h"

#अन्तर्भाव <memory>
#अन्तर्भाव <sstream>

शून्य V3Waiver::addEntry(V3ErrorCode errorCode, स्थिरांक std::string& filename, स्थिरांक std::string& संदेश)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    जर (filename == V3Options::getStdPackagePath()) परत;
    जर (filename == V3Options::getStdWaiverPath()) परत;
    स्थिरांक V3LockGuard lock{s_mutex};

    string trimmsg = संदेश;
    जर (!v3Global.opt.waiverMultiline()) {
        स्थिरांक माप_प्रकार pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        जर (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        साठी (स्थिरांक अक्षर* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            पर्यंत (*cp == ' ' || है_अंक(*cp)) ++cp;
            जर (*cp == '|') ++cp;
            // ^~~~~
            पर्यंत (*cp == ' ' || *cp == '^') ++cp;
            पर्यंत (*cp == '~') ++cp;
            पर्यंत (*cp && *cp != '\n') result += *cp++;
            पर्यंत (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        साठी (स्थिरांक अक्षर& c : trimmsg) {
            जर (c == '*' || !std::है_छाप(c)) {
                add = "*";
            } अन्यथा जर (c == ' ') {
                जर (add != "*") add += c;
            } अन्यथा {
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
    s_waiverList.ढकला_back(entry.str());
}

शून्य V3Waiver::लिख(स्थिरांक std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    स्थिरांक std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    जर (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    स्थिरांक V3LockGuard lock{s_mutex};

    जर (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    साठी (स्थिरांक स्वतः& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
