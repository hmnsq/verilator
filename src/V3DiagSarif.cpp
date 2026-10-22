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
    std::deque<VError_سنیہا> m_messages;
    std::سیٹ<V3_Error_کوڈ> m_codes;
    std::map<V3_Error_کوڈ, صحيح> m_کوڈ_اشاریہ;

    // METHODS
    فراغ calculate() {
        لكل (ثابت VError_سنیہا& msgr : m_messages)
            إذا (msgr.کوڈ().isNamed()) m_codes.emplace(msgr.کوڈ());
        صحيح i = 0;
        لكل (ثابت V3_Error_کوڈ& کوڈ : m_codes) m_کوڈ_اشاریہ[کوڈ] = i++;
    }

    فراغ put_قاعدے(V3_باہرلا_Json_File& دا) ثابت {
        لكل (ثابت V3_Error_کوڈ& کوڈ : m_codes) {
            دا.begin().put("id", کوڈ.ascii()).put("helpUri", کوڈ.url()).سرا();
        }
    }

    فراغ put_متن(V3_باہرلا_Json_File& دا, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        دا.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    فراغ putLocation(V3_باہرلا_Json_File& دا, ثابت File_سطر* fl) ثابت {
        ثابت string filename = V3Os::filename_Real_رستہ(fl->filename());
        دا.begin("physicalLocation");
        دا.begin("artifactLocation");
        إذا (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            دا.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } وإلا {
            دا.put("uri", "file://" + filename);
        }
        دا.سرا()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->پہلا_Lineno())
            .put("startColumn", fl->پہلا_کالم());
        إذا (fl->پہلا_Lineno() != fl->آخری_Lineno()) دا.put("endLine", fl->آخری_Lineno());
        دا.put("endColumn", fl->آخری_کالم());
        دا.begin("snippit", '{');
        put_متن(دا, fl->pretty_سروت(), V3Error::strip_Meta_متن(fl->warnContextPrimary(), خطأ));
        دا.سرا();  // snippit
        دا.سرا();  // region
        دا.سرا();  // artifactLocation
    }

    string substr_منزل_اگلا_Related(ثابت string& str) {
        ثابت ناپ_t pos = str.لبھو("__WARNRELATED(", 0);
        أرجع (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    فراغ put_نتیجہ(V3_باہرلا_Json_File& دا, ثابت VError_سنیہا& پیغام) {
        دا.begin();
        دا.put("level", پیغام.کوڈ().severityInfo()             ? "none"
                        : V3Error::isError(پیغام.کوڈ(), خطأ) ? "error"
                                                              : "warning");

        string متن = پیغام.متن();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string پہلا_clean = V3Error::strip_Meta_متن(متن, سچ);
        ثابت string پہلا_fmt = V3Error::strip_Meta_متن(متن, خطأ);

        دا.begin("message");
        put_متن(دا, پہلا_clean, پہلا_fmt);
        دا.سرا();
        إذا (ثابت File_سطر* ثابت fl = پیغام.fileline()) {
            دا.begin("locations", '[').begin();
            putLocation(دا, fl);
            دا.سرا();
            دا.سرا();
        }

        إذا (متن.لبھو("__WARNRELATED(") != std::string::npos) {
            دا.begin("relatedLocations", '[');
            ناپ_t pos = 0;
            طالما ((pos = متن.لبھو("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت ناپ_t شروع = pos + std::لڑی_لمبائی("__WARNRELATED(");
                ناپ_t سرا = شروع;
                طالما (سرا < متن.ناپ() && متن[سرا] != ')') ++سرا;
                ثابت ناپ_t اشاریہ = std::atoi(متن.substr(شروع, سرا + 1).c_str());
                إذا (سرا < متن.ناپ()) سرا += std::لڑی_لمبائی(")");
                متن = متن.substr(سرا, متن.ناپ());
                UASSERT_STATIC(اشاریہ < پیغام.filelines().ناپ(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::strip_Meta_متن(substr_منزل_اگلا_Related(متن), سچ);
                ثابت string related_fmt
                    = V3Error::strip_Meta_متن(substr_منزل_اگلا_Related(متن), خطأ);
                ثابت File_سطر* fl = پیغام.filelines()[اشاریہ];
                دا.begin().begin("message");
                put_متن(دا, related_clean, related_fmt);
                دا.سرا();  //سنیہا
                putLocation(دا, fl);
                دا.سرا();
            }
            دا.سرا();
        }

        إذا (پیغام.کوڈ().isNamed())
            دا.put("ruleId", پیغام.کوڈ().ascii()).put("ruleIndex", m_کوڈ_اشاریہ[پیغام.کوڈ()]);

        دا.سرا();
    }

    // STATIC FUNCTIONS
عام:
    ساكن V3DiagSarifImp& s() {
        ساكن V3DiagSarifImp s_s;
        أرجع s_s;
    }

    فراغ ڈالو_سنیہا(ثابت VError_سنیہا& پیغام) { m_messages.ڈالو_پٹھ(پیغام); }

    فراغ output(منطقي success) {
        V3_باہرلا_Json_File دا{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        دا.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        دا.begin("runs", '[').begin();

        دا.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::نسخہ(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        دا.begin("rules", '[');
        put_قاعدے(دا);
        دا.سرا()  //قاعدے
            .سرا()  // driver
            .سرا();  //tool

        دا.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_دلیلاں_String())
            .put("executionSuccessful", success)
            .سرا()
            .سرا();

        دا.begin("results", '[');
        لكل (ثابت VError_سنیہا& msgr : m_messages) put_نتیجہ(دا, msgr);
        دا.سرا();

        دا.سرا();  // runs ]
        دا.سرا();  // runs }
    }
};

فراغ V3DiagSarif::ڈالو_سنیہا(ثابت VError_سنیہا& پیغام) VL_MT_DISABLED {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    V3DiagSarifImp::s().ڈالو_سنیہا(پیغام);
}

فراغ V3DiagSarif::output(منطقي success) {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
