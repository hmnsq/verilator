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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreShell.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreProc.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ V3PreShellImp final {
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ ਵਰਗ V3PreShell;

    ਸਥਿਰ V3PreShellImp s_preImp;
    ਸਥਿਰ V3PreProc* s_preprocp;
    ਸਥਿਰ VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    ਸਥਿਰ ਖਾਲੀ boot() {
        // Create the implementation pointer
        ਜੇ (!s_preprocp) {
            File_ਸਤਰ* ਅਚਲ cmdfl = ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_ਸਤਰ* ਅਚਲ prefl = ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::built_ਵਿੱਚ_Filename()};
            s_preprocp->define_Cmd_ਸਤਰ(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ਸਤਰ(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ਸਤਰ(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_ਸਤਰ(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            ਜੇ (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_ਸਤਰ(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    ਖਾਲੀ shutdown() {
        ਜੇ (s_preprocp) VL_DO_DANGLING(ਮਿਟਾਓ s_preprocp, s_preprocp);
    }

    ਤਾਰਕਿਕ preproc(File_ਸਤਰ* fl, ਅਚਲ string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ਅਚਲ string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ਅਚਲ string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        ਜੇ (modfilename.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਝੂਠ;

        // Set language standard up front
        ਜੇ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ਅਚਲ File_ਸਤਰ* ਅਚਲ modfileline = ਨਵਾਂ File_ਸਤਰ{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ਧੱਕੋ_ਪਾਠ(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        ਜਦੋਂ (!s_preprocp->isEof()) {
            ਅਚਲ string ਸਤਰ = s_preprocp->getline();
            V3Parse::pp_ਧੱਕੋ_ਪਾਠ(parsep, ਸਤਰ);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ preprocInclude(File_ਸਤਰ* fl, ਅਚਲ string& modname) {
        ਜੇ (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

ਨਿਜੀ:
    string preprocOpen(File_ਸਤਰ* fl, VInFilter* filterp, ਅਚਲ string& modname,
                       ਅਚਲ string& lastpath,
                       ਅਚਲ string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.file_ਰਸਤਾ(fl, modname, lastpath, errmsg);
        ਜੇ (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ਅਚਲ string ppmodname = s_preprocp->remove_ਨਿਰਧਾਰਤ_ਕਰਦਾ(modname);

            filename = v3Global.opt.file_ਰਸਤਾ(fl, ppmodname, lastpath, errmsg);
        }
        ਜੇ (filename == "") ਵਾਪਸ "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        ਵਾਪਸ filename;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    V3PreShellImp() = ਮੂਲ;
    ~V3PreShellImp() = ਮੂਲ;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = ਖਾਲੀ_ਸੂਚਕ;
VInFilter* V3PreShellImp::s_filterp = ਖਾਲੀ_ਸੂਚਕ;

//######################################################################
// V3PreShell

ਖਾਲੀ V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
ਖਾਲੀ V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
ਤਾਰਕਿਕ V3PreShell::preproc(File_ਸਤਰ* fl, ਅਚਲ string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ਅਚਲ string& errmsg) {
    ਵਾਪਸ V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
ਖਾਲੀ V3PreShell::preprocInclude(File_ਸਤਰ* fl, ਅਚਲ string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
ਖਾਲੀ V3PreShell::define_Cmd_ਸਤਰ(ਅਚਲ string& name, ਅਚਲ string& ਮੁੱਲ) {
    File_ਸਤਰ* ਅਚਲ prefl = ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_ਸਤਰ(prefl, name, ਮੁੱਲ);
}
ਖਾਲੀ V3PreShell::undef(ਅਚਲ string& name) { V3PreShellImp::s_preprocp->undef(name); }
ਖਾਲੀ V3PreShell::dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ(std::ostream& os) { V3PreShellImp::s_preprocp->dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ(os); }
ਖਾਲੀ V3PreShell::candidate_ਨਿਰਧਾਰਤ_ਕਰਦਾ(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidate_ਨਿਰਧਾਰਤ_ਕਰਦਾ(spellerp);
}
ਖਾਲੀ V3PreShell::selfTest() { V3PreProc::selfTest(); }
