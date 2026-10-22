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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Global.h"

#अन्तर्भाव <iostream>
#अन्तर्भाव <list>
#अन्तर्भाव <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

रक्षित:
    VL_DEFINE_DEBUG_FUNCTIONS;

सार्वजनिक:
    // CONSTANTS
    गणना MiscConsts : पूर्णाङ्क {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_DEPTH_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        STREAM_DEPTH_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    आभास शून्य openFile(File_पंक्ति* fileline, VInFilter* filterp, ध्रुव string& filename) = 0;
    आभास string getline() = 0;  // Return next line/lines. (Null if done.)
    आभास bool isEof() ध्रुव = 0;  // Return true on EOF.
    आभास शून्य जोड़ो_Unreadback(ध्रुव string& text) = 0;

    File_पंक्ति* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    स्थिर पूर्णाङ्क keepComments() { प्रत्यागम 2; }  // Return comments, 0=no, 1=yes, 2=callback
    स्थिर bool keepWhitespace() { प्रत्यागम false; }
    स्थिर bool पंक्ति_Directives() {  // Insert `line directives
        प्रत्यागम !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_पंक्ति());
    }
    स्थिर bool pedantic() { प्रत्यागम false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    आभास शून्य comment(ध्रुव string& cmt) = 0;  // Comment detected (if keepComments==2)
    आभास शून्य include(ध्रुव string& filename) = 0;  // Request a include file be processed

    आभास शून्य undef(ध्रुव string& name) = 0;  // Remove a definition
    आभास शून्य define(File_पंक्ति* fileline, ध्रुव string& name, ध्रुव string& मूल्य,
                        ध्रुव string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    आभास शून्य define_Cmd_पंक्ति(File_पंक्ति* fileline, ध्रुव string& name,
                               ध्रुव string& मूल्य) {  // `define without any parameters
        define(fileline, name, मूल्य, "", true);
    }
    आभास string removeDefines(ध्रुव string& text) = 0;  // Remove defines in a text string

    // UTILITIES
    आभास शून्य dumpDefines(std::ostream& os) = 0;  ///< Print list of `defines
    आभास शून्य candidateDefines(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

रक्षित:
    // CONSTRUCTORS
    V3PreProc() {}
    शून्य configure(File_पंक्ति* fl);

सार्वजनिक:
    स्थिर V3PreProc* createPreProc(File_पंक्ति* fl) VL_MT_DISABLED;
    आभास ~V3PreProc() = स्वभाव;  // LCOV_EXCL_LINE  // Persistent
    स्थिर शून्य selfTest() VL_MT_DISABLED;
};

#endif  // Guard
