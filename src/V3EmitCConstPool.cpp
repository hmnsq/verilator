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
    std::array<VDouble0, VNType::अङ्क_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    अचर std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "__ConstPool";

    // METHODS
    शून्य emitVars(अचर AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<अचर AstVar*> varps;
        लागि (Ast_गाँठो* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) varps.थप_back(varp);
        }

        यदि (varps.empty()) फर्काऊ;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](अचर AstVar* ap, अचर AstVar* bp) {  //
            फर्काऊ ap->name() < bp->name();
        });

        लागि (अचर AstVar* varp : varps) {
            यदि (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                // Close old file
                closeOutputFile();
            }

            यदि (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), सत्य, असत्य,
                                        "Constant pool");
                माला_दो("\n");
                माला_दो("#include \"verilated.h\"\n");
            }

            माला_दो("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            यदि (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([एतत्](अचर AstConst* constp) {
                यदि (constp->अङ्क().isString()) {
                    splitSizeInc(Ast_गाँठो::INSTR_गन्ती_STR);
                } अन्यथा यदि (constp->isWide()) {
                    splitSizeInc(constp->width_शब्दहरू());
                } अन्यथा {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        यदि (ofp()) closeOutputFile();
    }

    // VISITORS
    शून्य visit(Ast_गाँठो* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    explicit EmitCConstPool(अचर AstPackage* poolp) {
        emitVars(poolp);
        अचर स्वतः addStat = [एतत्](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        लागि (पूर्णाङ्क t = VNType::first_गाँठो_Cons; t <= VNType::last_गाँठो_Cons; ++t) {
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
