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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3EmitC.h"
#അന്തര്ഭാവ "V3EmitCConstInit.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cinttypes>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Const pool emitter

വർഗ്ഗം EmitCConstPool final : പൊതു EmitCConstInit {
    // TYPES
    using OutCFilePair = std::pair<V3OutCFile*, AstCFile*>;

    // MEMBERS
    std::array<VDouble0, VNType::അക്കം_TYPES()> m_emitted;  // Entries emitted, by value type
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    സ്ഥിരം std::string m_fileBaseName = EmitCUtil::മുകൾ_Class_Name() + "__ConstPool";

    // METHODS
    ശൂന്യം emitVars(സ്ഥിരം AstPackage* poolp) {
        UASSERT(!ofp(), "Output file should not be open");

        std::vector<സ്ഥിരം AstVar*> varps;
        വേണ്ടി (Ast_കണ്ണി* nodep = poolp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) varps.തള്ളുക_back(varp);
        }

        എങ്കിൽ (varps.empty()) തിരികെ;  // Constant pool is empty, so we are done

        stable_sort(varps.begin(), varps.end(), [](സ്ഥിരം AstVar* ap, സ്ഥിരം AstVar* bp) {  //
            തിരികെ ap->name() < bp->name();
        });

        വേണ്ടി (സ്ഥിരം AstVar* varp : varps) {
            എങ്കിൽ (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(സത്യം);
                // Close old file
                closeOutputFile();
            }

            എങ്കിൽ (!ofp()) {
                openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), സത്യം, അസത്യം,
                                        "Constant pool");
                puts("\n");
                puts("#include \"verilated.h\"\n");
            }

            puts("\n");
            putns(varp, "extern ");
            // Literal types should be constinit (no code generation needed)
            എങ്കിൽ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
            putns(varp, "const ");
            emitVarDefn(varp);
            // Account for file splitting
            varp->valuep()->foreach([ഇത്](സ്ഥിരം AstConst* constp) {
                എങ്കിൽ (constp->അക്കം().isString()) {
                    splitSizeInc(Ast_കണ്ണി::INSTR_എണ്ണം_STR);
                } അല്ലെങ്കിൽ എങ്കിൽ (constp->isWide()) {
                    splitSizeInc(constp->width_വാക്കുകൾ());
                } അല്ലെങ്കിൽ {
                    splitSizeInc(1);
                }
            });
            putns(varp, ";\n");
            // Keep track of stats
            ++m_emitted[varp->valuep()->type()];
        }

        എങ്കിൽ (ofp()) closeOutputFile();
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("Unknown node type reached EmitCConstPool: " << nodep->prettyTypeName());
    }  // LCOV_EXCL_STOP

പൊതു:
    explicit EmitCConstPool(സ്ഥിരം AstPackage* poolp) {
        emitVars(poolp);
        സ്ഥിരം auto addStat = [ഇത്](VNType type) {
            V3Stats::addStat("ConstPool, "s + type.ascii() + " emitted", m_emitted[type]);
        };
        addStat(VNType::Const);
        വേണ്ടി (പൂർണ്ണസംഖ്യ t = VNType::first_കണ്ണി_Cons; t <= VNType::last_കണ്ണി_Cons; ++t) {
            addStat(static_cast<VNType::en>(t));
        }
    }
};

//######################################################################
// EmitC static functions

ശൂന്യം V3EmitC::emitcConstPool() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCConstPool(v3Global.rootp()->constPoolPkgp());
}
