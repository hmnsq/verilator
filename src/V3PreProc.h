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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3PREPROC_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3PREPROC_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <iostream>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

ಸಂರಕ್ಷಿತ:
    VL_DEFINE_DEBUG_FUNCTIONS;

ಸಾರ್ವಜನಿಕ:
    // CONSTANTS
    ಎಣಿಕೆ MiscConsts : ಪೂರ್ಣಾಂಕ {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_ಆಳ_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        ಹರಿವು_ಆಳ_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ openFile(File_ಸಾಲು* fileline, VInFilter* filterp, ಸ್ಥಿರಾಂಕ string& filename) = 0;
    ವಾಸ್ತವಿಕ string getline() = 0;  // Return next line/lines. (Null if done.)
    ವಾಸ್ತವಿಕ bool isEof() ಸ್ಥಿರಾಂಕ = 0;  // Return true on EOF.
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಸೇರಿಸು_Unreadback(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) = 0;

    File_ಸಾಲು* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ keepComments() { ಹಿಂತಿರುಗಿ 2; }  // Return comments, 0=no, 1=yes, 2=callback
    ಸ್ಥಿರ bool keepWhitespace() { ಹಿಂತಿರುಗಿ false; }
    ಸ್ಥಿರ bool ಸಾಲು_Directives() {  // Insert `line directives
        ಹಿಂತಿರುಗಿ !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_ಸಾಲು());
    }
    ಸ್ಥಿರ bool pedantic() { ಹಿಂತಿರುಗಿ false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ comment(ಸ್ಥಿರಾಂಕ string& cmt) = 0;  // Comment detected (if keepComments==2)
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ include(ಸ್ಥಿರಾಂಕ string& filename) = 0;  // Request a include file be processed

    ವಾಸ್ತವಿಕ ಶೂನ್ಯ undef(ಸ್ಥಿರಾಂಕ string& name) = 0;  // Remove a definition
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ define(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ,
                        ಸ್ಥಿರಾಂಕ string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ define_Cmd_ಸಾಲು(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name,
                               ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {  // `define without any parameters
        define(fileline, name, ಮೌಲ್ಯ, "", true);
    }
    ವಾಸ್ತವಿಕ string remove_ನಿರೂಪಿಸುತ್ತದೆ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) = 0;  // Remove defines in a text string

    // UTILITIES
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ dump_ನಿರೂಪಿಸುತ್ತದೆ(std::ostream& os) = 0;  ///< Print list of `defines
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ candidate_ನಿರೂಪಿಸುತ್ತದೆ(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTORS
    V3PreProc() {}
    ಶೂನ್ಯ configure(File_ಸಾಲು* fl);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3PreProc* createPreProc(File_ಸಾಲು* fl) VL_MT_DISABLED;
    ವಾಸ್ತವಿಕ ~V3PreProc() = ಪೂರ್ವನಿಯೋಜಿತ;  // LCOV_EXCL_LINE  // Persistent
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
