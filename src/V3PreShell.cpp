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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3PreShell.h"

#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3Parse.h"
#অন্তর্ভাৱ "V3PreProc.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্ৰেণী V3PreShellImp final {
রক্ষিত:
    friend শ্ৰেণী V3PreShell;

    স্থির V3PreShellImp s_preImp;
    স্থির V3PreProc* s_preprocp;
    স্থির VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    স্থির শূন্য boot() {
        // Create the implementation pointer
        যদি (!s_preprocp) {
            File_সাৰি* ধ্রুৱ cmdfl = নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_সাৰি* ধ্রুৱ prefl = নূতন File_সাৰি{File_সাৰি::builtInFilename()};
            s_preprocp->define_Cmd_সাৰি(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_সাৰি(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_সাৰি(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_সাৰি(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            যদি (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_সাৰি(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_সাৰি(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    শূন্য shutdown() {
        যদি (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    তার্কিক preproc(File_সাৰি* fl, ধ্রুৱ string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ধ্রুৱ string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ধ্রুৱ string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        যদি (modfilename.empty()) ঘূৰাই মিছা;

        // Set language standard up front
        যদি (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ধ্রুৱ File_সাৰি* ধ্রুৱ modfileline = নূতন File_সাৰি{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ঠেলো_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        যেতিয়ালৈ (!s_preprocp->isEof()) {
            ধ্রুৱ string সাৰি = s_preprocp->getline();
            V3Parse::pp_ঠেলো_Text(parsep, সাৰি);
        }
        ঘূৰাই সঁচা;
    }

    শূন্য preprocInclude(File_সাৰি* fl, ধ্রুৱ string& modname) {
        যদি (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

গুপ্ত:
    string preprocOpen(File_সাৰি* fl, VInFilter* filterp, ধ্রুৱ string& modname,
                       ধ্রুৱ string& lastpath,
                       ধ্রুৱ string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        যদি (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ধ্রুৱ string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        যদি (filename == "") ঘূৰাই "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        ঘূৰাই filename;
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    V3PreShellImp() = স্ৱভাৱ;
    ~V3PreShellImp() = স্ৱভাৱ;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

শূন্য V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
শূন্য V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
তার্কিক V3PreShell::preproc(File_সাৰি* fl, ধ্রুৱ string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ধ্রুৱ string& errmsg) {
    ঘূৰাই V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
শূন্য V3PreShell::preprocInclude(File_সাৰি* fl, ধ্রুৱ string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
শূন্য V3PreShell::define_Cmd_সাৰি(ধ্রুৱ string& name, ধ্রুৱ string& মান) {
    File_সাৰি* ধ্রুৱ prefl = নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_সাৰি(prefl, name, মান);
}
শূন্য V3PreShell::undef(ধ্রুৱ string& name) { V3PreShellImp::s_preprocp->undef(name); }
শূন্য V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
শূন্য V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
শূন্য V3PreShell::selfTest() { V3PreProc::selfTest(); }
