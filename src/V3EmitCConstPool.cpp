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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitC.h"
#અન્તર્ભાવ "V3EmitCConstInit.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

વર્ગ EmitCConstPool final : જાહેર EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::અંક_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    અચલ std::string m_fileBaseName = EmitCUtil::ટોચ_Class_Name() + "__ConstPool";

    // METHODS
    શૂન્ય emitVars(અચલ AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<અચલ AstVar*> varps;
        માટે (Ast_ગાંઠ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) varps.ધકેલો_back(varp);
        }

        જો (varps.empty()) પાછા;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](અચલ AstVar* ap, અચલ AstVar* bp) {  //
            પાછા ap->name() < bp->name();
        });

        માટે (અચલ AstVar* varp : varps) {
            જો (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(સાચું);
                // Close old file
                closeOutputFile();
            }

            જો (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), સાચું, ખોટું,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            જો (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([આ](અચલ AstConst* constp) {
                જો (constp->અંક().isString()) {
                    splitSizeInc(Ast_ગાંઠ::INSTR_ગણતરી_STR);
                } નહીંતર જો (constp->isWide()) {
                    splitSizeInc(constp->width_શબ્દો());
                } નહીંતર {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        જો (ofp()) closeOutputFile();
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

જાહેર:
    explicit EmitCConstPool(અચલ AstPackage* poolp) {
        emitVars(poolp);
        અચલ auto addStat = [આ](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        માટે (પૂર્ણાંક t = VNType::first_ગાંઠ_Cons; t <= VNType::last_ગાંઠ_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

શૂન્ય V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
