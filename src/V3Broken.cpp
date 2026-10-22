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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Broken.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

ਸਥਿਰ ਵਰਗ BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    ਸਥਿਰ constexpr uint8_t MIN_ਮੁੱਲ = 1;
    ਸਥਿਰ constexpr uint8_t MAX_ਮੁੱਲ = 127;

    uint8_t m_ਗਿਣਤੀ = MIN_ਮੁੱਲ;

ਜਨਤਕ:
    uint8_t get() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        UASSERT(MIN_ਮੁੱਲ <= m_ਗਿਣਤੀ && m_ਗਿਣਤੀ <= MAX_ਮੁੱਲ, "Invalid generation number");
        ਵਾਪਸ m_ਗਿਣਤੀ;
    }

    ਖਾਲੀ inc() {
        ++m_ਗਿਣਤੀ;
        ਜੇ (m_ਗਿਣਤੀ > MAX_ਮੁੱਲ) m_ਗਿਣਤੀ = MIN_ਮੁੱਲ;
    }
} s_brokenCntGlobal;

ਸਥਿਰ ਤਾਰਕਿਕ s_brokenAllowMidvisitorCheck = ਝੂਠ;

//######################################################################
// Table of allocated AstNode pointers

ਸਥਿਰ ਵਰਗ Alloc_ਸਾਰਣੀ final {
    ਮਿੱਤਰ ਵਰਗ V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ*> m_allocated VL_GUARDED_BY(m_mutex);

ਜਨਤਕ:
    // METHODS
    ਖਾਲੀ ਜੋੜੋ_Newed(ਅਚਲ Ast_ਗੰਢ* nodep) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਜੇ (VL_UNCOVERABLE(!m_allocated.emplace(nodep).ਦੂਜਾ)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    ਖਾਲੀ deleted(ਅਚਲ Ast_ਗੰਢ* nodep) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਜੇ (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

ਨਿਜੀ:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    ਤਾਰਕਿਕ isAllocated(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ VL_REQUIRES(m_mutex) {
        ਵਾਪਸ m_allocated.ਗਿਣਤੀ_2(nodep) != 0;
    }
    ਖਾਲੀ checkForLeaks() VL_REQUIRES(m_mutex) {
        ਜੇ (!v3Global.opt.debugCheck()) ਵਾਪਸ;

        ਅਚਲ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        ਲਈ (ਅਚਲ ਤਾਰਕਿਕ with_ਪਿੱਠ : {ਝੂਠ, ਸੱਚ}) {
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                ਜੇ (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ਅਚਲ ਤਾਰਕਿਕ has_ਪਿੱਠ = nodep->backp() != ਖਾਲੀ_ਸੂਚਕ;
                    ਜੇ (has_ਪਿੱਠ != with_ਪਿੱਠ) ਜਾਰੀ;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //ਲੜੀਵਾਰ
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_ਪਿੱਠ ? " with back pointer: " : ": ");
                    nodep->Ast_ਗੰਢ::dump(std::cerr);
                    std::cerr << ਸਤਰ_ਅੰਤ;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_ਸਾਰਣੀ;

ਖਾਲੀ V3Broken::ਜੋੜੋ_Newed(ਅਚਲ Ast_ਗੰਢ* nodep) { s_alloc_ਸਾਰਣੀ.ਜੋੜੋ_Newed(nodep); }
ਖਾਲੀ V3Broken::deleted(ਅਚਲ Ast_ਗੰਢ* nodep) { s_alloc_ਸਾਰਣੀ.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ਸਥਿਰ ਵਰਗ Linkable_ਸਾਰਣੀ final {
    // MEMBERS
    std::unordered_ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ*> m_linkable;  // Set of all nodes allocated but not freed

ਜਨਤਕ:
    // METHODS
    ਖਾਲੀ clear() { m_linkable.clear(); }
    ਖਾਲੀ ਜੋੜੋ_Linkable(ਅਚਲ Ast_ਗੰਢ* nodep) { m_linkable.emplace(nodep); }
    ਤਾਰਕਿਕ isLinkable(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ { ਵਾਪਸ m_linkable.ਗਿਣਤੀ_2(nodep) != 0; }
} s_linkable_ਸਾਰਣੀ;

ਤਾਰਕਿਕ V3Broken::isLinkable(ਅਚਲ Ast_ਗੰਢ* nodep) { ਵਾਪਸ s_linkable_ਸਾਰਣੀ.isLinkable(nodep); }

//######################################################################
// Check every node in tree

ਵਰਗ BrokenCheckVisitor final : ਜਨਤਕ VNVisitorConst {
    // Constants for marking we are under/not under a node
    ਅਚਲ uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ਅਚਲ uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::ਸੈੱਟ<ਅਚਲ AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ਅਚਲ AstVar*, ਅਚਲ Ast_ਗੰਢ_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::ਸਦਿਸ਼<std::unordered_ਸੈੱਟ<ਅਚਲ AstVar*>> m_locals_ਢੇਰ;
    // Number of write references encountered
    ਮਾਪ_t m_nWriteRefs = 0;
    // Number of function calls encountered
    ਮਾਪ_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ਅਚਲ AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current CFunc, if any
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Scope = ਝੂਠ;  // Under AstScope
    std::ਸੈੱਟ<std::string> m_c_Func_ਨਾਂ;  // CFunc by name in current class/module

ਨਿਜੀ:
    // METHODS
    ਸਥਿਰ ਖਾਲੀ check_ਚੌੜਾਈ_Min(ਅਚਲ Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(nodep->ਚੌੜਾਈ() == nodep->ਚੌੜਾਈ_Min()
                        || v3Global.ਚੌੜਾਈ_Min_Usage() != VWidthMinUsage::MATCHES_ਚੌੜਾਈ,
                    nodep, "Width != WidthMin");
    }

    ਖਾਲੀ processEnter(Ast_ਗੰਢ* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ਅਚਲ ਅੱਖਰ* ਅਚਲ whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        ਜੇ (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        ਜੇ (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_ਮੰਜ਼ਿਲ_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_ਮੰਜ਼ਿਲ_Hex(nodep->dtypep()));
        }
        ਜੇ (v3Global.assertDTypesResolved()) {
            ਜੇ (nodep->hasDType()) {
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(!VN_IS(nodep, ਗੰਢ_ਵਿਅੰਜਕ), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_ਬੱਚਾ_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dnodep = VN_CAST(nodep, ਗੰਢ_DType))
                check_ਚੌੜਾਈ_Min(dnodep);
        }
        check_ਚੌੜਾਈ_Min(nodep);
    }
    ਖਾਲੀ processExit(Ast_ਗੰਢ* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    ਖਾਲੀ processAndIterate(Ast_ਗੰਢ* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    ਖਾਲੀ processAndIterateList(Ast_ਗੰਢ* nodep) {
        ਜਦੋਂ (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    ਖਾਲੀ ਧੱਕੋ_Local_Scope() {
        ਜੇ (m_cfuncp) m_locals_ਢੇਰ.emplace_ਪਿੱਠ();
    }
    ਖਾਲੀ ਕੱਢੋ_Local_Scope() {
        ਜੇ (m_cfuncp) m_locals_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
    }
    ਤਾਰਕਿਕ is_ਵਿੱਚ_Scope_Local(ਅਚਲ AstVar* varp) ਅਚਲ {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਸੈੱਟ : m_locals_ਢੇਰ) {
            ਜੇ (ਸੈੱਟ.ਗਿਣਤੀ_2(varp)) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਸਥਿਰ ਖਾਲੀ checkArgRefs(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਅਚਲ ਅੱਖਰ* descrp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp) {
        ਜੇ (!std::strcmp(descrp, "TODO")) ਵਾਪਸ;  // Skip if not yet checked

        // Check each argument
        ਅਚਲ ਅੱਖਰ* dp = descrp;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* argp = argsp; argp; argp = VN_AS(argp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            ਜੇ (argp->fileline()->erroringOn()) ਵਾਪਸ;  // Intentionally skip all checks
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lvalp = argp->get_VAccess_ਨਿਸ਼ਾਨਾ_Recurse();
            ਅਚਲ VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            ਜੇ (dp[0] == '+') --dp;  // Repeats the entry before it
            ਚੋਣ (dp[0]) {
            ਹਾਲਤ 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                ਰੋਕੋ;
            ਹਾਲਤ 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                ਰੋਕੋ;
            ਹਾਲਤ 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                ਰੋਕੋ;
            ਮੂਲ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                ਰੋਕੋ;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ਅਚਲ ਮਾਪ_t nWriteRefs = m_nWriteRefs;
        ਅਚਲ ਮਾਪ_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        ਜੇ (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ਗੰਢ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ਗੰਢ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ਗੰਢ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ਗੰਢ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Scope);
        m_ਵਿੱਚ_Scope = ਸੱਚ;
        VL_RESTORER_CLEAR(m_c_Func_ਨਾਂ);
        processAndIterate(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_ਨਾਂ);
        processAndIterate(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_ਨਾਂ);
        processAndIterate(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_ਵਿੱਚ_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        ਜੇ (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = nodep->varp()) {
                ਜੇ (!is_ਵਿੱਚ_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        ਜੇ (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->ਵਿਧੀ().ਦਲੀਲਾਂ(), nodep->pinsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().ਦਲੀਲਾਂ(), nodep->pinsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ਢੇਰ.clear();
        ਧੱਕੋ_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_ਦਲੀਲਾਂ = nodep->name();
        ਜੇ (!nodep->argTypes().ਖਾਲੀ_ਥਾਵਾਂ()) name_ਦਲੀਲਾਂ += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_ਨਾਂ.emplace(name_ਦਲੀਲਾਂ).ਦੂਜਾ, nodep,
                    "Duplicate cfunc name: '" << name_ਦਲੀਲਾਂ << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.ਗਿਣਤੀ_2(pair.ਪਹਿਲਾ) == 0, pair.ਦੂਜਾ,
                        "Local variable not in scope where referenced: " << pair.ਪਹਿਲਾ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        // Each branch is a separate local variable scope
        ਧੱਕੋ_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ thensp = nodep->thensp()) {
            ਧੱਕੋ_Local_Scope();
            processAndIterateList(thensp);
            ਕੱਢੋ_Local_Scope();
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ elsesp = nodep->elsesp()) {
            ਧੱਕੋ_Local_Scope();
            processAndIterateList(elsesp);
            ਕੱਢੋ_Local_Scope();
        }
        processExit(nodep);
        ਕੱਢੋ_Local_Scope();
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ਧੱਕੋ_Local_Scope();
        processAndIterate(nodep);
        ਕੱਢੋ_Local_Scope();
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        processAndIterate(nodep);
        ਜੇ (m_cfuncp) {
            m_localVars.ਪਾਓ(nodep);
            m_locals_ਢੇਰ.ਪਿੱਠ().ਪਾਓ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = ਮੂਲ;
};

//######################################################################
// Broken check entry point

ਖਾਲੀ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ਸਥਿਰ ਤਾਰਕਿਕ s_ਵਿੱਚ_Broken = ਝੂਠ;
    ਜੇ (VL_UNCOVERABLE(s_ਵਿੱਚ_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } ਨਹੀਂਤਾਂ {
        s_ਵਿੱਚ_Broken = ਸੱਚ;

        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_alloc_ਸਾਰਣੀ.m_mutex};

        // Mark every node in the tree
        ਅਚਲ uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_ਗੰਢ* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_ਸਾਰਣੀ.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#ਜੇ_ਅੰਤ
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            ਜੇ (nodep->maybe_Pointed_ਮੰਜ਼ਿਲ()) s_linkable_ਸਾਰਣੀ.ਜੋੜੋ_Linkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ਅਚਲ BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ਅਚਲ ਅੱਖਰ* ਅਚਲ whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_alloc_ਸਾਰਣੀ.checkForLeaks();
        s_linkable_ਸਾਰਣੀ.clear();
        s_brokenCntGlobal.inc();
        s_ਵਿੱਚ_Broken = ਝੂਠ;
    }
}

ਖਾਲੀ V3Broken::allowMidvisitorCheck(ਤਾਰਕਿਕ flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

ਖਾਲੀ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_ਸਤਰ* ਅਚਲ fl = ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()};
    Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstBegin{fl, "[EditWrapper]", ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
    ਜੋੜੋ_Newed(newp);
    deleted(newp);
#ਜੇ_ਅੰਤ
    VL_DO_DANGLING(newp->delete_ਰੁੱਖ(), newp);
}
