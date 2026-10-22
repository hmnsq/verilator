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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Active.h"

#ضمن "V3Const.h"
#ضمن "V3Graph.h"

#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

صنف Latch_Detect_ګراف_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Latch_Detect_ګراف_راس, V3_ګراف_راس)
عام:
    تعداد راس_Type : uint8_t { VT_بلاک, VT_BRANCH, VT_OUTPUT };

خاص:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت راس_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_خوندي {  //   "
        بدل (m_type) {
        حالة VT_بلاک: أرجع "(||)";  // basic block node
        حالة VT_BRANCH: أرجع "(&&)";  // if/else branch mode
        حالة VT_OUTPUT: أرجع "(out)";  // var assignment
        افتراضي: أرجع "??";  // unknown
        }
    }

عام:
    Latch_Detect_ګراف_راس(V3_ګراف* graphp, ثابت string& name, راس_Type type = VT_بلاک)
        : V3_ګراف_راس{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ثابت override VL_MT_STABLE { أرجع m_name + " " + typestr(); }
    string dotColor() ثابت override { أرجع user() ? "green" : "black"; }
    مجازي صحيح type() ثابت { أرجع m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

صنف Latch_Detect_ګراف final : عام V3_ګراف {
محمي:
    Latch_Detect_ګراف_راس* m_curVertexp = تش_اشاره;  // Current latch detection graph vertex
    std::ویکټور<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ساكن Latch_Detect_ګراف_راس* castVertexp(فراغ* vertexp) {
        أرجع reinterpret_cast<Latch_Detect_ګراف_راس*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقي latch_Check_دننی(Latch_Detect_ګراف_راس* vertexp) {
        منطقي پایله = خطأ;
        بدل (vertexp->type()) {
        حالة Latch_Detect_ګراف_راس::VT_OUTPUT:  // Base case
            پایله = vertexp->user();
            اكسر;
        حالة Latch_Detect_ګراف_راس::VT_بلاک:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
                إذا (latch_Check_دننی(castVertexp(څنډه.سر()))) {
                    پایله = ریښتیا;
                    اكسر;
                }
            }
            اكسر;
        حالة Latch_Detect_ګراف_راس::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_ګراف_راس* ثابت ifp = castVertexp(vertexp->بهر_څنډې().frontp()->سر());
            Latch_Detect_ګراف_راس* ثابت elsp = castVertexp(vertexp->بهر_څنډې().backp()->سر());
            پایله = latch_Check_دننی(ifp) && latch_Check_دننی(elsp);
            اكسر;
        }
        vertexp->user(پایله);
        أرجع پایله;
    }

عام:
    Latch_Detect_ګراف() { clear(); }
    ~Latch_Detect_ګراف() override { clear(); }
    // ACCESSORS
    Latch_Detect_ګراف_راس* currentp() { أرجع m_curVertexp; }
    فراغ currentp(Latch_Detect_ګراف_راس* راس) { m_curVertexp = راس; }
    // METHODS
    فراغ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = جديد Latch_Detect_ګراف_راس{هذا, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    فراغ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_ګراف::clear();
        m_curVertexp = تش_اشاره;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_ګراف_راس* جمع_کړه_لار_راس(Latch_Detect_ګراف_راس* پلار, ثابت string& name,
                                          منطقي branch = خطأ) {
        m_curVertexp = جديد Latch_Detect_ګراف_راس{هذا, name,
                                                  branch ? Latch_Detect_ګراف_راس::VT_BRANCH
                                                         : Latch_Detect_ګراف_راس::VT_بلاک};
        جديد V3_ګراف_څنډه{هذا, پلار, m_curVertexp, 1};
        أرجع m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_ګراف_راس* جمع_کړه_Output_راس(AstVarRef* nodep) {
        Latch_Detect_ګراف_راس* ثابت بهر_Vertexp
            = جديد Latch_Detect_ګراف_راس{هذا, nodep->name(), Latch_Detect_ګراف_راس::VT_OUTPUT};
        nodep->varp()->user1p(بهر_Vertexp);
        m_outputs.ټېل_شا(nodep);
        أرجع بهر_Vertexp;
    }
    // Connect an output assignment to its parent control block
    فراغ جمع_کړه_Assignment(AstVarRef* nodep) {
        Latch_Detect_ګراف_راس* بهر_Vertexp;
        إذا (!nodep->varp()->user1p()) {  // Not seen this output before
            بهر_Vertexp = جمع_کړه_Output_راس(nodep);
        } وإلا {
            بهر_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        جديد V3_ګراف_څنډه{هذا, m_curVertexp, بهر_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    فراغ latchCheck(Ast_غوټه* nodep, منطقي latch_expected) {
        منطقي latch_detected = خطأ;
        لكل (ثابت AstVarRef* ثابت vrp : m_outputs) {
            Latch_Detect_ګراف_راس* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(ریښتیا);  // Identify the output vertex we are checking paths _to_
            إذا (!latch_Check_دننی(castVertexp(vertices().frontp()))) latch_detected = ریښتیا;
            إذا (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_نور()
                        << "... Suggest use of always_latch for intentional latches");
                إذا (dump_ګراف_Level() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(خطأ);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        إذا (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

صنف فعال_Namer final : عام VNVisitor {
    // STATE
    AstScope* m_scopep = تش_اشاره;  // Current scope to add statement to
    Ast_فعال* m_sActivep = تش_اشاره;  // For current scope, the Static active we're building
    Ast_فعال* m_iActivep = تش_اشاره;  // For current scope, the Initial active we're building
    Ast_فعال* m_fActivep = تش_اشاره;  // For current scope, the Final active we're building
    Ast_فعال* m_cActivep = تش_اشاره;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_ونه>, Ast_فعال*> m_فعال_Map;

    // METHODS
    فراغ جمع_کړه_فعال(Ast_فعال* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->جمع_کړه_Blocksp(nodep);
    }

    // VISITORS
    فراغ لیدنه(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = تش_اشاره;
        m_iActivep = تش_اشاره;
        m_fActivep = تش_اشاره;
        m_cActivep = تش_اشاره;
        m_فعال_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه_Stmt*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    کينډۍ <ډول_نوم T_Sen_توکی_ډول>
    Ast_فعال*& get_Special_فعال();

عام:
    // METHODS
    AstScope* scopep() { أرجع m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_فعال* make_فعال(File_کرښه* ثابت fl, Ast_Sen_ونه* ثابت senTreep) {
        Ast_فعال* ثابت activep = جديد Ast_فعال{fl, "", senTreep};
        activep->sen_ونه_Storep(activep->sentreep());
        جمع_کړه_فعال(activep);
        أرجع activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    کينډۍ <ډول_نوم T_Sen_توکی_ډول>
    Ast_فعال* make_Special_فعال(File_کرښه* ثابت fl) {
        Ast_Sen_ونه* ثابت senTreep = جديد Ast_Sen_ونه{fl, جديد Ast_Sen_توکی{fl, T_Sen_توکی_ډول{}}};
        أرجع make_فعال(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    کينډۍ <ډول_نوم T_Sen_توکی_ډول>
    Ast_فعال* get_Special_فعال(File_کرښه* fl) {
        Ast_فعال*& cachep = get_Special_فعال<T_Sen_توکی_ډول>();
        إذا (!cachep) cachep = make_Special_فعال<T_Sen_توکی_ډول>(fl);
        أرجع cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_فعال* get_فعال(File_کرښه* fl, Ast_Sen_ونه* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        خپلکاری it = m_فعال_Map.پیدا_کړه(*sentreep);
        // If found matching AstActive, return it
        إذا (it != m_فعال_Map.پای_ټکی()) أرجع it->دوهم;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_ونه* ثابت newsenp = sentreep->clone_ونه(خطأ);
        Ast_فعال* ثابت activep = جديد Ast_فعال{fl, "sequent", newsenp};
        activep->sen_ونه_Storep(activep->sentreep());
        جمع_کړه_فعال(activep);
        m_فعال_Map.emplace(*newsenp, activep);
        أرجع activep;
    }

    // CONSTRUCTORS
    فعال_Namer() = افتراضي;
    ~فعال_Namer() override = افتراضي;
    فراغ اصلي(AstScope* nodep) { iterate(nodep); }
};

کينډۍ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_توکی::Static>() {
    أرجع m_sActivep;
}
کينډۍ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_توکی::Initial>() {
    أرجع m_iActivep;
}
کينډۍ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_توکی::Final>() {
    أرجع m_fActivep;
}
کينډۍ <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_توکی::Combo>() {
    أرجع m_cActivep;
}

//######################################################################
// Latch checking visitor

صنف فعال_Latch_Check_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1_دننه_Use m_inuser1;
    // STATE
    Latch_Detect_ګراف m_ګراف;  // Graph used to detect latches in combo always
    // VISITORS
    فراغ لیدنه(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_کارول_شوی_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_ګراف.جمع_کړه_Assignment(nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        إذا (!nodep->isBoundsCheck()) {
            Latch_Detect_ګراف_راس* ثابت parentp = m_ګراف.currentp();
            Latch_Detect_ګراف_راس* ثابت branchp = m_ګراف.جمع_کړه_لار_راس(parentp, "BRANCH", ریښتیا);
            m_ګراف.جمع_کړه_لار_راس(branchp, "IF");
            iterate_And_بل_Const_Null(nodep->thensp());
            m_ګراف.جمع_کړه_لار_راس(branchp, "ELSE");
            iterate_And_بل_Const_Null(nodep->elsesp());
            m_ګراف.currentp(parentp);
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    فعال_Latch_Check_Visitor(Ast_غوټه* nodep, منطقي expectLatch) {
        m_ګراف.begin();
        iterateConst(nodep);
        m_ګراف.latchCheck(nodep, expectLatch);
    }
    ~فعال_Latch_Check_Visitor() override = افتراضي;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

صنف فعال_Dly_Visitor final : عام VNVisitor {
عام:
    تعداد CheckType : uint8_t { CT_COMB, CT_FINAL };

خاص:
    // MEMBERS
    ثابت CheckType m_check;  // Process type we are checking

    // VISITORS
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        // Issue appropriate warning
        إذا (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } وإلا {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_نور()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(جديد Ast_ټاکل{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_شا(),  //
            nodep->rhsp()->unlink_Fr_شا(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_شا() : تش_اشاره});
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    فعال_Dly_Visitor(Ast_غوټه* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~فعال_Dly_Visitor() override = افتراضي;
};

//######################################################################
// Active class functions

صنف فعال_Visitor final : عام VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    فعال_Namer m_namer;  // Tracking of active names
    منطقي m_clockedProcess = خطأ;  // Whether current process is a clocked process
    منطقي m_all_بدل_شوی = خطأ;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقي m_walkingBody = خطأ;  // Walking body of a process
    منطقي m_canBeComb = خطأ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    کينډۍ <ډول_نوم T>
    فراغ moveUnderSpecial(Ast_غوټه* nodep) {
        Ast_فعال* ثابت wantactivep = m_namer.get_Special_فعال<T>(nodep->fileline());
        nodep->unlink_Fr_شا();
        wantactivep->جمع_کړه_Stmtsp(nodep);
    }

    فراغ لیدنه_Always(Ast_غوټه* nodep, Ast_Sen_ونه* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        إذا (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }

        {
            ثابت VNUser1_دننه_Use user1_دننه_Use;

            // Walk sensitivity list
            m_clockedProcess = خطأ;
            m_all_بدل_شوی = ریښتیا;
            إذا (oldsentreep) {
                oldsentreep->unlink_Fr_شا();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            إذا (m_all_بدل_شوی) {
                ثابت VNUser2_دننه_Use user2_دننه_Use;
                m_walkingBody = ریښتیا;
                m_canBeComb = ریښتیا;
                iterateChildrenConst(nodep);
                m_walkingBody = خطأ;
                إذا (m_canBeComb) m_clockedProcess = خطأ;
            }
        }

        Ast_فعال* ثابت wantactivep
            = !m_clockedProcess ? m_namer.get_Special_فعال<Ast_Sen_توکی::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_فعال(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_فعال<Ast_Sen_توکی::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_شا();
        wantactivep->جمع_کړه_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        إذا (!m_clockedProcess) فعال_Dly_Visitor{nodep, فعال_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        إذا (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_ونه(), oldsentreep);

        // check combinational processes for latches
        إذا (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت فعال_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    فراغ لیدنه_Sen_توکي(Ast_غوټه* nodep) {
        إذا (v3Global.opt.timing().is_سېټ_True()) {
            nodep->foreach([هذا](Ast_Sen_توکی* senItemp) { لیدنه(senItemp); });
        }
    }

    ساكن فراغ mark_Event_څنډې(Ast_Sen_ونه* sentreep) {
        لكل (Ast_Sen_توکی* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_توکی)) {
            إذا (!senip->sensp()) استمر;
            إذا (ثابت Ast_غوټه_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isEvent()) senip->څنډه_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    فراغ لیدنه(AstScope* nodep) override {
        m_namer.اصلي(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    فراغ لیدنه(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_توکی::Static>(nodep); }
    فراغ لیدنه(AstInitial* nodep) override {
        لیدنه_Sen_توکي(nodep);
        moveUnderSpecial<Ast_Sen_توکی::Initial>(nodep);
    }
    فراغ لیدنه(AstFinal* nodep) override {
        ثابت فعال_Dly_Visitor dlyvisitor{nodep, فعال_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_توکی::Final>(nodep);
    }
    فراغ لیدنه(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_توکی::Combo>(nodep); }
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ټاکل) {
            moveUnderSpecial<Ast_Sen_توکی::Combo>(nodep);
            أرجع;
        }
        لیدنه_Sen_توکي(nodep);
        لیدنه_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    فراغ لیدنه(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_Special_فعال<Ast_Sen_توکی::Combo>(nodep->fileline());
        activep->جمع_کړه_Stmtsp(nodep->unlink_Fr_شا());
    }
    فراغ لیدنه(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_څنډې(nodep->sentreep());
        Ast_Sen_ونه* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_شا();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_فعال(nodep->fileline(), sentreep);
        activep->جمع_کړه_Stmtsp(nodep->unlink_Fr_شا());
    }
    فراغ لیدنه(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_څنډې(nodep->sentreep());
        Ast_Sen_ونه* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_شا();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_فعال(nodep->fileline(), sentreep);
        activep->جمع_کړه_Stmtsp(nodep->unlink_Fr_شا());
    }
    فراغ لیدنه(AstCFunc* nodep) override { لیدنه_Sen_توکي(nodep); }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        إذا (!nodep->sensp()) أرجع;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = ریښتیا;
        إذا (nodep->څنډه_Type() != VEdgeType::ET_بدل_شوی) m_all_بدل_شوی = خطأ;

        إذا (ثابت Ast_غوټه_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                إذا (basicp->isEvent()) nodep->څنډه_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(ریښتیا); });
    }

    فراغ لیدنه(AstVarRef* nodep) override {
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOnly()) {
            vscp->user2(ریښتیا);
        } وإلا {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ثابت AstVar* ثابت varp = vscp->varp();
            إذا (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = خطأ;
            }
        }
    }
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) m_clockedProcess = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstFireEvent* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_ټاکل_Force* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstRelease* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstFork* nodep) override {
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = ریښتیا;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    فراغ لیدنه(AstVar*) override {}  // Accelerate
    فراغ لیدنه(AstVarScope*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (!v3Global.opt.timing().is_سېټ_True() && m_walkingBody && !m_canBeComb) {
            أرجع;  // Accelerate
        }
        إذا (!nodep->is_خالص()) m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = ریښتیا;
            أرجع;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند فعال_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~فعال_Visitor() override = افتراضي;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

صنف CovergroupCollectVisitor final : عام VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = تش_اشاره;  // Current covergroup class context, or nullptr

    // VISITORS
    فراغ لیدنه(AstClass* nodep) override {
        إذا (!nodep->isCovergroup()) أرجع;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstScope* nodep) override { iterateChildren(nodep); }

    فراغ لیدنه(AstCFunc* nodep) override {
        إذا (!m_classp) أرجع;
        إذا (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    فراغ لیدنه(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_شا());
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = افتراضي;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

صنف CovergroupEventBindVisitor final : عام VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_سېټ<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (!m_memberps.شمېر(nodep->varp())) أرجع;
        File_کرښه* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    CovergroupEventBindVisitor(Ast_Sen_ونه* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = افتراضي;
};

صنف CovergroupInjectVisitor final : عام VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    فعال_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    فراغ لیدنه(AstScope* nodep) override {
        m_namer.اصلي(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_غوټه_DType* ثابت dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
        إذا (!classRefp) أرجع;

        AstClass* ثابت classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_ونه* ثابت eventp = VN_CAST(classp->user2p(), Sen_ونه);
        إذا (!eventp) أرجع;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ثابت sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_کرښه* ثابت fl = nodep->fileline();
        AstVarRef* ثابت varrefp = جديد AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ثابت cmethodCallp
            = جديد AstCMethodCall{fl, varrefp, sampleCFuncp, تش_اشاره};

        cmethodCallp->dtype_سېټ_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_ونه* senTreep = eventp->clone_ونه(خطأ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_فعال* ثابت activep = m_namer.get_فعال(fl, senTreep);
        VL_DO_DANGLING(ټېل_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->جمع_کړه_Stmtsp(
            جديد AstAlways{fl, VAlwaysKwd::ALWAYS_FF, تش_اشاره, cmethodCallp->makeStmt()});
    }

    فراغ لیدنه(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        إذا (Ast_Sen_ونه* ثابت eventp = VN_CAST(nodep->user2p(), Sen_ونه)) {
            VL_DO_DANGLING(ټېل_Deletep(eventp), eventp);
        }
    }

    فراغ لیدنه(Ast_فعال*) override {}  // Don't iterate into actives
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = افتراضي;
};

//######################################################################
// Active class functions

فراغ V3_فعال::فعال_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { فعال_Visitor{nodep}; }  // Destruct before checking
    إذا (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ثابت VNUser1_دننه_Use user1_دننه_Use;
        ثابت VNUser2_دننه_Use user2_دننه_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_ونه("active", 0, dump_ونه_Either_Level() >= 3);
}
