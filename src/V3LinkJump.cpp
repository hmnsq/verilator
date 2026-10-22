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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkJump.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Error.h"
#شامل "V3UniqueNames.h"

#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده LinkJumpVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;
    ثابت VNUser3InUse m_user3InUse;

    // STATE
    Ast_گره_Module* m_modp = nullptr;  // Current module
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گره* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    منطقی m_loopInc = نادرست;  // In loop increment
    منطقی m_inFork = نادرست;  // Under fork
    صحیح m_mod_Repeat_رقم = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_گره_Block*> m_block_پشته;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
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
    AstJumpBlock* getJumpBlock(Ast_گره* nodep, منطقی endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        اگر (endOfIter) {
            اگر (nodep->user1p()) بازگشت VN_AS(nodep->user1p(), JumpBlock);
        } وگرنه {
            اگر (nodep->user2p()) بازگشت VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_گره* underp = nullptr;
        منطقی under_and_next = درست;
        اگر (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } وگرنه اگر (Ast_گره_FTask* ثابت fTaskp = VN_CAST(nodep, گره_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } وگرنه اگر (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            اگر (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                اگر (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } وگرنه {
                underp = nodep;
                under_and_next = نادرست;  // IE we skip the entire foreach
            }
        } وگرنه اگر (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            اگر (endOfIter) {
                underp = loopp->stmtsp();
            } وگرنه {
                underp = nodep;
                under_and_next = نادرست;  // IE we skip the entire loop
            }
        } وگرنه {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            بازگشت nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        وقتی (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        اگر (!underp->nextp() || !under_and_next) {
            اگر (AstJumpBlock* ثابت blockp = VN_CAST(underp, JumpBlock)) بازگشت blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        اگر (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } وگرنه {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* ثابت blockp = جدید AstJumpBlock{nodep->fileline(), underp};
        اگر (endOfIter) {
            nodep->user1p(blockp);
        } وگرنه {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        برای (Ast_گره *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            اگر (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        بازگشت blockp;
    }
    تهی addPrefixToBlocksRecurse(ثابت std::string& prefix, Ast_گره* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        اگر (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            اگر (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        اگر (Ast_گره* ثابت refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        اگر (Ast_گره* ثابت refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        اگر (Ast_گره* ثابت refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        اگر (Ast_گره* ثابت refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        اگر (Ast_گره* ثابت refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    منطقی existsBlockAbove(ثابت std::string& name) ثابت {
        برای (ثابت Ast_گره_Block* ثابت stackp : vlstd::reverse_view(m_block_پشته)) {
            اگر (stackp->name() == name) بازگشت درست;
        }
        بازگشت نادرست;
    }
    ایستا Ast_گره_Module* findOwnerModulep(Ast_گره* nodep) {
        برای (Ast_گره* curp = nodep; curp; curp = curp->aboveLoopp()) {
            اگر (Ast_گره_Module* ثابت modp = VN_CAST(curp, گره_Module)) بازگشت modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        بازگشت nullptr;  // LCOV_EXCL_LINE
    }
    ایستا std::string targetInstancePath(ثابت AstDisable* ثابت nodep) {
        اگر (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            بازگشت xrefp->dotted();
        }
        بازگشت "";
    }
    ایستا std::string queueDotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        بازگشت processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_گره_Block* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = nullptr;
        Ast_گره_Block* prevBlockp = nullptr;
        برای (Ast_گره_Block* ثابت blockp : vlstd::reverse_view(m_block_پشته)) {
            اگر (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            اگر (blockp == targetp) بازگشت innerForkBranchp;
            prevBlockp = blockp;
        }
        بازگشت nullptr;
    }
    ایستا Ast_Stmt_عبارت* get_Queue_افزودن_Process_Selfp(AstVarRef* ثابت queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_سطر* ثابت flp = queueRefp->fileline();
        بازگشت جدید Ast_Stmt_عبارت{
            flp,
            جدید AstMethodCall{flp, queueRefp, "push_back",
                              جدید AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ایستا Ast_گره_Var_Ref* newQueueRefp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        اگر (dotted != "") بازگشت جدید AstVarXRef{fl, processQueuep, dotted, access};
        اگر (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            بازگشت جدید AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت بالاترین_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        بازگشت جدید AstVarRef{fl, بالاترین_Pkgp, processQueuep, access};
    }
    ایستا Ast_Stmt_عبارت* get_Queue_افزودن_Process_Selfp(File_سطر* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        بازگشت get_Queue_افزودن_Process_Selfp(queueWriteRefp);
    }
    ایستا Ast_Stmt_عبارت* getQueueKillStmtp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_گره_Var_Ref* ثابت queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        برای (Ast_گره* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            اگر (itemp->name() == "killQueue") {
                killQueueCall
                    = جدید AstTaskRef{fl, VN_AS(itemp, Task), جدید AstArg{fl, "", queueRefp}};
                بشکن;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        بازگشت جدید Ast_Stmt_عبارت{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت auto it = m_taskDisableBegins.find(taskp);
        اگر (it != m_taskDisableBegins.end()) بازگشت it->second;

        AstBegin* ثابت taskBodyp = جدید AstBegin{fl, "", nullptr, نادرست};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        برای (Ast_گره* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_گره* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            اگر (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = جدید AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        بازگشت taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_گره* ثابت nodep, File_سطر* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = جدید AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            جدید AstQueueDType{
                fl, VFlagChildDType{},
                جدید AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(درست);
        processQueuep->setIgnoreSchedWrite();
        بازگشت processQueuep;
    }
    AstVar* getProcessQueuep(Ast_گره* ثابت nodep, File_سطر* ثابت fl) {
        Ast_گره_Module* ثابت ownerp = findOwnerModulep(nodep);

        اگر (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت بالاترین_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            بالاترین_Pkgp->addStmtsp(processQueuep);
            بازگشت processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        بازگشت processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت auto it = m_taskDisableQueues.find(taskp);
        اگر (it != m_taskDisableQueues.end()) بازگشت it->second;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_عبارت* ثابت افزودن_Current_Processp = get_Queue_افزودن_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(افزودن_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        بازگشت processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت auto it = m_beginDisableBegins.find(beginp);
        اگر (it != m_beginDisableBegins.end()) بازگشت it->second;

        AstBegin* ثابت beginBodyp = جدید AstBegin{fl, "", nullptr, نادرست};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        اگر (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* ثابت forkp = جدید AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        بازگشت beginBodyp;
    }
    تهی prependForkBranchQueuePushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_سطر* ثابت fl, منطقی needProcess) {
        برای (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            اگر (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_افزودن_Process_Selfp(fl, processQueuep));
        }
    }
    تهی prependNestedForkBranchQueuePushes(Ast_گره* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_سطر* ثابت fl, منطقی needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت auto it = m_beginDisableQueues.find(beginp);
        اگر (it != m_beginDisableQueues.end()) بازگشت it->second;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_عبارت* ثابت افزودن_Current_Processp = get_Queue_افزودن_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(افزودن_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, نادرست);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        بازگشت processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_سطر* ثابت fl) {
        ثابت auto it = m_forkDisableQueues.find(forkp);
        اگر (it != m_forkDisableQueues.end()) بازگشت it->second;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, درست);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        برای (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, درست);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        بازگشت processQueuep;
    }
    Ast_Stmt_عبارت* درج_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_Stmt_عبارت* ثابت killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        بازگشت killStmtp;
    }
    تهی addJumpAfterKill(Ast_Stmt_عبارت* ثابت killStmtp, Ast_گره* ثابت targetp) {
        AstJumpBlock* ثابت jmpBlockp = getJumpBlock(targetp, نادرست);
        killStmtp->addNextHere(جدید AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    تهی handleDisableOnFork(AstDisable* ثابت nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_سطر* ثابت fl = nodep->fileline();
        AstFork* ثابت targetp = VN_AS(nodep->targetp(), Fork);
        اگر (m_ftaskp) {
            اگر (!m_ftaskp->exists(
                    [targetp](ثابت Ast_گره_Block* blockp) -> منطقی { بازگشت blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ثابت processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_عبارت* ثابت killStmtp = درج_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        اگر (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (nodep->dead()) بازگشت;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_رقم);
        m_modp = nodep;
        m_mod_Repeat_رقم = 0;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_پشته.افزودن_back(nodep);
        iterateChildren(nodep);
        m_block_پشته.برداشت_back();
    }
    تهی visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = درست;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        برای (Ast_گره_Block* ثابت blockp : vlstd::reverse_view(m_block_پشته)) {
            اگر (blockp->user3SetOnce()) بشکن;
        }
        m_block_پشته.افزودن_back(nodep);
        iterateChildren(nodep);
        m_block_پشته.برداشت_back();
    }
    تهی visit(AstStmtPragma* nodep) override {
        اگر (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        } وگرنه اگر (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        } وگرنه {
            iterateChildren(nodep);
        }
    }
    تهی visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_گره_عبارت* ثابت countp = nodep->countp()->unlinkFrBackWithNext();
        ثابت string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_رقم++);
        AstBegin* ثابت beginp = جدید AstBegin{nodep->fileline(), "", nullptr, درست};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = جدید AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(درست);
        beginp->addStmtsp(varp);
        Ast_گره* initsp = جدید AstAssign{
            nodep->fileline(), جدید AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_گره* ثابت decp = جدید AstAssign{
            nodep->fileline(), جدید AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            جدید AstSub{nodep->fileline(), جدید AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       جدید AstConst{nodep->fileline(), 1}}};
        Ast_گره_عبارت* ثابت zerosp = جدید AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_گره_عبارت* ثابت condp = جدید AstGtS{
            nodep->fileline(), جدید AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_گره* ثابت bodysp = nodep->stmtsp();
        اگر (bodysp) bodysp->unlinkFrBackWithNext();
        File_سطر* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        loopp->addStmtsp(جدید AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        اگر (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstLoop* nodep) override {
        اگر (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        اگر (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, درست);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = نادرست;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = درست;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        اگر (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    تهی visit(Ast_گره_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    تهی visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        اگر (m_randsequencep) {
            nodep->replaceWith(جدید AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        } وگرنه اگر (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        } وگرنه اگر (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } وگرنه اگر (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } وگرنه اگر (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } وگرنه {
            اگر (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(جدید AstAssign{
                    nodep->fileline(),
                    جدید AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* ثابت blockp = getJumpBlock(m_ftaskp, نادرست);
            nodep->addHereThisAsNext(جدید AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        اگر (!m_loopp && m_randsequencep) {
            nodep->replaceWith(جدید AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        } وگرنه اگر (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } وگرنه {
            // Jump to the end of the loop
            AstJumpBlock* ثابت blockp = getJumpBlock(m_loopp, نادرست);
            nodep->addNextHere(جدید AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        اگر (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } وگرنه {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* ثابت blockp = getJumpBlock(m_loopp, درست);
            nodep->addNextHere(جدید AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_گره* ثابت targetp = nodep->targetp();
        اگر (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        اگر (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_عبارت* ثابت killStmtp = درج_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            اگر (m_ftaskp == taskp) {
                Ast_گره* jumpTargetp = taskp;
                ثابت auto it = m_taskDisableBegins.find(taskp);
                اگر (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } وگرنه اگر (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } وگرنه اگر (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            اگر (existsBlockAbove(beginp->name())) {
                اگر (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* ثابت blockp = getJumpBlock(beginp, نادرست);
                    nodep->addNextHere(جدید AstJumpGo{nodep->fileline(), blockp});
                } وگرنه {
                    AstVar* ثابت processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_عبارت* ثابت killStmtp = درج_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ثابت branchp = innerForkBranchp(beginp);
                    AstBegin* ثابت jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } وگرنه {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                درج_Kill_Stmtp(nodep, processQueuep);
            }
        } وگرنه {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstFinish* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        اگر (m_inFork) {
            nodep->replaceWith(جدید AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* ثابت blockp = getJumpBlock(m_loopp, نادرست);
            nodep->addNextHere(جدید AstJumpGo{nodep->fileline(), blockp});
        }
    }
    تهی visit(AstVarRef* nodep) override {
        اگر (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(درست);
    }
    تهی visit(AstConst*) override {}
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = پیشفرض;
};

//######################################################################
// Task class functions

تهی V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkjump", 0, dump_درخت_Either_Level() >= 3);
}
