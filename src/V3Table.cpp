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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Table.h"

#অন্তর্ভাব "V3ConstPool.h"
#অন্তর্ভাব "V3SharedTmps.h"
#অন্তর্ভাব "V3Simulate.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <cmath>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
স্থির constexpr পূর্ণাঙ্ক TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
স্থির constexpr পূর্ণাঙ্ক TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
স্থির constexpr পূর্ণাঙ্ক TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
স্থির constexpr পূর্ণাঙ্ক TABLE_MIN_গ্রন্থি_গুনতি = 32;
// Assume an instruction is 4 bytes
স্থির constexpr পূর্ণাঙ্ক TABLE_BYTES_PER_INST = 4;

//######################################################################

বর্গ TableVisitor;

বর্গ TableSimulateVisitor final : সার্বজনিক SimulateVisitor {
    // MEMBERS
    TableVisitor* ধ্রুব m_cbthis;  ///< Class for callback

সার্বজনিক:
    ///< Call other-this function on all new var references
    শূন্য varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = স্বভাব;
};

//######################################################################
// Class for holding lookup table state during construction

বর্গ TableBuilder final {
    File_সারি* ধ্রুব m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

সার্বজনিক:
    explicit TableBuilder(File_সারি* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        যদি (m_consp) VL_DO_DANGLING(m_consp->delete_বৃক্ষ(), m_consp);
    }

    শূন্য setTableSize(Ast_গ্রন্থি_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ধ্রুব পূর্ণাঙ্ক width = elemDType->width();
        Ast_গ্রন্থি_DType* ধ্রুব subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* ধ্রুব tableDTypep = নূতন AstUnpackArrayDType{
            m_fl, subDTypep, নূতন AstRange{m_fl, static_cast<পূর্ণাঙ্ক>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* ধ্রুব defaultp
            = elemDType->isString()   ? নূতন AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? নূতন AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : নূতন AstConst{m_fl, AstConst::Widthed_মান{}, width, 0};
        m_consp = নূতন AstConsArray{m_fl, tableDTypep, defaultp};
    }

    শূন্য add_মান(unsigned index, ধ্রুব V3_সংখ্যা& মান) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        যদি (মান.isString() ? মান.toString().empty() : মান.isEqZero()) প্রত্যাগম;
        m_consp->valuep(index, নূতন AstConst{m_fl, মান});
    }

    AstVarScope* varScopep() {
        যদি (!m_varScopep) {
            AstVarRef* ধ্রুব refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
        }
        প্রত্যাগম m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

বর্গ TableOutputVar final {
    AstVarScope* ধ্রুব m_varScopep;  // The output variable
    ধ্রুব unsigned m_ord;  // Output ordinal number in this block
    তার্কিক m_mayBeUnassigned = অসত্য;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

সার্বজনিক:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() ধ্রুব { প্রত্যাগম m_varScopep; }
    string name() ধ্রুব { প্রত্যাগম varScopep()->varp()->name(); }
    unsigned ord() ধ্রুব { প্রত্যাগম m_ord; }
    শূন্য setMayBeUnassigned() { m_mayBeUnassigned = সত্য; }
    তার্কিক mayBeUnassigned() ধ্রুব { প্রত্যাগম m_mayBeUnassigned; }
    শূন্য setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    শূন্য add_মান(unsigned index, ধ্রুব V3_সংখ্যা& মান) { m_tableBuilder.add_মান(index, মান); }
    AstVarScope* tabeVarScopep() { প্রত্যাগম m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

বর্গ TableVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    দ্বিগুণ m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    তার্কিক m_assignDly = অসত্য;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

সার্বজনিক:
    শূন্য simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ধ্রুব vscp = nodep->varScopep();
        যদি (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        যদি (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ঠেলো_back(vscp);
        }
    }

গুপ্ত:
    তার্কিক বৃক্ষ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{এতত্};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ধ্রুব size_t chgWidth = m_outVarps.size();
        ধ্রুব দ্বিগুণ space = std::pow<দ্বিগুণ>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        ধ্রুব দ্বিগুণ time  // max(_, 1), so we won't divide by zero
            = std::max<দ্বিগুণ>(chkvis.instr_গুনতি() * TABLE_BYTES_PER_INST + chkvis.data_গুনতি(), 1);
        যদি (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        যদি (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        যদি (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        যদি (chkvis.instr_গুনতি() < TABLE_MIN_গ্রন্থি_গুনতি) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        যদি (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        যদি (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        যদি (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_গুনতি() << " Data=" << chkvis.data_গুনতি()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        যদি (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        প্রত্যাগম chkvis.optimizable();
    }

    শূন্য replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_সারি* ধ্রুব fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ধ্রুব indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        প্রতি (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_গ্রন্থি* ধ্রুব stmtsp = create_Lookup_প্রবেশ(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    শূন্য createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{এতত্};
        প্রতি (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            ধ্রুব uint32_t in_মান = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_মান);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            প্রতি (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                ধ্রুব AstConst cnst{invscp->fileline(), AstConst::Widthed_মান{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_মান >> shift)};
                simvis.new_মান(invscp, &cnst);
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
            V3_সংখ্যা outputAssignedMask{nodep, static_cast<পূর্ণাঙ্ক>(m_outVarps.size()), 0};
            প্রতি (TableOutputVar& tov : m_outVarps) {
                যদি (V3_সংখ্যা* ধ্রুব outnump = simvis.fetch_Out_সংখ্যা_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_মান(in_মান, *outnump);
                } অন্যথা {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_মান(in_মান, outputAssignedMask);
        }  // each value
    }

    Ast_গ্রন্থি* create_Lookup_প্রবেশ(File_সারি* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_গ্রন্থি_রাশিমালা* concatp = nullptr;
        প্রতি (AstVarScope* invscp : m_inVarps) {
            AstVarRef* ধ্রুব refp = নূতন AstVarRef{fl, invscp, VAccess::READ};
            যদি (concatp) {
                concatp = নূতন AstConcat{fl, refp, concatp};
            } অন্যথা {
                concatp = refp;
            }
        }

        প্রত্যাগম নূতন AstAssign{fl, নূতন AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_সারি* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ধ্রুব fromRefp = নূতন AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ধ্রুব indexRefp = নূতন AstVarRef{fl, indexp, VAccess::READ};
        প্রত্যাগম নূতন AstArraySel{fl, fromRefp, indexRefp};
    }

    শূন্য createOutputAssigns(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_সারি* ধ্রুব fl = nodep->fileline();
        প্রতি (TableOutputVar& tov : m_outVarps) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব alhsp = নূতন AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_গ্রন্থি* outsetp = m_assignDly
                                   ? static_cast<Ast_গ্রন্থি*>(নূতন AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_গ্রন্থি*>(নূতন AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            যদি (tov.mayBeUnassigned()) {
                V3_সংখ্যা outputChgMask{nodep, static_cast<পূর্ণাঙ্ক>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp
                    = নূতন AstNeq{fl,
                                 নূতন AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            নূতন AstConst{fl, outputChgMask}},
                                 নূতন AstConst{fl, AstConst::Widthed_মান{},
                                              static_cast<পূর্ণাঙ্ক>(m_outVarps.size()), 0}};
                outsetp = নূতন AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        যদি (বৃক্ষ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

শূন্য TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

শূন্য V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("table", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
