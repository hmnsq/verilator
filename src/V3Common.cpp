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

string V3Common::make_منزل_String_Call(Ast_گرہ_DType* nodep, ثابت std::string& lhs) {
    std::string stmt;
    إذا (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvt_منزل_Str(nodep->چوڑائی_الفاظ());
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
        = جديد AstCFunc{nodep->fileline(), "VL_TO_STRING", خالی_سوچک, "std::string"};
    funcp->argTypes("const " + EmitCUtil::اگیتر_Name_Protect(nodep) + "* obj");
    funcp->is_طریقہ(خطأ);
    funcp->isConst(خطأ);
    funcp->isStatic(خطأ);
    funcp->protect(خطأ);
    Ast_گرہ_اظہاریہ* ثابت exprp = جديد AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtype_سیٹ_String();
    funcp->جوڑو_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});
    nodep->جوڑو_Stmtsp(funcp);
}
ساكن فراغ make_Vl_منزل_String(Ast_گرہ_UOr_Struct_DType* nodep) {
    Ast_گرہ_Module* ثابت modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "VL_TO_STRING", خالی_سوچک, "std::string"};
    funcp->argTypes("const " + EmitCUtil::اگیتر_Name_Protect(nodep) + "& obj");
    funcp->is_طریقہ(خطأ);
    funcp->isConst(خطأ);
    funcp->isStatic(خطأ);
    funcp->protect(خطأ);
    funcp->جوڑو_Stmtsp(جديد AstCStmt{nodep->fileline(), "std::string out;"});
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
        funcp->جوڑو_Stmtsp(جديد AstCStmt{nodep->fileline(), stmt});
    }
    funcp->جوڑو_Stmtsp(جديد AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), "out"};
    exprp->dtype_سیٹ_String();
    funcp->جوڑو_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});

    modp->جوڑو_Stmtsp(funcp);
}
ساكن فراغ make_منزل_String(AstClass* nodep) {
    AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), "to_string", خالی_سوچک, "std::string"};
    funcp->isConst(سچ);
    funcp->isStatic(خطأ);
    funcp->isVirtual(سچ);
    funcp->protect(خطأ);
    AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtype_سیٹ_String();
    funcp->جوڑو_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});
    nodep->جوڑو_Stmtsp(funcp);
}
ساكن فراغ make_منزل_String_درمیان(AstClass* nodep) {
    AstCFunc* ثابت funcp
        = جديد AstCFunc{nodep->fileline(), "to_string_middle", خالی_سوچک, "std::string"};
    funcp->isConst(سچ);
    funcp->isStatic(خطأ);
    funcp->protect(خطأ);
    Ast_گرہ_Stmt* stmtsp = خالی_سوچک;
    std::string comma;
    لكل (Ast_گرہ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        إذا (ثابت خودکار* ثابت varp = VN_CAST(itemp, Var)) {
            ثابت AstBasicDType* ثابت basicp = varp->dtypeSkipRefp()->basicp();
            إذا (!varp->isParam() && !varp->is_اندرونی()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::make_منزل_String_Call(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(سچ);  // So what we extend dumps this
                stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, جديد AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    إذا (nodep->extendsp()) {
        string stmt = "out += ";
        إذا (!comma.خالی_تھاواں()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::اگیتر_Name_Protect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(سچ);  // So what we extend dumps this
        stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, جديد AstCStmt{nodep->fileline(), stmt});
    }

    Ast_گرہ_اظہاریہ* exprp;
    إذا (stmtsp) {
        funcp->جوڑو_Stmtsp(جديد AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->جوڑو_Stmtsp(stmtsp);
        exprp = جديد AstCExpr{nodep->fileline(), "out"};
        exprp->dtype_سیٹ_String();
    } وإلا {  // Nothing to print, return ""
        exprp = جديد AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->جوڑو_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});

    nodep->جوڑو_Stmtsp(funcp);
}

صنف منزل_String_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;
    ناپ_t m_emit_Towards_جڑ
        = 0;  // class that require its ancestors to emit to_string() sets this
    ناپ_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    تعداد صنف Printed_From_ماپا : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_ماپا,
        NO_PRINTED_FROM_ماپا
    };

    VDouble0 m_stat_Class_منزل_String;
    VDouble0 m_stat_Interface_منزل_String;
    VDouble0 m_stat_Struct_Union_منزل_String;

    فراغ markClassIterate(AstClass* classp) {
        إذا (classp->user1_سیٹ_Once()) أرجع;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        لكل (Ast_گرہ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                Ast_گرہ_DType* گرہ_Dtypep = varp->dtypep();
                إذا (AstIfaceRefDType* ثابت ifaceRedDTypep = VN_CAST(گرہ_Dtypep, IfaceRefDType)) {
                    إذا (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } وإلا {
                    طالما (گرہ_Dtypep && گرہ_Dtypep->subDTypep()
                           && گرہ_Dtypep->subDTypep()->skipRefp()) {
                        گرہ_Dtypep = گرہ_Dtypep->subDTypep()->skipRefp();
                        إذا (AstIfaceRefDType* ثابت ifaceRedDTypep
                            = VN_CAST(گرہ_Dtypep, IfaceRefDType)) {
                            إذا (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } وإلا إذا (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                                   = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        make_منزل_String(classp);
        make_منزل_String_درمیان(classp);
        ++m_stat_Class_منزل_String;
    }
    فراغ ویکھو(Ast_گرہ_UOr_Struct_DType* structp) override {
        إذا (structp->user1())
            أرجع;  // if to_string() was already emitted skip the rest of the function
        إذا (structp->emit_منزل_String()) ++m_emitForMembers;
        إذا (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_گرہ_DType* گرہ_Dtypep = memberp->dtypep();
                إذا (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                    = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } وإلا {
                    طالما (گرہ_Dtypep && گرہ_Dtypep->subDTypep()) {
                        گرہ_Dtypep = گرہ_Dtypep->subDTypep()->skipRefp();
                        إذا (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
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

            structp->user1(سچ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    فراغ ویکھو(AstIface* ifacep) override {
        إذا (ifacep->user1())
            أرجع;  // if to_string() was already emitted skip the rest of the function
        إذا (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            make_Vl_منزل_String(ifacep);
            ++m_stat_Interface_منزل_String;
            ifacep->user1(سچ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    فراغ ویکھو(AstClass* classp) override {
        إذا (classp->user1()) أرجع;
        إذا (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_جڑ);
            ++m_emit_Towards_جڑ;
            لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                إذا (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<صحيح>(Printed_From_ماپا::PRINTED_FROM_ماپا));
        } وإلا {
            Printed_From_ماپا printed_ماپا = static_cast<Printed_From_ماپا>(classp->user2());
            إذا (printed_ماپا != Printed_From_ماپا::UNRESOLVED && m_emit_Towards_جڑ == 0) {
                أرجع;
            }

            إذا (m_emit_Towards_جڑ > 0) markClassIterate(classp);

            منطقي is_جڑ = سچ;
            لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                إذا (extendp->classp()) {
                    إذا (static_cast<Printed_From_ماپا>(extendp->classp()->user2())
                            == Printed_From_ماپا::UNRESOLVED
                        || (m_emit_Towards_جڑ > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    إذا (printed_ماپا != Printed_From_ماپا::PRINTED_FROM_ماپا) {
                        printed_ماپا
                            = static_cast<Printed_From_ماپا>(extendp->classp()->user2());
                    }
                    is_جڑ = خطأ;
                }
            }

            إذا (is_جڑ) printed_ماپا = Printed_From_ماپا::NO_PRINTED_FROM_ماپا;

            classp->user2(static_cast<صحيح>(printed_ماپا));
            إذا (printed_ماپا == Printed_From_ماپا::PRINTED_FROM_ماپا) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_جڑ);
                ++m_emit_Towards_جڑ;
                لكل (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    إذا (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    واضح منزل_String_Visitor(Ast_گرہ* nodep) {
        إذا (v3Global.hasPrintedObjects()) {
            Ast_گرہ::user1_Clear_درخت();
            Ast_گرہ::user2_Clear_درخت();

            iterateConst(nodep);
        }

        V3Stats::جوڑو_Stat("Optimizations, Class ToString emitted", m_stat_Class_منزل_String);
        V3Stats::جوڑو_Stat("Optimizations, Interface ToString emitted", m_stat_Interface_منزل_String);
        V3Stats::جوڑو_Stat("Optimizations, Struct/union ToString emitted",
                         m_stat_Struct_Union_منزل_String);
    }
    ~منزل_String_Visitor() override = افتراضي;
};

//######################################################################
// V3Common class functions

فراغ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    منزل_String_Visitor{v3Global.rootp()};

    V3Global::dump_Check_Global_درخت("common", 0, dump_درخت_Either_Level() >= 3);
}
