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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkJump.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Error.h"
#شامل "V3UniqueNames.h"

#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت LinkJumpVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;
    ثابت VNUser3_اندر_Use m_user3_اندر_Use;

    // STATE
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    Ast_گرہ* m_loopp = خالی_اشاریہ;  // Current loop
    AstRandSequence* m_randsequencep = خالی_اشاریہ;  // Current randsequence
    منطقی m_loopInc = جھوٹ;  // In loop increment
    منطقی m_اندر_Fork = جھوٹ;  // Under fork
    صحیح m_mod_Repeat_عدد = 0;  // Repeat counter
    VOptionBool m_unroll_بھرا;  // Pragma full, disable, or default unrolling
    std::سمتیہ<Ast_گرہ_بلاک*> m_بلاک_ڈھیر;  // All begin blocks above current node
    V3_Unique_نام m_قطار_نام{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ثابت AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ثابت AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ثابت AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ثابت AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ثابت AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    Ast_Jump_بلاک* get_Jump_بلاک(Ast_گرہ* nodep, منطقی انتہا_کا_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        اگر (انتہا_کا_Iter) {
            اگر (nodep->user1p()) واپس VN_AS(nodep->user1p(), Jump_بلاک);
        } ورنہ {
            اگر (nodep->user2p()) واپس VN_AS(nodep->user2p(), Jump_بلاک);
        }

        Ast_گرہ* underp = خالی_اشاریہ;
        منطقی under_and_اگلا = سچ;
        اگر (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!انتہا_کا_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } ورنہ اگر (Ast_گرہ_FTask* ثابت fTaskp = VN_CAST(nodep, گرہ_FTask)) {
            UASSERT_OBJ(!انتہا_کا_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } ورنہ اگر (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            اگر (انتہا_کا_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                اگر (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } ورنہ {
                underp = nodep;
                under_and_اگلا = جھوٹ;  // IE we skip the entire foreach
            }
        } ورنہ اگر (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            اگر (انتہا_کا_Iter) {
                underp = loopp->stmtsp();
            } ورنہ {
                underp = nodep;
                under_and_اگلا = جھوٹ;  // IE we skip the entire loop
            }
        } ورنہ {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            واپس خالی_اشاریہ;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        جب (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        اگر (!underp->nextp() || !under_and_اگلا) {
            اگر (Ast_Jump_بلاک* ثابت blockp = VN_CAST(underp, Jump_بلاک)) واپس blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        اگر (under_and_اگلا) {
            underp->unlink_Fr_عقب_With_اگلا(&repHandle);
        } ورنہ {
            underp->unlink_Fr_عقب(&repHandle);
        }
        Ast_Jump_بلاک* ثابت blockp = نیا Ast_Jump_بلاک{nodep->fileline(), underp};
        اگر (انتہا_کا_Iter) {
            nodep->user1p(blockp);
        } ورنہ {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        برائے (Ast_گرہ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            اگر (VN_IS(varp, Var)) blockp->جوڑو_یہاں_This_As_اگلا(varp->unlink_Fr_عقب());
        }
        واپس blockp;
    }
    خالی جوڑو_سابقہ_مقصد_بلاکیں_Recurse(ثابت std::string& سابقہ, Ast_گرہ* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        اگر (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            اگر (beginp->name() != "") beginp->name(سابقہ + beginp->name());
        }

        اگر (Ast_گرہ* ثابت refp = nodep->op1p()) جوڑو_سابقہ_مقصد_بلاکیں_Recurse(سابقہ, refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op2p()) جوڑو_سابقہ_مقصد_بلاکیں_Recurse(سابقہ, refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op3p()) جوڑو_سابقہ_مقصد_بلاکیں_Recurse(سابقہ, refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op4p()) جوڑو_سابقہ_مقصد_بلاکیں_Recurse(سابقہ, refp);
        اگر (Ast_گرہ* ثابت refp = nodep->nextp()) جوڑو_سابقہ_مقصد_بلاکیں_Recurse(سابقہ, refp);
    }
    منطقی exists_بلاک_Above(ثابت std::string& name) ثابت {
        برائے (ثابت Ast_گرہ_بلاک* ثابت stackp : vlstd::الٹ_view(m_بلاک_ڈھیر)) {
            اگر (stackp->name() == name) واپس سچ;
        }
        واپس جھوٹ;
    }
    ساکن Ast_گرہ_Module* ڈھونڈو_Owner_Modulep(Ast_گرہ* nodep) {
        برائے (Ast_گرہ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            اگر (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) واپس modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        واپس خالی_اشاریہ;  // LCOV_EXCL_LINE
    }
    ساکن std::string ہدف_Instance_راستہ(ثابت AstDisable* ثابت nodep) {
        اگر (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->ہدف_Refp(), VarXRef)) {
            واپس xrefp->dotted();
        }
        واپس "";
    }
    ساکن std::string قطار_Dotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        واپس processQueuep->is_عارضی() ? ہدف_Instance_راستہ(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_گرہ_بلاک* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = خالی_اشاریہ;
        Ast_گرہ_بلاک* پچھلا_Blockp = خالی_اشاریہ;
        برائے (Ast_گرہ_بلاک* ثابت blockp : vlstd::الٹ_view(m_بلاک_ڈھیر)) {
            اگر (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(پچھلا_Blockp, Begin);
            }
            اگر (blockp == targetp) واپس innerForkBranchp;
            پچھلا_Blockp = blockp;
        }
        واپس خالی_اشاریہ;
    }
    ساکن Ast_Stmt_اظہاریہ* get_قطار_ڈالو_Process_Selfp(AstVarRef* ثابت قطار_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_سطر* ثابت flp = قطار_Refp->fileline();
        واپس نیا Ast_Stmt_اظہاریہ{
            flp,
            نیا Ast_طریقہ_Call{flp, قطار_Refp, "push_back",
                              نیا AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ساکن Ast_گرہ_Var_Ref* new_قطار_Refp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        اگر (dotted != "") واپس نیا AstVarXRef{fl, processQueuep, dotted, access};
        اگر (!processQueuep->lifetime().isStatic() || processQueuep->is_عارضی()) {
            واپس نیا AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت چوٹی_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        واپس نیا AstVarRef{fl, چوٹی_Pkgp, processQueuep, access};
    }
    ساکن Ast_Stmt_اظہاریہ* get_قطار_ڈالو_Process_Selfp(File_سطر* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت قطار_Write_Refp
            = VN_AS(new_قطار_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        واپس get_قطار_ڈالو_Process_Selfp(قطار_Write_Refp);
    }
    ساکن Ast_Stmt_اظہاریہ* get_قطار_Kill_Stmtp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_گرہ_Var_Ref* ثابت قطار_Refp
            = new_قطار_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_قطار_Call = خالی_اشاریہ;
        برائے (Ast_گرہ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            اگر (itemp->name() == "killQueue") {
                kill_قطار_Call
                    = نیا AstTaskRef{fl, VN_AS(itemp, Task), نیا AstArg{fl, "", قطار_Refp}};
                توڑو;
            }
        }
        UASSERT(kill_قطار_Call, "Should be found");
        kill_قطار_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        واپس نیا Ast_Stmt_اظہاریہ{fl, kill_قطار_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_taskDisableBegins.ڈھونڈو(taskp);
        اگر (it != m_taskDisableBegins.انتہا()) واپس it->دوسرا;

        AstBegin* ثابت taskBodyp = نیا AstBegin{fl, "", خالی_اشاریہ, جھوٹ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->سیٹ_Need_Process();
        v3Global.سیٹ_استعمال_کرتا_Timing();
        // Ports stay in the task so calls see them
        برائے (Ast_گرہ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_گرہ* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            اگر (!varp || !varp->isIO()) taskBodyp->جوڑو_Stmtsp(stmtp->unlink_Fr_عقب());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = نیا AstFork{fl, VJoinType::JOIN};
        forkp->جوڑو_Forksp(taskBodyp);
        taskp->جوڑو_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        واپس taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_گرہ* ثابت nodep, File_سطر* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = نیا AstVar{
            fl, varType, m_قطار_نام.get(nodep->name()), VFlag_بچہ_DType{},
            نیا Ast_قطار_DType{
                fl, VFlag_بچہ_DType{},
                نیا AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), خالی_اشاریہ},
                خالی_اشاریہ}};
        processQueuep->process_قطار(سچ);
        processQueuep->سیٹ_Ignore_Sched_Write();
        واپس processQueuep;
    }
    AstVar* getProcessQueuep(Ast_گرہ* ثابت nodep, File_سطر* ثابت fl) {
        Ast_گرہ_Module* ثابت ownerp = ڈھونڈو_Owner_Modulep(nodep);

        اگر (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت چوٹی_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            چوٹی_Pkgp->جوڑو_Stmtsp(processQueuep);
            واپس processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->جوڑو_Stmtsp(processQueuep);
        واپس processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_taskDisableQueues.ڈھونڈو(taskp);
        اگر (it != m_taskDisableQueues.انتہا()) واپس it->دوسرا;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_اظہاریہ* ثابت ڈالو_Current_Processp = get_قطار_ڈالو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ڈالو_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        واپس processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_beginDisableBegins.ڈھونڈو(beginp);
        اگر (it != m_beginDisableBegins.انتہا()) واپس it->دوسرا;

        AstBegin* ثابت beginBodyp = نیا AstBegin{fl, "", خالی_اشاریہ, جھوٹ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->سیٹ_Need_Process();
        v3Global.سیٹ_استعمال_کرتا_Timing();
        اگر (beginp->stmtsp()) beginBodyp->جوڑو_Stmtsp(beginp->stmtsp()->unlink_Fr_عقب_With_اگلا());

        AstFork* ثابت forkp = نیا AstFork{fl, VJoinType::JOIN};
        forkp->جوڑو_Forksp(beginBodyp);
        beginp->جوڑو_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        واپس beginBodyp;
    }
    خالی prepend_Fork_Branch_قطار_Pushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_سطر* ثابت fl, منطقی needProcess) {
        برائے (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            اگر (needProcess) branchp->سیٹ_Need_Process();
            branchp->prependStmtsp(get_قطار_ڈالو_Process_Selfp(fl, processQueuep));
        }
    }
    خالی prepend_Nested_Fork_Branch_قطار_Pushes(Ast_گرہ* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_سطر* ثابت fl, منطقی needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prepend_Fork_Branch_قطار_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_beginDisableQueues.ڈھونڈو(beginp);
        اگر (it != m_beginDisableQueues.انتہا()) واپس it->دوسرا;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_اظہاریہ* ثابت ڈالو_Current_Processp = get_قطار_ڈالو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ڈالو_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_قطار_Pushes(beginBodyp, processQueuep, fl, جھوٹ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        واپس processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_forkDisableQueues.ڈھونڈو(forkp);
        اگر (it != m_forkDisableQueues.انتہا()) واپس it->دوسرا;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_قطار_Pushes(forkp, processQueuep, fl, سچ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        برائے (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_قطار_Pushes(branchp, processQueuep, fl, سچ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        واپس processQueuep;
    }
    Ast_Stmt_اظہاریہ* داخل_کرو_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_Stmt_اظہاریہ* ثابت killStmtp
            = get_قطار_Kill_Stmtp(fl, processQueuep, قطار_Dotted(nodep, processQueuep));
        nodep->جوڑو_اگلا_یہاں(killStmtp);
        واپس killStmtp;
    }
    خالی جوڑو_Jump_After_Kill(Ast_Stmt_اظہاریہ* ثابت killStmtp, Ast_گرہ* ثابت targetp) {
        Ast_Jump_بلاک* ثابت jmpBlockp = get_Jump_بلاک(targetp, جھوٹ);
        killStmtp->جوڑو_اگلا_یہاں(نیا AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    خالی handleDisableOnFork(AstDisable* ثابت nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_سطر* ثابت fl = nodep->fileline();
        AstFork* ثابت targetp = VN_AS(nodep->targetp(), Fork);
        اگر (m_ftaskp) {
            اگر (!m_ftaskp->exists(
                    [targetp](ثابت Ast_گرہ_بلاک* blockp) -> منطقی { واپس blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ثابت processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        اگر (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            جوڑو_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->dead()) واپس;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_عدد);
        m_modp = nodep;
        m_mod_Repeat_عدد = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_بھرا);
        m_بلاک_ڈھیر.ڈالو_عقب(nodep);
        iterateChildren(nodep);
        m_بلاک_ڈھیر.نکالو_عقب();
    }
    خالی دورہ(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_بھرا);
        VL_RESTORER(m_اندر_Fork);
        m_اندر_Fork = سچ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        برائے (Ast_گرہ_بلاک* ثابت blockp : vlstd::الٹ_view(m_بلاک_ڈھیر)) {
            اگر (blockp->user3_سیٹ_Once()) توڑو;
        }
        m_بلاک_ڈھیر.ڈالو_عقب(nodep);
        iterateChildren(nodep);
        m_بلاک_ڈھیر.نکالو_عقب();
    }
    خالی دورہ(AstStmtPragma* nodep) override {
        اگر (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_بھرا = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        } ورنہ اگر (nodep->pragp()->pragType() == VPragmaType::UNROLL_بھرا) {
            m_unroll_بھرا = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        } ورنہ {
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_گرہ_اظہاریہ* ثابت countp = nodep->countp()->unlink_Fr_عقب_With_اگلا();
        ثابت string name = "__Vrepeat"s + cvt_مقصد_Str(m_mod_Repeat_عدد++);
        AstBegin* ثابت beginp = نیا AstBegin{nodep->fileline(), "", خالی_اشاریہ, سچ};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = نیا AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->ڈھونڈو_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->مستعمل_Loop_Idx(سچ);
        beginp->جوڑو_Stmtsp(varp);
        Ast_گرہ* initsp = نیا Ast_تفویض{
            nodep->fileline(), نیا AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_گرہ* ثابت decp = نیا Ast_تفویض{
            nodep->fileline(), نیا AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            نیا AstSub{nodep->fileline(), نیا AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       نیا AstConst{nodep->fileline(), 1}}};
        Ast_گرہ_اظہاریہ* ثابت zerosp = نیا AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_گرہ_اظہاریہ* ثابت condp = نیا AstGtS{
            nodep->fileline(), نیا AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_گرہ* ثابت bodysp = nodep->stmtsp();
        اگر (bodysp) bodysp->unlink_Fr_عقب_With_اگلا();
        File_سطر* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = نیا AstLoop{flp};
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, condp});
        loopp->جوڑو_Stmtsp(bodysp);
        loopp->جوڑو_Contsp(decp);
        اگر (!m_unroll_بھرا.isDefault()) loopp->unroll(m_unroll_بھرا);
        m_unroll_بھرا = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->جوڑو_Stmtsp(initsp);
        beginp->جوڑو_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstLoop* nodep) override {
        اگر (!m_unroll_بھرا.isDefault()) nodep->unroll(m_unroll_بھرا);
        اگر (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDLOOP, سچ);
        }
        m_unroll_بھرا = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = جھوٹ;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_loopInc = سچ;
        iterate_And_اگلا_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        اگر (nodep->contsp()) nodep->جوڑو_Stmtsp(nodep->contsp()->unlink_Fr_عقب_With_اگلا());
    }
    خالی دورہ(Ast_گرہ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_اگلا_Null(nodep->bodyp());
    }
    خالی دورہ(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        اگر (m_randsequencep) {
            nodep->replaceWith(نیا AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        } ورنہ اگر (m_اندر_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        } ورنہ اگر (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } ورنہ اگر (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } ورنہ اگر (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } ورنہ {
            اگر (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->جوڑو_یہاں_This_As_اگلا(نیا Ast_تفویض{
                    nodep->fileline(),
                    نیا AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_عقب_With_اگلا()});
            }
            // Jump to the end of the function call
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_ftaskp, جھوٹ);
            nodep->جوڑو_یہاں_This_As_اگلا(نیا AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstBreak* nodep) override {
        iterateChildren(nodep);
        اگر (!m_loopp && m_randsequencep) {
            nodep->replaceWith(نیا AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        } ورنہ اگر (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } ورنہ {
            // Jump to the end of the loop
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, جھوٹ);
            nodep->جوڑو_اگلا_یہاں(نیا AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstContinue* nodep) override {
        iterateChildren(nodep);
        اگر (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } ورنہ {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, سچ);
            nodep->جوڑو_اگلا_یہاں(نیا AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_گرہ* ثابت targetp = nodep->targetp();
        اگر (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        اگر (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            اگر (m_ftaskp == taskp) {
                Ast_گرہ* jumpTargetp = taskp;
                ثابت خودکار it = m_taskDisableBegins.ڈھونڈو(taskp);
                اگر (it != m_taskDisableBegins.انتہا()) jumpTargetp = it->دوسرا;
                جوڑو_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } ورنہ اگر (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } ورنہ اگر (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            اگر (exists_بلاک_Above(beginp->name())) {
                اگر (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(beginp, جھوٹ);
                    nodep->جوڑو_اگلا_یہاں(نیا AstJumpGo{nodep->fileline(), blockp});
                } ورنہ {
                    AstVar* ثابت processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ثابت branchp = innerForkBranchp(beginp);
                    AstBegin* ثابت jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    جوڑو_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } ورنہ {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                داخل_کرو_Kill_Stmtp(nodep, processQueuep);
            }
        } ورنہ {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(Ast_اختتام* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        اگر (m_اندر_Fork) {
            nodep->replaceWith(نیا Ast_اختتام_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, جھوٹ);
            nodep->جوڑو_اگلا_یہاں(نیا AstJumpGo{nodep->fileline(), blockp});
        }
    }
    خالی دورہ(AstVarRef* nodep) override {
        اگر (m_loopInc && nodep->varp()) nodep->varp()->مستعمل_Loop_Idx(سچ);
    }
    خالی دورہ(AstConst*) override {}
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = طےشدہ;
};

//######################################################################
// Task class functions

خالی V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkjump", 0, dump_درخت_Either_Level() >= 3);
}
