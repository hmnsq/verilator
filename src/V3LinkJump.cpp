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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkJump.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ LinkJumpVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_user3_ਵਿੱਚ_Use;

    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਗੰਢ* m_loopp = ਖਾਲੀ_ਸੂਚਕ;  // Current loop
    AstRandSequence* m_randsequencep = ਖਾਲੀ_ਸੂਚਕ;  // Current randsequence
    ਤਾਰਕਿਕ m_loopInc = ਝੂਠ;  // In loop increment
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Fork = ਝੂਠ;  // Under fork
    ਪੂਰਨ m_mod_Repeat_ਅੰਕ = 0;  // Repeat counter
    VOptionBool m_unroll_ਭਰਿਆ;  // Pragma full, disable, or default unrolling
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਖੰਡ*> m_ਖੰਡ_ਢੇਰ;  // All begin blocks above current node
    V3_Unique_ਨਾਂ m_ਕਤਾਰ_ਨਾਂ{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ਅਚਲ AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ਅਚਲ AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ਅਚਲ AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ਅਚਲ AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ਅਚਲ AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    Ast_Jump_ਖੰਡ* get_Jump_ਖੰਡ(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ ਸਿਰਾ_ਦਾ_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        ਜੇ (ਸਿਰਾ_ਦਾ_Iter) {
            ਜੇ (nodep->user1p()) ਵਾਪਸ VN_AS(nodep->user1p(), Jump_ਖੰਡ);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->user2p()) ਵਾਪਸ VN_AS(nodep->user2p(), Jump_ਖੰਡ);
        }

        Ast_ਗੰਢ* underp = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ under_and_ਅਗਲਾ = ਸੱਚ;
        ਜੇ (AstBegin* ਅਚਲ blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!ਸਿਰਾ_ਦਾ_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ fTaskp = VN_CAST(nodep, ਗੰਢ_FTask)) {
            UASSERT_OBJ(!ਸਿਰਾ_ਦਾ_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } ਨਹੀਂਤਾਂ ਜੇ (AstForeach* ਅਚਲ foreachp = VN_CAST(nodep, Foreach)) {
            ਜੇ (ਸਿਰਾ_ਦਾ_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                ਜੇ (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } ਨਹੀਂਤਾਂ {
                underp = nodep;
                under_and_ਅਗਲਾ = ਝੂਠ;  // IE we skip the entire foreach
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstLoop* ਅਚਲ loopp = VN_CAST(nodep, Loop)) {
            ਜੇ (ਸਿਰਾ_ਦਾ_Iter) {
                underp = loopp->stmtsp();
            } ਨਹੀਂਤਾਂ {
                underp = nodep;
                under_and_ਅਗਲਾ = ਝੂਠ;  // IE we skip the entire loop
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        ਜਦੋਂ (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        ਜੇ (!underp->nextp() || !under_and_ਅਗਲਾ) {
            ਜੇ (Ast_Jump_ਖੰਡ* ਅਚਲ blockp = VN_CAST(underp, Jump_ਖੰਡ)) ਵਾਪਸ blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        ਜੇ (under_and_ਅਗਲਾ) {
            underp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(&repHandle);
        } ਨਹੀਂਤਾਂ {
            underp->unlink_Fr_ਪਿੱਠ(&repHandle);
        }
        Ast_Jump_ਖੰਡ* ਅਚਲ blockp = ਨਵਾਂ Ast_Jump_ਖੰਡ{nodep->fileline(), underp};
        ਜੇ (ਸਿਰਾ_ਦਾ_Iter) {
            nodep->user1p(blockp);
        } ਨਹੀਂਤਾਂ {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        ਲਈ (Ast_ਗੰਢ *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            ਜੇ (VN_IS(varp, Var)) blockp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(varp->unlink_Fr_ਪਿੱਠ());
        }
        ਵਾਪਸ blockp;
    }
    ਖਾਲੀ ਜੋੜੋ_ਅਗੇਤਰ_ਮੰਜ਼ਿਲ_ਖੰਡਾਂ_Recurse(ਅਚਲ std::string& ਅਗੇਤਰ, Ast_ਗੰਢ* ਅਚਲ nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(nodep, Begin)) {
            ਜੇ (beginp->name() != "") beginp->name(ਅਗੇਤਰ + beginp->name());
        }

        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op1p()) ਜੋੜੋ_ਅਗੇਤਰ_ਮੰਜ਼ਿਲ_ਖੰਡਾਂ_Recurse(ਅਗੇਤਰ, refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op2p()) ਜੋੜੋ_ਅਗੇਤਰ_ਮੰਜ਼ਿਲ_ਖੰਡਾਂ_Recurse(ਅਗੇਤਰ, refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op3p()) ਜੋੜੋ_ਅਗੇਤਰ_ਮੰਜ਼ਿਲ_ਖੰਡਾਂ_Recurse(ਅਗੇਤਰ, refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op4p()) ਜੋੜੋ_ਅਗੇਤਰ_ਮੰਜ਼ਿਲ_ਖੰਡਾਂ_Recurse(ਅਗੇਤਰ, refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->nextp()) ਜੋੜੋ_ਅਗੇਤਰ_ਮੰਜ਼ਿਲ_ਖੰਡਾਂ_Recurse(ਅਗੇਤਰ, refp);
    }
    ਤਾਰਕਿਕ exists_ਖੰਡ_Above(ਅਚਲ std::string& name) ਅਚਲ {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਖੰਡ* ਅਚਲ stackp : vlstd::ਉਲਟ_view(m_ਖੰਡ_ਢੇਰ)) {
            ਜੇ (stackp->name() == name) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਸਥਿਰ Ast_ਗੰਢ_Module* ਲੱਭੋ_Owner_Modulep(Ast_ਗੰਢ* nodep) {
        ਲਈ (Ast_ਗੰਢ* curp = nodep; curp; curp = curp->aboveLoopp()) {
            ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(curp, ਗੰਢ_Module)) ਵਾਪਸ modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // LCOV_EXCL_LINE
    }
    ਸਥਿਰ std::string ਨਿਸ਼ਾਨਾ_Instance_ਰਸਤਾ(ਅਚਲ AstDisable* ਅਚਲ nodep) {
        ਜੇ (ਅਚਲ AstVarXRef* ਅਚਲ xrefp = VN_CAST(nodep->ਨਿਸ਼ਾਨਾ_Refp(), VarXRef)) {
            ਵਾਪਸ xrefp->dotted();
        }
        ਵਾਪਸ "";
    }
    ਸਥਿਰ std::string ਕਤਾਰ_Dotted(ਅਚਲ AstDisable* ਅਚਲ nodep,
                                   ਅਚਲ AstVar* ਅਚਲ processQueuep) {
        ਵਾਪਸ processQueuep->is_ਆਰਜ਼ੀ() ? ਨਿਸ਼ਾਨਾ_Instance_ਰਸਤਾ(nodep) : "";
    }
    AstBegin* innerForkBranchp(ਅਚਲ Ast_ਗੰਢ_ਖੰਡ* ਅਚਲ targetp) ਅਚਲ {
        AstBegin* innerForkBranchp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਖੰਡ* prevBlockp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ_ਖੰਡ* ਅਚਲ blockp : vlstd::ਉਲਟ_view(m_ਖੰਡ_ਢੇਰ)) {
            ਜੇ (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            ਜੇ (blockp == targetp) ਵਾਪਸ innerForkBranchp;
            prevBlockp = blockp;
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    ਸਥਿਰ Ast_Stmt_ਵਿਅੰਜਕ* get_ਕਤਾਰ_ਧੱਕੋ_Process_Selfp(AstVarRef* ਅਚਲ ਕਤਾਰ_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_ਸਤਰ* ਅਚਲ flp = ਕਤਾਰ_Refp->fileline();
        ਵਾਪਸ ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{
            flp,
            ਨਵਾਂ Ast_ਵਿਧੀ_Call{flp, ਕਤਾਰ_Refp, "push_back",
                              ਨਵਾਂ AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ਸਥਿਰ Ast_ਗੰਢ_Var_Ref* new_ਕਤਾਰ_Refp(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ processQueuep,
                                       ਅਚਲ VAccess& access, ਅਚਲ std::string& dotted = "") {
        ਜੇ (dotted != "") ਵਾਪਸ ਨਵਾਂ AstVarXRef{fl, processQueuep, dotted, access};
        ਜੇ (!processQueuep->lifetime().isStatic() || processQueuep->is_ਆਰਜ਼ੀ()) {
            ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ਅਚਲ ਸਿਖਰ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, ਸਿਖਰ_Pkgp, processQueuep, access};
    }
    ਸਥਿਰ Ast_Stmt_ਵਿਅੰਜਕ* get_ਕਤਾਰ_ਧੱਕੋ_Process_Selfp(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ processQueuep) {
        AstVarRef* ਅਚਲ ਕਤਾਰ_Write_Refp
            = VN_AS(new_ਕਤਾਰ_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        ਵਾਪਸ get_ਕਤਾਰ_ਧੱਕੋ_Process_Selfp(ਕਤਾਰ_Write_Refp);
    }
    ਸਥਿਰ Ast_Stmt_ਵਿਅੰਜਕ* get_ਕਤਾਰ_Kill_Stmtp(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ processQueuep,
                                          ਅਚਲ std::string& dotted = "") {
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ ਕਤਾਰ_Refp
            = new_ਕਤਾਰ_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_ਕਤਾਰ_Call = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            ਜੇ (itemp->name() == "killQueue") {
                kill_ਕਤਾਰ_Call
                    = ਨਵਾਂ AstTaskRef{fl, VN_AS(itemp, Task), ਨਵਾਂ AstArg{fl, "", ਕਤਾਰ_Refp}};
                ਰੋਕੋ;
            }
        }
        UASSERT(kill_ਕਤਾਰ_Call, "Should be found");
        kill_ਕਤਾਰ_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        ਵਾਪਸ ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{fl, kill_ਕਤਾਰ_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ਅਚਲ taskp, File_ਸਤਰ* ਅਚਲ fl) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_taskDisableBegins.ਲੱਭੋ(taskp);
        ਜੇ (it != m_taskDisableBegins.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        AstBegin* ਅਚਲ taskBodyp = ਨਵਾਂ AstBegin{fl, "", ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->ਸੈੱਟ_Need_Process();
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
        // Ports stay in the task so calls see them
        ਲਈ (Ast_ਗੰਢ* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ਗੰਢ* ਅਚਲ nextp = stmtp->nextp();
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var);
            ਜੇ (!varp || !varp->isIO()) taskBodyp->ਜੋੜੋ_Stmtsp(stmtp->unlink_Fr_ਪਿੱਠ());
            stmtp = nextp;
        }

        AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{fl, VJoinType::JOIN};
        forkp->ਜੋੜੋ_Forksp(taskBodyp);
        taskp->ਜੋੜੋ_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        ਵਾਪਸ taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ਗੰਢ* ਅਚਲ nodep, File_ਸਤਰ* ਅਚਲ fl, ਅਚਲ VVarType& varType) {
        AstVar* ਅਚਲ processQueuep = ਨਵਾਂ AstVar{
            fl, varType, m_ਕਤਾਰ_ਨਾਂ.get(nodep->name()), VFlag_ਬੱਚਾ_DType{},
            ਨਵਾਂ Ast_ਕਤਾਰ_DType{
                fl, VFlag_ਬੱਚਾ_DType{},
                ਨਵਾਂ AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), ਖਾਲੀ_ਸੂਚਕ},
                ਖਾਲੀ_ਸੂਚਕ}};
        processQueuep->process_ਕਤਾਰ(ਸੱਚ);
        processQueuep->ਸੈੱਟ_Ignore_Sched_Write();
        ਵਾਪਸ processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ਗੰਢ* ਅਚਲ nodep, File_ਸਤਰ* ਅਚਲ fl) {
        Ast_ਗੰਢ_Module* ਅਚਲ ownerp = ਲੱਭੋ_Owner_Modulep(nodep);

        ਜੇ (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ਅਚਲ ਸਿਖਰ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ਅਚਲ processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            ਸਿਖਰ_Pkgp->ਜੋੜੋ_Stmtsp(processQueuep);
            ਵਾਪਸ processQueuep;
        }

        AstVar* ਅਚਲ processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->ਜੋੜੋ_Stmtsp(processQueuep);
        ਵਾਪਸ processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ਅਚਲ taskp, File_ਸਤਰ* ਅਚਲ fl) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_taskDisableQueues.ਲੱਭੋ(taskp);
        ਜੇ (it != m_taskDisableQueues.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        AstVar* ਅਚਲ processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ ਧੱਕੋ_Current_Processp = get_ਕਤਾਰ_ਧੱਕੋ_Process_Selfp(fl, processQueuep);
        AstBegin* ਅਚਲ taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ਧੱਕੋ_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        ਵਾਪਸ processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ਅਚਲ beginp, File_ਸਤਰ* ਅਚਲ fl) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_beginDisableBegins.ਲੱਭੋ(beginp);
        ਜੇ (it != m_beginDisableBegins.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        AstBegin* ਅਚਲ beginBodyp = ਨਵਾਂ AstBegin{fl, "", ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->ਸੈੱਟ_Need_Process();
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
        ਜੇ (beginp->stmtsp()) beginBodyp->ਜੋੜੋ_Stmtsp(beginp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());

        AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{fl, VJoinType::JOIN};
        forkp->ਜੋੜੋ_Forksp(beginBodyp);
        beginp->ਜੋੜੋ_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        ਵਾਪਸ beginBodyp;
    }
    ਖਾਲੀ prepend_Fork_Branch_ਕਤਾਰ_Pushes(AstFork* ਅਚਲ forkp, AstVar* ਅਚਲ processQueuep,
                                      File_ਸਤਰ* ਅਚਲ fl, ਤਾਰਕਿਕ needProcess) {
        ਲਈ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            ਜੇ (needProcess) branchp->ਸੈੱਟ_Need_Process();
            branchp->prependStmtsp(get_ਕਤਾਰ_ਧੱਕੋ_Process_Selfp(fl, processQueuep));
        }
    }
    ਖਾਲੀ prepend_Nested_Fork_Branch_ਕਤਾਰ_Pushes(Ast_ਗੰਢ* ਅਚਲ nodep, AstVar* ਅਚਲ processQueuep,
                                            File_ਸਤਰ* ਅਚਲ fl, ਤਾਰਕਿਕ needProcess) {
        nodep->foreach([&](AstFork* ਅਚਲ forkp) {
            prepend_Fork_Branch_ਕਤਾਰ_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ਅਚਲ beginp, File_ਸਤਰ* ਅਚਲ fl) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_beginDisableQueues.ਲੱਭੋ(beginp);
        ਜੇ (it != m_beginDisableQueues.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        AstVar* ਅਚਲ processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ ਧੱਕੋ_Current_Processp = get_ਕਤਾਰ_ਧੱਕੋ_Process_Selfp(fl, processQueuep);
        AstBegin* ਅਚਲ beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ਧੱਕੋ_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_ਕਤਾਰ_Pushes(beginBodyp, processQueuep, fl, ਝੂਠ);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        ਵਾਪਸ processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ਅਚਲ forkp, File_ਸਤਰ* ਅਚਲ fl) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_forkDisableQueues.ਲੱਭੋ(forkp);
        ਜੇ (it != m_forkDisableQueues.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        AstVar* ਅਚਲ processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_ਕਤਾਰ_Pushes(forkp, processQueuep, fl, ਸੱਚ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        ਲਈ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_ਕਤਾਰ_Pushes(branchp, processQueuep, fl, ਸੱਚ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        ਵਾਪਸ processQueuep;
    }
    Ast_Stmt_ਵਿਅੰਜਕ* ਪਾਓ_Kill_Stmtp(AstDisable* ਅਚਲ nodep, AstVar* ਅਚਲ processQueuep) {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ killStmtp
            = get_ਕਤਾਰ_Kill_Stmtp(fl, processQueuep, ਕਤਾਰ_Dotted(nodep, processQueuep));
        nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(killStmtp);
        ਵਾਪਸ killStmtp;
    }
    ਖਾਲੀ ਜੋੜੋ_Jump_After_Kill(Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ killStmtp, Ast_ਗੰਢ* ਅਚਲ targetp) {
        Ast_Jump_ਖੰਡ* ਅਚਲ jmpBlockp = get_Jump_ਖੰਡ(targetp, ਝੂਠ);
        killStmtp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    ਖਾਲੀ handleDisableOnFork(AstDisable* ਅਚਲ nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstFork* ਅਚਲ targetp = VN_AS(nodep->targetp(), Fork);
        ਜੇ (m_ftaskp) {
            ਜੇ (!m_ftaskp->exists(
                    [targetp](ਅਚਲ Ast_ਗੰਢ_ਖੰਡ* blockp) -> ਤਾਰਕਿਕ { ਵਾਪਸ blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ਅਚਲ processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ killStmtp = ਪਾਓ_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        ਜੇ (AstBegin* ਅਚਲ branchp = innerForkBranchp(targetp)) {
            ਜੋੜੋ_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->dead()) ਵਾਪਸ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_ਅੰਕ);
        m_modp = nodep;
        m_mod_Repeat_ਅੰਕ = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ਭਰਿਆ);
        m_ਖੰਡ_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(nodep);
        iterateChildren(nodep);
        m_ਖੰਡ_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ਭਰਿਆ);
        VL_RESTORER(m_ਵਿੱਚ_Fork);
        m_ਵਿੱਚ_Fork = ਸੱਚ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        ਲਈ (Ast_ਗੰਢ_ਖੰਡ* ਅਚਲ blockp : vlstd::ਉਲਟ_view(m_ਖੰਡ_ਢੇਰ)) {
            ਜੇ (blockp->user3_ਸੈੱਟ_Once()) ਰੋਕੋ;
        }
        m_ਖੰਡ_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(nodep);
        iterateChildren(nodep);
        m_ਖੰਡ_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstStmtPragma* nodep) override {
        ਜੇ (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_ਭਰਿਆ = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->pragp()->pragType() == VPragmaType::UNROLL_ਭਰਿਆ) {
            m_unroll_ਭਰਿਆ = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ countp = nodep->countp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        ਅਚਲ string name = "__Vrepeat"s + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_Repeat_ਅੰਕ++);
        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{nodep->fileline(), "", ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ};
        // Spec says value is integral, if negative is ignored
        AstVar* ਅਚਲ varp
            = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->ਲੱਭੋ_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);
        beginp->ਜੋੜੋ_Stmtsp(varp);
        Ast_ਗੰਢ* initsp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ਗੰਢ* ਅਚਲ decp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            ਨਵਾਂ AstSub{nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       ਨਵਾਂ AstConst{nodep->fileline(), 1}}};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ zerosp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstGtS{
            nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ਗੰਢ* ਅਚਲ bodysp = nodep->stmtsp();
        ਜੇ (bodysp) bodysp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, condp});
        loopp->ਜੋੜੋ_Stmtsp(bodysp);
        loopp->ਜੋੜੋ_Contsp(decp);
        ਜੇ (!m_unroll_ਭਰਿਆ.isDefault()) loopp->unroll(m_unroll_ਭਰਿਆ);
        m_unroll_ਭਰਿਆ = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->ਜੋੜੋ_Stmtsp(initsp);
        beginp->ਜੋੜੋ_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        ਜੇ (!m_unroll_ਭਰਿਆ.isDefault()) nodep->unroll(m_unroll_ਭਰਿਆ);
        ਜੇ (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDLOOP, ਸੱਚ);
        }
        m_unroll_ਭਰਿਆ = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = ਝੂਠ;
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        m_loopInc = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        ਜੇ (nodep->contsp()) nodep->ਜੋੜੋ_Stmtsp(nodep->contsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_ਅਗਲਾ_Null(nodep->bodyp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstReturn* nodep) override {
        iterateChildren(nodep);
        ਅਚਲ AstFunc* ਅਚਲ funcp = VN_CAST(m_ftaskp, Func);
        ਜੇ (m_randsequencep) {
            nodep->replaceWith(ਨਵਾਂ AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } ਨਹੀਂਤਾਂ ਜੇ (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } ਨਹੀਂਤਾਂ ਜੇ (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } ਨਹੀਂਤਾਂ {
            ਜੇ (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    nodep->fileline(),
                    ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()});
            }
            // Jump to the end of the function call
            Ast_Jump_ਖੰਡ* ਅਚਲ blockp = get_Jump_ਖੰਡ(m_ftaskp, ਝੂਠ);
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਨਵਾਂ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBreak* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!m_loopp && m_randsequencep) {
            nodep->replaceWith(ਨਵਾਂ AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } ਨਹੀਂਤਾਂ {
            // Jump to the end of the loop
            Ast_Jump_ਖੰਡ* ਅਚਲ blockp = get_Jump_ਖੰਡ(m_loopp, ਝੂਠ);
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstContinue* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } ਨਹੀਂਤਾਂ {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_ਖੰਡ* ਅਚਲ blockp = get_Jump_ਖੰਡ(m_loopp, ਸੱਚ);
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ਗੰਢ* ਅਚਲ targetp = nodep->targetp();
        ਜੇ (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (AstTask* ਅਚਲ taskp = VN_CAST(targetp, Task)) {
            AstVar* ਅਚਲ processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ killStmtp = ਪਾਓ_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            ਜੇ (m_ftaskp == taskp) {
                Ast_ਗੰਢ* jumpTargetp = taskp;
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_taskDisableBegins.ਲੱਭੋ(taskp);
                ਜੇ (it != m_taskDisableBegins.ਸਿਰਾ()) jumpTargetp = it->ਦੂਜਾ;
                ਜੋੜੋ_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(targetp, Begin)) {
            ਜੇ (exists_ਖੰਡ_Above(beginp->name())) {
                ਜੇ (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_ਖੰਡ* ਅਚਲ blockp = get_Jump_ਖੰਡ(beginp, ਝੂਠ);
                    nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstJumpGo{nodep->fileline(), blockp});
                } ਨਹੀਂਤਾਂ {
                    AstVar* ਅਚਲ processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ killStmtp = ਪਾਓ_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ਅਚਲ branchp = innerForkBranchp(beginp);
                    AstBegin* ਅਚਲ jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    ਜੋੜੋ_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } ਨਹੀਂਤਾਂ {
                AstVar* ਅਚਲ processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ਪਾਓ_Kill_Stmtp(nodep, processQueuep);
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        ਜੇ (m_ਵਿੱਚ_Fork) {
            nodep->replaceWith(ਨਵਾਂ Ast_ਅੰਤ_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_ਖੰਡ* ਅਚਲ blockp = get_Jump_ਖੰਡ(m_loopp, ਝੂਠ);
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstJumpGo{nodep->fileline(), blockp});
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (m_loopInc && nodep->varp()) nodep->varp()->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst*) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = ਮੂਲ;
};

//######################################################################
// Task class functions

ਖਾਲੀ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("linkjump", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
