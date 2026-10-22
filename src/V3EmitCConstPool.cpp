// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCConstInit.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

ವರ್ಗ EmitCConstPool final : ಸಾರ್ವಜನಿಕ EmitCConstInit {
    // TYPES
    ಬಳಸಿ ಹೊರಹೋಗು_CFile_Pair = std::pair<V3_ಹೊರಹೋಗು_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::ಅಂಕೆ_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_ಹೆಸರುಗಳು m_unique_ಹೆಸರುಗಳು;  // Generates unique file names
    ಸ್ಥಿರಾಂಕ std::string m_file_ತಳ_Name = EmitCUtil::ತುದಿ_Class_Name() + "__ConstPool";

    // METHODS
    ಶೂನ್ಯ emitVars(ಸ್ಥಿರಾಂಕ AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ AstVar*> varps;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) varps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
        }

        ಆದರೆ (varps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;  // Constant pool is empty, so we are done

        stable_ವಿಂಗಡಿಸು(varps.begin(), varps.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ AstVar* ap, ಸ್ಥಿರಾಂಕ AstVar* bp) {  //
            ಹಿಂತಿರುಗಿ ap->name() < bp->name();
        });

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* varp : varps) {
            ಆದರೆ (ವಿಭಜಿಸು_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ನಿಜ);
                // Close old file
                closeOutputFile();
            }

            ಆದರೆ (!ofp()) {
                open_New_Output_ಮೂಲ_File(m_unique_ಹೆಸರುಗಳು.get(m_file_ತಳ_Name), ನಿಜ, ಸುಳ್ಳು,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            ಆದರೆ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([ಇದು](ಸ್ಥಿರಾಂಕ AstConst* constp) {
                ಆದರೆ (constp->ಅಂಕೆ().isString()) {
                    ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(Ast_ಗಂಟು::INSTR_ಎಣಿಕೆ_STR);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->isWide()) {
                    ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(constp->ಅಗಲ_ಪದಗಳು());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        ಆದರೆ (ofp()) closeOutputFile();
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ EmitCConstPool(ಸ್ಥಿರಾಂಕ AstPackage* poolp) {
        emitVars(poolp);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Stat = [ಇದು](VNType type) {
            V3Stats::ಕೂಡಿಸು_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        ಕೂಡಿಸು_Stat(VNType::Const);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ t = VNType::ಮೊದಲ_ಗಂಟು_Cons; t <= VNType::ಕೊನೆಯ_ಗಂಟು_Cons; ++t) {
            ಕೂಡಿಸು_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

ಶೂನ್ಯ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
