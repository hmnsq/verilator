// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve covergroup reference formal arguments for
//                         scheduling
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
// V3SchedCovergroup's Transformations:
//
// None - this only gathers information.
//
// A covergroup 'ref'/'const ref' formal argument becomes a pointer member of the covergroup
// class, bound when the covergroup is constructed. A sample() reading through it therefore
// holds no AstVarRef naming the design variable it reads, and V3Order would see the sampling
// block as reading nothing. Record what each construction binds, so V3Order can attribute
// those reads to the blocks that call sample().
//
// Bindings are keyed by the constructed handle where that is exact, which it is when every
// write of the handle is a construction of the recognized shape 'handle = new(...)'. Any other
// way for a covergroup object to reach a handle - an aliasing assignment, a temporary
// introduced by V3Task, an array element, passing the handle to a function by reference - is
// itself a write of that handle, and taints it. A tainted handle falls back to the union over
// all constructions of its class, which is an over-approximation, but safe.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3AstNodeExpr.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ಸ್ಥಿರಾಂಕ CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

ಶೂನ್ಯ CovergroupRefBindings::ಕೂಡಿಸು_Construction(ಸ್ಥಿರಾಂಕ AstClass* classp, ಸ್ಥಿರಾಂಕ AstVarScope* instp,
                                            ಸ್ಥಿರಾಂಕ Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.ಸೇರಿಸು(classBindings.ಕೊನೆ(), bindings.begin(), bindings.ಕೊನೆ());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    ಆದರೆ (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.ಸೇರಿಸು(instBindings.ಕೊನೆ(), bindings.begin(), bindings.ಕೊನೆ());
    }
}

ಸ್ಥಿರಾಂಕ CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(ಸ್ಥಿರಾಂಕ AstVarScope* instp, ಸ್ಥಿರಾಂಕ AstClass* classp) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (instp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_byInstance.ಕಂಡುಹಿಡಿ(instp);
        ಆದರೆ (it != m_byInstance.ಕೊನೆ()) {
            ++m_ಅಂಕೆ_Exact_Calls;
            ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
        }
    }
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_byClass.ಕಂಡುಹಿಡಿ(classp);
    ಆದರೆ (it != m_byClass.ಕೊನೆ()) {
        ++m_ಅಂಕೆ_Union_Calls;
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }
    // A covergroup with no reference formal at all
    ಹಿಂತಿರುಗಿ s_none;
}

ವರ್ಗ CovergroupRefBindVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  //ಫಲಿತಾಂಶ
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    ಸ್ಥಿರ AstClass* constructedCovergroup(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* nodep) {
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();
        ಆದರೆ (!funcp->isConstructor()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* ಸ್ಥಿರಾಂಕ classp = VN_AS(funcp->scopep()->modp(), Class);
        ಹಿಂತಿರುಗಿ classp->isCovergroup() ? classp : ಶೂನ್ಯಸೂಚಕ;
    }

    // True if this variable holds a handle to a covergroup object
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCovergroupHandle(ಸ್ಥಿರಾಂಕ AstVar* varp) {
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ಹಿಂತಿರುಗಿ dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasRefFormal(ಸ್ಥಿರಾಂಕ AstCFunc* funcp) {
        ಗಾಗಿ (Ast_ಗಂಟು* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_AS(portp, Var);
            ಆದರೆ (varp->declDirection().isRef() || varp->declDirection().isConstRef()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    ಶೂನ್ಯ ದಾಖಲೆ_Construction(Ast_ಗಂಟು_CCall* nodep, ಸ್ಥಿರಾಂಕ AstClass* classp,
                            ಸ್ಥಿರಾಂಕ AstVarScope* instp) {
        ಆದರೆ (!hasRefFormal(nodep->funcp())) ಹಿಂತಿರುಗಿ;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_ಗಂಟು* actualp = nodep->argsp();
        ಗಾಗಿ (Ast_ಗಂಟು* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thisActualp = actualp;
            actualp = actualp->nextp();
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_AS(portp, Var);
            ಆದರೆ (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) ಮುಂದುವರಿ;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
                bindings.ತಳ್ಳು_ಹಿಂಭಾಗ(vscp);
            });
        }
        m_bindings.ಕೂಡಿಸು_Construction(classp, instp, bindings);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        AstCNew* ಸ್ಥಿರಾಂಕ cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* ಸ್ಥಿರಾಂಕ classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!classp) {
            iterateChildrenConst(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ದಾಖಲೆ_Construction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = constructedCovergroup(nodep)) {
            ದಾಖಲೆ_Construction(nodep, classp, ಶೂನ್ಯಸೂಚಕ);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!nodep->access().isWriteOrRW()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!isCovergroupHandle(nodep->varp())) ಹಿಂತಿರುಗಿ;
        m_tainted.emplace(nodep->varScopep());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    CovergroupRefBindings take_bindings() { ಹಿಂತಿರುಗಿ std::move(m_bindings); }
};

ಸ್ಥಿರಾಂಕ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    ಆದರೆ (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    ಹಿಂತಿರುಗಿ bindings;
}

}  // namespace V3Sched
