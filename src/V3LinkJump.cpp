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
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;
    ثابت VNUser3InUse m_user3InUse;

    // STATE
    Ast_گرہ_Module* m_modp = nullptr;  // Current module
    Ast_گرہ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گرہ* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    منطقي m_loopInc = خطأ;  // In loop increment
    منطقي m_inFork = خطأ;  // Under fork
    صحيح m_mod_Repeat_عدد = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_گرہ_Block*> m_block_ڈھیر;  // All begin blocks above current node
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
    AstJumpBlock* getJumpBlock(Ast_گرہ* nodep, منطقي endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        إذا (endOfIter) {
            إذا (nodep->user1p()) أرجع VN_AS(nodep->user1p(), JumpBlock);
        } وإلا {
            إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_گرہ* underp = nullptr;
        منطقي under_and_next = true;
        إذا (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } وإلا إذا (Ast_گرہ_FTask* ثابت fTaskp = VN_CAST(nodep, گرہ_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } وإلا إذا (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            إذا (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                إذا (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } وإلا {
                underp = nodep;
                under_and_next = خطأ;  // IE we skip the entire foreach
            }
        } وإلا إذا (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            إذا (endOfIter) {
                underp = loopp->stmtsp();
            } وإلا {
                underp = nodep;
                under_and_next = خطأ;  // IE we skip the entire loop
            }
        } وإلا {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            أرجع nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        طالما (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        إذا (!underp->nextp() || !under_and_next) {
            إذا (AstJumpBlock* ثابت blockp = VN_CAST(underp, JumpBlock)) أرجع blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        إذا (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } وإلا {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* ثابت blockp = جديد AstJumpBlock{nodep->fileline(), underp};
        إذا (endOfIter) {
            nodep->user1p(blockp);
        } وإلا {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        لكل (Ast_گرہ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            إذا (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        أرجع blockp;
    }
    فراغ addPrefixToBlocksRecurse(ثابت std::string& prefix, Ast_گرہ* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        إذا (Ast_گرہ* ثابت refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    منطقي existsBlockAbove(ثابت std::string& name) ثابت {
        لكل (ثابت Ast_گرہ_Block* ثابت stackp : vlstd::reverse_view(m_block_ڈھیر)) {
            إذا (stackp->name() == name) أرجع true;
        }
        أرجع خطأ;
    }
    ساكن Ast_گرہ_Module* findOwnerModulep(Ast_گرہ* nodep) {
        لكل (Ast_گرہ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            إذا (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) أرجع modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        أرجع nullptr;  // LCOV_EXCL_LINE
    }
    ساكن std::string targetInstancePath(ثابت AstDisable* ثابت nodep) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            أرجع xrefp->dotted();
        }
        أرجع "";
    }
    ساكن std::string queueDotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        أرجع processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_گرہ_Block* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = nullptr;
        Ast_گرہ_Block* prevBlockp = nullptr;
        لكل (Ast_گرہ_Block* ثابت blockp : vlstd::reverse_view(m_block_ڈھیر)) {
            إذا (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            إذا (blockp == targetp) أرجع innerForkBranchp;
            prevBlockp = blockp;
        }
        أرجع nullptr;
    }
    ساكن Ast_Stmt_اظہاریہ* get_Queue_ڈالو_Process_Selfp(AstVarRef* ثابت queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_سطر* ثابت flp = queueRefp->fileline();
        أرجع جديد Ast_Stmt_اظہاریہ{
            flp,
            جديد AstMethodCall{flp, queueRefp, "push_back",
                              جديد AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ساكن Ast_گرہ_Var_Ref* newQueueRefp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        إذا (dotted != "") أرجع جديد AstVarXRef{fl, processQueuep, dotted, access};
        إذا (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            أرجع جديد AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت چوٹی_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        أرجع جديد AstVarRef{fl, چوٹی_Pkgp, processQueuep, access};
    }
    ساكن Ast_Stmt_اظہاریہ* get_Queue_ڈالو_Process_Selfp(File_سطر* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        أرجع get_Queue_ڈالو_Process_Selfp(queueWriteRefp);
    }
    ساكن Ast_Stmt_اظہاریہ* getQueueKillStmtp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_گرہ_Var_Ref* ثابت queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        لكل (Ast_گرہ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            إذا (itemp->name() == "killQueue") {
                killQueueCall
                    = جديد AstTaskRef{fl, VN_AS(itemp, Task), جديد AstArg{fl, "", queueRefp}};
                اكسر;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        أرجع جديد Ast_Stmt_اظہاریہ{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت auto it = m_taskDisableBegins.find(taskp);
        إذا (it != m_taskDisableBegins.end()) أرجع it->second;

        AstBegin* ثابت taskBodyp = جديد AstBegin{fl, "", nullptr, خطأ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        لكل (Ast_گرہ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_گرہ* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        أرجع taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_گرہ* ثابت nodep, File_سطر* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = جديد AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            جديد AstQueueDType{
                fl, VFlagChildDType{},
                جديد AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(true);
        processQueuep->setIgnoreSchedWrite();
        أرجع processQueuep;
    }
    AstVar* getProcessQueuep(Ast_گرہ* ثابت nodep, File_سطر* ثابت fl) {
        Ast_گرہ_Module* ثابت ownerp = findOwnerModulep(nodep);

        إذا (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت چوٹی_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            چوٹی_Pkgp->addStmtsp(processQueuep);
            أرجع processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت auto it = m_taskDisableQueues.find(taskp);
        إذا (it != m_taskDisableQueues.end()) أرجع it->second;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_اظہاریہ* ثابت ڈالو_Current_Processp = get_Queue_ڈالو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ڈالو_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        أرجع processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت auto it = m_beginDisableBegins.find(beginp);
        إذا (it != m_beginDisableBegins.end()) أرجع it->second;

        AstBegin* ثابت beginBodyp = جديد AstBegin{fl, "", nullptr, خطأ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        إذا (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        أرجع beginBodyp;
    }
    فراغ prependForkBranchQueuePushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_سطر* ثابت fl, منطقي needProcess) {
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            إذا (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ڈالو_Process_Selfp(fl, processQueuep));
        }
    }
    فراغ prependNestedForkBranchQueuePushes(Ast_گرہ* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_سطر* ثابت fl, منطقي needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت auto it = m_beginDisableQueues.find(beginp);
        إذا (it != m_beginDisableQueues.end()) أرجع it->second;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_اظہاریہ* ثابت ڈالو_Current_Processp = get_Queue_ڈالو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ڈالو_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, خطأ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_سطر* ثابت fl) {
        ثابت auto it = m_forkDisableQueues.find(forkp);
        إذا (it != m_forkDisableQueues.end()) أرجع it->second;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, true);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, true);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        أرجع processQueuep;
    }
    Ast_Stmt_اظہاریہ* داخل_کرو_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_Stmt_اظہاریہ* ثابت killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        أرجع killStmtp;
    }
    فراغ addJumpAfterKill(Ast_Stmt_اظہاریہ* ثابت killStmtp, Ast_گرہ* ثابت targetp) {
        AstJumpBlock* ثابت jmpBlockp = getJumpBlock(targetp, خطأ);
        killStmtp->addNextHere(جديد AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    فراغ handleDisableOnFork(AstDisable* ثابت nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_سطر* ثابت fl = nodep->fileline();
        AstFork* ثابت targetp = VN_AS(nodep->targetp(), Fork);
        إذا (m_ftaskp) {
            إذا (!m_ftaskp->exists(
                    [targetp](ثابت Ast_گرہ_Block* blockp) -> منطقي { أرجع blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ثابت processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        إذا (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    فراغ visit(Ast_گرہ_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_عدد);
        m_modp = nodep;
        m_mod_Repeat_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_ڈھیر.ڈالو_back(nodep);
        iterateChildren(nodep);
        m_block_ڈھیر.نکالو_back();
    }
    فراغ visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = true;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        لكل (Ast_گرہ_Block* ثابت blockp : vlstd::reverse_view(m_block_ڈھیر)) {
            إذا (blockp->user3SetOnce()) اكسر;
        }
        m_block_ڈھیر.ڈالو_back(nodep);
        iterateChildren(nodep);
        m_block_ڈھیر.نکالو_back();
    }
    فراغ visit(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlinkFrBack()), nodep);
        } وإلا إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlinkFrBack()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_گرہ_اظہاریہ* ثابت countp = nodep->countp()->unlinkFrBackWithNext();
        ثابت string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_عدد++);
        AstBegin* ثابت beginp = جديد AstBegin{nodep->fileline(), "", nullptr, true};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = جديد AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(true);
        beginp->addStmtsp(varp);
        Ast_گرہ* initsp = جديد AstAssign{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_گرہ* ثابت decp = جديد AstAssign{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            جديد AstSub{nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       جديد AstConst{nodep->fileline(), 1}}};
        Ast_گرہ_اظہاریہ* ثابت zerosp = جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_گرہ_اظہاریہ* ثابت condp = جديد AstGtS{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_گرہ* ثابت bodysp = nodep->stmtsp();
        إذا (bodysp) bodysp->unlinkFrBackWithNext();
        File_سطر* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        إذا (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ visit(AstLoop* nodep) override {
        إذا (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        إذا (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, true);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = خطأ;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = true;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        إذا (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    فراغ visit(Ast_گرہ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    فراغ visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        إذا (m_randsequencep) {
            nodep->replaceWith(جديد AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlinkFrBack()), nodep);
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
                nodep->addHereThisAsNext(جديد AstAssign{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* ثابت blockp = getJumpBlock(m_ftaskp, خطأ);
            nodep->addHereThisAsNext(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp && m_randsequencep) {
            nodep->replaceWith(جديد AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } وإلا {
            // Jump to the end of the loop
            AstJumpBlock* ثابت blockp = getJumpBlock(m_loopp, خطأ);
            nodep->addNextHere(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } وإلا {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* ثابت blockp = getJumpBlock(m_loopp, true);
            nodep->addNextHere(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_گرہ* ثابت targetp = nodep->targetp();
        إذا (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        إذا (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            إذا (m_ftaskp == taskp) {
                Ast_گرہ* jumpTargetp = taskp;
                ثابت auto it = m_taskDisableBegins.find(taskp);
                إذا (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } وإلا إذا (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            إذا (existsBlockAbove(beginp->name())) {
                إذا (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* ثابت blockp = getJumpBlock(beginp, خطأ);
                    nodep->addNextHere(جديد AstJumpGo{nodep->fileline(), blockp});
                } وإلا {
                    AstVar* ثابت processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ثابت branchp = innerForkBranchp(beginp);
                    AstBegin* ثابت jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } وإلا {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                داخل_کرو_Kill_Stmtp(nodep, processQueuep);
            }
        } وإلا {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ visit(AstFinish* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (m_inFork) {
            nodep->replaceWith(جديد AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* ثابت blockp = getJumpBlock(m_loopp, خطأ);
            nodep->addNextHere(جديد AstJumpGo{nodep->fileline(), blockp});
        }
    }
    فراغ visit(AstVarRef* nodep) override {
        إذا (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(true);
    }
    فراغ visit(AstConst*) override {}
    فراغ visit(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkjump", 0, dump_درخت_Either_Level() >= 3);
}
