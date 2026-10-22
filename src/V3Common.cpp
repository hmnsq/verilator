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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Common.h"

#অন্তর্ভাৱ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_গ্ৰন্থি_DType* nodep, ধ্রুৱ std::string& lhs) {
    std::string stmt;
    যদি (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_শব্দগুলি());
        stmt += ", ";
    } অন্যথা {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    ঘূৰাই stmt;
}

স্থির শূন্য makeVlToString(AstIface* nodep) {
    AstCFunc* ধ্রুৱ funcp
        = নূতন AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(মিছা);
    funcp->isConst(মিছা);
    funcp->isStatic(মিছা);
    funcp->protect(মিছা);
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = নূতন AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(নূতন AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
স্থির শূন্য makeVlToString(Ast_গ্ৰন্থি_UOr_Struct_DType* nodep) {
    Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ধ্রুৱ funcp
        = নূতন AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(মিছা);
    funcp->isConst(মিছা);
    funcp->isStatic(মিছা);
    funcp->protect(মিছা);
    funcp->addStmtsp(নূতন AstCStmt{nodep->fileline(), "std::string out;"});
    বাবে (ধ্রুৱ AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        যদি (itemp == nodep->membersp()) {
            stmt += "'{";
        } অন্যথা {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(নূতন AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(নূতন AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ধ্রুৱ exprp = নূতন AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(নূতন AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
স্থির শূন্য makeToString(AstClass* nodep) {
    AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(সঁচা);
    funcp->isStatic(মিছা);
    funcp->isVirtual(সঁচা);
    funcp->protect(মিছা);
    AstCExpr* ধ্রুৱ exprp = নূতন AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(নূতন AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
স্থির শূন্য make_To_String_মধ্য(AstClass* nodep) {
    AstCFunc* ধ্রুৱ funcp
        = নূতন AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(সঁচা);
    funcp->isStatic(মিছা);
    funcp->protect(মিছা);
    Ast_গ্ৰন্থি_Stmt* stmtsp = nullptr;
    std::string comma;
    বাবে (Ast_গ্ৰন্থি* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        যদি (ধ্রুৱ auto* ধ্রুৱ varp = VN_CAST(itemp, Var)) {
            ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = varp->dtypeSkipRefp()->basicp();
            যদি (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(সঁচা);  // So what we extend dumps this
                stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, নূতন AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    যদি (nodep->extendsp()) {
        string stmt = "out += ";
        যদি (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(সঁচা);  // So what we extend dumps this
        stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, নূতন AstCStmt{nodep->fileline(), stmt});
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* exprp;
    যদি (stmtsp) {
        funcp->addStmtsp(নূতন AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = নূতন AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } অন্যথা {  // Nothing to print, return ""
        exprp = নূতন AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(নূতন AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

শ্ৰেণী ToStringVisitor final : সার্ৱজনিক VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;
    size_t m_emit_Towards_মূল
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    গণনা শ্ৰেণী PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    শূন্য markClassIterate(AstClass* classp) {
        যদি (classp->user1SetOnce()) ঘূৰাই;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        বাবে (Ast_গ্ৰন্থি* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুৱ varp = VN_CAST(stmtp, Var)) {
                Ast_গ্ৰন্থি_DType* গ্ৰন্থি_Dtypep = varp->dtypep();
                যদি (AstIfaceRefDType* ধ্রুৱ ifaceRedDTypep = VN_CAST(গ্ৰন্থি_Dtypep, IfaceRefDType)) {
                    যদি (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } অন্যথা {
                    যেতিয়ালৈ (গ্ৰন্থি_Dtypep && গ্ৰন্থি_Dtypep->subDTypep()
                           && গ্ৰন্থি_Dtypep->subDTypep()->skipRefp()) {
                        গ্ৰন্থি_Dtypep = গ্ৰন্থি_Dtypep->subDTypep()->skipRefp();
                        যদি (AstIfaceRefDType* ধ্রুৱ ifaceRedDTypep
                            = VN_CAST(গ্ৰন্থি_Dtypep, IfaceRefDType)) {
                            যদি (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } অন্যথা যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ uOrStructDTypep
                                   = VN_CAST(গ্ৰন্থি_Dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_মধ্য(classp);
        ++m_statClassToString;
    }
    শূন্য visit(Ast_গ্ৰন্থি_UOr_Struct_DType* structp) override {
        যদি (structp->user1())
            ঘূৰাই;  // if to_string() was already emitted skip the rest of the function
        যদি (structp->emitToString()) ++m_emitForMembers;
        যদি (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            বাবে (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_গ্ৰন্থি_DType* গ্ৰন্থি_Dtypep = memberp->dtypep();
                যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ uOrStructDTypep
                    = VN_CAST(গ্ৰন্থি_Dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } অন্যথা {
                    যেতিয়ালৈ (গ্ৰন্থি_Dtypep && গ্ৰন্থি_Dtypep->subDTypep()) {
                        গ্ৰন্থি_Dtypep = গ্ৰন্থি_Dtypep->subDTypep()->skipRefp();
                        যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ uOrStructDTypep
                            = VN_CAST(গ্ৰন্থি_Dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            যদি (structp->emitToString()) --m_emitForMembers;

            যদি (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(সঁচা);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    শূন্য visit(AstIface* ifacep) override {
        যদি (ifacep->user1())
            ঘূৰাই;  // if to_string() was already emitted skip the rest of the function
        যদি (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(সঁচা);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    শূন্য visit(AstClass* classp) override {
        যদি (classp->user1()) ঘূৰাই;
        যদি (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_মূল);
            ++m_emit_Towards_মূল;
            বাবে (ধ্রুৱ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                যদি (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<পূর্ণাঙ্ক>(PrintedFromParent::PRINTED_FROM_PARENT));
        } অন্যথা {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            যদি (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_মূল == 0) {
                ঘূৰাই;
            }

            যদি (m_emit_Towards_মূল > 0) markClassIterate(classp);

            তার্কিক is_মূল = সঁচা;
            বাবে (ধ্রুৱ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                যদি (extendp->classp()) {
                    যদি (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_মূল > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    যদি (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_মূল = মিছা;
                }
            }

            যদি (is_মূল) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<পূর্ণাঙ্ক>(printed_parent));
            যদি (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_মূল);
                ++m_emit_Towards_মূল;
                বাবে (ধ্রুৱ AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    যদি (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    explicit ToStringVisitor(Ast_গ্ৰন্থি* nodep) {
        যদি (v3Global.hasPrintedObjects()) {
            Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();
            Ast_গ্ৰন্থি::user2_Clear_বৃক্ষ();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = স্ৱভাৱ;
};

//######################################################################
// V3Common class functions

শূন্য V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_বৃক্ষ("common", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
