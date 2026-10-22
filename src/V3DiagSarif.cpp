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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DiagSarif.h"

#ضمن "V3File.h"
#ضمن "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

صنف V3DiagSarifImp final {
    // MEMBERS
    std::deque<VError_پیغام> m_messages;
    std::سېټ<V3_Error_کوډ> m_codes;
    std::map<V3_Error_کوډ, صحيح> m_کوډ_شاخص;

    // METHODS
    فراغ calculate() {
        لكل (ثابت VError_پیغام& msgr : m_messages)
            إذا (msgr.کوډ().isNamed()) m_codes.emplace(msgr.کوډ());
        صحيح i = 0;
        لكل (ثابت V3_Error_کوډ& کوډ : m_codes) m_کوډ_شاخص[کوډ] = i++;
    }

    فراغ put_قواعد(V3_بهر_Json_File& د) ثابت {
        لكل (ثابت V3_Error_کوډ& کوډ : m_codes) {
            د.begin().put("id", کوډ.ascii()).put("helpUri", کوډ.url()).پای_ټکی();
        }
    }

    فراغ put_متن(V3_بهر_Json_File& د, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        د.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    فراغ putLocation(V3_بهر_Json_File& د, ثابت File_کرښه* fl) ثابت {
        ثابت string filename = V3Os::filename_Real_لار(fl->filename());
        د.begin("physicalLocation");
        د.begin("artifactLocation");
        إذا (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            د.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } وإلا {
            د.put("uri", "file://" + filename);
        }
        د.پای_ټکی()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->لومړی_Lineno())
            .put("startColumn", fl->لومړی_ستنه());
        إذا (fl->لومړی_Lineno() != fl->وروستی_Lineno()) د.put("endLine", fl->وروستی_Lineno());
        د.put("endColumn", fl->وروستی_ستنه());
        د.begin("snippit", '{');
        put_متن(د, fl->pretty_سرچینه(), V3Error::strip_Meta_متن(fl->warnContextPrimary(), خطأ));
        د.پای_ټکی();  // snippit
        د.پای_ټکی();  // region
        د.پای_ټکی();  // artifactLocation
    }

    string substr_منزل_بل_Related(ثابت string& str) {
        ثابت اندازه_t pos = str.پیدا_کړه("__WARNRELATED(", 0);
        أرجع (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    فراغ put_پایله(V3_بهر_Json_File& د, ثابت VError_پیغام& پیغام_2) {
        د.begin();
        د.put("level", پیغام_2.کوډ().severityInfo()             ? "none"
                        : V3Error::isError(پیغام_2.کوډ(), خطأ) ? "error"
                                                              : "warning");

        string متن = پیغام_2.متن();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string لومړی_clean = V3Error::strip_Meta_متن(متن, ریښتیا);
        ثابت string لومړی_fmt = V3Error::strip_Meta_متن(متن, خطأ);

        د.begin("message");
        put_متن(د, لومړی_clean, لومړی_fmt);
        د.پای_ټکی();
        إذا (ثابت File_کرښه* ثابت fl = پیغام_2.fileline()) {
            د.begin("locations", '[').begin();
            putLocation(د, fl);
            د.پای_ټکی();
            د.پای_ټکی();
        }

        إذا (متن.پیدا_کړه("__WARNRELATED(") != std::string::npos) {
            د.begin("relatedLocations", '[');
            اندازه_t pos = 0;
            طالما ((pos = متن.پیدا_کړه("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت اندازه_t پیل = pos + std::مزي_اوږدوالی("__WARNRELATED(");
                اندازه_t پای_ټکی = پیل;
                طالما (پای_ټکی < متن.اندازه() && متن[پای_ټکی] != ')') ++پای_ټکی;
                ثابت اندازه_t شاخص = std::atoi(متن.substr(پیل, پای_ټکی + 1).c_str());
                إذا (پای_ټکی < متن.اندازه()) پای_ټکی += std::مزي_اوږدوالی(")");
                متن = متن.substr(پای_ټکی, متن.اندازه());
                UASSERT_STATIC(شاخص < پیغام_2.filelines().اندازه(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::strip_Meta_متن(substr_منزل_بل_Related(متن), ریښتیا);
                ثابت string related_fmt
                    = V3Error::strip_Meta_متن(substr_منزل_بل_Related(متن), خطأ);
                ثابت File_کرښه* fl = پیغام_2.filelines()[شاخص];
                د.begin().begin("message");
                put_متن(د, related_clean, related_fmt);
                د.پای_ټکی();  //پیغام
                putLocation(د, fl);
                د.پای_ټکی();
            }
            د.پای_ټکی();
        }

        إذا (پیغام_2.کوډ().isNamed())
            د.put("ruleId", پیغام_2.کوډ().ascii()).put("ruleIndex", m_کوډ_شاخص[پیغام_2.کوډ()]);

        د.پای_ټکی();
    }

    // STATIC FUNCTIONS
عام:
    ساكن V3DiagSarifImp& s() {
        ساكن V3DiagSarifImp s_s;
        أرجع s_s;
    }

    فراغ ټېل_پیغام(ثابت VError_پیغام& پیغام_2) { m_messages.ټېل_شا(پیغام_2); }

    فراغ output(منطقي success) {
        V3_بهر_Json_File د{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        د.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        د.begin("runs", '[').begin();

        د.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::نسخه(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        د.begin("rules", '[');
        put_قواعد(د);
        د.پای_ټکی()  //قواعد
            .پای_ټکی()  // driver
            .پای_ټکی();  //tool

        د.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_دلیلونه_String())
            .put("executionSuccessful", success)
            .پای_ټکی()
            .پای_ټکی();

        د.begin("results", '[');
        لكل (ثابت VError_پیغام& msgr : m_messages) put_پایله(د, msgr);
        د.پای_ټکی();

        د.پای_ټکی();  // runs ]
        د.پای_ټکی();  // runs }
    }
};

فراغ V3DiagSarif::ټېل_پیغام(ثابت VError_پیغام& پیغام_2) VL_MT_DISABLED {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    V3DiagSarifImp::s().ټېل_پیغام(پیغام_2);
}

فراغ V3DiagSarif::output(منطقي success) {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
