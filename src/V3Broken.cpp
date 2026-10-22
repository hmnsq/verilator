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

ساکن جماعت BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    ساکن constexpr uint8_t MIN_قدر = 1;
    ساکن constexpr uint8_t MAX_قدر = 127;

    uint8_t m_گنتی = MIN_قدر;

عوامی:
    uint8_t get() ثابت VL_MT_محفوظ {
        UASSERT(MIN_قدر <= m_گنتی && m_گنتی <= MAX_قدر, "Invalid generation number");
        واپس m_گنتی;
    }

    خالی inc() {
        ++m_گنتی;
        اگر (m_گنتی > MAX_قدر) m_گنتی = MIN_قدر;
    }
} s_brokenCntGlobal;

ساکن منطقی s_brokenAllowMidvisitorCheck = جھوٹ;

//######################################################################
// Table of allocated AstNode pointers

ساکن جماعت Alloc_جدول final {
    دوست جماعت V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_سیٹ<ثابت Ast_گرہ*> m_allocated VL_GUARDED_BY(m_mutex);

عوامی:
    // METHODS
    خالی جوڑو_Newed(ثابت Ast_گرہ* nodep) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        اگر (VL_UNCOVERABLE(!m_allocated.emplace(nodep).دوسرا)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    خالی deleted(ثابت Ast_گرہ* nodep) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        اگر (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

نجی:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    منطقی isAllocated(ثابت Ast_گرہ* nodep) ثابت VL_REQUIRES(m_mutex) {
        واپس m_allocated.گنتی(nodep) != 0;
    }
    خالی checkForLeaks() VL_REQUIRES(m_mutex) {
        اگر (!v3Global.opt.debugCheck()) واپس;

        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        برائے (ثابت منطقی with_عقب : {جھوٹ, سچ}) {
            برائے (ثابت Ast_گرہ* ثابت nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                اگر (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ثابت منطقی has_عقب = nodep->backp() != خالی_اشاریہ;
                    اگر (has_عقب != with_عقب) جاری;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //تسلسل
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_عقب ? " with back pointer: " : ": ");
                    nodep->Ast_گرہ::dump(std::cerr);
                    std::cerr << سطر_ختم;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_جدول;

خالی V3Broken::جوڑو_Newed(ثابت Ast_گرہ* nodep) { s_alloc_جدول.جوڑو_Newed(nodep); }
خالی V3Broken::deleted(ثابت Ast_گرہ* nodep) { s_alloc_جدول.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

ساکن جماعت Linkable_جدول final {
    // MEMBERS
    std::unordered_سیٹ<ثابت Ast_گرہ*> m_linkable;  // Set of all nodes allocated but not freed

عوامی:
    // METHODS
    خالی clear() { m_linkable.clear(); }
    خالی جوڑو_Linkable(ثابت Ast_گرہ* nodep) { m_linkable.emplace(nodep); }
    منطقی isLinkable(ثابت Ast_گرہ* nodep) ثابت { واپس m_linkable.گنتی(nodep) != 0; }
} s_linkable_جدول;

منطقی V3Broken::isLinkable(ثابت Ast_گرہ* nodep) { واپس s_linkable_جدول.isLinkable(nodep); }

//######################################################################
// Check every node in tree

جماعت BrokenCheckVisitor final : عوامی VNVisitorConst {
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
    سائز_t m_nWriteRefs = 0;
    // Number of function calls encountered
    سائز_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ثابت AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current CFunc, if any
    منطقی m_اندر_Scope = جھوٹ;  // Under AstScope
    std::سیٹ<std::string> m_c_Func_نام;  // CFunc by name in current class/module

نجی:
    // METHODS
    ساکن خالی check_چوڑائی_Min(ثابت Ast_گرہ* nodep) {
        UASSERT_OBJ(nodep->چوڑائی() == nodep->چوڑائی_Min()
                        || v3Global.چوڑائی_Min_Usage() != VWidthMinUsage::MATCHES_چوڑائی,
                    nodep, "Width != WidthMin");
    }

    خالی processEnter(Ast_گرہ* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ثابت حرف* ثابت whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        اگر (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        اگر (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_مقصد_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_مقصد_Hex(nodep->dtypep()));
        }
        اگر (v3Global.assertDTypesResolved()) {
            اگر (nodep->hasDType()) {
            } ورنہ {
                UASSERT_OBJ(!VN_IS(nodep, گرہ_اظہاریہ), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_بچہ_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            اگر (ثابت Ast_گرہ_DType* ثابت dnodep = VN_CAST(nodep, گرہ_DType))
                check_چوڑائی_Min(dnodep);
        }
        check_چوڑائی_Min(nodep);
    }
    خالی processExit(Ast_گرہ* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    خالی processAndIterate(Ast_گرہ* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    خالی processAndIterateList(Ast_گرہ* nodep) {
        جب (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    خالی ڈالو_Local_Scope() {
        اگر (m_cfuncp) m_locals_ڈھیر.emplace_عقب();
    }
    خالی نکالو_Local_Scope() {
        اگر (m_cfuncp) m_locals_ڈھیر.نکالو_عقب();
    }
    منطقی is_اندر_Scope_Local(ثابت AstVar* varp) ثابت {
        برائے (ثابت خودکار& سیٹ : m_locals_ڈھیر) {
            اگر (سیٹ.گنتی(varp)) واپس سچ;
        }
        واپس جھوٹ;
    }
    ساکن خالی checkArgRefs(Ast_گرہ_اظہاریہ* nodep, ثابت حرف* descrp, Ast_گرہ_اظہاریہ* argsp) {
        اگر (!std::strcmp(descrp, "TODO")) واپس;  // Skip if not yet checked

        // Check each argument
        ثابت حرف* dp = descrp;
        برائے (Ast_گرہ_اظہاریہ* argp = argsp; argp; argp = VN_AS(argp->nextp(), گرہ_اظہاریہ)) {
            اگر (argp->fileline()->erroringOn()) واپس;  // Intentionally skip all checks
            ثابت Ast_گرہ_اظہاریہ* ثابت lvalp = argp->get_VAccess_ہدف_Recurse();
            ثابت VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            اگر (dp[0] == '+') --dp;  // Repeats the entry before it
            انتخاب (dp[0]) {
            حالت 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                توڑو;
            حالت 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                توڑو;
            حالت 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                توڑو;
            طےشدہ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                توڑو;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ثابت سائز_t nWriteRefs = m_nWriteRefs;
        ثابت سائز_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        اگر (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گرہ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گرہ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    خالی دورہ(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), گرہ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), گرہ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_اندر_Scope);
        m_اندر_Scope = سچ;
        VL_RESTORER_CLEAR(m_c_Func_نام);
        processAndIterate(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_نام);
        processAndIterate(nodep);
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_نام);
        processAndIterate(nodep);
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_اندر_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        اگر (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            اگر (ثابت AstVar* ثابت varp = nodep->varp()) {
                اگر (!is_اندر_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        اگر (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->طریقہ().دلائل(), nodep->pinsp());
    }
    خالی دورہ(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().دلائل(), nodep->pinsp());
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    خالی دورہ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_ڈھیر.clear();
        ڈالو_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_دلائل = nodep->name();
        اگر (!nodep->argTypes().خالی_خانے()) name_دلائل += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_نام.emplace(name_دلائل).دوسرا, nodep,
                    "Duplicate cfunc name: '" << name_دلائل << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        برائے (ثابت خودکار& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.گنتی(pair.پہلا) == 0, pair.دوسرا,
                        "Local variable not in scope where referenced: " << pair.پہلا);
        }
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        // Each branch is a separate local variable scope
        ڈالو_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        اگر (Ast_گرہ* ثابت thensp = nodep->thensp()) {
            ڈالو_Local_Scope();
            processAndIterateList(thensp);
            نکالو_Local_Scope();
        }
        اگر (Ast_گرہ* ثابت elsesp = nodep->elsesp()) {
            ڈالو_Local_Scope();
            processAndIterateList(elsesp);
            نکالو_Local_Scope();
        }
        processExit(nodep);
        نکالو_Local_Scope();
    }
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        ڈالو_Local_Scope();
        processAndIterate(nodep);
        نکالو_Local_Scope();
    }
    خالی دورہ(AstVar* nodep) override {
        processAndIterate(nodep);
        اگر (m_cfuncp) {
            m_localVars.داخل_کرو(nodep);
            m_locals_ڈھیر.عقب().داخل_کرو(nodep);
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = طےشدہ;
};

//######################################################################
// Broken check entry point

خالی V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    ساکن منطقی s_اندر_Broken = جھوٹ;
    اگر (VL_UNCOVERABLE(s_اندر_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } ورنہ {
        s_اندر_Broken = سچ;

        ثابت V3_تالا_Guard تالا_2{s_alloc_جدول.m_mutex};

        // Mark every node in the tree
        ثابت uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_گرہ* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#اگر_تعریف_شدہ VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_جدول.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#اگر_ختم
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            اگر (nodep->maybe_Pointed_مقصد()) s_linkable_جدول.جوڑو_Linkable(nodep);
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
        s_اندر_Broken = جھوٹ;
    }
}

خالی V3Broken::allowMidvisitorCheck(منطقی flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

خالی V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_سطر* ثابت fl = نیا File_سطر{File_سطر::command_سطر_Filename()};
    Ast_گرہ* ثابت newp = نیا AstBegin{fl, "[EditWrapper]", خالی_اشاریہ, جھوٹ};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#اگر_غیر_تعریف_شدہ VL_LEAK_CHECKS
    جوڑو_Newed(newp);
    deleted(newp);
#اگر_ختم
    VL_DO_DANGLING(newp->delete_درخت(), newp);
}
