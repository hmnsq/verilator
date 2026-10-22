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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Common.h"

#અન્તર્ભાવ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_ગાંઠ_DType* nodep, અચલ std::string& lhs) {
    std::string stmt;
    જો (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_શબ્દો());
        stmt += ", ";
    } નહીંતર {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    પાછા stmt;
}

સ્થિર શૂન્ય makeVlToString(AstIface* nodep) {
    AstCFunc* અચલ funcp
        = નવું AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(ખોટું);
    funcp->isConst(ખોટું);
    funcp->isStatic(ખોટું);
    funcp->protect(ખોટું);
    Ast_ગાંઠ_પદાવલિ* અચલ exprp = નવું AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(નવું AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
સ્થિર શૂન્ય makeVlToString(Ast_ગાંઠ_UOr_Struct_DType* nodep) {
    Ast_ગાંઠ_Module* અચલ modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* અચલ funcp
        = નવું AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(ખોટું);
    funcp->isConst(ખોટું);
    funcp->isStatic(ખોટું);
    funcp->protect(ખોટું);
    funcp->addStmtsp(નવું AstCStmt{nodep->fileline(), "std::string out;"});
    માટે (અચલ AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        જો (itemp == nodep->membersp()) {
            stmt += "'{";
        } નહીંતર {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(નવું AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(નવું AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* અચલ exprp = નવું AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(નવું AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
સ્થિર શૂન્ય makeToString(AstClass* nodep) {
    AstCFunc* અચલ funcp = નવું AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(સાચું);
    funcp->isStatic(ખોટું);
    funcp->isVirtual(સાચું);
    funcp->protect(ખોટું);
    AstCExpr* અચલ exprp = નવું AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(નવું AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
સ્થિર શૂન્ય make_To_String_મધ્ય(AstClass* nodep) {
    AstCFunc* અચલ funcp
        = નવું AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(સાચું);
    funcp->isStatic(ખોટું);
    funcp->protect(ખોટું);
    Ast_ગાંઠ_Stmt* stmtsp = nullptr;
    std::string comma;
    માટે (Ast_ગાંઠ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        જો (અચલ auto* અચલ varp = VN_CAST(itemp, Var)) {
            અચલ AstBasicDType* અચલ basicp = varp->dtypeSkipRefp()->basicp();
            જો (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(સાચું);  // So what we extend dumps this
                stmtsp = Ast_ગાંઠ::addNextNull(stmtsp, નવું AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    જો (nodep->extendsp()) {
        string stmt = "out += ";
        જો (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(સાચું);  // So what we extend dumps this
        stmtsp = Ast_ગાંઠ::addNextNull(stmtsp, નવું AstCStmt{nodep->fileline(), stmt});
    }

    Ast_ગાંઠ_પદાવલિ* exprp;
    જો (stmtsp) {
        funcp->addStmtsp(નવું AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = નવું AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } નહીંતર {  // Nothing to print, return ""
        exprp = નવું AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(નવું AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

વર્ગ ToStringVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    size_t m_emit_Towards_મૂળ
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    ગણના વર્ગ PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    શૂન્ય markClassIterate(AstClass* classp) {
        જો (classp->user1SetOnce()) પાછા;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        માટે (Ast_ગાંઠ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                Ast_ગાંઠ_DType* ગાંઠ_Dtypep = varp->dtypep();
                જો (AstIfaceRefDType* અચલ ifaceRedDTypep = VN_CAST(ગાંઠ_Dtypep, IfaceRefDType)) {
                    જો (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } નહીંતર {
                    જ્યારે (ગાંઠ_Dtypep && ગાંઠ_Dtypep->subDTypep()
                           && ગાંઠ_Dtypep->subDTypep()->skipRefp()) {
                        ગાંઠ_Dtypep = ગાંઠ_Dtypep->subDTypep()->skipRefp();
                        જો (AstIfaceRefDType* અચલ ifaceRedDTypep
                            = VN_CAST(ગાંઠ_Dtypep, IfaceRefDType)) {
                            જો (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } નહીંતર જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ uOrStructDTypep
                                   = VN_CAST(ગાંઠ_Dtypep, ગાંઠ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_મધ્ય(classp);
        ++m_statClassToString;
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* structp) override {
        જો (structp->user1())
            પાછા;  // if to_string() was already emitted skip the rest of the function
        જો (structp->emitToString()) ++m_emitForMembers;
        જો (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            માટે (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ગાંઠ_DType* ગાંઠ_Dtypep = memberp->dtypep();
                જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ uOrStructDTypep
                    = VN_CAST(ગાંઠ_Dtypep, ગાંઠ_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } નહીંતર {
                    જ્યારે (ગાંઠ_Dtypep && ગાંઠ_Dtypep->subDTypep()) {
                        ગાંઠ_Dtypep = ગાંઠ_Dtypep->subDTypep()->skipRefp();
                        જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ uOrStructDTypep
                            = VN_CAST(ગાંઠ_Dtypep, ગાંઠ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            જો (structp->emitToString()) --m_emitForMembers;

            જો (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(સાચું);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    શૂન્ય visit(AstIface* ifacep) override {
        જો (ifacep->user1())
            પાછા;  // if to_string() was already emitted skip the rest of the function
        જો (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(સાચું);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    શૂન્ય visit(AstClass* classp) override {
        જો (classp->user1()) પાછા;
        જો (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_મૂળ);
            ++m_emit_Towards_મૂળ;
            માટે (અચલ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                જો (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<પૂર્ણાંક>(PrintedFromParent::PRINTED_FROM_PARENT));
        } નહીંતર {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            જો (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_મૂળ == 0) {
                પાછા;
            }

            જો (m_emit_Towards_મૂળ > 0) markClassIterate(classp);

            તાર્કિક is_મૂળ = સાચું;
            માટે (અચલ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                જો (extendp->classp()) {
                    જો (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_મૂળ > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    જો (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_મૂળ = ખોટું;
                }
            }

            જો (is_મૂળ) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<પૂર્ણાંક>(printed_parent));
            જો (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_મૂળ);
                ++m_emit_Towards_મૂળ;
                માટે (અચલ AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    જો (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit ToStringVisitor(Ast_ગાંઠ* nodep) {
        જો (v3Global.hasPrintedObjects()) {
            Ast_ગાંઠ::user1_Clear_વૃક્ષ();
            Ast_ગાંઠ::user2_Clear_વૃક્ષ();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = મૂળભૂત;
};

//######################################################################
// V3Common class functions

શૂન્ય V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_વૃક્ષ("common", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
