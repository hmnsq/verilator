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
    uint8_t get() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        UASSERT(MIN_ಮೌಲ್ಯ <= m_ಎಣಿಕೆ && m_ಎಣಿಕೆ <= MAX_ಮೌಲ್ಯ, "Invalid generation number");
        ಹಿಂತಿರುಗಿ m_ಎಣಿಕೆ;
    }

    ಶೂನ್ಯ inc() {
        ++m_ಎಣಿಕೆ;
        ಆದರೆ (m_ಎಣಿಕೆ > MAX_ಮೌಲ್ಯ) m_ಎಣಿಕೆ = MIN_ಮೌಲ್ಯ;
    }
} s_brokenCntGlobal;

ಸ್ಥಿರ ತಾರ್ಕಿಕ s_brokenAllowMidvisitorCheck = ಸುಳ್ಳು;

//######################################################################
// Table of allocated AstNode pointers

ಸ್ಥಿರ ವರ್ಗ Alloc_ಕೋಷ್ಟಕ final {
    ಮಿತ್ರ ವರ್ಗ V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*> m_allocated VL_GUARDED_BY(m_mutex);

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_Newed(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಆದರೆ (VL_UNCOVERABLE(!m_allocated.emplace(nodep).ಎರಡನೇ)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    ಶೂನ್ಯ deleted(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಆದರೆ (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

ಖಾಸಗಿ:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    ತಾರ್ಕಿಕ isAllocated(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) {
        ಹಿಂತಿರುಗಿ m_allocated.ಎಣಿಕೆ_2(nodep) != 0;
    }
    ಶೂನ್ಯ checkForLeaks() VL_REQUIRES(m_mutex) {
        ಆದರೆ (!v3Global.opt.debugCheck()) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ with_ಹಿಂಭಾಗ : {ಸುಳ್ಳು, ನಿಜ}) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                ಆದರೆ (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ has_ಹಿಂಭಾಗ = nodep->backp() != ಶೂನ್ಯಸೂಚಕ;
                    ಆದರೆ (has_ಹಿಂಭಾಗ != with_ಹಿಂಭಾಗ) ಮುಂದುವರಿ;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //ಸತತ
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_ಹಿಂಭಾಗ ? " with back pointer: " : ": ");
                    nodep->Ast_ಗಂಟು::dump(std::cerr);
                    std::cerr << ಸಾಲು_ಅಂತ್ಯ;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_ಕೋಷ್ಟಕ;

ಶೂನ್ಯ V3Broken::ಕೂಡಿಸು_Newed(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { s_alloc_ಕೋಷ್ಟಕ.ಕೂಡಿಸು_Newed(nodep); }
ಶೂನ್ಯ V3Broken::deleted(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { s_alloc_ಕೋಷ್ಟಕ.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ಸ್ಥಿರ ವರ್ಗ Linkable_ಕೋಷ್ಟಕ final {
    // MEMBERS
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*> m_linkable;  // Set of all nodes allocated but not freed

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಶೂನ್ಯ clear() { m_linkable.clear(); }
    ಶೂನ್ಯ ಕೂಡಿಸು_Linkable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { m_linkable.emplace(nodep); }
    ತಾರ್ಕಿಕ isLinkable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_linkable.ಎಣಿಕೆ_2(nodep) != 0; }
} s_linkable_ಕೋಷ್ಟಕ;

ತಾರ್ಕಿಕ V3Broken::isLinkable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ s_linkable_ಕೋಷ್ಟಕ.isLinkable(nodep); }

//######################################################################
// Check every node in tree

ವರ್ಗ BrokenCheckVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Constants for marking we are under/not under a node
    ಸ್ಥಿರಾಂಕ uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ಸ್ಥಿರಾಂಕ uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ಸ್ಥಿರಾಂಕ AstVar*, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::ಸದಿಶ<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>> m_locals_ರಾಶಿ;
    // Number of write references encountered
    ಗಾತ್ರ_t m_nWriteRefs = 0;
    // Number of function calls encountered
    ಗಾತ್ರ_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ಸ್ಥಿರಾಂಕ AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current CFunc, if any
    ತಾರ್ಕಿಕ m_ಒಳ_Scope = ಸುಳ್ಳು;  // Under AstScope
    std::ಗಣ<std::string> m_c_Func_ಹೆಸರುಗಳು;  // CFunc by name in current class/module

ಖಾಸಗಿ:
    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ check_ಅಗಲ_Min(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(nodep->ಅಗಲ() == nodep->ಅಗಲ_Min()
                        || v3Global.ಅಗಲ_Min_Usage() != VWidthMinUsage::MATCHES_ಅಗಲ,
                    nodep, "Width != WidthMin");
    }

    ಶೂನ್ಯ processEnter(Ast_ಗಂಟು* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        ಆದರೆ (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        ಆದರೆ (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_ಗಮ್ಯ_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_ಗಮ್ಯ_Hex(nodep->dtypep()));
        }
        ಆದರೆ (v3Global.assertDTypesResolved()) {
            ಆದರೆ (nodep->hasDType()) {
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(!VN_IS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_ಮಗು_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dnodep = VN_CAST(nodep, ಗಂಟು_DType))
                check_ಅಗಲ_Min(dnodep);
        }
        check_ಅಗಲ_Min(nodep);
    }
    ಶೂನ್ಯ processExit(Ast_ಗಂಟು* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    ಶೂನ್ಯ processAndIterate(Ast_ಗಂಟು* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    ಶೂನ್ಯ processAndIterateList(Ast_ಗಂಟು* nodep) {
        ಆಗ (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    ಶೂನ್ಯ ತಳ್ಳು_Local_Scope() {
        ಆದರೆ (m_cfuncp) m_locals_ರಾಶಿ.emplace_ಹಿಂಭಾಗ();
    }
    ಶೂನ್ಯ ತೆಗೆ_Local_Scope() {
        ಆದರೆ (m_cfuncp) m_locals_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
    }
    ತಾರ್ಕಿಕ is_ಒಳ_Scope_Local(ಸ್ಥಿರಾಂಕ AstVar* varp) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಗಣ : m_locals_ರಾಶಿ) {
            ಆದರೆ (ಗಣ.ಎಣಿಕೆ_2(varp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ checkArgRefs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descrp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp) {
        ಆದರೆ (!std::strcmp(descrp, "TODO")) ಹಿಂತಿರುಗಿ;  // Skip if not yet checked

        // Check each argument
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* dp = descrp;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp = argsp; argp; argp = VN_AS(argp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            ಆದರೆ (argp->fileline()->erroringOn()) ಹಿಂತಿರುಗಿ;  // Intentionally skip all checks
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lvalp = argp->get_VAccess_ಗುರಿ_Recurse();
            ಸ್ಥಿರಾಂಕ VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            ಆದರೆ (dp[0] == '+') --dp;  // Repeats the entry before it
            ಆಯ್ಕೆ (dp[0]) {
            ಸ್ಥಿತಿ 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                ನಿಲ್ಲಿಸು;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nWriteRefs = m_nWriteRefs;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        ಆದರೆ (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ಗಂಟು_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ಗಂಟು_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_ಒಳ_Scope);
        m_ಒಳ_Scope = ನಿಜ;
        VL_RESTORER_CLEAR(m_c_Func_ಹೆಸರುಗಳು);
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_ಹೆಸರುಗಳು);
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_ಹೆಸರುಗಳು);
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_ಒಳ_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        ಆದರೆ (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp()) {
                ಆದರೆ (!is_ಒಳ_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        ಆದರೆ (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->ವಿಧಾನ().ವಾದಗಳು(), nodep->pinsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().ವಾದಗಳು(), nodep->pinsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ರಾಶಿ.clear();
        ತಳ್ಳು_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_ವಾದಗಳು = nodep->name();
        ಆದರೆ (!nodep->argTypes().ಖಾಲಿಗಳು()) name_ವಾದಗಳು += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_ಹೆಸರುಗಳು.emplace(name_ವಾದಗಳು).ಎರಡನೇ, nodep,
                    "Duplicate cfunc name: '" << name_ವಾದಗಳು << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.ಎಣಿಕೆ_2(pair.ಮೊದಲ) == 0, pair.ಎರಡನೇ,
                        "Local variable not in scope where referenced: " << pair.ಮೊದಲ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        // Each branch is a separate local variable scope
        ತಳ್ಳು_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thensp = nodep->thensp()) {
            ತಳ್ಳು_Local_Scope();
            processAndIterateList(thensp);
            ತೆಗೆ_Local_Scope();
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elsesp = nodep->elsesp()) {
            ತಳ್ಳು_Local_Scope();
            processAndIterateList(elsesp);
            ತೆಗೆ_Local_Scope();
        }
        processExit(nodep);
        ತೆಗೆ_Local_Scope();
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ತಳ್ಳು_Local_Scope();
        processAndIterate(nodep);
        ತೆಗೆ_Local_Scope();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        processAndIterate(nodep);
        ಆದರೆ (m_cfuncp) {
            m_localVars.ಸೇರಿಸು(nodep);
            m_locals_ರಾಶಿ.ಹಿಂಭಾಗ().ಸೇರಿಸು(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Broken check entry point

ಶೂನ್ಯ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_ಒಳ_Broken = ಸುಳ್ಳು;
    ಆದರೆ (VL_UNCOVERABLE(s_ಒಳ_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } ಇಲ್ಲದಿದ್ದರೆ {
        s_ಒಳ_Broken = ನಿಜ;

        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_alloc_ಕೋಷ್ಟಕ.m_mutex};

        // Mark every node in the tree
        ಸ್ಥಿರಾಂಕ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_ಗಂಟು* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_ಕೋಷ್ಟಕ.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#ಆದರೆ_ಅಂತ್ಯ
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            ಆದರೆ (nodep->maybe_Pointed_ಗಮ್ಯ()) s_linkable_ಕೋಷ್ಟಕ.ಕೂಡಿಸು_Linkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ಸ್ಥಿರಾಂಕ BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_alloc_ಕೋಷ್ಟಕ.checkForLeaks();
        s_linkable_ಕೋಷ್ಟಕ.clear();
        s_brokenCntGlobal.inc();
        s_ಒಳ_Broken = ಸುಳ್ಳು;
    }
}

ಶೂನ್ಯ V3Broken::allowMidvisitorCheck(ತಾರ್ಕಿಕ flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

ಶೂನ್ಯ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()};
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstBegin{fl, "[EditWrapper]", ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VL_LEAK_CHECKS
    ಕೂಡಿಸು_Newed(newp);
    deleted(newp);
#ಆದರೆ_ಅಂತ್ಯ
    VL_DO_DANGLING(newp->delete_ವೃಕ್ಷ(), newp);
}
