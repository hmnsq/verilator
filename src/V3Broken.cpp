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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Broken.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"

#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

ସ୍ଥିର ଶ୍ରେଣୀ BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    ସ୍ଥିର constexpr uint8_t MIN_ମୂଲ୍ୟ = 1;
    ସ୍ଥିର constexpr uint8_t MAX_ମୂଲ୍ୟ = 127;

    uint8_t m_ଗଣନା = MIN_ମୂଲ୍ୟ;

ସର୍ବସାଧାରଣ:
    uint8_t get() ଅଚଳ VL_MT_SAFE {
        UASSERT(MIN_ମୂଲ୍ୟ <= m_ଗଣନା && m_ଗଣନା <= MAX_ମୂଲ୍ୟ, "Invalid generation number");
        ଫେରନ୍ତୁ m_ଗଣନା;
    }

    ଶୂନ୍ୟ inc() {
        ++m_ଗଣନା;
        ଯଦି (m_ଗଣନା > MAX_ମୂଲ୍ୟ) m_ଗଣନା = MIN_ମୂଲ୍ୟ;
    }
} s_brokenCntGlobal;

ସ୍ଥିର ତାର୍କିକ s_brokenAllowMidvisitorCheck = ମିଥ୍ୟା;

//######################################################################
// Table of allocated AstNode pointers

ସ୍ଥିର ଶ୍ରେଣୀ AllocTable final {
    friend ଶ୍ରେଣୀ V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<ଅଚଳ Ast_ଗଣ୍ଠି*> m_allocated VL_GUARDED_BY(m_mutex);

ସର୍ବସାଧାରଣ:
    // METHODS
    ଶୂନ୍ୟ addNewed(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଯଦି (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    ଶୂନ୍ୟ deleted(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଯଦି (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

ବ୍ୟକ୍ତିଗତ:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    ତାର୍କିକ isAllocated(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ VL_REQUIRES(m_mutex) {
        ଫେରନ୍ତୁ m_allocated.ଗଣନା_2(nodep) != 0;
    }
    ଶୂନ୍ୟ checkForLeaks() VL_REQUIRES(m_mutex) {
        ଯଦି (!v3Global.opt.debugCheck()) ଫେରନ୍ତୁ;

        ଅଚଳ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        ପାଇଁ (ଅଚଳ ତାର୍କିକ withBack : {ମିଥ୍ୟା, ସତ୍ୟ}) {
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                ଯଦି (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ଅଚଳ ତାର୍କିକ hasBack = nodep->backp() != nullptr;
                    ଯଦି (hasBack != withBack) ଜାରି;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_ଗଣ୍ଠି::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

ଶୂନ୍ୟ V3Broken::addNewed(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { s_allocTable.addNewed(nodep); }
ଶୂନ୍ୟ V3Broken::deleted(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ସ୍ଥିର ଶ୍ରେଣୀ LinkableTable final {
    // MEMBERS
    std::unordered_set<ଅଚଳ Ast_ଗଣ୍ଠି*> m_linkable;  // Set of all nodes allocated but not freed

ସର୍ବସାଧାରଣ:
    // METHODS
    ଶୂନ୍ୟ clear() { m_linkable.clear(); }
    ଶୂନ୍ୟ addLinkable(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { m_linkable.emplace(nodep); }
    ତାର୍କିକ isLinkable(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ { ଫେରନ୍ତୁ m_linkable.ଗଣନା_2(nodep) != 0; }
} s_linkableTable;

ତାର୍କିକ V3Broken::isLinkable(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

ଶ୍ରେଣୀ BrokenCheckVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // Constants for marking we are under/not under a node
    ଅଚଳ uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ଅଚଳ uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<ଅଚଳ AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ଅଚଳ AstVar*, ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<ଅଚଳ AstVar*>> m_locals_ଗଦା;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ଅଚଳ AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    ତାର୍କିକ m_inScope = ମିଥ୍ୟା;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

ବ୍ୟକ୍ତିଗତ:
    // METHODS
    ସ୍ଥିର ଶୂନ୍ୟ checkWidthMin(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    ଶୂନ୍ୟ processEnter(Ast_ଗଣ୍ଠି* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        ଯଦି (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        ଯଦି (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        ଯଦି (v3Global.assertDTypesResolved()) {
            ଯଦି (nodep->hasDType()) {
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(!VN_IS(nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dnodep = VN_CAST(nodep, ଗଣ୍ଠି_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    ଶୂନ୍ୟ processExit(Ast_ଗଣ୍ଠି* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    ଶୂନ୍ୟ processAndIterate(Ast_ଗଣ୍ଠି* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    ଶୂନ୍ୟ processAndIterateList(Ast_ଗଣ୍ଠି* nodep) {
        ଯେତେବେଳେ (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    ଶୂନ୍ୟ ଠେଲ_Local_Scope() {
        ଯଦି (m_cfuncp) m_locals_ଗଦା.emplace_back();
    }
    ଶୂନ୍ୟ କାଢ଼_Local_Scope() {
        ଯଦି (m_cfuncp) m_locals_ଗଦା.କାଢ଼_back();
    }
    ତାର୍କିକ isInScopeLocal(ଅଚଳ AstVar* varp) ଅଚଳ {
        ପାଇଁ (ଅଚଳ auto& set : m_locals_ଗଦା) {
            ଯଦି (set.ଗଣନା_2(varp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ସ୍ଥିର ଶୂନ୍ୟ checkArgRefs(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ଅଚଳ ଅକ୍ଷର* descrp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp) {
        ଯଦି (!std::strcmp(descrp, "TODO")) ଫେରନ୍ତୁ;  // Skip if not yet checked

        // Check each argument
        ଅଚଳ ଅକ୍ଷର* dp = descrp;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argp = argsp; argp; argp = VN_AS(argp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            ଯଦି (argp->fileline()->erroringOn()) ଫେରନ୍ତୁ;  // Intentionally skip all checks
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lvalp = argp->getVAccessTargetRecurse();
            ଅଚଳ VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            ଯଦି (dp[0] == '+') --dp;  // Repeats the entry before it
            ବାଛ (dp[0]) {
            ସ୍ଥିତି 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                ଭାଙ୍ଗ;
            ସ୍ଥିତି 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                ଭାଙ୍ଗ;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ଅଚଳ size_t nWriteRefs = m_nWriteRefs;
        ଅଚଳ size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        ଯଦି (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = ସତ୍ୟ;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        ଯଦି (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = nodep->varp()) {
                ଯଦି (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        ଯଦି (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    ଶୂନ୍ୟ visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ଗଦା.clear();
        ଠେଲ_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        ଯଦି (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        ପାଇଁ (ଅଚଳ auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.ଗଣନା_2(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        // Each branch is a separate local variable scope
        ଠେଲ_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ thensp = nodep->thensp()) {
            ଠେଲ_Local_Scope();
            processAndIterateList(thensp);
            କାଢ଼_Local_Scope();
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ elsesp = nodep->elsesp()) {
            ଠେଲ_Local_Scope();
            processAndIterateList(elsesp);
            କାଢ଼_Local_Scope();
        }
        processExit(nodep);
        କାଢ଼_Local_Scope();
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ଠେଲ_Local_Scope();
        processAndIterate(nodep);
        କାଢ଼_Local_Scope();
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        processAndIterate(nodep);
        ଯଦି (m_cfuncp) {
            m_localVars.ଭର୍ତ୍ତି(nodep);
            m_locals_ଗଦା.back().ଭର୍ତ୍ତି(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Broken check entry point

ଶୂନ୍ୟ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ସ୍ଥିର ତାର୍କିକ s_inBroken = ମିଥ୍ୟା;
    ଯଦି (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } ଅନ୍ୟଥା {
        s_inBroken = ସତ୍ୟ;

        ଅଚଳ V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        ଅଚଳ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_ଗଣ୍ଠି* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            ଯଦି (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ଅଚଳ BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = ମିଥ୍ୟା;
    }
}

ଶୂନ୍ୟ V3Broken::allowMidvisitorCheck(ତାର୍କିକ flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

ଶୂନ୍ୟ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_ଧାଡ଼ି* ଅଚଳ fl = ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()};
    Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstBegin{fl, "[EditWrapper]", nullptr, ମିଥ୍ୟା};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_ବୃକ୍ଷ(), newp);
}
