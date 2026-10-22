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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Common.h"

#समावेश "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_गाँठ_DType* nodep, स्थिर std::string& lhs) {
    std::string stmt;
    अगर (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_शब्दगण());
        stmt += ", ";
    } अन्यथा {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    वापस stmt;
}

अटल व्योम makeVlToString(AstIface* nodep) {
    AstCFunc* स्थिर funcp
        = नया AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(false);
    funcp->isConst(false);
    funcp->isStatic(false);
    funcp->protect(false);
    Ast_गाँठ_व्यंजक* स्थिर exprp = नया AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(नया AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
अटल व्योम makeVlToString(Ast_गाँठ_UOr_Struct_DType* nodep) {
    Ast_गाँठ_Module* स्थिर modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* स्थिर funcp
        = नया AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(false);
    funcp->isConst(false);
    funcp->isStatic(false);
    funcp->protect(false);
    funcp->addStmtsp(नया AstCStmt{nodep->fileline(), "std::string out;"});
    क्रम (स्थिर AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        अगर (itemp == nodep->membersp()) {
            stmt += "'{";
        } अन्यथा {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(नया AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(नया AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* स्थिर exprp = नया AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(नया AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
अटल व्योम makeToString(AstClass* nodep) {
    AstCFunc* स्थिर funcp = नया AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(true);
    funcp->isStatic(false);
    funcp->isVirtual(true);
    funcp->protect(false);
    AstCExpr* स्थिर exprp = नया AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(नया AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
अटल व्योम make_To_String_मध्य(AstClass* nodep) {
    AstCFunc* स्थिर funcp
        = नया AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(true);
    funcp->isStatic(false);
    funcp->protect(false);
    Ast_गाँठ_Stmt* stmtsp = nullptr;
    std::string comma;
    क्रम (Ast_गाँठ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        अगर (स्थिर स्वतः* स्थिर varp = VN_CAST(itemp, Var)) {
            स्थिर AstBasicDType* स्थिर basicp = varp->dtypeSkipRefp()->basicp();
            अगर (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(true);  // So what we extend dumps this
                stmtsp = Ast_गाँठ::add_अग्र_Null(stmtsp, नया AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    अगर (nodep->extendsp()) {
        string stmt = "out += ";
        अगर (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(true);  // So what we extend dumps this
        stmtsp = Ast_गाँठ::add_अग्र_Null(stmtsp, नया AstCStmt{nodep->fileline(), stmt});
    }

    Ast_गाँठ_व्यंजक* exprp;
    अगर (stmtsp) {
        funcp->addStmtsp(नया AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = नया AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } अन्यथा {  // Nothing to print, return ""
        exprp = नया AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(नया AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

श्रेणी ToStringVisitor final : खुला VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    माप_प्रकार m_emit_Towards_मूल
        = 0;  // class that require its ancestors to emit to_string() sets this
    माप_प्रकार m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    क्रमागत श्रेणी PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    व्योम markClassIterate(AstClass* classp) {
        अगर (classp->user1SetOnce()) वापस;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        क्रम (Ast_गाँठ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                Ast_गाँठ_DType* गाँठ_Dtypep = varp->dtypep();
                अगर (AstIfaceRefDType* स्थिर ifaceRedDTypep = VN_CAST(गाँठ_Dtypep, IfaceRefDType)) {
                    अगर (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } अन्यथा {
                    जबतक (गाँठ_Dtypep && गाँठ_Dtypep->subDTypep()
                           && गाँठ_Dtypep->subDTypep()->skipRefp()) {
                        गाँठ_Dtypep = गाँठ_Dtypep->subDTypep()->skipRefp();
                        अगर (AstIfaceRefDType* स्थिर ifaceRedDTypep
                            = VN_CAST(गाँठ_Dtypep, IfaceRefDType)) {
                            अगर (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } अन्यथा अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर uOrStructDTypep
                                   = VN_CAST(गाँठ_Dtypep, गाँठ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_मध्य(classp);
        ++m_statClassToString;
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* structp) override {
        अगर (structp->user1())
            वापस;  // if to_string() was already emitted skip the rest of the function
        अगर (structp->emitToString()) ++m_emitForMembers;
        अगर (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            क्रम (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_गाँठ_DType* गाँठ_Dtypep = memberp->dtypep();
                अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर uOrStructDTypep
                    = VN_CAST(गाँठ_Dtypep, गाँठ_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } अन्यथा {
                    जबतक (गाँठ_Dtypep && गाँठ_Dtypep->subDTypep()) {
                        गाँठ_Dtypep = गाँठ_Dtypep->subDTypep()->skipRefp();
                        अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर uOrStructDTypep
                            = VN_CAST(गाँठ_Dtypep, गाँठ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            अगर (structp->emitToString()) --m_emitForMembers;

            अगर (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(true);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    व्योम visit(AstIface* ifacep) override {
        अगर (ifacep->user1())
            वापस;  // if to_string() was already emitted skip the rest of the function
        अगर (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(true);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    व्योम visit(AstClass* classp) override {
        अगर (classp->user1()) वापस;
        अगर (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_मूल);
            ++m_emit_Towards_मूल;
            क्रम (स्थिर AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                अगर (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<पूर्णांक>(PrintedFromParent::PRINTED_FROM_PARENT));
        } अन्यथा {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            अगर (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_मूल == 0) {
                वापस;
            }

            अगर (m_emit_Towards_मूल > 0) markClassIterate(classp);

            bool is_मूल = true;
            क्रम (स्थिर AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                अगर (extendp->classp()) {
                    अगर (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_मूल > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    अगर (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_मूल = false;
                }
            }

            अगर (is_मूल) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<पूर्णांक>(printed_parent));
            अगर (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_मूल);
                ++m_emit_Towards_मूल;
                क्रम (स्थिर AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    अगर (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    explicit ToStringVisitor(Ast_गाँठ* nodep) {
        अगर (v3Global.hasPrintedObjects()) {
            Ast_गाँठ::user1_Clear_वृक्ष();
            Ast_गाँठ::user2_Clear_वृक्ष();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = शेष;
};

//######################################################################
// V3Common class functions

व्योम V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_वृक्ष("common", 0, dump_वृक्ष_Either_Level() >= 3);
}
