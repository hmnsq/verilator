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

#समावेश "V3PchAstMT.h"

#समावेश "V3Broken.h"

#समावेश "V3ConstPool.h"

#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

अटल श्रेणी BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    अटल constexpr uint8_t MIN_मूल्य = 1;
    अटल constexpr uint8_t MAX_मूल्य = 127;

    uint8_t m_गिनती = MIN_मूल्य;

खुला:
    uint8_t लो() स्थिर VL_MT_SAFE {
        UASSERT(MIN_मूल्य <= m_गिनती && m_गिनती <= MAX_मूल्य, "Invalid generation number");
        वापस m_गिनती;
    }

    व्योम inc() {
        ++m_गिनती;
        अगर (m_गिनती > MAX_मूल्य) m_गिनती = MIN_मूल्य;
    }
} s_brokenCntGlobal;

अटल bool s_brokenAllowMidvisitorCheck = false;

//######################################################################
// Table of allocated AstNode pointers

अटल श्रेणी AllocTable final {
    मित्र श्रेणी V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<स्थिर Ast_गाँठ*> m_allocated VL_GUARDED_BY(m_mutex);

खुला:
    // METHODS
    व्योम addNewed(स्थिर Ast_गाँठ* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        स्थिर V3LockGuard lock{m_mutex};
        अगर (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    व्योम deleted(स्थिर Ast_गाँठ* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        स्थिर V3LockGuard lock{m_mutex};
        अगर (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

निजी:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    bool isAllocated(स्थिर Ast_गाँठ* nodep) स्थिर VL_REQUIRES(m_mutex) {
        वापस m_allocated.गिनती(nodep) != 0;
    }
    व्योम checkForLeaks() VL_REQUIRES(m_mutex) {
        अगर (!v3Global.opt.debugCheck()) वापस;

        स्थिर uint8_t brokenCntCurrent = s_brokenCntGlobal.लो();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        क्रम (स्थिर bool withBack : {false, true}) {
            क्रम (स्थिर Ast_गाँठ* स्थिर nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                अगर (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    स्थिर bool hasBack = nodep->backp() != nullptr;
                    अगर (hasBack != withBack) जारी;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::अ_त्रुटि << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_गाँठ::dump(std::अ_त्रुटि);
                    std::अ_त्रुटि << प_पूर्ण;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

व्योम V3Broken::addNewed(स्थिर Ast_गाँठ* nodep) { s_allocTable.addNewed(nodep); }
व्योम V3Broken::deleted(स्थिर Ast_गाँठ* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

अटल श्रेणी LinkableTable final {
    // MEMBERS
    std::unordered_set<स्थिर Ast_गाँठ*> m_linkable;  // Set of all nodes allocated but not freed

खुला:
    // METHODS
    व्योम सफा() { m_linkable.सफा(); }
    व्योम addLinkable(स्थिर Ast_गाँठ* nodep) { m_linkable.emplace(nodep); }
    bool isLinkable(स्थिर Ast_गाँठ* nodep) स्थिर { वापस m_linkable.गिनती(nodep) != 0; }
} s_linkableTable;

bool V3Broken::isLinkable(स्थिर Ast_गाँठ* nodep) { वापस s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

श्रेणी BrokenCheckVisitor final : खुला VNVisitorConst {
    // Constants for marking we are under/not under a node
    स्थिर uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.लो();  // Top bit is clear
    स्थिर uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<स्थिर AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<स्थिर AstVar*, स्थिर Ast_गाँठ_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<स्थिर AstVar*>> m_locals_ढेर;
    // Number of write references encountered
    माप_प्रकार m_nWriteRefs = 0;
    // Number of function calls encountered
    माप_प्रकार m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    स्थिर AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    bool m_inScope = false;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

निजी:
    // METHODS
    अटल व्योम checkWidthMin(स्थिर Ast_गाँठ* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    व्योम processEnter(Ast_गाँठ* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        स्थिर अक्षर* स्थिर whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        अगर (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        अगर (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        अगर (v3Global.assertDTypesResolved()) {
            अगर (nodep->hasDType()) {
            } अन्यथा {
                UASSERT_OBJ(!VN_IS(nodep, गाँठ_व्यंजक), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            अगर (स्थिर Ast_गाँठ_DType* स्थिर dnodep = VN_CAST(nodep, गाँठ_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    व्योम processExit(Ast_गाँठ* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    व्योम processAndIterate(Ast_गाँठ* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    व्योम processAndIterateList(Ast_गाँठ* nodep) {
        जबतक (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    व्योम धकेलो_Local_Scope() {
        अगर (m_cfuncp) m_locals_ढेर.emplace_back();
    }
    व्योम निकालो_Local_Scope() {
        अगर (m_cfuncp) m_locals_ढेर.निकालो_back();
    }
    bool isInScopeLocal(स्थिर AstVar* varp) स्थिर {
        क्रम (स्थिर स्वतः& set : m_locals_ढेर) {
            अगर (set.गिनती(varp)) वापस true;
        }
        वापस false;
    }
    अटल व्योम checkArgRefs(Ast_गाँठ_व्यंजक* nodep, स्थिर अक्षर* descrp, Ast_गाँठ_व्यंजक* argsp) {
        अगर (!std::म_भेद(descrp, "TODO")) वापस;  // Skip if not yet checked

        // Check each argument
        स्थिर अक्षर* dp = descrp;
        क्रम (Ast_गाँठ_व्यंजक* argp = argsp; argp; argp = VN_AS(argp->nextp(), गाँठ_व्यंजक)) {
            अगर (argp->fileline()->erroringOn()) वापस;  // Intentionally skip all checks
            स्थिर Ast_गाँठ_व्यंजक* स्थिर lvalp = argp->getVAccessTargetRecurse();
            स्थिर VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            अगर (dp[0] == '+') --dp;  // Repeats the entry before it
            चयन (dp[0]) {
            हाल 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                अवरोध;
            हाल 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                अवरोध;
            हाल 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                अवरोध;
            शेष:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                अवरोध;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        स्थिर माप_प्रकार nWriteRefs = m_nWriteRefs;
        स्थिर माप_प्रकार nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        अगर (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), गाँठ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), गाँठ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    व्योम visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), गाँठ_Var_Ref)
                      && !VN_AS(nodep->lhsp(), गाँठ_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = true;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        अगर (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            अगर (स्थिर AstVar* स्थिर varp = nodep->varp()) {
                अगर (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        अगर (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    व्योम visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    व्योम visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    व्योम visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.सफा();
        m_suspectRefs.सफा();
        m_locals_ढेर.सफा();
        धकेलो_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        अगर (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        क्रम (स्थिर स्वतः& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.गिनती(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        // Each branch is a separate local variable scope
        धकेलो_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        अगर (Ast_गाँठ* स्थिर thensp = nodep->thensp()) {
            धकेलो_Local_Scope();
            processAndIterateList(thensp);
            निकालो_Local_Scope();
        }
        अगर (Ast_गाँठ* स्थिर elsesp = nodep->elsesp()) {
            धकेलो_Local_Scope();
            processAndIterateList(elsesp);
            निकालो_Local_Scope();
        }
        processExit(nodep);
        निकालो_Local_Scope();
    }
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        धकेलो_Local_Scope();
        processAndIterate(nodep);
        निकालो_Local_Scope();
    }
    व्योम visit(AstVar* nodep) override {
        processAndIterate(nodep);
        अगर (m_cfuncp) {
            m_localVars.जोड़ो(nodep);
            m_locals_ढेर.back().जोड़ो(nodep);
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = शेष;
};

//######################################################################
// Broken check entry point

व्योम V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    अटल bool s_inBroken = false;
    अगर (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } अन्यथा {
        s_inBroken = true;

        स्थिर V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        स्थिर uint8_t brokenCntCurrent = s_brokenCntGlobal.लो();
        nodep->foreach([brokenCntCurrent](Ast_गाँठ* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#अगर_घोषित VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#पूर्ण_अगर
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            अगर (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        स्थिर BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        स्थिर अक्षर* स्थिर whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.सफा();
        s_brokenCntGlobal.inc();
        s_inBroken = false;
    }
}

व्योम V3Broken::allowMidvisitorCheck(bool flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

व्योम V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_पंक्ति* स्थिर fl = नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()};
    Ast_गाँठ* स्थिर newp = नया AstBegin{fl, "[EditWrapper]", nullptr, false};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#अगर_अघोषित VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#पूर्ण_अगर
    VL_DO_DANGLING(newp->delete_वृक्ष(), newp);
}
