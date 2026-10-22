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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Common.h"

#अन्तर्भाव "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_गाँठो_DType* nodep, अचर std::string& lhs) {
    std::string stmt;
    यदि (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_शब्दहरू());
        stmt += ", ";
    } अन्यथा {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    फर्काऊ stmt;
}

स्थिर शून्य makeVlToString(AstIface* nodep) {
    AstCFunc* अचर funcp
        = नूतन AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(असत्य);
    funcp->isConst(असत्य);
    funcp->isStatic(असत्य);
    funcp->protect(असत्य);
    Ast_गाँठो_अभिव्यक्ति* अचर exprp = नूतन AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(नूतन AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
स्थिर शून्य makeVlToString(Ast_गाँठो_UOr_Struct_DType* nodep) {
    Ast_गाँठो_Module* अचर modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* अचर funcp
        = नूतन AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(असत्य);
    funcp->isConst(असत्य);
    funcp->isStatic(असत्य);
    funcp->protect(असत्य);
    funcp->addStmtsp(नूतन AstCStmt{nodep->fileline(), "std::string out;"});
    लागि (अचर AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        यदि (itemp == nodep->membersp()) {
            stmt += "'{";
        } अन्यथा {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(नूतन AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(नूतन AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* अचर exprp = नूतन AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(नूतन AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
स्थिर शून्य makeToString(AstClass* nodep) {
    AstCFunc* अचर funcp = नूतन AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(सत्य);
    funcp->isStatic(असत्य);
    funcp->isVirtual(सत्य);
    funcp->protect(असत्य);
    AstCExpr* अचर exprp = नूतन AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(नूतन AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
स्थिर शून्य make_To_String_बीच(AstClass* nodep) {
    AstCFunc* अचर funcp
        = नूतन AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(सत्य);
    funcp->isStatic(असत्य);
    funcp->protect(असत्य);
    Ast_गाँठो_Stmt* stmtsp = nullptr;
    std::string comma;
    लागि (Ast_गाँठो* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        यदि (अचर स्वतः* अचर varp = VN_CAST(itemp, Var)) {
            अचर AstBasicDType* अचर basicp = varp->dtypeSkipRefp()->basicp();
            यदि (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(सत्य);  // So what we extend dumps this
                stmtsp = Ast_गाँठो::addNextNull(stmtsp, नूतन AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    यदि (nodep->extendsp()) {
        string stmt = "out += ";
        यदि (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(सत्य);  // So what we extend dumps this
        stmtsp = Ast_गाँठो::addNextNull(stmtsp, नूतन AstCStmt{nodep->fileline(), stmt});
    }

    Ast_गाँठो_अभिव्यक्ति* exprp;
    यदि (stmtsp) {
        funcp->addStmtsp(नूतन AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = नूतन AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } अन्यथा {  // Nothing to print, return ""
        exprp = नूतन AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(नूतन AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

वर्ग ToStringVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;
    माप_प्रकार m_emit_Towards_जरा
        = 0;  // class that require its ancestors to emit to_string() sets this
    माप_प्रकार m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    गणना वर्ग PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    शून्य markClassIterate(AstClass* classp) {
        यदि (classp->user1SetOnce()) फर्काऊ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        लागि (Ast_गाँठो* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                Ast_गाँठो_DType* गाँठो_Dtypep = varp->dtypep();
                यदि (AstIfaceRefDType* अचर ifaceRedDTypep = VN_CAST(गाँठो_Dtypep, IfaceRefDType)) {
                    यदि (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } अन्यथा {
                    जबसम्म (गाँठो_Dtypep && गाँठो_Dtypep->subDTypep()
                           && गाँठो_Dtypep->subDTypep()->skipRefp()) {
                        गाँठो_Dtypep = गाँठो_Dtypep->subDTypep()->skipRefp();
                        यदि (AstIfaceRefDType* अचर ifaceRedDTypep
                            = VN_CAST(गाँठो_Dtypep, IfaceRefDType)) {
                            यदि (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } अन्यथा यदि (Ast_गाँठो_UOr_Struct_DType* अचर uOrStructDTypep
                                   = VN_CAST(गाँठो_Dtypep, गाँठो_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_बीच(classp);
        ++m_statClassToString;
    }
    शून्य visit(Ast_गाँठो_UOr_Struct_DType* structp) override {
        यदि (structp->user1())
            फर्काऊ;  // if to_string() was already emitted skip the rest of the function
        यदि (structp->emitToString()) ++m_emitForMembers;
        यदि (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            लागि (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_गाँठो_DType* गाँठो_Dtypep = memberp->dtypep();
                यदि (Ast_गाँठो_UOr_Struct_DType* अचर uOrStructDTypep
                    = VN_CAST(गाँठो_Dtypep, गाँठो_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } अन्यथा {
                    जबसम्म (गाँठो_Dtypep && गाँठो_Dtypep->subDTypep()) {
                        गाँठो_Dtypep = गाँठो_Dtypep->subDTypep()->skipRefp();
                        यदि (Ast_गाँठो_UOr_Struct_DType* अचर uOrStructDTypep
                            = VN_CAST(गाँठो_Dtypep, गाँठो_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            यदि (structp->emitToString()) --m_emitForMembers;

            यदि (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(सत्य);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    शून्य visit(AstIface* ifacep) override {
        यदि (ifacep->user1())
            फर्काऊ;  // if to_string() was already emitted skip the rest of the function
        यदि (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(सत्य);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    शून्य visit(AstClass* classp) override {
        यदि (classp->user1()) फर्काऊ;
        यदि (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_जरा);
            ++m_emit_Towards_जरा;
            लागि (अचर AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                यदि (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<पूर्णाङ्क>(PrintedFromParent::PRINTED_FROM_PARENT));
        } अन्यथा {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            यदि (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_जरा == 0) {
                फर्काऊ;
            }

            यदि (m_emit_Towards_जरा > 0) markClassIterate(classp);

            तार्किक is_जरा = सत्य;
            लागि (अचर AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                यदि (extendp->classp()) {
                    यदि (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_जरा > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    यदि (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_जरा = असत्य;
                }
            }

            यदि (is_जरा) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<पूर्णाङ्क>(printed_parent));
            यदि (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_जरा);
                ++m_emit_Towards_जरा;
                लागि (अचर AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    यदि (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit ToStringVisitor(Ast_गाँठो* nodep) {
        यदि (v3Global.hasPrintedObjects()) {
            Ast_गाँठो::user1_Clear_रूख();
            Ast_गाँठो::user2_Clear_रूख();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = स्वभाव;
};

//######################################################################
// V3Common class functions

शून्य V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_रूख("common", 0, dump_रूख_Either_Level() >= 3);
}
