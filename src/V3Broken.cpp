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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3Broken.h"

#অন্তর্ভাৱ "V3ConstPool.h"

#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

স্থির শ্ৰেণী BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    স্থির constexpr uint8_t MIN_মান = 1;
    স্থির constexpr uint8_t MAX_মান = 127;

    uint8_t m_গুনতি = MIN_মান;

সার্ৱজনিক:
    uint8_t get() ধ্রুৱ VL_MT_SAFE {
        UASSERT(MIN_মান <= m_গুনতি && m_গুনতি <= MAX_মান, "Invalid generation number");
        ঘূৰাই m_গুনতি;
    }

    শূন্য inc() {
        ++m_গুনতি;
        যদি (m_গুনতি > MAX_মান) m_গুনতি = MIN_মান;
    }
} s_brokenCntGlobal;

স্থির তার্কিক s_brokenAllowMidvisitorCheck = মিছা;

//######################################################################
// Table of allocated AstNode pointers

স্থির শ্ৰেণী AllocTable final {
    friend শ্ৰেণী V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<ধ্রুৱ Ast_গ্ৰন্থি*> m_allocated VL_GUARDED_BY(m_mutex);

সার্ৱজনিক:
    // METHODS
    শূন্য addNewed(ধ্রুৱ Ast_গ্ৰন্থি* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ধ্রুৱ V3LockGuard lock{m_mutex};
        যদি (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    শূন্য deleted(ধ্রুৱ Ast_গ্ৰন্থি* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ধ্রুৱ V3LockGuard lock{m_mutex};
        যদি (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

গুপ্ত:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    তার্কিক isAllocated(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ VL_REQUIRES(m_mutex) {
        ঘূৰাই m_allocated.গুনতি(nodep) != 0;
    }
    শূন্য checkForLeaks() VL_REQUIRES(m_mutex) {
        যদি (!v3Global.opt.debugCheck()) ঘূৰাই;

        ধ্রুৱ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        বাবে (ধ্রুৱ তার্কিক withBack : {মিছা, সঁচা}) {
            বাবে (ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                যদি (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ধ্রুৱ তার্কিক hasBack = nodep->backp() != nullptr;
                    যদি (hasBack != withBack) অনুৱর্ত;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_গ্ৰন্থি::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

শূন্য V3Broken::addNewed(ধ্রুৱ Ast_গ্ৰন্থি* nodep) { s_allocTable.addNewed(nodep); }
শূন্য V3Broken::deleted(ধ্রুৱ Ast_গ্ৰন্থি* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

স্থির শ্ৰেণী LinkableTable final {
    // MEMBERS
    std::unordered_set<ধ্রুৱ Ast_গ্ৰন্থি*> m_linkable;  // Set of all nodes allocated but not freed

সার্ৱজনিক:
    // METHODS
    শূন্য clear() { m_linkable.clear(); }
    শূন্য addLinkable(ধ্রুৱ Ast_গ্ৰন্থি* nodep) { m_linkable.emplace(nodep); }
    তার্কিক isLinkable(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ { ঘূৰাই m_linkable.গুনতি(nodep) != 0; }
} s_linkableTable;

তার্কিক V3Broken::isLinkable(ধ্রুৱ Ast_গ্ৰন্থি* nodep) { ঘূৰাই s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

শ্ৰেণী BrokenCheckVisitor final : সার্ৱজনিক VNVisitorConst {
    // Constants for marking we are under/not under a node
    ধ্রুৱ uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ধ্রুৱ uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<ধ্রুৱ AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ধ্রুৱ AstVar*, ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<ধ্রুৱ AstVar*>> m_locals_স্তূপ;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ধ্রুৱ AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    তার্কিক m_inScope = মিছা;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

গুপ্ত:
    // METHODS
    স্থির শূন্য checkWidthMin(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    শূন্য processEnter(Ast_গ্ৰন্থি* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ধ্রুৱ ৱর্ণ* ধ্রুৱ whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        যদি (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        যদি (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        যদি (v3Global.assertDTypesResolved()) {
            যদি (nodep->hasDType()) {
            } অন্যথা {
                UASSERT_OBJ(!VN_IS(nodep, গ্ৰন্থি_ৰাশিমালা), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dnodep = VN_CAST(nodep, গ্ৰন্থি_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    শূন্য processExit(Ast_গ্ৰন্থি* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    শূন্য processAndIterate(Ast_গ্ৰন্থি* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    শূন্য processAndIterateList(Ast_গ্ৰন্থি* nodep) {
        যেতিয়ালৈ (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    শূন্য ঠেলো_Local_Scope() {
        যদি (m_cfuncp) m_locals_স্তূপ.emplace_back();
    }
    শূন্য বেৰ_কৰো_Local_Scope() {
        যদি (m_cfuncp) m_locals_স্তূপ.বেৰ_কৰো_back();
    }
    তার্কিক isInScopeLocal(ধ্রুৱ AstVar* varp) ধ্রুৱ {
        বাবে (ধ্রুৱ auto& set : m_locals_স্তূপ) {
            যদি (set.গুনতি(varp)) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }
    স্থির শূন্য checkArgRefs(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, ধ্রুৱ ৱর্ণ* descrp, Ast_গ্ৰন্থি_ৰাশিমালা* argsp) {
        যদি (!std::strcmp(descrp, "TODO")) ঘূৰাই;  // Skip if not yet checked

        // Check each argument
        ধ্রুৱ ৱর্ণ* dp = descrp;
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* argp = argsp; argp; argp = VN_AS(argp->nextp(), গ্ৰন্থি_ৰাশিমালা)) {
            যদি (argp->fileline()->erroringOn()) ঘূৰাই;  // Intentionally skip all checks
            ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lvalp = argp->getVAccessTargetRecurse();
            ধ্রুৱ VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            যদি (dp[0] == '+') --dp;  // Repeats the entry before it
            ৱিকল্প (dp[0]) {
            পক্ষ 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                ভঙ্গ;
            পক্ষ 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                ভঙ্গ;
            পক্ষ 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                ভঙ্গ;
            স্ৱভাৱ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                ভঙ্গ;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ধ্রুৱ size_t nWriteRefs = m_nWriteRefs;
        ধ্রুৱ size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        যদি (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), গ্ৰন্থি_Var_Ref)
                      && !VN_AS(nodep->lhsp(), গ্ৰন্থি_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    শূন্য visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), গ্ৰন্থি_Var_Ref)
                      && !VN_AS(nodep->lhsp(), গ্ৰন্থি_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = সঁচা;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        যদি (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = nodep->varp()) {
                যদি (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        যদি (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    শূন্য visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_স্তূপ.clear();
        ঠেলো_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        যদি (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        বাবে (ধ্রুৱ auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.গুনতি(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        // Each branch is a separate local variable scope
        ঠেলো_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ thensp = nodep->thensp()) {
            ঠেলো_Local_Scope();
            processAndIterateList(thensp);
            বেৰ_কৰো_Local_Scope();
        }
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ elsesp = nodep->elsesp()) {
            ঠেলো_Local_Scope();
            processAndIterateList(elsesp);
            বেৰ_কৰো_Local_Scope();
        }
        processExit(nodep);
        বেৰ_কৰো_Local_Scope();
    }
    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ঠেলো_Local_Scope();
        processAndIterate(nodep);
        বেৰ_কৰো_Local_Scope();
    }
    শূন্য visit(AstVar* nodep) override {
        processAndIterate(nodep);
        যদি (m_cfuncp) {
            m_localVars.ঢোকাও(nodep);
            m_locals_স্তূপ.back().ঢোকাও(nodep);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Broken check entry point

শূন্য V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    স্থির তার্কিক s_inBroken = মিছা;
    যদি (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } অন্যথা {
        s_inBroken = সঁচা;

        ধ্রুৱ V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        ধ্রুৱ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_গ্ৰন্থি* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            যদি (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ধ্রুৱ BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ধ্রুৱ ৱর্ণ* ধ্রুৱ whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = মিছা;
    }
}

শূন্য V3Broken::allowMidvisitorCheck(তার্কিক flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

শূন্য V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_সাৰি* ধ্রুৱ fl = নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()};
    Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstBegin{fl, "[EditWrapper]", nullptr, মিছা};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_বৃক্ষ(), newp);
}
