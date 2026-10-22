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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Localize.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

വർഗ്ഗം LocalizeVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser3InUse m_user3InUse;
    സ്ഥിരം VNUser4InUse m_user4InUse;

    AstUser3Allocator<AstVarScope, std::unordered_set<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<സ്ഥിരം AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::vector<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current active function
    uint32_t m_കണ്ണി_Depth = 0;  // Node depth under m_cfuncp

    // METHODS
    തർക്കം isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        എങ്കിൽ (VN_IS(nodep->dtypep(), NBACommitQueueDType)) തിരികെ അസത്യം;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        എങ്കിൽ (nodep->dtypep()->skipRefp()->isString()) തിരികെ അസത്യം;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        തിരികെ ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).size() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->widthTotalBytes() <= v3Global.opt.localizeMaxSize());
    }

    സ്ഥിതം തർക്കം existsNonLeaf(സ്ഥിരം std::unordered_set<AstCFunc*>& funcps) {
        വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം funcp : funcps) {
            എങ്കിൽ (funcp->user1()) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    ശൂന്യം moveVarScopes() {
        വേണ്ടി (AstVarScope* സ്ഥിരം nodep : m_varScopeps) {
            എങ്കിൽ (!isOptimizable(nodep)) തുടരുക;  // Not optimizable

            സ്ഥിരം std::unordered_set<AstCFunc*>& funcps = m_accessors(nodep);
            എങ്കിൽ (funcps.empty()) തുടരുക;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            എങ്കിൽ (funcps.size() > 1 && existsNonLeaf(funcps)) തുടരുക;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            തള്ളുക_Deletep(nodep->unlinkFrBack());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* സ്ഥിരം oldVarp = nodep->varp();
            വേണ്ടി (AstCFunc* സ്ഥിരം funcp : funcps) {
                // Create the new local variable.
                സ്ഥിരം string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* സ്ഥിരം newVarp
                    = പുതിയ AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(സത്യം);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->addVarsp(newVarp);

                // Fix up all the references within this function
                സ്ഥിരം auto er = m_references(funcp).equal_range(nodep);
                വേണ്ടി (auto it = er.first; it != er.second; ++it) {
                    AstVarRef* സ്ഥിരം refp = it->second;
                    refp->varScopep(nullptr);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.clear();
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    ശൂന്യം visit(AstCAwait* nodep) override {
        m_cfuncp->user1(സത്യം);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_കണ്ണി_Depth);
        m_cfuncp = nodep;
        m_കണ്ണി_Depth = 0;
        സ്ഥിരം VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstCNew* nodep) override {
        m_cfuncp->user1(സത്യം);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCCall* nodep) override {
        m_cfuncp->user1(സത്യം);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        എങ്കിൽ (m_കണ്ണി_Depth == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    ശൂന്യം visit(AstVarScope* nodep) override {
        എങ്കിൽ (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->isSigPublic()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->constPoolEntry()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.തള്ളുക_back(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* സ്ഥിരം varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).തിരുകുക(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        എങ്കിൽ (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            എങ്കിൽ (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        VL_RESTORER(m_കണ്ണി_Depth);
        ++m_കണ്ണി_Depth;
        iterateChildrenConst(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::addStat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

ശൂന്യം V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("localize", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
