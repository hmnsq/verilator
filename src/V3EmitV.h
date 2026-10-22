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

#ifndef VERILATOR_V3EMITV_H_
#define VERILATOR_V3EMITV_H_

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

class Ast_ଗଣ୍ଠି;
class Ast_Sen_ବୃକ୍ଷ;

//============================================================================

class V3EmitV final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ verilog_For_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, std::ostream& os = std::cout);
    ସ୍ଥିର ଶୂନ୍ୟ debug_Verilog_For_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, std::ostream& os);
    ସ୍ଥିର std::string debug_Verilog_For_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);
    ସ୍ଥିର ଶୂନ୍ୟ emitvFiles();
    ସ୍ଥିର ଶୂନ୍ୟ debugEmitV(ଅଚଳ string& filename);
};

#endif  // Guard
