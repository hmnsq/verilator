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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitC.h"
#ضمن "V3EmitCConstInit.h"
#ضمن "V3File.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

#ضمن <algorithm>
#ضمن <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

صنف EmitCConstPool final : عام EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::NUM_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::topClassName() + "__ConstPool";

    // METHODS
    فراغ emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ثابت AstVar*> varps;
        لكل (AstNode* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ܐܢ (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.push_back(varp);
        }

        ܐܢ (varps.empty()) ܦܢܐ;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            ܦܢܐ ap->name() < bp->name();
        });

        لكل (ثابت AstVar* varp : varps) {
            ܐܢ (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ܫܪܪܐ);
                // Close old file
                closeOutputFile();
            }

            ܐܢ (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), ܫܪܪܐ, ܕܓܠܐ,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            ܐܢ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([هذا](ثابت AstConst* constp) {
                ܐܢ (constp->num().isString()) {
                    splitSizeInc(AstNode::INSTR_COUNT_STR);
                } ܐܚܪܢܐ ܐܢ (constp->isWide()) {
                    splitSizeInc(constp->widthWords());
                } ܐܚܪܢܐ {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        ܐܢ (ofp()) closeOutputFile();
    }

    // VISITORS
    فراغ visit(AstNode* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عام:
    explicit EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت auto addStat = [هذا](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        لكل (صحيح t = VNType::firstNodeCons; t <= VNType::lastNodeCons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

فراغ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
