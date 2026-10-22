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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3DiagSarif.h"

#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

വർഗ്ഗം V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, പൂർണ്ണസംഖ്യ> m_codeIndex;

    // METHODS
    ശൂന്യം calculate() {
        വേണ്ടി (സ്ഥിരം VErrorMessage& msgr : m_messages)
            എങ്കിൽ (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        പൂർണ്ണസംഖ്യ i = 0;
        വേണ്ടി (സ്ഥിരം V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    ശൂന്യം putRules(V3OutJsonFile& of) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    ശൂന്യം putText(V3OutJsonFile& of, സ്ഥിരം string& clean, സ്ഥിരം string& formatted) സ്ഥിരം {
        സ്ഥിരം string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    ശൂന്യം putLocation(V3OutJsonFile& of, സ്ഥിരം File_വരി* fl) സ്ഥിരം {
        സ്ഥിരം string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        എങ്കിൽ (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } അല്ലെങ്കിൽ {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        എങ്കിൽ (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), അസത്യം));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(സ്ഥിരം string& str) {
        സ്ഥിരം size_t pos = str.find("__WARNRELATED(", 0);
        തിരികെ (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    ശൂന്യം putResult(V3OutJsonFile& of, സ്ഥിരം VErrorMessage& സന്ദേശം) {
        of.begin();
        of.put("level", സന്ദേശം.code().severityInfo()             ? "none"
                        : V3Error::isError(സന്ദേശം.code(), അസത്യം) ? "error"
                                                              : "warning");

        string text = സന്ദേശം.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        സ്ഥിരം string first_clean = V3Error::stripMetaText(text, സത്യം);
        സ്ഥിരം string first_fmt = V3Error::stripMetaText(text, അസത്യം);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        എങ്കിൽ (സ്ഥിരം File_വരി* സ്ഥിരം fl = സന്ദേശം.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        എങ്കിൽ (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            ആകുമ്പോൾ ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                സ്ഥിരം size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                ആകുമ്പോൾ (end < text.size() && text[end] != ')') ++end;
                സ്ഥിരം size_t index = std::atoi(text.substr(start, end + 1).c_str());
                എങ്കിൽ (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < സന്ദേശം.filelines().size(),
                               "Error message warnRelated without fileline");

                സ്ഥിരം string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), സത്യം);
                സ്ഥിരം string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), അസത്യം);
                സ്ഥിരം File_വരി* fl = സന്ദേശം.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        എങ്കിൽ (സന്ദേശം.code().isNamed())
            of.put("ruleId", സന്ദേശം.code().ascii()).put("ruleIndex", m_codeIndex[സന്ദേശം.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
പൊതു:
    സ്ഥിതം V3DiagSarifImp& s() {
        സ്ഥിതം V3DiagSarifImp s_s;
        തിരികെ s_s;
    }

    ശൂന്യം തള്ളുക_Message(സ്ഥിരം VErrorMessage& സന്ദേശം) { m_messages.തള്ളുക_back(സന്ദേശം); }

    ശൂന്യം output(തർക്കം success) {
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
        വേണ്ടി (സ്ഥിരം VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

ശൂന്യം V3DiagSarif::തള്ളുക_Message(സ്ഥിരം VErrorMessage& സന്ദേശം) VL_MT_DISABLED {
    എങ്കിൽ (!v3Global.opt.diagnosticsSarif()) തിരികെ;
    V3DiagSarifImp::s().തള്ളുക_Message(സന്ദേശം);
}

ശൂന്യം V3DiagSarif::output(തർക്കം success) {
    എങ്കിൽ (!v3Global.opt.diagnosticsSarif()) തിരികെ;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
