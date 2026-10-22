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

صنف Latch_Detect_گراف_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Latch_Detect_گراف_رأس, V3_گراف_رأس)
عام:
    تعداد رأس_Type : uint8_t { VT_بلوک, VT_BRANCH, VT_OUTPUT };

خاص:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت رأس_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_امن {  //   "
        بدل (m_type) {
        حالة VT_بلوک: أرجع "(||)";  // basic block node
        حالة VT_BRANCH: أرجع "(&&)";  // if/else branch mode
        حالة VT_OUTPUT: أرجع "(out)";  // var assignment
        افتراضي: أرجع "??";  // unknown
        }
    }

عام:
    Latch_Detect_گراف_رأس(V3_گراف* graphp, ثابت string& name, رأس_Type type = VT_بلوک)
        : V3_گراف_رأس{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ثابت override VL_MT_STABLE { أرجع m_name + " " + typestr(); }
    string dotColor() ثابت override { أرجع user() ? "green" : "black"; }
    مجازی صحيح type() ثابت { أرجع m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

صنف Latch_Detect_گراف final : عام V3_گراف {
محمي:
    Latch_Detect_گراف_رأس* m_curVertexp = اشاره_تهی;  // Current latch detection graph vertex
    std::وکتور<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ساكن Latch_Detect_گراف_رأس* castVertexp(فراغ* vertexp) {
        أرجع reinterpret_cast<Latch_Detect_گراف_رأس*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقي latch_Check_داخلی(Latch_Detect_گراف_رأس* vertexp) {
        منطقي نتیجه = خطأ;
        بدل (vertexp->type()) {
        حالة Latch_Detect_گراف_رأس::VT_OUTPUT:  // Base case
            نتیجه = vertexp->user();
            اكسر;
        حالة Latch_Detect_گراف_رأس::VT_بلوک:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
                إذا (latch_Check_داخلی(castVertexp(یال.بالاترین()))) {
                    نتیجه = درست;
                    اكسر;
                }
            }
            اكسر;
        حالة Latch_Detect_گراف_رأس::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_گراف_رأس* ثابت ifp = castVertexp(vertexp->بیرون_یالها().frontp()->بالاترین());
            Latch_Detect_گراف_رأس* ثابت elsp = castVertexp(vertexp->بیرون_یالها().backp()->بالاترین());
            نتیجه = latch_Check_داخلی(ifp) && latch_Check_داخلی(elsp);
            اكسر;
        }
        vertexp->user(نتیجه);
        أرجع نتیجه;
    }

عام:
    Latch_Detect_گراف() { clear(); }
    ~Latch_Detect_گراف() override { clear(); }
    // ACCESSORS
    Latch_Detect_گراف_رأس* currentp() { أرجع m_curVertexp; }
    فراغ currentp(Latch_Detect_گراف_رأس* رأس) { m_curVertexp = رأس; }
    // METHODS
    فراغ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = جديد Latch_Detect_گراف_رأس{هذا, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    فراغ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_گراف::clear();
        m_curVertexp = اشاره_تهی;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_گراف_رأس* جمع_کن_مسیر_رأس(Latch_Detect_گراف_رأس* والد, ثابت string& name,
                                          منطقي branch = خطأ) {
        m_curVertexp = جديد Latch_Detect_گراف_رأس{هذا, name,
                                                  branch ? Latch_Detect_گراف_رأس::VT_BRANCH
                                                         : Latch_Detect_گراف_رأس::VT_بلوک};
        جديد V3_گراف_یال{هذا, والد, m_curVertexp, 1};
        أرجع m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_گراف_رأس* جمع_کن_Output_رأس(AstVarRef* nodep) {
        Latch_Detect_گراف_رأس* ثابت بیرون_Vertexp
            = جديد Latch_Detect_گراف_رأس{هذا, nodep->name(), Latch_Detect_گراف_رأس::VT_OUTPUT};
        nodep->varp()->user1p(بیرون_Vertexp);
        m_outputs.افزودن_پشت(nodep);
        أرجع بیرون_Vertexp;
    }
    // Connect an output assignment to its parent control block
    فراغ جمع_کن_Assignment(AstVarRef* nodep) {
        Latch_Detect_گراف_رأس* بیرون_Vertexp;
        إذا (!nodep->varp()->user1p()) {  // Not seen this output before
            بیرون_Vertexp = جمع_کن_Output_رأس(nodep);
        } وإلا {
            بیرون_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        جديد V3_گراف_یال{هذا, m_curVertexp, بیرون_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    فراغ latchCheck(Ast_گره* nodep, منطقي latch_expected) {
        منطقي latch_detected = خطأ;
        لكل (ثابت AstVarRef* ثابت vrp : m_outputs) {
            Latch_Detect_گراف_رأس* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(درست);  // Identify the output vertex we are checking paths _to_
            إذا (!latch_Check_داخلی(castVertexp(vertices().frontp()))) latch_detected = درست;
            إذا (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_بیشتر()
                        << "... Suggest use of always_latch for intentional latches");
                إذا (dump_گراف_Level() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
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
    AstScope* m_scopep = اشاره_تهی;  // Current scope to add statement to
    Ast_فعال* m_sActivep = اشاره_تهی;  // For current scope, the Static active we're building
    Ast_فعال* m_iActivep = اشاره_تهی;  // For current scope, the Initial active we're building
    Ast_فعال* m_fActivep = اشاره_تهی;  // For current scope, the Final active we're building
    Ast_فعال* m_cActivep = اشاره_تهی;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_درخت>, Ast_فعال*> m_فعال_Map;

    // METHODS
    فراغ جمع_کن_فعال(Ast_فعال* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->جمع_کن_Blocksp(nodep);
    }

    // VISITORS
    فراغ بازدید(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = اشاره_تهی;
        m_iActivep = اشاره_تهی;
        m_fActivep = اشاره_تهی;
        m_cActivep = اشاره_تهی;
        m_فعال_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    فراغ بازدید(Ast_Sen_درخت* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    فراغ بازدید(Ast_گره_Stmt*) override {}  // Accelerate
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    قالب <نام_نوع T_Sen_قلم_نوع>
    Ast_فعال*& get_Special_فعال();

عام:
    // METHODS
    AstScope* scopep() { أرجع m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_فعال* make_فعال(File_سطر* ثابت fl, Ast_Sen_درخت* ثابت senTreep) {
        Ast_فعال* ثابت activep = جديد Ast_فعال{fl, "", senTreep};
        activep->sen_درخت_Storep(activep->sentreep());
        جمع_کن_فعال(activep);
        أرجع activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    قالب <نام_نوع T_Sen_قلم_نوع>
    Ast_فعال* make_Special_فعال(File_سطر* ثابت fl) {
        Ast_Sen_درخت* ثابت senTreep = جديد Ast_Sen_درخت{fl, جديد Ast_Sen_قلم{fl, T_Sen_قلم_نوع{}}};
        أرجع make_فعال(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    قالب <نام_نوع T_Sen_قلم_نوع>
    Ast_فعال* get_Special_فعال(File_سطر* fl) {
        Ast_فعال*& cachep = get_Special_فعال<T_Sen_قلم_نوع>();
        إذا (!cachep) cachep = make_Special_فعال<T_Sen_قلم_نوع>(fl);
        أرجع cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_فعال* get_فعال(File_سطر* fl, Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        خودکار it = m_فعال_Map.پیدا_کن(*sentreep);
        // If found matching AstActive, return it
        إذا (it != m_فعال_Map.انتها()) أرجع it->دومی;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_درخت* ثابت newsenp = sentreep->clone_درخت(خطأ);
        Ast_فعال* ثابت activep = جديد Ast_فعال{fl, "sequent", newsenp};
        activep->sen_درخت_Storep(activep->sentreep());
        جمع_کن_فعال(activep);
        m_فعال_Map.emplace(*newsenp, activep);
        أرجع activep;
    }

    // CONSTRUCTORS
    فعال_Namer() = افتراضي;
    ~فعال_Namer() override = افتراضي;
    فراغ اصلی(AstScope* nodep) { iterate(nodep); }
};

قالب <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_قلم::Static>() {
    أرجع m_sActivep;
}
قالب <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_قلم::Initial>() {
    أرجع m_iActivep;
}
قالب <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_قلم::Final>() {
    أرجع m_fActivep;
}
قالب <>
Ast_فعال*& فعال_Namer::get_Special_فعال<Ast_Sen_قلم::Combo>() {
    أرجع m_cActivep;
}

//######################################################################
// Latch checking visitor

صنف فعال_Latch_Check_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1_درون_Use m_inuser1;
    // STATE
    Latch_Detect_گراف m_گراف;  // Graph used to detect latches in combo always
    // VISITORS
    فراغ بازدید(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_استفاده_شده_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_گراف.جمع_کن_Assignment(nodep);
        }
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        إذا (!nodep->isBoundsCheck()) {
            Latch_Detect_گراف_رأس* ثابت parentp = m_گراف.currentp();
            Latch_Detect_گراف_رأس* ثابت branchp = m_گراف.جمع_کن_مسیر_رأس(parentp, "BRANCH", درست);
            m_گراف.جمع_کن_مسیر_رأس(branchp, "IF");
            iterate_And_بعدی_Const_Null(nodep->thensp());
            m_گراف.جمع_کن_مسیر_رأس(branchp, "ELSE");
            iterate_And_بعدی_Const_Null(nodep->elsesp());
            m_گراف.currentp(parentp);
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    فعال_Latch_Check_Visitor(Ast_گره* nodep, منطقي expectLatch) {
        m_گراف.begin();
        iterateConst(nodep);
        m_گراف.latchCheck(nodep, expectLatch);
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
    فراغ بازدید(Ast_تخصیص_بده_Dly* nodep) override {
        // Issue appropriate warning
        إذا (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } وإلا {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_بیشتر()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(جديد Ast_تخصیص_بده{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_پشت(),  //
            nodep->rhsp()->unlink_Fr_پشت(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_پشت() : اشاره_تهی});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    فعال_Dly_Visitor(Ast_گره* nodep, CheckType check)
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
    منطقي m_all_تغییریافته = خطأ;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقي m_walkingBody = خطأ;  // Walking body of a process
    منطقي m_canBeComb = خطأ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    قالب <نام_نوع T>
    فراغ moveUnderSpecial(Ast_گره* nodep) {
        Ast_فعال* ثابت wantactivep = m_namer.get_Special_فعال<T>(nodep->fileline());
        nodep->unlink_Fr_پشت();
        wantactivep->جمع_کن_Stmtsp(nodep);
    }

    فراغ بازدید_Always(Ast_گره* nodep, Ast_Sen_درخت* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        إذا (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }

        {
            ثابت VNUser1_درون_Use user1_درون_Use;

            // Walk sensitivity list
            m_clockedProcess = خطأ;
            m_all_تغییریافته = درست;
            إذا (oldsentreep) {
                oldsentreep->unlink_Fr_پشت();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            إذا (m_all_تغییریافته) {
                ثابت VNUser2_درون_Use user2_درون_Use;
                m_walkingBody = درست;
                m_canBeComb = درست;
                iterateChildrenConst(nodep);
                m_walkingBody = خطأ;
                إذا (m_canBeComb) m_clockedProcess = خطأ;
            }
        }

        Ast_فعال* ثابت wantactivep
            = !m_clockedProcess ? m_namer.get_Special_فعال<Ast_Sen_قلم::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_فعال(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_فعال<Ast_Sen_قلم::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_پشت();
        wantactivep->جمع_کن_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        إذا (!m_clockedProcess) فعال_Dly_Visitor{nodep, فعال_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        إذا (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_درخت(), oldsentreep);

        // check combinational processes for latches
        إذا (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت فعال_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    فراغ بازدید_Sen_اقلام(Ast_گره* nodep) {
        إذا (v3Global.opt.timing().is_مجموعه_True()) {
            nodep->foreach([هذا](Ast_Sen_قلم* senItemp) { بازدید(senItemp); });
        }
    }

    ساكن فراغ mark_Event_یالها(Ast_Sen_درخت* sentreep) {
        لكل (Ast_Sen_قلم* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_قلم)) {
            إذا (!senip->sensp()) استمر;
            إذا (ثابت Ast_گره_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isEvent()) senip->یال_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    فراغ بازدید(AstScope* nodep) override {
        m_namer.اصلی(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_فعال* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    فراغ بازدید(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_قلم::Static>(nodep); }
    فراغ بازدید(AstInitial* nodep) override {
        بازدید_Sen_اقلام(nodep);
        moveUnderSpecial<Ast_Sen_قلم::Initial>(nodep);
    }
    فراغ بازدید(AstFinal* nodep) override {
        ثابت فعال_Dly_Visitor dlyvisitor{nodep, فعال_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_قلم::Final>(nodep);
    }
    فراغ بازدید(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_قلم::Combo>(nodep); }
    فراغ بازدید(AstAlways* nodep) override {
        إذا (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }
        إذا (nodep->keyword() == VAlwaysKwd::CONT_تخصیص_بده) {
            moveUnderSpecial<Ast_Sen_قلم::Combo>(nodep);
            أرجع;
        }
        بازدید_Sen_اقلام(nodep);
        بازدید_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    فراغ بازدید(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_Special_فعال<Ast_Sen_قلم::Combo>(nodep->fileline());
        activep->جمع_کن_Stmtsp(nodep->unlink_Fr_پشت());
    }
    فراغ بازدید(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_یالها(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_پشت();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_فعال(nodep->fileline(), sentreep);
        activep->جمع_کن_Stmtsp(nodep->unlink_Fr_پشت());
    }
    فراغ بازدید(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_یالها(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_پشت();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_فعال* ثابت activep = m_namer.make_فعال(nodep->fileline(), sentreep);
        activep->جمع_کن_Stmtsp(nodep->unlink_Fr_پشت());
    }
    فراغ بازدید(AstCFunc* nodep) override { بازدید_Sen_اقلام(nodep); }
    فراغ بازدید(Ast_Sen_قلم* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        إذا (!nodep->sensp()) أرجع;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = درست;
        إذا (nodep->یال_Type() != VEdgeType::ET_تغییریافته) m_all_تغییریافته = خطأ;

        إذا (ثابت Ast_گره_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                إذا (basicp->isEvent()) nodep->یال_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(درست); });
    }

    فراغ بازدید(AstVarRef* nodep) override {
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOnly()) {
            vscp->user2(درست);
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
    فراغ بازدید(Ast_تخصیص_بده_Dly* nodep) override {
        m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) m_clockedProcess = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstFireEvent* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_تخصیص_بده_Force* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstRelease* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstFork* nodep) override {
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = درست;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    فراغ بازدید(AstVar*) override {}  // Accelerate
    فراغ بازدید(AstVarScope*) override {}  // Accelerate
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (!v3Global.opt.timing().is_مجموعه_True() && m_walkingBody && !m_canBeComb) {
            أرجع;  // Accelerate
        }
        إذا (!nodep->is_خالص()) m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = درست;
            أرجع;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    صریح فعال_Visitor(AstNetlist* nodep) { iterate(nodep); }
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
    AstClass* m_classp = اشاره_تهی;  // Current covergroup class context, or nullptr

    // VISITORS
    فراغ بازدید(AstClass* nodep) override {
        إذا (!nodep->isCovergroup()) أرجع;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    فراغ بازدید(AstScope* nodep) override { iterateChildren(nodep); }

    فراغ بازدید(AstCFunc* nodep) override {
        إذا (!m_classp) أرجع;
        إذا (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    فراغ بازدید(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_پشت());
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = افتراضي;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

صنف CovergroupEventBindVisitor final : عام VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_مجموعه<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_memberps.شمار(nodep->varp())) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    CovergroupEventBindVisitor(Ast_Sen_درخت* eventp, AstClass* classp, AstVarScope* instancep)
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
    فراغ بازدید(AstScope* nodep) override {
        m_namer.اصلی(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    فراغ بازدید(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
        إذا (!classRefp) أرجع;

        AstClass* ثابت classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_درخت* ثابت eventp = VN_CAST(classp->user2p(), Sen_درخت);
        إذا (!eventp) أرجع;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ثابت sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_سطر* ثابت fl = nodep->fileline();
        AstVarRef* ثابت varrefp = جديد AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ثابت cmethodCallp
            = جديد AstCMethodCall{fl, varrefp, sampleCFuncp, اشاره_تهی};

        cmethodCallp->dtype_مجموعه_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_درخت* senTreep = eventp->clone_درخت(خطأ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_فعال* ثابت activep = m_namer.get_فعال(fl, senTreep);
        VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->جمع_کن_Stmtsp(
            جديد AstAlways{fl, VAlwaysKwd::ALWAYS_FF, اشاره_تهی, cmethodCallp->makeStmt()});
    }

    فراغ بازدید(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        إذا (Ast_Sen_درخت* ثابت eventp = VN_CAST(nodep->user2p(), Sen_درخت)) {
            VL_DO_DANGLING(افزودن_Deletep(eventp), eventp);
        }
    }

    فراغ بازدید(Ast_فعال*) override {}  // Don't iterate into actives
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
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
        ثابت VNUser1_درون_Use user1_درون_Use;
        ثابت VNUser2_درون_Use user2_درون_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_درخت("active", 0, dump_درخت_Either_Level() >= 3);
}
