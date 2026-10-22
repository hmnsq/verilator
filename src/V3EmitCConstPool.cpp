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
    کارول بهر_CFile_Pair = std::pair<V3_بهر_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::عدد_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_نومونه m_unique_نومونه;  // Generates unique file names
    ثابت std::string m_file_بنسټ_Name = EmitCUtil::سر_Class_Name() + "__ConstPool";

    // METHODS
    فراغ emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::ویکټور<ثابت AstVar*> varps;
        لكل (Ast_غوټه* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.ټېل_شا(varp);
        }

        إذا (varps.تشه()) أرجع;  // Constant pool is empty, so we are done

        stable_ترتیبول(varps.begin(), varps.پای_ټکی(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstVar* varp : varps) {
            إذا (وېشل_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ریښتیا);
                // Close old file
                closeOutputFile();
            }

            إذا (!ofp()) {
                open_New_Output_سرچینه_File(m_unique_نومونه.get(m_file_بنسټ_Name), ریښتیا, خطأ,
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
                إذا (constp->عدد().isString()) {
                    وېشل_اندازه_Inc(Ast_غوټه::INSTR_شمېر_STR);
                } وإلا إذا (constp->isWide()) {
                    وېشل_اندازه_Inc(constp->پلنوالی_ټکي());
                } وإلا {
                    وېشل_اندازه_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        إذا (ofp()) closeOutputFile();
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عام:
    څرګند EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت خپلکاری جمع_کړه_Stat = [هذا](VNType type) {
            V3Stats::جمع_کړه_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        جمع_کړه_Stat(VNType::Const);
        لكل (صحيح t = VNType::لومړی_غوټه_Cons; t <= VNType::وروستی_غوټه_Cons; ++t) {
            جمع_کړه_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

فراغ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
