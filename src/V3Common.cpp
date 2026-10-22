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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Common.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::make_ಗಮ್ಯ_String_Call(Ast_ಗಂಟು_DType* nodep, ಸ್ಥಿರಾಂಕ std::string& lhs) {
    std::string stmt;
    ಆದರೆ (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_ಪದಗಳು());
        stmt += ", ";
    } ಇಲ್ಲದಿದ್ದರೆ {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    ಹಿಂತಿರುಗಿ stmt;
}

ಸ್ಥಿರ ಶೂನ್ಯ make_Vl_ಗಮ್ಯ_String(AstIface* nodep) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp
        = ಹೊಸ AstCFunc{nodep->fileline(), "VL_TO_STRING", ಶೂನ್ಯಸೂಚಕ, "std::string"};
    funcp->argTypes("const " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep) + "* obj");
    funcp->is_ವಿಧಾನ(ಸುಳ್ಳು);
    funcp->isConst(ಸುಳ್ಳು);
    funcp->isStatic(ಸುಳ್ಳು);
    funcp->protect(ಸುಳ್ಳು);
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtype_ಗಣ_String();
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{nodep->fileline(), exprp});
    nodep->ಕೂಡಿಸು_Stmtsp(funcp);
}
ಸ್ಥಿರ ಶೂನ್ಯ make_Vl_ಗಮ್ಯ_String(Ast_ಗಂಟು_UOr_Struct_DType* nodep) {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp
        = ಹೊಸ AstCFunc{nodep->fileline(), "VL_TO_STRING", ಶೂನ್ಯಸೂಚಕ, "std::string"};
    funcp->argTypes("const " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep) + "& obj");
    funcp->is_ವಿಧಾನ(ಸುಳ್ಳು);
    funcp->isConst(ಸುಳ್ಳು);
    funcp->isStatic(ಸುಳ್ಳು);
    funcp->protect(ಸುಳ್ಳು);
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{nodep->fileline(), "std::string out;"});
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        ಆದರೆ (itemp == nodep->membersp()) {
            stmt += "'{";
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::make_ಗಮ್ಯ_String_Call(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{nodep->fileline(), stmt});
    }
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstCExpr{nodep->fileline(), "out"};
    exprp->dtype_ಗಣ_String();
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{nodep->fileline(), exprp});

    modp->ಕೂಡಿಸು_Stmtsp(funcp);
}
ಸ್ಥಿರ ಶೂನ್ಯ make_ಗಮ್ಯ_String(AstClass* nodep) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{nodep->fileline(), "to_string", ಶೂನ್ಯಸೂಚಕ, "std::string"};
    funcp->isConst(ನಿಜ);
    funcp->isStatic(ಸುಳ್ಳು);
    funcp->isVirtual(ನಿಜ);
    funcp->protect(ಸುಳ್ಳು);
    AstCExpr* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtype_ಗಣ_String();
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{nodep->fileline(), exprp});
    nodep->ಕೂಡಿಸು_Stmtsp(funcp);
}
ಸ್ಥಿರ ಶೂನ್ಯ make_ಗಮ್ಯ_String_ಮಧ್ಯ(AstClass* nodep) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp
        = ಹೊಸ AstCFunc{nodep->fileline(), "to_string_middle", ಶೂನ್ಯಸೂಚಕ, "std::string"};
    funcp->isConst(ನಿಜ);
    funcp->isStatic(ಸುಳ್ಳು);
    funcp->protect(ಸುಳ್ಳು);
    Ast_ಗಂಟು_Stmt* stmtsp = ಶೂನ್ಯಸೂಚಕ;
    std::string comma;
    ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ varp = VN_CAST(itemp, Var)) {
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = varp->dtypeSkipRefp()->basicp();
            ಆದರೆ (!varp->isParam() && !varp->is_ಆಂತರಿಕ()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::make_ಗಮ್ಯ_String_Call(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(ನಿಜ);  // So what we extend dumps this
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, ಹೊಸ AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    ಆದರೆ (nodep->extendsp()) {
        string stmt = "out += ";
        ಆದರೆ (!comma.ಖಾಲಿಗಳು()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(ನಿಜ);  // So what we extend dumps this
        stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, ಹೊಸ AstCStmt{nodep->fileline(), stmt});
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;
    ಆದರೆ (stmtsp) {
        funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->ಕೂಡಿಸು_Stmtsp(stmtsp);
        exprp = ಹೊಸ AstCExpr{nodep->fileline(), "out"};
        exprp->dtype_ಗಣ_String();
    } ಇಲ್ಲದಿದ್ದರೆ {  // Nothing to print, return ""
        exprp = ಹೊಸ AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{nodep->fileline(), exprp});

    nodep->ಕೂಡಿಸು_Stmtsp(funcp);
}

ವರ್ಗ ಗಮ್ಯ_String_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಗಾತ್ರ_t m_emit_Towards_ಬೇರು
        = 0;  // class that require its ancestors to emit to_string() sets this
    ಗಾತ್ರ_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    ಎಣಿಕೆ ವರ್ಗ Printed_From_ಪೋಷಕ : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_ಪೋಷಕ,
        NO_PRINTED_FROM_ಪೋಷಕ
    };

    VDouble0 m_stat_Class_ಗಮ್ಯ_String;
    VDouble0 m_stat_Interface_ಗಮ್ಯ_String;
    VDouble0 m_stat_Struct_Union_ಗಮ್ಯ_String;

    ಶೂನ್ಯ markClassIterate(AstClass* classp) {
        ಆದರೆ (classp->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                Ast_ಗಂಟು_DType* ಗಂಟು_Dtypep = varp->dtypep();
                ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRedDTypep = VN_CAST(ಗಂಟು_Dtypep, IfaceRefDType)) {
                    ಆದರೆ (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆಗ (ಗಂಟು_Dtypep && ಗಂಟು_Dtypep->subDTypep()
                           && ಗಂಟು_Dtypep->subDTypep()->skipRefp()) {
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->subDTypep()->skipRefp();
                        ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRedDTypep
                            = VN_CAST(ಗಂಟು_Dtypep, IfaceRefDType)) {
                            ಆದರೆ (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ uOrStructDTypep
                                   = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        make_ಗಮ್ಯ_String(classp);
        make_ಗಮ್ಯ_String_ಮಧ್ಯ(classp);
        ++m_stat_Class_ಗಮ್ಯ_String;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* structp) override {
        ಆದರೆ (structp->user1())
            ಹಿಂತಿರುಗಿ;  // if to_string() was already emitted skip the rest of the function
        ಆದರೆ (structp->emit_ಗಮ್ಯ_String()) ++m_emitForMembers;
        ಆದರೆ (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            ಗಾಗಿ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_DType* ಗಂಟು_Dtypep = memberp->dtypep();
                ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ uOrStructDTypep
                    = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆಗ (ಗಂಟು_Dtypep && ಗಂಟು_Dtypep->subDTypep()) {
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->subDTypep()->skipRefp();
                        ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ uOrStructDTypep
                            = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            ಆದರೆ (structp->emit_ಗಮ್ಯ_String()) --m_emitForMembers;

            ಆದರೆ (!structp->packed()) {
                make_Vl_ಗಮ್ಯ_String(structp);
                ++m_stat_Struct_Union_ಗಮ್ಯ_String;
            }

            structp->user1(ನಿಜ);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIface* ifacep) override {
        ಆದರೆ (ifacep->user1())
            ಹಿಂತಿರುಗಿ;  // if to_string() was already emitted skip the rest of the function
        ಆದರೆ (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            make_Vl_ಗಮ್ಯ_String(ifacep);
            ++m_stat_Interface_ಗಮ್ಯ_String;
            ifacep->user1(ನಿಜ);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* classp) override {
        ಆದರೆ (classp->user1()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_ಬೇರು);
            ++m_emit_Towards_ಬೇರು;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ಆದರೆ (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<ಪೂರ್ಣಾಂಕ>(Printed_From_ಪೋಷಕ::PRINTED_FROM_ಪೋಷಕ));
        } ಇಲ್ಲದಿದ್ದರೆ {
            Printed_From_ಪೋಷಕ printed_ಪೋಷಕ = static_cast<Printed_From_ಪೋಷಕ>(classp->user2());
            ಆದರೆ (printed_ಪೋಷಕ != Printed_From_ಪೋಷಕ::UNRESOLVED && m_emit_Towards_ಬೇರು == 0) {
                ಹಿಂತಿರುಗಿ;
            }

            ಆದರೆ (m_emit_Towards_ಬೇರು > 0) markClassIterate(classp);

            ತಾರ್ಕಿಕ is_ಬೇರು = ನಿಜ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ಆದರೆ (extendp->classp()) {
                    ಆದರೆ (static_cast<Printed_From_ಪೋಷಕ>(extendp->classp()->user2())
                            == Printed_From_ಪೋಷಕ::UNRESOLVED
                        || (m_emit_Towards_ಬೇರು > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    ಆದರೆ (printed_ಪೋಷಕ != Printed_From_ಪೋಷಕ::PRINTED_FROM_ಪೋಷಕ) {
                        printed_ಪೋಷಕ
                            = static_cast<Printed_From_ಪೋಷಕ>(extendp->classp()->user2());
                    }
                    is_ಬೇರು = ಸುಳ್ಳು;
                }
            }

            ಆದರೆ (is_ಬೇರು) printed_ಪೋಷಕ = Printed_From_ಪೋಷಕ::NO_PRINTED_FROM_ಪೋಷಕ;

            classp->user2(static_cast<ಪೂರ್ಣಾಂಕ>(printed_ಪೋಷಕ));
            ಆದರೆ (printed_ಪೋಷಕ == Printed_From_ಪೋಷಕ::PRINTED_FROM_ಪೋಷಕ) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_ಬೇರು);
                ++m_emit_Towards_ಬೇರು;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    ಆದರೆ (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ಗಮ್ಯ_String_Visitor(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (v3Global.hasPrintedObjects()) {
            Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
            Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();

            iterateConst(nodep);
        }

        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Class ToString emitted", m_stat_Class_ಗಮ್ಯ_String);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Interface ToString emitted", m_stat_Interface_ಗಮ್ಯ_String);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Struct/union ToString emitted",
                         m_stat_Struct_Union_ಗಮ್ಯ_String);
    }
    ~ಗಮ್ಯ_String_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// V3Common class functions

ಶೂನ್ಯ V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    ಗಮ್ಯ_String_Visitor{v3Global.rootp()};

    V3Global::dump_Check_Global_ವೃಕ್ಷ("common", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
