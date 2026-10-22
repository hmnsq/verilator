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

#include "verilatedos.h"

#include "V3Waiver.h"

#include "V3File.h"
#include "V3Global.h"
#include "V3Options.h"

#include <memory>
#include <sstream>

void V3Waiver::addEntry(V3ErrorCode errorCode, const std::string& filename, const std::string& msg)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    ꦪꦼꦤ꧀ (filename == V3Options::getStdPackagePath()) ꦮꦭꦶ;
    ꦪꦼꦤ꧀ (filename == V3Options::getStdWaiverPath()) ꦮꦭꦶ;
    const V3LockGuard lock{s_mutex};

    string trimmsg = msg;
    ꦪꦼꦤ꧀ (!v3Global.opt.waiverMultiline()) {
        const size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        ꦪꦼꦤ꧀ (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        ꦏꦁꦒꦺꦴ (const var_na* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            ꦱꦼꦢꦼꦭ (*cp == ' ' || isdigit(*cp)) ++cp;
            ꦪꦼꦤ꧀ (*cp == '|') ++cp;
            // ^~~~~
            ꦱꦼꦢꦼꦭ (*cp == ' ' || *cp == '^') ++cp;
            ꦱꦼꦢꦼꦭ (*cp == '~') ++cp;
            ꦱꦼꦢꦼꦭ (*cp && *cp != '\n') result += *cp++;
            ꦱꦼꦢꦼꦭ (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        ꦏꦁꦒꦺꦴ (const var_na& c : trimmsg) {
            ꦪꦼꦤ꧀ (c == '*' || !std::isprint(c)) {
                add = "*";
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (c == ' ') {
                ꦪꦼꦤ꧀ (add != "*") add += c;
            } ꦭꦶꦪ {
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

void V3Waiver::write(const std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    const std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ꦪꦼꦤ꧀ (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    const V3LockGuard lock{s_mutex};

    ꦪꦼꦤ꧀ (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    ꦏꦁꦒꦺꦴ (const auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
