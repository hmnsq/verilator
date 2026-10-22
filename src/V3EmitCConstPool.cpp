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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCConstInit.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

வகுப்பு EmitCConstPool final : பொது EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::இலக்கம்_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    மாறிலி std::string m_fileBaseName = EmitCUtil::உச்சி_Class_Name() + "__ConstPool";

    // METHODS
    வெறுமை emitVars(மாறிலி AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<மாறிலி AstVar*> varps;
        க்கு (Ast_முனை* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) varps.தள்ளு_back(varp);
        }

        எனில் (varps.empty()) திரும்பு;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](மாறிலி AstVar* ap, மாறிலி AstVar* bp) {  //
            திரும்பு ap->name() < bp->name();
        });

        க்கு (மாறிலி AstVar* varp : varps) {
            எனில் (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(உண்மை);
                // Close old file
                closeOutputFile();
            }

            எனில் (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), உண்மை, பொய்,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            எனில் (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([இது](மாறிலி AstConst* constp) {
                எனில் (constp->இலக்கம்().isString()) {
                    splitSizeInc(Ast_முனை::INSTR_மொத்தம்_STR);
                } இல்லையெனில் எனில் (constp->isWide()) {
                    splitSizeInc(constp->width_சொற்கள்());
                } இல்லையெனில் {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        எனில் (ofp()) closeOutputFile();
    }

    // VISITORS
    வெறுமை visit(Ast_முனை* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

பொது:
    explicit EmitCConstPool(மாறிலி AstPackage* poolp) {
        emitVars(poolp);
        மாறிலி auto addStat = [இது](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        க்கு (முழு t = VNType::first_முனை_Cons; t <= VNType::last_முனை_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

வெறுமை V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
