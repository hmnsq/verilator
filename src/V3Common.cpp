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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Common.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::make_ਮੰਜ਼ਿਲ_String_Call(Ast_ਗੰਢ_DType* nodep, ਅਚਲ std::string& lhs) {
    std::string stmt;
    ਜੇ (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_ਸ਼ਬਦ());
        stmt += ", ";
    } ਨਹੀਂਤਾਂ {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    ਵਾਪਸ stmt;
}

ਸਥਿਰ ਖਾਲੀ make_Vl_ਮੰਜ਼ਿਲ_String(AstIface* nodep) {
    AstCFunc* ਅਚਲ funcp
        = ਨਵਾਂ AstCFunc{nodep->fileline(), "VL_TO_STRING", ਖਾਲੀ_ਸੂਚਕ, "std::string"};
    funcp->argTypes("const " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep) + "* obj");
    funcp->is_ਵਿਧੀ(ਝੂਠ);
    funcp->isConst(ਝੂਠ);
    funcp->isStatic(ਝੂਠ);
    funcp->protect(ਝੂਠ);
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = ਨਵਾਂ AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtype_ਸੈੱਟ_String();
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{nodep->fileline(), exprp});
    nodep->ਜੋੜੋ_Stmtsp(funcp);
}
ਸਥਿਰ ਖਾਲੀ make_Vl_ਮੰਜ਼ਿਲ_String(Ast_ਗੰਢ_UOr_Struct_DType* nodep) {
    Ast_ਗੰਢ_Module* ਅਚਲ modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ਅਚਲ funcp
        = ਨਵਾਂ AstCFunc{nodep->fileline(), "VL_TO_STRING", ਖਾਲੀ_ਸੂਚਕ, "std::string"};
    funcp->argTypes("const " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep) + "& obj");
    funcp->is_ਵਿਧੀ(ਝੂਠ);
    funcp->isConst(ਝੂਠ);
    funcp->isStatic(ਝੂਠ);
    funcp->protect(ਝੂਠ);
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{nodep->fileline(), "std::string out;"});
    ਲਈ (ਅਚਲ AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        ਜੇ (itemp == nodep->membersp()) {
            stmt += "'{";
        } ਨਹੀਂਤਾਂ {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::make_ਮੰਜ਼ਿਲ_String_Call(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{nodep->fileline(), stmt});
    }
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ਅਚਲ exprp = ਨਵਾਂ AstCExpr{nodep->fileline(), "out"};
    exprp->dtype_ਸੈੱਟ_String();
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{nodep->fileline(), exprp});

    modp->ਜੋੜੋ_Stmtsp(funcp);
}
ਸਥਿਰ ਖਾਲੀ make_ਮੰਜ਼ਿਲ_String(AstClass* nodep) {
    AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{nodep->fileline(), "to_string", ਖਾਲੀ_ਸੂਚਕ, "std::string"};
    funcp->isConst(ਸੱਚ);
    funcp->isStatic(ਝੂਠ);
    funcp->isVirtual(ਸੱਚ);
    funcp->protect(ਝੂਠ);
    AstCExpr* ਅਚਲ exprp = ਨਵਾਂ AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtype_ਸੈੱਟ_String();
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{nodep->fileline(), exprp});
    nodep->ਜੋੜੋ_Stmtsp(funcp);
}
ਸਥਿਰ ਖਾਲੀ make_ਮੰਜ਼ਿਲ_String_ਵਿਚਕਾਰ(AstClass* nodep) {
    AstCFunc* ਅਚਲ funcp
        = ਨਵਾਂ AstCFunc{nodep->fileline(), "to_string_middle", ਖਾਲੀ_ਸੂਚਕ, "std::string"};
    funcp->isConst(ਸੱਚ);
    funcp->isStatic(ਝੂਠ);
    funcp->protect(ਝੂਠ);
    Ast_ਗੰਢ_Stmt* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
    std::string comma;
    ਲਈ (Ast_ਗੰਢ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ varp = VN_CAST(itemp, Var)) {
            ਅਚਲ AstBasicDType* ਅਚਲ basicp = varp->dtypeSkipRefp()->basicp();
            ਜੇ (!varp->isParam() && !varp->is_ਅੰਦਰੂਨੀ()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::make_ਮੰਜ਼ਿਲ_String_Call(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(ਸੱਚ);  // So what we extend dumps this
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, ਨਵਾਂ AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    ਜੇ (nodep->extendsp()) {
        string stmt = "out += ";
        ਜੇ (!comma.ਖਾਲੀ_ਥਾਵਾਂ()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(ਸੱਚ);  // So what we extend dumps this
        stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, ਨਵਾਂ AstCStmt{nodep->fileline(), stmt});
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp;
    ਜੇ (stmtsp) {
        funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->ਜੋੜੋ_Stmtsp(stmtsp);
        exprp = ਨਵਾਂ AstCExpr{nodep->fileline(), "out"};
        exprp->dtype_ਸੈੱਟ_String();
    } ਨਹੀਂਤਾਂ {  // Nothing to print, return ""
        exprp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{nodep->fileline(), exprp});

    nodep->ਜੋੜੋ_Stmtsp(funcp);
}

ਵਰਗ ਮੰਜ਼ਿਲ_String_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਮਾਪ_t m_emit_Towards_ਜੜ੍ਹ
        = 0;  // class that require its ancestors to emit to_string() sets this
    ਮਾਪ_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    ਗਿਣਤੀ ਵਰਗ Printed_From_ਮਾਪਾ : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_ਮਾਪਾ,
        NO_PRINTED_FROM_ਮਾਪਾ
    };

    VDouble0 m_stat_Class_ਮੰਜ਼ਿਲ_String;
    VDouble0 m_stat_Interface_ਮੰਜ਼ਿਲ_String;
    VDouble0 m_stat_Struct_Union_ਮੰਜ਼ਿਲ_String;

    ਖਾਲੀ markClassIterate(AstClass* classp) {
        ਜੇ (classp->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        ਲਈ (Ast_ਗੰਢ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                Ast_ਗੰਢ_DType* ਗੰਢ_Dtypep = varp->dtypep();
                ਜੇ (AstIfaceRefDType* ਅਚਲ ifaceRedDTypep = VN_CAST(ਗੰਢ_Dtypep, IfaceRefDType)) {
                    ਜੇ (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } ਨਹੀਂਤਾਂ {
                    ਜਦੋਂ (ਗੰਢ_Dtypep && ਗੰਢ_Dtypep->subDTypep()
                           && ਗੰਢ_Dtypep->subDTypep()->skipRefp()) {
                        ਗੰਢ_Dtypep = ਗੰਢ_Dtypep->subDTypep()->skipRefp();
                        ਜੇ (AstIfaceRefDType* ਅਚਲ ifaceRedDTypep
                            = VN_CAST(ਗੰਢ_Dtypep, IfaceRefDType)) {
                            ਜੇ (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ uOrStructDTypep
                                   = VN_CAST(ਗੰਢ_Dtypep, ਗੰਢ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        make_ਮੰਜ਼ਿਲ_String(classp);
        make_ਮੰਜ਼ਿਲ_String_ਵਿਚਕਾਰ(classp);
        ++m_stat_Class_ਮੰਜ਼ਿਲ_String;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* structp) override {
        ਜੇ (structp->user1())
            ਵਾਪਸ;  // if to_string() was already emitted skip the rest of the function
        ਜੇ (structp->emit_ਮੰਜ਼ਿਲ_String()) ++m_emitForMembers;
        ਜੇ (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            ਲਈ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ਗੰਢ_DType* ਗੰਢ_Dtypep = memberp->dtypep();
                ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ uOrStructDTypep
                    = VN_CAST(ਗੰਢ_Dtypep, ਗੰਢ_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } ਨਹੀਂਤਾਂ {
                    ਜਦੋਂ (ਗੰਢ_Dtypep && ਗੰਢ_Dtypep->subDTypep()) {
                        ਗੰਢ_Dtypep = ਗੰਢ_Dtypep->subDTypep()->skipRefp();
                        ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ uOrStructDTypep
                            = VN_CAST(ਗੰਢ_Dtypep, ਗੰਢ_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            ਜੇ (structp->emit_ਮੰਜ਼ਿਲ_String()) --m_emitForMembers;

            ਜੇ (!structp->packed()) {
                make_Vl_ਮੰਜ਼ਿਲ_String(structp);
                ++m_stat_Struct_Union_ਮੰਜ਼ਿਲ_String;
            }

            structp->user1(ਸੱਚ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstIface* ifacep) override {
        ਜੇ (ifacep->user1())
            ਵਾਪਸ;  // if to_string() was already emitted skip the rest of the function
        ਜੇ (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            make_Vl_ਮੰਜ਼ਿਲ_String(ifacep);
            ++m_stat_Interface_ਮੰਜ਼ਿਲ_String;
            ifacep->user1(ਸੱਚ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    ਖਾਲੀ ਦੇਖੋ(AstClass* classp) override {
        ਜੇ (classp->user1()) ਵਾਪਸ;
        ਜੇ (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_ਜੜ੍ਹ);
            ++m_emit_Towards_ਜੜ੍ਹ;
            ਲਈ (ਅਚਲ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ਜੇ (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<ਪੂਰਨ>(Printed_From_ਮਾਪਾ::PRINTED_FROM_ਮਾਪਾ));
        } ਨਹੀਂਤਾਂ {
            Printed_From_ਮਾਪਾ printed_ਮਾਪਾ = static_cast<Printed_From_ਮਾਪਾ>(classp->user2());
            ਜੇ (printed_ਮਾਪਾ != Printed_From_ਮਾਪਾ::UNRESOLVED && m_emit_Towards_ਜੜ੍ਹ == 0) {
                ਵਾਪਸ;
            }

            ਜੇ (m_emit_Towards_ਜੜ੍ਹ > 0) markClassIterate(classp);

            ਤਾਰਕਿਕ is_ਜੜ੍ਹ = ਸੱਚ;
            ਲਈ (ਅਚਲ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ਜੇ (extendp->classp()) {
                    ਜੇ (static_cast<Printed_From_ਮਾਪਾ>(extendp->classp()->user2())
                            == Printed_From_ਮਾਪਾ::UNRESOLVED
                        || (m_emit_Towards_ਜੜ੍ਹ > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    ਜੇ (printed_ਮਾਪਾ != Printed_From_ਮਾਪਾ::PRINTED_FROM_ਮਾਪਾ) {
                        printed_ਮਾਪਾ
                            = static_cast<Printed_From_ਮਾਪਾ>(extendp->classp()->user2());
                    }
                    is_ਜੜ੍ਹ = ਝੂਠ;
                }
            }

            ਜੇ (is_ਜੜ੍ਹ) printed_ਮਾਪਾ = Printed_From_ਮਾਪਾ::NO_PRINTED_FROM_ਮਾਪਾ;

            classp->user2(static_cast<ਪੂਰਨ>(printed_ਮਾਪਾ));
            ਜੇ (printed_ਮਾਪਾ == Printed_From_ਮਾਪਾ::PRINTED_FROM_ਮਾਪਾ) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_ਜੜ੍ਹ);
                ++m_emit_Towards_ਜੜ੍ਹ;
                ਲਈ (ਅਚਲ AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    ਜੇ (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ ਮੰਜ਼ਿਲ_String_Visitor(Ast_ਗੰਢ* nodep) {
        ਜੇ (v3Global.hasPrintedObjects()) {
            Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
            Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();

            iterateConst(nodep);
        }

        V3Stats::ਜੋੜੋ_Stat("Optimizations, Class ToString emitted", m_stat_Class_ਮੰਜ਼ਿਲ_String);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Interface ToString emitted", m_stat_Interface_ਮੰਜ਼ਿਲ_String);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Struct/union ToString emitted",
                         m_stat_Struct_Union_ਮੰਜ਼ਿਲ_String);
    }
    ~ਮੰਜ਼ਿਲ_String_Visitor() override = ਮੂਲ;
};

//######################################################################
// V3Common class functions

ਖਾਲੀ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ਮੰਜ਼ਿਲ_String_Visitor{v3Global.rootp()};

    V3Global::dump_Check_Global_ਰੁੱਖ("common", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
