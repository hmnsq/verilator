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
    std::deque<VError_सन्देशः> m_messages;
    std::समुच्चयः<V3_Error_संहिता> m_codes;
    std::map<V3_Error_संहिता, पूर्णाङ्क> m_संहिता_सूचकांक;

    // METHODS
    शून्य calculate() {
        प्रति (ध्रुव VError_सन्देशः& msgr : m_messages)
            यदि (msgr.संहिता().isNamed()) m_codes.emplace(msgr.संहिता());
        पूर्णाङ्क i = 0;
        प्रति (ध्रुव V3_Error_संहिता& संहिता : m_codes) m_संहिता_सूचकांक[संहिता] = i++;
    }

    शून्य put_नियमाः(V3_बहिर्गमनम्_Json_File& सम्बन्धि) ध्रुव {
        प्रति (ध्रुव V3_Error_संहिता& संहिता : m_codes) {
            सम्बन्धि.begin().put("id", संहिता.ascii()).put("helpUri", संहिता.url()).अन्तः_2();
        }
    }

    शून्य putText(V3_बहिर्गमनम्_Json_File& सम्बन्धि, ध्रुव string& clean, ध्रुव string& formatted) ध्रुव {
        ध्रुव string markdown = "```\n" + formatted + "\n```\n";
        सम्बन्धि.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    शून्य putLocation(V3_बहिर्गमनम्_Json_File& सम्बन्धि, ध्रुव File_पङ्क्तिः* fl) ध्रुव {
        ध्रुव string filename = V3Os::filename_Real_मार्गः(fl->filename());
        सम्बन्धि.begin("physicalLocation");
        सम्बन्धि.begin("artifactLocation");
        यदि (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            सम्बन्धि.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } अन्यथा {
            सम्बन्धि.put("uri", "file://" + filename);
        }
        सम्बन्धि.अन्तः_2()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        यदि (fl->firstLineno() != fl->चरमः_Lineno()) सम्बन्धि.put("endLine", fl->चरमः_Lineno());
        सम्बन्धि.put("endColumn", fl->चरमः_Column());
        सम्बन्धि.begin("snippit", '{');
        putText(सम्बन्धि, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), असत्य));
        सम्बन्धि.अन्तः_2();  // snippit
        सम्बन्धि.अन्तः_2();  // region
        सम्बन्धि.अन्तः_2();  // artifactLocation
    }

    string substr_गन्तव्यम्_अग्र_Related(ध्रुव string& str) {
        ध्रुव आकार_t pos = str.find("__WARNRELATED(", 0);
        प्रत्यागम (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    शून्य put_परिणाम(V3_बहिर्गमनम्_Json_File& सम्बन्धि, ध्रुव VError_सन्देशः& सन्देशः_2) {
        सम्बन्धि.begin();
        सम्बन्धि.put("level", सन्देशः_2.संहिता().severityInfo()             ? "none"
                        : V3Error::isError(सन्देशः_2.संहिता(), असत्य) ? "error"
                                                              : "warning");

        string text = सन्देशः_2.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ध्रुव string first_clean = V3Error::stripMetaText(text, सत्य);
        ध्रुव string first_fmt = V3Error::stripMetaText(text, असत्य);

        सम्बन्धि.begin("message");
        putText(सम्बन्धि, first_clean, first_fmt);
        सम्बन्धि.अन्तः_2();
        यदि (ध्रुव File_पङ्क्तिः* ध्रुव fl = सन्देशः_2.fileline()) {
            सम्बन्धि.begin("locations", '[').begin();
            putLocation(सम्बन्धि, fl);
            सम्बन्धि.अन्तः_2();
            सम्बन्धि.अन्तः_2();
        }

        यदि (text.find("__WARNRELATED(") != std::string::npos) {
            सम्बन्धि.begin("relatedLocations", '[');
            आकार_t pos = 0;
            यावत् ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ध्रुव आकार_t start = pos + std::वाक्यदैर्घ्यम्("__WARNRELATED(");
                आकार_t अन्तः_2 = start;
                यावत् (अन्तः_2 < text.आकार() && text[अन्तः_2] != ')') ++अन्तः_2;
                ध्रुव आकार_t सूचकांक = std::atoi(text.substr(start, अन्तः_2 + 1).c_str());
                यदि (अन्तः_2 < text.आकार()) अन्तः_2 += std::वाक्यदैर्घ्यम्(")");
                text = text.substr(अन्तः_2, text.आकार());
                UASSERT_STATIC(सूचकांक < सन्देशः_2.filelines().आकार(),
                               "Error message warnRelated without fileline");

                ध्रुव string related_clean
                    = V3Error::stripMetaText(substr_गन्तव्यम्_अग्र_Related(text), सत्य);
                ध्रुव string related_fmt
                    = V3Error::stripMetaText(substr_गन्तव्यम्_अग्र_Related(text), असत्य);
                ध्रुव File_पङ्क्तिः* fl = सन्देशः_2.filelines()[सूचकांक];
                सम्बन्धि.begin().begin("message");
                putText(सम्बन्धि, related_clean, related_fmt);
                सम्बन्धि.अन्तः_2();  //सन्देशः
                putLocation(सम्बन्धि, fl);
                सम्बन्धि.अन्तः_2();
            }
            सम्बन्धि.अन्तः_2();
        }

        यदि (सन्देशः_2.संहिता().isNamed())
            सम्बन्धि.put("ruleId", सन्देशः_2.संहिता().ascii()).put("ruleIndex", m_संहिता_सूचकांक[सन्देशः_2.संहिता()]);

        सम्बन्धि.अन्तः_2();
    }

    // STATIC FUNCTIONS
सार्वजनिक:
    स्थिर V3DiagSarifImp& s() {
        स्थिर V3DiagSarifImp s_s;
        प्रत्यागम s_s;
    }

    शून्य प्रक्षिप_सन्देशः(ध्रुव VError_सन्देशः& सन्देशः_2) { m_messages.प्रक्षिप_पृष्ठभागः(सन्देशः_2); }

    शून्य output(तार्किक success) {
        V3_बहिर्गमनम्_Json_File सम्बन्धि{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        सम्बन्धि.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        सम्बन्धि.begin("runs", '[').begin();

        सम्बन्धि.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::संस्करणम्(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        सम्बन्धि.begin("rules", '[');
        put_नियमाः(सम्बन्धि);
        सम्बन्धि.अन्तः_2()  //नियमाः
            .अन्तः_2()  // driver
            .अन्तः_2();  //tool

        सम्बन्धि.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_तर्काः_String())
            .put("executionSuccessful", success)
            .अन्तः_2()
            .अन्तः_2();

        सम्बन्धि.begin("results", '[');
        प्रति (ध्रुव VError_सन्देशः& msgr : m_messages) put_परिणाम(सम्बन्धि, msgr);
        सम्बन्धि.अन्तः_2();

        सम्बन्धि.अन्तः_2();  // runs ]
        सम्बन्धि.अन्तः_2();  // runs }
    }
};

शून्य V3DiagSarif::प्रक्षिप_सन्देशः(ध्रुव VError_सन्देशः& सन्देशः_2) VL_MT_DISABLED {
    यदि (!v3Global.opt.diagnosticsSarif()) प्रत्यागम;
    V3DiagSarifImp::s().प्रक्षिप_सन्देशः(सन्देशः_2);
}

शून्य V3DiagSarif::output(तार्किक success) {
    यदि (!v3Global.opt.diagnosticsSarif()) प्रत्यागम;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
