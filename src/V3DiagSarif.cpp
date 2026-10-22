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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3DiagSarif.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ଶ୍ରେଣୀ V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_codeIndex;

    // METHODS
    ଶୂନ୍ୟ calculate() {
        ପାଇଁ (ଅଚଳ VErrorMessage& msgr : m_messages)
            ଯଦି (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0;
        ପାଇଁ (ଅଚଳ V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    ଶୂନ୍ୟ putRules(V3OutJsonFile& of) ଅଚଳ {
        ପାଇଁ (ଅଚଳ V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    ଶୂନ୍ୟ putText(V3OutJsonFile& of, ଅଚଳ string& clean, ଅଚଳ string& formatted) ଅଚଳ {
        ଅଚଳ string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    ଶୂନ୍ୟ putLocation(V3OutJsonFile& of, ଅଚଳ File_ଧାଡ଼ି* fl) ଅଚଳ {
        ଅଚଳ string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        ଯଦି (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } ଅନ୍ୟଥା {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        ଯଦି (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), ମିଥ୍ୟା));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ଅଚଳ string& str) {
        ଅଚଳ size_t pos = str.find("__WARNRELATED(", 0);
        ଫେରନ୍ତୁ (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    ଶୂନ୍ୟ putResult(V3OutJsonFile& of, ଅଚଳ VErrorMessage& ସନ୍ଦେଶ) {
        of.begin();
        of.put("level", ସନ୍ଦେଶ.code().severityInfo()             ? "none"
                        : V3Error::isError(ସନ୍ଦେଶ.code(), ମିଥ୍ୟା) ? "error"
                                                              : "warning");

        string text = ସନ୍ଦେଶ.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ଅଚଳ string first_clean = V3Error::stripMetaText(text, ସତ୍ୟ);
        ଅଚଳ string first_fmt = V3Error::stripMetaText(text, ମିଥ୍ୟା);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        ଯଦି (ଅଚଳ File_ଧାଡ଼ି* ଅଚଳ fl = ସନ୍ଦେଶ.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        ଯଦି (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            ଯେତେବେଳେ ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ଅଚଳ size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                ଯେତେବେଳେ (end < text.size() && text[end] != ')') ++end;
                ଅଚଳ size_t index = std::atoi(text.substr(start, end + 1).c_str());
                ଯଦି (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < ସନ୍ଦେଶ.filelines().size(),
                               "Error message warnRelated without fileline");

                ଅଚଳ string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), ସତ୍ୟ);
                ଅଚଳ string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), ମିଥ୍ୟା);
                ଅଚଳ File_ଧାଡ଼ି* fl = ସନ୍ଦେଶ.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        ଯଦି (ସନ୍ଦେଶ.code().isNamed())
            of.put("ruleId", ସନ୍ଦେଶ.code().ascii()).put("ruleIndex", m_codeIndex[ସନ୍ଦେଶ.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର V3DiagSarifImp& s() {
        ସ୍ଥିର V3DiagSarifImp s_s;
        ଫେରନ୍ତୁ s_s;
    }

    ଶୂନ୍ୟ ଠେଲ_Message(ଅଚଳ VErrorMessage& ସନ୍ଦେଶ) { m_messages.ଠେଲ_back(ସନ୍ଦେଶ); }

    ଶୂନ୍ୟ output(ତାର୍କିକ success) {
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
        ପାଇଁ (ଅଚଳ VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

ଶୂନ୍ୟ V3DiagSarif::ଠେଲ_Message(ଅଚଳ VErrorMessage& ସନ୍ଦେଶ) VL_MT_DISABLED {
    ଯଦି (!v3Global.opt.diagnosticsSarif()) ଫେରନ୍ତୁ;
    V3DiagSarifImp::s().ଠେଲ_Message(ସନ୍ଦେଶ);
}

ଶୂନ୍ୟ V3DiagSarif::output(ତାର୍କିକ success) {
    ଯଦି (!v3Global.opt.diagnosticsSarif()) ଫେରନ୍ତୁ;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
