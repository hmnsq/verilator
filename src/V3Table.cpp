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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Table.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Simulate.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <cmath>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ಕೋಷ್ಟಕ_MAX_ಬೈಟುಗಳು = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ಕೋಷ್ಟಕ_ಒಟ್ಟು_ಬೈಟುಗಳು = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ಕೋಷ್ಟಕ_SPACE_ಸಮಯ_MULT = 8;
// If < 32 instructions, not worth the effort
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ಕೋಷ್ಟಕ_MIN_ಗಂಟು_ಎಣಿಕೆ = 32;
// Assume an instruction is 4 bytes
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ಕೋಷ್ಟಕ_ಬೈಟುಗಳು_ಪ್ರತಿ_INST = 4;

//######################################################################

ವರ್ಗ ಕೋಷ್ಟಕ_Visitor;

ವರ್ಗ ಕೋಷ್ಟಕ_Simulate_Visitor final : ಸಾರ್ವಜನಿಕ SimulateVisitor {
    // MEMBERS
    ಕೋಷ್ಟಕ_Visitor* ಸ್ಥಿರಾಂಕ m_cbthis;  ///< Class for callback

ಸಾರ್ವಜನಿಕ:
    ///< Call other-this function on all new var references
    ಶೂನ್ಯ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಕೋಷ್ಟಕ_Simulate_Visitor(ಕೋಷ್ಟಕ_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~ಕೋಷ್ಟಕ_Simulate_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Class for holding lookup table state during construction

ವರ್ಗ ಕೋಷ್ಟಕ_Builder final {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ m_fl;  // FileLine used during construction
    AstConsArray* m_consp = ಶೂನ್ಯಸೂಚಕ;  // The lookup table initializer values
    AstVarScope* m_varScopep = ಶೂನ್ಯಸೂಚಕ;  // The scoped variable holding the table

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ಕೋಷ್ಟಕ_Builder(File_ಸಾಲು* fl)
        : m_fl{fl} {}

    ~ಕೋಷ್ಟಕ_Builder() {
        ಆದರೆ (m_consp) VL_DO_DANGLING(m_consp->delete_ವೃಕ್ಷ(), m_consp);
    }

    ಶೂನ್ಯ ಗಣ_ಕೋಷ್ಟಕ_ಗಾತ್ರ(Ast_ಗಂಟು_DType* elemDType, ಚಿಹ್ನೆರಹಿತ ಗಾತ್ರ_2) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(ಗಾತ್ರ_2 > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = elemDType->ಅಗಲ();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ, VSigning::UNSIGNED);
        AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_DTypep = ಹೊಸ AstUnpackArrayDType{
            m_fl, subDTypep, ಹೊಸ AstRange{m_fl, static_cast<ಪೂರ್ಣಾಂಕ>(ಗಾತ್ರ_2), 0}};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(ಕೋಷ್ಟಕ_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ಸ್ಥಿರಾಂಕ defaultp
            = elemDType->isString()   ? ಹೊಸ AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? ಹೊಸ AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : ಹೊಸ AstConst{m_fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ, 0};
        m_consp = ಹೊಸ AstConsArray{m_fl, ಕೋಷ್ಟಕ_DTypep, defaultp};
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಮೌಲ್ಯ(ಚಿಹ್ನೆರಹಿತ ಸೂಚ್ಯಂಕ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        ಆದರೆ (ಮೌಲ್ಯ.isString() ? ಮೌಲ್ಯ.ಗಮ್ಯ_String().ಖಾಲಿಗಳು() : ಮೌಲ್ಯ.isEqZero()) ಹಿಂತಿರುಗಿ;
        m_consp->valuep(ಸೂಚ್ಯಂಕ, ಹೊಸ AstConst{m_fl, ಮೌಲ್ಯ});
    }

    AstVarScope* varScopep() {
        ಆದರೆ (!m_varScopep) {
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = V3ConstPool::ಕಂಡುಹಿಡಿ(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
        }
        ಹಿಂತಿರುಗಿ m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

ವರ್ಗ ಕೋಷ್ಟಕ_Output_Var final {
    AstVarScope* ಸ್ಥಿರಾಂಕ m_varScopep;  // The output variable
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ m_ord;  // Output ordinal number in this block
    ತಾರ್ಕಿಕ m_mayBeUnassigned = ಸುಳ್ಳು;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    ಕೋಷ್ಟಕ_Builder m_ಕೋಷ್ಟಕ_Builder;

ಸಾರ್ವಜನಿಕ:
    ಕೋಷ್ಟಕ_Output_Var(AstVarScope* varScopep, ಚಿಹ್ನೆರಹಿತ ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_ಕೋಷ್ಟಕ_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_varScopep; }
    string name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varScopep()->varp()->name(); }
    ಚಿಹ್ನೆರಹಿತ ord() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ord; }
    ಶೂನ್ಯ ಗಣ_May_Be_Unassigned() { m_mayBeUnassigned = ನಿಜ; }
    ತಾರ್ಕಿಕ mayBeUnassigned() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_mayBeUnassigned; }
    ಶೂನ್ಯ ಗಣ_ಕೋಷ್ಟಕ_ಗಾತ್ರ(ಚಿಹ್ನೆರಹಿತ ಗಾತ್ರ_2) { m_ಕೋಷ್ಟಕ_Builder.ಗಣ_ಕೋಷ್ಟಕ_ಗಾತ್ರ(varScopep()->dtypep(), ಗಾತ್ರ_2); }
    ಶೂನ್ಯ ಕೂಡಿಸು_ಮೌಲ್ಯ(ಚಿಹ್ನೆರಹಿತ ಸೂಚ್ಯಂಕ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ) { m_ಕೋಷ್ಟಕ_Builder.ಕೂಡಿಸು_ಮೌಲ್ಯ(ಸೂಚ್ಯಂಕ, ಮೌಲ್ಯ); }
    AstVarScope* tabeVarScopep() { ಹಿಂತಿರುಗಿ m_ಕೋಷ್ಟಕ_Builder.varScopep(); }
};

//######################################################################
// Table class functions

ವರ್ಗ ಕೋಷ್ಟಕ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    ದ್ವಿಗುಣ m_ಒಟ್ಟು_ಬೈಟುಗಳು = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_ಸೂಚ್ಯಂಕ_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current SCOPE

    //  State cleared on each always/assignw
    ತಾರ್ಕಿಕ m_ನಿಯೋಜಿಸು_Dly = ಸುಳ್ಳು;  // Consists of delayed assignments instead of normal assignments
    ಚಿಹ್ನೆರಹಿತ m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು = 0;  // Input table width - in bits
    ಚಿಹ್ನೆರಹಿತ m_ಹೊರಹೋಗು_ಅಗಲ_ಬೈಟುಗಳು = 0;  // Output table width - in bytes
    std::ಸದಿಶ<AstVarScope*> m_ಒಳ_Varps;  // Input variable list
    std::ಸದಿಶ<ಕೋಷ್ಟಕ_Output_Var> m_ಹೊರಹೋಗು_Varps;  // Output variable list

    // METHODS

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_ಹೊರಹೋಗು_ಅಗಲ_ಬೈಟುಗಳು += nodep->varp()->dtypeSkipRefp()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು();
            m_ಹೊರಹೋಗು_Varps.emplace_ಹಿಂಭಾಗ(vscp, static_cast<ಚಿಹ್ನೆರಹಿತ>(m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2()));
        }
        ಆದರೆ (nodep->access().isReadOrRW()) {
            m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು += nodep->varp()->ಅಗಲ();
            m_ಒಳ_Varps.ತಳ್ಳು_ಹಿಂಭಾಗ(vscp);
        }
    }

ಖಾಸಗಿ:
    ತಾರ್ಕಿಕ ವೃಕ್ಷ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು = 0;
        m_ಹೊರಹೋಗು_ಅಗಲ_ಬೈಟುಗಳು = 0;
        m_ಒಳ_Varps.clear();
        m_ಹೊರಹೋಗು_Varps.clear();

        // Collect stats
        ಕೋಷ್ಟಕ_Simulate_Visitor chkvis{ಇದು};
        chkvis.main_ಕೋಷ್ಟಕ_Check(nodep);
        m_ನಿಯೋಜಿಸು_Dly = chkvis.is_ನಿಯೋಜಿಸು_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t chg_ಅಗಲ = m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ space = std::ಘಾತ<ದ್ವಿಗುಣ>(2.0, m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು) * (m_ಹೊರಹೋಗು_ಅಗಲ_ಬೈಟುಗಳು + chg_ಅಗಲ);
        // Instruction count bytes (ok, it's space also not time :)
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಸಮಯ_2  // max(_, 1), so we won't divide by zero
            = std::max<ದ್ವಿಗುಣ>(chkvis.instr_ಎಣಿಕೆ() * ಕೋಷ್ಟಕ_ಬೈಟುಗಳು_ಪ್ರತಿ_INST + chkvis.ದತ್ತಾಂಶ_ಎಣಿಕೆ(), 1);
        ಆದರೆ (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        ಆದರೆ (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        ಆದರೆ (!m_ಹೊರಹೋಗು_ಅಗಲ_ಬೈಟುಗಳು || !m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        ಆದರೆ (chkvis.instr_ಎಣಿಕೆ() < ಕೋಷ್ಟಕ_MIN_ಗಂಟು_ಎಣಿಕೆ) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        ಆದರೆ (space > ಕೋಷ್ಟಕ_MAX_ಬೈಟುಗಳು) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        ಆದರೆ (space > ಸಮಯ_2 * ಕೋಷ್ಟಕ_SPACE_ಸಮಯ_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        ಆದರೆ (m_ಒಟ್ಟು_ಬೈಟುಗಳು > ಕೋಷ್ಟಕ_ಒಟ್ಟು_ಬೈಟುಗಳು) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_ಎಣಿಕೆ() << " Data=" << chkvis.ದತ್ತಾಂಶ_ಎಣಿಕೆ()
                                << " in width (bits)=" << m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು << " out width (bytes)="
                                << m_ಹೊರಹೋಗು_ಅಗಲ_ಬೈಟುಗಳು << " Spacetime=" << (space / ಸಮಯ_2) << "("
                                << space << "/" << ಸಮಯ_2 << ")" << ": " << nodep);
        ಆದರೆ (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / ಸಮಯ_2) << " " << nodep);
            m_ಒಟ್ಟು_ಬೈಟುಗಳು += space;
        }
        ಹಿಂತಿರುಗಿ chkvis.optimizable();
    }

    ಶೂನ್ಯ replace_With_ಕೋಷ್ಟಕ(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_Vscp = m_ಸೂಚ್ಯಂಕ_Tmps.make(fl, m_scopep, m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು);

        // The 'output assigned' table builder
        ಕೋಷ್ಟಕ_Builder output_Assigned_ಕೋಷ್ಟಕ_Builder{fl};
        output_Assigned_ಕೋಷ್ಟಕ_Builder.ಗಣ_ಕೋಷ್ಟಕ_ಗಾತ್ರ(
            nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2(), m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2(), VSigning::UNSIGNED),
            VL_ಮುಖವಾಡ_I(m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು));

        // Set sizes of output tables
        ಗಾಗಿ (ಕೋಷ್ಟಕ_Output_Var& tov : m_ಹೊರಹೋಗು_Varps) tov.ಗಣ_ಕೋಷ್ಟಕ_ಗಾತ್ರ(VL_ಮುಖವಾಡ_I(m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು));

        // Populate the tables
        createTables(nodep, output_Assigned_ಕೋಷ್ಟಕ_Builder);

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = create_Lookup_ಒಳಹರಿವು(fl, ಸೂಚ್ಯಂಕ_Vscp);
        createOutputAssigns(nodep, stmtsp, ಸೂಚ್ಯಂಕ_Vscp, output_Assigned_ಕೋಷ್ಟಕ_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
        nodep->ಕೂಡಿಸು_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    ಶೂನ್ಯ createTables(AstAlways* nodep, ಕೋಷ್ಟಕ_Builder& output_Assigned_ಕೋಷ್ಟಕ_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        ಕೋಷ್ಟಕ_Simulate_Visitor simvis{ಇದು};
        ಗಾಗಿ (uint32_t i = 0; i <= VL_ಮುಖವಾಡ_I(m_ಒಳ_ಅಗಲ_ಬಿಟ್ಟುಗಳು); ++i) {
            ಸ್ಥಿರಾಂಕ uint32_t ಒಳ_ಮೌಲ್ಯ = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << ಒಳ_ಮೌಲ್ಯ);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t ಸರಿಸು_2 = 0;
            ಗಾಗಿ (AstVarScope* invscp : m_ಒಳ_Varps) {
                // LSB is first variable, so extract it that way
                ಸ್ಥಿರಾಂಕ AstConst cnst{invscp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, invscp->ಅಗಲ(),
                                    VL_ಮುಖವಾಡ_I(invscp->ಅಗಲ()) & (ಒಳ_ಮೌಲ್ಯ >> ಸರಿಸು_2)};
                simvis.new_ಮೌಲ್ಯ(invscp, &cnst);
                ಸರಿಸು_2 += invscp->ಅಗಲ();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(ಸರಿಸು_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_ಕೋಷ್ಟಕ_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_ಸಂದೇಶ());

            // Build output value tables and the assigned flags table
            V3_ಸಂಖ್ಯೆ output_Assigned_ಮುಖವಾಡ{nodep, static_cast<ಪೂರ್ಣಾಂಕ>(m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2()), 0};
            ಗಾಗಿ (ಕೋಷ್ಟಕ_Output_Var& tov : m_ಹೊರಹೋಗು_Varps) {
                ಆದರೆ (V3_ಸಂಖ್ಯೆ* ಸ್ಥಿರಾಂಕ outnump = simvis.fetch_ಹೊರಹೋಗು_ಸಂಖ್ಯೆ_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_ಮುಖವಾಡ.ಗಣ_ಬಿಟ್(tov.ord(), 1);  // Mark output as assigned
                    tov.ಕೂಡಿಸು_ಮೌಲ್ಯ(ಒಳ_ಮೌಲ್ಯ, *outnump);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.ಗಣ_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_ಕೋಷ್ಟಕ_Builder.ಕೂಡಿಸು_ಮೌಲ್ಯ(ಒಳ_ಮೌಲ್ಯ, output_Assigned_ಮುಖವಾಡ);
        }  // each value
    }

    Ast_ಗಂಟು* create_Lookup_ಒಳಹರಿವು(File_ಸಾಲು* fl, AstVarScope* ಸೂಚ್ಯಂಕ_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* concatp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstVarScope* invscp : m_ಒಳ_Varps) {
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{fl, invscp, VAccess::READ};
            ಆದರೆ (concatp) {
                concatp = ಹೊಸ AstConcat{fl, refp, concatp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                concatp = refp;
            }
        }

        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, ಸೂಚ್ಯಂಕ_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_ಸಾಲು* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ಸ್ಥಿರಾಂಕ fromRefp = ಹೊಸ AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_Refp = ಹೊಸ AstVarRef{fl, indexp, VAccess::READ};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstArraySel{fl, fromRefp, ಸೂಚ್ಯಂಕ_Refp};
    }

    ಶೂನ್ಯ createOutputAssigns(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* stmtsp, AstVarScope* ಸೂಚ್ಯಂಕ_Vscp,
                             AstVarScope* output_Assigned_ಕೋಷ್ಟಕ_Vscp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಗಾಗಿ (ಕೋಷ್ಟಕ_Output_Var& tov : m_ಹೊರಹೋಗು_Varps) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ alhsp = ಹೊಸ AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ arhsp = select(fl, tov.tabeVarScopep(), ಸೂಚ್ಯಂಕ_Vscp);
            Ast_ಗಂಟು* outsetp = m_ನಿಯೋಜಿಸು_Dly
                                   ? static_cast<Ast_ಗಂಟು*>(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ಗಂಟು*>(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            ಆದರೆ (tov.mayBeUnassigned()) {
                V3_ಸಂಖ್ಯೆ output_Chg_ಮುಖವಾಡ{nodep, static_cast<ಪೂರ್ಣಾಂಕ>(m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2()), 0};
                output_Chg_ಮುಖವಾಡ.ಗಣ_ಬಿಟ್(tov.ord(), 1);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp
                    = ಹೊಸ AstNeq{fl,
                                 ಹೊಸ AstAnd{fl, select(fl, output_Assigned_ಕೋಷ್ಟಕ_Vscp, ಸೂಚ್ಯಂಕ_Vscp),
                                            ಹೊಸ AstConst{fl, output_Chg_ಮುಖವಾಡ}},
                                 ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{},
                                              static_cast<ಪೂರ್ಣಾಂಕ>(m_ಹೊರಹೋಗು_Varps.ಗಾತ್ರ_2()), 0}};
                outsetp = ಹೊಸ AstIf{fl, condp, outsetp};
            }

            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(outsetp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        ಆದರೆ (ವೃಕ್ಷ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_ಕೋಷ್ಟಕ(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಕೋಷ್ಟಕ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ಕೋಷ್ಟಕ_Visitor() override {  //
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

ಶೂನ್ಯ ಕೋಷ್ಟಕ_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

ಶೂನ್ಯ V3_ಕೋಷ್ಟಕ::ಕೋಷ್ಟಕ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಕೋಷ್ಟಕ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("table", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
