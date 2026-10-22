// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Inline small CFuncs into their callers
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

#अगर_अघोषित VERILATOR_V3INLINECFUNCS_H_
#घोषणा VERILATOR_V3INLINECFUNCS_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class AstNetlist;

class V3InlineCFuncs final {
खुला:
    अटल व्योम inlineAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
