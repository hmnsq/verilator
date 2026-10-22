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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3DiagSarif.h"

#समावेश "V3File.h"
#समावेश "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

श्रेणी V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, पूर्णांक> m_code_सूचकांक;

    // METHODS
    व्योम calculate() {
        क्रम (स्थिर VErrorMessage& msgr : m_messages)
            अगर (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        पूर्णांक i = 0;
        क्रम (स्थिर V3ErrorCode& code : m_codes) m_code_सूचकांक[code] = i++;
    }

    व्योम putRules(V3OutJsonFile& of) स्थिर {
        क्रम (स्थिर V3ErrorCode& code : m_codes) {
            of.begin().दो("id", code.ascii()).दो("helpUri", code.url()).end();
        }
    }

    व्योम putText(V3OutJsonFile& of, स्थिर string& clean, स्थिर string& formatted) स्थिर {
        स्थिर string markdown = "```\n" + formatted + "\n```\n";
        of.दो("text", VString::trimWhitespace(clean)).दो("markdown", markdown);
    }

    व्योम putLocation(V3OutJsonFile& of, स्थिर File_पंक्ति* fl) स्थिर {
        स्थिर string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        अगर (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.दो("uri", "file:" + filename).दो("uriBaseId", "%srcroot%");
        } अन्यथा {
            of.दो("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .दो("sourceLanguage", "systemverilog")
            .दो("startLine", fl->firstLineno())
            .दो("startColumn", fl->firstColumn());
        अगर (fl->firstLineno() != fl->lastLineno()) of.दो("endLine", fl->lastLineno());
        of.दो("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), false));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substr_To_अग्र_Related(स्थिर string& str) {
        स्थिर माप_प्रकार pos = str.find("__WARNRELATED(", 0);
        वापस (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    व्योम put_परिणाम(V3OutJsonFile& of, स्थिर VErrorMessage& संदेश) {
        of.begin();
        of.दो("level", संदेश.code().severityInfo()             ? "none"
                        : V3Error::isError(संदेश.code(), false) ? "error"
                                                              : "warning");

        string text = संदेश.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        स्थिर string first_clean = V3Error::stripMetaText(text, true);
        स्थिर string first_fmt = V3Error::stripMetaText(text, false);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        अगर (स्थिर File_पंक्ति* स्थिर fl = संदेश.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        अगर (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            माप_प्रकार pos = 0;
            जबतक ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                स्थिर माप_प्रकार start = pos + std::म_माप("__WARNRELATED(");
                माप_प्रकार end = start;
                जबतक (end < text.आकार() && text[end] != ')') ++end;
                स्थिर माप_प्रकार सूचकांक_2 = std::म_से_प(text.substr(start, end + 1).c_str());
                अगर (end < text.आकार()) end += std::म_माप(")");
                text = text.substr(end, text.आकार());
                UASSERT_STATIC(सूचकांक_2 < संदेश.filelines().आकार(),
                               "Error message warnRelated without fileline");

                स्थिर string related_clean
                    = V3Error::stripMetaText(substr_To_अग्र_Related(text), true);
                स्थिर string related_fmt
                    = V3Error::stripMetaText(substr_To_अग्र_Related(text), false);
                स्थिर File_पंक्ति* fl = संदेश.filelines()[सूचकांक_2];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        अगर (संदेश.code().isNamed())
            of.दो("ruleId", संदेश.code().ascii()).दो("ruleIndex", m_code_सूचकांक[संदेश.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
खुला:
    अटल V3DiagSarifImp& s() {
        अटल V3DiagSarifImp s_s;
        वापस s_s;
    }

    व्योम धकेलो_Message(स्थिर VErrorMessage& संदेश) { m_messages.धकेलो_back(संदेश); }

    व्योम output(bool success) {
        V3OutJsonFile of{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        of.दो("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .दो("version", "2.1.0");

        of.begin("runs", '[').begin();

        of.begin("tool")
            .begin("driver")
            .दो("name", "Verilator")
            .दो("version", VString::replaceSubstr(V3Options::version(), "Verilator ", ""))
            .दो("informationUri", "https://verilator.org");

        of.begin("rules", '[');
        putRules(of);
        of.end()  // rules
            .end()  // driver
            .end();  //tool

        of.begin("invocations", '[')
            .begin()
            .दो("commandLine", v3Global.opt.allArgsString())
            .दो("executionSuccessful", success)
            .end()
            .end();

        of.begin("results", '[');
        क्रम (स्थिर VErrorMessage& msgr : m_messages) put_परिणाम(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

व्योम V3DiagSarif::धकेलो_Message(स्थिर VErrorMessage& संदेश) VL_MT_DISABLED {
    अगर (!v3Global.opt.diagnosticsSarif()) वापस;
    V3DiagSarifImp::s().धकेलो_Message(संदेश);
}

व्योम V3DiagSarif::output(bool success) {
    अगर (!v3Global.opt.diagnosticsSarif()) वापस;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
