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
    ثابت VNUser1_درون_Use m_user1_درون_Use;
    ثابت VNUser2_درون_Use m_user2_درون_Use;
    ثابت VNUser3_درون_Use m_user3_درون_Use;

    // STATE
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Current function/task
    Ast_گره* m_loopp = اشاره_تهی;  // Current loop
    AstRandSequence* m_randsequencep = اشاره_تهی;  // Current randsequence
    منطقي m_loopInc = خطأ;  // In loop increment
    منطقي m_درون_Fork = خطأ;  // Under fork
    صحيح m_mod_Repeat_رقم = 0;  // Repeat counter
    VOptionBool m_unroll_پر;  // Pragma full, disable, or default unrolling
    std::وکتور<Ast_گره_بلوک*> m_بلوک_پشته;  // All begin blocks above current node
    V3_Unique_نامها m_صف_نامها{
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
    Ast_Jump_بلوک* get_Jump_بلوک(Ast_گره* nodep, منطقي انتها_از_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        إذا (انتها_از_Iter) {
            إذا (nodep->user1p()) أرجع VN_AS(nodep->user1p(), Jump_بلوک);
        } وإلا {
            إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), Jump_بلوک);
        }

        Ast_گره* underp = اشاره_تهی;
        منطقي under_and_بعدی = درست;
        إذا (AstBegin* ثابت blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!انتها_از_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } وإلا إذا (Ast_گره_FTask* ثابت fTaskp = VN_CAST(nodep, گره_FTask)) {
            UASSERT_OBJ(!انتها_از_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } وإلا إذا (AstForeach* ثابت foreachp = VN_CAST(nodep, Foreach)) {
            إذا (انتها_از_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                إذا (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } وإلا {
                underp = nodep;
                under_and_بعدی = خطأ;  // IE we skip the entire foreach
            }
        } وإلا إذا (AstLoop* ثابت loopp = VN_CAST(nodep, Loop)) {
            إذا (انتها_از_Iter) {
                underp = loopp->stmtsp();
            } وإلا {
                underp = nodep;
                under_and_بعدی = خطأ;  // IE we skip the entire loop
            }
        } وإلا {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            أرجع اشاره_تهی;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        طالما (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        إذا (!underp->nextp() || !under_and_بعدی) {
            إذا (Ast_Jump_بلوک* ثابت blockp = VN_CAST(underp, Jump_بلوک)) أرجع blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        إذا (under_and_بعدی) {
            underp->unlink_Fr_پشت_With_بعدی(&repHandle);
        } وإلا {
            underp->unlink_Fr_پشت(&repHandle);
        }
        Ast_Jump_بلوک* ثابت blockp = جديد Ast_Jump_بلوک{nodep->fileline(), underp};
        إذا (انتها_از_Iter) {
            nodep->user1p(blockp);
        } وإلا {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        لكل (Ast_گره *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            إذا (VN_IS(varp, Var)) blockp->جمع_کن_اینجا_This_As_بعدی(varp->unlink_Fr_پشت());
        }
        أرجع blockp;
    }
    فراغ جمع_کن_پیشوند_مقصد_بلوکها_Recurse(ثابت std::string& پیشوند, Ast_گره* ثابت nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->name() != "") beginp->name(پیشوند + beginp->name());
        }

        إذا (Ast_گره* ثابت refp = nodep->op1p()) جمع_کن_پیشوند_مقصد_بلوکها_Recurse(پیشوند, refp);
        إذا (Ast_گره* ثابت refp = nodep->op2p()) جمع_کن_پیشوند_مقصد_بلوکها_Recurse(پیشوند, refp);
        إذا (Ast_گره* ثابت refp = nodep->op3p()) جمع_کن_پیشوند_مقصد_بلوکها_Recurse(پیشوند, refp);
        إذا (Ast_گره* ثابت refp = nodep->op4p()) جمع_کن_پیشوند_مقصد_بلوکها_Recurse(پیشوند, refp);
        إذا (Ast_گره* ثابت refp = nodep->nextp()) جمع_کن_پیشوند_مقصد_بلوکها_Recurse(پیشوند, refp);
    }
    منطقي exists_بلوک_Above(ثابت std::string& name) ثابت {
        لكل (ثابت Ast_گره_بلوک* ثابت stackp : vlstd::معکوس_view(m_بلوک_پشته)) {
            إذا (stackp->name() == name) أرجع درست;
        }
        أرجع خطأ;
    }
    ساكن Ast_گره_Module* پیدا_کن_Owner_Modulep(Ast_گره* nodep) {
        لكل (Ast_گره* curp = nodep; curp; curp = curp->aboveLoopp()) {
            إذا (Ast_گره_Module* ثابت modp = VN_CAST(curp, گره_Module)) أرجع modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        أرجع اشاره_تهی;  // LCOV_EXCL_LINE
    }
    ساكن std::string هدف_Instance_مسیر(ثابت AstDisable* ثابت nodep) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(nodep->هدف_Refp(), VarXRef)) {
            أرجع xrefp->dotted();
        }
        أرجع "";
    }
    ساكن std::string صف_Dotted(ثابت AstDisable* ثابت nodep,
                                   ثابت AstVar* ثابت processQueuep) {
        أرجع processQueuep->is_موقت() ? هدف_Instance_مسیر(nodep) : "";
    }
    AstBegin* innerForkBranchp(ثابت Ast_گره_بلوک* ثابت targetp) ثابت {
        AstBegin* innerForkBranchp = اشاره_تهی;
        Ast_گره_بلوک* prevBlockp = اشاره_تهی;
        لكل (Ast_گره_بلوک* ثابت blockp : vlstd::معکوس_view(m_بلوک_پشته)) {
            إذا (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            إذا (blockp == targetp) أرجع innerForkBranchp;
            prevBlockp = blockp;
        }
        أرجع اشاره_تهی;
    }
    ساكن Ast_Stmt_عبارت* get_صف_افزودن_Process_Selfp(AstVarRef* ثابت صف_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_سطر* ثابت flp = صف_Refp->fileline();
        أرجع جديد Ast_Stmt_عبارت{
            flp,
            جديد Ast_روش_Call{flp, صف_Refp, "push_back",
                              جديد AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ساكن Ast_گره_Var_Ref* new_صف_Refp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                       ثابت VAccess& access, ثابت std::string& dotted = "") {
        إذا (dotted != "") أرجع جديد AstVarXRef{fl, processQueuep, dotted, access};
        إذا (!processQueuep->lifetime().isStatic() || processQueuep->is_موقت()) {
            أرجع جديد AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ثابت بالاترین_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        أرجع جديد AstVarRef{fl, بالاترین_Pkgp, processQueuep, access};
    }
    ساكن Ast_Stmt_عبارت* get_صف_افزودن_Process_Selfp(File_سطر* ثابت fl, AstVar* ثابت processQueuep) {
        AstVarRef* ثابت صف_Write_Refp
            = VN_AS(new_صف_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        أرجع get_صف_افزودن_Process_Selfp(صف_Write_Refp);
    }
    ساكن Ast_Stmt_عبارت* get_صف_Kill_Stmtp(File_سطر* ثابت fl, AstVar* ثابت processQueuep,
                                          ثابت std::string& dotted = "") {
        Ast_گره_Var_Ref* ثابت صف_Refp
            = new_صف_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_صف_Call = اشاره_تهی;
        لكل (Ast_گره* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            إذا (itemp->name() == "killQueue") {
                kill_صف_Call
                    = جديد AstTaskRef{fl, VN_AS(itemp, Task), جديد AstArg{fl, "", صف_Refp}};
                اكسر;
            }
        }
        UASSERT(kill_صف_Call, "Should be found");
        kill_صف_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        أرجع جديد Ast_Stmt_عبارت{fl, kill_صف_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_taskDisableBegins.پیدا_کن(taskp);
        إذا (it != m_taskDisableBegins.انتها()) أرجع it->دومی;

        AstBegin* ثابت taskBodyp = جديد AstBegin{fl, "", اشاره_تهی, خطأ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->مجموعه_Need_Process();
        v3Global.مجموعه_استفاده_میکند_Timing();
        // Ports stay in the task so calls see them
        لكل (Ast_گره* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_گره* ثابت nextp = stmtp->nextp();
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) taskBodyp->جمع_کن_Stmtsp(stmtp->unlink_Fr_پشت());
            stmtp = nextp;
        }

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جمع_کن_Forksp(taskBodyp);
        taskp->جمع_کن_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        أرجع taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_گره* ثابت nodep, File_سطر* ثابت fl, ثابت VVarType& varType) {
        AstVar* ثابت processQueuep = جديد AstVar{
            fl, varType, m_صف_نامها.get(nodep->name()), VFlag_فرزند_DType{},
            جديد Ast_صف_DType{
                fl, VFlag_فرزند_DType{},
                جديد AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), اشاره_تهی},
                اشاره_تهی}};
        processQueuep->process_صف(درست);
        processQueuep->مجموعه_Ignore_Sched_Write();
        أرجع processQueuep;
    }
    AstVar* getProcessQueuep(Ast_گره* ثابت nodep, File_سطر* ثابت fl) {
        Ast_گره_Module* ثابت ownerp = پیدا_کن_Owner_Modulep(nodep);

        إذا (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ثابت بالاترین_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            بالاترین_Pkgp->جمع_کن_Stmtsp(processQueuep);
            أرجع processQueuep;
        }

        AstVar* ثابت processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->جمع_کن_Stmtsp(processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ثابت taskp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_taskDisableQueues.پیدا_کن(taskp);
        إذا (it != m_taskDisableQueues.انتها()) أرجع it->دومی;

        AstVar* ثابت processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_عبارت* ثابت افزودن_Current_Processp = get_صف_افزودن_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(افزودن_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        أرجع processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_beginDisableBegins.پیدا_کن(beginp);
        إذا (it != m_beginDisableBegins.انتها()) أرجع it->دومی;

        AstBegin* ثابت beginBodyp = جديد AstBegin{fl, "", اشاره_تهی, خطأ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->مجموعه_Need_Process();
        v3Global.مجموعه_استفاده_میکند_Timing();
        إذا (beginp->stmtsp()) beginBodyp->جمع_کن_Stmtsp(beginp->stmtsp()->unlink_Fr_پشت_With_بعدی());

        AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN};
        forkp->جمع_کن_Forksp(beginBodyp);
        beginp->جمع_کن_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        أرجع beginBodyp;
    }
    فراغ prepend_Fork_Branch_صف_Pushes(AstFork* ثابت forkp, AstVar* ثابت processQueuep,
                                      File_سطر* ثابت fl, منطقي needProcess) {
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            إذا (needProcess) branchp->مجموعه_Need_Process();
            branchp->prependStmtsp(get_صف_افزودن_Process_Selfp(fl, processQueuep));
        }
    }
    فراغ prepend_Nested_Fork_Branch_صف_Pushes(Ast_گره* ثابت nodep, AstVar* ثابت processQueuep,
                                            File_سطر* ثابت fl, منطقي needProcess) {
        nodep->foreach([&](AstFork* ثابت forkp) {
            prepend_Fork_Branch_صف_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ثابت beginp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_beginDisableQueues.پیدا_کن(beginp);
        إذا (it != m_beginDisableQueues.انتها()) أرجع it->دومی;

        AstVar* ثابت processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_عبارت* ثابت افزودن_Current_Processp = get_صف_افزودن_Process_Selfp(fl, processQueuep);
        AstBegin* ثابت beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(افزودن_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_صف_Pushes(beginBodyp, processQueuep, fl, خطأ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        أرجع processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ثابت forkp, File_سطر* ثابت fl) {
        ثابت خودکار it = m_forkDisableQueues.پیدا_کن(forkp);
        إذا (it != m_forkDisableQueues.انتها()) أرجع it->دومی;

        AstVar* ثابت processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_صف_Pushes(forkp, processQueuep, fl, درست);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        لكل (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_صف_Pushes(branchp, processQueuep, fl, درست);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        أرجع processQueuep;
    }
    Ast_Stmt_عبارت* درج_Kill_Stmtp(AstDisable* ثابت nodep, AstVar* ثابت processQueuep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_Stmt_عبارت* ثابت killStmtp
            = get_صف_Kill_Stmtp(fl, processQueuep, صف_Dotted(nodep, processQueuep));
        nodep->جمع_کن_بعدی_اینجا(killStmtp);
        أرجع killStmtp;
    }
    فراغ جمع_کن_Jump_After_Kill(Ast_Stmt_عبارت* ثابت killStmtp, Ast_گره* ثابت targetp) {
        Ast_Jump_بلوک* ثابت jmpBlockp = get_Jump_بلوک(targetp, خطأ);
        killStmtp->جمع_کن_بعدی_اینجا(جديد AstJumpGo{killStmtp->fileline(), jmpBlockp});
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
                    [targetp](ثابت Ast_گره_بلوک* blockp) -> منطقي { أرجع blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ثابت processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_عبارت* ثابت killStmtp = درج_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        إذا (AstBegin* ثابت branchp = innerForkBranchp(targetp)) {
            جمع_کن_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_رقم);
        m_modp = nodep;
        m_mod_Repeat_رقم = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_پر);
        m_بلوک_پشته.افزودن_پشت(nodep);
        iterateChildren(nodep);
        m_بلوک_پشته.برداشت_پشت();
    }
    فراغ بازدید(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_پر);
        VL_RESTORER(m_درون_Fork);
        m_درون_Fork = درست;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        لكل (Ast_گره_بلوک* ثابت blockp : vlstd::معکوس_view(m_بلوک_پشته)) {
            إذا (blockp->user3_مجموعه_Once()) اكسر;
        }
        m_بلوک_پشته.افزودن_پشت(nodep);
        iterateChildren(nodep);
        m_بلوک_پشته.برداشت_پشت();
    }
    فراغ بازدید(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_پر = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
        } وإلا إذا (nodep->pragp()->pragType() == VPragmaType::UNROLL_پر) {
            m_unroll_پر = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ بازدید(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_گره_عبارت* ثابت countp = nodep->countp()->unlink_Fr_پشت_With_بعدی();
        ثابت string name = "__Vrepeat"s + cvt_مقصد_Str(m_mod_Repeat_رقم++);
        AstBegin* ثابت beginp = جديد AstBegin{nodep->fileline(), "", اشاره_تهی, درست};
        // Spec says value is integral, if negative is ignored
        AstVar* ثابت varp
            = جديد AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->پیدا_کن_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->استفاده_شده_Loop_Idx(درست);
        beginp->جمع_کن_Stmtsp(varp);
        Ast_گره* initsp = جديد Ast_تخصیص_بده{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_گره* ثابت decp = جديد Ast_تخصیص_بده{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            جديد AstSub{nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       جديد AstConst{nodep->fileline(), 1}}};
        Ast_گره_عبارت* ثابت zerosp = جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_گره_عبارت* ثابت condp = جديد AstGtS{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_گره* ثابت bodysp = nodep->stmtsp();
        إذا (bodysp) bodysp->unlink_Fr_پشت_With_بعدی();
        File_سطر* ثابت flp = nodep->fileline();
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
        loopp->جمع_کن_Stmtsp(bodysp);
        loopp->جمع_کن_Contsp(decp);
        إذا (!m_unroll_پر.isDefault()) loopp->unroll(m_unroll_پر);
        m_unroll_پر = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->جمع_کن_Stmtsp(initsp);
        beginp->جمع_کن_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstLoop* nodep) override {
        إذا (!m_unroll_پر.isDefault()) nodep->unroll(m_unroll_پر);
        إذا (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_کد::UNUSEDLOOP, درست);
        }
        m_unroll_پر = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = خطأ;
        iterate_And_بعدی_Null(nodep->stmtsp());
        m_loopInc = درست;
        iterate_And_بعدی_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        إذا (nodep->contsp()) nodep->جمع_کن_Stmtsp(nodep->contsp()->unlink_Fr_پشت_With_بعدی());
    }
    فراغ بازدید(Ast_گره_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_بعدی_Null(nodep->bodyp());
    }
    فراغ بازدید(AstReturn* nodep) override {
        iterateChildren(nodep);
        ثابت AstFunc* ثابت funcp = VN_CAST(m_ftaskp, Func);
        إذا (m_randsequencep) {
            nodep->replaceWith(جديد AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (m_درون_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
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
                nodep->جمع_کن_اینجا_This_As_بعدی(جديد Ast_تخصیص_بده{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_پشت_With_بعدی()});
            }
            // Jump to the end of the function call
            Ast_Jump_بلوک* ثابت blockp = get_Jump_بلوک(m_ftaskp, خطأ);
            nodep->جمع_کن_اینجا_This_As_بعدی(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstBreak* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp && m_randsequencep) {
            nodep->replaceWith(جديد AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;
        } وإلا إذا (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } وإلا {
            // Jump to the end of the loop
            Ast_Jump_بلوک* ثابت blockp = get_Jump_بلوک(m_loopp, خطأ);
            nodep->جمع_کن_بعدی_اینجا(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstContinue* nodep) override {
        iterateChildren(nodep);
        إذا (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } وإلا {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_بلوک* ثابت blockp = get_Jump_بلوک(m_loopp, درست);
            nodep->جمع_کن_بعدی_اینجا(جديد AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_گره* ثابت targetp = nodep->targetp();
        إذا (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        إذا (AstTask* ثابت taskp = VN_CAST(targetp, Task)) {
            AstVar* ثابت processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_عبارت* ثابت killStmtp = درج_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            إذا (m_ftaskp == taskp) {
                Ast_گره* jumpTargetp = taskp;
                ثابت خودکار it = m_taskDisableBegins.پیدا_کن(taskp);
                إذا (it != m_taskDisableBegins.انتها()) jumpTargetp = it->دومی;
                جمع_کن_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } وإلا إذا (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(targetp, Begin)) {
            إذا (exists_بلوک_Above(beginp->name())) {
                إذا (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_بلوک* ثابت blockp = get_Jump_بلوک(beginp, خطأ);
                    nodep->جمع_کن_بعدی_اینجا(جديد AstJumpGo{nodep->fileline(), blockp});
                } وإلا {
                    AstVar* ثابت processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_عبارت* ثابت killStmtp = درج_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ثابت branchp = innerForkBranchp(beginp);
                    AstBegin* ثابت jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    جمع_کن_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } وإلا {
                AstVar* ثابت processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                درج_Kill_Stmtp(nodep, processQueuep);
            }
        } وإلا {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(Ast_پایان* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (m_درون_Fork) {
            nodep->replaceWith(جديد Ast_پایان_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_بلوک* ثابت blockp = get_Jump_بلوک(m_loopp, خطأ);
            nodep->جمع_کن_بعدی_اینجا(جديد AstJumpGo{nodep->fileline(), blockp});
        }
    }
    فراغ بازدید(AstVarRef* nodep) override {
        إذا (m_loopInc && nodep->varp()) nodep->varp()->استفاده_شده_Loop_Idx(درست);
    }
    فراغ بازدید(AstConst*) override {}
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkjump", 0, dump_درخت_Either_Level() >= 3);
}
