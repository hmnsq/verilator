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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Fork.h"

#அந்தர்பாவ "V3AstNodeExpr.h"
#அந்தர்பாவ "V3MemberMap.h"

#அந்தர்பாவ <limits>
#அந்தர்பாவ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு ForkDynScopeInstance final {
பொது:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    தருக்க initialized() மாறிலி { திரும்பு m_classp != nullptr; }
};

வகுப்பு ForkDynScopeFrame final {
    // MEMBERS
    Ast_முனை_Module* மாறிலி m_modp;  // Module to insert the scope into
    Ast_முனை* மாறிலி m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    மாறிலி size_t m_class_id;  // Dynscope class ID
    மாறிலி size_t m_id;  // Dynscope ID

பொது:
    ForkDynScopeFrame(Ast_முனை_Module* modp, Ast_முனை* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = புதிய AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = புதிய AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = புதிய AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(பொய்);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        திரும்பு m_instance;
    }

    மாறிலி ForkDynScopeInstance& instance() மாறிலி { திரும்பு m_instance; }
    வெறுமை capture_Var_செருகு(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        எனில் (r.second) m_captureOrder.தள்ளு_back(varp);
    }
    தருக்க captured(AstVar* varp) { திரும்பு m_captures.மொத்தம்(varp) != 0; }
    Ast_முனை* procp() மாறிலி { திரும்பு m_procp; }

    வெறுமை populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        க்கு (AstVar* varp : m_captureOrder) {
            எனில் (varp->direction().isAny()) {
                varp = varp->clone_மரம்(பொய்);
                varp->direction(VDirection::NONE);
            } இல்லையெனில் {
                varp->unlinkFrBack();
            }
            varp->funcLocal(பொய்);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(பொய்);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* மாறிலி newp
            = புதிய AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(உண்மை);
        newp->classMethod(உண்மை);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    வெறுமை linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        எனில் (AstFork* மாறிலி forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            திரும்பு;
        }

        Ast_முனை* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_முனை* initp = stmtp;
        போது (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* மாறிலி newp = புதிய AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), முனை_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_முனை* மாறிலி asgnp = புதிய AstAssign{
            m_procp->fileline(),
            புதிய AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_முனை* initsp = nullptr;  // Arguments need to be copied
        க்கு (AstVar* varp : m_captureOrder) {
            எனில் (!varp->direction().isAny()) தொடர்;

            எனில் (varp->direction().isNonOutput()) {
                AstMemberSel* மாறிலி memberselp = புதிய AstMemberSel{
                    varp->fileline(),
                    புதிய AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_முனை* initAsgnp
                    = புதிய AstAssign{varp->fileline(), memberselp,
                                    புதிய AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_முனை::addNext(initsp, initAsgnp);
            }

            எனில் (AstBasicDType* மாறிலி dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                எனில் (dtypep->isEvent()) தொடர்;
            }

            எனில் (varp->direction().isWritable()) {
                AstMemberSel* மாறிலி memberselp = புதிய AstMemberSel{
                    varp->fileline(),
                    புதிய AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_முனை* writebackAsgnp = புதிய AstAssign{
                    varp->fileline(), புதிய AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_முனை::addNext(stmtp, writebackAsgnp);
            }
        }
        எனில் (initsp) Ast_முனை::addNext(asgnp, initsp);

        எனில் (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } இல்லையெனில் {
            Ast_முனை::addNext(asgnp, static_cast<Ast_முனை*>(initp));
            stmtp = asgnp;
        }

        Ast_முனை::addNext(static_cast<Ast_முனை*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    தருக்க linked() மாறிலி { திரும்பு m_instance.initialized() && m_instance.m_handlep->backp(); }

தனிப்பட்ட:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* மாறிலி newp = புதிய AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), முனை_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        திரும்பு புதிய AstAssign{
            m_procp->fileline(),
            புதிய AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    வெறுமை linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        மாறிலி std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* மாறிலி beginp = புதிய AstBegin{forkp->fileline(), name, m_instance.m_handlep, உண்மை};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        எனில் (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { திரும்பு "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(மாறிலி Ast_முனை* fromp) {
        திரும்பு "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_முனை* getProcStmts() {
        Ast_முனை* stmtsp = nullptr;
        எனில் (!m_procp) திரும்பு nullptr;
        எனில் (AstBegin* மாறிலி beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } இல்லையெனில் எனில் (Ast_முனை_FTask* மாறிலி taskp = VN_CAST(m_procp, முனை_FTask)) {
            stmtsp = taskp->stmtsp();
        } இல்லையெனில் {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        திரும்பு stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

வகுப்பு DynScopeVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;

    // STATE
    தருக்க m_inFunc = பொய்;  // True if in a function
    Ast_முனை_Module* m_modp = nullptr;  // Module we are currently under
    Ast_முனை* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_முனை*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_முனை*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    தருக்க m_afterTimingControl = பொய்;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_முனை* nodep) {
        auto frameIt = m_frames.find(nodep);
        எனில் (frameIt == m_frames.end()) திரும்பு nullptr;
        திரும்பு frameIt->second;
    }

    மாறிலி ForkDynScopeFrame* frameOf(Ast_முனை* nodep) மாறிலி {
        auto frameIt = m_frames.find(nodep);
        எனில் (frameIt == m_frames.end()) திரும்பு nullptr;
        திரும்பு frameIt->second;
    }

    ForkDynScopeFrame* தள்ளு_Dyn_Scope_Frame(Ast_முனை* procp) {
        ForkDynScopeFrame* மாறிலி framep
            = புதிய ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.தள்ளு_back(procp);
        திரும்பு framep;
    }

    வெறுமை replaceWithMemberSel(AstVarRef* refp, மாறிலி ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* மாறிலி membersel = புதிய AstMemberSel{
            refp->fileline(), புதிய AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        எனில் (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } இல்லையெனில் {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
    }

    நிலையான தருக்க hasAsyncFork(Ast_முனை* nodep) {
        திரும்பு nodep->exists([](AstFork* forkp) { திரும்பு !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { திரும்பு உண்மை; });
    }

    வெறுமை bind_முனை_To_Dyn_Scope(Ast_முனை* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        எனில் (r.second) m_frameOrder.தள்ளு_back(nodep);
    }
    வெறுமை bindInitIterate(Ast_முனை* stmtsp, ForkDynScopeFrame* framep) {
        க்கு (Ast_முனை* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstAssign* மாறிலி asgnp = VN_CAST(stmtp, Assign)) {
                bind_முனை_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } இல்லையெனில் எனில் (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } இல்லையெனில் எனில் (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } இல்லையெனில் {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    தருக்க needsDynScope(மாறிலி AstVarRef* refp) மாறிலி {
        மாறிலி AstVar* மாறிலி varp = refp->varp();
        திரும்பு
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_மதிப்பு() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        எனில் (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        எனில் (hasAsyncFork(nodep)) தள்ளு_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        எனில் (hasAsyncFork(nodep)) தள்ளு_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        எனில் (!nodep->joinType().join()) ++m_forkDepth;

        மாறிலி தருக்க oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        எனில் (nodep->declsp() || nodep->stmtsp()) framep = தள்ளு_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        க்கு (Ast_முனை* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* மாறிலி varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            எனில் (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                எனில் (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_செருகு(varp);
                bind_முனை_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        க்கு (Ast_முனை* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = பொய்;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        எனில் (nodep->isTimingControl()) m_afterTimingControl = உண்மை;
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        visit(static_cast<Ast_முனை_கோவை*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = உண்மை;
    }
    வெறுமை visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* மாறிலி framep = frameOf(m_procp);
        எனில் (!framep) திரும்பு;  // Cannot be legally referenced from a fork
        bind_முனை_To_Dyn_Scope(nodep, framep);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* மாறிலி framep = frameOf(nodep->varp());
        எனில் (!framep) திரும்பு;
        எனில் (needsDynScope(nodep)) {
            தருக்க isEvent = பொய்;
            எனில் (AstBasicDType* மாறிலி dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            எனில் (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            எனில் (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_செருகு(nodep->varp());
        }
        bind_முனை_To_Dyn_Scope(nodep, framep);
    }
    வெறுமை visit(AstAssign* nodep) override {
        எனில் (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_மதிப்பு()) {
            nodep->lhsp()->user2(உண்மை);
        }
        visit(static_cast<Ast_முனை_Stmt*>(nodep));
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        எனில் (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    திரும்பு refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(உண்மை);
            // Put it in a fork to prevent lifetime issues with the local
            File_வரி* மாறிலி flp = nodep->fileline();
            AstFork* மாறிலி forkp = புதிய AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(புதிய AstBegin{flp, "", nodep, பொய்});
            UINFO(9, "assign new fork " << forkp);
        } இல்லையெனில் {
            visit(static_cast<Ast_முனை_Stmt*>(nodep));
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (nodep->isTimingControl()) m_afterTimingControl = உண்மை;
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        தருக்க typesAdded = பொய்;
        க்கு (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            எனில் (!framep->instance().initialized()) தொடர்;
            எனில் (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = உண்மை;
            }
            எனில் (AstVarRef* மாறிலி refp = VN_CAST(frameIt->first, VarRef)) {
                எனில் (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        எனில் (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        க்கு (auto முனை_frame : m_frames) frames.செருகு(முனை_frame.second);
        க்கு (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

வகுப்பு ForkVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    மாறிலி VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_முனை_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    தருக்க m_inFork = பொய்;  // Traversal in an async fork
    தருக்க m_hasNestedFork = பொய்;  // Branch being taskified contains a nested fork
    தருக்க m_inInitStmt = பொய்;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        க்கு (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            எனில் (varp->name() == refp->name()) நிறுத்து;
        }
        எனில் (varp) திரும்பு varp;

        // Create a local copy to capture
        File_வரி* மாறிலி flp = refp->fileline();
        varp = புதிய AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::உள்ளீடு);
        varp->funcLocal(உண்மை);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_முனை::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* மாறிலி argp = புதிய AstArg{flp, refp->name(), refp->clone_மரம்(பொய்)};
        m_capturedArgsp = Ast_முனை::addNext(m_capturedArgsp, argp);
        திரும்பு varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    தருக்க taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = பொய்;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        எனில் (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) திரும்பு பொய்;

        // Create task holding the statement and repalce statement with call to that task
        File_வரி* மாறிலி flp = beginp->fileline();
        மாறிலி std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* மாறிலி taskp = புதிய AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        எனில் (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_முனை::addNext(m_tasksp, taskp);
        எனில் (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        எனில் (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* மாறிலி callp = புதிய AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        க்கு (AstVar* மாறிலி localp : m_forkLocalsp) localp->funcLocal(உண்மை);

        // We did wrap the body
        திரும்பு உண்மை;
    }
    நிலையான தருக்க isForkJoinNoneSentinelDelay(மாறிலி Ast_முனை* மாறிலி nodep) {
        மாறிலி AstDelay* மாறிலி delayp = VN_CAST(nodep, Delay);
        எனில் (!delayp) திரும்பு பொய்;
        மாறிலி AstConst* மாறிலி constp = VN_CAST(delayp->lhsp(), Const);
        திரும்பு constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    நிலையான தருக்க is_Disable_Queue_தள்ளு_Self_Prefix(Ast_முனை* nodep) {
        போது (AstJumpBlock* மாறிலி jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        திரும்பு nodep && nodep->is_Disable_Queue_தள்ளு_Self_Stmt();
    }
    template <typename T_Owner>
    நிலையான தருக்க செருகு_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* மாறிலி ownerp,
                                                          Ast_முனை* மாறிலி firstStmtp,
                                                          Ast_முனை* மாறிலி delayp) {
        Ast_முனை* செருகு_Beforep = firstStmtp;
        போது (செருகு_Beforep && செருகு_Beforep->is_Disable_Queue_தள்ளு_Self_Stmt()) {
            செருகு_Beforep = செருகு_Beforep->nextp();
        }
        எனில் (AstJumpBlock* மாறிலி jumpBlockp = VN_CAST(செருகு_Beforep, JumpBlock)) {
            எனில் (செருகு_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                திரும்பு உண்மை;
            }
        }
        எனில் (செருகு_Beforep == firstStmtp) திரும்பு பொய்;
        எனில் (செருகு_Beforep) {
            செருகு_Beforep->addHereThisAsNext(delayp);
        } இல்லையெனில் {
            ownerp->addStmtsp(delayp);
        }
        திரும்பு உண்மை;
    }
    நிலையான வெறுமை moveForkSentinelAfterDisableQueuePushes(AstBegin* மாறிலி beginp) {
        Ast_முனை* மாறிலி firstStmtp = beginp->stmtsp();
        எனில் (!isForkJoinNoneSentinelDelay(firstStmtp)) திரும்பு;
        Ast_முனை* மாறிலி afterSentinelp = firstStmtp->nextp();
        எனில் (!is_Disable_Queue_தள்ளு_Self_Prefix(afterSentinelp)) திரும்பு;

        Ast_முனை* மாறிலி delayp = firstStmtp->unlinkFrBack();
        மாறிலி தருக்க moved
            = செருகு_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    நிலையான தருக்க forkIsDisableable(AstFork* மாறிலி nodep) {
        க்கு (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            எனில் (is_Disable_Queue_தள்ளு_Self_Prefix(itemp->stmtsp())) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        எனில் (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    வெறுமை visit(AstFork* nodep) override {
        m_hasNestedFork = உண்மை;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        எனில் ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_வரி* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            க்கு (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                எனில் (!itemp->stmtsp()) தொடர்;
                AstDelay* மாறிலி delayp = புதிய AstDelay{
                    fl,
                    புதிய AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    பொய்};
                itemp->prependStmtsp(delayp);
                moveForkSentinelAfterDisableQueuePushes(itemp);
            }
        }

        iterateAndNextNull(nodep->declsp());
        iterateAndNextNull(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        எனில் (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            திரும்பு;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = உண்மை;
            க்கு (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                எனில் (taskify(itemp)) wrappedp.தள்ளு_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        க்கு (AstBegin* மாறிலி beginp : wrappedp) iterateAndNextNull(beginp);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (m_inFork) m_forkLocalsp.செருகு(nodep);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_inFork) திரும்பு;
        AstVar* மாறிலி varp = nodep->varp();
        // Not sure why this is OK ...
        எனில் (!varp->isFuncLocal() && varp->isClassMember()) திரும்பு;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        எனில் (varp->lifetime().isStatic()) திரும்பு;
        // If this ref is to a variable that will move into the task, then nothing to do
        எனில் (m_forkLocalsp.மொத்தம்(varp)) திரும்பு;

        எனில் (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_மதிப்பு() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            திரும்பு;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    வெறுமை visit(AstAssign* nodep) override {
        எனில் (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_மதிப்பு()) {
            nodep->lhsp()->user2(உண்மை);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = உண்மை;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstThisRef* nodep) override {}
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = இயல்பு;
};

//######################################################################
// Fork class functions

வெறுமை V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("fork_dynscope", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("fork_tasks", 0, dump_மரம்_Either_Level() >= 3);
}
