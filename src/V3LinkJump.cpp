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
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;
    ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;
    ثابت VNUser3_اندريون_Use m_user3_اندريون_Use;

    // STATE
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    Ast_ڳنڍ* m_loopp = خالي_اشارو;  // Current loop
    AstRandSequence* m_randsequencep = خالي_اشارو;  // Current randsequence
    منطقي m_loopInc = خطأ;  // In loop increment
    منطقي m_اندريون_Fork = خطأ;  // Under fork
    صحيح m_mod_Repeat_انگ = 0;  // Repeat counter
    VOptionBool m_unroll_ڀريل;  // Pragma full, disable, or default unrolling
    std::ويڪٽر<Ast_ڳنڍ_بلاڪ*> m_بلاڪ_ڍير;  // All begin blocks above current node
    V3_Unique_نالا m_قطار_نالا{
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
    Ast_Jump_بلاڪ* get_Jump_بلاڪ(Ast_ڳنڍ* nodep, منطقي پڇاڙي_جو_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        إذا (پڇاڙي_جو_Iter) {
            إذا (nodep->user1p()) أرجع VN_AS(nodep->user1p(), Jump_بلاڪ);
        } وإلا {
            إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), Jump_بلاڪ);
        }

        Ast_ڳنڍ* underp = خالي_اشارو;
        منطقي under_and_ايندڙ = سچ;
        إذا (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!پڇاڙي_جو_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } وإلا إذا (Ast_ڳنڍ_FTask* ثابت fTaskp = VN_CAST(nodep, ڳنڍ_FTask)) {
            UASSERT_OBJ(!پڇاڙي_جو_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } وإلا إذا (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            إذا (پڇاڙي_جو_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                إذا (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } وإلا {
                underp = nodep;
                under_and_ايندڙ = خطأ;  // IE we skip the entire foreach
            }
        } وإلا إذا (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            إذا (پڇاڙي_جو_Iter) {
                underp = loopp->stmtsp();
            } وإلا {
                underp = nodep;
                under_and_ايندڙ = خطأ;  // IE we skip the entire loop
            }
        } وإلا {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            أرجع خالي_اشارو;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        طالما (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        إذا (!underp->nextp() || !under_and_ايندڙ) {
            إذا (Ast_Jump_بلاڪ* ثابت blockp = VN_CAST(underp, Jump_بلاڪ)) أرجع blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        إذا (under_and_ايندڙ) {
            underp->unlink_Fr_پٺ_With_ايندڙ(&repHandle);
        } وإلا {
            underp->unlink_Fr_پٺ(&repHandle);
        }
        Ast_Jump_بلاڪ* ثابت blockp = جديد Ast_Jump_بلاڪ{nodep->fileline(), underp};
        إذا (پڇاڙي_جو_Iter) {
            nodep->user1p(blockp);
        } وإلا {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        لكل (Ast_ڳنڍ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            إذا (VN_IS(varp, Var)) blockp->جوڙيو_هتي_This_As_ايندڙ(varp->unlink_Fr_پٺ());
        }
        أرجع blockp;
    }
    فراغ جوڙيو_اڳياڙي_منزل_بلاڪون_Recurse(ثابت std::string& اڳياڙي, Ast_ڳنڍ* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->name() != "") beginp->name(اڳياڙي + beginp->name());
        }

        إذا (Ast_ڳنڍ* ثابت refp = nodep->op1p()) جوڙيو_اڳياڙي_منزل_بلاڪون_Recurse(اڳياڙي, refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op2p()) جوڙيو_اڳياڙي_منزل_بلاڪون_Recurse(اڳياڙي, refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op3p()) جوڙيو_اڳياڙي_منزل_بلاڪون_Recurse(اڳياڙي, refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op4p()) جوڙيو_اڳياڙي_منزل_بلاڪون_Recurse(اڳياڙي, refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->nextp()) جوڙيو_اڳياڙي_منزل_بلاڪون_Recurse(اڳياڙي, refp);
    }
    منطقي exists_بلاڪ_Above(ثابت std::string& name) ثابت {
        لكل (ثابت Ast_ڳنڍ_بلاڪ* ثابت stackp : vlstd::ابتو_view(m_بلاڪ_ڍير)) {
            إذا (stackp->name() == name) أرجع سچ;
        }
        أرجع خطأ;
    }
    ساكن Ast_ڳنڍ_Module* ڳوليو_Owner_Modulep(Ast_ڳنڍ* nodep) {
        لكل (Ast_ڳنڍ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            إذا (Ast_ڳنڍ_Module* ثابت modp = VN_CAST(curp, ڳنڍ_Module)) أرجع modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        أرجع خالي_اشارو;  // LCOV_EXCL_LINE
    }
    ساكن std::string هدف_Instance_رستو(ثابت AstDisable* ثابت nodep) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->هدف_Refp(), VarXRef)) {
            أرجع xrefp->dotted();
        }
        أرجع "";
    }
    ساكن std::string قطار_Dotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        أرجع processQueuep->is_عارضي() ? هدف_Instance_رستو(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_ڳنڍ_بلاڪ* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = خالي_اشارو;
        Ast_ڳنڍ_بلاڪ* prevBlockp = خالي_اشارو;
        لكل (Ast_ڳنڍ_بلاڪ* ثابت blockp : vlstd::ابتو_view(m_بلاڪ_ڍير)) {
            إذا (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            إذا (blockp == targetp) أرجع innerForkBranchp;
            prevBlockp = blockp;
        }
        أرجع خالي_اشارو;
    }
    ساكن Ast_Stmt_اظهار* get_قطار_ڌڪو_Process_Selfp(AstVarRef* ثابت قطار_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_سٽ* ثابت flp = قطار_Refp->fileline();
        أرجع جديد Ast_Stmt_اظهار{
            flp,
            جديد Ast_طريقو_Call{flp, قطار_Refp, "push_back",
                              جديد AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ساكن Ast_ڳنڍ_Var_Ref* new_قطار_Refp(File_سٽ* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        إذا (dotted != "") أرجع جديد AstVarXRef{fl, processQueuep, dotted, access};
        إذا (!processQueuep->lifetime().isStatic() || processQueuep->is_عارضي()) {
            أرجع جديد AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت چوٽي_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        أرجع جديد AstVarRef{fl, چوٽي_Pkgp, processQueuep, access};
    }
    ساكن Ast_Stmt_اظهار* get_قطار_ڌڪو_Process_Selfp(File_سٽ* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت قطار_Write_Refp
            = VN_AS(new_قطار_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        أرجع get_قطار_ڌڪو_Process_Selfp(قطار_Write_Refp);
    }
    ساكن Ast_Stmt_اظهار* get_قطار_Kill_Stmtp(File_سٽ* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_ڳنڍ_Var_Ref* ثابت قطار_Refp
            = new_قطار_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_قطار_Call = خالي_اشارو;
        لكل (Ast_ڳنڍ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            إذا (itemp->name() == "killQueue") {
                kill_قطار_Call
                    = جديد AstTaskRef{fl, VN_AS(itemp, Task), جديد AstArg{fl, "", قطار_Refp}};
                اكسر;
            }
        }
        UASSERT(kill_قطار_Call, "Should be found");
        kill_قطار_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        أرجع جديد Ast_Stmt_اظهار{fl, kill_قطار_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_سٽ* ثابت fl) {
        ثابت خودڪار it = m_taskDisableBegins.ڳوليو(taskp);
        إذا (it != m_taskDisableBegins.پڇاڙي()) أرجع it->ٻيو;

        AstBegin* ثابت taskBodyp = جديد AstBegin{fl, "", خالي_اشارو, خطأ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->سيٽ_Need_Process();
        v3Global.سيٽ_استعمال_ڪري_Timing();
        // Ports stay in the task so calls see them
        لكل (Ast_ڳنڍ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ڳنڍ* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) taskBodyp->جوڙيو_Stmtsp(stmtp->unlink_Fr_پٺ());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جوڙيو_Forksp(taskBodyp);
        taskp->جوڙيو_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        أرجع taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ڳنڍ* ثابت nodep, File_سٽ* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = جديد AstVar{
            fl, varType, m_قطار_نالا.get(nodep->name()), VFlag_ٻار_DType{},
            جديد Ast_قطار_DType{
                fl, VFlag_ٻار_DType{},
                جديد AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), خالي_اشارو},
                خالي_اشارو}};
        processQueuep->process_قطار(سچ);
        processQueuep->سيٽ_Ignore_Sched_Write();
        أرجع processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ڳنڍ* ثابت nodep, File_سٽ* ثابت fl) {
        Ast_ڳنڍ_Module* ثابت ownerp = ڳوليو_Owner_Modulep(nodep);

        إذا (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت چوٽي_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            چوٽي_Pkgp->جوڙيو_Stmtsp(processQueuep);
            أرجع processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->جوڙيو_Stmtsp(processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_سٽ* ثابت fl) {
        ثابت خودڪار it = m_taskDisableQueues.ڳوليو(taskp);
        إذا (it != m_taskDisableQueues.پڇاڙي()) أرجع it->ٻيو;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_اظهار* ثابت ڌڪو_Current_Processp = get_قطار_ڌڪو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ڌڪو_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        أرجع processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_سٽ* ثابت fl) {
        ثابت خودڪار it = m_beginDisableBegins.ڳوليو(beginp);
        إذا (it != m_beginDisableBegins.پڇاڙي()) أرجع it->ٻيو;

        AstBegin* ثابت beginBodyp = جديد AstBegin{fl, "", خالي_اشارو, خطأ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->سيٽ_Need_Process();
        v3Global.سيٽ_استعمال_ڪري_Timing();
        إذا (beginp->stmtsp()) beginBodyp->جوڙيو_Stmtsp(beginp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جوڙيو_Forksp(beginBodyp);
        beginp->جوڙيو_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        أرجع beginBodyp;
    }
    فراغ prepend_Fork_Branch_قطار_Pushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_سٽ* ثابت fl, منطقي needProcess) {
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            إذا (needProcess) branchp->سيٽ_Need_Process();
            branchp->prependStmtsp(get_قطار_ڌڪو_Process_Selfp(fl, processQueuep));
        }
    }
    فراغ prepend_Nested_Fork_Branch_قطار_Pushes(Ast_ڳنڍ* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_سٽ* ثابت fl, منطقي needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prepend_Fork_Branch_قطار_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_سٽ* ثابت fl) {
        ثابت خودڪار it = m_beginDisableQueues.ڳوليو(beginp);
        إذا (it != m_beginDisableQueues.پڇاڙي()) أرجع it->ٻيو;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_اظهار* ثابت ڌڪو_Current_Processp = get_قطار_ڌڪو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ڌڪو_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_قطار_Pushes(beginBodyp, processQueuep, fl, خطأ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_سٽ* ثابت fl) {
        ثابت خودڪار it = m_forkDisableQueues.ڳوليو(forkp);
        إذا (it != m_forkDisableQueues.پڇاڙي()) أرجع it->ٻيو;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_قطار_Pushes(forkp, processQueuep, fl, سچ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_قطار_Pushes(branchp, processQueuep, fl, سچ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        أرجع processQueuep;
    }
    Ast_Stmt_اظهار* وجهو_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_Stmt_اظهار* ثابت killStmtp
            = get_قطار_Kill_Stmtp(fl, processQueuep, قطار_Dotted(nodep, processQueuep));
        nodep->جوڙيو_ايندڙ_هتي(killStmtp);
        أرجع killStmtp;
    }
    فراغ جوڙيو_Jump_After_Kill(Ast_Stmt_اظهار* ثابت killStmtp, Ast_ڳنڍ* ثابت targetp) {
        Ast_Jump_بلاڪ* ثابت jmpBlockp = get_Jump_بلاڪ(targetp, خطأ);
        killStmtp->جوڙيو_ايندڙ_هتي(جديد AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    فراغ handleDisableOnFork(AstDisable* ثابت nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_سٽ* ثابت fl = nodep->fileline();
        AstFork* ثابت targetp = VN_AS(nodep->targetp(), Fork);
        إذا (m_ftaskp) {
            إذا (!m_ftaskp->exists(
                    [targetp](ثابت Ast_ڳنڍ_بلاڪ* blockp) -> منطقي { أرجع blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ثابت processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_اظهار* ثابت killStmtp = وجهو_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        إذا (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            جوڙيو_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_انگ);
        m_modp = nodep;
        m_mod_Repeat_انگ = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ڀريل);
        m_بلاڪ_ڍير.ڌڪو_پٺ(nodep);
        iterateChildren(nodep);
        m_بلاڪ_ڍير.ڪڍو_پٺ();
    }
    فراغ گهمو(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ڀريل);
        VL_RESTORER(m_اندريون_Fork);
        m_اندريون_Fork = سچ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        لكل (Ast_ڳنڍ_بلاڪ* ثابت blockp : vlstd::ابتو_view(m_بلاڪ_ڍير)) {
            إذا (blockp->user3_سيٽ_Once()) اكسر;
        }
        m_بلاڪ_ڍير.ڌڪو_پٺ(nodep);
        iterateChildren(nodep);
        m_بلاڪ_ڍير.ڪڍو_پٺ();
    }
    فراغ گهمو(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_ڀريل = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        } وإلا إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_ڀريل) {
            m_unroll_ڀريل = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ڳنڍ_اظهار* ثابت countp = nodep->countp()->unlink_Fr_پٺ_With_ايندڙ();
        ثابت string name = "__Vrepeat"s + cvt_منزل_Str(m_mod_Repeat_انگ++);
        AstBegin* ثابت beginp = جديد AstBegin{nodep->fileline(), "", خالي_اشارو, سچ};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = جديد AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->ڳوليو_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->استعمال_ٿيل_Loop_Idx(سچ);
        beginp->جوڙيو_Stmtsp(varp);
        Ast_ڳنڍ* initsp = جديد Ast_سونپيو{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ڳنڍ* ثابت decp = جديد Ast_سونپيو{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            جديد AstSub{nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       جديد AstConst{nodep->fileline(), 1}}};
        Ast_ڳنڍ_اظهار* ثابت zerosp = جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ڳنڍ_اظهار* ثابت condp = جديد AstGtS{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ڳنڍ* ثابت bodysp = nodep->stmtsp();
        إذا (bodysp) bodysp->unlink_Fr_پٺ_With_ايندڙ();
        File_سٽ* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
        loopp->جوڙيو_Stmtsp(bodysp);
        loopp->جوڙيو_Contsp(decp);
        إذا (!m_unroll_ڀريل.isDefault()) loopp->unroll(m_unroll_ڀريل);
        m_unroll_ڀريل = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->جوڙيو_Stmtsp(initsp);
        beginp->جوڙيو_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstLoop* nodep) override {
        إذا (!m_unroll_ڀريل.isDefault()) nodep->unroll(m_unroll_ڀريل);
        إذا (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_ڪوڊ::UNUSEDLOOP, سچ);
        }
        m_unroll_ڀريل = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = خطأ;
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        m_loopInc = سچ;
        iterate_And_ايندڙ_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        إذا (nodep->contsp()) nodep->جوڙيو_Stmtsp(nodep->contsp()->unlink_Fr_پٺ_With_ايندڙ());
    }
    فراغ گهمو(Ast_ڳنڍ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_ايندڙ_Null(nodep->bodyp());
    }
    فراغ گهمو(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        إذا (m_randsequencep) {
            nodep->replaceWith(جديد AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_اندريون_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
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
                nodep->جوڙيو_هتي_This_As_ايندڙ(جديد Ast_سونپيو{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_پٺ_With_ايندڙ()});
            }
            // Jump to the end of the function call
            Ast_Jump_بلاڪ* ثابت blockp = get_Jump_بلاڪ(m_ftaskp, خطأ);
            nodep->جوڙيو_هتي_This_As_ايندڙ(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstBreak* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp && m_randsequencep) {
            nodep->replaceWith(جديد AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } وإلا {
            // Jump to the end of the loop
            Ast_Jump_بلاڪ* ثابت blockp = get_Jump_بلاڪ(m_loopp, خطأ);
            nodep->جوڙيو_ايندڙ_هتي(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstContinue* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } وإلا {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_بلاڪ* ثابت blockp = get_Jump_بلاڪ(m_loopp, سچ);
            nodep->جوڙيو_ايندڙ_هتي(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ڳنڍ* ثابت targetp = nodep->targetp();
        إذا (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        إذا (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_اظهار* ثابت killStmtp = وجهو_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            إذا (m_ftaskp == taskp) {
                Ast_ڳنڍ* jumpTargetp = taskp;
                ثابت خودڪار it = m_taskDisableBegins.ڳوليو(taskp);
                إذا (it != m_taskDisableBegins.پڇاڙي()) jumpTargetp = it->ٻيو;
                جوڙيو_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } وإلا إذا (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            إذا (exists_بلاڪ_Above(beginp->name())) {
                إذا (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_بلاڪ* ثابت blockp = get_Jump_بلاڪ(beginp, خطأ);
                    nodep->جوڙيو_ايندڙ_هتي(جديد AstJumpGo{nodep->fileline(), blockp});
                } وإلا {
                    AstVar* ثابت processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_اظهار* ثابت killStmtp = وجهو_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ثابت branchp = innerForkBranchp(beginp);
                    AstBegin* ثابت jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    جوڙيو_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } وإلا {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                وجهو_Kill_Stmtp(nodep, processQueuep);
            }
        } وإلا {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_پڄاڻي* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (m_اندريون_Fork) {
            nodep->replaceWith(جديد Ast_پڄاڻي_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        } وإلا إذا (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_بلاڪ* ثابت blockp = get_Jump_بلاڪ(m_loopp, خطأ);
            nodep->جوڙيو_ايندڙ_هتي(جديد AstJumpGo{nodep->fileline(), blockp});
        }
    }
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (m_loopInc && nodep->varp()) nodep->varp()->استعمال_ٿيل_Loop_Idx(سچ);
    }
    فراغ گهمو(AstConst*) override {}
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("linkjump", 0, dump_وڻ_Either_Level() >= 3);
}
