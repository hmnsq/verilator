// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Diag Sarif output file
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3DiagSarif.h"

#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

শ্ৰেণী V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, পূর্ণাঙ্ক> m_codeIndex;

    // METHODS
    শূন্য calculate() {
        বাবে (ধ্রুৱ VErrorMessage& msgr : m_messages)
            যদি (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        পূর্ণাঙ্ক i = 0;
        বাবে (ধ্রুৱ V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    শূন্য putRules(V3OutJsonFile& of) ধ্রুৱ {
        বাবে (ধ্রুৱ V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    শূন্য putText(V3OutJsonFile& of, ধ্রুৱ string& clean, ধ্রুৱ string& formatted) ধ্রুৱ {
        ধ্রুৱ string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    শূন্য putLocation(V3OutJsonFile& of, ধ্রুৱ File_সাৰি* fl) ধ্রুৱ {
        ধ্রুৱ string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        যদি (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } অন্যথা {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        যদি (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), মিছা));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ধ্রুৱ string& str) {
        ধ্রুৱ size_t pos = str.find("__WARNRELATED(", 0);
        ঘূৰাই (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    শূন্য putResult(V3OutJsonFile& of, ধ্রুৱ VErrorMessage& বাৰ্তা) {
        of.begin();
        of.put("level", বাৰ্তা.code().severityInfo()             ? "none"
                        : V3Error::isError(বাৰ্তা.code(), মিছা) ? "error"
                                                              : "warning");

        string text = বাৰ্তা.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ধ্রুৱ string first_clean = V3Error::stripMetaText(text, সঁচা);
        ধ্রুৱ string first_fmt = V3Error::stripMetaText(text, মিছা);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        যদি (ধ্রুৱ File_সাৰি* ধ্রুৱ fl = বাৰ্তা.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        যদি (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            যেতিয়ালৈ ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ধ্রুৱ size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                যেতিয়ালৈ (end < text.size() && text[end] != ')') ++end;
                ধ্রুৱ size_t index = std::atoi(text.substr(start, end + 1).c_str());
                যদি (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < বাৰ্তা.filelines().size(),
                               "Error message warnRelated without fileline");

                ধ্রুৱ string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), সঁচা);
                ধ্রুৱ string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), মিছা);
                ধ্রুৱ File_সাৰি* fl = বাৰ্তা.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        যদি (বাৰ্তা.code().isNamed())
            of.put("ruleId", বাৰ্তা.code().ascii()).put("ruleIndex", m_codeIndex[বাৰ্তা.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
সার্ৱজনিক:
    স্থির V3DiagSarifImp& s() {
        স্থির V3DiagSarifImp s_s;
        ঘূৰাই s_s;
    }

    শূন্য ঠেলো_Message(ধ্রুৱ VErrorMessage& বাৰ্তা) { m_messages.ঠেলো_back(বাৰ্তা); }

    শূন্য output(তার্কিক success) {
        V3OutJsonFile of{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        of.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        of.begin("runs", '[').begin();

        of.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::version(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        of.begin("rules", '[');
        putRules(of);
        of.end()  // rules
            .end()  // driver
            .end();  //tool

        of.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.allArgsString())
            .put("executionSuccessful", success)
            .end()
            .end();

        of.begin("results", '[');
        বাবে (ধ্রুৱ VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

শূন্য V3DiagSarif::ঠেলো_Message(ধ্রুৱ VErrorMessage& বাৰ্তা) VL_MT_DISABLED {
    যদি (!v3Global.opt.diagnosticsSarif()) ঘূৰাই;
    V3DiagSarifImp::s().ঠেলো_Message(বাৰ্তা);
}

শূন্য V3DiagSarif::output(তার্কিক success) {
    যদি (!v3Global.opt.diagnosticsSarif()) ঘূৰাই;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
