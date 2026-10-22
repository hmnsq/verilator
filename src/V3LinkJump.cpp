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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3LinkJump.h"

#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্ৰেণী LinkJumpVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ধ্রুৱ VNUser1InUse m_user1InUse;
    ধ্রুৱ VNUser2InUse m_user2InUse;
    ধ্রুৱ VNUser3InUse m_user3InUse;

    // STATE
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্ৰন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্ৰন্থি* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    তার্কিক m_loopInc = মিছা;  // In loop increment
    তার্কিক m_inFork = মিছা;  // Under fork
    পূর্ণাঙ্ক m_mod_Repeat_অঙ্ক = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_গ্ৰন্থি_Block*> m_block_স্তূপ;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ধ্রুৱ AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ধ্রুৱ AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ধ্রুৱ AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ধ্রুৱ AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ধ্রুৱ AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_গ্ৰন্থি* nodep, তার্কিক endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        যদি (endOfIter) {
            যদি (nodep->user1p()) ঘূৰাই VN_AS(nodep->user1p(), JumpBlock);
        } অন্যথা {
            যদি (nodep->user2p()) ঘূৰাই VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_গ্ৰন্থি* underp = nullptr;
        তার্কিক under_and_next = সঁচা;
        যদি (AstBegin* ধ্রুৱ blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } অন্যথা যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ fTaskp = VN_CAST(nodep, গ্ৰন্থি_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } অন্যথা যদি (AstForeach* ধ্রুৱ foreachp = VN_CAST(nodep, Foreach)) {
            যদি (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                যদি (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } অন্যথা {
                underp = nodep;
                under_and_next = মিছা;  // IE we skip the entire foreach
            }
        } অন্যথা যদি (AstLoop* ধ্রুৱ loopp = VN_CAST(nodep, Loop)) {
            যদি (endOfIter) {
                underp = loopp->stmtsp();
            } অন্যথা {
                underp = nodep;
                under_and_next = মিছা;  // IE we skip the entire loop
            }
        } অন্যথা {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            ঘূৰাই nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        যেতিয়ালৈ (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        যদি (!underp->nextp() || !under_and_next) {
            যদি (AstJumpBlock* ধ্রুৱ blockp = VN_CAST(underp, JumpBlock)) ঘূৰাই blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        যদি (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } অন্যথা {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* ধ্রুৱ blockp = নূতন AstJumpBlock{nodep->fileline(), underp};
        যদি (endOfIter) {
            nodep->user1p(blockp);
        } অন্যথা {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        বাবে (Ast_গ্ৰন্থি *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            যদি (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        ঘূৰাই blockp;
    }
    শূন্য addPrefixToBlocksRecurse(ধ্রুৱ std::string& prefix, Ast_গ্ৰন্থি* ধ্রুৱ nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        যদি (AstBegin* ধ্রুৱ beginp = VN_CAST(nodep, Begin)) {
            যদি (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    তার্কিক existsBlockAbove(ধ্রুৱ std::string& name) ধ্রুৱ {
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি_Block* ধ্রুৱ stackp : vlstd::reverse_view(m_block_স্তূপ)) {
            যদি (stackp->name() == name) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }
    স্থির Ast_গ্ৰন্থি_Module* findOwnerModulep(Ast_গ্ৰন্থি* nodep) {
        বাবে (Ast_গ্ৰন্থি* curp = nodep; curp; curp = curp->aboveLoopp()) {
            যদি (Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = VN_CAST(curp, গ্ৰন্থি_Module)) ঘূৰাই modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        ঘূৰাই nullptr;  // LCOV_EXCL_LINE
    }
    স্থির std::string targetInstancePath(ধ্রুৱ AstDisable* ধ্রুৱ nodep) {
        যদি (ধ্রুৱ AstVarXRef* ধ্রুৱ xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            ঘূৰাই xrefp->dotted();
        }
        ঘূৰাই "";
    }
    স্থির std::string queueDotted(ধ্রুৱ AstDisable* ধ্রুৱ nodep,
                                   ধ্রুৱ AstVar* ধ্রুৱ processQueuep) {
        ঘূৰাই processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(ধ্রুৱ Ast_গ্ৰন্থি_Block* ধ্রুৱ targetp) ধ্রুৱ {
        AstBegin* innerForkBranchp = nullptr;
        Ast_গ্ৰন্থি_Block* prevBlockp = nullptr;
        বাবে (Ast_গ্ৰন্থি_Block* ধ্রুৱ blockp : vlstd::reverse_view(m_block_স্তূপ)) {
            যদি (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            যদি (blockp == targetp) ঘূৰাই innerForkBranchp;
            prevBlockp = blockp;
        }
        ঘূৰাই nullptr;
    }
    স্থির Ast_Stmt_ৰাশিমালা* get_Queue_ঠেলো_Process_Selfp(AstVarRef* ধ্রুৱ queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_সাৰি* ধ্রুৱ flp = queueRefp->fileline();
        ঘূৰাই নূতন Ast_Stmt_ৰাশিমালা{
            flp,
            নূতন AstMethodCall{flp, queueRefp, "push_back",
                              নূতন AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    স্থির Ast_গ্ৰন্থি_Var_Ref* newQueueRefp(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ processQueuep,
                                       ধ্রুৱ VAccess& access, ধ্রুৱ std::string& dotted = "") {
        যদি (dotted != "") ঘূৰাই নূতন AstVarXRef{fl, processQueuep, dotted, access};
        যদি (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            ঘূৰাই নূতন AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ধ্রুৱ শীৰ্ষ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        ঘূৰাই নূতন AstVarRef{fl, শীৰ্ষ_Pkgp, processQueuep, access};
    }
    স্থির Ast_Stmt_ৰাশিমালা* get_Queue_ঠেলো_Process_Selfp(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ processQueuep) {
        AstVarRef* ধ্রুৱ queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        ঘূৰাই get_Queue_ঠেলো_Process_Selfp(queueWriteRefp);
    }
    স্থির Ast_Stmt_ৰাশিমালা* getQueueKillStmtp(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ processQueuep,
                                          ধ্রুৱ std::string& dotted = "") {
        Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        বাবে (Ast_গ্ৰন্থি* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            যদি (itemp->name() == "killQueue") {
                killQueueCall
                    = নূতন AstTaskRef{fl, VN_AS(itemp, Task), নূতন AstArg{fl, "", queueRefp}};
                ভঙ্গ;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        ঘূৰাই নূতন Ast_Stmt_ৰাশিমালা{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ধ্রুৱ taskp, File_সাৰি* ধ্রুৱ fl) {
        ধ্রুৱ auto it = m_taskDisableBegins.find(taskp);
        যদি (it != m_taskDisableBegins.end()) ঘূৰাই it->second;

        AstBegin* ধ্রুৱ taskBodyp = নূতন AstBegin{fl, "", nullptr, মিছা};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        বাবে (Ast_গ্ৰন্থি* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_গ্ৰন্থি* ধ্রুৱ nextp = stmtp->nextp();
            ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(stmtp, Var);
            যদি (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* ধ্রুৱ forkp = নূতন AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        ঘূৰাই taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_গ্ৰন্থি* ধ্রুৱ nodep, File_সাৰি* ধ্রুৱ fl, ধ্রুৱ VVarType& varType) {
        AstVar* ধ্রুৱ processQueuep = নূতন AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            নূতন AstQueueDType{
                fl, VFlagChildDType{},
                নূতন AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(সঁচা);
        processQueuep->setIgnoreSchedWrite();
        ঘূৰাই processQueuep;
    }
    AstVar* getProcessQueuep(Ast_গ্ৰন্থি* ধ্রুৱ nodep, File_সাৰি* ধ্রুৱ fl) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ ownerp = findOwnerModulep(nodep);

        যদি (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ধ্রুৱ শীৰ্ষ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ধ্রুৱ processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            শীৰ্ষ_Pkgp->addStmtsp(processQueuep);
            ঘূৰাই processQueuep;
        }

        AstVar* ধ্রুৱ processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        ঘূৰাই processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ধ্রুৱ taskp, File_সাৰি* ধ্রুৱ fl) {
        ধ্রুৱ auto it = m_taskDisableQueues.find(taskp);
        যদি (it != m_taskDisableQueues.end()) ঘূৰাই it->second;

        AstVar* ধ্রুৱ processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_ৰাশিমালা* ধ্রুৱ ঠেলো_Current_Processp = get_Queue_ঠেলো_Process_Selfp(fl, processQueuep);
        AstBegin* ধ্রুৱ taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ঠেলো_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        ঘূৰাই processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ধ্রুৱ beginp, File_সাৰি* ধ্রুৱ fl) {
        ধ্রুৱ auto it = m_beginDisableBegins.find(beginp);
        যদি (it != m_beginDisableBegins.end()) ঘূৰাই it->second;

        AstBegin* ধ্রুৱ beginBodyp = নূতন AstBegin{fl, "", nullptr, মিছা};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        যদি (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* ধ্রুৱ forkp = নূতন AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        ঘূৰাই beginBodyp;
    }
    শূন্য prependForkBranchQueuePushes(AstFork* ধ্রুৱ forkp, AstVar* ধ্রুৱ processQueuep,
                                      File_সাৰি* ধ্রুৱ fl, তার্কিক needProcess) {
        বাবে (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            যদি (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ঠেলো_Process_Selfp(fl, processQueuep));
        }
    }
    শূন্য prependNestedForkBranchQueuePushes(Ast_গ্ৰন্থি* ধ্রুৱ nodep, AstVar* ধ্রুৱ processQueuep,
                                            File_সাৰি* ধ্রুৱ fl, তার্কিক needProcess) {
        nodep->foreach([&](AstFork* ধ্রুৱ forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ধ্রুৱ beginp, File_সাৰি* ধ্রুৱ fl) {
        ধ্রুৱ auto it = m_beginDisableQueues.find(beginp);
        যদি (it != m_beginDisableQueues.end()) ঘূৰাই it->second;

        AstVar* ধ্রুৱ processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_ৰাশিমালা* ধ্রুৱ ঠেলো_Current_Processp = get_Queue_ঠেলো_Process_Selfp(fl, processQueuep);
        AstBegin* ধ্রুৱ beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ঠেলো_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, মিছা);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        ঘূৰাই processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ধ্রুৱ forkp, File_সাৰি* ধ্রুৱ fl) {
        ধ্রুৱ auto it = m_forkDisableQueues.find(forkp);
        যদি (it != m_forkDisableQueues.end()) ঘূৰাই it->second;

        AstVar* ধ্রুৱ processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, সঁচা);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        বাবে (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, সঁচা);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        ঘূৰাই processQueuep;
    }
    Ast_Stmt_ৰাশিমালা* ঢোকাও_Kill_Stmtp(AstDisable* ধ্রুৱ nodep, AstVar* ধ্রুৱ processQueuep) {
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_Stmt_ৰাশিমালা* ধ্রুৱ killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        ঘূৰাই killStmtp;
    }
    শূন্য addJumpAfterKill(Ast_Stmt_ৰাশিমালা* ধ্রুৱ killStmtp, Ast_গ্ৰন্থি* ধ্রুৱ targetp) {
        AstJumpBlock* ধ্রুৱ jmpBlockp = getJumpBlock(targetp, মিছা);
        killStmtp->addNextHere(নূতন AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    শূন্য handleDisableOnFork(AstDisable* ধ্রুৱ nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        AstFork* ধ্রুৱ targetp = VN_AS(nodep->targetp(), Fork);
        যদি (m_ftaskp) {
            যদি (!m_ftaskp->exists(
                    [targetp](ধ্রুৱ Ast_গ্ৰন্থি_Block* blockp) -> তার্কিক { ঘূৰাই blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ধ্রুৱ processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_ৰাশিমালা* ধ্রুৱ killStmtp = ঢোকাও_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        যদি (AstBegin* ধ্রুৱ branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        যদি (nodep->dead()) ঘূৰাই;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_অঙ্ক);
        m_modp = nodep;
        m_mod_Repeat_অঙ্ক = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_স্তূপ.ঠেলো_back(nodep);
        iterateChildren(nodep);
        m_block_স্তূপ.বেৰ_কৰো_back();
    }
    শূন্য visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = সঁচা;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        বাবে (Ast_গ্ৰন্থি_Block* ধ্রুৱ blockp : vlstd::reverse_view(m_block_স্তূপ)) {
            যদি (blockp->user3SetOnce()) ভঙ্গ;
        }
        m_block_স্তূপ.ঠেলো_back(nodep);
        iterateChildren(nodep);
        m_block_স্তূপ.বেৰ_কৰো_back();
    }
    শূন্য visit(AstStmtPragma* nodep) override {
        যদি (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা যদি (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা {
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ countp = nodep->countp()->unlinkFrBackWithNext();
        ধ্রুৱ string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_অঙ্ক++);
        AstBegin* ধ্রুৱ beginp = নূতন AstBegin{nodep->fileline(), "", nullptr, সঁচা};
        // Spec says value is integral, if negative is ignored
        AstVar* ধ্রুৱ varp
            = নূতন AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(সঁচা);
        beginp->addStmtsp(varp);
        Ast_গ্ৰন্থি* initsp = নূতন AstAssign{
            nodep->fileline(), নূতন AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_গ্ৰন্থি* ধ্রুৱ decp = নূতন AstAssign{
            nodep->fileline(), নূতন AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            নূতন AstSub{nodep->fileline(), নূতন AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       নূতন AstConst{nodep->fileline(), 1}}};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ zerosp = নূতন AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = নূতন AstGtS{
            nodep->fileline(), নূতন AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_গ্ৰন্থি* ধ্রুৱ bodysp = nodep->stmtsp();
        যদি (bodysp) bodysp->unlinkFrBackWithNext();
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
        loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        যদি (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstLoop* nodep) override {
        যদি (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        যদি (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, সঁচা);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = মিছা;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = সঁচা;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        যদি (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    শূন্য visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        ধ্রুৱ AstFunc* ধ্রুৱ funcp = VN_CAST(m_ftaskp, Func);
        যদি (m_randsequencep) {
            nodep->replaceWith(নূতন AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        } অন্যথা যদি (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        } অন্যথা যদি (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } অন্যথা যদি (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } অন্যথা যদি (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } অন্যথা {
            যদি (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(নূতন AstAssign{
                    nodep->fileline(),
                    নূতন AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* ধ্রুৱ blockp = getJumpBlock(m_ftaskp, মিছা);
            nodep->addHereThisAsNext(নূতন AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        যদি (!m_loopp && m_randsequencep) {
            nodep->replaceWith(নূতন AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        } অন্যথা যদি (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } অন্যথা {
            // Jump to the end of the loop
            AstJumpBlock* ধ্রুৱ blockp = getJumpBlock(m_loopp, মিছা);
            nodep->addNextHere(নূতন AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        যদি (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } অন্যথা {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* ধ্রুৱ blockp = getJumpBlock(m_loopp, সঁচা);
            nodep->addNextHere(নূতন AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_গ্ৰন্থি* ধ্রুৱ targetp = nodep->targetp();
        যদি (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        যদি (AstTask* ধ্রুৱ taskp = VN_CAST(targetp, Task)) {
            AstVar* ধ্রুৱ processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_ৰাশিমালা* ধ্রুৱ killStmtp = ঢোকাও_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            যদি (m_ftaskp == taskp) {
                Ast_গ্ৰন্থি* jumpTargetp = taskp;
                ধ্রুৱ auto it = m_taskDisableBegins.find(taskp);
                যদি (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } অন্যথা যদি (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } অন্যথা যদি (AstBegin* ধ্রুৱ beginp = VN_CAST(targetp, Begin)) {
            যদি (existsBlockAbove(beginp->name())) {
                যদি (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* ধ্রুৱ blockp = getJumpBlock(beginp, মিছা);
                    nodep->addNextHere(নূতন AstJumpGo{nodep->fileline(), blockp});
                } অন্যথা {
                    AstVar* ধ্রুৱ processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_ৰাশিমালা* ধ্রুৱ killStmtp = ঢোকাও_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ধ্রুৱ branchp = innerForkBranchp(beginp);
                    AstBegin* ধ্রুৱ jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } অন্যথা {
                AstVar* ধ্রুৱ processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ঢোকাও_Kill_Stmtp(nodep, processQueuep);
            }
        } অন্যথা {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstFinish* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        যদি (m_inFork) {
            nodep->replaceWith(নূতন AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* ধ্রুৱ blockp = getJumpBlock(m_loopp, মিছা);
            nodep->addNextHere(নূতন AstJumpGo{nodep->fileline(), blockp});
        }
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(সঁচা);
    }
    শূন্য visit(AstConst*) override {}
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Task class functions

শূন্য V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkjump", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
