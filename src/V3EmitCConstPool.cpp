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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCConstInit.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

తరగతి EmitCConstPool final : బహిరంగ EmitCConstInit {
    // TYPES
    ఉపయోగించి బహిర్గతం_CFile_Pair = std::pair<V3_బహిర్గతం_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::అంకె_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_పేర్లు m_unique_పేర్లు;  // Generates unique file names
    స్థిరరాశి std::string m_file_పునాది_Name = EmitCUtil::శిఖరం_Class_Name() + "__ConstPool";

    // METHODS
    శూన్య emitVars(స్థిరరాశి AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::సదిశ<స్థిరరాశి AstVar*> varps;
        కొరకు (Ast_కణుపు* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) varps.నెట్టు_వెనుక(varp);
        }

        ఒకవేళ (varps.ఖాళీలు()) తిరిగి;  // Constant pool is empty, so we are done

        stable_క్రమీకరించు(varps.begin(), varps.చివర(), [](స్థిరరాశి AstVar* ap, స్థిరరాశి AstVar* bp) {  //
            తిరిగి ap->name() < bp->name();
        });

        కొరకు (స్థిరరాశి AstVar* varp : varps) {
            ఒకవేళ (విభజించు_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(నిజం);
                // Close old file
                closeOutputFile();
            }

            ఒకవేళ (!ofp()) {
                open_New_Output_ఆధారం_File(m_unique_పేర్లు.get(m_file_పునాది_Name), నిజం, అబద్ధం,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            ఒకవేళ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([ఇది](స్థిరరాశి AstConst* constp) {
                ఒకవేళ (constp->అంకె().isString()) {
                    విభజించు_కొలత_Inc(Ast_కణుపు::INSTR_లెక్క_STR);
                } లేకపోతే ఒకవేళ (constp->isWide()) {
                    విభజించు_కొలత_Inc(constp->వెడల్పు_పదాలు());
                } లేకపోతే {
                    విభజించు_కొలత_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        ఒకవేళ (ofp()) closeOutputFile();
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

బహిరంగ:
    స్పష్టమైన EmitCConstPool(స్థిరరాశి AstPackage* poolp) {
        emitVars(poolp);
        స్థిరరాశి స్వయంచాలక కూడు_Stat = [ఇది](VNType type) {
            V3Stats::కూడు_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        కూడు_Stat(VNType::Const);
        కొరకు (పూర్ణాంకం t = VNType::మొదటి_కణుపు_Cons; t <= VNType::చివరి_కణుపు_Cons; ++t) {
            కూడు_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

శూన్య V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
