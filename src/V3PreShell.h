// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Preprocessing wrapper program
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3PRESHELL_H_
#నిర్వచించు VERILATOR_V3PRESHELL_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"

class V3ParseImp;
class VInFilter;
class VSpellCheck;

//============================================================================

class V3PreShell final {
    // Static class for calling preprocessor
బహిరంగ:
    స్థిర శూన్య boot() VL_MT_DISABLED;
    స్థిర శూన్య shutdown() VL_MT_DISABLED;
    స్థిర bool preproc(File_పంక్తి* fl, స్థిరరాశి string& modname, VInFilter* filterp,
                        V3ParseImp* parsep, స్థిరరాశి string& errmsg) VL_MT_DISABLED;
    స్థిర శూన్య preprocInclude(File_పంక్తి* fl, స్థిరరాశి string& modname) VL_MT_DISABLED;
    స్థిర శూన్య define_Cmd_పంక్తి(స్థిరరాశి string& name, స్థిరరాశి string& విలువ) VL_MT_DISABLED;
    స్థిర శూన్య undef(స్థిరరాశి string& name) VL_MT_DISABLED;
    స్థిర శూన్య dump_నిర్వచిస్తుంది(std::ostream& os) VL_MT_DISABLED;
    స్థిర శూన్య candidate_నిర్వచిస్తుంది(VSpellCheck* spellerp) VL_MT_DISABLED;
    స్థిర శూన్య selfTest() VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
