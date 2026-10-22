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

جماعت V3DiagSarifImp final {
    // MEMBERS
    std::deque<VError_پیغام> m_messages;
    std::سیٹ<V3_Error_رمز> m_codes;
    std::map<V3_Error_رمز, صحیح> m_رمز_اشاریہ;

    // METHODS
    خالی calculate() {
        برائے (ثابت VError_پیغام& msgr : m_messages)
            اگر (msgr.رمز().isNamed()) m_codes.emplace(msgr.رمز());
        صحیح i = 0;
        برائے (ثابت V3_Error_رمز& رمز : m_codes) m_رمز_اشاریہ[رمز] = i++;
    }

    خالی put_قواعد(V3_برآمد_Json_File& کا) ثابت {
        برائے (ثابت V3_Error_رمز& رمز : m_codes) {
            کا.begin().put("id", رمز.ascii()).put("helpUri", رمز.url()).انتہا();
        }
    }

    خالی put_متن(V3_برآمد_Json_File& کا, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        کا.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    خالی putLocation(V3_برآمد_Json_File& کا, ثابت File_سطر* fl) ثابت {
        ثابت string filename = V3Os::filename_Real_راستہ(fl->filename());
        کا.begin("physicalLocation");
        کا.begin("artifactLocation");
        اگر (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            کا.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } ورنہ {
            کا.put("uri", "file://" + filename);
        }
        کا.انتہا()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->پہلا_Lineno())
            .put("startColumn", fl->پہلا_عمود());
        اگر (fl->پہلا_Lineno() != fl->آخری_Lineno()) کا.put("endLine", fl->آخری_Lineno());
        کا.put("endColumn", fl->آخری_عمود());
        کا.begin("snippit", '{');
        put_متن(کا, fl->pretty_ماخذ(), V3Error::strip_Meta_متن(fl->warnContextPrimary(), جھوٹ));
        کا.انتہا();  // snippit
        کا.انتہا();  // region
        کا.انتہا();  // artifactLocation
    }

    string substr_مقصد_اگلا_Related(ثابت string& str) {
        ثابت سائز_t pos = str.ڈھونڈو("__WARNRELATED(", 0);
        واپس (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    خالی put_نتیجہ(V3_برآمد_Json_File& کا, ثابت VError_پیغام& پیغام_2) {
        کا.begin();
        کا.put("level", پیغام_2.رمز().severityInfo()             ? "none"
                        : V3Error::isError(پیغام_2.رمز(), جھوٹ) ? "error"
                                                              : "warning");

        string متن = پیغام_2.متن();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string پہلا_clean = V3Error::strip_Meta_متن(متن, سچ);
        ثابت string پہلا_fmt = V3Error::strip_Meta_متن(متن, جھوٹ);

        کا.begin("message");
        put_متن(کا, پہلا_clean, پہلا_fmt);
        کا.انتہا();
        اگر (ثابت File_سطر* ثابت fl = پیغام_2.fileline()) {
            کا.begin("locations", '[').begin();
            putLocation(کا, fl);
            کا.انتہا();
            کا.انتہا();
        }

        اگر (متن.ڈھونڈو("__WARNRELATED(") != std::string::npos) {
            کا.begin("relatedLocations", '[');
            سائز_t pos = 0;
            جب ((pos = متن.ڈھونڈو("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت سائز_t آغاز = pos + std::لڑی_لمبائی("__WARNRELATED(");
                سائز_t انتہا = آغاز;
                جب (انتہا < متن.سائز() && متن[انتہا] != ')') ++انتہا;
                ثابت سائز_t اشاریہ = std::atoi(متن.substr(آغاز, انتہا + 1).c_str());
                اگر (انتہا < متن.سائز()) انتہا += std::لڑی_لمبائی(")");
                متن = متن.substr(انتہا, متن.سائز());
                UASSERT_STATIC(اشاریہ < پیغام_2.filelines().سائز(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::strip_Meta_متن(substr_مقصد_اگلا_Related(متن), سچ);
                ثابت string related_fmt
                    = V3Error::strip_Meta_متن(substr_مقصد_اگلا_Related(متن), جھوٹ);
                ثابت File_سطر* fl = پیغام_2.filelines()[اشاریہ];
                کا.begin().begin("message");
                put_متن(کا, related_clean, related_fmt);
                کا.انتہا();  //پیغام
                putLocation(کا, fl);
                کا.انتہا();
            }
            کا.انتہا();
        }

        اگر (پیغام_2.رمز().isNamed())
            کا.put("ruleId", پیغام_2.رمز().ascii()).put("ruleIndex", m_رمز_اشاریہ[پیغام_2.رمز()]);

        کا.انتہا();
    }

    // STATIC FUNCTIONS
عوامی:
    ساکن V3DiagSarifImp& s() {
        ساکن V3DiagSarifImp s_s;
        واپس s_s;
    }

    خالی ڈالو_پیغام(ثابت VError_پیغام& پیغام_2) { m_messages.ڈالو_عقب(پیغام_2); }

    خالی output(منطقی success) {
        V3_برآمد_Json_File کا{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        کا.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        کا.begin("runs", '[').begin();

        کا.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::نسخہ(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        کا.begin("rules", '[');
        put_قواعد(کا);
        کا.انتہا()  //قواعد
            .انتہا()  // driver
            .انتہا();  //tool

        کا.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_دلائل_String())
            .put("executionSuccessful", success)
            .انتہا()
            .انتہا();

        کا.begin("results", '[');
        برائے (ثابت VError_پیغام& msgr : m_messages) put_نتیجہ(کا, msgr);
        کا.انتہا();

        کا.انتہا();  // runs ]
        کا.انتہا();  // runs }
    }
};

خالی V3DiagSarif::ڈالو_پیغام(ثابت VError_پیغام& پیغام_2) VL_MT_DISABLED {
    اگر (!v3Global.opt.diagnosticsSarif()) واپس;
    V3DiagSarifImp::s().ڈالو_پیغام(پیغام_2);
}

خالی V3DiagSarif::output(منطقی success) {
    اگر (!v3Global.opt.diagnosticsSarif()) واپس;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
