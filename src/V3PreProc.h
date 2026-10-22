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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Global.h"

#অন্তর্ভাব <iostream>
#অন্তর্ভাব <list>
#অন্তর্ভাব <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

সুরক্ষিত:
    VL_DEFINE_DEBUG_FUNCTIONS;

সর্বজনীন:
    // CONSTANTS
    গণনা MiscConsts : পূর্ণসংখ্যা {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    আভাসী শূন্য openFile(File_সারি* fileline, VInFilter* filterp, ধ্রুবক string& filename) = 0;
    আভাসী string getline() = 0;  // Return next line/lines. (Null if done.)
    আভাসী bool isEof() ধ্রুবক = 0;  // Return true on EOF.
    আভাসী শূন্য ঢোকাও_Unreadback(ধ্রুবক string& text) = 0;

    File_সারি* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    স্থির পূর্ণসংখ্যা keepComments() { ফেরত 2; }  // Return comments, 0=no, 1=yes, 2=callback
    স্থির bool keepWhitespace() { ফেরত false; }
    স্থির bool সারি_Directives() {  // Insert `line directives
        ফেরত !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_সারি());
    }
    স্থির bool pedantic() { ফেরত false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    আভাসী শূন্য comment(ধ্রুবক string& cmt) = 0;  // Comment detected (if keepComments==2)
    আভাসী শূন্য include(ধ্রুবক string& filename) = 0;  // Request a include file be processed

    আভাসী শূন্য undef(ধ্রুবক string& name) = 0;  // Remove a definition
    আভাসী শূন্য define(File_সারি* fileline, ধ্রুবক string& name, ধ্রুবক string& মান,
                        ধ্রুবক string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    আভাসী শূন্য define_Cmd_সারি(File_সারি* fileline, ধ্রুবক string& name,
                               ধ্রুবক string& মান) {  // `define without any parameters
        define(fileline, name, মান, "", true);
    }
    আভাসী string removeDefines(ধ্রুবক string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    আভাসী শূন্য dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    আভাসী শূন্য candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

সুরক্ষিত:
    // CONSTRUCTORS
    V3PreProc() {}
    শূন্য configure(File_সারি* fl);

সর্বজনীন:
    স্থির V3PreProc* createPreProc(File_সারি* fl) VL_MT_DISABLED;
    আভাসী ~V3PreProc() = পূর্বনির্ধারিত;  // LCOV_EXCL_LINE  // Persistent
    স্থির শূন্য selfTest() VL_MT_DISABLED;
};

#endif  // Guard
