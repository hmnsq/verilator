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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3PreShell.h"

#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Os.h"
#شامل "V3Parse.h"
#شامل "V3PreProc.h"

#شامل <algorithm>
#شامل <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده V3PreShellImp final {
محافظتشده:
    friend رده V3PreShell;

    ایستا V3PreShellImp s_preImp;
    ایستا V3PreProc* s_preprocp;
    ایستا VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    ایستا تهی boot() {
        // Create the implementation pointer
        اگر (!s_preprocp) {
            File_سطر* ثابت cmdfl = جدید File_سطر{File_سطر::command_سطر_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_سطر* ثابت prefl = جدید File_سطر{File_سطر::builtInFilename()};
            s_preprocp->define_Cmd_سطر(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_سطر(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_سطر(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_سطر(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            اگر (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_سطر(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_سطر(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    تهی shutdown() {
        اگر (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    منطقی preproc(File_سطر* fl, ثابت string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 ثابت string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        ثابت string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        اگر (modfilename.empty()) بازگشت نادرست;

        // Set language standard up front
        اگر (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            ثابت File_سطر* ثابت modfileline = جدید File_سطر{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_افزودن_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        وقتی (!s_preprocp->isEof()) {
            ثابت string سطر = s_preprocp->getline();
            V3Parse::pp_افزودن_Text(parsep, سطر);
        }
        بازگشت درست;
    }

    تهی preprocInclude(File_سطر* fl, ثابت string& modname) {
        اگر (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

خصوصی:
    string preprocOpen(File_سطر* fl, VInFilter* filterp, ثابت string& modname,
                       ثابت string& lastpath,
                       ثابت string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        اگر (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            ثابت string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        اگر (filename == "") بازگشت "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        بازگشت filename;
    }

عمومی:
    // CONSTRUCTORS
    V3PreShellImp() = پیشفرض;
    ~V3PreShellImp() = پیشفرض;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

تهی V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
تهی V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
منطقی V3PreShell::preproc(File_سطر* fl, ثابت string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, ثابت string& errmsg) {
    بازگشت V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
تهی V3PreShell::preprocInclude(File_سطر* fl, ثابت string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
تهی V3PreShell::define_Cmd_سطر(ثابت string& name, ثابت string& مقدار) {
    File_سطر* ثابت prefl = جدید File_سطر{File_سطر::command_سطر_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_سطر(prefl, name, مقدار);
}
تهی V3PreShell::undef(ثابت string& name) { V3PreShellImp::s_preprocp->undef(name); }
تهی V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
تهی V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
تهی V3PreShell::selfTest() { V3PreProc::selfTest(); }
