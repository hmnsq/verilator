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

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Waiver.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Options.h"

#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <sstream>

ଶୂନ୍ୟ V3Waiver::addEntry(V3ErrorCode errorCode, ଅଚଳ std::string& filename, ଅଚଳ std::string& ସନ୍ଦେଶ)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    ଯଦି (filename == V3Options::getStdPackagePath()) ଫେରନ୍ତୁ;
    ଯଦି (filename == V3Options::getStdWaiverPath()) ଫେରନ୍ତୁ;
    ଅଚଳ V3LockGuard lock{s_mutex};

    string trimmsg = ସନ୍ଦେଶ;
    ଯଦି (!v3Global.opt.waiverMultiline()) {
        ଅଚଳ size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        ଯଦି (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            ଯେତେବେଳେ (*cp == ' ' || isdigit(*cp)) ++cp;
            ଯଦି (*cp == '|') ++cp;
            // ^~~~~
            ଯେତେବେଳେ (*cp == ' ' || *cp == '^') ++cp;
            ଯେତେବେଳେ (*cp == '~') ++cp;
            ଯେତେବେଳେ (*cp && *cp != '\n') result += *cp++;
            ଯେତେବେଳେ (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର& c : trimmsg) {
            ଯଦି (c == '*' || !std::isprint(c)) {
                add = "*";
            } ଅନ୍ୟଥା ଯଦି (c == ' ') {
                ଯଦି (add != "*") add += c;
            } ଅନ୍ୟଥା {
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
    s_waiverList.ଠେଲ_back(entry.str());
}

ଶୂନ୍ୟ V3Waiver::ଲିଖ(ଅଚଳ std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    ଅଚଳ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ଯଦି (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ଅଚଳ V3LockGuard lock{s_mutex};

    ଯଦି (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    ପାଇଁ (ଅଚଳ auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
