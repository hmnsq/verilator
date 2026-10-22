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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkJump.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্রেণী LinkJumpVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ধ্রুবক VNUser1InUse m_user1InUse;
    ধ্রুবক VNUser2InUse m_user2InUse;
    ধ্রুবক VNUser3InUse m_user3InUse;

    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্রন্থি* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    যৌক্তিক m_loopInc = মিথ্যা;  // In loop increment
    যৌক্তিক m_inFork = মিথ্যা;  // Under fork
    পূর্ণসংখ্যা m_mod_Repeat_অঙ্ক = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_গ্রন্থি_Block*> m_block_স্তূপ;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ধ্রুবক AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ধ্রুবক AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ধ্রুবক AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ধ্রুবক AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ধ্রুবক AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_গ্রন্থি* nodep, যৌক্তিক endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        যদি (endOfIter) {
            যদি (nodep->user1p()) ফেরত VN_AS(nodep->user1p(), JumpBlock);
        } অন্যথা {
            যদি (nodep->user2p()) ফেরত VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_গ্রন্থি* underp = nullptr;
        যৌক্তিক under_and_পরবর্তী = সত্য;
        যদি (AstBegin* ধ্রুবক blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } অন্যথা যদি (Ast_গ্রন্থি_FTask* ধ্রুবক fTaskp = VN_CAST(nodep, গ্রন্থি_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } অন্যথা যদি (AstForeach* ধ্রুবক foreachp = VN_CAST(nodep, Foreach)) {
            যদি (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                যদি (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } অন্যথা {
                underp = nodep;
                under_and_পরবর্তী = মিথ্যা;  // IE we skip the entire foreach
            }
        } অন্যথা যদি (AstLoop* ধ্রুবক loopp = VN_CAST(nodep, Loop)) {
            যদি (endOfIter) {
                underp = loopp->stmtsp();
            } অন্যথা {
                underp = nodep;
                under_and_পরবর্তী = মিথ্যা;  // IE we skip the entire loop
            }
        } অন্যথা {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            ফেরত nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        যখন (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        যদি (!underp->nextp() || !under_and_পরবর্তী) {
            যদি (AstJumpBlock* ধ্রুবক blockp = VN_CAST(underp, JumpBlock)) ফেরত blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        যদি (under_and_পরবর্তী) {
            underp->unlink_Fr_Back_With_পরবর্তী(&repHandle);
        } অন্যথা {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* ধ্রুবক blockp = নতুন AstJumpBlock{nodep->fileline(), underp};
        যদি (endOfIter) {
            nodep->user1p(blockp);
        } অন্যথা {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        জন্য (Ast_গ্রন্থি *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            যদি (VN_IS(varp, Var)) blockp->add_Here_This_As_পরবর্তী(varp->unlinkFrBack());
        }
        ফেরত blockp;
    }
    শূন্য addPrefixToBlocksRecurse(ধ্রুবক std::string& prefix, Ast_গ্রন্থি* ধ্রুবক nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        যদি (AstBegin* ধ্রুবক beginp = VN_CAST(nodep, Begin)) {
            যদি (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    যৌক্তিক existsBlockAbove(ধ্রুবক std::string& name) ধ্রুবক {
        জন্য (ধ্রুবক Ast_গ্রন্থি_Block* ধ্রুবক stackp : vlstd::reverse_view(m_block_স্তূপ)) {
            যদি (stackp->name() == name) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }
    স্থির Ast_গ্রন্থি_Module* findOwnerModulep(Ast_গ্রন্থি* nodep) {
        জন্য (Ast_গ্রন্থি* curp = nodep; curp; curp = curp->aboveLoopp()) {
            যদি (Ast_গ্রন্থি_Module* ধ্রুবক modp = VN_CAST(curp, গ্রন্থি_Module)) ফেরত modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        ফেরত nullptr;  // LCOV_EXCL_LINE
    }
    স্থির std::string targetInstancePath(ধ্রুবক AstDisable* ধ্রুবক nodep) {
        যদি (ধ্রুবক AstVarXRef* ধ্রুবক xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            ফেরত xrefp->dotted();
        }
        ফেরত "";
    }
    স্থির std::string queueDotted(ধ্রুবক AstDisable* ধ্রুবক nodep,
                                   ধ্রুবক AstVar* ধ্রুবক processQueuep) {
        ফেরত processQueuep->is_অস্থায়ী() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(ধ্রুবক Ast_গ্রন্থি_Block* ধ্রুবক targetp) ধ্রুবক {
        AstBegin* innerForkBranchp = nullptr;
        Ast_গ্রন্থি_Block* পূর্ব_Blockp = nullptr;
        জন্য (Ast_গ্রন্থি_Block* ধ্রুবক blockp : vlstd::reverse_view(m_block_স্তূপ)) {
            যদি (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(পূর্ব_Blockp, Begin);
            }
            যদি (blockp == targetp) ফেরত innerForkBranchp;
            পূর্ব_Blockp = blockp;
        }
        ফেরত nullptr;
    }
    স্থির Ast_Stmt_রাশিমালা* get_Queue_ঠেলো_Process_Selfp(AstVarRef* ধ্রুবক queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_সারি* ধ্রুবক flp = queueRefp->fileline();
        ফেরত নতুন Ast_Stmt_রাশিমালা{
            flp,
            নতুন AstMethodCall{flp, queueRefp, "push_back",
                              নতুন AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    স্থির Ast_গ্রন্থি_Var_Ref* newQueueRefp(File_সারি* ধ্রুবক fl, AstVar* ধ্রুবক processQueuep,
                                       ধ্রুবক VAccess& access, ধ্রুবক std::string& dotted = "") {
        যদি (dotted != "") ফেরত নতুন AstVarXRef{fl, processQueuep, dotted, access};
        যদি (!processQueuep->lifetime().isStatic() || processQueuep->is_অস্থায়ী()) {
            ফেরত নতুন AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ধ্রুবক শীর্ষ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        ফেরত নতুন AstVarRef{fl, শীর্ষ_Pkgp, processQueuep, access};
    }
    স্থির Ast_Stmt_রাশিমালা* get_Queue_ঠেলো_Process_Selfp(File_সারি* ধ্রুবক fl, AstVar* ধ্রুবক processQueuep) {
        AstVarRef* ধ্রুবক queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        ফেরত get_Queue_ঠেলো_Process_Selfp(queueWriteRefp);
    }
    স্থির Ast_Stmt_রাশিমালা* getQueueKillStmtp(File_সারি* ধ্রুবক fl, AstVar* ধ্রুবক processQueuep,
                                          ধ্রুবক std::string& dotted = "") {
        Ast_গ্রন্থি_Var_Ref* ধ্রুবক queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        জন্য (Ast_গ্রন্থি* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            যদি (itemp->name() == "killQueue") {
                killQueueCall
                    = নতুন AstTaskRef{fl, VN_AS(itemp, Task), নতুন AstArg{fl, "", queueRefp}};
                বিরতি;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        ফেরত নতুন Ast_Stmt_রাশিমালা{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ধ্রুবক taskp, File_সারি* ধ্রুবক fl) {
        ধ্রুবক auto it = m_taskDisableBegins.find(taskp);
        যদি (it != m_taskDisableBegins.end()) ফেরত it->second;

        AstBegin* ধ্রুবক taskBodyp = নতুন AstBegin{fl, "", nullptr, মিথ্যা};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        জন্য (Ast_গ্রন্থি* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_গ্রন্থি* ধ্রুবক nextp = stmtp->nextp();
            ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var);
            যদি (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* ধ্রুবক forkp = নতুন AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        ফেরত taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_গ্রন্থি* ধ্রুবক nodep, File_সারি* ধ্রুবক fl, ধ্রুবক VVarType& varType) {
        AstVar* ধ্রুবক processQueuep = নতুন AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            নতুন AstQueueDType{
                fl, VFlagChildDType{},
                নতুন AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(সত্য);
        processQueuep->setIgnoreSchedWrite();
        ফেরত processQueuep;
    }
    AstVar* getProcessQueuep(Ast_গ্রন্থি* ধ্রুবক nodep, File_সারি* ধ্রুবক fl) {
        Ast_গ্রন্থি_Module* ধ্রুবক ownerp = findOwnerModulep(nodep);

        যদি (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ধ্রুবক শীর্ষ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ধ্রুবক processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            শীর্ষ_Pkgp->addStmtsp(processQueuep);
            ফেরত processQueuep;
        }

        AstVar* ধ্রুবক processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        ফেরত processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ধ্রুবক taskp, File_সারি* ধ্রুবক fl) {
        ধ্রুবক auto it = m_taskDisableQueues.find(taskp);
        যদি (it != m_taskDisableQueues.end()) ফেরত it->second;

        AstVar* ধ্রুবক processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_রাশিমালা* ধ্রুবক ঠেলো_Current_Processp = get_Queue_ঠেলো_Process_Selfp(fl, processQueuep);
        AstBegin* ধ্রুবক taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ঠেলো_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        ফেরত processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ধ্রুবক beginp, File_সারি* ধ্রুবক fl) {
        ধ্রুবক auto it = m_beginDisableBegins.find(beginp);
        যদি (it != m_beginDisableBegins.end()) ফেরত it->second;

        AstBegin* ধ্রুবক beginBodyp = নতুন AstBegin{fl, "", nullptr, মিথ্যা};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        যদি (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlink_Fr_Back_With_পরবর্তী());

        AstFork* ধ্রুবক forkp = নতুন AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        ফেরত beginBodyp;
    }
    শূন্য prependForkBranchQueuePushes(AstFork* ধ্রুবক forkp, AstVar* ধ্রুবক processQueuep,
                                      File_সারি* ধ্রুবক fl, যৌক্তিক needProcess) {
        জন্য (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            যদি (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ঠেলো_Process_Selfp(fl, processQueuep));
        }
    }
    শূন্য prependNestedForkBranchQueuePushes(Ast_গ্রন্থি* ধ্রুবক nodep, AstVar* ধ্রুবক processQueuep,
                                            File_সারি* ধ্রুবক fl, যৌক্তিক needProcess) {
        nodep->foreach([&](AstFork* ধ্রুবক forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ধ্রুবক beginp, File_সারি* ধ্রুবক fl) {
        ধ্রুবক auto it = m_beginDisableQueues.find(beginp);
        যদি (it != m_beginDisableQueues.end()) ফেরত it->second;

        AstVar* ধ্রুবক processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_রাশিমালা* ধ্রুবক ঠেলো_Current_Processp = get_Queue_ঠেলো_Process_Selfp(fl, processQueuep);
        AstBegin* ধ্রুবক beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ঠেলো_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, মিথ্যা);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        ফেরত processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ধ্রুবক forkp, File_সারি* ধ্রুবক fl) {
        ধ্রুবক auto it = m_forkDisableQueues.find(forkp);
        যদি (it != m_forkDisableQueues.end()) ফেরত it->second;

        AstVar* ধ্রুবক processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, সত্য);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        জন্য (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, সত্য);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        ফেরত processQueuep;
    }
    Ast_Stmt_রাশিমালা* ঢোকাও_Kill_Stmtp(AstDisable* ধ্রুবক nodep, AstVar* ধ্রুবক processQueuep) {
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_Stmt_রাশিমালা* ধ্রুবক killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->add_পরবর্তী_Here(killStmtp);
        ফেরত killStmtp;
    }
    শূন্য addJumpAfterKill(Ast_Stmt_রাশিমালা* ধ্রুবক killStmtp, Ast_গ্রন্থি* ধ্রুবক targetp) {
        AstJumpBlock* ধ্রুবক jmpBlockp = getJumpBlock(targetp, মিথ্যা);
        killStmtp->add_পরবর্তী_Here(নতুন AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    শূন্য handleDisableOnFork(AstDisable* ধ্রুবক nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_সারি* ধ্রুবক fl = nodep->fileline();
        AstFork* ধ্রুবক targetp = VN_AS(nodep->targetp(), Fork);
        যদি (m_ftaskp) {
            যদি (!m_ftaskp->exists(
                    [targetp](ধ্রুবক Ast_গ্রন্থি_Block* blockp) -> যৌক্তিক { ফেরত blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ধ্রুবক processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_রাশিমালা* ধ্রুবক killStmtp = ঢোকাও_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        যদি (AstBegin* ধ্রুবক branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->dead()) ফেরত;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_অঙ্ক);
        m_modp = nodep;
        m_mod_Repeat_অঙ্ক = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_স্তূপ.ঠেলো_back(nodep);
        iterateChildren(nodep);
        m_block_স্তূপ.বের_করো_back();
    }
    শূন্য visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = সত্য;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        জন্য (Ast_গ্রন্থি_Block* ধ্রুবক blockp : vlstd::reverse_view(m_block_স্তূপ)) {
            যদি (blockp->user3SetOnce()) বিরতি;
        }
        m_block_স্তূপ.ঠেলো_back(nodep);
        iterateChildren(nodep);
        m_block_স্তূপ.বের_করো_back();
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
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক countp = nodep->countp()->unlink_Fr_Back_With_পরবর্তী();
        ধ্রুবক string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_অঙ্ক++);
        AstBegin* ধ্রুবক beginp = নতুন AstBegin{nodep->fileline(), "", nullptr, সত্য};
        // Spec says value is integral, if negative is ignored
        AstVar* ধ্রুবক varp
            = নতুন AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(সত্য);
        beginp->addStmtsp(varp);
        Ast_গ্রন্থি* initsp = নতুন AstAssign{
            nodep->fileline(), নতুন AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_গ্রন্থি* ধ্রুবক decp = নতুন AstAssign{
            nodep->fileline(), নতুন AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            নতুন AstSub{nodep->fileline(), নতুন AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       নতুন AstConst{nodep->fileline(), 1}}};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক zerosp = নতুন AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = নতুন AstGtS{
            nodep->fileline(), নতুন AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_গ্রন্থি* ধ্রুবক bodysp = nodep->stmtsp();
        যদি (bodysp) bodysp->unlink_Fr_Back_With_পরবর্তী();
        File_সারি* ধ্রুবক flp = nodep->fileline();
        AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
        loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, condp});
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
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, সত্য);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = মিথ্যা;
        iterate_And_পরবর্তী_Null(nodep->stmtsp());
        m_loopInc = সত্য;
        iterate_And_পরবর্তী_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        যদি (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlink_Fr_Back_With_পরবর্তী());
    }
    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_পরবর্তী_Null(nodep->bodyp());
    }
    শূন্য visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        ধ্রুবক AstFunc* ধ্রুবক funcp = VN_CAST(m_ftaskp, Func);
        যদি (m_randsequencep) {
            nodep->replaceWith(নতুন AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        } অন্যথা যদি (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        } অন্যথা যদি (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } অন্যথা যদি (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } অন্যথা যদি (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } অন্যথা {
            যদি (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->add_Here_This_As_পরবর্তী(নতুন AstAssign{
                    nodep->fileline(),
                    নতুন AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_Back_With_পরবর্তী()});
            }
            // Jump to the end of the function call
            AstJumpBlock* ধ্রুবক blockp = getJumpBlock(m_ftaskp, মিথ্যা);
            nodep->add_Here_This_As_পরবর্তী(নতুন AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        যদি (!m_loopp && m_randsequencep) {
            nodep->replaceWith(নতুন AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        } অন্যথা যদি (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } অন্যথা {
            // Jump to the end of the loop
            AstJumpBlock* ধ্রুবক blockp = getJumpBlock(m_loopp, মিথ্যা);
            nodep->add_পরবর্তী_Here(নতুন AstJumpGo{nodep->fileline(), blockp});
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
            AstJumpBlock* ধ্রুবক blockp = getJumpBlock(m_loopp, সত্য);
            nodep->add_পরবর্তী_Here(নতুন AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_গ্রন্থি* ধ্রুবক targetp = nodep->targetp();
        যদি (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        যদি (AstTask* ধ্রুবক taskp = VN_CAST(targetp, Task)) {
            AstVar* ধ্রুবক processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_রাশিমালা* ধ্রুবক killStmtp = ঢোকাও_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            যদি (m_ftaskp == taskp) {
                Ast_গ্রন্থি* jumpTargetp = taskp;
                ধ্রুবক auto it = m_taskDisableBegins.find(taskp);
                যদি (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } অন্যথা যদি (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } অন্যথা যদি (AstBegin* ধ্রুবক beginp = VN_CAST(targetp, Begin)) {
            যদি (existsBlockAbove(beginp->name())) {
                যদি (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* ধ্রুবক blockp = getJumpBlock(beginp, মিথ্যা);
                    nodep->add_পরবর্তী_Here(নতুন AstJumpGo{nodep->fileline(), blockp});
                } অন্যথা {
                    AstVar* ধ্রুবক processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_রাশিমালা* ধ্রুবক killStmtp = ঢোকাও_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ধ্রুবক branchp = innerForkBranchp(beginp);
                    AstBegin* ধ্রুবক jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } অন্যথা {
                AstVar* ধ্রুবক processQueuep
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
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        যদি (m_inFork) {
            nodep->replaceWith(নতুন AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* ধ্রুবক blockp = getJumpBlock(m_loopp, মিথ্যা);
            nodep->add_পরবর্তী_Here(নতুন AstJumpGo{nodep->fileline(), blockp});
        }
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(সত্য);
    }
    শূন্য visit(AstConst*) override {}
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Task class functions

শূন্য V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkjump", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
