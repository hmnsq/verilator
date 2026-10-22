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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Common.h"

#அந்தர்பாவ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_முனை_DType* nodep, மாறிலி std::string& lhs) {
    std::string stmt;
    எனில் (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_சொற்கள்());
        stmt += ", ";
    } இல்லையெனில் {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    திரும்பு stmt;
}

நிலையான வெறுமை makeVlToString(AstIface* nodep) {
    AstCFunc* மாறிலி funcp
        = புதிய AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(பொய்);
    funcp->isConst(பொய்);
    funcp->isStatic(பொய்);
    funcp->protect(பொய்);
    Ast_முனை_கோவை* மாறிலி exprp = புதிய AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(புதிய AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
நிலையான வெறுமை makeVlToString(Ast_முனை_UOr_Struct_DType* nodep) {
    Ast_முனை_Module* மாறிலி modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* மாறிலி funcp
        = புதிய AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(பொய்);
    funcp->isConst(பொய்);
    funcp->isStatic(பொய்);
    funcp->protect(பொய்);
    funcp->addStmtsp(புதிய AstCStmt{nodep->fileline(), "std::string out;"});
    க்கு (மாறிலி AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        எனில் (itemp == nodep->membersp()) {
            stmt += "'{";
        } இல்லையெனில் {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(புதிய AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(புதிய AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* மாறிலி exprp = புதிய AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(புதிய AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
நிலையான வெறுமை makeToString(AstClass* nodep) {
    AstCFunc* மாறிலி funcp = புதிய AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(உண்மை);
    funcp->isStatic(பொய்);
    funcp->isVirtual(உண்மை);
    funcp->protect(பொய்);
    AstCExpr* மாறிலி exprp = புதிய AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(புதிய AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
நிலையான வெறுமை make_To_String_நடு(AstClass* nodep) {
    AstCFunc* மாறிலி funcp
        = புதிய AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(உண்மை);
    funcp->isStatic(பொய்);
    funcp->protect(பொய்);
    Ast_முனை_Stmt* stmtsp = nullptr;
    std::string comma;
    க்கு (Ast_முனை* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        எனில் (மாறிலி auto* மாறிலி varp = VN_CAST(itemp, Var)) {
            மாறிலி AstBasicDType* மாறிலி basicp = varp->dtypeSkipRefp()->basicp();
            எனில் (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(உண்மை);  // So what we extend dumps this
                stmtsp = Ast_முனை::addNextNull(stmtsp, புதிய AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    எனில் (nodep->extendsp()) {
        string stmt = "out += ";
        எனில் (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(உண்மை);  // So what we extend dumps this
        stmtsp = Ast_முனை::addNextNull(stmtsp, புதிய AstCStmt{nodep->fileline(), stmt});
    }

    Ast_முனை_கோவை* exprp;
    எனில் (stmtsp) {
        funcp->addStmtsp(புதிய AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = புதிய AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } இல்லையெனில் {  // Nothing to print, return ""
        exprp = புதிய AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(புதிய AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

வகுப்பு ToStringVisitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    size_t m_emit_Towards_வேர்
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    எண்ணிக்கை வகுப்பு PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    வெறுமை markClassIterate(AstClass* classp) {
        எனில் (classp->user1SetOnce()) திரும்பு;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        க்கு (Ast_முனை* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                Ast_முனை_DType* முனை_Dtypep = varp->dtypep();
                எனில் (AstIfaceRefDType* மாறிலி ifaceRedDTypep = VN_CAST(முனை_Dtypep, IfaceRefDType)) {
                    எனில் (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } இல்லையெனில் {
                    போது (முனை_Dtypep && முனை_Dtypep->subDTypep()
                           && முனை_Dtypep->subDTypep()->skipRefp()) {
                        முனை_Dtypep = முனை_Dtypep->subDTypep()->skipRefp();
                        எனில் (AstIfaceRefDType* மாறிலி ifaceRedDTypep
                            = VN_CAST(முனை_Dtypep, IfaceRefDType)) {
                            எனில் (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } இல்லையெனில் எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி uOrStructDTypep
                                   = VN_CAST(முனை_Dtypep, முனை_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_நடு(classp);
        ++m_statClassToString;
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* structp) override {
        எனில் (structp->user1())
            திரும்பு;  // if to_string() was already emitted skip the rest of the function
        எனில் (structp->emitToString()) ++m_emitForMembers;
        எனில் (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            க்கு (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_முனை_DType* முனை_Dtypep = memberp->dtypep();
                எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி uOrStructDTypep
                    = VN_CAST(முனை_Dtypep, முனை_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } இல்லையெனில் {
                    போது (முனை_Dtypep && முனை_Dtypep->subDTypep()) {
                        முனை_Dtypep = முனை_Dtypep->subDTypep()->skipRefp();
                        எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி uOrStructDTypep
                            = VN_CAST(முனை_Dtypep, முனை_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            எனில் (structp->emitToString()) --m_emitForMembers;

            எனில் (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(உண்மை);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    வெறுமை visit(AstIface* ifacep) override {
        எனில் (ifacep->user1())
            திரும்பு;  // if to_string() was already emitted skip the rest of the function
        எனில் (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(உண்மை);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    வெறுமை visit(AstClass* classp) override {
        எனில் (classp->user1()) திரும்பு;
        எனில் (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_வேர்);
            ++m_emit_Towards_வேர்;
            க்கு (மாறிலி AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                எனில் (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<முழு>(PrintedFromParent::PRINTED_FROM_PARENT));
        } இல்லையெனில் {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            எனில் (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_வேர் == 0) {
                திரும்பு;
            }

            எனில் (m_emit_Towards_வேர் > 0) markClassIterate(classp);

            தருக்க is_வேர் = உண்மை;
            க்கு (மாறிலி AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                எனில் (extendp->classp()) {
                    எனில் (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_வேர் > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    எனில் (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_வேர் = பொய்;
                }
            }

            எனில் (is_வேர்) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<முழு>(printed_parent));
            எனில் (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_வேர்);
                ++m_emit_Towards_வேர்;
                க்கு (மாறிலி AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    எனில் (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    explicit ToStringVisitor(Ast_முனை* nodep) {
        எனில் (v3Global.hasPrintedObjects()) {
            Ast_முனை::user1_Clear_மரம்();
            Ast_முனை::user2_Clear_மரம்();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = இயல்பு;
};

//######################################################################
// V3Common class functions

வெறுமை V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_மரம்("common", 0, dump_மரம்_Either_Level() >= 3);
}
