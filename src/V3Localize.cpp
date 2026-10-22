// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert BLOCKTEMPs to local variables
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
// LOCALIZE TRANSFORMATIONS:
//      All modules:
//          VARSCOPE(BLOCKTEMP...
//             if only referenced in one CFUNC, make it local
//          VARSCOPE
//             if non-public, always written before used, make it local
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Localize.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

صنف LocalizeVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;
    ثابت VNUser3_وچ_Use m_user3_وچ_Use;
    ثابت VNUser4_وچ_Use m_user4_وچ_Use;

    AstUser3Allocator<AstVarScope, std::unordered_سیٹ<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<ثابت AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::سمتیہ<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = خالی_سوچک;  // Current active function
    uint32_t m_گرہ_ڈونگھائی = 0;  // Node depth under m_cfuncp

    // METHODS
    منطقي isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        إذا (VN_IS(nodep->dtypep(), NBACommit_قطار_DType)) أرجع خطأ;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        إذا (nodep->dtypep()->skipRefp()->isString()) أرجع خطأ;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        أرجع ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).ناپ() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->چوڑائی_کل_بائٹاں() <= v3Global.opt.localize_Max_ناپ());
    }

    ساكن منطقي exists_Non_پتی(ثابت std::unordered_سیٹ<AstCFunc*>& funcps) {
        لكل (ثابت AstCFunc* ثابت funcp : funcps) {
            إذا (funcp->user1()) أرجع سچ;
        }
        أرجع خطأ;
    }

    فراغ moveVarScopes() {
        لكل (AstVarScope* ثابت nodep : m_varScopeps) {
            إذا (!isOptimizable(nodep)) استمر;  // Not optimizable

            ثابت std::unordered_سیٹ<AstCFunc*>& funcps = m_accessors(nodep);
            إذا (funcps.خالی_تھاواں()) استمر;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            إذا (funcps.ناپ() > 1 && exists_Non_پتی(funcps)) استمر;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            ڈالو_Deletep(nodep->unlink_Fr_پٹھ());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* ثابت oldVarp = nodep->varp();
            لكل (AstCFunc* ثابت funcp : funcps) {
                // Create the new local variable.
                ثابت string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* ثابت newVarp
                    = جديد AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(سچ);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->جوڑو_Varsp(newVarp);

                // Fix up all the references within this function
                ثابت خودکار er = m_references(funcp).equal_range(nodep);
                لكل (خودکار it = er.پہلا; it != er.دوجا; ++it) {
                    AstVarRef* ثابت refp = it->دوجا;
                    refp->varScopep(خالی_سوچک);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.clear();
    }

    // VISITORS
    فراغ ویکھو(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    فراغ ویکھو(AstCAwait* nodep) override {
        m_cfuncp->user1(سچ);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_گرہ_ڈونگھائی);
        m_cfuncp = nodep;
        m_گرہ_ڈونگھائی = 0;
        ثابت VNUser2_وچ_Use user2_وچ_Use;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstCNew* nodep) override {
        m_cfuncp->user1(سچ);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstCCall* nodep) override {
        m_cfuncp->user1(سچ);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        إذا (m_گرہ_ڈونگھائی == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    فراغ ویکھو(AstVarScope* nodep) override {
        إذا (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->is_Sig_عوامی()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->const_Pool_اندراج()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.ڈالو_پٹھ(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    فراغ ویکھو(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* ثابت varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).داخل_کرو(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        إذا (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            إذا (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    فراغ ویکھو(Ast_گرہ* nodep) override {
        VL_RESTORER(m_گرہ_ڈونگھائی);
        ++m_گرہ_ڈونگھائی;
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

فراغ V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("localize", 0, dump_درخت_Either_Level() >= 6);
}
