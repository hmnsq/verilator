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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkJump.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ LinkJumpVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ಧ್ರುವ VNUser1InUse m_user1InUse;
    ಧ್ರುವ VNUser2InUse m_user2InUse;
    ಧ್ರುವ VNUser3InUse m_user3InUse;

    // STATE
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ಗಂಟು* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    ತಾರ್ಕಿಕ m_loopInc = ಅಸತ್ಯ;  // In loop increment
    ತಾರ್ಕಿಕ m_inFork = ಅಸತ್ಯ;  // Under fork
    ಪೂರ್ಣಾಙ್ಕ m_mod_Repeat_ಅಂಕೆ = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_ಗಂಟು_Block*> m_block_ರಾಶಿ;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ಧ್ರುವ AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ಧ್ರುವ AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ಧ್ರುವ AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ಧ್ರುವ AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ಧ್ರುವ AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        ಯದಿ (endOfIter) {
            ಯದಿ (nodep->user1p()) ಪ್ರತ್ಯಾಗಮ VN_AS(nodep->user1p(), JumpBlock);
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->user2p()) ಪ್ರತ್ಯಾಗಮ VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_ಗಂಟು* underp = nullptr;
        ತಾರ್ಕಿಕ under_and_next = ಸತ್ಯ;
        ಯದಿ (AstBegin* ಧ್ರುವ blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ fTaskp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } ಅನ್ಯಥಾ ಯದಿ (AstForeach* ಧ್ರುವ foreachp = VN_CAST(nodep, Foreach)) {
            ಯದಿ (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                ಯದಿ (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } ಅನ್ಯಥಾ {
                underp = nodep;
                under_and_next = ಅಸತ್ಯ;  // IE we skip the entire foreach
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstLoop* ಧ್ರುವ loopp = VN_CAST(nodep, Loop)) {
            ಯದಿ (endOfIter) {
                underp = loopp->stmtsp();
            } ಅನ್ಯಥಾ {
                underp = nodep;
                under_and_next = ಅಸತ್ಯ;  // IE we skip the entire loop
            }
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        ಯಾವತ್ (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        ಯದಿ (!underp->nextp() || !under_and_next) {
            ಯದಿ (AstJumpBlock* ಧ್ರುವ blockp = VN_CAST(underp, JumpBlock)) ಪ್ರತ್ಯಾಗಮ blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        ಯದಿ (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } ಅನ್ಯಥಾ {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* ಧ್ರುವ blockp = ನೂತನ AstJumpBlock{nodep->fileline(), underp};
        ಯದಿ (endOfIter) {
            nodep->user1p(blockp);
        } ಅನ್ಯಥಾ {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            ಯದಿ (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        ಪ್ರತ್ಯಾಗಮ blockp;
    }
    ಶೂನ್ಯ addPrefixToBlocksRecurse(ಧ್ರುವ std::string& prefix, Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(nodep, Begin)) {
            ಯದಿ (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    ತಾರ್ಕಿಕ existsBlockAbove(ಧ್ರುವ std::string& name) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_Block* ಧ್ರುವ stackp : vlstd::reverse_view(m_block_ರಾಶಿ)) {
            ಯದಿ (stackp->name() == name) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Module* findOwnerModulep(Ast_ಗಂಟು* nodep) {
        ಪ್ರತಿ (Ast_ಗಂಟು* curp = nodep; curp; curp = curp->aboveLoopp()) {
            ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(curp, ಗಂಟು_Module)) ಪ್ರತ್ಯಾಗಮ modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        ಪ್ರತ್ಯಾಗಮ nullptr;  // LCOV_EXCL_LINE
    }
    ಸ್ಥಿರ std::string targetInstancePath(ಧ್ರುವ AstDisable* ಧ್ರುವ nodep) {
        ಯದಿ (ಧ್ರುವ AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            ಪ್ರತ್ಯಾಗಮ xrefp->dotted();
        }
        ಪ್ರತ್ಯಾಗಮ "";
    }
    ಸ್ಥಿರ std::string queueDotted(ಧ್ರುವ AstDisable* ಧ್ರುವ nodep,
                                   ಧ್ರುವ AstVar* ಧ್ರುವ processQueuep) {
        ಪ್ರತ್ಯಾಗಮ processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(ಧ್ರುವ Ast_ಗಂಟು_Block* ಧ್ರುವ targetp) ಧ್ರುವ {
        AstBegin* innerForkBranchp = nullptr;
        Ast_ಗಂಟು_Block* prevBlockp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು_Block* ಧ್ರುವ blockp : vlstd::reverse_view(m_block_ರಾಶಿ)) {
            ಯದಿ (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            ಯದಿ (blockp == targetp) ಪ್ರತ್ಯಾಗಮ innerForkBranchp;
            prevBlockp = blockp;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* get_Queue_ತಳ್ಳು_Process_Selfp(AstVarRef* ಧ್ರುವ queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_ಸಾಲು* ಧ್ರುವ flp = queueRefp->fileline();
        ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{
            flp,
            ನೂತನ AstMethodCall{flp, queueRefp, "push_back",
                              ನೂತನ AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Var_Ref* newQueueRefp(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ processQueuep,
                                       ಧ್ರುವ VAccess& access, ಧ್ರುವ std::string& dotted = "") {
        ಯದಿ (dotted != "") ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarXRef{fl, processQueuep, dotted, access};
        ಯದಿ (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ಧ್ರುವ ತುದಿ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, ತುದಿ_Pkgp, processQueuep, access};
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* get_Queue_ತಳ್ಳು_Process_Selfp(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ processQueuep) {
        AstVarRef* ಧ್ರುವ queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        ಪ್ರತ್ಯಾಗಮ get_Queue_ತಳ್ಳು_Process_Selfp(queueWriteRefp);
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* getQueueKillStmtp(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ processQueuep,
                                          ಧ್ರುವ std::string& dotted = "") {
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            ಯದಿ (itemp->name() == "killQueue") {
                killQueueCall
                    = ನೂತನ AstTaskRef{fl, VN_AS(itemp, Task), ನೂತನ AstArg{fl, "", queueRefp}};
                ಭಙ್ಗ;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ಧ್ರುವ taskp, File_ಸಾಲು* ಧ್ರುವ fl) {
        ಧ್ರುವ auto it = m_taskDisableBegins.find(taskp);
        ಯದಿ (it != m_taskDisableBegins.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        AstBegin* ಧ್ರುವ taskBodyp = ನೂತನ AstBegin{fl, "", nullptr, ಅಸತ್ಯ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = stmtp->nextp();
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var);
            ಯದಿ (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        ಪ್ರತ್ಯಾಗಮ taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ಗಂಟು* ಧ್ರುವ nodep, File_ಸಾಲು* ಧ್ರುವ fl, ಧ್ರುವ VVarType& varType) {
        AstVar* ಧ್ರುವ processQueuep = ನೂತನ AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            ನೂತನ AstQueueDType{
                fl, VFlagChildDType{},
                ನೂತನ AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(ಸತ್ಯ);
        processQueuep->setIgnoreSchedWrite();
        ಪ್ರತ್ಯಾಗಮ processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ಗಂಟು* ಧ್ರುವ nodep, File_ಸಾಲು* ಧ್ರುವ fl) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ ownerp = findOwnerModulep(nodep);

        ಯದಿ (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ಧ್ರುವ ತುದಿ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ಧ್ರುವ processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            ತುದಿ_Pkgp->addStmtsp(processQueuep);
            ಪ್ರತ್ಯಾಗಮ processQueuep;
        }

        AstVar* ಧ್ರುವ processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        ಪ್ರತ್ಯಾಗಮ processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ಧ್ರುವ taskp, File_ಸಾಲು* ಧ್ರುವ fl) {
        ಧ್ರುವ auto it = m_taskDisableQueues.find(taskp);
        ಯದಿ (it != m_taskDisableQueues.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        AstVar* ಧ್ರುವ processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ತಳ್ಳು_Current_Processp = get_Queue_ತಳ್ಳು_Process_Selfp(fl, processQueuep);
        AstBegin* ಧ್ರುವ taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ತಳ್ಳು_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        ಪ್ರತ್ಯಾಗಮ processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ಧ್ರುವ beginp, File_ಸಾಲು* ಧ್ರುವ fl) {
        ಧ್ರುವ auto it = m_beginDisableBegins.find(beginp);
        ಯದಿ (it != m_beginDisableBegins.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        AstBegin* ಧ್ರುವ beginBodyp = ನೂತನ AstBegin{fl, "", nullptr, ಅಸತ್ಯ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        ಯದಿ (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        ಪ್ರತ್ಯಾಗಮ beginBodyp;
    }
    ಶೂನ್ಯ prependForkBranchQueuePushes(AstFork* ಧ್ರುವ forkp, AstVar* ಧ್ರುವ processQueuep,
                                      File_ಸಾಲು* ಧ್ರುವ fl, ತಾರ್ಕಿಕ needProcess) {
        ಪ್ರತಿ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            ಯದಿ (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ತಳ್ಳು_Process_Selfp(fl, processQueuep));
        }
    }
    ಶೂನ್ಯ prependNestedForkBranchQueuePushes(Ast_ಗಂಟು* ಧ್ರುವ nodep, AstVar* ಧ್ರುವ processQueuep,
                                            File_ಸಾಲು* ಧ್ರುವ fl, ತಾರ್ಕಿಕ needProcess) {
        nodep->foreach([&](AstFork* ಧ್ರುವ forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ಧ್ರುವ beginp, File_ಸಾಲು* ಧ್ರುವ fl) {
        ಧ್ರುವ auto it = m_beginDisableQueues.find(beginp);
        ಯದಿ (it != m_beginDisableQueues.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        AstVar* ಧ್ರುವ processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ತಳ್ಳು_Current_Processp = get_Queue_ತಳ್ಳು_Process_Selfp(fl, processQueuep);
        AstBegin* ಧ್ರುವ beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ತಳ್ಳು_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, ಅಸತ್ಯ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        ಪ್ರತ್ಯಾಗಮ processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ಧ್ರುವ forkp, File_ಸಾಲು* ಧ್ರುವ fl) {
        ಧ್ರುವ auto it = m_forkDisableQueues.find(forkp);
        ಯದಿ (it != m_forkDisableQueues.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        AstVar* ಧ್ರುವ processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, ಸತ್ಯ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        ಪ್ರತಿ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, ಸತ್ಯ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        ಪ್ರತ್ಯಾಗಮ processQueuep;
    }
    Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸೇರಿಸು_Kill_Stmtp(AstDisable* ಧ್ರುವ nodep, AstVar* ಧ್ರುವ processQueuep) {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        ಪ್ರತ್ಯಾಗಮ killStmtp;
    }
    ಶೂನ್ಯ addJumpAfterKill(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ killStmtp, Ast_ಗಂಟು* ಧ್ರುವ targetp) {
        AstJumpBlock* ಧ್ರುವ jmpBlockp = getJumpBlock(targetp, ಅಸತ್ಯ);
        killStmtp->addNextHere(ನೂತನ AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    ಶೂನ್ಯ handleDisableOnFork(AstDisable* ಧ್ರುವ nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstFork* ಧ್ರುವ targetp = VN_AS(nodep->targetp(), Fork);
        ಯದಿ (m_ftaskp) {
            ಯದಿ (!m_ftaskp->exists(
                    [targetp](ಧ್ರುವ Ast_ಗಂಟು_Block* blockp) -> ತಾರ್ಕಿಕ { ಪ್ರತ್ಯಾಗಮ blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ಧ್ರುವ processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ killStmtp = ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        ಯದಿ (AstBegin* ಧ್ರುವ branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->dead()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_ಅಂಕೆ);
        m_modp = nodep;
        m_mod_Repeat_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_ರಾಶಿ.ತಳ್ಳು_back(nodep);
        iterateChildren(nodep);
        m_block_ರಾಶಿ.ತೆಗೆ_back();
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = ಸತ್ಯ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        ಪ್ರತಿ (Ast_ಗಂಟು_Block* ಧ್ರುವ blockp : vlstd::reverse_view(m_block_ರಾಶಿ)) {
            ಯದಿ (blockp->user3SetOnce()) ಭಙ್ಗ;
        }
        m_block_ರಾಶಿ.ತಳ್ಳು_back(nodep);
        iterateChildren(nodep);
        m_block_ರಾಶಿ.ತೆಗೆ_back();
    }
    ಶೂನ್ಯ visit(AstStmtPragma* nodep) override {
        ಯದಿ (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ countp = nodep->countp()->unlinkFrBackWithNext();
        ಧ್ರುವ string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_ಅಂಕೆ++);
        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{nodep->fileline(), "", nullptr, ಸತ್ಯ};
        // Spec says value is integral, if negative is ignored
        AstVar* ಧ್ರುವ varp
            = ನೂತನ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(ಸತ್ಯ);
        beginp->addStmtsp(varp);
        Ast_ಗಂಟು* initsp = ನೂತನ AstAssign{
            nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ಗಂಟು* ಧ್ರುವ decp = ನೂತನ AstAssign{
            nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            ನೂತನ AstSub{nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       ನೂತನ AstConst{nodep->fileline(), 1}}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ zerosp = ನೂತನ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstGtS{
            nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ಗಂಟು* ಧ್ರುವ bodysp = nodep->stmtsp();
        ಯದಿ (bodysp) bodysp->unlinkFrBackWithNext();
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        ಯದಿ (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        ಯದಿ (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        ಯದಿ (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, ಸತ್ಯ);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = ಅಸತ್ಯ;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = ಸತ್ಯ;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        ಯದಿ (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    ಶೂನ್ಯ visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        ಧ್ರುವ AstFunc* ಧ್ರುವ funcp = VN_CAST(m_ftaskp, Func);
        ಯದಿ (m_randsequencep) {
            nodep->replaceWith(ನೂತನ AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } ಅನ್ಯಥಾ ಯದಿ (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } ಅನ್ಯಥಾ ಯದಿ (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } ಅನ್ಯಥಾ {
            ಯದಿ (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(ನೂತನ AstAssign{
                    nodep->fileline(),
                    ನೂತನ AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* ಧ್ರುವ blockp = getJumpBlock(m_ftaskp, ಅಸತ್ಯ);
            nodep->addHereThisAsNext(ನೂತನ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!m_loopp && m_randsequencep) {
            nodep->replaceWith(ನೂತನ AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } ಅನ್ಯಥಾ {
            // Jump to the end of the loop
            AstJumpBlock* ಧ್ರುವ blockp = getJumpBlock(m_loopp, ಅಸತ್ಯ);
            nodep->addNextHere(ನೂತನ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } ಅನ್ಯಥಾ {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* ಧ್ರುವ blockp = getJumpBlock(m_loopp, ಸತ್ಯ);
            nodep->addNextHere(ನೂತನ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ಗಂಟು* ಧ್ರುವ targetp = nodep->targetp();
        ಯದಿ (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (AstTask* ಧ್ರುವ taskp = VN_CAST(targetp, Task)) {
            AstVar* ಧ್ರುವ processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ killStmtp = ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            ಯದಿ (m_ftaskp == taskp) {
                Ast_ಗಂಟು* jumpTargetp = taskp;
                ಧ್ರುವ auto it = m_taskDisableBegins.find(taskp);
                ಯದಿ (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(targetp, Begin)) {
            ಯದಿ (existsBlockAbove(beginp->name())) {
                ಯದಿ (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* ಧ್ರುವ blockp = getJumpBlock(beginp, ಅಸತ್ಯ);
                    nodep->addNextHere(ನೂತನ AstJumpGo{nodep->fileline(), blockp});
                } ಅನ್ಯಥಾ {
                    AstVar* ಧ್ರುವ processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ killStmtp = ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ಧ್ರುವ branchp = innerForkBranchp(beginp);
                    AstBegin* ಧ್ರುವ jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } ಅನ್ಯಥಾ {
                AstVar* ಧ್ರುವ processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);
            }
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstFinish* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        ಯದಿ (m_inFork) {
            nodep->replaceWith(ನೂತನ AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* ಧ್ರುವ blockp = getJumpBlock(m_loopp, ಅಸತ್ಯ);
            nodep->addNextHere(ನೂತನ AstJumpGo{nodep->fileline(), blockp});
        }
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstConst*) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Task class functions

ಶೂನ್ಯ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkjump", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
