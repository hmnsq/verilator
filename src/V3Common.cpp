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

string V3Common::make_مقصد_String_Call(Ast_گرہ_DType* nodep, ثابت std::string& lhs) {
    std::string stmt;
    اگر (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvt_مقصد_Str(nodep->چوڑائی_الفاظ());
        stmt += ", ";
    } ورنہ {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    واپس stmt;
}

ساکن خالی make_Vl_مقصد_String(AstIface* nodep) {
    AstCFunc* ثابت funcp
        = نیا AstCFunc{nodep->fileline(), "VL_TO_STRING", خالی_اشاریہ, "std::string"};
    funcp->argTypes("const " + EmitCUtil::سابقہ_Name_Protect(nodep) + "* obj");
    funcp->is_طریقہ(جھوٹ);
    funcp->isConst(جھوٹ);
    funcp->isStatic(جھوٹ);
    funcp->protect(جھوٹ);
    Ast_گرہ_اظہاریہ* ثابت exprp = نیا AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtype_سیٹ_String();
    funcp->جوڑو_Stmtsp(نیا AstCReturn{nodep->fileline(), exprp});
    nodep->جوڑو_Stmtsp(funcp);
}
ساکن خالی make_Vl_مقصد_String(Ast_گرہ_UOr_Struct_DType* nodep) {
    Ast_گرہ_Module* ثابت modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ثابت funcp
        = نیا AstCFunc{nodep->fileline(), "VL_TO_STRING", خالی_اشاریہ, "std::string"};
    funcp->argTypes("const " + EmitCUtil::سابقہ_Name_Protect(nodep) + "& obj");
    funcp->is_طریقہ(جھوٹ);
    funcp->isConst(جھوٹ);
    funcp->isStatic(جھوٹ);
    funcp->protect(جھوٹ);
    funcp->جوڑو_Stmtsp(نیا AstCStmt{nodep->fileline(), "std::string out;"});
    برائے (ثابت AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        اگر (itemp == nodep->membersp()) {
            stmt += "'{";
        } ورنہ {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::make_مقصد_String_Call(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->جوڑو_Stmtsp(نیا AstCStmt{nodep->fileline(), stmt});
    }
    funcp->جوڑو_Stmtsp(نیا AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ثابت exprp = نیا AstCExpr{nodep->fileline(), "out"};
    exprp->dtype_سیٹ_String();
    funcp->جوڑو_Stmtsp(نیا AstCReturn{nodep->fileline(), exprp});

    modp->جوڑو_Stmtsp(funcp);
}
ساکن خالی make_مقصد_String(AstClass* nodep) {
    AstCFunc* ثابت funcp = نیا AstCFunc{nodep->fileline(), "to_string", خالی_اشاریہ, "std::string"};
    funcp->isConst(سچ);
    funcp->isStatic(جھوٹ);
    funcp->isVirtual(سچ);
    funcp->protect(جھوٹ);
    AstCExpr* ثابت exprp = نیا AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtype_سیٹ_String();
    funcp->جوڑو_Stmtsp(نیا AstCReturn{nodep->fileline(), exprp});
    nodep->جوڑو_Stmtsp(funcp);
}
ساکن خالی make_مقصد_String_درمیان(AstClass* nodep) {
    AstCFunc* ثابت funcp
        = نیا AstCFunc{nodep->fileline(), "to_string_middle", خالی_اشاریہ, "std::string"};
    funcp->isConst(سچ);
    funcp->isStatic(جھوٹ);
    funcp->protect(جھوٹ);
    Ast_گرہ_Stmt* stmtsp = خالی_اشاریہ;
    std::string comma;
    برائے (Ast_گرہ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        اگر (ثابت خودکار* ثابت varp = VN_CAST(itemp, Var)) {
            ثابت AstBasicDType* ثابت basicp = varp->dtypeSkipRefp()->basicp();
            اگر (!varp->isParam() && !varp->is_اندرونی()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::make_مقصد_String_Call(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(سچ);  // So what we extend dumps this
                stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, نیا AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    اگر (nodep->extendsp()) {
        string stmt = "out += ";
        اگر (!comma.خالی_خانے()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::سابقہ_Name_Protect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(سچ);  // So what we extend dumps this
        stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, نیا AstCStmt{nodep->fileline(), stmt});
    }

    Ast_گرہ_اظہاریہ* exprp;
    اگر (stmtsp) {
        funcp->جوڑو_Stmtsp(نیا AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->جوڑو_Stmtsp(stmtsp);
        exprp = نیا AstCExpr{nodep->fileline(), "out"};
        exprp->dtype_سیٹ_String();
    } ورنہ {  // Nothing to print, return ""
        exprp = نیا AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->جوڑو_Stmtsp(نیا AstCReturn{nodep->fileline(), exprp});

    nodep->جوڑو_Stmtsp(funcp);
}

جماعت مقصد_String_Visitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    سائز_t m_emit_Towards_جڑ
        = 0;  // class that require its ancestors to emit to_string() sets this
    سائز_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    شمار جماعت Printed_From_والد : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_والد,
        NO_PRINTED_FROM_والد
    };

    VDouble0 m_stat_Class_مقصد_String;
    VDouble0 m_stat_Interface_مقصد_String;
    VDouble0 m_stat_Struct_Union_مقصد_String;

    خالی markClassIterate(AstClass* classp) {
        اگر (classp->user1_سیٹ_Once()) واپس;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        برائے (Ast_گرہ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                Ast_گرہ_DType* گرہ_Dtypep = varp->dtypep();
                اگر (AstIfaceRefDType* ثابت ifaceRedDTypep = VN_CAST(گرہ_Dtypep, IfaceRefDType)) {
                    اگر (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } ورنہ {
                    جب (گرہ_Dtypep && گرہ_Dtypep->subDTypep()
                           && گرہ_Dtypep->subDTypep()->skipRefp()) {
                        گرہ_Dtypep = گرہ_Dtypep->subDTypep()->skipRefp();
                        اگر (AstIfaceRefDType* ثابت ifaceRedDTypep
                            = VN_CAST(گرہ_Dtypep, IfaceRefDType)) {
                            اگر (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } ورنہ اگر (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                                   = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        make_مقصد_String(classp);
        make_مقصد_String_درمیان(classp);
        ++m_stat_Class_مقصد_String;
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* structp) override {
        اگر (structp->user1())
            واپس;  // if to_string() was already emitted skip the rest of the function
        اگر (structp->emit_مقصد_String()) ++m_emitForMembers;
        اگر (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            برائے (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_گرہ_DType* گرہ_Dtypep = memberp->dtypep();
                اگر (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                    = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } ورنہ {
                    جب (گرہ_Dtypep && گرہ_Dtypep->subDTypep()) {
                        گرہ_Dtypep = گرہ_Dtypep->subDTypep()->skipRefp();
                        اگر (Ast_گرہ_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گرہ_Dtypep, گرہ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            اگر (structp->emit_مقصد_String()) --m_emitForMembers;

            اگر (!structp->packed()) {
                make_Vl_مقصد_String(structp);
                ++m_stat_Struct_Union_مقصد_String;
            }

            structp->user1(سچ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    خالی دورہ(AstIface* ifacep) override {
        اگر (ifacep->user1())
            واپس;  // if to_string() was already emitted skip the rest of the function
        اگر (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            make_Vl_مقصد_String(ifacep);
            ++m_stat_Interface_مقصد_String;
            ifacep->user1(سچ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    خالی دورہ(AstClass* classp) override {
        اگر (classp->user1()) واپس;
        اگر (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_جڑ);
            ++m_emit_Towards_جڑ;
            برائے (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                اگر (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<صحیح>(Printed_From_والد::PRINTED_FROM_والد));
        } ورنہ {
            Printed_From_والد printed_والد = static_cast<Printed_From_والد>(classp->user2());
            اگر (printed_والد != Printed_From_والد::UNRESOLVED && m_emit_Towards_جڑ == 0) {
                واپس;
            }

            اگر (m_emit_Towards_جڑ > 0) markClassIterate(classp);

            منطقی is_جڑ = سچ;
            برائے (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                اگر (extendp->classp()) {
                    اگر (static_cast<Printed_From_والد>(extendp->classp()->user2())
                            == Printed_From_والد::UNRESOLVED
                        || (m_emit_Towards_جڑ > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    اگر (printed_والد != Printed_From_والد::PRINTED_FROM_والد) {
                        printed_والد
                            = static_cast<Printed_From_والد>(extendp->classp()->user2());
                    }
                    is_جڑ = جھوٹ;
                }
            }

            اگر (is_جڑ) printed_والد = Printed_From_والد::NO_PRINTED_FROM_والد;

            classp->user2(static_cast<صحیح>(printed_والد));
            اگر (printed_والد == Printed_From_والد::PRINTED_FROM_والد) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_جڑ);
                ++m_emit_Towards_جڑ;
                برائے (ثابت AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    اگر (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    صریح مقصد_String_Visitor(Ast_گرہ* nodep) {
        اگر (v3Global.hasPrintedObjects()) {
            Ast_گرہ::user1_Clear_درخت();
            Ast_گرہ::user2_Clear_درخت();

            iterateConst(nodep);
        }

        V3Stats::جوڑو_Stat("Optimizations, Class ToString emitted", m_stat_Class_مقصد_String);
        V3Stats::جوڑو_Stat("Optimizations, Interface ToString emitted", m_stat_Interface_مقصد_String);
        V3Stats::جوڑو_Stat("Optimizations, Struct/union ToString emitted",
                         m_stat_Struct_Union_مقصد_String);
    }
    ~مقصد_String_Visitor() override = طےشدہ;
};

//######################################################################
// V3Common class functions

خالی V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    مقصد_String_Visitor{v3Global.rootp()};

    V3Global::dump_Check_Global_درخت("common", 0, dump_درخت_Either_Level() >= 3);
}
