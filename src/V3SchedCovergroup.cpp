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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AstNodeExpr.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

अचर CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

शून्य CovergroupRefBindings::addConstruction(अचर AstClass* classp, अचर AstVarScope* instp,
                                            अचर Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.घुसाउ(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    यदि (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.घुसाउ(instBindings.end(), bindings.begin(), bindings.end());
    }
}

अचर CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(अचर AstVarScope* instp, अचर AstClass* classp) अचर {
    यदि (instp) {
        अचर स्वतः it = m_byInstance.find(instp);
        यदि (it != m_byInstance.end()) {
            ++m_अङ्क_Exact_Calls;
            फर्काऊ it->second;
        }
    }
    अचर स्वतः it = m_byClass.find(classp);
    यदि (it != m_byClass.end()) {
        ++m_अङ्क_Union_Calls;
        फर्काऊ it->second;
    }
    // A covergroup with no reference formal at all
    फर्काऊ s_none;
}

वर्ग CovergroupRefBindVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<अचर AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    स्थिर AstClass* constructedCovergroup(अचर Ast_गाँठो_CCall* nodep) {
        अचर AstCFunc* अचर funcp = nodep->funcp();
        यदि (!funcp->isConstructor()) फर्काऊ nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* अचर classp = VN_AS(funcp->scopep()->modp(), Class);
        फर्काऊ classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    स्थिर तार्किक isCovergroupHandle(अचर AstVar* varp) {
        अचर AstClassRefDType* अचर dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        फर्काऊ dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    स्थिर तार्किक hasRefFormal(अचर AstCFunc* funcp) {
        लागि (Ast_गाँठो* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            अचर AstVar* अचर varp = VN_AS(portp, Var);
            यदि (varp->declDirection().isRef() || varp->declDirection().isConstRef()) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    शून्य recordConstruction(Ast_गाँठो_CCall* nodep, अचर AstClass* classp,
                            अचर AstVarScope* instp) {
        यदि (!hasRefFormal(nodep->funcp())) फर्काऊ;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_गाँठो* actualp = nodep->argsp();
        लागि (Ast_गाँठो* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_गाँठो* अचर thisActualp = actualp;
            actualp = actualp->nextp();
            अचर AstVar* अचर varp = VN_AS(portp, Var);
            यदि (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) जारी;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* अचर vscp = refp->varScopep();
                bindings.थप_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        AstCNew* अचर cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* अचर lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* अचर classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        यदि (!classp) {
            iterateChildrenConst(nodep);
            फर्काऊ;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    शून्य visit(Ast_गाँठो_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        यदि (अचर AstClass* अचर classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (!nodep->access().isWriteOrRW()) फर्काऊ;
        यदि (!isCovergroupHandle(nodep->varp())) फर्काऊ;
        m_tainted.emplace(nodep->varScopep());
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        लागि (अचर AstVarScope* अचर vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = स्वभाव;

    // METHODS
    CovergroupRefBindings take_bindings() { फर्काऊ std::move(m_bindings); }
};

अचर CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    यदि (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    फर्काऊ bindings;
}

}  // namespace V3Sched
