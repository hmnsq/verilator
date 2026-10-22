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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3PreShell.h"

#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Parse.h"
#അന്തര്ഭാവ "V3PreProc.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം V3PreShellImp final {
സംരക്ഷിത:
    friend വർഗ്ഗം V3PreShell;

    സ്ഥിതം V3PreShellImp s_preImp;
    സ്ഥിതം V3PreProc* s_preprocp;
    സ്ഥിതം VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    സ്ഥിതം ശൂന്യം boot() {
        // Create the implementation pointer
        എങ്കിൽ (!s_preprocp) {
            File_വരി* സ്ഥിരം cmdfl = പുതിയ File_വരി{File_വരി::command_വരി_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_വരി* സ്ഥിരം prefl = പുതിയ File_വരി{File_വരി::builtInFilename()};
            s_preprocp->define_Cmd_വരി(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_വരി(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_വരി(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_വരി(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            എങ്കിൽ (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_വരി(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_വരി(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    ശൂന്യം shutdown() {
        എങ്കിൽ (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    തർക്കം preproc(File_വരി* fl, സ്ഥിരം string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 സ്ഥിരം string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        സ്ഥിരം string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        എങ്കിൽ (modfilename.empty()) തിരികെ അസത്യം;

        // Set language standard up front
        എങ്കിൽ (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            സ്ഥിരം File_വരി* സ്ഥിരം modfileline = പുതിയ File_വരി{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_തള്ളുക_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        ആകുമ്പോൾ (!s_preprocp->isEof()) {
            സ്ഥിരം string വരി = s_preprocp->getline();
            V3Parse::pp_തള്ളുക_Text(parsep, വരി);
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം preprocInclude(File_വരി* fl, സ്ഥിരം string& modname) {
        എങ്കിൽ (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

സ്വകാര്യ:
    string preprocOpen(File_വരി* fl, VInFilter* filterp, സ്ഥിരം string& modname,
                       സ്ഥിരം string& lastpath,
                       സ്ഥിരം string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        എങ്കിൽ (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            സ്ഥിരം string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        എങ്കിൽ (filename == "") തിരികെ "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        തിരികെ filename;
    }

പൊതു:
    // CONSTRUCTORS
    V3PreShellImp() = സ്ഥിരസ്ഥിതി;
    ~V3PreShellImp() = സ്ഥിരസ്ഥിതി;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

ശൂന്യം V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
ശൂന്യം V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
തർക്കം V3PreShell::preproc(File_വരി* fl, സ്ഥിരം string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, സ്ഥിരം string& errmsg) {
    തിരികെ V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
ശൂന്യം V3PreShell::preprocInclude(File_വരി* fl, സ്ഥിരം string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
ശൂന്യം V3PreShell::define_Cmd_വരി(സ്ഥിരം string& name, സ്ഥിരം string& മൂല്യം) {
    File_വരി* സ്ഥിരം prefl = പുതിയ File_വരി{File_വരി::command_വരി_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_വരി(prefl, name, മൂല്യം);
}
ശൂന്യം V3PreShell::undef(സ്ഥിരം string& name) { V3PreShellImp::s_preprocp->undef(name); }
ശൂന്യം V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
ശൂന്യം V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
ശൂന്യം V3PreShell::selfTest() { V3PreProc::selfTest(); }
