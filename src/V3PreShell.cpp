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

#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3PreShell.h"

#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Os.h"
#ضمن "V3Parse.h"
#ضمن "V3PreProc.h"

#ضمن <algorithm>
#ضمن <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف V3PreShellImp final {
محمي:
    دوست صنف V3PreShell;

    ساكن V3PreShellImp s_preImp;
    ساكن V3PreProc* s_preprocp;
    ساكن VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    ساكن فراغ boot() {
        // Create the implementation pointer
        إذا (!s_preprocp) {
            File_سٽ* ثابت cmdfl = جديد File_سٽ{File_سٽ::command_سٽ_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_سٽ* ثابت prefl = جديد File_سٽ{File_سٽ::built_اندريون_Filename()};
            s_preprocp->define_Cmd_سٽ(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_سٽ(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_سٽ(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_سٽ(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            إذا (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_سٽ(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_سٽ(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    فراغ shutdown() {
        إذا (s_preprocp) VL_DO_DANGLING(ميٽيو s_preprocp, s_preprocp);
    }

    منطقي preproc(File_سٽ* fl, ثابت string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ثابت string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ثابت string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        إذا (modfilename.خالي_جايون()) أرجع خطأ;

        // Set language standard up front
        إذا (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ثابت File_سٽ* ثابت modfileline = جديد File_سٽ{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ڌڪو_متن(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        طالما (!s_preprocp->isEof()) {
            ثابت string سٽ = s_preprocp->getline();
            V3Parse::pp_ڌڪو_متن(parsep, سٽ);
        }
        أرجع سچ;
    }

    فراغ preprocInclude(File_سٽ* fl, ثابت string& modname) {
        إذا (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

خاص:
    string preprocOpen(File_سٽ* fl, VInFilter* filterp, ثابت string& modname,
                       ثابت string& lastpath,
                       ثابت string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.file_رستو(fl, modname, lastpath, errmsg);
        إذا (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ثابت string ppmodname = s_preprocp->remove_مقرر_ڪري(modname);

            filename = v3Global.opt.file_رستو(fl, ppmodname, lastpath, errmsg);
        }
        إذا (filename == "") أرجع "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        أرجع filename;
    }

عام:
    // CONSTRUCTORS
    V3PreShellImp() = افتراضي;
    ~V3PreShellImp() = افتراضي;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = خالي_اشارو;
VInFilter* V3PreShellImp::s_filterp = خالي_اشارو;

//######################################################################
// V3PreShell

فراغ V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
فراغ V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
منطقي V3PreShell::preproc(File_سٽ* fl, ثابت string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ثابت string& errmsg) {
    أرجع V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
فراغ V3PreShell::preprocInclude(File_سٽ* fl, ثابت string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
فراغ V3PreShell::define_Cmd_سٽ(ثابت string& name, ثابت string& قدر) {
    File_سٽ* ثابت prefl = جديد File_سٽ{File_سٽ::command_سٽ_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_سٽ(prefl, name, قدر);
}
فراغ V3PreShell::undef(ثابت string& name) { V3PreShellImp::s_preprocp->undef(name); }
فراغ V3PreShell::dump_مقرر_ڪري(std::ostream& os) { V3PreShellImp::s_preprocp->dump_مقرر_ڪري(os); }
فراغ V3PreShell::candidate_مقرر_ڪري(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidate_مقرر_ڪري(spellerp);
}
فراغ V3PreShell::selfTest() { V3PreProc::selfTest(); }
