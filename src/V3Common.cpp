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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Common.h"

#అన్తర్భావ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Common component builders

string V3Common::make_గమ్యం_String_Call(Ast_కణుపు_DType* nodep, స్థిరరాశి std::string& lhs) {
    std::string stmt;
    ఒకవేళ (VN_IS(nodep->skipRefp(), BasicDType) && nodep->isWide()) {
        stmt += "VL_TO_STRING_W(";
        stmt += cvt_గమ్యం_Str(nodep->వెడల్పు_పదాలు());
        stmt += ", ";
    } లేకపోతే {
        stmt += "VL_TO_STRING(";
    }
    stmt += lhs;
    stmt += ")";
    తిరిగి stmt;
}

స్థిర శూన్య make_Vl_గమ్యం_String(AstIface* nodep) {
    AstCFunc* స్థిరరాశి funcp
        = కొత్త AstCFunc{nodep->fileline(), "VL_TO_STRING", శూన్యసూచిక, "std::string"};
    funcp->argTypes("const " + EmitCUtil::ఉపసర్గ_Name_Protect(nodep) + "* obj");
    funcp->is_పద్ధతి(అబద్ధం);
    funcp->isConst(అబద్ధం);
    funcp->isStatic(అబద్ధం);
    funcp->protect(అబద్ధం);
    Ast_కణుపు_సమాసం* స్థిరరాశి exprp = కొత్త AstCExpr{nodep->fileline(), "obj ? obj->vlNamep : \"null\""};
    exprp->dtype_సమితి_String();
    funcp->కూడు_Stmtsp(కొత్త AstCReturn{nodep->fileline(), exprp});
    nodep->కూడు_Stmtsp(funcp);
}
స్థిర శూన్య make_Vl_గమ్యం_String(Ast_కణుపు_UOr_Struct_DType* nodep) {
    Ast_కణుపు_Module* స్థిరరాశి modp = nodep->classOrPackagep();
    UASSERT_OBJ(modp, nodep, "Unlinked struct package");
    AstCFunc* స్థిరరాశి funcp
        = కొత్త AstCFunc{nodep->fileline(), "VL_TO_STRING", శూన్యసూచిక, "std::string"};
    funcp->argTypes("const " + EmitCUtil::ఉపసర్గ_Name_Protect(nodep) + "& obj");
    funcp->is_పద్ధతి(అబద్ధం);
    funcp->isConst(అబద్ధం);
    funcp->isStatic(అబద్ధం);
    funcp->protect(అబద్ధం);
    funcp->కూడు_Stmtsp(కొత్త AstCStmt{nodep->fileline(), "std::string out;"});
    కొరకు (స్థిరరాశి AstMemberDType* itemp = nodep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        std::string stmt = "out += \"";
        ఒకవేళ (itemp == nodep->membersp()) {
            stmt += "'{";
        } లేకపోతే {
            stmt += ", ";
        }
        stmt += VIdProtect::protect(itemp->prettyName()) + ":\" + ";
        stmt += V3Common::make_గమ్యం_String_Call(itemp->dtypep(), "obj."s + itemp->nameProtect());
        stmt += ";";
        funcp->కూడు_Stmtsp(కొత్త AstCStmt{nodep->fileline(), stmt});
    }
    funcp->కూడు_Stmtsp(కొత్త AstCStmt{nodep->fileline(), "out += \"}\";"});

    AstCExpr* స్థిరరాశి exprp = కొత్త AstCExpr{nodep->fileline(), "out"};
    exprp->dtype_సమితి_String();
    funcp->కూడు_Stmtsp(కొత్త AstCReturn{nodep->fileline(), exprp});

    modp->కూడు_Stmtsp(funcp);
}
స్థిర శూన్య make_గమ్యం_String(AstClass* nodep) {
    AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{nodep->fileline(), "to_string", శూన్యసూచిక, "std::string"};
    funcp->isConst(నిజం);
    funcp->isStatic(అబద్ధం);
    funcp->isVirtual(నిజం);
    funcp->protect(అబద్ధం);
    AstCExpr* స్థిరరాశి exprp = కొత్త AstCExpr{nodep->fileline(), R"("'{"s + to_string_middle() + "}")"};
    exprp->dtype_సమితి_String();
    funcp->కూడు_Stmtsp(కొత్త AstCReturn{nodep->fileline(), exprp});
    nodep->కూడు_Stmtsp(funcp);
}
స్థిర శూన్య make_గమ్యం_String_మధ్య(AstClass* nodep) {
    AstCFunc* స్థిరరాశి funcp
        = కొత్త AstCFunc{nodep->fileline(), "to_string_middle", శూన్యసూచిక, "std::string"};
    funcp->isConst(నిజం);
    funcp->isStatic(అబద్ధం);
    funcp->protect(అబద్ధం);
    Ast_కణుపు_Stmt* stmtsp = శూన్యసూచిక;
    std::string comma;
    కొరకు (Ast_కణుపు* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి varp = VN_CAST(itemp, Var)) {
            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = varp->dtypeSkipRefp()->basicp();
            ఒకవేళ (!varp->isParam() && !varp->is_అంతర్గత()
                && !(basicp && (basicp->isRandomGenerator() || basicp->isStdRandomGenerator()))) {
                string stmt = "out += \"";
                stmt += comma;
                comma = ", ";
                stmt += itemp->origNameProtect();
                stmt += ":\" + ";
                stmt += V3Common::make_గమ్యం_String_Call(itemp->dtypep(), itemp->nameProtect());
                stmt += ";";
                nodep->user1(నిజం);  // So what we extend dumps this
                stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, కొత్త AstCStmt{nodep->fileline(), stmt});
            }
        }
    }
    ఒకవేళ (nodep->extendsp()) {
        string stmt = "out += ";
        ఒకవేళ (!comma.ఖాళీలు()) stmt += "\", \" + ";
        // comma = ", ";  // Nothing further so not needed
        stmt += EmitCUtil::ఉపసర్గ_Name_Protect(nodep->extendsp()->dtypep());
        stmt += "::to_string_middle();";
        nodep->user1(నిజం);  // So what we extend dumps this
        stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, కొత్త AstCStmt{nodep->fileline(), stmt});
    }

    Ast_కణుపు_సమాసం* exprp;
    ఒకవేళ (stmtsp) {
        funcp->కూడు_Stmtsp(కొత్త AstCStmt{nodep->fileline(), "std::string out;"});
        funcp->కూడు_Stmtsp(stmtsp);
        exprp = కొత్త AstCExpr{nodep->fileline(), "out"};
        exprp->dtype_సమితి_String();
    } లేకపోతే {  // Nothing to print, return ""
        exprp = కొత్త AstConst{nodep->fileline(), AstConst::String{}, ""};
    }
    funcp->కూడు_Stmtsp(కొత్త AstCReturn{nodep->fileline(), exprp});

    nodep->కూడు_Stmtsp(funcp);
}

తరగతి గమ్యం_String_Visitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()  -> bool. to_string() was emitted
    //  AstClass::user2()  -> PrintedFromParent. Used to propagate emission of to_string() from
    //  parent to child
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    కొలత_t m_emit_Towards_మూలం
        = 0;  // class that require its ancestors to emit to_string() sets this
    కొలత_t m_emitForMembers = 0;  // class/struct that requires its members (and their members
                                  // recursively) to emit to_string() sets this
    గణన తరగతి Printed_From_మాతృ : uint8_t {
        UNRESOLVED,
        PRINTED_FROM_మాతృ,
        NO_PRINTED_FROM_మాతృ
    };

    VDouble0 m_stat_Class_గమ్యం_String;
    VDouble0 m_stat_Interface_గమ్యం_String;
    VDouble0 m_stat_Struct_Union_గమ్యం_String;

    శూన్య markClassIterate(AstClass* classp) {
        ఒకవేళ (classp->user1_సమితి_Once()) తిరిగి;
        VL_RESTORER(m_emitForMembers);
        ++m_emitForMembers;
        కొరకు (Ast_కణుపు* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                Ast_కణుపు_DType* కణుపు_Dtypep = varp->dtypep();
                ఒకవేళ (AstIfaceRefDType* స్థిరరాశి ifaceRedDTypep = VN_CAST(కణుపు_Dtypep, IfaceRefDType)) {
                    ఒకవేళ (ifaceRedDTypep->ifacep()) iterateConst(ifaceRedDTypep->ifacep());
                } లేకపోతే {
                    అయితే (కణుపు_Dtypep && కణుపు_Dtypep->subDTypep()
                           && కణుపు_Dtypep->subDTypep()->skipRefp()) {
                        కణుపు_Dtypep = కణుపు_Dtypep->subDTypep()->skipRefp();
                        ఒకవేళ (AstIfaceRefDType* స్థిరరాశి ifaceRedDTypep
                            = VN_CAST(కణుపు_Dtypep, IfaceRefDType)) {
                            ఒకవేళ (ifaceRedDTypep->ifacep()) {
                                iterateConst(ifaceRedDTypep->ifacep());
                            }
                        } లేకపోతే ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి uOrStructDTypep
                                   = VN_CAST(కణుపు_Dtypep, కణుపు_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
        }

        make_గమ్యం_String(classp);
        make_గమ్యం_String_మధ్య(classp);
        ++m_stat_Class_గమ్యం_String;
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* structp) override {
        ఒకవేళ (structp->user1())
            తిరిగి;  // if to_string() was already emitted skip the rest of the function
        ఒకవేళ (structp->emit_గమ్యం_String()) ++m_emitForMembers;
        ఒకవేళ (m_emitForMembers
            > 0) {  // either this struct directly requires emission of to_string() or the
                    // class/struct that has this struct as a field requires it to emit to_string()
            కొరకు (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_కణుపు_DType* కణుపు_Dtypep = memberp->dtypep();
                ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి uOrStructDTypep
                    = VN_CAST(కణుపు_Dtypep, కణుపు_UOr_Struct_DType)) {
                    iterateConst(uOrStructDTypep);
                } లేకపోతే {
                    అయితే (కణుపు_Dtypep && కణుపు_Dtypep->subDTypep()) {
                        కణుపు_Dtypep = కణుపు_Dtypep->subDTypep()->skipRefp();
                        ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి uOrStructDTypep
                            = VN_CAST(కణుపు_Dtypep, కణుపు_UOr_Struct_DType)) {
                            iterateConst(uOrStructDTypep);
                        }
                    }
                }
            }
            ఒకవేళ (structp->emit_గమ్యం_String()) --m_emitForMembers;

            ఒకవేళ (!structp->packed()) {
                make_Vl_గమ్యం_String(structp);
                ++m_stat_Struct_Union_గమ్యం_String;
            }

            structp->user1(నిజం);  // mark that to_string() was emitted to not emit it twice (if
                                   // e.g two printed classes have this struct as a field)
        }
    }
    శూన్య సందర్శించు(AstIface* ifacep) override {
        ఒకవేళ (ifacep->user1())
            తిరిగి;  // if to_string() was already emitted skip the rest of the function
        ఒకవేళ (m_emitForMembers
            > 0) {  // class that has this iface as a field requires it to emit to_string()
            make_Vl_గమ్యం_String(ifacep);
            ++m_stat_Interface_గమ్యం_String;
            ifacep->user1(నిజం);  // mark that to_string() was emitted to not emit it twice (if e.g
                                  // two printed classes have this interface as a field)
        }
    }
    // go towards class hierarchy root, if there is any printed from class on the way, mark its
    // ancestors, and mark everything on the return path from the printed from class
    శూన్య సందర్శించు(AstClass* classp) override {
        ఒకవేళ (classp->user1()) తిరిగి;
        ఒకవేళ (classp->isPrintedFrom()) {
            markClassIterate(classp);
            VL_RESTORER(m_emit_Towards_మూలం);
            ++m_emit_Towards_మూలం;
            కొరకు (స్థిరరాశి AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ఒకవేళ (extendp->classp() && !extendp->classp()->user1()) {
                    iterateConst(extendp->classp());
                }
            }
            classp->user2(static_cast<పూర్ణాంకం>(Printed_From_మాతృ::PRINTED_FROM_మాతృ));
        } లేకపోతే {
            Printed_From_మాతృ printed_మాతృ = static_cast<Printed_From_మాతృ>(classp->user2());
            ఒకవేళ (printed_మాతృ != Printed_From_మాతృ::UNRESOLVED && m_emit_Towards_మూలం == 0) {
                తిరిగి;
            }

            ఒకవేళ (m_emit_Towards_మూలం > 0) markClassIterate(classp);

            తార్కిక is_మూలం = నిజం;
            కొరకు (స్థిరరాశి AstClassExtends* extendp = classp->extendsp(); extendp;
                 extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                ఒకవేళ (extendp->classp()) {
                    ఒకవేళ (static_cast<Printed_From_మాతృ>(extendp->classp()->user2())
                            == Printed_From_మాతృ::UNRESOLVED
                        || (m_emit_Towards_మూలం > 0 && !extendp->classp()->user1())) {
                        iterateConst(extendp->classp());
                    }
                    ఒకవేళ (printed_మాతృ != Printed_From_మాతృ::PRINTED_FROM_మాతృ) {
                        printed_మాతృ
                            = static_cast<Printed_From_మాతృ>(extendp->classp()->user2());
                    }
                    is_మూలం = అబద్ధం;
                }
            }

            ఒకవేళ (is_మూలం) printed_మాతృ = Printed_From_మాతృ::NO_PRINTED_FROM_మాతృ;

            classp->user2(static_cast<పూర్ణాంకం>(printed_మాతృ));
            ఒకవేళ (printed_మాతృ == Printed_From_మాతృ::PRINTED_FROM_మాతృ) {
                markClassIterate(classp);

                // If one parent is printed from, other parent also has to emit to_string() to
                // handle cases like:
                // Base  IFaceClass (printed from)
                //   V    V
                //   Derived (this class)
                VL_RESTORER(m_emit_Towards_మూలం);
                ++m_emit_Towards_మూలం;
                కొరకు (స్థిరరాశి AstClassExtends* extendp = classp->extendsp(); extendp;
                     extendp = VN_AS(extendp->nextp(), ClassExtends)) {
                    ఒకవేళ (extendp->classp() && !extendp->classp()->user1()) {
                        iterateConst(extendp->classp());
                    }
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్పష్టమైన గమ్యం_String_Visitor(Ast_కణుపు* nodep) {
        ఒకవేళ (v3Global.hasPrintedObjects()) {
            Ast_కణుపు::user1_Clear_వృక్షం();
            Ast_కణుపు::user2_Clear_వృక్షం();

            iterateConst(nodep);
        }

        V3Stats::కూడు_Stat("Optimizations, Class ToString emitted", m_stat_Class_గమ్యం_String);
        V3Stats::కూడు_Stat("Optimizations, Interface ToString emitted", m_stat_Interface_గమ్యం_String);
        V3Stats::కూడు_Stat("Optimizations, Struct/union ToString emitted",
                         m_stat_Struct_Union_గమ్యం_String);
    }
    ~గమ్యం_String_Visitor() override = అప్రమేయ;
};

//######################################################################
// V3Common class functions

శూన్య V3Common::commonAll() {
    UINFO(2, __FUNCTION__ << ":");

    గమ్యం_String_Visitor{v3Global.rootp()};

    V3Global::dump_Check_Global_వృక్షం("common", 0, dump_వృక్షం_Either_Level() >= 3);
}
