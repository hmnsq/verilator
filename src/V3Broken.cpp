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

#ضمن "V3PchAstMT.h"

#ضمن "V3Broken.h"

#ضمن "V3ConstPool.h"

#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

ساكن صنف BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    ساكن constexpr uint8_t MIN_قىممەت = 1;
    ساكن constexpr uint8_t MAX_قىممەت = 127;

    uint8_t m_سانى = MIN_قىممەت;

عام:
    uint8_t get() ثابت VL_MT_SAFE {
        UASSERT(MIN_قىممەت <= m_سانى && m_سانى <= MAX_قىممەت, "Invalid generation number");
        أرجع m_سانى;
    }

    فراغ inc() {
        ++m_سانى;
        إذا (m_سانى > MAX_قىممەت) m_سانى = MIN_قىممەت;
    }
} s_brokenCntGlobal;

ساكن منطقي s_brokenAllowMidvisitorCheck = خطأ;

//######################################################################
// Table of allocated AstNode pointers

ساكن صنف AllocTable final {
    friend صنف V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<ثابت Ast_تۈگۈن*> m_allocated VL_GUARDED_BY(m_mutex);

عام:
    // METHODS
    فراغ addNewed(ثابت Ast_تۈگۈن* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3LockGuard lock{m_mutex};
        إذا (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    فراغ deleted(ثابت Ast_تۈگۈن* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3LockGuard lock{m_mutex};
        إذا (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

خاص:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    منطقي isAllocated(ثابت Ast_تۈگۈن* nodep) ثابت VL_REQUIRES(m_mutex) {
        أرجع m_allocated.سانى(nodep) != 0;
    }
    فراغ checkForLeaks() VL_REQUIRES(m_mutex) {
        إذا (!v3Global.opt.debugCheck()) أرجع;

        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        لكل (ثابت منطقي withBack : {خطأ, true}) {
            لكل (ثابت Ast_تۈگۈن* ثابت nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                إذا (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ثابت منطقي hasBack = nodep->backp() != nullptr;
                    إذا (hasBack != withBack) استمر;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_تۈگۈن::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

فراغ V3Broken::addNewed(ثابت Ast_تۈگۈن* nodep) { s_allocTable.addNewed(nodep); }
فراغ V3Broken::deleted(ثابت Ast_تۈگۈن* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ساكن صنف LinkableTable final {
    // MEMBERS
    std::unordered_set<ثابت Ast_تۈگۈن*> m_linkable;  // Set of all nodes allocated but not freed

عام:
    // METHODS
    فراغ clear() { m_linkable.clear(); }
    فراغ addLinkable(ثابت Ast_تۈگۈن* nodep) { m_linkable.emplace(nodep); }
    منطقي isLinkable(ثابت Ast_تۈگۈن* nodep) ثابت { أرجع m_linkable.سانى(nodep) != 0; }
} s_linkableTable;

منطقي V3Broken::isLinkable(ثابت Ast_تۈگۈن* nodep) { أرجع s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

صنف BrokenCheckVisitor final : عام VNVisitorConst {
    // Constants for marking we are under/not under a node
    ثابت uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ثابت uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<ثابت AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ثابت AstVar*, ثابت Ast_تۈگۈن_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<ثابت AstVar*>> m_locals_دۆۋە;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    منطقي m_inScope = خطأ;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

خاص:
    // METHODS
    ساكن فراغ checkWidthMin(ثابت Ast_تۈگۈن* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    فراغ processEnter(Ast_تۈگۈن* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ثابت حرف* ثابت whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        إذا (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        إذا (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        إذا (v3Global.assertDTypesResolved()) {
            إذا (nodep->hasDType()) {
            } وإلا {
                UASSERT_OBJ(!VN_IS(nodep, تۈگۈن_ئىپادە), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            إذا (ثابت Ast_تۈگۈن_DType* ثابت dnodep = VN_CAST(nodep, تۈگۈن_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    فراغ processExit(Ast_تۈگۈن* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    فراغ processAndIterate(Ast_تۈگۈن* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    فراغ processAndIterateList(Ast_تۈگۈن* nodep) {
        طالما (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    فراغ ئىتتىرىش_Local_Scope() {
        إذا (m_cfuncp) m_locals_دۆۋە.emplace_back();
    }
    فراغ چىقىرىش_Local_Scope() {
        إذا (m_cfuncp) m_locals_دۆۋە.چىقىرىش_back();
    }
    منطقي isInScopeLocal(ثابت AstVar* varp) ثابت {
        لكل (ثابت auto& set : m_locals_دۆۋە) {
            إذا (set.سانى(varp)) أرجع true;
        }
        أرجع خطأ;
    }
    ساكن فراغ checkArgRefs(Ast_تۈگۈن_ئىپادە* nodep, ثابت حرف* descrp, Ast_تۈگۈن_ئىپادە* argsp) {
        إذا (!std::strcmp(descrp, "TODO")) أرجع;  // Skip if not yet checked

        // Check each argument
        ثابت حرف* dp = descrp;
        لكل (Ast_تۈگۈن_ئىپادە* argp = argsp; argp; argp = VN_AS(argp->nextp(), تۈگۈن_ئىپادە)) {
            إذا (argp->fileline()->erroringOn()) أرجع;  // Intentionally skip all checks
            ثابت Ast_تۈگۈن_ئىپادە* ثابت lvalp = argp->getVAccessTargetRecurse();
            ثابت VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            إذا (dp[0] == '+') --dp;  // Repeats the entry before it
            بدل (dp[0]) {
            حالة 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                اكسر;
            حالة 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                اكسر;
            حالة 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                اكسر;
            افتراضي:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                اكسر;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    فراغ visit(Ast_تۈگۈن_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ثابت size_t nWriteRefs = m_nWriteRefs;
        ثابت size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        إذا (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), تۈگۈن_Var_Ref)
                      && !VN_AS(nodep->lhsp(), تۈگۈن_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    فراغ visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), تۈگۈن_Var_Ref)
                      && !VN_AS(nodep->lhsp(), تۈگۈن_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = true;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    فراغ visit(Ast_تۈگۈن_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    فراغ visit(Ast_تۈگۈن_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    فراغ visit(Ast_تۈگۈن_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        إذا (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            إذا (ثابت AstVar* ثابت varp = nodep->varp()) {
                إذا (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        إذا (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    فراغ visit(Ast_تۈگۈن_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    فراغ visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    فراغ visit(Ast_تۈگۈن_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_دۆۋە.clear();
        ئىتتىرىش_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        إذا (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        لكل (ثابت auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.سانى(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    فراغ visit(Ast_تۈگۈن_If* nodep) override {
        // Each branch is a separate local variable scope
        ئىتتىرىش_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        إذا (Ast_تۈگۈن* ثابت thensp = nodep->thensp()) {
            ئىتتىرىش_Local_Scope();
            processAndIterateList(thensp);
            چىقىرىش_Local_Scope();
        }
        إذا (Ast_تۈگۈن* ثابت elsesp = nodep->elsesp()) {
            ئىتتىرىش_Local_Scope();
            processAndIterateList(elsesp);
            چىقىرىش_Local_Scope();
        }
        processExit(nodep);
        چىقىرىش_Local_Scope();
    }
    فراغ visit(Ast_تۈگۈن_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ئىتتىرىش_Local_Scope();
        processAndIterate(nodep);
        چىقىرىش_Local_Scope();
    }
    فراغ visit(AstVar* nodep) override {
        processAndIterate(nodep);
        إذا (m_cfuncp) {
            m_localVars.قىستۇرۇش(nodep);
            m_locals_دۆۋە.back().قىستۇرۇش(nodep);
        }
    }
    فراغ visit(Ast_تۈگۈن* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = افتراضي;
};

//######################################################################
// Broken check entry point

فراغ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ساكن منطقي s_inBroken = خطأ;
    إذا (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } وإلا {
        s_inBroken = true;

        ثابت V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_تۈگۈن* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            إذا (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ثابت BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ثابت حرف* ثابت whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = خطأ;
    }
}

فراغ V3Broken::allowMidvisitorCheck(منطقي flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

فراغ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_قۇر* ثابت fl = جديد File_قۇر{File_قۇر::command_قۇر_Filename()};
    Ast_تۈگۈن* ثابت newp = جديد AstBegin{fl, "[EditWrapper]", nullptr, خطأ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_دەرەخ(), newp);
}
