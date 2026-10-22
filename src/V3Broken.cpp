// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Find broken links in tree
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
// V3Broken's Transformations:
//
// Entire netlist
//      Mark all nodes
//      Check all links point to marked nodes
//      Check local variables in CFuncs appear before they are referenced
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Broken.h"

#અન્તર્ભાવ "V3ConstPool.h"

#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

સ્થિર વર્ગ BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    સ્થિર constexpr uint8_t MIN_કિંમત = 1;
    સ્થિર constexpr uint8_t MAX_કિંમત = 127;

    uint8_t m_ગણતરી = MIN_કિંમત;

જાહેર:
    uint8_t get() અચલ VL_MT_SAFE {
        UASSERT(MIN_કિંમત <= m_ગણતરી && m_ગણતરી <= MAX_કિંમત, "Invalid generation number");
        પાછા m_ગણતરી;
    }

    શૂન્ય inc() {
        ++m_ગણતરી;
        જો (m_ગણતરી > MAX_કિંમત) m_ગણતરી = MIN_કિંમત;
    }
} s_brokenCntGlobal;

સ્થિર તાર્કિક s_brokenAllowMidvisitorCheck = ખોટું;

//######################################################################
// Table of allocated AstNode pointers

સ્થિર વર્ગ AllocTable final {
    friend વર્ગ V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<અચલ Ast_ગાંઠ*> m_allocated VL_GUARDED_BY(m_mutex);

જાહેર:
    // METHODS
    શૂન્ય addNewed(અચલ Ast_ગાંઠ* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        અચલ V3LockGuard lock{m_mutex};
        જો (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    શૂન્ય deleted(અચલ Ast_ગાંઠ* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        અચલ V3LockGuard lock{m_mutex};
        જો (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

ખાનગી:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    તાર્કિક isAllocated(અચલ Ast_ગાંઠ* nodep) અચલ VL_REQUIRES(m_mutex) {
        પાછા m_allocated.ગણતરી(nodep) != 0;
    }
    શૂન્ય checkForLeaks() VL_REQUIRES(m_mutex) {
        જો (!v3Global.opt.debugCheck()) પાછા;

        અચલ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        માટે (અચલ તાર્કિક withBack : {ખોટું, સાચું}) {
            માટે (અચલ Ast_ગાંઠ* અચલ nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                જો (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    અચલ તાર્કિક hasBack = nodep->backp() != nullptr;
                    જો (hasBack != withBack) ચાલુ;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_ગાંઠ::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

શૂન્ય V3Broken::addNewed(અચલ Ast_ગાંઠ* nodep) { s_allocTable.addNewed(nodep); }
શૂન્ય V3Broken::deleted(અચલ Ast_ગાંઠ* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

સ્થિર વર્ગ LinkableTable final {
    // MEMBERS
    std::unordered_set<અચલ Ast_ગાંઠ*> m_linkable;  // Set of all nodes allocated but not freed

જાહેર:
    // METHODS
    શૂન્ય clear() { m_linkable.clear(); }
    શૂન્ય addLinkable(અચલ Ast_ગાંઠ* nodep) { m_linkable.emplace(nodep); }
    તાર્કિક isLinkable(અચલ Ast_ગાંઠ* nodep) અચલ { પાછા m_linkable.ગણતરી(nodep) != 0; }
} s_linkableTable;

તાર્કિક V3Broken::isLinkable(અચલ Ast_ગાંઠ* nodep) { પાછા s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

વર્ગ BrokenCheckVisitor final : જાહેર VNVisitorConst {
    // Constants for marking we are under/not under a node
    અચલ uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    અચલ uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<અચલ AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<અચલ AstVar*, અચલ Ast_ગાંઠ_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<અચલ AstVar*>> m_locals_ઢગલો;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    અચલ AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    તાર્કિક m_inScope = ખોટું;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

ખાનગી:
    // METHODS
    સ્થિર શૂન્ય checkWidthMin(અચલ Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    શૂન્ય processEnter(Ast_ગાંઠ* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        અચલ અક્ષર* અચલ whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        જો (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        જો (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        જો (v3Global.assertDTypesResolved()) {
            જો (nodep->hasDType()) {
            } નહીંતર {
                UASSERT_OBJ(!VN_IS(nodep, ગાંઠ_પદાવલિ), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            જો (અચલ Ast_ગાંઠ_DType* અચલ dnodep = VN_CAST(nodep, ગાંઠ_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    શૂન્ય processExit(Ast_ગાંઠ* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    શૂન્ય processAndIterate(Ast_ગાંઠ* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    શૂન્ય processAndIterateList(Ast_ગાંઠ* nodep) {
        જ્યારે (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    શૂન્ય ધકેલો_Local_Scope() {
        જો (m_cfuncp) m_locals_ઢગલો.emplace_back();
    }
    શૂન્ય કાઢો_Local_Scope() {
        જો (m_cfuncp) m_locals_ઢગલો.કાઢો_back();
    }
    તાર્કિક isInScopeLocal(અચલ AstVar* varp) અચલ {
        માટે (અચલ auto& set : m_locals_ઢગલો) {
            જો (set.ગણતરી(varp)) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    સ્થિર શૂન્ય checkArgRefs(Ast_ગાંઠ_પદાવલિ* nodep, અચલ અક્ષર* descrp, Ast_ગાંઠ_પદાવલિ* argsp) {
        જો (!std::strcmp(descrp, "TODO")) પાછા;  // Skip if not yet checked

        // Check each argument
        અચલ અક્ષર* dp = descrp;
        માટે (Ast_ગાંઠ_પદાવલિ* argp = argsp; argp; argp = VN_AS(argp->nextp(), ગાંઠ_પદાવલિ)) {
            જો (argp->fileline()->erroringOn()) પાછા;  // Intentionally skip all checks
            અચલ Ast_ગાંઠ_પદાવલિ* અચલ lvalp = argp->getVAccessTargetRecurse();
            અચલ VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            જો (dp[0] == '+') --dp;  // Repeats the entry before it
            પસંદગી (dp[0]) {
            સ્થિતિ 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                તોડો;
            સ્થિતિ 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                તોડો;
            સ્થિતિ 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                તોડો;
            મૂળભૂત:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                તોડો;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        અચલ size_t nWriteRefs = m_nWriteRefs;
        અચલ size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        જો (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ગાંઠ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ગાંઠ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    શૂન્ય visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ગાંઠ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ગાંઠ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = સાચું;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        જો (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            જો (અચલ AstVar* અચલ varp = nodep->varp()) {
                જો (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        જો (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    શૂન્ય visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ઢગલો.clear();
        ધકેલો_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        જો (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        માટે (અચલ auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.ગણતરી(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        // Each branch is a separate local variable scope
        ધકેલો_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        જો (Ast_ગાંઠ* અચલ thensp = nodep->thensp()) {
            ધકેલો_Local_Scope();
            processAndIterateList(thensp);
            કાઢો_Local_Scope();
        }
        જો (Ast_ગાંઠ* અચલ elsesp = nodep->elsesp()) {
            ધકેલો_Local_Scope();
            processAndIterateList(elsesp);
            કાઢો_Local_Scope();
        }
        processExit(nodep);
        કાઢો_Local_Scope();
    }
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ધકેલો_Local_Scope();
        processAndIterate(nodep);
        કાઢો_Local_Scope();
    }
    શૂન્ય visit(AstVar* nodep) override {
        processAndIterate(nodep);
        જો (m_cfuncp) {
            m_localVars.ઉમેરો(nodep);
            m_locals_ઢગલો.back().ઉમેરો(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = મૂળભૂત;
};

//######################################################################
// Broken check entry point

શૂન્ય V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    સ્થિર તાર્કિક s_inBroken = ખોટું;
    જો (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } નહીંતર {
        s_inBroken = સાચું;

        અચલ V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        અચલ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_ગાંઠ* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            જો (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        અચલ BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        અચલ અક્ષર* અચલ whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = ખોટું;
    }
}

શૂન્ય V3Broken::allowMidvisitorCheck(તાર્કિક flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

શૂન્ય V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_લીટી* અચલ fl = નવું File_લીટી{File_લીટી::command_લીટી_Filename()};
    Ast_ગાંઠ* અચલ newp = નવું AstBegin{fl, "[EditWrapper]", nullptr, ખોટું};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_વૃક્ષ(), newp);
}
