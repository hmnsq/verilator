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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3DiagSarif.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ವರ್ಗ V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, ಪೂರ್ಣಾಙ್ಕ> m_codeIndex;

    // METHODS
    ಶೂನ್ಯ calculate() {
        ಪ್ರತಿ (ಧ್ರುವ VErrorMessage& msgr : m_messages)
            ಯದಿ (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        ಪೂರ್ಣಾಙ್ಕ i = 0;
        ಪ್ರತಿ (ಧ್ರುವ V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    ಶೂನ್ಯ putRules(V3OutJsonFile& of) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    ಶೂನ್ಯ putText(V3OutJsonFile& of, ಧ್ರುವ string& clean, ಧ್ರುವ string& formatted) ಧ್ರುವ {
        ಧ್ರುವ string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    ಶೂನ್ಯ putLocation(V3OutJsonFile& of, ಧ್ರುವ File_ಸಾಲು* fl) ಧ್ರುವ {
        ಧ್ರುವ string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        ಯದಿ (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } ಅನ್ಯಥಾ {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        ಯದಿ (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), ಅಸತ್ಯ));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(ಧ್ರುವ string& str) {
        ಧ್ರುವ size_t pos = str.find("__WARNRELATED(", 0);
        ಪ್ರತ್ಯಾಗಮ (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    ಶೂನ್ಯ putResult(V3OutJsonFile& of, ಧ್ರುವ VErrorMessage& ಸಂದೇಶ) {
        of.begin();
        of.put("level", ಸಂದೇಶ.code().severityInfo()             ? "none"
                        : V3Error::isError(ಸಂದೇಶ.code(), ಅಸತ್ಯ) ? "error"
                                                              : "warning");

        string text = ಸಂದೇಶ.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ಧ್ರುವ string first_clean = V3Error::stripMetaText(text, ಸತ್ಯ);
        ಧ್ರುವ string first_fmt = V3Error::stripMetaText(text, ಅಸತ್ಯ);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        ಯದಿ (ಧ್ರುವ File_ಸಾಲು* ಧ್ರುವ fl = ಸಂದೇಶ.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        ಯದಿ (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            ಯಾವತ್ ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                ಧ್ರುವ size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                ಯಾವತ್ (end < text.size() && text[end] != ')') ++end;
                ಧ್ರುವ size_t index = std::atoi(text.substr(start, end + 1).c_str());
                ಯದಿ (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < ಸಂದೇಶ.filelines().size(),
                               "Error message warnRelated without fileline");

                ಧ್ರುವ string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), ಸತ್ಯ);
                ಧ್ರುವ string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), ಅಸತ್ಯ);
                ಧ್ರುವ File_ಸಾಲು* fl = ಸಂದೇಶ.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        ಯದಿ (ಸಂದೇಶ.code().isNamed())
            of.put("ruleId", ಸಂದೇಶ.code().ascii()).put("ruleIndex", m_codeIndex[ಸಂದೇಶ.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3DiagSarifImp& s() {
        ಸ್ಥಿರ V3DiagSarifImp s_s;
        ಪ್ರತ್ಯಾಗಮ s_s;
    }

    ಶೂನ್ಯ ತಳ್ಳು_Message(ಧ್ರುವ VErrorMessage& ಸಂದೇಶ) { m_messages.ತಳ್ಳು_back(ಸಂದೇಶ); }

    ಶೂನ್ಯ output(ತಾರ್ಕಿಕ success) {
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
        ಪ್ರತಿ (ಧ್ರುವ VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

ಶೂನ್ಯ V3DiagSarif::ತಳ್ಳು_Message(ಧ್ರುವ VErrorMessage& ಸಂದೇಶ) VL_MT_DISABLED {
    ಯದಿ (!v3Global.opt.diagnosticsSarif()) ಪ್ರತ್ಯಾಗಮ;
    V3DiagSarifImp::s().ತಳ್ಳು_Message(ಸಂದೇಶ);
}

ಶೂನ್ಯ V3DiagSarif::output(ತಾರ್ಕಿಕ success) {
    ಯದಿ (!v3Global.opt.diagnosticsSarif()) ಪ್ರತ್ಯಾಗಮ;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
