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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

class AstNetlist;
class Ast_ಗಂಟು;
class AstForeach;

//============================================================================

class V3Begin final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ debeginAll(AstNetlist* nodep) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು* convertToWhile(AstForeach* nodep) VL_MT_DISABLED;
};

#endif  // Guard
