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

#اگر_غیر_تعریف_شدہ VERILATOR_V3PREPROC_H_
#تعریف_کرو VERILATOR_V3PREPROC_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Global.h"

#شامل <iostream>
#شامل <list>
#شامل <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

محفوظ:
    VL_DEFINE_DEBUG_FUNCTIONS;

عوامی:
    // CONSTANTS
    شمار MiscConsts : صحیح {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_گہرائی_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        دھارا_گہرائی_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    مجازی خالی openFile(File_سطر* fileline, VInFilter* filterp, ثابت string& filename) = 0;
    مجازی string getline() = 0;  // Return next line/lines. (Null if done.)
    مجازی bool isEof() ثابت = 0;  // Return true on EOF.
    مجازی خالی داخل_کرو_Unreadback(ثابت string& متن) = 0;

    File_سطر* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ساکن صحیح keepComments() { واپس 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ساکن bool keepWhitespace() { واپس false; }
    ساکن bool سطر_Directives() {  // Insert `line directives
        واپس !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_سطر());
    }
    ساکن bool pedantic() { واپس false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    مجازی خالی comment(ثابت string& cmt) = 0;  // Comment detected (if keepComments==2)
    مجازی خالی include(ثابت string& filename) = 0;  // Request a include file be processed

    مجازی خالی undef(ثابت string& name) = 0;  // Remove a definition
    مجازی خالی define(File_سطر* fileline, ثابت string& name, ثابت string& قدر,
                        ثابت string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    مجازی خالی define_Cmd_سطر(File_سطر* fileline, ثابت string& name,
                               ثابت string& قدر) {  // `define without any parameters
        define(fileline, name, قدر, "", true);
    }
    مجازی string remove_متعین_کرتا(ثابت string& متن) = 0;  // Remove defines in a text string

    // UTILITIES
    مجازی خالی dump_متعین_کرتا(std::ostream& os) = 0;  ///< Print list of `defines
    مجازی خالی candidate_متعین_کرتا(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

محفوظ:
    // CONSTRUCTORS
    V3PreProc() {}
    خالی configure(File_سطر* fl);

عوامی:
    ساکن V3PreProc* createPreProc(File_سطر* fl) VL_MT_DISABLED;
    مجازی ~V3PreProc() = طےشدہ;  // LCOV_EXCL_LINE  // Persistent
    ساکن خالی selfTest() VL_MT_DISABLED;
};

#اگر_ختم  // Guard
