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

#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Waiver.h"

#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Options.h"

#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <sstream>

শূন্য V3Waiver::addEntry(V3ErrorCode errorCode, ধ্রুৱ std::string& filename, ধ্রুৱ std::string& বাৰ্তা)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    যদি (filename == V3Options::getStdPackagePath()) ঘূৰাই;
    যদি (filename == V3Options::getStdWaiverPath()) ঘূৰাই;
    ধ্রুৱ V3LockGuard lock{s_mutex};

    string trimmsg = বাৰ্তা;
    যদি (!v3Global.opt.waiverMultiline()) {
        ধ্রুৱ size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        যদি (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        বাবে (ধ্রুৱ ৱর্ণ* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            যেতিয়ালৈ (*cp == ' ' || isdigit(*cp)) ++cp;
            যদি (*cp == '|') ++cp;
            // ^~~~~
            যেতিয়ালৈ (*cp == ' ' || *cp == '^') ++cp;
            যেতিয়ালৈ (*cp == '~') ++cp;
            যেতিয়ালৈ (*cp && *cp != '\n') result += *cp++;
            যেতিয়ালৈ (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        বাবে (ধ্রুৱ ৱর্ণ& c : trimmsg) {
            যদি (c == '*' || !std::isprint(c)) {
                add = "*";
            } অন্যথা যদি (c == ' ') {
                যদি (add != "*") add += c;
            } অন্যথা {
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
    s_waiverList.ঠেলো_back(entry.str());
}

শূন্য V3Waiver::লিখ(ধ্রুৱ std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    ধ্রুৱ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    যদি (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ধ্রুৱ V3LockGuard lock{s_mutex};

    যদি (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    বাবে (ধ্রুৱ auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
