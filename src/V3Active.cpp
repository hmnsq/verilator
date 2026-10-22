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

صنف Latch_Detect_گراف_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Latch_Detect_گراف_راس, V3_گراف_راس)
عام:
    تعداد راس_Type : uint8_t { VT_بلاڪ, VT_BRANCH, VT_OUTPUT };

خاص:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت راس_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_محفوظ {  //   "
        بدل (m_type) {
        حالة VT_بلاڪ: أرجع "(||)";  // basic block node
        حالة VT_BRANCH: أرجع "(&&)";  // if/else branch mode
        حالة VT_OUTPUT: أرجع "(out)";  // var assignment
        افتراضي: أرجع "??";  // unknown
        }
    }

عام:
    Latch_Detect_گراف_راس(V3_گراف* graphp, ثابت string& name, راس_Type type = VT_بلاڪ)
        : V3_گراف_راس{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ثابت override VL_MT_STABLE { أرجع m_name + " " + typestr(); }
    string dotColor() ثابت override { أرجع user() ? "green" : "black"; }
    مجازي صحيح type() ثابت { أرجع m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

صنف Latch_Detect_گراف final : عام V3_گراف {
محمي:
    Latch_Detect_گراف_راس* m_curVertexp = خالي_اشارو;  // Current latch detection graph vertex
    std::ويڪٽر<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ساكن Latch_Detect_گراف_راس* castVertexp(فراغ* vertexp) {
        أرجع reinterpret_cast<Latch_Detect_گراف_راس*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقي latch_Check_اندروني(Latch_Detect_گراف_راس* vertexp) {
        منطقي نتيجو = خطأ;
        بدل (vertexp->type()) {
        حالة Latch_Detect_گراف_راس::VT_OUTPUT:  // Base case
            نتيجو = vertexp->user();
            اكسر;
        حالة Latch_Detect_گراف_راس::VT_بلاڪ:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
                إذا (latch_Check_اندروني(castVertexp(ڪنارو.چوٽي()))) {
                    نتيجو = سچ;
                    اكسر;
                }
            }
            اكسر;
        حالة Latch_Detect_گراف_راس::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_گراف_راس* ثابت ifp = castVertexp(vertexp->ٻاهريون_ڪنارا().frontp()->چوٽي());
            Latch_Detect_گراف_راس* ثابت elsp = castVertexp(vertexp->ٻاهريون_ڪنارا().backp()->چوٽي());
            نتيجو = latch_Check_اندروني(ifp) && latch_Check_اندروني(elsp);
            اكسر;
        }
        vertexp->user(نتيجو);
        أرجع نتيجو;
    }

عام:
    Latch_Detect_گراف() { clear(); }
    ~Latch_Detect_گراف() override { clear(); }
    // ACCESSORS
    Latch_Detect_گراف_راس* currentp() { أرجع m_curVertexp; }
    فراغ currentp(Latch_Detect_گراف_راس* راس) { m_curVertexp = راس; }
    // METHODS
    فراغ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = جديد Latch_Detect_گراف_راس{هذا, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    فراغ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_گراف::clear();
        m_curVertexp = خالي_اشارو;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_گراف_راس* جوڙيو_رستو_راس(Latch_Detect_گراف_راس* والد, ثابت string& name,
                                          منطقي branch = خطأ) {
        m_curVertexp = جديد Latch_Detect_گراف_راس{هذا, name,
                                                  branch ? Latch_Detect_گراف_راس::VT_BRANCH
                                                         : Latch_Detect_گراف_راس::VT_بلاڪ};
        جديد V3_گراف_ڪنارو{هذا, والد, m_curVertexp, 1};
        أرجع m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_گراف_راس* جوڙيو_Output_راس(AstVarRef* nodep) {
        Latch_Detect_گراف_راس* ثابت ٻاهريون_Vertexp
            = جديد Latch_Detect_گراف_راس{هذا, nodep->name(), Latch_Detect_گراف_راس::VT_OUTPUT};
        nodep->varp()->user1p(ٻاهريون_Vertexp);
        m_outputs.ڌڪو_پٺ(nodep);
        أرجع ٻاهريون_Vertexp;
    }
    // Connect an output assignment to its parent control block
    فراغ جوڙيو_Assignment(AstVarRef* nodep) {
        Latch_Detect_گراف_راس* ٻاهريون_Vertexp;
        إذا (!nodep->varp()->user1p()) {  // Not seen this output before
            ٻاهريون_Vertexp = جوڙيو_Output_راس(nodep);
        } وإلا {
            ٻاهريون_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        جديد V3_گراف_ڪنارو{هذا, m_curVertexp, ٻاهريون_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    فراغ latchCheck(Ast_ڳنڍ* nodep, منطقي latch_expected) {
        منطقي latch_detected = خطأ;
        لكل (ثابت AstVarRef* ثابت vrp : m_outputs) {
            Latch_Detect_گراف_راس* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(سچ);  // Identify the output vertex we are checking paths _to_
            إذا (!latch_Check_اندروني(castVertexp(vertices().frontp()))) latch_detected = سچ;
            إذا (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_وڌيڪ()
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

صنف سرگرم_Namer final : عام VNVisitor {
    // STATE
    AstScope* m_scopep = خالي_اشارو;  // Current scope to add statement to
    Ast_سرگرم* m_sActivep = خالي_اشارو;  // For current scope, the Static active we're building
    Ast_سرگرم* m_iActivep = خالي_اشارو;  // For current scope, the Initial active we're building
    Ast_سرگرم* m_fActivep = خالي_اشارو;  // For current scope, the Final active we're building
    Ast_سرگرم* m_cActivep = خالي_اشارو;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_وڻ>, Ast_سرگرم*> m_سرگرم_Map;

    // METHODS
    فراغ جوڙيو_سرگرم(Ast_سرگرم* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->جوڙيو_Blocksp(nodep);
    }

    // VISITORS
    فراغ گهمو(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = خالي_اشارو;
        m_iActivep = خالي_اشارو;
        m_fActivep = خالي_اشارو;
        m_cActivep = خالي_اشارو;
        m_سرگرم_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    فراغ گهمو(Ast_ڳنڍ_Stmt*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    سانچو <قسم_نالو T_Sen_شي_قسم>
    Ast_سرگرم*& get_Special_سرگرم();

عام:
    // METHODS
    AstScope* scopep() { أرجع m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_سرگرم* make_سرگرم(File_سٽ* ثابت fl, Ast_Sen_وڻ* ثابت senTreep) {
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{fl, "", senTreep};
        activep->sen_وڻ_Storep(activep->sentreep());
        جوڙيو_سرگرم(activep);
        أرجع activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    سانچو <قسم_نالو T_Sen_شي_قسم>
    Ast_سرگرم* make_Special_سرگرم(File_سٽ* ثابت fl) {
        Ast_Sen_وڻ* ثابت senTreep = جديد Ast_Sen_وڻ{fl, جديد Ast_Sen_شي{fl, T_Sen_شي_قسم{}}};
        أرجع make_سرگرم(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    سانچو <قسم_نالو T_Sen_شي_قسم>
    Ast_سرگرم* get_Special_سرگرم(File_سٽ* fl) {
        Ast_سرگرم*& cachep = get_Special_سرگرم<T_Sen_شي_قسم>();
        إذا (!cachep) cachep = make_Special_سرگرم<T_Sen_شي_قسم>(fl);
        أرجع cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_سرگرم* get_سرگرم(File_سٽ* fl, Ast_Sen_وڻ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        خودڪار it = m_سرگرم_Map.ڳوليو(*sentreep);
        // If found matching AstActive, return it
        إذا (it != m_سرگرم_Map.پڇاڙي()) أرجع it->ٻيو;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_وڻ* ثابت newsenp = sentreep->clone_وڻ(خطأ);
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{fl, "sequent", newsenp};
        activep->sen_وڻ_Storep(activep->sentreep());
        جوڙيو_سرگرم(activep);
        m_سرگرم_Map.emplace(*newsenp, activep);
        أرجع activep;
    }

    // CONSTRUCTORS
    سرگرم_Namer() = افتراضي;
    ~سرگرم_Namer() override = افتراضي;
    فراغ مکيه(AstScope* nodep) { iterate(nodep); }
};

سانچو <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شي::Static>() {
    أرجع m_sActivep;
}
سانچو <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شي::Initial>() {
    أرجع m_iActivep;
}
سانچو <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شي::Final>() {
    أرجع m_fActivep;
}
سانچو <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شي::Combo>() {
    أرجع m_cActivep;
}

//######################################################################
// Latch checking visitor

صنف سرگرم_Latch_Check_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1_اندريون_Use m_inuser1;
    // STATE
    Latch_Detect_گراف m_گراف;  // Graph used to detect latches in combo always
    // VISITORS
    فراغ گهمو(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_استعمال_ٿيل_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_گراف.جوڙيو_Assignment(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        إذا (!nodep->isBoundsCheck()) {
            Latch_Detect_گراف_راس* ثابت parentp = m_گراف.currentp();
            Latch_Detect_گراف_راس* ثابت branchp = m_گراف.جوڙيو_رستو_راس(parentp, "BRANCH", سچ);
            m_گراف.جوڙيو_رستو_راس(branchp, "IF");
            iterate_And_ايندڙ_Const_Null(nodep->thensp());
            m_گراف.جوڙيو_رستو_راس(branchp, "ELSE");
            iterate_And_ايندڙ_Const_Null(nodep->elsesp());
            m_گراف.currentp(parentp);
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    سرگرم_Latch_Check_Visitor(Ast_ڳنڍ* nodep, منطقي expectLatch) {
        m_گراف.begin();
        iterateConst(nodep);
        m_گراف.latchCheck(nodep, expectLatch);
    }
    ~سرگرم_Latch_Check_Visitor() override = افتراضي;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

صنف سرگرم_Dly_Visitor final : عام VNVisitor {
عام:
    تعداد CheckType : uint8_t { CT_COMB, CT_FINAL };

خاص:
    // MEMBERS
    ثابت CheckType m_check;  // Process type we are checking

    // VISITORS
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        // Issue appropriate warning
        إذا (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } وإلا {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_وڌيڪ()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(جديد Ast_سونپيو{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_پٺ(),  //
            nodep->rhsp()->unlink_Fr_پٺ(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_پٺ() : خالي_اشارو});
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    سرگرم_Dly_Visitor(Ast_ڳنڍ* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~سرگرم_Dly_Visitor() override = افتراضي;
};

//######################################################################
// Active class functions

صنف سرگرم_Visitor final : عام VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    سرگرم_Namer m_namer;  // Tracking of active names
    منطقي m_clockedProcess = خطأ;  // Whether current process is a clocked process
    منطقي m_all_بدليو = خطأ;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقي m_walkingBody = خطأ;  // Walking body of a process
    منطقي m_canBeComb = خطأ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    سانچو <قسم_نالو T>
    فراغ moveUnderSpecial(Ast_ڳنڍ* nodep) {
        Ast_سرگرم* ثابت wantactivep = m_namer.get_Special_سرگرم<T>(nodep->fileline());
        nodep->unlink_Fr_پٺ();
        wantactivep->جوڙيو_Stmtsp(nodep);
    }

    فراغ گهمو_Always(Ast_ڳنڍ* nodep, Ast_Sen_وڻ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        إذا (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }

        {
            ثابت VNUser1_اندريون_Use user1_اندريون_Use;

            // Walk sensitivity list
            m_clockedProcess = خطأ;
            m_all_بدليو = سچ;
            إذا (oldsentreep) {
                oldsentreep->unlink_Fr_پٺ();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            إذا (m_all_بدليو) {
                ثابت VNUser2_اندريون_Use user2_اندريون_Use;
                m_walkingBody = سچ;
                m_canBeComb = سچ;
                iterateChildrenConst(nodep);
                m_walkingBody = خطأ;
                إذا (m_canBeComb) m_clockedProcess = خطأ;
            }
        }

        Ast_سرگرم* ثابت wantactivep
            = !m_clockedProcess ? m_namer.get_Special_سرگرم<Ast_Sen_شي::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_سرگرم(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_سرگرم<Ast_Sen_شي::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_پٺ();
        wantactivep->جوڙيو_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        إذا (!m_clockedProcess) سرگرم_Dly_Visitor{nodep, سرگرم_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        إذا (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_وڻ(), oldsentreep);

        // check combinational processes for latches
        إذا (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت سرگرم_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    فراغ گهمو_Sen_شيون(Ast_ڳنڍ* nodep) {
        إذا (v3Global.opt.timing().is_سيٽ_True()) {
            nodep->foreach([هذا](Ast_Sen_شي* senItemp) { گهمو(senItemp); });
        }
    }

    ساكن فراغ mark_Event_ڪنارا(Ast_Sen_وڻ* sentreep) {
        لكل (Ast_Sen_شي* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_شي)) {
            إذا (!senip->sensp()) استمر;
            إذا (ثابت Ast_ڳنڍ_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isEvent()) senip->ڪنارو_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    فراغ گهمو(AstScope* nodep) override {
        m_namer.مکيه(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_سرگرم* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    فراغ گهمو(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_شي::Static>(nodep); }
    فراغ گهمو(AstInitial* nodep) override {
        گهمو_Sen_شيون(nodep);
        moveUnderSpecial<Ast_Sen_شي::Initial>(nodep);
    }
    فراغ گهمو(AstFinal* nodep) override {
        ثابت سرگرم_Dly_Visitor dlyvisitor{nodep, سرگرم_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_شي::Final>(nodep);
    }
    فراغ گهمو(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_شي::Combo>(nodep); }
    فراغ گهمو(AstAlways* nodep) override {
        إذا (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }
        إذا (nodep->keyword() == VAlwaysKwd::CONT_سونپيو) {
            moveUnderSpecial<Ast_Sen_شي::Combo>(nodep);
            أرجع;
        }
        گهمو_Sen_شيون(nodep);
        گهمو_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    فراغ گهمو(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_سرگرم* ثابت activep = m_namer.make_Special_سرگرم<Ast_Sen_شي::Combo>(nodep->fileline());
        activep->جوڙيو_Stmtsp(nodep->unlink_Fr_پٺ());
    }
    فراغ گهمو(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_ڪنارا(nodep->sentreep());
        Ast_Sen_وڻ* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_پٺ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_سرگرم* ثابت activep = m_namer.make_سرگرم(nodep->fileline(), sentreep);
        activep->جوڙيو_Stmtsp(nodep->unlink_Fr_پٺ());
    }
    فراغ گهمو(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_ڪنارا(nodep->sentreep());
        Ast_Sen_وڻ* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_پٺ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_سرگرم* ثابت activep = m_namer.make_سرگرم(nodep->fileline(), sentreep);
        activep->جوڙيو_Stmtsp(nodep->unlink_Fr_پٺ());
    }
    فراغ گهمو(AstCFunc* nodep) override { گهمو_Sen_شيون(nodep); }
    فراغ گهمو(Ast_Sen_شي* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        إذا (!nodep->sensp()) أرجع;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = سچ;
        إذا (nodep->ڪنارو_Type() != VEdgeType::ET_بدليو) m_all_بدليو = خطأ;

        إذا (ثابت Ast_ڳنڍ_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                إذا (basicp->isEvent()) nodep->ڪنارو_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(سچ); });
    }

    فراغ گهمو(AstVarRef* nodep) override {
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOnly()) {
            vscp->user2(سچ);
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
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) m_clockedProcess = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstFireEvent* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_سونپيو_Force* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstRelease* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstFork* nodep) override {
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = سچ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    فراغ گهمو(AstVar*) override {}  // Accelerate
    فراغ گهمو(AstVarScope*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (!v3Global.opt.timing().is_سيٽ_True() && m_walkingBody && !m_canBeComb) {
            أرجع;  // Accelerate
        }
        إذا (!nodep->is_خالص()) m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = سچ;
            أرجع;
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح سرگرم_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~سرگرم_Visitor() override = افتراضي;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

صنف CovergroupCollectVisitor final : عام VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = خالي_اشارو;  // Current covergroup class context, or nullptr

    // VISITORS
    فراغ گهمو(AstClass* nodep) override {
        إذا (!nodep->isCovergroup()) أرجع;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    فراغ گهمو(AstScope* nodep) override { iterateChildren(nodep); }

    فراغ گهمو(AstCFunc* nodep) override {
        إذا (!m_classp) أرجع;
        إذا (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    فراغ گهمو(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_پٺ());
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = افتراضي;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

صنف CovergroupEventBindVisitor final : عام VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_سيٽ<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    فراغ گهمو(AstVarRef* nodep) override {
        إذا (!m_memberps.ڳڻپ(nodep->varp())) أرجع;
        File_سٽ* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    CovergroupEventBindVisitor(Ast_Sen_وڻ* eventp, AstClass* classp, AstVarScope* instancep)
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
    سرگرم_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    فراغ گهمو(AstScope* nodep) override {
        m_namer.مکيه(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    فراغ گهمو(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
        إذا (!classRefp) أرجع;

        AstClass* ثابت classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_وڻ* ثابت eventp = VN_CAST(classp->user2p(), Sen_وڻ);
        إذا (!eventp) أرجع;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ثابت sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_سٽ* ثابت fl = nodep->fileline();
        AstVarRef* ثابت varrefp = جديد AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ثابت cmethodCallp
            = جديد AstCMethodCall{fl, varrefp, sampleCFuncp, خالي_اشارو};

        cmethodCallp->dtype_سيٽ_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_وڻ* senTreep = eventp->clone_وڻ(خطأ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_سرگرم* ثابت activep = m_namer.get_سرگرم(fl, senTreep);
        VL_DO_DANGLING(ڌڪو_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->جوڙيو_Stmtsp(
            جديد AstAlways{fl, VAlwaysKwd::ALWAYS_FF, خالي_اشارو, cmethodCallp->makeStmt()});
    }

    فراغ گهمو(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        إذا (Ast_Sen_وڻ* ثابت eventp = VN_CAST(nodep->user2p(), Sen_وڻ)) {
            VL_DO_DANGLING(ڌڪو_Deletep(eventp), eventp);
        }
    }

    فراغ گهمو(Ast_سرگرم*) override {}  // Don't iterate into actives
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = افتراضي;
};

//######################################################################
// Active class functions

فراغ V3_سرگرم::سرگرم_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { سرگرم_Visitor{nodep}; }  // Destruct before checking
    إذا (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;
        ثابت VNUser2_اندريون_Use user2_اندريون_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_وڻ("active", 0, dump_وڻ_Either_Level() >= 3);
}
