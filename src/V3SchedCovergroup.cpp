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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeExpr.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਅਚਲ CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

ਖਾਲੀ CovergroupRefBindings::ਜੋੜੋ_Construction(ਅਚਲ AstClass* classp, ਅਚਲ AstVarScope* instp,
                                            ਅਚਲ Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.ਪਾਓ(classBindings.ਸਿਰਾ(), bindings.begin(), bindings.ਸਿਰਾ());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    ਜੇ (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.ਪਾਓ(instBindings.ਸਿਰਾ(), bindings.begin(), bindings.ਸਿਰਾ());
    }
}

ਅਚਲ CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(ਅਚਲ AstVarScope* instp, ਅਚਲ AstClass* classp) ਅਚਲ {
    ਜੇ (instp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_byInstance.ਲੱਭੋ(instp);
        ਜੇ (it != m_byInstance.ਸਿਰਾ()) {
            ++m_ਅੰਕ_Exact_Calls;
            ਵਾਪਸ it->ਦੂਜਾ;
        }
    }
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_byClass.ਲੱਭੋ(classp);
    ਜੇ (it != m_byClass.ਸਿਰਾ()) {
        ++m_ਅੰਕ_Union_Calls;
        ਵਾਪਸ it->ਦੂਜਾ;
    }
    // A covergroup with no reference formal at all
    ਵਾਪਸ s_none;
}

ਵਰਗ CovergroupRefBindVisitor final : ਜਨਤਕ VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  //ਨਤੀਜਾ
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    ਸਥਿਰ AstClass* constructedCovergroup(ਅਚਲ Ast_ਗੰਢ_CCall* nodep) {
        ਅਚਲ AstCFunc* ਅਚਲ funcp = nodep->funcp();
        ਜੇ (!funcp->isConstructor()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* ਅਚਲ classp = VN_AS(funcp->scopep()->modp(), Class);
        ਵਾਪਸ classp->isCovergroup() ? classp : ਖਾਲੀ_ਸੂਚਕ;
    }

    // True if this variable holds a handle to a covergroup object
    ਸਥਿਰ ਤਾਰਕਿਕ isCovergroupHandle(ਅਚਲ AstVar* varp) {
        ਅਚਲ AstClassRefDType* ਅਚਲ dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ਵਾਪਸ dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    ਸਥਿਰ ਤਾਰਕਿਕ hasRefFormal(ਅਚਲ AstCFunc* funcp) {
        ਲਈ (Ast_ਗੰਢ* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_AS(portp, Var);
            ਜੇ (varp->declDirection().isRef() || varp->declDirection().isConstRef()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    ਖਾਲੀ ਰਿਕਾਰਡ_Construction(Ast_ਗੰਢ_CCall* nodep, ਅਚਲ AstClass* classp,
                            ਅਚਲ AstVarScope* instp) {
        ਜੇ (!hasRefFormal(nodep->funcp())) ਵਾਪਸ;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_ਗੰਢ* actualp = nodep->argsp();
        ਲਈ (Ast_ਗੰਢ* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_ਗੰਢ* ਅਚਲ thisActualp = actualp;
            actualp = actualp->nextp();
            ਅਚਲ AstVar* ਅਚਲ varp = VN_AS(portp, Var);
            ਜੇ (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) ਜਾਰੀ;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* ਅਚਲ vscp = refp->varScopep();
                bindings.ਧੱਕੋ_ਪਿੱਠ(vscp);
            });
        }
        m_bindings.ਜੋੜੋ_Construction(classp, instp, bindings);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        AstCNew* ਅਚਲ cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* ਅਚਲ lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* ਅਚਲ classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!classp) {
            iterateChildrenConst(nodep);
            ਵਾਪਸ;
        }
        ਰਿਕਾਰਡ_Construction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = constructedCovergroup(nodep)) {
            ਰਿਕਾਰਡ_Construction(nodep, classp, ਖਾਲੀ_ਸੂਚਕ);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!nodep->access().isWriteOrRW()) ਵਾਪਸ;
        ਜੇ (!isCovergroupHandle(nodep->varp())) ਵਾਪਸ;
        m_tainted.emplace(nodep->varScopep());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        ਲਈ (ਅਚਲ AstVarScope* ਅਚਲ vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = ਮੂਲ;

    // METHODS
    CovergroupRefBindings take_bindings() { ਵਾਪਸ std::move(m_bindings); }
};

ਅਚਲ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    ਜੇ (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    ਵਾਪਸ bindings;
}

}  // namespace V3Sched
