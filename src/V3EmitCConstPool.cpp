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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCConstInit.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

वर्ग EmitCConstPool final : सार्वजनिक EmitCConstInit {
    // TYPES
    उपयुज्य बहिर्गमनम्_CFile_Pair = std::pair<V3_बहिर्गमनम्_CFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::अङ्कः_TYPES()> m_emitted;  // Entries emitted, by value type
    V3_Unique_नामानि m_unique_नामानि;  // Generates unique file names
    ध्रुव std::string m_file_आधारः_Name = EmitCUtil::शिखरम्_Class_Name() + "__ConstPool";

    // METHODS
    शून्य emitVars(ध्रुव AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::सदिशम्<ध्रुव AstVar*> varps;
        प्रति (Ast_ग्रन्थिः* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) varps.प्रक्षिप_पृष्ठभागः(varp);
        }

        यदि (varps.रिक्तानि()) प्रत्यागम;  // Constant pool is empty, so we are done

        stable_क्रमय(varps.begin(), varps.अन्तः_2(), [](ध्रुव AstVar* ap, ध्रुव AstVar* bp) {  //
            प्रत्यागम ap->name() < bp->name();
        });

        प्रति (ध्रुव AstVar* varp : varps) {
            यदि (विभज_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                // Close old file
                closeOutputFile();
            }

            यदि (!ofp()) {
                openNewOutputSourceFile(m_unique_नामानि.get(m_file_आधारः_Name), सत्य, असत्य,
                                        "Constant pool");
                पङ्क्तिं_लिख("\n");
                पङ्क्तिं_लिख("#include \"verilated.h\"\n");
            }

            पङ्क्तिं_लिख("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            यदि (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([एतत्](ध्रुव AstConst* constp) {
                यदि (constp->अङ्कः().isString()) {
                    विभज_आकार_Inc(Ast_ग्रन्थिः::INSTR_गणनम्_STR);
                } अन्यथा यदि (constp->isWide()) {
                    विभज_आकार_Inc(constp->विस्तारः_शब्दाः());
                } अन्यथा {
                    विभज_आकार_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        यदि (ofp()) closeOutputFile();
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    स्पष्ट EmitCConstPool(ध्रुव AstPackage* poolp) {
        emitVars(poolp);
        ध्रुव स्वचालित सङ्कलय_Stat = [एतत्](VNType type) {
            V3Stats::सङ्कलय_Stat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        सङ्कलय_Stat(VNType::Const);
        प्रति (पूर्णाङ्क t = VNType::first_ग्रन्थिः_Cons; t <= VNType::चरमः_ग्रन्थिः_Cons; ++t) {
            सङ्कलय_Stat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

शून्य V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
