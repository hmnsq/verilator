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
    std::deque<VError_پيغام> m_messages;
    std::سيٽ<V3_Error_ڪوڊ> m_codes;
    std::map<V3_Error_ڪوڊ, صحيح> m_ڪوڊ_اشاريو;

    // METHODS
    فراغ calculate() {
        لكل (ثابت VError_پيغام& msgr : m_messages)
            إذا (msgr.ڪوڊ().isNamed()) m_codes.emplace(msgr.ڪوڊ());
        صحيح i = 0;
        لكل (ثابت V3_Error_ڪوڊ& ڪوڊ : m_codes) m_ڪوڊ_اشاريو[ڪوڊ] = i++;
    }

    فراغ put_قاعدا(V3_ٻاهريون_Json_File& جو) ثابت {
        لكل (ثابت V3_Error_ڪوڊ& ڪوڊ : m_codes) {
            جو.begin().put("id", ڪوڊ.ascii()).put("helpUri", ڪوڊ.url()).پڇاڙي();
        }
    }

    فراغ put_متن(V3_ٻاهريون_Json_File& جو, ثابت string& clean, ثابت string& formatted) ثابت {
        ثابت string markdown = "```\n" + formatted + "\n```\n";
        جو.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    فراغ putLocation(V3_ٻاهريون_Json_File& جو, ثابت File_سٽ* fl) ثابت {
        ثابت string filename = V3Os::filename_Real_رستو(fl->filename());
        جو.begin("physicalLocation");
        جو.begin("artifactLocation");
        إذا (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            جو.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } وإلا {
            جو.put("uri", "file://" + filename);
        }
        جو.پڇاڙي()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->پهريون_Lineno())
            .put("startColumn", fl->پهريون_ڪالم());
        إذا (fl->پهريون_Lineno() != fl->آخري_Lineno()) جو.put("endLine", fl->آخري_Lineno());
        جو.put("endColumn", fl->آخري_ڪالم());
        جو.begin("snippit", '{');
        put_متن(جو, fl->pretty_ذريعو(), V3Error::strip_Meta_متن(fl->warnContextPrimary(), خطأ));
        جو.پڇاڙي();  // snippit
        جو.پڇاڙي();  // region
        جو.پڇاڙي();  // artifactLocation
    }

    string substr_منزل_ايندڙ_Related(ثابت string& str) {
        ثابت سائيز_t pos = str.ڳوليو("__WARNRELATED(", 0);
        أرجع (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    فراغ put_نتيجو(V3_ٻاهريون_Json_File& جو, ثابت VError_پيغام& نياپو) {
        جو.begin();
        جو.put("level", نياپو.ڪوڊ().severityInfo()             ? "none"
                        : V3Error::isError(نياپو.ڪوڊ(), خطأ) ? "error"
                                                              : "warning");

        string متن = نياپو.متن();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ثابت string پهريون_clean = V3Error::strip_Meta_متن(متن, سچ);
        ثابت string پهريون_fmt = V3Error::strip_Meta_متن(متن, خطأ);

        جو.begin("message");
        put_متن(جو, پهريون_clean, پهريون_fmt);
        جو.پڇاڙي();
        إذا (ثابت File_سٽ* ثابت fl = نياپو.fileline()) {
            جو.begin("locations", '[').begin();
            putLocation(جو, fl);
            جو.پڇاڙي();
            جو.پڇاڙي();
        }

        إذا (متن.ڳوليو("__WARNRELATED(") != std::string::npos) {
            جو.begin("relatedLocations", '[');
            سائيز_t pos = 0;
            طالما ((pos = متن.ڳوليو("__WARNRELATED(", pos)) != std::string::npos) {
                ثابت سائيز_t شروعات = pos + std::لڙي_ڊيگهه("__WARNRELATED(");
                سائيز_t پڇاڙي = شروعات;
                طالما (پڇاڙي < متن.سائيز() && متن[پڇاڙي] != ')') ++پڇاڙي;
                ثابت سائيز_t اشاريو = std::atoi(متن.substr(شروعات, پڇاڙي + 1).c_str());
                إذا (پڇاڙي < متن.سائيز()) پڇاڙي += std::لڙي_ڊيگهه(")");
                متن = متن.substr(پڇاڙي, متن.سائيز());
                UASSERT_STATIC(اشاريو < نياپو.filelines().سائيز(),
                               "Error message warnRelated without fileline");

                ثابت string related_clean
                    = V3Error::strip_Meta_متن(substr_منزل_ايندڙ_Related(متن), سچ);
                ثابت string related_fmt
                    = V3Error::strip_Meta_متن(substr_منزل_ايندڙ_Related(متن), خطأ);
                ثابت File_سٽ* fl = نياپو.filelines()[اشاريو];
                جو.begin().begin("message");
                put_متن(جو, related_clean, related_fmt);
                جو.پڇاڙي();  //پيغام
                putLocation(جو, fl);
                جو.پڇاڙي();
            }
            جو.پڇاڙي();
        }

        إذا (نياپو.ڪوڊ().isNamed())
            جو.put("ruleId", نياپو.ڪوڊ().ascii()).put("ruleIndex", m_ڪوڊ_اشاريو[نياپو.ڪوڊ()]);

        جو.پڇاڙي();
    }

    // STATIC FUNCTIONS
عام:
    ساكن V3DiagSarifImp& s() {
        ساكن V3DiagSarifImp s_s;
        أرجع s_s;
    }

    فراغ ڌڪو_پيغام(ثابت VError_پيغام& نياپو) { m_messages.ڌڪو_پٺ(نياپو); }

    فراغ output(منطقي success) {
        V3_ٻاهريون_Json_File جو{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        جو.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        جو.begin("runs", '[').begin();

        جو.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::نسخو(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        جو.begin("rules", '[');
        put_قاعدا(جو);
        جو.پڇاڙي()  //قاعدا
            .پڇاڙي()  // driver
            .پڇاڙي();  //tool

        جو.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_دليلون_String())
            .put("executionSuccessful", success)
            .پڇاڙي()
            .پڇاڙي();

        جو.begin("results", '[');
        لكل (ثابت VError_پيغام& msgr : m_messages) put_نتيجو(جو, msgr);
        جو.پڇاڙي();

        جو.پڇاڙي();  // runs ]
        جو.پڇاڙي();  // runs }
    }
};

فراغ V3DiagSarif::ڌڪو_پيغام(ثابت VError_پيغام& نياپو) VL_MT_DISABLED {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    V3DiagSarifImp::s().ڌڪو_پيغام(نياپو);
}

فراغ V3DiagSarif::output(منطقي success) {
    إذا (!v3Global.opt.diagnosticsSarif()) أرجع;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
