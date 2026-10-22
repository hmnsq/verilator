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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Broken.h"

#అన్తర్భావ "V3ConstPool.h"

#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Generation counter for AstNode::m_brokenState

స్థిర తరగతి BrokenCntGlobal {
    // This is a 7 bit generation counter, stored in the bottom 7 bits of AstNode::m_brokenState,
    // used to mark a node as being present under the root AstNetlist in the current traversal. A
    // value 0 is invalid, as the AstNode constructor uses that to initialize m_brokenState
    స్థిర constexpr uint8_t MIN_విలువ = 1;
    స్థిర constexpr uint8_t MAX_విలువ = 127;

    uint8_t m_లెక్క = MIN_విలువ;

బహిరంగ:
    uint8_t get() స్థిరరాశి VL_MT_సురక్షిత {
        UASSERT(MIN_విలువ <= m_లెక్క && m_లెక్క <= MAX_విలువ, "Invalid generation number");
        తిరిగి m_లెక్క;
    }

    శూన్య inc() {
        ++m_లెక్క;
        ఒకవేళ (m_లెక్క > MAX_విలువ) m_లెక్క = MIN_విలువ;
    }
} s_brokenCntGlobal;

స్థిర తార్కిక s_brokenAllowMidvisitorCheck = అబద్ధం;

//######################################################################
// Table of allocated AstNode pointers

స్థిర తరగతి Alloc_పట్టిక final {
    మిత్ర తరగతి V3Broken;
    // MEMBERS
    V3Mutex m_mutex;  // Mutex for m_allocated
    // Set of all nodes allocated but not freed
    std::unordered_సమితి<స్థిరరాశి Ast_కణుపు*> m_allocated VL_GUARDED_BY(m_mutex);

బహిరంగ:
    // METHODS
    శూన్య కూడు_Newed(స్థిరరాశి Ast_కణుపు* nodep) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        // Called by operator new on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        ఒకవేళ (VL_UNCOVERABLE(!m_allocated.emplace(nodep).రెండవ)) {
            nodep->v3fatalSrc("Newing AstNode object that is already allocated");
        }
        // LCOV_EXCL_STOP
    }
    శూన్య deleted(స్థిరరాశి Ast_కణుపు* nodep) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        // Called by operator delete on any node - only if VL_LEAK_CHECKS
        // LCOV_EXCL_START
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        ఒకవేళ (VL_UNCOVERABLE(m_allocated.erase(nodep) == 0)) {
            nodep->v3fatalSrc("Deleting AstNode object that was not allocated or already freed");
        }
        // LCOV_EXCL_STOP
    }

అంతరంగ:  // for V3Broken only
    // cppcheck-suppress unusedPrivateFunction
    తార్కిక isAllocated(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి VL_REQUIRES(m_mutex) {
        తిరిగి m_allocated.లెక్క(nodep) != 0;
    }
    శూన్య checkForLeaks() VL_REQUIRES(m_mutex) {
        ఒకవేళ (!v3Global.opt.debugCheck()) తిరిగి;

        స్థిరరాశి uint8_t brokenCntCurrent = s_brokenCntGlobal.get();

        // Those with backp() are probably under a parent that was leaked and has no backp()
        కొరకు (స్థిరరాశి తార్కిక with_వెనుక : {అబద్ధం, నిజం}) {
            కొరకు (స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep : m_allocated) {
                // LCOV_EXCL_START
                // Most likely not leaked, so check that first
                ఒకవేళ (VL_UNCOVERABLE(nodep->brokenState() != brokenCntCurrent)) {
                    స్థిరరాశి తార్కిక has_వెనుక = nodep->backp() != శూన్యసూచిక;
                    ఒకవేళ (has_వెనుక != with_వెనుక) కొనసాగు;
                    // Use only AstNode::dump instead of the virtual one, as there
                    // may be varp() and other cross links that are bad.
                    // When get this message, find what forgot to delete the
                    // node by running GDB, where for node "<e###>" use:
                    //    watch *(AstNode::s_editCntGbl)==####
                    //నిరంతరం
                    //    bt
                    std::cerr << "%Error: LeakedNode"
                              << (with_వెనుక ? " with back pointer: " : ": ");
                    nodep->Ast_కణుపు::dump(std::cerr);
                    std::cerr << పంక్తి_ముగింపు;
                    V3Error::incErrors();
                }
                // LCOV_EXCL_STOP
            }
        }
    }
} s_alloc_పట్టిక;

శూన్య V3Broken::కూడు_Newed(స్థిరరాశి Ast_కణుపు* nodep) { s_alloc_పట్టిక.కూడు_Newed(nodep); }
శూన్య V3Broken::deleted(స్థిరరాశి Ast_కణుపు* nodep) { s_alloc_పట్టిక.deleted(nodep); }

//######################################################################
// Table of AstNode pointers that can be linked to via member pointers

స్థిర తరగతి Linkable_పట్టిక final {
    // MEMBERS
    std::unordered_సమితి<స్థిరరాశి Ast_కణుపు*> m_linkable;  // Set of all nodes allocated but not freed

బహిరంగ:
    // METHODS
    శూన్య clear() { m_linkable.clear(); }
    శూన్య కూడు_Linkable(స్థిరరాశి Ast_కణుపు* nodep) { m_linkable.emplace(nodep); }
    తార్కిక isLinkable(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి { తిరిగి m_linkable.లెక్క(nodep) != 0; }
} s_linkable_పట్టిక;

తార్కిక V3Broken::isLinkable(స్థిరరాశి Ast_కణుపు* nodep) { తిరిగి s_linkable_పట్టిక.isLinkable(nodep); }

//######################################################################
// Check every node in tree

తరగతి BrokenCheckVisitor final : బహిరంగ VNVisitorConst {
    // Constants for marking we are under/not under a node
    స్థిరరాశి uint8_t m_brokenCntCurrentNotUnder = s_brokenCntGlobal.get();  // Top bit is clear
    స్థిరరాశి uint8_t m_brokenCntCurrentUnder = m_brokenCntCurrentNotUnder | 0x80;  // Top bit is set

    // STATE - across all visitors
    // All local variables declared in current function
    std::సమితి<స్థిరరాశి AstVar*> m_localVars;
    // Variable references in current function that do not reference an in-scope local
    std::map<స్థిరరాశి AstVar*, స్థిరరాశి Ast_కణుపు_Var_Ref*> m_suspectRefs;
    // Local variables declared in the scope of the current statement
    std::సదిశ<std::unordered_సమితి<స్థిరరాశి AstVar*>> m_locals_దొంతర;
    // Number of write references encountered
    కొలత_t m_nWriteRefs = 0;
    // Number of function calls encountered
    కొలత_t m_nCalls = 0;

    // STATE - for current visit position (use VL_RESTORER)
    స్థిరరాశి AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current CFunc, if any
    తార్కిక m_లోపల_Scope = అబద్ధం;  // Under AstScope
    std::సమితి<std::string> m_c_Func_పేర్లు;  // CFunc by name in current class/module

అంతరంగ:
    // METHODS
    స్థిర శూన్య check_వెడల్పు_Min(స్థిరరాశి Ast_కణుపు* nodep) {
        UASSERT_OBJ(nodep->వెడల్పు() == nodep->వెడల్పు_Min()
                        || v3Global.వెడల్పు_Min_Usage() != VWidthMinUsage::MATCHES_వెడల్పు,
                    nodep, "Width != WidthMin");
    }

    శూన్య processEnter(Ast_కణుపు* nodep) {
        nodep->brokenState(m_brokenCntCurrentUnder);
        స్థిరరాశి అక్షరం* స్థిరరాశి whyp = nodep->brokenGen();
        UASSERT_OBJ(!whyp, nodep,
                    "Broken link in node (or something without maybePointedTo): " << whyp);
        ఒకవేళ (!s_brokenAllowMidvisitorCheck) nodep->checkIter();
        ఒకవేళ (nodep->dtypep()) {
            UASSERT_OBJ(nodep->dtypep()->brokeExists(), nodep,
                        "Broken link in node->dtypep() to " << cvt_గమ్యం_Hex(nodep->dtypep()));
            UASSERT_OBJ(nodep->dtypep(), nodep,
                        "Non-dtype link in node->dtypep() to " << cvt_గమ్యం_Hex(nodep->dtypep()));
        }
        ఒకవేళ (v3Global.assertDTypesResolved()) {
            ఒకవేళ (nodep->hasDType()) {
            } లేకపోతే {
                UASSERT_OBJ(!VN_IS(nodep, కణుపు_సమాసం), nodep,
                            "All AstNodeExpr must have a dtype post V3WidthCommit");
                UASSERT_OBJ(!nodep->dtypep(), nodep,
                            "DType on node without hasDType(): " << nodep->prettyTypeName());
            }
            UASSERT_OBJ(!nodep->get_బిడ్డ_DTypep(), nodep,
                        "childDTypep() non-null on node after should have removed");
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dnodep = VN_CAST(nodep, కణుపు_DType))
                check_వెడల్పు_Min(dnodep);
        }
        check_వెడల్పు_Min(nodep);
    }
    శూన్య processExit(Ast_కణుపు* nodep) { nodep->brokenState(m_brokenCntCurrentNotUnder); }
    శూన్య processAndIterate(Ast_కణుపు* nodep) {
        processEnter(nodep);
        iterateChildrenConst(nodep);
        processExit(nodep);
    }
    శూన్య processAndIterateList(Ast_కణుపు* nodep) {
        అయితే (nodep) {
            processAndIterate(nodep);
            nodep = nodep->nextp();
        }
    }
    శూన్య నెట్టు_Local_Scope() {
        ఒకవేళ (m_cfuncp) m_locals_దొంతర.emplace_వెనుక();
    }
    శూన్య తీయి_Local_Scope() {
        ఒకవేళ (m_cfuncp) m_locals_దొంతర.తీయి_వెనుక();
    }
    తార్కిక is_లోపల_Scope_Local(స్థిరరాశి AstVar* varp) స్థిరరాశి {
        కొరకు (స్థిరరాశి స్వయంచాలక& సమితి : m_locals_దొంతర) {
            ఒకవేళ (సమితి.లెక్క(varp)) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    స్థిర శూన్య checkArgRefs(Ast_కణుపు_సమాసం* nodep, స్థిరరాశి అక్షరం* descrp, Ast_కణుపు_సమాసం* argsp) {
        ఒకవేళ (!std::strcmp(descrp, "TODO")) తిరిగి;  // Skip if not yet checked

        // Check each argument
        స్థిరరాశి అక్షరం* dp = descrp;
        కొరకు (Ast_కణుపు_సమాసం* argp = argsp; argp; argp = VN_AS(argp->nextp(), కణుపు_సమాసం)) {
            ఒకవేళ (argp->fileline()->erroringOn()) తిరిగి;  // Intentionally skip all checks
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి lvalp = argp->get_VAccess_లక్ష్యం_Recurse();
            స్థిరరాశి VAccess access
                = lvalp ? lvalp->getVAccessRecurse()  // Since we call it on a result of above call
                                                      // there should be no recursion at all
                        : VAccess{VAccess::READ};
            ఒకవేళ (dp[0] == '+') --dp;  // Repeats the entry before it
            ఎంపిక (dp[0]) {
            స్థితి 'r':
                UASSERT_OBJ(access.isReadOnly(), argp,
                            "Input argument of library call is not a read-only expression");
                ఆపు;
            స్థితి 'w':
                UASSERT_OBJ(lvalp, argp,  //
                            "Output argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isWriteOnly(), argp,
                            "Output argument of library call is not write-only");
                ఆపు;
            స్థితి 'm':
                UASSERT_OBJ(lvalp, argp,  //
                            "Inout argument of library call is not a valid C++ LValue");
                UASSERT_OBJ(access.isRW(), argp,
                            "Inout argument of library call is not read-write");
                ఆపు;
            అప్రమేయ:
                UASSERT_OBJ(dp[0], argp, "Unexpected trailing arguments to library call");
                ఆపు;  // LCOV_EXCL_LINE
            }
            ++dp;
        }
        UASSERT_OBJ(!dp[0] || dp[0] == '+', nodep, "Insufficient arguments to library call");
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        processEnter(nodep);
        iterateConst(nodep->rhsp());
        స్థిరరాశి కొలత_t nWriteRefs = m_nWriteRefs;
        స్థిరరాశి కొలత_t nCalls = m_nCalls;
        iterateConst(nodep->lhsp());
        // Only check if there are no calls on the LHS, as calls might return an LValue
        ఒకవేళ (v3Global.assertDTypesResolved() && m_nCalls == nCalls) {
            UASSERT_OBJ(m_nWriteRefs > nWriteRefs, nodep, "No write refs on LHS of assignment");
        }
        processExit(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), కణుపు_Var_Ref)
                      && !VN_AS(nodep->lhsp(), కణుపు_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Assignment LHS is not an lvalue");
    }
    శూన్య సందర్శించు(AstRelease* nodep) override {
        processAndIterate(nodep);
        UASSERT_OBJ(!(v3Global.assertDTypesResolved() && VN_IS(nodep->lhsp(), కణుపు_Var_Ref)
                      && !VN_AS(nodep->lhsp(), కణుపు_Var_Ref)->access().isWriteOrRW()),
                    nodep, "Release LHS is not an lvalue");
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_లోపల_Scope);
        m_లోపల_Scope = నిజం;
        VL_RESTORER_CLEAR(m_c_Func_పేర్లు);
        processAndIterate(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_పేర్లు);
        processAndIterate(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        VL_RESTORER_CLEAR(m_c_Func_పేర్లు);
        processAndIterate(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        processAndIterate(nodep);
        // m_inScope because some Vars have initial variable references without scopes
        // This might false fire with some debug flags, as not certain we don't have temporary
        // clear varScopep's during some an infrequent dump just before we re-LinkDot.
        UASSERT_OBJ(
            !(v3Global.assertScoped() && m_లోపల_Scope && nodep->varp() && !nodep->varScopep()), nodep,
            "VarRef missing VarScope pointer");
        UASSERT_OBJ(!v3Global.assertScoped() || !nodep->classOrPackagep(), nodep,
                    "VarRef classOrPackagep must be nullptr after V3Scope");
        ఒకవేళ (m_cfuncp) {
            // Check if variable is an in-scope local, otherwise mark as suspect
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varp()) {
                ఒకవేళ (!is_లోపల_Scope_Local(varp)) {
                    // This only stores the first ref for each Var, which is what we want
                    m_suspectRefs.emplace(varp, nodep);
                }
            }
        }
        ఒకవేళ (nodep->access().isWriteOrRW()) ++m_nWriteRefs;
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->పద్ధతి().వాదనలు(), nodep->pinsp());
    }
    శూన్య సందర్శించు(AstCFuncHard* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
        checkArgRefs(nodep, nodep->function().వాదనలు(), nodep->pinsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ++m_nCalls;
        processAndIterate(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Nested AstCFunc");
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        m_localVars.clear();
        m_suspectRefs.clear();
        m_locals_దొంతర.clear();
        నెట్టు_Local_Scope();

        // Check for duplicate names, otherwise linker might just ignore them
        string name_వాదనలు = nodep->name();
        ఒకవేళ (!nodep->argTypes().ఖాళీలు()) name_వాదనలు += "(" + nodep->argTypes() + ")";
        UASSERT_OBJ(m_c_Func_పేర్లు.emplace(name_వాదనలు).రెండవ, nodep,
                    "Duplicate cfunc name: '" << name_వాదనలు << "'");

        processAndIterate(nodep);

        // Check suspect references are all to non-locals
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_suspectRefs) {
            UASSERT_OBJ(m_localVars.లెక్క(pair.మొదటి) == 0, pair.రెండవ,
                        "Local variable not in scope where referenced: " << pair.మొదటి);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        // Each branch is a separate local variable scope
        నెట్టు_Local_Scope();
        processEnter(nodep);
        processAndIterate(nodep->condp());
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి thensp = nodep->thensp()) {
            నెట్టు_Local_Scope();
            processAndIterateList(thensp);
            తీయి_Local_Scope();
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి elsesp = nodep->elsesp()) {
            నెట్టు_Local_Scope();
            processAndIterateList(elsesp);
            తీయి_Local_Scope();
        }
        processExit(nodep);
        తీయి_Local_Scope();
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        // For local variable checking act as if any statement introduces a new scope.
        // This is aggressive but conservatively correct.
        నెట్టు_Local_Scope();
        processAndIterate(nodep);
        తీయి_Local_Scope();
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        processAndIterate(nodep);
        ఒకవేళ (m_cfuncp) {
            m_localVars.చొప్పించు(nodep);
            m_locals_దొంతర.వెనుక().చొప్పించు(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Process not just iterate
        processAndIterate(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన BrokenCheckVisitor(AstNetlist* nodep) { iterateConstNull(nodep); }
    ~BrokenCheckVisitor() override = అప్రమేయ;
};

//######################################################################
// Broken check entry point

శూన్య V3Broken::brokenAll(AstNetlist* nodep) {
    // UINFO(9, __FUNCTION__ << ": ");
    స్థిర తార్కిక s_లోపల_Broken = అబద్ధం;
    ఒకవేళ (VL_UNCOVERABLE(s_లోపల_Broken)) {
        // A error called by broken can recurse back into broken; avoid this
        UINFO(1, "Broken called under broken, skipping recursion.");  // LCOV_EXCL_LINE
    } లేకపోతే {
        s_లోపల_Broken = నిజం;

        స్థిరరాశి V3_తాళం_Guard తాళం_2{s_alloc_పట్టిక.m_mutex};

        // Mark every node in the tree
        స్థిరరాశి uint8_t brokenCntCurrent = s_brokenCntGlobal.get();
        nodep->foreach([brokenCntCurrent](Ast_కణుపు* nodep) VL_NO_THREAD_SAFETY_ANALYSIS {
#నిర్వచితమైతే VL_LEAK_CHECKS
            UASSERT_OBJ(s_alloc_పట్టిక.isAllocated(nodep), nodep,
                        "AstNode is in tree, but not allocated");
#అయితే_ముగింపు
            UASSERT_OBJ(nodep->brokenState() != brokenCntCurrent, nodep,
                        "AstNode is already in tree at another location");
            ఒకవేళ (nodep->maybe_Pointed_గమ్యం()) s_linkable_పట్టిక.కూడు_Linkable(nodep);
            nodep->brokenState(brokenCntCurrent);
        });

        // Check every node in tree
        స్థిరరాశి BrokenCheckVisitor cvisitor{nodep};

        // Check the constant pool lookup cache refers only to nodes in the tree
        స్థిరరాశి అక్షరం* స్థిరరాశి whyp = V3ConstPool::broken();
        UASSERT_OBJ(!whyp, nodep, "Broken constant pool cache: " << whyp);

        s_alloc_పట్టిక.checkForLeaks();
        s_linkable_పట్టిక.clear();
        s_brokenCntGlobal.inc();
        s_లోపల_Broken = అబద్ధం;
    }
}

శూన్య V3Broken::allowMidvisitorCheck(తార్కిక flag) { s_brokenAllowMidvisitorCheck = flag; }

//######################################################################
// Self test

శూన్య V3Broken::selfTest() {
    // Exercise addNewed and deleted for coverage, as otherwise only used with VL_LEAK_CHECKS
    File_పంక్తి* స్థిరరాశి fl = కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()};
    Ast_కణుపు* స్థిరరాశి newp = కొత్త AstBegin{fl, "[EditWrapper]", శూన్యసూచిక, అబద్ధం};
    // Don't actually do it with VL_LEAK_CHECKS, when new/delete calls these.
    // Otherwise you call addNewed twice on the same address, which is an error.
#నిర్వచితం_కాకపోతే VL_LEAK_CHECKS
    కూడు_Newed(newp);
    deleted(newp);
#అయితే_ముగింపు
    VL_DO_DANGLING(newp->delete_వృక్షం(), newp);
}
