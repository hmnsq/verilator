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

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Waiver.h"

#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Options.h"

#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <sstream>

ശൂന്യം V3Waiver::addEntry(V3ErrorCode errorCode, സ്ഥിരം std::string& filename, സ്ഥിരം std::string& സന്ദേശം)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    എങ്കിൽ (filename == V3Options::getStdPackagePath()) തിരികെ;
    എങ്കിൽ (filename == V3Options::getStdWaiverPath()) തിരികെ;
    സ്ഥിരം V3LockGuard lock{s_mutex};

    string trimmsg = സന്ദേശം;
    എങ്കിൽ (!v3Global.opt.waiverMultiline()) {
        സ്ഥിരം size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        എങ്കിൽ (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            ആകുമ്പോൾ (*cp == ' ' || isdigit(*cp)) ++cp;
            എങ്കിൽ (*cp == '|') ++cp;
            // ^~~~~
            ആകുമ്പോൾ (*cp == ' ' || *cp == '^') ++cp;
            ആകുമ്പോൾ (*cp == '~') ++cp;
            ആകുമ്പോൾ (*cp && *cp != '\n') result += *cp++;
            ആകുമ്പോൾ (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        വേണ്ടി (സ്ഥിരം അക്ഷരം& c : trimmsg) {
            എങ്കിൽ (c == '*' || !std::isprint(c)) {
                add = "*";
            } അല്ലെങ്കിൽ എങ്കിൽ (c == ' ') {
                എങ്കിൽ (add != "*") add += c;
            } അല്ലെങ്കിൽ {
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
    s_waiverList.തള്ളുക_back(entry.str());
}

ശൂന്യം V3Waiver::ലിഖ(സ്ഥിരം std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    സ്ഥിരം std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    എങ്കിൽ (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    സ്ഥിരം V3LockGuard lock{s_mutex};

    എങ്കിൽ (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    വേണ്ടി (സ്ഥിരം auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
