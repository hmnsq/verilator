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
    स्थिर constexpr uint8_t MIN_मानम् = 1;
    स्थिर constexpr uint8_t MAX_मानम् = 127;

    uint8_t m_गणनम् = MIN_मानम्;

सार्वजनिक:
    uint8_t get() ध्रुव VL_MT_SAFE {
        UASSERT(MIN_मानम् <= m_गणनम् && m_गणनम् <= MAX_मानम्, "Invalid generation number");
        प्रत्यागम m_गणनम्;
    }

    शून्य inc() {
        ++m_गणनम्;
        यदि (m_गणनम् > MAX_मानम्) m_गणनम् = MIN_मानम्;
    }
} s_brokenCntGlobal;

स्थिर तार्किक s_brokenAllowMidvisitorCheck = असत्य;

//######################################################################
// Table of allocated AstNode pointers

स्थिर वर्ग Alloc_सारणी final {
    मित्रम् वर्ग V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_समुच्चयः<ध्रुव Ast_ग्रन्थिः*> m_allocated VL_GUARDED_BY(m_mutex);

सार्वजनिक:
    // METHODS
    शून्य सङ्कलय_Newed(ध्रुव Ast_ग्रन्थिः* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        यदि (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    शून्य deleted(ध्रुव Ast_ग्रन्थिः* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        यदि (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

गुप्त:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    तार्किक isAllocated(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव VL_REQUIRES(m_mutex) {
        प्रत्यागम m_allocated.गणनम्(nodep) != 0;
    }
    शून्य checkForLeaks() VL_REQUIRES(m_mutex) {
        यदि (!v3Global.opt.debugCheck()) प्रत्यागम;

        ध्रुव uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        प्रति (ध्रुव तार्किक with_पृष्ठभागः : {असत्य, सत्य}) {
            प्रति (ध्रुव Ast_ग्रन्थिः* ध्रुव nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                यदि (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    ध्रुव तार्किक has_पृष्ठभागः = nodep->backp() != शून्यसूचकः;
                    यदि (has_पृष्ठभागः != with_पृष्ठभागः) अनुवर्त;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //सततम्
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_पृष्ठभागः ? " with back pointer: " : ": ");
                    nodep->Ast_ग्रन्थिः::dump(std::cerr);
                    std::cerr << पङ्क्त्यन्तः;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_सारणी;

शून्य V3Broken::सङ्कलय_Newed(ध्रुव Ast_ग्रन्थिः* nodep) { s_alloc_सारणी.सङ्कलय_Newed(nodep); }
शून्य V3Broken::deleted(ध्रुव Ast_ग्रन्थिः* nodep) { s_alloc_सारणी.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

स्थिर वर्ग Linkable_सारणी final {
    // MEMBERS
    std::unordered_समुच्चयः<ध्रुव Ast_ग्रन्थिः*> m_linkable;  // Set of all nodes allocated but not freed

सार्वजनिक:
    // METHODS
    शून्य clear() { m_linkable.clear(); }
    शून्य सङ्कलय_Linkable(ध्रुव Ast_ग्रन्थिः* nodep) { m_linkable.emplace(nodep); }
    तार्किक isLinkable(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव { प्रत्यागम m_linkable.गणनम्(nodep) != 0; }
} s_linkable_सारणी;

तार्किक V3Broken::isLinkable(ध्रुव Ast_ग्रन्थिः* nodep) { प्रत्यागम s_linkable_सारणी.isLinkable(nodep); }

//######################################################################
// Check every node in tree

वर्ग BrokenCheckVisitor final : सार्वजनिक VNVisitorConst {
    // Constants for marking we are under/not under a node
    ध्रुव uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    ध्रुव uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::समुच्चयः<ध्रुव AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<ध्रुव AstVar*, ध्रुव Ast_ग्रन्थिः_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::सदिशम्<std::unordered_समुच्चयः<ध्रुव AstVar*>> m_locals_राशिः;
    // Number of write references encountered
    आकार_t m_nWriteRefs = 0;
    // Number of function calls encountered
    आकार_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव AstCFunc* m_cfuncp = शून्यसूचकः;  // Current CFunc, if any
    तार्किक m_अन्तर्_Scope = असत्य;  // Under AstScope
    std::समुच्चयः<std::string> m_c_Func_नामानि;  // CFunc by name in current class/module

गुप्त:
    // METHODS
    स्थिर शून्य check_विस्तारः_Min(ध्रुव Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(nodep->विस्तारः() == nodep->विस्तारः_Min()
                        || v3Global.विस्तारः_Min_Usage() != VWidthMinUsage::MATCHES_विस्तारः,
                    nodep, "Width != WidthMin");
    }

    शून्य processEnter(Ast_ग्रन्थिः* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        ध्रुव वर्ण* ध्रुव whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        यदि (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        यदि (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_गन्तव्यम्_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_गन्तव्यम्_Hex(nodep->dtypep()));
        }
        यदि (v3Global.assertDTypesResolved()) {
            यदि (nodep->hasDType()) {
            } अन्यथा {
                UASSERT_OBJ(!VN_IS(nodep, ग्रन्थिः_व्यञ्जकम्), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dnodep = VN_CAST(nodep, ग्रन्थिः_DType))
                check_विस्तारः_Min(dnodep);
        }
        check_विस्तारः_Min(nodep);
    }
    शून्य processExit(Ast_ग्रन्थिः* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    शून्य processAndIterate(Ast_ग्रन्थिः* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    शून्य processAndIterateList(Ast_ग्रन्थिः* nodep) {
        यावत् (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    शून्य प्रक्षिप_Local_Scope() {
        यदि (m_cfuncp) m_locals_राशिः.emplace_पृष्ठभागः();
    }
    शून्य निष्कासय_Local_Scope() {
        यदि (m_cfuncp) m_locals_राशिः.निष्कासय_पृष्ठभागः();
    }
    तार्किक is_अन्तर्_Scope_Local(ध्रुव AstVar* varp) ध्रुव {
        प्रति (ध्रुव स्वचालित& समुच्चयः : m_locals_राशिः) {
            यदि (समुच्चयः.गणनम्(varp)) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    स्थिर शून्य checkArgRefs(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, ध्रुव वर्ण* descrp, Ast_ग्रन्थिः_व्यञ्जकम्* argsp) {
        यदि (!std::वाक्यं_तुलय(descrp, "TODO")) प्रत्यागम;  // Skip if not yet checked

        // Check each argument
        ध्रुव वर्ण* dp = descrp;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* argp = argsp; argp; argp = VN_AS(argp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
            यदि (argp->fileline()->erroringOn()) प्रत्यागम;  // Intentionally skip all checks
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lvalp = argp->getVAccessTargetRecurse();
            ध्रुव VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            यदि (dp[0] == '+') --dp;  // Repeats the entry before it
            विकल्प (dp[0]) {
            पक्ष 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                भङ्ग;
            पक्ष 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                भङ्ग;
            पक्ष 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                भङ्ग;
            स्वभाव:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                भङ्ग;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        ध्रुव आकार_t nWriteRefs = m_nWriteRefs;
        ध्रुव आकार_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        यदि (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ग्रन्थिः_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ग्रन्थिः_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    शून्य दर्शय(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), ग्रन्थिः_Var_Ref)
                      && !VN_AS(nodep->lhsp(), ग्रन्थिः_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    शून्य दर्शय(AstScope* nodep) override {
        VL_RESTORER(m_अन्तर्_Scope);
        m_अन्तर्_Scope = सत्य;
        VL_RESTORER_CLEAR(m_c_Func_नामानि);
        processAndIterate(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_नामानि);
        processAndIterate(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_नामानि);
        processAndIterate(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_अन्तर्_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        यदि (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            यदि (ध्रुव AstVar* ध्रुव varp = nodep->varp()) {
                यदि (!is_अन्तर्_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        यदि (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->विधानम्().तर्काः(), nodep->pinsp());
    }
    शून्य दर्शय(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().तर्काः(), nodep->pinsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_राशिः.clear();
        प्रक्षिप_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_तर्काः = nodep->name();
        यदि (!nodep->argTypes().रिक्तानि()) name_तर्काः += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_नामानि.emplace(name_तर्काः).second, nodep,
                    "Duplicate cfunc name: '" << name_तर्काः << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        प्रति (ध्रुव स्वचालित& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.गणनम्(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        // Each branch is a separate local variable scope
        प्रक्षिप_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        यदि (Ast_ग्रन्थिः* ध्रुव thensp = nodep->thensp()) {
            प्रक्षिप_Local_Scope();
            processAndIterateList(thensp);
            निष्कासय_Local_Scope();
        }
        यदि (Ast_ग्रन्थिः* ध्रुव elsesp = nodep->elsesp()) {
            प्रक्षिप_Local_Scope();
            processAndIterateList(elsesp);
            निष्कासय_Local_Scope();
        }
        processExit(nodep);
        निष्कासय_Local_Scope();
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        प्रक्षिप_Local_Scope();
        processAndIterate(nodep);
        निष्कासय_Local_Scope();
    }
    शून्य दर्शय(AstVar* nodep) override {
        processAndIterate(nodep);
        यदि (m_cfuncp) {
            m_localVars.निवेशय(nodep);
            m_locals_राशिः.पृष्ठभागः().निवेशय(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = स्वभाव;
};

//######################################################################
// Broken check entry point

शून्य V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    स्थिर तार्किक s_अन्तर्_Broken = असत्य;
    यदि (VL_UNCOVERABLE(s_अन्तर्_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } अन्यथा {
        s_अन्तर्_Broken = सत्य;

        ध्रुव V3_तालकम्_Guard तालकम्_2{s_alloc_सारणी.m_mutex};

        // Mark every node in the tree
        ध्रुव uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_ग्रन्थिः* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#यदि_परिभाषितम् VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_सारणी.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#यदि_अन्तः
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            यदि (nodep->maybe_Pointed_गन्तव्यम्()) s_linkable_सारणी.सङ्कलय_Linkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        ध्रुव BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        ध्रुव वर्ण* ध्रुव whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_alloc_सारणी.checkForLeaks();
        s_linkable_सारणी.clear();
        s_brokenCntGlobal.inc();
        s_अन्तर्_Broken = असत्य;
    }
}

शून्य V3Broken::allowMidvisitorCheck(तार्किक flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

शून्य V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_पङ्क्तिः* ध्रुव fl = नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()};
    Ast_ग्रन्थिः* ध्रुव newp = नूतन AstBegin{fl, "[EditWrapper]", शून्यसूचकः, असत्य};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#यदि_अपरिभाषितम् VL_LEAK_CHECKS
    सङ्कलय_Newed(newp);
    deleted(newp);
#यदि_अन्तः
    VL_DO_DANGLING(newp->delete_वृक्षः(), newp);
}
