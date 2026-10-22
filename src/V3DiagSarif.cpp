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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3DiagSarif.h"

#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

வகுப்பு V3DiagSarifImp final {
    // MEMBERS
    std::deque<VErrorMessage> m_messages;
    std::set<V3ErrorCode> m_codes;
    std::map<V3ErrorCode, முழு> m_codeIndex;

    // METHODS
    வெறுமை calculate() {
        க்கு (மாறிலி VErrorMessage& msgr : m_messages)
            எனில் (msgr.code().isNamed()) m_codes.emplace(msgr.code());
        முழு i = 0;
        க்கு (மாறிலி V3ErrorCode& code : m_codes) m_codeIndex[code] = i++;
    }

    வெறுமை putRules(V3OutJsonFile& of) மாறிலி {
        க்கு (மாறிலி V3ErrorCode& code : m_codes) {
            of.begin().put("id", code.ascii()).put("helpUri", code.url()).end();
        }
    }

    வெறுமை putText(V3OutJsonFile& of, மாறிலி string& clean, மாறிலி string& formatted) மாறிலி {
        மாறிலி string markdown = "```\n" + formatted + "\n```\n";
        of.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    வெறுமை putLocation(V3OutJsonFile& of, மாறிலி File_வரி* fl) மாறிலி {
        மாறிலி string filename = V3Os::filenameRealPath(fl->filename());
        of.begin("physicalLocation");
        of.begin("artifactLocation");
        எனில் (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            of.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } இல்லையெனில் {
            of.put("uri", "file://" + filename);
        }
        of.end()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->firstLineno())
            .put("startColumn", fl->firstColumn());
        எனில் (fl->firstLineno() != fl->lastLineno()) of.put("endLine", fl->lastLineno());
        of.put("endColumn", fl->lastColumn());
        of.begin("snippit", '{');
        putText(of, fl->prettySource(), V3Error::stripMetaText(fl->warnContextPrimary(), பொய்));
        of.end();  // snippit
        of.end();  // region
        of.end();  // artifactLocation
    }

    string substrToNextRelated(மாறிலி string& str) {
        மாறிலி size_t pos = str.find("__WARNRELATED(", 0);
        திரும்பு (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    வெறுமை putResult(V3OutJsonFile& of, மாறிலி VErrorMessage& செய்தி) {
        of.begin();
        of.put("level", செய்தி.code().severityInfo()             ? "none"
                        : V3Error::isError(செய்தி.code(), பொய்) ? "error"
                                                              : "warning");

        string text = செய்தி.text();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        மாறிலி string first_clean = V3Error::stripMetaText(text, உண்மை);
        மாறிலி string first_fmt = V3Error::stripMetaText(text, பொய்);

        of.begin("message");
        putText(of, first_clean, first_fmt);
        of.end();
        எனில் (மாறிலி File_வரி* மாறிலி fl = செய்தி.fileline()) {
            of.begin("locations", '[').begin();
            putLocation(of, fl);
            of.end();
            of.end();
        }

        எனில் (text.find("__WARNRELATED(") != std::string::npos) {
            of.begin("relatedLocations", '[');
            size_t pos = 0;
            போது ((pos = text.find("__WARNRELATED(", pos)) != std::string::npos) {
                மாறிலி size_t start = pos + std::strlen("__WARNRELATED(");
                size_t end = start;
                போது (end < text.size() && text[end] != ')') ++end;
                மாறிலி size_t index = std::atoi(text.substr(start, end + 1).c_str());
                எனில் (end < text.size()) end += std::strlen(")");
                text = text.substr(end, text.size());
                UASSERT_STATIC(index < செய்தி.filelines().size(),
                               "Error message warnRelated without fileline");

                மாறிலி string related_clean
                    = V3Error::stripMetaText(substrToNextRelated(text), உண்மை);
                மாறிலி string related_fmt
                    = V3Error::stripMetaText(substrToNextRelated(text), பொய்);
                மாறிலி File_வரி* fl = செய்தி.filelines()[index];
                of.begin().begin("message");
                putText(of, related_clean, related_fmt);
                of.end();  // message
                putLocation(of, fl);
                of.end();
            }
            of.end();
        }

        எனில் (செய்தி.code().isNamed())
            of.put("ruleId", செய்தி.code().ascii()).put("ruleIndex", m_codeIndex[செய்தி.code()]);

        of.end();
    }

    // STATIC FUNCTIONS
பொது:
    நிலையான V3DiagSarifImp& s() {
        நிலையான V3DiagSarifImp s_s;
        திரும்பு s_s;
    }

    வெறுமை தள்ளு_Message(மாறிலி VErrorMessage& செய்தி) { m_messages.தள்ளு_back(செய்தி); }

    வெறுமை output(தருக்க success) {
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
        க்கு (மாறிலி VErrorMessage& msgr : m_messages) putResult(of, msgr);
        of.end();

        of.end();  // runs ]
        of.end();  // runs }
    }
};

வெறுமை V3DiagSarif::தள்ளு_Message(மாறிலி VErrorMessage& செய்தி) VL_MT_DISABLED {
    எனில் (!v3Global.opt.diagnosticsSarif()) திரும்பு;
    V3DiagSarifImp::s().தள்ளு_Message(செய்தி);
}

வெறுமை V3DiagSarif::output(தருக்க success) {
    எனில் (!v3Global.opt.diagnosticsSarif()) திரும்பு;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
