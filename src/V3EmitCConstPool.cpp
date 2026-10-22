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
    استعمال ٻاهريون_CFile_Pair = std::pair<V3_ٻاهريون_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::انگ_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_نالا m_unique_نالا;  // Generates unique file names
    ثابت std::string m_file_بنياد_Name = EmitCUtil::چوٽي_Class_Name() + "__ConstPool";

    // METHODS
    فراغ emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::ويڪٽر<ثابت AstVar*> varps;
        لكل (Ast_ڳنڍ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.ڌڪو_پٺ(varp);
        }

        إذا (varps.خالي_جايون()) أرجع;  // Constant pool is empty, so we are done

        stable_ترتيب_ڏيو(varps.begin(), varps.پڇاڙي(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstVar* varp : varps) {
            إذا (ورهايو_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(سچ);
                // Close old file
                closeOutputFile();
            }

            إذا (!ofp()) {
                open_New_Output_ذريعو_File(m_unique_نالا.get(m_file_بنياد_Name), سچ, خطأ,
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
                إذا (constp->انگ().isString()) {
                    ورهايو_سائيز_Inc(Ast_ڳنڍ::INSTR_ڳڻپ_STR);
                } وإلا إذا (constp->isWide()) {
                    ورهايو_سائيز_Inc(constp->ويڪر_لفظ());
                } وإلا {
                    ورهايو_سائيز_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        إذا (ofp()) closeOutputFile();
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عام:
    واضح EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت خودڪار جوڙيو_Stat = [هذا](VNType type) {
            V3Stats::جوڙيو_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        جوڙيو_Stat(VNType::Const);
        لكل (صحيح t = VNType::پهريون_ڳنڍ_Cons; t <= VNType::آخري_ڳنڍ_Cons; ++t) {
            جوڙيو_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

فراغ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
