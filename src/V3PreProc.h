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

রক্ষিত:
    VL_DEFINE_DEBUG_FUNCTIONS;

সার্বজনিক:
    // CONSTANTS
    গণনা MiscConsts : পূর্ণাঙ্ক {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    আভাস শূন্য openFile(File_সারি* fileline, VInFilter* filterp, ধ্রুব string& filename) = 0;
    আভাস string getline() = 0;  // Return next line/lines. (Null if done.)
    আভাস bool isEof() ধ্রুব = 0;  // Return true on EOF.
    আভাস শূন্য ঢোকাও_Unreadback(ধ্রুব string& text) = 0;

    File_সারি* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    স্থির পূর্ণাঙ্ক keepComments() { প্রত্যাগম 2; }  // Return comments, 0=no, 1=yes, 2=callback
    স্থির bool keepWhitespace() { প্রত্যাগম false; }
    স্থির bool সারি_Directives() {  // Insert `line directives
        প্রত্যাগম !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_সারি());
    }
    স্থির bool pedantic() { প্রত্যাগম false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    আভাস শূন্য comment(ধ্রুব string& cmt) = 0;  // Comment detected (if keepComments==2)
    আভাস শূন্য include(ধ্রুব string& filename) = 0;  // Request a include file be processed

    আভাস শূন্য undef(ধ্রুব string& name) = 0;  // Remove a definition
    আভাস শূন্য define(File_সারি* fileline, ধ্রুব string& name, ধ্রুব string& মান,
                        ধ্রুব string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    আভাস শূন্য define_Cmd_সারি(File_সারি* fileline, ধ্রুব string& name,
                               ধ্রুব string& মান) {  // `define without any parameters
        define(fileline, name, মান, "", true);
    }
    আভাস string removeDefines(ধ্রুব string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    আভাস শূন্য dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    আভাস শূন্য candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

রক্ষিত:
    // CONSTRUCTORS
    V3PreProc() {}
    শূন্য configure(File_সারি* fl);

সার্বজনিক:
    স্থির V3PreProc* createPreProc(File_সারি* fl) VL_MT_DISABLED;
    আভাস ~V3PreProc() = স্বভাব;  // LCOV_EXCL_LINE  // Persistent
    স্থির শূন্য selfTest() VL_MT_DISABLED;
};

#endif  // Guard
