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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Table.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Simulate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cmath>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
ਸਥਿਰ constexpr ਪੂਰਨ ਸਾਰਣੀ_MAX_ਬਾਈਟਾਂ = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ਸਥਿਰ constexpr ਪੂਰਨ ਸਾਰਣੀ_ਕੁੱਲ_ਬਾਈਟਾਂ = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ਸਥਿਰ constexpr ਪੂਰਨ ਸਾਰਣੀ_SPACE_ਸਮਾਂ_MULT = 8;
// If < 32 instructions, not worth the effort
ਸਥਿਰ constexpr ਪੂਰਨ ਸਾਰਣੀ_MIN_ਗੰਢ_ਗਿਣਤੀ = 32;
// Assume an instruction is 4 bytes
ਸਥਿਰ constexpr ਪੂਰਨ ਸਾਰਣੀ_ਬਾਈਟਾਂ_ਪ੍ਰਤੀ_INST = 4;

//######################################################################

ਵਰਗ ਸਾਰਣੀ_Visitor;

ਵਰਗ ਸਾਰਣੀ_Simulate_Visitor final : ਜਨਤਕ SimulateVisitor {
    // MEMBERS
    ਸਾਰਣੀ_Visitor* ਅਚਲ m_cbthis;  ///< Class for callback

ਜਨਤਕ:
    ///< Call other-this function on all new var references
    ਖਾਲੀ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਸਾਰਣੀ_Simulate_Visitor(ਸਾਰਣੀ_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~ਸਾਰਣੀ_Simulate_Visitor() override = ਮੂਲ;
};

//######################################################################
// Class for holding lookup table state during construction

ਵਰਗ ਸਾਰਣੀ_Builder final {
    File_ਸਤਰ* ਅਚਲ m_fl;  // FileLine used during construction
    AstConsArray* m_consp = ਖਾਲੀ_ਸੂਚਕ;  // The lookup table initializer values
    AstVarScope* m_varScopep = ਖਾਲੀ_ਸੂਚਕ;  // The scoped variable holding the table

ਜਨਤਕ:
    ਸਪਸ਼ਟ ਸਾਰਣੀ_Builder(File_ਸਤਰ* fl)
        : m_fl{fl} {}

    ~ਸਾਰਣੀ_Builder() {
        ਜੇ (m_consp) VL_DO_DANGLING(m_consp->delete_ਰੁੱਖ(), m_consp);
    }

    ਖਾਲੀ ਸੈੱਟ_ਸਾਰਣੀ_ਮਾਪ(Ast_ਗੰਢ_DType* elemDType, ਬਿਨਾਨਿਸ਼ਾਨੀ ਮਾਪ) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(ਮਾਪ > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = elemDType->ਚੌੜਾਈ();
        Ast_ਗੰਢ_DType* ਅਚਲ subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, VSigning::UNSIGNED);
        AstUnpackArrayDType* ਅਚਲ ਸਾਰਣੀ_DTypep = ਨਵਾਂ AstUnpackArrayDType{
            m_fl, subDTypep, ਨਵਾਂ AstRange{m_fl, static_cast<ਪੂਰਨ>(ਮਾਪ), 0}};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(ਸਾਰਣੀ_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ਅਚਲ defaultp
            = elemDType->isString()   ? ਨਵਾਂ AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? ਨਵਾਂ AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : ਨਵਾਂ AstConst{m_fl, AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ, 0};
        m_consp = ਨਵਾਂ AstConsArray{m_fl, ਸਾਰਣੀ_DTypep, defaultp};
    }

    ਖਾਲੀ ਜੋੜੋ_ਮੁੱਲ(ਬਿਨਾਨਿਸ਼ਾਨੀ ਸੂਚਕਾਂਕ, ਅਚਲ V3_ਸੰਖਿਆ& ਮੁੱਲ) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        ਜੇ (ਮੁੱਲ.isString() ? ਮੁੱਲ.ਮੰਜ਼ਿਲ_String().ਖਾਲੀ_ਥਾਵਾਂ() : ਮੁੱਲ.isEqZero()) ਵਾਪਸ;
        m_consp->valuep(ਸੂਚਕਾਂਕ, ਨਵਾਂ AstConst{m_fl, ਮੁੱਲ});
    }

    AstVarScope* varScopep() {
        ਜੇ (!m_varScopep) {
            AstVarRef* ਅਚਲ refp = V3ConstPool::ਲੱਭੋ(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
        }
        ਵਾਪਸ m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

ਵਰਗ ਸਾਰਣੀ_Output_Var final {
    AstVarScope* ਅਚਲ m_varScopep;  // The output variable
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ m_ord;  // Output ordinal number in this block
    ਤਾਰਕਿਕ m_mayBeUnassigned = ਝੂਠ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    ਸਾਰਣੀ_Builder m_ਸਾਰਣੀ_Builder;

ਜਨਤਕ:
    ਸਾਰਣੀ_Output_Var(AstVarScope* varScopep, ਬਿਨਾਨਿਸ਼ਾਨੀ ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_ਸਾਰਣੀ_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ਅਚਲ { ਵਾਪਸ m_varScopep; }
    string name() ਅਚਲ { ਵਾਪਸ varScopep()->varp()->name(); }
    ਬਿਨਾਨਿਸ਼ਾਨੀ ord() ਅਚਲ { ਵਾਪਸ m_ord; }
    ਖਾਲੀ ਸੈੱਟ_May_Be_Unassigned() { m_mayBeUnassigned = ਸੱਚ; }
    ਤਾਰਕਿਕ mayBeUnassigned() ਅਚਲ { ਵਾਪਸ m_mayBeUnassigned; }
    ਖਾਲੀ ਸੈੱਟ_ਸਾਰਣੀ_ਮਾਪ(ਬਿਨਾਨਿਸ਼ਾਨੀ ਮਾਪ) { m_ਸਾਰਣੀ_Builder.ਸੈੱਟ_ਸਾਰਣੀ_ਮਾਪ(varScopep()->dtypep(), ਮਾਪ); }
    ਖਾਲੀ ਜੋੜੋ_ਮੁੱਲ(ਬਿਨਾਨਿਸ਼ਾਨੀ ਸੂਚਕਾਂਕ, ਅਚਲ V3_ਸੰਖਿਆ& ਮੁੱਲ) { m_ਸਾਰਣੀ_Builder.ਜੋੜੋ_ਮੁੱਲ(ਸੂਚਕਾਂਕ, ਮੁੱਲ); }
    AstVarScope* tabeVarScopep() { ਵਾਪਸ m_ਸਾਰਣੀ_Builder.varScopep(); }
};

//######################################################################
// Table class functions

ਵਰਗ ਸਾਰਣੀ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    ਦੁੱਗਣਾ m_ਕੁੱਲ_ਬਾਈਟਾਂ = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_ਸੂਚਕਾਂਕ_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current SCOPE

    //  State cleared on each always/assignw
    ਤਾਰਕਿਕ m_ਸੌਂਪੋ_Dly = ਝੂਠ;  // Consists of delayed assignments instead of normal assignments
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ = 0;  // Input table width - in bits
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_ਬਾਹਰਲਾ_ਚੌੜਾਈ_ਬਾਈਟਾਂ = 0;  // Output table width - in bytes
    std::ਸਦਿਸ਼<AstVarScope*> m_ਵਿੱਚ_Varps;  // Input variable list
    std::ਸਦਿਸ਼<ਸਾਰਣੀ_Output_Var> m_ਬਾਹਰਲਾ_Varps;  // Output variable list

    // METHODS

ਜਨਤਕ:
    ਖਾਲੀ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        ਜੇ (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_ਬਾਹਰਲਾ_ਚੌੜਾਈ_ਬਾਈਟਾਂ += nodep->varp()->dtypeSkipRefp()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ();
            m_ਬਾਹਰਲਾ_Varps.emplace_ਪਿੱਠ(vscp, static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(m_ਬਾਹਰਲਾ_Varps.ਮਾਪ()));
        }
        ਜੇ (nodep->access().isReadOrRW()) {
            m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ += nodep->varp()->ਚੌੜਾਈ();
            m_ਵਿੱਚ_Varps.ਧੱਕੋ_ਪਿੱਠ(vscp);
        }
    }

ਨਿਜੀ:
    ਤਾਰਕਿਕ ਰੁੱਖ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ = 0;
        m_ਬਾਹਰਲਾ_ਚੌੜਾਈ_ਬਾਈਟਾਂ = 0;
        m_ਵਿੱਚ_Varps.clear();
        m_ਬਾਹਰਲਾ_Varps.clear();

        // Collect stats
        ਸਾਰਣੀ_Simulate_Visitor chkvis{ਇਹ};
        chkvis.main_ਸਾਰਣੀ_Check(nodep);
        m_ਸੌਂਪੋ_Dly = chkvis.is_ਸੌਂਪੋ_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ਅਚਲ ਮਾਪ_t chg_ਚੌੜਾਈ = m_ਬਾਹਰਲਾ_Varps.ਮਾਪ();
        ਅਚਲ ਦੁੱਗਣਾ space = std::ਘਾਤ<ਦੁੱਗਣਾ>(2.0, m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ) * (m_ਬਾਹਰਲਾ_ਚੌੜਾਈ_ਬਾਈਟਾਂ + chg_ਚੌੜਾਈ);
        // Instruction count bytes (ok, it's space also not time :)
        ਅਚਲ ਦੁੱਗਣਾ ਸਮਾਂ_2  // max(_, 1), so we won't divide by zero
            = std::max<ਦੁੱਗਣਾ>(chkvis.instr_ਗਿਣਤੀ() * ਸਾਰਣੀ_ਬਾਈਟਾਂ_ਪ੍ਰਤੀ_INST + chkvis.ਅੰਕੜੇ_ਗਿਣਤੀ(), 1);
        ਜੇ (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        ਜੇ (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        ਜੇ (!m_ਬਾਹਰਲਾ_ਚੌੜਾਈ_ਬਾਈਟਾਂ || !m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        ਜੇ (chkvis.instr_ਗਿਣਤੀ() < ਸਾਰਣੀ_MIN_ਗੰਢ_ਗਿਣਤੀ) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        ਜੇ (space > ਸਾਰਣੀ_MAX_ਬਾਈਟਾਂ) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        ਜੇ (space > ਸਮਾਂ_2 * ਸਾਰਣੀ_SPACE_ਸਮਾਂ_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        ਜੇ (m_ਕੁੱਲ_ਬਾਈਟਾਂ > ਸਾਰਣੀ_ਕੁੱਲ_ਬਾਈਟਾਂ) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_ਗਿਣਤੀ() << " Data=" << chkvis.ਅੰਕੜੇ_ਗਿਣਤੀ()
                                << " in width (bits)=" << m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ << " out width (bytes)="
                                << m_ਬਾਹਰਲਾ_ਚੌੜਾਈ_ਬਾਈਟਾਂ << " Spacetime=" << (space / ਸਮਾਂ_2) << "("
                                << space << "/" << ਸਮਾਂ_2 << ")" << ": " << nodep);
        ਜੇ (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / ਸਮਾਂ_2) << " " << nodep);
            m_ਕੁੱਲ_ਬਾਈਟਾਂ += space;
        }
        ਵਾਪਸ chkvis.optimizable();
    }

    ਖਾਲੀ replace_With_ਸਾਰਣੀ(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ਅਚਲ ਸੂਚਕਾਂਕ_Vscp = m_ਸੂਚਕਾਂਕ_Tmps.make(fl, m_scopep, m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ);

        // The 'output assigned' table builder
        ਸਾਰਣੀ_Builder output_Assigned_ਸਾਰਣੀ_Builder{fl};
        output_Assigned_ਸਾਰਣੀ_Builder.ਸੈੱਟ_ਸਾਰਣੀ_ਮਾਪ(
            nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(m_ਬਾਹਰਲਾ_Varps.ਮਾਪ(), m_ਬਾਹਰਲਾ_Varps.ਮਾਪ(), VSigning::UNSIGNED),
            VL_ਨਕਾਬ_I(m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ));

        // Set sizes of output tables
        ਲਈ (ਸਾਰਣੀ_Output_Var& tov : m_ਬਾਹਰਲਾ_Varps) tov.ਸੈੱਟ_ਸਾਰਣੀ_ਮਾਪ(VL_ਨਕਾਬ_I(m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ));

        // Populate the tables
        createTables(nodep, output_Assigned_ਸਾਰਣੀ_Builder);

        Ast_ਗੰਢ* ਅਚਲ stmtsp = create_Lookup_ਨਿਵੇਸ਼(fl, ਸੂਚਕਾਂਕ_Vscp);
        createOutputAssigns(nodep, stmtsp, ਸੂਚਕਾਂਕ_Vscp, output_Assigned_ਸਾਰਣੀ_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
        nodep->ਜੋੜੋ_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    ਖਾਲੀ createTables(AstAlways* nodep, ਸਾਰਣੀ_Builder& output_Assigned_ਸਾਰਣੀ_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        ਸਾਰਣੀ_Simulate_Visitor simvis{ਇਹ};
        ਲਈ (uint32_t i = 0; i <= VL_ਨਕਾਬ_I(m_ਵਿੱਚ_ਚੌੜਾਈ_ਬਿੱਟ); ++i) {
            ਅਚਲ uint32_t ਵਿੱਚ_ਮੁੱਲ = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << ਵਿੱਚ_ਮੁੱਲ);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t ਖਿਸਕਾਓ_2 = 0;
            ਲਈ (AstVarScope* invscp : m_ਵਿੱਚ_Varps) {
                // LSB is first variable, so extract it that way
                ਅਚਲ AstConst cnst{invscp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, invscp->ਚੌੜਾਈ(),
                                    VL_ਨਕਾਬ_I(invscp->ਚੌੜਾਈ()) & (ਵਿੱਚ_ਮੁੱਲ >> ਖਿਸਕਾਓ_2)};
                simvis.new_ਮੁੱਲ(invscp, &cnst);
                ਖਿਸਕਾਓ_2 += invscp->ਚੌੜਾਈ();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(ਖਿਸਕਾਓ_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_ਸਾਰਣੀ_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_ਸੁਨੇਹਾ());

            // Build output value tables and the assigned flags table
            V3_ਸੰਖਿਆ output_Assigned_ਨਕਾਬ{nodep, static_cast<ਪੂਰਨ>(m_ਬਾਹਰਲਾ_Varps.ਮਾਪ()), 0};
            ਲਈ (ਸਾਰਣੀ_Output_Var& tov : m_ਬਾਹਰਲਾ_Varps) {
                ਜੇ (V3_ਸੰਖਿਆ* ਅਚਲ outnump = simvis.fetch_ਬਾਹਰਲਾ_ਸੰਖਿਆ_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_ਨਕਾਬ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(tov.ord(), 1);  // Mark output as assigned
                    tov.ਜੋੜੋ_ਮੁੱਲ(ਵਿੱਚ_ਮੁੱਲ, *outnump);
                } ਨਹੀਂਤਾਂ {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.ਸੈੱਟ_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_ਸਾਰਣੀ_Builder.ਜੋੜੋ_ਮੁੱਲ(ਵਿੱਚ_ਮੁੱਲ, output_Assigned_ਨਕਾਬ);
        }  // each value
    }

    Ast_ਗੰਢ* create_Lookup_ਨਿਵੇਸ਼(File_ਸਤਰ* fl, AstVarScope* ਸੂਚਕਾਂਕ_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* concatp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstVarScope* invscp : m_ਵਿੱਚ_Varps) {
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{fl, invscp, VAccess::READ};
            ਜੇ (concatp) {
                concatp = ਨਵਾਂ AstConcat{fl, refp, concatp};
            } ਨਹੀਂਤਾਂ {
                concatp = refp;
            }
        }

        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, ਸੂਚਕਾਂਕ_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_ਸਤਰ* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ਅਚਲ fromRefp = ਨਵਾਂ AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ਅਚਲ ਸੂਚਕਾਂਕ_Refp = ਨਵਾਂ AstVarRef{fl, indexp, VAccess::READ};
        ਵਾਪਸ ਨਵਾਂ AstArraySel{fl, fromRefp, ਸੂਚਕਾਂਕ_Refp};
    }

    ਖਾਲੀ createOutputAssigns(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* stmtsp, AstVarScope* ਸੂਚਕਾਂਕ_Vscp,
                             AstVarScope* output_Assigned_ਸਾਰਣੀ_Vscp) {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਲਈ (ਸਾਰਣੀ_Output_Var& tov : m_ਬਾਹਰਲਾ_Varps) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ alhsp = ਨਵਾਂ AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ arhsp = select(fl, tov.tabeVarScopep(), ਸੂਚਕਾਂਕ_Vscp);
            Ast_ਗੰਢ* outsetp = m_ਸੌਂਪੋ_Dly
                                   ? static_cast<Ast_ਗੰਢ*>(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ਗੰਢ*>(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            ਜੇ (tov.mayBeUnassigned()) {
                V3_ਸੰਖਿਆ output_Chg_ਨਕਾਬ{nodep, static_cast<ਪੂਰਨ>(m_ਬਾਹਰਲਾ_Varps.ਮਾਪ()), 0};
                output_Chg_ਨਕਾਬ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(tov.ord(), 1);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp
                    = ਨਵਾਂ AstNeq{fl,
                                 ਨਵਾਂ AstAnd{fl, select(fl, output_Assigned_ਸਾਰਣੀ_Vscp, ਸੂਚਕਾਂਕ_Vscp),
                                            ਨਵਾਂ AstConst{fl, output_Chg_ਨਕਾਬ}},
                                 ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{},
                                              static_cast<ਪੂਰਨ>(m_ਬਾਹਰਲਾ_Varps.ਮਾਪ()), 0}};
                outsetp = ਨਵਾਂ AstIf{fl, condp, outsetp};
            }

            stmtsp->ਜੋੜੋ_ਅਗਲਾ(outsetp);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        ਜੇ (ਰੁੱਖ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_ਸਾਰਣੀ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਸਾਰਣੀ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਸਾਰਣੀ_Visitor() override {  //
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

ਖਾਲੀ ਸਾਰਣੀ_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

ਖਾਲੀ V3_ਸਾਰਣੀ::ਸਾਰਣੀ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਸਾਰਣੀ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("table", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
