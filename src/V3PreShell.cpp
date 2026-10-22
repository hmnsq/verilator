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

#define VL_MT_DISABLED_CODE_UNIT 1

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3PreShell.h"

#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3Parse.h"
#અન્તર્ભાવ "V3PreProc.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ V3PreShellImp final {
સુરક્ષિત:
    friend વર્ગ V3PreShell;

    સ્થિર V3PreShellImp s_preImp;
    સ્થિર V3PreProc* s_preprocp;
    સ્થિર VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    સ્થિર શૂન્ય boot() {
        // Create the implementation pointer
        જો (!s_preprocp) {
            File_લીટી* અચલ cmdfl = નવું File_લીટી{File_લીટી::command_લીટી_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_લીટી* અચલ prefl = નવું File_લીટી{File_લીટી::builtInFilename()};
            s_preprocp->define_Cmd_લીટી(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_લીટી(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_લીટી(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_લીટી(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            જો (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_લીટી(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_લીટી(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    શૂન્ય shutdown() {
        જો (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    તાર્કિક preproc(File_લીટી* fl, અચલ string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 અચલ string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        અચલ string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        જો (modfilename.empty()) પાછા ખોટું;

        // Set language standard up front
        જો (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            અચલ File_લીટી* અચલ modfileline = નવું File_લીટી{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ધકેલો_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        જ્યારે (!s_preprocp->isEof()) {
            અચલ string લીટી = s_preprocp->getline();
            V3Parse::pp_ધકેલો_Text(parsep, લીટી);
        }
        પાછા સાચું;
    }

    શૂન્ય preprocInclude(File_લીટી* fl, અચલ string& modname) {
        જો (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

ખાનગી:
    string preprocOpen(File_લીટી* fl, VInFilter* filterp, અચલ string& modname,
                       અચલ string& lastpath,
                       અચલ string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        જો (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            અચલ string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        જો (filename == "") પાછા "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        પાછા filename;
    }

જાહેર:
    // CONSTRUCTORS
    V3PreShellImp() = મૂળભૂત;
    ~V3PreShellImp() = મૂળભૂત;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

શૂન્ય V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
શૂન્ય V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
તાર્કિક V3PreShell::preproc(File_લીટી* fl, અચલ string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, અચલ string& errmsg) {
    પાછા V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
શૂન્ય V3PreShell::preprocInclude(File_લીટી* fl, અચલ string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
શૂન્ય V3PreShell::define_Cmd_લીટી(અચલ string& name, અચલ string& કિંમત) {
    File_લીટી* અચલ prefl = નવું File_લીટી{File_લીટી::command_લીટી_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_લીટી(prefl, name, કિંમત);
}
શૂન્ય V3PreShell::undef(અચલ string& name) { V3PreShellImp::s_preprocp->undef(name); }
શૂન્ય V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
શૂન્ય V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
શૂન્ય V3PreShell::selfTest() { V3PreProc::selfTest(); }
