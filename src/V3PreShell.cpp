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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3PreShell.h"

#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3Parse.h"
#অন্তর্ভাব "V3PreProc.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্রেণী V3PreShellImp final {
সুরক্ষিত:
    friend শ্রেণী V3PreShell;

    স্থির V3PreShellImp s_preImp;
    স্থির V3PreProc* s_preprocp;
    স্থির VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    স্থির শূন্য boot() {
        // Create the implementation pointer
        যদি (!s_preprocp) {
            File_সারি* ধ্রুবক cmdfl = নতুন File_সারি{File_সারি::command_সারি_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_সারি* ধ্রুবক prefl = নতুন File_সারি{File_সারি::builtInFilename()};
            s_preprocp->define_Cmd_সারি(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_সারি(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_সারি(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_সারি(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            যদি (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_সারি(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_সারি(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    শূন্য shutdown() {
        যদি (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    যৌক্তিক preproc(File_সারি* fl, ধ্রুবক string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ধ্রুবক string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ধ্রুবক string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        যদি (modfilename.empty()) ফেরত মিথ্যা;

        // Set language standard up front
        যদি (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ধ্রুবক File_সারি* ধ্রুবক modfileline = নতুন File_সারি{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ঠেলো_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        যখন (!s_preprocp->isEof()) {
            ধ্রুবক string সারি = s_preprocp->getline();
            V3Parse::pp_ঠেলো_Text(parsep, সারি);
        }
        ফেরত সত্য;
    }

    শূন্য preprocInclude(File_সারি* fl, ধ্রুবক string& modname) {
        যদি (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

ব্যক্তিগত:
    string preprocOpen(File_সারি* fl, VInFilter* filterp, ধ্রুবক string& modname,
                       ধ্রুবক string& lastpath,
                       ধ্রুবক string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        যদি (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ধ্রুবক string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        যদি (filename == "") ফেরত "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        ফেরত filename;
    }

সর্বজনীন:
    // CONSTRUCTORS
    V3PreShellImp() = পূর্বনির্ধারিত;
    ~V3PreShellImp() = পূর্বনির্ধারিত;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

শূন্য V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
শূন্য V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
যৌক্তিক V3PreShell::preproc(File_সারি* fl, ধ্রুবক string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ধ্রুবক string& errmsg) {
    ফেরত V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
শূন্য V3PreShell::preprocInclude(File_সারি* fl, ধ্রুবক string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
শূন্য V3PreShell::define_Cmd_সারি(ধ্রুবক string& name, ধ্রুবক string& মান) {
    File_সারি* ধ্রুবক prefl = নতুন File_সারি{File_সারি::command_সারি_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_সারি(prefl, name, মান);
}
শূন্য V3PreShell::undef(ধ্রুবক string& name) { V3PreShellImp::s_preprocp->undef(name); }
শূন্য V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
শূন্য V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
শূন্য V3PreShell::selfTest() { V3PreProc::selfTest(); }
