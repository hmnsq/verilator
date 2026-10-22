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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3DiagSarif.h"

#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

वग्ग V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, पूर्णाङ्क> m_codeIndex;

    // METHODS
    शून्य calculate() {
        अत्थाय (ध्रुव VErrorMessage& msgr : m_messages)
            सचे (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        पूर्णाङ्क i = 0;
        अत्थाय (ध्रुव V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    शून्य putRules(V3OutJsonFile& of) ध्रुव {
        अत्थाय (ध्रुव V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    शून्य putText(V3OutJsonFile& of, ध्रुव string& clean, ध्रुव string& formatted) ध्रुव {
        ध्रुव string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    शून्य putLocation(V3OutJsonFile& of, ध्रुव File_पङ्क्तिः* fl) ध्रुव {
        ध्रुव string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        सचे (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } अञ्ञथा {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        सचे (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), मिच्छा));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ध्रुव string& str) {
        ध्रुव size_t pos = str.find("__WARNRELATED(", 0);
        पटिनिवत्त (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    शून्य putResult(V3OutJsonFile& of, ध्रुव VErrorMessage& सन्देशः) {
        of.begin();
        of.put("level", सन्देशः.code().severityInfo()             ? "none"
                        : V3Error::isError(सन्देशः.code(), मिच्छा) ? "error"
                                                              : "warning");

        string text = सन्देशः.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ध्रुव string first_clean = V3Error::stripMetaText(text, सच्च);
        ध्रुव string first_fmt = V3Error::stripMetaText(text, मिच्छा);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        सचे (ध्रुव File_पङ्क्तिः* ध्रुव fl = सन्देशः.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        सचे (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            याव ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ध्रुव size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                याव (end < text.size() && text[end] != ')') ++end;
                ध्रुव size_t index = std::atoi(text.substr(start, end + 1).c_str());
                सचे (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < सन्देशः.filelines().size(),
                               "Error message warnRelated without fileline");

                ध्रुव string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), सच्च);
                ध्रुव string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), मिच्छा);
                ध्रुव File_पङ्क्तिः* fl = सन्देशः.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        सचे (सन्देशः.code().isNamed())
            of.put("ruleId", सन्देशः.code().ascii()).put("ruleIndex", m_codeIndex[सन्देशः.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
सार्वजनिक:
    स्थिर V3DiagSarifImp& s() {
        स्थिर V3DiagSarifImp s_s;
        पटिनिवत्त s_s;
    }

    शून्य प्रक्षिप_Message(ध्रुव VErrorMessage& सन्देशः) { m_messages.प्रक्षिप_back(सन्देशः); }

    शून्य output(तार्किक success) {
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
        अत्थाय (ध्रुव VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

शून्य V3DiagSarif::प्रक्षिप_Message(ध्रुव VErrorMessage& सन्देशः) VL_MT_DISABLED {
    सचे (!v3Global.opt.diagnosticsSarif()) पटिनिवत्त;
    V3DiagSarifImp::s().प्रक्षिप_Message(सन्देशः);
}

शून्य V3DiagSarif::output(तार्किक success) {
    सचे (!v3Global.opt.diagnosticsSarif()) पटिनिवत्त;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
