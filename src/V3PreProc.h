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

#अगर_अघोषित VERILATOR_V3PREPROC_H_
#घोषणा VERILATOR_V3PREPROC_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3Global.h"

#समावेश <iostream>
#समावेश <list>
#समावेश <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

रक्षित:
    VL_DEFINE_DEBUG_FUNCTIONS;

खुला:
    // CONSTANTS
    क्रमागत MiscConsts : पूर्णांक {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    भव व्योम openFile(File_पंक्ति* fileline, VInFilter* filterp, स्थिर string& filename) = 0;
    भव string getline() = 0;  // Return next line/lines. (Null if done.)
    भव bool isEof() स्थिर = 0;  // Return true on EOF.
    भव व्योम जोड़ो_Unreadback(स्थिर string& text) = 0;

    File_पंक्ति* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    अटल पूर्णांक keepComments() { वापस 2; }  // Return comments, 0=no, 1=yes, 2=callback
    अटल bool keepWhitespace() { वापस false; }
    अटल bool पंक्ति_Directives() {  // Insert `line directives
        वापस !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_पंक्ति());
    }
    अटल bool pedantic() { वापस false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    भव व्योम comment(स्थिर string& cmt) = 0;  // Comment detected (if keepComments==2)
    भव व्योम include(स्थिर string& filename) = 0;  // Request a include file be processed

    भव व्योम undef(स्थिर string& name) = 0;  // Remove a definition
    भव व्योम define(File_पंक्ति* fileline, स्थिर string& name, स्थिर string& मूल्य,
                        स्थिर string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    भव व्योम define_Cmd_पंक्ति(File_पंक्ति* fileline, स्थिर string& name,
                               स्थिर string& मूल्य) {  // `define without any parameters
        define(fileline, name, मूल्य, "", true);
    }
    भव string removeDefines(स्थिर string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    भव व्योम dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    भव व्योम candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

रक्षित:
    // CONSTRUCTORS
    V3PreProc() {}
    व्योम configure(File_पंक्ति* fl);

खुला:
    अटल V3PreProc* createPreProc(File_पंक्ति* fl) VL_MT_DISABLED;
    भव ~V3PreProc() = शेष;  // LCOV_EXCL_LINE  // Persistent
    अटल व्योम selfTest() VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
