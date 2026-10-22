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

शून्य V3Waiver::सङ्कलय_प्रविष्टिः(V3_Error_संहिता error_संहिता, ध्रुव std::string& filename, ध्रुव std::string& सन्देशः_2)
    VL_MT_SAFE_EXCLUDES(s_mutex) {
    यदि (filename == V3Options::get_Std_Package_मार्गः()) प्रत्यागम;
    यदि (filename == V3Options::get_Std_Waiver_मार्गः()) प्रत्यागम;
    ध्रुव V3_तालकम्_Guard तालकम्_2{s_mutex};

    string trimmsg = सन्देशः_2;
    यदि (!v3Global.opt.waiverMultiline()) {
        ध्रुव आकार_t pos = trimmsg.find('\n');
        trimmsg.resize(pos);
        यदि (pos != std::string::npos) trimmsg += '*';
    }
    {  // Remove line numbers and context "\n [0-9] | ", "\n  ^[~]+"
        string परिणाम;
        प्रति (ध्रुव वर्ण* cp = trimmsg.c_str(); *cp; cp = *cp ? cp + 1 : cp) {
            यावत् (*cp == ' ' || अङ्कः_वा(*cp)) ++cp;
            यदि (*cp == '|') ++cp;
            // ^~~~~
            यावत् (*cp == ' ' || *cp == '^') ++cp;
            यावत् (*cp == '~') ++cp;
            यावत् (*cp && *cp != '\n') परिणाम += *cp++;
            यावत् (*cp == '\n') परिणाम += *cp++;
        }
        trimmsg = परिणाम;
    }
    trimmsg += '*';
    {  // "\n"->"*", " *"->"*", "* "->"*"
        string परिणाम;
        string सङ्कलय;
        परिणाम.reserve(trimmsg.आकार());
        प्रति (ध्रुव वर्ण& c : trimmsg) {
            यदि (c == '*' || !std::isprint(c)) {
                सङ्कलय = "*";
            } अन्यथा यदि (c == ' ') {
                यदि (सङ्कलय != "*") सङ्कलय += c;
            } अन्यथा {
                परिणाम += सङ्कलय + c;
                सङ्कलय = "";
            }
        }
        परिणाम += सङ्कलय;
        trimmsg = परिणाम;
    }

    std::stringstream प्रविष्टिः;
    प्रविष्टिः << "lint_off -rule " << error_संहिता.ascii() << " -file \"*"
          << VString::quoteBackslash(filename) << "\" -match \"" << trimmsg << "\"";
    s_waiverList.प्रक्षिप_पृष्ठभागः(प्रविष्टिः.str());
}

शून्य V3Waiver::लिख(ध्रुव std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex) {
    ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    *ofp << "// DESCR"
            "IPTION: Verilator output: Waivers generated with --waiver-output\n\n";

    *ofp << "`verilator_config\n\n";

    *ofp << "// Below are suggested waivers. You have three options:\n";
    *ofp << "//   1. Fix the reason for the linter warning in the Verilog sources\n";
    *ofp << "//   2. Keep the waiver permanently if you are sure it is okay\n";
    *ofp << "//   3. Keep the waiver temporarily to suppress the output\n\n";

    ध्रुव V3_तालकम्_Guard तालकम्_2{s_mutex};

    यदि (s_waiverList.रिक्तानि()) *ofp << "// No waivers needed - great!\n";

    प्रति (ध्रुव स्वचालित& i : s_waiverList) *ofp << "// " << i << "\n\n";
}

V3Mutex V3Waiver::s_mutex;
V3Waiver::WaiverList V3Waiver::s_waiverList;
