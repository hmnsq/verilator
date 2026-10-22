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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3PreShell.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"
#ଅନ୍ତର୍ଭାଵ "V3PreProc.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ V3PreShellImp final {
ସୁରକ୍ଷିତ:
    friend ଶ୍ରେଣୀ V3PreShell;

    ସ୍ଥିର V3PreShellImp s_preImp;
    ସ୍ଥିର V3PreProc* s_preprocp;
    ସ୍ଥିର VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    ସ୍ଥିର ଶୂନ୍ୟ boot() {
        // Create the implementation pointer
        ଯଦି (!s_preprocp) {
            File_ଧାଡ଼ି* ଅଚଳ cmdfl = ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_ଧାଡ଼ି* ଅଚଳ prefl = ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::builtInFilename()};
            s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            ଯଦି (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    ଶୂନ୍ୟ shutdown() {
        ଯଦି (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    ତାର୍କିକ preproc(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ଅଚଳ string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ଅଚଳ string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        ଯଦି (modfilename.empty()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Set language standard up front
        ଯଦି (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ଅଚଳ File_ଧାଡ଼ି* ଅଚଳ modfileline = ନୂତନ File_ଧାଡ଼ି{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ଠେଲ_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        ଯେତେବେଳେ (!s_preprocp->isEof()) {
            ଅଚଳ string ଧାଡ଼ି = s_preprocp->getline();
            V3Parse::pp_ଠେଲ_Text(parsep, ଧାଡ଼ି);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ preprocInclude(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname) {
        ଯଦି (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

ବ୍ୟକ୍ତିଗତ:
    string preprocOpen(File_ଧାଡ଼ି* fl, VInFilter* filterp, ଅଚଳ string& modname,
                       ଅଚଳ string& lastpath,
                       ଅଚଳ string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        ଯଦି (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ଅଚଳ string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        ଯଦି (filename == "") ଫେରନ୍ତୁ "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        ଫେରନ୍ତୁ filename;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    V3PreShellImp() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3PreShellImp() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

ଶୂନ୍ୟ V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
ଶୂନ୍ୟ V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
ତାର୍କିକ V3PreShell::preproc(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ଅଚଳ string& errmsg) {
    ଫେରନ୍ତୁ V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
ଶୂନ୍ୟ V3PreShell::preprocInclude(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
ଶୂନ୍ୟ V3PreShell::define_Cmd_ଧାଡ଼ି(ଅଚଳ string& name, ଅଚଳ string& ମୂଲ୍ୟ) {
    File_ଧାଡ଼ି* ଅଚଳ prefl = ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_ଧାଡ଼ି(prefl, name, ମୂଲ୍ୟ);
}
ଶୂନ୍ୟ V3PreShell::undef(ଅଚଳ string& name) { V3PreShellImp::s_preprocp->undef(name); }
ଶୂନ୍ୟ V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
ଶୂନ୍ୟ V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
ଶୂନ୍ୟ V3PreShell::selfTest() { V3PreProc::selfTest(); }
