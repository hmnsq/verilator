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

स्थिरांक CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

शून्य CovergroupRefBindings::addConstruction(स्थिरांक AstClass* classp, स्थिरांक AstVarScope* instp,
                                            स्थिरांक Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.घाला(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    जर (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.घाला(instBindings.end(), bindings.begin(), bindings.end());
    }
}

स्थिरांक CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(स्थिरांक AstVarScope* instp, स्थिरांक AstClass* classp) स्थिरांक {
    जर (instp) {
        स्थिरांक स्वतः it = m_byInstance.find(instp);
        जर (it != m_byInstance.end()) {
            ++m_अंक_Exact_Calls;
            परत it->second;
        }
    }
    स्थिरांक स्वतः it = m_byClass.find(classp);
    जर (it != m_byClass.end()) {
        ++m_अंक_Union_Calls;
        परत it->second;
    }
    // A covergroup with no reference formal at all
    परत s_none;
}

वर्ग CovergroupRefBindVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<स्थिरांक AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    स्थिर AstClass* constructedCovergroup(स्थिरांक Ast_गाठ_CCall* nodep) {
        स्थिरांक AstCFunc* स्थिरांक funcp = nodep->funcp();
        जर (!funcp->isConstructor()) परत nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* स्थिरांक classp = VN_AS(funcp->scopep()->modp(), Class);
        परत classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    स्थिर तार्किक isCovergroupHandle(स्थिरांक AstVar* varp) {
        स्थिरांक AstClassRefDType* स्थिरांक dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        परत dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    स्थिर तार्किक hasRefFormal(स्थिरांक AstCFunc* funcp) {
        साठी (Ast_गाठ* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            स्थिरांक AstVar* स्थिरांक varp = VN_AS(portp, Var);
            जर (varp->declDirection().isRef() || varp->declDirection().isConstRef()) परत सत्य;
        }
        परत असत्य;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    शून्य recordConstruction(Ast_गाठ_CCall* nodep, स्थिरांक AstClass* classp,
                            स्थिरांक AstVarScope* instp) {
        जर (!hasRefFormal(nodep->funcp())) परत;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_गाठ* actualp = nodep->argsp();
        साठी (Ast_गाठ* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_गाठ* स्थिरांक thisActualp = actualp;
            actualp = actualp->nextp();
            स्थिरांक AstVar* स्थिरांक varp = VN_AS(portp, Var);
            जर (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) सुरूठेवा;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* स्थिरांक vscp = refp->varScopep();
                bindings.ढकला_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        AstCNew* स्थिरांक cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* स्थिरांक lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* स्थिरांक classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        जर (!classp) {
            iterateChildrenConst(nodep);
            परत;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    शून्य visit(Ast_गाठ_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        जर (स्थिरांक AstClass* स्थिरांक classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    शून्य visit(AstVarRef* nodep) override {
        जर (!nodep->access().isWriteOrRW()) परत;
        जर (!isCovergroupHandle(nodep->varp())) परत;
        m_tainted.emplace(nodep->varScopep());
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        साठी (स्थिरांक AstVarScope* स्थिरांक vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = पूर्वनिर्धारित;

    // METHODS
    CovergroupRefBindings take_bindings() { परत std::move(m_bindings); }
};

स्थिरांक CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    जर (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    परत bindings;
}

}  // namespace V3Sched
