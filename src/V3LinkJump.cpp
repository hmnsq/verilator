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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkJump.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு LinkJumpVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    மாறிலி VNUser1InUse m_user1InUse;
    மாறிலி VNUser2InUse m_user2InUse;
    மாறிலி VNUser3InUse m_user3InUse;

    // STATE
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_முனை* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    தருக்க m_loopInc = பொய்;  // In loop increment
    தருக்க m_inFork = பொய்;  // Under fork
    முழு m_mod_Repeat_இலக்கம் = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_முனை_Block*> m_block_அடுக்கு;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<மாறிலி AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<மாறிலி AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<மாறிலி AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<மாறிலி AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<மாறிலி AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_முனை* nodep, தருக்க endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        எனில் (endOfIter) {
            எனில் (nodep->user1p()) திரும்பு VN_AS(nodep->user1p(), JumpBlock);
        } இல்லையெனில் {
            எனில் (nodep->user2p()) திரும்பு VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_முனை* underp = nullptr;
        தருக்க under_and_next = உண்மை;
        எனில் (AstBegin* மாறிலி blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } இல்லையெனில் எனில் (Ast_முனை_FTask* மாறிலி fTaskp = VN_CAST(nodep, முனை_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } இல்லையெனில் எனில் (AstForeach* மாறிலி foreachp = VN_CAST(nodep, Foreach)) {
            எனில் (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                எனில் (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } இல்லையெனில் {
                underp = nodep;
                under_and_next = பொய்;  // IE we skip the entire foreach
            }
        } இல்லையெனில் எனில் (AstLoop* மாறிலி loopp = VN_CAST(nodep, Loop)) {
            எனில் (endOfIter) {
                underp = loopp->stmtsp();
            } இல்லையெனில் {
                underp = nodep;
                under_and_next = பொய்;  // IE we skip the entire loop
            }
        } இல்லையெனில் {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            திரும்பு nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        போது (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        எனில் (!underp->nextp() || !under_and_next) {
            எனில் (AstJumpBlock* மாறிலி blockp = VN_CAST(underp, JumpBlock)) திரும்பு blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        எனில் (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } இல்லையெனில் {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* மாறிலி blockp = புதிய AstJumpBlock{nodep->fileline(), underp};
        எனில் (endOfIter) {
            nodep->user1p(blockp);
        } இல்லையெனில் {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        க்கு (Ast_முனை *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            எனில் (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        திரும்பு blockp;
    }
    வெறுமை addPrefixToBlocksRecurse(மாறிலி std::string& prefix, Ast_முனை* மாறிலி nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        எனில் (AstBegin* மாறிலி beginp = VN_CAST(nodep, Begin)) {
            எனில் (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        எனில் (Ast_முனை* மாறிலி refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    தருக்க existsBlockAbove(மாறிலி std::string& name) மாறிலி {
        க்கு (மாறிலி Ast_முனை_Block* மாறிலி stackp : vlstd::reverse_view(m_block_அடுக்கு)) {
            எனில் (stackp->name() == name) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    நிலையான Ast_முனை_Module* findOwnerModulep(Ast_முனை* nodep) {
        க்கு (Ast_முனை* curp = nodep; curp; curp = curp->aboveLoopp()) {
            எனில் (Ast_முனை_Module* மாறிலி modp = VN_CAST(curp, முனை_Module)) திரும்பு modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        திரும்பு nullptr;  // LCOV_EXCL_LINE
    }
    நிலையான std::string targetInstancePath(மாறிலி AstDisable* மாறிலி nodep) {
        எனில் (மாறிலி AstVarXRef* மாறிலி xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            திரும்பு xrefp->dotted();
        }
        திரும்பு "";
    }
    நிலையான std::string queueDotted(மாறிலி AstDisable* மாறிலி nodep,
                                   மாறிலி AstVar* மாறிலி processQueuep) {
        திரும்பு processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(மாறிலி Ast_முனை_Block* மாறிலி targetp) மாறிலி {
        AstBegin* innerForkBranchp = nullptr;
        Ast_முனை_Block* prevBlockp = nullptr;
        க்கு (Ast_முனை_Block* மாறிலி blockp : vlstd::reverse_view(m_block_அடுக்கு)) {
            எனில் (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            எனில் (blockp == targetp) திரும்பு innerForkBranchp;
            prevBlockp = blockp;
        }
        திரும்பு nullptr;
    }
    நிலையான Ast_Stmt_கோவை* get_Queue_தள்ளு_Process_Selfp(AstVarRef* மாறிலி queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_வரி* மாறிலி flp = queueRefp->fileline();
        திரும்பு புதிய Ast_Stmt_கோவை{
            flp,
            புதிய AstMethodCall{flp, queueRefp, "push_back",
                              புதிய AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    நிலையான Ast_முனை_Var_Ref* newQueueRefp(File_வரி* மாறிலி fl, AstVar* மாறிலி processQueuep,
                                       மாறிலி VAccess& access, மாறிலி std::string& dotted = "") {
        எனில் (dotted != "") திரும்பு புதிய AstVarXRef{fl, processQueuep, dotted, access};
        எனில் (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            திரும்பு புதிய AstVarRef{fl, processQueuep, access};
        }
        AstPackage* மாறிலி உச்சி_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        திரும்பு புதிய AstVarRef{fl, உச்சி_Pkgp, processQueuep, access};
    }
    நிலையான Ast_Stmt_கோவை* get_Queue_தள்ளு_Process_Selfp(File_வரி* மாறிலி fl, AstVar* மாறிலி processQueuep) {
        AstVarRef* மாறிலி queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        திரும்பு get_Queue_தள்ளு_Process_Selfp(queueWriteRefp);
    }
    நிலையான Ast_Stmt_கோவை* getQueueKillStmtp(File_வரி* மாறிலி fl, AstVar* மாறிலி processQueuep,
                                          மாறிலி std::string& dotted = "") {
        Ast_முனை_Var_Ref* மாறிலி queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        க்கு (Ast_முனை* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            எனில் (itemp->name() == "killQueue") {
                killQueueCall
                    = புதிய AstTaskRef{fl, VN_AS(itemp, Task), புதிய AstArg{fl, "", queueRefp}};
                நிறுத்து;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        திரும்பு புதிய Ast_Stmt_கோவை{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* மாறிலி taskp, File_வரி* மாறிலி fl) {
        மாறிலி auto it = m_taskDisableBegins.find(taskp);
        எனில் (it != m_taskDisableBegins.end()) திரும்பு it->second;

        AstBegin* மாறிலி taskBodyp = புதிய AstBegin{fl, "", nullptr, பொய்};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        க்கு (Ast_முனை* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_முனை* மாறிலி nextp = stmtp->nextp();
            மாறிலி AstVar* மாறிலி varp = VN_CAST(stmtp, Var);
            எனில் (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* மாறிலி forkp = புதிய AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        திரும்பு taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_முனை* மாறிலி nodep, File_வரி* மாறிலி fl, மாறிலி VVarType& varType) {
        AstVar* மாறிலி processQueuep = புதிய AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            புதிய AstQueueDType{
                fl, VFlagChildDType{},
                புதிய AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(உண்மை);
        processQueuep->setIgnoreSchedWrite();
        திரும்பு processQueuep;
    }
    AstVar* getProcessQueuep(Ast_முனை* மாறிலி nodep, File_வரி* மாறிலி fl) {
        Ast_முனை_Module* மாறிலி ownerp = findOwnerModulep(nodep);

        எனில் (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* மாறிலி உச்சி_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* மாறிலி processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            உச்சி_Pkgp->addStmtsp(processQueuep);
            திரும்பு processQueuep;
        }

        AstVar* மாறிலி processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        திரும்பு processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* மாறிலி taskp, File_வரி* மாறிலி fl) {
        மாறிலி auto it = m_taskDisableQueues.find(taskp);
        எனில் (it != m_taskDisableQueues.end()) திரும்பு it->second;

        AstVar* மாறிலி processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_கோவை* மாறிலி தள்ளு_Current_Processp = get_Queue_தள்ளு_Process_Selfp(fl, processQueuep);
        AstBegin* மாறிலி taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(தள்ளு_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        திரும்பு processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* மாறிலி beginp, File_வரி* மாறிலி fl) {
        மாறிலி auto it = m_beginDisableBegins.find(beginp);
        எனில் (it != m_beginDisableBegins.end()) திரும்பு it->second;

        AstBegin* மாறிலி beginBodyp = புதிய AstBegin{fl, "", nullptr, பொய்};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        எனில் (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* மாறிலி forkp = புதிய AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        திரும்பு beginBodyp;
    }
    வெறுமை prependForkBranchQueuePushes(AstFork* மாறிலி forkp, AstVar* மாறிலி processQueuep,
                                      File_வரி* மாறிலி fl, தருக்க needProcess) {
        க்கு (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            எனில் (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_தள்ளு_Process_Selfp(fl, processQueuep));
        }
    }
    வெறுமை prependNestedForkBranchQueuePushes(Ast_முனை* மாறிலி nodep, AstVar* மாறிலி processQueuep,
                                            File_வரி* மாறிலி fl, தருக்க needProcess) {
        nodep->foreach([&](AstFork* மாறிலி forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* மாறிலி beginp, File_வரி* மாறிலி fl) {
        மாறிலி auto it = m_beginDisableQueues.find(beginp);
        எனில் (it != m_beginDisableQueues.end()) திரும்பு it->second;

        AstVar* மாறிலி processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_கோவை* மாறிலி தள்ளு_Current_Processp = get_Queue_தள்ளு_Process_Selfp(fl, processQueuep);
        AstBegin* மாறிலி beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(தள்ளு_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, பொய்);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        திரும்பு processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* மாறிலி forkp, File_வரி* மாறிலி fl) {
        மாறிலி auto it = m_forkDisableQueues.find(forkp);
        எனில் (it != m_forkDisableQueues.end()) திரும்பு it->second;

        AstVar* மாறிலி processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, உண்மை);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        க்கு (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, உண்மை);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        திரும்பு processQueuep;
    }
    Ast_Stmt_கோவை* செருகு_Kill_Stmtp(AstDisable* மாறிலி nodep, AstVar* மாறிலி processQueuep) {
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_Stmt_கோவை* மாறிலி killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        திரும்பு killStmtp;
    }
    வெறுமை addJumpAfterKill(Ast_Stmt_கோவை* மாறிலி killStmtp, Ast_முனை* மாறிலி targetp) {
        AstJumpBlock* மாறிலி jmpBlockp = getJumpBlock(targetp, பொய்);
        killStmtp->addNextHere(புதிய AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    வெறுமை handleDisableOnFork(AstDisable* மாறிலி nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_வரி* மாறிலி fl = nodep->fileline();
        AstFork* மாறிலி targetp = VN_AS(nodep->targetp(), Fork);
        எனில் (m_ftaskp) {
            எனில் (!m_ftaskp->exists(
                    [targetp](மாறிலி Ast_முனை_Block* blockp) -> தருக்க { திரும்பு blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* மாறிலி processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_கோவை* மாறிலி killStmtp = செருகு_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        எனில் (AstBegin* மாறிலி branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->dead()) திரும்பு;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_இலக்கம்);
        m_modp = nodep;
        m_mod_Repeat_இலக்கம் = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_அடுக்கு.தள்ளு_back(nodep);
        iterateChildren(nodep);
        m_block_அடுக்கு.எடு_back();
    }
    வெறுமை visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = உண்மை;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        க்கு (Ast_முனை_Block* மாறிலி blockp : vlstd::reverse_view(m_block_அடுக்கு)) {
            எனில் (blockp->user3SetOnce()) நிறுத்து;
        }
        m_block_அடுக்கு.தள்ளு_back(nodep);
        iterateChildren(nodep);
        m_block_அடுக்கு.எடு_back();
    }
    வெறுமை visit(AstStmtPragma* nodep) override {
        எனில் (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        } இல்லையெனில் எனில் (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_முனை_கோவை* மாறிலி countp = nodep->countp()->unlinkFrBackWithNext();
        மாறிலி string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_இலக்கம்++);
        AstBegin* மாறிலி beginp = புதிய AstBegin{nodep->fileline(), "", nullptr, உண்மை};
        // Spec says value is integral, if negative is ignored
        AstVar* மாறிலி varp
            = புதிய AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(உண்மை);
        beginp->addStmtsp(varp);
        Ast_முனை* initsp = புதிய AstAssign{
            nodep->fileline(), புதிய AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_முனை* மாறிலி decp = புதிய AstAssign{
            nodep->fileline(), புதிய AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            புதிய AstSub{nodep->fileline(), புதிய AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       புதிய AstConst{nodep->fileline(), 1}}};
        Ast_முனை_கோவை* மாறிலி zerosp = புதிய AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_முனை_கோவை* மாறிலி condp = புதிய AstGtS{
            nodep->fileline(), புதிய AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_முனை* மாறிலி bodysp = nodep->stmtsp();
        எனில் (bodysp) bodysp->unlinkFrBackWithNext();
        File_வரி* மாறிலி flp = nodep->fileline();
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        எனில் (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstLoop* nodep) override {
        எனில் (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        எனில் (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, உண்மை);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = பொய்;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = உண்மை;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        எனில் (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    வெறுமை visit(Ast_முனை_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    வெறுமை visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        மாறிலி AstFunc* மாறிலி funcp = VN_CAST(m_ftaskp, Func);
        எனில் (m_randsequencep) {
            nodep->replaceWith(புதிய AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் எனில் (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        } இல்லையெனில் எனில் (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } இல்லையெனில் எனில் (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } இல்லையெனில் எனில் (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } இல்லையெனில் {
            எனில் (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(புதிய AstAssign{
                    nodep->fileline(),
                    புதிய AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* மாறிலி blockp = getJumpBlock(m_ftaskp, பொய்);
            nodep->addHereThisAsNext(புதிய AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        எனில் (!m_loopp && m_randsequencep) {
            nodep->replaceWith(புதிய AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் எனில் (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } இல்லையெனில் {
            // Jump to the end of the loop
            AstJumpBlock* மாறிலி blockp = getJumpBlock(m_loopp, பொய்);
            nodep->addNextHere(புதிய AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        எனில் (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } இல்லையெனில் {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* மாறிலி blockp = getJumpBlock(m_loopp, உண்மை);
            nodep->addNextHere(புதிய AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_முனை* மாறிலி targetp = nodep->targetp();
        எனில் (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        எனில் (AstTask* மாறிலி taskp = VN_CAST(targetp, Task)) {
            AstVar* மாறிலி processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_கோவை* மாறிலி killStmtp = செருகு_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            எனில் (m_ftaskp == taskp) {
                Ast_முனை* jumpTargetp = taskp;
                மாறிலி auto it = m_taskDisableBegins.find(taskp);
                எனில் (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } இல்லையெனில் எனில் (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } இல்லையெனில் எனில் (AstBegin* மாறிலி beginp = VN_CAST(targetp, Begin)) {
            எனில் (existsBlockAbove(beginp->name())) {
                எனில் (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* மாறிலி blockp = getJumpBlock(beginp, பொய்);
                    nodep->addNextHere(புதிய AstJumpGo{nodep->fileline(), blockp});
                } இல்லையெனில் {
                    AstVar* மாறிலி processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_கோவை* மாறிலி killStmtp = செருகு_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* மாறிலி branchp = innerForkBranchp(beginp);
                    AstBegin* மாறிலி jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } இல்லையெனில் {
                AstVar* மாறிலி processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                செருகு_Kill_Stmtp(nodep, processQueuep);
            }
        } இல்லையெனில் {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstFinish* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        எனில் (m_inFork) {
            nodep->replaceWith(புதிய AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* மாறிலி blockp = getJumpBlock(m_loopp, பொய்);
            nodep->addNextHere(புதிய AstJumpGo{nodep->fileline(), blockp});
        }
    }
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(உண்மை);
    }
    வெறுமை visit(AstConst*) override {}
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = இயல்பு;
};

//######################################################################
// Task class functions

வெறுமை V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("linkjump", 0, dump_மரம்_Either_Level() >= 3);
}
