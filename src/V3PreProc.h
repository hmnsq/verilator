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

#నిర్వచితం_కాకపోతే VERILATOR_V3PREPROC_H_
#నిర్వచించు VERILATOR_V3PREPROC_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Global.h"

#అన్తర్భావ <iostream>
#అన్తర్భావ <list>
#అన్తర్భావ <map>

class VInFilter;
class VSpellCheck;

class V3PreProc VL_NOT_FINAL {
    // This defines a preprocessor.  Functions are virtual so implementation can be hidden.
    // After creating, call open(), then getline() in a loop.  The class will to the rest...

రక్షిత:
    VL_DEFINE_DEBUG_FUNCTIONS;

బహిరంగ:
    // CONSTANTS
    గణన MiscConsts : పూర్ణాంకం {
        DEFINE_RECURSION_LEVEL_MAX = 1000,  // How many `def substitutions before an error
        INCLUDE_లోతు_MAX = 500,  // How many `includes deep before an error
        // Streams deep (sometimes `def deep) before an error.
        // Set more than DEFINE_RECURSION_LEVEL_MAX or INCLUDE_DEPTH_MAX.
        ప్రవాహం_లోతు_LEVEL_MAX = 2000,
        NEWLINES_VS_TICKLINE = 20  // Use `line in place of this many newlines
    };

    // ACCESSORS
    // Insert given file into this point in input stream
    మాయా శూన్య openFile(File_పంక్తి* fileline, VInFilter* filterp, స్థిరరాశి string& filename) = 0;
    మాయా string getline() = 0;  // Return next line/lines. (Null if done.)
    మాయా bool isEof() స్థిరరాశి = 0;  // Return true on EOF.
    మాయా శూన్య చొప్పించు_Unreadback(స్థిరరాశి string& పాఠ్యం) = 0;

    File_పంక్తి* fileline() VL_MT_DISABLED;  ///< File/Line number for last getline call

    // CONTROL METHODS
    // These options control how the parsing proceeds
    స్థిర పూర్ణాంకం keepComments() { తిరిగి 2; }  // Return comments, 0=no, 1=yes, 2=callback
    స్థిర bool keepWhitespace() { తిరిగి false; }
    స్థిర bool పంక్తి_Directives() {  // Insert `line directives
        తిరిగి !(v3Global.opt.preprocOnly() && v3Global.opt.preproc_No_పంక్తి());
    }
    స్థిర bool pedantic() { తిరిగి false; }  // Obey standard; Don't substitute `error

    // CALLBACK METHODS
    // This probably will want to be overridden for given child users of this class.
    మాయా శూన్య comment(స్థిరరాశి string& cmt) = 0;  // Comment detected (if keepComments==2)
    మాయా శూన్య include(స్థిరరాశి string& filename) = 0;  // Request a include file be processed

    మాయా శూన్య undef(స్థిరరాశి string& name) = 0;  // Remove a definition
    మాయా శూన్య define(File_పంక్తి* fileline, స్థిరరాశి string& name, స్థిరరాశి string& విలువ,
                        స్థిరరాశి string& params = "",
                        bool cmdline = false)
        = 0;  // `define without any parameters
    మాయా శూన్య define_Cmd_పంక్తి(File_పంక్తి* fileline, స్థిరరాశి string& name,
                               స్థిరరాశి string& విలువ) {  // `define without any parameters
        define(fileline, name, విలువ, "", true);
    }
    మాయా string remove_నిర్వచిస్తుంది(స్థిరరాశి string& పాఠ్యం) = 0;  // Remove defines in a text string

    // UTILITIES
    మాయా శూన్య dump_నిర్వచిస్తుంది(std::ostream& os) = 0;  ///< Print list of `defines
    మాయా శూన్య candidate_నిర్వచిస్తుంది(VSpellCheck* spellerp) = 0;  ///< Spell check candidate defines

రక్షిత:
    // CONSTRUCTORS
    V3PreProc() {}
    శూన్య configure(File_పంక్తి* fl);

బహిరంగ:
    స్థిర V3PreProc* createPreProc(File_పంక్తి* fl) VL_MT_DISABLED;
    మాయా ~V3PreProc() = అప్రమేయ;  // LCOV_EXCL_LINE  // Persistent
    స్థిర శూన్య selfTest() VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
