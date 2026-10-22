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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Common.h"

#അന്തര്ഭാവ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::makeToStringCall(Ast_കണ്ണി_DType* nodep, സ്ഥിരം std::string& lhs) {
    std::string stmt;
    എങ്കിൽ (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvtToStr(nodep->width_വാക്കുകൾ());
        stmt += ", ";
    } അല്ലെങ്കിൽ {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    തിരികെ stmt;
}

സ്ഥിതം ശൂന്യം makeVlToString(AstIface* nodep) {
    AstCFunc* സ്ഥിരം funcp
        = പുതിയ AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "* obj");
    funcp->isMethod(അസത്യം);
    funcp->isConst(അസത്യം);
    funcp->isStatic(അസത്യം);
    funcp->protect(അസത്യം);
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = പുതിയ AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtypeSetString();
    funcp->addStmtsp(പുതിയ AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
സ്ഥിതം ശൂന്യം makeVlToString(Ast_കണ്ണി_UOr_Struct_DType* nodep) {
    Ast_കണ്ണി_Module* സ്ഥിരം modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* സ്ഥിരം funcp
        = പുതിയ AstCFunc{nodep->fileline(), "VL_TO_STRING", nullptr, "std::string"};
    funcp->argTypes("const " + EmitCUtil::prefixNameProtect(nodep) + "& obj");
    funcp->isMethod(അസത്യം);
    funcp->isConst(അസത്യം);
    funcp->isStatic(അസത്യം);
    funcp->protect(അസത്യം);
    funcp->addStmtsp(പുതിയ AstCStmt{nodep->fileline(), "std::string out;"});
    വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        എങ്കിൽ (itemp == nodep->membersp()) {
            stmt += "'{";
        } അല്ലെങ്കിൽ {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::makeToStringCall(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->addStmtsp(പുതിയ AstCStmt{nodep->fileline(), stmt});
    }
    funcp->addStmtsp(പുതിയ AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* സ്ഥിരം exprp = പുതിയ AstCExpr{nodep->fileline(), "out"};
    exprp->dtypeSetString();
    funcp->addStmtsp(പുതിയ AstCReturn{nodep->fileline(), exprp});

    modp->addStmtsp(funcp);
}
സ്ഥിതം ശൂന്യം makeToString(AstClass* nodep) {
    AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{nodep->fileline(), "to_string", nullptr, "std::string"};
    funcp->isConst(സത്യം);
    funcp->isStatic(അസത്യം);
    funcp->isVirtual(സത്യം);
    funcp->protect(അസത്യം);
    AstCExpr* സ്ഥിരം exprp = പുതിയ AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtypeSetString();
    funcp->addStmtsp(പുതിയ AstCReturn{nodep->fileline(), exprp});
    nodep->addStmtsp(funcp);
}
സ്ഥിതം ശൂന്യം make_To_String_നടുവ്(AstClass* nodep) {
    AstCFunc* സ്ഥിരം funcp
        = പുതിയ AstCFunc{nodep->fileline(), "to_string_middle", nullptr, "std::string"};
    funcp->isConst(സത്യം);
    funcp->isStatic(അസത്യം);
    funcp->protect(അസത്യം);
    Ast_കണ്ണി_Stmt* stmtsp = nullptr;
    std::string comma;
    വേണ്ടി (Ast_കണ്ണി* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം varp = VN_CAST(itemp, Var)) {
            സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = varp->dtypeSkipRefp()->basicp();
            എങ്കിൽ (!varp->isParam() && !varp->isInternal()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::makeToStringCall(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(സത്യം);  // So what we extend dumps this
                stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, പുതിയ AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    എങ്കിൽ (nodep->extendsp()) {
        string stmt = "out += ";
        എങ്കിൽ (!comma.empty()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::prefixNameProtect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(സത്യം);  // So what we extend dumps this
        stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, പുതിയ AstCStmt{nodep->fileline(), stmt});
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* exprp;
    എങ്കിൽ (stmtsp) {
        funcp->addStmtsp(പുതിയ AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->addStmtsp(stmtsp);
        exprp = പുതിയ AstCExpr{nodep->fileline(), "out"};
        exprp->dtypeSetString();
    } അല്ലെങ്കിൽ {  // Nothing to print, return ""
        exprp = പുതിയ AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->addStmtsp(പുതിയ AstCReturn{nodep->fileline(), exprp});

    nodep->addStmtsp(funcp);
}

വർഗ്ഗം ToStringVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    size_t m_emit_Towards_വേര്
        = 0;  // class that require its ancestors to emit to_string() sets this
    size_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    എണ്ണൽ വർഗ്ഗം PrintedFromParent : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_PARENT,
        NO_PRINTED_FROM_PARENT
    };

    VDouble0 m_statClassToString;
    VDouble0 m_statInterfaceToString;
    VDouble0 m_statStructUnionToString;

    ശൂന്യം markClassIterate(AstClass* classp) {
        എങ്കിൽ (classp->user1SetOnce()) തിരികെ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        വേണ്ടി (Ast_കണ്ണി* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                Ast_കണ്ണി_DType* കണ്ണി_Dtypep = varp->dtypep();
                എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം ifaceRedDTypep = VN_CAST(കണ്ണി_Dtypep, IfaceRefDType)) {
                    എങ്കിൽ (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } അല്ലെങ്കിൽ {
                    ആകുമ്പോൾ (കണ്ണി_Dtypep && കണ്ണി_Dtypep->subDTypep()
                           && കണ്ണി_Dtypep->subDTypep()->skipRefp()) {
                        കണ്ണി_Dtypep = കണ്ണി_Dtypep->subDTypep()->skipRefp();
                        എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം ifaceRedDTypep
                            = VN_CAST(കണ്ണി_Dtypep, IfaceRefDType)) {
                            എങ്കിൽ (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം uOrStructDTypep
                                   = VN_CAST(കണ്ണി_Dtypep, കണ്ണി_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        makeToString(classp);
        make_To_String_നടുവ്(classp);
        ++m_statClassToString;
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* structp) override {
        എങ്കിൽ (structp->user1())
            തിരികെ;  // if to_string() was already emitted skip the rest of the function
        എങ്കിൽ (structp->emitToString()) ++m_emitForMembers;
        എങ്കിൽ (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            വേണ്ടി (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_കണ്ണി_DType* കണ്ണി_Dtypep = memberp->dtypep();
                എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം uOrStructDTypep
                    = VN_CAST(കണ്ണി_Dtypep, കണ്ണി_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } അല്ലെങ്കിൽ {
                    ആകുമ്പോൾ (കണ്ണി_Dtypep && കണ്ണി_Dtypep->subDTypep()) {
                        കണ്ണി_Dtypep = കണ്ണി_Dtypep->subDTypep()->skipRefp();
                        എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം uOrStructDTypep
                            = VN_CAST(കണ്ണി_Dtypep, കണ്ണി_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            എങ്കിൽ (structp->emitToString()) --m_emitForMembers;

            എങ്കിൽ (!structp->packed()) {
                makeVlToString(structp);
                ++m_statStructUnionToString;
            }

            structp->user1(സത്യം);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    ശൂന്യം visit(AstIface* ifacep) override {
        എങ്കിൽ (ifacep->user1())
            തിരികെ;  // if to_string() was already emitted skip the rest of the function
        എങ്കിൽ (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            makeVlToString(ifacep);
            ++m_statInterfaceToString;
            ifacep->user1(സത്യം);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    ശൂന്യം visit(AstClass* classp) override {
        എങ്കിൽ (classp->user1()) തിരികെ;
        എങ്കിൽ (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_വേര്);
            ++m_emit_Towards_വേര്;
            വേണ്ടി (സ്ഥിരം AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                എങ്കിൽ (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<പൂർണ്ണസംഖ്യ>(PrintedFromParent::PRINTED_FROM_PARENT));
        } അല്ലെങ്കിൽ {
            PrintedFromParent printed_parent = static_cast<PrintedFromParent>(classp->user2());
            എങ്കിൽ (printed_parent != PrintedFromParent::UNRESOLVED && m_emit_Towards_വേര് == 0) {
                തിരികെ;
            }

            എങ്കിൽ (m_emit_Towards_വേര് > 0) markClassIterate(classp);

            തർക്കം is_വേര് = സത്യം;
            വേണ്ടി (സ്ഥിരം AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                എങ്കിൽ (extendp->classp()) {
                    എങ്കിൽ (static_cast<PrintedFromParent>(extendp->classp()->user2())
                            == PrintedFromParent::UNRESOLVED
                        || (m_emit_Towards_വേര് > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    എങ്കിൽ (printed_parent != PrintedFromParent::PRINTED_FROM_PARENT) {
                        printed_parent
                            = static_cast<PrintedFromParent>(extendp->classp()->user2());
                    }
                    is_വേര് = അസത്യം;
                }
            }

            എങ്കിൽ (is_വേര്) printed_parent = PrintedFromParent::NO_PRINTED_FROM_PARENT;

            classp->user2(static_cast<പൂർണ്ണസംഖ്യ>(printed_parent));
            എങ്കിൽ (printed_parent == PrintedFromParent::PRINTED_FROM_PARENT) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_വേര്);
                ++m_emit_Towards_വേര്;
                വേണ്ടി (സ്ഥിരം AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    എങ്കിൽ (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    explicit ToStringVisitor(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (v3Global.hasPrintedObjects()) {
            Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
            Ast_കണ്ണി::user2_Clear_വൃക്ഷം();

            iterateConst(nodep);
        }

        V3Stats::addStat("Optimizations, Class ToString emitted", m_statClassToString);
        V3Stats::addStat("Optimizations, Interface ToString emitted", m_statInterfaceToString);
        V3Stats::addStat("Optimizations, Struct/union ToString emitted",
                         m_statStructUnionToString);
    }
    ~ToStringVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// V3Common class functions

ശൂന്യം V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ToStringVisitor{v3Global.rootp()};

    V3Global::dump_Check_Global_വൃക്ഷം("common", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
