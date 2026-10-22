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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Localize.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Stats.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

श्रेणी LocalizeVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser3InUse m_user3InUse;
    स्थिर VNUser4InUse m_user4InUse;

    AstUser3Allocator<AstVarScope, std::unordered_set<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<स्थिर AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::vector<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current active function
    uint32_t m_गाँठ_Depth = 0;  // Node depth under m_cfuncp

    // METHODS
    bool isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        अगर (VN_IS(nodep->dtypep(), NBACommitQueueDType)) वापस false;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        अगर (nodep->dtypep()->skipRefp()->isString()) वापस false;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        वापस ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).आकार() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->width_योग_Bytes() <= v3Global.opt.localize_Max_आकार());
    }

    अटल bool existsNonLeaf(स्थिर std::unordered_set<AstCFunc*>& funcps) {
        क्रम (स्थिर AstCFunc* स्थिर funcp : funcps) {
            अगर (funcp->user1()) वापस true;
        }
        वापस false;
    }

    व्योम moveVarScopes() {
        क्रम (AstVarScope* स्थिर nodep : m_varScopeps) {
            अगर (!isOptimizable(nodep)) जारी;  // Not optimizable

            स्थिर std::unordered_set<AstCFunc*>& funcps = m_accessors(nodep);
            अगर (funcps.empty()) जारी;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            अगर (funcps.आकार() > 1 && existsNonLeaf(funcps)) जारी;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            धकेलो_Deletep(nodep->unlinkFrBack());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* स्थिर oldVarp = nodep->varp();
            क्रम (AstCFunc* स्थिर funcp : funcps) {
                // Create the new local variable.
                स्थिर string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* स्थिर newVarp
                    = नया AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(true);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->addVarsp(newVarp);

                // Fix up all the references within this function
                स्थिर स्वतः er = m_references(funcp).equal_range(nodep);
                क्रम (स्वतः it = er.first; it != er.second; ++it) {
                    AstVarRef* स्थिर refp = it->second;
                    refp->varScopep(nullptr);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.सफा();
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    व्योम visit(AstCAwait* nodep) override {
        m_cfuncp->user1(true);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_गाँठ_Depth);
        m_cfuncp = nodep;
        m_गाँठ_Depth = 0;
        स्थिर VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstCNew* nodep) override {
        m_cfuncp->user1(true);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    व्योम visit(AstCCall* nodep) override {
        m_cfuncp->user1(true);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        अगर (m_गाँठ_Depth == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    व्योम visit(AstVarScope* nodep) override {
        अगर (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->isSigPublic()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->constPoolEntry()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.धकेलो_back(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    व्योम visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* स्थिर varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).जोड़ो(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        अगर (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            अगर (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        VL_RESTORER(m_गाँठ_Depth);
        ++m_गाँठ_Depth;
        iterateChildrenConst(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::addStat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

व्योम V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("localize", 0, dump_वृक्ष_Either_Level() >= 6);
}
