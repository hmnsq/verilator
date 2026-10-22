// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Node attributes/ expression widths
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_ਚੌੜਾਈ_H
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_ਚੌੜਾਈ_H

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class AstNetlist;
class Ast_ਗੰਢ;
class Ast_ਗੰਢ_ਸੌਂਪੋ;
class Ast_ਗੰਢ_DType;

//============================================================================

class V3_ਚੌੜਾਈ final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ ਚੌੜਾਈ(AstNetlist* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ* ਚੌੜਾਈ_Params_Edit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ* ਚੌੜਾਈ_Generate_Params_Edit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ* selectNonConstantRecurse(Ast_ਗੰਢ* nodep, bool ਵਿੱਚ_Sel = false) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ ਧਾਰਾ_ਸੌਂਪੋ_ਹੇਠਲਾ_Edit(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) VL_MT_DISABLED;

    // For use only in WidthVisitor
    // Replace AstSelBit, etc with AstSel/AstArraySel
    // Returns replacement node if nodep was deleted, or null if none.
    ਸਥਿਰ Ast_ਗੰਢ* ਚੌੜਾਈ_Sel_No_Iter_Edit(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
