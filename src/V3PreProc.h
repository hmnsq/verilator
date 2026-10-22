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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Global.h"

#અન્તર્ભાવ <iostream>
#અન્તર્ભાવ <list>
#અન્તર્ભાવ <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

સુરક્ષિત:
    VL_DEFINE_DEBUG_FUNCTIONS;

જાહેર:
    // CONSTANTS
    ગણના MiscConsts : પૂર્ણાંક {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    આભાસી શૂન્ય openFile(File_લીટી* fileline, VInFilter* filterp, અચલ string& filename) = 0;
    આભાસી string getline() = 0;  // Return next line/lines. (Null if done.)
    આભાસી bool isEof() અચલ = 0;  // Return true on EOF.
    આભાસી શૂન્ય ઉમેરો_Unreadback(અચલ string& text) = 0;

    File_લીટી* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    સ્થિર પૂર્ણાંક keepComments() { પાછા 2; }  // Return comments, 0=no, 1=yes, 2=callback
    સ્થિર bool keepWhitespace() { પાછા false; }
    સ્થિર bool લીટી_Directives() {  // Insert `line directives
        પાછા !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_લીટી());
    }
    સ્થિર bool pedantic() { પાછા false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    આભાસી શૂન્ય comment(અચલ string& cmt) = 0;  // Comment detected (if keepComments==2)
    આભાસી શૂન્ય include(અચલ string& filename) = 0;  // Request a include file be processed

    આભાસી શૂન્ય undef(અચલ string& name) = 0;  // Remove a definition
    આભાસી શૂન્ય define(File_લીટી* fileline, અચલ string& name, અચલ string& કિંમત,
                        અચલ string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    આભાસી શૂન્ય define_Cmd_લીટી(File_લીટી* fileline, અચલ string& name,
                               અચલ string& કિંમત) {  // `define without any parameters
        define(fileline, name, કિંમત, "", true);
    }
    આભાસી string removeDefines(અચલ string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    આભાસી શૂન્ય dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    આભાસી શૂન્ય candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

સુરક્ષિત:
    // CONSTRUCTORS
    V3PreProc() {}
    શૂન્ય configure(File_લીટી* fl);

જાહેર:
    સ્થિર V3PreProc* createPreProc(File_લીટી* fl) VL_MT_DISABLED;
    આભાસી ~V3PreProc() = મૂળભૂત;  // LCOV_EXCL_LINE  // Persistent
    સ્થિર શૂન્ય selfTest() VL_MT_DISABLED;
};

#endif  // Guard
