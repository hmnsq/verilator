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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3AstNodeExpr.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ଅଚଳ CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

ଶୂନ୍ୟ CovergroupRefBindings::addConstruction(ଅଚଳ AstClass* classp, ଅଚଳ AstVarScope* instp,
                                            ଅଚଳ Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.ଭର୍ତ୍ତି(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    ଯଦି (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.ଭର୍ତ୍ତି(instBindings.end(), bindings.begin(), bindings.end());
    }
}

ଅଚଳ CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(ଅଚଳ AstVarScope* instp, ଅଚଳ AstClass* classp) ଅଚଳ {
    ଯଦି (instp) {
        ଅଚଳ auto it = m_byInstance.find(instp);
        ଯଦି (it != m_byInstance.end()) {
            ++m_ଅଙ୍କ_Exact_Calls;
            ଫେରନ୍ତୁ it->second;
        }
    }
    ଅଚଳ auto it = m_byClass.find(classp);
    ଯଦି (it != m_byClass.end()) {
        ++m_ଅଙ୍କ_Union_Calls;
        ଫେରନ୍ତୁ it->second;
    }
    // A covergroup with no reference formal at all
    ଫେରନ୍ତୁ s_none;
}

ଶ୍ରେଣୀ CovergroupRefBindVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<ଅଚଳ AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    ସ୍ଥିର AstClass* constructedCovergroup(ଅଚଳ Ast_ଗଣ୍ଠି_CCall* nodep) {
        ଅଚଳ AstCFunc* ଅଚଳ funcp = nodep->funcp();
        ଯଦି (!funcp->isConstructor()) ଫେରନ୍ତୁ nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* ଅଚଳ classp = VN_AS(funcp->scopep()->modp(), Class);
        ଫେରନ୍ତୁ classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    ସ୍ଥିର ତାର୍କିକ isCovergroupHandle(ଅଚଳ AstVar* varp) {
        ଅଚଳ AstClassRefDType* ଅଚଳ dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        ଫେରନ୍ତୁ dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    ସ୍ଥିର ତାର୍କିକ hasRefFormal(ଅଚଳ AstCFunc* funcp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_AS(portp, Var);
            ଯଦି (varp->declDirection().isRef() || varp->declDirection().isConstRef()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    ଶୂନ୍ୟ recordConstruction(Ast_ଗଣ୍ଠି_CCall* nodep, ଅଚଳ AstClass* classp,
                            ଅଚଳ AstVarScope* instp) {
        ଯଦି (!hasRefFormal(nodep->funcp())) ଫେରନ୍ତୁ;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_ଗଣ୍ଠି* actualp = nodep->argsp();
        ପାଇଁ (Ast_ଗଣ୍ଠି* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ thisActualp = actualp;
            actualp = actualp->nextp();
            ଅଚଳ AstVar* ଅଚଳ varp = VN_AS(portp, Var);
            ଯଦି (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) ଜାରି;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* ଅଚଳ vscp = refp->varScopep();
                bindings.ଠେଲ_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        AstCNew* ଅଚଳ cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* ଅଚଳ lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* ଅଚଳ classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        ଯଦି (!classp) {
            iterateChildrenConst(nodep);
            ଫେରନ୍ତୁ;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!nodep->access().isWriteOrRW()) ଫେରନ୍ତୁ;
        ଯଦି (!isCovergroupHandle(nodep->varp())) ଫେରନ୍ତୁ;
        m_tainted.emplace(nodep->varScopep());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        ପାଇଁ (ଅଚଳ AstVarScope* ଅଚଳ vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    CovergroupRefBindings take_bindings() { ଫେରନ୍ତୁ std::move(m_bindings); }
};

ଅଚଳ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    ଯଦି (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    ଫେରନ୍ତୁ bindings;
}

}  // namespace V3Sched
