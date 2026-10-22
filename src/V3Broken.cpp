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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Broken.h"

#അന്തര്ഭാവ "V3ConstPool.h"

#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

സ്ഥിതം വർഗ്ഗം BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    സ്ഥിതം constexpr uint8_t MIN_മൂല്യം = 1;
    സ്ഥിതം constexpr uint8_t MAX_മൂല്യം = 127;

    uint8_t m_എണ്ണം = MIN_മൂല്യം;

പൊതു:
    uint8_t get() സ്ഥിരം VL_MT_SAFE {
        UASSERT(MIN_മൂല്യം <= m_എണ്ണം && m_എണ്ണം <= MAX_മൂല്യം, "Invalid generation number");
        തിരികെ m_എണ്ണം;
    }

    ശൂന്യം inc() {
        ++m_എണ്ണം;
        എങ്കിൽ (m_എണ്ണം > MAX_മൂല്യം) m_എണ്ണം = MIN_മൂല്യം;
    }
} s_brokenCntGlobal;

സ്ഥിതം തർക്കം s_brokenAllowMidvisitorCheck = അസത്യം;

//######################################################################
// Table of allocated AstNode pointers

സ്ഥിതം വർഗ്ഗം AllocTable final {
    friend വർഗ്ഗം V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<സ്ഥിരം Ast_കണ്ണി*> m_allocated VL_GUARDED_BY(m_mutex);

പൊതു:
    // METHODS
    ശൂന്യം addNewed(സ്ഥിരം Ast_കണ്ണി* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        സ്ഥിരം V3LockGuard lock{m_mutex};
        എങ്കിൽ (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    ശൂന്യം deleted(സ്ഥിരം Ast_കണ്ണി* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        സ്ഥിരം V3LockGuard lock{m_mutex};
        എങ്കിൽ (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

സ്വകാര്യ:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    തർക്കം isAllocated(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം VL_REQUIRES(m_mutex) {
        തിരികെ m_allocated.എണ്ണം(nodep) != 0;
    }
    ശൂന്യം checkForLeaks() VL_REQUIRES(m_mutex) {
        എങ്കിൽ (!v3Global.opt.debugCheck()) തിരികെ;

        സ്ഥിരം uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        വേണ്ടി (സ്ഥിരം തർക്കം withBack : {അസത്യം, സത്യം}) {
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                എങ്കിൽ (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    സ്ഥിരം തർക്കം hasBack = nodep->backp() != nullptr;
                    എങ്കിൽ (hasBack != withBack) തുടരുക;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_കണ്ണി::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

ശൂന്യം V3Broken::addNewed(സ്ഥിരം Ast_കണ്ണി* nodep) { s_allocTable.addNewed(nodep); }
ശൂന്യം V3Broken::deleted(സ്ഥിരം Ast_കണ്ണി* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

സ്ഥിതം വർഗ്ഗം LinkableTable final {
    // MEMBERS
    std::unordered_set<സ്ഥിരം Ast_കണ്ണി*> m_linkable;  // Set of all nodes allocated but not freed

പൊതു:
    // METHODS
    ശൂന്യം clear() { m_linkable.clear(); }
    ശൂന്യം addLinkable(സ്ഥിരം Ast_കണ്ണി* nodep) { m_linkable.emplace(nodep); }
    തർക്കം isLinkable(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം { തിരികെ m_linkable.എണ്ണം(nodep) != 0; }
} s_linkableTable;

തർക്കം V3Broken::isLinkable(സ്ഥിരം Ast_കണ്ണി* nodep) { തിരികെ s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

വർഗ്ഗം BrokenCheckVisitor final : പൊതു VNVisitorConst {
    // Constants for marking we are under/not under a node
    സ്ഥിരം uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    സ്ഥിരം uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<സ്ഥിരം AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<സ്ഥിരം AstVar*, സ്ഥിരം Ast_കണ്ണി_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<സ്ഥിരം AstVar*>> m_locals_അട്ടി;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    സ്ഥിരം AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    തർക്കം m_inScope = അസത്യം;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

സ്വകാര്യ:
    // METHODS
    സ്ഥിതം ശൂന്യം checkWidthMin(സ്ഥിരം Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    ശൂന്യം processEnter(Ast_കണ്ണി* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        സ്ഥിരം അക്ഷരം* സ്ഥിരം whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        എങ്കിൽ (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        എങ്കിൽ (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        എങ്കിൽ (v3Global.assertDTypesResolved()) {
            എങ്കിൽ (nodep->hasDType()) {
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(!VN_IS(nodep, കണ്ണി_വ്യഞ്ജകം), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dnodep = VN_CAST(nodep, കണ്ണി_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    ശൂന്യം processExit(Ast_കണ്ണി* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    ശൂന്യം processAndIterate(Ast_കണ്ണി* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    ശൂന്യം processAndIterateList(Ast_കണ്ണി* nodep) {
        ആകുമ്പോൾ (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    ശൂന്യം തള്ളുക_Local_Scope() {
        എങ്കിൽ (m_cfuncp) m_locals_അട്ടി.emplace_back();
    }
    ശൂന്യം എടുക്കുക_Local_Scope() {
        എങ്കിൽ (m_cfuncp) m_locals_അട്ടി.എടുക്കുക_back();
    }
    തർക്കം isInScopeLocal(സ്ഥിരം AstVar* varp) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം auto& set : m_locals_അട്ടി) {
            എങ്കിൽ (set.എണ്ണം(varp)) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    സ്ഥിതം ശൂന്യം checkArgRefs(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, സ്ഥിരം അക്ഷരം* descrp, Ast_കണ്ണി_വ്യഞ്ജകം* argsp) {
        എങ്കിൽ (!std::strcmp(descrp, "TODO")) തിരികെ;  // Skip if not yet checked

        // Check each argument
        സ്ഥിരം അക്ഷരം* dp = descrp;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* argp = argsp; argp; argp = VN_AS(argp->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
            എങ്കിൽ (argp->fileline()->erroringOn()) തിരികെ;  // Intentionally skip all checks
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lvalp = argp->getVAccessTargetRecurse();
            സ്ഥിരം VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            എങ്കിൽ (dp[0] == '+') --dp;  // Repeats the entry before it
            തിരഞ്ഞെടുപ്പ് (dp[0]) {
            അവസ്ഥ 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                നിർത്തുക;
            അവസ്ഥ 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                നിർത്തുക;
            അവസ്ഥ 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                നിർത്തുക;
            സ്ഥിരസ്ഥിതി:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                നിർത്തുക;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        സ്ഥിരം size_t nWriteRefs = m_nWriteRefs;
        സ്ഥിരം size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        എങ്കിൽ (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), കണ്ണി_Var_Ref)
                      && !VN_AS(nodep->lhsp(), കണ്ണി_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    ശൂന്യം visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), കണ്ണി_Var_Ref)
                      && !VN_AS(nodep->lhsp(), കണ്ണി_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = സത്യം;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        എങ്കിൽ (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varp()) {
                എങ്കിൽ (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        എങ്കിൽ (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    ശൂന്യം visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_അട്ടി.clear();
        തള്ളുക_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        എങ്കിൽ (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        വേണ്ടി (സ്ഥിരം auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.എണ്ണം(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        // Each branch is a separate local variable scope
        തള്ളുക_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം thensp = nodep->thensp()) {
            തള്ളുക_Local_Scope();
            processAndIterateList(thensp);
            എടുക്കുക_Local_Scope();
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം elsesp = nodep->elsesp()) {
            തള്ളുക_Local_Scope();
            processAndIterateList(elsesp);
            എടുക്കുക_Local_Scope();
        }
        processExit(nodep);
        എടുക്കുക_Local_Scope();
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        തള്ളുക_Local_Scope();
        processAndIterate(nodep);
        എടുക്കുക_Local_Scope();
    }
    ശൂന്യം visit(AstVar* nodep) override {
        processAndIterate(nodep);
        എങ്കിൽ (m_cfuncp) {
            m_localVars.തിരുകുക(nodep);
            m_locals_അട്ടി.back().തിരുകുക(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Broken check entry point

ശൂന്യം V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    സ്ഥിതം തർക്കം s_inBroken = അസത്യം;
    എങ്കിൽ (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } അല്ലെങ്കിൽ {
        s_inBroken = സത്യം;

        സ്ഥിരം V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        സ്ഥിരം uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_കണ്ണി* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            എങ്കിൽ (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        സ്ഥിരം BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        സ്ഥിരം അക്ഷരം* സ്ഥിരം whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = അസത്യം;
    }
}

ശൂന്യം V3Broken::allowMidvisitorCheck(തർക്കം flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

ശൂന്യം V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_വരി* സ്ഥിരം fl = പുതിയ File_വരി{File_വരി::command_വരി_Filename()};
    Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstBegin{fl, "[EditWrapper]", nullptr, അസത്യം};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_വൃക്ഷം(), newp);
}
