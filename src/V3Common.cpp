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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Common.h"

#ضمن "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(AstNodeDType* nodep, ثابت std::string& lhs) {
    std::string stmt;
    ܐܢ (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->widthWords());
        stmt += ", ";
    } ܐܚܪܢܐ {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    ܦܢܐ stmt;
}

ساكن فراغ makeVlToString(AstIface* nodep) {
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(ܕܓܠܐ);
    funcp->isConst(ܕܓܠܐ);
    funcp->isStatic(ܕܓܠܐ);
    funcp->protect(ܕܓܠܐ);
    AstNodeExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(جديد AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ساكن فراغ makeVlToString(AstNodeUOrStructDType* nodep) {
    AstNodeModule* ثابت modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(ܕܓܠܐ);
    funcp->isConst(ܕܓܠܐ);
    funcp->isStatic(ܕܓܠܐ);
    funcp->protect(ܕܓܠܐ);
    funcp->addStmtsp(جديد AstCStmt{nodep->fileline(), "std::string out;"});
    لكل (ثابت AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        ܐܢ (itemp == nodep->membersp()) {
            stmt += "'{";
        } ܐܚܪܢܐ {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(جديد AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(جديد AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(جديد AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
ساكن فراغ makeToString(AstClass* nodep) {
    AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(ܫܪܪܐ);
    funcp->isStatic(ܕܓܠܐ);
    funcp->isVirtual(ܫܪܪܐ);
    funcp->protect(ܕܓܠܐ);
    AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(جديد AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
ساكن فراغ makeToStringMiddle(AstClass* nodep) {
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(ܫܪܪܐ);
    funcp->isStatic(ܕܓܠܐ);
    funcp->protect(ܕܓܠܐ);
    AstNodeStmt* stmtsp = nullptr;
    std::string comma;
    لكل (AstNode* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        ܐܢ (ثابت auto* ثابت varp = VN_CAST(itemp, Var)) {
            ثابت AstBasicDType* ثابت basicp = varp->dtypeSkipRefp()->basicp();
            ܐܢ (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(ܫܪܪܐ);  // So what we extend dumps this
                stmtsp = AstNode::addNextNull(stmtsp, جديد AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    ܐܢ (nodep->extendsp()) {
        string stmt = "out += ";
        ܐܢ (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(ܫܪܪܐ);  // So what we extend dumps this
        stmtsp = AstNode::addNextNull(stmtsp, جديد AstCStmt{nodep->fileline(), stmt});
    }

    AstNodeExpr* exprp;
    ܐܢ (stmtsp) {
        funcp->addStmtsp(جديد AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = جديد AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } ܐܚܪܢܐ {  // Nothing to print, return ""
        exprp = جديد AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(جديد AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

صنف ToStringVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    size_t m_emitTowardsRoot
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    تعداد صنف PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    فراغ markClassIterate(AstClass* classp) {
        ܐܢ (classp->user1SetOnce()) ܦܢܐ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        لكل (AstNode* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ܐܢ (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                AstNodeDType* nodeDtypep = varp->dtypep();
                ܐܢ (AstIfaceRefDType* ثابت ifaceRedDTypep = VN_CAST(nodeDtypep, IfaceRefDType)) {
                    ܐܢ (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } ܐܚܪܢܐ {
                    ܟܡܐ (nodeDtypep && nodeDtypep->subDTypep()
                           && nodeDtypep->subDTypep()->skipRefp()) {
                        nodeDtypep = nodeDtypep->subDTypep()->skipRefp();
                        ܐܢ (AstIfaceRefDType* ثابت ifaceRedDTypep
                            = VN_CAST(nodeDtypep, IfaceRefDType)) {
                            ܐܢ (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } ܐܚܪܢܐ ܐܢ (AstNodeUOrStructDType* ثابت uOrStructDTypep
                                   = VN_CAST(nodeDtypep, NodeUOrStructDType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        makeToStringMiddle(classp);
        ++m_statClassToString;
    }
    فراغ visit(AstNodeUOrStructDType* structp) override {
        ܐܢ (structp->user1())
            ܦܢܐ;  // if to_string() was already emitted skip the rest of the function
        ܐܢ (structp->emitToString()) ++m_emitForMembers;
        ܐܢ (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                AstNodeDType* nodeDtypep = memberp->dtypep();
                ܐܢ (AstNodeUOrStructDType* ثابت uOrStructDTypep
                    = VN_CAST(nodeDtypep, NodeUOrStructDType)) {
                    iterateConst(uOrStructDTypep);
                } ܐܚܪܢܐ {
                    ܟܡܐ (nodeDtypep && nodeDtypep->subDTypep()) {
                        nodeDtypep = nodeDtypep->subDTypep()->skipRefp();
                        ܐܢ (AstNodeUOrStructDType* ثابت uOrStructDTypep
                            = VN_CAST(nodeDtypep, NodeUOrStructDType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            ܐܢ (structp->emitToString()) --m_emitForMembers;

            ܐܢ (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(ܫܪܪܐ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    فراغ visit(AstIface* ifacep) override {
        ܐܢ (ifacep->user1())
            ܦܢܐ;  // if to_string() was already emitted skip the rest of the function
        ܐܢ (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(ܫܪܪܐ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    فراغ visit(AstClass* classp) override {
        ܐܢ (classp->user1()) ܦܢܐ;
        ܐܢ (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emitTowardsRoot);
            ++m_emitTowardsRoot;
            لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ܐܢ (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<صحيح>(PrintedFromParent::PRINTED_FROM_PARENT));
        } ܐܚܪܢܐ {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            ܐܢ (printed_parent != PrintedFromParent::UNRESOLVED && m_emitTowardsRoot == 0) {
                ܦܢܐ;
            }

            ܐܢ (m_emitTowardsRoot > 0) markClassIterate(classp);

            منطقي is_root = ܫܪܪܐ;
            لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ܐܢ (extendp->classp()) {
                    ܐܢ (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emitTowardsRoot > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    ܐܢ (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_root = ܕܓܠܐ;
                }
            }

            ܐܢ (is_root) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<صحيح>(printed_parent));
            ܐܢ (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emitTowardsRoot);
                ++m_emitTowardsRoot;
                لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    ܐܢ (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

عام:
    explicit ToStringVisitor(AstNode* nodep) {
        ܐܢ (v3Global.hasPrintedObjects()) {
            AstNode::user1ClearTree();
            AstNode::user2ClearTree();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = افتراضي;
};

//######################################################################
// V3Common class functions

فراغ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dumpCheckGlobalTree("common", 0, dumpTreeEitherLevel() >= 3);
}
