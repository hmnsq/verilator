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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkJump.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి LinkJumpVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;
    స్థిరరాశి VNUser3_లోపల_Use m_user3_లోపల_Use;

    // STATE
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_కణుపు* m_loopp = శూన్యసూచిక;  // Current loop
    AstRandSequence* m_randsequencep = శూన్యసూచిక;  // Current randsequence
    తార్కిక m_loopInc = అబద్ధం;  // In loop increment
    తార్కిక m_లోపల_Fork = అబద్ధం;  // Under fork
    పూర్ణాంకం m_mod_Repeat_అంకె = 0;  // Repeat counter
    VOptionBool m_unroll_నిండిన;  // Pragma full, disable, or default unrolling
    std::సదిశ<Ast_కణుపు_ఖండం*> m_ఖండం_దొంతర;  // All begin blocks above current node
    V3_Unique_పేర్లు m_వరుస_పేర్లు{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<స్థిరరాశి AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<స్థిరరాశి AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<స్థిరరాశి AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<స్థిరరాశి AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<స్థిరరాశి AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    Ast_Jump_ఖండం* get_Jump_ఖండం(Ast_కణుపు* nodep, తార్కిక చివర_యొక్క_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        ఒకవేళ (చివర_యొక్క_Iter) {
            ఒకవేళ (nodep->user1p()) తిరిగి VN_AS(nodep->user1p(), Jump_ఖండం);
        } లేకపోతే {
            ఒకవేళ (nodep->user2p()) తిరిగి VN_AS(nodep->user2p(), Jump_ఖండం);
        }

        Ast_కణుపు* underp = శూన్యసూచిక;
        తార్కిక under_and_తదుపరి = నిజం;
        ఒకవేళ (AstBegin* స్థిరరాశి blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!చివర_యొక్క_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } లేకపోతే ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి fTaskp = VN_CAST(nodep, కణుపు_FTask)) {
            UASSERT_OBJ(!చివర_యొక్క_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } లేకపోతే ఒకవేళ (AstForeach* స్థిరరాశి foreachp = VN_CAST(nodep, Foreach)) {
            ఒకవేళ (చివర_యొక్క_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                ఒకవేళ (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } లేకపోతే {
                underp = nodep;
                under_and_తదుపరి = అబద్ధం;  // IE we skip the entire foreach
            }
        } లేకపోతే ఒకవేళ (AstLoop* స్థిరరాశి loopp = VN_CAST(nodep, Loop)) {
            ఒకవేళ (చివర_యొక్క_Iter) {
                underp = loopp->stmtsp();
            } లేకపోతే {
                underp = nodep;
                under_and_తదుపరి = అబద్ధం;  // IE we skip the entire loop
            }
        } లేకపోతే {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            తిరిగి శూన్యసూచిక;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        అయితే (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        ఒకవేళ (!underp->nextp() || !under_and_తదుపరి) {
            ఒకవేళ (Ast_Jump_ఖండం* స్థిరరాశి blockp = VN_CAST(underp, Jump_ఖండం)) తిరిగి blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        ఒకవేళ (under_and_తదుపరి) {
            underp->unlink_Fr_వెనుక_With_తదుపరి(&repHandle);
        } లేకపోతే {
            underp->unlink_Fr_వెనుక(&repHandle);
        }
        Ast_Jump_ఖండం* స్థిరరాశి blockp = కొత్త Ast_Jump_ఖండం{nodep->fileline(), underp};
        ఒకవేళ (చివర_యొక్క_Iter) {
            nodep->user1p(blockp);
        } లేకపోతే {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        కొరకు (Ast_కణుపు *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            ఒకవేళ (VN_IS(varp, Var)) blockp->కూడు_ఇక్కడ_This_As_తదుపరి(varp->unlink_Fr_వెనుక());
        }
        తిరిగి blockp;
    }
    శూన్య కూడు_ఉపసర్గ_గమ్యం_ఖండాలు_Recurse(స్థిరరాశి std::string& ఉపసర్గ, Ast_కణుపు* స్థిరరాశి nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(nodep, Begin)) {
            ఒకవేళ (beginp->name() != "") beginp->name(ఉపసర్గ + beginp->name());
        }

        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op1p()) కూడు_ఉపసర్గ_గమ్యం_ఖండాలు_Recurse(ఉపసర్గ, refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op2p()) కూడు_ఉపసర్గ_గమ్యం_ఖండాలు_Recurse(ఉపసర్గ, refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op3p()) కూడు_ఉపసర్గ_గమ్యం_ఖండాలు_Recurse(ఉపసర్గ, refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op4p()) కూడు_ఉపసర్గ_గమ్యం_ఖండాలు_Recurse(ఉపసర్గ, refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->nextp()) కూడు_ఉపసర్గ_గమ్యం_ఖండాలు_Recurse(ఉపసర్గ, refp);
    }
    తార్కిక exists_ఖండం_Above(స్థిరరాశి std::string& name) స్థిరరాశి {
        కొరకు (స్థిరరాశి Ast_కణుపు_ఖండం* స్థిరరాశి stackp : vlstd::తిరగేయి_view(m_ఖండం_దొంతర)) {
            ఒకవేళ (stackp->name() == name) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    స్థిర Ast_కణుపు_Module* కనుగొను_Owner_Modulep(Ast_కణుపు* nodep) {
        కొరకు (Ast_కణుపు* curp = nodep; curp; curp = curp->aboveLoopp()) {
            ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి modp = VN_CAST(curp, కణుపు_Module)) తిరిగి modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        తిరిగి శూన్యసూచిక;  // LCOV_EXCL_LINE
    }
    స్థిర std::string లక్ష్యం_Instance_మార్గం(స్థిరరాశి AstDisable* స్థిరరాశి nodep) {
        ఒకవేళ (స్థిరరాశి AstVarXRef* స్థిరరాశి xrefp = VN_CAST(nodep->లక్ష్యం_Refp(), VarXRef)) {
            తిరిగి xrefp->dotted();
        }
        తిరిగి "";
    }
    స్థిర std::string వరుస_Dotted(స్థిరరాశి AstDisable* స్థిరరాశి nodep,
                                   స్థిరరాశి AstVar* స్థిరరాశి processQueuep) {
        తిరిగి processQueuep->is_తాత్కాలిక() ? లక్ష్యం_Instance_మార్గం(nodep) : "";
    }
    AstBegin* innerForkBranchp(స్థిరరాశి Ast_కణుపు_ఖండం* స్థిరరాశి targetp) స్థిరరాశి {
        AstBegin* innerForkBranchp = శూన్యసూచిక;
        Ast_కణుపు_ఖండం* prevBlockp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు_ఖండం* స్థిరరాశి blockp : vlstd::తిరగేయి_view(m_ఖండం_దొంతర)) {
            ఒకవేళ (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            ఒకవేళ (blockp == targetp) తిరిగి innerForkBranchp;
            prevBlockp = blockp;
        }
        తిరిగి శూన్యసూచిక;
    }
    స్థిర Ast_Stmt_సమాసం* get_వరుస_నెట్టు_Process_Selfp(AstVarRef* స్థిరరాశి వరుస_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_పంక్తి* స్థిరరాశి flp = వరుస_Refp->fileline();
        తిరిగి కొత్త Ast_Stmt_సమాసం{
            flp,
            కొత్త Ast_పద్ధతి_Call{flp, వరుస_Refp, "push_back",
                              కొత్త AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    స్థిర Ast_కణుపు_Var_Ref* new_వరుస_Refp(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి processQueuep,
                                       స్థిరరాశి VAccess& access, స్థిరరాశి std::string& dotted = "") {
        ఒకవేళ (dotted != "") తిరిగి కొత్త AstVarXRef{fl, processQueuep, dotted, access};
        ఒకవేళ (!processQueuep->lifetime().isStatic() || processQueuep->is_తాత్కాలిక()) {
            తిరిగి కొత్త AstVarRef{fl, processQueuep, access};
        }
        AstPackage* స్థిరరాశి శిఖరం_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        తిరిగి కొత్త AstVarRef{fl, శిఖరం_Pkgp, processQueuep, access};
    }
    స్థిర Ast_Stmt_సమాసం* get_వరుస_నెట్టు_Process_Selfp(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి processQueuep) {
        AstVarRef* స్థిరరాశి వరుస_Write_Refp
            = VN_AS(new_వరుస_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        తిరిగి get_వరుస_నెట్టు_Process_Selfp(వరుస_Write_Refp);
    }
    స్థిర Ast_Stmt_సమాసం* get_వరుస_Kill_Stmtp(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి processQueuep,
                                          స్థిరరాశి std::string& dotted = "") {
        Ast_కణుపు_Var_Ref* స్థిరరాశి వరుస_Refp
            = new_వరుస_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_వరుస_Call = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            ఒకవేళ (itemp->name() == "killQueue") {
                kill_వరుస_Call
                    = కొత్త AstTaskRef{fl, VN_AS(itemp, Task), కొత్త AstArg{fl, "", వరుస_Refp}};
                ఆపు;
            }
        }
        UASSERT(kill_వరుస_Call, "Should be found");
        kill_వరుస_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        తిరిగి కొత్త Ast_Stmt_సమాసం{fl, kill_వరుస_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* స్థిరరాశి taskp, File_పంక్తి* స్థిరరాశి fl) {
        స్థిరరాశి స్వయంచాలక it = m_taskDisableBegins.కనుగొను(taskp);
        ఒకవేళ (it != m_taskDisableBegins.చివర()) తిరిగి it->రెండవ;

        AstBegin* స్థిరరాశి taskBodyp = కొత్త AstBegin{fl, "", శూన్యసూచిక, అబద్ధం};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->సమితి_Need_Process();
        v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
        // Ports stay in the task so calls see them
        కొరకు (Ast_కణుపు* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_కణుపు* స్థిరరాశి nextp = stmtp->nextp();
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var);
            ఒకవేళ (!varp || !varp->isIO()) taskBodyp->కూడు_Stmtsp(stmtp->unlink_Fr_వెనుక());
            stmtp = nextp;
        }

        AstFork* స్థిరరాశి forkp = కొత్త AstFork{fl, VJoinType::JOIN};
        forkp->కూడు_Forksp(taskBodyp);
        taskp->కూడు_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        తిరిగి taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_కణుపు* స్థిరరాశి nodep, File_పంక్తి* స్థిరరాశి fl, స్థిరరాశి VVarType& varType) {
        AstVar* స్థిరరాశి processQueuep = కొత్త AstVar{
            fl, varType, m_వరుస_పేర్లు.get(nodep->name()), VFlag_బిడ్డ_DType{},
            కొత్త Ast_వరుస_DType{
                fl, VFlag_బిడ్డ_DType{},
                కొత్త AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), శూన్యసూచిక},
                శూన్యసూచిక}};
        processQueuep->process_వరుస(నిజం);
        processQueuep->సమితి_Ignore_Sched_Write();
        తిరిగి processQueuep;
    }
    AstVar* getProcessQueuep(Ast_కణుపు* స్థిరరాశి nodep, File_పంక్తి* స్థిరరాశి fl) {
        Ast_కణుపు_Module* స్థిరరాశి ownerp = కనుగొను_Owner_Modulep(nodep);

        ఒకవేళ (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* స్థిరరాశి శిఖరం_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* స్థిరరాశి processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            శిఖరం_Pkgp->కూడు_Stmtsp(processQueuep);
            తిరిగి processQueuep;
        }

        AstVar* స్థిరరాశి processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->కూడు_Stmtsp(processQueuep);
        తిరిగి processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* స్థిరరాశి taskp, File_పంక్తి* స్థిరరాశి fl) {
        స్థిరరాశి స్వయంచాలక it = m_taskDisableQueues.కనుగొను(taskp);
        ఒకవేళ (it != m_taskDisableQueues.చివర()) తిరిగి it->రెండవ;

        AstVar* స్థిరరాశి processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_సమాసం* స్థిరరాశి నెట్టు_Current_Processp = get_వరుస_నెట్టు_Process_Selfp(fl, processQueuep);
        AstBegin* స్థిరరాశి taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(నెట్టు_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        తిరిగి processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* స్థిరరాశి beginp, File_పంక్తి* స్థిరరాశి fl) {
        స్థిరరాశి స్వయంచాలక it = m_beginDisableBegins.కనుగొను(beginp);
        ఒకవేళ (it != m_beginDisableBegins.చివర()) తిరిగి it->రెండవ;

        AstBegin* స్థిరరాశి beginBodyp = కొత్త AstBegin{fl, "", శూన్యసూచిక, అబద్ధం};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->సమితి_Need_Process();
        v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
        ఒకవేళ (beginp->stmtsp()) beginBodyp->కూడు_Stmtsp(beginp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());

        AstFork* స్థిరరాశి forkp = కొత్త AstFork{fl, VJoinType::JOIN};
        forkp->కూడు_Forksp(beginBodyp);
        beginp->కూడు_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        తిరిగి beginBodyp;
    }
    శూన్య prepend_Fork_Branch_వరుస_Pushes(AstFork* స్థిరరాశి forkp, AstVar* స్థిరరాశి processQueuep,
                                      File_పంక్తి* స్థిరరాశి fl, తార్కిక needProcess) {
        కొరకు (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            ఒకవేళ (needProcess) branchp->సమితి_Need_Process();
            branchp->prependStmtsp(get_వరుస_నెట్టు_Process_Selfp(fl, processQueuep));
        }
    }
    శూన్య prepend_Nested_Fork_Branch_వరుస_Pushes(Ast_కణుపు* స్థిరరాశి nodep, AstVar* స్థిరరాశి processQueuep,
                                            File_పంక్తి* స్థిరరాశి fl, తార్కిక needProcess) {
        nodep->foreach([&](AstFork* స్థిరరాశి forkp) {
            prepend_Fork_Branch_వరుస_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* స్థిరరాశి beginp, File_పంక్తి* స్థిరరాశి fl) {
        స్థిరరాశి స్వయంచాలక it = m_beginDisableQueues.కనుగొను(beginp);
        ఒకవేళ (it != m_beginDisableQueues.చివర()) తిరిగి it->రెండవ;

        AstVar* స్థిరరాశి processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_సమాసం* స్థిరరాశి నెట్టు_Current_Processp = get_వరుస_నెట్టు_Process_Selfp(fl, processQueuep);
        AstBegin* స్థిరరాశి beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(నెట్టు_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_వరుస_Pushes(beginBodyp, processQueuep, fl, అబద్ధం);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        తిరిగి processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* స్థిరరాశి forkp, File_పంక్తి* స్థిరరాశి fl) {
        స్థిరరాశి స్వయంచాలక it = m_forkDisableQueues.కనుగొను(forkp);
        ఒకవేళ (it != m_forkDisableQueues.చివర()) తిరిగి it->రెండవ;

        AstVar* స్థిరరాశి processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_వరుస_Pushes(forkp, processQueuep, fl, నిజం);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        కొరకు (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_వరుస_Pushes(branchp, processQueuep, fl, నిజం);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        తిరిగి processQueuep;
    }
    Ast_Stmt_సమాసం* చొప్పించు_Kill_Stmtp(AstDisable* స్థిరరాశి nodep, AstVar* స్థిరరాశి processQueuep) {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_Stmt_సమాసం* స్థిరరాశి killStmtp
            = get_వరుస_Kill_Stmtp(fl, processQueuep, వరుస_Dotted(nodep, processQueuep));
        nodep->కూడు_తదుపరి_ఇక్కడ(killStmtp);
        తిరిగి killStmtp;
    }
    శూన్య కూడు_Jump_After_Kill(Ast_Stmt_సమాసం* స్థిరరాశి killStmtp, Ast_కణుపు* స్థిరరాశి targetp) {
        Ast_Jump_ఖండం* స్థిరరాశి jmpBlockp = get_Jump_ఖండం(targetp, అబద్ధం);
        killStmtp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    శూన్య handleDisableOnFork(AstDisable* స్థిరరాశి nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstFork* స్థిరరాశి targetp = VN_AS(nodep->targetp(), Fork);
        ఒకవేళ (m_ftaskp) {
            ఒకవేళ (!m_ftaskp->exists(
                    [targetp](స్థిరరాశి Ast_కణుపు_ఖండం* blockp) -> తార్కిక { తిరిగి blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* స్థిరరాశి processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_సమాసం* స్థిరరాశి killStmtp = చొప్పించు_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        ఒకవేళ (AstBegin* స్థిరరాశి branchp = innerForkBranchp(targetp)) {
            కూడు_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->dead()) తిరిగి;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_అంకె);
        m_modp = nodep;
        m_mod_Repeat_అంకె = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_నిండిన);
        m_ఖండం_దొంతర.నెట్టు_వెనుక(nodep);
        iterateChildren(nodep);
        m_ఖండం_దొంతర.తీయి_వెనుక();
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_నిండిన);
        VL_RESTORER(m_లోపల_Fork);
        m_లోపల_Fork = నిజం;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        కొరకు (Ast_కణుపు_ఖండం* స్థిరరాశి blockp : vlstd::తిరగేయి_view(m_ఖండం_దొంతర)) {
            ఒకవేళ (blockp->user3_సమితి_Once()) ఆపు;
        }
        m_ఖండం_దొంతర.నెట్టు_వెనుక(nodep);
        iterateChildren(nodep);
        m_ఖండం_దొంతర.తీయి_వెనుక();
    }
    శూన్య సందర్శించు(AstStmtPragma* nodep) override {
        ఒకవేళ (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_నిండిన = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        } లేకపోతే ఒకవేళ (nodep->pragp()->pragType() == VPragmaType::UNROLL_నిండిన) {
            m_unroll_నిండిన = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_కణుపు_సమాసం* స్థిరరాశి countp = nodep->countp()->unlink_Fr_వెనుక_With_తదుపరి();
        స్థిరరాశి string name = "__Vrepeat"s + cvt_గమ్యం_Str(m_mod_Repeat_అంకె++);
        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{nodep->fileline(), "", శూన్యసూచిక, నిజం};
        // Spec says value is integral, if negative is ignored
        AstVar* స్థిరరాశి varp
            = కొత్త AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->కనుగొను_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->వాడిన_Loop_Idx(నిజం);
        beginp->కూడు_Stmtsp(varp);
        Ast_కణుపు* initsp = కొత్త Ast_కేటాయించు{
            nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_కణుపు* స్థిరరాశి decp = కొత్త Ast_కేటాయించు{
            nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            కొత్త AstSub{nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       కొత్త AstConst{nodep->fileline(), 1}}};
        Ast_కణుపు_సమాసం* స్థిరరాశి zerosp = కొత్త AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstGtS{
            nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_కణుపు* స్థిరరాశి bodysp = nodep->stmtsp();
        ఒకవేళ (bodysp) bodysp->unlink_Fr_వెనుక_With_తదుపరి();
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, condp});
        loopp->కూడు_Stmtsp(bodysp);
        loopp->కూడు_Contsp(decp);
        ఒకవేళ (!m_unroll_నిండిన.isDefault()) loopp->unroll(m_unroll_నిండిన);
        m_unroll_నిండిన = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->కూడు_Stmtsp(initsp);
        beginp->కూడు_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        ఒకవేళ (!m_unroll_నిండిన.isDefault()) nodep->unroll(m_unroll_నిండిన);
        ఒకవేళ (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDLOOP, నిజం);
        }
        m_unroll_నిండిన = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = అబద్ధం;
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        m_loopInc = నిజం;
        iterate_And_తదుపరి_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        ఒకవేళ (nodep->contsp()) nodep->కూడు_Stmtsp(nodep->contsp()->unlink_Fr_వెనుక_With_తదుపరి());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_తదుపరి_Null(nodep->bodyp());
    }
    శూన్య సందర్శించు(AstReturn* nodep) override {
        iterateChildren(nodep);
        స్థిరరాశి AstFunc* స్థిరరాశి funcp = VN_CAST(m_ftaskp, Func);
        ఒకవేళ (m_randsequencep) {
            nodep->replaceWith(కొత్త AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (m_లోపల_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } లేకపోతే ఒకవేళ (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } లేకపోతే ఒకవేళ (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } లేకపోతే {
            ఒకవేళ (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->కూడు_ఇక్కడ_This_As_తదుపరి(కొత్త Ast_కేటాయించు{
                    nodep->fileline(),
                    కొత్త AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_వెనుక_With_తదుపరి()});
            }
            // Jump to the end of the function call
            Ast_Jump_ఖండం* స్థిరరాశి blockp = get_Jump_ఖండం(m_ftaskp, అబద్ధం);
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(కొత్త AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstBreak* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!m_loopp && m_randsequencep) {
            nodep->replaceWith(కొత్త AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } లేకపోతే {
            // Jump to the end of the loop
            Ast_Jump_ఖండం* స్థిరరాశి blockp = get_Jump_ఖండం(m_loopp, అబద్ధం);
            nodep->కూడు_తదుపరి_ఇక్కడ(కొత్త AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstContinue* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } లేకపోతే {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_ఖండం* స్థిరరాశి blockp = get_Jump_ఖండం(m_loopp, నిజం);
            nodep->కూడు_తదుపరి_ఇక్కడ(కొత్త AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_కణుపు* స్థిరరాశి targetp = nodep->targetp();
        ఒకవేళ (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        ఒకవేళ (AstTask* స్థిరరాశి taskp = VN_CAST(targetp, Task)) {
            AstVar* స్థిరరాశి processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_సమాసం* స్థిరరాశి killStmtp = చొప్పించు_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            ఒకవేళ (m_ftaskp == taskp) {
                Ast_కణుపు* jumpTargetp = taskp;
                స్థిరరాశి స్వయంచాలక it = m_taskDisableBegins.కనుగొను(taskp);
                ఒకవేళ (it != m_taskDisableBegins.చివర()) jumpTargetp = it->రెండవ;
                కూడు_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } లేకపోతే ఒకవేళ (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } లేకపోతే ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(targetp, Begin)) {
            ఒకవేళ (exists_ఖండం_Above(beginp->name())) {
                ఒకవేళ (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_ఖండం* స్థిరరాశి blockp = get_Jump_ఖండం(beginp, అబద్ధం);
                    nodep->కూడు_తదుపరి_ఇక్కడ(కొత్త AstJumpGo{nodep->fileline(), blockp});
                } లేకపోతే {
                    AstVar* స్థిరరాశి processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_సమాసం* స్థిరరాశి killStmtp = చొప్పించు_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* స్థిరరాశి branchp = innerForkBranchp(beginp);
                    AstBegin* స్థిరరాశి jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    కూడు_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } లేకపోతే {
                AstVar* స్థిరరాశి processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                చొప్పించు_Kill_Stmtp(nodep, processQueuep);
            }
        } లేకపోతే {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_ముగింపు* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        ఒకవేళ (m_లోపల_Fork) {
            nodep->replaceWith(కొత్త Ast_ముగింపు_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_ఖండం* స్థిరరాశి blockp = get_Jump_ఖండం(m_loopp, అబద్ధం);
            nodep->కూడు_తదుపరి_ఇక్కడ(కొత్త AstJumpGo{nodep->fileline(), blockp});
        }
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (m_loopInc && nodep->varp()) nodep->varp()->వాడిన_Loop_Idx(నిజం);
    }
    శూన్య సందర్శించు(AstConst*) override {}
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = అప్రమేయ;
};

//######################################################################
// Task class functions

శూన్య V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("linkjump", 0, dump_వృక్షం_Either_Level() >= 3);
}
