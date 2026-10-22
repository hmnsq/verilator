// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Loop unroller
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3UNROLL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3UNROLL_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

//============================================================================
// Generate loop unroller

class UnrollGenVisitor;

class GenForUnroller final {
    // MEMBERS
    UnrollGenVisitor* ਅਚਲ m_unrollerp;  // Unroller implementation class

ਜਨਤਕ:
    // CONSTRUCTOR
    GenForUnroller() VL_MT_DISABLED;
    ~GenForUnroller() VL_MT_DISABLED;
    // METHODS
    ਖਾਲੀ unroll(AstGenFor* nodep, ਅਚਲ std::string& beginName) VL_MT_DISABLED;
};

//============================================================================
// Loop statement unroller

class V3Unroll final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ unrollAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
