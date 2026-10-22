// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity inst domains
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

#अगर_अघोषित VERILATOR_V3INST_H_
#घोषणा VERILATOR_V3INST_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class AstAssignW;
class AstCell;
class AstNetlist;
class AstPin;

//============================================================================

class V3Inst final {
खुला:
    अटल व्योम instAll(AstNetlist* nodep) VL_MT_DISABLED;
    अटल AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, bool forTristate,
                                          bool alwaysCvt = false) VL_MT_DISABLED;
    अटल व्योम checkOutputShort(स्थिर AstPin* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
