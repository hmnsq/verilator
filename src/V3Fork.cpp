// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create separate tasks for forked processes that
//              can outlive their parents
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
// V3Fork's Transformations:
//
// Each module:
//      Look for FORKs [JOIN_NONE]/[JOIN_ANY]
//          VARREF(var) -> MEMBERSEL(var->name, VARREF(dynscope)) (for write/RW refs)
//          FORK(stmts) -> TASK(stmts), FORK(TASKREF(inits))
//
// FORKs that spawn tasks which might outlive their parents require those
// tasks to carry their own frames and as such they require their own
// variable scopes.
//
// There are two mechanisms that work together to achieve that. ForkVisitor
// moves bodies of forked processes into new tasks, which results in them getting their
// own scopes. The original statements get replaced with a call to the task which
// passes the required variables by value.
//
// The second mechanism, DynScopeVisitor, is designed to handle variables which can't be
// captured by value and instead require a reference. Those variables get moved into an
// "anonymous" object, ie. a class with appropriate fields gets generated and an object
// of this class gets instantiated in place of the original variable declarations.
// Any references to those variables are replaced with references to the object's field.
// Since objects are reference-counted this ensures that the variables are accessible
// as long as both the parent and the forked processes require them to be.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Fork.h"

#అన్తర్భావ "V3AstNodeExpr.h"
#అన్తర్భావ "V3MemberMap.h"

#అన్తర్భావ <limits>
#అన్తర్భావ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి ForkDynScopeInstance final {
బహిరంగ:
    AstClass* m_classp = శూన్యసూచిక;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = శూన్యసూచిక;  // RefDType for the above
    AstVar* m_handlep = శూన్యసూచిక;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    తార్కిక initialized() స్థిరరాశి { తిరిగి m_classp != శూన్యసూచిక; }
};

తరగతి Fork_Dyn_Scope_చట్రం final {
    // MEMBERS
    Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Module to insert the scope into
    Ast_కణుపు* స్థిరరాశి m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_క్రమం;  // Variables to be moved into the dynscope
    std::సమితి<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    స్థిరరాశి కొలత_t m_class_id;  // Dynscope class ID
    స్థిరరాశి కొలత_t m_id;  // Dynscope ID

బహిరంగ:
    Fork_Dyn_Scope_చట్రం(Ast_కణుపు_Module* modp, Ast_కణుపు* procp, కొలత_t class_id, కొలత_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = కొత్త AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = కొత్త AstClassRefDType{m_procp->fileline(), m_instance.m_classp, శూన్యసూచిక};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = కొత్త AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(అబద్ధం);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        తిరిగి m_instance;
    }

    స్థిరరాశి ForkDynScopeInstance& instance() స్థిరరాశి { తిరిగి m_instance; }
    శూన్య capture_Var_చొప్పించు(AstVar* varp) {
        స్వయంచాలక r = m_captures.emplace(varp);
        ఒకవేళ (r.రెండవ) m_capture_క్రమం.నెట్టు_వెనుక(varp);
    }
    తార్కిక captured(AstVar* varp) { తిరిగి m_captures.లెక్క(varp) != 0; }
    Ast_కణుపు* procp() స్థిరరాశి { తిరిగి m_procp; }

    శూన్య populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        కొరకు (AstVar* varp : m_capture_క్రమం) {
            ఒకవేళ (varp->direction().isAny()) {
                varp = varp->clone_వృక్షం(అబద్ధం);
                varp->direction(VDirection::NONE);
            } లేకపోతే {
                varp->unlink_Fr_వెనుక();
            }
            varp->funcLocal(అబద్ధం);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->వాడిన_Loop_Idx(అబద్ధం);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->కూడు_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* స్థిరరాశి newp
            = కొత్త AstFunc{m_instance.m_classp->fileline(), "new", శూన్యసూచిక, శూన్యసూచిక};
        newp->isConstructor(నిజం);
        newp->class_పద్ధతి(నిజం);
        newp->dtypep(newp->కనుగొను_Void_DType());
        m_instance.m_classp->కూడు_Stmtsp(newp);
    }

    శూన్య linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        ఒకవేళ (AstFork* స్థిరరాశి forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_యొక్క_Fork(memberMap, forkp);
            తిరిగి;
        }

        Ast_కణుపు* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_వెనుక_With_తదుపరి(&stmtpHandle);

        // Find node after last variable declaration
        Ast_కణుపు* initp = stmtp;
        అయితే (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* స్థిరరాశి newp = కొత్త AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.కనుగొను_Member(m_instance.m_classp, "new"), కణుపు_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_కణుపు* స్థిరరాశి asgnp = కొత్త Ast_కేటాయించు{
            m_procp->fileline(),
            కొత్త AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_కణుపు* initsp = శూన్యసూచిక;  // Arguments need to be copied
        కొరకు (AstVar* varp : m_capture_క్రమం) {
            ఒకవేళ (!varp->direction().isAny()) కొనసాగు;

            ఒకవేళ (varp->direction().isNonOutput()) {
                AstMemberSel* స్థిరరాశి memberselp = కొత్త AstMemberSel{
                    varp->fileline(),
                    కొత్త AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.కనుగొను_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_కణుపు* initAsgnp
                    = కొత్త Ast_కేటాయించు{varp->fileline(), memberselp,
                                    కొత్త AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_కణుపు::కూడు_తదుపరి(initsp, initAsgnp);
            }

            ఒకవేళ (AstBasicDType* స్థిరరాశి dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.సమితి_Assigns_Events();
                ఒకవేళ (dtypep->isEvent()) కొనసాగు;
            }

            ఒకవేళ (varp->direction().isWritable()) {
                AstMemberSel* స్థిరరాశి memberselp = కొత్త AstMemberSel{
                    varp->fileline(),
                    కొత్త AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.కనుగొను_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_కణుపు* writebackAsgnp = కొత్త Ast_కేటాయించు{
                    varp->fileline(), కొత్త AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_కణుపు::కూడు_తదుపరి(stmtp, writebackAsgnp);
            }
        }
        ఒకవేళ (initsp) Ast_కణుపు::కూడు_తదుపరి(asgnp, initsp);

        ఒకవేళ (initp != stmtp) {
            initp->కూడు_ఇక్కడ_This_As_తదుపరి(asgnp);
        } లేకపోతే {
            Ast_కణుపు::కూడు_తదుపరి(asgnp, static_cast<Ast_కణుపు*>(initp));
            stmtp = asgnp;
        }

        Ast_కణుపు::కూడు_తదుపరి(static_cast<Ast_కణుపు*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->కూడు_Stmtsp(m_instance.m_classp);
    }

    తార్కిక linked() స్థిరరాశి { తిరిగి m_instance.initialized() && m_instance.m_handlep->backp(); }

అంతరంగ:
    Ast_కేటాయించు* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* స్థిరరాశి newp = కొత్త AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.కనుగొను_Member(m_instance.m_classp, "new"), కణుపు_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        తిరిగి కొత్త Ast_కేటాయించు{
            m_procp->fileline(),
            కొత్త AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    శూన్య link_Nodes_యొక్క_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        స్థిరరాశి std::string name = "_Vwrapped_"  //
                                 + (forkp->name().ఖాళీలు() ? "" : forkp->name() + "_")  //
                                 + std::గమ్యం_string(m_id);
        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{forkp->fileline(), name, m_instance.m_handlep, నిజం};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->కూడు_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        ఒకవేళ (forkp->stmtsp()) beginp->కూడు_Stmtsp(forkp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        // Put the Fork back at the end of the Begin
        beginp->కూడు_Stmtsp(forkp);
        m_modp->కూడు_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { తిరిగి "__VDynScope_" + cvt_గమ్యం_Str(m_class_id); }

    string generateDynScopeHandleName(స్థిరరాశి Ast_కణుపు* fromp) {
        తిరిగి "__VDynScope_" + (!fromp->name().ఖాళీలు() ? (fromp->name() + "_") : "ANON_")
               + cvt_గమ్యం_Str(m_id);
    }

    Ast_కణుపు* getProcStmts() {
        Ast_కణుపు* stmtsp = శూన్యసూచిక;
        ఒకవేళ (!m_procp) తిరిగి శూన్యసూచిక;
        ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } లేకపోతే ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి taskp = VN_CAST(m_procp, కణుపు_FTask)) {
            stmtsp = taskp->stmtsp();
        } లేకపోతే {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        తిరిగి stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

తరగతి DynScopeVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    తార్కిక m_లోపల_Func = అబద్ధం;  // True if in a function
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Module we are currently under
    Ast_కణుపు* m_procp = శూన్యసూచిక;  // Function/task/block we are currently under
    std::deque<Ast_కణుపు*> m_చట్రం_క్రమం;  // Ordered list of frames (for determinism)
    std::map<Ast_కణుపు*, Fork_Dyn_Scope_చట్రం*> m_చట్రాలు;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_లోతు = 0;  // Number of asynchronous forks we are currently under
    తార్కిక m_afterTimingControl = అబద్ధం;  // A timing control might've be executed in the current
                                        // process
    కొలత_t m_id = 0;  // Unique ID for a frame
    కొలత_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_చట్రం* చట్రం_యొక్క(Ast_కణుపు* nodep) {
        స్వయంచాలక చట్రం_It = m_చట్రాలు.కనుగొను(nodep);
        ఒకవేళ (చట్రం_It == m_చట్రాలు.చివర()) తిరిగి శూన్యసూచిక;
        తిరిగి చట్రం_It->రెండవ;
    }

    స్థిరరాశి Fork_Dyn_Scope_చట్రం* చట్రం_యొక్క(Ast_కణుపు* nodep) స్థిరరాశి {
        స్వయంచాలక చట్రం_It = m_చట్రాలు.కనుగొను(nodep);
        ఒకవేళ (చట్రం_It == m_చట్రాలు.చివర()) తిరిగి శూన్యసూచిక;
        తిరిగి చట్రం_It->రెండవ;
    }

    Fork_Dyn_Scope_చట్రం* నెట్టు_Dyn_Scope_చట్రం(Ast_కణుపు* procp) {
        Fork_Dyn_Scope_చట్రం* స్థిరరాశి framep
            = కొత్త Fork_Dyn_Scope_చట్రం{m_modp, procp, m_class_id++, m_id++};
        స్వయంచాలక r = m_చట్రాలు.emplace(procp, framep);
        UASSERT_OBJ(r.రెండవ, m_modp, "Procedure already contains a frame");
        m_చట్రం_క్రమం.నెట్టు_వెనుక(procp);
        తిరిగి framep;
    }

    శూన్య replaceWithMemberSel(AstVarRef* refp, స్థిరరాశి ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_వెనుక(&handle);
        AstMemberSel* స్థిరరాశి membersel = కొత్త AstMemberSel{
            refp->fileline(), కొత్త AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        ఒకవేళ (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.కనుగొను_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } లేకపోతే {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
    }

    స్థిర తార్కిక hasAsyncFork(Ast_కణుపు* nodep) {
        తిరిగి nodep->exists([](AstFork* forkp) { తిరిగి !forkp->joinType().join(); })
               || nodep->exists([](Ast_కేటాయించు_Dly*) { తిరిగి నిజం; });
    }

    శూన్య bind_కణుపు_గమ్యం_Dyn_Scope(Ast_కణుపు* nodep, Fork_Dyn_Scope_చట్రం* framep) {
        స్వయంచాలక r = m_చట్రాలు.emplace(nodep, framep);
        ఒకవేళ (r.రెండవ) m_చట్రం_క్రమం.నెట్టు_వెనుక(nodep);
    }
    శూన్య bindInitIterate(Ast_కణుపు* stmtsp, Fork_Dyn_Scope_చట్రం* framep) {
        కొరకు (Ast_కణుపు* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి asgnp = VN_CAST(stmtp, కేటాయించు)) {
                bind_కణుపు_గమ్యం_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } లేకపోతే ఒకవేళ (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } లేకపోతే ఒకవేళ (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } లేకపోతే {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    తార్కిక needsDynScope(స్థిరరాశి AstVarRef* refp) స్థిరరాశి {
        స్థిరరాశి AstVar* స్థిరరాశి varp = refp->varp();
        తిరిగి
            // Can this variable escape the scope
            ((m_fork_లోతు > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_విలువ() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        ఒకవేళ (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_లోపల_Func);
        m_లోపల_Func = VN_IS(nodep, Func);
        ఒకవేళ (hasAsyncFork(nodep)) నెట్టు_Dyn_Scope_చట్రం(m_procp);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        ఒకవేళ (hasAsyncFork(nodep)) నెట్టు_Dyn_Scope_చట్రం(m_procp);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        VL_RESTORER(m_fork_లోతు);
        ఒకవేళ (!nodep->joinType().join()) ++m_fork_లోతు;

        స్థిరరాశి తార్కిక oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_చట్రం* framep = శూన్యసూచిక;
        ఒకవేళ (nodep->declsp() || nodep->stmtsp()) framep = నెట్టు_Dyn_Scope_చట్రం(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        కొరకు (Ast_కణుపు* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* స్థిరరాశి varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            ఒకవేళ (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                ఒకవేళ (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_చొప్పించు(varp);
                bind_కణుపు_గమ్యం_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        కొరకు (Ast_కణుపు* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = అబద్ధం;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        ఒకవేళ (nodep->isTimingControl()) m_afterTimingControl = నిజం;
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు_సమాసం*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = నిజం;
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        nodep->user1(m_fork_లోతు);
        Fork_Dyn_Scope_చట్రం* స్థిరరాశి framep = చట్రం_యొక్క(m_procp);
        ఒకవేళ (!framep) తిరిగి;  // Cannot be legally referenced from a fork
        bind_కణుపు_గమ్యం_Dyn_Scope(nodep, framep);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        Fork_Dyn_Scope_చట్రం* స్థిరరాశి framep = చట్రం_యొక్క(nodep->varp());
        ఒకవేళ (!framep) తిరిగి;
        ఒకవేళ (needsDynScope(nodep)) {
            తార్కిక isEvent = అబద్ధం;
            ఒకవేళ (AstBasicDType* స్థిరరాశి dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.సమితి_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            ఒకవేళ (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_లోపల_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            ఒకవేళ (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_చొప్పించు(nodep->varp());
        }
        bind_కణుపు_గమ్యం_Dyn_Scope(nodep, framep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        ఒకవేళ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_విలువ()) {
            nodep->lhsp()->user2(నిజం);
        }
        సందర్శించు(static_cast<Ast_కణుపు_Stmt*>(nodep));
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        ఒకవేళ (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    తిరిగి refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(నిజం);
            // Put it in a fork to prevent lifetime issues with the local
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            AstFork* స్థిరరాశి forkp = కొత్త AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->కూడు_Forksp(కొత్త AstBegin{flp, "", nodep, అబద్ధం});
            UINFO(9, "assign new fork " << forkp);
        } లేకపోతే {
            సందర్శించు(static_cast<Ast_కణుపు_Stmt*>(nodep));
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (nodep->isTimingControl()) m_afterTimingControl = నిజం;
        iterateChildren(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        సందర్శించు(nodep);

        // Commit changes to AST
        తార్కిక typesAdded = అబద్ధం;
        కొరకు (స్వయంచాలక orderp : m_చట్రం_క్రమం) {
            UINFO(9, "Frame commit " << orderp);
            స్వయంచాలక చట్రం_It = m_చట్రాలు.కనుగొను(orderp);
            UASSERT_OBJ(చట్రం_It != m_చట్రాలు.చివర(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_చట్రం* framep = చట్రం_It->రెండవ;
            ఒకవేళ (!framep->instance().initialized()) కొనసాగు;
            ఒకవేళ (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = నిజం;
            }
            ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(చట్రం_It->మొదటి, VarRef)) {
                ఒకవేళ (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        ఒకవేళ (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::సమితి<Fork_Dyn_Scope_చట్రం*> చట్రాలు;
        కొరకు (స్వయంచాలక కణుపు_చట్రం : m_చట్రాలు) చట్రాలు.చొప్పించు(కణుపు_చట్రం.రెండవ);
        కొరకు (స్వయంచాలక* framep : చట్రాలు) తొలగించు framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

తరగతి ForkVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Class/module we are currently under
    AstTask* m_tasksp = శూన్యసూచిక;  // Tasks exrtracted under current module
    కొలత_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    తార్కిక m_లోపల_Fork = అబద్ధం;  // Traversal in an async fork
    తార్కిక m_hasNestedFork = అబద్ధం;  // Branch being taskified contains a nested fork
    తార్కిక m_లోపల_Init_Stmt = అబద్ధం;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Enclosing function/task
    std::సమితి<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = శూన్యసూచిక;  // Local copies of captured variables
    AstArg* m_capturedArgsp = శూన్యసూచిక;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = శూన్యసూచిక;
        కొరకు (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ఒకవేళ (varp->name() == refp->name()) ఆపు;
        }
        ఒకవేళ (varp) తిరిగి varp;

        // Create a local copy to capture
        File_పంక్తి* స్థిరరాశి flp = refp->fileline();
        varp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::ఆదానం);
        varp->funcLocal(నిజం);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_కణుపు::కూడు_తదుపరి(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* స్థిరరాశి argp = కొత్త AstArg{flp, refp->name(), refp->clone_వృక్షం(అబద్ధం)};
        m_capturedArgsp = Ast_కణుపు::కూడు_తదుపరి(m_capturedArgsp, argp);
        తిరిగి varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    తార్కిక taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = అబద్ధం;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = శూన్యసూచిక;
        m_capturedArgsp = శూన్యసూచిక;
        iterate(beginp);

        // No need to do it if no variabels are captured
        ఒకవేళ (m_forkLocalsp.ఖాళీలు() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) తిరిగి అబద్ధం;

        // Create task holding the statement and repalce statement with call to that task
        File_పంక్తి* స్థిరరాశి flp = beginp->fileline();
        స్థిరరాశి std::string name = "__VforkTask_" + std::గమ్యం_string(m_nForkTasks++);
        AstTask* స్థిరరాశి taskp = కొత్త AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        ఒకవేళ (m_hasNestedFork) {
            taskp->class_పద్ధతి(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_కణుపు::కూడు_తదుపరి(m_tasksp, taskp);
        ఒకవేళ (beginp->declsp()) taskp->కూడు_Stmtsp(beginp->declsp()->unlink_Fr_వెనుక_With_తదుపరి());
        ఒకవేళ (beginp->stmtsp()) taskp->కూడు_Stmtsp(beginp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        AstTaskRef* స్థిరరాశి callp = కొత్త AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->కూడు_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        కొరకు (AstVar* స్థిరరాశి localp : m_forkLocalsp) localp->funcLocal(నిజం);

        // We did wrap the body
        తిరిగి నిజం;
    }
    స్థిర తార్కిక isForkJoinNoneSentinelDelay(స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) {
        స్థిరరాశి AstDelay* స్థిరరాశి delayp = VN_CAST(nodep, Delay);
        ఒకవేళ (!delayp) తిరిగి అబద్ధం;
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(delayp->lhsp(), Const);
        తిరిగి constp && (constp->గమ్యం_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    స్థిర తార్కిక is_Disable_వరుస_నెట్టు_Self_ఉపసర్గ(Ast_కణుపు* nodep) {
        అయితే (Ast_Jump_ఖండం* స్థిరరాశి jumpBlockp = VN_CAST(nodep, Jump_ఖండం)) {
            nodep = jumpBlockp->stmtsp();
        }
        తిరిగి nodep && nodep->is_Disable_వరుస_నెట్టు_Self_Stmt();
    }
    మూస <రకం_పేరు T_Owner>
    స్థిర తార్కిక చొప్పించు_Fork_Sentinel_After_Disable_వరుస_Pushes(T_Owner* స్థిరరాశి ownerp,
                                                          Ast_కణుపు* స్థిరరాశి మొదటి_Stmtp,
                                                          Ast_కణుపు* స్థిరరాశి delayp) {
        Ast_కణుపు* చొప్పించు_Beforep = మొదటి_Stmtp;
        అయితే (చొప్పించు_Beforep && చొప్పించు_Beforep->is_Disable_వరుస_నెట్టు_Self_Stmt()) {
            చొప్పించు_Beforep = చొప్పించు_Beforep->nextp();
        }
        ఒకవేళ (Ast_Jump_ఖండం* స్థిరరాశి jumpBlockp = VN_CAST(చొప్పించు_Beforep, Jump_ఖండం)) {
            ఒకవేళ (చొప్పించు_Fork_Sentinel_After_Disable_వరుస_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                తిరిగి నిజం;
            }
        }
        ఒకవేళ (చొప్పించు_Beforep == మొదటి_Stmtp) తిరిగి అబద్ధం;
        ఒకవేళ (చొప్పించు_Beforep) {
            చొప్పించు_Beforep->కూడు_ఇక్కడ_This_As_తదుపరి(delayp);
        } లేకపోతే {
            ownerp->కూడు_Stmtsp(delayp);
        }
        తిరిగి నిజం;
    }
    స్థిర శూన్య move_Fork_Sentinel_After_Disable_వరుస_Pushes(AstBegin* స్థిరరాశి beginp) {
        Ast_కణుపు* స్థిరరాశి మొదటి_Stmtp = beginp->stmtsp();
        ఒకవేళ (!isForkJoinNoneSentinelDelay(మొదటి_Stmtp)) తిరిగి;
        Ast_కణుపు* స్థిరరాశి afterSentinelp = మొదటి_Stmtp->nextp();
        ఒకవేళ (!is_Disable_వరుస_నెట్టు_Self_ఉపసర్గ(afterSentinelp)) తిరిగి;

        Ast_కణుపు* స్థిరరాశి delayp = మొదటి_Stmtp->unlink_Fr_వెనుక();
        స్థిరరాశి తార్కిక moved
            = చొప్పించు_Fork_Sentinel_After_Disable_వరుస_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    స్థిర తార్కిక forkIsDisableable(AstFork* స్థిరరాశి nodep) {
        కొరకు (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            ఒకవేళ (is_Disable_వరుస_నెట్టు_Self_ఉపసర్గ(itemp->stmtsp())) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = శూన్యసూచిక;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        ఒకవేళ (m_tasksp) nodep->కూడు_Stmtsp(m_tasksp);
    }

    శూన్య సందర్శించు(AstFork* nodep) override {
        m_hasNestedFork = నిజం;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        ఒకవేళ ((nodep->joinType().joinNone() && !nodep->తక్షణ_ప్రారంభం())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_పంక్తి* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            కొరకు (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                ఒకవేళ (!itemp->stmtsp()) కొనసాగు;
                AstDelay* స్థిరరాశి delayp = కొత్త AstDelay{
                    fl,
                    కొత్త AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    అబద్ధం};
                itemp->prependStmtsp(delayp);
                move_Fork_Sentinel_After_Disable_వరుస_Pushes(itemp);
            }
        }

        iterate_And_తదుపరి_Null(nodep->declsp());
        iterate_And_తదుపరి_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        ఒకవేళ (nodep->joinType().join()) {
            iterate_And_తదుపరి_Null(nodep->forksp());
            తిరిగి;
        }

        std::సదిశ<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_లోపల_Fork);
            m_లోపల_Fork = నిజం;
            కొరకు (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                ఒకవేళ (taskify(itemp)) wrappedp.నెట్టు_వెనుక(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        కొరకు (AstBegin* స్థిరరాశి beginp : wrappedp) iterate_And_తదుపరి_Null(beginp);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (m_లోపల_Fork) m_forkLocalsp.చొప్పించు(nodep);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_లోపల_Fork) తిరిగి;
        AstVar* స్థిరరాశి varp = nodep->varp();
        // Not sure why this is OK ...
        ఒకవేళ (!varp->isFuncLocal() && varp->isClassMember()) తిరిగి;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        ఒకవేళ (varp->lifetime().isStatic()) తిరిగి;
        // If this ref is to a variable that will move into the task, then nothing to do
        ఒకవేళ (m_forkLocalsp.లెక్క(varp)) తిరిగి;

        ఒకవేళ (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_విలువ() || nodep->user2())
            && !m_లోపల_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_మరిన్ని()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            తిరిగి;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        ఒకవేళ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_విలువ()) {
            nodep->lhsp()->user2(నిజం);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Init_Stmt);
        m_లోపల_Init_Stmt = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Init_Stmt);
        m_లోపల_Init_Stmt = నిజం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstThisRef* nodep) override {}
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ForkVisitor(AstNetlist* nodep) { సందర్శించు(nodep); }
    ~ForkVisitor() override = అప్రమేయ;
};

//######################################################################
// Fork class functions

శూన్య V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("fork_dynscope", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("fork_tasks", 0, dump_వృక్షం_Either_Level() >= 3);
}
