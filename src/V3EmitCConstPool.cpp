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

جماعت EmitCConstPool final : عوامی EmitCConstInit {
    // TYPES
    استعمال برآمد_CFile_Pair = std::pair<V3_برآمد_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::عدد_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_نام m_unique_نام;  // Generates unique file names
    ثابت std::string m_file_بنیاد_Name = EmitCUtil::چوٹی_Class_Name() + "__ConstPool";

    // METHODS
    خالی emitVars(ثابت AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::سمتیہ<ثابت AstVar*> varps;
        برائے (Ast_گرہ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) varps.ڈالو_عقب(varp);
        }

        اگر (varps.خالی_خانے()) واپس;  // Constant pool is empty, so we are done

        stable_ترتیب_دو(varps.begin(), varps.انتہا(), [](ثابت AstVar* ap, ثابت AstVar* bp) {  //
            واپس ap->name() < bp->name();
        });

        برائے (ثابت AstVar* varp : varps) {
            اگر (بانٹو_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(سچ);
                // Close old file
                closeOutputFile();
            }

            اگر (!ofp()) {
                open_New_Output_ماخذ_File(m_unique_نام.get(m_file_بنیاد_Name), سچ, جھوٹ,
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
            varp->valuep()->foreach([یہ](ثابت AstConst* constp) {
                اگر (constp->عدد().isString()) {
                    بانٹو_سائز_Inc(Ast_گرہ::INSTR_گنتی_STR);
                } ورنہ اگر (constp->isWide()) {
                    بانٹو_سائز_Inc(constp->چوڑائی_الفاظ());
                } ورنہ {
                    بانٹو_سائز_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        اگر (ofp()) closeOutputFile();
    }

    // VISITORS
    خالی دورہ(Ast_گرہ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

عوامی:
    صریح EmitCConstPool(ثابت AstPackage* poolp) {
        emitVars(poolp);
        ثابت خودکار جوڑو_Stat = [یہ](VNType type) {
            V3Stats::جوڑو_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        جوڑو_Stat(VNType::Const);
        برائے (صحیح t = VNType::پہلا_گرہ_Cons; t <= VNType::آخری_گرہ_Cons; ++t) {
            جوڑو_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

خالی V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
