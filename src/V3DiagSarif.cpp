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

শ্রেণী V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, পূর্ণসংখ্যা> m_code_সূচক;

    // METHODS
    শূন্য calculate() {
        জন্য (ধ্রুবক VErrorMessage& msgr : m_messages)
            যদি (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        পূর্ণসংখ্যা i = 0;
        জন্য (ধ্রুবক V3ErrorCode& code : m_codes) m_code_সূচক[code] = i++;
    }

    শূন্য putRules(V3OutJsonFile& of) ধ্রুবক {
        জন্য (ধ্রুবক V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    শূন্য putText(V3OutJsonFile& of, ধ্রুবক string& clean, ধ্রুবক string& formatted) ধ্রুবক {
        ধ্রুবক string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    শূন্য putLocation(V3OutJsonFile& of, ধ্রুবক File_সারি* fl) ধ্রুবক {
        ধ্রুবক string filename = V3Os::filenameRealPath(fl->filename());
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
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), মিথ্যা));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substr_To_পরবর্তী_Related(ধ্রুবক string& str) {
        ধ্রুবক আকার_t pos = str.find("__WARNRELATED(", 0);
        ফেরত (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    শূন্য put_ফলাফল(V3OutJsonFile& of, ধ্রুবক VErrorMessage& বার্তা) {
        of.begin();
        of.put("level", বার্তা.code().severityInfo()             ? "none"
                        : V3Error::isError(বার্তা.code(), মিথ্যা) ? "error"
                                                              : "warning");

        string text = বার্তা.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ধ্রুবক string first_clean = V3Error::stripMetaText(text, সত্য);
        ধ্রুবক string first_fmt = V3Error::stripMetaText(text, মিথ্যা);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        যদি (ধ্রুবক File_সারি* ধ্রুবক fl = বার্তা.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        যদি (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            আকার_t pos = 0;
            যখন ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ধ্রুবক আকার_t start = pos + std::strlen("__WARNRELATED(");
                আকার_t end = start;
                যখন (end < text.আকার_2() && text[end] != ')') ++end;
                ধ্রুবক আকার_t সূচক = std::atoi(text.substr(start, end + 1).c_str());
                যদি (end < text.আকার_2()) end += std::strlen(")");
                text = text.substr(end, text.আকার_2());
                UASSERT_STATIC(সূচক < বার্তা.filelines().আকার_2(),
                               "Error message warnRelated without fileline");

                ধ্রুবক string related_clean
                    = V3Error::stripMetaText(substr_To_পরবর্তী_Related(text), সত্য);
                ধ্রুবক string related_fmt
                    = V3Error::stripMetaText(substr_To_পরবর্তী_Related(text), মিথ্যা);
                ধ্রুবক File_সারি* fl = বার্তা.filelines()[সূচক];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        যদি (বার্তা.code().isNamed())
            of.put("ruleId", বার্তা.code().ascii()).put("ruleIndex", m_code_সূচক[বার্তা.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
সর্বজনীন:
    স্থির V3DiagSarifImp& s() {
        স্থির V3DiagSarifImp s_s;
        ফেরত s_s;
    }

    শূন্য ঠেলো_Message(ধ্রুবক VErrorMessage& বার্তা) { m_messages.ঠেলো_back(বার্তা); }

    শূন্য output(যৌক্তিক success) {
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
        জন্য (ধ্রুবক VErrorMessage& msgr : m_messages) put_ফলাফল(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

শূন্য V3DiagSarif::ঠেলো_Message(ধ্রুবক VErrorMessage& বার্তা) VL_MT_DISABLED {
    যদি (!v3Global.opt.diagnosticsSarif()) ফেরত;
    V3DiagSarifImp::s().ঠেলো_Message(বার্তা);
}

শূন্য V3DiagSarif::output(যৌক্তিক success) {
    যদি (!v3Global.opt.diagnosticsSarif()) ফেরত;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
