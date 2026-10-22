// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split packed variables into bit ranges
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

#నిర్వచితం_కాకపోతే VERILATOR_V3BITBLAST_H_
#నిర్వచించు VERILATOR_V3BITBLAST_H_

#అన్తర్భావ "verilatedos.h"

//============================================================================

class AstNetlist;

class V3_బిట్టు_Blast final {
బహిరంగ:
    స్థిర శూన్య బిట్టు_Blast_All(AstNetlist* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
