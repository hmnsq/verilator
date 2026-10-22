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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Table.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Simulate.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <cmath>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
స్థిర constexpr పూర్ణాంకం పట్టిక_MAX_బైట్లు = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
స్థిర constexpr పూర్ణాంకం పట్టిక_మొత్తం_బైట్లు = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
స్థిర constexpr పూర్ణాంకం పట్టిక_SPACE_సమయం_MULT = 8;
// If < 32 instructions, not worth the effort
స్థిర constexpr పూర్ణాంకం పట్టిక_MIN_కణుపు_లెక్క = 32;
// Assume an instruction is 4 bytes
స్థిర constexpr పూర్ణాంకం పట్టిక_బైట్లు_చొప్పున_INST = 4;

//######################################################################

తరగతి పట్టిక_Visitor;

తరగతి పట్టిక_Simulate_Visitor final : బహిరంగ SimulateVisitor {
    // MEMBERS
    పట్టిక_Visitor* స్థిరరాశి m_cbthis;  ///< Class for callback

బహిరంగ:
    ///< Call other-this function on all new var references
    శూన్య varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    స్పష్టమైన పట్టిక_Simulate_Visitor(పట్టిక_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~పట్టిక_Simulate_Visitor() override = అప్రమేయ;
};

//######################################################################
// Class for holding lookup table state during construction

తరగతి పట్టిక_Builder final {
    File_పంక్తి* స్థిరరాశి m_fl;  // FileLine used during construction
    AstConsArray* m_consp = శూన్యసూచిక;  // The lookup table initializer values
    AstVarScope* m_varScopep = శూన్యసూచిక;  // The scoped variable holding the table

బహిరంగ:
    స్పష్టమైన పట్టిక_Builder(File_పంక్తి* fl)
        : m_fl{fl} {}

    ~పట్టిక_Builder() {
        ఒకవేళ (m_consp) VL_DO_DANGLING(m_consp->delete_వృక్షం(), m_consp);
    }

    శూన్య సమితి_పట్టిక_కొలత(Ast_కణుపు_DType* elemDType, గుర్తులేని కొలత) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(కొలత > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        స్థిరరాశి పూర్ణాంకం వెడల్పు = elemDType->వెడల్పు();
        Ast_కణుపు_DType* స్థిరరాశి subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు, VSigning::UNSIGNED);
        AstUnpackArrayDType* స్థిరరాశి పట్టిక_DTypep = కొత్త AstUnpackArrayDType{
            m_fl, subDTypep, కొత్త AstRange{m_fl, static_cast<పూర్ణాంకం>(కొలత), 0}};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(పట్టిక_DTypep);
        // Create table initializer (with default value 0)
        AstConst* స్థిరరాశి defaultp
            = elemDType->isString()   ? కొత్త AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? కొత్త AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : కొత్త AstConst{m_fl, AstConst::Widthed_విలువ{}, వెడల్పు, 0};
        m_consp = కొత్త AstConsArray{m_fl, పట్టిక_DTypep, defaultp};
    }

    శూన్య కూడు_విలువ(గుర్తులేని సూచీ, స్థిరరాశి V3_సంఖ్య& విలువ) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        ఒకవేళ (విలువ.isString() ? విలువ.గమ్యం_String().ఖాళీలు() : విలువ.isEqZero()) తిరిగి;
        m_consp->valuep(సూచీ, కొత్త AstConst{m_fl, విలువ});
    }

    AstVarScope* varScopep() {
        ఒకవేళ (!m_varScopep) {
            AstVarRef* స్థిరరాశి refp = V3ConstPool::కనుగొను(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
        }
        తిరిగి m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

తరగతి పట్టిక_Output_Var final {
    AstVarScope* స్థిరరాశి m_varScopep;  // The output variable
    స్థిరరాశి గుర్తులేని m_ord;  // Output ordinal number in this block
    తార్కిక m_mayBeUnassigned = అబద్ధం;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    పట్టిక_Builder m_పట్టిక_Builder;

బహిరంగ:
    పట్టిక_Output_Var(AstVarScope* varScopep, గుర్తులేని ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_పట్టిక_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() స్థిరరాశి { తిరిగి m_varScopep; }
    string name() స్థిరరాశి { తిరిగి varScopep()->varp()->name(); }
    గుర్తులేని ord() స్థిరరాశి { తిరిగి m_ord; }
    శూన్య సమితి_May_Be_Unassigned() { m_mayBeUnassigned = నిజం; }
    తార్కిక mayBeUnassigned() స్థిరరాశి { తిరిగి m_mayBeUnassigned; }
    శూన్య సమితి_పట్టిక_కొలత(గుర్తులేని కొలత) { m_పట్టిక_Builder.సమితి_పట్టిక_కొలత(varScopep()->dtypep(), కొలత); }
    శూన్య కూడు_విలువ(గుర్తులేని సూచీ, స్థిరరాశి V3_సంఖ్య& విలువ) { m_పట్టిక_Builder.కూడు_విలువ(సూచీ, విలువ); }
    AstVarScope* tabeVarScopep() { తిరిగి m_పట్టిక_Builder.varScopep(); }
};

//######################################################################
// Table class functions

తరగతి పట్టిక_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    ద్విగుణ m_మొత్తం_బైట్లు = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_సూచీ_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = శూన్యసూచిక;  // Current SCOPE

    //  State cleared on each always/assignw
    తార్కిక m_కేటాయించు_Dly = అబద్ధం;  // Consists of delayed assignments instead of normal assignments
    గుర్తులేని m_లోపల_వెడల్పు_బిట్లు = 0;  // Input table width - in bits
    గుర్తులేని m_బహిర్గతం_వెడల్పు_బైట్లు = 0;  // Output table width - in bytes
    std::సదిశ<AstVarScope*> m_లోపల_Varps;  // Input variable list
    std::సదిశ<పట్టిక_Output_Var> m_బహిర్గతం_Varps;  // Output variable list

    // METHODS

బహిరంగ:
    శూన్య simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        ఒకవేళ (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_బహిర్గతం_వెడల్పు_బైట్లు += nodep->varp()->dtypeSkipRefp()->వెడల్పు_మొత్తం_బైట్లు();
            m_బహిర్గతం_Varps.emplace_వెనుక(vscp, static_cast<గుర్తులేని>(m_బహిర్గతం_Varps.కొలత()));
        }
        ఒకవేళ (nodep->access().isReadOrRW()) {
            m_లోపల_వెడల్పు_బిట్లు += nodep->varp()->వెడల్పు();
            m_లోపల_Varps.నెట్టు_వెనుక(vscp);
        }
    }

అంతరంగ:
    తార్కిక వృక్షం_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_లోపల_వెడల్పు_బిట్లు = 0;
        m_బహిర్గతం_వెడల్పు_బైట్లు = 0;
        m_లోపల_Varps.clear();
        m_బహిర్గతం_Varps.clear();

        // Collect stats
        పట్టిక_Simulate_Visitor chkvis{ఇది};
        chkvis.main_పట్టిక_Check(nodep);
        m_కేటాయించు_Dly = chkvis.is_కేటాయించు_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        స్థిరరాశి కొలత_t chg_వెడల్పు = m_బహిర్గతం_Varps.కొలత();
        స్థిరరాశి ద్విగుణ space = std::ఘాతం<ద్విగుణ>(2.0, m_లోపల_వెడల్పు_బిట్లు) * (m_బహిర్గతం_వెడల్పు_బైట్లు + chg_వెడల్పు);
        // Instruction count bytes (ok, it's space also not time :)
        స్థిరరాశి ద్విగుణ సమయం_2  // max(_, 1), so we won't divide by zero
            = std::max<ద్విగుణ>(chkvis.instr_లెక్క() * పట్టిక_బైట్లు_చొప్పున_INST + chkvis.దత్తాంశం_లెక్క(), 1);
        ఒకవేళ (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        ఒకవేళ (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        ఒకవేళ (!m_బహిర్గతం_వెడల్పు_బైట్లు || !m_లోపల_వెడల్పు_బిట్లు) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        ఒకవేళ (chkvis.instr_లెక్క() < పట్టిక_MIN_కణుపు_లెక్క) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        ఒకవేళ (space > పట్టిక_MAX_బైట్లు) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        ఒకవేళ (space > సమయం_2 * పట్టిక_SPACE_సమయం_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        ఒకవేళ (m_మొత్తం_బైట్లు > పట్టిక_మొత్తం_బైట్లు) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_లెక్క() << " Data=" << chkvis.దత్తాంశం_లెక్క()
                                << " in width (bits)=" << m_లోపల_వెడల్పు_బిట్లు << " out width (bytes)="
                                << m_బహిర్గతం_వెడల్పు_బైట్లు << " Spacetime=" << (space / సమయం_2) << "("
                                << space << "/" << సమయం_2 << ")" << ": " << nodep);
        ఒకవేళ (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / సమయం_2) << " " << nodep);
            m_మొత్తం_బైట్లు += space;
        }
        తిరిగి chkvis.optimizable();
    }

    శూన్య replace_With_పట్టిక(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* స్థిరరాశి సూచీ_Vscp = m_సూచీ_Tmps.make(fl, m_scopep, m_లోపల_వెడల్పు_బిట్లు);

        // The 'output assigned' table builder
        పట్టిక_Builder output_Assigned_పట్టిక_Builder{fl};
        output_Assigned_పట్టిక_Builder.సమితి_పట్టిక_కొలత(
            nodep->కనుగొను_బిట్టు_DType(m_బహిర్గతం_Varps.కొలత(), m_బహిర్గతం_Varps.కొలత(), VSigning::UNSIGNED),
            VL_ముసుగు_I(m_లోపల_వెడల్పు_బిట్లు));

        // Set sizes of output tables
        కొరకు (పట్టిక_Output_Var& tov : m_బహిర్గతం_Varps) tov.సమితి_పట్టిక_కొలత(VL_ముసుగు_I(m_లోపల_వెడల్పు_బిట్లు));

        // Populate the tables
        createTables(nodep, output_Assigned_పట్టిక_Builder);

        Ast_కణుపు* స్థిరరాశి stmtsp = create_Lookup_ఆదానం(fl, సూచీ_Vscp);
        createOutputAssigns(nodep, stmtsp, సూచీ_Vscp, output_Assigned_పట్టిక_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
        nodep->కూడు_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    శూన్య createTables(AstAlways* nodep, పట్టిక_Builder& output_Assigned_పట్టిక_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        పట్టిక_Simulate_Visitor simvis{ఇది};
        కొరకు (uint32_t i = 0; i <= VL_ముసుగు_I(m_లోపల_వెడల్పు_బిట్లు); ++i) {
            స్థిరరాశి uint32_t లోపల_విలువ = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << లోపల_విలువ);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t జరుపు_2 = 0;
            కొరకు (AstVarScope* invscp : m_లోపల_Varps) {
                // LSB is first variable, so extract it that way
                స్థిరరాశి AstConst cnst{invscp->fileline(), AstConst::Widthed_విలువ{}, invscp->వెడల్పు(),
                                    VL_ముసుగు_I(invscp->వెడల్పు()) & (లోపల_విలువ >> జరుపు_2)};
                simvis.new_విలువ(invscp, &cnst);
                జరుపు_2 += invscp->వెడల్పు();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(జరుపు_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_పట్టిక_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_సందేశం());

            // Build output value tables and the assigned flags table
            V3_సంఖ్య output_Assigned_ముసుగు{nodep, static_cast<పూర్ణాంకం>(m_బహిర్గతం_Varps.కొలత()), 0};
            కొరకు (పట్టిక_Output_Var& tov : m_బహిర్గతం_Varps) {
                ఒకవేళ (V3_సంఖ్య* స్థిరరాశి outnump = simvis.fetch_బహిర్గతం_సంఖ్య_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_ముసుగు.సమితి_బిట్టు(tov.ord(), 1);  // Mark output as assigned
                    tov.కూడు_విలువ(లోపల_విలువ, *outnump);
                } లేకపోతే {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.సమితి_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_పట్టిక_Builder.కూడు_విలువ(లోపల_విలువ, output_Assigned_ముసుగు);
        }  // each value
    }

    Ast_కణుపు* create_Lookup_ఆదానం(File_పంక్తి* fl, AstVarScope* సూచీ_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_కణుపు_సమాసం* concatp = శూన్యసూచిక;
        కొరకు (AstVarScope* invscp : m_లోపల_Varps) {
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{fl, invscp, VAccess::READ};
            ఒకవేళ (concatp) {
                concatp = కొత్త AstConcat{fl, refp, concatp};
            } లేకపోతే {
                concatp = refp;
            }
        }

        తిరిగి కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, సూచీ_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_పంక్తి* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* స్థిరరాశి fromRefp = కొత్త AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* స్థిరరాశి సూచీ_Refp = కొత్త AstVarRef{fl, indexp, VAccess::READ};
        తిరిగి కొత్త AstArraySel{fl, fromRefp, సూచీ_Refp};
    }

    శూన్య createOutputAssigns(Ast_కణుపు* nodep, Ast_కణుపు* stmtsp, AstVarScope* సూచీ_Vscp,
                             AstVarScope* output_Assigned_పట్టిక_Vscp) {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        కొరకు (పట్టిక_Output_Var& tov : m_బహిర్గతం_Varps) {
            Ast_కణుపు_సమాసం* స్థిరరాశి alhsp = కొత్త AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_కణుపు_సమాసం* స్థిరరాశి arhsp = select(fl, tov.tabeVarScopep(), సూచీ_Vscp);
            Ast_కణుపు* outsetp = m_కేటాయించు_Dly
                                   ? static_cast<Ast_కణుపు*>(కొత్త Ast_కేటాయించు_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_కణుపు*>(కొత్త Ast_కేటాయించు{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            ఒకవేళ (tov.mayBeUnassigned()) {
                V3_సంఖ్య output_Chg_ముసుగు{nodep, static_cast<పూర్ణాంకం>(m_బహిర్గతం_Varps.కొలత()), 0};
                output_Chg_ముసుగు.సమితి_బిట్టు(tov.ord(), 1);
                Ast_కణుపు_సమాసం* స్థిరరాశి condp
                    = కొత్త AstNeq{fl,
                                 కొత్త AstAnd{fl, select(fl, output_Assigned_పట్టిక_Vscp, సూచీ_Vscp),
                                            కొత్త AstConst{fl, output_Chg_ముసుగు}},
                                 కొత్త AstConst{fl, AstConst::Widthed_విలువ{},
                                              static_cast<పూర్ణాంకం>(m_బహిర్గతం_Varps.కొలత()), 0}};
                outsetp = కొత్త AstIf{fl, condp, outsetp};
            }

            stmtsp->కూడు_తదుపరి(outsetp);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        ఒకవేళ (వృక్షం_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_పట్టిక(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన పట్టిక_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~పట్టిక_Visitor() override {  //
        V3Stats::కూడు_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

శూన్య పట్టిక_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

శూన్య V3_పట్టిక::పట్టిక_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { పట్టిక_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("table", 0, dump_వృక్షం_Either_Level() >= 3);
}
