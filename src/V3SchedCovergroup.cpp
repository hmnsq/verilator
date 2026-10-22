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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3AstNodeExpr.h"
#அந்தர்பாவ "V3Sched.h"

#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

மாறிலி CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

வெறுமை CovergroupRefBindings::addConstruction(மாறிலி AstClass* classp, மாறிலி AstVarScope* instp,
                                            மாறிலி Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.செருகு(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    எனில் (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.செருகு(instBindings.end(), bindings.begin(), bindings.end());
    }
}

மாறிலி CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(மாறிலி AstVarScope* instp, மாறிலி AstClass* classp) மாறிலி {
    எனில் (instp) {
        மாறிலி auto it = m_byInstance.find(instp);
        எனில் (it != m_byInstance.end()) {
            ++m_இலக்கம்_Exact_Calls;
            திரும்பு it->second;
        }
    }
    மாறிலி auto it = m_byClass.find(classp);
    எனில் (it != m_byClass.end()) {
        ++m_இலக்கம்_Union_Calls;
        திரும்பு it->second;
    }
    // A covergroup with no reference formal at all
    திரும்பு s_none;
}

வகுப்பு CovergroupRefBindVisitor final : பொது VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<மாறிலி AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    நிலையான AstClass* constructedCovergroup(மாறிலி Ast_முனை_CCall* nodep) {
        மாறிலி AstCFunc* மாறிலி funcp = nodep->funcp();
        எனில் (!funcp->isConstructor()) திரும்பு nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* மாறிலி classp = VN_AS(funcp->scopep()->modp(), Class);
        திரும்பு classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    நிலையான தருக்க isCovergroupHandle(மாறிலி AstVar* varp) {
        மாறிலி AstClassRefDType* மாறிலி dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        திரும்பு dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    நிலையான தருக்க hasRefFormal(மாறிலி AstCFunc* funcp) {
        க்கு (Ast_முனை* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            மாறிலி AstVar* மாறிலி varp = VN_AS(portp, Var);
            எனில் (varp->declDirection().isRef() || varp->declDirection().isConstRef()) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    வெறுமை recordConstruction(Ast_முனை_CCall* nodep, மாறிலி AstClass* classp,
                            மாறிலி AstVarScope* instp) {
        எனில் (!hasRefFormal(nodep->funcp())) திரும்பு;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_முனை* actualp = nodep->argsp();
        க்கு (Ast_முனை* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_முனை* மாறிலி thisActualp = actualp;
            actualp = actualp->nextp();
            மாறிலி AstVar* மாறிலி varp = VN_AS(portp, Var);
            எனில் (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) தொடர்;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* மாறிலி vscp = refp->varScopep();
                bindings.தள்ளு_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        AstCNew* மாறிலி cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* மாறிலி lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* மாறிலி classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        எனில் (!classp) {
            iterateChildrenConst(nodep);
            திரும்பு;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        எனில் (மாறிலி AstClass* மாறிலி classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!nodep->access().isWriteOrRW()) திரும்பு;
        எனில் (!isCovergroupHandle(nodep->varp())) திரும்பு;
        m_tainted.emplace(nodep->varScopep());
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        க்கு (மாறிலி AstVarScope* மாறிலி vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = இயல்பு;

    // METHODS
    CovergroupRefBindings take_bindings() { திரும்பு std::move(m_bindings); }
};

மாறிலி CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    எனில் (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    திரும்பு bindings;
}

}  // namespace V3Sched
