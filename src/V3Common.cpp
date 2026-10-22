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

string V3Common::make_منزل_String_Call(Ast_غوټه_DType* nodep, ثابت std::string& lhs) {
    std::string stmt;
    إذا (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvt_منزل_Str(nodep->پلنوالی_ټکي());
        stmt += ", ";
    } وإلا {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    أرجع stmt;
}

ساكن فراغ make_Vl_منزل_String(AstIface* nodep) {
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "VL_TO_STRING", تش_اشاره, "std::string"};
    funcp->argTypes("const " + EmitCUtil::مختاړی_Name_Protect(nodep) + "* obj");
    funcp->is_لاره(خطأ);
    funcp->isConst(خطأ);
    funcp->isStatic(خطأ);
    funcp->protect(خطأ);
    Ast_غوټه_بیان* ثابت exprp = جديد AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtype_سېټ_String();
    funcp->جمع_کړه_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});
    nodep->جمع_کړه_Stmtsp(funcp);
}
ساكن فراغ make_Vl_منزل_String(Ast_غوټه_UOr_Struct_DType* nodep) {
    Ast_غوټه_Module* ثابت modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "VL_TO_STRING", تش_اشاره, "std::string"};
    funcp->argTypes("const " + EmitCUtil::مختاړی_Name_Protect(nodep) + "& obj");
    funcp->is_لاره(خطأ);
    funcp->isConst(خطأ);
    funcp->isStatic(خطأ);
    funcp->protect(خطأ);
    funcp->جمع_کړه_Stmtsp(جديد AstCStmt{nodep->fileline(), "std::string out;"});
    لكل (ثابت AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        إذا (itemp == nodep->membersp()) {
            stmt += "'{";
        } وإلا {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::make_منزل_String_Call(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->جمع_کړه_Stmtsp(جديد AstCStmt{nodep->fileline(), stmt});
    }
    funcp->جمع_کړه_Stmtsp(جديد AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), "out"};
    exprp->dtype_سېټ_String();
    funcp->جمع_کړه_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});

    modp->جمع_کړه_Stmtsp(funcp);
}
ساكن فراغ make_منزل_String(AstClass* nodep) {
    AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), "to_string", تش_اشاره, "std::string"};
    funcp->isConst(ریښتیا);
    funcp->isStatic(خطأ);
    funcp->isVirtual(ریښتیا);
    funcp->protect(خطأ);
    AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtype_سېټ_String();
    funcp->جمع_کړه_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});
    nodep->جمع_کړه_Stmtsp(funcp);
}
ساكن فراغ make_منزل_String_منځ(AstClass* nodep) {
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "to_string_middle", تش_اشاره, "std::string"};
    funcp->isConst(ریښتیا);
    funcp->isStatic(خطأ);
    funcp->protect(خطأ);
    Ast_غوټه_Stmt* stmtsp = تش_اشاره;
    std::string comma;
    لكل (Ast_غوټه* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        إذا (ثابت خپلکاری* ثابت varp = VN_CAST(itemp, Var)) {
            ثابت AstBasicDType* ثابت basicp = varp->dtypeSkipRefp()->basicp();
            إذا (!varp->isParam() && !varp->is_دننی()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::make_منزل_String_Call(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(ریښتیا);  // So what we extend dumps this
                stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, جديد AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    إذا (nodep->extendsp()) {
        string stmt = "out += ";
        إذا (!comma.تشه()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::مختاړی_Name_Protect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(ریښتیا);  // So what we extend dumps this
        stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, جديد AstCStmt{nodep->fileline(), stmt});
    }

    Ast_غوټه_بیان* exprp;
    إذا (stmtsp) {
        funcp->جمع_کړه_Stmtsp(جديد AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->جمع_کړه_Stmtsp(stmtsp);
        exprp = جديد AstCExpr{nodep->fileline(), "out"};
        exprp->dtype_سېټ_String();
    } وإلا {  // Nothing to print, return ""
        exprp = جديد AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->جمع_کړه_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});

    nodep->جمع_کړه_Stmtsp(funcp);
}

صنف منزل_String_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    اندازه_t m_emit_Towards_ریښه
        = 0;  // class that require its ancestors to emit to_string() sets this
    اندازه_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    تعداد صنف Printed_From_پلار : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_پلار,
        NO_PRINTED_FROM_پلار
    };

    VDouble0 m_stat_Class_منزل_String;
    VDouble0 m_stat_Interface_منزل_String;
    VDouble0 m_stat_Struct_Union_منزل_String;

    فراغ markClassIterate(AstClass* classp) {
        إذا (classp->user1_سېټ_Once()) أرجع;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        لكل (Ast_غوټه* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                Ast_غوټه_DType* غوټه_Dtypep = varp->dtypep();
                إذا (AstIfaceRefDType* ثابت ifaceRedDTypep = VN_CAST(غوټه_Dtypep, IfaceRefDType)) {
                    إذا (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } وإلا {
                    طالما (غوټه_Dtypep && غوټه_Dtypep->subDTypep()
                           && غوټه_Dtypep->subDTypep()->skipRefp()) {
                        غوټه_Dtypep = غوټه_Dtypep->subDTypep()->skipRefp();
                        إذا (AstIfaceRefDType* ثابت ifaceRedDTypep
                            = VN_CAST(غوټه_Dtypep, IfaceRefDType)) {
                            إذا (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } وإلا إذا (Ast_غوټه_UOr_Struct_DType* ثابت uOrStructDTypep
                                   = VN_CAST(غوټه_Dtypep, غوټه_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        make_منزل_String(classp);
        make_منزل_String_منځ(classp);
        ++m_stat_Class_منزل_String;
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* structp) override {
        إذا (structp->user1())
            أرجع;  // if to_string() was already emitted skip the rest of the function
        إذا (structp->emit_منزل_String()) ++m_emitForMembers;
        إذا (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_غوټه_DType* غوټه_Dtypep = memberp->dtypep();
                إذا (Ast_غوټه_UOr_Struct_DType* ثابت uOrStructDTypep
                    = VN_CAST(غوټه_Dtypep, غوټه_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } وإلا {
                    طالما (غوټه_Dtypep && غوټه_Dtypep->subDTypep()) {
                        غوټه_Dtypep = غوټه_Dtypep->subDTypep()->skipRefp();
                        إذا (Ast_غوټه_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(غوټه_Dtypep, غوټه_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            إذا (structp->emit_منزل_String()) --m_emitForMembers;

            إذا (!structp->packed()) {
                make_Vl_منزل_String(structp);
                ++m_stat_Struct_Union_منزل_String;
            }

            structp->user1(ریښتیا);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    فراغ لیدنه(AstIface* ifacep) override {
        إذا (ifacep->user1())
            أرجع;  // if to_string() was already emitted skip the rest of the function
        إذا (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            make_Vl_منزل_String(ifacep);
            ++m_stat_Interface_منزل_String;
            ifacep->user1(ریښتیا);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    فراغ لیدنه(AstClass* classp) override {
        إذا (classp->user1()) أرجع;
        إذا (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_ریښه);
            ++m_emit_Towards_ریښه;
            لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                إذا (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<صحيح>(Printed_From_پلار::PRINTED_FROM_پلار));
        } وإلا {
            Printed_From_پلار printed_پلار = static_cast<Printed_From_پلار>(classp->user2());
            إذا (printed_پلار != Printed_From_پلار::UNRESOLVED && m_emit_Towards_ریښه == 0) {
                أرجع;
            }

            إذا (m_emit_Towards_ریښه > 0) markClassIterate(classp);

            منطقي is_ریښه = ریښتیا;
            لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                إذا (extendp->classp()) {
                    إذا (static_cast<Printed_From_پلار>(extendp->classp()->user2())
                            == Printed_From_پلار::UNRESOLVED
                        || (m_emit_Towards_ریښه > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    إذا (printed_پلار != Printed_From_پلار::PRINTED_FROM_پلار) {
                        printed_پلار
                            = static_cast<Printed_From_پلار>(extendp->classp()->user2());
                    }
                    is_ریښه = خطأ;
                }
            }

            إذا (is_ریښه) printed_پلار = Printed_From_پلار::NO_PRINTED_FROM_پلار;

            classp->user2(static_cast<صحيح>(printed_پلار));
            إذا (printed_پلار == Printed_From_پلار::PRINTED_FROM_پلار) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_ریښه);
                ++m_emit_Towards_ریښه;
                لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    إذا (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    څرګند منزل_String_Visitor(Ast_غوټه* nodep) {
        إذا (v3Global.hasPrintedObjects()) {
            Ast_غوټه::user1_Clear_ونه();
            Ast_غوټه::user2_Clear_ونه();

            iterateConst(nodep);
        }

        V3Stats::جمع_کړه_Stat("Optimizations, Class ToString emitted", m_stat_Class_منزل_String);
        V3Stats::جمع_کړه_Stat("Optimizations, Interface ToString emitted", m_stat_Interface_منزل_String);
        V3Stats::جمع_کړه_Stat("Optimizations, Struct/union ToString emitted",
                         m_stat_Struct_Union_منزل_String);
    }
    ~منزل_String_Visitor() override = افتراضي;
};

//######################################################################
// V3Common class functions

فراغ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    منزل_String_Visitor{v3Global.rootp()};

    V3Global::dump_Check_Global_ونه("common", 0, dump_ونه_Either_Level() >= 3);
}
