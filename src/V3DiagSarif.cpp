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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3DiagSarif.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ਵਰਗ V3DiagSarifImp final {
    // MEMBERS
    std::deque<VError_ਸੁਨੇਹਾ> m_messages;
    std::ਸੈੱਟ<V3_Error_ਕੋਡ> m_codes;
    std::map<V3_Error_ਕੋਡ, ਪੂਰਨ> m_ਕੋਡ_ਸੂਚਕਾਂਕ;

    // METHODS
    ਖਾਲੀ calculate() {
        ਲਈ (ਅਚਲ VError_ਸੁਨੇਹਾ& msgr : m_messages)
            ਜੇ (msgr.ਕੋਡ().isNamed()) m_codes.emplace(msgr.ਕੋਡ());
        ਪੂਰਨ i = 0;
        ਲਈ (ਅਚਲ V3_Error_ਕੋਡ& ਕੋਡ : m_codes) m_ਕੋਡ_ਸੂਚਕਾਂਕ[ਕੋਡ] = i++;
    }

    ਖਾਲੀ put_ਨਿਯਮਾਂ(V3_ਬਾਹਰਲਾ_Json_File& ਦਾ) ਅਚਲ {
        ਲਈ (ਅਚਲ V3_Error_ਕੋਡ& ਕੋਡ : m_codes) {
            ਦਾ.begin().put("id", ਕੋਡ.ascii()).put("helpUri", ਕੋਡ.url()).ਸਿਰਾ();
        }
    }

    ਖਾਲੀ put_ਪਾਠ(V3_ਬਾਹਰਲਾ_Json_File& ਦਾ, ਅਚਲ string& clean, ਅਚਲ string& formatted) ਅਚਲ {
        ਅਚਲ string markdown = "```\n" + formatted + "\n```\n";
        ਦਾ.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    ਖਾਲੀ putLocation(V3_ਬਾਹਰਲਾ_Json_File& ਦਾ, ਅਚਲ File_ਸਤਰ* fl) ਅਚਲ {
        ਅਚਲ string filename = V3Os::filename_Real_ਰਸਤਾ(fl->filename());
        ਦਾ.begin("physicalLocation");
        ਦਾ.begin("artifactLocation");
        ਜੇ (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            ਦਾ.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } ਨਹੀਂਤਾਂ {
            ਦਾ.put("uri", "file://" + filename);
        }
        ਦਾ.ਸਿਰਾ()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->ਪਹਿਲਾ_Lineno())
            .put("startColumn", fl->ਪਹਿਲਾ_ਕਾਲਮ());
        ਜੇ (fl->ਪਹਿਲਾ_Lineno() != fl->ਆਖਰੀ_Lineno()) ਦਾ.put("endLine", fl->ਆਖਰੀ_Lineno());
        ਦਾ.put("endColumn", fl->ਆਖਰੀ_ਕਾਲਮ());
        ਦਾ.begin("snippit", '{');
        put_ਪਾਠ(ਦਾ, fl->pretty_ਸਰੋਤ(), V3Error::strip_Meta_ਪਾਠ(fl->warnContextPrimary(), ਝੂਠ));
        ਦਾ.ਸਿਰਾ();  // snippit
        ਦਾ.ਸਿਰਾ();  // region
        ਦਾ.ਸਿਰਾ();  // artifactLocation
    }

    string substr_ਮੰਜ਼ਿਲ_ਅਗਲਾ_Related(ਅਚਲ string& str) {
        ਅਚਲ ਮਾਪ_t pos = str.ਲੱਭੋ("__WARNRELATED(", 0);
        ਵਾਪਸ (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    ਖਾਲੀ put_ਨਤੀਜਾ(V3_ਬਾਹਰਲਾ_Json_File& ਦਾ, ਅਚਲ VError_ਸੁਨੇਹਾ& ਸੁਨੇਹਾ_2) {
        ਦਾ.begin();
        ਦਾ.put("level", ਸੁਨੇਹਾ_2.ਕੋਡ().severityInfo()             ? "none"
                        : V3Error::isError(ਸੁਨੇਹਾ_2.ਕੋਡ(), ਝੂਠ) ? "error"
                                                              : "warning");

        string ਪਾਠ = ਸੁਨੇਹਾ_2.ਪਾਠ();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ਅਚਲ string ਪਹਿਲਾ_clean = V3Error::strip_Meta_ਪਾਠ(ਪਾਠ, ਸੱਚ);
        ਅਚਲ string ਪਹਿਲਾ_fmt = V3Error::strip_Meta_ਪਾਠ(ਪਾਠ, ਝੂਠ);

        ਦਾ.begin("message");
        put_ਪਾਠ(ਦਾ, ਪਹਿਲਾ_clean, ਪਹਿਲਾ_fmt);
        ਦਾ.ਸਿਰਾ();
        ਜੇ (ਅਚਲ File_ਸਤਰ* ਅਚਲ fl = ਸੁਨੇਹਾ_2.fileline()) {
            ਦਾ.begin("locations", '[').begin();
            putLocation(ਦਾ, fl);
            ਦਾ.ਸਿਰਾ();
            ਦਾ.ਸਿਰਾ();
        }

        ਜੇ (ਪਾਠ.ਲੱਭੋ("__WARNRELATED(") != std::string::npos) {
            ਦਾ.begin("relatedLocations", '[');
            ਮਾਪ_t pos = 0;
            ਜਦੋਂ ((pos = ਪਾਠ.ਲੱਭੋ("__WARNRELATED(", pos)) != std::string::npos) {
                ਅਚਲ ਮਾਪ_t ਸ਼ੁਰੂ = pos + std::ਲੜੀ_ਲੰਬਾਈ("__WARNRELATED(");
                ਮਾਪ_t ਸਿਰਾ = ਸ਼ੁਰੂ;
                ਜਦੋਂ (ਸਿਰਾ < ਪਾਠ.ਮਾਪ() && ਪਾਠ[ਸਿਰਾ] != ')') ++ਸਿਰਾ;
                ਅਚਲ ਮਾਪ_t ਸੂਚਕਾਂਕ = std::atoi(ਪਾਠ.substr(ਸ਼ੁਰੂ, ਸਿਰਾ + 1).c_str());
                ਜੇ (ਸਿਰਾ < ਪਾਠ.ਮਾਪ()) ਸਿਰਾ += std::ਲੜੀ_ਲੰਬਾਈ(")");
                ਪਾਠ = ਪਾਠ.substr(ਸਿਰਾ, ਪਾਠ.ਮਾਪ());
                UASSERT_STATIC(ਸੂਚਕਾਂਕ < ਸੁਨੇਹਾ_2.filelines().ਮਾਪ(),
                               "Error message warnRelated without fileline");

                ਅਚਲ string related_clean
                    = V3Error::strip_Meta_ਪਾਠ(substr_ਮੰਜ਼ਿਲ_ਅਗਲਾ_Related(ਪਾਠ), ਸੱਚ);
                ਅਚਲ string related_fmt
                    = V3Error::strip_Meta_ਪਾਠ(substr_ਮੰਜ਼ਿਲ_ਅਗਲਾ_Related(ਪਾਠ), ਝੂਠ);
                ਅਚਲ File_ਸਤਰ* fl = ਸੁਨੇਹਾ_2.filelines()[ਸੂਚਕਾਂਕ];
                ਦਾ.begin().begin("message");
                put_ਪਾਠ(ਦਾ, related_clean, related_fmt);
                ਦਾ.ਸਿਰਾ();  //ਸੁਨੇਹਾ
                putLocation(ਦਾ, fl);
                ਦਾ.ਸਿਰਾ();
            }
            ਦਾ.ਸਿਰਾ();
        }

        ਜੇ (ਸੁਨੇਹਾ_2.ਕੋਡ().isNamed())
            ਦਾ.put("ruleId", ਸੁਨੇਹਾ_2.ਕੋਡ().ascii()).put("ruleIndex", m_ਕੋਡ_ਸੂਚਕਾਂਕ[ਸੁਨੇਹਾ_2.ਕੋਡ()]);

        ਦਾ.ਸਿਰਾ();
    }

    // STATIC FUNCTIONS
ਜਨਤਕ:
    ਸਥਿਰ V3DiagSarifImp& s() {
        ਸਥਿਰ V3DiagSarifImp s_s;
        ਵਾਪਸ s_s;
    }

    ਖਾਲੀ ਧੱਕੋ_ਸੁਨੇਹਾ(ਅਚਲ VError_ਸੁਨੇਹਾ& ਸੁਨੇਹਾ_2) { m_messages.ਧੱਕੋ_ਪਿੱਠ(ਸੁਨੇਹਾ_2); }

    ਖਾਲੀ output(ਤਾਰਕਿਕ success) {
        V3_ਬਾਹਰਲਾ_Json_File ਦਾ{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        ਦਾ.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        ਦਾ.begin("runs", '[').begin();

        ਦਾ.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::ਸੰਸਕਰਣ(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        ਦਾ.begin("rules", '[');
        put_ਨਿਯਮਾਂ(ਦਾ);
        ਦਾ.ਸਿਰਾ()  //ਨਿਯਮਾਂ
            .ਸਿਰਾ()  // driver
            .ਸਿਰਾ();  //tool

        ਦਾ.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_ਦਲੀਲਾਂ_String())
            .put("executionSuccessful", success)
            .ਸਿਰਾ()
            .ਸਿਰਾ();

        ਦਾ.begin("results", '[');
        ਲਈ (ਅਚਲ VError_ਸੁਨੇਹਾ& msgr : m_messages) put_ਨਤੀਜਾ(ਦਾ, msgr);
        ਦਾ.ਸਿਰਾ();

        ਦਾ.ਸਿਰਾ();  // runs ]
        ਦਾ.ਸਿਰਾ();  // runs }
    }
};

ਖਾਲੀ V3DiagSarif::ਧੱਕੋ_ਸੁਨੇਹਾ(ਅਚਲ VError_ਸੁਨੇਹਾ& ਸੁਨੇਹਾ_2) VL_MT_DISABLED {
    ਜੇ (!v3Global.opt.diagnosticsSarif()) ਵਾਪਸ;
    V3DiagSarifImp::s().ਧੱਕੋ_ਸੁਨੇਹਾ(ਸੁਨੇਹਾ_2);
}

ਖਾਲੀ V3DiagSarif::output(ਤਾਰਕਿਕ success) {
    ਜੇ (!v3Global.opt.diagnosticsSarif()) ਵਾਪਸ;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
