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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkJump.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം LinkJumpVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser2InUse m_user2InUse;
    സ്ഥിരം VNUser3InUse m_user3InUse;

    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_കണ്ണി* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    തർക്കം m_loopInc = അസത്യം;  // In loop increment
    തർക്കം m_inFork = അസത്യം;  // Under fork
    പൂർണ്ണസംഖ്യ m_mod_Repeat_അക്കം = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_കണ്ണി_Block*> m_block_അട്ടി;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<സ്ഥിരം AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<സ്ഥിരം AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<സ്ഥിരം AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<സ്ഥിരം AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<സ്ഥിരം AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_കണ്ണി* nodep, തർക്കം endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        എങ്കിൽ (endOfIter) {
            എങ്കിൽ (nodep->user1p()) തിരികെ VN_AS(nodep->user1p(), JumpBlock);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->user2p()) തിരികെ VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_കണ്ണി* underp = nullptr;
        തർക്കം under_and_next = സത്യം;
        എങ്കിൽ (AstBegin* സ്ഥിരം blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം fTaskp = VN_CAST(nodep, കണ്ണി_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstForeach* സ്ഥിരം foreachp = VN_CAST(nodep, Foreach)) {
            എങ്കിൽ (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                എങ്കിൽ (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } അല്ലെങ്കിൽ {
                underp = nodep;
                under_and_next = അസത്യം;  // IE we skip the entire foreach
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstLoop* സ്ഥിരം loopp = VN_CAST(nodep, Loop)) {
            എങ്കിൽ (endOfIter) {
                underp = loopp->stmtsp();
            } അല്ലെങ്കിൽ {
                underp = nodep;
                under_and_next = അസത്യം;  // IE we skip the entire loop
            }
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            തിരികെ nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        ആകുമ്പോൾ (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        എങ്കിൽ (!underp->nextp() || !under_and_next) {
            എങ്കിൽ (AstJumpBlock* സ്ഥിരം blockp = VN_CAST(underp, JumpBlock)) തിരികെ blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        എങ്കിൽ (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } അല്ലെങ്കിൽ {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* സ്ഥിരം blockp = പുതിയ AstJumpBlock{nodep->fileline(), underp};
        എങ്കിൽ (endOfIter) {
            nodep->user1p(blockp);
        } അല്ലെങ്കിൽ {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        വേണ്ടി (Ast_കണ്ണി *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            എങ്കിൽ (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        തിരികെ blockp;
    }
    ശൂന്യം addPrefixToBlocksRecurse(സ്ഥിരം std::string& prefix, Ast_കണ്ണി* സ്ഥിരം nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(nodep, Begin)) {
            എങ്കിൽ (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    തർക്കം existsBlockAbove(സ്ഥിരം std::string& name) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_Block* സ്ഥിരം stackp : vlstd::reverse_view(m_block_അട്ടി)) {
            എങ്കിൽ (stackp->name() == name) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    സ്ഥിതം Ast_കണ്ണി_Module* findOwnerModulep(Ast_കണ്ണി* nodep) {
        വേണ്ടി (Ast_കണ്ണി* curp = nodep; curp; curp = curp->aboveLoopp()) {
            എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(curp, കണ്ണി_Module)) തിരികെ modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        തിരികെ nullptr;  // LCOV_EXCL_LINE
    }
    സ്ഥിതം std::string targetInstancePath(സ്ഥിരം AstDisable* സ്ഥിരം nodep) {
        എങ്കിൽ (സ്ഥിരം AstVarXRef* സ്ഥിരം xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            തിരികെ xrefp->dotted();
        }
        തിരികെ "";
    }
    സ്ഥിതം std::string queueDotted(സ്ഥിരം AstDisable* സ്ഥിരം nodep,
                                   സ്ഥിരം AstVar* സ്ഥിരം processQueuep) {
        തിരികെ processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(സ്ഥിരം Ast_കണ്ണി_Block* സ്ഥിരം targetp) സ്ഥിരം {
        AstBegin* innerForkBranchp = nullptr;
        Ast_കണ്ണി_Block* prevBlockp = nullptr;
        വേണ്ടി (Ast_കണ്ണി_Block* സ്ഥിരം blockp : vlstd::reverse_view(m_block_അട്ടി)) {
            എങ്കിൽ (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            എങ്കിൽ (blockp == targetp) തിരികെ innerForkBranchp;
            prevBlockp = blockp;
        }
        തിരികെ nullptr;
    }
    സ്ഥിതം Ast_Stmt_വ്യഞ്ജകം* get_Queue_തള്ളുക_Process_Selfp(AstVarRef* സ്ഥിരം queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_വരി* സ്ഥിരം flp = queueRefp->fileline();
        തിരികെ പുതിയ Ast_Stmt_വ്യഞ്ജകം{
            flp,
            പുതിയ AstMethodCall{flp, queueRefp, "push_back",
                              പുതിയ AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    സ്ഥിതം Ast_കണ്ണി_Var_Ref* newQueueRefp(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം processQueuep,
                                       സ്ഥിരം VAccess& access, സ്ഥിരം std::string& dotted = "") {
        എങ്കിൽ (dotted != "") തിരികെ പുതിയ AstVarXRef{fl, processQueuep, dotted, access};
        എങ്കിൽ (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            തിരികെ പുതിയ AstVarRef{fl, processQueuep, access};
        }
        AstPackage* സ്ഥിരം മുകൾ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        തിരികെ പുതിയ AstVarRef{fl, മുകൾ_Pkgp, processQueuep, access};
    }
    സ്ഥിതം Ast_Stmt_വ്യഞ്ജകം* get_Queue_തള്ളുക_Process_Selfp(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം processQueuep) {
        AstVarRef* സ്ഥിരം queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        തിരികെ get_Queue_തള്ളുക_Process_Selfp(queueWriteRefp);
    }
    സ്ഥിതം Ast_Stmt_വ്യഞ്ജകം* getQueueKillStmtp(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം processQueuep,
                                          സ്ഥിരം std::string& dotted = "") {
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        വേണ്ടി (Ast_കണ്ണി* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            എങ്കിൽ (itemp->name() == "killQueue") {
                killQueueCall
                    = പുതിയ AstTaskRef{fl, VN_AS(itemp, Task), പുതിയ AstArg{fl, "", queueRefp}};
                നിർത്തുക;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        തിരികെ പുതിയ Ast_Stmt_വ്യഞ്ജകം{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* സ്ഥിരം taskp, File_വരി* സ്ഥിരം fl) {
        സ്ഥിരം auto it = m_taskDisableBegins.find(taskp);
        എങ്കിൽ (it != m_taskDisableBegins.end()) തിരികെ it->second;

        AstBegin* സ്ഥിരം taskBodyp = പുതിയ AstBegin{fl, "", nullptr, അസത്യം};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        വേണ്ടി (Ast_കണ്ണി* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_കണ്ണി* സ്ഥിരം nextp = stmtp->nextp();
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* സ്ഥിരം forkp = പുതിയ AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        തിരികെ taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_കണ്ണി* സ്ഥിരം nodep, File_വരി* സ്ഥിരം fl, സ്ഥിരം VVarType& varType) {
        AstVar* സ്ഥിരം processQueuep = പുതിയ AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            പുതിയ AstQueueDType{
                fl, VFlagChildDType{},
                പുതിയ AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(സത്യം);
        processQueuep->setIgnoreSchedWrite();
        തിരികെ processQueuep;
    }
    AstVar* getProcessQueuep(Ast_കണ്ണി* സ്ഥിരം nodep, File_വരി* സ്ഥിരം fl) {
        Ast_കണ്ണി_Module* സ്ഥിരം ownerp = findOwnerModulep(nodep);

        എങ്കിൽ (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* സ്ഥിരം മുകൾ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* സ്ഥിരം processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            മുകൾ_Pkgp->addStmtsp(processQueuep);
            തിരികെ processQueuep;
        }

        AstVar* സ്ഥിരം processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        തിരികെ processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* സ്ഥിരം taskp, File_വരി* സ്ഥിരം fl) {
        സ്ഥിരം auto it = m_taskDisableQueues.find(taskp);
        എങ്കിൽ (it != m_taskDisableQueues.end()) തിരികെ it->second;

        AstVar* സ്ഥിരം processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം തള്ളുക_Current_Processp = get_Queue_തള്ളുക_Process_Selfp(fl, processQueuep);
        AstBegin* സ്ഥിരം taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(തള്ളുക_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        തിരികെ processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* സ്ഥിരം beginp, File_വരി* സ്ഥിരം fl) {
        സ്ഥിരം auto it = m_beginDisableBegins.find(beginp);
        എങ്കിൽ (it != m_beginDisableBegins.end()) തിരികെ it->second;

        AstBegin* സ്ഥിരം beginBodyp = പുതിയ AstBegin{fl, "", nullptr, അസത്യം};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        എങ്കിൽ (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* സ്ഥിരം forkp = പുതിയ AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        തിരികെ beginBodyp;
    }
    ശൂന്യം prependForkBranchQueuePushes(AstFork* സ്ഥിരം forkp, AstVar* സ്ഥിരം processQueuep,
                                      File_വരി* സ്ഥിരം fl, തർക്കം needProcess) {
        വേണ്ടി (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            എങ്കിൽ (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_തള്ളുക_Process_Selfp(fl, processQueuep));
        }
    }
    ശൂന്യം prependNestedForkBranchQueuePushes(Ast_കണ്ണി* സ്ഥിരം nodep, AstVar* സ്ഥിരം processQueuep,
                                            File_വരി* സ്ഥിരം fl, തർക്കം needProcess) {
        nodep->foreach([&](AstFork* സ്ഥിരം forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* സ്ഥിരം beginp, File_വരി* സ്ഥിരം fl) {
        സ്ഥിരം auto it = m_beginDisableQueues.find(beginp);
        എങ്കിൽ (it != m_beginDisableQueues.end()) തിരികെ it->second;

        AstVar* സ്ഥിരം processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം തള്ളുക_Current_Processp = get_Queue_തള്ളുക_Process_Selfp(fl, processQueuep);
        AstBegin* സ്ഥിരം beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(തള്ളുക_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, അസത്യം);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        തിരികെ processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* സ്ഥിരം forkp, File_വരി* സ്ഥിരം fl) {
        സ്ഥിരം auto it = m_forkDisableQueues.find(forkp);
        എങ്കിൽ (it != m_forkDisableQueues.end()) തിരികെ it->second;

        AstVar* സ്ഥിരം processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, സത്യം);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        വേണ്ടി (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, സത്യം);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        തിരികെ processQueuep;
    }
    Ast_Stmt_വ്യഞ്ജകം* തിരുകുക_Kill_Stmtp(AstDisable* സ്ഥിരം nodep, AstVar* സ്ഥിരം processQueuep) {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        തിരികെ killStmtp;
    }
    ശൂന്യം addJumpAfterKill(Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം killStmtp, Ast_കണ്ണി* സ്ഥിരം targetp) {
        AstJumpBlock* സ്ഥിരം jmpBlockp = getJumpBlock(targetp, അസത്യം);
        killStmtp->addNextHere(പുതിയ AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    ശൂന്യം handleDisableOnFork(AstDisable* സ്ഥിരം nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstFork* സ്ഥിരം targetp = VN_AS(nodep->targetp(), Fork);
        എങ്കിൽ (m_ftaskp) {
            എങ്കിൽ (!m_ftaskp->exists(
                    [targetp](സ്ഥിരം Ast_കണ്ണി_Block* blockp) -> തർക്കം { തിരികെ blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* സ്ഥിരം processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം killStmtp = തിരുകുക_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        എങ്കിൽ (AstBegin* സ്ഥിരം branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->dead()) തിരികെ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_അക്കം);
        m_modp = nodep;
        m_mod_Repeat_അക്കം = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_അട്ടി.തള്ളുക_back(nodep);
        iterateChildren(nodep);
        m_block_അട്ടി.എടുക്കുക_back();
    }
    ശൂന്യം visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = സത്യം;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        വേണ്ടി (Ast_കണ്ണി_Block* സ്ഥിരം blockp : vlstd::reverse_view(m_block_അട്ടി)) {
            എങ്കിൽ (blockp->user3SetOnce()) നിർത്തുക;
        }
        m_block_അട്ടി.തള്ളുക_back(nodep);
        iterateChildren(nodep);
        m_block_അട്ടി.എടുക്കുക_back();
    }
    ശൂന്യം visit(AstStmtPragma* nodep) override {
        എങ്കിൽ (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം countp = nodep->countp()->unlinkFrBackWithNext();
        സ്ഥിരം string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_അക്കം++);
        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{nodep->fileline(), "", nullptr, സത്യം};
        // Spec says value is integral, if negative is ignored
        AstVar* സ്ഥിരം varp
            = പുതിയ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(സത്യം);
        beginp->addStmtsp(varp);
        Ast_കണ്ണി* initsp = പുതിയ AstAssign{
            nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_കണ്ണി* സ്ഥിരം decp = പുതിയ AstAssign{
            nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            പുതിയ AstSub{nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       പുതിയ AstConst{nodep->fileline(), 1}}};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം zerosp = പുതിയ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstGtS{
            nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_കണ്ണി* സ്ഥിരം bodysp = nodep->stmtsp();
        എങ്കിൽ (bodysp) bodysp->unlinkFrBackWithNext();
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        എങ്കിൽ (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        എങ്കിൽ (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        എങ്കിൽ (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, സത്യം);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = അസത്യം;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = സത്യം;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        എങ്കിൽ (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    ശൂന്യം visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        സ്ഥിരം AstFunc* സ്ഥിരം funcp = VN_CAST(m_ftaskp, Func);
        എങ്കിൽ (m_randsequencep) {
            nodep->replaceWith(പുതിയ AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } അല്ലെങ്കിൽ എങ്കിൽ (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } അല്ലെങ്കിൽ എങ്കിൽ (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(പുതിയ AstAssign{
                    nodep->fileline(),
                    പുതിയ AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* സ്ഥിരം blockp = getJumpBlock(m_ftaskp, അസത്യം);
            nodep->addHereThisAsNext(പുതിയ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!m_loopp && m_randsequencep) {
            nodep->replaceWith(പുതിയ AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } അല്ലെങ്കിൽ {
            // Jump to the end of the loop
            AstJumpBlock* സ്ഥിരം blockp = getJumpBlock(m_loopp, അസത്യം);
            nodep->addNextHere(പുതിയ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } അല്ലെങ്കിൽ {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* സ്ഥിരം blockp = getJumpBlock(m_loopp, സത്യം);
            nodep->addNextHere(പുതിയ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_കണ്ണി* സ്ഥിരം targetp = nodep->targetp();
        എങ്കിൽ (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        എങ്കിൽ (AstTask* സ്ഥിരം taskp = VN_CAST(targetp, Task)) {
            AstVar* സ്ഥിരം processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം killStmtp = തിരുകുക_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            എങ്കിൽ (m_ftaskp == taskp) {
                Ast_കണ്ണി* jumpTargetp = taskp;
                സ്ഥിരം auto it = m_taskDisableBegins.find(taskp);
                എങ്കിൽ (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(targetp, Begin)) {
            എങ്കിൽ (existsBlockAbove(beginp->name())) {
                എങ്കിൽ (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* സ്ഥിരം blockp = getJumpBlock(beginp, അസത്യം);
                    nodep->addNextHere(പുതിയ AstJumpGo{nodep->fileline(), blockp});
                } അല്ലെങ്കിൽ {
                    AstVar* സ്ഥിരം processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം killStmtp = തിരുകുക_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* സ്ഥിരം branchp = innerForkBranchp(beginp);
                    AstBegin* സ്ഥിരം jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } അല്ലെങ്കിൽ {
                AstVar* സ്ഥിരം processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                തിരുകുക_Kill_Stmtp(nodep, processQueuep);
            }
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstFinish* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        എങ്കിൽ (m_inFork) {
            nodep->replaceWith(പുതിയ AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* സ്ഥിരം blockp = getJumpBlock(m_loopp, അസത്യം);
            nodep->addNextHere(പുതിയ AstJumpGo{nodep->fileline(), blockp});
        }
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(സത്യം);
    }
    ശൂന്യം visit(AstConst*) override {}
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Task class functions

ശൂന്യം V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("linkjump", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
