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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Ast.h"
#شامل "V3Cfg.h"
#شامل "V3Const.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3EmitV.h"

#شامل <algorithm>
#شامل <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
سانچہ <قسم_نام T_راس, قسم_نام T_گرہ>
T_راس* make_راس(ثابت T_گرہ* nodep, Dfg_گراف& dfg, ثابت Dfg_ڈیٹا_Type& dtype) {
    واپس نیا T_راس{dfg, nodep->fileline(), dtype};
}

سانچہ <>
DfgArraySel* make_راس<DfgArraySel, AstArraySel>(ثابت AstArraySel* nodep, Dfg_گراف& dfg,
                                                  ثابت Dfg_ڈیٹا_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    اگر (VN_IS(nodep->fromp(), Const)) واپس خالی_اشاریہ;
    اگر (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) واپس خالی_اشاریہ;
    واپس نیا DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
جماعت Ast_مقصد_Dfg_Converter final : عوامی VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    Dfg_گراف& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = خالی_اشاریہ;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::سمتیہ<std::pair<AstVarScope*, Dfg_راس_Var*>>* m_updatesp = خالی_اشاریہ;

    منطقی m_ملا_Unhandled = جھوٹ;  // Found node not implemented as DFG or not implemented 'visit'
    منطقی m_converting = جھوٹ;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    سانچہ <قسم_نام راس_2, قسم_نام... دلائل_2>
    راس_2* make(دلائل_2&&... دلائل) {
        static_assert(!std::is_بنیاد_کا<Dfg_راس_Var, راس_2>::قدر, "Do not use for variables");
        static_assert(std::is_بنیاد_کا<Dfg_راس, راس_2>::قدر, "'Vertex' must be a 'DfgVertex'");
        راس_2* ثابت vtxp = نیا راس_2{m_dfg, std::forward<دلائل_2>(دلائل)...};
        m_logicp->synth().emplace_عقب(vtxp);
        واپس vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    منطقی unhandled(Ast_گرہ_اظہاریہ* nodep) {
        // Short-circuiting if something was already unhandled
        اگر (m_ملا_Unhandled) {
            // Impure nodes cannot be represented
            اگر (!nodep->is_خالص()) {
                m_ملا_Unhandled = سچ;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        واپس m_ملا_Unhandled;
    }

    منطقی isSupported(ثابت AstVarRef* nodep) {
        // Cannot represent cross module references
        اگر (nodep->classOrPackagep()) واپس جھوٹ;
        // Check target
        واپس V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    Dfg_راس* convertRValue(Ast_گرہ_اظہاریہ* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_ملا_Unhandled);
        m_converting = سچ;
        m_ملا_Unhandled = جھوٹ;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        اگر (m_ملا_Unhandled) واپس خالی_اشاریہ;

        // Traversal set user2p to the equivalent vertex
        Dfg_راس* ثابت vtxp = nodep->user2u().مقصد<Dfg_راس*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        واپس vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<Dfg_راس_Splice*, uint32_t> convertLValue(Ast_گرہ_اظہاریہ* nodep) {
        اگر (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            اگر (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                واپس {خالی_اشاریہ, 0};
            }

            // Get (or create a new) temporary for this variable
            ثابت Dfg_راس_Var* ثابت vtxp = [&]() -> Dfg_راس_Var* {
                // The variable being assigned
                AstVarScope* ثابت vscp = vrefp->varScopep();

                // Find existing one, if any
                برائے (ثابت خودکار& pair : *m_updatesp) {
                    اگر (pair.پہلا == vscp) واپس pair.دوسرا;
                }

                // Create new one
                Dfg_راس_Var* ثابت newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_عقب(vscp, newp);

                // Create the Splice driver for the new temporary
                اگر (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } ورنہ اگر (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } ورنہ {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                واپس newp;
            }();

            // Return the Splice driver
            واپس {vtxp->srcp()->as<Dfg_راس_Splice>(), 0};
        }

        اگر (ثابت AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            اگر (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                واپس {خالی_اشاریہ, 0};
            }
            uint32_t lsb = lsbp->مقصد_UInt();

            // Convert the 'fromp' sub-expression
            ثابت خودکار pair = convertLValue(selp->fromp());
            اگر (!pair.پہلا) واپس {خالی_اشاریہ, 0};
            DfgSplicePacked* ثابت splicep = pair.پہلا->سانچہ as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.دوسرا;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            اگر (lsb + static_cast<uint32_t>(selp->چوڑائی_Const()) > splicep->سائز()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                واپس {خالی_اشاریہ, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            واپس {splicep, lsb};
        }

        اگر (ثابت AstArraySel* ثابت aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت indexp = VN_CAST(aselp->bitp(), Const);
            اگر (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                واپس {خالی_اشاریہ, 0};
            }
            uint32_t اشاریہ = indexp->مقصد_UInt();

            // Convert the 'fromp' sub-expression
            ثابت خودکار pair = convertLValue(aselp->fromp());
            اگر (!pair.پہلا) واپس {خالی_اشاریہ, 0};
            DfgSpliceArray* ثابت splicep = pair.پہلا->سانچہ as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            اشاریہ += pair.دوسرا;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            اگر (اشاریہ + 1U > splicep->سائز()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                واپس {خالی_اشاریہ, 0};
            }

            // Ensure the Splice driver exists for this element
            اگر (!splicep->driverAt(اشاریہ)) {
                File_سطر* ثابت flp = nodep->fileline();
                // This should never fail
                ثابت Dfg_ڈیٹا_Type& dtype = *Dfg_ڈیٹا_Type::fromAst(nodep->dtypep());
                اگر (dtype.isPacked()) {
                    DfgSplicePacked* ثابت newp = make<DfgSplicePacked>(flp, dtype);
                    ثابت Dfg_ڈیٹا_Type& uaDtype = Dfg_ڈیٹا_Type::array(dtype, 1);
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->جوڑو_Driver(uap, اشاریہ, flp);
                } ورنہ اگر (dtype.isArray()) {
                    DfgSpliceArray* ثابت newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->جوڑو_Driver(newp, اشاریہ, flp);
                } ورنہ {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            Dfg_راس* driverp = splicep->driverAt(اشاریہ);
            اگر (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            واپس {driverp->as<Dfg_راس_Splice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        واپس {خالی_اشاریہ, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    منطقی convertAssignment(File_سطر* flp, Ast_گرہ_اظہاریہ* lhsp, Dfg_راس* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        ساخت Assignment final {
            Dfg_راس_Splice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            Dfg_راس* m_rhsp;  // Vertex driving that position
            Assignment() = حذف;
            Assignment(Dfg_راس_Splice* lhsp, uint32_t idx, Dfg_راس* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        اگر (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), گرہ_اظہاریہ);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        اگر (VN_IS(lhsp, Concat) && !vtxp->is<Dfg_راس_Var>() && !vtxp->is<DfgConst>()) {
            Dfg_راس_Var* ثابت tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::سمتیہ<Assignment> assignments;
        ثابت std::function<منطقی(Ast_گرہ_اظہاریہ*, uint32_t)> convertAllLValues
            = [&](Ast_گرہ_اظہاریہ* subp, uint32_t lsb) -> منطقی {
            // Concatenation on the LHS, convert each part
            اگر (AstConcat* ثابت concatp = VN_CAST(subp, Concat)) {
                Ast_گرہ_اظہاریہ* ثابت cRhsp = concatp->rhsp();
                Ast_گرہ_اظہاریہ* ثابت cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                اگر (!convertAllLValues(cRhsp, lsb)) واپس جھوٹ;
                // Convert Left of concat
                واپس convertAllLValues(cLhsp, lsb + cRhsp->چوڑائی());
            }

            // Non-concatenation, convert the LValue
            ثابت خودکار pair = convertLValue(subp);
            اگر (!pair.پہلا) واپس جھوٹ;

            // If whole lhs, just use it
            اگر (subp == lhsp) {
                assignments.emplace_عقب(pair.پہلا, pair.دوسرا, vtxp);
                واپس سچ;
            }

            // Otherwise select the relevant bits
            ثابت Dfg_ڈیٹا_Type& dtype = *Dfg_ڈیٹا_Type::fromAst(subp->dtypep());
            DfgSel* ثابت selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_عقب(pair.پہلا, pair.دوسرا, selp);
            واپس سچ;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        اگر (!convertAllLValues(lhsp, 0)) واپس جھوٹ;

        // All successful, connect the drivers
        برائے (ثابت Assignment& شے_3 : assignments) {
            اگر (DfgSplicePacked* ثابت spp = شے_3.m_lhsp->سانچہ cast<DfgSplicePacked>()) {
                spp->جوڑو_Driver(شے_3.m_rhsp, شے_3.m_idx, flp);
            } ورنہ اگر (DfgSpliceArray* ثابت sap = شے_3.m_lhsp->سانچہ cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ثابت Dfg_ڈیٹا_Type& rDt = شے_3.m_rhsp->dtype();
                اگر (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, Dfg_ڈیٹا_Type::array(rDt, 1));
                    uap->srcp(شے_3.m_rhsp);
                    sap->جوڑو_Driver(uap, شے_3.m_idx, flp);
                } ورنہ {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->جوڑو_Driver(شے_3.m_rhsp, شے_3.m_idx, flp);
                }
            } ورنہ {
                شے_3.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        واپس سچ;
    }

    // VISITORS

    // Unhandled node
    خالی دورہ(Ast_گرہ* /*nodep*/) override {
        اگر (!m_ملا_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_ملا_Unhandled = سچ;
    }

    // Expressions - mostly auto generated, but a few special ones
    خالی دورہ(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) واپس;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        اگر (!isSupported(nodep)) {
            m_ملا_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepVarRef;
            واپس;
        }

        // Variable should have been bound before starting conversion
        Dfg_راس* ثابت vtxp = nodep->varScopep()->user2u().سانچہ مقصد<Dfg_راس_Var*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    خالی دورہ(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) واپس;

        اگر (nodep->چوڑائی() != nodep->عدد().چوڑائی()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_نمبر عدد{nodep, nodep->چوڑائی()};
            عدد.opSel(nodep->عدد(), nodep->چوڑائی() - 1, 0);
            Dfg_راس* ثابت vtxp = make<DfgConst>(nodep->fileline(), عدد);
            nodep->user2p(vtxp);
        } ورنہ {
            Dfg_راس* ثابت vtxp = make<DfgConst>(nodep->fileline(), nodep->عدد());
            nodep->user2p(vtxp);
        }
    }
    خالی دورہ(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) واپس;

        ثابت Dfg_ڈیٹا_Type* ثابت dtypep = Dfg_ڈیٹا_Type::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_ملا_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            واپس;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        Dfg_راس* ثابت vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    خالی دورہ(Ast_مطابقت_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) واپس;

        ثابت Dfg_ڈیٹا_Type* ثابت dtypep = Dfg_ڈیٹا_Type::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_ملا_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            واپس;
        }

        iterate(nodep->lhsp());
        اگر (m_ملا_Unhandled) واپس;
        iterate(nodep->matchp());
        اگر (m_ملا_Unhandled) واپس;

        File_سطر* ثابت flp = nodep->fileline();
        Dfg_مطابقت_Masked* ثابت vtxp = make<Dfg_مطابقت_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().مقصد<Dfg_راس*>());
        vtxp->matchp(nodep->matchp()->user2u().مقصد<Dfg_راس*>());
        nodep->user2p(vtxp);
    }
    خالی دورہ(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) واپس;

        ثابت Dfg_ڈیٹا_Type* ثابت dtypep = Dfg_ڈیٹا_Type::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_ملا_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            واپس;
        }

        iterate(nodep->srcp());
        اگر (m_ملا_Unhandled) واپس;

        File_سطر* ثابت flp = nodep->fileline();
        DfgRep* ثابت vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().مقصد<Dfg_راس*>());
        nodep->user2p(vtxp);
    }
    خالی دورہ(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) واپس;

        ثابت Dfg_ڈیٹا_Type* ثابت dtypep = Dfg_ڈیٹا_Type::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_ملا_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            واپس;
        }

        iterate(nodep->fromp());
        اگر (m_ملا_Unhandled) واپس;

        File_سطر* ثابت flp = nodep->fileline();
        Dfg_راس* vtxp = خالی_اشاریہ;
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت uint32_t lsb = constp->مقصد_UInt();
            ثابت uint32_t msb = lsb + nodep->چوڑائی_Const() - 1;
            Dfg_راس* ثابت fromp = nodep->fromp()->user2u().مقصد<Dfg_راس*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            اگر (msb >= fromp->سائز()) {
                m_ملا_Unhandled = سچ;
                ++m_ctx.m_conv.nonRepOOBSel;
                واپس;
            }
            DfgSel* ثابت selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } ورنہ {
            iterate(nodep->lsbp());
            اگر (m_ملا_Unhandled) واپس;
            DfgMux* ثابت muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().مقصد<Dfg_راس*>());
            muxp->lsbp(nodep->lsbp()->user2u().مقصد<Dfg_راس*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#شامل "V3Dfg__gen_ast_to_dfg.h"

عوامی:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    Dfg_راس_Var* createTmp(DfgLogic& logic, File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype,
                            ثابت std::string& سابقہ) {
        Dfg_راس_Var* ثابت vtxp = m_dfg.makeNewVar(flp, سابقہ, dtype, logic.scopep());
        logic.synth().emplace_عقب(vtxp);
        vtxp->vscp()->varp()->is_اندرونی(سچ);
        vtxp->tmpForp(vtxp->vscp());
        واپس vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    Dfg_راس_Var* createTmp(DfgLogic& logic, AstVarScope* vscp, ثابت std::string& سابقہ) {
        AstVar* ثابت astVarp = vscp->varp();
        File_سطر* ثابت flp = astVarp->fileline();
        ثابت Dfg_ڈیٹا_Type& dtype = *Dfg_ڈیٹا_Type::fromAst(astVarp->dtypep());
        ثابت std::string prfx = سابقہ + "_" + astVarp->name();
        Dfg_راس_Var* ثابت vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        واپس vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    منطقی convert(std::سمتیہ<std::pair<AstVarScope*, Dfg_راس_Var*>>& updates, DfgLogic& vtx,
                 Ast_گرہ_تفویض* nodep) {
        UASSERT_OBJ(VN_IS(nodep, تفویض) || VN_IS(nodep, تفویض_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.خالی_خانے(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.داخلہ_Assignments;
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
        // Check data types are compatible.
        ثابت Dfg_ڈیٹا_Type* ثابت lDtypep = Dfg_ڈیٹا_Type::fromAst(lhsp->dtypep());
        ثابت Dfg_ڈیٹا_Type* ثابت rDtypep = Dfg_ڈیٹا_Type::fromAst(rhsp->dtypep());
        اگر (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            واپس جھوٹ;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        اگر (rDtypep->isArray()) {
            اگر (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_درخت(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                واپس جھوٹ;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->چوڑائی() == rhsp->چوڑائی(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        Dfg_راس* ثابت rVtxp = convertRValue(rhsp);
        اگر (!rVtxp) واپس جھوٹ;
        // Connect the RHS vertex to the LHS targets
        ثابت منطقی success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        اگر (success) ++m_ctx.m_conv.representable;
        واپس success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    Dfg_راس* convert(DfgLogic& vtx, Ast_گرہ_اظہاریہ* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.داخلہ_Expressions;
        Dfg_راس* ثابت vtxp = convertRValue(nodep);
        اگر (vtxp) ++m_ctx.m_conv.representable;
        واپس vtxp;
    }

    // CONSTRUCTOR
    Ast_مقصد_Dfg_Converter(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

جماعت Ast_مقصد_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    ساخت AstVarScopeComparator final {
        ساکن صحیح s_vscp_Id_شمار;  // Counter for lazily allocating the unique AstVarScope IDs
        منطقی عامل()(AstVarScope* lhs, AstVarScope* rhs) ثابت {
            اگر (!lhs->user4()) lhs->user4(++s_vscp_Id_شمار);
            اگر (!rhs->user4()) rhs->user4(++s_vscp_Id_شمار);
            واپس lhs->user4() < rhs->user4();
        }
    };
    استعمال SymTab = std::map<AstVarScope*, Dfg_راس_Var*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    ساخت Driver final {
        Dfg_راس* m_vtxp = خالی_اشاریہ;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_سطر* m_flp = خالی_اشاریہ;  // Location of driver in source

        Driver() = طےشدہ;
        Driver(Dfg_راس* vtxp, uint32_t lo, File_سطر* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->سائز() - 1U}
            , m_flp{flp} {}
        عامل منطقی() ثابت { واپس m_vtxp != خالی_اشاریہ; }

        منطقی عامل<(ثابت Driver& other) ثابت {
            اگر (m_lo != other.m_lo) واپس m_lo < other.m_lo;
            اگر (m_hi != other.m_hi) واپس m_hi < other.m_hi;
            واپس m_flp->operatorCompare(*other.m_flp) < 0;
        }

        منطقی عامل<=(ثابت Driver& other) ثابت { واپس !(other < *یہ); }
    };

    // STATE - Persistent
    Dfg_گراف& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_مقصد_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_مقصد_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = خالی_اشاریہ;  // Current logic vertex we are synthesizing
    Cfg_بلاک_Map<SymTab> m_bb_مقصد_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_بلاک_Map<SymTab> m_bb_مقصد_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_بلاک_Map<Dfg_راس_Var*> m_bb_مقصد_Condp;  // Map from CfgBlock ->  terminating branch condition
    Cfg_کنارہ_Map<Dfg_راس_Var*> m_کنارہ_مقصد_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_درخت m_dom_درخت;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = خالی_اشاریہ;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_سیٹ<ثابت Dfg_راس*>> m_debugOSrcConep{خالی_اشاریہ};

    // METHODS

    // Dump current graph for debugging ...
    خالی debugDump(ثابت حرف* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        اگر (VL_UNLIKELY(m_debugLogicp)) {
            std::سمتیہ<ثابت Dfg_راس*> outputs;
            m_debugLogicp->foreachSink([&outputs](ثابت Dfg_راس& v) {
                outputs.emplace_عقب(v.singleSink()->as<Dfg_راس_Var>());
                واپس جھوٹ;
            });
            m_debugOSrcConep = m_dfg.ماخذ_Cone(outputs);
        }

        اگر (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            اگر (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ثابت Dfg_راس& v) -> منطقی {
                    واپس m_debugOSrcConep->گنتی(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    سانچہ <قسم_نام راس_2, قسم_نام... دلائل_2>
    راس_2* make(دلائل_2&&... دلائل) {
        static_assert(!std::is_بنیاد_کا<Dfg_راس_Var, راس_2>::قدر, "Do not use for variables");
        static_assert(std::is_بنیاد_کا<Dfg_راس, راس_2>::قدر, "'Vertex' must be a 'DfgVertex'");
        راس_2* ثابت vtxp = نیا راس_2{m_dfg, std::forward<دلائل_2>(دلائل)...};
        اگر (m_logicp) m_logicp->synth().emplace_عقب(vtxp);
        واپس vtxp;
    }

    // Gather all drivers of a resolved variable
    ساکن std::سمتیہ<Driver> gatherDrivers(Dfg_راس_Splice* vtxp) {
        // Collect them all, check if they are sorted
        std::سمتیہ<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        منطقی sorted = سچ;
        vtxp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سطر* flp) {
            // Collect the driver
            drivers.emplace_عقب(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ثابت سائز_t n = drivers.سائز();
            اگر (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = جھوٹ;
            واپس جھوٹ;
        });

        // Sort if unsorted
        اگر (!sorted) std::stable_ترتیب_دو(drivers.begin(), drivers.انتہا());

        //مکمل
        واپس drivers;
    }

    // Returns true if the driver supplies a complete array element.
    ساکن منطقی driver_Covers_پورا_Element(ثابت Driver& driver) {
        ثابت DfgUnitArray* ثابت unitp = driver.m_vtxp->cast<DfgUnitArray>();
        اگر (!unitp) واپس جھوٹ;
        اگر (Dfg_راس_Splice* ثابت splicep = unitp->srcp()->cast<Dfg_راس_Splice>()) {
            واپس splicep->wholep();
        }
        واپس سچ;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    ساکن منطقی splice_Covers_پورا(Dfg_راس_Splice* ثابت splicep) {
        اگر (splicep->wholep()) واپس سچ;
        اگر (splicep->isPacked()) واپس جھوٹ;
        uint32_t اگلا = 0;
        برائے (ثابت Driver& driver : gatherDrivers(splicep)) {
            اگر (driver.m_lo != اگلا || !driver_Covers_پورا_Element(driver)) واپس جھوٹ;
            اگلا = driver.m_hi + 1;
        }
        واپس اگلا == splicep->سائز();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    ساکن منطقی شامل_ہے_Tri_Lowered_Var(Dfg_راس* rootp) {
        std::سمتیہ<ثابت Dfg_راس*> ڈھیر;
        std::سمتیہ<ثابت Dfg_راس*> دیکھا;
        ڈھیر.emplace_عقب(rootp);
        جب (!ڈھیر.خالی_خانے()) {
            ثابت Dfg_راس* ثابت vtxp = ڈھیر.عقب();
            ڈھیر.نکالو_عقب();
            اگر (std::ڈھونڈو(دیکھا.begin(), دیکھا.انتہا(), vtxp) != دیکھا.انتہا()) جاری;
            دیکھا.emplace_عقب(vtxp);
            اگر (ثابت Dfg_راس_Var* ثابت varp = vtxp->cast<Dfg_راس_Var>()) {
                اگر (varp->vscp()->varp()->dfgTriLowered()) واپس سچ;
            }
            vtxp->foreach_ماخذ([&](ثابت Dfg_راس& src) {
                ڈھیر.emplace_عقب(&src);
                واپس جھوٹ;
            });
        }
        واپس جھوٹ;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    ساکن منطقی is_براہ_راست_Var_Driver(ثابت Dfg_راس* vtxp) { واپس vtxp->is<Dfg_راس_Var>(); }

    // Gather all synthesized drivers of an unresolved variable
    ساکن std::سمتیہ<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::سمتیہ<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ثابت std::function<خالی(Dfg_راس*, uint32_t, File_سطر*)> gather
            = [&](Dfg_راس* vtxp, uint32_t lo, File_سطر* flp) -> خالی {
            اگر (DfgConcat* ثابت concatp = vtxp->cast<DfgConcat>()) {
                Dfg_راس* ثابت rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                Dfg_راس* ثابت lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->چوڑائی(), lhsp->fileline());
                واپس;
            }
            drivers.emplace_عقب(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreach_ماخذ([&](Dfg_راس& src) {
            // Can ignore the original DfgLogic
            اگر (src.is<DfgLogic>()) واپس جھوٹ;
            // Synthesized drivers must be a splice at this point
            Dfg_راس_Splice* ثابت splicep = src.as<Dfg_راس_Splice>();
            // Collect the driver
            splicep->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سطر* flp) {
                gather(&src, lo, flp);
                واپس جھوٹ;
            });
            واپس جھوٹ;
        });

        // Sort the drivers
        std::stable_ترتیب_دو(drivers.begin(), drivers.انتہا());

        //مکمل
        واپس drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<منطقی, منطقی> combineDrivers(Dfg_راس_Var& var, ثابت std::string& sub,  //
                                         Driver& a, ثابت Driver& b) {
        // We can only combine array drivers ...
        اگر (a.m_vtxp->isPacked()) واپس {جھوٹ, جھوٹ};
        // ... that both drive a single element ...
        اگر (a.m_lo != b.m_lo) واپس {جھوٹ, جھوٹ};
        ثابت DfgUnitArray* ثابت aUap = a.m_vtxp->سانچہ cast<DfgUnitArray>();
        اگر (!aUap) واپس {جھوٹ, جھوٹ};
        ثابت DfgUnitArray* ثابت bUap = b.m_vtxp->سانچہ cast<DfgUnitArray>();
        اگر (!bUap) واپس {جھوٹ, جھوٹ};
        // ... and are themeselves partial
        DfgSplicePacked* ثابت aSp = aUap->srcp()->سانچہ cast<DfgSplicePacked>();
        اگر (!aSp) واپس {جھوٹ, جھوٹ};
        DfgSplicePacked* ثابت bSp = bUap->srcp()->سانچہ cast<DfgSplicePacked>();
        اگر (!bSp) واپس {جھوٹ, جھوٹ};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::سمتیہ<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::سمتیہ<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::سمتیہ<Driver> abDrivers;
        abDrivers.reserve(aDrivers.سائز() + bDrivers.سائز());
        std::ملاؤ(  //
            aDrivers.begin(), aDrivers.انتہا(),  //
            bDrivers.begin(), bDrivers.انتہا(),  //
            std::عقب_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        اگر (!normalizeDrivers(var, abDrivers, sub + "[" + std::مقصد_string(a.m_lo) + "]")) {
            واپس {جھوٹ, سچ};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_سطر* ثابت flp = var.fileline();
        DfgSplicePacked* ثابت splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        برائے (ثابت Driver& d : abDrivers) splicep->جوڑو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        واپس {سچ, جھوٹ};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    منطقی normalizeDrivers(Dfg_راس_Var& var, std::سمتیہ<Driver>& drivers,
                          ثابت std::string& sub = "") {
        اگر (drivers.خالی_خانے()) واپس سچ;

        // What type of values are we combining
        ثابت منطقی isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        منطقی multiDriven = جھوٹ;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        سائز_t i = 0;
        برائے (سائز_t j = 1; j < drivers.سائز();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            اگر (!jD) {
                ++j;
                جاری;
            }
            // If 'i' was moved, move 'j' in it's place
            اگر (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                جاری;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            اگر (iD.m_hi < jD.m_lo) {
                ++i;
                اگر (i == j) ++j;
                جاری;
            }

            // There is an overlap. Attempt to combine them.
            منطقی combined = جھوٹ;
            منطقی warned = جھوٹ;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            اگر (combined) {
                jD = Driver{};
                ++j;
                جاری;
            }

            // Found overlap that cannot be resolved
            multiDriven = سچ;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            اگر (warned) جاری;

            // The variable to warn on
            AstVarScope* ثابت vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            اگر (vscp->varp()->is_مستعمل_Loop_Idx()) جاری;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            اگر (vscp->varp()->dfgAllowMultidriveTri()) {
                ثابت منطقی iTri = شامل_ہے_Tri_Lowered_Var(iD.m_vtxp);
                ثابت منطقی jTri = شامل_ہے_Tri_Lowered_Var(jD.m_vtxp);
                ثابت منطقی triPair = iTri && jTri;
                ثابت منطقی triAndBridge = (iTri && is_براہ_راست_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_براہ_راست_Var_Driver(iD.m_vtxp));
                اگر (triPair || triAndBridge) جاری;
            }

            // Warn the user now
            ثابت std::string lo = std::مقصد_string(jD.m_lo);
            ثابت std::string hi = std::مقصد_string(std::min(iD.m_hi, jD.m_hi));
            ثابت std::string قسم = isPacked ? "Bit" : "Element";
            ثابت std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                قسم << part << " of signal '" << vscp->prettyName() << sub << "'"
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
        اگر (!multiDriven && isPacked) coalesceDrivers(drivers);

        واپس !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    خالی coalesceDrivers(std::سمتیہ<Driver>& drivers) {
        UASSERT(!drivers.خالی_خانے(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        سائز_t i = 0;
        برائے (سائز_t j = 1; j < drivers.سائز();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            اگر (!jD) {
                ++j;
                جاری;
            }
            // If 'i' was moved, move 'j' in it's place
            اگر (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                جاری;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            اگر (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                اگر (i == j) ++j;
                جاری;
            }

            // Coalesce Adjacent ranges,
            ثابت Dfg_ڈیٹا_Type& dt = Dfg_ڈیٹا_Type::packed(iD.m_vtxp->چوڑائی() + jD.m_vtxp->چوڑائی());
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
    Dfg_راس_Splice* makeSplice(Dfg_راس_Var& var, ثابت std::سمتیہ<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.خالی_خانے(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        Dfg_راس_Splice* splicep = خالی_اشاریہ;
        اگر (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } ورنہ اگر (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } ورنہ {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        برائے (ثابت Driver& d : newDrivers) splicep->جوڑو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        واپس splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    خالی initialize_اندراج_علامت_جدول(SymTab& iSymTab) {
        m_logicp->foreach_ماخذ([&](Dfg_راس& src) {
            Dfg_راس_Var* ثابت vvp = src.as<Dfg_راس_Var>();
            iSymTab[vvp->vscp()] = vvp;
            واپس جھوٹ;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    Dfg_راس_Var* joinDrivers(AstVarScope* vscp, Dfg_راس_Var* predicatep,  //
                              Dfg_راس_Var* thenp, Dfg_راس_Var* elsep) {
        AstVarScope* ثابت thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ثابت elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        اگر (thenp == elsep) واپس thenp;

        // We can't join the input variable just yet, so bail
        اگر (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_داخلہ;
            واپس خالی_اشاریہ;
        }
        اگر (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_داخلہ;
            واپس خالی_اشاریہ;
        }

        // Can't do arrays yet
        اگر (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            واپس خالی_اشاریہ;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        Dfg_راس* ثابت tDefaultp = thenp->defaultp();
        std::سمتیہ<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<Dfg_راس_Splice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        Dfg_راس* ثابت eDefaultp = elsep->defaultp();
        std::سمتیہ<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<Dfg_راس_Splice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_سطر* ثابت flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        Dfg_راس_Var* ثابت joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        Dfg_راس_Splice* ثابت joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        اگر (tDrivers.سائز() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->چوڑائی() - 1  //
            && eDrivers.سائز() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->چوڑائی() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ثابت condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->جوڑو_Driver(condp, 0, tDrivers[0].m_flp);

            //مکمل
            واپس joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        اگر (tDrivers.سائز() != eDrivers.سائز()) {
            ++m_ctx.m_synt.nonSynLatch;
            واپس خالی_اشاریہ;
        }

        برائے (سائز_t i = 0; i < tDrivers.سائز(); ++i) {
            ثابت Driver& tDriver = tDrivers[i];
            ثابت Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            اگر (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                واپس خالی_اشاریہ;
            }

            ثابت Dfg_ڈیٹا_Type& dtype = Dfg_ڈیٹا_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
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
            joinSplicep->جوڑو_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        اگر (tDefaultp) joinp->defaultp(tDefaultp);

        //مکمل
        واپس joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    منطقی join_علامت_Tables(SymTab& elseSymTab, Dfg_راس_Var* predicatep, ثابت SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::سمتیہ<AstVarScope*> مقصد_Remove;

        // Join each symbol
        برائے (std::pair<AstVarScope* ثابت, Dfg_راس_Var*>& pair : elseSymTab) {
            AstVarScope* ثابت varp = pair.پہلا;
            // Find same variable on the else path
            ثابت خودکار it = thenSymTab.ڈھونڈو(varp);
            // Record for removal if not assigned on both paths
            اگر (it == thenSymTab.انتہا()) {
                مقصد_Remove.emplace_عقب(varp);
                جاری;
            }
            // Join paths with the predicate
            Dfg_راس_Var* ثابت thenp = it->دوسرا;
            Dfg_راس_Var* ثابت elsep = pair.دوسرا;
            Dfg_راس_Var* ثابت newp = joinDrivers(varp, predicatep, thenp, elsep);
            اگر (!newp) واپس جھوٹ;
            pair.دوسرا = newp;
        }

        // Remove variables not assigned on both paths
        برائے (AstVarScope* ثابت varp : مقصد_Remove) elseSymTab.erase(varp);

        //مکمل
        واپس سچ;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<Dfg_راس_Var*, ثابت Cfg_بلاک*, ثابت Cfg_بلاک*>  //
    how_مقصد_Join(ثابت Cfg_کنارہ* ثابت ap, ثابت Cfg_کنارہ* ثابت bp) {
        // Find the closest common dominator of the two paths
        ثابت Cfg_بلاک* ثابت domp = m_dom_درخت.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ثابت Cfg_کنارہ* ثابت لیا_گیا_Edgep = domp->لیا_گیا_Edgep();
        ثابت Cfg_کنارہ* ثابت untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        اگر (لیا_گیا_Edgep == ap || m_dom_درخت.dominates(لیا_گیا_Edgep->dstp(), ap->srcp())) {
            اگر (untknEdgep == bp || m_dom_درخت.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                واپس std::make_tuple(m_bb_مقصد_Condp[domp], ap->srcp(), bp->srcp());
            }
        } ورنہ اگر (لیا_گیا_Edgep == bp || m_dom_درخت.dominates(لیا_گیا_Edgep->dstp(), bp->srcp())) {
            اگر (untknEdgep == ap || m_dom_درخت.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                واپس std::make_tuple(m_bb_مقصد_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_راستہ_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        واپس std::make_tuple(m_کنارہ_مقصد_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    منطقی create_داخلہ_علامت_جدول(ثابت Cfg_بلاک& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_مقصد_ISym_Tab[bb];

        // Input symbol table of entry block is special
        اگر (bb.isEnter()) {
            initialize_اندراج_علامت_جدول(joined);
            واپس سچ;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.خالی_خانے(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        اگر (!bb.isJoin()) {
            joined = m_bb_مقصد_OSym_Tab[bb.پہلا_Predecessorp()];
            واپس سچ;
        }

        // We also have a simpler job if there are 2 predecessors
        اگر (bb.isTwoWayJoin()) {
            Dfg_راس_Var* predicatep = خالی_اشاریہ;
            ثابت Cfg_بلاک* thenp = خالی_اشاریہ;
            ثابت Cfg_بلاک* elsep = خالی_اشاریہ;
            std::tie(predicatep, thenp, elsep)
                = how_مقصد_Join(bb.پہلا_Predecessor_Edgep(), bb.آخری_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_مقصد_OSym_Tab[elsep];
            // Join with then
            واپس join_علامت_Tables(joined, predicatep, m_bb_مقصد_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        ساخت Predecessor final {
            ثابت Cfg_بلاک* m_bbp;  // Predeccessor block
            Dfg_راس_Var* m_predicatep;  // Predicate predecessor reached this block with
            ثابت SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = حذف;
            Predecessor(ثابت Cfg_بلاک* bbp, Dfg_راس_Var* predicatep, ثابت SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ثابت std::سمتیہ<Predecessor> predecessors = [&]() {
            std::سمتیہ<Predecessor> res;
            برائے (ثابت V3_گراف_کنارہ& کنارہ : bb.اندر_کنارے()) {
                ثابت Cfg_کنارہ& cfg_کنارہ = static_cast<ثابت Cfg_کنارہ&>(کنارہ);
                ثابت Cfg_بلاک* ثابت predecessorp = cfg_کنارہ.srcp();
                Dfg_راس_Var* ثابت predicatep = m_کنارہ_مقصد_Predicatep[cfg_کنارہ];
                ثابت SymTab* ثابت oSymTabp = &m_bb_مقصد_OSym_Tab[predecessorp];
                res.emplace_عقب(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::ترتیب_دو(res.begin(), res.انتہا(), [](ثابت Predecessor& a, ثابت Predecessor& b) {  //
                واپس *a.m_bbp > *b.m_bbp;
            });
            واپس res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        برائے (سائز_t i = 1; i < predecessors.سائز(); ++i) {
            Dfg_راس_Var* ثابت predicatep = predecessors[i].m_predicatep;
            ثابت SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            اگر (!join_علامت_Tables(joined, predicatep, oSymTab)) واپس جھوٹ;
        }

        واپس سچ;
    }

    std::سمتیہ<Driver> computePropagatedDrivers(ثابت std::سمتیہ<Driver>& newDrivers,
                                                 Dfg_راس_Var* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ثابت std::سمتیہ<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_راس_Splice>());
        UASSERT_OBJ(!oldDrivers.خالی_خانے(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::سمتیہ<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ثابت خودکار جوڑو_Old_Driver = [&](File_سطر* ثابت flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.خالی_خانے() || lsb > propagatedDrivers.عقب().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ثابت selp = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_عقب(selp, lsb, flp);
        };

        // Incorporate old drivers
        برائے (ثابت Driver& oDriver : oldDrivers) {
            File_سطر* ثابت flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ثابت uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            برائے (ثابت Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                اگر (nDriver.m_hi < oldLo) جاری;
                // If new driver is entirely above old driver, we can stop
                اگر (oldHi < nDriver.m_lo) توڑو;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                اگر (oldLo < nDriver.m_lo) جوڑو_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                اگر (oldLo > oldHi) توڑو;
            }

            // Insert remaining bits if any
            اگر (oldHi >= oldLo) جوڑو_Old_Driver(flp, oldHi, oldLo);
        }

        واپس propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    منطقی computePropagatedArrayDrivers(ثابت std::سمتیہ<Driver>& newDrivers,
                                       Dfg_راس_Var* ثابت oldp,
                                       std::سمتیہ<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        ساکن constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        اگر (oldp->سائز() > MAX_ARRAY_ELEMENTS) واپس جھوٹ;
        برائے (ثابت Driver& driver : newDrivers) {
            اگر (!driver_Covers_پورا_Element(driver)) واپس جھوٹ;
        }
        ثابت std::سمتیہ<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_راس_Splice>());
        UASSERT_OBJ(!oldDrivers.خالی_خانے(), oldp, "Should have a proper driver");
        برائے (ثابت Driver& driver : oldDrivers) {
            اگر (!driver_Covers_پورا_Element(driver)) واپس جھوٹ;
        }

        propagatedDrivers.reserve(oldDrivers.سائز());
        خودکار nIt = newDrivers.begin();
        برائے (ثابت Driver& oDriver : oldDrivers) {
            جب (nIt != newDrivers.انتہا() && nIt->m_lo < oDriver.m_lo) ++nIt;
            اگر (nIt != newDrivers.انتہا() && nIt->m_lo == oDriver.m_lo) جاری;

            File_سطر* ثابت flp = oDriver.m_flp;
            ثابت DfgUnitArray* ثابت oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ثابت selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<سائز_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ثابت newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_عقب(newUnitp, oDriver.m_lo, flp);
        }
        واپس سچ;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    منطقی incorporate_پچھلا_قدر(AstVarScope* vscp, Dfg_راس_Var* newp, Dfg_راس_Var* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        اگر (!oldp) واپس سچ;

        // New driver was not yet coalesced, so should always be a splice
        Dfg_راس_Splice* ثابت nSplicep = newp->srcp()->as<Dfg_راس_Splice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        اگر (oldp->vscp() == vscp) {
            اگر (!splice_Covers_پورا(nSplicep)) newp->defaultp(oldp);
            واپس سچ;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::سمتیہ<Driver> nDrivers = gatherDrivers(newp->srcp()->as<Dfg_راس_Splice>());
        UASSERT_OBJ(!nDrivers.خالی_خانے(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::سمتیہ<Driver> pDrivers;
        اگر (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } ورنہ اگر (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            واپس جھوٹ;
        }

        اگر (!pDrivers.خالی_خانے()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::سمتیہ<Driver> drivers;
            drivers.reserve(nDrivers.سائز() + pDrivers.سائز());
            std::ملاؤ(nDrivers.begin(), nDrivers.انتہا(), pDrivers.begin(), pDrivers.انتہا(),
                       std::عقب_inserter(drivers));
            // Coalesce adjacent ranges
            اگر (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            برائے (ثابت Driver& d : drivers) nSplicep->جوڑو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        اگر (oldp->defaultp() && !splice_Covers_پورا(nSplicep)) newp->defaultp(oldp->defaultp());

        //مکمل
        واپس سچ;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    منطقی synthesize_Basic_بلاک(SymTab& oSymTab, Dfg_راس*& condpr,
                              ثابت std::سمتیہ<Ast_گرہ_Stmt*>& stmtps, ثابت SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ثابت VNUser2_اندر_Use user2_اندر_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        برائے (ثابت خودکار& pair : iSymTab) {
            AstVarScope* ثابت varp = pair.پہلا;
            Dfg_راس_Var* ثابت vtxp = pair.دوسرا;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::سمتیہ<std::pair<AstVarScope*, Dfg_راس_Var*>> updates;
        برائے (Ast_گرہ_Stmt* ثابت stmtp : stmtps) {
            // Regular statements
            Ast_گرہ_تفویض* ثابت ap = VN_CAST(stmtp, گرہ_تفویض);
            اگر (ap && (VN_IS(ap, تفویض) || VN_IS(ap, تفویض_W))) {
                // Convert this assignment
                اگر (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    واپس جھوٹ;
                }
                // Apply variable updates from this statement
                برائے (ثابت خودکار& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ثابت vscp = pair.پہلا;
                    // The new, potentially partially assigned value
                    Dfg_راس_Var* ثابت newp = pair.دوسرا;
                    // Normalize drivers within this statement, bail if multidriven
                    Dfg_راس_Splice* ثابت srcp = newp->srcp()->as<Dfg_راس_Splice>();
                    std::سمتیہ<Driver> drivers = gatherDrivers(srcp);
                    ثابت منطقی single = drivers.سائز() == 1;
                    اگر (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        واپس جھوٹ;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    اگر (!single) {
                        srcp->resetDrivers();
                        برائے (ثابت Driver& d : drivers) srcp->جوڑو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    Dfg_راس_Var* ثابت oldp = vscp->user2u().سانچہ مقصد<Dfg_راس_Var*>();
                    // Inncorporate old value into the new value
                    اگر (!incorporate_پچھلا_قدر(vscp, newp, oldp)) واپس جھوٹ;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                جاری;
            }

            // Terminator branches
            اگر (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.عقب(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                Dfg_راس* condp = m_converter.convert(*m_logicp, ifp->condp());
                اگر (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    واپس جھوٹ;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                اگر (condp->چوڑائی() != 1) {
                    File_سطر* ثابت flp = condp->fileline();
                    DfgNeq* ثابت neqp = make<DfgNeq>(flp, Dfg_ڈیٹا_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->چوڑائی(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                جاری;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            واپس جھوٹ;
        }

        واپس سچ;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    خالی تفویض_راستہ_Predicates(ثابت Cfg_بلاک& bb) {
        // Nothing to do for the exit block
        اگر (bb.isExit()) واپس;

        // Get the predicate of this block
        Dfg_راس* ثابت predp = [&]() -> Dfg_راس* {
            // Entry block has no predecessors, use constant true
            اگر (bb.isEnter()) واپس make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ثابت خودکار& اندر_کنارے = bb.اندر_کنارے();
            خودکار it = اندر_کنارے.begin();
            Dfg_راس* resp = m_کنارہ_مقصد_Predicatep[static_cast<ثابت Cfg_کنارہ&>(*it)];
            جب (++it != اندر_کنارے.انتہا()) {
                DfgOr* ثابت orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_کنارہ_مقصد_Predicatep[static_cast<ثابت Cfg_کنارہ&>(*it)]);
                resp = orp;
            }
            واپس resp;
        }();

        ثابت Dfg_ڈیٹا_Type& dtype = predp->dtype();

        ثابت خودکار mkTmp = [&](File_سطر* flp, ثابت حرف* name, Dfg_راس* srcp) {
            ثابت std::string سابقہ = "_BB" + std::مقصد_string(bb.id()) + "_" + name;
            Dfg_راس_Var* ثابت tmpp = m_converter.createTmp(*m_logicp, flp, dtype, سابقہ);
            tmpp->srcp(srcp);
            واپس tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        Dfg_راس_Var* ثابت pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        اگر (!bb.isBranch()) {
            m_کنارہ_مقصد_Predicatep[bb.لیا_گیا_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            واپس;
        }

        // For branches, we need to factor in the branch condition
        Dfg_راس* ثابت condp = m_bb_مقصد_Condp[bb];
        File_سطر* ثابت flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ثابت لیا_گیا_Predp = make<DfgAnd>(flp, dtype);
            لیا_گیا_Predp->lhsp(pInp);
            لیا_گیا_Predp->rhsp(condp);
            m_کنارہ_مقصد_Predicatep[bb.لیا_گیا_Edgep()] = mkTmp(flp, "Taken", لیا_گیا_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ثابت untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ثابت notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_کنارہ_مقصد_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    منطقی checkExtWrites() {
        برائے (ثابت Dfg_راس* ثابت vtxp : m_logicp->synth()) {
            ثابت Dfg_راس_Var* ثابت varp = vtxp->cast<Dfg_راس_Var>();
            اگر (!varp) جاری;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            اگر (!varp->hasSinks()) جاری;
            اگر (!Dfg_راس_Var::isVolatile(varp->tmpForp())) جاری;
            ++m_ctx.m_synt.nonSynExtWrite;
            واپس جھوٹ;
        }
        واپس سچ;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    منطقی جوڑو_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ثابت منطقی missing = m_logicp->foreachSink([&](ثابت Dfg_راس& sink) {
            ثابت DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ثابت vscp = unresolvedp->singleSink()->as<Dfg_راس_Var>()->vscp();
            واپس !oSymTab.گنتی(vscp);
        });
        اگر (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            واپس جھوٹ;
        }

        // Add sinks to read the computed values for the target variables
        واپس !m_logicp->foreachSink([&](Dfg_راس& sink) {
            DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            ثابت Dfg_راس_Var* ثابت varp = unresolvedp->singleSink()->as<Dfg_راس_Var>();
            Dfg_راس_Var* ثابت resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            اگر (!resp->hasSinks()) {
                unresolvedp->جوڑو_Driver(resp->srcp()->as<Dfg_راس_Splice>());
                واپس جھوٹ;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            اگر (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                واپس سچ;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ثابت splicep
                = نیا DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ثابت std::سمتیہ<Driver> drivers = computePropagatedDrivers({}, resp);
            برائے (ثابت Driver& d : drivers) splicep->جوڑو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->جوڑو_Driver(splicep);
            واپس جھوٹ;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    منطقی synthesize_تفویض_W(Ast_تفویض_W* nodep) {
        ++m_ctx.m_synt.داخلہ_تفویض;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_اندراج_علامت_جدول(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        Dfg_راس* condp = خالی_اشاریہ;
        ثابت منطقی success = synthesize_Basic_بلاک(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        اگر (!success) واپس جھوٹ;

        // Check exernal writes are observed correctly
        اگر (!checkExtWrites()) واپس جھوٹ;

        // Add resolved output variable drivers
        واپس جوڑو_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    منطقی synthesizeCfg(Cfg_گراف& cfg) {
        ++m_ctx.m_synt.داخلہ_Always;

        // If there is a backward edge (loop), we can't synthesize it
        اگر (cfg.شامل_ہے_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            واپس جھوٹ;
        }

        // If it's a trivial CFG we can save on some work
        اگر (cfg.n_بلاکیں() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } ورنہ {
            // Insert two-way join blocks to aid multiplexer ordering
            اگر (cfg.داخل_کرو_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } ورنہ {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_درخت = Cfg_Dominator_درخت{cfg};
            m_کنارہ_مقصد_Predicatep = cfg.make_کنارہ_Map<Dfg_راس_Var*>();
        }

        // Initialize CfgMaps
        m_bb_مقصد_ISym_Tab = cfg.make_بلاک_Map<SymTab>();
        m_bb_مقصد_OSym_Tab = cfg.make_بلاک_Map<SymTab>();
        m_bb_مقصد_Condp = cfg.make_بلاک_Map<Dfg_راس_Var*>();

        // Synthesize all blocks
        برائے (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاک& bb = static_cast<ثابت Cfg_بلاک&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            اگر (!create_داخلہ_علامت_جدول(bb)) واپس جھوٹ;
            // Synthesize this block
            Dfg_راس* condp = خالی_اشاریہ;
            اگر (!synthesize_Basic_بلاک(m_bb_مقصد_OSym_Tab[bb], condp, bb.stmtps(), m_bb_مقصد_ISym_Tab[bb])) {
                واپس جھوٹ;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            اگر (condp) {
                File_سطر* ثابت flp = condp->fileline();
                ثابت Dfg_ڈیٹا_Type& dtype = condp->dtype();
                ثابت std::string سابقہ = "_BB" + std::مقصد_string(bb.id()) + "_Cond";
                Dfg_راس_Var* ثابت vp = m_converter.createTmp(*m_logicp, flp, dtype, سابقہ);
                vp->srcp(condp);
                m_bb_مقصد_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            تفویض_راستہ_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        اگر (!checkExtWrites()) واپس جھوٹ;

        // Add resolved output variable drivers
        واپس جوڑو_Synthesized_Output(m_bb_مقصد_OSym_Tab[cfg.باہر_نکلو()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    منطقی synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        اگر (Ast_تفویض_W* ثابت ap = VN_CAST(vtx.nodep()->stmtsp(), تفویض_W)) {
            اگر (ap->nextp()) واپس جھوٹ;
            اگر (!synthesize_تفویض_W(ap)) واپس جھوٹ;
            ++m_ctx.m_synt.synth_تفویض;
            واپس سچ;
        }

        اگر (!synthesizeCfg(vtx.cfg())) واپس جھوٹ;
        ++m_ctx.m_synt.synthAlways;
        واپس سچ;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    خالی revert(VDouble0& statCountr) {
        m_مقصد_Revert.foreach([&](Dfg_راس& vtx) {
            // Process DfgLogic
            اگر (DfgLogic* ثابت vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                برائے (Dfg_راس* ثابت p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->سیٹ_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](Dfg_راس& snk) {
                    m_مقصد_Revert.ڈالو_front(*snk.as<DfgUnresolved>());
                    واپس جھوٹ;
                });
                واپس;
            }

            // Process DfgUnresolved
            اگر (DfgUnresolved* ثابت vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_راس_Var>()->سیٹ_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreach_ماخذ([&](Dfg_راس& src) {
                    DfgLogic* ثابت lp = src.cast<DfgLogic>();
                    اگر (lp && !lp->reverted()) m_مقصد_Revert.ڈالو_front(*lp);
                    واپس جھوٹ;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                واپس;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    خالی مین() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        برائے (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            اگر (!vtxp->is<DfgUnresolved>()) جاری;
            منطقی anySelected = جھوٹ;
            منطقی anyUnselected = جھوٹ;
            vtxp->foreach_ماخذ([&](Dfg_راس& src) {
                ثابت DfgLogic& logic = *src.as<DfgLogic>();
                اگر (logic.selectedForSynthesis()) {
                    anySelected = سچ;
                } ورنہ {
                    anyUnselected = سچ;
                }
                واپس جھوٹ;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            اگر (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_راس_Var>()->سیٹ_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        برائے (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            اگر (!logicp) جاری;
            اگر (logicp->selectedForSynthesis()) جاری;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ثابت VNUser4_اندر_Use m_user4_اندر_Use;
            logicp->foreach_ماخذ([](Dfg_راس& src) {
                src.as<Dfg_راس_Var>()->vscp()->user4p(&src);
                واپس جھوٹ;
            });
            V3DfgPasses::جوڑو_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_گرہ* varp) {  //
                واپس varp->user4u().مقصد<Dfg_راس_Var*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ثابت VNUser4_اندر_Use user4_اندر_Use;
            برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
                DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
                اگر (!logicp) جاری;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ثابت خودکار debugCallback = [&]() -> خالی {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_درخت(logicp->nodep(), std::خارج);
                    debugDump("synth-lastok");
                };
                اگر (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) توڑو;

                // Synthesize it, if failed, enqueue for reversion
                اگر (!synthesize(*logicp)) {
                    logicp->سیٹ_Non_Synthesizable();
                    m_مقصد_Revert.ڈالو_front(*logicp);
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
        std::سمتیہ<Dfg_راس_Var*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ثابت Dfg_راس_Var*, Dfg_راس_Splice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        برائے (Dfg_راس_Var& var : m_dfg.varVertices()) {
            اگر (!var.srcp()) جاری;
            DfgUnresolved* ثابت unresolvedp = var.srcp()->cast<DfgUnresolved>();
            اگر (!unresolvedp) توڑو;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            Dfg_راس_Splice* ثابت resolvedp = [&]() -> Dfg_راس_Splice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                اگر (unresolvedp->nInputs() == 1) {
                    واپس unresolvedp->inputp(0)->as<Dfg_راس_Splice>();
                }
                // Otherwise gather the synthesized drivers
                std::سمتیہ<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                اگر (normalizeDrivers(var, drivers)) واپس makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_عقب(&var);
                m_مقصد_Revert.ڈالو_front(*unresolvedp);
                واپس خالی_اشاریہ;
            }();
            // Bail if multidriven
            اگر (!resolvedp) جاری;
            // Add to map for next loop
            ثابت منطقی new_اندراج = resolvedDrivers.emplace(&var, resolvedp).دوسرا;
            UASSERT_OBJ(new_اندراج, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        برائے (ثابت Dfg_راس_Var& var : m_dfg.varVertices()) {
            اگر (!var.srcp()) جاری;
            DfgUnresolved* ثابت srcp = var.srcp()->cast<DfgUnresolved>();
            اگر (!srcp) توڑو;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        برائے (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            اگر (!logicp) جاری;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            اگر (!logicp->synth().خالی_خانے()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_عقب()->delete_درخت();
            } ورنہ {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ثابت VNUser4_اندر_Use m_user4_اندر_Use;
                logicp->foreach_ماخذ([](Dfg_راس& src) {
                    src.as<Dfg_راس_Var>()->vscp()->user4p(&src);
                    واپس جھوٹ;
                });
                V3DfgPasses::جوڑو_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_گرہ* varp) {  //
                    واپس varp->user4u().مقصد<Dfg_راس_Var*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = خالی_اشاریہ;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        برائے (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            Dfg_راس_Splice* ثابت splicep = vtxp->cast<Dfg_راس_Splice>();
            اگر (!splicep) جاری;

            // Might not have a sink if the driving logic was revered, remove
            اگر (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                جاری;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            اگر (Dfg_راس* ثابت wholep = splicep->wholep()) {
                اگر (Dfg_راس_Var* ثابت varp = splicep->singleSink()->cast<Dfg_راس_Var>()) {
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
        مین();
    }

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx) {
        Ast_مقصد_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_غیر_مستعمل(dfg);
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        اگر (v3Global.opt.debugCheck()) {
            برائے (Dfg_راس& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_مضرب_Sinks(), &vtx, "Operation has multiple sinks");
                برائے (سائز_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

صحیح Ast_مقصد_Dfg_Synthesize::AstVarScopeComparator::s_vscp_Id_شمار = 0;

// Decide which DfgLogic to attempt to synthesize
ساکن خالی dfgSelectLogicForSynthesis(Dfg_گراف& dfg) {
    // If we are told to synthesize everything, we will do so ...
    اگر (v3Global.opt.fDfgSynthesizeAll()) {
        برائے (Dfg_راس& vtx : dfg.opVertices()) {
            اگر (DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) logicp->سیٹ_Selected_For_Synthesis();
        }
        واپس;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::سمتیہ<Dfg_راس_Var*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        برائے (Dfg_راس_Var& var : dfg.varVertices()) {
            اگر (!scc.at(var)) جاری;
            // This is a circular variable
            circularVarps.emplace_عقب(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    برائے (ثابت Dfg_راس_Var* ثابت varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreach_ماخذ([&](Dfg_راس& driver) {
            worklist.ڈالو_front(*driver.as<DfgLogic>());
            واپس جھوٹ;
        });
    }

    // Choose some simple special cases to always synthesize
    برائے (Dfg_راس& vtx : dfg.opVertices()) {
        DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        اگر (!logicp || worklist.شامل_ہے(*logicp)) جاری;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        اگر (logicp->drives_غیر_مستعمل_Vars()) {
            worklist.ڈالو_front(*logicp);
            جاری;
        }
        // Blocks corresponding to continuous assignments
        اگر (logicp->nodep()->keyword() == VAlwaysKwd::CONT_تفویض) {
            worklist.ڈالو_front(*logicp);
            جاری;
        }
        ثابت Cfg_گراف& cfg = logicp->cfg();
        // Straight line code with no branches
        اگر (cfg.n_بلاکیں() == 1) {
            worklist.ڈالو_front(*logicp);
            جاری;
        }
        // Blocks driving exactly 1 variable
        اگر (!logicp->has_مضرب_Sinks()) worklist.ڈالو_front(*logicp);
    }

    std::unordered_سیٹ<DfgUnresolved*> is_دیکھا;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](Dfg_راس& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.سیٹ_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](Dfg_راس& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            اگر (!is_دیکھا.داخل_کرو(unresolved).دوسرا) واپس جھوٹ;
            unresolved->foreach_ماخذ([&](Dfg_راس& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                اگر (!siblingLogic.selectedForSynthesis()) worklist.ڈالو_front(siblingLogic);
                واپس جھوٹ;
            });
            واپس جھوٹ;
        });
    });
}

خالی V3DfgPasses::synthesize(Dfg_گراف& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    Ast_مقصد_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
