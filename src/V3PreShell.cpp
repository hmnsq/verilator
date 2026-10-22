// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Preprocessing wrapper
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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3PreShell.h"

#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Parse.h"
#అన్తర్భావ "V3PreProc.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి V3PreShellImp final {
రక్షిత:
    మిత్ర తరగతి V3PreShell;

    స్థిర V3PreShellImp s_preImp;
    స్థిర V3PreProc* s_preprocp;
    స్థిర VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    స్థిర శూన్య boot() {
        // Create the implementation pointer
        ఒకవేళ (!s_preprocp) {
            File_పంక్తి* స్థిరరాశి cmdfl = కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_పంక్తి* స్థిరరాశి prefl = కొత్త File_పంక్తి{File_పంక్తి::built_లోపల_Filename()};
            s_preprocp->define_Cmd_పంక్తి(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_పంక్తి(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_పంక్తి(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_పంక్తి(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            ఒకవేళ (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_పంక్తి(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_పంక్తి(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    శూన్య shutdown() {
        ఒకవేళ (s_preprocp) VL_DO_DANGLING(తొలగించు s_preprocp, s_preprocp);
    }

    తార్కిక preproc(File_పంక్తి* fl, స్థిరరాశి string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 స్థిరరాశి string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        స్థిరరాశి string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        ఒకవేళ (modfilename.ఖాళీలు()) తిరిగి అబద్ధం;

        // Set language standard up front
        ఒకవేళ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            స్థిరరాశి File_పంక్తి* స్థిరరాశి modfileline = కొత్త File_పంక్తి{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_నెట్టు_పాఠ్యం(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        అయితే (!s_preprocp->isEof()) {
            స్థిరరాశి string పంక్తి = s_preprocp->getline();
            V3Parse::pp_నెట్టు_పాఠ్యం(parsep, పంక్తి);
        }
        తిరిగి నిజం;
    }

    శూన్య preprocInclude(File_పంక్తి* fl, స్థిరరాశి string& modname) {
        ఒకవేళ (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

అంతరంగ:
    string preprocOpen(File_పంక్తి* fl, VInFilter* filterp, స్థిరరాశి string& modname,
                       స్థిరరాశి string& lastpath,
                       స్థిరరాశి string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.file_మార్గం(fl, modname, lastpath, errmsg);
        ఒకవేళ (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            స్థిరరాశి string ppmodname = s_preprocp->remove_నిర్వచిస్తుంది(modname);

            filename = v3Global.opt.file_మార్గం(fl, ppmodname, lastpath, errmsg);
        }
        ఒకవేళ (filename == "") తిరిగి "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        తిరిగి filename;
    }

బహిరంగ:
    // CONSTRUCTORS
    V3PreShellImp() = అప్రమేయ;
    ~V3PreShellImp() = అప్రమేయ;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = శూన్యసూచిక;
VInFilter* V3PreShellImp::s_filterp = శూన్యసూచిక;

//######################################################################
// V3PreShell

శూన్య V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
శూన్య V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
తార్కిక V3PreShell::preproc(File_పంక్తి* fl, స్థిరరాశి string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, స్థిరరాశి string& errmsg) {
    తిరిగి V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
శూన్య V3PreShell::preprocInclude(File_పంక్తి* fl, స్థిరరాశి string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
శూన్య V3PreShell::define_Cmd_పంక్తి(స్థిరరాశి string& name, స్థిరరాశి string& విలువ) {
    File_పంక్తి* స్థిరరాశి prefl = కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_పంక్తి(prefl, name, విలువ);
}
శూన్య V3PreShell::undef(స్థిరరాశి string& name) { V3PreShellImp::s_preprocp->undef(name); }
శూన్య V3PreShell::dump_నిర్వచిస్తుంది(std::ostream& os) { V3PreShellImp::s_preprocp->dump_నిర్వచిస్తుంది(os); }
శూన్య V3PreShell::candidate_నిర్వచిస్తుంది(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidate_నిర్వచిస్తుంది(spellerp);
}
శూన్య V3PreShell::selfTest() { V3PreProc::selfTest(); }
