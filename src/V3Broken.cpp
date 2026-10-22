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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Broken.h"

#अन्तर्भाव "V3ConstPool.h"

#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

स्थिर वर्ग BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    स्थिर constexpr uint8_t MIN_मान = 1;
    स्थिर constexpr uint8_t MAX_मान = 127;

    uint8_t m_गन्ती = MIN_मान;

सार्वजनिक:
    uint8_t get() अचर VL_MT_SAFE {
        UASSERT(MIN_मान <= m_गन्ती && m_गन्ती <= MAX_मान, "Invalid generation number");
        फर्काऊ m_गन्ती;
    }

    शून्य inc() {
        ++m_गन्ती;
        यदि (m_गन्ती > MAX_मान) m_गन्ती = MIN_मान;
    }
} s_brokenCntGlobal;

स्थिर तार्किक s_brokenAllowMidvisitorCheck = असत्य;

//######################################################################
// Table of allocated AstNode pointers

स्थिर वर्ग AllocTable final {
    मित्र वर्ग V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<अचर Ast_गाँठो*> m_allocated VL_GUARDED_BY(m_mutex);

सार्वजनिक:
    // METHODS
    शून्य addNewed(अचर Ast_गाँठो* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        अचर V3LockGuard lock{m_mutex};
        यदि (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    शून्य deleted(अचर Ast_गाँठो* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        अचर V3LockGuard lock{m_mutex};
        यदि (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

गुप्त:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    तार्किक isAllocated(अचर Ast_गाँठो* nodep) अचर VL_REQUIRES(m_mutex) {
        फर्काऊ m_allocated.गन्ती(nodep) != 0;
    }
    शून्य checkForLeaks() VL_REQUIRES(m_mutex) {
        यदि (!v3Global.opt.debugCheck()) फर्काऊ;

        अचर uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        लागि (अचर तार्किक withBack : {असत्य, सत्य}) {
            लागि (अचर Ast_गाँठो* अचर nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                यदि (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    अचर तार्किक hasBack = nodep->backp() != nullptr;
                    यदि (hasBack != withBack) जारी;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_गाँठो::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

शून्य V3Broken::addNewed(अचर Ast_गाँठो* nodep) { s_allocTable.addNewed(nodep); }
शून्य V3Broken::deleted(अचर Ast_गाँठो* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

स्थिर वर्ग LinkableTable final {
    // MEMBERS
    std::unordered_set<अचर Ast_गाँठो*> m_linkable;  // Set of all nodes allocated but not freed

सार्वजनिक:
    // METHODS
    शून्य clear() { m_linkable.clear(); }
    शून्य addLinkable(अचर Ast_गाँठो* nodep) { m_linkable.emplace(nodep); }
    तार्किक isLinkable(अचर Ast_गाँठो* nodep) अचर { फर्काऊ m_linkable.गन्ती(nodep) != 0; }
} s_linkableTable;

तार्किक V3Broken::isLinkable(अचर Ast_गाँठो* nodep) { फर्काऊ s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

वर्ग BrokenCheckVisitor final : सार्वजनिक VNVisitorConst {
    // Constants for marking we are under/not under a node
    अचर uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    अचर uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<अचर AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<अचर AstVar*, अचर Ast_गाँठो_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<अचर AstVar*>> m_locals_थाक;
    // Number of write references encountered
    माप_प्रकार m_nWriteRefs = 0;
    // Number of function calls encountered
    माप_प्रकार m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    अचर AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    तार्किक m_inScope = असत्य;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

गुप्त:
    // METHODS
    स्थिर शून्य checkWidthMin(अचर Ast_गाँठो* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    शून्य processEnter(Ast_गाँठो* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        अचर वर्ण* अचर whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        यदि (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        यदि (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        यदि (v3Global.assertDTypesResolved()) {
            यदि (nodep->hasDType()) {
            } अन्यथा {
                UASSERT_OBJ(!VN_IS(nodep, गाँठो_अभिव्यक्ति), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            यदि (अचर Ast_गाँठो_DType* अचर dnodep = VN_CAST(nodep, गाँठो_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    शून्य processExit(Ast_गाँठो* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    शून्य processAndIterate(Ast_गाँठो* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    शून्य processAndIterateList(Ast_गाँठो* nodep) {
        जबसम्म (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    शून्य थप_Local_Scope() {
        यदि (m_cfuncp) m_locals_थाक.emplace_back();
    }
    शून्य झिक_Local_Scope() {
        यदि (m_cfuncp) m_locals_थाक.झिक_back();
    }
    तार्किक isInScopeLocal(अचर AstVar* varp) अचर {
        लागि (अचर स्वतः& set : m_locals_थाक) {
            यदि (set.गन्ती(varp)) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }
    स्थिर शून्य checkArgRefs(Ast_गाँठो_अभिव्यक्ति* nodep, अचर वर्ण* descrp, Ast_गाँठो_अभिव्यक्ति* argsp) {
        यदि (!std::म_भेद(descrp, "TODO")) फर्काऊ;  // Skip if not yet checked

        // Check each argument
        अचर वर्ण* dp = descrp;
        लागि (Ast_गाँठो_अभिव्यक्ति* argp = argsp; argp; argp = VN_AS(argp->nextp(), गाँठो_अभिव्यक्ति)) {
            यदि (argp->fileline()->erroringOn()) फर्काऊ;  // Intentionally skip all checks
            अचर Ast_गाँठो_अभिव्यक्ति* अचर lvalp = argp->getVAccessTargetRecurse();
            अचर VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            यदि (dp[0] == '+') --dp;  // Repeats the entry before it
            विकल्प (dp[0]) {
            पक्ष 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                रोक;
            पक्ष 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                रोक;
            पक्ष 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                रोक;
            स्वभाव:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                रोक;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        अचर माप_प्रकार nWriteRefs = m_nWriteRefs;
        अचर माप_प्रकार nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        यदि (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), गाँठो_Var_Ref)
                      && !VN_AS(nodep->lhsp(), गाँठो_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    शून्य visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), गाँठो_Var_Ref)
                      && !VN_AS(nodep->lhsp(), गाँठो_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = सत्य;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    शून्य visit(Ast_गाँठो_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        यदि (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            यदि (अचर AstVar* अचर varp = nodep->varp()) {
                यदि (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        यदि (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    शून्य visit(Ast_गाँठो_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    शून्य visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_थाक.clear();
        थप_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        यदि (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        लागि (अचर स्वतः& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.गन्ती(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    शून्य visit(Ast_गाँठो_If* nodep) override {
        // Each branch is a separate local variable scope
        थप_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        यदि (Ast_गाँठो* अचर thensp = nodep->thensp()) {
            थप_Local_Scope();
            processAndIterateList(thensp);
            झिक_Local_Scope();
        }
        यदि (Ast_गाँठो* अचर elsesp = nodep->elsesp()) {
            थप_Local_Scope();
            processAndIterateList(elsesp);
            झिक_Local_Scope();
        }
        processExit(nodep);
        झिक_Local_Scope();
    }
    शून्य visit(Ast_गाँठो_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        थप_Local_Scope();
        processAndIterate(nodep);
        झिक_Local_Scope();
    }
    शून्य visit(AstVar* nodep) override {
        processAndIterate(nodep);
        यदि (m_cfuncp) {
            m_localVars.घुसाउ(nodep);
            m_locals_थाक.back().घुसाउ(nodep);
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = स्वभाव;
};

//######################################################################
// Broken check entry point

शून्य V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    स्थिर तार्किक s_inBroken = असत्य;
    यदि (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } अन्यथा {
        s_inBroken = सत्य;

        अचर V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        अचर uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_गाँठो* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            यदि (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        अचर BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        अचर वर्ण* अचर whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = असत्य;
    }
}

शून्य V3Broken::allowMidvisitorCheck(तार्किक flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

शून्य V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_पङ्क्ति* अचर fl = नूतन File_पङ्क्ति{File_पङ्क्ति::command_पङ्क्ति_Filename()};
    Ast_गाँठो* अचर newp = नूतन AstBegin{fl, "[EditWrapper]", nullptr, असत्य};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_रूख(), newp);
}
