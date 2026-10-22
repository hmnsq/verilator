// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Make lookup tables
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
// TABLE TRANSFORMATIONS:
//      Look at all large always and assignments.
//      Count # of input bits and # of output bits, and # of statements
//      If high # of statements relative to inpbits*outbits,
//      replace with lookup table
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Table.h"

#ضمن "V3ConstPool.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Simulate.h"
#ضمن "V3Stats.h"

#ضمن <cmath>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
ساكن constexpr صحيح جدول_MAX_بایټونه = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ساكن constexpr صحيح جدول_ټول_بایټونه = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ساكن constexpr صحيح جدول_SPACE_وخت_MULT = 8;
// If < 32 instructions, not worth the effort
ساكن constexpr صحيح جدول_MIN_غوټه_شمېر = 32;
// Assume an instruction is 4 bytes
ساكن constexpr صحيح جدول_بایټونه_فی_INST = 4;

//######################################################################

صنف جدول_Visitor;

صنف جدول_Simulate_Visitor final : عام SimulateVisitor {
    // MEMBERS
    جدول_Visitor* ثابت m_cbthis;  ///< Class for callback

عام:
    ///< Call other-this function on all new var references
    فراغ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    څرګند جدول_Simulate_Visitor(جدول_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~جدول_Simulate_Visitor() override = افتراضي;
};

//######################################################################
// Class for holding lookup table state during construction

صنف جدول_Builder final {
    File_کرښه* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = تش_اشاره;  // The lookup table initializer values
    AstVarScope* m_varScopep = تش_اشاره;  // The scoped variable holding the table

عام:
    څرګند جدول_Builder(File_کرښه* fl)
        : m_fl{fl} {}

    ~جدول_Builder() {
        إذا (m_consp) VL_DO_DANGLING(m_consp->delete_ونه(), m_consp);
    }

    فراغ سېټ_جدول_اندازه(Ast_غوټه_DType* elemDType, بینښه اندازه) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(اندازه > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحيح پلنوالی = elemDType->پلنوالی();
        Ast_غوټه_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->پیدا_کړه_بټ_DType(پلنوالی, پلنوالی, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت جدول_DTypep = جديد AstUnpackArrayDType{
            m_fl, subDTypep, جديد AstRange{m_fl, static_cast<صحيح>(اندازه), 0}};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(جدول_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? جديد AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? جديد AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : جديد AstConst{m_fl, AstConst::Widthed_ارزښت{}, پلنوالی, 0};
        m_consp = جديد AstConsArray{m_fl, جدول_DTypep, defaultp};
    }

    فراغ جمع_کړه_ارزښت(بینښه شاخص, ثابت V3_شمېره& ارزښت) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        إذا (ارزښت.isString() ? ارزښت.منزل_String().تشه() : ارزښت.isEqZero()) أرجع;
        m_consp->valuep(شاخص, جديد AstConst{m_fl, ارزښت});
    }

    AstVarScope* varScopep() {
        إذا (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::پیدا_کړه(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_ونه(), refp);
        }
        أرجع m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

صنف جدول_Output_Var final {
    AstVarScope* ثابت m_varScopep;  // The output variable
    ثابت بینښه m_ord;  // Output ordinal number in this block
    منطقي m_mayBeUnassigned = خطأ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    جدول_Builder m_جدول_Builder;

عام:
    جدول_Output_Var(AstVarScope* varScopep, بینښه ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_جدول_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    string name() ثابت { أرجع varScopep()->varp()->name(); }
    بینښه ord() ثابت { أرجع m_ord; }
    فراغ سېټ_May_Be_Unassigned() { m_mayBeUnassigned = ریښتیا; }
    منطقي mayBeUnassigned() ثابت { أرجع m_mayBeUnassigned; }
    فراغ سېټ_جدول_اندازه(بینښه اندازه) { m_جدول_Builder.سېټ_جدول_اندازه(varScopep()->dtypep(), اندازه); }
    فراغ جمع_کړه_ارزښت(بینښه شاخص, ثابت V3_شمېره& ارزښت) { m_جدول_Builder.جمع_کړه_ارزښت(شاخص, ارزښت); }
    AstVarScope* tabeVarScopep() { أرجع m_جدول_Builder.varScopep(); }
};

//######################################################################
// Table class functions

صنف جدول_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    مزدوج m_ټول_بایټونه = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_شاخص_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = تش_اشاره;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقي m_ټاکل_Dly = خطأ;  // Consists of delayed assignments instead of normal assignments
    بینښه m_دننه_پلنوالی_بټونه = 0;  // Input table width - in bits
    بینښه m_بهر_پلنوالی_بایټونه = 0;  // Output table width - in bytes
    std::ویکټور<AstVarScope*> m_دننه_Varps;  // Input variable list
    std::ویکټور<جدول_Output_Var> m_بهر_Varps;  // Output variable list

    // METHODS

عام:
    فراغ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_بهر_پلنوالی_بایټونه += nodep->varp()->dtypeSkipRefp()->پلنوالی_ټول_بایټونه();
            m_بهر_Varps.emplace_شا(vscp, static_cast<بینښه>(m_بهر_Varps.اندازه()));
        }
        إذا (nodep->access().isReadOrRW()) {
            m_دننه_پلنوالی_بټونه += nodep->varp()->پلنوالی();
            m_دننه_Varps.ټېل_شا(vscp);
        }
    }

خاص:
    منطقي ونه_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_دننه_پلنوالی_بټونه = 0;
        m_بهر_پلنوالی_بایټونه = 0;
        m_دننه_Varps.clear();
        m_بهر_Varps.clear();

        // Collect stats
        جدول_Simulate_Visitor chkvis{هذا};
        chkvis.main_جدول_Check(nodep);
        m_ټاکل_Dly = chkvis.is_ټاکل_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت اندازه_t chg_پلنوالی = m_بهر_Varps.اندازه();
        ثابت مزدوج space = std::توان<مزدوج>(2.0, m_دننه_پلنوالی_بټونه) * (m_بهر_پلنوالی_بایټونه + chg_پلنوالی);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت مزدوج وخت_2  // max(_, 1), so we won't divide by zero
            = std::max<مزدوج>(chkvis.instr_شمېر() * جدول_بایټونه_فی_INST + chkvis.معلومات_شمېر(), 1);
        إذا (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        إذا (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        إذا (!m_بهر_پلنوالی_بایټونه || !m_دننه_پلنوالی_بټونه) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        إذا (chkvis.instr_شمېر() < جدول_MIN_غوټه_شمېر) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        إذا (space > جدول_MAX_بایټونه) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        إذا (space > وخت_2 * جدول_SPACE_وخت_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        إذا (m_ټول_بایټونه > جدول_ټول_بایټونه) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_شمېر() << " Data=" << chkvis.معلومات_شمېر()
                                << " in width (bits)=" << m_دننه_پلنوالی_بټونه << " out width (bytes)="
                                << m_بهر_پلنوالی_بایټونه << " Spacetime=" << (space / وخت_2) << "("
                                << space << "/" << وخت_2 << ")" << ": " << nodep);
        إذا (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / وخت_2) << " " << nodep);
            m_ټول_بایټونه += space;
        }
        أرجع chkvis.optimizable();
    }

    فراغ replace_With_جدول(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_کرښه* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت شاخص_Vscp = m_شاخص_Tmps.make(fl, m_scopep, m_دننه_پلنوالی_بټونه);

        // The 'output assigned' table builder
        جدول_Builder output_Assigned_جدول_Builder{fl};
        output_Assigned_جدول_Builder.سېټ_جدول_اندازه(
            nodep->پیدا_کړه_بټ_DType(m_بهر_Varps.اندازه(), m_بهر_Varps.اندازه(), VSigning::UNSIGNED),
            VL_ماسک_I(m_دننه_پلنوالی_بټونه));

        // Set sizes of output tables
        لكل (جدول_Output_Var& tov : m_بهر_Varps) tov.سېټ_جدول_اندازه(VL_ماسک_I(m_دننه_پلنوالی_بټونه));

        // Populate the tables
        createTables(nodep, output_Assigned_جدول_Builder);

        Ast_غوټه* ثابت stmtsp = create_Lookup_ننوت(fl, شاخص_Vscp);
        createOutputAssigns(nodep, stmtsp, شاخص_Vscp, output_Assigned_جدول_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_شا_With_بل()->delete_ونه();
        nodep->جمع_کړه_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    فراغ createTables(AstAlways* nodep, جدول_Builder& output_Assigned_جدول_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        جدول_Simulate_Visitor simvis{هذا};
        لكل (uint32_t i = 0; i <= VL_ماسک_I(m_دننه_پلنوالی_بټونه); ++i) {
            ثابت uint32_t دننه_ارزښت = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << دننه_ارزښت);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t ښويدنه_2 = 0;
            لكل (AstVarScope* invscp : m_دننه_Varps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_ارزښت{}, invscp->پلنوالی(),
                                    VL_ماسک_I(invscp->پلنوالی()) & (دننه_ارزښت >> ښويدنه_2)};
                simvis.new_ارزښت(invscp, &cnst);
                ښويدنه_2 += invscp->پلنوالی();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(ښويدنه_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_جدول_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_پیغام());

            // Build output value tables and the assigned flags table
            V3_شمېره output_Assigned_ماسک{nodep, static_cast<صحيح>(m_بهر_Varps.اندازه()), 0};
            لكل (جدول_Output_Var& tov : m_بهر_Varps) {
                إذا (V3_شمېره* ثابت outnump = simvis.fetch_بهر_شمېره_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_ماسک.سېټ_بټ(tov.ord(), 1);  // Mark output as assigned
                    tov.جمع_کړه_ارزښت(دننه_ارزښت, *outnump);
                } وإلا {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.سېټ_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_جدول_Builder.جمع_کړه_ارزښت(دننه_ارزښت, output_Assigned_ماسک);
        }  // each value
    }

    Ast_غوټه* create_Lookup_ننوت(File_کرښه* fl, AstVarScope* شاخص_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_غوټه_بیان* concatp = تش_اشاره;
        لكل (AstVarScope* invscp : m_دننه_Varps) {
            AstVarRef* ثابت refp = جديد AstVarRef{fl, invscp, VAccess::READ};
            إذا (concatp) {
                concatp = جديد AstConcat{fl, refp, concatp};
            } وإلا {
                concatp = refp;
            }
        }

        أرجع جديد Ast_ټاکل{fl, جديد AstVarRef{fl, شاخص_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_کرښه* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = جديد AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت شاخص_Refp = جديد AstVarRef{fl, indexp, VAccess::READ};
        أرجع جديد AstArraySel{fl, fromRefp, شاخص_Refp};
    }

    فراغ createOutputAssigns(Ast_غوټه* nodep, Ast_غوټه* stmtsp, AstVarScope* شاخص_Vscp,
                             AstVarScope* output_Assigned_جدول_Vscp) {
        File_کرښه* ثابت fl = nodep->fileline();
        لكل (جدول_Output_Var& tov : m_بهر_Varps) {
            Ast_غوټه_بیان* ثابت alhsp = جديد AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_غوټه_بیان* ثابت arhsp = select(fl, tov.tabeVarScopep(), شاخص_Vscp);
            Ast_غوټه* outsetp = m_ټاکل_Dly
                                   ? static_cast<Ast_غوټه*>(جديد Ast_ټاکل_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_غوټه*>(جديد Ast_ټاکل{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            إذا (tov.mayBeUnassigned()) {
                V3_شمېره output_Chg_ماسک{nodep, static_cast<صحيح>(m_بهر_Varps.اندازه()), 0};
                output_Chg_ماسک.سېټ_بټ(tov.ord(), 1);
                Ast_غوټه_بیان* ثابت condp
                    = جديد AstNeq{fl,
                                 جديد AstAnd{fl, select(fl, output_Assigned_جدول_Vscp, شاخص_Vscp),
                                            جديد AstConst{fl, output_Chg_ماسک}},
                                 جديد AstConst{fl, AstConst::Widthed_ارزښت{},
                                              static_cast<صحيح>(m_بهر_Varps.اندازه()), 0}};
                outsetp = جديد AstIf{fl, condp, outsetp};
            }

            stmtsp->جمع_کړه_بل(outsetp);
        }
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        إذا (ونه_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_جدول(nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عام:
    // CONSTRUCTORS
    څرګند جدول_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~جدول_Visitor() override {  //
        V3Stats::جمع_کړه_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

فراغ جدول_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

فراغ V3_جدول::جدول_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { جدول_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("table", 0, dump_ونه_Either_Level() >= 3);
}
