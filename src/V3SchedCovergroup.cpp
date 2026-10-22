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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3AstNodeExpr.h"
#समावेश "V3Sched.h"

#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

स्थिर CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

व्योम CovergroupRefBindings::addConstruction(स्थिर AstClass* classp, स्थिर AstVarScope* instp,
                                            स्थिर Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.जोड़ो(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    अगर (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.जोड़ो(instBindings.end(), bindings.begin(), bindings.end());
    }
}

स्थिर CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(स्थिर AstVarScope* instp, स्थिर AstClass* classp) स्थिर {
    अगर (instp) {
        स्थिर स्वतः it = m_byInstance.find(instp);
        अगर (it != m_byInstance.end()) {
            ++m_अंक_Exact_Calls;
            वापस it->second;
        }
    }
    स्थिर स्वतः it = m_byClass.find(classp);
    अगर (it != m_byClass.end()) {
        ++m_अंक_Union_Calls;
        वापस it->second;
    }
    // A covergroup with no reference formal at all
    वापस s_none;
}

श्रेणी CovergroupRefBindVisitor final : खुला VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  //परिणाम
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<स्थिर AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    अटल AstClass* constructedCovergroup(स्थिर Ast_गाँठ_CCall* nodep) {
        स्थिर AstCFunc* स्थिर funcp = nodep->funcp();
        अगर (!funcp->isConstructor()) वापस nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* स्थिर classp = VN_AS(funcp->scopep()->modp(), Class);
        वापस classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    अटल bool isCovergroupHandle(स्थिर AstVar* varp) {
        स्थिर AstClassRefDType* स्थिर dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        वापस dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    अटल bool hasRefFormal(स्थिर AstCFunc* funcp) {
        क्रम (Ast_गाँठ* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            स्थिर AstVar* स्थिर varp = VN_AS(portp, Var);
            अगर (varp->declDirection().isRef() || varp->declDirection().isConstRef()) वापस true;
        }
        वापस false;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    व्योम recordConstruction(Ast_गाँठ_CCall* nodep, स्थिर AstClass* classp,
                            स्थिर AstVarScope* instp) {
        अगर (!hasRefFormal(nodep->funcp())) वापस;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_गाँठ* actualp = nodep->argsp();
        क्रम (Ast_गाँठ* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_गाँठ* स्थिर thisActualp = actualp;
            actualp = actualp->nextp();
            स्थिर AstVar* स्थिर varp = VN_AS(portp, Var);
            अगर (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) जारी;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* स्थिर vscp = refp->varScopep();
                bindings.धकेलो_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        AstCNew* स्थिर cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* स्थिर lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* स्थिर classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        अगर (!classp) {
            iterateChildrenConst(nodep);
            वापस;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        अगर (स्थिर AstClass* स्थिर classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    व्योम visit(AstVarRef* nodep) override {
        अगर (!nodep->access().isWriteOrRW()) वापस;
        अगर (!isCovergroupHandle(nodep->varp())) वापस;
        m_tainted.emplace(nodep->varScopep());
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        क्रम (स्थिर AstVarScope* स्थिर vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = शेष;

    // METHODS
    CovergroupRefBindings take_bindings() { वापस std::move(m_bindings); }
};

स्थिर CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    अगर (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    वापस bindings;
}

}  // namespace V3Sched
