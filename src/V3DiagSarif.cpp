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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3DiagSarif.h"

#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

తరగతి V3DiagSarifImp final {
    // MEMBERS
    std::deque<VError_సందేశం> m_messages;
    std::సమితి<V3_Error_సంకేతం> m_codes;
    std::map<V3_Error_సంకేతం, పూర్ణాంకం> m_సంకేతం_సూచీ;

    // METHODS
    శూన్య calculate() {
        కొరకు (స్థిరరాశి VError_సందేశం& msgr : m_messages)
            ఒకవేళ (msgr.సంకేతం_2().isNamed()) m_codes.emplace(msgr.సంకేతం_2());
        పూర్ణాంకం i = 0;
        కొరకు (స్థిరరాశి V3_Error_సంకేతం& సంకేతం_2 : m_codes) m_సంకేతం_సూచీ[సంకేతం_2] = i++;
    }

    శూన్య put_నియమాలు(V3_బహిర్గతం_Json_File& యొక్క) స్థిరరాశి {
        కొరకు (స్థిరరాశి V3_Error_సంకేతం& సంకేతం_2 : m_codes) {
            యొక్క.begin().put("id", సంకేతం_2.ascii()).put("helpUri", సంకేతం_2.url()).చివర();
        }
    }

    శూన్య put_పాఠ్యం(V3_బహిర్గతం_Json_File& యొక్క, స్థిరరాశి string& clean, స్థిరరాశి string& formatted) స్థిరరాశి {
        స్థిరరాశి string markdown = "```\n" + formatted + "\n```\n";
        యొక్క.put("text", VString::trimWhitespace(clean)).put("markdown", markdown);
    }

    శూన్య putLocation(V3_బహిర్గతం_Json_File& యొక్క, స్థిరరాశి File_పంక్తి* fl) స్థిరరాశి {
        స్థిరరాశి string filename = V3Os::filename_Real_మార్గం(fl->filename());
        యొక్క.begin("physicalLocation");
        యొక్క.begin("artifactLocation");
        ఒకవేళ (V3Os::filenameIsRel(filename)) {  // SARIF spec says rel has no //
            యొక్క.put("uri", "file:" + filename).put("uriBaseId", "%srcroot%");
        } లేకపోతే {
            యొక్క.put("uri", "file://" + filename);
        }
        యొక్క.చివర()
            .begin("region")
            .put("sourceLanguage", "systemverilog")
            .put("startLine", fl->మొదటి_Lineno())
            .put("startColumn", fl->మొదటి_స్తంభం());
        ఒకవేళ (fl->మొదటి_Lineno() != fl->చివరి_Lineno()) యొక్క.put("endLine", fl->చివరి_Lineno());
        యొక్క.put("endColumn", fl->చివరి_స్తంభం());
        యొక్క.begin("snippit", '{');
        put_పాఠ్యం(యొక్క, fl->pretty_ఆధారం(), V3Error::strip_Meta_పాఠ్యం(fl->warnContextPrimary(), అబద్ధం));
        యొక్క.చివర();  // snippit
        యొక్క.చివర();  // region
        యొక్క.చివర();  // artifactLocation
    }

    string substr_గమ్యం_తదుపరి_Related(స్థిరరాశి string& str) {
        స్థిరరాశి కొలత_t pos = str.కనుగొను("__WARNRELATED(", 0);
        తిరిగి (pos == std::string::npos) ? str : str.substr(0, pos);
    }

    శూన్య put_ఫలితం(V3_బహిర్గతం_Json_File& యొక్క, స్థిరరాశి VError_సందేశం& సందేశం_2) {
        యొక్క.begin();
        యొక్క.put("level", సందేశం_2.సంకేతం_2().severityInfo()             ? "none"
                        : V3Error::isError(సందేశం_2.సంకేతం_2(), అబద్ధం) ? "error"
                                                              : "warning");

        string పాఠ్యం = సందేశం_2.పాఠ్యం();
        // UINFO(9, "Result raw text " << text);

        // We put the entire message including relatedLocations text
        // into the primary message text, because viewers such as
        // https://microsoft.github.io/sarif-web-component/
        // currently appear to ignore relatedLocations.
        స్థిరరాశి string మొదటి_clean = V3Error::strip_Meta_పాఠ్యం(పాఠ్యం, నిజం);
        స్థిరరాశి string మొదటి_fmt = V3Error::strip_Meta_పాఠ్యం(పాఠ్యం, అబద్ధం);

        యొక్క.begin("message");
        put_పాఠ్యం(యొక్క, మొదటి_clean, మొదటి_fmt);
        యొక్క.చివర();
        ఒకవేళ (స్థిరరాశి File_పంక్తి* స్థిరరాశి fl = సందేశం_2.fileline()) {
            యొక్క.begin("locations", '[').begin();
            putLocation(యొక్క, fl);
            యొక్క.చివర();
            యొక్క.చివర();
        }

        ఒకవేళ (పాఠ్యం.కనుగొను("__WARNRELATED(") != std::string::npos) {
            యొక్క.begin("relatedLocations", '[');
            కొలత_t pos = 0;
            అయితే ((pos = పాఠ్యం.కనుగొను("__WARNRELATED(", pos)) != std::string::npos) {
                స్థిరరాశి కొలత_t ప్రారంభం = pos + std::అక్షరశ్రేణి_పొడవు("__WARNRELATED(");
                కొలత_t చివర = ప్రారంభం;
                అయితే (చివర < పాఠ్యం.కొలత() && పాఠ్యం[చివర] != ')') ++చివర;
                స్థిరరాశి కొలత_t సూచీ = std::atoi(పాఠ్యం.substr(ప్రారంభం, చివర + 1).c_str());
                ఒకవేళ (చివర < పాఠ్యం.కొలత()) చివర += std::అక్షరశ్రేణి_పొడవు(")");
                పాఠ్యం = పాఠ్యం.substr(చివర, పాఠ్యం.కొలత());
                UASSERT_STATIC(సూచీ < సందేశం_2.filelines().కొలత(),
                               "Error message warnRelated without fileline");

                స్థిరరాశి string related_clean
                    = V3Error::strip_Meta_పాఠ్యం(substr_గమ్యం_తదుపరి_Related(పాఠ్యం), నిజం);
                స్థిరరాశి string related_fmt
                    = V3Error::strip_Meta_పాఠ్యం(substr_గమ్యం_తదుపరి_Related(పాఠ్యం), అబద్ధం);
                స్థిరరాశి File_పంక్తి* fl = సందేశం_2.filelines()[సూచీ];
                యొక్క.begin().begin("message");
                put_పాఠ్యం(యొక్క, related_clean, related_fmt);
                యొక్క.చివర();  //సందేశం
                putLocation(యొక్క, fl);
                యొక్క.చివర();
            }
            యొక్క.చివర();
        }

        ఒకవేళ (సందేశం_2.సంకేతం_2().isNamed())
            యొక్క.put("ruleId", సందేశం_2.సంకేతం_2().ascii()).put("ruleIndex", m_సంకేతం_సూచీ[సందేశం_2.సంకేతం_2()]);

        యొక్క.చివర();
    }

    // STATIC FUNCTIONS
బహిరంగ:
    స్థిర V3DiagSarifImp& s() {
        స్థిర V3DiagSarifImp s_s;
        తిరిగి s_s;
    }

    శూన్య నెట్టు_సందేశం(స్థిరరాశి VError_సందేశం& సందేశం_2) { m_messages.నెట్టు_వెనుక(సందేశం_2); }

    శూన్య output(తార్కిక success) {
        V3_బహిర్గతం_Json_File యొక్క{v3Global.opt.diagnosticsSarifOutput()};
        calculate();

        యొక్క.put("$schema", "https://json.schemastore.org/sarif-2.1.0-rtm.5.json")
            .put("version", "2.1.0");

        యొక్క.begin("runs", '[').begin();

        యొక్క.begin("tool")
            .begin("driver")
            .put("name", "Verilator")
            .put("version", VString::replaceSubstr(V3Options::సంస్కరణ(), "Verilator ", ""))
            .put("informationUri", "https://verilator.org");

        యొక్క.begin("rules", '[');
        put_నియమాలు(యొక్క);
        యొక్క.చివర()  //నియమాలు
            .చివర()  // driver
            .చివర();  //tool

        యొక్క.begin("invocations", '[')
            .begin()
            .put("commandLine", v3Global.opt.all_వాదనలు_String())
            .put("executionSuccessful", success)
            .చివర()
            .చివర();

        యొక్క.begin("results", '[');
        కొరకు (స్థిరరాశి VError_సందేశం& msgr : m_messages) put_ఫలితం(యొక్క, msgr);
        యొక్క.చివర();

        యొక్క.చివర();  // runs ]
        యొక్క.చివర();  // runs }
    }
};

శూన్య V3DiagSarif::నెట్టు_సందేశం(స్థిరరాశి VError_సందేశం& సందేశం_2) VL_MT_DISABLED {
    ఒకవేళ (!v3Global.opt.diagnosticsSarif()) తిరిగి;
    V3DiagSarifImp::s().నెట్టు_సందేశం(సందేశం_2);
}

శూన్య V3DiagSarif::output(తార్కిక success) {
    ఒకవేళ (!v3Global.opt.diagnosticsSarif()) తిరిగి;
    UINFO(2, __FUNCTION__ << ":");
    V3DiagSarifImp::s().output(success);
}
