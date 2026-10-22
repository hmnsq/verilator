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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3AstNodeExpr.h"
#અન્તર્ભાવ "V3Sched.h"

#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

અચલ CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

શૂન્ય CovergroupRefBindings::addConstruction(અચલ AstClass* classp, અચલ AstVarScope* instp,
                                            અચલ Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.ઉમેરો(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    જો (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.ઉમેરો(instBindings.end(), bindings.begin(), bindings.end());
    }
}

અચલ CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(અચલ AstVarScope* instp, અચલ AstClass* classp) અચલ {
    જો (instp) {
        અચલ auto it = m_byInstance.find(instp);
        જો (it != m_byInstance.end()) {
            ++m_અંક_Exact_Calls;
            પાછા it->second;
        }
    }
    અચલ auto it = m_byClass.find(classp);
    જો (it != m_byClass.end()) {
        ++m_અંક_Union_Calls;
        પાછા it->second;
    }
    // A covergroup with no reference formal at all
    પાછા s_none;
}

વર્ગ CovergroupRefBindVisitor final : જાહેર VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<અચલ AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    સ્થિર AstClass* constructedCovergroup(અચલ Ast_ગાંઠ_CCall* nodep) {
        અચલ AstCFunc* અચલ funcp = nodep->funcp();
        જો (!funcp->isConstructor()) પાછા nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* અચલ classp = VN_AS(funcp->scopep()->modp(), Class);
        પાછા classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    સ્થિર તાર્કિક isCovergroupHandle(અચલ AstVar* varp) {
        અચલ AstClassRefDType* અચલ dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        પાછા dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    સ્થિર તાર્કિક hasRefFormal(અચલ AstCFunc* funcp) {
        માટે (Ast_ગાંઠ* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            અચલ AstVar* અચલ varp = VN_AS(portp, Var);
            જો (varp->declDirection().isRef() || varp->declDirection().isConstRef()) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    શૂન્ય recordConstruction(Ast_ગાંઠ_CCall* nodep, અચલ AstClass* classp,
                            અચલ AstVarScope* instp) {
        જો (!hasRefFormal(nodep->funcp())) પાછા;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_ગાંઠ* actualp = nodep->argsp();
        માટે (Ast_ગાંઠ* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_ગાંઠ* અચલ thisActualp = actualp;
            actualp = actualp->nextp();
            અચલ AstVar* અચલ varp = VN_AS(portp, Var);
            જો (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) ચાલુ;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* અચલ vscp = refp->varScopep();
                bindings.ધકેલો_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        AstCNew* અચલ cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* અચલ lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* અચલ classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        જો (!classp) {
            iterateChildrenConst(nodep);
            પાછા;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        જો (અચલ AstClass* અચલ classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!nodep->access().isWriteOrRW()) પાછા;
        જો (!isCovergroupHandle(nodep->varp())) પાછા;
        m_tainted.emplace(nodep->varScopep());
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        માટે (અચલ AstVarScope* અચલ vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = મૂળભૂત;

    // METHODS
    CovergroupRefBindings take_bindings() { પાછા std::move(m_bindings); }
};

અચલ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    જો (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    પાછા bindings;
}

}  // namespace V3Sched
