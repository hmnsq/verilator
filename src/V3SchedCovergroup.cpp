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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3AstNodeExpr.h"
#অন্তর্ভাৱ "V3Sched.h"

#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

ধ্রুৱ CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

শূন্য CovergroupRefBindings::addConstruction(ধ্রুৱ AstClass* classp, ধ্রুৱ AstVarScope* instp,
                                            ধ্রুৱ Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.ঢোকাও(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    যদি (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.ঢোকাও(instBindings.end(), bindings.begin(), bindings.end());
    }
}

ধ্রুৱ CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(ধ্রুৱ AstVarScope* instp, ধ্রুৱ AstClass* classp) ধ্রুৱ {
    যদি (instp) {
        ধ্রুৱ auto it = m_byInstance.find(instp);
        যদি (it != m_byInstance.end()) {
            ++m_অঙ্ক_Exact_Calls;
            ঘূৰাই it->second;
        }
    }
    ধ্রুৱ auto it = m_byClass.find(classp);
    যদি (it != m_byClass.end()) {
        ++m_অঙ্ক_Union_Calls;
        ঘূৰাই it->second;
    }
    // A covergroup with no reference formal at all
    ঘূৰাই s_none;
}

শ্ৰেণী CovergroupRefBindVisitor final : সার্ৱজনিক VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<ধ্রুৱ AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    স্থির AstClass* constructedCovergroup(ধ্রুৱ Ast_গ্ৰন্থি_CCall* nodep) {
        ধ্রুৱ AstCFunc* ধ্রুৱ funcp = nodep->funcp();
        যদি (!funcp->isConstructor()) ঘূৰাই nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* ধ্রুৱ classp = VN_AS(funcp->scopep()->modp(), Class);
        ঘূৰাই classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    স্থির তার্কিক isCovergroupHandle(ধ্রুৱ AstVar* varp) {
        ধ্রুৱ AstClassRefDType* ধ্রুৱ dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ঘূৰাই dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    স্থির তার্কিক hasRefFormal(ধ্রুৱ AstCFunc* funcp) {
        বাবে (Ast_গ্ৰন্থি* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            ধ্রুৱ AstVar* ধ্রুৱ varp = VN_AS(portp, Var);
            যদি (varp->declDirection().isRef() || varp->declDirection().isConstRef()) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    শূন্য recordConstruction(Ast_গ্ৰন্থি_CCall* nodep, ধ্রুৱ AstClass* classp,
                            ধ্রুৱ AstVarScope* instp) {
        যদি (!hasRefFormal(nodep->funcp())) ঘূৰাই;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_গ্ৰন্থি* actualp = nodep->argsp();
        বাবে (Ast_গ্ৰন্থি* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_গ্ৰন্থি* ধ্রুৱ thisActualp = actualp;
            actualp = actualp->nextp();
            ধ্রুৱ AstVar* ধ্রুৱ varp = VN_AS(portp, Var);
            যদি (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) অনুৱর্ত;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* ধ্রুৱ vscp = refp->varScopep();
                bindings.ঠেলো_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        AstCNew* ধ্রুৱ cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* ধ্রুৱ lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* ধ্রুৱ classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        যদি (!classp) {
            iterateChildrenConst(nodep);
            ঘূৰাই;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        যদি (ধ্রুৱ AstClass* ধ্রুৱ classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!nodep->access().isWriteOrRW()) ঘূৰাই;
        যদি (!isCovergroupHandle(nodep->varp())) ঘূৰাই;
        m_tainted.emplace(nodep->varScopep());
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        বাবে (ধ্রুৱ AstVarScope* ধ্রুৱ vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = স্ৱভাৱ;

    // METHODS
    CovergroupRefBindings take_bindings() { ঘূৰাই std::move(m_bindings); }
};

ধ্রুৱ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    যদি (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    ঘূৰাই bindings;
}

}  // namespace V3Sched
