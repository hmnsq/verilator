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
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::ಅಂಕೆ_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ಧ್ರುವ std::string m_fileBaseName = EmitCUtil::ತುದಿ_Class_Name() + "__ConstPool";

    // METHODS
    ಶೂನ್ಯ emitVars(ಧ್ರುವ AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ಧ್ರುವ AstVar*> varps;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) varps.ತಳ್ಳು_back(varp);
        }

        ಯದಿ (varps.empty()) ಪ್ರತ್ಯಾಗಮ;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ಧ್ರುವ AstVar* ap, ಧ್ರುವ AstVar* bp) {  //
            ಪ್ರತ್ಯಾಗಮ ap->name() < bp->name();
        });

        ಪ್ರತಿ (ಧ್ರುವ AstVar* varp : varps) {
            ಯದಿ (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ಸತ್ಯ);
                // Close old file
                closeOutputFile();
            }

            ಯದಿ (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), ಸತ್ಯ, ಅಸತ್ಯ,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            ಯದಿ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([ಏತತ್](ಧ್ರುವ AstConst* constp) {
                ಯದಿ (constp->ಅಂಕೆ().isString()) {
                    splitSizeInc(Ast_ಗಂಟು::INSTR_ಎಣಿಕೆ_STR);
                } ಅನ್ಯಥಾ ಯದಿ (constp->isWide()) {
                    splitSizeInc(constp->width_ಪದಗಳು());
                } ಅನ್ಯಥಾ {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        ಯದಿ (ofp()) closeOutputFile();
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

ಸಾರ್ವಜನಿಕ:
    explicit EmitCConstPool(ಧ್ರುವ AstPackage* poolp) {
        emitVars(poolp);
        ಧ್ರುವ auto addStat = [ಏತತ್](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ t = VNType::first_ಗಂಟು_Cons; t <= VNType::last_ಗಂಟು_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

ಶೂನ್ಯ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
