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

#समावेश "verilatedos.h"

#समावेश "V3Waiver.h"

#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3Options.h"

#समावेश <memory>
#समावेश <sstream>

व्योम V3Waiver::addEntry(V3ErrorCode errorCode, स्थिर std::string& filename, स्थिर std::string& संदेश)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    अगर (filename == V3Options::getStdPackagePath()) वापस;
    अगर (filename == V3Options::getStdWaiverPath()) वापस;
    स्थिर V3LockGuard lock{s_mutex};

    string trimmsg = संदेश;
    अगर (!v3Global.opt.waiverMultiline()) {
        स्थिर माप_प्रकार pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        अगर (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string परिणाम;
        क्रम (स्थिर अक्षर* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            जबतक (*cp == ' ' || है_अंक(*cp)) ++cp;
            अगर (*cp == '|') ++cp;
            // ^~~~~
            जबतक (*cp == ' ' || *cp == '^') ++cp;
            जबतक (*cp == '~') ++cp;
            जबतक (*cp && *cp != '\n') परिणाम += *cp++;
            जबतक (*cp == '\n') परिणाम += *cp++;
        }
        trimmsg = परिणाम;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string परिणाम;
        string add;
        परिणाम.reserve(trimmsg.आकार());
        क्रम (स्थिर अक्षर& c : trimmsg) {
            अगर (c == '*' || !std::है_छाप(c)) {
                add = "*";
            } अन्यथा अगर (c == ' ') {
                अगर (add != "*") add += c;
            } अन्यथा {
                परिणाम += add + c;
                add = "";
            }
        }
        परिणाम += add;
        trimmsg = परिणाम;
    }

    std::stringstream entry;
    entry << "lint_off -rule " << errorCode.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.धकेलो_back(entry.str());
}

व्योम V3Waiver::ग_लिखो(स्थिर std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    स्थिर std::unique_ptr<std::बख_प्रवाह> ofp{V3File::new_ofstream(filename)};
    अगर (ofp->हार()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    स्थिर V3LockGuard lock{s_mutex};

    अगर (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    क्रम (स्थिर स्वतः& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
