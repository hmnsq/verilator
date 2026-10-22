// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Replace return/continue with jumps
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
// V3LinkJump's Transformations:
//
// Each module:
//   Look for BEGINs
//      BEGIN(VAR...) -> VAR ... {renamed}
//   FOR -> WHILEs
//
//   Add JumpLabel which branches to after statements within JumpLabel
//      RETURN -> JUMPBLOCK(statements with RETURN changed to JUMPGO, ..., JUMPLABEL)
//      WHILE(... BREAK) -> JUMPBLOCK(WHILE(... statements with BREAK changed to JUMPGO),
//                                    ... JUMPLABEL)
//      WHILE(... CONTINUE) -> WHILE(JUMPBLOCK(... statements with CONTINUE changed to JUMPGO,
//                                    ... JUMPPABEL))
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkJump.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Error.h"
#ضمن "V3UniqueNames.h"

#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف LinkJumpVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;
    ثابت VNUser2_دننه_Use m_user2_دننه_Use;
    ثابت VNUser3_دننه_Use m_user3_دننه_Use;

    // STATE
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    Ast_غوټه* m_loopp = تش_اشاره;  // Current loop
    AstRandSequence* m_randsequencep = تش_اشاره;  // Current randsequence
    منطقي m_loopInc = خطأ;  // In loop increment
    منطقي m_دننه_Fork = خطأ;  // Under fork
    صحيح m_mod_Repeat_عدد = 0;  // Repeat counter
    VOptionBool m_unroll_ډک;  // Pragma full, disable, or default unrolling
    std::ویکټور<Ast_غوټه_بلاک*> m_بلاک_ډېری;  // All begin blocks above current node
    V3_Unique_نومونه m_کتار_نومونه{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ثابت AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ثابت AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ثابت AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ثابت AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ثابت AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    Ast_Jump_بلاک* get_Jump_بلاک(Ast_غوټه* nodep, منطقي پای_ټکی_د_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        إذا (پای_ټکی_د_Iter) {
            إذا (nodep->user1p()) أرجع VN_AS(nodep->user1p(), Jump_بلاک);
        } وإلا {
            إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), Jump_بلاک);
        }

        Ast_غوټه* underp = تش_اشاره;
        منطقي under_and_بل = ریښتیا;
        إذا (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!پای_ټکی_د_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } وإلا إذا (Ast_غوټه_FTask* ثابت fTaskp = VN_CAST(nodep, غوټه_FTask)) {
            UASSERT_OBJ(!پای_ټکی_د_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } وإلا إذا (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            إذا (پای_ټکی_د_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                إذا (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } وإلا {
                underp = nodep;
                under_and_بل = خطأ;  // IE we skip the entire foreach
            }
        } وإلا إذا (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            إذا (پای_ټکی_د_Iter) {
                underp = loopp->stmtsp();
            } وإلا {
                underp = nodep;
                under_and_بل = خطأ;  // IE we skip the entire loop
            }
        } وإلا {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            أرجع تش_اشاره;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        طالما (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        إذا (!underp->nextp() || !under_and_بل) {
            إذا (Ast_Jump_بلاک* ثابت blockp = VN_CAST(underp, Jump_بلاک)) أرجع blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        إذا (under_and_بل) {
            underp->unlink_Fr_شا_With_بل(&repHandle);
        } وإلا {
            underp->unlink_Fr_شا(&repHandle);
        }
        Ast_Jump_بلاک* ثابت blockp = جديد Ast_Jump_بلاک{nodep->fileline(), underp};
        إذا (پای_ټکی_د_Iter) {
            nodep->user1p(blockp);
        } وإلا {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        لكل (Ast_غوټه *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            إذا (VN_IS(varp, Var)) blockp->جمع_کړه_دلته_This_As_بل(varp->unlink_Fr_شا());
        }
        أرجع blockp;
    }
    فراغ جمع_کړه_مختاړی_منزل_بلاکونه_Recurse(ثابت std::string& مختاړی, Ast_غوټه* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->name() != "") beginp->name(مختاړی + beginp->name());
        }

        إذا (Ast_غوټه* ثابت refp = nodep->op1p()) جمع_کړه_مختاړی_منزل_بلاکونه_Recurse(مختاړی, refp);
        إذا (Ast_غوټه* ثابت refp = nodep->op2p()) جمع_کړه_مختاړی_منزل_بلاکونه_Recurse(مختاړی, refp);
        إذا (Ast_غوټه* ثابت refp = nodep->op3p()) جمع_کړه_مختاړی_منزل_بلاکونه_Recurse(مختاړی, refp);
        إذا (Ast_غوټه* ثابت refp = nodep->op4p()) جمع_کړه_مختاړی_منزل_بلاکونه_Recurse(مختاړی, refp);
        إذا (Ast_غوټه* ثابت refp = nodep->nextp()) جمع_کړه_مختاړی_منزل_بلاکونه_Recurse(مختاړی, refp);
    }
    منطقي exists_بلاک_Above(ثابت std::string& name) ثابت {
        لكل (ثابت Ast_غوټه_بلاک* ثابت stackp : vlstd::سرچپه_view(m_بلاک_ډېری)) {
            إذا (stackp->name() == name) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    ساكن Ast_غوټه_Module* پیدا_کړه_Owner_Modulep(Ast_غوټه* nodep) {
        لكل (Ast_غوټه* curp = nodep; curp; curp = curp->aboveLoopp()) {
            إذا (Ast_غوټه_Module* ثابت modp = VN_CAST(curp, غوټه_Module)) أرجع modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        أرجع تش_اشاره;  // LCOV_EXCL_LINE
    }
    ساكن std::string موخه_Instance_لار(ثابت AstDisable* ثابت nodep) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->موخه_Refp(), VarXRef)) {
            أرجع xrefp->dotted();
        }
        أرجع "";
    }
    ساكن std::string کتار_Dotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        أرجع processQueuep->is_لنډمهاله() ? موخه_Instance_لار(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_غوټه_بلاک* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = تش_اشاره;
        Ast_غوټه_بلاک* prevBlockp = تش_اشاره;
        لكل (Ast_غوټه_بلاک* ثابت blockp : vlstd::سرچپه_view(m_بلاک_ډېری)) {
            إذا (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            إذا (blockp == targetp) أرجع innerForkBranchp;
            prevBlockp = blockp;
        }
        أرجع تش_اشاره;
    }
    ساكن Ast_Stmt_بیان* get_کتار_ټېل_Process_Selfp(AstVarRef* ثابت کتار_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_کرښه* ثابت flp = کتار_Refp->fileline();
        أرجع جديد Ast_Stmt_بیان{
            flp,
            جديد Ast_لاره_Call{flp, کتار_Refp, "push_back",
                              جديد AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ساكن Ast_غوټه_Var_Ref* new_کتار_Refp(File_کرښه* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        إذا (dotted != "") أرجع جديد AstVarXRef{fl, processQueuep, dotted, access};
        إذا (!processQueuep->lifetime().isStatic() || processQueuep->is_لنډمهاله()) {
            أرجع جديد AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت سر_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        أرجع جديد AstVarRef{fl, سر_Pkgp, processQueuep, access};
    }
    ساكن Ast_Stmt_بیان* get_کتار_ټېل_Process_Selfp(File_کرښه* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت کتار_Write_Refp
            = VN_AS(new_کتار_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        أرجع get_کتار_ټېل_Process_Selfp(کتار_Write_Refp);
    }
    ساكن Ast_Stmt_بیان* get_کتار_Kill_Stmtp(File_کرښه* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_غوټه_Var_Ref* ثابت کتار_Refp
            = new_کتار_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_کتار_Call = تش_اشاره;
        لكل (Ast_غوټه* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            إذا (itemp->name() == "killQueue") {
                kill_کتار_Call
                    = جديد AstTaskRef{fl, VN_AS(itemp, Task), جديد AstArg{fl, "", کتار_Refp}};
                اكسر;
            }
        }
        UASSERT(kill_کتار_Call, "Should be found");
        kill_کتار_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        أرجع جديد Ast_Stmt_بیان{fl, kill_کتار_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_کرښه* ثابت fl) {
        ثابت خپلکاری it = m_taskDisableBegins.پیدا_کړه(taskp);
        إذا (it != m_taskDisableBegins.پای_ټکی()) أرجع it->دوهم;

        AstBegin* ثابت taskBodyp = جديد AstBegin{fl, "", تش_اشاره, خطأ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->سېټ_Need_Process();
        v3Global.سېټ_کاروي_Timing();
        // Ports stay in the task so calls see them
        لكل (Ast_غوټه* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_غوټه* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) taskBodyp->جمع_کړه_Stmtsp(stmtp->unlink_Fr_شا());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جمع_کړه_Forksp(taskBodyp);
        taskp->جمع_کړه_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        أرجع taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_غوټه* ثابت nodep, File_کرښه* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = جديد AstVar{
            fl, varType, m_کتار_نومونه.get(nodep->name()), VFlag_ماشوم_DType{},
            جديد Ast_کتار_DType{
                fl, VFlag_ماشوم_DType{},
                جديد AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), تش_اشاره},
                تش_اشاره}};
        processQueuep->process_کتار(ریښتیا);
        processQueuep->سېټ_Ignore_Sched_Write();
        أرجع processQueuep;
    }
    AstVar* getProcessQueuep(Ast_غوټه* ثابت nodep, File_کرښه* ثابت fl) {
        Ast_غوټه_Module* ثابت ownerp = پیدا_کړه_Owner_Modulep(nodep);

        إذا (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت سر_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            سر_Pkgp->جمع_کړه_Stmtsp(processQueuep);
            أرجع processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->جمع_کړه_Stmtsp(processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_کرښه* ثابت fl) {
        ثابت خپلکاری it = m_taskDisableQueues.پیدا_کړه(taskp);
        إذا (it != m_taskDisableQueues.پای_ټکی()) أرجع it->دوهم;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_بیان* ثابت ټېل_Current_Processp = get_کتار_ټېل_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ټېل_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        أرجع processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_کرښه* ثابت fl) {
        ثابت خپلکاری it = m_beginDisableBegins.پیدا_کړه(beginp);
        إذا (it != m_beginDisableBegins.پای_ټکی()) أرجع it->دوهم;

        AstBegin* ثابت beginBodyp = جديد AstBegin{fl, "", تش_اشاره, خطأ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->سېټ_Need_Process();
        v3Global.سېټ_کاروي_Timing();
        إذا (beginp->stmtsp()) beginBodyp->جمع_کړه_Stmtsp(beginp->stmtsp()->unlink_Fr_شا_With_بل());

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جمع_کړه_Forksp(beginBodyp);
        beginp->جمع_کړه_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        أرجع beginBodyp;
    }
    فراغ prepend_Fork_Branch_کتار_Pushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_کرښه* ثابت fl, منطقي needProcess) {
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            إذا (needProcess) branchp->سېټ_Need_Process();
            branchp->prependStmtsp(get_کتار_ټېل_Process_Selfp(fl, processQueuep));
        }
    }
    فراغ prepend_Nested_Fork_Branch_کتار_Pushes(Ast_غوټه* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_کرښه* ثابت fl, منطقي needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prepend_Fork_Branch_کتار_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_کرښه* ثابت fl) {
        ثابت خپلکاری it = m_beginDisableQueues.پیدا_کړه(beginp);
        إذا (it != m_beginDisableQueues.پای_ټکی()) أرجع it->دوهم;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_بیان* ثابت ټېل_Current_Processp = get_کتار_ټېل_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ټېل_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_کتار_Pushes(beginBodyp, processQueuep, fl, خطأ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_کرښه* ثابت fl) {
        ثابت خپلکاری it = m_forkDisableQueues.پیدا_کړه(forkp);
        إذا (it != m_forkDisableQueues.پای_ټکی()) أرجع it->دوهم;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_کتار_Pushes(forkp, processQueuep, fl, ریښتیا);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_کتار_Pushes(branchp, processQueuep, fl, ریښتیا);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        أرجع processQueuep;
    }
    Ast_Stmt_بیان* ورننباسه_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_Stmt_بیان* ثابت killStmtp
            = get_کتار_Kill_Stmtp(fl, processQueuep, کتار_Dotted(nodep, processQueuep));
        nodep->جمع_کړه_بل_دلته(killStmtp);
        أرجع killStmtp;
    }
    فراغ جمع_کړه_Jump_After_Kill(Ast_Stmt_بیان* ثابت killStmtp, Ast_غوټه* ثابت targetp) {
        Ast_Jump_بلاک* ثابت jmpBlockp = get_Jump_بلاک(targetp, خطأ);
        killStmtp->جمع_کړه_بل_دلته(جديد AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    فراغ handleDisableOnFork(AstDisable* ثابت nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_کرښه* ثابت fl = nodep->fileline();
        AstFork* ثابت targetp = VN_AS(nodep->targetp(), Fork);
        إذا (m_ftaskp) {
            إذا (!m_ftaskp->exists(
                    [targetp](ثابت Ast_غوټه_بلاک* blockp) -> منطقي { أرجع blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ثابت processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_بیان* ثابت killStmtp = ورننباسه_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        إذا (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            جمع_کړه_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_عدد);
        m_modp = nodep;
        m_mod_Repeat_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ډک);
        m_بلاک_ډېری.ټېل_شا(nodep);
        iterateChildren(nodep);
        m_بلاک_ډېری.راوباسه_شا();
    }
    فراغ لیدنه(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ډک);
        VL_RESTORER(m_دننه_Fork);
        m_دننه_Fork = ریښتیا;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        لكل (Ast_غوټه_بلاک* ثابت blockp : vlstd::سرچپه_view(m_بلاک_ډېری)) {
            إذا (blockp->user3_سېټ_Once()) اكسر;
        }
        m_بلاک_ډېری.ټېل_شا(nodep);
        iterateChildren(nodep);
        m_بلاک_ډېری.راوباسه_شا();
    }
    فراغ لیدنه(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_ډک = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        } وإلا إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_ډک) {
            m_unroll_ډک = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_غوټه_بیان* ثابت countp = nodep->countp()->unlink_Fr_شا_With_بل();
        ثابت string name = "__Vrepeat"s + cvt_منزل_Str(m_mod_Repeat_عدد++);
        AstBegin* ثابت beginp = جديد AstBegin{nodep->fileline(), "", تش_اشاره, ریښتیا};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = جديد AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->پیدا_کړه_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->کارول_شوی_Loop_Idx(ریښتیا);
        beginp->جمع_کړه_Stmtsp(varp);
        Ast_غوټه* initsp = جديد Ast_ټاکل{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_غوټه* ثابت decp = جديد Ast_ټاکل{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            جديد AstSub{nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       جديد AstConst{nodep->fileline(), 1}}};
        Ast_غوټه_بیان* ثابت zerosp = جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_غوټه_بیان* ثابت condp = جديد AstGtS{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_غوټه* ثابت bodysp = nodep->stmtsp();
        إذا (bodysp) bodysp->unlink_Fr_شا_With_بل();
        File_کرښه* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
        loopp->جمع_کړه_Stmtsp(bodysp);
        loopp->جمع_کړه_Contsp(decp);
        إذا (!m_unroll_ډک.isDefault()) loopp->unroll(m_unroll_ډک);
        m_unroll_ډک = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->جمع_کړه_Stmtsp(initsp);
        beginp->جمع_کړه_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstLoop* nodep) override {
        إذا (!m_unroll_ډک.isDefault()) nodep->unroll(m_unroll_ډک);
        إذا (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNUSEDLOOP, ریښتیا);
        }
        m_unroll_ډک = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = خطأ;
        iterate_And_بل_Null(nodep->stmtsp());
        m_loopInc = ریښتیا;
        iterate_And_بل_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        إذا (nodep->contsp()) nodep->جمع_کړه_Stmtsp(nodep->contsp()->unlink_Fr_شا_With_بل());
    }
    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_بل_Null(nodep->bodyp());
    }
    فراغ لیدنه(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        إذا (m_randsequencep) {
            nodep->replaceWith(جديد AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_دننه_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        } وإلا إذا (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } وإلا إذا (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } وإلا إذا (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } وإلا {
            إذا (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->جمع_کړه_دلته_This_As_بل(جديد Ast_ټاکل{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_شا_With_بل()});
            }
            // Jump to the end of the function call
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_ftaskp, خطأ);
            nodep->جمع_کړه_دلته_This_As_بل(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstBreak* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp && m_randsequencep) {
            nodep->replaceWith(جديد AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } وإلا {
            // Jump to the end of the loop
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, خطأ);
            nodep->جمع_کړه_بل_دلته(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstContinue* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } وإلا {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, ریښتیا);
            nodep->جمع_کړه_بل_دلته(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_غوټه* ثابت targetp = nodep->targetp();
        إذا (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        إذا (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_بیان* ثابت killStmtp = ورننباسه_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            إذا (m_ftaskp == taskp) {
                Ast_غوټه* jumpTargetp = taskp;
                ثابت خپلکاری it = m_taskDisableBegins.پیدا_کړه(taskp);
                إذا (it != m_taskDisableBegins.پای_ټکی()) jumpTargetp = it->دوهم;
                جمع_کړه_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } وإلا إذا (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            إذا (exists_بلاک_Above(beginp->name())) {
                إذا (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(beginp, خطأ);
                    nodep->جمع_کړه_بل_دلته(جديد AstJumpGo{nodep->fileline(), blockp});
                } وإلا {
                    AstVar* ثابت processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_بیان* ثابت killStmtp = ورننباسه_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ثابت branchp = innerForkBranchp(beginp);
                    AstBegin* ثابت jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    جمع_کړه_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } وإلا {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ورننباسه_Kill_Stmtp(nodep, processQueuep);
            }
        } وإلا {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_پای* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (m_دننه_Fork) {
            nodep->replaceWith(جديد Ast_پای_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, خطأ);
            nodep->جمع_کړه_بل_دلته(جديد AstJumpGo{nodep->fileline(), blockp});
        }
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (m_loopInc && nodep->varp()) nodep->varp()->کارول_شوی_Loop_Idx(ریښتیا);
    }
    فراغ لیدنه(AstConst*) override {}
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("linkjump", 0, dump_ونه_Either_Level() >= 3);
}
