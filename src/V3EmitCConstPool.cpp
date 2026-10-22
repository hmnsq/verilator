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

#समावेश "V3PchAstMT.h"

#समावेश "V3EmitC.h"
#समावेश "V3EmitCConstInit.h"
#समावेश "V3File.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

#समावेश <algorithm>
#समावेश <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

श्रेणी EmitCConstPool final : खुला EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::अंक_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    स्थिर std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "__ConstPool";

    // METHODS
    व्योम emitVars(स्थिर AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<स्थिर AstVar*> varps;
        क्रम (Ast_गाँठ* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) varps.धकेलो_back(varp);
        }

        अगर (varps.empty()) वापस;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](स्थिर AstVar* ap, स्थिर AstVar* bp) {  //
            वापस ap->name() < bp->name();
        });

        क्रम (स्थिर AstVar* varp : varps) {
            अगर (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(true);
                // Close old file
                closeOutputFile();
            }

            अगर (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.लो(m_fileBaseName), true, false,
                                        "Constant pool");
                माला_दो("\n");
                माला_दो("#include \"verilated.h\"\n");
            }

            माला_दो("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            अगर (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([यह](स्थिर AstConst* constp) {
                अगर (constp->अंक().isString()) {
                    split_आकार_Inc(Ast_गाँठ::INSTR_गिनती_STR);
                } अन्यथा अगर (constp->isWide()) {
                    split_आकार_Inc(constp->width_शब्दगण());
                } अन्यथा {
                    split_आकार_Inc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        अगर (ofp()) closeOutputFile();
    }

    // VISITORS
    व्योम visit(Ast_गाँठ* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

खुला:
    explicit EmitCConstPool(स्थिर AstPackage* poolp) {
        emitVars(poolp);
        स्थिर स्वतः addStat = [यह](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        क्रम (पूर्णांक t = VNType::first_गाँठ_Cons; t <= VNType::last_गाँठ_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

व्योम V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
