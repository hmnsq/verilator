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
    استفاده بیرون_CFile_Pair = std::pair<V3_بیرون_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::رقم_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_نامها m_unique_نامها;  // Generates unique file names
    ثابت std::string m_file_پایه_Name = EmitCUtil::بالاترین_Class_Name() + "__ConstPool";

    // METHODS
    فراغ emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::وکتور<ثابت AstVar*> varps;
        لكل (Ast_گره* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.افزودن_پشت(varp);
        }

        إذا (varps.خالی()) أرجع;  // Constant pool is empty, so we are done

        stable_مرتب_سازی(varps.begin(), varps.انتها(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstVar* varp : varps) {
            إذا (تقسیم_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(درست);
                // Close old file
                closeOutputFile();
            }

            إذا (!ofp()) {
                open_New_Output_منبع_File(m_unique_نامها.get(m_file_پایه_Name), درست, خطأ,
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
                إذا (constp->رقم().isString()) {
                    تقسیم_اندازه_Inc(Ast_گره::INSTR_شمار_STR);
                } وإلا إذا (constp->isWide()) {
                    تقسیم_اندازه_Inc(constp->عرض_واژگان());
                } وإلا {
                    تقسیم_اندازه_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        إذا (ofp()) closeOutputFile();
    }

    // VISITORS
    فراغ بازدید(Ast_گره* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عام:
    صریح EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت خودکار جمع_کن_Stat = [هذا](VNType type) {
            V3Stats::جمع_کن_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        جمع_کن_Stat(VNType::Const);
        لكل (صحيح t = VNType::اولی_گره_Cons; t <= VNType::آخرین_گره_Cons; ++t) {
            جمع_کن_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

فراغ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
