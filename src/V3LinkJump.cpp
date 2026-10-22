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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkJump.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग LinkJumpVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    स्थिरांक VNUser1InUse m_user1InUse;
    स्थिरांक VNUser2InUse m_user2InUse;
    स्थिरांक VNUser3InUse m_user3InUse;

    // STATE
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाठ* m_loopp = nullptr;  // Current loop
    AstRandSequence* m_randsequencep = nullptr;  // Current randsequence
    तार्किक m_loopInc = असत्य;  // In loop increment
    तार्किक m_inFork = असत्य;  // Under fork
    पूर्णांक m_mod_Repeat_अंक = 0;  // Repeat counter
    VOptionBool m_unrollFull;  // Pragma full, disable, or default unrolling
    std::vector<Ast_गाठ_Block*> m_block_चळत;  // All begin blocks above current node
    V3UniqueNames m_queueNames{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<स्थिरांक AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<स्थिरांक AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<स्थिरांक AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<स्थिरांक AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<स्थिरांक AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    AstJumpBlock* getJumpBlock(Ast_गाठ* nodep, तार्किक endOfIter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        जर (endOfIter) {
            जर (nodep->user1p()) परत VN_AS(nodep->user1p(), JumpBlock);
        } अन्यथा {
            जर (nodep->user2p()) परत VN_AS(nodep->user2p(), JumpBlock);
        }

        Ast_गाठ* underp = nullptr;
        तार्किक under_and_next = सत्य;
        जर (AstBegin* स्थिरांक blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } अन्यथा जर (Ast_गाठ_FTask* स्थिरांक fTaskp = VN_CAST(nodep, गाठ_FTask)) {
            UASSERT_OBJ(!endOfIter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } अन्यथा जर (AstForeach* स्थिरांक foreachp = VN_CAST(nodep, Foreach)) {
            जर (endOfIter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                जर (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } अन्यथा {
                underp = nodep;
                under_and_next = असत्य;  // IE we skip the entire foreach
            }
        } अन्यथा जर (AstLoop* स्थिरांक loopp = VN_CAST(nodep, Loop)) {
            जर (endOfIter) {
                underp = loopp->stmtsp();
            } अन्यथा {
                underp = nodep;
                under_and_next = असत्य;  // IE we skip the entire loop
            }
        } अन्यथा {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            परत nullptr;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        पर्यंत (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        जर (!underp->nextp() || !under_and_next) {
            जर (AstJumpBlock* स्थिरांक blockp = VN_CAST(underp, JumpBlock)) परत blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        जर (under_and_next) {
            underp->unlinkFrBackWithNext(&repHandle);
        } अन्यथा {
            underp->unlinkFrBack(&repHandle);
        }
        AstJumpBlock* स्थिरांक blockp = नवीन AstJumpBlock{nodep->fileline(), underp};
        जर (endOfIter) {
            nodep->user1p(blockp);
        } अन्यथा {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        साठी (Ast_गाठ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            जर (VN_IS(varp, Var)) blockp->addHereThisAsNext(varp->unlinkFrBack());
        }
        परत blockp;
    }
    शून्य addPrefixToBlocksRecurse(स्थिरांक std::string& prefix, Ast_गाठ* स्थिरांक nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        जर (AstBegin* स्थिरांक beginp = VN_CAST(nodep, Begin)) {
            जर (beginp->name() != "") beginp->name(prefix + beginp->name());
        }

        जर (Ast_गाठ* स्थिरांक refp = nodep->op1p()) addPrefixToBlocksRecurse(prefix, refp);
        जर (Ast_गाठ* स्थिरांक refp = nodep->op2p()) addPrefixToBlocksRecurse(prefix, refp);
        जर (Ast_गाठ* स्थिरांक refp = nodep->op3p()) addPrefixToBlocksRecurse(prefix, refp);
        जर (Ast_गाठ* स्थिरांक refp = nodep->op4p()) addPrefixToBlocksRecurse(prefix, refp);
        जर (Ast_गाठ* स्थिरांक refp = nodep->nextp()) addPrefixToBlocksRecurse(prefix, refp);
    }
    तार्किक existsBlockAbove(स्थिरांक std::string& name) स्थिरांक {
        साठी (स्थिरांक Ast_गाठ_Block* स्थिरांक stackp : vlstd::reverse_view(m_block_चळत)) {
            जर (stackp->name() == name) परत सत्य;
        }
        परत असत्य;
    }
    स्थिर Ast_गाठ_Module* findOwnerModulep(Ast_गाठ* nodep) {
        साठी (Ast_गाठ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            जर (Ast_गाठ_Module* स्थिरांक modp = VN_CAST(curp, गाठ_Module)) परत modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        परत nullptr;  // LCOV_EXCL_LINE
    }
    स्थिर std::string targetInstancePath(स्थिरांक AstDisable* स्थिरांक nodep) {
        जर (स्थिरांक AstVarXRef* स्थिरांक xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            परत xrefp->dotted();
        }
        परत "";
    }
    स्थिर std::string queueDotted(स्थिरांक AstDisable* स्थिरांक nodep,
                                   स्थिरांक AstVar* स्थिरांक processQueuep) {
        परत processQueuep->isTemp() ? targetInstancePath(nodep) : "";
    }
    AstBegin* innerForkBranchp(स्थिरांक Ast_गाठ_Block* स्थिरांक targetp) स्थिरांक {
        AstBegin* innerForkBranchp = nullptr;
        Ast_गाठ_Block* prevBlockp = nullptr;
        साठी (Ast_गाठ_Block* स्थिरांक blockp : vlstd::reverse_view(m_block_चळत)) {
            जर (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            जर (blockp == targetp) परत innerForkBranchp;
            prevBlockp = blockp;
        }
        परत nullptr;
    }
    स्थिर Ast_Stmt_अभिव्यक्ती* get_Queue_ढकला_Process_Selfp(AstVarRef* स्थिरांक queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_ओळ* स्थिरांक flp = queueRefp->fileline();
        परत नवीन Ast_Stmt_अभिव्यक्ती{
            flp,
            नवीन AstMethodCall{flp, queueRefp, "push_back",
                              नवीन AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    स्थिर Ast_गाठ_Var_Ref* newQueueRefp(File_ओळ* स्थिरांक fl, AstVar* स्थिरांक processQueuep,
                                       स्थिरांक VAccess& access, स्थिरांक std::string& dotted = "") {
        जर (dotted != "") परत नवीन AstVarXRef{fl, processQueuep, dotted, access};
        जर (!processQueuep->lifetime().isStatic() || processQueuep->isTemp()) {
            परत नवीन AstVarRef{fl, processQueuep, access};
        }
        AstPackage* स्थिरांक शीर्ष_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        परत नवीन AstVarRef{fl, शीर्ष_Pkgp, processQueuep, access};
    }
    स्थिर Ast_Stmt_अभिव्यक्ती* get_Queue_ढकला_Process_Selfp(File_ओळ* स्थिरांक fl, AstVar* स्थिरांक processQueuep) {
        AstVarRef* स्थिरांक queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        परत get_Queue_ढकला_Process_Selfp(queueWriteRefp);
    }
    स्थिर Ast_Stmt_अभिव्यक्ती* getQueueKillStmtp(File_ओळ* स्थिरांक fl, AstVar* स्थिरांक processQueuep,
                                          स्थिरांक std::string& dotted = "") {
        Ast_गाठ_Var_Ref* स्थिरांक queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = nullptr;
        साठी (Ast_गाठ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            जर (itemp->name() == "killQueue") {
                killQueueCall
                    = नवीन AstTaskRef{fl, VN_AS(itemp, Task), नवीन AstArg{fl, "", queueRefp}};
                थांबा;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        परत नवीन Ast_Stmt_अभिव्यक्ती{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* स्थिरांक taskp, File_ओळ* स्थिरांक fl) {
        स्थिरांक स्वतः it = m_taskDisableBegins.find(taskp);
        जर (it != m_taskDisableBegins.end()) परत it->second;

        AstBegin* स्थिरांक taskBodyp = नवीन AstBegin{fl, "", nullptr, असत्य};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        // Ports stay in the task so calls see them
        साठी (Ast_गाठ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_गाठ* स्थिरांक nextp = stmtp->nextp();
            स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtp, Var);
            जर (!varp || !varp->isIO()) taskBodyp->addStmtsp(stmtp->unlinkFrBack());
            stmtp = nextp;
        }

        AstFork* स्थिरांक forkp = नवीन AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(taskBodyp);
        taskp->addStmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        परत taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_गाठ* स्थिरांक nodep, File_ओळ* स्थिरांक fl, स्थिरांक VVarType& varType) {
        AstVar* स्थिरांक processQueuep = नवीन AstVar{
            fl, varType, m_queueNames.get(nodep->name()), VFlagChildDType{},
            नवीन AstQueueDType{
                fl, VFlagChildDType{},
                नवीन AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), nullptr},
                nullptr}};
        processQueuep->processQueue(सत्य);
        processQueuep->setIgnoreSchedWrite();
        परत processQueuep;
    }
    AstVar* getProcessQueuep(Ast_गाठ* स्थिरांक nodep, File_ओळ* स्थिरांक fl) {
        Ast_गाठ_Module* स्थिरांक ownerp = findOwnerModulep(nodep);

        जर (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* स्थिरांक शीर्ष_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* स्थिरांक processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            शीर्ष_Pkgp->addStmtsp(processQueuep);
            परत processQueuep;
        }

        AstVar* स्थिरांक processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->addStmtsp(processQueuep);
        परत processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* स्थिरांक taskp, File_ओळ* स्थिरांक fl) {
        स्थिरांक स्वतः it = m_taskDisableQueues.find(taskp);
        जर (it != m_taskDisableQueues.end()) परत it->second;

        AstVar* स्थिरांक processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_अभिव्यक्ती* स्थिरांक ढकला_Current_Processp = get_Queue_ढकला_Process_Selfp(fl, processQueuep);
        AstBegin* स्थिरांक taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ढकला_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        परत processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* स्थिरांक beginp, File_ओळ* स्थिरांक fl) {
        स्थिरांक स्वतः it = m_beginDisableBegins.find(beginp);
        जर (it != m_beginDisableBegins.end()) परत it->second;

        AstBegin* स्थिरांक beginBodyp = नवीन AstBegin{fl, "", nullptr, असत्य};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->setNeedProcess();
        v3Global.setUsesTiming();
        जर (beginp->stmtsp()) beginBodyp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());

        AstFork* स्थिरांक forkp = नवीन AstFork{fl, VJoinType::JOIN};
        forkp->addForksp(beginBodyp);
        beginp->addStmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        परत beginBodyp;
    }
    शून्य prependForkBranchQueuePushes(AstFork* स्थिरांक forkp, AstVar* स्थिरांक processQueuep,
                                      File_ओळ* स्थिरांक fl, तार्किक needProcess) {
        साठी (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            जर (needProcess) branchp->setNeedProcess();
            branchp->prependStmtsp(get_Queue_ढकला_Process_Selfp(fl, processQueuep));
        }
    }
    शून्य prependNestedForkBranchQueuePushes(Ast_गाठ* स्थिरांक nodep, AstVar* स्थिरांक processQueuep,
                                            File_ओळ* स्थिरांक fl, तार्किक needProcess) {
        nodep->foreach([&](AstFork* स्थिरांक forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* स्थिरांक beginp, File_ओळ* स्थिरांक fl) {
        स्थिरांक स्वतः it = m_beginDisableQueues.find(beginp);
        जर (it != m_beginDisableQueues.end()) परत it->second;

        AstVar* स्थिरांक processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_अभिव्यक्ती* स्थिरांक ढकला_Current_Processp = get_Queue_ढकला_Process_Selfp(fl, processQueuep);
        AstBegin* स्थिरांक beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ढकला_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, असत्य);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        परत processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* स्थिरांक forkp, File_ओळ* स्थिरांक fl) {
        स्थिरांक स्वतः it = m_forkDisableQueues.find(forkp);
        जर (it != m_forkDisableQueues.end()) परत it->second;

        AstVar* स्थिरांक processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, सत्य);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        साठी (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, सत्य);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        परत processQueuep;
    }
    Ast_Stmt_अभिव्यक्ती* घाला_Kill_Stmtp(AstDisable* स्थिरांक nodep, AstVar* स्थिरांक processQueuep) {
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_Stmt_अभिव्यक्ती* स्थिरांक killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->addNextHere(killStmtp);
        परत killStmtp;
    }
    शून्य addJumpAfterKill(Ast_Stmt_अभिव्यक्ती* स्थिरांक killStmtp, Ast_गाठ* स्थिरांक targetp) {
        AstJumpBlock* स्थिरांक jmpBlockp = getJumpBlock(targetp, असत्य);
        killStmtp->addNextHere(नवीन AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    शून्य handleDisableOnFork(AstDisable* स्थिरांक nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_ओळ* स्थिरांक fl = nodep->fileline();
        AstFork* स्थिरांक targetp = VN_AS(nodep->targetp(), Fork);
        जर (m_ftaskp) {
            जर (!m_ftaskp->exists(
                    [targetp](स्थिरांक Ast_गाठ_Block* blockp) -> तार्किक { परत blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* स्थिरांक processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_अभिव्यक्ती* स्थिरांक killStmtp = घाला_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        जर (AstBegin* स्थिरांक branchp = innerForkBranchp(targetp)) {
            addJumpAfterKill(killStmtp, branchp);
        }
    }
    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        जर (nodep->dead()) परत;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_अंक);
        m_modp = nodep;
        m_mod_Repeat_अंक = 0;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        m_block_चळत.ढकला_back(nodep);
        iterateChildren(nodep);
        m_block_चळत.काढा_back();
    }
    शून्य visit(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unrollFull);
        VL_RESTORER(m_inFork);
        m_inFork = सत्य;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        साठी (Ast_गाठ_Block* स्थिरांक blockp : vlstd::reverse_view(m_block_चळत)) {
            जर (blockp->user3SetOnce()) थांबा;
        }
        m_block_चळत.ढकला_back(nodep);
        iterateChildren(nodep);
        m_block_चळत.काढा_back();
    }
    शून्य visit(AstStmtPragma* nodep) override {
        जर (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unrollFull = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा जर (nodep->pragp()->pragType() == VPragmaType::UNROLL_FULL) {
            m_unrollFull = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_गाठ_अभिव्यक्ती* स्थिरांक countp = nodep->countp()->unlinkFrBackWithNext();
        स्थिरांक string name = "__Vrepeat"s + cvtToStr(m_mod_Repeat_अंक++);
        AstBegin* स्थिरांक beginp = नवीन AstBegin{nodep->fileline(), "", nullptr, सत्य};
        // Spec says value is integral, if negative is ignored
        AstVar* स्थिरांक varp
            = नवीन AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->usedLoopIdx(सत्य);
        beginp->addStmtsp(varp);
        Ast_गाठ* initsp = नवीन AstAssign{
            nodep->fileline(), नवीन AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_गाठ* स्थिरांक decp = नवीन AstAssign{
            nodep->fileline(), नवीन AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            नवीन AstSub{nodep->fileline(), नवीन AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       नवीन AstConst{nodep->fileline(), 1}}};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक zerosp = नवीन AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = नवीन AstGtS{
            nodep->fileline(), नवीन AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_गाठ* स्थिरांक bodysp = nodep->stmtsp();
        जर (bodysp) bodysp->unlinkFrBackWithNext();
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
        loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, condp});
        loopp->addStmtsp(bodysp);
        loopp->addContsp(decp);
        जर (!m_unrollFull.isDefault()) loopp->unroll(m_unrollFull);
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->addStmtsp(initsp);
        beginp->addStmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstLoop* nodep) override {
        जर (!m_unrollFull.isDefault()) nodep->unroll(m_unrollFull);
        जर (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, सत्य);
        }
        m_unrollFull = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = असत्य;
        iterateAndNextNull(nodep->stmtsp());
        m_loopInc = सत्य;
        iterateAndNextNull(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        जर (nodep->contsp()) nodep->addStmtsp(nodep->contsp()->unlinkFrBackWithNext());
    }
    शून्य visit(Ast_गाठ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterateAndNextNull(nodep->bodyp());
    }
    शून्य visit(AstReturn* nodep) override {
        iterateChildren(nodep);
        स्थिरांक AstFunc* स्थिरांक funcp = VN_CAST(m_ftaskp, Func);
        जर (m_randsequencep) {
            nodep->replaceWith(नवीन AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        } अन्यथा जर (m_inFork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        } अन्यथा जर (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } अन्यथा जर (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } अन्यथा जर (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } अन्यथा {
            जर (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->addHereThisAsNext(नवीन AstAssign{
                    nodep->fileline(),
                    नवीन AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlinkFrBackWithNext()});
            }
            // Jump to the end of the function call
            AstJumpBlock* स्थिरांक blockp = getJumpBlock(m_ftaskp, असत्य);
            nodep->addHereThisAsNext(नवीन AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstBreak* nodep) override {
        iterateChildren(nodep);
        जर (!m_loopp && m_randsequencep) {
            nodep->replaceWith(नवीन AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        } अन्यथा जर (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } अन्यथा {
            // Jump to the end of the loop
            AstJumpBlock* स्थिरांक blockp = getJumpBlock(m_loopp, असत्य);
            nodep->addNextHere(नवीन AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstContinue* nodep) override {
        iterateChildren(nodep);
        जर (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } अन्यथा {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            AstJumpBlock* स्थिरांक blockp = getJumpBlock(m_loopp, सत्य);
            nodep->addNextHere(नवीन AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_गाठ* स्थिरांक targetp = nodep->targetp();
        जर (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        जर (AstTask* स्थिरांक taskp = VN_CAST(targetp, Task)) {
            AstVar* स्थिरांक processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_अभिव्यक्ती* स्थिरांक killStmtp = घाला_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            जर (m_ftaskp == taskp) {
                Ast_गाठ* jumpTargetp = taskp;
                स्थिरांक स्वतः it = m_taskDisableBegins.find(taskp);
                जर (it != m_taskDisableBegins.end()) jumpTargetp = it->second;
                addJumpAfterKill(killStmtp, jumpTargetp);
            }
        } अन्यथा जर (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } अन्यथा जर (AstBegin* स्थिरांक beginp = VN_CAST(targetp, Begin)) {
            जर (existsBlockAbove(beginp->name())) {
                जर (!beginp->user3()) {
                    // Jump to the end of the named block
                    AstJumpBlock* स्थिरांक blockp = getJumpBlock(beginp, असत्य);
                    nodep->addNextHere(नवीन AstJumpGo{nodep->fileline(), blockp});
                } अन्यथा {
                    AstVar* स्थिरांक processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_अभिव्यक्ती* स्थिरांक killStmtp = घाला_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* स्थिरांक branchp = innerForkBranchp(beginp);
                    AstBegin* स्थिरांक jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    addJumpAfterKill(killStmtp, jumpTargetp);
                }
            } अन्यथा {
                AstVar* स्थिरांक processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                घाला_Kill_Stmtp(nodep, processQueuep);
            }
        } अन्यथा {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstFinish* nodep) override {
        जर (nodep->user1SetOnce()) परत;  // Process once
        iterateChildren(nodep);
        जर (m_inFork) {
            nodep->replaceWith(नवीन AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (m_loopp) {
            // Jump to the end of the loop (post-finish)
            AstJumpBlock* स्थिरांक blockp = getJumpBlock(m_loopp, असत्य);
            nodep->addNextHere(नवीन AstJumpGo{nodep->fileline(), blockp});
        }
    }
    शून्य visit(AstVarRef* nodep) override {
        जर (m_loopInc && nodep->varp()) nodep->varp()->usedLoopIdx(सत्य);
    }
    शून्य visit(AstConst*) override {}
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Task class functions

शून्य V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("linkjump", 0, dump_वृक्ष_Either_Level() >= 3);
}
