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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3AstNodeExpr.h"
#അന്തര്ഭാവ "V3Sched.h"

#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

സ്ഥിരം CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

ശൂന്യം CovergroupRefBindings::addConstruction(സ്ഥിരം AstClass* classp, സ്ഥിരം AstVarScope* instp,
                                            സ്ഥിരം Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.തിരുകുക(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    എങ്കിൽ (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.തിരുകുക(instBindings.end(), bindings.begin(), bindings.end());
    }
}

സ്ഥിരം CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(സ്ഥിരം AstVarScope* instp, സ്ഥിരം AstClass* classp) സ്ഥിരം {
    എങ്കിൽ (instp) {
        സ്ഥിരം auto it = m_byInstance.find(instp);
        എങ്കിൽ (it != m_byInstance.end()) {
            ++m_അക്കം_Exact_Calls;
            തിരികെ it->second;
        }
    }
    സ്ഥിരം auto it = m_byClass.find(classp);
    എങ്കിൽ (it != m_byClass.end()) {
        ++m_അക്കം_Union_Calls;
        തിരികെ it->second;
    }
    // A covergroup with no reference formal at all
    തിരികെ s_none;
}

വർഗ്ഗം CovergroupRefBindVisitor final : പൊതു VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<സ്ഥിരം AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    സ്ഥിതം AstClass* constructedCovergroup(സ്ഥിരം Ast_കണ്ണി_CCall* nodep) {
        സ്ഥിരം AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        എങ്കിൽ (!funcp->isConstructor()) തിരികെ nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* സ്ഥിരം classp = VN_AS(funcp->scopep()->modp(), Class);
        തിരികെ classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    സ്ഥിതം തർക്കം isCovergroupHandle(സ്ഥിരം AstVar* varp) {
        സ്ഥിരം AstClassRefDType* സ്ഥിരം dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        തിരികെ dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    സ്ഥിതം തർക്കം hasRefFormal(സ്ഥിരം AstCFunc* funcp) {
        വേണ്ടി (Ast_കണ്ണി* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_AS(portp, Var);
            എങ്കിൽ (varp->declDirection().isRef() || varp->declDirection().isConstRef()) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    ശൂന്യം recordConstruction(Ast_കണ്ണി_CCall* nodep, സ്ഥിരം AstClass* classp,
                            സ്ഥിരം AstVarScope* instp) {
        എങ്കിൽ (!hasRefFormal(nodep->funcp())) തിരികെ;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_കണ്ണി* actualp = nodep->argsp();
        വേണ്ടി (Ast_കണ്ണി* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_കണ്ണി* സ്ഥിരം thisActualp = actualp;
            actualp = actualp->nextp();
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_AS(portp, Var);
            എങ്കിൽ (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) തുടരുക;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* സ്ഥിരം vscp = refp->varScopep();
                bindings.തള്ളുക_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        AstCNew* സ്ഥിരം cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* സ്ഥിരം lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* സ്ഥിരം classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        എങ്കിൽ (!classp) {
            iterateChildrenConst(nodep);
            തിരികെ;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!nodep->access().isWriteOrRW()) തിരികെ;
        എങ്കിൽ (!isCovergroupHandle(nodep->varp())) തിരികെ;
        m_tainted.emplace(nodep->varScopep());
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        വേണ്ടി (സ്ഥിരം AstVarScope* സ്ഥിരം vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = സ്ഥിരസ്ഥിതി;

    // METHODS
    CovergroupRefBindings take_bindings() { തിരികെ std::move(m_bindings); }
};

സ്ഥിരം CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    എങ്കിൽ (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    തിരികെ bindings;
}

}  // namespace V3Sched
