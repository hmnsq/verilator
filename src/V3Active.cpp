// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity active domains
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
// V3Active's Transformations:
//
// Note this can be called multiple times.
//   Create a IACTIVE(initial), SACTIVE(combo)
//      ALWAYS: Remove any-edges from sense list
//          If no POS/NEG in senselist, Fold into SACTIVE(combo)
//          Else fold into SACTIVE(sequent).
//          OPTIMIZE: When support async clocks, fold into that active if possible
//      INITIAL: Move into IACTIVE
//      WIRE: Move into SACTIVE(combo)
//
//*************************************************************************

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Active.h"

#شامل "V3Const.h"
#شامل "V3Graph.h"

#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

جماعت Latch_Detect_گراف_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Latch_Detect_گراف_راس, V3_گراف_راس)
عوامی:
    شمار راس_Type : uint8_t { VT_بلاک, VT_BRANCH, VT_OUTPUT };

نجی:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت راس_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_محفوظ {  //   "
        انتخاب (m_type) {
        حالت VT_بلاک: واپس "(||)";  // basic block node
        حالت VT_BRANCH: واپس "(&&)";  // if/else branch mode
        حالت VT_OUTPUT: واپس "(out)";  // var assignment
        طےشدہ: واپس "??";  // unknown
        }
    }

عوامی:
    Latch_Detect_گراف_راس(V3_گراف* graphp, ثابت string& name, راس_Type type = VT_بلاک)
        : V3_گراف_راس{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ثابت override VL_MT_STABLE { واپس m_name + " " + typestr(); }
    string dotColor() ثابت override { واپس user() ? "green" : "black"; }
    مجازی صحیح type() ثابت { واپس m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

جماعت Latch_Detect_گراف final : عوامی V3_گراف {
محفوظ:
    Latch_Detect_گراف_راس* m_curVertexp = خالی_اشاریہ;  // Current latch detection graph vertex
    std::سمتیہ<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ساکن Latch_Detect_گراف_راس* castVertexp(خالی* vertexp) {
        واپس reinterpret_cast<Latch_Detect_گراف_راس*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقی latch_Check_اندرونی(Latch_Detect_گراف_راس* vertexp) {
        منطقی نتیجہ = جھوٹ;
        انتخاب (vertexp->type()) {
        حالت Latch_Detect_گراف_راس::VT_OUTPUT:  // Base case
            نتیجہ = vertexp->user();
            توڑو;
        حالت Latch_Detect_گراف_راس::VT_بلاک:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
                اگر (latch_Check_اندرونی(castVertexp(کنارہ.چوٹی()))) {
                    نتیجہ = سچ;
                    توڑو;
                }
            }
            توڑو;
        حالت Latch_Detect_گراف_راس::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_گراف_راس* ثابت ifp = castVertexp(vertexp->برآمد_کنارے().frontp()->چوٹی());
            Latch_Detect_گراف_راس* ثابت elsp = castVertexp(vertexp->برآمد_کنارے().backp()->چوٹی());
            نتیجہ = latch_Check_اندرونی(ifp) && latch_Check_اندرونی(elsp);
            توڑو;
        }
        vertexp->user(نتیجہ);
        واپس نتیجہ;
    }

عوامی:
    Latch_Detect_گراف() { clear(); }
    ~Latch_Detect_گراف() override { clear(); }
    // ACCESSORS
    Latch_Detect_گراف_راس* currentp() { واپس m_curVertexp; }
    خالی currentp(Latch_Detect_گراف_راس* راس) { m_curVertexp = راس; }
    // METHODS
    خالی begin() {
        // Start a new if/else tracking graph
        m_curVertexp = نیا Latch_Detect_گراف_راس{یہ, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    خالی clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_گراف::clear();
        m_curVertexp = خالی_اشاریہ;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_گراف_راس* جوڑو_راستہ_راس(Latch_Detect_گراف_راس* والد, ثابت string& name,
                                          منطقی branch = جھوٹ) {
        m_curVertexp = نیا Latch_Detect_گراف_راس{یہ, name,
                                                  branch ? Latch_Detect_گراف_راس::VT_BRANCH
                                                         : Latch_Detect_گراف_راس::VT_بلاک};
        نیا V3_گراف_کنارہ{یہ, والد, m_curVertexp, 1};
        واپس m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_گراف_راس* جوڑو_Output_راس(AstVarRef* nodep) {
        Latch_Detect_گراف_راس* ثابت برآمد_Vertexp
            = نیا Latch_Detect_گراف_راس{یہ, nodep->name(), Latch_Detect_گراف_راس::VT_OUTPUT};
        nodep->varp()->user1p(برآمد_Vertexp);
        m_outputs.ڈالو_عقب(nodep);
        واپس برآمد_Vertexp;
    }
    // Connect an output assignment to its parent control block
    خالی جوڑو_Assignment(AstVarRef* nodep) {
        Latch_Detect_گراف_راس* برآمد_Vertexp;
        اگر (!nodep->varp()->user1p()) {  // Not seen this output before
            برآمد_Vertexp = جوڑو_Output_راس(nodep);
        } ورنہ {
            برآمد_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        نیا V3_گراف_کنارہ{یہ, m_curVertexp, برآمد_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    خالی latchCheck(Ast_گرہ* nodep, منطقی latch_expected) {
        منطقی latch_detected = جھوٹ;
        برائے (ثابت AstVarRef* ثابت vrp : m_outputs) {
            Latch_Detect_گراف_راس* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(سچ);  // Identify the output vertex we are checking paths _to_
            اگر (!latch_Check_اندرونی(castVertexp(vertices().frontp()))) latch_detected = سچ;
            اگر (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_مزید()
                        << "... Suggest use of always_latch for intentional latches");
                اگر (dump_گراف_Level() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(جھوٹ);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        اگر (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

جماعت فعال_Namer final : عوامی VNVisitor {
    // STATE
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope to add statement to
    Ast_فعال* m_sActivep = خالی_اشاریہ;  // For current scope, the Static active we're building
    Ast_فعال* m_iActivep = خالی_اشاریہ;  // For current scope, the Initial active we're building
    Ast_فعال* m_fActivep = خالی_اشاریہ;  // For current scope, the Final active we're building
    Ast_فعال* m_cActivep = خالی_اشاریہ;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_درخت>, Ast_فعال*> m_فعال_Map;

    // METHODS
    خالی جوڑو_فعال(Ast_فعال* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->جوڑو_Blocksp(nodep);
    }

    // VISITORS
    خالی دورہ(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = خالی_اشاریہ;
        m_iActivep = خالی_اشاریہ;
        m_fActivep = خالی_اشاریہ;
        m_cActivep = خالی_اشاریہ;
        m_فعال_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    خالی دورہ(Ast_گرہ_Stmt*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    سانچہ <قسم_نام T_Sen_شے_قسم>
    Ast_فعال*& get_Special_فعال();

عوامی:
    // METHODS
    AstScope* scopep() { واپس m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_فعال* make_فعال(File_سطر* ثابت fl, Ast_Sen_درخت* ثابت senTreep) {
        Ast_فعال* ثابت activep = نیا Ast_فعال{fl, "", senTreep};
        activep->sen_درخت_Storep(activep->sentreep());
        جوڑو_فعال(activep);
        واپس activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    سانچہ <قسم_نام T_Sen_شے_قسم>
    Ast_فعال* make_Special_فعال(File_سطر* ثابت fl) {
        Ast_Sen_درخت* ثابت senTreep = نیا Ast_Sen_درخت{fl, نیا Ast_Sen_شے{fl, T_Sen_شے_قسم{}}};
        واپس make_فعال(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    سانچہ <قسم_نام T_Sen_شے_قسم>
    Ast_فعال* get_Special_فعال(File_سطر* fl) {
        Ast_فعال*& cachep = get_Special_فعال<T_Sen_شے_قسم>();
        اگر (!cachep) cachep = make_Special_فعال<T_Sen_شے_قسم>(fl);
        واپس cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_فعال* get_فعال(File_سطر* fl, Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        خودکار it = m_فعال_Map.ڈھونڈو(*sentreep);
        // If found matching AstActive, return it
        اگر (it != m_فعال_Map.انتہا()) واپس it->دوسرا;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_درخت* ثابت newsenp = sentreep->clone_درخت(جھوٹ);
        Ast_فعال* ثابت activep = نیا Ast_فعال{fl, "sequent", newsenp};
        activep->sen_درخت_Storep(activep->sentreep());
        جوڑو_فعال(activep);
        m_فعال_Map.emplace(*newsenp, activep);
        واپس activep;
    }

    // CONSTRUCTORS
    فعال_Namer() = طےشدہ;
    ~فعال_Namer() override = طےشدہ;
    خالی مین(AstScope* nodep) { iterate(nodep); }
};

سانچہ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_شے::Static>() {
    واپس m_sActivep;
}
سانچہ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_شے::Initial>() {
    واپس m_iActivep;
}
سانچہ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_شے::Final>() {
    واپس m_fActivep;
}
سانچہ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_شے::Combo>() {
    واپس m_cActivep;
}

//######################################################################
// Latch checking visitor

جماعت فعال_Latch_Check_Visitor final : عوامی VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1_اندر_Use m_inuser1;
    // STATE
    Latch_Detect_گراف m_گراف;  // Graph used to detect latches in combo always
    // VISITORS
    خالی دورہ(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        اگر (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_مستعمل_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_گراف.جوڑو_Assignment(nodep);
        }
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        اگر (!nodep->isBoundsCheck()) {
            Latch_Detect_گراف_راس* ثابت parentp = m_گراف.currentp();
            Latch_Detect_گراف_راس* ثابت branchp = m_گراف.جوڑو_راستہ_راس(parentp, "BRANCH", سچ);
            m_گراف.جوڑو_راستہ_راس(branchp, "IF");
            iterate_And_اگلا_Const_Null(nodep->thensp());
            m_گراف.جوڑو_راستہ_راس(branchp, "ELSE");
            iterate_And_اگلا_Const_Null(nodep->elsesp());
            m_گراف.currentp(parentp);
        } ورنہ {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    // CONSTRUCTORS
    فعال_Latch_Check_Visitor(Ast_گرہ* nodep, منطقی expectLatch) {
        m_گراف.begin();
        iterateConst(nodep);
        m_گراف.latchCheck(nodep, expectLatch);
    }
    ~فعال_Latch_Check_Visitor() override = طےشدہ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

جماعت فعال_Dly_Visitor final : عوامی VNVisitor {
عوامی:
    شمار CheckType : uint8_t { CT_COMB, CT_FINAL };

نجی:
    // MEMBERS
    ثابت CheckType m_check;  // Process type we are checking

    // VISITORS
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        // Issue appropriate warning
        اگر (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } ورنہ {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_مزید()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(نیا Ast_تفویض{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_عقب(),  //
            nodep->rhsp()->unlink_Fr_عقب(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_عقب() : خالی_اشاریہ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    فعال_Dly_Visitor(Ast_گرہ* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~فعال_Dly_Visitor() override = طےشدہ;
};

//######################################################################
// Active class functions

جماعت فعال_Visitor final : عوامی VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    فعال_Namer m_namer;  // Tracking of active names
    منطقی m_clockedProcess = جھوٹ;  // Whether current process is a clocked process
    منطقی m_all_بدلا = جھوٹ;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقی m_walkingBody = جھوٹ;  // Walking body of a process
    منطقی m_canBeComb = جھوٹ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    سانچہ <قسم_نام T>
    خالی moveUnderSpecial(Ast_گرہ* nodep) {
        Ast_فعال* ثابت wantactivep = m_namer.get_Special_فعال<T>(nodep->fileline());
        nodep->unlink_Fr_عقب();
        wantactivep->جوڑو_Stmtsp(nodep);
    }

    خالی دورہ_Always(Ast_گرہ* nodep, Ast_Sen_درخت* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        اگر (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }

        {
            ثابت VNUser1_اندر_Use user1_اندر_Use;

            // Walk sensitivity list
            m_clockedProcess = جھوٹ;
            m_all_بدلا = سچ;
            اگر (oldsentreep) {
                oldsentreep->unlink_Fr_عقب();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            اگر (m_all_بدلا) {
                ثابت VNUser2_اندر_Use user2_اندر_Use;
                m_walkingBody = سچ;
                m_canBeComb = سچ;
                iterateChildrenConst(nodep);
                m_walkingBody = جھوٹ;
                اگر (m_canBeComb) m_clockedProcess = جھوٹ;
            }
        }

        Ast_فعال* ثابت wantactivep
            = !m_clockedProcess ? m_namer.get_Special_فعال<Ast_Sen_شے::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_فعال(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_فعال<Ast_Sen_شے::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_عقب();
        wantactivep->جوڑو_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        اگر (!m_clockedProcess) فعال_Dly_Visitor{nodep, فعال_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        اگر (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_درخت(), oldsentreep);

        // check combinational processes for latches
        اگر (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت فعال_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    خالی دورہ_Sen_اشیاء(Ast_گرہ* nodep) {
        اگر (v3Global.opt.timing().is_سیٹ_True()) {
            nodep->foreach([یہ](Ast_Sen_شے* senItemp) { دورہ(senItemp); });
        }
    }

    ساکن خالی mark_Event_کنارے(Ast_Sen_درخت* sentreep) {
        برائے (Ast_Sen_شے* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_شے)) {
            اگر (!senip->sensp()) جاری;
            اگر (ثابت Ast_گرہ_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                اگر (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    اگر (basicp->isEvent()) senip->کنارہ_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    خالی دورہ(AstScope* nodep) override {
        m_namer.مین(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_فعال* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    خالی دورہ(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_شے::Static>(nodep); }
    خالی دورہ(AstInitial* nodep) override {
        دورہ_Sen_اشیاء(nodep);
        moveUnderSpecial<Ast_Sen_شے::Initial>(nodep);
    }
    خالی دورہ(AstFinal* nodep) override {
        ثابت فعال_Dly_Visitor dlyvisitor{nodep, فعال_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_شے::Final>(nodep);
    }
    خالی دورہ(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_شے::Combo>(nodep); }
    خالی دورہ(AstAlways* nodep) override {
        اگر (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        اگر (nodep->keyword() == VAlwaysKwd::CONT_تفویض) {
            moveUnderSpecial<Ast_Sen_شے::Combo>(nodep);
            واپس;
        }
        دورہ_Sen_اشیاء(nodep);
        دورہ_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    خالی دورہ(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_Special_فعال<Ast_Sen_شے::Combo>(nodep->fileline());
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_عقب());
    }
    خالی دورہ(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_کنارے(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_عقب();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_فعال(nodep->fileline(), sentreep);
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_عقب());
    }
    خالی دورہ(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_کنارے(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_عقب();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_فعال(nodep->fileline(), sentreep);
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_عقب());
    }
    خالی دورہ(AstCFunc* nodep) override { دورہ_Sen_اشیاء(nodep); }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        اگر (!nodep->sensp()) واپس;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = سچ;
        اگر (nodep->کنارہ_Type() != VEdgeType::ET_بدلا) m_all_بدلا = جھوٹ;

        اگر (ثابت Ast_گرہ_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            اگر (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                اگر (basicp->isEvent()) nodep->کنارہ_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(سچ); });
    }

    خالی دورہ(AstVarRef* nodep) override {
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (nodep->access().isWriteOnly()) {
            vscp->user2(سچ);
        } ورنہ {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ثابت AstVar* ثابت varp = vscp->varp();
            اگر (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = جھوٹ;
            }
        }
    }
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        m_canBeComb = جھوٹ;
        اگر (nodep->isTimingControl()) m_clockedProcess = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstFireEvent* nodep) override {
        m_canBeComb = جھوٹ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_تفویض_Force* nodep) override {
        m_canBeComb = جھوٹ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstRelease* nodep) override {
        m_canBeComb = جھوٹ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstFork* nodep) override {
        اگر (nodep->isTimingControl()) {
            m_canBeComb = جھوٹ;
            m_clockedProcess = سچ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    خالی دورہ(AstVar*) override {}  // Accelerate
    خالی دورہ(AstVarScope*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (!v3Global.opt.timing().is_سیٹ_True() && m_walkingBody && !m_canBeComb) {
            واپس;  // Accelerate
        }
        اگر (!nodep->is_خالص()) m_canBeComb = جھوٹ;
        اگر (nodep->isTimingControl()) {
            m_canBeComb = جھوٹ;
            m_clockedProcess = سچ;
            واپس;
        }
        iterateChildren(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح فعال_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~فعال_Visitor() override = طےشدہ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

جماعت CovergroupCollectVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = خالی_اشاریہ;  // Current covergroup class context, or nullptr

    // VISITORS
    خالی دورہ(AstClass* nodep) override {
        اگر (!nodep->isCovergroup()) واپس;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    خالی دورہ(AstScope* nodep) override { iterateChildren(nodep); }

    خالی دورہ(AstCFunc* nodep) override {
        اگر (!m_classp) واپس;
        اگر (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    خالی دورہ(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_عقب());
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = طےشدہ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

جماعت CovergroupEventBindVisitor final : عوامی VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_سیٹ<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_memberps.گنتی(nodep->varp())) واپس;
        File_سطر* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = نیا AstMemberSel{fl, نیا AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    CovergroupEventBindVisitor(Ast_Sen_درخت* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = طےشدہ;
};

جماعت CovergroupInjectVisitor final : عوامی VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    فعال_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    خالی دورہ(AstScope* nodep) override {
        m_namer.مین(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    خالی دورہ(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
        اگر (!classRefp) واپس;

        AstClass* ثابت classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_درخت* ثابت eventp = VN_CAST(classp->user2p(), Sen_درخت);
        اگر (!eventp) واپس;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ثابت sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_سطر* ثابت fl = nodep->fileline();
        AstVarRef* ثابت varrefp = نیا AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ثابت cmethodCallp
            = نیا AstCMethodCall{fl, varrefp, sampleCFuncp, خالی_اشاریہ};

        cmethodCallp->dtype_سیٹ_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_درخت* senTreep = eventp->clone_درخت(جھوٹ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_فعال* ثابت activep = m_namer.get_فعال(fl, senTreep);
        VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->جوڑو_Stmtsp(
            نیا AstAlways{fl, VAlwaysKwd::ALWAYS_FF, خالی_اشاریہ, cmethodCallp->makeStmt()});
    }

    خالی دورہ(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        اگر (Ast_Sen_درخت* ثابت eventp = VN_CAST(nodep->user2p(), Sen_درخت)) {
            VL_DO_DANGLING(ڈالو_Deletep(eventp), eventp);
        }
    }

    خالی دورہ(Ast_فعال*) override {}  // Don't iterate into actives
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = طےشدہ;
};

//######################################################################
// Active class functions

خالی V3_فعال::فعال_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { فعال_Visitor{nodep}; }  // Destruct before checking
    اگر (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ثابت VNUser1_اندر_Use user1_اندر_Use;
        ثابت VNUser2_اندر_Use user2_اندر_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_درخت("active", 0, dump_درخت_Either_Level() >= 3);
}
