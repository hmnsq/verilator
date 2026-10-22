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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCConstInit.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

ଶ୍ରେଣୀ EmitCConstPool final : ସର୍ବସାଧାରଣ EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::ଅଙ୍କ_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ଅଚଳ std::string m_fileBaseName = EmitCUtil::ଶୀର୍ଷ_Class_Name() + "__ConstPool";

    // METHODS
    ଶୂନ୍ୟ emitVars(ଅଚଳ AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<ଅଚଳ AstVar*> varps;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) varps.ଠେଲ_back(varp);
        }

        ଯଦି (varps.empty()) ଫେରନ୍ତୁ;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](ଅଚଳ AstVar* ap, ଅଚଳ AstVar* bp) {  //
            ଫେରନ୍ତୁ ap->name() < bp->name();
        });

        ପାଇଁ (ଅଚଳ AstVar* varp : varps) {
            ଯଦି (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ସତ୍ୟ);
                // Close old file
                closeOutputFile();
            }

            ଯଦି (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), ସତ୍ୟ, ମିଥ୍ୟା,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            ଯଦି (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([ଏହି](ଅଚଳ AstConst* constp) {
                ଯଦି (constp->ଅଙ୍କ().isString()) {
                    splitSizeInc(Ast_ଗଣ୍ଠି::INSTR_ଗଣନା_STR);
                } ଅନ୍ୟଥା ଯଦି (constp->isWide()) {
                    splitSizeInc(constp->width_ଶବ୍ଦଗୁଡ଼ିକ());
                } ଅନ୍ୟଥା {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        ଯଦି (ofp()) closeOutputFile();
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

ସର୍ବସାଧାରଣ:
    explicit EmitCConstPool(ଅଚଳ AstPackage* poolp) {
        emitVars(poolp);
        ଅଚଳ auto addStat = [ଏହି](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା t = VNType::first_ଗଣ୍ଠି_Cons; t <= VNType::last_ଗଣ୍ଠି_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

ଶୂନ୍ୟ V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
