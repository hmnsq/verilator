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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Broken.h"

#அந்தர்பாவ "V3ConstPool.h"

#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

நிலையான வகுப்பு BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    நிலையான constexpr uint8_t MIN_மதிப்பு = 1;
    நிலையான constexpr uint8_t MAX_மதிப்பு = 127;

    uint8_t m_மொத்தம் = MIN_மதிப்பு;

பொது:
    uint8_t get() மாறிலி VL_MT_SAFE {
        UASSERT(MIN_மதிப்பு <= m_மொத்தம் && m_மொத்தம் <= MAX_மதிப்பு, "Invalid generation number");
        திரும்பு m_மொத்தம்;
    }

    வெறுமை inc() {
        ++m_மொத்தம்;
        எனில் (m_மொத்தம் > MAX_மதிப்பு) m_மொத்தம் = MIN_மதிப்பு;
    }
} s_brokenCntGlobal;

நிலையான தருக்க s_brokenAllowMidvisitorCheck = பொய்;

//######################################################################
// Table of allocated AstNode pointers

நிலையான வகுப்பு AllocTable final {
    friend வகுப்பு V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_set<மாறிலி Ast_முனை*> m_allocated VL_GUARDED_BY(m_mutex);

பொது:
    // METHODS
    வெறுமை addNewed(மாறிலி Ast_முனை* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        மாறிலி V3LockGuard lock{m_mutex};
        எனில் (VL_UNCOVERABLE(!m_allocated.emplace(nodep).second)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    வெறுமை deleted(மாறிலி Ast_முனை* nodep) VL_MT_SAFE_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        மாறிலி V3LockGuard lock{m_mutex};
        எனில் (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

தனிப்பட்ட:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    தருக்க isAllocated(மாறிலி Ast_முனை* nodep) மாறிலி VL_REQUIRES(m_mutex) {
        திரும்பு m_allocated.மொத்தம்(nodep) != 0;
    }
    வெறுமை checkForLeaks() VL_REQUIRES(m_mutex) {
        எனில் (!v3Global.opt.debugCheck()) திரும்பு;

        மாறிலி uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        க்கு (மாறிலி தருக்க withBack : {பொய், உண்மை}) {
            க்கு (மாறிலி Ast_முனை* மாறிலி nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                எனில் (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    மாறிலி தருக்க hasBack = nodep->backp() != nullptr;
                    எனில் (hasBack != withBack) தொடர்;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //    run
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (withBack ? " with back pointer: " : ": ");
                    nodep->Ast_முனை::dump(std::cerr);
                    std::cerr << endl;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_allocTable;

வெறுமை V3Broken::addNewed(மாறிலி Ast_முனை* nodep) { s_allocTable.addNewed(nodep); }
வெறுமை V3Broken::deleted(மாறிலி Ast_முனை* nodep) { s_allocTable.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

நிலையான வகுப்பு LinkableTable final {
    // MEMBERS
    std::unordered_set<மாறிலி Ast_முனை*> m_linkable;  // Set of all nodes allocated but not freed

பொது:
    // METHODS
    வெறுமை clear() { m_linkable.clear(); }
    வெறுமை addLinkable(மாறிலி Ast_முனை* nodep) { m_linkable.emplace(nodep); }
    தருக்க isLinkable(மாறிலி Ast_முனை* nodep) மாறிலி { திரும்பு m_linkable.மொத்தம்(nodep) != 0; }
} s_linkableTable;

தருக்க V3Broken::isLinkable(மாறிலி Ast_முனை* nodep) { திரும்பு s_linkableTable.isLinkable(nodep); }

//######################################################################
// Check every node in tree

வகுப்பு BrokenCheckVisitor final : பொது VNVisitorConst {
    // Constants for marking we are under/not under a node
    மாறிலி uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    மாறிலி uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::set<மாறிலி AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<மாறிலி AstVar*, மாறிலி Ast_முனை_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::vector<std::unordered_set<மாறிலி AstVar*>> m_locals_அடுக்கு;
    // Number of write references encountered
    size_t m_nWriteRefs = 0;
    // Number of function calls encountered
    size_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    மாறிலி AstCFunc* m_cfuncp = nullptr;  // Current CFunc, if any
    தருக்க m_inScope = பொய்;  // Under AstScope
    std::set<std::string> m_cFuncNames;  // CFunc by name in current class/module

தனிப்பட்ட:
    // METHODS
    நிலையான வெறுமை checkWidthMin(மாறிலி Ast_முனை* nodep) {
        UASSERT_OBJ(nodep->width() == nodep->widthMin()
                        || v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH,
                    nodep, "Width != WidthMin");
    }

    வெறுமை processEnter(Ast_முனை* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        மாறிலி எழுத்து* மாறிலி whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        எனில் (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        எனில் (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvtToHex(nodep->dtypep()));
        }
        எனில் (v3Global.assertDTypesResolved()) {
            எனில் (nodep->hasDType()) {
            } இல்லையெனில் {
                UASSERT_OBJ(!VN_IS(nodep, முனை_கோவை), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->getChildDTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            எனில் (மாறிலி Ast_முனை_DType* மாறிலி dnodep = VN_CAST(nodep, முனை_DType))
                checkWidthMin(dnodep);
        }
        checkWidthMin(nodep);
    }
    வெறுமை processExit(Ast_முனை* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    வெறுமை processAndIterate(Ast_முனை* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    வெறுமை processAndIterateList(Ast_முனை* nodep) {
        போது (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    வெறுமை தள்ளு_Local_Scope() {
        எனில் (m_cfuncp) m_locals_அடுக்கு.emplace_back();
    }
    வெறுமை எடு_Local_Scope() {
        எனில் (m_cfuncp) m_locals_அடுக்கு.எடு_back();
    }
    தருக்க isInScopeLocal(மாறிலி AstVar* varp) மாறிலி {
        க்கு (மாறிலி auto& set : m_locals_அடுக்கு) {
            எனில் (set.மொத்தம்(varp)) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    நிலையான வெறுமை checkArgRefs(Ast_முனை_கோவை* nodep, மாறிலி எழுத்து* descrp, Ast_முனை_கோவை* argsp) {
        எனில் (!std::strcmp(descrp, "TODO")) திரும்பு;  // Skip if not yet checked

        // Check each argument
        மாறிலி எழுத்து* dp = descrp;
        க்கு (Ast_முனை_கோவை* argp = argsp; argp; argp = VN_AS(argp->nextp(), முனை_கோவை)) {
            எனில் (argp->fileline()->erroringOn()) திரும்பு;  // Intentionally skip all checks
            மாறிலி Ast_முனை_கோவை* மாறிலி lvalp = argp->getVAccessTargetRecurse();
            மாறிலி VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            எனில் (dp[0] == '+') --dp;  // Repeats the entry before it
            தேர்வு (dp[0]) {
            நிலை 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                நிறுத்து;
            நிலை 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                நிறுத்து;
            நிலை 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                நிறுத்து;
            இயல்பு:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                நிறுத்து;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        மாறிலி size_t nWriteRefs = m_nWriteRefs;
        மாறிலி size_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        எனில் (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), முனை_Var_Ref)
                      && !VN_AS(nodep->lhsp(), முனை_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    வெறுமை visit(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), முனை_Var_Ref)
                      && !VN_AS(nodep->lhsp(), முனை_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_inScope);
        m_inScope = உண்மை;
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_cFuncNames);
        processAndIterate(nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_inScope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        எனில் (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            எனில் (மாறிலி AstVar* மாறிலி varp = nodep->varp()) {
                எனில் (!isInScopeLocal(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        எனில் (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->method().args(), nodep->pinsp());
    }
    வெறுமை visit(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().args(), nodep->pinsp());
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_அடுக்கு.clear();
        தள்ளு_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string nameArgs = nodep->name();
        எனில் (!nodep->argTypes().empty()) nameArgs += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_cFuncNames.emplace(nameArgs).second, nodep,
                    "Duplicate cfunc name: '" << nameArgs << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        க்கு (மாறிலி auto& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.மொத்தம்(pair.first) == 0, pair.second,
                        "Local variable not in scope where referenced: " << pair.first);
        }
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        // Each branch is a separate local variable scope
        தள்ளு_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        எனில் (Ast_முனை* மாறிலி thensp = nodep->thensp()) {
            தள்ளு_Local_Scope();
            processAndIterateList(thensp);
            எடு_Local_Scope();
        }
        எனில் (Ast_முனை* மாறிலி elsesp = nodep->elsesp()) {
            தள்ளு_Local_Scope();
            processAndIterateList(elsesp);
            எடு_Local_Scope();
        }
        processExit(nodep);
        எடு_Local_Scope();
    }
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        தள்ளு_Local_Scope();
        processAndIterate(nodep);
        எடு_Local_Scope();
    }
    வெறுமை visit(AstVar* nodep) override {
        processAndIterate(nodep);
        எனில் (m_cfuncp) {
            m_localVars.செருகு(nodep);
            m_locals_அடுக்கு.back().செருகு(nodep);
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = இயல்பு;
};

//######################################################################
// Broken check entry point

வெறுமை V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    நிலையான தருக்க s_inBroken = பொய்;
    எனில் (VL_UNCOVERABLE(s_inBroken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } இல்லையெனில் {
        s_inBroken = உண்மை;

        மாறிலி V3LockGuard lock{s_allocTable.m_mutex};

        // Mark every node in the tree
        மாறிலி uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_முனை* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#ifdef VL_LEAK_CHECKS
            UASSERT_OBJ(s_allocTable.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#endif
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            எனில் (nodep->maybePointedTo()) s_linkableTable.addLinkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        மாறிலி BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        மாறிலி எழுத்து* மாறிலி whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_allocTable.checkForLeaks();
        s_linkableTable.clear();
        s_brokenCntGlobal.inc();
        s_inBroken = பொய்;
    }
}

வெறுமை V3Broken::allowMidvisitorCheck(தருக்க flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

வெறுமை V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_வரி* மாறிலி fl = புதிய File_வரி{File_வரி::command_வரி_Filename()};
    Ast_முனை* மாறிலி newp = புதிய AstBegin{fl, "[EditWrapper]", nullptr, பொய்};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#ifndef VL_LEAK_CHECKS
    addNewed(newp);
    deleted(newp);
#endif
    VL_DO_DANGLING(newp->delete_மரம்(), newp);
}
