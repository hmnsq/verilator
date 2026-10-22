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

#شامل "V3PchAstMT.h"

#شامل "V3EmitC.h"
#شامل "V3EmitCConstInit.h"
#شامل "V3File.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

#شامل <algorithm>
#شامل <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

رده EmitCConstPool final : عمومی EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::رقم_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::بالاترین_Class_Name() + "__ConstPool";

    // METHODS
    تهی emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ثابت AstVar*> varps;
        برای (Ast_گره* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.افزودن_back(varp);
        }

        اگر (varps.empty()) بازگشت;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            بازگشت ap->name() < bp->name();
        });

        برای (ثابت AstVar* varp : varps) {
            اگر (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(درست);
                // Close old file
                closeOutputFile();
            }

            اگر (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), درست, نادرست,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            اگر (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([این](ثابت AstConst* constp) {
                اگر (constp->رقم().isString()) {
                    splitSizeInc(Ast_گره::INSTR_شمار_STR);
                } وگرنه اگر (constp->isWide()) {
                    splitSizeInc(constp->width_واژگان());
                } وگرنه {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        اگر (ofp()) closeOutputFile();
    }

    // VISITORS
    تهی visit(Ast_گره* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عمومی:
    explicit EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت auto addStat = [این](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        برای (صحیح t = VNType::first_گره_Cons; t <= VNType::last_گره_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

تهی V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
