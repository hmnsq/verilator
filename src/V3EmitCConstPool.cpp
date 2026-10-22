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
    std::array<VDouble0, VNType::अंक_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    स्थिरांक std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "__ConstPool";

    // METHODS
    शून्य emitVars(स्थिरांक AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<स्थिरांक AstVar*> varps;
        साठी (Ast_गाठ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) varps.ढकला_back(varp);
        }

        जर (varps.empty()) परत;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](स्थिरांक AstVar* ap, स्थिरांक AstVar* bp) {  //
            परत ap->name() < bp->name();
        });

        साठी (स्थिरांक AstVar* varp : varps) {
            जर (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                // Close old file
                closeOutputFile();
            }

            जर (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), सत्य, असत्य,
                                        "Constant pool");
                माला_दो("\n");
                माला_दो("#include \"verilated.h\"\n");
            }

            माला_दो("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            जर (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([हे](स्थिरांक AstConst* constp) {
                जर (constp->अंक().isString()) {
                    splitSizeInc(Ast_गाठ::INSTR_मोजणी_STR);
                } अन्यथा जर (constp->isWide()) {
                    splitSizeInc(constp->width_शब्द());
                } अन्यथा {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        जर (ofp()) closeOutputFile();
    }

    // VISITORS
    शून्य visit(Ast_गाठ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    explicit EmitCConstPool(स्थिरांक AstPackage* poolp) {
        emitVars(poolp);
        स्थिरांक स्वतः addStat = [हे](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        साठी (पूर्णांक t = VNType::first_गाठ_Cons; t <= VNType::last_गाठ_Cons; ++t) {
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
