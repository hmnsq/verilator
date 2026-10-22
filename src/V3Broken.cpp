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

#شامل "V3PchAstMT.h"

#شامل "V3Broken.h"

#شامل "V3ConstPool.h"

#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

ایستا رده BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    ایستا constexpr uint8_t MIN_مقدار = 1;
    ایستا constexpr uint8_t MAX_مقدار = 127;

    uint8_t m_شمار = MIN_مقدار;

عمومی:
    uint8_t get() ثابت VL_MT_SAFE {
        UASSERT(MIN_مقدار <= m_شمار && m_شمار <= MAX_مقدار, "Invalid generation number");
        بازگشت m_شمار;
    }

    تهی inc() {
        ++m_شمار;
        اگر (m_شمار > MAX_مقدار) m_شمار = MIN_مقدار;
    }
} s_brokenCntGlobal;

ایستا منطقی s_brokenAllowMidvisitorCheck = نادرست;

//######################################################################
// Table of allocated AstNode pointers

ایستا رده AllocTable final {
    friend رده V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<ثابت Ast_گره*> m_allocated VL_GUARDED_BY(m_mutex);

عمومی:
    // METHODS
    تهی addNewed(ثابت Ast_گره* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3LockGuard lock{m_mutex};
        اگر (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    تهی deleted(ثابت Ast_گره* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3LockGuard lock{m_mutex};
        اگر (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

خصوصی:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    منطقی isAllocated(ثابت Ast_گره* nodep) ثابت VL_REQUIRES(m_mutex) {
        بازگشت m_allocated.شمار(nodep) != 0;
    }
    تهی checkForLeaks() VL_REQUIRES(m_mutex) {
        اگر (!v3Global.opt.debugCheck()) بازگشت;

        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        برای (ثابت منطقی withBack : {نادرست, درست}) {
            برای (ثابت Ast_گره* ثابت nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                اگر (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ثابت منطقی hasBack = nodep->backp() != nullptr;
                    اگر (hasBack != withBack) ادامه;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_گره::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

تهی V3Broken::addNewed(ثابت Ast_گره* nodep) { s_allocTable.addNewed(nodep); }
تهی V3Broken::deleted(ثابت Ast_گره* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ایستا رده LinkableTable final {
    // MEMBERS
    std::unordered_set<ثابت Ast_گره*> m_linkable;  // Set of all nodes allocated but not freed

عمومی:
    // METHODS
    تهی clear() { m_linkable.clear(); }
    تهی addLinkable(ثابت Ast_گره* nodep) { m_linkable.emplace(nodep); }
    منطقی isLinkable(ثابت Ast_گره* nodep) ثابت { بازگشت m_linkable.شمار(nodep) != 0; }
} s_linkableTable;

منطقی V3Broken::isLinkable(ثابت Ast_گره* nodep) { بازگشت s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

رده BrokenCheckVisitor final : عمومی VNVisitorConst {
    // Constants for marking we are under/not under a node
    ثابت uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ثابت uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<ثابت AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ثابت AstVar*, ثابت Ast_گره_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<ثابت AstVar*>> m_locals_پشته;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    منطقی m_inScope = نادرست;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

خصوصی:
    // METHODS
    ایستا تهی checkWidthMin(ثابت Ast_گره* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    تهی processEnter(Ast_گره* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ثابت نویسه* ثابت whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        اگر (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        اگر (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        اگر (v3Global.assertDTypesResolved()) {
            اگر (nodep->hasDType()) {
            } وگرنه {
                UASSERT_OBJ(!VN_IS(nodep, گره_عبارت), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            اگر (ثابت Ast_گره_DType* ثابت dnodep = VN_CAST(nodep, گره_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    تهی processExit(Ast_گره* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    تهی processAndIterate(Ast_گره* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    تهی processAndIterateList(Ast_گره* nodep) {
        وقتی (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    تهی افزودن_Local_Scope() {
        اگر (m_cfuncp) m_locals_پشته.emplace_back();
    }
    تهی برداشت_Local_Scope() {
        اگر (m_cfuncp) m_locals_پشته.برداشت_back();
    }
    منطقی isInScopeLocal(ثابت AstVar* varp) ثابت {
        برای (ثابت auto& set : m_locals_پشته) {
            اگر (set.شمار(varp)) بازگشت درست;
        }
        بازگشت نادرست;
    }
    ایستا تهی checkArgRefs(Ast_گره_عبارت* nodep, ثابت نویسه* descrp, Ast_گره_عبارت* argsp) {
        اگر (!std::strcmp(descrp, "TODO")) بازگشت;  // Skip if not yet checked

        // Check each argument
        ثابت نویسه* dp = descrp;
        برای (Ast_گره_عبارت* argp = argsp; argp; argp = VN_AS(argp->nextp(), گره_عبارت)) {
            اگر (argp->fileline()->erroringOn()) بازگشت;  // Intentionally skip all checks
            ثابت Ast_گره_عبارت* ثابت lvalp = argp->getVAccessTargetRecurse();
            ثابت VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            اگر (dp[0] == '+') --dp;  // Repeats the entry before it
            انتخاب (dp[0]) {
            حالت 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                بشکن;
            حالت 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                بشکن;
            حالت 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                بشکن;
            پیشفرض:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                بشکن;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    تهی visit(Ast_گره_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ثابت size_t nWriteRefs = m_nWriteRefs;
        ثابت size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        اگر (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گره_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گره_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    تهی visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گره_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گره_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = درست;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        اگر (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            اگر (ثابت AstVar* ثابت varp = nodep->varp()) {
                اگر (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        اگر (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    تهی visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    تهی visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_پشته.clear();
        افزودن_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        اگر (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        برای (ثابت auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.شمار(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    تهی visit(Ast_گره_If* nodep) override {
        // Each branch is a separate local variable scope
        افزودن_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        اگر (Ast_گره* ثابت thensp = nodep->thensp()) {
            افزودن_Local_Scope();
            processAndIterateList(thensp);
            برداشت_Local_Scope();
        }
        اگر (Ast_گره* ثابت elsesp = nodep->elsesp()) {
            افزودن_Local_Scope();
            processAndIterateList(elsesp);
            برداشت_Local_Scope();
        }
        processExit(nodep);
        برداشت_Local_Scope();
    }
    تهی visit(Ast_گره_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        افزودن_Local_Scope();
        processAndIterate(nodep);
        برداشت_Local_Scope();
    }
    تهی visit(AstVar* nodep) override {
        processAndIterate(nodep);
        اگر (m_cfuncp) {
            m_localVars.درج(nodep);
            m_locals_پشته.back().درج(nodep);
        }
    }
    تهی visit(Ast_گره* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = پیشفرض;
};

//######################################################################
// Broken check entry point

تهی V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ایستا منطقی s_inBroken = نادرست;
    اگر (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } وگرنه {
        s_inBroken = درست;

        ثابت V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_گره* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            اگر (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ثابت BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ثابت نویسه* ثابت whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = نادرست;
    }
}

تهی V3Broken::allowMidvisitorCheck(منطقی flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

تهی V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_سطر* ثابت fl = جدید File_سطر{File_سطر::command_سطر_Filename()};
    Ast_گره* ثابت newp = جدید AstBegin{fl, "[EditWrapper]", nullptr, نادرست};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_درخت(), newp);
}
