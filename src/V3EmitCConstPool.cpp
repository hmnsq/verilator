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
    ورتوں باہرلا_CFile_Pair = std::pair<V3_باہرلا_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::عدد_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_ناں m_unique_ناں;  // Generates unique file names
    ثابت std::string m_file_ادھار_Name = EmitCUtil::چوٹی_Class_Name() + "__ConstPool";

    // METHODS
    فراغ emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::سمتیہ<ثابت AstVar*> varps;
        لكل (Ast_گرہ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.ڈالو_پٹھ(varp);
        }

        إذا (varps.خالی_تھاواں()) أرجع;  // Constant pool is empty, so we are done

        stable_ترتیب_دیو(varps.begin(), varps.سرا(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstVar* varp : varps) {
            إذا (ونڈو_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(سچ);
                // Close old file
                closeOutputFile();
            }

            إذا (!ofp()) {
                open_New_Output_سروت_File(m_unique_ناں.get(m_file_ادھار_Name), سچ, خطأ,
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
                    ونڈو_ناپ_Inc(Ast_گرہ::INSTR_گنتی_STR);
                } وإلا إذا (constp->isWide()) {
                    ونڈو_ناپ_Inc(constp->چوڑائی_الفاظ());
                } وإلا {
                    ونڈو_ناپ_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        إذا (ofp()) closeOutputFile();
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عام:
    واضح EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت خودکار جوڑو_Stat = [هذا](VNType type) {
            V3Stats::جوڑو_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        جوڑو_Stat(VNType::Const);
        لكل (صحيح t = VNType::پہلا_گرہ_Cons; t <= VNType::آخری_گرہ_Cons; ++t) {
            جوڑو_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

فراغ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
