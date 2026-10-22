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
    std::deque<VError_پیام> m_messages;
    std::مجموعه<V3_Error_کد> m_codes;
    std::map<V3_Error_کد, صحيح> m_کد_شاخص;

    // METHODS
    فراغ calculate() {
        لكل (ثابت VError_پیام& msgr : m_messages)
            إذا (msgr.کد().isNamed()) m_codes.emplace(msgr.کد());
        صحيح i = 0;
        لكل (ثابت V3_Error_کد& کد : m_codes) m_کد_شاخص[کد] = i++;
    }

    فراغ put_قواعد(V3_بیرون_Json_File& از) ثابت {
        لكل (ثابت V3_Error_کد& کد : m_codes) {
            از.begin().put("id", کد.ascii()).put("helpUri", کد.url()).انتها();
        }
    }

    فراغ put_متن(V3_بیرون_Json_File& از, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        از.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    فراغ putLocation(V3_بیرون_Json_File& از, ثابت File_سطر* fl) ثابت {
        ثابت string filename = V3Os::filename_Real_مسیر(fl->filename());
        از.begin("physicalLocation");
        از.begin("artifactLocation");
        إذا (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            از.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } وإلا {
            از.put("uri", "file://" + filename);
        }
        از.انتها()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->اولی_Lineno())
            .put("startColumn", fl->اولی_ستونی());
        إذا (fl->اولی_Lineno() != fl->آخرین_Lineno()) از.put("endLine", fl->آخرین_Lineno());
        از.put("endColumn", fl->آخرین_ستونی());
        از.begin("snippit", '{');
        put_متن(از, fl->pretty_منبع(), V3Error::strip_Meta_متن(fl->warnContextPrimary(), خطأ));
        از.انتها();  // snippit
        از.انتها();  // region
        از.انتها();  // artifactLocation
    }

    string substr_مقصد_بعدی_Related(ثابت string& str) {
        ثابت اندازه_t pos = str.پیدا_کن("__WARNRELATED(", 0);
        أرجع (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    فراغ put_نتیجه(V3_بیرون_Json_File& از, ثابت VError_پیام& پیام_2) {
        از.begin();
        از.put("level", پیام_2.کد().severityInfo()             ? "none"
                        : V3Error::isError(پیام_2.کد(), خطأ) ? "error"
                                                              : "warning");

        string متن = پیام_2.متن();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string اولی_clean = V3Error::strip_Meta_متن(متن, درست);
        ثابت string اولی_fmt = V3Error::strip_Meta_متن(متن, خطأ);

        از.begin("message");
        put_متن(از, اولی_clean, اولی_fmt);
        از.انتها();
        إذا (ثابت File_سطر* ثابت fl = پیام_2.fileline()) {
            از.begin("locations", '[').begin();
            putLocation(از, fl);
            از.انتها();
            از.انتها();
        }

        إذا (متن.پیدا_کن("__WARNRELATED(") != std::string::npos) {
            از.begin("relatedLocations", '[');
            اندازه_t pos = 0;
            طالما ((pos = متن.پیدا_کن("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت اندازه_t شروع = pos + std::طول_رشته("__WARNRELATED(");
                اندازه_t انتها = شروع;
                طالما (انتها < متن.اندازه() && متن[انتها] != ')') ++انتها;
                ثابت اندازه_t شاخص = std::atoi(متن.substr(شروع, انتها + 1).c_str());
                إذا (انتها < متن.اندازه()) انتها += std::طول_رشته(")");
                متن = متن.substr(انتها, متن.اندازه());
                UASSERT_STATIC(شاخص < پیام_2.filelines().اندازه(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::strip_Meta_متن(substr_مقصد_بعدی_Related(متن), درست);
                ثابت string related_fmt
                    = V3Error::strip_Meta_متن(substr_مقصد_بعدی_Related(متن), خطأ);
                ثابت File_سطر* fl = پیام_2.filelines()[شاخص];
                از.begin().begin("message");
                put_متن(از, related_clean, related_fmt);
                از.انتها();  //پیام
                putLocation(از, fl);
                از.انتها();
            }
            از.انتها();
        }

        إذا (پیام_2.کد().isNamed())
            از.put("ruleId", پیام_2.کد().ascii()).put("ruleIndex", m_کد_شاخص[پیام_2.کد()]);

        از.انتها();
    }

    // STATIC FUNCTIONS
عام:
    ساكن V3DiagSarifImp& s() {
        ساكن V3DiagSarifImp s_s;
        أرجع s_s;
    }

    فراغ افزودن_پیام(ثابت VError_پیام& پیام_2) { m_messages.افزودن_پشت(پیام_2); }

    فراغ output(منطقي success) {
        V3_بیرون_Json_File از{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        از.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        از.begin("runs", '[').begin();

        از.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::نسخه(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        از.begin("rules", '[');
        put_قواعد(از);
        از.انتها()  //قواعد
            .انتها()  // driver
            .انتها();  //tool

        از.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_آرگومانها_String())
            .put("executionSuccessful", success)
            .انتها()
            .انتها();

        از.begin("results", '[');
        لكل (ثابت VError_پیام& msgr : m_messages) put_نتیجه(از, msgr);
        از.انتها();

        از.انتها();  // runs ]
        از.انتها();  // runs }
    }
};

فراغ V3DiagSarif::افزودن_پیام(ثابت VError_پیام& پیام_2) VL_MT_DISABLED {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    V3DiagSarifImp::s().افزودن_پیام(پیام_2);
}

فراغ V3DiagSarif::output(منطقي success) {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
