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

#ifndef VERILATOR_V3UNROLL_H_
#define VERILATOR_V3UNROLL_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"

//============================================================================
// Generate loop unroller

class UnrollGenVisitor;

class GenForUnroller final {
    // MEMBERS
    UnrollGenVisitor* സ്ഥിരം m_unrollerp;  // Unroller implementation class

പൊതു:
    // CONSTRUCTOR
    GenForUnroller() VL_MT_DISABLED;
    ~GenForUnroller() VL_MT_DISABLED;
    // METHODS
    ശൂന്യം unroll(AstGenFor* nodep, സ്ഥിരം std::string& beginName) VL_MT_DISABLED;
};

//============================================================================
// Loop statement unroller

class V3Unroll final {
പൊതു:
    സ്ഥിതം ശൂന്യം unrollAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
