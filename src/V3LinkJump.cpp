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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkJump.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ LinkJumpVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    અચલ VNUser1InUse m_user1InUse;
    અચલ VNUser2InUse m_user2InUse;
    અચલ VNUser3InUse m_user3InUse;

    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ગાંઠ* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    તાર્કિક m_loopInc = ખોટું;  // In loop increment
    તાર્કિક m_inFork = ખોટું;  // Under fork
    પૂર્ણાંક m_mod_Repeat_અંક = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_ગાંઠ_Block*> m_block_ઢગલો;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<અચલ AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<અચલ AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<અચલ AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<અચલ AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<અચલ AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_ગાંઠ* nodep, તાર્કિક endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        જો (endOfIter) {
            જો (nodep->user1p()) પાછા VN_AS(nodep->user1p(), JumpBlock);
        } નહીંતર {
            જો (nodep->user2p()) પાછા VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_ગાંઠ* underp = nullptr;
        તાર્કિક under_and_next = સાચું;
        જો (AstBegin* અચલ blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } નહીંતર જો (Ast_ગાંઠ_FTask* અચલ fTaskp = VN_CAST(nodep, ગાંઠ_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } નહીંતર જો (AstForeach* અચલ foreachp = VN_CAST(nodep, Foreach)) {
            જો (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                જો (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } નહીંતર {
                underp = nodep;
                under_and_next = ખોટું;  // IE we skip the entire foreach
            }
        } નહીંતર જો (AstLoop* અચલ loopp = VN_CAST(nodep, Loop)) {
            જો (endOfIter) {
                underp = loopp->stmtsp();
            } નહીંતર {
                underp = nodep;
                under_and_next = ખોટું;  // IE we skip the entire loop
            }
        } નહીંતર {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            પાછા nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        જ્યારે (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        જો (!underp->nextp() || !under_and_next) {
            જો (AstJumpBlock* અચલ blockp = VN_CAST(underp, JumpBlock)) પાછા blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        જો (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } નહીંતર {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* અચલ blockp = નવું AstJumpBlock{nodep->fileline(), underp};
        જો (endOfIter) {
            nodep->user1p(blockp);
        } નહીંતર {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        માટે (Ast_ગાંઠ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            જો (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        પાછા blockp;
    }
    શૂન્ય addPrefixToBlocksRecurse(અચલ std::string& prefix, Ast_ગાંઠ* અચલ nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        જો (AstBegin* અચલ beginp = VN_CAST(nodep, Begin)) {
            જો (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        જો (Ast_ગાંઠ* અચલ refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    તાર્કિક existsBlockAbove(અચલ std::string& name) અચલ {
        માટે (અચલ Ast_ગાંઠ_Block* અચલ stackp : vlstd::reverse_view(m_block_ઢગલો)) {
            જો (stackp->name() == name) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    સ્થિર Ast_ગાંઠ_Module* findOwnerModulep(Ast_ગાંઠ* nodep) {
        માટે (Ast_ગાંઠ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            જો (Ast_ગાંઠ_Module* અચલ modp = VN_CAST(curp, ગાંઠ_Module)) પાછા modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        પાછા nullptr;  // LCOV_EXCL_LINE
    }
    સ્થિર std::string targetInstancePath(અચલ AstDisable* અચલ nodep) {
        જો (અચલ AstVarXRef* અચલ xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            પાછા xrefp->dotted();
        }
        પાછા "";
    }
    સ્થિર std::string queueDotted(અચલ AstDisable* અચલ nodep,
                                   અચલ AstVar* અચલ processQueuep) {
        પાછા processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(અચલ Ast_ગાંઠ_Block* અચલ targetp) અચલ {
        AstBegin* innerForkBranchp = nullptr;
        Ast_ગાંઠ_Block* prevBlockp = nullptr;
        માટે (Ast_ગાંઠ_Block* અચલ blockp : vlstd::reverse_view(m_block_ઢગલો)) {
            જો (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            જો (blockp == targetp) પાછા innerForkBranchp;
            prevBlockp = blockp;
        }
        પાછા nullptr;
    }
    સ્થિર Ast_Stmt_પદાવલિ* get_Queue_ધકેલો_Process_Selfp(AstVarRef* અચલ queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_લીટી* અચલ flp = queueRefp->fileline();
        પાછા નવું Ast_Stmt_પદાવલિ{
            flp,
            નવું AstMethodCall{flp, queueRefp, "push_back",
                              નવું AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    સ્થિર Ast_ગાંઠ_Var_Ref* newQueueRefp(File_લીટી* અચલ fl, AstVar* અચલ processQueuep,
                                       અચલ VAccess& access, અચલ std::string& dotted = "") {
        જો (dotted != "") પાછા નવું AstVarXRef{fl, processQueuep, dotted, access};
        જો (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            પાછા નવું AstVarRef{fl, processQueuep, access};
        }
        AstPackage* અચલ ટોચ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        પાછા નવું AstVarRef{fl, ટોચ_Pkgp, processQueuep, access};
    }
    સ્થિર Ast_Stmt_પદાવલિ* get_Queue_ધકેલો_Process_Selfp(File_લીટી* અચલ fl, AstVar* અચલ processQueuep) {
        AstVarRef* અચલ queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        પાછા get_Queue_ધકેલો_Process_Selfp(queueWriteRefp);
    }
    સ્થિર Ast_Stmt_પદાવલિ* getQueueKillStmtp(File_લીટી* અચલ fl, AstVar* અચલ processQueuep,
                                          અચલ std::string& dotted = "") {
        Ast_ગાંઠ_Var_Ref* અચલ queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        માટે (Ast_ગાંઠ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            જો (itemp->name() == "killQueue") {
                killQueueCall
                    = નવું AstTaskRef{fl, VN_AS(itemp, Task), નવું AstArg{fl, "", queueRefp}};
                તોડો;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        પાછા નવું Ast_Stmt_પદાવલિ{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* અચલ taskp, File_લીટી* અચલ fl) {
        અચલ auto it = m_taskDisableBegins.find(taskp);
        જો (it != m_taskDisableBegins.end()) પાછા it->second;

        AstBegin* અચલ taskBodyp = નવું AstBegin{fl, "", nullptr, ખોટું};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        માટે (Ast_ગાંઠ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ગાંઠ* અચલ nextp = stmtp->nextp();
            અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var);
            જો (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* અચલ forkp = નવું AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        પાછા taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ગાંઠ* અચલ nodep, File_લીટી* અચલ fl, અચલ VVarType& varType) {
        AstVar* અચલ processQueuep = નવું AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            નવું AstQueueDType{
                fl, VFlagChildDType{},
                નવું AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(સાચું);
        processQueuep->setIgnoreSchedWrite();
        પાછા processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ગાંઠ* અચલ nodep, File_લીટી* અચલ fl) {
        Ast_ગાંઠ_Module* અચલ ownerp = findOwnerModulep(nodep);

        જો (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* અચલ ટોચ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* અચલ processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            ટોચ_Pkgp->addStmtsp(processQueuep);
            પાછા processQueuep;
        }

        AstVar* અચલ processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        પાછા processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* અચલ taskp, File_લીટી* અચલ fl) {
        અચલ auto it = m_taskDisableQueues.find(taskp);
        જો (it != m_taskDisableQueues.end()) પાછા it->second;

        AstVar* અચલ processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_પદાવલિ* અચલ ધકેલો_Current_Processp = get_Queue_ધકેલો_Process_Selfp(fl, processQueuep);
        AstBegin* અચલ taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ધકેલો_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        પાછા processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* અચલ beginp, File_લીટી* અચલ fl) {
        અચલ auto it = m_beginDisableBegins.find(beginp);
        જો (it != m_beginDisableBegins.end()) પાછા it->second;

        AstBegin* અચલ beginBodyp = નવું AstBegin{fl, "", nullptr, ખોટું};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        જો (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* અચલ forkp = નવું AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        પાછા beginBodyp;
    }
    શૂન્ય prependForkBranchQueuePushes(AstFork* અચલ forkp, AstVar* અચલ processQueuep,
                                      File_લીટી* અચલ fl, તાર્કિક needProcess) {
        માટે (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            જો (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ધકેલો_Process_Selfp(fl, processQueuep));
        }
    }
    શૂન્ય prependNestedForkBranchQueuePushes(Ast_ગાંઠ* અચલ nodep, AstVar* અચલ processQueuep,
                                            File_લીટી* અચલ fl, તાર્કિક needProcess) {
        nodep->foreach([&](AstFork* અચલ forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* અચલ beginp, File_લીટી* અચલ fl) {
        અચલ auto it = m_beginDisableQueues.find(beginp);
        જો (it != m_beginDisableQueues.end()) પાછા it->second;

        AstVar* અચલ processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_પદાવલિ* અચલ ધકેલો_Current_Processp = get_Queue_ધકેલો_Process_Selfp(fl, processQueuep);
        AstBegin* અચલ beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ધકેલો_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, ખોટું);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        પાછા processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* અચલ forkp, File_લીટી* અચલ fl) {
        અચલ auto it = m_forkDisableQueues.find(forkp);
        જો (it != m_forkDisableQueues.end()) પાછા it->second;

        AstVar* અચલ processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, સાચું);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        માટે (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, સાચું);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        પાછા processQueuep;
    }
    Ast_Stmt_પદાવલિ* ઉમેરો_Kill_Stmtp(AstDisable* અચલ nodep, AstVar* અચલ processQueuep) {
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_Stmt_પદાવલિ* અચલ killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        પાછા killStmtp;
    }
    શૂન્ય addJumpAfterKill(Ast_Stmt_પદાવલિ* અચલ killStmtp, Ast_ગાંઠ* અચલ targetp) {
        AstJumpBlock* અચલ jmpBlockp = getJumpBlock(targetp, ખોટું);
        killStmtp->addNextHere(નવું AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    શૂન્ય handleDisableOnFork(AstDisable* અચલ nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_લીટી* અચલ fl = nodep->fileline();
        AstFork* અચલ targetp = VN_AS(nodep->targetp(), Fork);
        જો (m_ftaskp) {
            જો (!m_ftaskp->exists(
                    [targetp](અચલ Ast_ગાંઠ_Block* blockp) -> તાર્કિક { પાછા blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* અચલ processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_પદાવલિ* અચલ killStmtp = ઉમેરો_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        જો (AstBegin* અચલ branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->dead()) પાછા;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_અંક);
        m_modp = nodep;
        m_mod_Repeat_અંક = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_ઢગલો.ધકેલો_back(nodep);
        iterateChildren(nodep);
        m_block_ઢગલો.કાઢો_back();
    }
    શૂન્ય visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = સાચું;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        માટે (Ast_ગાંઠ_Block* અચલ blockp : vlstd::reverse_view(m_block_ઢગલો)) {
            જો (blockp->user3SetOnce()) તોડો;
        }
        m_block_ઢગલો.ધકેલો_back(nodep);
        iterateChildren(nodep);
        m_block_ઢગલો.કાઢો_back();
    }
    શૂન્ય visit(AstStmtPragma* nodep) override {
        જો (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        } નહીંતર જો (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        } નહીંતર {
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ગાંઠ_પદાવલિ* અચલ countp = nodep->countp()->unlinkFrBackWithNext();
        અચલ string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_અંક++);
        AstBegin* અચલ beginp = નવું AstBegin{nodep->fileline(), "", nullptr, સાચું};
        // Spec says value is integral, if negative is ignored
        AstVar* અચલ varp
            = નવું AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(સાચું);
        beginp->addStmtsp(varp);
        Ast_ગાંઠ* initsp = નવું AstAssign{
            nodep->fileline(), નવું AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ગાંઠ* અચલ decp = નવું AstAssign{
            nodep->fileline(), નવું AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            નવું AstSub{nodep->fileline(), નવું AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       નવું AstConst{nodep->fileline(), 1}}};
        Ast_ગાંઠ_પદાવલિ* અચલ zerosp = નવું AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstGtS{
            nodep->fileline(), નવું AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ગાંઠ* અચલ bodysp = nodep->stmtsp();
        જો (bodysp) bodysp->unlinkFrBackWithNext();
        File_લીટી* અચલ flp = nodep->fileline();
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        loopp->addStmtsp(નવું AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        જો (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstLoop* nodep) override {
        જો (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        જો (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, સાચું);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = ખોટું;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = સાચું;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        જો (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    શૂન્ય visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        અચલ AstFunc* અચલ funcp = VN_CAST(m_ftaskp, Func);
        જો (m_randsequencep) {
            nodep->replaceWith(નવું AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        } નહીંતર જો (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        } નહીંતર જો (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } નહીંતર જો (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } નહીંતર જો (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } નહીંતર {
            જો (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(નવું AstAssign{
                    nodep->fileline(),
                    નવું AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* અચલ blockp = getJumpBlock(m_ftaskp, ખોટું);
            nodep->addHereThisAsNext(નવું AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        જો (!m_loopp && m_randsequencep) {
            nodep->replaceWith(નવું AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        } નહીંતર જો (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } નહીંતર {
            // Jump to the end of the loop
            AstJumpBlock* અચલ blockp = getJumpBlock(m_loopp, ખોટું);
            nodep->addNextHere(નવું AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        જો (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } નહીંતર {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* અચલ blockp = getJumpBlock(m_loopp, સાચું);
            nodep->addNextHere(નવું AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ગાંઠ* અચલ targetp = nodep->targetp();
        જો (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        જો (AstTask* અચલ taskp = VN_CAST(targetp, Task)) {
            AstVar* અચલ processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_પદાવલિ* અચલ killStmtp = ઉમેરો_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            જો (m_ftaskp == taskp) {
                Ast_ગાંઠ* jumpTargetp = taskp;
                અચલ auto it = m_taskDisableBegins.find(taskp);
                જો (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } નહીંતર જો (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } નહીંતર જો (AstBegin* અચલ beginp = VN_CAST(targetp, Begin)) {
            જો (existsBlockAbove(beginp->name())) {
                જો (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* અચલ blockp = getJumpBlock(beginp, ખોટું);
                    nodep->addNextHere(નવું AstJumpGo{nodep->fileline(), blockp});
                } નહીંતર {
                    AstVar* અચલ processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_પદાવલિ* અચલ killStmtp = ઉમેરો_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* અચલ branchp = innerForkBranchp(beginp);
                    AstBegin* અચલ jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } નહીંતર {
                AstVar* અચલ processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ઉમેરો_Kill_Stmtp(nodep, processQueuep);
            }
        } નહીંતર {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstFinish* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        જો (m_inFork) {
            nodep->replaceWith(નવું AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* અચલ blockp = getJumpBlock(m_loopp, ખોટું);
            nodep->addNextHere(નવું AstJumpGo{nodep->fileline(), blockp});
        }
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(સાચું);
    }
    શૂન્ય visit(AstConst*) override {}
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = મૂળભૂત;
};

//######################################################################
// Task class functions

શૂન્ય V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("linkjump", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
