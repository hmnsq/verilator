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

वर्ग V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, पूर्णांक> m_codeIndex;

    // METHODS
    शून्य calculate() {
        साठी (स्थिरांक VErrorMessage& msgr : m_messages)
            जर (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        पूर्णांक i = 0;
        साठी (स्थिरांक V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    शून्य putRules(V3OutJsonFile& of) स्थिरांक {
        साठी (स्थिरांक V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    शून्य putText(V3OutJsonFile& of, स्थिरांक string& clean, स्थिरांक string& formatted) स्थिरांक {
        स्थिरांक string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    शून्य putLocation(V3OutJsonFile& of, स्थिरांक File_ओळ* fl) स्थिरांक {
        स्थिरांक string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        जर (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } अन्यथा {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        जर (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), असत्य));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(स्थिरांक string& str) {
        स्थिरांक माप_प्रकार pos = str.find("__WARNRELATED(", 0);
        परत (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    शून्य putResult(V3OutJsonFile& of, स्थिरांक VErrorMessage& संदेश) {
        of.begin();
        of.put("level", संदेश.code().severityInfo()             ? "none"
                        : V3Error::isError(संदेश.code(), असत्य) ? "error"
                                                              : "warning");

        string text = संदेश.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        स्थिरांक string first_clean = V3Error::stripMetaText(text, सत्य);
        स्थिरांक string first_fmt = V3Error::stripMetaText(text, असत्य);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        जर (स्थिरांक File_ओळ* स्थिरांक fl = संदेश.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        जर (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            माप_प्रकार pos = 0;
            पर्यंत ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                स्थिरांक माप_प्रकार start = pos + std::म_माप("__WARNRELATED(");
                माप_प्रकार end = start;
                पर्यंत (end < text.size() && text[end] != ')') ++end;
                स्थिरांक माप_प्रकार index = std::म_से_प(text.substr(start, end + 1).c_str());
                जर (end < text.size()) end += std::म_माप(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < संदेश.filelines().size(),
                               "Error message warnRelated without fileline");

                स्थिरांक string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), सत्य);
                स्थिरांक string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), असत्य);
                स्थिरांक File_ओळ* fl = संदेश.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        जर (संदेश.code().isNamed())
            of.put("ruleId", संदेश.code().ascii()).put("ruleIndex", m_codeIndex[संदेश.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
सार्वजनिक:
    स्थिर V3DiagSarifImp& s() {
        स्थिर V3DiagSarifImp s_s;
        परत s_s;
    }

    शून्य ढकला_Message(स्थिरांक VErrorMessage& संदेश) { m_messages.ढकला_back(संदेश); }

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
        साठी (स्थिरांक VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

शून्य V3DiagSarif::ढकला_Message(स्थिरांक VErrorMessage& संदेश) VL_MT_DISABLED {
    जर (!v3Global.opt.diagnosticsSarif()) परत;
    V3DiagSarifImp::s().ढकला_Message(संदेश);
}

शून्य V3DiagSarif::output(तार्किक success) {
    जर (!v3Global.opt.diagnosticsSarif()) परत;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
