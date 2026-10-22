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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3DiagSarif.h"

#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

વર્ગ V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, પૂર્ણાંક> m_codeIndex;

    // METHODS
    શૂન્ય calculate() {
        માટે (અચલ VErrorMessage& msgr : m_messages)
            જો (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        પૂર્ણાંક i = 0;
        માટે (અચલ V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    શૂન્ય putRules(V3OutJsonFile& of) અચલ {
        માટે (અચલ V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    શૂન્ય putText(V3OutJsonFile& of, અચલ string& clean, અચલ string& formatted) અચલ {
        અચલ string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    શૂન્ય putLocation(V3OutJsonFile& of, અચલ File_લીટી* fl) અચલ {
        અચલ string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        જો (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } નહીંતર {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        જો (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), ખોટું));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(અચલ string& str) {
        અચલ size_t pos = str.find("__WARNRELATED(", 0);
        પાછા (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    શૂન્ય putResult(V3OutJsonFile& of, અચલ VErrorMessage& સંદેશ) {
        of.begin();
        of.put("level", સંદેશ.code().severityInfo()             ? "none"
                        : V3Error::isError(સંદેશ.code(), ખોટું) ? "error"
                                                              : "warning");

        string text = સંદેશ.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        અચલ string first_clean = V3Error::stripMetaText(text, સાચું);
        અચલ string first_fmt = V3Error::stripMetaText(text, ખોટું);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        જો (અચલ File_લીટી* અચલ fl = સંદેશ.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        જો (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            જ્યારે ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                અચલ size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                જ્યારે (end < text.size() && text[end] != ')') ++end;
                અચલ size_t index = std::atoi(text.substr(start, end + 1).c_str());
                જો (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < સંદેશ.filelines().size(),
                               "Error message warnRelated without fileline");

                અચલ string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), સાચું);
                અચલ string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), ખોટું);
                અચલ File_લીટી* fl = સંદેશ.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        જો (સંદેશ.code().isNamed())
            of.put("ruleId", સંદેશ.code().ascii()).put("ruleIndex", m_codeIndex[સંદેશ.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
જાહેર:
    સ્થિર V3DiagSarifImp& s() {
        સ્થિર V3DiagSarifImp s_s;
        પાછા s_s;
    }

    શૂન્ય ધકેલો_Message(અચલ VErrorMessage& સંદેશ) { m_messages.ધકેલો_back(સંદેશ); }

    શૂન્ય output(તાર્કિક success) {
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
        માટે (અચલ VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

શૂન્ય V3DiagSarif::ધકેલો_Message(અચલ VErrorMessage& સંદેશ) VL_MT_DISABLED {
    જો (!v3Global.opt.diagnosticsSarif()) પાછા;
    V3DiagSarifImp::s().ધકેલો_Message(સંદેશ);
}

શૂન્ય V3DiagSarif::output(તાર્કિક success) {
    જો (!v3Global.opt.diagnosticsSarif()) પાછા;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
