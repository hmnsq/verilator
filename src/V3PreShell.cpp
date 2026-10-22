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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3PreShell.h"

#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3Os.h"
#समावेश "V3Parse.h"
#समावेश "V3PreProc.h"

#समावेश <algorithm>
#समावेश <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी V3PreShellImp final {
रक्षित:
    मित्र श्रेणी V3PreShell;

    अटल V3PreShellImp s_preImp;
    अटल V3PreProc* s_preprocp;
    अटल VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    अटल व्योम boot() {
        // Create the implementation pointer
        अगर (!s_preprocp) {
            File_पंक्ति* स्थिर cmdfl = नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_पंक्ति* स्थिर prefl = नया File_पंक्ति{File_पंक्ति::builtInFilename()};
            s_preprocp->define_Cmd_पंक्ति(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_पंक्ति(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_पंक्ति(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_पंक्ति(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            अगर (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_पंक्ति(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_पंक्ति(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    व्योम shutdown() {
        अगर (s_preprocp) VL_DO_DANGLING(मिटाओ s_preprocp, s_preprocp);
    }

    bool preproc(File_पंक्ति* fl, स्थिर string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 स्थिर string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        स्थिर string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        अगर (modfilename.empty()) वापस false;

        // Set language standard up front
        अगर (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            स्थिर File_पंक्ति* स्थिर modfileline = नया File_पंक्ति{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_धकेलो_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        जबतक (!s_preprocp->isEof()) {
            स्थिर string पंक्ति = s_preprocp->लो_पंक्ति();
            V3Parse::pp_धकेलो_Text(parsep, पंक्ति);
        }
        वापस true;
    }

    व्योम preprocInclude(File_पंक्ति* fl, स्थिर string& modname) {
        अगर (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

निजी:
    string preprocOpen(File_पंक्ति* fl, VInFilter* filterp, स्थिर string& modname,
                       स्थिर string& lastpath,
                       स्थिर string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        अगर (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            स्थिर string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        अगर (filename == "") वापस "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        वापस filename;
    }

खुला:
    // CONSTRUCTORS
    V3PreShellImp() = शेष;
    ~V3PreShellImp() = शेष;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

व्योम V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
व्योम V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
bool V3PreShell::preproc(File_पंक्ति* fl, स्थिर string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, स्थिर string& errmsg) {
    वापस V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
व्योम V3PreShell::preprocInclude(File_पंक्ति* fl, स्थिर string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
व्योम V3PreShell::define_Cmd_पंक्ति(स्थिर string& name, स्थिर string& मूल्य) {
    File_पंक्ति* स्थिर prefl = नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_पंक्ति(prefl, name, मूल्य);
}
व्योम V3PreShell::undef(स्थिर string& name) { V3PreShellImp::s_preprocp->undef(name); }
व्योम V3PreShell::dumpDefines(std::ब_प्रवाह& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
व्योम V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
व्योम V3PreShell::selfTest() { V3PreProc::selfTest(); }
