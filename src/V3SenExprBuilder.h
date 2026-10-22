// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Builder for sensitivity checking expressions.
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

#यदि_अपरिभाषितम् VERILATOR_V3SENEXPRBUILDER_H_
#परिभाषय VERILATOR_V3SENEXPRBUILDER_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_व्यञ्जकम्_Builder final {
सार्वजनिक:
    // TYPES
    संरचना Results final {
        std::सदिशम्<Ast_ग्रन्थिः_Stmt*> m_inits;  // Initialization statements for previous values
        std::सदिशम्<Ast_ग्रन्थिः_Stmt*> m_preUpdates;  // Pre update assignments
        std::सदिशम्<Ast_ग्रन्थिः_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::सदिशम्<Ast_ग्रन्थिः_Stmt*> m_destructivePostUpdates;
        std::सदिशम्<AstVar*> m_vars;  // Created temporary variables
    };

गुप्त:
    // STATE
    AstScope* ध्रुव m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ग्रन्थिः>, AstVarScope*> m_पूर्व;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ग्रन्थिः>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_समुच्चयः<VNRef<Ast_ग्रन्थिः>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_समुच्चयः<VNRef<Ast_ग्रन्थिः>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_नामानि m_पूर्व_नामानि{"__Vtrigprevexpr"};  // For generating unique temporary names

    स्थिर bool isSupportedDType(Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (VN_IS(dtypep, BasicDType)) प्रत्यागम true;
        यदि (VN_IS(dtypep, PackArrayDType)) प्रत्यागम true;
        यदि (VN_IS(dtypep, UnpackArrayDType)) प्रत्यागम isSupportedDType(dtypep->subDTypep());
        यदि (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType)) प्रत्यागम true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        यदि (VN_IS(dtypep, ClassRefDType)) प्रत्यागम true;
        प्रत्यागम false;
    }

    स्थिर bool is_Simple_व्यञ्जकम्(ध्रुव Ast_ग्रन्थिः* ध्रुव exprp) {
        प्रत्यागम exprp->forall([](ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
            प्रत्यागम VN_IS(nodep, Const) || VN_IS(nodep, ग्रन्थिः_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ग्रन्थिः_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    स्थिर bool isNullableRef(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव refp) {
        यदि (!refp->dtypep()) प्रत्यागम false;
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = refp->dtypep()->skipRefp();
        ध्रुव AstIfaceRefDType* ध्रुव ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        प्रत्यागम VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    स्थिर bool hasNullableMemberAccess(ध्रुव Ast_ग्रन्थिः* ध्रुव exprp) {
        प्रत्यागम exprp && exprp->exists([](ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
            ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep, MemberSel);
            प्रत्यागम mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    स्थिर ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* get_आधारः_Nullable_Ref(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        यावत् (exprp) {
            यदि (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(exprp, MemberSel)) {
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = mselp->fromp();
                यदि (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    यदि (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        अनुवर्त;
                    }
                    प्रत्यागम fromp;
                }
                exprp = fromp;
            } अन्यथा {
                प्रत्यागम nullptr;
            }
        }
        प्रत्यागम nullptr;
    }

    // METHODS
    AstVarScope* crate_अस्थायी(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        यदि (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ध्रुव vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_पूर्व_नामानि.get(name);
        } अन्यथा {
            name = m_पूर्व_नामानि.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_पङ्क्तिः* ध्रुव flp = m_scopep->fileline();
        AstVar* ध्रुव varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_आन्तरिकम्(true);
        m_scopep->modp()->सङ्कलय_Stmtsp(varp);
        AstVarScope* ध्रुव vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->सङ्कलय_Varsp(vscp);
        m_results.m_vars.प्रक्षिप_पृष्ठभागः(varp);
        प्रत्यागम vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ग्रन्थिः_Stmt* wrapStmtWithNullCheck(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_Stmt* stmtp,
                                       ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* आधारः_Refp) {
        यदि (!आधारः_Refp) प्रत्यागम stmtp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव checkp
            = new AstNeq{flp, const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(आधारः_Refp)->clone_वृक्षः(false), nullp};
        प्रत्यागम new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ग्रन्थिः_व्यञ्जकम्* wrap_व्यञ्जकम्_With_Null_Check(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                       ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* आधारः_Refp) {
        यदि (!आधारः_Refp) प्रत्यागम exprp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव checkp
            = new AstNeq{flp, const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(आधारः_Refp)->clone_वृक्षः(false), nullp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_समुच्चयः_Bit();
        प्रत्यागम condp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* getCurr(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        यदि (is_Simple_व्यञ्जकम्(exprp)) प्रत्यागम exprp->clone_वृक्षः(false);

        // Create the 'current value' variable
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        स्वचालित परिणाम = m_curr.emplace(*exprp, nullptr);
        यदि (परिणाम.second) परिणाम.first->second = crate_अस्थायी(exprp);
        AstVarScope* ध्रुव currp = परिणाम.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव आधारः_Refp
            = hasNullableMemberAccess(exprp) ? get_आधारः_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        यदि (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.प्रक्षिप_पृष्ठभागः(
                wrapStmtWithNullCheck(flp,
                                      new Ast_नियोजय{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_वृक्षः(false)},
                                      आधारः_Refp));
        }
        प्रत्यागम new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* get_पूर्व(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        ध्रुव स्वचालित rdCurr = [this, exprp]() { प्रत्यागम getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव आधारः_Refp
            = hasNullableMemberAccess(exprp) ? get_आधारः_Nullable_Ref(exprp) : nullptr;

        Ast_ग्रन्थिः* scopeExprp = exprp;
        यदि (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ध्रुव स्वचालित pair = m_पूर्व.emplace(*scopeExprp, nullptr);
        यदि (pair.second) {
            AstVarScope* ध्रुव prevp = crate_अस्थायी(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_नियोजय* ध्रुव initp = new Ast_नियोजय{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_वृक्षः(false)};
            m_results.m_inits.प्रक्षिप_पृष्ठभागः(wrapStmtWithNullCheck(flp, initp, आधारः_Refp));
        }

        AstVarScope* ध्रुव prevp = pair.first->second;

        ध्रुव स्वचालित wr_पूर्व = [=]() { प्रत्यागम new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        यदि (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_ग्रन्थिः_DType* ध्रुव व्यञ्जकम्_Dtp = exprp->dtypep()->skipRefp();
            यदि (!isSupportedDType(व्यञ्जकम्_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << व्यञ्जकम्_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_अधिकम्()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                प्रत्यागम prevp;
            }
            यदि (VN_IS(व्यञ्जकम्_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ध्रुव cmhp
                    = new AstCMethodHard{flp, wr_पूर्व(), VCMethod::UNPACKED_नियोजय, rdCurr()};
                cmhp->dtype_समुच्चयः_Void();
                m_results.m_postUpdates.प्रक्षिप_पृष्ठभागः(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), आधारः_Refp));
            } अन्यथा {
                m_results.m_postUpdates.प्रक्षिप_पृष्ठभागः(
                    wrapStmtWithNullCheck(flp, new Ast_नियोजय{flp, wr_पूर्व(), rdCurr()}, आधारः_Refp));
            }
        }

        प्रत्यागम prevp;
    }

    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, bool> create_पदांशः(ध्रुव AstSenItem* senItemp) {
        File_पङ्क्तिः* ध्रुव flp = senItemp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव आधारः_Refp
            = hasNullableMemberAccess(senp) ? get_आधारः_Nullable_Ref(senp) : nullptr;

        ध्रुव स्वचालित currp = [this, senp]() { प्रत्यागम getCurr(senp); };
        ध्रुव स्वचालित prevp
            = [this, flp, senp]() { प्रत्यागम new AstVarRef{flp, get_पूर्व(senp), VAccess::READ}; };
        ध्रुव स्वचालित lsb = [=](Ast_ग्रन्थिः_व्यञ्जकम्* opp) { प्रत्यागम new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        विकल्प (senItemp->edgeType()) {
        पक्ष VEdgeType::ET_परिवर्तितम्:
        पक्ष VEdgeType::ET_HYBRID:  //
            यदि (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ध्रुव resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_समुच्चयः_Bit();
                प्रत्यागम {wrap_व्यञ्जकम्_With_Null_Check(flp, resultp, आधारः_Refp), true};
            }
            प्रत्यागम {wrap_व्यञ्जकम्_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, आधारः_Refp), true};
        पक्ष VEdgeType::ET_BOTHEDGE:  //
            प्रत्यागम {wrap_व्यञ्जकम्_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), आधारः_Refp),
                    false};
        पक्ष VEdgeType::ET_POSEDGE:  //
            प्रत्यागम {wrap_व्यञ्जकम्_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), आधारः_Refp),
                    false};
        पक्ष VEdgeType::ET_NEGEDGE:  //
            प्रत्यागम {wrap_व्यञ्जकम्_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), आधारः_Refp),
                    false};
        पक्ष VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clearFromp = currp();
            यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), ग्रन्थिः_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ध्रुव clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_समुच्चयः_Void();
            Ast_ग्रन्थिः_Stmt* ध्रुव clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), आधारः_Refp);
            m_results.m_postUpdates.प्रक्षिप_पृष्ठभागः(clearStmtp);
            m_results.m_destructivePostUpdates.प्रक्षिप_पृष्ठभागः(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ध्रुव callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_समुच्चयः_Bit();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firedp = wrap_व्यञ्जकम्_With_Null_Check(flp, callp, आधारः_Refp);
            यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                यदि (refp->varScopep() == v3Global.rootp()->nbaEventp()) प्रत्यागम {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            प्रत्यागम {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        पक्ष VEdgeType::ET_TRUE:  //
            प्रत्यागम {currp(), false};
        पक्ष VEdgeType::ET_INITIAL_NBA:  //
            प्रत्यागम {new AstConst{flp, AstConst::BitFalse{}}, true};
        स्वभाव:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            प्रत्यागम {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

सार्वजनिक:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, bool> build(ध्रुव AstSenItem* senItemp) {
        स्वचालित पदांशः = create_पदांशः(senItemp);
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = senItemp->condp()) {
            पदांशः.first = new AstAnd{senItemp->fileline(), condp->clone_वृक्षः_विशुद्धम्(false), पदांशः.first};
        }
        प्रत्यागम पदांशः;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, bool> build(ध्रुव Ast_Sen_वृक्षः* senTreep) {
        File_पङ्क्तिः* ध्रुव flp = senTreep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
        bool firedAtInitialization = false;
        प्रति (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ध्रुव स्वचालित& pair = build(senItemp);
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        प्रत्यागम {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        प्रत्यागम ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_पूर्व.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        प्रत्यागम std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_व्यञ्जकम्_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#यदि_अन्तः  // Guard
