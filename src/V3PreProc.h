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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"

#ଅନ୍ତର୍ଭାଵ <iostream>
#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

ସୁରକ୍ଷିତ:
    VL_DEFINE_DEBUG_FUNCTIONS;

ସର୍ବସାଧାରଣ:
    // CONSTANTS
    ଗଣନା MiscConsts : ପୂର୍ଣ୍ଣସଂଖ୍ୟା {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    ଆଭାସୀ ଶୂନ୍ୟ openFile(File_ଧାଡ଼ି* fileline, VInFilter* filterp, ଅଚଳ string& filename) = 0;
    ଆଭାସୀ string getline() = 0;  // Return next line/lines. (Null if done.)
    ଆଭାସୀ bool isEof() ଅଚଳ = 0;  // Return true on EOF.
    ଆଭାସୀ ଶୂନ୍ୟ ଭର୍ତ୍ତି_Unreadback(ଅଚଳ string& text) = 0;

    File_ଧାଡ଼ି* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା keepComments() { ଫେରନ୍ତୁ 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ସ୍ଥିର bool keepWhitespace() { ଫେରନ୍ତୁ false; }
    ସ୍ଥିର bool ଧାଡ଼ି_Directives() {  // Insert `line directives
        ଫେରନ୍ତୁ !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_ଧାଡ଼ି());
    }
    ସ୍ଥିର bool pedantic() { ଫେରନ୍ତୁ false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    ଆଭାସୀ ଶୂନ୍ୟ comment(ଅଚଳ string& cmt) = 0;  // Comment detected (if keepComments==2)
    ଆଭାସୀ ଶୂନ୍ୟ include(ଅଚଳ string& filename) = 0;  // Request a include file be processed

    ଆଭାସୀ ଶୂନ୍ୟ undef(ଅଚଳ string& name) = 0;  // Remove a definition
    ଆଭାସୀ ଶୂନ୍ୟ define(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name, ଅଚଳ string& ମୂଲ୍ୟ,
                        ଅଚଳ string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    ଆଭାସୀ ଶୂନ୍ୟ define_Cmd_ଧାଡ଼ି(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name,
                               ଅଚଳ string& ମୂଲ୍ୟ) {  // `define without any parameters
        define(fileline, name, ମୂଲ୍ୟ, "", true);
    }
    ଆଭାସୀ string removeDefines(ଅଚଳ string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    ଆଭାସୀ ଶୂନ୍ୟ dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    ଆଭାସୀ ଶୂନ୍ୟ candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

ସୁରକ୍ଷିତ:
    // CONSTRUCTORS
    V3PreProc() {}
    ଶୂନ୍ୟ configure(File_ଧାଡ଼ି* fl);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର V3PreProc* createPreProc(File_ଧାଡ଼ି* fl) VL_MT_DISABLED;
    ଆଭାସୀ ~V3PreProc() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;  // LCOV_EXCL_LINE  // Persistent
    ସ୍ଥିର ଶୂନ୍ୟ selfTest() VL_MT_DISABLED;
};

#endif  // Guard
