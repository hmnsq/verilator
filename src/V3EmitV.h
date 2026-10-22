// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Verilog code for module tree
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3EMITV_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3EMITV_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

class Ast_ਗੰਢ;
class Ast_Sen_ਰੁੱਖ;

//============================================================================

class V3EmitV final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ verilog_For_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* nodep, std::ostream& os = std::cout);
    ਸਥਿਰ ਖਾਲੀ debug_Verilog_For_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* nodep, std::ostream& os);
    ਸਥਿਰ std::string debug_Verilog_For_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* nodep);
    ਸਥਿਰ ਖਾਲੀ emitvFiles();
    ਸਥਿਰ ਖਾਲੀ debugEmitV(ਅਚਲ string& filename);
};

#ਜੇ_ਅੰਤ  // Guard
