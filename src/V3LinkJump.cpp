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
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;
    ध्रुव VNUser2_अन्तर्_Use m_user2_अन्तर्_Use;
    ध्रुव VNUser3_अन्तर्_Use m_user3_अन्तर्_Use;

    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    Ast_ग्रन्थिः* m_loopp = शून्यसूचकः;  // Current loop
    AstRandSequence* m_randsequencep = शून्यसूचकः;  // Current randsequence
    तार्किक m_loopInc = असत्य;  // In loop increment
    तार्किक m_अन्तर्_Fork = असत्य;  // Under fork
    पूर्णाङ्क m_mod_Repeat_अङ्कः = 0;  // Repeat counter
    VOptionBool m_unroll_पूर्णम्;  // Pragma full, disable, or default unrolling
    std::सदिशम्<Ast_ग्रन्थिः_खण्डम्*> m_खण्डम्_राशिः;  // All begin blocks above current node
    V3_Unique_नामानि m_queue_नामानि{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ध्रुव AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ध्रुव AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ध्रुव AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ध्रुव AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ध्रुव AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    Ast_Jump_खण्डम्* get_Jump_खण्डम्(Ast_ग्रन्थिः* nodep, तार्किक अन्तः_सम्बन्धि_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        यदि (अन्तः_सम्बन्धि_Iter) {
            यदि (nodep->user1p()) प्रत्यागम VN_AS(nodep->user1p(), Jump_खण्डम्);
        } अन्यथा {
            यदि (nodep->user2p()) प्रत्यागम VN_AS(nodep->user2p(), Jump_खण्डम्);
        }

        Ast_ग्रन्थिः* underp = शून्यसूचकः;
        तार्किक under_and_अग्र = सत्य;
        यदि (AstBegin* ध्रुव blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!अन्तः_सम्बन्धि_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } अन्यथा यदि (Ast_ग्रन्थिः_FTask* ध्रुव fTaskp = VN_CAST(nodep, ग्रन्थिः_FTask)) {
            UASSERT_OBJ(!अन्तः_सम्बन्धि_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } अन्यथा यदि (AstForeach* ध्रुव foreachp = VN_CAST(nodep, Foreach)) {
            यदि (अन्तः_सम्बन्धि_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                यदि (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } अन्यथा {
                underp = nodep;
                under_and_अग्र = असत्य;  // IE we skip the entire foreach
            }
        } अन्यथा यदि (AstLoop* ध्रुव loopp = VN_CAST(nodep, Loop)) {
            यदि (अन्तः_सम्बन्धि_Iter) {
                underp = loopp->stmtsp();
            } अन्यथा {
                underp = nodep;
                under_and_अग्र = असत्य;  // IE we skip the entire loop
            }
        } अन्यथा {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            प्रत्यागम शून्यसूचकः;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        यावत् (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        यदि (!underp->nextp() || !under_and_अग्र) {
            यदि (Ast_Jump_खण्डम्* ध्रुव blockp = VN_CAST(underp, Jump_खण्डम्)) प्रत्यागम blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        यदि (under_and_अग्र) {
            underp->unlink_Fr_पृष्ठभागः_With_अग्र(&repHandle);
        } अन्यथा {
            underp->unlink_Fr_पृष्ठभागः(&repHandle);
        }
        Ast_Jump_खण्डम्* ध्रुव blockp = नूतन Ast_Jump_खण्डम्{nodep->fileline(), underp};
        यदि (अन्तः_सम्बन्धि_Iter) {
            nodep->user1p(blockp);
        } अन्यथा {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        प्रति (Ast_ग्रन्थिः *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            यदि (VN_IS(varp, Var)) blockp->सङ्कलय_अत्र_This_As_अग्र(varp->unlink_Fr_पृष्ठभागः());
        }
        प्रत्यागम blockp;
    }
    शून्य सङ्कलय_उपसर्गः_गन्तव्यम्_खण्डानि_Recurse(ध्रुव std::string& उपसर्गः, Ast_ग्रन्थिः* ध्रुव nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        यदि (AstBegin* ध्रुव beginp = VN_CAST(nodep, Begin)) {
            यदि (beginp->name() != "") beginp->name(उपसर्गः + beginp->name());
        }

        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) सङ्कलय_उपसर्गः_गन्तव्यम्_खण्डानि_Recurse(उपसर्गः, refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) सङ्कलय_उपसर्गः_गन्तव्यम्_खण्डानि_Recurse(उपसर्गः, refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) सङ्कलय_उपसर्गः_गन्तव्यम्_खण्डानि_Recurse(उपसर्गः, refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) सङ्कलय_उपसर्गः_गन्तव्यम्_खण्डानि_Recurse(उपसर्गः, refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->nextp()) सङ्कलय_उपसर्गः_गन्तव्यम्_खण्डानि_Recurse(उपसर्गः, refp);
    }
    तार्किक exists_खण्डम्_Above(ध्रुव std::string& name) ध्रुव {
        प्रति (ध्रुव Ast_ग्रन्थिः_खण्डम्* ध्रुव stackp : vlstd::reverse_view(m_खण्डम्_राशिः)) {
            यदि (stackp->name() == name) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    स्थिर Ast_ग्रन्थिः_Module* findOwnerModulep(Ast_ग्रन्थिः* nodep) {
        प्रति (Ast_ग्रन्थिः* curp = nodep; curp; curp = curp->aboveLoopp()) {
            यदि (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(curp, ग्रन्थिः_Module)) प्रत्यागम modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        प्रत्यागम शून्यसूचकः;  // LCOV_EXCL_LINE
    }
    स्थिर std::string target_Instance_मार्गः(ध्रुव AstDisable* ध्रुव nodep) {
        यदि (ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(nodep->targetRefp(), VarXRef)) {
            प्रत्यागम xrefp->dotted();
        }
        प्रत्यागम "";
    }
    स्थिर std::string queueDotted(ध्रुव AstDisable* ध्रुव nodep,
                                   ध्रुव AstVar* ध्रुव processQueuep) {
        प्रत्यागम processQueuep->is_अस्थायी() ? target_Instance_मार्गः(nodep) : "";
    }
    AstBegin* innerForkBranchp(ध्रुव Ast_ग्रन्थिः_खण्डम्* ध्रुव targetp) ध्रुव {
        AstBegin* innerForkBranchp = शून्यसूचकः;
        Ast_ग्रन्थिः_खण्डम्* पूर्व_Blockp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः_खण्डम्* ध्रुव blockp : vlstd::reverse_view(m_खण्डम्_राशिः)) {
            यदि (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(पूर्व_Blockp, Begin);
            }
            यदि (blockp == targetp) प्रत्यागम innerForkBranchp;
            पूर्व_Blockp = blockp;
        }
        प्रत्यागम शून्यसूचकः;
    }
    स्थिर Ast_Stmt_व्यञ्जकम्* get_Queue_प्रक्षिप_Process_Selfp(AstVarRef* ध्रुव queueRefp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_पङ्क्तिः* ध्रुव flp = queueRefp->fileline();
        प्रत्यागम नूतन Ast_Stmt_व्यञ्जकम्{
            flp,
            नूतन Ast_विधानम्_Call{flp, queueRefp, "push_back",
                              नूतन AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    स्थिर Ast_ग्रन्थिः_Var_Ref* newQueueRefp(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव processQueuep,
                                       ध्रुव VAccess& access, ध्रुव std::string& dotted = "") {
        यदि (dotted != "") प्रत्यागम नूतन AstVarXRef{fl, processQueuep, dotted, access};
        यदि (!processQueuep->lifetime().isStatic() || processQueuep->is_अस्थायी()) {
            प्रत्यागम नूतन AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ध्रुव शिखरम्_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        प्रत्यागम नूतन AstVarRef{fl, शिखरम्_Pkgp, processQueuep, access};
    }
    स्थिर Ast_Stmt_व्यञ्जकम्* get_Queue_प्रक्षिप_Process_Selfp(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव processQueuep) {
        AstVarRef* ध्रुव queueWriteRefp
            = VN_AS(newQueueRefp(fl, processQueuep, VAccess::WRITE), VarRef);
        प्रत्यागम get_Queue_प्रक्षिप_Process_Selfp(queueWriteRefp);
    }
    स्थिर Ast_Stmt_व्यञ्जकम्* getQueueKillStmtp(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव processQueuep,
                                          ध्रुव std::string& dotted = "") {
        Ast_ग्रन्थिः_Var_Ref* ध्रुव queueRefp
            = newQueueRefp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* killQueueCall = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            यदि (itemp->name() == "killQueue") {
                killQueueCall
                    = नूतन AstTaskRef{fl, VN_AS(itemp, Task), नूतन AstArg{fl, "", queueRefp}};
                भङ्ग;
            }
        }
        UASSERT(killQueueCall, "Should be found");
        killQueueCall->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        प्रत्यागम नूतन Ast_Stmt_व्यञ्जकम्{fl, killQueueCall};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ध्रुव taskp, File_पङ्क्तिः* ध्रुव fl) {
        ध्रुव स्वचालित it = m_taskDisableBegins.find(taskp);
        यदि (it != m_taskDisableBegins.अन्तः_2()) प्रत्यागम it->second;

        AstBegin* ध्रुव taskBodyp = नूतन AstBegin{fl, "", शून्यसूचकः, असत्य};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->समुच्चयः_Need_Process();
        v3Global.समुच्चयः_उपयुङ्क्ते_Timing();
        // Ports stay in the task so calls see them
        प्रति (Ast_ग्रन्थिः* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
            ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            यदि (!varp || !varp->isIO()) taskBodyp->सङ्कलय_Stmtsp(stmtp->unlink_Fr_पृष्ठभागः());
            stmtp = nextp;
        }

        AstFork* ध्रुव forkp = नूतन AstFork{fl, VJoinType::JOIN};
        forkp->सङ्कलय_Forksp(taskBodyp);
        taskp->सङ्कलय_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        प्रत्यागम taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ग्रन्थिः* ध्रुव nodep, File_पङ्क्तिः* ध्रुव fl, ध्रुव VVarType& varType) {
        AstVar* ध्रुव processQueuep = नूतन AstVar{
            fl, varType, m_queue_नामानि.get(nodep->name()), VFlagChildDType{},
            नूतन AstQueueDType{
                fl, VFlagChildDType{},
                नूतन AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), शून्यसूचकः},
                शून्यसूचकः}};
        processQueuep->processQueue(सत्य);
        processQueuep->समुच्चयः_Ignore_Sched_Write();
        प्रत्यागम processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ग्रन्थिः* ध्रुव nodep, File_पङ्क्तिः* ध्रुव fl) {
        Ast_ग्रन्थिः_Module* ध्रुव ownerp = findOwnerModulep(nodep);

        यदि (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ध्रुव शिखरम्_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ध्रुव processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            शिखरम्_Pkgp->सङ्कलय_Stmtsp(processQueuep);
            प्रत्यागम processQueuep;
        }

        AstVar* ध्रुव processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->सङ्कलय_Stmtsp(processQueuep);
        प्रत्यागम processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ध्रुव taskp, File_पङ्क्तिः* ध्रुव fl) {
        ध्रुव स्वचालित it = m_taskDisableQueues.find(taskp);
        यदि (it != m_taskDisableQueues.अन्तः_2()) प्रत्यागम it->second;

        AstVar* ध्रुव processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_व्यञ्जकम्* ध्रुव प्रक्षिप_Current_Processp = get_Queue_प्रक्षिप_Process_Selfp(fl, processQueuep);
        AstBegin* ध्रुव taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(प्रक्षिप_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        प्रत्यागम processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ध्रुव beginp, File_पङ्क्तिः* ध्रुव fl) {
        ध्रुव स्वचालित it = m_beginDisableBegins.find(beginp);
        यदि (it != m_beginDisableBegins.अन्तः_2()) प्रत्यागम it->second;

        AstBegin* ध्रुव beginBodyp = नूतन AstBegin{fl, "", शून्यसूचकः, असत्य};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->समुच्चयः_Need_Process();
        v3Global.समुच्चयः_उपयुङ्क्ते_Timing();
        यदि (beginp->stmtsp()) beginBodyp->सङ्कलय_Stmtsp(beginp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());

        AstFork* ध्रुव forkp = नूतन AstFork{fl, VJoinType::JOIN};
        forkp->सङ्कलय_Forksp(beginBodyp);
        beginp->सङ्कलय_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        प्रत्यागम beginBodyp;
    }
    शून्य prependForkBranchQueuePushes(AstFork* ध्रुव forkp, AstVar* ध्रुव processQueuep,
                                      File_पङ्क्तिः* ध्रुव fl, तार्किक needProcess) {
        प्रति (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            यदि (needProcess) branchp->समुच्चयः_Need_Process();
            branchp->prependStmtsp(get_Queue_प्रक्षिप_Process_Selfp(fl, processQueuep));
        }
    }
    शून्य prependNestedForkBranchQueuePushes(Ast_ग्रन्थिः* ध्रुव nodep, AstVar* ध्रुव processQueuep,
                                            File_पङ्क्तिः* ध्रुव fl, तार्किक needProcess) {
        nodep->foreach([&](AstFork* ध्रुव forkp) {
            prependForkBranchQueuePushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ध्रुव beginp, File_पङ्क्तिः* ध्रुव fl) {
        ध्रुव स्वचालित it = m_beginDisableQueues.find(beginp);
        यदि (it != m_beginDisableQueues.अन्तः_2()) प्रत्यागम it->second;

        AstVar* ध्रुव processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_व्यञ्जकम्* ध्रुव प्रक्षिप_Current_Processp = get_Queue_प्रक्षिप_Process_Selfp(fl, processQueuep);
        AstBegin* ध्रुव beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(प्रक्षिप_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prependNestedForkBranchQueuePushes(beginBodyp, processQueuep, fl, असत्य);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        प्रत्यागम processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ध्रुव forkp, File_पङ्क्तिः* ध्रुव fl) {
        ध्रुव स्वचालित it = m_forkDisableQueues.find(forkp);
        यदि (it != m_forkDisableQueues.अन्तः_2()) प्रत्यागम it->second;

        AstVar* ध्रुव processQueuep = getProcessQueuep(forkp, fl);
        prependForkBranchQueuePushes(forkp, processQueuep, fl, सत्य);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        प्रति (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prependNestedForkBranchQueuePushes(branchp, processQueuep, fl, सत्य);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        प्रत्यागम processQueuep;
    }
    Ast_Stmt_व्यञ्जकम्* निवेशय_Kill_Stmtp(AstDisable* ध्रुव nodep, AstVar* ध्रुव processQueuep) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_Stmt_व्यञ्जकम्* ध्रुव killStmtp
            = getQueueKillStmtp(fl, processQueuep, queueDotted(nodep, processQueuep));
        nodep->सङ्कलय_अग्र_अत्र(killStmtp);
        प्रत्यागम killStmtp;
    }
    शून्य सङ्कलय_Jump_After_Kill(Ast_Stmt_व्यञ्जकम्* ध्रुव killStmtp, Ast_ग्रन्थिः* ध्रुव targetp) {
        Ast_Jump_खण्डम्* ध्रुव jmpBlockp = get_Jump_खण्डम्(targetp, असत्य);
        killStmtp->सङ्कलय_अग्र_अत्र(नूतन AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    शून्य handleDisableOnFork(AstDisable* ध्रुव nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstFork* ध्रुव targetp = VN_AS(nodep->targetp(), Fork);
        यदि (m_ftaskp) {
            यदि (!m_ftaskp->exists(
                    [targetp](ध्रुव Ast_ग्रन्थिः_खण्डम्* blockp) -> तार्किक { प्रत्यागम blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ध्रुव processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_व्यञ्जकम्* ध्रुव killStmtp = निवेशय_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        यदि (AstBegin* ध्रुव branchp = innerForkBranchp(targetp)) {
            सङ्कलय_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        यदि (nodep->dead()) प्रत्यागम;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_अङ्कः);
        m_modp = nodep;
        m_mod_Repeat_अङ्कः = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_पूर्णम्);
        m_खण्डम्_राशिः.प्रक्षिप_पृष्ठभागः(nodep);
        iterateChildren(nodep);
        m_खण्डम्_राशिः.निष्कासय_पृष्ठभागः();
    }
    शून्य दर्शय(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_पूर्णम्);
        VL_RESTORER(m_अन्तर्_Fork);
        m_अन्तर्_Fork = सत्य;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        प्रति (Ast_ग्रन्थिः_खण्डम्* ध्रुव blockp : vlstd::reverse_view(m_खण्डम्_राशिः)) {
            यदि (blockp->user3_समुच्चयः_Once()) भङ्ग;
        }
        m_खण्डम्_राशिः.प्रक्षिप_पृष्ठभागः(nodep);
        iterateChildren(nodep);
        m_खण्डम्_राशिः.निष्कासय_पृष्ठभागः();
    }
    शून्य दर्शय(AstStmtPragma* nodep) override {
        यदि (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_पूर्णम् = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        } अन्यथा यदि (nodep->pragp()->pragType() == VPragmaType::UNROLL_पूर्णम्) {
            m_unroll_पूर्णम् = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp = nodep->countp()->unlink_Fr_पृष्ठभागः_With_अग्र();
        ध्रुव string name = "__Vrepeat"s + cvt_गन्तव्यम्_Str(m_mod_Repeat_अङ्कः++);
        AstBegin* ध्रुव beginp = नूतन AstBegin{nodep->fileline(), "", शून्यसूचकः, सत्य};
        // Spec says value is integral, if negative is ignored
        AstVar* ध्रुव varp
            = नूतन AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->findIntDType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->प्रयुक्तम्_Loop_Idx(सत्य);
        beginp->सङ्कलय_Stmtsp(varp);
        Ast_ग्रन्थिः* initsp = नूतन Ast_नियोजय{
            nodep->fileline(), नूतन AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ग्रन्थिः* ध्रुव decp = नूतन Ast_नियोजय{
            nodep->fileline(), नूतन AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            नूतन AstSub{nodep->fileline(), नूतन AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       नूतन AstConst{nodep->fileline(), 1}}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव zerosp = नूतन AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstGtS{
            nodep->fileline(), नूतन AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ग्रन्थिः* ध्रुव bodysp = nodep->stmtsp();
        यदि (bodysp) bodysp->unlink_Fr_पृष्ठभागः_With_अग्र();
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, condp});
        loopp->सङ्कलय_Stmtsp(bodysp);
        loopp->सङ्कलय_Contsp(decp);
        यदि (!m_unroll_पूर्णम्.isDefault()) loopp->unroll(m_unroll_पूर्णम्);
        m_unroll_पूर्णम् = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->सङ्कलय_Stmtsp(initsp);
        beginp->सङ्कलय_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstLoop* nodep) override {
        यदि (!m_unroll_पूर्णम्.isDefault()) nodep->unroll(m_unroll_पूर्णम्);
        यदि (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNUSEDLOOP, सत्य);
        }
        m_unroll_पूर्णम् = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = असत्य;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_loopInc = सत्य;
        iterate_And_अग्र_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        यदि (nodep->contsp()) nodep->सङ्कलय_Stmtsp(nodep->contsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_अग्र_Null(nodep->bodyp());
    }
    शून्य दर्शय(AstReturn* nodep) override {
        iterateChildren(nodep);
        ध्रुव AstFunc* ध्रुव funcp = VN_CAST(m_ftaskp, Func);
        यदि (m_randsequencep) {
            nodep->replaceWith(नूतन AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        } अन्यथा यदि (m_अन्तर्_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        } अन्यथा यदि (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } अन्यथा यदि (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } अन्यथा यदि (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } अन्यथा {
            यदि (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->सङ्कलय_अत्र_This_As_अग्र(नूतन Ast_नियोजय{
                    nodep->fileline(),
                    नूतन AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_पृष्ठभागः_With_अग्र()});
            }
            // Jump to the end of the function call
            Ast_Jump_खण्डम्* ध्रुव blockp = get_Jump_खण्डम्(m_ftaskp, असत्य);
            nodep->सङ्कलय_अत्र_This_As_अग्र(नूतन AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstBreak* nodep) override {
        iterateChildren(nodep);
        यदि (!m_loopp && m_randsequencep) {
            nodep->replaceWith(नूतन AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        } अन्यथा यदि (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } अन्यथा {
            // Jump to the end of the loop
            Ast_Jump_खण्डम्* ध्रुव blockp = get_Jump_खण्डम्(m_loopp, असत्य);
            nodep->सङ्कलय_अग्र_अत्र(नूतन AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstContinue* nodep) override {
        iterateChildren(nodep);
        यदि (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } अन्यथा {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_खण्डम्* ध्रुव blockp = get_Jump_खण्डम्(m_loopp, सत्य);
            nodep->सङ्कलय_अग्र_अत्र(नूतन AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ग्रन्थिः* ध्रुव targetp = nodep->targetp();
        यदि (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        यदि (AstTask* ध्रुव taskp = VN_CAST(targetp, Task)) {
            AstVar* ध्रुव processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_व्यञ्जकम्* ध्रुव killStmtp = निवेशय_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            यदि (m_ftaskp == taskp) {
                Ast_ग्रन्थिः* jumpTargetp = taskp;
                ध्रुव स्वचालित it = m_taskDisableBegins.find(taskp);
                यदि (it != m_taskDisableBegins.अन्तः_2()) jumpTargetp = it->second;
                सङ्कलय_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } अन्यथा यदि (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } अन्यथा यदि (AstBegin* ध्रुव beginp = VN_CAST(targetp, Begin)) {
            यदि (exists_खण्डम्_Above(beginp->name())) {
                यदि (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_खण्डम्* ध्रुव blockp = get_Jump_खण्डम्(beginp, असत्य);
                    nodep->सङ्कलय_अग्र_अत्र(नूतन AstJumpGo{nodep->fileline(), blockp});
                } अन्यथा {
                    AstVar* ध्रुव processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_व्यञ्जकम्* ध्रुव killStmtp = निवेशय_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ध्रुव branchp = innerForkBranchp(beginp);
                    AstBegin* ध्रुव jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    सङ्कलय_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } अन्यथा {
                AstVar* ध्रुव processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                निवेशय_Kill_Stmtp(nodep, processQueuep);
            }
        } अन्यथा {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstFinish* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        यदि (m_अन्तर्_Fork) {
            nodep->replaceWith(नूतन AstFinishFork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_खण्डम्* ध्रुव blockp = get_Jump_खण्डम्(m_loopp, असत्य);
            nodep->सङ्कलय_अग्र_अत्र(नूतन AstJumpGo{nodep->fileline(), blockp});
        }
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (m_loopInc && nodep->varp()) nodep->varp()->प्रयुक्तम्_Loop_Idx(सत्य);
    }
    शून्य दर्शय(AstConst*) override {}
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = स्वभाव;
};

//######################################################################
// Task class functions

शून्य V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("linkjump", 0, dump_वृक्षः_Either_Level() >= 3);
}
