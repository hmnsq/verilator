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
    friend صنف V3PreShell;

    ساكن V3PreShellImp s_preImp;
    ساكن V3PreProc* s_preprocp;
    ساكن VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    ساكن فراغ boot() {
        // Create the implementation pointer
        إذا (!s_preprocp) {
            File_قۇر* ثابت cmdfl = جديد File_قۇر{File_قۇر::command_قۇر_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_قۇر* ثابت prefl = جديد File_قۇر{File_قۇر::builtInFilename()};
            s_preprocp->define_Cmd_قۇر(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_قۇر(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_قۇر(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_قۇر(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            إذا (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_قۇر(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_قۇر(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    فراغ shutdown() {
        إذا (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    منطقي preproc(File_قۇر* fl, ثابت string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ثابت string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ثابت string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        إذا (modfilename.empty()) أرجع خطأ;

        // Set language standard up front
        إذا (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ثابت File_قۇر* ثابت modfileline = جديد File_قۇر{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_ئىتتىرىش_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        طالما (!s_preprocp->isEof()) {
            ثابت string قۇر = s_preprocp->getline();
            V3Parse::pp_ئىتتىرىش_Text(parsep, قۇر);
        }
        أرجع true;
    }

    فراغ preprocInclude(File_قۇر* fl, ثابت string& modname) {
        إذا (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

خاص:
    string preprocOpen(File_قۇر* fl, VInFilter* filterp, ثابت string& modname,
                       ثابت string& lastpath,
                       ثابت string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        إذا (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ثابت string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
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
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

فراغ V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
فراغ V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
منطقي V3PreShell::preproc(File_قۇر* fl, ثابت string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ثابت string& errmsg) {
    أرجع V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
فراغ V3PreShell::preprocInclude(File_قۇر* fl, ثابت string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
فراغ V3PreShell::define_Cmd_قۇر(ثابت string& name, ثابت string& قىممەت) {
    File_قۇر* ثابت prefl = جديد File_قۇر{File_قۇر::command_قۇر_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_قۇر(prefl, name, قىممەت);
}
فراغ V3PreShell::undef(ثابت string& name) { V3PreShellImp::s_preprocp->undef(name); }
فراغ V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
فراغ V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
فراغ V3PreShell::selfTest() { V3PreProc::selfTest(); }
