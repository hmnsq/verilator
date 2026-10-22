// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3LINKLEVEL_H_
#నిర్వచించు VERILATOR_V3LINKLEVEL_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"

#అన్తర్భావ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::సదిశ<Ast_కణుపు_Module*>;

    స్థిర శూన్య timescaling(స్థిరరాశి ModVec& mods) VL_MT_DISABLED;
    స్థిర శూన్య wrap_శిఖరం_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    స్థిర శూన్య wrap_శిఖరం_Packages(AstNetlist* rootp) VL_MT_DISABLED;

బహిరంగ:
    స్థిర శూన్య mod_క్రమీకరించు_By_Level() VL_MT_DISABLED;
    స్థిర శూన్య wrap_శిఖరం(AstNetlist* rootp) VL_MT_DISABLED;
    స్థిర శూన్య non_Wrap_శిఖరం(AstNetlist* rootp) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
