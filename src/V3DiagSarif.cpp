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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3DiagSarif.h"

#شامل "V3File.h"
#شامل "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

رده V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, صحیح> m_codeIndex;

    // METHODS
    تهی calculate() {
        برای (ثابت VErrorMessage& msgr : m_messages)
            اگر (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        صحیح i = 0;
        برای (ثابت V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    تهی putRules(V3OutJsonFile& of) ثابت {
        برای (ثابت V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    تهی putText(V3OutJsonFile& of, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    تهی putLocation(V3OutJsonFile& of, ثابت File_سطر* fl) ثابت {
        ثابت string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        اگر (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } وگرنه {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        اگر (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), نادرست));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ثابت string& str) {
        ثابت size_t pos = str.find("__WARNRELATED(", 0);
        بازگشت (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    تهی putResult(V3OutJsonFile& of, ثابت VErrorMessage& پیام) {
        of.begin();
        of.put("level", پیام.code().severityInfo()             ? "none"
                        : V3Error::isError(پیام.code(), نادرست) ? "error"
                                                              : "warning");

        string text = پیام.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string first_clean = V3Error::stripMetaText(text, درست);
        ثابت string first_fmt = V3Error::stripMetaText(text, نادرست);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        اگر (ثابت File_سطر* ثابت fl = پیام.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        اگر (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            وقتی ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                وقتی (end < text.size() && text[end] != ')') ++end;
                ثابت size_t index = std::atoi(text.substr(start, end + 1).c_str());
                اگر (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < پیام.filelines().size(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), درست);
                ثابت string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), نادرست);
                ثابت File_سطر* fl = پیام.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        اگر (پیام.code().isNamed())
            of.put("ruleId", پیام.code().ascii()).put("ruleIndex", m_codeIndex[پیام.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
عمومی:
    ایستا V3DiagSarifImp& s() {
        ایستا V3DiagSarifImp s_s;
        بازگشت s_s;
    }

    تهی افزودن_Message(ثابت VErrorMessage& پیام) { m_messages.افزودن_back(پیام); }

    تهی output(منطقی success) {
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
        برای (ثابت VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

تهی V3DiagSarif::افزودن_Message(ثابت VErrorMessage& پیام) VL_MT_DISABLED {
    اگر (!v3Global.opt.diagnosticsSarif()) بازگشت;
    V3DiagSarifImp::s().افزودن_Message(پیام);
}

تهی V3DiagSarif::output(منطقی success) {
    اگر (!v3Global.opt.diagnosticsSarif()) بازگشت;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
