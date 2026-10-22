// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert DfgLogic into primitive operations
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
//
// Synthesize DfgLogic vertices in as a graph, as created by V3DfgAstToDfg
// into primitive vertices.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Ast.h"
#ضمن "V3Cfg.h"
#ضمن "V3Const.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3EmitV.h"

#ضمن <algorithm>
#ضمن <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
قالب <نام_نوع T_رأس, نام_نوع T_گره>
T_رأس* make_رأس(ثابت T_گره* nodep, Dfg_گراف& dfg, ثابت Dfg_داده_Type& dtype) {
    أرجع جديد T_رأس{dfg, nodep->fileline(), dtype};
}

قالب <>
DfgArraySel* make_رأس<DfgArraySel, AstArraySel>(ثابت AstArraySel* nodep, Dfg_گراف& dfg,
                                                  ثابت Dfg_داده_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    إذا (VN_IS(nodep->fromp(), Const)) أرجع اشاره_تهی;
    إذا (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع اشاره_تهی;
    أرجع جديد DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
صنف Ast_مقصد_Dfg_Converter final : عام VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    Dfg_گراف& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = اشاره_تهی;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::وکتور<std::pair<AstVarScope*, Dfg_رأس_Var*>>* m_updatesp = اشاره_تهی;

    منطقي m_یافت_Unhandled = خطأ;  // Found node not implemented as DFG or not implemented 'visit'
    منطقي m_converting = خطأ;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    قالب <نام_نوع رأس_2, نام_نوع... آرگومانها_2>
    رأس_2* make(آرگومانها_2&&... آرگومانها) {
        static_assert(!std::is_پایه_از<Dfg_رأس_Var, رأس_2>::مقدار, "Do not use for variables");
        static_assert(std::is_پایه_از<Dfg_رأس, رأس_2>::مقدار, "'Vertex' must be a 'DfgVertex'");
        رأس_2* ثابت vtxp = جديد رأس_2{m_dfg, std::forward<آرگومانها_2>(آرگومانها)...};
        m_logicp->synth().emplace_پشت(vtxp);
        أرجع vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    منطقي unhandled(Ast_گره_عبارت* nodep) {
        // Short-circuiting if something was already unhandled
        إذا (m_یافت_Unhandled) {
            // Impure nodes cannot be represented
            إذا (!nodep->is_خالص()) {
                m_یافت_Unhandled = درست;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        أرجع m_یافت_Unhandled;
    }

    منطقي isSupported(ثابت AstVarRef* nodep) {
        // Cannot represent cross module references
        إذا (nodep->classOrPackagep()) أرجع خطأ;
        // Check target
        أرجع V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    Dfg_رأس* convertRValue(Ast_گره_عبارت* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_یافت_Unhandled);
        m_converting = درست;
        m_یافت_Unhandled = خطأ;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        إذا (m_یافت_Unhandled) أرجع اشاره_تهی;

        // Traversal set user2p to the equivalent vertex
        Dfg_رأس* ثابت vtxp = nodep->user2u().مقصد<Dfg_رأس*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        أرجع vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<Dfg_رأس_Splice*, uint32_t> convertLValue(Ast_گره_عبارت* nodep) {
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            إذا (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                أرجع {اشاره_تهی, 0};
            }

            // Get (or create a new) temporary for this variable
            ثابت Dfg_رأس_Var* ثابت vtxp = [&]() -> Dfg_رأس_Var* {
                // The variable being assigned
                AstVarScope* ثابت vscp = vrefp->varScopep();

                // Find existing one, if any
                لكل (ثابت خودکار& pair : *m_updatesp) {
                    إذا (pair.اولی == vscp) أرجع pair.دومی;
                }

                // Create new one
                Dfg_رأس_Var* ثابت newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_پشت(vscp, newp);

                // Create the Splice driver for the new temporary
                إذا (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } وإلا إذا (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } وإلا {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                أرجع newp;
            }();

            // Return the Splice driver
            أرجع {vtxp->srcp()->as<Dfg_رأس_Splice>(), 0};
        }

        إذا (ثابت AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            إذا (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                أرجع {اشاره_تهی, 0};
            }
            uint32_t lsb = lsbp->مقصد_UInt();

            // Convert the 'fromp' sub-expression
            ثابت خودکار pair = convertLValue(selp->fromp());
            إذا (!pair.اولی) أرجع {اشاره_تهی, 0};
            DfgSplicePacked* ثابت splicep = pair.اولی->قالب as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.دومی;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            إذا (lsb + static_cast<uint32_t>(selp->عرض_Const()) > splicep->اندازه()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                أرجع {اشاره_تهی, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            أرجع {splicep, lsb};
        }

        إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت indexp = VN_CAST(aselp->bitp(), Const);
            إذا (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                أرجع {اشاره_تهی, 0};
            }
            uint32_t شاخص = indexp->مقصد_UInt();

            // Convert the 'fromp' sub-expression
            ثابت خودکار pair = convertLValue(aselp->fromp());
            إذا (!pair.اولی) أرجع {اشاره_تهی, 0};
            DfgSpliceArray* ثابت splicep = pair.اولی->قالب as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            شاخص += pair.دومی;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            إذا (شاخص + 1U > splicep->اندازه()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                أرجع {اشاره_تهی, 0};
            }

            // Ensure the Splice driver exists for this element
            إذا (!splicep->driverAt(شاخص)) {
                File_سطر* ثابت flp = nodep->fileline();
                // This should never fail
                ثابت Dfg_داده_Type& dtype = *Dfg_داده_Type::fromAst(nodep->dtypep());
                إذا (dtype.isPacked()) {
                    DfgSplicePacked* ثابت newp = make<DfgSplicePacked>(flp, dtype);
                    ثابت Dfg_داده_Type& uaDtype = Dfg_داده_Type::array(dtype, 1);
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->جمع_کن_Driver(uap, شاخص, flp);
                } وإلا إذا (dtype.isArray()) {
                    DfgSpliceArray* ثابت newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->جمع_کن_Driver(newp, شاخص, flp);
                } وإلا {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            Dfg_رأس* driverp = splicep->driverAt(شاخص);
            إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            أرجع {driverp->as<Dfg_رأس_Splice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        أرجع {اشاره_تهی, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    منطقي convertAssignment(File_سطر* flp, Ast_گره_عبارت* lhsp, Dfg_رأس* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        بنية Assignment final {
            Dfg_رأس_Splice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            Dfg_رأس* m_rhsp;  // Vertex driving that position
            Assignment() = حذف;
            Assignment(Dfg_رأس_Splice* lhsp, uint32_t idx, Dfg_رأس* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        إذا (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), گره_عبارت);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        إذا (VN_IS(lhsp, Concat) && !vtxp->is<Dfg_رأس_Var>() && !vtxp->is<DfgConst>()) {
            Dfg_رأس_Var* ثابت tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::وکتور<Assignment> assignments;
        ثابت std::function<منطقي(Ast_گره_عبارت*, uint32_t)> convertAllLValues
            = [&](Ast_گره_عبارت* subp, uint32_t lsb) -> منطقي {
            // Concatenation on the LHS, convert each part
            إذا (AstConcat* ثابت concatp = VN_CAST(subp, Concat)) {
                Ast_گره_عبارت* ثابت cRhsp = concatp->rhsp();
                Ast_گره_عبارت* ثابت cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                إذا (!convertAllLValues(cRhsp, lsb)) أرجع خطأ;
                // Convert Left of concat
                أرجع convertAllLValues(cLhsp, lsb + cRhsp->عرض());
            }

            // Non-concatenation, convert the LValue
            ثابت خودکار pair = convertLValue(subp);
            إذا (!pair.اولی) أرجع خطأ;

            // If whole lhs, just use it
            إذا (subp == lhsp) {
                assignments.emplace_پشت(pair.اولی, pair.دومی, vtxp);
                أرجع درست;
            }

            // Otherwise select the relevant bits
            ثابت Dfg_داده_Type& dtype = *Dfg_داده_Type::fromAst(subp->dtypep());
            DfgSel* ثابت selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_پشت(pair.اولی, pair.دومی, selp);
            أرجع درست;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        إذا (!convertAllLValues(lhsp, 0)) أرجع خطأ;

        // All successful, connect the drivers
        لكل (ثابت Assignment& قلم_2 : assignments) {
            إذا (DfgSplicePacked* ثابت spp = قلم_2.m_lhsp->قالب cast<DfgSplicePacked>()) {
                spp->جمع_کن_Driver(قلم_2.m_rhsp, قلم_2.m_idx, flp);
            } وإلا إذا (DfgSpliceArray* ثابت sap = قلم_2.m_lhsp->قالب cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ثابت Dfg_داده_Type& rDt = قلم_2.m_rhsp->dtype();
                إذا (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, Dfg_داده_Type::array(rDt, 1));
                    uap->srcp(قلم_2.m_rhsp);
                    sap->جمع_کن_Driver(uap, قلم_2.m_idx, flp);
                } وإلا {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->جمع_کن_Driver(قلم_2.m_rhsp, قلم_2.m_idx, flp);
                }
            } وإلا {
                قلم_2.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        أرجع درست;
    }

    // VISITORS

    // Unhandled node
    فراغ بازدید(Ast_گره* /*nodep*/) override {
        إذا (!m_یافت_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_یافت_Unhandled = درست;
    }

    // Expressions - mostly auto generated, but a few special ones
    فراغ بازدید(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        إذا (!isSupported(nodep)) {
            m_یافت_Unhandled = درست;
            ++m_ctx.m_conv.nonRepVarRef;
            أرجع;
        }

        // Variable should have been bound before starting conversion
        Dfg_رأس* ثابت vtxp = nodep->varScopep()->user2u().قالب مقصد<Dfg_رأس_Var*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    فراغ بازدید(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        إذا (nodep->عرض() != nodep->رقم().عرض()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_عدد رقم{nodep, nodep->عرض()};
            رقم.opSel(nodep->رقم(), nodep->عرض() - 1, 0);
            Dfg_رأس* ثابت vtxp = make<DfgConst>(nodep->fileline(), رقم);
            nodep->user2p(vtxp);
        } وإلا {
            Dfg_رأس* ثابت vtxp = make<DfgConst>(nodep->fileline(), nodep->رقم());
            nodep->user2p(vtxp);
        }
    }
    فراغ بازدید(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_داده_Type* ثابت dtypep = Dfg_داده_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_یافت_Unhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        Dfg_رأس* ثابت vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    فراغ بازدید(Ast_تطابق_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_داده_Type* ثابت dtypep = Dfg_داده_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_یافت_Unhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        iterate(nodep->lhsp());
        إذا (m_یافت_Unhandled) أرجع;
        iterate(nodep->matchp());
        إذا (m_یافت_Unhandled) أرجع;

        File_سطر* ثابت flp = nodep->fileline();
        Dfg_تطابق_Masked* ثابت vtxp = make<Dfg_تطابق_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().مقصد<Dfg_رأس*>());
        vtxp->matchp(nodep->matchp()->user2u().مقصد<Dfg_رأس*>());
        nodep->user2p(vtxp);
    }
    فراغ بازدید(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_داده_Type* ثابت dtypep = Dfg_داده_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_یافت_Unhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        iterate(nodep->srcp());
        إذا (m_یافت_Unhandled) أرجع;

        File_سطر* ثابت flp = nodep->fileline();
        DfgRep* ثابت vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().مقصد<Dfg_رأس*>());
        nodep->user2p(vtxp);
    }
    فراغ بازدید(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_داده_Type* ثابت dtypep = Dfg_داده_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_یافت_Unhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        iterate(nodep->fromp());
        إذا (m_یافت_Unhandled) أرجع;

        File_سطر* ثابت flp = nodep->fileline();
        Dfg_رأس* vtxp = اشاره_تهی;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت uint32_t lsb = constp->مقصد_UInt();
            ثابت uint32_t msb = lsb + nodep->عرض_Const() - 1;
            Dfg_رأس* ثابت fromp = nodep->fromp()->user2u().مقصد<Dfg_رأس*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            إذا (msb >= fromp->اندازه()) {
                m_یافت_Unhandled = درست;
                ++m_ctx.m_conv.nonRepOOBSel;
                أرجع;
            }
            DfgSel* ثابت selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } وإلا {
            iterate(nodep->lsbp());
            إذا (m_یافت_Unhandled) أرجع;
            DfgMux* ثابت muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().مقصد<Dfg_رأس*>());
            muxp->lsbp(nodep->lsbp()->user2u().مقصد<Dfg_رأس*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#ضمن "V3Dfg__gen_ast_to_dfg.h"

عام:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    Dfg_رأس_Var* createTmp(DfgLogic& logic, File_سطر* flp, ثابت Dfg_داده_Type& dtype,
                            ثابت std::string& پیشوند) {
        Dfg_رأس_Var* ثابت vtxp = m_dfg.makeNewVar(flp, پیشوند, dtype, logic.scopep());
        logic.synth().emplace_پشت(vtxp);
        vtxp->vscp()->varp()->is_داخلی(درست);
        vtxp->tmpForp(vtxp->vscp());
        أرجع vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    Dfg_رأس_Var* createTmp(DfgLogic& logic, AstVarScope* vscp, ثابت std::string& پیشوند) {
        AstVar* ثابت astVarp = vscp->varp();
        File_سطر* ثابت flp = astVarp->fileline();
        ثابت Dfg_داده_Type& dtype = *Dfg_داده_Type::fromAst(astVarp->dtypep());
        ثابت std::string prfx = پیشوند + "_" + astVarp->name();
        Dfg_رأس_Var* ثابت vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        أرجع vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    منطقي convert(std::وکتور<std::pair<AstVarScope*, Dfg_رأس_Var*>>& updates, DfgLogic& vtx,
                 Ast_گره_تخصیص_بده* nodep) {
        UASSERT_OBJ(VN_IS(nodep, تخصیص_بده) || VN_IS(nodep, تخصیص_بده_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.خالی(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.درون_داد_Assignments;
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        // Check data types are compatible.
        ثابت Dfg_داده_Type* ثابت lDtypep = Dfg_داده_Type::fromAst(lhsp->dtypep());
        ثابت Dfg_داده_Type* ثابت rDtypep = Dfg_داده_Type::fromAst(rhsp->dtypep());
        إذا (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            أرجع خطأ;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        إذا (rDtypep->isArray()) {
            إذا (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_درخت(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                أرجع خطأ;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->عرض() == rhsp->عرض(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        Dfg_رأس* ثابت rVtxp = convertRValue(rhsp);
        إذا (!rVtxp) أرجع خطأ;
        // Connect the RHS vertex to the LHS targets
        ثابت منطقي success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        إذا (success) ++m_ctx.m_conv.representable;
        أرجع success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    Dfg_رأس* convert(DfgLogic& vtx, Ast_گره_عبارت* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.درون_داد_Expressions;
        Dfg_رأس* ثابت vtxp = convertRValue(nodep);
        إذا (vtxp) ++m_ctx.m_conv.representable;
        أرجع vtxp;
    }

    // CONSTRUCTOR
    Ast_مقصد_Dfg_Converter(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

صنف Ast_مقصد_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    بنية AstVarScopeComparator final {
        ساكن صحيح s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        منطقي عملگر()(AstVarScope* lhs, AstVarScope* rhs) ثابت {
            إذا (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            إذا (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            أرجع lhs->user4() < rhs->user4();
        }
    };
    استفاده SymTab = std::map<AstVarScope*, Dfg_رأس_Var*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    بنية Driver final {
        Dfg_رأس* m_vtxp = اشاره_تهی;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_سطر* m_flp = اشاره_تهی;  // Location of driver in source

        Driver() = افتراضي;
        Driver(Dfg_رأس* vtxp, uint32_t lo, File_سطر* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->اندازه() - 1U}
            , m_flp{flp} {}
        عملگر منطقي() ثابت { أرجع m_vtxp != اشاره_تهی; }

        منطقي عملگر<(ثابت Driver& other) ثابت {
            إذا (m_lo != other.m_lo) أرجع m_lo < other.m_lo;
            إذا (m_hi != other.m_hi) أرجع m_hi < other.m_hi;
            أرجع m_flp->operatorCompare(*other.m_flp) < 0;
        }

        منطقي عملگر<=(ثابت Driver& other) ثابت { أرجع !(other < *هذا); }
    };

    // STATE - Persistent
    Dfg_گراف& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_مقصد_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_مقصد_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = اشاره_تهی;  // Current logic vertex we are synthesizing
    Cfg_بلوک_Map<SymTab> m_bb_مقصد_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_بلوک_Map<SymTab> m_bb_مقصد_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_بلوک_Map<Dfg_رأس_Var*> m_bb_مقصد_Condp;  // Map from CfgBlock ->  terminating branch condition
    Cfg_یال_Map<Dfg_رأس_Var*> m_یال_مقصد_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_درخت m_dom_درخت;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = اشاره_تهی;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_مجموعه<ثابت Dfg_رأس*>> m_debugOSrcConep{اشاره_تهی};

    // METHODS

    // Dump current graph for debugging ...
    فراغ debugDump(ثابت حرف* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        إذا (VL_UNLIKELY(m_debugLogicp)) {
            std::وکتور<ثابت Dfg_رأس*> outputs;
            m_debugLogicp->foreachSink([&outputs](ثابت Dfg_رأس& v) {
                outputs.emplace_پشت(v.singleSink()->as<Dfg_رأس_Var>());
                أرجع خطأ;
            });
            m_debugOSrcConep = m_dfg.منبع_Cone(outputs);
        }

        إذا (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            إذا (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ثابت Dfg_رأس& v) -> منطقي {
                    أرجع m_debugOSrcConep->شمار(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    قالب <نام_نوع رأس_2, نام_نوع... آرگومانها_2>
    رأس_2* make(آرگومانها_2&&... آرگومانها) {
        static_assert(!std::is_پایه_از<Dfg_رأس_Var, رأس_2>::مقدار, "Do not use for variables");
        static_assert(std::is_پایه_از<Dfg_رأس, رأس_2>::مقدار, "'Vertex' must be a 'DfgVertex'");
        رأس_2* ثابت vtxp = جديد رأس_2{m_dfg, std::forward<آرگومانها_2>(آرگومانها)...};
        إذا (m_logicp) m_logicp->synth().emplace_پشت(vtxp);
        أرجع vtxp;
    }

    // Gather all drivers of a resolved variable
    ساكن std::وکتور<Driver> gatherDrivers(Dfg_رأس_Splice* vtxp) {
        // Collect them all, check if they are sorted
        std::وکتور<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        منطقي sorted = درست;
        vtxp->foreachDriver([&](Dfg_رأس& src, uint32_t lo, File_سطر* flp) {
            // Collect the driver
            drivers.emplace_پشت(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ثابت اندازه_t n = drivers.اندازه();
            إذا (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = خطأ;
            أرجع خطأ;
        });

        // Sort if unsorted
        إذا (!sorted) std::stable_مرتب_سازی(drivers.begin(), drivers.انتها());

        //انجامشده
        أرجع drivers;
    }

    // Returns true if the driver supplies a complete array element.
    ساكن منطقي driver_Covers_کامل_Element(ثابت Driver& driver) {
        ثابت DfgUnitArray* ثابت unitp = driver.m_vtxp->cast<DfgUnitArray>();
        إذا (!unitp) أرجع خطأ;
        إذا (Dfg_رأس_Splice* ثابت splicep = unitp->srcp()->cast<Dfg_رأس_Splice>()) {
            أرجع splicep->wholep();
        }
        أرجع درست;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    ساكن منطقي splice_Covers_کامل(Dfg_رأس_Splice* ثابت splicep) {
        إذا (splicep->wholep()) أرجع درست;
        إذا (splicep->isPacked()) أرجع خطأ;
        uint32_t بعدی = 0;
        لكل (ثابت Driver& driver : gatherDrivers(splicep)) {
            إذا (driver.m_lo != بعدی || !driver_Covers_کامل_Element(driver)) أرجع خطأ;
            بعدی = driver.m_hi + 1;
        }
        أرجع بعدی == splicep->اندازه();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    ساكن منطقي شامل_است_Tri_Lowered_Var(Dfg_رأس* rootp) {
        std::وکتور<ثابت Dfg_رأس*> پشته;
        std::وکتور<ثابت Dfg_رأس*> دیدهشده;
        پشته.emplace_پشت(rootp);
        طالما (!پشته.خالی()) {
            ثابت Dfg_رأس* ثابت vtxp = پشته.پشت();
            پشته.برداشت_پشت();
            إذا (std::پیدا_کن(دیدهشده.begin(), دیدهشده.انتها(), vtxp) != دیدهشده.انتها()) استمر;
            دیدهشده.emplace_پشت(vtxp);
            إذا (ثابت Dfg_رأس_Var* ثابت varp = vtxp->cast<Dfg_رأس_Var>()) {
                إذا (varp->vscp()->varp()->dfgTriLowered()) أرجع درست;
            }
            vtxp->foreach_منبع([&](ثابت Dfg_رأس& src) {
                پشته.emplace_پشت(&src);
                أرجع خطأ;
            });
        }
        أرجع خطأ;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    ساكن منطقي is_مستقیم_Var_Driver(ثابت Dfg_رأس* vtxp) { أرجع vtxp->is<Dfg_رأس_Var>(); }

    // Gather all synthesized drivers of an unresolved variable
    ساكن std::وکتور<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::وکتور<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ثابت std::function<فراغ(Dfg_رأس*, uint32_t, File_سطر*)> gather
            = [&](Dfg_رأس* vtxp, uint32_t lo, File_سطر* flp) -> فراغ {
            إذا (DfgConcat* ثابت concatp = vtxp->cast<DfgConcat>()) {
                Dfg_رأس* ثابت rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                Dfg_رأس* ثابت lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->عرض(), lhsp->fileline());
                أرجع;
            }
            drivers.emplace_پشت(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreach_منبع([&](Dfg_رأس& src) {
            // Can ignore the original DfgLogic
            إذا (src.is<DfgLogic>()) أرجع خطأ;
            // Synthesized drivers must be a splice at this point
            Dfg_رأس_Splice* ثابت splicep = src.as<Dfg_رأس_Splice>();
            // Collect the driver
            splicep->foreachDriver([&](Dfg_رأس& src, uint32_t lo, File_سطر* flp) {
                gather(&src, lo, flp);
                أرجع خطأ;
            });
            أرجع خطأ;
        });

        // Sort the drivers
        std::stable_مرتب_سازی(drivers.begin(), drivers.انتها());

        //انجامشده
        أرجع drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<منطقي, منطقي> combineDrivers(Dfg_رأس_Var& var, ثابت std::string& sub,  //
                                         Driver& a, ثابت Driver& b) {
        // We can only combine array drivers ...
        إذا (a.m_vtxp->isPacked()) أرجع {خطأ, خطأ};
        // ... that both drive a single element ...
        إذا (a.m_lo != b.m_lo) أرجع {خطأ, خطأ};
        ثابت DfgUnitArray* ثابت aUap = a.m_vtxp->قالب cast<DfgUnitArray>();
        إذا (!aUap) أرجع {خطأ, خطأ};
        ثابت DfgUnitArray* ثابت bUap = b.m_vtxp->قالب cast<DfgUnitArray>();
        إذا (!bUap) أرجع {خطأ, خطأ};
        // ... and are themeselves partial
        DfgSplicePacked* ثابت aSp = aUap->srcp()->قالب cast<DfgSplicePacked>();
        إذا (!aSp) أرجع {خطأ, خطأ};
        DfgSplicePacked* ثابت bSp = bUap->srcp()->قالب cast<DfgSplicePacked>();
        إذا (!bSp) أرجع {خطأ, خطأ};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::وکتور<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::وکتور<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::وکتور<Driver> abDrivers;
        abDrivers.reserve(aDrivers.اندازه() + bDrivers.اندازه());
        std::ادغام(  //
            aDrivers.begin(), aDrivers.انتها(),  //
            bDrivers.begin(), bDrivers.انتها(),  //
            std::پشت_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        إذا (!normalizeDrivers(var, abDrivers, sub + "[" + std::مقصد_string(a.m_lo) + "]")) {
            أرجع {خطأ, درست};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_سطر* ثابت flp = var.fileline();
        DfgSplicePacked* ثابت splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        لكل (ثابت Driver& d : abDrivers) splicep->جمع_کن_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        أرجع {درست, خطأ};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    منطقي normalizeDrivers(Dfg_رأس_Var& var, std::وکتور<Driver>& drivers,
                          ثابت std::string& sub = "") {
        إذا (drivers.خالی()) أرجع درست;

        // What type of values are we combining
        ثابت منطقي isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        منطقي multiDriven = خطأ;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        اندازه_t i = 0;
        لكل (اندازه_t j = 1; j < drivers.اندازه();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            إذا (!jD) {
                ++j;
                استمر;
            }
            // If 'i' was moved, move 'j' in it's place
            إذا (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                استمر;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            إذا (iD.m_hi < jD.m_lo) {
                ++i;
                إذا (i == j) ++j;
                استمر;
            }

            // There is an overlap. Attempt to combine them.
            منطقي combined = خطأ;
            منطقي warned = خطأ;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            إذا (combined) {
                jD = Driver{};
                ++j;
                استمر;
            }

            // Found overlap that cannot be resolved
            multiDriven = درست;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            إذا (warned) استمر;

            // The variable to warn on
            AstVarScope* ثابت vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            إذا (vscp->varp()->is_استفاده_شده_Loop_Idx()) استمر;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            إذا (vscp->varp()->dfgAllowMultidriveTri()) {
                ثابت منطقي iTri = شامل_است_Tri_Lowered_Var(iD.m_vtxp);
                ثابت منطقي jTri = شامل_است_Tri_Lowered_Var(jD.m_vtxp);
                ثابت منطقي triPair = iTri && jTri;
                ثابت منطقي triAndBridge = (iTri && is_مستقیم_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_مستقیم_Var_Driver(iD.m_vtxp));
                إذا (triPair || triAndBridge) استمر;
            }

            // Warn the user now
            ثابت std::string lo = std::مقصد_string(jD.m_lo);
            ثابت std::string hi = std::مقصد_string(std::min(iD.m_hi, jD.m_hi));
            ثابت std::string نوع = isPacked ? "Bit" : "Element";
            ثابت std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                نوع << part << " of signal '" << vscp->prettyName() << sub << "'"
                     << " have multiple combinational drivers."
                     << " This can cause performance degradation.\n"
                     << iD.m_flp->warnOther() << "... Location of offending driver\n"
                     << iD.m_flp->warnContextPrimary() << '\n'
                     << jD.m_flp->warnOther() << "... Location of offending driver\n"
                     << jD.m_flp->warnContextSecondary());
        }
        // Rightsize vector
        drivers.resize(i + 1);

        // Coalesce adjacent drivers
        إذا (!multiDriven && isPacked) coalesceDrivers(drivers);

        أرجع !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    فراغ coalesceDrivers(std::وکتور<Driver>& drivers) {
        UASSERT(!drivers.خالی(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        اندازه_t i = 0;
        لكل (اندازه_t j = 1; j < drivers.اندازه();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            إذا (!jD) {
                ++j;
                استمر;
            }
            // If 'i' was moved, move 'j' in it's place
            إذا (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                استمر;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            إذا (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                إذا (i == j) ++j;
                استمر;
            }

            // Coalesce Adjacent ranges,
            ثابت Dfg_داده_Type& dt = Dfg_داده_Type::packed(iD.m_vtxp->عرض() + jD.m_vtxp->عرض());
            DfgConcat* ثابت concatp = make<DfgConcat>(iD.m_flp, dt);
            concatp->rhsp(iD.m_vtxp);
            concatp->lhsp(jD.m_vtxp);
            iD.m_vtxp = concatp;
            iD.m_hi = jD.m_hi;
            jD = Driver{};

            // Consider next driver
            ++j;
        }
        // Rightsize vector
        drivers.resize(i + 1);
    }

    // Make a new splice with the given drivers
    Dfg_رأس_Splice* makeSplice(Dfg_رأس_Var& var, ثابت std::وکتور<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.خالی(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        Dfg_رأس_Splice* splicep = اشاره_تهی;
        إذا (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } وإلا إذا (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } وإلا {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        لكل (ثابت Driver& d : newDrivers) splicep->جمع_کن_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        أرجع splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    فراغ initialize_مدخل_نماد_جدول(SymTab& iSymTab) {
        m_logicp->foreach_منبع([&](Dfg_رأس& src) {
            Dfg_رأس_Var* ثابت vvp = src.as<Dfg_رأس_Var>();
            iSymTab[vvp->vscp()] = vvp;
            أرجع خطأ;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    Dfg_رأس_Var* joinDrivers(AstVarScope* vscp, Dfg_رأس_Var* predicatep,  //
                              Dfg_رأس_Var* thenp, Dfg_رأس_Var* elsep) {
        AstVarScope* ثابت thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ثابت elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        إذا (thenp == elsep) أرجع thenp;

        // We can't join the input variable just yet, so bail
        إذا (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_درون_داد;
            أرجع اشاره_تهی;
        }
        إذا (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_درون_داد;
            أرجع اشاره_تهی;
        }

        // Can't do arrays yet
        إذا (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            أرجع اشاره_تهی;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        Dfg_رأس* ثابت tDefaultp = thenp->defaultp();
        std::وکتور<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<Dfg_رأس_Splice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        Dfg_رأس* ثابت eDefaultp = elsep->defaultp();
        std::وکتور<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<Dfg_رأس_Splice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_سطر* ثابت flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        Dfg_رأس_Var* ثابت joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        Dfg_رأس_Splice* ثابت joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        إذا (tDrivers.اندازه() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->عرض() - 1  //
            && eDrivers.اندازه() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->عرض() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ثابت condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->جمع_کن_Driver(condp, 0, tDrivers[0].m_flp);

            //انجامشده
            أرجع joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        إذا (tDrivers.اندازه() != eDrivers.اندازه()) {
            ++m_ctx.m_synt.nonSynLatch;
            أرجع اشاره_تهی;
        }

        لكل (اندازه_t i = 0; i < tDrivers.اندازه(); ++i) {
            ثابت Driver& tDriver = tDrivers[i];
            ثابت Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            إذا (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                أرجع اشاره_تهی;
            }

            ثابت Dfg_داده_Type& dtype = Dfg_داده_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* ثابت condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* ثابت thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* ثابت elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->جمع_کن_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        إذا (tDefaultp) joinp->defaultp(tDefaultp);

        //انجامشده
        أرجع joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    منطقي join_نماد_Tables(SymTab& elseSymTab, Dfg_رأس_Var* predicatep, ثابت SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::وکتور<AstVarScope*> مقصد_Remove;

        // Join each symbol
        لكل (std::pair<AstVarScope* ثابت, Dfg_رأس_Var*>& pair : elseSymTab) {
            AstVarScope* ثابت varp = pair.اولی;
            // Find same variable on the else path
            ثابت خودکار it = thenSymTab.پیدا_کن(varp);
            // Record for removal if not assigned on both paths
            إذا (it == thenSymTab.انتها()) {
                مقصد_Remove.emplace_پشت(varp);
                استمر;
            }
            // Join paths with the predicate
            Dfg_رأس_Var* ثابت thenp = it->دومی;
            Dfg_رأس_Var* ثابت elsep = pair.دومی;
            Dfg_رأس_Var* ثابت newp = joinDrivers(varp, predicatep, thenp, elsep);
            إذا (!newp) أرجع خطأ;
            pair.دومی = newp;
        }

        // Remove variables not assigned on both paths
        لكل (AstVarScope* ثابت varp : مقصد_Remove) elseSymTab.erase(varp);

        //انجامشده
        أرجع درست;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<Dfg_رأس_Var*, ثابت Cfg_بلوک*, ثابت Cfg_بلوک*>  //
    how_مقصد_Join(ثابت Cfg_یال* ثابت ap, ثابت Cfg_یال* ثابت bp) {
        // Find the closest common dominator of the two paths
        ثابت Cfg_بلوک* ثابت domp = m_dom_درخت.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ثابت Cfg_یال* ثابت گرفته_Edgep = domp->گرفته_Edgep();
        ثابت Cfg_یال* ثابت untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        إذا (گرفته_Edgep == ap || m_dom_درخت.dominates(گرفته_Edgep->dstp(), ap->srcp())) {
            إذا (untknEdgep == bp || m_dom_درخت.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                أرجع std::make_tuple(m_bb_مقصد_Condp[domp], ap->srcp(), bp->srcp());
            }
        } وإلا إذا (گرفته_Edgep == bp || m_dom_درخت.dominates(گرفته_Edgep->dstp(), bp->srcp())) {
            إذا (untknEdgep == ap || m_dom_درخت.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                أرجع std::make_tuple(m_bb_مقصد_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_مسیر_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        أرجع std::make_tuple(m_یال_مقصد_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    منطقي create_درون_داد_نماد_جدول(ثابت Cfg_بلوک& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_مقصد_ISym_Tab[bb];

        // Input symbol table of entry block is special
        إذا (bb.isEnter()) {
            initialize_مدخل_نماد_جدول(joined);
            أرجع درست;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.خالی(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        إذا (!bb.isJoin()) {
            joined = m_bb_مقصد_OSym_Tab[bb.اولی_Predecessorp()];
            أرجع درست;
        }

        // We also have a simpler job if there are 2 predecessors
        إذا (bb.isTwoWayJoin()) {
            Dfg_رأس_Var* predicatep = اشاره_تهی;
            ثابت Cfg_بلوک* thenp = اشاره_تهی;
            ثابت Cfg_بلوک* elsep = اشاره_تهی;
            std::tie(predicatep, thenp, elsep)
                = how_مقصد_Join(bb.اولی_Predecessor_Edgep(), bb.آخرین_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_مقصد_OSym_Tab[elsep];
            // Join with then
            أرجع join_نماد_Tables(joined, predicatep, m_bb_مقصد_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        بنية Predecessor final {
            ثابت Cfg_بلوک* m_bbp;  // Predeccessor block
            Dfg_رأس_Var* m_predicatep;  // Predicate predecessor reached this block with
            ثابت SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = حذف;
            Predecessor(ثابت Cfg_بلوک* bbp, Dfg_رأس_Var* predicatep, ثابت SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ثابت std::وکتور<Predecessor> predecessors = [&]() {
            std::وکتور<Predecessor> res;
            لكل (ثابت V3_گراف_یال& یال : bb.درون_یالها()) {
                ثابت Cfg_یال& cfg_یال = static_cast<ثابت Cfg_یال&>(یال);
                ثابت Cfg_بلوک* ثابت predecessorp = cfg_یال.srcp();
                Dfg_رأس_Var* ثابت predicatep = m_یال_مقصد_Predicatep[cfg_یال];
                ثابت SymTab* ثابت oSymTabp = &m_bb_مقصد_OSym_Tab[predecessorp];
                res.emplace_پشت(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::مرتب_سازی(res.begin(), res.انتها(), [](ثابت Predecessor& a, ثابت Predecessor& b) {  //
                أرجع *a.m_bbp > *b.m_bbp;
            });
            أرجع res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        لكل (اندازه_t i = 1; i < predecessors.اندازه(); ++i) {
            Dfg_رأس_Var* ثابت predicatep = predecessors[i].m_predicatep;
            ثابت SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            إذا (!join_نماد_Tables(joined, predicatep, oSymTab)) أرجع خطأ;
        }

        أرجع درست;
    }

    std::وکتور<Driver> computePropagatedDrivers(ثابت std::وکتور<Driver>& newDrivers,
                                                 Dfg_رأس_Var* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ثابت std::وکتور<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_رأس_Splice>());
        UASSERT_OBJ(!oldDrivers.خالی(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::وکتور<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ثابت خودکار جمع_کن_Old_Driver = [&](File_سطر* ثابت flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.خالی() || lsb > propagatedDrivers.پشت().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ثابت selp = make<DfgSel>(flp, Dfg_داده_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_پشت(selp, lsb, flp);
        };

        // Incorporate old drivers
        لكل (ثابت Driver& oDriver : oldDrivers) {
            File_سطر* ثابت flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ثابت uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            لكل (ثابت Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                إذا (nDriver.m_hi < oldLo) استمر;
                // If new driver is entirely above old driver, we can stop
                إذا (oldHi < nDriver.m_lo) اكسر;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                إذا (oldLo < nDriver.m_lo) جمع_کن_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                إذا (oldLo > oldHi) اكسر;
            }

            // Insert remaining bits if any
            إذا (oldHi >= oldLo) جمع_کن_Old_Driver(flp, oldHi, oldLo);
        }

        أرجع propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    منطقي computePropagatedArrayDrivers(ثابت std::وکتور<Driver>& newDrivers,
                                       Dfg_رأس_Var* ثابت oldp,
                                       std::وکتور<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        ساكن constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        إذا (oldp->اندازه() > MAX_ARRAY_ELEMENTS) أرجع خطأ;
        لكل (ثابت Driver& driver : newDrivers) {
            إذا (!driver_Covers_کامل_Element(driver)) أرجع خطأ;
        }
        ثابت std::وکتور<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_رأس_Splice>());
        UASSERT_OBJ(!oldDrivers.خالی(), oldp, "Should have a proper driver");
        لكل (ثابت Driver& driver : oldDrivers) {
            إذا (!driver_Covers_کامل_Element(driver)) أرجع خطأ;
        }

        propagatedDrivers.reserve(oldDrivers.اندازه());
        خودکار nIt = newDrivers.begin();
        لكل (ثابت Driver& oDriver : oldDrivers) {
            طالما (nIt != newDrivers.انتها() && nIt->m_lo < oDriver.m_lo) ++nIt;
            إذا (nIt != newDrivers.انتها() && nIt->m_lo == oDriver.m_lo) استمر;

            File_سطر* ثابت flp = oDriver.m_flp;
            ثابت DfgUnitArray* ثابت oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ثابت selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<اندازه_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ثابت newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_پشت(newUnitp, oDriver.m_lo, flp);
        }
        أرجع درست;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    منطقي incorporate_قبلی_مقدار(AstVarScope* vscp, Dfg_رأس_Var* newp, Dfg_رأس_Var* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        إذا (!oldp) أرجع درست;

        // New driver was not yet coalesced, so should always be a splice
        Dfg_رأس_Splice* ثابت nSplicep = newp->srcp()->as<Dfg_رأس_Splice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        إذا (oldp->vscp() == vscp) {
            إذا (!splice_Covers_کامل(nSplicep)) newp->defaultp(oldp);
            أرجع درست;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::وکتور<Driver> nDrivers = gatherDrivers(newp->srcp()->as<Dfg_رأس_Splice>());
        UASSERT_OBJ(!nDrivers.خالی(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::وکتور<Driver> pDrivers;
        إذا (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } وإلا إذا (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            أرجع خطأ;
        }

        إذا (!pDrivers.خالی()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::وکتور<Driver> drivers;
            drivers.reserve(nDrivers.اندازه() + pDrivers.اندازه());
            std::ادغام(nDrivers.begin(), nDrivers.انتها(), pDrivers.begin(), pDrivers.انتها(),
                       std::پشت_inserter(drivers));
            // Coalesce adjacent ranges
            إذا (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            لكل (ثابت Driver& d : drivers) nSplicep->جمع_کن_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        إذا (oldp->defaultp() && !splice_Covers_کامل(nSplicep)) newp->defaultp(oldp->defaultp());

        //انجامشده
        أرجع درست;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    منطقي synthesize_Basic_بلوک(SymTab& oSymTab, Dfg_رأس*& condpr,
                              ثابت std::وکتور<Ast_گره_Stmt*>& stmtps, ثابت SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ثابت VNUser2_درون_Use user2_درون_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        لكل (ثابت خودکار& pair : iSymTab) {
            AstVarScope* ثابت varp = pair.اولی;
            Dfg_رأس_Var* ثابت vtxp = pair.دومی;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::وکتور<std::pair<AstVarScope*, Dfg_رأس_Var*>> updates;
        لكل (Ast_گره_Stmt* ثابت stmtp : stmtps) {
            // Regular statements
            Ast_گره_تخصیص_بده* ثابت ap = VN_CAST(stmtp, گره_تخصیص_بده);
            إذا (ap && (VN_IS(ap, تخصیص_بده) || VN_IS(ap, تخصیص_بده_W))) {
                // Convert this assignment
                إذا (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    أرجع خطأ;
                }
                // Apply variable updates from this statement
                لكل (ثابت خودکار& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ثابت vscp = pair.اولی;
                    // The new, potentially partially assigned value
                    Dfg_رأس_Var* ثابت newp = pair.دومی;
                    // Normalize drivers within this statement, bail if multidriven
                    Dfg_رأس_Splice* ثابت srcp = newp->srcp()->as<Dfg_رأس_Splice>();
                    std::وکتور<Driver> drivers = gatherDrivers(srcp);
                    ثابت منطقي single = drivers.اندازه() == 1;
                    إذا (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        أرجع خطأ;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    إذا (!single) {
                        srcp->resetDrivers();
                        لكل (ثابت Driver& d : drivers) srcp->جمع_کن_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    Dfg_رأس_Var* ثابت oldp = vscp->user2u().قالب مقصد<Dfg_رأس_Var*>();
                    // Inncorporate old value into the new value
                    إذا (!incorporate_قبلی_مقدار(vscp, newp, oldp)) أرجع خطأ;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                استمر;
            }

            // Terminator branches
            إذا (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.پشت(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                Dfg_رأس* condp = m_converter.convert(*m_logicp, ifp->condp());
                إذا (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    أرجع خطأ;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                إذا (condp->عرض() != 1) {
                    File_سطر* ثابت flp = condp->fileline();
                    DfgNeq* ثابت neqp = make<DfgNeq>(flp, Dfg_داده_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->عرض(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                استمر;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            أرجع خطأ;
        }

        أرجع درست;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    فراغ تخصیص_بده_مسیر_Predicates(ثابت Cfg_بلوک& bb) {
        // Nothing to do for the exit block
        إذا (bb.isExit()) أرجع;

        // Get the predicate of this block
        Dfg_رأس* ثابت predp = [&]() -> Dfg_رأس* {
            // Entry block has no predecessors, use constant true
            إذا (bb.isEnter()) أرجع make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ثابت خودکار& درون_یالها = bb.درون_یالها();
            خودکار it = درون_یالها.begin();
            Dfg_رأس* resp = m_یال_مقصد_Predicatep[static_cast<ثابت Cfg_یال&>(*it)];
            طالما (++it != درون_یالها.انتها()) {
                DfgOr* ثابت orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_یال_مقصد_Predicatep[static_cast<ثابت Cfg_یال&>(*it)]);
                resp = orp;
            }
            أرجع resp;
        }();

        ثابت Dfg_داده_Type& dtype = predp->dtype();

        ثابت خودکار mkTmp = [&](File_سطر* flp, ثابت حرف* name, Dfg_رأس* srcp) {
            ثابت std::string پیشوند = "_BB" + std::مقصد_string(bb.id()) + "_" + name;
            Dfg_رأس_Var* ثابت tmpp = m_converter.createTmp(*m_logicp, flp, dtype, پیشوند);
            tmpp->srcp(srcp);
            أرجع tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        Dfg_رأس_Var* ثابت pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        إذا (!bb.isBranch()) {
            m_یال_مقصد_Predicatep[bb.گرفته_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            أرجع;
        }

        // For branches, we need to factor in the branch condition
        Dfg_رأس* ثابت condp = m_bb_مقصد_Condp[bb];
        File_سطر* ثابت flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ثابت گرفته_Predp = make<DfgAnd>(flp, dtype);
            گرفته_Predp->lhsp(pInp);
            گرفته_Predp->rhsp(condp);
            m_یال_مقصد_Predicatep[bb.گرفته_Edgep()] = mkTmp(flp, "Taken", گرفته_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ثابت untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ثابت notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_یال_مقصد_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    منطقي checkExtWrites() {
        لكل (ثابت Dfg_رأس* ثابت vtxp : m_logicp->synth()) {
            ثابت Dfg_رأس_Var* ثابت varp = vtxp->cast<Dfg_رأس_Var>();
            إذا (!varp) استمر;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            إذا (!varp->hasSinks()) استمر;
            إذا (!Dfg_رأس_Var::isVolatile(varp->tmpForp())) استمر;
            ++m_ctx.m_synt.nonSynExtWrite;
            أرجع خطأ;
        }
        أرجع درست;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    منطقي جمع_کن_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ثابت منطقي missing = m_logicp->foreachSink([&](ثابت Dfg_رأس& sink) {
            ثابت DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ثابت vscp = unresolvedp->singleSink()->as<Dfg_رأس_Var>()->vscp();
            أرجع !oSymTab.شمار(vscp);
        });
        إذا (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            أرجع خطأ;
        }

        // Add sinks to read the computed values for the target variables
        أرجع !m_logicp->foreachSink([&](Dfg_رأس& sink) {
            DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            ثابت Dfg_رأس_Var* ثابت varp = unresolvedp->singleSink()->as<Dfg_رأس_Var>();
            Dfg_رأس_Var* ثابت resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            إذا (!resp->hasSinks()) {
                unresolvedp->جمع_کن_Driver(resp->srcp()->as<Dfg_رأس_Splice>());
                أرجع خطأ;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            إذا (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                أرجع درست;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ثابت splicep
                = جديد DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ثابت std::وکتور<Driver> drivers = computePropagatedDrivers({}, resp);
            لكل (ثابت Driver& d : drivers) splicep->جمع_کن_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->جمع_کن_Driver(splicep);
            أرجع خطأ;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    منطقي synthesize_تخصیص_بده_W(Ast_تخصیص_بده_W* nodep) {
        ++m_ctx.m_synt.درون_داد_تخصیص_بده;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_مدخل_نماد_جدول(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        Dfg_رأس* condp = اشاره_تهی;
        ثابت منطقي success = synthesize_Basic_بلوک(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        إذا (!success) أرجع خطأ;

        // Check exernal writes are observed correctly
        إذا (!checkExtWrites()) أرجع خطأ;

        // Add resolved output variable drivers
        أرجع جمع_کن_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    منطقي synthesizeCfg(Cfg_گراف& cfg) {
        ++m_ctx.m_synt.درون_داد_Always;

        // If there is a backward edge (loop), we can't synthesize it
        إذا (cfg.شامل_است_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            أرجع خطأ;
        }

        // If it's a trivial CFG we can save on some work
        إذا (cfg.n_بلوکها() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } وإلا {
            // Insert two-way join blocks to aid multiplexer ordering
            إذا (cfg.درج_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } وإلا {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_درخت = Cfg_Dominator_درخت{cfg};
            m_یال_مقصد_Predicatep = cfg.make_یال_Map<Dfg_رأس_Var*>();
        }

        // Initialize CfgMaps
        m_bb_مقصد_ISym_Tab = cfg.make_بلوک_Map<SymTab>();
        m_bb_مقصد_OSym_Tab = cfg.make_بلوک_Map<SymTab>();
        m_bb_مقصد_Condp = cfg.make_بلوک_Map<Dfg_رأس_Var*>();

        // Synthesize all blocks
        لكل (ثابت V3_گراف_رأس& vtx : cfg.vertices()) {
            ثابت Cfg_بلوک& bb = static_cast<ثابت Cfg_بلوک&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            إذا (!create_درون_داد_نماد_جدول(bb)) أرجع خطأ;
            // Synthesize this block
            Dfg_رأس* condp = اشاره_تهی;
            إذا (!synthesize_Basic_بلوک(m_bb_مقصد_OSym_Tab[bb], condp, bb.stmtps(), m_bb_مقصد_ISym_Tab[bb])) {
                أرجع خطأ;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            إذا (condp) {
                File_سطر* ثابت flp = condp->fileline();
                ثابت Dfg_داده_Type& dtype = condp->dtype();
                ثابت std::string پیشوند = "_BB" + std::مقصد_string(bb.id()) + "_Cond";
                Dfg_رأس_Var* ثابت vp = m_converter.createTmp(*m_logicp, flp, dtype, پیشوند);
                vp->srcp(condp);
                m_bb_مقصد_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            تخصیص_بده_مسیر_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        إذا (!checkExtWrites()) أرجع خطأ;

        // Add resolved output variable drivers
        أرجع جمع_کن_Synthesized_Output(m_bb_مقصد_OSym_Tab[cfg.خروج()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    منطقي synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        إذا (Ast_تخصیص_بده_W* ثابت ap = VN_CAST(vtx.nodep()->stmtsp(), تخصیص_بده_W)) {
            إذا (ap->nextp()) أرجع خطأ;
            إذا (!synthesize_تخصیص_بده_W(ap)) أرجع خطأ;
            ++m_ctx.m_synt.synth_تخصیص_بده;
            أرجع درست;
        }

        إذا (!synthesizeCfg(vtx.cfg())) أرجع خطأ;
        ++m_ctx.m_synt.synthAlways;
        أرجع درست;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    فراغ revert(VDouble0& statCountr) {
        m_مقصد_Revert.foreach([&](Dfg_رأس& vtx) {
            // Process DfgLogic
            إذا (DfgLogic* ثابت vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                لكل (Dfg_رأس* ثابت p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->مجموعه_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](Dfg_رأس& snk) {
                    m_مقصد_Revert.افزودن_front(*snk.as<DfgUnresolved>());
                    أرجع خطأ;
                });
                أرجع;
            }

            // Process DfgUnresolved
            إذا (DfgUnresolved* ثابت vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_رأس_Var>()->مجموعه_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreach_منبع([&](Dfg_رأس& src) {
                    DfgLogic* ثابت lp = src.cast<DfgLogic>();
                    إذا (lp && !lp->reverted()) m_مقصد_Revert.افزودن_front(*lp);
                    أرجع خطأ;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                أرجع;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    فراغ اصلی() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        لكل (Dfg_رأس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            إذا (!vtxp->is<DfgUnresolved>()) استمر;
            منطقي anySelected = خطأ;
            منطقي anyUnselected = خطأ;
            vtxp->foreach_منبع([&](Dfg_رأس& src) {
                ثابت DfgLogic& logic = *src.as<DfgLogic>();
                إذا (logic.selectedForSynthesis()) {
                    anySelected = درست;
                } وإلا {
                    anyUnselected = درست;
                }
                أرجع خطأ;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            إذا (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_رأس_Var>()->مجموعه_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        لكل (Dfg_رأس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            إذا (!logicp) استمر;
            إذا (logicp->selectedForSynthesis()) استمر;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ثابت VNUser4_درون_Use m_user4_درون_Use;
            logicp->foreach_منبع([](Dfg_رأس& src) {
                src.as<Dfg_رأس_Var>()->vscp()->user4p(&src);
                أرجع خطأ;
            });
            V3DfgPasses::جمع_کن_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_گره* varp) {  //
                أرجع varp->user4u().مقصد<Dfg_رأس_Var*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ثابت VNUser4_درون_Use user4_درون_Use;
            لكل (Dfg_رأس& vtx : m_dfg.opVertices()) {
                DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
                إذا (!logicp) استمر;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ثابت خودکار debugCallback = [&]() -> فراغ {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_درخت(logicp->nodep(), std::خروجی);
                    debugDump("synth-lastok");
                };
                إذا (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) اكسر;

                // Synthesize it, if failed, enqueue for reversion
                إذا (!synthesize(*logicp)) {
                    logicp->مجموعه_Non_Synthesizable();
                    m_مقصد_Revert.افزودن_front(*logicp);
                }
            }
        }
        debugDump("synth-converted");

        //-------------------------------------------------------------------
        UINFO(5, "Step 2: Revert drivers of variables with unsynthesizeable drivers");
        // We do this as the variables might be multi-driven, we can't know if synthesis failed
        revert(m_ctx.m_synt.revertNonSyn);
        debugDump("synth-reverted");

        //-------------------------------------------------------------------
        UINFO(5, "Step 3: Resolve synthesized drivers of original (non-temporary) variables");
        // List of multi-driven variables
        std::وکتور<Dfg_رأس_Var*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ثابت Dfg_رأس_Var*, Dfg_رأس_Splice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        لكل (Dfg_رأس_Var& var : m_dfg.varVertices()) {
            إذا (!var.srcp()) استمر;
            DfgUnresolved* ثابت unresolvedp = var.srcp()->cast<DfgUnresolved>();
            إذا (!unresolvedp) اكسر;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            Dfg_رأس_Splice* ثابت resolvedp = [&]() -> Dfg_رأس_Splice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                إذا (unresolvedp->nInputs() == 1) {
                    أرجع unresolvedp->inputp(0)->as<Dfg_رأس_Splice>();
                }
                // Otherwise gather the synthesized drivers
                std::وکتور<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                إذا (normalizeDrivers(var, drivers)) أرجع makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_پشت(&var);
                m_مقصد_Revert.افزودن_front(*unresolvedp);
                أرجع اشاره_تهی;
            }();
            // Bail if multidriven
            إذا (!resolvedp) استمر;
            // Add to map for next loop
            ثابت منطقي new_مدخل = resolvedDrivers.emplace(&var, resolvedp).دومی;
            UASSERT_OBJ(new_مدخل, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        لكل (ثابت Dfg_رأس_Var& var : m_dfg.varVertices()) {
            إذا (!var.srcp()) استمر;
            DfgUnresolved* ثابت srcp = var.srcp()->cast<DfgUnresolved>();
            إذا (!srcp) اكسر;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        لكل (Dfg_رأس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            إذا (!logicp) استمر;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            إذا (!logicp->synth().خالی()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_پشت()->delete_درخت();
            } وإلا {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ثابت VNUser4_درون_Use m_user4_درون_Use;
                logicp->foreach_منبع([](Dfg_رأس& src) {
                    src.as<Dfg_رأس_Var>()->vscp()->user4p(&src);
                    أرجع خطأ;
                });
                V3DfgPasses::جمع_کن_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_گره* varp) {  //
                    أرجع varp->user4u().مقصد<Dfg_رأس_Var*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = اشاره_تهی;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        لكل (Dfg_رأس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            Dfg_رأس_Splice* ثابت splicep = vtxp->cast<Dfg_رأس_Splice>();
            إذا (!splicep) استمر;

            // Might not have a sink if the driving logic was revered, remove
            إذا (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                استمر;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            إذا (Dfg_رأس* ثابت wholep = splicep->wholep()) {
                إذا (Dfg_رأس_Var* ثابت varp = splicep->singleSink()->cast<Dfg_رأس_Var>()) {
                    varp->driver_File_سطر(splicep->driver_File_سطر(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    Ast_مقصد_Dfg_Synthesize(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        اصلی();
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx) {
        Ast_مقصد_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_استفادهنشده(dfg);
        إذا (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        إذا (v3Global.opt.debugCheck()) {
            لكل (Dfg_رأس& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_مضرب_Sinks(), &vtx, "Operation has multiple sinks");
                لكل (اندازه_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

صحيح Ast_مقصد_Dfg_Synthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
ساكن فراغ dfgSelectLogicForSynthesis(Dfg_گراف& dfg) {
    // If we are told to synthesize everything, we will do so ...
    إذا (v3Global.opt.fDfgSynthesizeAll()) {
        لكل (Dfg_رأس& vtx : dfg.opVertices()) {
            إذا (DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) logicp->مجموعه_Selected_For_Synthesis();
        }
        أرجع;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::وکتور<Dfg_رأس_Var*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        لكل (Dfg_رأس_Var& var : dfg.varVertices()) {
            إذا (!scc.at(var)) استمر;
            // This is a circular variable
            circularVarps.emplace_پشت(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    لكل (ثابت Dfg_رأس_Var* ثابت varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreach_منبع([&](Dfg_رأس& driver) {
            worklist.افزودن_front(*driver.as<DfgLogic>());
            أرجع خطأ;
        });
    }

    // Choose some simple special cases to always synthesize
    لكل (Dfg_رأس& vtx : dfg.opVertices()) {
        DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        إذا (!logicp || worklist.شامل_است(*logicp)) استمر;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        إذا (logicp->drives_استفادهنشده_Vars()) {
            worklist.افزودن_front(*logicp);
            استمر;
        }
        // Blocks corresponding to continuous assignments
        إذا (logicp->nodep()->keyword() == VAlwaysKwd::CONT_تخصیص_بده) {
            worklist.افزودن_front(*logicp);
            استمر;
        }
        ثابت Cfg_گراف& cfg = logicp->cfg();
        // Straight line code with no branches
        إذا (cfg.n_بلوکها() == 1) {
            worklist.افزودن_front(*logicp);
            استمر;
        }
        // Blocks driving exactly 1 variable
        إذا (!logicp->has_مضرب_Sinks()) worklist.افزودن_front(*logicp);
    }

    std::unordered_مجموعه<DfgUnresolved*> is_دیدهشده;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](Dfg_رأس& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.مجموعه_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](Dfg_رأس& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            إذا (!is_دیدهشده.درج(unresolved).دومی) أرجع خطأ;
            unresolved->foreach_منبع([&](Dfg_رأس& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                إذا (!siblingLogic.selectedForSynthesis()) worklist.افزودن_front(siblingLogic);
                أرجع خطأ;
            });
            أرجع خطأ;
        });
    });
}

فراغ V3DfgPasses::synthesize(Dfg_گراف& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    Ast_مقصد_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
