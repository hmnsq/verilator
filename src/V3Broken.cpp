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
    ساكن constexpr uint8_t MIN_مقدار = 1;
    ساكن constexpr uint8_t MAX_مقدار = 127;

    uint8_t m_شمار = MIN_مقدار;

عام:
    uint8_t get() ثابت VL_MT_امن {
        UASSERT(MIN_مقدار <= m_شمار && m_شمار <= MAX_مقدار, "Invalid generation number");
        أرجع m_شمار;
    }

    فراغ inc() {
        ++m_شمار;
        إذا (m_شمار > MAX_مقدار) m_شمار = MIN_مقدار;
    }
} s_brokenCntGlobal;

ساكن منطقي s_brokenAllowMidvisitorCheck = خطأ;

//######################################################################
// Table of allocated AstNode pointers

ساكن صنف Alloc_جدول final {
    دوست صنف V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_مجموعه<ثابت Ast_گره*> m_allocated VL_GUARDED_BY(m_mutex);

عام:
    // METHODS
    فراغ جمع_کن_Newed(ثابت Ast_گره* nodep) VL_MT_امن_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        إذا (VL_UNCOVERABLE(!m_allocated.emplace(nodep).دومی)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    فراغ deleted(ثابت Ast_گره* nodep) VL_MT_امن_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        إذا (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

خاص:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    منطقي isAllocated(ثابت Ast_گره* nodep) ثابت VL_REQUIRES(m_mutex) {
        أرجع m_allocated.شمار(nodep) != 0;
    }
    فراغ checkForLeaks() VL_REQUIRES(m_mutex) {
        إذا (!v3Global.opt.debugCheck()) أرجع;

        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        لكل (ثابت منطقي with_پشت : {خطأ, درست}) {
            لكل (ثابت Ast_گره* ثابت nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                إذا (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ثابت منطقي has_پشت = nodep->backp() != اشاره_تهی;
                    إذا (has_پشت != with_پشت) استمر;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //پیاپی
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_پشت ? " with back pointer: " : ": ");
                    nodep->Ast_گره::dump(std::cerr);
                    std::cerr << پایان_سطر;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_جدول;

فراغ V3Broken::جمع_کن_Newed(ثابت Ast_گره* nodep) { s_alloc_جدول.جمع_کن_Newed(nodep); }
فراغ V3Broken::deleted(ثابت Ast_گره* nodep) { s_alloc_جدول.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ساكن صنف Linkable_جدول final {
    // MEMBERS
    std::unordered_مجموعه<ثابت Ast_گره*> m_linkable;  // Set of all nodes allocated but not freed

عام:
    // METHODS
    فراغ clear() { m_linkable.clear(); }
    فراغ جمع_کن_Linkable(ثابت Ast_گره* nodep) { m_linkable.emplace(nodep); }
    منطقي isLinkable(ثابت Ast_گره* nodep) ثابت { أرجع m_linkable.شمار(nodep) != 0; }
} s_linkable_جدول;

منطقي V3Broken::isLinkable(ثابت Ast_گره* nodep) { أرجع s_linkable_جدول.isLinkable(nodep); }

//######################################################################
// Check every node in tree

صنف BrokenCheckVisitor final : عام VNVisitorConst {
    // Constants for marking we are under/not under a node
    ثابت uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ثابت uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::مجموعه<ثابت AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ثابت AstVar*, ثابت Ast_گره_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::وکتور<std::unordered_مجموعه<ثابت AstVar*>> m_locals_پشته;
    // Number of write references encountered
    اندازه_t m_nWriteRefs = 0;
    // Number of function calls encountered
    اندازه_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت AstCFunc* m_cfuncp = اشاره_تهی;  // Current CFunc, if any
    منطقي m_درون_Scope = خطأ;  // Under AstScope
    std::مجموعه<std::string> m_c_Func_نامها;  // CFunc by name in current class/module

خاص:
    // METHODS
    ساكن فراغ check_عرض_Min(ثابت Ast_گره* nodep) {
        UASSERT_OBJ(nodep->عرض() == nodep->عرض_Min()
                        || v3Global.عرض_Min_Usage() != VWidthMinUsage::MATCHES_عرض,
                    nodep, "Width != WidthMin");
    }

    فراغ processEnter(Ast_گره* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ثابت حرف* ثابت whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        إذا (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        إذا (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_مقصد_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_مقصد_Hex(nodep->dtypep()));
        }
        إذا (v3Global.assertDTypesResolved()) {
            إذا (nodep->hasDType()) {
            } وإلا {
                UASSERT_OBJ(!VN_IS(nodep, گره_عبارت), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_فرزند_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            إذا (ثابت Ast_گره_DType* ثابت dnodep = VN_CAST(nodep, گره_DType))
                check_عرض_Min(dnodep);
        }
        check_عرض_Min(nodep);
    }
    فراغ processExit(Ast_گره* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    فراغ processAndIterate(Ast_گره* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    فراغ processAndIterateList(Ast_گره* nodep) {
        طالما (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    فراغ افزودن_Local_Scope() {
        إذا (m_cfuncp) m_locals_پشته.emplace_پشت();
    }
    فراغ برداشت_Local_Scope() {
        إذا (m_cfuncp) m_locals_پشته.برداشت_پشت();
    }
    منطقي is_درون_Scope_Local(ثابت AstVar* varp) ثابت {
        لكل (ثابت خودکار& مجموعه : m_locals_پشته) {
            إذا (مجموعه.شمار(varp)) أرجع درست;
        }
        أرجع خطأ;
    }
    ساكن فراغ checkArgRefs(Ast_گره_عبارت* nodep, ثابت حرف* descrp, Ast_گره_عبارت* argsp) {
        إذا (!std::strcmp(descrp, "TODO")) أرجع;  // Skip if not yet checked

        // Check each argument
        ثابت حرف* dp = descrp;
        لكل (Ast_گره_عبارت* argp = argsp; argp; argp = VN_AS(argp->nextp(), گره_عبارت)) {
            إذا (argp->fileline()->erroringOn()) أرجع;  // Intentionally skip all checks
            ثابت Ast_گره_عبارت* ثابت lvalp = argp->get_VAccess_هدف_Recurse();
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
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ثابت اندازه_t nWriteRefs = m_nWriteRefs;
        ثابت اندازه_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        إذا (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گره_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گره_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    فراغ بازدید(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گره_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گره_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    فراغ بازدید(AstScope* nodep) override {
        VL_RESTORER(m_درون_Scope);
        m_درون_Scope = درست;
        VL_RESTORER_CLEAR(m_c_Func_نامها);
        processAndIterate(nodep);
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_نامها);
        processAndIterate(nodep);
    }
    فراغ بازدید(Ast_گره_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_نامها);
        processAndIterate(nodep);
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_درون_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        إذا (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            إذا (ثابت AstVar* ثابت varp = nodep->varp()) {
                إذا (!is_درون_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        إذا (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    فراغ بازدید(Ast_گره_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ بازدید(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->روش().آرگومانها(), nodep->pinsp());
    }
    فراغ بازدید(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().آرگومانها(), nodep->pinsp());
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ بازدید(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_پشته.clear();
        افزودن_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_آرگومانها = nodep->name();
        إذا (!nodep->argTypes().خالی()) name_آرگومانها += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_نامها.emplace(name_آرگومانها).دومی, nodep,
                    "Duplicate cfunc name: '" << name_آرگومانها << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        لكل (ثابت خودکار& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.شمار(pair.اولی) == 0, pair.دومی,
                        "Local variable not in scope where referenced: " << pair.اولی);
        }
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        // Each branch is a separate local variable scope
        افزودن_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        إذا (Ast_گره* ثابت thensp = nodep->thensp()) {
            افزودن_Local_Scope();
            processAndIterateList(thensp);
            برداشت_Local_Scope();
        }
        إذا (Ast_گره* ثابت elsesp = nodep->elsesp()) {
            افزودن_Local_Scope();
            processAndIterateList(elsesp);
            برداشت_Local_Scope();
        }
        processExit(nodep);
        برداشت_Local_Scope();
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        افزودن_Local_Scope();
        processAndIterate(nodep);
        برداشت_Local_Scope();
    }
    فراغ بازدید(AstVar* nodep) override {
        processAndIterate(nodep);
        إذا (m_cfuncp) {
            m_localVars.درج(nodep);
            m_locals_پشته.پشت().درج(nodep);
        }
    }
    فراغ بازدید(Ast_گره* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

عام:
    // CONSTRUCTORS
    صریح BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = افتراضي;
};

//######################################################################
// Broken check entry point

فراغ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ساكن منطقي s_درون_Broken = خطأ;
    إذا (VL_UNCOVERABLE(s_درون_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } وإلا {
        s_درون_Broken = درست;

        ثابت V3_قفل_Guard قفل_2{s_alloc_جدول.m_mutex};

        // Mark every node in the tree
        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_گره* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#اگر_تعریف_شده VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_جدول.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#پایان_اگر
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            إذا (nodep->maybe_Pointed_مقصد()) s_linkable_جدول.جمع_کن_Linkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ثابت BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ثابت حرف* ثابت whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_alloc_جدول.checkForLeaks();
        s_linkable_جدول.clear();
        s_brokenCntGlobal.inc();
        s_درون_Broken = خطأ;
    }
}

فراغ V3Broken::allowMidvisitorCheck(منطقي flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

فراغ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_سطر* ثابت fl = جديد File_سطر{File_سطر::command_سطر_Filename()};
    Ast_گره* ثابت newp = جديد AstBegin{fl, "[EditWrapper]", اشاره_تهی, خطأ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#اگر_تعریف_نشده VL_LEAK_CHECKS
    جمع_کن_Newed(newp);
    deleted(newp);
#پایان_اگر
    VL_DO_DANGLING(newp->delete_درخت(), newp);
}
