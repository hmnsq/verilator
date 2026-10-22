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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3AstNodeExpr.h"
#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

స్థిరరాశి CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

శూన్య CovergroupRefBindings::కూడు_Construction(స్థిరరాశి AstClass* classp, స్థిరరాశి AstVarScope* instp,
                                            స్థిరరాశి Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.చొప్పించు(classBindings.చివర(), bindings.begin(), bindings.చివర());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    ఒకవేళ (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.చొప్పించు(instBindings.చివర(), bindings.begin(), bindings.చివర());
    }
}

స్థిరరాశి CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(స్థిరరాశి AstVarScope* instp, స్థిరరాశి AstClass* classp) స్థిరరాశి {
    ఒకవేళ (instp) {
        స్థిరరాశి స్వయంచాలక it = m_byInstance.కనుగొను(instp);
        ఒకవేళ (it != m_byInstance.చివర()) {
            ++m_అంకె_Exact_Calls;
            తిరిగి it->రెండవ;
        }
    }
    స్థిరరాశి స్వయంచాలక it = m_byClass.కనుగొను(classp);
    ఒకవేళ (it != m_byClass.చివర()) {
        ++m_అంకె_Union_Calls;
        తిరిగి it->రెండవ;
    }
    // A covergroup with no reference formal at all
    తిరిగి s_none;
}

తరగతి CovergroupRefBindVisitor final : బహిరంగ VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  //ఫలితం
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_సమితి<స్థిరరాశి AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    స్థిర AstClass* constructedCovergroup(స్థిరరాశి Ast_కణుపు_CCall* nodep) {
        స్థిరరాశి AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        ఒకవేళ (!funcp->isConstructor()) తిరిగి శూన్యసూచిక;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* స్థిరరాశి classp = VN_AS(funcp->scopep()->modp(), Class);
        తిరిగి classp->isCovergroup() ? classp : శూన్యసూచిక;
    }

    // True if this variable holds a handle to a covergroup object
    స్థిర తార్కిక isCovergroupHandle(స్థిరరాశి AstVar* varp) {
        స్థిరరాశి AstClassRefDType* స్థిరరాశి dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        తిరిగి dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    స్థిర తార్కిక hasRefFormal(స్థిరరాశి AstCFunc* funcp) {
        కొరకు (Ast_కణుపు* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_AS(portp, Var);
            ఒకవేళ (varp->declDirection().isRef() || varp->declDirection().isConstRef()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    శూన్య రికార్డు_Construction(Ast_కణుపు_CCall* nodep, స్థిరరాశి AstClass* classp,
                            స్థిరరాశి AstVarScope* instp) {
        ఒకవేళ (!hasRefFormal(nodep->funcp())) తిరిగి;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_కణుపు* actualp = nodep->argsp();
        కొరకు (Ast_కణుపు* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_కణుపు* స్థిరరాశి thisActualp = actualp;
            actualp = actualp->nextp();
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_AS(portp, Var);
            ఒకవేళ (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) కొనసాగు;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* స్థిరరాశి vscp = refp->varScopep();
                bindings.నెట్టు_వెనుక(vscp);
            });
        }
        m_bindings.కూడు_Construction(classp, instp, bindings);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        AstCNew* స్థిరరాశి cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* స్థిరరాశి lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* స్థిరరాశి classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : శూన్యసూచిక;
        ఒకవేళ (!classp) {
            iterateChildrenConst(nodep);
            తిరిగి;
        }
        రికార్డు_Construction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = constructedCovergroup(nodep)) {
            రికార్డు_Construction(nodep, classp, శూన్యసూచిక);
        }
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!nodep->access().isWriteOrRW()) తిరిగి;
        ఒకవేళ (!isCovergroupHandle(nodep->varp())) తిరిగి;
        m_tainted.emplace(nodep->varScopep());
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        కొరకు (స్థిరరాశి AstVarScope* స్థిరరాశి vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = అప్రమేయ;

    // METHODS
    CovergroupRefBindings take_bindings() { తిరిగి std::move(m_bindings); }
};

స్థిరరాశి CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    ఒకవేళ (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    తిరిగి bindings;
}

}  // namespace V3Sched
