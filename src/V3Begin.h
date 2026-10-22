// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Removal of named begin blocks
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

#ifndef VERILATOR_V3BEGIN_H_
#define VERILATOR_V3BEGIN_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

class AstNetlist;
class Ast_گره;
class AstForeach;

//============================================================================

class V3Begin final {
عمومی:
    ایستا تهی debeginAll(AstNetlist* nodep) VL_MT_DISABLED;
    ایستا Ast_گره* convertToWhile(AstForeach* nodep) VL_MT_DISABLED;
};

#endif  // Guard
