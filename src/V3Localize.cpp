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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Localize.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

वर्ग LocalizeVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;
    ध्रुव VNUser3_अन्तर्_Use m_user3_अन्तर्_Use;
    ध्रुव VNUser4_अन्तर्_Use m_user4_अन्तर्_Use;

    AstUser3Allocator<AstVarScope, std::unordered_समुच्चयः<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<ध्रुव AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::सदिशम्<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = शून्यसूचकः;  // Current active function
    uint32_t m_ग्रन्थिः_गभीरता = 0;  // Node depth under m_cfuncp

    // METHODS
    तार्किक isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        यदि (VN_IS(nodep->dtypep(), NBACommitQueueDType)) प्रत्यागम असत्य;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        यदि (nodep->dtypep()->skipRefp()->isString()) प्रत्यागम असत्य;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        प्रत्यागम ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).आकार() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->विस्तारः_योग_अष्टकानि() <= v3Global.opt.localize_Max_आकार());
    }

    स्थिर तार्किक exists_Non_पर्णम्(ध्रुव std::unordered_समुच्चयः<AstCFunc*>& funcps) {
        प्रति (ध्रुव AstCFunc* ध्रुव funcp : funcps) {
            यदि (funcp->user1()) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    शून्य moveVarScopes() {
        प्रति (AstVarScope* ध्रुव nodep : m_varScopeps) {
            यदि (!isOptimizable(nodep)) अनुवर्त;  // Not optimizable

            ध्रुव std::unordered_समुच्चयः<AstCFunc*>& funcps = m_accessors(nodep);
            यदि (funcps.रिक्तानि()) अनुवर्त;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            यदि (funcps.आकार() > 1 && exists_Non_पर्णम्(funcps)) अनुवर्त;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* ध्रुव oldVarp = nodep->varp();
            प्रति (AstCFunc* ध्रुव funcp : funcps) {
                // Create the new local variable.
                ध्रुव string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* ध्रुव newVarp
                    = नूतन AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(सत्य);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->सङ्कलय_Varsp(newVarp);

                // Fix up all the references within this function
                ध्रुव स्वचालित er = m_references(funcp).equal_range(nodep);
                प्रति (स्वचालित it = er.first; it != er.second; ++it) {
                    AstVarRef* ध्रुव refp = it->second;
                    refp->varScopep(शून्यसूचकः);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.clear();
    }

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    शून्य दर्शय(AstCAwait* nodep) override {
        m_cfuncp->user1(सत्य);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_ग्रन्थिः_गभीरता);
        m_cfuncp = nodep;
        m_ग्रन्थिः_गभीरता = 0;
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstCNew* nodep) override {
        m_cfuncp->user1(सत्य);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCCall* nodep) override {
        m_cfuncp->user1(सत्य);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        यदि (m_ग्रन्थिः_गभीरता == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    शून्य दर्शय(AstVarScope* nodep) override {
        यदि (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->is_Sig_लौकिकम्()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->const_Pool_प्रविष्टिः()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.प्रक्षिप_पृष्ठभागः(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* ध्रुव varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).निवेशय(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        यदि (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            यदि (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        VL_RESTORER(m_ग्रन्थिः_गभीरता);
        ++m_ग्रन्थिः_गभीरता;
        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::सङ्कलय_Stat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

शून्य V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("localize", 0, dump_वृक्षः_Either_Level() >= 6);
}
