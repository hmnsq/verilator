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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3PREPROC_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3PREPROC_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iostream>
#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

ਸੁਰੱਖਿਅਤ:
    VL_DEFINE_DEBUG_FUNCTIONS;

ਜਨਤਕ:
    // CONSTANTS
    ਗਿਣਤੀ MiscConsts : ਪੂਰਨ {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_ਡੂੰਘਾਈ_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        ਧਾਰਾ_ਡੂੰਘਾਈ_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    ਆਭਾਸੀ ਖਾਲੀ openFile(File_ਸਤਰ* fileline, VInFilter* filterp, ਅਚਲ string& filename) = 0;
    ਆਭਾਸੀ string getline() = 0;  // Return next line/lines. (Null if done.)
    ਆਭਾਸੀ bool isEof() ਅਚਲ = 0;  // Return true on EOF.
    ਆਭਾਸੀ ਖਾਲੀ ਪਾਓ_Unreadback(ਅਚਲ string& ਪਾਠ) = 0;

    File_ਸਤਰ* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ਸਥਿਰ ਪੂਰਨ keepComments() { ਵਾਪਸ 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ਸਥਿਰ bool keepWhitespace() { ਵਾਪਸ false; }
    ਸਥਿਰ bool ਸਤਰ_Directives() {  // Insert `line directives
        ਵਾਪਸ !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_ਸਤਰ());
    }
    ਸਥਿਰ bool pedantic() { ਵਾਪਸ false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    ਆਭਾਸੀ ਖਾਲੀ comment(ਅਚਲ string& cmt) = 0;  // Comment detected (if keepComments==2)
    ਆਭਾਸੀ ਖਾਲੀ include(ਅਚਲ string& filename) = 0;  // Request a include file be processed

    ਆਭਾਸੀ ਖਾਲੀ undef(ਅਚਲ string& name) = 0;  // Remove a definition
    ਆਭਾਸੀ ਖਾਲੀ define(File_ਸਤਰ* fileline, ਅਚਲ string& name, ਅਚਲ string& ਮੁੱਲ,
                        ਅਚਲ string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    ਆਭਾਸੀ ਖਾਲੀ define_Cmd_ਸਤਰ(File_ਸਤਰ* fileline, ਅਚਲ string& name,
                               ਅਚਲ string& ਮੁੱਲ) {  // `define without any parameters
        define(fileline, name, ਮੁੱਲ, "", true);
    }
    ਆਭਾਸੀ string remove_ਨਿਰਧਾਰਤ_ਕਰਦਾ(ਅਚਲ string& ਪਾਠ) = 0;  // Remove defines in a text string

    // UTILITIES
    ਆਭਾਸੀ ਖਾਲੀ dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ(std::ostream& os) = 0;  ///< Print list of `defines
    ਆਭਾਸੀ ਖਾਲੀ candidate_ਨਿਰਧਾਰਤ_ਕਰਦਾ(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTORS
    V3PreProc() {}
    ਖਾਲੀ configure(File_ਸਤਰ* fl);

ਜਨਤਕ:
    ਸਥਿਰ V3PreProc* createPreProc(File_ਸਤਰ* fl) VL_MT_DISABLED;
    ਆਭਾਸੀ ~V3PreProc() = ਮੂਲ;  // LCOV_EXCL_LINE  // Persistent
    ਸਥਿਰ ਖਾਲੀ selfTest() VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
