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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3EmitC.h"
#অন্তর্ভাৱ "V3EmitCConstInit.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

শ্ৰেণী EmitCConstPool final : সার্ৱজনিক EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::অঙ্ক_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ধ্রুৱ std::string m_fileBaseName = EmitCUtil::শীৰ্ষ_Class_Name() + "__ConstPool";

    // METHODS
    শূন্য emitVars(ধ্রুৱ AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ধ্রুৱ AstVar*> varps;
        বাবে (Ast_গ্ৰন্থি* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) varps.ঠেলো_back(varp);
        }

        যদি (varps.empty()) ঘূৰাই;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ধ্রুৱ AstVar* ap, ধ্রুৱ AstVar* bp) {  //
            ঘূৰাই ap->name() < bp->name();
        });

        বাবে (ধ্রুৱ AstVar* varp : varps) {
            যদি (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(সঁচা);
                // Close old file
                closeOutputFile();
            }

            যদি (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), সঁচা, মিছা,
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
            varp->valuep()->foreach([এতত্](ধ্রুৱ AstConst* constp) {
                যদি (constp->অঙ্ক().isString()) {
                    splitSizeInc(Ast_গ্ৰন্থি::INSTR_গুনতি_STR);
                } অন্যথা যদি (constp->isWide()) {
                    splitSizeInc(constp->width_শব্দগুলি());
                } অন্যথা {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        যদি (ofp()) closeOutputFile();
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

সার্ৱজনিক:
    explicit EmitCConstPool(ধ্রুৱ AstPackage* poolp) {
        emitVars(poolp);
        ধ্রুৱ auto addStat = [এতত্](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        বাবে (পূর্ণাঙ্ক t = VNType::first_গ্ৰন্থি_Cons; t <= VNType::last_গ্ৰন্থি_Cons; ++t) {
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
