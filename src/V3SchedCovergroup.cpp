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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AstNodeExpr.h"
#شامل "V3Sched.h"

#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام V3Sched {

ثابت CovergroupRefBindings::Bindings CovergroupRefBindings::s_none;

تهی CovergroupRefBindings::addConstruction(ثابت AstClass* classp, ثابت AstVarScope* instp,
                                            ثابت Bindings& bindings) {
    Bindings& classBindings = m_byClass[classp];
    classBindings.درج(classBindings.end(), bindings.begin(), bindings.end());
    // Note this creates an entry even when 'bindings' is empty, so that a handle binding
    // nothing stays distinguishable from a handle we know nothing about
    اگر (instp) {
        Bindings& instBindings = m_byInstance[instp];
        instBindings.درج(instBindings.end(), bindings.begin(), bindings.end());
    }
}

ثابت CovergroupRefBindings::Bindings&
CovergroupRefBindings::forSample(ثابت AstVarScope* instp, ثابت AstClass* classp) ثابت {
    اگر (instp) {
        ثابت auto it = m_byInstance.find(instp);
        اگر (it != m_byInstance.end()) {
            ++m_رقم_Exact_Calls;
            بازگشت it->second;
        }
    }
    ثابت auto it = m_byClass.find(classp);
    اگر (it != m_byClass.end()) {
        ++m_رقم_Union_Calls;
        بازگشت it->second;
    }
    // A covergroup with no reference formal at all
    بازگشت s_none;
}

رده CovergroupRefBindVisitor final : عمومی VNVisitorConst {
    // STATE
    CovergroupRefBindings m_bindings;  // Result
    // Covergroup handles written by something other than a construction we recognized
    std::unordered_set<ثابت AstVarScope*> m_tainted;

    // METHODS

    // The covergroup class this call constructs, or nullptr if it is not a covergroup
    // construction
    ایستا AstClass* constructedCovergroup(ثابت Ast_گره_CCall* nodep) {
        ثابت AstCFunc* ثابت funcp = nodep->funcp();
        اگر (!funcp->isConstructor()) بازگشت nullptr;
        // A constructor is a class method, so it is scoped and its scope is that of a class
        AstClass* ثابت classp = VN_AS(funcp->scopep()->modp(), Class);
        بازگشت classp->isCovergroup() ? classp : nullptr;
    }

    // True if this variable holds a handle to a covergroup object
    ایستا منطقی isCovergroupHandle(ثابت AstVar* varp) {
        ثابت AstClassRefDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        بازگشت dtypep && dtypep->classp()->isCovergroup();
    }

    // True if this constructor takes a reference formal, and so has anything to bind
    ایستا منطقی hasRefFormal(ثابت AstCFunc* funcp) {
        برای (Ast_گره* portp = funcp->argsp(); portp; portp = portp->nextp()) {
            ثابت AstVar* ثابت varp = VN_AS(portp, Var);
            اگر (varp->declDirection().isRef() || varp->declDirection().isConstRef()) بازگشت درست;
        }
        بازگشت نادرست;
    }

    // Record one construction of 'classp' assigning to 'instp' (nullptr if not identified).
    // A covergroup with no reference formal has nothing to bind, and is left out entirely, so
    // that an entry with no bindings means only 'this handle binds nothing a sample() reads'.
    تهی recordConstruction(Ast_گره_CCall* nodep, ثابت AstClass* classp,
                            ثابت AstVarScope* instp) {
        اگر (!hasRefFormal(nodep->funcp())) بازگشت;
        CovergroupRefBindings::Bindings bindings;
        // Actuals correspond one to one, in order, with the function's argument variables.
        // A constructor returns void, so none of them is a return value variable.
        Ast_گره* actualp = nodep->argsp();
        برای (Ast_گره* portp = nodep->funcp()->argsp(); portp; portp = portp->nextp()) {
            Ast_گره* ثابت thisActualp = actualp;
            actualp = actualp->nextp();
            ثابت AstVar* ثابت varp = VN_AS(portp, Var);
            اگر (!varp->declDirection().isRef() && !varp->declDirection().isConstRef()) ادامه;
            // A 'ref' actual is an lvalue expression, so it need not be a plain variable
            // reference. Bind every variable it names: for 'sigs[0]' that is 'sigs', which is
            // exact rather than approximate, as V3Order models the whole array as one
            // VarScope. An index expression contributes its own variables as well, which is an
            // over-approximation, and so safe.
            thisActualp->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                bindings.افزودن_back(vscp);
            });
        }
        m_bindings.addConstruction(classp, instp, bindings);
    }

    // VISITORS
    تهی visit(Ast_گره_Assign* nodep) override {
        AstCNew* ثابت cnewp = VN_CAST(nodep->rhsp(), CNew);
        AstVarRef* ثابت lhsRefp = VN_CAST(nodep->lhsp(), VarRef);
        AstClass* ثابت classp = cnewp && lhsRefp ? constructedCovergroup(cnewp) : nullptr;
        اگر (!classp) {
            iterateChildrenConst(nodep);
            بازگشت;
        }
        recordConstruction(cnewp, classp, lhsRefp->varScopep());
        // Deliberately not iterating the destination: this write is the one shape that does not
        // taint the handle. Do iterate the arguments, which may write handles of their own.
        iterateChildrenConst(cnewp);
    }

    تهی visit(Ast_گره_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // A construction reached only here is one whose destination we could not identify, so
        // every handle of the class must assume it
        اگر (ثابت AstClass* ثابت classp = constructedCovergroup(nodep)) {
            recordConstruction(nodep, classp, nullptr);
        }
    }

    تهی visit(AstVarRef* nodep) override {
        اگر (!nodep->access().isWriteOrRW()) بازگشت;
        اگر (!isCovergroupHandle(nodep->varp())) بازگشت;
        m_tainted.emplace(nodep->varScopep());
    }

    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit CovergroupRefBindVisitor(AstNetlist* nodep) {
        iterateConst(nodep);
        برای (ثابت AstVarScope* ثابت vscp : m_tainted) m_bindings.dropInstance(vscp);
    }
    ~CovergroupRefBindVisitor() override = پیشفرض;

    // METHODS
    CovergroupRefBindings take_bindings() { بازگشت std::move(m_bindings); }
};

ثابت CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CovergroupRefBindings bindings{};
    اگر (v3Global.useCovergroup()) bindings = CovergroupRefBindVisitor{nodep}.take_bindings();
    بازگشت bindings;
}

}  // namespace V3Sched
