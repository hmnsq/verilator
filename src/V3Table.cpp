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
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ TABLE_MIN_ಗಂಟು_ಎಣಿಕೆ = 32;
// Assume an instruction is 4 bytes
ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ TABLE_BYTES_PER_INST = 4;

//######################################################################

ವರ್ಗ TableVisitor;

ವರ್ಗ TableSimulateVisitor final : ಸಾರ್ವಜನಿಕ SimulateVisitor {
    // MEMBERS
    TableVisitor* ಧ್ರುವ m_cbthis;  ///< Class for callback

ಸಾರ್ವಜನಿಕ:
    ///< Call other-this function on all new var references
    ಶೂನ್ಯ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Class for holding lookup table state during construction

ವರ್ಗ TableBuilder final {
    File_ಸಾಲು* ಧ್ರುವ m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

ಸಾರ್ವಜನಿಕ:
    explicit TableBuilder(File_ಸಾಲು* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        ಯದಿ (m_consp) VL_DO_DANGLING(m_consp->delete_ವೃಕ್ಷ(), m_consp);
    }

    ಶೂನ್ಯ setTableSize(Ast_ಗಂಟು_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = elemDType->width();
        Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* ಧ್ರುವ tableDTypep = ನೂತನ AstUnpackArrayDType{
            m_fl, subDTypep, ನೂತನ AstRange{m_fl, static_cast<ಪೂರ್ಣಾಙ್ಕ>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* ಧ್ರುವ defaultp
            = elemDType->isString()   ? ನೂತನ AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? ನೂತನ AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : ನೂತನ AstConst{m_fl, AstConst::Widthed_ಮೌಲ್ಯ{}, width, 0};
        m_consp = ನೂತನ AstConsArray{m_fl, tableDTypep, defaultp};
    }

    ಶೂನ್ಯ add_ಮೌಲ್ಯ(unsigned index, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        ಯದಿ (ಮೌಲ್ಯ.isString() ? ಮೌಲ್ಯ.toString().empty() : ಮೌಲ್ಯ.isEqZero()) ಪ್ರತ್ಯಾಗಮ;
        m_consp->valuep(index, ನೂತನ AstConst{m_fl, ಮೌಲ್ಯ});
    }

    AstVarScope* varScopep() {
        ಯದಿ (!m_varScopep) {
            AstVarRef* ಧ್ರುವ refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
        }
        ಪ್ರತ್ಯಾಗಮ m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

ವರ್ಗ TableOutputVar final {
    AstVarScope* ಧ್ರುವ m_varScopep;  // The output variable
    ಧ್ರುವ unsigned m_ord;  // Output ordinal number in this block
    ತಾರ್ಕಿಕ m_mayBeUnassigned = ಅಸತ್ಯ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

ಸಾರ್ವಜನಿಕ:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_varScopep; }
    string name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varScopep()->varp()->name(); }
    unsigned ord() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ord; }
    ಶೂನ್ಯ setMayBeUnassigned() { m_mayBeUnassigned = ಸತ್ಯ; }
    ತಾರ್ಕಿಕ mayBeUnassigned() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_mayBeUnassigned; }
    ಶೂನ್ಯ setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    ಶೂನ್ಯ add_ಮೌಲ್ಯ(unsigned index, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ) { m_tableBuilder.add_ಮೌಲ್ಯ(index, ಮೌಲ್ಯ); }
    AstVarScope* tabeVarScopep() { ಪ್ರತ್ಯಾಗಮ m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

ವರ್ಗ TableVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    ದ್ವಿಗುಣ m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    ತಾರ್ಕಿಕ m_assignDly = ಅಸತ್ಯ;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
        ಯದಿ (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        ಯದಿ (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ತಳ್ಳು_back(vscp);
        }
    }

ಗುಪ್ತ:
    ತಾರ್ಕಿಕ ವೃಕ್ಷ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{ಏತತ್};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ಧ್ರುವ size_t chgWidth = m_outVarps.size();
        ಧ್ರುವ ದ್ವಿಗುಣ space = std::pow<ದ್ವಿಗುಣ>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        ಧ್ರುವ ದ್ವಿಗುಣ time  // max(_, 1), so we won't divide by zero
            = std::max<ದ್ವಿಗುಣ>(chkvis.instr_ಎಣಿಕೆ() * TABLE_BYTES_PER_INST + chkvis.data_ಎಣಿಕೆ(), 1);
        ಯದಿ (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        ಯದಿ (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        ಯದಿ (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        ಯದಿ (chkvis.instr_ಎಣಿಕೆ() < TABLE_MIN_ಗಂಟು_ಎಣಿಕೆ) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        ಯದಿ (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        ಯದಿ (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        ಯದಿ (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_ಎಣಿಕೆ() << " Data=" << chkvis.data_ಎಣಿಕೆ()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        ಯದಿ (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        ಪ್ರತ್ಯಾಗಮ chkvis.optimizable();
    }

    ಶೂನ್ಯ replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ಧ್ರುವ indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        ಪ್ರತಿ (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_ಗಂಟು* ಧ್ರುವ stmtsp = create_Lookup_ಒಳಹರಿವು(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    ಶೂನ್ಯ createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{ಏತತ್};
        ಪ್ರತಿ (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            ಧ್ರುವ uint32_t in_ಮೌಲ್ಯ = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_ಮೌಲ್ಯ);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            ಪ್ರತಿ (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                ಧ್ರುವ AstConst cnst{invscp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_ಮೌಲ್ಯ >> shift)};
                simvis.new_ಮೌಲ್ಯ(invscp, &cnst);
                shift += invscp->width();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(shift <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.mainTableEmulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.whyNotMessage());

            // Build output value tables and the assigned flags table
            V3_ಸಂಖ್ಯೆ outputAssignedMask{nodep, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_outVarps.size()), 0};
            ಪ್ರತಿ (TableOutputVar& tov : m_outVarps) {
                ಯದಿ (V3_ಸಂಖ್ಯೆ* ಧ್ರುವ outnump = simvis.fetch_Out_ಸಂಖ್ಯೆ_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_ಮೌಲ್ಯ(in_ಮೌಲ್ಯ, *outnump);
                } ಅನ್ಯಥಾ {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_ಮೌಲ್ಯ(in_ಮೌಲ್ಯ, outputAssignedMask);
        }  // each value
    }

    Ast_ಗಂಟು* create_Lookup_ಒಳಹರಿವು(File_ಸಾಲು* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* concatp = nullptr;
        ಪ್ರತಿ (AstVarScope* invscp : m_inVarps) {
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{fl, invscp, VAccess::READ};
            ಯದಿ (concatp) {
                concatp = ನೂತನ AstConcat{fl, refp, concatp};
            } ಅನ್ಯಥಾ {
                concatp = refp;
            }
        }

        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_ಸಾಲು* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ಧ್ರುವ fromRefp = ನೂತನ AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ಧ್ರುವ indexRefp = ನೂತನ AstVarRef{fl, indexp, VAccess::READ};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstArraySel{fl, fromRefp, indexRefp};
    }

    ಶೂನ್ಯ createOutputAssigns(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪ್ರತಿ (TableOutputVar& tov : m_outVarps) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ alhsp = ನೂತನ AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_ಗಂಟು* outsetp = m_assignDly
                                   ? static_cast<Ast_ಗಂಟು*>(ನೂತನ AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ಗಂಟು*>(ನೂತನ AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            ಯದಿ (tov.mayBeUnassigned()) {
                V3_ಸಂಖ್ಯೆ outputChgMask{nodep, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp
                    = ನೂತನ AstNeq{fl,
                                 ನೂತನ AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            ನೂತನ AstConst{fl, outputChgMask}},
                                 ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{},
                                              static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_outVarps.size()), 0}};
                outsetp = ನೂತನ AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        ಯದಿ (ವೃಕ್ಷ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

ಶೂನ್ಯ TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

ಶೂನ್ಯ V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("table", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
