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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3EMITV_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3EMITV_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

class Ast_ಗಂಟು;
class Ast_Sen_ವೃಕ್ಷ;

//============================================================================

class V3EmitV final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ verilog_For_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, std::ostream& os = std::cout);
    ಸ್ಥಿರ ಶೂನ್ಯ debug_Verilog_For_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, std::ostream& os);
    ಸ್ಥಿರ std::string debug_Verilog_For_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ emitvFiles();
    ಸ್ಥಿರ ಶೂನ್ಯ debugEmitV(ಸ್ಥಿರಾಂಕ string& filename);
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
