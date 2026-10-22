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
سانچو <قسم_نالو T_راس, قسم_نالو T_ڳنڍ>
T_راس* make_راس(ثابت T_ڳنڍ* nodep, Dfg_گراف& dfg, ثابت Dfg_معلومات_Type& dtype) {
    أرجع جديد T_راس{dfg, nodep->fileline(), dtype};
}

سانچو <>
DfgArraySel* make_راس<DfgArraySel, AstArraySel>(ثابت AstArraySel* nodep, Dfg_گراف& dfg,
                                                  ثابت Dfg_معلومات_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    إذا (VN_IS(nodep->fromp(), Const)) أرجع خالي_اشارو;
    إذا (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خالي_اشارو;
    أرجع جديد DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
صنف Ast_منزل_Dfg_Converter final : عام VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    Dfg_گراف& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = خالي_اشارو;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::ويڪٽر<std::pair<AstVarScope*, Dfg_راس_Var*>>* m_updatesp = خالي_اشارو;

    منطقي m_مليو_Unhandled = خطأ;  // Found node not implemented as DFG or not implemented 'visit'
    منطقي m_converting = خطأ;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    سانچو <قسم_نالو راس_2, قسم_نالو... دليلون_2>
    راس_2* make(دليلون_2&&... دليلون) {
        static_assert(!std::is_بنياد_جو<Dfg_راس_Var, راس_2>::قدر, "Do not use for variables");
        static_assert(std::is_بنياد_جو<Dfg_راس, راس_2>::قدر, "'Vertex' must be a 'DfgVertex'");
        راس_2* ثابت vtxp = جديد راس_2{m_dfg, std::forward<دليلون_2>(دليلون)...};
        m_logicp->synth().emplace_پٺ(vtxp);
        أرجع vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    منطقي unhandled(Ast_ڳنڍ_اظهار* nodep) {
        // Short-circuiting if something was already unhandled
        إذا (m_مليو_Unhandled) {
            // Impure nodes cannot be represented
            إذا (!nodep->is_خالص()) {
                m_مليو_Unhandled = سچ;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        أرجع m_مليو_Unhandled;
    }

    منطقي isSupported(ثابت AstVarRef* nodep) {
        // Cannot represent cross module references
        إذا (nodep->classOrPackagep()) أرجع خطأ;
        // Check target
        أرجع V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    Dfg_راس* convertRValue(Ast_ڳنڍ_اظهار* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_مليو_Unhandled);
        m_converting = سچ;
        m_مليو_Unhandled = خطأ;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        إذا (m_مليو_Unhandled) أرجع خالي_اشارو;

        // Traversal set user2p to the equivalent vertex
        Dfg_راس* ثابت vtxp = nodep->user2u().منزل<Dfg_راس*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        أرجع vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<Dfg_راس_Splice*, uint32_t> convertLValue(Ast_ڳنڍ_اظهار* nodep) {
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            إذا (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                أرجع {خالي_اشارو, 0};
            }

            // Get (or create a new) temporary for this variable
            ثابت Dfg_راس_Var* ثابت vtxp = [&]() -> Dfg_راس_Var* {
                // The variable being assigned
                AstVarScope* ثابت vscp = vrefp->varScopep();

                // Find existing one, if any
                لكل (ثابت خودڪار& pair : *m_updatesp) {
                    إذا (pair.پهريون == vscp) أرجع pair.ٻيو;
                }

                // Create new one
                Dfg_راس_Var* ثابت newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_پٺ(vscp, newp);

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
            أرجع {vtxp->srcp()->as<Dfg_راس_Splice>(), 0};
        }

        إذا (ثابت AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            إذا (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                أرجع {خالي_اشارو, 0};
            }
            uint32_t lsb = lsbp->منزل_UInt();

            // Convert the 'fromp' sub-expression
            ثابت خودڪار pair = convertLValue(selp->fromp());
            إذا (!pair.پهريون) أرجع {خالي_اشارو, 0};
            DfgSplicePacked* ثابت splicep = pair.پهريون->سانچو as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.ٻيو;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            إذا (lsb + static_cast<uint32_t>(selp->ويڪر_Const()) > splicep->سائيز()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                أرجع {خالي_اشارو, 0};
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
                أرجع {خالي_اشارو, 0};
            }
            uint32_t اشاريو = indexp->منزل_UInt();

            // Convert the 'fromp' sub-expression
            ثابت خودڪار pair = convertLValue(aselp->fromp());
            إذا (!pair.پهريون) أرجع {خالي_اشارو, 0};
            DfgSpliceArray* ثابت splicep = pair.پهريون->سانچو as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            اشاريو += pair.ٻيو;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            إذا (اشاريو + 1U > splicep->سائيز()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                أرجع {خالي_اشارو, 0};
            }

            // Ensure the Splice driver exists for this element
            إذا (!splicep->driverAt(اشاريو)) {
                File_سٽ* ثابت flp = nodep->fileline();
                // This should never fail
                ثابت Dfg_معلومات_Type& dtype = *Dfg_معلومات_Type::fromAst(nodep->dtypep());
                إذا (dtype.isPacked()) {
                    DfgSplicePacked* ثابت newp = make<DfgSplicePacked>(flp, dtype);
                    ثابت Dfg_معلومات_Type& uaDtype = Dfg_معلومات_Type::array(dtype, 1);
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->جوڙيو_Driver(uap, اشاريو, flp);
                } وإلا إذا (dtype.isArray()) {
                    DfgSpliceArray* ثابت newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->جوڙيو_Driver(newp, اشاريو, flp);
                } وإلا {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            Dfg_راس* driverp = splicep->driverAt(اشاريو);
            إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            أرجع {driverp->as<Dfg_راس_Splice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        أرجع {خالي_اشارو, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    منطقي convertAssignment(File_سٽ* flp, Ast_ڳنڍ_اظهار* lhsp, Dfg_راس* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        بنية Assignment final {
            Dfg_راس_Splice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            Dfg_راس* m_rhsp;  // Vertex driving that position
            Assignment() = ميٽيو;
            Assignment(Dfg_راس_Splice* lhsp, uint32_t idx, Dfg_راس* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        إذا (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), ڳنڍ_اظهار);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        إذا (VN_IS(lhsp, Concat) && !vtxp->is<Dfg_راس_Var>() && !vtxp->is<DfgConst>()) {
            Dfg_راس_Var* ثابت tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::ويڪٽر<Assignment> assignments;
        ثابت std::function<منطقي(Ast_ڳنڍ_اظهار*, uint32_t)> convertAllLValues
            = [&](Ast_ڳنڍ_اظهار* subp, uint32_t lsb) -> منطقي {
            // Concatenation on the LHS, convert each part
            إذا (AstConcat* ثابت concatp = VN_CAST(subp, Concat)) {
                Ast_ڳنڍ_اظهار* ثابت cRhsp = concatp->rhsp();
                Ast_ڳنڍ_اظهار* ثابت cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                إذا (!convertAllLValues(cRhsp, lsb)) أرجع خطأ;
                // Convert Left of concat
                أرجع convertAllLValues(cLhsp, lsb + cRhsp->ويڪر());
            }

            // Non-concatenation, convert the LValue
            ثابت خودڪار pair = convertLValue(subp);
            إذا (!pair.پهريون) أرجع خطأ;

            // If whole lhs, just use it
            إذا (subp == lhsp) {
                assignments.emplace_پٺ(pair.پهريون, pair.ٻيو, vtxp);
                أرجع سچ;
            }

            // Otherwise select the relevant bits
            ثابت Dfg_معلومات_Type& dtype = *Dfg_معلومات_Type::fromAst(subp->dtypep());
            DfgSel* ثابت selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_پٺ(pair.پهريون, pair.ٻيو, selp);
            أرجع سچ;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        إذا (!convertAllLValues(lhsp, 0)) أرجع خطأ;

        // All successful, connect the drivers
        لكل (ثابت Assignment& شي_2 : assignments) {
            إذا (DfgSplicePacked* ثابت spp = شي_2.m_lhsp->سانچو cast<DfgSplicePacked>()) {
                spp->جوڙيو_Driver(شي_2.m_rhsp, شي_2.m_idx, flp);
            } وإلا إذا (DfgSpliceArray* ثابت sap = شي_2.m_lhsp->سانچو cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ثابت Dfg_معلومات_Type& rDt = شي_2.m_rhsp->dtype();
                إذا (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, Dfg_معلومات_Type::array(rDt, 1));
                    uap->srcp(شي_2.m_rhsp);
                    sap->جوڙيو_Driver(uap, شي_2.m_idx, flp);
                } وإلا {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->جوڙيو_Driver(شي_2.m_rhsp, شي_2.m_idx, flp);
                }
            } وإلا {
                شي_2.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        أرجع سچ;
    }

    // VISITORS

    // Unhandled node
    فراغ گهمو(Ast_ڳنڍ* /*nodep*/) override {
        إذا (!m_مليو_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_مليو_Unhandled = سچ;
    }

    // Expressions - mostly auto generated, but a few special ones
    فراغ گهمو(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        إذا (!isSupported(nodep)) {
            m_مليو_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepVarRef;
            أرجع;
        }

        // Variable should have been bound before starting conversion
        Dfg_راس* ثابت vtxp = nodep->varScopep()->user2u().سانچو منزل<Dfg_راس_Var*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    فراغ گهمو(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        إذا (nodep->ويڪر() != nodep->انگ().ويڪر()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_عدد انگ{nodep, nodep->ويڪر()};
            انگ.opSel(nodep->انگ(), nodep->ويڪر() - 1, 0);
            Dfg_راس* ثابت vtxp = make<DfgConst>(nodep->fileline(), انگ);
            nodep->user2p(vtxp);
        } وإلا {
            Dfg_راس* ثابت vtxp = make<DfgConst>(nodep->fileline(), nodep->انگ());
            nodep->user2p(vtxp);
        }
    }
    فراغ گهمو(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_معلومات_Type* ثابت dtypep = Dfg_معلومات_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_مليو_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        Dfg_راس* ثابت vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    فراغ گهمو(Ast_ميلاپ_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_معلومات_Type* ثابت dtypep = Dfg_معلومات_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_مليو_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        iterate(nodep->lhsp());
        إذا (m_مليو_Unhandled) أرجع;
        iterate(nodep->matchp());
        إذا (m_مليو_Unhandled) أرجع;

        File_سٽ* ثابت flp = nodep->fileline();
        Dfg_ميلاپ_Masked* ثابت vtxp = make<Dfg_ميلاپ_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().منزل<Dfg_راس*>());
        vtxp->matchp(nodep->matchp()->user2u().منزل<Dfg_راس*>());
        nodep->user2p(vtxp);
    }
    فراغ گهمو(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_معلومات_Type* ثابت dtypep = Dfg_معلومات_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_مليو_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        iterate(nodep->srcp());
        إذا (m_مليو_Unhandled) أرجع;

        File_سٽ* ثابت flp = nodep->fileline();
        DfgRep* ثابت vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().منزل<Dfg_راس*>());
        nodep->user2p(vtxp);
    }
    فراغ گهمو(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        إذا (unhandled(nodep)) أرجع;

        ثابت Dfg_معلومات_Type* ثابت dtypep = Dfg_معلومات_Type::fromAst(nodep->dtypep());
        إذا (!dtypep) {
            m_مليو_Unhandled = سچ;
            ++m_ctx.m_conv.nonRepDType;
            أرجع;
        }

        iterate(nodep->fromp());
        إذا (m_مليو_Unhandled) أرجع;

        File_سٽ* ثابت flp = nodep->fileline();
        Dfg_راس* vtxp = خالي_اشارو;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت uint32_t lsb = constp->منزل_UInt();
            ثابت uint32_t msb = lsb + nodep->ويڪر_Const() - 1;
            Dfg_راس* ثابت fromp = nodep->fromp()->user2u().منزل<Dfg_راس*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            إذا (msb >= fromp->سائيز()) {
                m_مليو_Unhandled = سچ;
                ++m_ctx.m_conv.nonRepOOBSel;
                أرجع;
            }
            DfgSel* ثابت selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } وإلا {
            iterate(nodep->lsbp());
            إذا (m_مليو_Unhandled) أرجع;
            DfgMux* ثابت muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().منزل<Dfg_راس*>());
            muxp->lsbp(nodep->lsbp()->user2u().منزل<Dfg_راس*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#ضمن "V3Dfg__gen_ast_to_dfg.h"

عام:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    Dfg_راس_Var* createTmp(DfgLogic& logic, File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype,
                            ثابت std::string& اڳياڙي) {
        Dfg_راس_Var* ثابت vtxp = m_dfg.makeNewVar(flp, اڳياڙي, dtype, logic.scopep());
        logic.synth().emplace_پٺ(vtxp);
        vtxp->vscp()->varp()->is_اندروني(سچ);
        vtxp->tmpForp(vtxp->vscp());
        أرجع vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    Dfg_راس_Var* createTmp(DfgLogic& logic, AstVarScope* vscp, ثابت std::string& اڳياڙي) {
        AstVar* ثابت astVarp = vscp->varp();
        File_سٽ* ثابت flp = astVarp->fileline();
        ثابت Dfg_معلومات_Type& dtype = *Dfg_معلومات_Type::fromAst(astVarp->dtypep());
        ثابت std::string prfx = اڳياڙي + "_" + astVarp->name();
        Dfg_راس_Var* ثابت vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        أرجع vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    منطقي convert(std::ويڪٽر<std::pair<AstVarScope*, Dfg_راس_Var*>>& updates, DfgLogic& vtx,
                 Ast_ڳنڍ_سونپيو* nodep) {
        UASSERT_OBJ(VN_IS(nodep, سونپيو) || VN_IS(nodep, سونپيو_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.خالي_جايون(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.داخلا_Assignments;
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();
        // Check data types are compatible.
        ثابت Dfg_معلومات_Type* ثابت lDtypep = Dfg_معلومات_Type::fromAst(lhsp->dtypep());
        ثابت Dfg_معلومات_Type* ثابت rDtypep = Dfg_معلومات_Type::fromAst(rhsp->dtypep());
        إذا (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            أرجع خطأ;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        إذا (rDtypep->isArray()) {
            إذا (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_وڻ(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                أرجع خطأ;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->ويڪر() == rhsp->ويڪر(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        Dfg_راس* ثابت rVtxp = convertRValue(rhsp);
        إذا (!rVtxp) أرجع خطأ;
        // Connect the RHS vertex to the LHS targets
        ثابت منطقي success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        إذا (success) ++m_ctx.m_conv.representable;
        أرجع success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    Dfg_راس* convert(DfgLogic& vtx, Ast_ڳنڍ_اظهار* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.داخلا_Expressions;
        Dfg_راس* ثابت vtxp = convertRValue(nodep);
        إذا (vtxp) ++m_ctx.m_conv.representable;
        أرجع vtxp;
    }

    // CONSTRUCTOR
    Ast_منزل_Dfg_Converter(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

صنف Ast_منزل_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    بنية AstVarScopeComparator final {
        ساكن صحيح s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        منطقي عامل()(AstVarScope* lhs, AstVarScope* rhs) ثابت {
            إذا (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            إذا (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            أرجع lhs->user4() < rhs->user4();
        }
    };
    استعمال SymTab = std::map<AstVarScope*, Dfg_راس_Var*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    بنية Driver final {
        Dfg_راس* m_vtxp = خالي_اشارو;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_سٽ* m_flp = خالي_اشارو;  // Location of driver in source

        Driver() = افتراضي;
        Driver(Dfg_راس* vtxp, uint32_t lo, File_سٽ* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->سائيز() - 1U}
            , m_flp{flp} {}
        عامل منطقي() ثابت { أرجع m_vtxp != خالي_اشارو; }

        منطقي عامل<(ثابت Driver& other) ثابت {
            إذا (m_lo != other.m_lo) أرجع m_lo < other.m_lo;
            إذا (m_hi != other.m_hi) أرجع m_hi < other.m_hi;
            أرجع m_flp->operatorCompare(*other.m_flp) < 0;
        }

        منطقي عامل<=(ثابت Driver& other) ثابت { أرجع !(other < *هذا); }
    };

    // STATE - Persistent
    Dfg_گراف& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_منزل_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_منزل_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = خالي_اشارو;  // Current logic vertex we are synthesizing
    Cfg_بلاڪ_Map<SymTab> m_bb_منزل_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_بلاڪ_Map<SymTab> m_bb_منزل_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_بلاڪ_Map<Dfg_راس_Var*> m_bb_منزل_Condp;  // Map from CfgBlock ->  terminating branch condition
    Cfg_ڪنارو_Map<Dfg_راس_Var*> m_ڪنارو_منزل_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_وڻ m_dom_وڻ;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = خالي_اشارو;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_سيٽ<ثابت Dfg_راس*>> m_debugOSrcConep{خالي_اشارو};

    // METHODS

    // Dump current graph for debugging ...
    فراغ debugDump(ثابت حرف* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        إذا (VL_UNLIKELY(m_debugLogicp)) {
            std::ويڪٽر<ثابت Dfg_راس*> outputs;
            m_debugLogicp->foreachSink([&outputs](ثابت Dfg_راس& v) {
                outputs.emplace_پٺ(v.singleSink()->as<Dfg_راس_Var>());
                أرجع خطأ;
            });
            m_debugOSrcConep = m_dfg.ذريعو_Cone(outputs);
        }

        إذا (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            إذا (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ثابت Dfg_راس& v) -> منطقي {
                    أرجع m_debugOSrcConep->ڳڻپ(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    سانچو <قسم_نالو راس_2, قسم_نالو... دليلون_2>
    راس_2* make(دليلون_2&&... دليلون) {
        static_assert(!std::is_بنياد_جو<Dfg_راس_Var, راس_2>::قدر, "Do not use for variables");
        static_assert(std::is_بنياد_جو<Dfg_راس, راس_2>::قدر, "'Vertex' must be a 'DfgVertex'");
        راس_2* ثابت vtxp = جديد راس_2{m_dfg, std::forward<دليلون_2>(دليلون)...};
        إذا (m_logicp) m_logicp->synth().emplace_پٺ(vtxp);
        أرجع vtxp;
    }

    // Gather all drivers of a resolved variable
    ساكن std::ويڪٽر<Driver> gatherDrivers(Dfg_راس_Splice* vtxp) {
        // Collect them all, check if they are sorted
        std::ويڪٽر<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        منطقي sorted = سچ;
        vtxp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سٽ* flp) {
            // Collect the driver
            drivers.emplace_پٺ(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ثابت سائيز_t n = drivers.سائيز();
            إذا (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = خطأ;
            أرجع خطأ;
        });

        // Sort if unsorted
        إذا (!sorted) std::stable_ترتيب_ڏيو(drivers.begin(), drivers.پڇاڙي());

        //پورو
        أرجع drivers;
    }

    // Returns true if the driver supplies a complete array element.
    ساكن منطقي driver_Covers_سڄو_Element(ثابت Driver& driver) {
        ثابت DfgUnitArray* ثابت unitp = driver.m_vtxp->cast<DfgUnitArray>();
        إذا (!unitp) أرجع خطأ;
        إذا (Dfg_راس_Splice* ثابت splicep = unitp->srcp()->cast<Dfg_راس_Splice>()) {
            أرجع splicep->wholep();
        }
        أرجع سچ;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    ساكن منطقي splice_Covers_سڄو(Dfg_راس_Splice* ثابت splicep) {
        إذا (splicep->wholep()) أرجع سچ;
        إذا (splicep->isPacked()) أرجع خطأ;
        uint32_t ايندڙ = 0;
        لكل (ثابت Driver& driver : gatherDrivers(splicep)) {
            إذا (driver.m_lo != ايندڙ || !driver_Covers_سڄو_Element(driver)) أرجع خطأ;
            ايندڙ = driver.m_hi + 1;
        }
        أرجع ايندڙ == splicep->سائيز();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    ساكن منطقي شامل_آهي_Tri_Lowered_Var(Dfg_راس* rootp) {
        std::ويڪٽر<ثابت Dfg_راس*> ڍير;
        std::ويڪٽر<ثابت Dfg_راس*> گهميل;
        ڍير.emplace_پٺ(rootp);
        طالما (!ڍير.خالي_جايون()) {
            ثابت Dfg_راس* ثابت vtxp = ڍير.پٺ();
            ڍير.ڪڍو_پٺ();
            إذا (std::ڳوليو(گهميل.begin(), گهميل.پڇاڙي(), vtxp) != گهميل.پڇاڙي()) استمر;
            گهميل.emplace_پٺ(vtxp);
            إذا (ثابت Dfg_راس_Var* ثابت varp = vtxp->cast<Dfg_راس_Var>()) {
                إذا (varp->vscp()->varp()->dfgTriLowered()) أرجع سچ;
            }
            vtxp->foreach_ذريعو([&](ثابت Dfg_راس& src) {
                ڍير.emplace_پٺ(&src);
                أرجع خطأ;
            });
        }
        أرجع خطأ;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    ساكن منطقي is_سڌو_Var_Driver(ثابت Dfg_راس* vtxp) { أرجع vtxp->is<Dfg_راس_Var>(); }

    // Gather all synthesized drivers of an unresolved variable
    ساكن std::ويڪٽر<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::ويڪٽر<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ثابت std::function<فراغ(Dfg_راس*, uint32_t, File_سٽ*)> gather
            = [&](Dfg_راس* vtxp, uint32_t lo, File_سٽ* flp) -> فراغ {
            إذا (DfgConcat* ثابت concatp = vtxp->cast<DfgConcat>()) {
                Dfg_راس* ثابت rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                Dfg_راس* ثابت lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->ويڪر(), lhsp->fileline());
                أرجع;
            }
            drivers.emplace_پٺ(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreach_ذريعو([&](Dfg_راس& src) {
            // Can ignore the original DfgLogic
            إذا (src.is<DfgLogic>()) أرجع خطأ;
            // Synthesized drivers must be a splice at this point
            Dfg_راس_Splice* ثابت splicep = src.as<Dfg_راس_Splice>();
            // Collect the driver
            splicep->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سٽ* flp) {
                gather(&src, lo, flp);
                أرجع خطأ;
            });
            أرجع خطأ;
        });

        // Sort the drivers
        std::stable_ترتيب_ڏيو(drivers.begin(), drivers.پڇاڙي());

        //پورو
        أرجع drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<منطقي, منطقي> combineDrivers(Dfg_راس_Var& var, ثابت std::string& sub,  //
                                         Driver& a, ثابت Driver& b) {
        // We can only combine array drivers ...
        إذا (a.m_vtxp->isPacked()) أرجع {خطأ, خطأ};
        // ... that both drive a single element ...
        إذا (a.m_lo != b.m_lo) أرجع {خطأ, خطأ};
        ثابت DfgUnitArray* ثابت aUap = a.m_vtxp->سانچو cast<DfgUnitArray>();
        إذا (!aUap) أرجع {خطأ, خطأ};
        ثابت DfgUnitArray* ثابت bUap = b.m_vtxp->سانچو cast<DfgUnitArray>();
        إذا (!bUap) أرجع {خطأ, خطأ};
        // ... and are themeselves partial
        DfgSplicePacked* ثابت aSp = aUap->srcp()->سانچو cast<DfgSplicePacked>();
        إذا (!aSp) أرجع {خطأ, خطأ};
        DfgSplicePacked* ثابت bSp = bUap->srcp()->سانچو cast<DfgSplicePacked>();
        إذا (!bSp) أرجع {خطأ, خطأ};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::ويڪٽر<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::ويڪٽر<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::ويڪٽر<Driver> abDrivers;
        abDrivers.reserve(aDrivers.سائيز() + bDrivers.سائيز());
        std::ملايو(  //
            aDrivers.begin(), aDrivers.پڇاڙي(),  //
            bDrivers.begin(), bDrivers.پڇاڙي(),  //
            std::پٺ_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        إذا (!normalizeDrivers(var, abDrivers, sub + "[" + std::منزل_string(a.m_lo) + "]")) {
            أرجع {خطأ, سچ};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_سٽ* ثابت flp = var.fileline();
        DfgSplicePacked* ثابت splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        لكل (ثابت Driver& d : abDrivers) splicep->جوڙيو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        أرجع {سچ, خطأ};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    منطقي normalizeDrivers(Dfg_راس_Var& var, std::ويڪٽر<Driver>& drivers,
                          ثابت std::string& sub = "") {
        إذا (drivers.خالي_جايون()) أرجع سچ;

        // What type of values are we combining
        ثابت منطقي isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        منطقي multiDriven = خطأ;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        سائيز_t i = 0;
        لكل (سائيز_t j = 1; j < drivers.سائيز();) {
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
            multiDriven = سچ;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            إذا (warned) استمر;

            // The variable to warn on
            AstVarScope* ثابت vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            إذا (vscp->varp()->is_استعمال_ٿيل_Loop_Idx()) استمر;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            إذا (vscp->varp()->dfgAllowMultidriveTri()) {
                ثابت منطقي iTri = شامل_آهي_Tri_Lowered_Var(iD.m_vtxp);
                ثابت منطقي jTri = شامل_آهي_Tri_Lowered_Var(jD.m_vtxp);
                ثابت منطقي triPair = iTri && jTri;
                ثابت منطقي triAndBridge = (iTri && is_سڌو_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_سڌو_Var_Driver(iD.m_vtxp));
                إذا (triPair || triAndBridge) استمر;
            }

            // Warn the user now
            ثابت std::string lo = std::منزل_string(jD.m_lo);
            ثابت std::string hi = std::منزل_string(std::min(iD.m_hi, jD.m_hi));
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
        إذا (!multiDriven && isPacked) coalesceDrivers(drivers);

        أرجع !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    فراغ coalesceDrivers(std::ويڪٽر<Driver>& drivers) {
        UASSERT(!drivers.خالي_جايون(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        سائيز_t i = 0;
        لكل (سائيز_t j = 1; j < drivers.سائيز();) {
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
            ثابت Dfg_معلومات_Type& dt = Dfg_معلومات_Type::packed(iD.m_vtxp->ويڪر() + jD.m_vtxp->ويڪر());
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
    Dfg_راس_Splice* makeSplice(Dfg_راس_Var& var, ثابت std::ويڪٽر<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.خالي_جايون(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        Dfg_راس_Splice* splicep = خالي_اشارو;
        إذا (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } وإلا إذا (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } وإلا {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        لكل (ثابت Driver& d : newDrivers) splicep->جوڙيو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        أرجع splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    فراغ initialize_داخلا_نشان_جدول(SymTab& iSymTab) {
        m_logicp->foreach_ذريعو([&](Dfg_راس& src) {
            Dfg_راس_Var* ثابت vvp = src.as<Dfg_راس_Var>();
            iSymTab[vvp->vscp()] = vvp;
            أرجع خطأ;
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
        إذا (thenp == elsep) أرجع thenp;

        // We can't join the input variable just yet, so bail
        إذا (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_داخلا;
            أرجع خالي_اشارو;
        }
        إذا (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_داخلا;
            أرجع خالي_اشارو;
        }

        // Can't do arrays yet
        إذا (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            أرجع خالي_اشارو;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        Dfg_راس* ثابت tDefaultp = thenp->defaultp();
        std::ويڪٽر<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<Dfg_راس_Splice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        Dfg_راس* ثابت eDefaultp = elsep->defaultp();
        std::ويڪٽر<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<Dfg_راس_Splice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_سٽ* ثابت flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        Dfg_راس_Var* ثابت joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        Dfg_راس_Splice* ثابت joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        إذا (tDrivers.سائيز() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->ويڪر() - 1  //
            && eDrivers.سائيز() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->ويڪر() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ثابت condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->جوڙيو_Driver(condp, 0, tDrivers[0].m_flp);

            //پورو
            أرجع joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        إذا (tDrivers.سائيز() != eDrivers.سائيز()) {
            ++m_ctx.m_synt.nonSynLatch;
            أرجع خالي_اشارو;
        }

        لكل (سائيز_t i = 0; i < tDrivers.سائيز(); ++i) {
            ثابت Driver& tDriver = tDrivers[i];
            ثابت Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            إذا (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                أرجع خالي_اشارو;
            }

            ثابت Dfg_معلومات_Type& dtype = Dfg_معلومات_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
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
            joinSplicep->جوڙيو_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        إذا (tDefaultp) joinp->defaultp(tDefaultp);

        //پورو
        أرجع joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    منطقي join_نشان_Tables(SymTab& elseSymTab, Dfg_راس_Var* predicatep, ثابت SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::ويڪٽر<AstVarScope*> منزل_Remove;

        // Join each symbol
        لكل (std::pair<AstVarScope* ثابت, Dfg_راس_Var*>& pair : elseSymTab) {
            AstVarScope* ثابت varp = pair.پهريون;
            // Find same variable on the else path
            ثابت خودڪار it = thenSymTab.ڳوليو(varp);
            // Record for removal if not assigned on both paths
            إذا (it == thenSymTab.پڇاڙي()) {
                منزل_Remove.emplace_پٺ(varp);
                استمر;
            }
            // Join paths with the predicate
            Dfg_راس_Var* ثابت thenp = it->ٻيو;
            Dfg_راس_Var* ثابت elsep = pair.ٻيو;
            Dfg_راس_Var* ثابت newp = joinDrivers(varp, predicatep, thenp, elsep);
            إذا (!newp) أرجع خطأ;
            pair.ٻيو = newp;
        }

        // Remove variables not assigned on both paths
        لكل (AstVarScope* ثابت varp : منزل_Remove) elseSymTab.erase(varp);

        //پورو
        أرجع سچ;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<Dfg_راس_Var*, ثابت Cfg_بلاڪ*, ثابت Cfg_بلاڪ*>  //
    how_منزل_Join(ثابت Cfg_ڪنارو* ثابت ap, ثابت Cfg_ڪنارو* ثابت bp) {
        // Find the closest common dominator of the two paths
        ثابت Cfg_بلاڪ* ثابت domp = m_dom_وڻ.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ثابت Cfg_ڪنارو* ثابت ورتل_Edgep = domp->ورتل_Edgep();
        ثابت Cfg_ڪنارو* ثابت untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        إذا (ورتل_Edgep == ap || m_dom_وڻ.dominates(ورتل_Edgep->dstp(), ap->srcp())) {
            إذا (untknEdgep == bp || m_dom_وڻ.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                أرجع std::make_tuple(m_bb_منزل_Condp[domp], ap->srcp(), bp->srcp());
            }
        } وإلا إذا (ورتل_Edgep == bp || m_dom_وڻ.dominates(ورتل_Edgep->dstp(), bp->srcp())) {
            إذا (untknEdgep == ap || m_dom_وڻ.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                أرجع std::make_tuple(m_bb_منزل_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_رستو_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        أرجع std::make_tuple(m_ڪنارو_منزل_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    منطقي create_داخلا_نشان_جدول(ثابت Cfg_بلاڪ& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_منزل_ISym_Tab[bb];

        // Input symbol table of entry block is special
        إذا (bb.isEnter()) {
            initialize_داخلا_نشان_جدول(joined);
            أرجع سچ;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.خالي_جايون(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        إذا (!bb.isJoin()) {
            joined = m_bb_منزل_OSym_Tab[bb.پهريون_Predecessorp()];
            أرجع سچ;
        }

        // We also have a simpler job if there are 2 predecessors
        إذا (bb.isTwoWayJoin()) {
            Dfg_راس_Var* predicatep = خالي_اشارو;
            ثابت Cfg_بلاڪ* thenp = خالي_اشارو;
            ثابت Cfg_بلاڪ* elsep = خالي_اشارو;
            std::tie(predicatep, thenp, elsep)
                = how_منزل_Join(bb.پهريون_Predecessor_Edgep(), bb.آخري_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_منزل_OSym_Tab[elsep];
            // Join with then
            أرجع join_نشان_Tables(joined, predicatep, m_bb_منزل_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        بنية Predecessor final {
            ثابت Cfg_بلاڪ* m_bbp;  // Predeccessor block
            Dfg_راس_Var* m_predicatep;  // Predicate predecessor reached this block with
            ثابت SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = ميٽيو;
            Predecessor(ثابت Cfg_بلاڪ* bbp, Dfg_راس_Var* predicatep, ثابت SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ثابت std::ويڪٽر<Predecessor> predecessors = [&]() {
            std::ويڪٽر<Predecessor> res;
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : bb.اندريون_ڪنارا()) {
                ثابت Cfg_ڪنارو& cfg_ڪنارو = static_cast<ثابت Cfg_ڪنارو&>(ڪنارو);
                ثابت Cfg_بلاڪ* ثابت predecessorp = cfg_ڪنارو.srcp();
                Dfg_راس_Var* ثابت predicatep = m_ڪنارو_منزل_Predicatep[cfg_ڪنارو];
                ثابت SymTab* ثابت oSymTabp = &m_bb_منزل_OSym_Tab[predecessorp];
                res.emplace_پٺ(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::ترتيب_ڏيو(res.begin(), res.پڇاڙي(), [](ثابت Predecessor& a, ثابت Predecessor& b) {  //
                أرجع *a.m_bbp > *b.m_bbp;
            });
            أرجع res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        لكل (سائيز_t i = 1; i < predecessors.سائيز(); ++i) {
            Dfg_راس_Var* ثابت predicatep = predecessors[i].m_predicatep;
            ثابت SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            إذا (!join_نشان_Tables(joined, predicatep, oSymTab)) أرجع خطأ;
        }

        أرجع سچ;
    }

    std::ويڪٽر<Driver> computePropagatedDrivers(ثابت std::ويڪٽر<Driver>& newDrivers,
                                                 Dfg_راس_Var* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ثابت std::ويڪٽر<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_راس_Splice>());
        UASSERT_OBJ(!oldDrivers.خالي_جايون(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::ويڪٽر<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ثابت خودڪار جوڙيو_Old_Driver = [&](File_سٽ* ثابت flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.خالي_جايون() || lsb > propagatedDrivers.پٺ().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ثابت selp = make<DfgSel>(flp, Dfg_معلومات_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_پٺ(selp, lsb, flp);
        };

        // Incorporate old drivers
        لكل (ثابت Driver& oDriver : oldDrivers) {
            File_سٽ* ثابت flp = oDriver.m_flp;
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
                إذا (oldLo < nDriver.m_lo) جوڙيو_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                إذا (oldLo > oldHi) اكسر;
            }

            // Insert remaining bits if any
            إذا (oldHi >= oldLo) جوڙيو_Old_Driver(flp, oldHi, oldLo);
        }

        أرجع propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    منطقي computePropagatedArrayDrivers(ثابت std::ويڪٽر<Driver>& newDrivers,
                                       Dfg_راس_Var* ثابت oldp,
                                       std::ويڪٽر<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        ساكن constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        إذا (oldp->سائيز() > MAX_ARRAY_ELEMENTS) أرجع خطأ;
        لكل (ثابت Driver& driver : newDrivers) {
            إذا (!driver_Covers_سڄو_Element(driver)) أرجع خطأ;
        }
        ثابت std::ويڪٽر<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_راس_Splice>());
        UASSERT_OBJ(!oldDrivers.خالي_جايون(), oldp, "Should have a proper driver");
        لكل (ثابت Driver& driver : oldDrivers) {
            إذا (!driver_Covers_سڄو_Element(driver)) أرجع خطأ;
        }

        propagatedDrivers.reserve(oldDrivers.سائيز());
        خودڪار nIt = newDrivers.begin();
        لكل (ثابت Driver& oDriver : oldDrivers) {
            طالما (nIt != newDrivers.پڇاڙي() && nIt->m_lo < oDriver.m_lo) ++nIt;
            إذا (nIt != newDrivers.پڇاڙي() && nIt->m_lo == oDriver.m_lo) استمر;

            File_سٽ* ثابت flp = oDriver.m_flp;
            ثابت DfgUnitArray* ثابت oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ثابت selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<سائيز_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ثابت newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_پٺ(newUnitp, oDriver.m_lo, flp);
        }
        أرجع سچ;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    منطقي incorporate_اڳيون_قدر(AstVarScope* vscp, Dfg_راس_Var* newp, Dfg_راس_Var* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        إذا (!oldp) أرجع سچ;

        // New driver was not yet coalesced, so should always be a splice
        Dfg_راس_Splice* ثابت nSplicep = newp->srcp()->as<Dfg_راس_Splice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        إذا (oldp->vscp() == vscp) {
            إذا (!splice_Covers_سڄو(nSplicep)) newp->defaultp(oldp);
            أرجع سچ;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::ويڪٽر<Driver> nDrivers = gatherDrivers(newp->srcp()->as<Dfg_راس_Splice>());
        UASSERT_OBJ(!nDrivers.خالي_جايون(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::ويڪٽر<Driver> pDrivers;
        إذا (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } وإلا إذا (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            أرجع خطأ;
        }

        إذا (!pDrivers.خالي_جايون()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::ويڪٽر<Driver> drivers;
            drivers.reserve(nDrivers.سائيز() + pDrivers.سائيز());
            std::ملايو(nDrivers.begin(), nDrivers.پڇاڙي(), pDrivers.begin(), pDrivers.پڇاڙي(),
                       std::پٺ_inserter(drivers));
            // Coalesce adjacent ranges
            إذا (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            لكل (ثابت Driver& d : drivers) nSplicep->جوڙيو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        إذا (oldp->defaultp() && !splice_Covers_سڄو(nSplicep)) newp->defaultp(oldp->defaultp());

        //پورو
        أرجع سچ;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    منطقي synthesize_Basic_بلاڪ(SymTab& oSymTab, Dfg_راس*& condpr,
                              ثابت std::ويڪٽر<Ast_ڳنڍ_Stmt*>& stmtps, ثابت SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ثابت VNUser2_اندريون_Use user2_اندريون_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        لكل (ثابت خودڪار& pair : iSymTab) {
            AstVarScope* ثابت varp = pair.پهريون;
            Dfg_راس_Var* ثابت vtxp = pair.ٻيو;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::ويڪٽر<std::pair<AstVarScope*, Dfg_راس_Var*>> updates;
        لكل (Ast_ڳنڍ_Stmt* ثابت stmtp : stmtps) {
            // Regular statements
            Ast_ڳنڍ_سونپيو* ثابت ap = VN_CAST(stmtp, ڳنڍ_سونپيو);
            إذا (ap && (VN_IS(ap, سونپيو) || VN_IS(ap, سونپيو_W))) {
                // Convert this assignment
                إذا (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    أرجع خطأ;
                }
                // Apply variable updates from this statement
                لكل (ثابت خودڪار& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ثابت vscp = pair.پهريون;
                    // The new, potentially partially assigned value
                    Dfg_راس_Var* ثابت newp = pair.ٻيو;
                    // Normalize drivers within this statement, bail if multidriven
                    Dfg_راس_Splice* ثابت srcp = newp->srcp()->as<Dfg_راس_Splice>();
                    std::ويڪٽر<Driver> drivers = gatherDrivers(srcp);
                    ثابت منطقي single = drivers.سائيز() == 1;
                    إذا (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        أرجع خطأ;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    إذا (!single) {
                        srcp->resetDrivers();
                        لكل (ثابت Driver& d : drivers) srcp->جوڙيو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    Dfg_راس_Var* ثابت oldp = vscp->user2u().سانچو منزل<Dfg_راس_Var*>();
                    // Inncorporate old value into the new value
                    إذا (!incorporate_اڳيون_قدر(vscp, newp, oldp)) أرجع خطأ;
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
                UASSERT_OBJ(ifp == stmtps.پٺ(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                Dfg_راس* condp = m_converter.convert(*m_logicp, ifp->condp());
                إذا (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    أرجع خطأ;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                إذا (condp->ويڪر() != 1) {
                    File_سٽ* ثابت flp = condp->fileline();
                    DfgNeq* ثابت neqp = make<DfgNeq>(flp, Dfg_معلومات_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->ويڪر(), 0U));
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

        أرجع سچ;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    فراغ سونپيو_رستو_Predicates(ثابت Cfg_بلاڪ& bb) {
        // Nothing to do for the exit block
        إذا (bb.isExit()) أرجع;

        // Get the predicate of this block
        Dfg_راس* ثابت predp = [&]() -> Dfg_راس* {
            // Entry block has no predecessors, use constant true
            إذا (bb.isEnter()) أرجع make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ثابت خودڪار& اندريون_ڪنارا = bb.اندريون_ڪنارا();
            خودڪار it = اندريون_ڪنارا.begin();
            Dfg_راس* resp = m_ڪنارو_منزل_Predicatep[static_cast<ثابت Cfg_ڪنارو&>(*it)];
            طالما (++it != اندريون_ڪنارا.پڇاڙي()) {
                DfgOr* ثابت orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_ڪنارو_منزل_Predicatep[static_cast<ثابت Cfg_ڪنارو&>(*it)]);
                resp = orp;
            }
            أرجع resp;
        }();

        ثابت Dfg_معلومات_Type& dtype = predp->dtype();

        ثابت خودڪار mkTmp = [&](File_سٽ* flp, ثابت حرف* name, Dfg_راس* srcp) {
            ثابت std::string اڳياڙي = "_BB" + std::منزل_string(bb.id()) + "_" + name;
            Dfg_راس_Var* ثابت tmpp = m_converter.createTmp(*m_logicp, flp, dtype, اڳياڙي);
            tmpp->srcp(srcp);
            أرجع tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        Dfg_راس_Var* ثابت pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        إذا (!bb.isBranch()) {
            m_ڪنارو_منزل_Predicatep[bb.ورتل_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            أرجع;
        }

        // For branches, we need to factor in the branch condition
        Dfg_راس* ثابت condp = m_bb_منزل_Condp[bb];
        File_سٽ* ثابت flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ثابت ورتل_Predp = make<DfgAnd>(flp, dtype);
            ورتل_Predp->lhsp(pInp);
            ورتل_Predp->rhsp(condp);
            m_ڪنارو_منزل_Predicatep[bb.ورتل_Edgep()] = mkTmp(flp, "Taken", ورتل_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ثابت untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ثابت notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_ڪنارو_منزل_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    منطقي checkExtWrites() {
        لكل (ثابت Dfg_راس* ثابت vtxp : m_logicp->synth()) {
            ثابت Dfg_راس_Var* ثابت varp = vtxp->cast<Dfg_راس_Var>();
            إذا (!varp) استمر;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            إذا (!varp->hasSinks()) استمر;
            إذا (!Dfg_راس_Var::isVolatile(varp->tmpForp())) استمر;
            ++m_ctx.m_synt.nonSynExtWrite;
            أرجع خطأ;
        }
        أرجع سچ;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    منطقي جوڙيو_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ثابت منطقي missing = m_logicp->foreachSink([&](ثابت Dfg_راس& sink) {
            ثابت DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ثابت vscp = unresolvedp->singleSink()->as<Dfg_راس_Var>()->vscp();
            أرجع !oSymTab.ڳڻپ(vscp);
        });
        إذا (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            أرجع خطأ;
        }

        // Add sinks to read the computed values for the target variables
        أرجع !m_logicp->foreachSink([&](Dfg_راس& sink) {
            DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            ثابت Dfg_راس_Var* ثابت varp = unresolvedp->singleSink()->as<Dfg_راس_Var>();
            Dfg_راس_Var* ثابت resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            إذا (!resp->hasSinks()) {
                unresolvedp->جوڙيو_Driver(resp->srcp()->as<Dfg_راس_Splice>());
                أرجع خطأ;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            إذا (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                أرجع سچ;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ثابت splicep
                = جديد DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ثابت std::ويڪٽر<Driver> drivers = computePropagatedDrivers({}, resp);
            لكل (ثابت Driver& d : drivers) splicep->جوڙيو_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->جوڙيو_Driver(splicep);
            أرجع خطأ;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    منطقي synthesize_سونپيو_W(Ast_سونپيو_W* nodep) {
        ++m_ctx.m_synt.داخلا_سونپيو;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_داخلا_نشان_جدول(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        Dfg_راس* condp = خالي_اشارو;
        ثابت منطقي success = synthesize_Basic_بلاڪ(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        إذا (!success) أرجع خطأ;

        // Check exernal writes are observed correctly
        إذا (!checkExtWrites()) أرجع خطأ;

        // Add resolved output variable drivers
        أرجع جوڙيو_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    منطقي synthesizeCfg(Cfg_گراف& cfg) {
        ++m_ctx.m_synt.داخلا_Always;

        // If there is a backward edge (loop), we can't synthesize it
        إذا (cfg.شامل_آهي_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            أرجع خطأ;
        }

        // If it's a trivial CFG we can save on some work
        إذا (cfg.n_بلاڪون() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } وإلا {
            // Insert two-way join blocks to aid multiplexer ordering
            إذا (cfg.وجهو_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } وإلا {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_وڻ = Cfg_Dominator_وڻ{cfg};
            m_ڪنارو_منزل_Predicatep = cfg.make_ڪنارو_Map<Dfg_راس_Var*>();
        }

        // Initialize CfgMaps
        m_bb_منزل_ISym_Tab = cfg.make_بلاڪ_Map<SymTab>();
        m_bb_منزل_OSym_Tab = cfg.make_بلاڪ_Map<SymTab>();
        m_bb_منزل_Condp = cfg.make_بلاڪ_Map<Dfg_راس_Var*>();

        // Synthesize all blocks
        لكل (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاڪ& bb = static_cast<ثابت Cfg_بلاڪ&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            إذا (!create_داخلا_نشان_جدول(bb)) أرجع خطأ;
            // Synthesize this block
            Dfg_راس* condp = خالي_اشارو;
            إذا (!synthesize_Basic_بلاڪ(m_bb_منزل_OSym_Tab[bb], condp, bb.stmtps(), m_bb_منزل_ISym_Tab[bb])) {
                أرجع خطأ;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            إذا (condp) {
                File_سٽ* ثابت flp = condp->fileline();
                ثابت Dfg_معلومات_Type& dtype = condp->dtype();
                ثابت std::string اڳياڙي = "_BB" + std::منزل_string(bb.id()) + "_Cond";
                Dfg_راس_Var* ثابت vp = m_converter.createTmp(*m_logicp, flp, dtype, اڳياڙي);
                vp->srcp(condp);
                m_bb_منزل_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            سونپيو_رستو_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        إذا (!checkExtWrites()) أرجع خطأ;

        // Add resolved output variable drivers
        أرجع جوڙيو_Synthesized_Output(m_bb_منزل_OSym_Tab[cfg.ٻاهر_نڪرو()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    منطقي synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        إذا (Ast_سونپيو_W* ثابت ap = VN_CAST(vtx.nodep()->stmtsp(), سونپيو_W)) {
            إذا (ap->nextp()) أرجع خطأ;
            إذا (!synthesize_سونپيو_W(ap)) أرجع خطأ;
            ++m_ctx.m_synt.synth_سونپيو;
            أرجع سچ;
        }

        إذا (!synthesizeCfg(vtx.cfg())) أرجع خطأ;
        ++m_ctx.m_synt.synthAlways;
        أرجع سچ;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    فراغ revert(VDouble0& statCountr) {
        m_منزل_Revert.foreach([&](Dfg_راس& vtx) {
            // Process DfgLogic
            إذا (DfgLogic* ثابت vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                لكل (Dfg_راس* ثابت p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->سيٽ_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](Dfg_راس& snk) {
                    m_منزل_Revert.ڌڪو_front(*snk.as<DfgUnresolved>());
                    أرجع خطأ;
                });
                أرجع;
            }

            // Process DfgUnresolved
            إذا (DfgUnresolved* ثابت vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_راس_Var>()->سيٽ_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreach_ذريعو([&](Dfg_راس& src) {
                    DfgLogic* ثابت lp = src.cast<DfgLogic>();
                    إذا (lp && !lp->reverted()) m_منزل_Revert.ڌڪو_front(*lp);
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
    فراغ مکيه() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        لكل (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            إذا (!vtxp->is<DfgUnresolved>()) استمر;
            منطقي anySelected = خطأ;
            منطقي anyUnselected = خطأ;
            vtxp->foreach_ذريعو([&](Dfg_راس& src) {
                ثابت DfgLogic& logic = *src.as<DfgLogic>();
                إذا (logic.selectedForSynthesis()) {
                    anySelected = سچ;
                } وإلا {
                    anyUnselected = سچ;
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
                vtxp->singleSink()->as<Dfg_راس_Var>()->سيٽ_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        لكل (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            إذا (!logicp) استمر;
            إذا (logicp->selectedForSynthesis()) استمر;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ثابت VNUser4_اندريون_Use m_user4_اندريون_Use;
            logicp->foreach_ذريعو([](Dfg_راس& src) {
                src.as<Dfg_راس_Var>()->vscp()->user4p(&src);
                أرجع خطأ;
            });
            V3DfgPasses::جوڙيو_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ڳنڍ* varp) {  //
                أرجع varp->user4u().منزل<Dfg_راس_Var*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ثابت VNUser4_اندريون_Use user4_اندريون_Use;
            لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
                DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
                إذا (!logicp) استمر;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ثابت خودڪار debugCallback = [&]() -> فراغ {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_وڻ(logicp->nodep(), std::ٻاهر);
                    debugDump("synth-lastok");
                };
                إذا (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) اكسر;

                // Synthesize it, if failed, enqueue for reversion
                إذا (!synthesize(*logicp)) {
                    logicp->سيٽ_Non_Synthesizable();
                    m_منزل_Revert.ڌڪو_front(*logicp);
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
        std::ويڪٽر<Dfg_راس_Var*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ثابت Dfg_راس_Var*, Dfg_راس_Splice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        لكل (Dfg_راس_Var& var : m_dfg.varVertices()) {
            إذا (!var.srcp()) استمر;
            DfgUnresolved* ثابت unresolvedp = var.srcp()->cast<DfgUnresolved>();
            إذا (!unresolvedp) اكسر;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            Dfg_راس_Splice* ثابت resolvedp = [&]() -> Dfg_راس_Splice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                إذا (unresolvedp->nInputs() == 1) {
                    أرجع unresolvedp->inputp(0)->as<Dfg_راس_Splice>();
                }
                // Otherwise gather the synthesized drivers
                std::ويڪٽر<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                إذا (normalizeDrivers(var, drivers)) أرجع makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_پٺ(&var);
                m_منزل_Revert.ڌڪو_front(*unresolvedp);
                أرجع خالي_اشارو;
            }();
            // Bail if multidriven
            إذا (!resolvedp) استمر;
            // Add to map for next loop
            ثابت منطقي new_داخلا = resolvedDrivers.emplace(&var, resolvedp).ٻيو;
            UASSERT_OBJ(new_داخلا, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        لكل (ثابت Dfg_راس_Var& var : m_dfg.varVertices()) {
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
        لكل (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            إذا (!logicp) استمر;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            إذا (!logicp->synth().خالي_جايون()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_پٺ()->delete_وڻ();
            } وإلا {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ثابت VNUser4_اندريون_Use m_user4_اندريون_Use;
                logicp->foreach_ذريعو([](Dfg_راس& src) {
                    src.as<Dfg_راس_Var>()->vscp()->user4p(&src);
                    أرجع خطأ;
                });
                V3DfgPasses::جوڙيو_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ڳنڍ* varp) {  //
                    أرجع varp->user4u().منزل<Dfg_راس_Var*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = خالي_اشارو;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        لكل (Dfg_راس* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            Dfg_راس_Splice* ثابت splicep = vtxp->cast<Dfg_راس_Splice>();
            إذا (!splicep) استمر;

            // Might not have a sink if the driving logic was revered, remove
            إذا (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                استمر;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            إذا (Dfg_راس* ثابت wholep = splicep->wholep()) {
                إذا (Dfg_راس_Var* ثابت varp = splicep->singleSink()->cast<Dfg_راس_Var>()) {
                    varp->driver_File_سٽ(splicep->driver_File_سٽ(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    Ast_منزل_Dfg_Synthesize(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        مکيه();
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3DfgSynthesisContext& ctx) {
        Ast_منزل_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_اڻ_استعمال_ٿيل(dfg);
        إذا (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        إذا (v3Global.opt.debugCheck()) {
            لكل (Dfg_راس& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_گڻو_Sinks(), &vtx, "Operation has multiple sinks");
                لكل (سائيز_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

صحيح Ast_منزل_Dfg_Synthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
ساكن فراغ dfgSelectLogicForSynthesis(Dfg_گراف& dfg) {
    // If we are told to synthesize everything, we will do so ...
    إذا (v3Global.opt.fDfgSynthesizeAll()) {
        لكل (Dfg_راس& vtx : dfg.opVertices()) {
            إذا (DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) logicp->سيٽ_Selected_For_Synthesis();
        }
        أرجع;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::ويڪٽر<Dfg_راس_Var*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        لكل (Dfg_راس_Var& var : dfg.varVertices()) {
            إذا (!scc.at(var)) استمر;
            // This is a circular variable
            circularVarps.emplace_پٺ(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    لكل (ثابت Dfg_راس_Var* ثابت varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreach_ذريعو([&](Dfg_راس& driver) {
            worklist.ڌڪو_front(*driver.as<DfgLogic>());
            أرجع خطأ;
        });
    }

    // Choose some simple special cases to always synthesize
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        إذا (!logicp || worklist.شامل_آهي(*logicp)) استمر;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        إذا (logicp->drives_اڻ_استعمال_ٿيل_Vars()) {
            worklist.ڌڪو_front(*logicp);
            استمر;
        }
        // Blocks corresponding to continuous assignments
        إذا (logicp->nodep()->keyword() == VAlwaysKwd::CONT_سونپيو) {
            worklist.ڌڪو_front(*logicp);
            استمر;
        }
        ثابت Cfg_گراف& cfg = logicp->cfg();
        // Straight line code with no branches
        إذا (cfg.n_بلاڪون() == 1) {
            worklist.ڌڪو_front(*logicp);
            استمر;
        }
        // Blocks driving exactly 1 variable
        إذا (!logicp->has_گڻو_Sinks()) worklist.ڌڪو_front(*logicp);
    }

    std::unordered_سيٽ<DfgUnresolved*> is_گهميل;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](Dfg_راس& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.سيٽ_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](Dfg_راس& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            إذا (!is_گهميل.وجهو(unresolved).ٻيو) أرجع خطأ;
            unresolved->foreach_ذريعو([&](Dfg_راس& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                إذا (!siblingLogic.selectedForSynthesis()) worklist.ڌڪو_front(siblingLogic);
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
    Ast_منزل_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
