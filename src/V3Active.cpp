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
    تعداد راس_Type : uint8_t { VT_بلاک, VT_BRANCH, VT_OUTPUT };

خاص:
    ثابت string m_name;  // Only used for .dot file generation
    ثابت راس_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ثابت VL_MT_محفوظ {  //   "
        بدل (m_type) {
        حالة VT_بلاک: أرجع "(||)";  // basic block node
        حالة VT_BRANCH: أرجع "(&&)";  // if/else branch mode
        حالة VT_OUTPUT: أرجع "(out)";  // var assignment
        افتراضي: أرجع "??";  // unknown
        }
    }

عام:
    Latch_Detect_گراف_راس(V3_گراف* graphp, ثابت string& name, راس_Type type = VT_بلاک)
        : V3_گراف_راس{graphp}
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
    Latch_Detect_گراف_راس* m_curVertexp = خالی_سوچک;  // Current latch detection graph vertex
    std::سمتیہ<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ساكن Latch_Detect_گراف_راس* castVertexp(فراغ* vertexp) {
        أرجع reinterpret_cast<Latch_Detect_گراف_راس*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    منطقي latch_Check_اندرونی(Latch_Detect_گراف_راس* vertexp) {
        منطقي نتیجہ = خطأ;
        بدل (vertexp->type()) {
        حالة Latch_Detect_گراف_راس::VT_OUTPUT:  // Base case
            نتیجہ = vertexp->user();
            اكسر;
        حالة Latch_Detect_گراف_راس::VT_بلاک:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
                إذا (latch_Check_اندرونی(castVertexp(کنارا.چوٹی()))) {
                    نتیجہ = سچ;
                    اكسر;
                }
            }
            اكسر;
        حالة Latch_Detect_گراف_راس::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_گراف_راس* ثابت ifp = castVertexp(vertexp->باہرلا_کنارے().frontp()->چوٹی());
            Latch_Detect_گراف_راس* ثابت elsp = castVertexp(vertexp->باہرلا_کنارے().backp()->چوٹی());
            نتیجہ = latch_Check_اندرونی(ifp) && latch_Check_اندرونی(elsp);
            اكسر;
        }
        vertexp->user(نتیجہ);
        أرجع نتیجہ;
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
        m_curVertexp = خالی_سوچک;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_گراف_راس* جوڑو_رستہ_راس(Latch_Detect_گراف_راس* ماپا, ثابت string& name,
                                          منطقي branch = خطأ) {
        m_curVertexp = جديد Latch_Detect_گراف_راس{هذا, name,
                                                  branch ? Latch_Detect_گراف_راس::VT_BRANCH
                                                         : Latch_Detect_گراف_راس::VT_بلاک};
        جديد V3_گراف_کنارا{هذا, ماپا, m_curVertexp, 1};
        أرجع m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_گراف_راس* جوڑو_Output_راس(AstVarRef* nodep) {
        Latch_Detect_گراف_راس* ثابت باہرلا_Vertexp
            = جديد Latch_Detect_گراف_راس{هذا, nodep->name(), Latch_Detect_گراف_راس::VT_OUTPUT};
        nodep->varp()->user1p(باہرلا_Vertexp);
        m_outputs.ڈالو_پٹھ(nodep);
        أرجع باہرلا_Vertexp;
    }
    // Connect an output assignment to its parent control block
    فراغ جوڑو_Assignment(AstVarRef* nodep) {
        Latch_Detect_گراف_راس* باہرلا_Vertexp;
        إذا (!nodep->varp()->user1p()) {  // Not seen this output before
            باہرلا_Vertexp = جوڑو_Output_راس(nodep);
        } وإلا {
            باہرلا_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        جديد V3_گراف_کنارا{هذا, m_curVertexp, باہرلا_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    فراغ latchCheck(Ast_گرہ* nodep, منطقي latch_expected) {
        منطقي latch_detected = خطأ;
        لكل (ثابت AstVarRef* ثابت vrp : m_outputs) {
            Latch_Detect_گراف_راس* ثابت vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(سچ);  // Identify the output vertex we are checking paths _to_
            إذا (!latch_Check_اندرونی(castVertexp(vertices().frontp()))) latch_detected = سچ;
            إذا (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_ہور()
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
    AstScope* m_scopep = خالی_سوچک;  // Current scope to add statement to
    Ast_سرگرم* m_sActivep = خالی_سوچک;  // For current scope, the Static active we're building
    Ast_سرگرم* m_iActivep = خالی_سوچک;  // For current scope, the Initial active we're building
    Ast_سرگرم* m_fActivep = خالی_سوچک;  // For current scope, the Final active we're building
    Ast_سرگرم* m_cActivep = خالی_سوچک;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_درخت>, Ast_سرگرم*> m_سرگرم_Map;

    // METHODS
    فراغ جوڑو_سرگرم(Ast_سرگرم* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->جوڑو_Blocksp(nodep);
    }

    // VISITORS
    فراغ ویکھو(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = خالی_سوچک;
        m_iActivep = خالی_سوچک;
        m_fActivep = خالی_سوچک;
        m_cActivep = خالی_سوچک;
        m_سرگرم_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    فراغ ویکھو(Ast_Sen_درخت* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    فراغ ویکھو(Ast_گرہ_Stmt*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    سانچا <قسم_ناں T_Sen_شے_قسم>
    Ast_سرگرم*& get_Special_سرگرم();

عام:
    // METHODS
    AstScope* scopep() { أرجع m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_سرگرم* make_سرگرم(File_سطر* ثابت fl, Ast_Sen_درخت* ثابت senTreep) {
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{fl, "", senTreep};
        activep->sen_درخت_Storep(activep->sentreep());
        جوڑو_سرگرم(activep);
        أرجع activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    سانچا <قسم_ناں T_Sen_شے_قسم>
    Ast_سرگرم* make_Special_سرگرم(File_سطر* ثابت fl) {
        Ast_Sen_درخت* ثابت senTreep = جديد Ast_Sen_درخت{fl, جديد Ast_Sen_شے{fl, T_Sen_شے_قسم{}}};
        أرجع make_سرگرم(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    سانچا <قسم_ناں T_Sen_شے_قسم>
    Ast_سرگرم* get_Special_سرگرم(File_سطر* fl) {
        Ast_سرگرم*& cachep = get_Special_سرگرم<T_Sen_شے_قسم>();
        إذا (!cachep) cachep = make_Special_سرگرم<T_Sen_شے_قسم>(fl);
        أرجع cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_سرگرم* get_سرگرم(File_سطر* fl, Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        خودکار it = m_سرگرم_Map.لبھو(*sentreep);
        // If found matching AstActive, return it
        إذا (it != m_سرگرم_Map.سرا()) أرجع it->دوجا;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_درخت* ثابت newsenp = sentreep->clone_درخت(خطأ);
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{fl, "sequent", newsenp};
        activep->sen_درخت_Storep(activep->sentreep());
        جوڑو_سرگرم(activep);
        m_سرگرم_Map.emplace(*newsenp, activep);
        أرجع activep;
    }

    // CONSTRUCTORS
    سرگرم_Namer() = افتراضي;
    ~سرگرم_Namer() override = افتراضي;
    فراغ مکھ(AstScope* nodep) { iterate(nodep); }
};

سانچا <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شے::Static>() {
    أرجع m_sActivep;
}
سانچا <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شے::Initial>() {
    أرجع m_iActivep;
}
سانچا <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شے::Final>() {
    أرجع m_fActivep;
}
سانچا <>
Ast_سرگرم*& سرگرم_Namer::get_Special_سرگرم<Ast_Sen_شے::Combo>() {
    أرجع m_cActivep;
}

//######################################################################
// Latch checking visitor

صنف سرگرم_Latch_Check_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ثابت VNUser1_وچ_Use m_inuser1;
    // STATE
    Latch_Detect_گراف m_گراف;  // Graph used to detect latches in combo always
    // VISITORS
    فراغ ویکھو(AstVarRef* nodep) override {
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_ورتیا_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_گراف.جوڑو_Assignment(nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        إذا (!nodep->isBoundsCheck()) {
            Latch_Detect_گراف_راس* ثابت parentp = m_گراف.currentp();
            Latch_Detect_گراف_راس* ثابت branchp = m_گراف.جوڑو_رستہ_راس(parentp, "BRANCH", سچ);
            m_گراف.جوڑو_رستہ_راس(branchp, "IF");
            iterate_And_اگلا_Const_Null(nodep->thensp());
            m_گراف.جوڑو_رستہ_راس(branchp, "ELSE");
            iterate_And_اگلا_Const_Null(nodep->elsesp());
            m_گراف.currentp(parentp);
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    سرگرم_Latch_Check_Visitor(Ast_گرہ* nodep, منطقي expectLatch) {
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
    فراغ ویکھو(Ast_سونپو_Dly* nodep) override {
        // Issue appropriate warning
        إذا (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } وإلا {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_ہور()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(جديد Ast_سونپو{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_پٹھ(),  //
            nodep->rhsp()->unlink_Fr_پٹھ(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_پٹھ() : خالی_سوچک});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    سرگرم_Dly_Visitor(Ast_گرہ* nodep, CheckType check)
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
    منطقي m_all_بدلیا = خطأ;  // Whether all SenItem in the SenTree are ET_CHANGED
    منطقي m_walkingBody = خطأ;  // Walking body of a process
    منطقي m_canBeComb = خطأ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    سانچا <قسم_ناں T>
    فراغ moveUnderSpecial(Ast_گرہ* nodep) {
        Ast_سرگرم* ثابت wantactivep = m_namer.get_Special_سرگرم<T>(nodep->fileline());
        nodep->unlink_Fr_پٹھ();
        wantactivep->جوڑو_Stmtsp(nodep);
    }

    فراغ ویکھو_Always(Ast_گرہ* nodep, Ast_Sen_درخت* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        إذا (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
            أرجع;
        }

        {
            ثابت VNUser1_وچ_Use user1_وچ_Use;

            // Walk sensitivity list
            m_clockedProcess = خطأ;
            m_all_بدلیا = سچ;
            إذا (oldsentreep) {
                oldsentreep->unlink_Fr_پٹھ();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            إذا (m_all_بدلیا) {
                ثابت VNUser2_وچ_Use user2_وچ_Use;
                m_walkingBody = سچ;
                m_canBeComb = سچ;
                iterateChildrenConst(nodep);
                m_walkingBody = خطأ;
                إذا (m_canBeComb) m_clockedProcess = خطأ;
            }
        }

        Ast_سرگرم* ثابت wantactivep
            = !m_clockedProcess ? m_namer.get_Special_سرگرم<Ast_Sen_شے::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_سرگرم(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_سرگرم<Ast_Sen_شے::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_پٹھ();
        wantactivep->جوڑو_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        إذا (!m_clockedProcess) سرگرم_Dly_Visitor{nodep, سرگرم_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        إذا (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_درخت(), oldsentreep);

        // check combinational processes for latches
        إذا (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ثابت سرگرم_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    فراغ ویکھو_Sen_اشیاء(Ast_گرہ* nodep) {
        إذا (v3Global.opt.timing().is_سیٹ_True()) {
            nodep->foreach([هذا](Ast_Sen_شے* senItemp) { ویکھو(senItemp); });
        }
    }

    ساكن فراغ mark_Event_کنارے(Ast_Sen_درخت* sentreep) {
        لكل (Ast_Sen_شے* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_شے)) {
            إذا (!senip->sensp()) استمر;
            إذا (ثابت Ast_گرہ_DType* ثابت dtypep = senip->sensp()->dtypep()) {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isEvent()) senip->کنارا_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    فراغ ویکھو(AstScope* nodep) override {
        m_namer.مکھ(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    فراغ ویکھو(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_شے::Static>(nodep); }
    فراغ ویکھو(AstInitial* nodep) override {
        ویکھو_Sen_اشیاء(nodep);
        moveUnderSpecial<Ast_Sen_شے::Initial>(nodep);
    }
    فراغ ویکھو(AstFinal* nodep) override {
        ثابت سرگرم_Dly_Visitor dlyvisitor{nodep, سرگرم_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_شے::Final>(nodep);
    }
    فراغ ویکھو(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_شے::Combo>(nodep); }
    فراغ ویکھو(AstAlways* nodep) override {
        إذا (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
            أرجع;
        }
        إذا (nodep->keyword() == VAlwaysKwd::CONT_سونپو) {
            moveUnderSpecial<Ast_Sen_شے::Combo>(nodep);
            أرجع;
        }
        ویکھو_Sen_اشیاء(nodep);
        ویکھو_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    فراغ ویکھو(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_سرگرم* ثابت activep = m_namer.make_Special_سرگرم<Ast_Sen_شے::Combo>(nodep->fileline());
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_پٹھ());
    }
    فراغ ویکھو(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_کنارے(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_پٹھ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_سرگرم* ثابت activep = m_namer.make_سرگرم(nodep->fileline(), sentreep);
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_پٹھ());
    }
    فراغ ویکھو(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_کنارے(nodep->sentreep());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        sentreep->unlink_Fr_پٹھ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_سرگرم* ثابت activep = m_namer.make_سرگرم(nodep->fileline(), sentreep);
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_پٹھ());
    }
    فراغ ویکھو(AstCFunc* nodep) override { ویکھو_Sen_اشیاء(nodep); }
    فراغ ویکھو(Ast_Sen_شے* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        إذا (!nodep->sensp()) أرجع;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = سچ;
        إذا (nodep->کنارا_Type() != VEdgeType::ET_بدلیا) m_all_بدلیا = خطأ;

        إذا (ثابت Ast_گرہ_DType* ثابت dtypep = nodep->sensp()->dtypep()) {
            إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                إذا (basicp->isEvent()) nodep->کنارا_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ثابت AstVarRef* refp) { refp->varScopep()->user1(سچ); });
    }

    فراغ ویکھو(AstVarRef* nodep) override {
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
    فراغ ویکھو(Ast_سونپو_Dly* nodep) override {
        m_canBeComb = خطأ;
        إذا (nodep->isTimingControl()) m_clockedProcess = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstFireEvent* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_سونپو_Force* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstRelease* nodep) override {
        m_canBeComb = خطأ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstFork* nodep) override {
        إذا (nodep->isTimingControl()) {
            m_canBeComb = خطأ;
            m_clockedProcess = سچ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    فراغ ویکھو(AstVar*) override {}  // Accelerate
    فراغ ویکھو(AstVarScope*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (!v3Global.opt.timing().is_سیٹ_True() && m_walkingBody && !m_canBeComb) {
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
    AstClass* m_classp = خالی_سوچک;  // Current covergroup class context, or nullptr

    // VISITORS
    فراغ ویکھو(AstClass* nodep) override {
        إذا (!nodep->isCovergroup()) أرجع;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstScope* nodep) override { iterateChildren(nodep); }

    فراغ ویکھو(AstCFunc* nodep) override {
        إذا (!m_classp) أرجع;
        إذا (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    فراغ ویکھو(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_پٹھ());
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = افتراضي;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

صنف CovergroupEventBindVisitor final : عام VNVisitor {
    AstVarScope* ثابت m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_سیٹ<ثابت AstVar*> m_memberps;  // Non-static variables in the covergroup class

    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (!m_memberps.گنتی(nodep->varp())) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

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
    سرگرم_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    فراغ ویکھو(AstScope* nodep) override {
        m_namer.مکھ(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep();
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
            = جديد AstCMethodCall{fl, varrefp, sampleCFuncp, خالی_سوچک};

        cmethodCallp->dtype_سیٹ_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_درخت* senTreep = eventp->clone_درخت(خطأ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_سرگرم* ثابت activep = m_namer.get_سرگرم(fl, senTreep);
        VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->جوڑو_Stmtsp(
            جديد AstAlways{fl, VAlwaysKwd::ALWAYS_FF, خالی_سوچک, cmethodCallp->makeStmt()});
    }

    فراغ ویکھو(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        إذا (Ast_Sen_درخت* ثابت eventp = VN_CAST(nodep->user2p(), Sen_درخت)) {
            VL_DO_DANGLING(ڈالو_Deletep(eventp), eventp);
        }
    }

    فراغ ویکھو(Ast_سرگرم*) override {}  // Don't iterate into actives
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

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
        ثابت VNUser1_وچ_Use user1_وچ_Use;
        ثابت VNUser2_وچ_Use user2_وچ_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_درخت("active", 0, dump_درخت_Either_Level() >= 3);
}
