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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Common.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_ଗଣ୍ଠି_DType* nodep, ଅଚଳ std::string& lhs) {
    std::string stmt;
    ଯଦି (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_ଶବ୍ଦଗୁଡ଼ିକ());
        stmt += ", ";
    } ଅନ୍ୟଥା {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    ଫେରନ୍ତୁ stmt;
}

ସ୍ଥିର ଶୂନ୍ୟ makeVlToString(AstIface* nodep) {
    AstCFunc* ଅଚଳ funcp
        = ନୂତନ AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(ମିଥ୍ୟା);
    funcp->isConst(ମିଥ୍ୟା);
    funcp->isStatic(ମିଥ୍ୟା);
    funcp->protect(ମିଥ୍ୟା);
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = ନୂତନ AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(ନୂତନ AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ସ୍ଥିର ଶୂନ୍ୟ makeVlToString(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ଅଚଳ funcp
        = ନୂତନ AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(ମିଥ୍ୟା);
    funcp->isConst(ମିଥ୍ୟା);
    funcp->isStatic(ମିଥ୍ୟା);
    funcp->protect(ମିଥ୍ୟା);
    funcp->addStmtsp(ନୂତନ AstCStmt{nodep->fileline(), "std::string out;"});
    ପାଇଁ (ଅଚଳ AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        ଯଦି (itemp == nodep->membersp()) {
            stmt += "'{";
        } ଅନ୍ୟଥା {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(ନୂତନ AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(ନୂତନ AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ଅଚଳ exprp = ନୂତନ AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(ନୂତନ AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
ସ୍ଥିର ଶୂନ୍ୟ makeToString(AstClass* nodep) {
    AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(ସତ୍ୟ);
    funcp->isStatic(ମିଥ୍ୟା);
    funcp->isVirtual(ସତ୍ୟ);
    funcp->protect(ମିଥ୍ୟା);
    AstCExpr* ଅଚଳ exprp = ନୂତନ AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(ନୂତନ AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ସ୍ଥିର ଶୂନ୍ୟ make_To_String_ମଧ୍ୟ(AstClass* nodep) {
    AstCFunc* ଅଚଳ funcp
        = ନୂତନ AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(ସତ୍ୟ);
    funcp->isStatic(ମିଥ୍ୟା);
    funcp->protect(ମିଥ୍ୟା);
    Ast_ଗଣ୍ଠି_Stmt* stmtsp = nullptr;
    std::string comma;
    ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        ଯଦି (ଅଚଳ auto* ଅଚଳ varp = VN_CAST(itemp, Var)) {
            ଅଚଳ AstBasicDType* ଅଚଳ basicp = varp->dtypeSkipRefp()->basicp();
            ଯଦି (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(ସତ୍ୟ);  // So what we extend dumps this
                stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, ନୂତନ AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    ଯଦି (nodep->extendsp()) {
        string stmt = "out += ";
        ଯଦି (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(ସତ୍ୟ);  // So what we extend dumps this
        stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, ନୂତନ AstCStmt{nodep->fileline(), stmt});
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp;
    ଯଦି (stmtsp) {
        funcp->addStmtsp(ନୂତନ AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = ନୂତନ AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } ଅନ୍ୟଥା {  // Nothing to print, return ""
        exprp = ନୂତନ AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(ନୂତନ AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

ଶ୍ରେଣୀ ToStringVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    size_t m_emit_Towards_ମୂଳ
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    ଗଣନା ଶ୍ରେଣୀ PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    ଶୂନ୍ୟ markClassIterate(AstClass* classp) {
        ଯଦି (classp->user1SetOnce()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                Ast_ଗଣ୍ଠି_DType* ଗଣ୍ଠି_Dtypep = varp->dtypep();
                ଯଦି (AstIfaceRefDType* ଅଚଳ ifaceRedDTypep = VN_CAST(ଗଣ୍ଠି_Dtypep, IfaceRefDType)) {
                    ଯଦି (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } ଅନ୍ୟଥା {
                    ଯେତେବେଳେ (ଗଣ୍ଠି_Dtypep && ଗଣ୍ଠି_Dtypep->subDTypep()
                           && ଗଣ୍ଠି_Dtypep->subDTypep()->skipRefp()) {
                        ଗଣ୍ଠି_Dtypep = ଗଣ୍ଠି_Dtypep->subDTypep()->skipRefp();
                        ଯଦି (AstIfaceRefDType* ଅଚଳ ifaceRedDTypep
                            = VN_CAST(ଗଣ୍ଠି_Dtypep, IfaceRefDType)) {
                            ଯଦି (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ uOrStructDTypep
                                   = VN_CAST(ଗଣ୍ଠି_Dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_ମଧ୍ୟ(classp);
        ++m_statClassToString;
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* structp) override {
        ଯଦି (structp->user1())
            ଫେରନ୍ତୁ;  // if to_string() was already emitted skip the rest of the function
        ଯଦି (structp->emitToString()) ++m_emitForMembers;
        ଯଦି (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            ପାଇଁ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ଗଣ୍ଠି_DType* ଗଣ୍ଠି_Dtypep = memberp->dtypep();
                ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ uOrStructDTypep
                    = VN_CAST(ଗଣ୍ଠି_Dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } ଅନ୍ୟଥା {
                    ଯେତେବେଳେ (ଗଣ୍ଠି_Dtypep && ଗଣ୍ଠି_Dtypep->subDTypep()) {
                        ଗଣ୍ଠି_Dtypep = ଗଣ୍ଠି_Dtypep->subDTypep()->skipRefp();
                        ଯଦି (Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ uOrStructDTypep
                            = VN_CAST(ଗଣ୍ଠି_Dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            ଯଦି (structp->emitToString()) --m_emitForMembers;

            ଯଦି (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(ସତ୍ୟ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    ଶୂନ୍ୟ visit(AstIface* ifacep) override {
        ଯଦି (ifacep->user1())
            ଫେରନ୍ତୁ;  // if to_string() was already emitted skip the rest of the function
        ଯଦି (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(ସତ୍ୟ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    ଶୂନ୍ୟ visit(AstClass* classp) override {
        ଯଦି (classp->user1()) ଫେରନ୍ତୁ;
        ଯଦି (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_ମୂଳ);
            ++m_emit_Towards_ମୂଳ;
            ପାଇଁ (ଅଚଳ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ଯଦି (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(PrintedFromParent::PRINTED_FROM_PARENT));
        } ଅନ୍ୟଥା {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            ଯଦି (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_ମୂଳ == 0) {
                ଫେରନ୍ତୁ;
            }

            ଯଦି (m_emit_Towards_ମୂଳ > 0) markClassIterate(classp);

            ତାର୍କିକ is_ମୂଳ = ସତ୍ୟ;
            ପାଇଁ (ଅଚଳ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ଯଦି (extendp->classp()) {
                    ଯଦି (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_ମୂଳ > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    ଯଦି (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_ମୂଳ = ମିଥ୍ୟା;
                }
            }

            ଯଦି (is_ମୂଳ) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(printed_parent));
            ଯଦି (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_ମୂଳ);
                ++m_emit_Towards_ମୂଳ;
                ପାଇଁ (ଅଚଳ AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    ଯଦି (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit ToStringVisitor(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (v3Global.hasPrintedObjects()) {
            Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();
            Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// V3Common class functions

ଶୂନ୍ୟ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_ବୃକ୍ଷ("common", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
