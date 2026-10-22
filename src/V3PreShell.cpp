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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3PreShell.h"

#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Parse.h"
#அந்தர்பாவ "V3PreProc.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <iostream>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு V3PreShellImp final {
பாதுகாக்கப்பட்ட:
    friend வகுப்பு V3PreShell;

    நிலையான V3PreShellImp s_preImp;
    நிலையான V3PreProc* s_preprocp;
    நிலையான VInFilter* s_filterp;

    //---------------------------------------
    // METHODS

    நிலையான வெறுமை boot() {
        // Create the implementation pointer
        எனில் (!s_preprocp) {
            File_வரி* மாறிலி cmdfl = புதிய File_வரி{File_வரி::command_வரி_Filename()};
            s_preprocp = V3PreProc::createPreProc(cmdfl);
            // Default defines
            File_வரி* மாறிலி prefl = புதிய File_வரி{File_வரி::builtInFilename()};
            s_preprocp->define_Cmd_வரி(prefl, "VERILATOR", "1");  // LEAK_OK
            s_preprocp->define_Cmd_வரி(prefl, "verilator", "1");  // LEAK_OK
            s_preprocp->define_Cmd_வரி(prefl, "verilator3", "1");  // LEAK_OK
            s_preprocp->define_Cmd_வரி(prefl, "coverage_block_off",
                                      "/*verilator coverage_block_off*/");  // LEAK_OK
            எனில் (prefl->language().systemVerilog()) {
                // Synthesis compatibility
                s_preprocp->define_Cmd_வரி(prefl, "SYSTEMVERILOG", "1");  // LEAK_OK
                // IEEE predefined
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_START", "0");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_STOP", "1");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_RESET", "2");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_CHECK", "3");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_MODULE", "10");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_HIER", "11");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_ASSERTION", "20");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_FSM_STATE", "21");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_STATEMENT", "22");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_TOGGLE", "23");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_OVERFLOW", "-2");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_ERROR", "-1");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_NOCOV", "0");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_OK", "1");
                s_preprocp->define_Cmd_வரி(prefl, "SV_COV_PARTIAL", "2");
            }
        }
    }

    வெறுமை shutdown() {
        எனில் (s_preprocp) VL_DO_DANGLING(delete s_preprocp, s_preprocp);
    }

    தருக்க preproc(File_வரி* fl, மாறிலி string& modname, VInFilter* filterp, V3ParseImp* parsep,
                 மாறிலி string& errmsg) {  // "" for no error
        // Preprocess the given module, putting output in vppFilename
        UINFO(1, "Preprocessing " << modname);

        // Preprocess
        s_filterp = filterp;
        மாறிலி string modfilename = preprocOpen(fl, s_filterp, modname, "", errmsg);
        எனில் (modfilename.empty()) திரும்பு பொய்;

        // Set language standard up front
        எனில் (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
            // Letting lex parse this saves us from having to specially en/decode
            // from the V3LangCode to the various Lex BEGIN states. The language
            // of this source file is updated here, in case there have been any
            // intervening +<lang>ext+ options since it was first encountered.
            மாறிலி File_வரி* மாறிலி modfileline = புதிய File_வரி{modfilename};
            modfileline->language(v3Global.opt.fileLanguage(modfilename));
            V3Parse::pp_தள்ளு_Text(
                parsep, ("`begin_keywords \""s + modfileline->language().ascii() + "\"\n"));
            // FileLine tracks and frees modfileline
        }

        போது (!s_preprocp->isEof()) {
            மாறிலி string வரி = s_preprocp->getline();
            V3Parse::pp_தள்ளு_Text(parsep, வரி);
        }
        திரும்பு உண்மை;
    }

    வெறுமை preprocInclude(File_வரி* fl, மாறிலி string& modname) {
        எனில் (modname[0] == '/' || modname[0] == '\\') {
            fl->v3warn(INCABSPATH,
                       "Suggest `include with absolute path be made relative, and use +include: "
                           << modname);
        }
        preprocOpen(fl, s_filterp, modname, V3Os::filenameDir(fl->filename()),
                    "Cannot find include file: ");
    }

தனிப்பட்ட:
    string preprocOpen(File_வரி* fl, VInFilter* filterp, மாறிலி string& modname,
                       மாறிலி string& lastpath,
                       மாறிலி string& errmsg) {  // Error message or "" to suppress
        // Returns filename if successful
        // Try a pure name in case user has a bogus `filename they don't expect
        string filename = v3Global.opt.filePath(fl, modname, lastpath, errmsg);
        எனில் (filename == "") {
            // Allow user to put `defined names on the command line instead of filenames,
            // then convert them properly.
            மாறிலி string ppmodname = s_preprocp->removeDefines(modname);

            filename = v3Global.opt.filePath(fl, ppmodname, lastpath, errmsg);
        }
        எனில் (filename == "") திரும்பு "";  // Not found

        UINFO(2, "    Reading " << filename);
        s_preprocp->openFile(fl, filterp, filename);
        திரும்பு filename;
    }

பொது:
    // CONSTRUCTORS
    V3PreShellImp() = இயல்பு;
    ~V3PreShellImp() = இயல்பு;
};

V3PreShellImp V3PreShellImp::s_preImp;
V3PreProc* V3PreShellImp::s_preprocp = nullptr;
VInFilter* V3PreShellImp::s_filterp = nullptr;

//######################################################################
// V3PreShell

வெறுமை V3PreShell::boot() { V3PreShellImp::s_preImp.boot(); }
வெறுமை V3PreShell::shutdown() { V3PreShellImp::s_preImp.shutdown(); }
தருக்க V3PreShell::preproc(File_வரி* fl, மாறிலி string& modname, VInFilter* filterp,
                         V3ParseImp* parsep, மாறிலி string& errmsg) {
    திரும்பு V3PreShellImp::s_preImp.preproc(fl, modname, filterp, parsep, errmsg);
}
வெறுமை V3PreShell::preprocInclude(File_வரி* fl, மாறிலி string& modname) {
    V3PreShellImp::s_preImp.preprocInclude(fl, modname);
}
வெறுமை V3PreShell::define_Cmd_வரி(மாறிலி string& name, மாறிலி string& மதிப்பு) {
    File_வரி* மாறிலி prefl = புதிய File_வரி{File_வரி::command_வரி_Filename()};
    V3PreShellImp::s_preprocp->define_Cmd_வரி(prefl, name, மதிப்பு);
}
வெறுமை V3PreShell::undef(மாறிலி string& name) { V3PreShellImp::s_preprocp->undef(name); }
வெறுமை V3PreShell::dumpDefines(std::ostream& os) { V3PreShellImp::s_preprocp->dumpDefines(os); }
வெறுமை V3PreShell::candidateDefines(VSpellCheck* spellerp) {
    V3PreShellImp::s_preprocp->candidateDefines(spellerp);
}
வெறுமை V3PreShell::selfTest() { V3PreProc::selfTest(); }
