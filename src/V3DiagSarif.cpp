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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3DiagSarif.h"

#include "V3File.h"
#include "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

class V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, ཧྲིལ> m_codeIndex;

    // METHODS
    void calculate() {
        དོན་ལུ (const VErrorMessage& msgr : m_messages)
            གལ་སྲིད (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        ཧྲིལ i = 0;
        དོན་ལུ (const V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    void putRules(V3OutJsonFile& of) const {
        དོན་ལུ (const V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    void putText(V3OutJsonFile& of, const string& clean, const string& formatted) const {
        const string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    void putLocation(V3OutJsonFile& of, const FileLine* fl) const {
        const string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        གལ་སྲིད (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } དེ་མིན {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        གལ་སྲིད (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), false));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(const string& str) {
        const size_t pos = str.find("__WARNRELATED(", 0);
        ལོག (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    void putResult(V3OutJsonFile& of, const VErrorMessage& msg) {
        of.begin();
        of.put("level", msg.code().severityInfo()             ? "none"
                        : V3Error::isError(msg.code(), false) ? "error"
                                                              : "warning");

        string text = msg.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        const string first_clean = V3Error::stripMetaText(text, true);
        const string first_fmt = V3Error::stripMetaText(text, false);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        གལ་སྲིད (const FileLine* const fl = msg.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        གལ་སྲིད (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            སྐབས ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                const size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                སྐབས (end < text.size() && text[end] != ')') ++end;
                const size_t index = std::atoi(text.substr(start, end + 1).c_str());
                གལ་སྲིད (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < msg.filelines().size(),
                               "Error message warnRelated without fileline");

                const string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), true);
                const string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), false);
                const FileLine* fl = msg.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        གལ་སྲིད (msg.code().isNamed())
            of.put("ruleId", msg.code().ascii()).put("ruleIndex", m_codeIndex[msg.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
public:
    static V3DiagSarifImp& s() {
        static V3DiagSarifImp s_s;
        ལོག s_s;
    }

    void pushMessage(const VErrorMessage& msg) { m_messages.push_back(msg); }

    void output(bool success) {
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
        དོན་ལུ (const VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

void V3DiagSarif::pushMessage(const VErrorMessage& msg) VL_MT_DISABLED {
    གལ་སྲིད (!v3Global.opt.diagnosticsSarif()) ལོག;
    V3DiagSarifImp::s().pushMessage(msg);
}

void V3DiagSarif::output(bool success) {
    གལ་སྲིད (!v3Global.opt.diagnosticsSarif()) ལོག;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
