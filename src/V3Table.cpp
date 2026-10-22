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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Table.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Simulate.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <cmath>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
સ્થિર constexpr પૂર્ણાંક TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
સ્થિર constexpr પૂર્ણાંક TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
સ્થિર constexpr પૂર્ણાંક TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
સ્થિર constexpr પૂર્ણાંક TABLE_MIN_ગાંઠ_ગણતરી = 32;
// Assume an instruction is 4 bytes
સ્થિર constexpr પૂર્ણાંક TABLE_BYTES_PER_INST = 4;

//######################################################################

વર્ગ TableVisitor;

વર્ગ TableSimulateVisitor final : જાહેર SimulateVisitor {
    // MEMBERS
    TableVisitor* અચલ m_cbthis;  ///< Class for callback

જાહેર:
    ///< Call other-this function on all new var references
    શૂન્ય varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = મૂળભૂત;
};

//######################################################################
// Class for holding lookup table state during construction

વર્ગ TableBuilder final {
    File_લીટી* અચલ m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

જાહેર:
    explicit TableBuilder(File_લીટી* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        જો (m_consp) VL_DO_DANGLING(m_consp->delete_વૃક્ષ(), m_consp);
    }

    શૂન્ય setTableSize(Ast_ગાંઠ_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        અચલ પૂર્ણાંક width = elemDType->width();
        Ast_ગાંઠ_DType* અચલ subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* અચલ tableDTypep = નવું AstUnpackArrayDType{
            m_fl, subDTypep, નવું AstRange{m_fl, static_cast<પૂર્ણાંક>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* અચલ defaultp
            = elemDType->isString()   ? નવું AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? નવું AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : નવું AstConst{m_fl, AstConst::Widthed_કિંમત{}, width, 0};
        m_consp = નવું AstConsArray{m_fl, tableDTypep, defaultp};
    }

    શૂન્ય add_કિંમત(unsigned index, અચલ V3_સંખ્યા& કિંમત) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        જો (કિંમત.isString() ? કિંમત.toString().empty() : કિંમત.isEqZero()) પાછા;
        m_consp->valuep(index, નવું AstConst{m_fl, કિંમત});
    }

    AstVarScope* varScopep() {
        જો (!m_varScopep) {
            AstVarRef* અચલ refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_વૃક્ષ(), refp);
        }
        પાછા m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

વર્ગ TableOutputVar final {
    AstVarScope* અચલ m_varScopep;  // The output variable
    અચલ unsigned m_ord;  // Output ordinal number in this block
    તાર્કિક m_mayBeUnassigned = ખોટું;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

જાહેર:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() અચલ { પાછા m_varScopep; }
    string name() અચલ { પાછા varScopep()->varp()->name(); }
    unsigned ord() અચલ { પાછા m_ord; }
    શૂન્ય setMayBeUnassigned() { m_mayBeUnassigned = સાચું; }
    તાર્કિક mayBeUnassigned() અચલ { પાછા m_mayBeUnassigned; }
    શૂન્ય setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    શૂન્ય add_કિંમત(unsigned index, અચલ V3_સંખ્યા& કિંમત) { m_tableBuilder.add_કિંમત(index, કિંમત); }
    AstVarScope* tabeVarScopep() { પાછા m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

વર્ગ TableVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    દ્વિગુણ m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    તાર્કિક m_assignDly = ખોટું;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

જાહેર:
    શૂન્ય simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* અચલ vscp = nodep->varScopep();
        જો (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        જો (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ધકેલો_back(vscp);
        }
    }

ખાનગી:
    તાર્કિક વૃક્ષ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{આ};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        અચલ size_t chgWidth = m_outVarps.size();
        અચલ દ્વિગુણ space = std::pow<દ્વિગુણ>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        અચલ દ્વિગુણ time  // max(_, 1), so we won't divide by zero
            = std::max<દ્વિગુણ>(chkvis.instr_ગણતરી() * TABLE_BYTES_PER_INST + chkvis.data_ગણતરી(), 1);
        જો (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        જો (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        જો (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        જો (chkvis.instr_ગણતરી() < TABLE_MIN_ગાંઠ_ગણતરી) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        જો (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        જો (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        જો (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_ગણતરી() << " Data=" << chkvis.data_ગણતરી()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        જો (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        પાછા chkvis.optimizable();
    }

    શૂન્ય replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_લીટી* અચલ fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* અચલ indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        માટે (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_ગાંઠ* અચલ stmtsp = create_Lookup_આગત(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    શૂન્ય createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{આ};
        માટે (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            અચલ uint32_t in_કિંમત = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_કિંમત);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            માટે (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                અચલ AstConst cnst{invscp->fileline(), AstConst::Widthed_કિંમત{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_કિંમત >> shift)};
                simvis.new_કિંમત(invscp, &cnst);
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
            V3_સંખ્યા outputAssignedMask{nodep, static_cast<પૂર્ણાંક>(m_outVarps.size()), 0};
            માટે (TableOutputVar& tov : m_outVarps) {
                જો (V3_સંખ્યા* અચલ outnump = simvis.fetch_Out_સંખ્યા_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_કિંમત(in_કિંમત, *outnump);
                } નહીંતર {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_કિંમત(in_કિંમત, outputAssignedMask);
        }  // each value
    }

    Ast_ગાંઠ* create_Lookup_આગત(File_લીટી* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ગાંઠ_પદાવલિ* concatp = nullptr;
        માટે (AstVarScope* invscp : m_inVarps) {
            AstVarRef* અચલ refp = નવું AstVarRef{fl, invscp, VAccess::READ};
            જો (concatp) {
                concatp = નવું AstConcat{fl, refp, concatp};
            } નહીંતર {
                concatp = refp;
            }
        }

        પાછા નવું AstAssign{fl, નવું AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_લીટી* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* અચલ fromRefp = નવું AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* અચલ indexRefp = નવું AstVarRef{fl, indexp, VAccess::READ};
        પાછા નવું AstArraySel{fl, fromRefp, indexRefp};
    }

    શૂન્ય createOutputAssigns(Ast_ગાંઠ* nodep, Ast_ગાંઠ* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_લીટી* અચલ fl = nodep->fileline();
        માટે (TableOutputVar& tov : m_outVarps) {
            Ast_ગાંઠ_પદાવલિ* અચલ alhsp = નવું AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ગાંઠ_પદાવલિ* અચલ arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_ગાંઠ* outsetp = m_assignDly
                                   ? static_cast<Ast_ગાંઠ*>(નવું AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ગાંઠ*>(નવું AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            જો (tov.mayBeUnassigned()) {
                V3_સંખ્યા outputChgMask{nodep, static_cast<પૂર્ણાંક>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_ગાંઠ_પદાવલિ* અચલ condp
                    = નવું AstNeq{fl,
                                 નવું AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            નવું AstConst{fl, outputChgMask}},
                                 નવું AstConst{fl, AstConst::Widthed_કિંમત{},
                                              static_cast<પૂર્ણાંક>(m_outVarps.size()), 0}};
                outsetp = નવું AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        જો (વૃક્ષ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

જાહેર:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

શૂન્ય TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

શૂન્ય V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("table", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
