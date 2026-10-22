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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkJump.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Error.h"
#ضمن "V3UniqueNames.h"

#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف LinkJumpVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;
    ثابت VNUser2_وچ_Use m_user2_وچ_Use;
    ثابت VNUser3_وچ_Use m_user3_وچ_Use;

    // STATE
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Current function/task
    Ast_گرہ* m_loopp = خالی_سوچک;  // Current loop
    AstRandSequence* m_randsequencep = خالی_سوچک;  // Current randsequence
    منطقي m_loopInc = خطأ;  // In loop increment
    منطقي m_وچ_Fork = خطأ;  // Under fork
    صحيح m_mod_Repeat_عدد = 0;  // Repeat counter
    VOptionBool m_unroll_بھریا;  // Pragma full, disable, or default unrolling
    std::سمتیہ<Ast_گرہ_بلاک*> m_بلاک_ڈھیر;  // All begin blocks above current node
    V3_Unique_ناں m_قطار_ناں{
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
    Ast_Jump_بلاک* get_Jump_بلاک(Ast_گرہ* nodep, منطقي سرا_دا_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        إذا (سرا_دا_Iter) {
            إذا (nodep->user1p()) أرجع VN_AS(nodep->user1p(), Jump_بلاک);
        } وإلا {
            إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), Jump_بلاک);
        }

        Ast_گرہ* underp = خالی_سوچک;
        منطقي under_and_اگلا = سچ;
        إذا (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!سرا_دا_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } وإلا إذا (Ast_گرہ_FTask* ثابت fTaskp = VN_CAST(nodep, گرہ_FTask)) {
            UASSERT_OBJ(!سرا_دا_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } وإلا إذا (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            إذا (سرا_دا_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                إذا (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } وإلا {
                underp = nodep;
                under_and_اگلا = خطأ;  // IE we skip the entire foreach
            }
        } وإلا إذا (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            إذا (سرا_دا_Iter) {
                underp = loopp->stmtsp();
            } وإلا {
                underp = nodep;
                under_and_اگلا = خطأ;  // IE we skip the entire loop
            }
        } وإلا {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            أرجع خالی_سوچک;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        طالما (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        إذا (!underp->nextp() || !under_and_اگلا) {
            إذا (Ast_Jump_بلاک* ثابت blockp = VN_CAST(underp, Jump_بلاک)) أرجع blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        إذا (under_and_اگلا) {
            underp->unlink_Fr_پٹھ_With_اگلا(&repHandle);
        } وإلا {
            underp->unlink_Fr_پٹھ(&repHandle);
        }
        Ast_Jump_بلاک* ثابت blockp = جديد Ast_Jump_بلاک{nodep->fileline(), underp};
        إذا (سرا_دا_Iter) {
            nodep->user1p(blockp);
        } وإلا {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        لكل (Ast_گرہ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            إذا (VN_IS(varp, Var)) blockp->جوڑو_ایتھے_This_As_اگلا(varp->unlink_Fr_پٹھ());
        }
        أرجع blockp;
    }
    فراغ جوڑو_اگیتر_منزل_بلاکاں_Recurse(ثابت std::string& اگیتر, Ast_گرہ* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->name() != "") beginp->name(اگیتر + beginp->name());
        }

        إذا (Ast_گرہ* ثابت refp = nodep->op1p()) جوڑو_اگیتر_منزل_بلاکاں_Recurse(اگیتر, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op2p()) جوڑو_اگیتر_منزل_بلاکاں_Recurse(اگیتر, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op3p()) جوڑو_اگیتر_منزل_بلاکاں_Recurse(اگیتر, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op4p()) جوڑو_اگیتر_منزل_بلاکاں_Recurse(اگیتر, refp);
        إذا (Ast_گرہ* ثابت refp = nodep->nextp()) جوڑو_اگیتر_منزل_بلاکاں_Recurse(اگیتر, refp);
    }
    منطقي exists_بلاک_Above(ثابت std::string& name) ثابت {
        لكل (ثابت Ast_گرہ_بلاک* ثابت stackp : vlstd::الٹ_view(m_بلاک_ڈھیر)) {
            إذا (stackp->name() == name) أرجع سچ;
        }
        أرجع خطأ;
    }
    ساكن Ast_گرہ_Module* لبھو_Owner_Modulep(Ast_گرہ* nodep) {
        لكل (Ast_گرہ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            إذا (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) أرجع modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        أرجع خالی_سوچک;  // LCOV_EXCL_LINE
    }
    ساكن std::string نشانہ_Instance_رستہ(ثابت AstDisable* ثابت nodep) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->نشانہ_Refp(), VarXRef)) {
            أرجع xrefp->dotted();
        }
        أرجع "";
    }
    ساكن std::string قطار_Dotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        أرجع processQueuep->is_عارضی() ? نشانہ_Instance_رستہ(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_گرہ_بلاک* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = خالی_سوچک;
        Ast_گرہ_بلاک* prevBlockp = خالی_سوچک;
        لكل (Ast_گرہ_بلاک* ثابت blockp : vlstd::الٹ_view(m_بلاک_ڈھیر)) {
            إذا (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            إذا (blockp == targetp) أرجع innerForkBranchp;
            prevBlockp = blockp;
        }
        أرجع خالی_سوچک;
    }
    ساكن Ast_Stmt_اظہاریہ* get_قطار_ڈالو_Process_Selfp(AstVarRef* ثابت قطار_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_سطر* ثابت flp = قطار_Refp->fileline();
        أرجع جديد Ast_Stmt_اظہاریہ{
            flp,
            جديد Ast_طریقہ_Call{flp, قطار_Refp, "push_back",
                              جديد AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ساكن Ast_گرہ_Var_Ref* new_قطار_Refp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        إذا (dotted != "") أرجع جديد AstVarXRef{fl, processQueuep, dotted, access};
        إذا (!processQueuep->lifetime().isStatic() || processQueuep->is_عارضی()) {
            أرجع جديد AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت چوٹی_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        أرجع جديد AstVarRef{fl, چوٹی_Pkgp, processQueuep, access};
    }
    ساكن Ast_Stmt_اظہاریہ* get_قطار_ڈالو_Process_Selfp(File_سطر* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت قطار_Write_Refp
            = VN_AS(new_قطار_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        أرجع get_قطار_ڈالو_Process_Selfp(قطار_Write_Refp);
    }
    ساكن Ast_Stmt_اظہاریہ* get_قطار_Kill_Stmtp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_گرہ_Var_Ref* ثابت قطار_Refp
            = new_قطار_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_قطار_Call = خالی_سوچک;
        لكل (Ast_گرہ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            إذا (itemp->name() == "killQueue") {
                kill_قطار_Call
                    = جديد AstTaskRef{fl, VN_AS(itemp, Task), جديد AstArg{fl, "", قطار_Refp}};
                اكسر;
            }
        }
        UASSERT(kill_قطار_Call, "Should be found");
        kill_قطار_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        أرجع جديد Ast_Stmt_اظہاریہ{fl, kill_قطار_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_taskDisableBegins.لبھو(taskp);
        إذا (it != m_taskDisableBegins.سرا()) أرجع it->دوجا;

        AstBegin* ثابت taskBodyp = جديد AstBegin{fl, "", خالی_سوچک, خطأ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->سیٹ_Need_Process();
        v3Global.سیٹ_ورتدا_Timing();
        // Ports stay in the task so calls see them
        لكل (Ast_گرہ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_گرہ* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) taskBodyp->جوڑو_Stmtsp(stmtp->unlink_Fr_پٹھ());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جوڑو_Forksp(taskBodyp);
        taskp->جوڑو_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        أرجع taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_گرہ* ثابت nodep, File_سطر* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = جديد AstVar{
            fl, varType, m_قطار_ناں.get(nodep->name()), VFlag_بچہ_DType{},
            جديد Ast_قطار_DType{
                fl, VFlag_بچہ_DType{},
                جديد AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), خالی_سوچک},
                خالی_سوچک}};
        processQueuep->process_قطار(سچ);
        processQueuep->سیٹ_Ignore_Sched_Write();
        أرجع processQueuep;
    }
    AstVar* getProcessQueuep(Ast_گرہ* ثابت nodep, File_سطر* ثابت fl) {
        Ast_گرہ_Module* ثابت ownerp = لبھو_Owner_Modulep(nodep);

        إذا (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت چوٹی_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            چوٹی_Pkgp->جوڑو_Stmtsp(processQueuep);
            أرجع processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->جوڑو_Stmtsp(processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_taskDisableQueues.لبھو(taskp);
        إذا (it != m_taskDisableQueues.سرا()) أرجع it->دوجا;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_اظہاریہ* ثابت ڈالو_Current_Processp = get_قطار_ڈالو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ڈالو_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        أرجع processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_beginDisableBegins.لبھو(beginp);
        إذا (it != m_beginDisableBegins.سرا()) أرجع it->دوجا;

        AstBegin* ثابت beginBodyp = جديد AstBegin{fl, "", خالی_سوچک, خطأ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->سیٹ_Need_Process();
        v3Global.سیٹ_ورتدا_Timing();
        إذا (beginp->stmtsp()) beginBodyp->جوڑو_Stmtsp(beginp->stmtsp()->unlink_Fr_پٹھ_With_اگلا());

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جوڑو_Forksp(beginBodyp);
        beginp->جوڑو_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        أرجع beginBodyp;
    }
    فراغ prepend_Fork_Branch_قطار_Pushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_سطر* ثابت fl, منطقي needProcess) {
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            إذا (needProcess) branchp->سیٹ_Need_Process();
            branchp->prependStmtsp(get_قطار_ڈالو_Process_Selfp(fl, processQueuep));
        }
    }
    فراغ prepend_Nested_Fork_Branch_قطار_Pushes(Ast_گرہ* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_سطر* ثابت fl, منطقي needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prepend_Fork_Branch_قطار_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_beginDisableQueues.لبھو(beginp);
        إذا (it != m_beginDisableQueues.سرا()) أرجع it->دوجا;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_اظہاریہ* ثابت ڈالو_Current_Processp = get_قطار_ڈالو_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ڈالو_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_قطار_Pushes(beginBodyp, processQueuep, fl, خطأ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_forkDisableQueues.لبھو(forkp);
        إذا (it != m_forkDisableQueues.سرا()) أرجع it->دوجا;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_قطار_Pushes(forkp, processQueuep, fl, سچ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_قطار_Pushes(branchp, processQueuep, fl, سچ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        أرجع processQueuep;
    }
    Ast_Stmt_اظہاریہ* داخل_کرو_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_Stmt_اظہاریہ* ثابت killStmtp
            = get_قطار_Kill_Stmtp(fl, processQueuep, قطار_Dotted(nodep, processQueuep));
        nodep->جوڑو_اگلا_ایتھے(killStmtp);
        أرجع killStmtp;
    }
    فراغ جوڑو_Jump_After_Kill(Ast_Stmt_اظہاریہ* ثابت killStmtp, Ast_گرہ* ثابت targetp) {
        Ast_Jump_بلاک* ثابت jmpBlockp = get_Jump_بلاک(targetp, خطأ);
        killStmtp->جوڑو_اگلا_ایتھے(جديد AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    فراغ handleDisableOnFork(AstDisable* ثابت nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_سطر* ثابت fl = nodep->fileline();
        AstFork* ثابت targetp = VN_AS(nodep->targetp(), Fork);
        إذا (m_ftaskp) {
            إذا (!m_ftaskp->exists(
                    [targetp](ثابت Ast_گرہ_بلاک* blockp) -> منطقي { أرجع blockp == targetp; })) {
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
        إذا (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            جوڑو_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_عدد);
        m_modp = nodep;
        m_mod_Repeat_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_بھریا);
        m_بلاک_ڈھیر.ڈالو_پٹھ(nodep);
        iterateChildren(nodep);
        m_بلاک_ڈھیر.نکالو_پٹھ();
    }
    فراغ ویکھو(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_بھریا);
        VL_RESTORER(m_وچ_Fork);
        m_وچ_Fork = سچ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        لكل (Ast_گرہ_بلاک* ثابت blockp : vlstd::الٹ_view(m_بلاک_ڈھیر)) {
            إذا (blockp->user3_سیٹ_Once()) اكسر;
        }
        m_بلاک_ڈھیر.ڈالو_پٹھ(nodep);
        iterateChildren(nodep);
        m_بلاک_ڈھیر.نکالو_پٹھ();
    }
    فراغ ویکھو(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_بھریا = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
        } وإلا إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_بھریا) {
            m_unroll_بھریا = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ ویکھو(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_گرہ_اظہاریہ* ثابت countp = nodep->countp()->unlink_Fr_پٹھ_With_اگلا();
        ثابت string name = "__Vrepeat"s + cvt_منزل_Str(m_mod_Repeat_عدد++);
        AstBegin* ثابت beginp = جديد AstBegin{nodep->fileline(), "", خالی_سوچک, سچ};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = جديد AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->لبھو_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->ورتیا_Loop_Idx(سچ);
        beginp->جوڑو_Stmtsp(varp);
        Ast_گرہ* initsp = جديد Ast_سونپو{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_گرہ* ثابت decp = جديد Ast_سونپو{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            جديد AstSub{nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       جديد AstConst{nodep->fileline(), 1}}};
        Ast_گرہ_اظہاریہ* ثابت zerosp = جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_گرہ_اظہاریہ* ثابت condp = جديد AstGtS{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_گرہ* ثابت bodysp = nodep->stmtsp();
        إذا (bodysp) bodysp->unlink_Fr_پٹھ_With_اگلا();
        File_سطر* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
        loopp->جوڑو_Stmtsp(bodysp);
        loopp->جوڑو_Contsp(decp);
        إذا (!m_unroll_بھریا.isDefault()) loopp->unroll(m_unroll_بھریا);
        m_unroll_بھریا = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->جوڑو_Stmtsp(initsp);
        beginp->جوڑو_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstLoop* nodep) override {
        إذا (!m_unroll_بھریا.isDefault()) nodep->unroll(m_unroll_بھریا);
        إذا (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDLOOP, سچ);
        }
        m_unroll_بھریا = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = خطأ;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_loopInc = سچ;
        iterate_And_اگلا_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        إذا (nodep->contsp()) nodep->جوڑو_Stmtsp(nodep->contsp()->unlink_Fr_پٹھ_With_اگلا());
    }
    فراغ ویکھو(Ast_گرہ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_اگلا_Null(nodep->bodyp());
    }
    فراغ ویکھو(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        إذا (m_randsequencep) {
            nodep->replaceWith(جديد AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_وچ_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
            أرجع;
        } وإلا إذا (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } وإلا إذا (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } وإلا إذا (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } وإلا {
            إذا (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->جوڑو_ایتھے_This_As_اگلا(جديد Ast_سونپو{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_پٹھ_With_اگلا()});
            }
            // Jump to the end of the function call
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_ftaskp, خطأ);
            nodep->جوڑو_ایتھے_This_As_اگلا(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstBreak* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp && m_randsequencep) {
            nodep->replaceWith(جديد AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } وإلا {
            // Jump to the end of the loop
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, خطأ);
            nodep->جوڑو_اگلا_ایتھے(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstContinue* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } وإلا {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, سچ);
            nodep->جوڑو_اگلا_ایتھے(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_گرہ* ثابت targetp = nodep->targetp();
        إذا (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
            أرجع;
        }
        إذا (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_اظہاریہ* ثابت killStmtp = داخل_کرو_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            إذا (m_ftaskp == taskp) {
                Ast_گرہ* jumpTargetp = taskp;
                ثابت خودکار it = m_taskDisableBegins.لبھو(taskp);
                إذا (it != m_taskDisableBegins.سرا()) jumpTargetp = it->دوجا;
                جوڑو_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } وإلا إذا (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            إذا (exists_بلاک_Above(beginp->name())) {
                إذا (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(beginp, خطأ);
                    nodep->جوڑو_اگلا_ایتھے(جديد AstJumpGo{nodep->fileline(), blockp});
                } وإلا {
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
            } وإلا {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                داخل_کرو_Kill_Stmtp(nodep, processQueuep);
            }
        } وإلا {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(Ast_انت* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (m_وچ_Fork) {
            nodep->replaceWith(جديد Ast_انت_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_بلاک* ثابت blockp = get_Jump_بلاک(m_loopp, خطأ);
            nodep->جوڑو_اگلا_ایتھے(جديد AstJumpGo{nodep->fileline(), blockp});
        }
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (m_loopInc && nodep->varp()) nodep->varp()->ورتیا_Loop_Idx(سچ);
    }
    فراغ ویکھو(AstConst*) override {}
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkjump", 0, dump_درخت_Either_Level() >= 3);
}
