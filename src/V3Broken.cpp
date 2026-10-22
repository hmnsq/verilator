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
    ساكن constexpr uint8_t MIN_قدر = 1;
    ساكن constexpr uint8_t MAX_قدر = 127;

    uint8_t m_گنتی = MIN_قدر;

عام:
    uint8_t get() ثابت VL_MT_محفوظ {
        UASSERT(MIN_قدر <= m_گنتی && m_گنتی <= MAX_قدر, "Invalid generation number");
        أرجع m_گنتی;
    }

    فراغ inc() {
        ++m_گنتی;
        إذا (m_گنتی > MAX_قدر) m_گنتی = MIN_قدر;
    }
} s_brokenCntGlobal;

ساكن منطقي s_brokenAllowMidvisitorCheck = خطأ;

//######################################################################
// Table of allocated AstNode pointers

ساكن صنف Alloc_جدول final {
    یار صنف V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_سیٹ<ثابت Ast_گرہ*> m_allocated VL_GUARDED_BY(m_mutex);

عام:
    // METHODS
    فراغ جوڑو_Newed(ثابت Ast_گرہ* nodep) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        إذا (VL_UNCOVERABLE(!m_allocated.emplace(nodep).دوجا)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    فراغ deleted(ثابت Ast_گرہ* nodep) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        إذا (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

خاص:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    منطقي isAllocated(ثابت Ast_گرہ* nodep) ثابت VL_REQUIRES(m_mutex) {
        أرجع m_allocated.گنتی(nodep) != 0;
    }
    فراغ checkForLeaks() VL_REQUIRES(m_mutex) {
        إذا (!v3Global.opt.debugCheck()) أرجع;

        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        لكل (ثابت منطقي with_پٹھ : {خطأ, سچ}) {
            لكل (ثابت Ast_گرہ* ثابت nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                إذا (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ثابت منطقي has_پٹھ = nodep->backp() != خالی_سوچک;
                    إذا (has_پٹھ != with_پٹھ) استمر;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //لگاتار
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_پٹھ ? " with back pointer: " : ": ");
                    nodep->Ast_گرہ::dump(std::cerr);
                    std::cerr << سطر_انت;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_جدول;

فراغ V3Broken::جوڑو_Newed(ثابت Ast_گرہ* nodep) { s_alloc_جدول.جوڑو_Newed(nodep); }
فراغ V3Broken::deleted(ثابت Ast_گرہ* nodep) { s_alloc_جدول.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ساكن صنف Linkable_جدول final {
    // MEMBERS
    std::unordered_سیٹ<ثابت Ast_گرہ*> m_linkable;  // Set of all nodes allocated but not freed

عام:
    // METHODS
    فراغ clear() { m_linkable.clear(); }
    فراغ جوڑو_Linkable(ثابت Ast_گرہ* nodep) { m_linkable.emplace(nodep); }
    منطقي isLinkable(ثابت Ast_گرہ* nodep) ثابت { أرجع m_linkable.گنتی(nodep) != 0; }
} s_linkable_جدول;

منطقي V3Broken::isLinkable(ثابت Ast_گرہ* nodep) { أرجع s_linkable_جدول.isLinkable(nodep); }

//######################################################################
// Check every node in tree

صنف BrokenCheckVisitor final : عام VNVisitorConst {
    // Constants for marking we are under/not under a node
    ثابت uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ثابت uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::سیٹ<ثابت AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ثابت AstVar*, ثابت Ast_گرہ_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::سمتیہ<std::unordered_سیٹ<ثابت AstVar*>> m_locals_ڈھیر;
    // Number of write references encountered
    ناپ_t m_nWriteRefs = 0;
    // Number of function calls encountered
    ناپ_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت AstCFunc* m_cfuncp = خالی_سوچک;  // Current CFunc, if any
    منطقي m_وچ_Scope = خطأ;  // Under AstScope
    std::سیٹ<std::string> m_c_Func_ناں;  // CFunc by name in current class/module

خاص:
    // METHODS
    ساكن فراغ check_چوڑائی_Min(ثابت Ast_گرہ* nodep) {
        UASSERT_OBJ(nodep->چوڑائی() == nodep->چوڑائی_Min()
                        || v3Global.چوڑائی_Min_Usage() != VWidthMinUsage::MATCHES_چوڑائی,
                    nodep, "Width != WidthMin");
    }

    فراغ processEnter(Ast_گرہ* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ثابت حرف* ثابت whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        إذا (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        إذا (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_منزل_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_منزل_Hex(nodep->dtypep()));
        }
        إذا (v3Global.assertDTypesResolved()) {
            إذا (nodep->hasDType()) {
            } وإلا {
                UASSERT_OBJ(!VN_IS(nodep, گرہ_اظہاریہ), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_بچہ_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            إذا (ثابت Ast_گرہ_DType* ثابت dnodep = VN_CAST(nodep, گرہ_DType))
                check_چوڑائی_Min(dnodep);
        }
        check_چوڑائی_Min(nodep);
    }
    فراغ processExit(Ast_گرہ* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    فراغ processAndIterate(Ast_گرہ* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    فراغ processAndIterateList(Ast_گرہ* nodep) {
        طالما (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    فراغ ڈالو_Local_Scope() {
        إذا (m_cfuncp) m_locals_ڈھیر.emplace_پٹھ();
    }
    فراغ نکالو_Local_Scope() {
        إذا (m_cfuncp) m_locals_ڈھیر.نکالو_پٹھ();
    }
    منطقي is_وچ_Scope_Local(ثابت AstVar* varp) ثابت {
        لكل (ثابت خودکار& سیٹ : m_locals_ڈھیر) {
            إذا (سیٹ.گنتی(varp)) أرجع سچ;
        }
        أرجع خطأ;
    }
    ساكن فراغ checkArgRefs(Ast_گرہ_اظہاریہ* nodep, ثابت حرف* descrp, Ast_گرہ_اظہاریہ* argsp) {
        إذا (!std::strcmp(descrp, "TODO")) أرجع;  // Skip if not yet checked

        // Check each argument
        ثابت حرف* dp = descrp;
        لكل (Ast_گرہ_اظہاریہ* argp = argsp; argp; argp = VN_AS(argp->nextp(), گرہ_اظہاریہ)) {
            إذا (argp->fileline()->erroringOn()) أرجع;  // Intentionally skip all checks
            ثابت Ast_گرہ_اظہاریہ* ثابت lvalp = argp->get_VAccess_نشانہ_Recurse();
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
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ثابت ناپ_t nWriteRefs = m_nWriteRefs;
        ثابت ناپ_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        إذا (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گرہ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گرہ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    فراغ ویکھو(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گرہ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گرہ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    فراغ ویکھو(AstScope* nodep) override {
        VL_RESTORER(m_وچ_Scope);
        m_وچ_Scope = سچ;
        VL_RESTORER_CLEAR(m_c_Func_ناں);
        processAndIterate(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_ناں);
        processAndIterate(nodep);
    }
    فراغ ویکھو(Ast_گرہ_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_ناں);
        processAndIterate(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_وچ_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        إذا (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            إذا (ثابت AstVar* ثابت varp = nodep->varp()) {
                إذا (!is_وچ_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        إذا (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ ویکھو(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->طریقہ().دلیلاں(), nodep->pinsp());
    }
    فراغ ویکھو(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().دلیلاں(), nodep->pinsp());
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ڈھیر.clear();
        ڈالو_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_دلیلاں = nodep->name();
        إذا (!nodep->argTypes().خالی_تھاواں()) name_دلیلاں += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_ناں.emplace(name_دلیلاں).دوجا, nodep,
                    "Duplicate cfunc name: '" << name_دلیلاں << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        لكل (ثابت خودکار& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.گنتی(pair.پہلا) == 0, pair.دوجا,
                        "Local variable not in scope where referenced: " << pair.پہلا);
        }
    }
    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        // Each branch is a separate local variable scope
        ڈالو_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        إذا (Ast_گرہ* ثابت thensp = nodep->thensp()) {
            ڈالو_Local_Scope();
            processAndIterateList(thensp);
            نکالو_Local_Scope();
        }
        إذا (Ast_گرہ* ثابت elsesp = nodep->elsesp()) {
            ڈالو_Local_Scope();
            processAndIterateList(elsesp);
            نکالو_Local_Scope();
        }
        processExit(nodep);
        نکالو_Local_Scope();
    }
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ڈالو_Local_Scope();
        processAndIterate(nodep);
        نکالو_Local_Scope();
    }
    فراغ ویکھو(AstVar* nodep) override {
        processAndIterate(nodep);
        إذا (m_cfuncp) {
            m_localVars.داخل_کرو(nodep);
            m_locals_ڈھیر.پٹھ().داخل_کرو(nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = افتراضي;
};

//######################################################################
// Broken check entry point

فراغ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ساكن منطقي s_وچ_Broken = خطأ;
    إذا (VL_UNCOVERABLE(s_وچ_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } وإلا {
        s_وچ_Broken = سچ;

        ثابت V3_جندرا_Guard جندرا_2{s_alloc_جدول.m_mutex};

        // Mark every node in the tree
        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_گرہ* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#جے_تعریف_شدہ VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_جدول.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#جے_انت
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            إذا (nodep->maybe_Pointed_منزل()) s_linkable_جدول.جوڑو_Linkable(nodep);
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
        s_وچ_Broken = خطأ;
    }
}

فراغ V3Broken::allowMidvisitorCheck(منطقي flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

فراغ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_سطر* ثابت fl = جديد File_سطر{File_سطر::command_سطر_Filename()};
    Ast_گرہ* ثابت newp = جديد AstBegin{fl, "[EditWrapper]", خالی_سوچک, خطأ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#جے_بنا_تعریف VL_LEAK_CHECKS
    جوڑو_Newed(newp);
    deleted(newp);
#جے_انت
    VL_DO_DANGLING(newp->delete_درخت(), newp);
}
