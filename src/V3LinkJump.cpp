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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkJump.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ LinkJumpVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ଅଚଳ VNUser1InUse m_user1InUse;
    ଅଚଳ VNUser2InUse m_user2InUse;
    ଅଚଳ VNUser3InUse m_user3InUse;

    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ଗଣ୍ଠି* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    ତାର୍କିକ m_loopInc = ମିଥ୍ୟା;  // In loop increment
    ତାର୍କିକ m_inFork = ମିଥ୍ୟା;  // Under fork
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_mod_Repeat_ଅଙ୍କ = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_ଗଣ୍ଠି_Block*> m_block_ଗଦା;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ଅଚଳ AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ଅଚଳ AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ଅଚଳ AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ଅଚଳ AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ଅଚଳ AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        ଯଦି (endOfIter) {
            ଯଦି (nodep->user1p()) ଫେରନ୍ତୁ VN_AS(nodep->user1p(), JumpBlock);
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->user2p()) ଫେରନ୍ତୁ VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_ଗଣ୍ଠି* underp = nullptr;
        ତାର୍କିକ under_and_next = ସତ୍ୟ;
        ଯଦି (AstBegin* ଅଚଳ blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ fTaskp = VN_CAST(nodep, ଗଣ୍ଠି_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } ଅନ୍ୟଥା ଯଦି (AstForeach* ଅଚଳ foreachp = VN_CAST(nodep, Foreach)) {
            ଯଦି (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                ଯଦି (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } ଅନ୍ୟଥା {
                underp = nodep;
                under_and_next = ମିଥ୍ୟା;  // IE we skip the entire foreach
            }
        } ଅନ୍ୟଥା ଯଦି (AstLoop* ଅଚଳ loopp = VN_CAST(nodep, Loop)) {
            ଯଦି (endOfIter) {
                underp = loopp->stmtsp();
            } ଅନ୍ୟଥା {
                underp = nodep;
                under_and_next = ମିଥ୍ୟା;  // IE we skip the entire loop
            }
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            ଫେରନ୍ତୁ nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        ଯେତେବେଳେ (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        ଯଦି (!underp->nextp() || !under_and_next) {
            ଯଦି (AstJumpBlock* ଅଚଳ blockp = VN_CAST(underp, JumpBlock)) ଫେରନ୍ତୁ blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        ଯଦି (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } ଅନ୍ୟଥା {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* ଅଚଳ blockp = ନୂତନ AstJumpBlock{nodep->fileline(), underp};
        ଯଦି (endOfIter) {
            nodep->user1p(blockp);
        } ଅନ୍ୟଥା {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            ଯଦି (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        ଫେରନ୍ତୁ blockp;
    }
    ଶୂନ୍ୟ addPrefixToBlocksRecurse(ଅଚଳ std::string& prefix, Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(nodep, Begin)) {
            ଯଦି (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    ତାର୍କିକ existsBlockAbove(ଅଚଳ std::string& name) ଅଚଳ {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_Block* ଅଚଳ stackp : vlstd::reverse_view(m_block_ଗଦା)) {
            ଯଦି (stackp->name() == name) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Module* findOwnerModulep(Ast_ଗଣ୍ଠି* nodep) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* curp = nodep; curp; curp = curp->aboveLoopp()) {
            ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(curp, ଗଣ୍ଠି_Module)) ଫେରନ୍ତୁ modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        ଫେରନ୍ତୁ nullptr;  // LCOV_EXCL_LINE
    }
    ସ୍ଥିର std::string targetInstancePath(ଅଚଳ AstDisable* ଅଚଳ nodep) {
        ଯଦି (ଅଚଳ AstVarXRef* ଅଚଳ xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            ଫେରନ୍ତୁ xrefp->dotted();
        }
        ଫେରନ୍ତୁ "";
    }
    ସ୍ଥିର std::string queueDotted(ଅଚଳ AstDisable* ଅଚଳ nodep,
                                   ଅଚଳ AstVar* ଅଚଳ processQueuep) {
        ଫେରନ୍ତୁ processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(ଅଚଳ Ast_ଗଣ୍ଠି_Block* ଅଚଳ targetp) ଅଚଳ {
        AstBegin* innerForkBranchp = nullptr;
        Ast_ଗଣ୍ଠି_Block* prevBlockp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Block* ଅଚଳ blockp : vlstd::reverse_view(m_block_ଗଦା)) {
            ଯଦି (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            ଯଦି (blockp == targetp) ଫେରନ୍ତୁ innerForkBranchp;
            prevBlockp = blockp;
        }
        ଫେରନ୍ତୁ nullptr;
    }
    ସ୍ଥିର Ast_Stmt_ପଦସମଷ୍ଟି* get_Queue_ଠେଲ_Process_Selfp(AstVarRef* ଅଚଳ queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_ଧାଡ଼ି* ଅଚଳ flp = queueRefp->fileline();
        ଫେରନ୍ତୁ ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{
            flp,
            ନୂତନ AstMethodCall{flp, queueRefp, "push_back",
                              ନୂତନ AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Var_Ref* newQueueRefp(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ processQueuep,
                                       ଅଚଳ VAccess& access, ଅଚଳ std::string& dotted = "") {
        ଯଦି (dotted != "") ଫେରନ୍ତୁ ନୂତନ AstVarXRef{fl, processQueuep, dotted, access};
        ଯଦି (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ଅଚଳ ଶୀର୍ଷ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, ଶୀର୍ଷ_Pkgp, processQueuep, access};
    }
    ସ୍ଥିର Ast_Stmt_ପଦସମଷ୍ଟି* get_Queue_ଠେଲ_Process_Selfp(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ processQueuep) {
        AstVarRef* ଅଚଳ queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        ଫେରନ୍ତୁ get_Queue_ଠେଲ_Process_Selfp(queueWriteRefp);
    }
    ସ୍ଥିର Ast_Stmt_ପଦସମଷ୍ଟି* getQueueKillStmtp(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ processQueuep,
                                          ଅଚଳ std::string& dotted = "") {
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            ଯଦି (itemp->name() == "killQueue") {
                killQueueCall
                    = ନୂତନ AstTaskRef{fl, VN_AS(itemp, Task), ନୂତନ AstArg{fl, "", queueRefp}};
                ଭାଙ୍ଗ;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        ଫେରନ୍ତୁ ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ଅଚଳ taskp, File_ଧାଡ଼ି* ଅଚଳ fl) {
        ଅଚଳ auto it = m_taskDisableBegins.find(taskp);
        ଯଦି (it != m_taskDisableBegins.end()) ଫେରନ୍ତୁ it->second;

        AstBegin* ଅଚଳ taskBodyp = ନୂତନ AstBegin{fl, "", nullptr, ମିଥ୍ୟା};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = stmtp->nextp();
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var);
            ଯଦି (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* ଅଚଳ forkp = ନୂତନ AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        ଫେରନ୍ତୁ taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, File_ଧାଡ଼ି* ଅଚଳ fl, ଅଚଳ VVarType& varType) {
        AstVar* ଅଚଳ processQueuep = ନୂତନ AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            ନୂତନ AstQueueDType{
                fl, VFlagChildDType{},
                ନୂତନ AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(ସତ୍ୟ);
        processQueuep->setIgnoreSchedWrite();
        ଫେରନ୍ତୁ processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, File_ଧାଡ଼ି* ଅଚଳ fl) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerp = findOwnerModulep(nodep);

        ଯଦି (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ଅଚଳ ଶୀର୍ଷ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ଅଚଳ processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            ଶୀର୍ଷ_Pkgp->addStmtsp(processQueuep);
            ଫେରନ୍ତୁ processQueuep;
        }

        AstVar* ଅଚଳ processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        ଫେରନ୍ତୁ processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ଅଚଳ taskp, File_ଧାଡ଼ି* ଅଚଳ fl) {
        ଅଚଳ auto it = m_taskDisableQueues.find(taskp);
        ଯଦି (it != m_taskDisableQueues.end()) ଫେରନ୍ତୁ it->second;

        AstVar* ଅଚଳ processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ ଠେଲ_Current_Processp = get_Queue_ଠେଲ_Process_Selfp(fl, processQueuep);
        AstBegin* ଅଚଳ taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ଠେଲ_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        ଫେରନ୍ତୁ processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ଅଚଳ beginp, File_ଧାଡ଼ି* ଅଚଳ fl) {
        ଅଚଳ auto it = m_beginDisableBegins.find(beginp);
        ଯଦି (it != m_beginDisableBegins.end()) ଫେରନ୍ତୁ it->second;

        AstBegin* ଅଚଳ beginBodyp = ନୂତନ AstBegin{fl, "", nullptr, ମିଥ୍ୟା};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        ଯଦି (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* ଅଚଳ forkp = ନୂତନ AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        ଫେରନ୍ତୁ beginBodyp;
    }
    ଶୂନ୍ୟ prependForkBranchQueuePushes(AstFork* ଅଚଳ forkp, AstVar* ଅଚଳ processQueuep,
                                      File_ଧାଡ଼ି* ଅଚଳ fl, ତାର୍କିକ needProcess) {
        ପାଇଁ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            ଯଦି (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ଠେଲ_Process_Selfp(fl, processQueuep));
        }
    }
    ଶୂନ୍ୟ prependNestedForkBranchQueuePushes(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, AstVar* ଅଚଳ processQueuep,
                                            File_ଧାଡ଼ି* ଅଚଳ fl, ତାର୍କିକ needProcess) {
        nodep->foreach([&](AstFork* ଅଚଳ forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ଅଚଳ beginp, File_ଧାଡ଼ି* ଅଚଳ fl) {
        ଅଚଳ auto it = m_beginDisableQueues.find(beginp);
        ଯଦି (it != m_beginDisableQueues.end()) ଫେରନ୍ତୁ it->second;

        AstVar* ଅଚଳ processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ ଠେଲ_Current_Processp = get_Queue_ଠେଲ_Process_Selfp(fl, processQueuep);
        AstBegin* ଅଚଳ beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ଠେଲ_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, ମିଥ୍ୟା);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        ଫେରନ୍ତୁ processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ଅଚଳ forkp, File_ଧାଡ଼ି* ଅଚଳ fl) {
        ଅଚଳ auto it = m_forkDisableQueues.find(forkp);
        ଯଦି (it != m_forkDisableQueues.end()) ଫେରନ୍ତୁ it->second;

        AstVar* ଅଚଳ processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, ସତ୍ୟ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        ପାଇଁ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, ସତ୍ୟ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        ଫେରନ୍ତୁ processQueuep;
    }
    Ast_Stmt_ପଦସମଷ୍ଟି* ଭର୍ତ୍ତି_Kill_Stmtp(AstDisable* ଅଚଳ nodep, AstVar* ଅଚଳ processQueuep) {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        ଫେରନ୍ତୁ killStmtp;
    }
    ଶୂନ୍ୟ addJumpAfterKill(Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ killStmtp, Ast_ଗଣ୍ଠି* ଅଚଳ targetp) {
        AstJumpBlock* ଅଚଳ jmpBlockp = getJumpBlock(targetp, ମିଥ୍ୟା);
        killStmtp->addNextHere(ନୂତନ AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    ଶୂନ୍ୟ handleDisableOnFork(AstDisable* ଅଚଳ nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstFork* ଅଚଳ targetp = VN_AS(nodep->targetp(), Fork);
        ଯଦି (m_ftaskp) {
            ଯଦି (!m_ftaskp->exists(
                    [targetp](ଅଚଳ Ast_ଗଣ୍ଠି_Block* blockp) -> ତାର୍କିକ { ଫେରନ୍ତୁ blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ଅଚଳ processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ killStmtp = ଭର୍ତ୍ତି_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        ଯଦି (AstBegin* ଅଚଳ branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->dead()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_ଅଙ୍କ);
        m_modp = nodep;
        m_mod_Repeat_ଅଙ୍କ = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_ଗଦା.ଠେଲ_back(nodep);
        iterateChildren(nodep);
        m_block_ଗଦା.କାଢ଼_back();
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = ସତ୍ୟ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        ପାଇଁ (Ast_ଗଣ୍ଠି_Block* ଅଚଳ blockp : vlstd::reverse_view(m_block_ଗଦା)) {
            ଯଦି (blockp->user3SetOnce()) ଭାଙ୍ଗ;
        }
        m_block_ଗଦା.ଠେଲ_back(nodep);
        iterateChildren(nodep);
        m_block_ଗଦା.କାଢ଼_back();
    }
    ଶୂନ୍ୟ visit(AstStmtPragma* nodep) override {
        ଯଦି (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ countp = nodep->countp()->unlinkFrBackWithNext();
        ଅଚଳ string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_ଅଙ୍କ++);
        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{nodep->fileline(), "", nullptr, ସତ୍ୟ};
        // Spec says value is integral, if negative is ignored
        AstVar* ଅଚଳ varp
            = ନୂତନ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(ସତ୍ୟ);
        beginp->addStmtsp(varp);
        Ast_ଗଣ୍ଠି* initsp = ନୂତନ AstAssign{
            nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ଗଣ୍ଠି* ଅଚଳ decp = ନୂତନ AstAssign{
            nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            ନୂତନ AstSub{nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       ନୂତନ AstConst{nodep->fileline(), 1}}};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ zerosp = ନୂତନ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstGtS{
            nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ଗଣ୍ଠି* ଅଚଳ bodysp = nodep->stmtsp();
        ଯଦି (bodysp) bodysp->unlinkFrBackWithNext();
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        ଯଦି (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        ଯଦି (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        ଯଦି (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, ସତ୍ୟ);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = ମିଥ୍ୟା;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = ସତ୍ୟ;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        ଯଦି (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    ଶୂନ୍ୟ visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        ଅଚଳ AstFunc* ଅଚଳ funcp = VN_CAST(m_ftaskp, Func);
        ଯଦି (m_randsequencep) {
            nodep->replaceWith(ନୂତନ AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } ଅନ୍ୟଥା ଯଦି (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } ଅନ୍ୟଥା ଯଦି (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } ଅନ୍ୟଥା {
            ଯଦି (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(ନୂତନ AstAssign{
                    nodep->fileline(),
                    ନୂତନ AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* ଅଚଳ blockp = getJumpBlock(m_ftaskp, ମିଥ୍ୟା);
            nodep->addHereThisAsNext(ନୂତନ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!m_loopp && m_randsequencep) {
            nodep->replaceWith(ନୂତନ AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } ଅନ୍ୟଥା {
            // Jump to the end of the loop
            AstJumpBlock* ଅଚଳ blockp = getJumpBlock(m_loopp, ମିଥ୍ୟା);
            nodep->addNextHere(ନୂତନ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } ଅନ୍ୟଥା {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* ଅଚଳ blockp = getJumpBlock(m_loopp, ସତ୍ୟ);
            nodep->addNextHere(ନୂତନ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ଗଣ୍ଠି* ଅଚଳ targetp = nodep->targetp();
        ଯଦି (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (AstTask* ଅଚଳ taskp = VN_CAST(targetp, Task)) {
            AstVar* ଅଚଳ processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ killStmtp = ଭର୍ତ୍ତି_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            ଯଦି (m_ftaskp == taskp) {
                Ast_ଗଣ୍ଠି* jumpTargetp = taskp;
                ଅଚଳ auto it = m_taskDisableBegins.find(taskp);
                ଯଦି (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } ଅନ୍ୟଥା ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(targetp, Begin)) {
            ଯଦି (existsBlockAbove(beginp->name())) {
                ଯଦି (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* ଅଚଳ blockp = getJumpBlock(beginp, ମିଥ୍ୟା);
                    nodep->addNextHere(ନୂତନ AstJumpGo{nodep->fileline(), blockp});
                } ଅନ୍ୟଥା {
                    AstVar* ଅଚଳ processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ killStmtp = ଭର୍ତ୍ତି_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ଅଚଳ branchp = innerForkBranchp(beginp);
                    AstBegin* ଅଚଳ jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } ଅନ୍ୟଥା {
                AstVar* ଅଚଳ processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ଭର୍ତ୍ତି_Kill_Stmtp(nodep, processQueuep);
            }
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstFinish* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        ଯଦି (m_inFork) {
            nodep->replaceWith(ନୂତନ AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* ଅଚଳ blockp = getJumpBlock(m_loopp, ମିଥ୍ୟା);
            nodep->addNextHere(ନୂତନ AstJumpGo{nodep->fileline(), blockp});
        }
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstConst*) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Task class functions

ଶୂନ୍ୟ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkjump", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
