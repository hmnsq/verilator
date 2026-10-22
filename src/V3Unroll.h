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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"

//============================================================================
// Generate loop unroller

class UnrollGenVisitor;

class GenForUnroller final {
    // MEMBERS
    UnrollGenVisitor* स्थिरांक m_unrollerp;  // Unroller implementation class

सार्वजनिक:
    // CONSTRUCTOR
    GenForUnroller() VL_MT_DISABLED;
    ~GenForUnroller() VL_MT_DISABLED;
    // METHODS
    शून्य unroll(AstGenFor* nodep, स्थिरांक std::string& beginName) VL_MT_DISABLED;
};

//============================================================================
// Loop statement unroller

class V3Unroll final {
सार्वजनिक:
    स्थिर शून्य unrollAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
