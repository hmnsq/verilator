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

#اگر_تعریف_نشده VERILATOR_V3PREPROC_H_
#تعریف_کن VERILATOR_V3PREPROC_H_

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
        INCLUDE_عمق_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        جریان_عمق_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    مجازی فراغ openFile(File_سطر* fileline, VInFilter* filterp, ثابت string& filename) = 0;
    مجازی string getline() = 0;  // Return next line/lines. (Null if done.)
    مجازی bool isEof() ثابت = 0;  // Return true on EOF.
    مجازی فراغ درج_Unreadback(ثابت string& متن) = 0;

    File_سطر* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ساكن صحيح keepComments() { أرجع 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ساكن bool keepWhitespace() { أرجع false; }
    ساكن bool سطر_Directives() {  // Insert `line directives
        أرجع !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_سطر());
    }
    ساكن bool pedantic() { أرجع false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    مجازی فراغ comment(ثابت string& cmt) = 0;  // Comment detected (if keepComments==2)
    مجازی فراغ include(ثابت string& filename) = 0;  // Request a include file be processed

    مجازی فراغ undef(ثابت string& name) = 0;  // Remove a definition
    مجازی فراغ define(File_سطر* fileline, ثابت string& name, ثابت string& مقدار,
                        ثابت string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    مجازی فراغ define_Cmd_سطر(File_سطر* fileline, ثابت string& name,
                               ثابت string& مقدار) {  // `define without any parameters
        define(fileline, name, مقدار, "", true);
    }
    مجازی string remove_تعریف_میکند(ثابت string& متن) = 0;  // Remove defines in a text string

    // UTILITIES
    مجازی فراغ dump_تعریف_میکند(std::ostream& os) = 0;  ///< Print list of `defines
    مجازی فراغ candidate_تعریف_میکند(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

محمي:
    // CONSTRUCTORS
    V3PreProc() {}
    فراغ configure(File_سطر* fl);

عام:
    ساكن V3PreProc* createPreProc(File_سطر* fl) VL_MT_DISABLED;
    مجازی ~V3PreProc() = افتراضي;  // LCOV_EXCL_LINE  // Persistent
    ساكن فراغ selfTest() VL_MT_DISABLED;
};

#پایان_اگر  // Guard
