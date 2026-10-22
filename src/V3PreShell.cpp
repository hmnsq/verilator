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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3PreShell.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3PreProc.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ V3PreShellImp final {
ರಕ್ಷಿತ:
    friend ವರ್ಗ V3PreShell;

    ಸ್ಥಿರ V3PreShellImp s_preImp;
    ಸ್ಥಿರ V3PreProc* s_preprocp;
    ಸ್ಥಿರ VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    ಸ್ಥಿರ ಶೂನ್ಯ boot() {
        // Create the implementation pointer
        ಯದಿ (!s_preprocp) {
            File_ಸಾಲು* ಧ್ರುವ cmdfl = ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_ಸಾಲು* ಧ್ರುವ prefl = ನೂತನ File_ಸಾಲು{File_ಸಾಲು::builtInFilename()};
            s_preprocp->define_Cmd_ಸಾಲು(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ಸಾಲು(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ಸಾಲು(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ಸಾಲು(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            ಯದಿ (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_ಸಾಲು(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    ಶೂನ್ಯ shutdown() {
        ಯದಿ (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    ತಾರ್ಕಿಕ preproc(File_ಸಾಲು* fl, ಧ್ರುವ string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ಧ್ರುವ string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ಧ್ರುವ string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        ಯದಿ (modfilename.empty()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Set language standard up front
        ಯದಿ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ಧ್ರುವ File_ಸಾಲು* ಧ್ರುವ modfileline = ನೂತನ File_ಸಾಲು{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ತಳ್ಳು_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        ಯಾವತ್ (!s_preprocp->isEof()) {
            ಧ್ರುವ string ಸಾಲು = s_preprocp->getline();
            V3Parse::pp_ತಳ್ಳು_Text(parsep, ಸಾಲು);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ preprocInclude(File_ಸಾಲು* fl, ಧ್ರುವ string& modname) {
        ಯದಿ (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

ಗುಪ್ತ:
    string preprocOpen(File_ಸಾಲು* fl, VInFilter* filterp, ಧ್ರುವ string& modname,
                       ಧ್ರುವ string& lastpath,
                       ಧ್ರುವ string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        ಯದಿ (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ಧ್ರುವ string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        ಯದಿ (filename == "") ಪ್ರತ್ಯಾಗಮ "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        ಪ್ರತ್ಯಾಗಮ filename;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3PreShellImp() = ಸ್ವಭಾವ;
    ~V3PreShellImp() = ಸ್ವಭಾವ;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

ಶೂನ್ಯ V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
ಶೂನ್ಯ V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
ತಾರ್ಕಿಕ V3PreShell::preproc(File_ಸಾಲು* fl, ಧ್ರುವ string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ಧ್ರುವ string& errmsg) {
    ಪ್ರತ್ಯಾಗಮ V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
ಶೂನ್ಯ V3PreShell::preprocInclude(File_ಸಾಲು* fl, ಧ್ರುವ string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
ಶೂನ್ಯ V3PreShell::define_Cmd_ಸಾಲು(ಧ್ರುವ string& name, ಧ್ರುವ string& ಮೌಲ್ಯ) {
    File_ಸಾಲು* ಧ್ರುವ prefl = ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_ಸಾಲು(prefl, name, ಮೌಲ್ಯ);
}
ಶೂನ್ಯ V3PreShell::undef(ಧ್ರುವ string& name) { V3PreShellImp::s_preprocp->undef(name); }
ಶೂನ್ಯ V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
ಶೂನ್ಯ V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
ಶೂನ್ಯ V3PreShell::selfTest() { V3PreProc::selfTest(); }
