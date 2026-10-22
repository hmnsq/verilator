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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3BEGIN_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3BEGIN_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class AstNetlist;
class Ast_ਗੰਢ;
class AstForeach;

//============================================================================

class V3Begin final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ debeginAll(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ* convert_ਮੰਜ਼ਿਲ_While(AstForeach* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
