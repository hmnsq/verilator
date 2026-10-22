// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break case statements up and add Unknown assigns
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

#ifndef VERILATOR_V3CASE_H_
#define VERILATOR_V3CASE_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

class AstNetlist;
class AstGenCase;

//============================================================================

class V3Case final {
പൊതു:
    സ്ഥിതം ശൂന്യം caseAll(AstNetlist* nodep) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം caseLint(AstGenCase* nodep) VL_MT_DISABLED;
};

#endif  // Guard
