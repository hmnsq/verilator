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

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Waiver.h"

#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Options.h"

#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <sstream>

શૂન્ય V3Waiver::addEntry(V3ErrorCode errorCode, અચલ std::string& filename, અચલ std::string& સંદેશ)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    જો (filename == V3Options::getStdPackagePath()) પાછા;
    જો (filename == V3Options::getStdWaiverPath()) પાછા;
    અચલ V3LockGuard lock{s_mutex};

    string trimmsg = સંદેશ;
    જો (!v3Global.opt.waiverMultiline()) {
        અચલ size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        જો (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        માટે (અચલ અક્ષર* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            જ્યારે (*cp == ' ' || isdigit(*cp)) ++cp;
            જો (*cp == '|') ++cp;
            // ^~~~~
            જ્યારે (*cp == ' ' || *cp == '^') ++cp;
            જ્યારે (*cp == '~') ++cp;
            જ્યારે (*cp && *cp != '\n') result += *cp++;
            જ્યારે (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        માટે (અચલ અક્ષર& c : trimmsg) {
            જો (c == '*' || !std::isprint(c)) {
                add = "*";
            } નહીંતર જો (c == ' ') {
                જો (add != "*") add += c;
            } નહીંતર {
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
    s_waiverList.ધકેલો_back(entry.str());
}

શૂન્ય V3Waiver::લિખ(અચલ std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    અચલ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    જો (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    અચલ V3LockGuard lock{s_mutex};

    જો (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    માટે (અચલ auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
