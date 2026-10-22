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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3DiagSarif.h"

#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

বর্গ V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, পূর্ণাঙ্ক> m_codeIndex;

    // METHODS
    শূন্য calculate() {
        প্রতি (ধ্রুব VErrorMessage& msgr : m_messages)
            যদি (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        পূর্ণাঙ্ক i = 0;
        প্রতি (ধ্রুব V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    শূন্য putRules(V3OutJsonFile& of) ধ্রুব {
        প্রতি (ধ্রুব V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    শূন্য putText(V3OutJsonFile& of, ধ্রুব string& clean, ধ্রুব string& formatted) ধ্রুব {
        ধ্রুব string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    শূন্য putLocation(V3OutJsonFile& of, ধ্রুব File_সারি* fl) ধ্রুব {
        ধ্রুব string filename = V3Os::filenameRealPath(fl->filename());
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
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), অসত্য));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ধ্রুব string& str) {
        ধ্রুব size_t pos = str.find("__WARNRELATED(", 0);
        প্রত্যাগম (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    শূন্য putResult(V3OutJsonFile& of, ধ্রুব VErrorMessage& বার্তা) {
        of.begin();
        of.put("level", বার্তা.code().severityInfo()             ? "none"
                        : V3Error::isError(বার্তা.code(), অসত্য) ? "error"
                                                              : "warning");

        string text = বার্তা.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ধ্রুব string first_clean = V3Error::stripMetaText(text, সত্য);
        ধ্রুব string first_fmt = V3Error::stripMetaText(text, অসত্য);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        যদি (ধ্রুব File_সারি* ধ্রুব fl = বার্তা.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        যদি (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            যাবত্ ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ধ্রুব size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                যাবত্ (end < text.size() && text[end] != ')') ++end;
                ধ্রুব size_t index = std::atoi(text.substr(start, end + 1).c_str());
                যদি (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < বার্তা.filelines().size(),
                               "Error message warnRelated without fileline");

                ধ্রুব string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), সত্য);
                ধ্রুব string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), অসত্য);
                ধ্রুব File_সারি* fl = বার্তা.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        যদি (বার্তা.code().isNamed())
            of.put("ruleId", বার্তা.code().ascii()).put("ruleIndex", m_codeIndex[বার্তা.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
সার্বজনিক:
    স্থির V3DiagSarifImp& s() {
        স্থির V3DiagSarifImp s_s;
        প্রত্যাগম s_s;
    }

    শূন্য ঠেলো_Message(ধ্রুব VErrorMessage& বার্তা) { m_messages.ঠেলো_back(বার্তা); }

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
        প্রতি (ধ্রুব VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

শূন্য V3DiagSarif::ঠেলো_Message(ধ্রুব VErrorMessage& বার্তা) VL_MT_DISABLED {
    যদি (!v3Global.opt.diagnosticsSarif()) প্রত্যাগম;
    V3DiagSarifImp::s().ঠেলো_Message(বার্তা);
}

শূন্য V3DiagSarif::output(তার্কিক success) {
    যদি (!v3Global.opt.diagnosticsSarif()) প্রত্যাগম;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
