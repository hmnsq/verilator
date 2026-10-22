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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Global.h"

#அந்தர்பாவ <iostream>
#அந்தர்பாவ <list>
#அந்தர்பாவ <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

பாதுகாக்கப்பட்ட:
    VL_DEFINE_DEBUG_FUNCTIONS;

பொது:
    // CONSTANTS
    எண்ணிக்கை MiscConsts : முழு {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    மெய்நிகர் வெறுமை openFile(File_வரி* fileline, VInFilter* filterp, மாறிலி string& filename) = 0;
    மெய்நிகர் string getline() = 0;  // Return next line/lines. (Null if done.)
    மெய்நிகர் bool isEof() மாறிலி = 0;  // Return true on EOF.
    மெய்நிகர் வெறுமை செருகு_Unreadback(மாறிலி string& text) = 0;

    File_வரி* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    நிலையான முழு keepComments() { திரும்பு 2; }  // Return comments, 0=no, 1=yes, 2=callback
    நிலையான bool keepWhitespace() { திரும்பு false; }
    நிலையான bool வரி_Directives() {  // Insert `line directives
        திரும்பு !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_வரி());
    }
    நிலையான bool pedantic() { திரும்பு false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    மெய்நிகர் வெறுமை comment(மாறிலி string& cmt) = 0;  // Comment detected (if keepComments==2)
    மெய்நிகர் வெறுமை include(மாறிலி string& filename) = 0;  // Request a include file be processed

    மெய்நிகர் வெறுமை undef(மாறிலி string& name) = 0;  // Remove a definition
    மெய்நிகர் வெறுமை define(File_வரி* fileline, மாறிலி string& name, மாறிலி string& மதிப்பு,
                        மாறிலி string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    மெய்நிகர் வெறுமை define_Cmd_வரி(File_வரி* fileline, மாறிலி string& name,
                               மாறிலி string& மதிப்பு) {  // `define without any parameters
        define(fileline, name, மதிப்பு, "", true);
    }
    மெய்நிகர் string removeDefines(மாறிலி string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    மெய்நிகர் வெறுமை dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    மெய்நிகர் வெறுமை candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

பாதுகாக்கப்பட்ட:
    // CONSTRUCTORS
    V3PreProc() {}
    வெறுமை configure(File_வரி* fl);

பொது:
    நிலையான V3PreProc* createPreProc(File_வரி* fl) VL_MT_DISABLED;
    மெய்நிகர் ~V3PreProc() = இயல்பு;  // LCOV_EXCL_LINE  // Persistent
    நிலையான வெறுமை selfTest() VL_MT_DISABLED;
};

#endif  // Guard
