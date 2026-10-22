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
    ساكن constexpr uint8_t MIN_ارزښت = 1;
    ساكن constexpr uint8_t MAX_ارزښت = 127;

    uint8_t m_شمېر = MIN_ارزښت;

عام:
    uint8_t get() ثابت VL_MT_خوندي {
        UASSERT(MIN_ارزښت <= m_شمېر && m_شمېر <= MAX_ارزښت, "Invalid generation number");
        أرجع m_شمېر;
    }

    فراغ inc() {
        ++m_شمېر;
        إذا (m_شمېر > MAX_ارزښت) m_شمېر = MIN_ارزښت;
    }
} s_brokenCntGlobal;

ساكن منطقي s_brokenAllowMidvisitorCheck = خطأ;

//######################################################################
// Table of allocated AstNode pointers

ساكن صنف Alloc_جدول final {
    ملګری صنف V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_سېټ<ثابت Ast_غوټه*> m_allocated VL_GUARDED_BY(m_mutex);

عام:
    // METHODS
    فراغ جمع_کړه_Newed(ثابت Ast_غوټه* nodep) VL_MT_خوندي_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        إذا (VL_UNCOVERABLE(!m_allocated.emplace(nodep).دوهم)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    فراغ deleted(ثابت Ast_غوټه* nodep) VL_MT_خوندي_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        إذا (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

خاص:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    منطقي isAllocated(ثابت Ast_غوټه* nodep) ثابت VL_REQUIRES(m_mutex) {
        أرجع m_allocated.شمېر(nodep) != 0;
    }
    فراغ checkForLeaks() VL_REQUIRES(m_mutex) {
        إذا (!v3Global.opt.debugCheck()) أرجع;

        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        لكل (ثابت منطقي with_شا : {خطأ, ریښتیا}) {
            لكل (ثابت Ast_غوټه* ثابت nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                إذا (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ثابت منطقي has_شا = nodep->backp() != تش_اشاره;
                    إذا (has_شا != with_شا) استمر;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //پرلپسې
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_شا ? " with back pointer: " : ": ");
                    nodep->Ast_غوټه::dump(std::cerr);
                    std::cerr << کرښه_پای;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_جدول;

فراغ V3Broken::جمع_کړه_Newed(ثابت Ast_غوټه* nodep) { s_alloc_جدول.جمع_کړه_Newed(nodep); }
فراغ V3Broken::deleted(ثابت Ast_غوټه* nodep) { s_alloc_جدول.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ساكن صنف Linkable_جدول final {
    // MEMBERS
    std::unordered_سېټ<ثابت Ast_غوټه*> m_linkable;  // Set of all nodes allocated but not freed

عام:
    // METHODS
    فراغ clear() { m_linkable.clear(); }
    فراغ جمع_کړه_Linkable(ثابت Ast_غوټه* nodep) { m_linkable.emplace(nodep); }
    منطقي isLinkable(ثابت Ast_غوټه* nodep) ثابت { أرجع m_linkable.شمېر(nodep) != 0; }
} s_linkable_جدول;

منطقي V3Broken::isLinkable(ثابت Ast_غوټه* nodep) { أرجع s_linkable_جدول.isLinkable(nodep); }

//######################################################################
// Check every node in tree

صنف BrokenCheckVisitor final : عام VNVisitorConst {
    // Constants for marking we are under/not under a node
    ثابت uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ثابت uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::سېټ<ثابت AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ثابت AstVar*, ثابت Ast_غوټه_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::ویکټور<std::unordered_سېټ<ثابت AstVar*>> m_locals_ډېری;
    // Number of write references encountered
    اندازه_t m_nWriteRefs = 0;
    // Number of function calls encountered
    اندازه_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت AstCFunc* m_cfuncp = تش_اشاره;  // Current CFunc, if any
    منطقي m_دننه_Scope = خطأ;  // Under AstScope
    std::سېټ<std::string> m_c_Func_نومونه;  // CFunc by name in current class/module

خاص:
    // METHODS
    ساكن فراغ check_پلنوالی_Min(ثابت Ast_غوټه* nodep) {
        UASSERT_OBJ(nodep->پلنوالی() == nodep->پلنوالی_Min()
                        || v3Global.پلنوالی_Min_Usage() != VWidthMinUsage::MATCHES_پلنوالی,
                    nodep, "Width != WidthMin");
    }

    فراغ processEnter(Ast_غوټه* nodep) {
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
                UASSERT_OBJ(!VN_IS(nodep, غوټه_بیان), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_ماشوم_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            إذا (ثابت Ast_غوټه_DType* ثابت dnodep = VN_CAST(nodep, غوټه_DType))
                check_پلنوالی_Min(dnodep);
        }
        check_پلنوالی_Min(nodep);
    }
    فراغ processExit(Ast_غوټه* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    فراغ processAndIterate(Ast_غوټه* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    فراغ processAndIterateList(Ast_غوټه* nodep) {
        طالما (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    فراغ ټېل_Local_Scope() {
        إذا (m_cfuncp) m_locals_ډېری.emplace_شا();
    }
    فراغ راوباسه_Local_Scope() {
        إذا (m_cfuncp) m_locals_ډېری.راوباسه_شا();
    }
    منطقي is_دننه_Scope_Local(ثابت AstVar* varp) ثابت {
        لكل (ثابت خپلکاری& سېټ : m_locals_ډېری) {
            إذا (سېټ.شمېر(varp)) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    ساكن فراغ checkArgRefs(Ast_غوټه_بیان* nodep, ثابت حرف* descrp, Ast_غوټه_بیان* argsp) {
        إذا (!std::strcmp(descrp, "TODO")) أرجع;  // Skip if not yet checked

        // Check each argument
        ثابت حرف* dp = descrp;
        لكل (Ast_غوټه_بیان* argp = argsp; argp; argp = VN_AS(argp->nextp(), غوټه_بیان)) {
            إذا (argp->fileline()->erroringOn()) أرجع;  // Intentionally skip all checks
            ثابت Ast_غوټه_بیان* ثابت lvalp = argp->get_VAccess_موخه_Recurse();
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
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
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
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), غوټه_Var_Ref)
                      && !VN_AS(nodep->lhsp(), غوټه_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    فراغ لیدنه(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), غوټه_Var_Ref)
                      && !VN_AS(nodep->lhsp(), غوټه_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    فراغ لیدنه(AstScope* nodep) override {
        VL_RESTORER(m_دننه_Scope);
        m_دننه_Scope = ریښتیا;
        VL_RESTORER_CLEAR(m_c_Func_نومونه);
        processAndIterate(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_نومونه);
        processAndIterate(nodep);
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_نومونه);
        processAndIterate(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_دننه_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        إذا (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            إذا (ثابت AstVar* ثابت varp = nodep->varp()) {
                إذا (!is_دننه_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        إذا (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->لاره().دلیلونه(), nodep->pinsp());
    }
    فراغ لیدنه(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().دلیلونه(), nodep->pinsp());
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ډېری.clear();
        ټېل_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_دلیلونه = nodep->name();
        إذا (!nodep->argTypes().تشه()) name_دلیلونه += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_نومونه.emplace(name_دلیلونه).دوهم, nodep,
                    "Duplicate cfunc name: '" << name_دلیلونه << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        لكل (ثابت خپلکاری& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.شمېر(pair.لومړی) == 0, pair.دوهم,
                        "Local variable not in scope where referenced: " << pair.لومړی);
        }
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        // Each branch is a separate local variable scope
        ټېل_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        إذا (Ast_غوټه* ثابت thensp = nodep->thensp()) {
            ټېل_Local_Scope();
            processAndIterateList(thensp);
            راوباسه_Local_Scope();
        }
        إذا (Ast_غوټه* ثابت elsesp = nodep->elsesp()) {
            ټېل_Local_Scope();
            processAndIterateList(elsesp);
            راوباسه_Local_Scope();
        }
        processExit(nodep);
        راوباسه_Local_Scope();
    }
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ټېل_Local_Scope();
        processAndIterate(nodep);
        راوباسه_Local_Scope();
    }
    فراغ لیدنه(AstVar* nodep) override {
        processAndIterate(nodep);
        إذا (m_cfuncp) {
            m_localVars.ورننباسه(nodep);
            m_locals_ډېری.شا().ورننباسه(nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = افتراضي;
};

//######################################################################
// Broken check entry point

فراغ V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ساكن منطقي s_دننه_Broken = خطأ;
    إذا (VL_UNCOVERABLE(s_دننه_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } وإلا {
        s_دننه_Broken = ریښتیا;

        ثابت V3_قلف_Guard قلف_2{s_alloc_جدول.m_mutex};

        // Mark every node in the tree
        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_غوټه* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#که_تعریف_شوی VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_جدول.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#د_که_پای
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            إذا (nodep->maybe_Pointed_منزل()) s_linkable_جدول.جمع_کړه_Linkable(nodep);
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
        s_دننه_Broken = خطأ;
    }
}

فراغ V3Broken::allowMidvisitorCheck(منطقي flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

فراغ V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_کرښه* ثابت fl = جديد File_کرښه{File_کرښه::command_کرښه_Filename()};
    Ast_غوټه* ثابت newp = جديد AstBegin{fl, "[EditWrapper]", تش_اشاره, خطأ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#که_نه_تعریف_شوی VL_LEAK_CHECKS
    جمع_کړه_Newed(newp);
    deleted(newp);
#د_که_پای
    VL_DO_DANGLING(newp->delete_ونه(), newp);
}
