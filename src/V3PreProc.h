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

#ifndef VERILATOR_V3PREPROC_H_
#define VERILATOR_V3PREPROC_H_

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

محافظتشده:
    VL_DEFINE_DEBUG_FUNCTIONS;

عمومی:
    // CONSTANTS
    شمارش MiscConsts : صحیح {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    مجازی تهی openFile(File_سطر* fileline, VInFilter* filterp, ثابت string& filename) = 0;
    مجازی string getline() = 0;  // Return next line/lines. (Null if done.)
    مجازی bool isEof() ثابت = 0;  // Return true on EOF.
    مجازی تهی درج_Unreadback(ثابت string& text) = 0;

    File_سطر* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ایستا صحیح keepComments() { بازگشت 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ایستا bool keepWhitespace() { بازگشت false; }
    ایستا bool سطر_Directives() {  // Insert `line directives
        بازگشت !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_سطر());
    }
    ایستا bool pedantic() { بازگشت false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    مجازی تهی comment(ثابت string& cmt) = 0;  // Comment detected (if keepComments==2)
    مجازی تهی include(ثابت string& filename) = 0;  // Request a include file be processed

    مجازی تهی undef(ثابت string& name) = 0;  // Remove a definition
    مجازی تهی define(File_سطر* fileline, ثابت string& name, ثابت string& مقدار,
                        ثابت string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    مجازی تهی define_Cmd_سطر(File_سطر* fileline, ثابت string& name,
                               ثابت string& مقدار) {  // `define without any parameters
        define(fileline, name, مقدار, "", true);
    }
    مجازی string removeDefines(ثابت string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    مجازی تهی dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    مجازی تهی candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

محافظتشده:
    // CONSTRUCTORS
    V3PreProc() {}
    تهی configure(File_سطر* fl);

عمومی:
    ایستا V3PreProc* createPreProc(File_سطر* fl) VL_MT_DISABLED;
    مجازی ~V3PreProc() = پیشفرض;  // LCOV_EXCL_LINE  // Persistent
    ایستا تهی selfTest() VL_MT_DISABLED;
};

#endif  // Guard
