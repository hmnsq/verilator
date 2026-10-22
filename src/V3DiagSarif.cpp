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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DiagSarif.h"

#ضمن "V3File.h"
#ضمن "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

صنف V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, صحيح> m_codeIndex;

    // METHODS
    فراغ calculate() {
        لكل (ثابت VErrorMessage& msgr : m_messages)
            إذا (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        صحيح i = 0;
        لكل (ثابت V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    فراغ putRules(V3OutJsonFile& of) ثابت {
        لكل (ثابت V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    فراغ putText(V3OutJsonFile& of, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    فراغ putLocation(V3OutJsonFile& of, ثابت File_هێڵ* fl) ثابت {
        ثابت string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        إذا (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } وإلا {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        إذا (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), خطأ));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ثابت string& str) {
        ثابت size_t pos = str.find("__WARNRELATED(", 0);
        أرجع (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    فراغ putResult(V3OutJsonFile& of, ثابت VErrorMessage& پەیام) {
        of.begin();
        of.put("level", پەیام.code().severityInfo()             ? "none"
                        : V3Error::isError(پەیام.code(), خطأ) ? "error"
                                                              : "warning");

        string text = پەیام.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string first_clean = V3Error::stripMetaText(text, true);
        ثابت string first_fmt = V3Error::stripMetaText(text, خطأ);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        إذا (ثابت File_هێڵ* ثابت fl = پەیام.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        إذا (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            طالما ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                طالما (end < text.size() && text[end] != ')') ++end;
                ثابت size_t index = std::atoi(text.substr(start, end + 1).c_str());
                إذا (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < پەیام.filelines().size(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), true);
                ثابت string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), خطأ);
                ثابت File_هێڵ* fl = پەیام.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        إذا (پەیام.code().isNamed())
            of.put("ruleId", پەیام.code().ascii()).put("ruleIndex", m_codeIndex[پەیام.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
عام:
    ساكن V3DiagSarifImp& s() {
        ساكن V3DiagSarifImp s_s;
        أرجع s_s;
    }

    فراغ پاڵنان_Message(ثابت VErrorMessage& پەیام) { m_messages.پاڵنان_back(پەیام); }

    فراغ output(منطقي success) {
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
        لكل (ثابت VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

فراغ V3DiagSarif::پاڵنان_Message(ثابت VErrorMessage& پەیام) VL_MT_DISABLED {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    V3DiagSarifImp::s().پاڵنان_Message(پەیام);
}

فراغ V3DiagSarif::output(منطقي success) {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
