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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkJump.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ LinkJumpVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstBegin/etc::user1()  -> AstJumpBlock*, for body of this loop
    //  AstFinish::user1()     -> bool, processed
    //  AstNode::user2()       -> AstJumpBlock*, for this block
    //  AstNodeBegin::user3()  -> bool, true if contains a fork
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_user3_ಒಳ_Use;

    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ಗಂಟು* m_loopp = ಶೂನ್ಯಸೂಚಕ;  // Current loop
    AstRandSequence* m_randsequencep = ಶೂನ್ಯಸೂಚಕ;  // Current randsequence
    ತಾರ್ಕಿಕ m_loopInc = ಸುಳ್ಳು;  // In loop increment
    ತಾರ್ಕಿಕ m_ಒಳ_Fork = ಸುಳ್ಳು;  // Under fork
    ಪೂರ್ಣಾಂಕ m_mod_Repeat_ಅಂಕೆ = 0;  // Repeat counter
    VOptionBool m_unroll_ತುಂಬಿದ;  // Pragma full, disable, or default unrolling
    std::ಸದಿಶ<Ast_ಗಂಟು_ಖಂಡ*> m_ಖಂಡ_ರಾಶಿ;  // All begin blocks above current node
    V3_Unique_ಹೆಸರುಗಳು m_ಸರತಿ_ಹೆಸರುಗಳು{
        "__VprocessQueue"};  // Names for queues needed for 'disable' handling
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstTask*, AstVar*> m_taskDisableQueues;  // Per-task process queues
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstBegin*, AstVar*> m_beginDisableQueues;  // Per-begin process queues
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstFork*, AstVar*> m_forkDisableQueues;  // Per-fork process queues
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstTask*, AstBegin*>
        m_taskDisableBegins;  // Per-task process wrappers
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstBegin*, AstBegin*>
        m_beginDisableBegins;  // Per-begin process wrappers

    // METHODS
    // Get (and create if necessary) the JumpBlock for this statement
    Ast_Jump_ಖಂಡ* get_Jump_ಖಂಡ(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ಕೊನೆ_ದ_Iter) {
        // Wrap 'nodep' in JumpBlock. If loop, wrap the body instead if endOfIter is true
        UINFO(4, "Create JumpBlock for " << nodep);

        // Made it previously?  We always jump to the end, so this works out
        ಆದರೆ (ಕೊನೆ_ದ_Iter) {
            ಆದರೆ (nodep->user1p()) ಹಿಂತಿರುಗಿ VN_AS(nodep->user1p(), Jump_ಖಂಡ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->user2p()) ಹಿಂತಿರುಗಿ VN_AS(nodep->user2p(), Jump_ಖಂಡ);
        }

        Ast_ಗಂಟು* underp = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ under_and_ಮುಂದಿನ = ನಿಜ;
        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ blockp = VN_CAST(nodep, Begin)) {
            UASSERT_OBJ(!ಕೊನೆ_ದ_Iter, nodep, "No endOfIter for Begin");
            underp = blockp->stmtsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ fTaskp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            UASSERT_OBJ(!ಕೊನೆ_ದ_Iter, nodep, "No endOfIter for FTask");
            underp = fTaskp->stmtsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstForeach* ಸ್ಥಿರಾಂಕ foreachp = VN_CAST(nodep, Foreach)) {
            ಆದರೆ (ಕೊನೆ_ದ_Iter) {
                underp = foreachp->bodyp();
                // Keep a LoopTest **at the front** outside the jump block
                ಆದರೆ (VN_IS(underp, LoopTest)) underp = underp->nextp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                underp = nodep;
                under_and_ಮುಂದಿನ = ಸುಳ್ಳು;  // IE we skip the entire foreach
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstLoop* ಸ್ಥಿರಾಂಕ loopp = VN_CAST(nodep, Loop)) {
            ಆದರೆ (ಕೊನೆ_ದ_Iter) {
                underp = loopp->stmtsp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                underp = nodep;
                under_and_ಮುಂದಿನ = ಸುಳ್ಳು;  // IE we skip the entire loop
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Unknown jump point for break/disable/continue");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // Skip over variables as we'll just move them in a moment
        // Also this would otherwise prevent us from using a label twice
        // see t_func_return test.
        ಆಗ (underp && VN_IS(underp, Var)) underp = underp->nextp();
        UASSERT_OBJ(underp, nodep, "Break/disable/continue not under expected statement");
        UINFO(5, "  Underpoint is " << underp);

        // If already wrapped, we are done ...
        ಆದರೆ (!underp->nextp() || !under_and_ಮುಂದಿನ) {
            ಆದರೆ (Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = VN_CAST(underp, Jump_ಖಂಡ)) ಹಿಂತಿರುಗಿ blockp;
        }

        // Move underp stuff to be under a new AstJumpBlock
        VNRelinker repHandle;
        ಆದರೆ (under_and_ಮುಂದಿನ) {
            underp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(&repHandle);
        } ಇಲ್ಲದಿದ್ದರೆ {
            underp->unlink_Fr_ಹಿಂಭಾಗ(&repHandle);
        }
        Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = ಹೊಸ Ast_Jump_ಖಂಡ{nodep->fileline(), underp};
        ಆದರೆ (ಕೊನೆ_ದ_Iter) {
            nodep->user1p(blockp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->user2p(blockp);
        }
        repHandle.relink(blockp);

        // Keep any AstVars under the function not under the new JumpLabel
        ಗಾಗಿ (Ast_ಗಂಟು *nextp, *varp = underp; varp; varp = nextp) {
            nextp = varp->nextp();
            ಆದರೆ (VN_IS(varp, Var)) blockp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(varp->unlink_Fr_ಹಿಂಭಾಗ());
        }
        ಹಿಂತಿರುಗಿ blockp;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_ಪೂರ್ವಪ್ರತ್ಯಯ_ಗಮ್ಯ_ಖಂಡಗಳು_Recurse(ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        // Add a prefix to blocks
        // Used to not have blocks with duplicated names
        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(nodep, Begin)) {
            ಆದರೆ (beginp->name() != "") beginp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + beginp->name());
        }

        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op1p()) ಕೂಡಿಸು_ಪೂರ್ವಪ್ರತ್ಯಯ_ಗಮ್ಯ_ಖಂಡಗಳು_Recurse(ಪೂರ್ವಪ್ರತ್ಯಯ, refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op2p()) ಕೂಡಿಸು_ಪೂರ್ವಪ್ರತ್ಯಯ_ಗಮ್ಯ_ಖಂಡಗಳು_Recurse(ಪೂರ್ವಪ್ರತ್ಯಯ, refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op3p()) ಕೂಡಿಸು_ಪೂರ್ವಪ್ರತ್ಯಯ_ಗಮ್ಯ_ಖಂಡಗಳು_Recurse(ಪೂರ್ವಪ್ರತ್ಯಯ, refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op4p()) ಕೂಡಿಸು_ಪೂರ್ವಪ್ರತ್ಯಯ_ಗಮ್ಯ_ಖಂಡಗಳು_Recurse(ಪೂರ್ವಪ್ರತ್ಯಯ, refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->nextp()) ಕೂಡಿಸು_ಪೂರ್ವಪ್ರತ್ಯಯ_ಗಮ್ಯ_ಖಂಡಗಳು_Recurse(ಪೂರ್ವಪ್ರತ್ಯಯ, refp);
    }
    ತಾರ್ಕಿಕ exists_ಖಂಡ_Above(ಸ್ಥಿರಾಂಕ std::string& name) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಖಂಡ* ಸ್ಥಿರಾಂಕ stackp : vlstd::ಹಿಮ್ಮುಖ_view(m_ಖಂಡ_ರಾಶಿ)) {
            ಆದರೆ (stackp->name() == name) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Owner_Modulep(Ast_ಗಂಟು* nodep) {
        ಗಾಗಿ (Ast_ಗಂಟು* curp = nodep; curp; curp = curp->aboveLoopp()) {
            ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(curp, ಗಂಟು_Module)) ಹಿಂತಿರುಗಿ modp;
        }
        nodep->v3fatalSrc("Disable target is not under a module");
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // LCOV_EXCL_LINE
    }
    ಸ್ಥಿರ std::string ಗುರಿ_Instance_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ AstDisable* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(nodep->ಗುರಿ_Refp(), VarXRef)) {
            ಹಿಂತಿರುಗಿ xrefp->dotted();
        }
        ಹಿಂತಿರುಗಿ "";
    }
    ಸ್ಥಿರ std::string ಸರತಿ_Dotted(ಸ್ಥಿರಾಂಕ AstDisable* ಸ್ಥಿರಾಂಕ nodep,
                                   ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ processQueuep) {
        ಹಿಂತಿರುಗಿ processQueuep->is_ತಾತ್ಕಾಲಿಕ() ? ಗುರಿ_Instance_ಮಾರ್ಗ(nodep) : "";
    }
    AstBegin* innerForkBranchp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಖಂಡ* ಸ್ಥಿರಾಂಕ targetp) ಸ್ಥಿರಾಂಕ {
        AstBegin* innerForkBranchp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಖಂಡ* prevBlockp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp : vlstd::ಹಿಮ್ಮುಖ_view(m_ಖಂಡ_ರಾಶಿ)) {
            ಆದರೆ (!innerForkBranchp && VN_IS(blockp, Fork)) {
                innerForkBranchp = VN_CAST(prevBlockp, Begin);
            }
            ಆದರೆ (blockp == targetp) ಹಿಂತಿರುಗಿ innerForkBranchp;
            prevBlockp = blockp;
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* get_ಸರತಿ_ತಳ್ಳು_Process_Selfp(AstVarRef* ಸ್ಥಿರಾಂಕ ಸರತಿ_Refp) {
        // Constructs queue.push_back(std::process::self()) statement
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ಸರತಿ_Refp->fileline();
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{
            flp,
            ಹೊಸ Ast_ವಿಧಾನ_Call{flp, ಸರತಿ_Refp, "push_back",
                              ಹೊಸ AstArg{flp, "", v3Global.rootp()->stdPackageProcessSelfp(flp)}}};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Var_Ref* new_ಸರತಿ_Refp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ processQueuep,
                                       ಸ್ಥಿರಾಂಕ VAccess& access, ಸ್ಥಿರಾಂಕ std::string& dotted = "") {
        ಆದರೆ (dotted != "") ಹಿಂತಿರುಗಿ ಹೊಸ AstVarXRef{fl, processQueuep, dotted, access};
        ಆದರೆ (!processQueuep->lifetime().isStatic() || processQueuep->is_ತಾತ್ಕಾಲಿಕ()) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, processQueuep, access};
        }
        AstPackage* ಸ್ಥಿರಾಂಕ ತುದಿ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, ತುದಿ_Pkgp, processQueuep, access};
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* get_ಸರತಿ_ತಳ್ಳು_Process_Selfp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ processQueuep) {
        AstVarRef* ಸ್ಥಿರಾಂಕ ಸರತಿ_Write_Refp
            = VN_AS(new_ಸರತಿ_Refp(fl, processQueuep, VAccess::WRITE), VarRef);
        ಹಿಂತಿರುಗಿ get_ಸರತಿ_ತಳ್ಳು_Process_Selfp(ಸರತಿ_Write_Refp);
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* get_ಸರತಿ_Kill_Stmtp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ processQueuep,
                                          ಸ್ಥಿರಾಂಕ std::string& dotted = "") {
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ ಸರತಿ_Refp
            = new_ಸರತಿ_Refp(fl, processQueuep, VAccess::READWRITE, dotted);
        AstTaskRef* kill_ಸರತಿ_Call = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = v3Global.rootp()->stdPackageProcessp()->stmtsp(); itemp;
             itemp = itemp->nextp()) {
            ಆದರೆ (itemp->name() == "killQueue") {
                kill_ಸರತಿ_Call
                    = ಹೊಸ AstTaskRef{fl, VN_AS(itemp, Task), ಹೊಸ AstArg{fl, "", ಸರತಿ_Refp}};
                ನಿಲ್ಲಿಸು;
            }
        }
        UASSERT(kill_ಸರತಿ_Call, "Should be found");
        kill_ಸರತಿ_Call->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, kill_ಸರತಿ_Call};
    }
    AstBegin* getOrCreateTaskDisableBeginp(AstTask* ಸ್ಥಿರಾಂಕ taskp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_taskDisableBegins.ಕಂಡುಹಿಡಿ(taskp);
        ಆದರೆ (it != m_taskDisableBegins.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        AstBegin* ಸ್ಥಿರಾಂಕ taskBodyp = ಹೊಸ AstBegin{fl, "", ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
        // Disable-by-name rewrites kill this detached task-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        taskBodyp->ಗಣ_Need_Process();
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
        // Ports stay in the task so calls see them
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = taskp->stmtsp(); stmtp;) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = stmtp->nextp();
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var);
            ಆದರೆ (!varp || !varp->isIO()) taskBodyp->ಕೂಡಿಸು_Stmtsp(stmtp->unlink_Fr_ಹಿಂಭಾಗ());
            stmtp = nextp;
        }

        AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{fl, VJoinType::JOIN};
        forkp->ಕೂಡಿಸು_Forksp(taskBodyp);
        taskp->ಕೂಡಿಸು_Stmtsp(forkp);

        m_taskDisableBegins.emplace(taskp, taskBodyp);
        ಹಿಂತಿರುಗಿ taskBodyp;
    }
    AstVar* newProcessQueuep(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, ಸ್ಥಿರಾಂಕ VVarType& varType) {
        AstVar* ಸ್ಥಿರಾಂಕ processQueuep = ಹೊಸ AstVar{
            fl, varType, m_ಸರತಿ_ಹೆಸರುಗಳು.get(nodep->name()), VFlag_ಮಗು_DType{},
            ಹೊಸ Ast_ಸರತಿ_DType{
                fl, VFlag_ಮಗು_DType{},
                ಹೊಸ AstClassRefDType{fl, v3Global.rootp()->stdPackageProcessp(), ಶೂನ್ಯಸೂಚಕ},
                ಶೂನ್ಯಸೂಚಕ}};
        processQueuep->process_ಸರತಿ(ನಿಜ);
        processQueuep->ಗಣ_Ignore_Sched_Write();
        ಹಿಂತಿರುಗಿ processQueuep;
    }
    AstVar* getProcessQueuep(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerp = ಕಂಡುಹಿಡಿ_Owner_Modulep(nodep);

        ಆದರೆ (VN_IS(ownerp, Package) || VN_IS(ownerp, Class)) {
            AstPackage* ಸ್ಥಿರಾಂಕ ತುದಿ_Pkgp = v3Global.rootp()->dollarUnitPkgp();
            AstVar* ಸ್ಥಿರಾಂಕ processQueuep = newProcessQueuep(nodep, fl, VVarType::VAR);
            processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
            ತುದಿ_Pkgp->ಕೂಡಿಸು_Stmtsp(processQueuep);
            ಹಿಂತಿರುಗಿ processQueuep;
        }

        AstVar* ಸ್ಥಿರಾಂಕ processQueuep = newProcessQueuep(nodep, fl, VVarType::MODULETEMP);
        processQueuep->lifetime(VLifetime::STATIC_EXPLICIT);
        ownerp->ಕೂಡಿಸು_Stmtsp(processQueuep);
        ಹಿಂತಿರುಗಿ processQueuep;
    }
    AstVar* getOrCreateTaskDisableQueuep(AstTask* ಸ್ಥಿರಾಂಕ taskp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_taskDisableQueues.ಕಂಡುಹಿಡಿ(taskp);
        ಆದರೆ (it != m_taskDisableQueues.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        AstVar* ಸ್ಥಿರಾಂಕ processQueuep = getProcessQueuep(taskp, fl);
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಳ್ಳು_Current_Processp = get_ಸರತಿ_ತಳ್ಳು_Process_Selfp(fl, processQueuep);
        AstBegin* ಸ್ಥಿರಾಂಕ taskBodyp = getOrCreateTaskDisableBeginp(taskp, fl);
        taskBodyp->prependStmtsp(ತಳ್ಳು_Current_Processp);
        m_taskDisableQueues.emplace(taskp, processQueuep);
        ಹಿಂತಿರುಗಿ processQueuep;
    }
    AstBegin* getOrCreateBeginDisableBeginp(AstBegin* ಸ್ಥಿರಾಂಕ beginp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_beginDisableBegins.ಕಂಡುಹಿಡಿ(beginp);
        ಆದರೆ (it != m_beginDisableBegins.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        AstBegin* ಸ್ಥಿರಾಂಕ beginBodyp = ಹೊಸ AstBegin{fl, "", ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
        // Disable-by-name rewrites kill this detached block-body process, so mark it as process
        // backed to ensure fork/join kill-accounting hooks are always emitted.
        beginBodyp->ಗಣ_Need_Process();
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
        ಆದರೆ (beginp->stmtsp()) beginBodyp->ಕೂಡಿಸು_Stmtsp(beginp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());

        AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{fl, VJoinType::JOIN};
        forkp->ಕೂಡಿಸು_Forksp(beginBodyp);
        beginp->ಕೂಡಿಸು_Stmtsp(forkp);

        m_beginDisableBegins.emplace(beginp, beginBodyp);
        ಹಿಂತಿರುಗಿ beginBodyp;
    }
    ಶೂನ್ಯ prepend_Fork_Branch_ಸರತಿ_Pushes(AstFork* ಸ್ಥಿರಾಂಕ forkp, AstVar* ಸ್ಥಿರಾಂಕ processQueuep,
                                      File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, ತಾರ್ಕಿಕ needProcess) {
        ಗಾಗಿ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            ಆದರೆ (needProcess) branchp->ಗಣ_Need_Process();
            branchp->prependStmtsp(get_ಸರತಿ_ತಳ್ಳು_Process_Selfp(fl, processQueuep));
        }
    }
    ಶೂನ್ಯ prepend_Nested_Fork_Branch_ಸರತಿ_Pushes(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, AstVar* ಸ್ಥಿರಾಂಕ processQueuep,
                                            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, ತಾರ್ಕಿಕ needProcess) {
        nodep->foreach([&](AstFork* ಸ್ಥಿರಾಂಕ forkp) {
            prepend_Fork_Branch_ಸರತಿ_Pushes(forkp, processQueuep, fl, needProcess);
        });
    }
    AstVar* getOrCreateBeginDisableQueuep(AstBegin* ಸ್ಥಿರಾಂಕ beginp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_beginDisableQueues.ಕಂಡುಹಿಡಿ(beginp);
        ಆದರೆ (it != m_beginDisableQueues.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        AstVar* ಸ್ಥಿರಾಂಕ processQueuep = getProcessQueuep(beginp, fl);
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಳ್ಳು_Current_Processp = get_ಸರತಿ_ತಳ್ಳು_Process_Selfp(fl, processQueuep);
        AstBegin* ಸ್ಥಿರಾಂಕ beginBodyp = getOrCreateBeginDisableBeginp(beginp, fl);
        beginBodyp->prependStmtsp(ತಳ್ಳು_Current_Processp);

        // Named-block disable must also terminate detached descendants created by forks
        // under the block, so track each fork branch process in the same queue.
        prepend_Nested_Fork_Branch_ಸರತಿ_Pushes(beginBodyp, processQueuep, fl, ಸುಳ್ಳು);
        m_beginDisableQueues.emplace(beginp, processQueuep);
        ಹಿಂತಿರುಗಿ processQueuep;
    }
    AstVar* getOrCreateForkDisableQueuep(AstFork* ಸ್ಥಿರಾಂಕ forkp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_forkDisableQueues.ಕಂಡುಹಿಡಿ(forkp);
        ಆದರೆ (it != m_forkDisableQueues.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        AstVar* ಸ್ಥಿರಾಂಕ processQueuep = getProcessQueuep(forkp, fl);
        prepend_Fork_Branch_ಸರತಿ_Pushes(forkp, processQueuep, fl, ನಿಜ);
        // Disabling a fork must also terminate detached descendants created by nested forks
        // under each branch, so track nested fork branch processes in the same queue.
        ಗಾಗಿ (AstBegin* branchp = forkp->forksp(); branchp;
             branchp = VN_AS(branchp->nextp(), Begin)) {
            prepend_Nested_Fork_Branch_ಸರತಿ_Pushes(branchp, processQueuep, fl, ನಿಜ);
        }
        m_forkDisableQueues.emplace(forkp, processQueuep);
        ಹಿಂತಿರುಗಿ processQueuep;
    }
    Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸೇರಿಸು_Kill_Stmtp(AstDisable* ಸ್ಥಿರಾಂಕ nodep, AstVar* ಸ್ಥಿರಾಂಕ processQueuep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ killStmtp
            = get_ಸರತಿ_Kill_Stmtp(fl, processQueuep, ಸರತಿ_Dotted(nodep, processQueuep));
        nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(killStmtp);
        ಹಿಂತಿರುಗಿ killStmtp;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Jump_After_Kill(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ killStmtp, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ targetp) {
        Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ jmpBlockp = get_Jump_ಖಂಡ(targetp, ಸುಳ್ಳು);
        killStmtp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstJumpGo{killStmtp->fileline(), jmpBlockp});
    }
    ಶೂನ್ಯ handleDisableOnFork(AstDisable* ಸ್ಥಿರಾಂಕ nodep) {
        // The support utilizes the process::kill()` method. For each disabled fork a queue of
        // processes is declared. At the beginning of each fork branch (and each nested fork
        // branch) its process handle is pushed to the queue. The `disable` statement is replaced
        // with calling `kill()` on each element of the queue.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstFork* ಸ್ಥಿರಾಂಕ targetp = VN_AS(nodep->targetp(), Fork);
        ಆದರೆ (m_ftaskp) {
            ಆದರೆ (!m_ftaskp->exists(
                    [targetp](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಖಂಡ* blockp) -> ತಾರ್ಕಿಕ { ಹಿಂತಿರುಗಿ blockp == targetp; })) {
                // Disabling a fork, which is within the same task, is not a problem
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: disabling fork from task / function");
            }
        }

        AstVar* ಸ್ಥಿರಾಂಕ processQueuep = getOrCreateForkDisableQueuep(targetp, fl);
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ killStmtp = ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);

        // 'process::kill' does not immediately kill the current process executing the disable
        // statement (because it's in the running state). If the disable runs under the target
        // fork, jump to the end of the innermost enclosing fork branch (the branch that holds the
        // running process) so statements after the disable do not execute. Targeting the innermost
        // branch keeps the jump inside a single emitted coroutine even when the disable sits in a
        // nested sub-fork; targeting the outer target fork's branch would cross a coroutine
        // boundary once forks are split into separate functions.
        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ branchp = innerForkBranchp(targetp)) {
            ಕೂಡಿಸು_Jump_After_Kill(killStmtp, branchp);
        }
    }
    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->dead()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Repeat_ಅಂಕೆ);
        m_modp = nodep;
        m_mod_Repeat_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ತುಂಬಿದ);
        m_ಖಂಡ_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        iterateChildren(nodep);
        m_ಖಂಡ_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        UINFO(8, "  " << nodep);
        VL_RESTORER(m_unroll_ತುಂಬಿದ);
        VL_RESTORER(m_ಒಳ_Fork);
        m_ಒಳ_Fork = ನಿಜ;
        // Mark all upper blocks, can stop once see one set to avoid O(n^2)
        ಗಾಗಿ (Ast_ಗಂಟು_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp : vlstd::ಹಿಮ್ಮುಖ_view(m_ಖಂಡ_ರಾಶಿ)) {
            ಆದರೆ (blockp->user3_ಗಣ_Once()) ನಿಲ್ಲಿಸು;
        }
        m_ಖಂಡ_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        iterateChildren(nodep);
        m_ಖಂಡ_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStmtPragma* nodep) override {
        ಆದರೆ (nodep->pragp()->pragType() == VPragmaType::UNROLL_DISABLE) {
            m_unroll_ತುಂಬಿದ = VOptionBool::OPT_FALSE;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->pragp()->pragType() == VPragmaType::UNROLL_ತುಂಬಿದ) {
            m_unroll_ತುಂಬಿದ = VOptionBool::OPT_TRUE;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandSequence* nodep) override {
        VL_RESTORER(m_randsequencep);
        m_randsequencep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRepeat* nodep) override {
        // So later optimizations don't need to deal with them,
        //    REPEAT(count,body) -> loop=count,WHILE(loop>0) { body, loop-- }
        // Note var can be signed or unsigned based on original number.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ countp = nodep->countp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        ಸ್ಥಿರಾಂಕ string name = "__Vrepeat"s + cvt_ಗಮ್ಯ_Str(m_mod_Repeat_ಅಂಕೆ++);
        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{nodep->fileline(), "", ಶೂನ್ಯಸೂಚಕ, ನಿಜ};
        // Spec says value is integral, if negative is ignored
        AstVar* ಸ್ಥಿರಾಂಕ varp
            = ಹೊಸ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, name, nodep->ಕಂಡುಹಿಡಿ_Int_DType()};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->ಬಳಸಿದ_Loop_Idx(ನಿಜ);
        beginp->ಕೂಡಿಸು_Stmtsp(varp);
        Ast_ಗಂಟು* initsp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, countp};
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ decp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::WRITE},
            ಹೊಸ AstSub{nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::READ},
                       ಹೊಸ AstConst{nodep->fileline(), 1}}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ zerosp = ಹೊಸ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstGtS{
            nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::READ}, zerosp};
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodysp = nodep->stmtsp();
        ಆದರೆ (bodysp) bodysp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, condp});
        loopp->ಕೂಡಿಸು_Stmtsp(bodysp);
        loopp->ಕೂಡಿಸು_Contsp(decp);
        ಆದರೆ (!m_unroll_ತುಂಬಿದ.isDefault()) loopp->unroll(m_unroll_ತುಂಬಿದ);
        m_unroll_ತುಂಬಿದ = VOptionBool::OPT_DEFAULT_FALSE;
        beginp->ಕೂಡಿಸು_Stmtsp(initsp);
        beginp->ಕೂಡಿಸು_Stmtsp(loopp);
        // Replacement AstBegin will be iterated next
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        ಆದರೆ (!m_unroll_ತುಂಬಿದ.isDefault()) nodep->unroll(m_unroll_ತುಂಬಿದ);
        ಆದರೆ (m_modp->hasParameterList() || m_modp->hasGParam()) {
            nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDLOOP, ನಿಜ);
        }
        m_unroll_ತುಂಬಿದ = VOptionBool::OPT_DEFAULT_FALSE;
        VL_RESTORER(m_loopp);
        VL_RESTORER(m_loopInc);
        m_loopp = nodep;
        m_loopInc = ಸುಳ್ಳು;
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        m_loopInc = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep->contsp());
        // Move contsp into stmtsp, no longer needed to keep separately
        ಆದರೆ (nodep->contsp()) nodep->ಕೂಡಿಸು_Stmtsp(nodep->contsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {
        VL_RESTORER(m_loopp);
        m_loopp = nodep;
        iterate_And_ಮುಂದಿನ_Null(nodep->bodyp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReturn* nodep) override {
        iterateChildren(nodep);
        ಸ್ಥಿರಾಂಕ AstFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(m_ftaskp, Func);
        ಆದರೆ (m_randsequencep) {
            nodep->replaceWith(ಹೊಸ AstRSReturn{nodep->fileline()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_Fork) {
            nodep->v3error("Return isn't legal under fork (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_ftaskp) {
            nodep->v3error("Return isn't underneath a task or function");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (funcp && !nodep->lhsp() && !funcp->isConstructor()) {
            nodep->v3error("Return underneath a function should have return value");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!funcp && nodep->lhsp()) {
            nodep->v3error("Return underneath a task shouldn't have return value");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (funcp && nodep->lhsp()) {
                // Set output variable to return value
                nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    nodep->fileline(),
                    ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(funcp->fvarp(), Var), VAccess::WRITE},
                    nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()});
            }
            // Jump to the end of the function call
            Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = get_Jump_ಖಂಡ(m_ftaskp, ಸುಳ್ಳು);
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಹೊಸ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBreak* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!m_loopp && m_randsequencep) {
            nodep->replaceWith(ಹೊಸ AstRSBreak{nodep->fileline()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_loopp) {
            nodep->v3error("break isn't underneath a loop");
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Jump to the end of the loop
            Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = get_Jump_ಖಂಡ(m_loopp, ಸುಳ್ಳು);
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstContinue* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!m_loopp) {
            nodep->v3error("continue isn't underneath a loop");
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Jump to the end of this iteration
            // If a "for" loop then need to still do the post-loop increment
            Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = get_Jump_ಖಂಡ(m_loopp, ನಿಜ);
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstJumpGo{nodep->fileline(), blockp});
        }
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisable* nodep) override {
        UINFO(8, "   DISABLE " << nodep);
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ targetp = nodep->targetp();
        ಆದರೆ (!targetp) {
            // Linking errors on the disable target are already reported upstream.
            // Drop this node to avoid cascading into an internal assertion.
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(targetp, Task)) {
            AstVar* ಸ್ಥಿರಾಂಕ processQueuep = getOrCreateTaskDisableQueuep(taskp, nodep->fileline());
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ killStmtp = ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);

            // process::kill does not terminate the currently running process immediately.
            // If we disable the current task by name from inside itself, jump to its end.
            ಆದರೆ (m_ftaskp == taskp) {
                Ast_ಗಂಟು* jumpTargetp = taskp;
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_taskDisableBegins.ಕಂಡುಹಿಡಿ(taskp);
                ಆದರೆ (it != m_taskDisableBegins.ಕೊನೆ()) jumpTargetp = it->ಎರಡನೇ;
                ಕೂಡಿಸು_Jump_After_Kill(killStmtp, jumpTargetp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(targetp, Fork)) {
            handleDisableOnFork(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(targetp, Begin)) {
            ಆದರೆ (exists_ಖಂಡ_Above(beginp->name())) {
                ಆದರೆ (!beginp->user3()) {
                    // Jump to the end of the named block
                    Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = get_Jump_ಖಂಡ(beginp, ಸುಳ್ಳು);
                    nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstJumpGo{nodep->fileline(), blockp});
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstVar* ಸ್ಥಿರಾಂಕ processQueuep
                        = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                    Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ killStmtp = ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);

                    // process::kill does not terminate the currently running process immediately.
                    // If disable executes inside a fork branch of this named block, jump to the
                    // end of that branch to prevent statements after disable from executing.
                    AstBegin* ಸ್ಥಿರಾಂಕ branchp = innerForkBranchp(beginp);
                    AstBegin* ಸ್ಥಿರಾಂಕ jumpTargetp
                        = branchp ? branchp : m_beginDisableBegins.at(beginp);
                    ಕೂಡಿಸು_Jump_After_Kill(killStmtp, jumpTargetp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstVar* ಸ್ಥಿರಾಂಕ processQueuep
                    = getOrCreateBeginDisableQueuep(beginp, nodep->fileline());
                ಸೇರಿಸು_Kill_Stmtp(nodep, processQueuep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Disable linked with node of unhandled type "
                              << targetp->prettyTypeName());
        }
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        iterateChildren(nodep);
        ಆದರೆ (m_ಒಳ_Fork) {
            nodep->replaceWith(ಹೊಸ Ast_ಮುಕ್ತಾಯ_Fork{nodep->fileline()});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_loopp) {
            // Jump to the end of the loop (post-finish)
            Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = get_Jump_ಖಂಡ(m_loopp, ಸುಳ್ಳು);
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstJumpGo{nodep->fileline(), blockp});
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (m_loopInc && nodep->varp()) nodep->varp()->ಬಳಸಿದ_Loop_Idx(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LinkJumpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkJumpVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Task class functions

ಶೂನ್ಯ V3LinkJump::linkJump(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkJumpVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkjump", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
