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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Broken.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"

#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

ಸ್ಥಿರ ವರ್ಗ BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    ಸ್ಥಿರ constexpr uint8_t MIN_ಮೌಲ್ಯ = 1;
    ಸ್ಥಿರ constexpr uint8_t MAX_ಮೌಲ್ಯ = 127;

    uint8_t m_ಎಣಿಕೆ = MIN_ಮೌಲ್ಯ;

ಸಾರ್ವಜನಿಕ:
    uint8_t get() ಧ್ರುವ VL_MT_SAFE {
        UASSERT(MIN_ಮೌಲ್ಯ <= m_ಎಣಿಕೆ && m_ಎಣಿಕೆ <= MAX_ಮೌಲ್ಯ, "Invalid generation number");
        ಪ್ರತ್ಯಾಗಮ m_ಎಣಿಕೆ;
    }

    ಶೂನ್ಯ inc() {
        ++m_ಎಣಿಕೆ;
        ಯದಿ (m_ಎಣಿಕೆ > MAX_ಮೌಲ್ಯ) m_ಎಣಿಕೆ = MIN_ಮೌಲ್ಯ;
    }
} s_brokenCntGlobal;

ಸ್ಥಿರ ತಾರ್ಕಿಕ s_brokenAllowMidvisitorCheck = ಅಸತ್ಯ;

//######################################################################
// Table of allocated AstNode pointers

ಸ್ಥಿರ ವರ್ಗ AllocTable final {
    friend ವರ್ಗ V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<ಧ್ರುವ Ast_ಗಂಟು*> m_allocated VL_GUARDED_BY(m_mutex);

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಶೂನ್ಯ addNewed(ಧ್ರುವ Ast_ಗಂಟು* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಯದಿ (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    ಶೂನ್ಯ deleted(ಧ್ರುವ Ast_ಗಂಟು* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಯದಿ (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

ಗುಪ್ತ:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    ತಾರ್ಕಿಕ isAllocated(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ VL_REQUIRES(m_mutex) {
        ಪ್ರತ್ಯಾಗಮ m_allocated.ಎಣಿಕೆ(nodep) != 0;
    }
    ಶೂನ್ಯ checkForLeaks() VL_REQUIRES(m_mutex) {
        ಯದಿ (!v3Global.opt.debugCheck()) ಪ್ರತ್ಯಾಗಮ;

        ಧ್ರುವ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        ಪ್ರತಿ (ಧ್ರುವ ತಾರ್ಕಿಕ withBack : {ಅಸತ್ಯ, ಸತ್ಯ}) {
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                ಯದಿ (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ಧ್ರುವ ತಾರ್ಕಿಕ hasBack = nodep->backp() != nullptr;
                    ಯದಿ (hasBack != withBack) ಅನುವರ್ತ;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_ಗಂಟು::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

ಶೂನ್ಯ V3Broken::addNewed(ಧ್ರುವ Ast_ಗಂಟು* nodep) { s_allocTable.addNewed(nodep); }
ಶೂನ್ಯ V3Broken::deleted(ಧ್ರುವ Ast_ಗಂಟು* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ಸ್ಥಿರ ವರ್ಗ LinkableTable final {
    // MEMBERS
    std::unordered_set<ಧ್ರುವ Ast_ಗಂಟು*> m_linkable;  // Set of all nodes allocated but not freed

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಶೂನ್ಯ clear() { m_linkable.clear(); }
    ಶೂನ್ಯ addLinkable(ಧ್ರುವ Ast_ಗಂಟು* nodep) { m_linkable.emplace(nodep); }
    ತಾರ್ಕಿಕ isLinkable(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_linkable.ಎಣಿಕೆ(nodep) != 0; }
} s_linkableTable;

ತಾರ್ಕಿಕ V3Broken::isLinkable(ಧ್ರುವ Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

ವರ್ಗ BrokenCheckVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Constants for marking we are under/not under a node
    ಧ್ರುವ uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ಧ್ರುವ uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<ಧ್ರುವ AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ಧ್ರುವ AstVar*, ಧ್ರುವ Ast_ಗಂಟು_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<ಧ್ರುವ AstVar*>> m_locals_ರಾಶಿ;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ಧ್ರುವ AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    ತಾರ್ಕಿಕ m_inScope = ಅಸತ್ಯ;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

ಗುಪ್ತ:
    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ checkWidthMin(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    ಶೂನ್ಯ processEnter(Ast_ಗಂಟು* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        ಯದಿ (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        ಯದಿ (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        ಯದಿ (v3Global.assertDTypesResolved()) {
            ಯದಿ (nodep->hasDType()) {
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(!VN_IS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dnodep = VN_CAST(nodep, ಗಂಟು_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    ಶೂನ್ಯ processExit(Ast_ಗಂಟು* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    ಶೂನ್ಯ processAndIterate(Ast_ಗಂಟು* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    ಶೂನ್ಯ processAndIterateList(Ast_ಗಂಟು* nodep) {
        ಯಾವತ್ (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    ಶೂನ್ಯ ತಳ್ಳು_Local_Scope() {
        ಯದಿ (m_cfuncp) m_locals_ರಾಶಿ.emplace_back();
    }
    ಶೂನ್ಯ ತೆಗೆ_Local_Scope() {
        ಯದಿ (m_cfuncp) m_locals_ರಾಶಿ.ತೆಗೆ_back();
    }
    ತಾರ್ಕಿಕ isInScopeLocal(ಧ್ರುವ AstVar* varp) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ auto& set : m_locals_ರಾಶಿ) {
            ಯದಿ (set.ಎಣಿಕೆ(varp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ checkArgRefs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಧ್ರುವ ವರ್ಣ* descrp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp) {
        ಯದಿ (!std::strcmp(descrp, "TODO")) ಪ್ರತ್ಯಾಗಮ;  // Skip if not yet checked

        // Check each argument
        ಧ್ರುವ ವರ್ಣ* dp = descrp;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp = argsp; argp; argp = VN_AS(argp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            ಯದಿ (argp->fileline()->erroringOn()) ಪ್ರತ್ಯಾಗಮ;  // Intentionally skip all checks
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lvalp = argp->getVAccessTargetRecurse();
            ಧ್ರುವ VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            ಯದಿ (dp[0] == '+') --dp;  // Repeats the entry before it
            ವಿಕಲ್ಪ (dp[0]) {
            ಪಕ್ಷ 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                ಭಙ್ಗ;
            ಪಕ್ಷ 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                ಭಙ್ಗ;
            ಪಕ್ಷ 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                ಭಙ್ಗ;
            ಸ್ವಭಾವ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                ಭಙ್ಗ;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ಧ್ರುವ size_t nWriteRefs = m_nWriteRefs;
        ಧ್ರುವ size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        ಯದಿ (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ಗಂಟು_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    ಶೂನ್ಯ visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ಗಂಟು_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = ಸತ್ಯ;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        ಯದಿ (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varp()) {
                ಯದಿ (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        ಯದಿ (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    ಶೂನ್ಯ visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ರಾಶಿ.clear();
        ತಳ್ಳು_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        ಯದಿ (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.ಎಣಿಕೆ(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        // Each branch is a separate local variable scope
        ತಳ್ಳು_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ thensp = nodep->thensp()) {
            ತಳ್ಳು_Local_Scope();
            processAndIterateList(thensp);
            ತೆಗೆ_Local_Scope();
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ elsesp = nodep->elsesp()) {
            ತಳ್ಳು_Local_Scope();
            processAndIterateList(elsesp);
            ತೆಗೆ_Local_Scope();
        }
        processExit(nodep);
        ತೆಗೆ_Local_Scope();
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ತಳ್ಳು_Local_Scope();
        processAndIterate(nodep);
        ತೆಗೆ_Local_Scope();
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        processAndIterate(nodep);
        ಯದಿ (m_cfuncp) {
            m_localVars.ಸೇರಿಸು(nodep);
            m_locals_ರಾಶಿ.back().ಸೇರಿಸು(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Broken check entry point

ಶೂನ್ಯ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_inBroken = ಅಸತ್ಯ;
    ಯದಿ (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } ಅನ್ಯಥಾ {
        s_inBroken = ಸತ್ಯ;

        ಧ್ರುವ V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        ಧ್ರುವ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_ಗಂಟು* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            ಯದಿ (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ಧ್ರುವ BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = ಅಸತ್ಯ;
    }
}

ಶೂನ್ಯ V3Broken::allowMidvisitorCheck(ತಾರ್ಕಿಕ flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

ಶೂನ್ಯ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_ಸಾಲು* ಧ್ರುವ fl = ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()};
    Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstBegin{fl, "[EditWrapper]", nullptr, ಅಸತ್ಯ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_ವೃಕ್ಷ(), newp);
}
