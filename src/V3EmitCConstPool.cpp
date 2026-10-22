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
    std::array<VDouble0, VNType::رەقەم_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::چوققا_Class_Name() + "__ConstPool";

    // METHODS
    فراغ emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ثابت AstVar*> varps;
        لكل (Ast_تۈگۈن* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.ئىتتىرىش_back(varp);
        }

        إذا (varps.empty()) أرجع;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstVar* varp : varps) {
            إذا (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(true);
                // Close old file
                closeOutputFile();
            }

            إذا (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), true, خطأ,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            إذا (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([هذا](ثابت AstConst* constp) {
                إذا (constp->رەقەم().isString()) {
                    splitSizeInc(Ast_تۈگۈن::INSTR_سانى_STR);
                } وإلا إذا (constp->isWide()) {
                    splitSizeInc(constp->width_سۆزلەر());
                } وإلا {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        إذا (ofp()) closeOutputFile();
    }

    // VISITORS
    فراغ visit(Ast_تۈگۈن* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عام:
    explicit EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت auto addStat = [هذا](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        لكل (صحيح t = VNType::first_تۈگۈن_Cons; t <= VNType::last_تۈگۈن_Cons; ++t) {
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
