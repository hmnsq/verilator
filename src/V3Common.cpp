// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add common contents to modules
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
// V3Common's Transformations:
//
//  Each class:
//      Emit to_string() if given class is printed from (used as format arg), or any of its
//      parents/children is printed from, or if any of its children has another parent that is
//      printed from. If class emits to_string(), emit to_string() for its struct/union/interface
//      fields.
//  Each struct/union:
//      Emit to_string() if given struct/union is used as format arg or is a field of a
//      class/struct/union that emits to_string(). If struct/union emits to_string(), emit
//      to_string() for its struct/union fields.
//  Each interface:
//      Emit to_string() if given interface is a field of a class that emits to_string()
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Common.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_ಗಂಟು_DType* nodep, ಧ್ರುವ std::string& lhs) {
    std::string stmt;
    ಯದಿ (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_ಪದಗಳು());
        stmt += ", ";
    } ಅನ್ಯಥಾ {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    ಪ್ರತ್ಯಾಗಮ stmt;
}

ಸ್ಥಿರ ಶೂನ್ಯ makeVlToString(AstIface* nodep) {
    AstCFunc* ಧ್ರುವ funcp
        = ನೂತನ AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(ಅಸತ್ಯ);
    funcp->isConst(ಅಸತ್ಯ);
    funcp->isStatic(ಅಸತ್ಯ);
    funcp->protect(ಅಸತ್ಯ);
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = ನೂತನ AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(ನೂತನ AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ಸ್ಥಿರ ಶೂನ್ಯ makeVlToString(Ast_ಗಂಟು_UOr_Struct_DType* nodep) {
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ಧ್ರುವ funcp
        = ನೂತನ AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(ಅಸತ್ಯ);
    funcp->isConst(ಅಸತ್ಯ);
    funcp->isStatic(ಅಸತ್ಯ);
    funcp->protect(ಅಸತ್ಯ);
    funcp->addStmtsp(ನೂತನ AstCStmt{nodep->fileline(), "std::string out;"});
    ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        ಯದಿ (itemp == nodep->membersp()) {
            stmt += "'{";
        } ಅನ್ಯಥಾ {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(ನೂತನ AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(ನೂತನ AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ಧ್ರುವ exprp = ನೂತನ AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(ನೂತನ AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
ಸ್ಥಿರ ಶೂನ್ಯ makeToString(AstClass* nodep) {
    AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(ಸತ್ಯ);
    funcp->isStatic(ಅಸತ್ಯ);
    funcp->isVirtual(ಸತ್ಯ);
    funcp->protect(ಅಸತ್ಯ);
    AstCExpr* ಧ್ರುವ exprp = ನೂತನ AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(ನೂತನ AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ಸ್ಥಿರ ಶೂನ್ಯ make_To_String_ಮಧ್ಯ(AstClass* nodep) {
    AstCFunc* ಧ್ರುವ funcp
        = ನೂತನ AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(ಸತ್ಯ);
    funcp->isStatic(ಅಸತ್ಯ);
    funcp->protect(ಅಸತ್ಯ);
    Ast_ಗಂಟು_Stmt* stmtsp = nullptr;
    std::string comma;
    ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ varp = VN_CAST(itemp, Var)) {
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = varp->dtypeSkipRefp()->basicp();
            ಯದಿ (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(ಸತ್ಯ);  // So what we extend dumps this
                stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, ನೂತನ AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    ಯದಿ (nodep->extendsp()) {
        string stmt = "out += ";
        ಯದಿ (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(ಸತ್ಯ);  // So what we extend dumps this
        stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, ನೂತನ AstCStmt{nodep->fileline(), stmt});
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;
    ಯದಿ (stmtsp) {
        funcp->addStmtsp(ನೂತನ AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = ನೂತನ AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } ಅನ್ಯಥಾ {  // Nothing to print, return ""
        exprp = ನೂತನ AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(ನೂತನ AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

ವರ್ಗ ToStringVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    size_t m_emit_Towards_ಬೇರು
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    ಗಣನಾ ವರ್ಗ PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    ಶೂನ್ಯ markClassIterate(AstClass* classp) {
        ಯದಿ (classp->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                Ast_ಗಂಟು_DType* ಗಂಟು_Dtypep = varp->dtypep();
                ಯದಿ (AstIfaceRefDType* ಧ್ರುವ ifaceRedDTypep = VN_CAST(ಗಂಟು_Dtypep, IfaceRefDType)) {
                    ಯದಿ (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } ಅನ್ಯಥಾ {
                    ಯಾವತ್ (ಗಂಟು_Dtypep && ಗಂಟು_Dtypep->subDTypep()
                           && ಗಂಟು_Dtypep->subDTypep()->skipRefp()) {
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->subDTypep()->skipRefp();
                        ಯದಿ (AstIfaceRefDType* ಧ್ರುವ ifaceRedDTypep
                            = VN_CAST(ಗಂಟು_Dtypep, IfaceRefDType)) {
                            ಯದಿ (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ uOrStructDTypep
                                   = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_ಮಧ್ಯ(classp);
        ++m_statClassToString;
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* structp) override {
        ಯದಿ (structp->user1())
            ಪ್ರತ್ಯಾಗಮ;  // if to_string() was already emitted skip the rest of the function
        ಯದಿ (structp->emitToString()) ++m_emitForMembers;
        ಯದಿ (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            ಪ್ರತಿ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_DType* ಗಂಟು_Dtypep = memberp->dtypep();
                ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ uOrStructDTypep
                    = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } ಅನ್ಯಥಾ {
                    ಯಾವತ್ (ಗಂಟು_Dtypep && ಗಂಟು_Dtypep->subDTypep()) {
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->subDTypep()->skipRefp();
                        ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ uOrStructDTypep
                            = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            ಯದಿ (structp->emitToString()) --m_emitForMembers;

            ಯದಿ (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(ಸತ್ಯ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    ಶೂನ್ಯ visit(AstIface* ifacep) override {
        ಯದಿ (ifacep->user1())
            ಪ್ರತ್ಯಾಗಮ;  // if to_string() was already emitted skip the rest of the function
        ಯದಿ (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(ಸತ್ಯ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    ಶೂನ್ಯ visit(AstClass* classp) override {
        ಯದಿ (classp->user1()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_ಬೇರು);
            ++m_emit_Towards_ಬೇರು;
            ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ಯದಿ (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<ಪೂರ್ಣಾಙ್ಕ>(PrintedFromParent::PRINTED_FROM_PARENT));
        } ಅನ್ಯಥಾ {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            ಯದಿ (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_ಬೇರು == 0) {
                ಪ್ರತ್ಯಾಗಮ;
            }

            ಯದಿ (m_emit_Towards_ಬೇರು > 0) markClassIterate(classp);

            ತಾರ್ಕಿಕ is_ಬೇರು = ಸತ್ಯ;
            ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ಯದಿ (extendp->classp()) {
                    ಯದಿ (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_ಬೇರು > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    ಯದಿ (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_ಬೇರು = ಅಸತ್ಯ;
                }
            }

            ಯದಿ (is_ಬೇರು) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<ಪೂರ್ಣಾಙ್ಕ>(printed_parent));
            ಯದಿ (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_ಬೇರು);
                ++m_emit_Towards_ಬೇರು;
                ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    ಯದಿ (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit ToStringVisitor(Ast_ಗಂಟು* nodep) {
        ಯದಿ (v3Global.hasPrintedObjects()) {
            Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
            Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// V3Common class functions

ಶೂನ್ಯ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_ವೃಕ್ಷ("common", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
