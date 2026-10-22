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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Table.h"

#شامل "V3ConstPool.h"
#شامل "V3SharedTmps.h"
#شامل "V3Simulate.h"
#شامل "V3Stats.h"

#شامل <cmath>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
ساکن constexpr صحیح جدول_MAX_بائٹیں = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ساکن constexpr صحیح جدول_کل_بائٹیں = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ساکن constexpr صحیح جدول_SPACE_وقت_MULT = 8;
// If < 32 instructions, not worth the effort
ساکن constexpr صحیح جدول_MIN_گرہ_گنتی = 32;
// Assume an instruction is 4 bytes
ساکن constexpr صحیح جدول_بائٹیں_فی_INST = 4;

//######################################################################

جماعت جدول_Visitor;

جماعت جدول_Simulate_Visitor final : عوامی SimulateVisitor {
    // MEMBERS
    جدول_Visitor* ثابت m_cbthis;  ///< Class for callback

عوامی:
    ///< Call other-this function on all new var references
    خالی varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    صریح جدول_Simulate_Visitor(جدول_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~جدول_Simulate_Visitor() override = طےشدہ;
};

//######################################################################
// Class for holding lookup table state during construction

جماعت جدول_Builder final {
    File_سطر* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = خالی_اشاریہ;  // The lookup table initializer values
    AstVarScope* m_varScopep = خالی_اشاریہ;  // The scoped variable holding the table

عوامی:
    صریح جدول_Builder(File_سطر* fl)
        : m_fl{fl} {}

    ~جدول_Builder() {
        اگر (m_consp) VL_DO_DANGLING(m_consp->delete_درخت(), m_consp);
    }

    خالی سیٹ_جدول_سائز(Ast_گرہ_DType* elemDType, غیرعلامتی سائز) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(سائز > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحیح چوڑائی = elemDType->چوڑائی();
        Ast_گرہ_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->ڈھونڈو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت جدول_DTypep = نیا AstUnpackArrayDType{
            m_fl, subDTypep, نیا AstRange{m_fl, static_cast<صحیح>(سائز), 0}};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(جدول_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? نیا AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? نیا AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : نیا AstConst{m_fl, AstConst::Widthed_قدر{}, چوڑائی, 0};
        m_consp = نیا AstConsArray{m_fl, جدول_DTypep, defaultp};
    }

    خالی جوڑو_قدر(غیرعلامتی اشاریہ, ثابت V3_نمبر& قدر) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        اگر (قدر.isString() ? قدر.مقصد_String().خالی_خانے() : قدر.isEqZero()) واپس;
        m_consp->valuep(اشاریہ, نیا AstConst{m_fl, قدر});
    }

    AstVarScope* varScopep() {
        اگر (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::ڈھونڈو(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_درخت(), refp);
        }
        واپس m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

جماعت جدول_Output_Var final {
    AstVarScope* ثابت m_varScopep;  // The output variable
    ثابت غیرعلامتی m_ord;  // Output ordinal number in this block
    منطقی m_mayBeUnassigned = جھوٹ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    جدول_Builder m_جدول_Builder;

عوامی:
    جدول_Output_Var(AstVarScope* varScopep, غیرعلامتی ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_جدول_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { واپس m_varScopep; }
    string name() ثابت { واپس varScopep()->varp()->name(); }
    غیرعلامتی ord() ثابت { واپس m_ord; }
    خالی سیٹ_May_Be_Unassigned() { m_mayBeUnassigned = سچ; }
    منطقی mayBeUnassigned() ثابت { واپس m_mayBeUnassigned; }
    خالی سیٹ_جدول_سائز(غیرعلامتی سائز) { m_جدول_Builder.سیٹ_جدول_سائز(varScopep()->dtypep(), سائز); }
    خالی جوڑو_قدر(غیرعلامتی اشاریہ, ثابت V3_نمبر& قدر) { m_جدول_Builder.جوڑو_قدر(اشاریہ, قدر); }
    AstVarScope* tabeVarScopep() { واپس m_جدول_Builder.varScopep(); }
};

//######################################################################
// Table class functions

جماعت جدول_Visitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    دوہرا m_کل_بائٹیں = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_اشاریہ_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = خالی_اشاریہ;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقی m_تفویض_Dly = جھوٹ;  // Consists of delayed assignments instead of normal assignments
    غیرعلامتی m_اندر_چوڑائی_بٹ = 0;  // Input table width - in bits
    غیرعلامتی m_برآمد_چوڑائی_بائٹیں = 0;  // Output table width - in bytes
    std::سمتیہ<AstVarScope*> m_اندر_Varps;  // Input variable list
    std::سمتیہ<جدول_Output_Var> m_برآمد_Varps;  // Output variable list

    // METHODS

عوامی:
    خالی simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_برآمد_چوڑائی_بائٹیں += nodep->varp()->dtypeSkipRefp()->چوڑائی_کل_بائٹیں();
            m_برآمد_Varps.emplace_عقب(vscp, static_cast<غیرعلامتی>(m_برآمد_Varps.سائز()));
        }
        اگر (nodep->access().isReadOrRW()) {
            m_اندر_چوڑائی_بٹ += nodep->varp()->چوڑائی();
            m_اندر_Varps.ڈالو_عقب(vscp);
        }
    }

نجی:
    منطقی درخت_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_اندر_چوڑائی_بٹ = 0;
        m_برآمد_چوڑائی_بائٹیں = 0;
        m_اندر_Varps.clear();
        m_برآمد_Varps.clear();

        // Collect stats
        جدول_Simulate_Visitor chkvis{یہ};
        chkvis.main_جدول_Check(nodep);
        m_تفویض_Dly = chkvis.is_تفویض_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت سائز_t chg_چوڑائی = m_برآمد_Varps.سائز();
        ثابت دوہرا space = std::قوت<دوہرا>(2.0, m_اندر_چوڑائی_بٹ) * (m_برآمد_چوڑائی_بائٹیں + chg_چوڑائی);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت دوہرا وقت_2  // max(_, 1), so we won't divide by zero
            = std::max<دوہرا>(chkvis.instr_گنتی() * جدول_بائٹیں_فی_INST + chkvis.ڈیٹا_گنتی(), 1);
        اگر (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        اگر (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        اگر (!m_برآمد_چوڑائی_بائٹیں || !m_اندر_چوڑائی_بٹ) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        اگر (chkvis.instr_گنتی() < جدول_MIN_گرہ_گنتی) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        اگر (space > جدول_MAX_بائٹیں) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        اگر (space > وقت_2 * جدول_SPACE_وقت_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        اگر (m_کل_بائٹیں > جدول_کل_بائٹیں) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_گنتی() << " Data=" << chkvis.ڈیٹا_گنتی()
                                << " in width (bits)=" << m_اندر_چوڑائی_بٹ << " out width (bytes)="
                                << m_برآمد_چوڑائی_بائٹیں << " Spacetime=" << (space / وقت_2) << "("
                                << space << "/" << وقت_2 << ")" << ": " << nodep);
        اگر (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / وقت_2) << " " << nodep);
            m_کل_بائٹیں += space;
        }
        واپس chkvis.optimizable();
    }

    خالی replace_With_جدول(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_سطر* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت اشاریہ_Vscp = m_اشاریہ_Tmps.make(fl, m_scopep, m_اندر_چوڑائی_بٹ);

        // The 'output assigned' table builder
        جدول_Builder output_Assigned_جدول_Builder{fl};
        output_Assigned_جدول_Builder.سیٹ_جدول_سائز(
            nodep->ڈھونڈو_ثنائی_ہندسہ_DType(m_برآمد_Varps.سائز(), m_برآمد_Varps.سائز(), VSigning::UNSIGNED),
            VL_نقاب_I(m_اندر_چوڑائی_بٹ));

        // Set sizes of output tables
        برائے (جدول_Output_Var& tov : m_برآمد_Varps) tov.سیٹ_جدول_سائز(VL_نقاب_I(m_اندر_چوڑائی_بٹ));

        // Populate the tables
        createTables(nodep, output_Assigned_جدول_Builder);

        Ast_گرہ* ثابت stmtsp = create_Lookup_داخلہ(fl, اشاریہ_Vscp);
        createOutputAssigns(nodep, stmtsp, اشاریہ_Vscp, output_Assigned_جدول_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
        nodep->جوڑو_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    خالی createTables(AstAlways* nodep, جدول_Builder& output_Assigned_جدول_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        جدول_Simulate_Visitor simvis{یہ};
        برائے (uint32_t i = 0; i <= VL_نقاب_I(m_اندر_چوڑائی_بٹ); ++i) {
            ثابت uint32_t اندر_قدر = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << اندر_قدر);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t سرکاؤ_2 = 0;
            برائے (AstVarScope* invscp : m_اندر_Varps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_قدر{}, invscp->چوڑائی(),
                                    VL_نقاب_I(invscp->چوڑائی()) & (اندر_قدر >> سرکاؤ_2)};
                simvis.new_قدر(invscp, &cnst);
                سرکاؤ_2 += invscp->چوڑائی();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(سرکاؤ_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_جدول_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_پیغام());

            // Build output value tables and the assigned flags table
            V3_نمبر output_Assigned_نقاب{nodep, static_cast<صحیح>(m_برآمد_Varps.سائز()), 0};
            برائے (جدول_Output_Var& tov : m_برآمد_Varps) {
                اگر (V3_نمبر* ثابت outnump = simvis.fetch_برآمد_نمبر_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_نقاب.سیٹ_ثنائی_ہندسہ(tov.ord(), 1);  // Mark output as assigned
                    tov.جوڑو_قدر(اندر_قدر, *outnump);
                } ورنہ {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.سیٹ_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_جدول_Builder.جوڑو_قدر(اندر_قدر, output_Assigned_نقاب);
        }  // each value
    }

    Ast_گرہ* create_Lookup_داخلہ(File_سطر* fl, AstVarScope* اشاریہ_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_گرہ_اظہاریہ* concatp = خالی_اشاریہ;
        برائے (AstVarScope* invscp : m_اندر_Varps) {
            AstVarRef* ثابت refp = نیا AstVarRef{fl, invscp, VAccess::READ};
            اگر (concatp) {
                concatp = نیا AstConcat{fl, refp, concatp};
            } ورنہ {
                concatp = refp;
            }
        }

        واپس نیا Ast_تفویض{fl, نیا AstVarRef{fl, اشاریہ_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_سطر* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = نیا AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت اشاریہ_Refp = نیا AstVarRef{fl, indexp, VAccess::READ};
        واپس نیا AstArraySel{fl, fromRefp, اشاریہ_Refp};
    }

    خالی createOutputAssigns(Ast_گرہ* nodep, Ast_گرہ* stmtsp, AstVarScope* اشاریہ_Vscp,
                             AstVarScope* output_Assigned_جدول_Vscp) {
        File_سطر* ثابت fl = nodep->fileline();
        برائے (جدول_Output_Var& tov : m_برآمد_Varps) {
            Ast_گرہ_اظہاریہ* ثابت alhsp = نیا AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_گرہ_اظہاریہ* ثابت arhsp = select(fl, tov.tabeVarScopep(), اشاریہ_Vscp);
            Ast_گرہ* outsetp = m_تفویض_Dly
                                   ? static_cast<Ast_گرہ*>(نیا Ast_تفویض_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_گرہ*>(نیا Ast_تفویض{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            اگر (tov.mayBeUnassigned()) {
                V3_نمبر output_Chg_نقاب{nodep, static_cast<صحیح>(m_برآمد_Varps.سائز()), 0};
                output_Chg_نقاب.سیٹ_ثنائی_ہندسہ(tov.ord(), 1);
                Ast_گرہ_اظہاریہ* ثابت condp
                    = نیا AstNeq{fl,
                                 نیا AstAnd{fl, select(fl, output_Assigned_جدول_Vscp, اشاریہ_Vscp),
                                            نیا AstConst{fl, output_Chg_نقاب}},
                                 نیا AstConst{fl, AstConst::Widthed_قدر{},
                                              static_cast<صحیح>(m_برآمد_Varps.سائز()), 0}};
                outsetp = نیا AstIf{fl, condp, outsetp};
            }

            stmtsp->جوڑو_اگلا(outsetp);
        }
    }

    // VISITORS
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        اگر (درخت_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_جدول(nodep);
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عوامی:
    // CONSTRUCTORS
    صریح جدول_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~جدول_Visitor() override {  //
        V3Stats::جوڑو_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

خالی جدول_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

خالی V3_جدول::جدول_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { جدول_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("table", 0, dump_درخت_Either_Level() >= 3);
}
