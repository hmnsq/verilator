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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCConstInit.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

ਵਰਗ EmitCConstPool final : ਜਨਤਕ EmitCConstInit {
    // TYPES
    ਵਰਤੋਂ ਬਾਹਰਲਾ_CFile_Pair = std::pair<V3_ਬਾਹਰਲਾ_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::ਅੰਕ_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_ਨਾਂ m_unique_ਨਾਂ;  // Generates unique file names
    ਅਚਲ std::string m_file_ਅਧਾਰ_Name = EmitCUtil::ਸਿਖਰ_Class_Name() + "__ConstPool";

    // METHODS
    ਖਾਲੀ emitVars(ਅਚਲ AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::ਸਦਿਸ਼<ਅਚਲ AstVar*> varps;
        ਲਈ (Ast_ਗੰਢ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) varps.ਧੱਕੋ_ਪਿੱਠ(varp);
        }

        ਜੇ (varps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;  // Constant pool is empty, so we are done

        stable_ਛਾਂਟੋ(varps.begin(), varps.ਸਿਰਾ(), [](ਅਚਲ AstVar* ap, ਅਚਲ AstVar* bp) {  //
            ਵਾਪਸ ap->name() < bp->name();
        });

        ਲਈ (ਅਚਲ AstVar* varp : varps) {
            ਜੇ (ਵੰਡੋ_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ਸੱਚ);
                // Close old file
                closeOutputFile();
            }

            ਜੇ (!ofp()) {
                open_New_Output_ਸਰੋਤ_File(m_unique_ਨਾਂ.get(m_file_ਅਧਾਰ_Name), ਸੱਚ, ਝੂਠ,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            ਜੇ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([ਇਹ](ਅਚਲ AstConst* constp) {
                ਜੇ (constp->ਅੰਕ().isString()) {
                    ਵੰਡੋ_ਮਾਪ_Inc(Ast_ਗੰਢ::INSTR_ਗਿਣਤੀ_STR);
                } ਨਹੀਂਤਾਂ ਜੇ (constp->isWide()) {
                    ਵੰਡੋ_ਮਾਪ_Inc(constp->ਚੌੜਾਈ_ਸ਼ਬਦ());
                } ਨਹੀਂਤਾਂ {
                    ਵੰਡੋ_ਮਾਪ_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        ਜੇ (ofp()) closeOutputFile();
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

ਜਨਤਕ:
    ਸਪਸ਼ਟ EmitCConstPool(ਅਚਲ AstPackage* poolp) {
        emitVars(poolp);
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Stat = [ਇਹ](VNType type) {
            V3Stats::ਜੋੜੋ_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        ਜੋੜੋ_Stat(VNType::Const);
        ਲਈ (ਪੂਰਨ t = VNType::ਪਹਿਲਾ_ਗੰਢ_Cons; t <= VNType::ਆਖਰੀ_ਗੰਢ_Cons; ++t) {
            ਜੋੜੋ_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

ਖਾਲੀ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
