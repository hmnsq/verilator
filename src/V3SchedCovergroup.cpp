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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3AstNodeExpr.h"
#অন্তর্ভাব "V3Sched.h"

#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

ধ্রুব CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

শূন্য CovergroupRefBindings::addConstruction(ধ্রুব AstClass* classp, ধ্রুব AstVarScope* instp,
                                            ধ্রুব Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.ঢোকাও(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    যদি (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.ঢোকাও(instBindings.end(), bindings.begin(), bindings.end());
    }
}

ধ্রুব CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(ধ্রুব AstVarScope* instp, ধ্রুব AstClass* classp) ধ্রুব {
    যদি (instp) {
        ধ্রুব auto it = m_byInstance.find(instp);
        যদি (it != m_byInstance.end()) {
            ++m_অঙ্ক_Exact_Calls;
            প্রত্যাগম it->second;
        }
    }
    ধ্রুব auto it = m_byClass.find(classp);
    যদি (it != m_byClass.end()) {
        ++m_অঙ্ক_Union_Calls;
        প্রত্যাগম it->second;
    }
    // A covergroup with no reference formal at all
    প্রত্যাগম s_none;
}

বর্গ CovergroupRefBindVisitor final : সার্বজনিক VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<ধ্রুব AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    স্থির AstClass* constructedCovergroup(ধ্রুব Ast_গ্রন্থি_CCall* nodep) {
        ধ্রুব AstCFunc* ধ্রুব funcp = nodep->funcp();
        যদি (!funcp->isConstructor()) প্রত্যাগম nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* ধ্রুব classp = VN_AS(funcp->scopep()->modp(), Class);
        প্রত্যাগম classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    স্থির তার্কিক isCovergroupHandle(ধ্রুব AstVar* varp) {
        ধ্রুব AstClassRefDType* ধ্রুব dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        প্রত্যাগম dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    স্থির তার্কিক hasRefFormal(ধ্রুব AstCFunc* funcp) {
        প্রতি (Ast_গ্রন্থি* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            ধ্রুব AstVar* ধ্রুব varp = VN_AS(portp, Var);
            যদি (varp->declDirection().isRef() || varp->declDirection().isConstRef()) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    শূন্য recordConstruction(Ast_গ্রন্থি_CCall* nodep, ধ্রুব AstClass* classp,
                            ধ্রুব AstVarScope* instp) {
        যদি (!hasRefFormal(nodep->funcp())) প্রত্যাগম;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_গ্রন্থি* actualp = nodep->argsp();
        প্রতি (Ast_গ্রন্থি* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_গ্রন্থি* ধ্রুব thisActualp = actualp;
            actualp = actualp->nextp();
            ধ্রুব AstVar* ধ্রুব varp = VN_AS(portp, Var);
            যদি (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) অনুবর্ত;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* ধ্রুব vscp = refp->varScopep();
                bindings.ঠেলো_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        AstCNew* ধ্রুব cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* ধ্রুব lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* ধ্রুব classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        যদি (!classp) {
            iterateChildrenConst(nodep);
            প্রত্যাগম;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        যদি (ধ্রুব AstClass* ধ্রুব classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!nodep->access().isWriteOrRW()) প্রত্যাগম;
        যদি (!isCovergroupHandle(nodep->varp())) প্রত্যাগম;
        m_tainted.emplace(nodep->varScopep());
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        প্রতি (ধ্রুব AstVarScope* ধ্রুব vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = স্বভাব;

    // METHODS
    CovergroupRefBindings take_bindings() { প্রত্যাগম std::move(m_bindings); }
};

ধ্রুব CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    যদি (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    প্রত্যাগম bindings;
}

}  // namespace V3Sched
