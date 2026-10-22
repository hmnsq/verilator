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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

//============================================================================
// Generate loop unroller

class UnrollGenVisitor;

class GenForUnroller final {
    // MEMBERS
    UnrollGenVisitor* ثابت m_unrollerp;  // Unroller implementation class

عمومی:
    // CONSTRUCTOR
    GenForUnroller() VL_MT_DISABLED;
    ~GenForUnroller() VL_MT_DISABLED;
    // METHODS
    تهی unroll(AstGenFor* nodep, ثابت std::string& beginName) VL_MT_DISABLED;
};

//============================================================================
// Loop statement unroller

class V3Unroll final {
عمومی:
    ایستا تهی unrollAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
