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

#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Waiver.h"

#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Options.h"

#அந்தர்பாவ <memory>
#அந்தர்பாவ <sstream>

வெறுமை V3Waiver::addEntry(V3ErrorCode errorCode, மாறிலி std::string& filename, மாறிலி std::string& செய்தி)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    எனில் (filename == V3Options::getStdPackagePath()) திரும்பு;
    எனில் (filename == V3Options::getStdWaiverPath()) திரும்பு;
    மாறிலி V3LockGuard lock{s_mutex};

    string trimmsg = செய்தி;
    எனில் (!v3Global.opt.waiverMultiline()) {
        மாறிலி size_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        எனில் (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string result;
        க்கு (மாறிலி எழுத்து* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            போது (*cp == ' ' || isdigit(*cp)) ++cp;
            எனில் (*cp == '|') ++cp;
            // ^~~~~
            போது (*cp == ' ' || *cp == '^') ++cp;
            போது (*cp == '~') ++cp;
            போது (*cp && *cp != '\n') result += *cp++;
            போது (*cp == '\n') result += *cp++;
        }
        trimmsg = result;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string result;
        string add;
        result.reserve(trimmsg.size());
        க்கு (மாறிலி எழுத்து& c : trimmsg) {
            எனில் (c == '*' || !std::isprint(c)) {
                add = "*";
            } இல்லையெனில் எனில் (c == ' ') {
                எனில் (add != "*") add += c;
            } இல்லையெனில் {
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
    s_waiverList.தள்ளு_back(entry.str());
}

வெறுமை V3Waiver::லிக(மாறிலி std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    மாறிலி std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    எனில் (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    மாறிலி V3LockGuard lock{s_mutex};

    எனில் (s_waiverList.empty()) *ofp << "// No waivers needed - great!\n";

    க்கு (மாறிலி auto& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
