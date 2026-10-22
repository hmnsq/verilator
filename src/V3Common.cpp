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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Common.h"

#شامل "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_گره_DType* nodep, ثابت std::string& lhs) {
    std::string stmt;
    اگر (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_واژگان());
        stmt += ", ";
    } وگرنه {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    بازگشت stmt;
}

ایستا تهی makeVlToString(AstIface* nodep) {
    AstCFunc* ثابت funcp
        = جدید AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(نادرست);
    funcp->isConst(نادرست);
    funcp->isStatic(نادرست);
    funcp->protect(نادرست);
    Ast_گره_عبارت* ثابت exprp = جدید AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(جدید AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ایستا تهی makeVlToString(Ast_گره_UOr_Struct_DType* nodep) {
    Ast_گره_Module* ثابت modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ثابت funcp
        = جدید AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(نادرست);
    funcp->isConst(نادرست);
    funcp->isStatic(نادرست);
    funcp->protect(نادرست);
    funcp->addStmtsp(جدید AstCStmt{nodep->fileline(), "std::string out;"});
    برای (ثابت AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        اگر (itemp == nodep->membersp()) {
            stmt += "'{";
        } وگرنه {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(جدید AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(جدید AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ثابت exprp = جدید AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(جدید AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
ایستا تهی makeToString(AstClass* nodep) {
    AstCFunc* ثابت funcp = جدید AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(درست);
    funcp->isStatic(نادرست);
    funcp->isVirtual(درست);
    funcp->protect(نادرست);
    AstCExpr* ثابت exprp = جدید AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(جدید AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ایستا تهی make_To_String_میانه(AstClass* nodep) {
    AstCFunc* ثابت funcp
        = جدید AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(درست);
    funcp->isStatic(نادرست);
    funcp->protect(نادرست);
    Ast_گره_Stmt* stmtsp = nullptr;
    std::string comma;
    برای (Ast_گره* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        اگر (ثابت auto* ثابت varp = VN_CAST(itemp, Var)) {
            ثابت AstBasicDType* ثابت basicp = varp->dtypeSkipRefp()->basicp();
            اگر (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(درست);  // So what we extend dumps this
                stmtsp = Ast_گره::addNextNull(stmtsp, جدید AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    اگر (nodep->extendsp()) {
        string stmt = "out += ";
        اگر (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(درست);  // So what we extend dumps this
        stmtsp = Ast_گره::addNextNull(stmtsp, جدید AstCStmt{nodep->fileline(), stmt});
    }

    Ast_گره_عبارت* exprp;
    اگر (stmtsp) {
        funcp->addStmtsp(جدید AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = جدید AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } وگرنه {  // Nothing to print, return ""
        exprp = جدید AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(جدید AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

رده ToStringVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    size_t m_emit_Towards_ریشه
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    شمارش رده PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    تهی markClassIterate(AstClass* classp) {
        اگر (classp->user1SetOnce()) بازگشت;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        برای (Ast_گره* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                Ast_گره_DType* گره_Dtypep = varp->dtypep();
                اگر (AstIfaceRefDType* ثابت ifaceRedDTypep = VN_CAST(گره_Dtypep, IfaceRefDType)) {
                    اگر (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } وگرنه {
                    وقتی (گره_Dtypep && گره_Dtypep->subDTypep()
                           && گره_Dtypep->subDTypep()->skipRefp()) {
                        گره_Dtypep = گره_Dtypep->subDTypep()->skipRefp();
                        اگر (AstIfaceRefDType* ثابت ifaceRedDTypep
                            = VN_CAST(گره_Dtypep, IfaceRefDType)) {
                            اگر (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } وگرنه اگر (Ast_گره_UOr_Struct_DType* ثابت uOrStructDTypep
                                   = VN_CAST(گره_Dtypep, گره_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_میانه(classp);
        ++m_statClassToString;
    }
    تهی visit(Ast_گره_UOr_Struct_DType* structp) override {
        اگر (structp->user1())
            بازگشت;  // if to_string() was already emitted skip the rest of the function
        اگر (structp->emitToString()) ++m_emitForMembers;
        اگر (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            برای (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_گره_DType* گره_Dtypep = memberp->dtypep();
                اگر (Ast_گره_UOr_Struct_DType* ثابت uOrStructDTypep
                    = VN_CAST(گره_Dtypep, گره_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } وگرنه {
                    وقتی (گره_Dtypep && گره_Dtypep->subDTypep()) {
                        گره_Dtypep = گره_Dtypep->subDTypep()->skipRefp();
                        اگر (Ast_گره_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گره_Dtypep, گره_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            اگر (structp->emitToString()) --m_emitForMembers;

            اگر (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(درست);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    تهی visit(AstIface* ifacep) override {
        اگر (ifacep->user1())
            بازگشت;  // if to_string() was already emitted skip the rest of the function
        اگر (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(درست);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    تهی visit(AstClass* classp) override {
        اگر (classp->user1()) بازگشت;
        اگر (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_ریشه);
            ++m_emit_Towards_ریشه;
            برای (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                اگر (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<صحیح>(PrintedFromParent::PRINTED_FROM_PARENT));
        } وگرنه {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            اگر (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_ریشه == 0) {
                بازگشت;
            }

            اگر (m_emit_Towards_ریشه > 0) markClassIterate(classp);

            منطقی is_ریشه = درست;
            برای (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                اگر (extendp->classp()) {
                    اگر (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_ریشه > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    اگر (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_ریشه = نادرست;
                }
            }

            اگر (is_ریشه) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<صحیح>(printed_parent));
            اگر (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_ریشه);
                ++m_emit_Towards_ریشه;
                برای (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    اگر (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    explicit ToStringVisitor(Ast_گره* nodep) {
        اگر (v3Global.hasPrintedObjects()) {
            Ast_گره::user1_Clear_درخت();
            Ast_گره::user2_Clear_درخت();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = پیشفرض;
};

//######################################################################
// V3Common class functions

تهی V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_درخت("common", 0, dump_درخت_Either_Level() >= 3);
}
