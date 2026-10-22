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
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::अङ्कः_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::शिखरम्_Class_Name() + "__ConstPool";

    // METHODS
    शून्य emitVars(ध्रुव AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ध्रुव AstVar*> varps;
        प्रति (Ast_ग्रन्थिः* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            जइ (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) varps.प्रक्षिप_back(varp);
        }

        जइ (varps.empty()) प्रत्यागम;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ध्रुव AstVar* ap, ध्रुव AstVar* bp) {  //
            प्रत्यागम ap->name() < bp->name();
        });

        प्रति (ध्रुव AstVar* varp : varps) {
            जइ (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सच्च);
                // Close old file
                closeOutputFile();
            }

            जइ (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), सच्च, मिच्छ,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            जइ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([एतत्](ध्रुव AstConst* constp) {
                जइ (constp->अङ्कः().isString()) {
                    splitSizeInc(Ast_ग्रन्थिः::INSTR_गणनम्_STR);
                } अण्णहा जइ (constp->isWide()) {
                    splitSizeInc(constp->width_शब्दाः());
                } अण्णहा {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        जइ (ofp()) closeOutputFile();
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    explicit EmitCConstPool(ध्रुव AstPackage* poolp) {
        emitVars(poolp);
        ध्रुव auto addStat = [एतत्](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        प्रति (पूर्णाङ्क t = VNType::first_ग्रन्थिः_Cons; t <= VNType::last_ग्रन्थिः_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

शून्य V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
