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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkJump.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Error.h"
#समावेश "V3UniqueNames.h"

#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी LinkJumpVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser2InUse m_user2InUse;
    स्थिर VNUser3InUse m_user3InUse;

    // STATE
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    bool m_loopInc = false;  // In loop increment
    bool m_inFork = false;  // Under fork
    पूर्णांक m_mod_Repeat_अंक = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_गाँठ_Block*> m_block_ढेर;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<स्थिर AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<स्थिर AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<स्थिर AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<स्थिर AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<स्थिर AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_गाँठ* nodep, bool endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        अगर (endOfIter) {
            अगर (nodep->user1p()) वापस VN_AS(nodep->user1p(), JumpBlock);
        } अन्यथा {
            अगर (nodep->user2p()) वापस VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_गाँठ* underp = nullptr;
        bool under_and_अग्र = true;
        अगर (AstBegin* स्थिर blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } अन्यथा अगर (Ast_गाँठ_FTask* स्थिर fTaskp = VN_CAST(nodep, गाँठ_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } अन्यथा अगर (AstForeach* स्थिर foreachp = VN_CAST(nodep, Foreach)) {
            अगर (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                अगर (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } अन्यथा {
                underp = nodep;
                under_and_अग्र = false;  // IE we skip the entire foreach
            }
        } अन्यथा अगर (AstLoop* स्थिर loopp = VN_CAST(nodep, Loop)) {
            अगर (endOfIter) {
                underp = loopp->stmtsp();
            } अन्यथा {
                underp = nodep;
                under_and_अग्र = false;  // IE we skip the entire loop
            }
        } अन्यथा {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            वापस nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        जबतक (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        अगर (!underp->nextp() || !under_and_अग्र) {
            अगर (AstJumpBlock* स्थिर blockp = VN_CAST(underp, JumpBlock)) वापस blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        अगर (under_and_अग्र) {
            underp->unlink_Fr_Back_With_अग्र(&repHandle);
        } अन्यथा {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* स्थिर blockp = नया AstJumpBlock{nodep->fileline(), underp};
        अगर (endOfIter) {
            nodep->user1p(blockp);
        } अन्यथा {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        क्रम (Ast_गाँठ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            अगर (VN_IS(varp, Var)) blockp->add_Here_This_As_अग्र(varp->unlinkFrBack());
        }
        वापस blockp;
    }
    व्योम addPrefixToBlocksRecurse(स्थिर std::string& prefix, Ast_गाँठ* स्थिर nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        अगर (AstBegin* स्थिर beginp = VN_CAST(nodep, Begin)) {
            अगर (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        अगर (Ast_गाँठ* स्थिर refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        अगर (Ast_गाँठ* स्थिर refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        अगर (Ast_गाँठ* स्थिर refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        अगर (Ast_गाँठ* स्थिर refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        अगर (Ast_गाँठ* स्थिर refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    bool existsBlockAbove(स्थिर std::string& name) स्थिर {
        क्रम (स्थिर Ast_गाँठ_Block* स्थिर stackp : vlstd::reverse_view(m_block_ढेर)) {
            अगर (stackp->name() == name) वापस true;
        }
        वापस false;
    }
    अटल Ast_गाँठ_Module* findOwnerModulep(Ast_गाँठ* nodep) {
        क्रम (Ast_गाँठ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            अगर (Ast_गाँठ_Module* स्थिर modp = VN_CAST(curp, गाँठ_Module)) वापस modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        वापस nullptr;  // LCOV_EXCL_LINE
    }
    अटल std::string targetInstancePath(स्थिर AstDisable* स्थिर nodep) {
        अगर (स्थिर AstVarXRef* स्थिर xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            वापस xrefp->dotted();
        }
        वापस "";
    }
    अटल std::string queueDotted(स्थिर AstDisable* स्थिर nodep,
                                   स्थिर AstVar* स्थिर processQueuep) {
        वापस processQueuep->is_अस्थायी() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(स्थिर Ast_गाँठ_Block* स्थिर targetp) स्थिर {
        AstBegin* innerForkBranchp = nullptr;
        Ast_गाँठ_Block* पूर्व_Blockp = nullptr;
        क्रम (Ast_गाँठ_Block* स्थिर blockp : vlstd::reverse_view(m_block_ढेर)) {
            अगर (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(पूर्व_Blockp, Begin);
            }
            अगर (blockp == targetp) वापस innerForkBranchp;
            पूर्व_Blockp = blockp;
        }
        वापस nullptr;
    }
    अटल Ast_Stmt_व्यंजक* get_Queue_धकेलो_Process_Selfp(AstVarRef* स्थिर queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_पंक्ति* स्थिर flp = queueRefp->fileline();
        वापस नया Ast_Stmt_व्यंजक{
            flp,
            नया AstMethodCall{flp, queueRefp, "push_back",
                              नया AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    अटल Ast_गाँठ_Var_Ref* newQueueRefp(File_पंक्ति* स्थिर fl, AstVar* स्थिर processQueuep,
                                       स्थिर VAccess& access, स्थिर std::string& dotted = "") {
        अगर (dotted != "") वापस नया AstVarXRef{fl, processQueuep, dotted, access};
        अगर (!processQueuep->lifetime().isStatic() || processQueuep->is_अस्थायी()) {
            वापस नया AstVarRef{fl, processQueuep, access};
        }
        AstPackage* स्थिर शीर्ष_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        वापस नया AstVarRef{fl, शीर्ष_Pkgp, processQueuep, access};
    }
    अटल Ast_Stmt_व्यंजक* get_Queue_धकेलो_Process_Selfp(File_पंक्ति* स्थिर fl, AstVar* स्थिर processQueuep) {
        AstVarRef* स्थिर queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        वापस get_Queue_धकेलो_Process_Selfp(queueWriteRefp);
    }
    अटल Ast_Stmt_व्यंजक* getQueueKillStmtp(File_पंक्ति* स्थिर fl, AstVar* स्थिर processQueuep,
                                          स्थिर std::string& dotted = "") {
        Ast_गाँठ_Var_Ref* स्थिर queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        क्रम (Ast_गाँठ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            अगर (itemp->name() == "killQueue") {
                killQueueCall
                    = नया AstTaskRef{fl, VN_AS(itemp, Task), नया AstArg{fl, "", queueRefp}};
                अवरोध;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        वापस नया Ast_Stmt_व्यंजक{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* स्थिर taskp, File_पंक्ति* स्थिर fl) {
        स्थिर स्वतः it = m_taskDisableBegins.find(taskp);
        अगर (it != m_taskDisableBegins.end()) वापस it->second;

        AstBegin* स्थिर taskBodyp = नया AstBegin{fl, "", nullptr, false};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        क्रम (Ast_गाँठ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_गाँठ* स्थिर nextp = stmtp->nextp();
            स्थिर AstVar* स्थिर varp = VN_CAST(stmtp, Var);
            अगर (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* स्थिर forkp = नया AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        वापस taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_गाँठ* स्थिर nodep, File_पंक्ति* स्थिर fl, स्थिर VVarType& varType) {
        AstVar* स्थिर processQueuep = नया AstVar{
            fl, varType, m_queueNames.लो(nodep->name()), VFlagChildDType{},
            नया AstQueueDType{
                fl, VFlagChildDType{},
                नया AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(true);
        processQueuep->setIgnoreSchedWrite();
        वापस processQueuep;
    }
    AstVar* getProcessQueuep(Ast_गाँठ* स्थिर nodep, File_पंक्ति* स्थिर fl) {
        Ast_गाँठ_Module* स्थिर ownerp = findOwnerModulep(nodep);

        अगर (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* स्थिर शीर्ष_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* स्थिर processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            शीर्ष_Pkgp->addStmtsp(processQueuep);
            वापस processQueuep;
        }

        AstVar* स्थिर processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        वापस processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* स्थिर taskp, File_पंक्ति* स्थिर fl) {
        स्थिर स्वतः it = m_taskDisableQueues.find(taskp);
        अगर (it != m_taskDisableQueues.end()) वापस it->second;

        AstVar* स्थिर processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_व्यंजक* स्थिर धकेलो_Current_Processp = get_Queue_धकेलो_Process_Selfp(fl, processQueuep);
        AstBegin* स्थिर taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(धकेलो_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        वापस processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* स्थिर beginp, File_पंक्ति* स्थिर fl) {
        स्थिर स्वतः it = m_beginDisableBegins.find(beginp);
        अगर (it != m_beginDisableBegins.end()) वापस it->second;

        AstBegin* स्थिर beginBodyp = नया AstBegin{fl, "", nullptr, false};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        अगर (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlink_Fr_Back_With_अग्र());

        AstFork* स्थिर forkp = नया AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        वापस beginBodyp;
    }
    व्योम prependForkBranchQueuePushes(AstFork* स्थिर forkp, AstVar* स्थिर processQueuep,
                                      File_पंक्ति* स्थिर fl, bool needProcess) {
        क्रम (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            अगर (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_धकेलो_Process_Selfp(fl, processQueuep));
        }
    }
    व्योम prependNestedForkBranchQueuePushes(Ast_गाँठ* स्थिर nodep, AstVar* स्थिर processQueuep,
                                            File_पंक्ति* स्थिर fl, bool needProcess) {
        nodep->foreach([&](AstFork* स्थिर forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* स्थिर beginp, File_पंक्ति* स्थिर fl) {
        स्थिर स्वतः it = m_beginDisableQueues.find(beginp);
        अगर (it != m_beginDisableQueues.end()) वापस it->second;

        AstVar* स्थिर processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_व्यंजक* स्थिर धकेलो_Current_Processp = get_Queue_धकेलो_Process_Selfp(fl, processQueuep);
        AstBegin* स्थिर beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(धकेलो_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, false);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        वापस processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* स्थिर forkp, File_पंक्ति* स्थिर fl) {
        स्थिर स्वतः it = m_forkDisableQueues.find(forkp);
        अगर (it != m_forkDisableQueues.end()) वापस it->second;

        AstVar* स्थिर processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, true);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        क्रम (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, true);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        वापस processQueuep;
    }
    Ast_Stmt_व्यंजक* जोड़ो_Kill_Stmtp(AstDisable* स्थिर nodep, AstVar* स्थिर processQueuep) {
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_Stmt_व्यंजक* स्थिर killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->add_अग्र_Here(killStmtp);
        वापस killStmtp;
    }
    व्योम addJumpAfterKill(Ast_Stmt_व्यंजक* स्थिर killStmtp, Ast_गाँठ* स्थिर targetp) {
        AstJumpBlock* स्थिर jmpBlockp = getJumpBlock(targetp, false);
        killStmtp->add_अग्र_Here(नया AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    व्योम handleDisableOnFork(AstDisable* स्थिर nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_पंक्ति* स्थिर fl = nodep->fileline();
        AstFork* स्थिर targetp = VN_AS(nodep->targetp(), Fork);
        अगर (m_ftaskp) {
            अगर (!m_ftaskp->exists(
                    [targetp](स्थिर Ast_गाँठ_Block* blockp) -> bool { वापस blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* स्थिर processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_व्यंजक* स्थिर killStmtp = जोड़ो_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        अगर (AstBegin* स्थिर branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->dead()) वापस;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_अंक);
        m_modp = nodep;
        m_mod_Repeat_अंक = 0;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_ढेर.धकेलो_back(nodep);
        iterateChildren(nodep);
        m_block_ढेर.निकालो_back();
    }
    व्योम visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = true;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        क्रम (Ast_गाँठ_Block* स्थिर blockp : vlstd::reverse_view(m_block_ढेर)) {
            अगर (blockp->user3SetOnce()) अवरोध;
        }
        m_block_ढेर.धकेलो_back(nodep);
        iterateChildren(nodep);
        m_block_ढेर.निकालो_back();
    }
    व्योम visit(AstStmtPragma* nodep) override {
        अगर (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा अगर (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_गाँठ_व्यंजक* स्थिर countp = nodep->countp()->unlink_Fr_Back_With_अग्र();
        स्थिर string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_अंक++);
        AstBegin* स्थिर beginp = नया AstBegin{nodep->fileline(), "", nullptr, true};
        // Spec says value is integral, if negative is ignored
        AstVar* स्थिर varp
            = नया AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(true);
        beginp->addStmtsp(varp);
        Ast_गाँठ* initsp = नया AstAssign{
            nodep->fileline(), नया AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_गाँठ* स्थिर decp = नया AstAssign{
            nodep->fileline(), नया AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            नया AstSub{nodep->fileline(), नया AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       नया AstConst{nodep->fileline(), 1}}};
        Ast_गाँठ_व्यंजक* स्थिर zerosp = नया AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_गाँठ_व्यंजक* स्थिर condp = नया AstGtS{
            nodep->fileline(), नया AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_गाँठ* स्थिर bodysp = nodep->stmtsp();
        अगर (bodysp) bodysp->unlink_Fr_Back_With_अग्र();
        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        loopp->addStmtsp(नया AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        अगर (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstLoop* nodep) override {
        अगर (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        अगर (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, true);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = false;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_loopInc = true;
        iterate_And_अग्र_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        अगर (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlink_Fr_Back_With_अग्र());
    }
    व्योम visit(Ast_गाँठ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_अग्र_Null(nodep->bodyp());
    }
    व्योम visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        स्थिर AstFunc* स्थिर funcp = VN_CAST(m_ftaskp, Func);
        अगर (m_randsequencep) {
            nodep->replaceWith(नया AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        } अन्यथा अगर (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        } अन्यथा अगर (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } अन्यथा अगर (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } अन्यथा अगर (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } अन्यथा {
            अगर (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->add_Here_This_As_अग्र(नया AstAssign{
                    nodep->fileline(),
                    नया AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_Back_With_अग्र()});
            }
            // Jump to the end of the function call
            AstJumpBlock* स्थिर blockp = getJumpBlock(m_ftaskp, false);
            nodep->add_Here_This_As_अग्र(नया AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        अगर (!m_loopp && m_randsequencep) {
            nodep->replaceWith(नया AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        } अन्यथा अगर (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } अन्यथा {
            // Jump to the end of the loop
            AstJumpBlock* स्थिर blockp = getJumpBlock(m_loopp, false);
            nodep->add_अग्र_Here(नया AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        अगर (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } अन्यथा {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* स्थिर blockp = getJumpBlock(m_loopp, true);
            nodep->add_अग्र_Here(नया AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_गाँठ* स्थिर targetp = nodep->targetp();
        अगर (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        अगर (AstTask* स्थिर taskp = VN_CAST(targetp, Task)) {
            AstVar* स्थिर processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_व्यंजक* स्थिर killStmtp = जोड़ो_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            अगर (m_ftaskp == taskp) {
                Ast_गाँठ* jumpTargetp = taskp;
                स्थिर स्वतः it = m_taskDisableBegins.find(taskp);
                अगर (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } अन्यथा अगर (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } अन्यथा अगर (AstBegin* स्थिर beginp = VN_CAST(targetp, Begin)) {
            अगर (existsBlockAbove(beginp->name())) {
                अगर (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* स्थिर blockp = getJumpBlock(beginp, false);
                    nodep->add_अग्र_Here(नया AstJumpGo{nodep->fileline(), blockp});
                } अन्यथा {
                    AstVar* स्थिर processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_व्यंजक* स्थिर killStmtp = जोड़ो_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* स्थिर branchp = innerForkBranchp(beginp);
                    AstBegin* स्थिर jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } अन्यथा {
                AstVar* स्थिर processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                जोड़ो_Kill_Stmtp(nodep, processQueuep);
            }
        } अन्यथा {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstFinish* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        iterateChildren(nodep);
        अगर (m_inFork) {
            nodep->replaceWith(नया AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* स्थिर blockp = getJumpBlock(m_loopp, false);
            nodep->add_अग्र_Here(नया AstJumpGo{nodep->fileline(), blockp});
        }
    }
    व्योम visit(AstVarRef* nodep) override {
        अगर (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(true);
    }
    व्योम visit(AstConst*) override {}
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = शेष;
};

//######################################################################
// Task class functions

व्योम V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("linkjump", 0, dump_वृक्ष_Either_Level() >= 3);
}
