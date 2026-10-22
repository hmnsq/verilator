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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3EmitC.h"
#অন্তর্ভাব "V3EmitCConstInit.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

শ্রেণী EmitCConstPool final : সর্বজনীন EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::অঙ্ক_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুবক std::string m_fileBaseName = EmitCUtil::শীর্ষ_Class_Name() + "__ConstPool";

    // METHODS
    শূন্য emitVars(ধ্রুবক AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ধ্রুবক AstVar*> varps;
        জন্য (Ast_গ্রন্থি* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) varps.ঠেলো_back(varp);
        }

        যদি (varps.empty()) ফেরত;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ধ্রুবক AstVar* ap, ধ্রুবক AstVar* bp) {  //
            ফেরত ap->name() < bp->name();
        });

        জন্য (ধ্রুবক AstVar* varp : varps) {
            যদি (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(সত্য);
                // Close old file
                closeOutputFile();
            }

            যদি (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), সত্য, মিথ্যা,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            যদি (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([এই](ধ্রুবক AstConst* constp) {
                যদি (constp->অঙ্ক().isString()) {
                    split_আকার_Inc(Ast_গ্রন্থি::INSTR_গুনতি_STR);
                } অন্যথা যদি (constp->isWide()) {
                    split_আকার_Inc(constp->width_শব্দগুলি());
                } অন্যথা {
                    split_আকার_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        যদি (ofp()) closeOutputFile();
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

সর্বজনীন:
    explicit EmitCConstPool(ধ্রুবক AstPackage* poolp) {
        emitVars(poolp);
        ধ্রুবক auto addStat = [এই](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        জন্য (পূর্ণসংখ্যা t = VNType::first_গ্রন্থি_Cons; t <= VNType::last_গ্রন্থি_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

শূন্য V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
