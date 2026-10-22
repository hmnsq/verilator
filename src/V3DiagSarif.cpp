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
    std::deque<VError_ಸಂದೇಶ> m_messages;
    std::ಗಣ<V3_Error_ಸಂಕೇತ> m_codes;
    std::map<V3_Error_ಸಂಕೇತ, ಪೂರ್ಣಾಂಕ> m_ಸಂಕೇತ_ಸೂಚ್ಯಂಕ;

    // METHODS
    ಶೂನ್ಯ calculate() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VError_ಸಂದೇಶ& msgr : m_messages)
            ಆದರೆ (msgr.ಸಂಕೇತ_2().isNamed()) m_codes.emplace(msgr.ಸಂಕೇತ_2());
        ಪೂರ್ಣಾಂಕ i = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& ಸಂಕೇತ_2 : m_codes) m_ಸಂಕೇತ_ಸೂಚ್ಯಂಕ[ಸಂಕೇತ_2] = i++;
    }

    ಶೂನ್ಯ put_ನಿಯಮಗಳು(V3_ಹೊರಹೋಗು_Json_File& ದ) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& ಸಂಕೇತ_2 : m_codes) {
            ದ.begin().put("id", ಸಂಕೇತ_2.ascii()).put("helpUri", ಸಂಕೇತ_2.url()).ಕೊನೆ();
        }
    }

    ಶೂನ್ಯ put_ಪಠ್ಯ(V3_ಹೊರಹೋಗು_Json_File& ದ, ಸ್ಥಿರಾಂಕ string& clean, ಸ್ಥಿರಾಂಕ string& formatted) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ string markdown = "```\n" + formatted + "\n```\n";
        ದ.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    ಶೂನ್ಯ putLocation(V3_ಹೊರಹೋಗು_Json_File& ದ, ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fl) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ string filename = V3Os::filename_Real_ಮಾರ್ಗ(fl->filename());
        ದ.begin("physicalLocation");
        ದ.begin("artifactLocation");
        ಆದರೆ (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            ದ.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ದ.put("uri", "file://" + filename);
        }
        ದ.ಕೊನೆ()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->ಮೊದಲ_Lineno())
            .put("startColumn", fl->ಮೊದಲ_ಸ್ತಂಭ());
        ಆದರೆ (fl->ಮೊದಲ_Lineno() != fl->ಕೊನೆಯ_Lineno()) ದ.put("endLine", fl->ಕೊನೆಯ_Lineno());
        ದ.put("endColumn", fl->ಕೊನೆಯ_ಸ್ತಂಭ());
        ದ.begin("snippit", '{');
        put_ಪಠ್ಯ(ದ, fl->pretty_ಮೂಲ(), V3Error::strip_Meta_ಪಠ್ಯ(fl->warnContextPrimary(), ಸುಳ್ಳು));
        ದ.ಕೊನೆ();  // snippit
        ದ.ಕೊನೆ();  // region
        ದ.ಕೊನೆ();  // artifactLocation
    }

    string substr_ಗಮ್ಯ_ಮುಂದಿನ_Related(ಸ್ಥಿರಾಂಕ string& str) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pos = str.ಕಂಡುಹಿಡಿ("__WARNRELATED(", 0);
        ಹಿಂತಿರುಗಿ (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    ಶೂನ್ಯ put_ಫಲಿತಾಂಶ(V3_ಹೊರಹೋಗು_Json_File& ದ, ಸ್ಥಿರಾಂಕ VError_ಸಂದೇಶ& ಸಂದೇಶ_2) {
        ದ.begin();
        ದ.put("level", ಸಂದೇಶ_2.ಸಂಕೇತ_2().severityInfo()             ? "none"
                        : V3Error::isError(ಸಂದೇಶ_2.ಸಂಕೇತ_2(), ಸುಳ್ಳು) ? "error"
                                                              : "warning");

        string ಪಠ್ಯ = ಸಂದೇಶ_2.ಪಠ್ಯ();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        ಸ್ಥಿರಾಂಕ string ಮೊದಲ_clean = V3Error::strip_Meta_ಪಠ್ಯ(ಪಠ್ಯ, ನಿಜ);
        ಸ್ಥಿರಾಂಕ string ಮೊದಲ_fmt = V3Error::strip_Meta_ಪಠ್ಯ(ಪಠ್ಯ, ಸುಳ್ಳು);

        ದ.begin("message");
        put_ಪಠ್ಯ(ದ, ಮೊದಲ_clean, ಮೊದಲ_fmt);
        ದ.ಕೊನೆ();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಸಂದೇಶ_2.fileline()) {
            ದ.begin("locations", '[').begin();
            putLocation(ದ, fl);
            ದ.ಕೊನೆ();
            ದ.ಕೊನೆ();
        }

        ಆದರೆ (ಪಠ್ಯ.ಕಂಡುಹಿಡಿ("__WARNRELATED(") != std::string::npos) {
            ದ.begin("relatedLocations", '[');
            ಗಾತ್ರ_t pos = 0;
            ಆಗ ((pos = ಪಠ್ಯ.ಕಂಡುಹಿಡಿ("__WARNRELATED(", pos)) != std::string::npos) {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಆರಂಭ = pos + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__WARNRELATED(");
                ಗಾತ್ರ_t ಕೊನೆ = ಆರಂಭ;
                ಆಗ (ಕೊನೆ < ಪಠ್ಯ.ಗಾತ್ರ_2() && ಪಠ್ಯ[ಕೊನೆ] != ')') ++ಕೊನೆ;
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಸೂಚ್ಯಂಕ = std::atoi(ಪಠ್ಯ.substr(ಆರಂಭ, ಕೊನೆ + 1).c_str());
                ಆದರೆ (ಕೊನೆ < ಪಠ್ಯ.ಗಾತ್ರ_2()) ಕೊನೆ += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(")");
                ಪಠ್ಯ = ಪಠ್ಯ.substr(ಕೊನೆ, ಪಠ್ಯ.ಗಾತ್ರ_2());
                UASSERT_STATIC(ಸೂಚ್ಯಂಕ < ಸಂದೇಶ_2.filelines().ಗಾತ್ರ_2(),
                               "Error message warnRelated without fileline");

                ಸ್ಥಿರಾಂಕ string related_clean
                    = V3Error::strip_Meta_ಪಠ್ಯ(substr_ಗಮ್ಯ_ಮುಂದಿನ_Related(ಪಠ್ಯ), ನಿಜ);
                ಸ್ಥಿರಾಂಕ string related_fmt
                    = V3Error::strip_Meta_ಪಠ್ಯ(substr_ಗಮ್ಯ_ಮುಂದಿನ_Related(ಪಠ್ಯ), ಸುಳ್ಳು);
                ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fl = ಸಂದೇಶ_2.filelines()[ಸೂಚ್ಯಂಕ];
                ದ.begin().begin("message");
                put_ಪಠ್ಯ(ದ, related_clean, related_fmt);
                ದ.ಕೊನೆ();  //ಸಂದೇಶ
                putLocation(ದ, fl);
                ದ.ಕೊನೆ();
            }
            ದ.ಕೊನೆ();
        }

        ಆದರೆ (ಸಂದೇಶ_2.ಸಂಕೇತ_2().isNamed())
            ದ.put("ruleId", ಸಂದೇಶ_2.ಸಂಕೇತ_2().ascii()).put("ruleIndex", m_ಸಂಕೇತ_ಸೂಚ್ಯಂಕ[ಸಂದೇಶ_2.ಸಂಕೇತ_2()]);

        ದ.ಕೊನೆ();
    }

    // STATIC FUNCTIONS
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3DiagSarifImp& s() {
        ಸ್ಥಿರ V3DiagSarifImp s_s;
        ಹಿಂತಿರುಗಿ s_s;
    }

    ಶೂನ್ಯ ತಳ್ಳು_ಸಂದೇಶ(ಸ್ಥಿರಾಂಕ VError_ಸಂದೇಶ& ಸಂದೇಶ_2) { m_messages.ತಳ್ಳು_ಹಿಂಭಾಗ(ಸಂದೇಶ_2); }

    ಶೂನ್ಯ output(ತಾರ್ಕಿಕ success) {
        V3_ಹೊರಹೋಗು_Json_File ದ{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        ದ.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        ದ.begin("runs", '[').begin();

        ದ.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::ಆವೃತ್ತಿ(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        ದ.begin("rules", '[');
        put_ನಿಯಮಗಳು(ದ);
        ದ.ಕೊನೆ()  //ನಿಯಮಗಳು
            .ಕೊನೆ()  // driver
            .ಕೊನೆ();  //tool

        ದ.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_ವಾದಗಳು_String())
            .put("executionSuccessful", success)
            .ಕೊನೆ()
            .ಕೊನೆ();

        ದ.begin("results", '[');
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VError_ಸಂದೇಶ& msgr : m_messages) put_ಫಲಿತಾಂಶ(ದ, msgr);
        ದ.ಕೊನೆ();

        ದ.ಕೊನೆ();  // runs ]
        ದ.ಕೊನೆ();  // runs }
    }
};

ಶೂನ್ಯ V3DiagSarif::ತಳ್ಳು_ಸಂದೇಶ(ಸ್ಥಿರಾಂಕ VError_ಸಂದೇಶ& ಸಂದೇಶ_2) VL_MT_DISABLED {
    ಆದರೆ (!v3Global.opt.diagnosticsSarif()) ಹಿಂತಿರುಗಿ;
    V3DiagSarifImp::s().ತಳ್ಳು_ಸಂದೇಶ(ಸಂದೇಶ_2);
}

ಶೂನ್ಯ V3DiagSarif::output(ತಾರ್ಕಿಕ success) {
    ಆದರೆ (!v3Global.opt.diagnosticsSarif()) ಹಿಂತಿರುಗಿ;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
