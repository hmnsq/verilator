// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilog::Preproc: Preprocess verilog code
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2000-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#که_نه_تعریف_شوی VERILATOR_V3PREPROC_H_
#تعریف_کړه VERILATOR_V3PREPROC_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Global.h"

#ضمن <iostream>
#ضمن <list>
#ضمن <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

محمي:
    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    // CONSTANTS
    تعداد MiscConsts : صحيح {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_ژوروالی_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        بهیر_ژوروالی_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    مجازي فراغ openFile(File_کرښه* fileline, VInFilter* filterp, ثابت string& filename) = 0;
    مجازي string getline() = 0;  // Return next line/lines. (Null if done.)
    مجازي bool isEof() ثابت = 0;  // Return true on EOF.
    مجازي فراغ ورننباسه_Unreadback(ثابت string& متن) = 0;

    File_کرښه* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ساكن صحيح keepComments() { أرجع 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ساكن bool keepWhitespace() { أرجع false; }
    ساكن bool کرښه_Directives() {  // Insert `line directives
        أرجع !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_کرښه());
    }
    ساكن bool pedantic() { أرجع false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    مجازي فراغ comment(ثابت string& cmt) = 0;  // Comment detected (if keepComments==2)
    مجازي فراغ include(ثابت string& filename) = 0;  // Request a include file be processed

    مجازي فراغ undef(ثابت string& name) = 0;  // Remove a definition
    مجازي فراغ define(File_کرښه* fileline, ثابت string& name, ثابت string& ارزښت,
                        ثابت string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    مجازي فراغ define_Cmd_کرښه(File_کرښه* fileline, ثابت string& name,
                               ثابت string& ارزښت) {  // `define without any parameters
        define(fileline, name, ارزښت, "", true);
    }
    مجازي string remove_تعریفوي(ثابت string& متن) = 0;  // Remove defines in a text string

    // UTILITIES
    مجازي فراغ dump_تعریفوي(std::ostream& os) = 0;  ///< Print list of `defines
    مجازي فراغ candidate_تعریفوي(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

محمي:
    // CONSTRUCTORS
    V3PreProc() {}
    فراغ configure(File_کرښه* fl);

عام:
    ساكن V3PreProc* createPreProc(File_کرښه* fl) VL_MT_DISABLED;
    مجازي ~V3PreProc() = افتراضي;  // LCOV_EXCL_LINE  // Persistent
    ساكن فراغ selfTest() VL_MT_DISABLED;
};

#د_که_پای  // Guard
