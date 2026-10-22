// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prepare AST for features features
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

#यदि_अपरिभाषितम् _V3TIMING_H_
#परिभाषय _V3TIMING_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

//============================================================================

class V3Timing final {
सार्वजनिक:
    स्थिर शून्य timingAll(AstNetlist* nodep) VL_MT_DISABLED;
    // Scale a delay in the given time unit to time precision ticks, rounding real delays; #1step
    // is one tick
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_गन्तव्यम्_Ticks(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, VTimescale timeunit) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
