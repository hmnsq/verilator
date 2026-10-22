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
স্থির constexpr পূর্ণসংখ্যা TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
স্থির constexpr পূর্ণসংখ্যা TABLE_যোগ_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
স্থির constexpr পূর্ণসংখ্যা TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
স্থির constexpr পূর্ণসংখ্যা TABLE_MIN_গ্রন্থি_গুনতি = 32;
// Assume an instruction is 4 bytes
স্থির constexpr পূর্ণসংখ্যা TABLE_BYTES_PER_INST = 4;

//######################################################################

শ্রেণী TableVisitor;

শ্রেণী TableSimulateVisitor final : সর্বজনীন SimulateVisitor {
    // MEMBERS
    TableVisitor* ধ্রুবক m_cbthis;  ///< Class for callback

সর্বজনীন:
    ///< Call other-this function on all new var references
    শূন্য varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Class for holding lookup table state during construction

শ্রেণী TableBuilder final {
    File_সারি* ধ্রুবক m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

সর্বজনীন:
    explicit TableBuilder(File_সারি* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        যদি (m_consp) VL_DO_DANGLING(m_consp->delete_বৃক্ষ(), m_consp);
    }

    শূন্য set_Table_আকার(Ast_গ্রন্থি_DType* elemDType, unsigned আকার_2) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(আকার_2 > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ধ্রুবক পূর্ণসংখ্যা width = elemDType->width();
        Ast_গ্রন্থি_DType* ধ্রুবক subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* ধ্রুবক tableDTypep = নতুন AstUnpackArrayDType{
            m_fl, subDTypep, নতুন AstRange{m_fl, static_cast<পূর্ণসংখ্যা>(আকার_2), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* ধ্রুবক defaultp
            = elemDType->isString()   ? নতুন AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? নতুন AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : নতুন AstConst{m_fl, AstConst::Widthed_মান{}, width, 0};
        m_consp = নতুন AstConsArray{m_fl, tableDTypep, defaultp};
    }

    শূন্য add_মান(unsigned সূচক, ধ্রুবক V3_সংখ্যা& মান) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        যদি (মান.isString() ? মান.toString().empty() : মান.isEqZero()) ফেরত;
        m_consp->valuep(সূচক, নতুন AstConst{m_fl, মান});
    }

    AstVarScope* varScopep() {
        যদি (!m_varScopep) {
            AstVarRef* ধ্রুবক refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
        }
        ফেরত m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

শ্রেণী TableOutputVar final {
    AstVarScope* ধ্রুবক m_varScopep;  // The output variable
    ধ্রুবক unsigned m_ord;  // Output ordinal number in this block
    যৌক্তিক m_mayBeUnassigned = মিথ্যা;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

সর্বজনীন:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() ধ্রুবক { ফেরত m_varScopep; }
    string name() ধ্রুবক { ফেরত varScopep()->varp()->name(); }
    unsigned ord() ধ্রুবক { ফেরত m_ord; }
    শূন্য setMayBeUnassigned() { m_mayBeUnassigned = সত্য; }
    যৌক্তিক mayBeUnassigned() ধ্রুবক { ফেরত m_mayBeUnassigned; }
    শূন্য set_Table_আকার(unsigned আকার_2) { m_tableBuilder.set_Table_আকার(varScopep()->dtypep(), আকার_2); }
    শূন্য add_মান(unsigned সূচক, ধ্রুবক V3_সংখ্যা& মান) { m_tableBuilder.add_মান(সূচক, মান); }
    AstVarScope* tabeVarScopep() { ফেরত m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

শ্রেণী TableVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    দ্বিগুণ m_যোগ_Bytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_সূচক_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    যৌক্তিক m_assignDly = মিথ্যা;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

সর্বজনীন:
    শূন্য simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ধ্রুবক vscp = nodep->varScopep();
        যদি (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->width_যোগ_Bytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.আকার_2()));
        }
        যদি (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ঠেলো_back(vscp);
        }
    }

ব্যক্তিগত:
    যৌক্তিক বৃক্ষ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{এই};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ধ্রুবক আকার_t chgWidth = m_outVarps.আকার_2();
        ধ্রুবক দ্বিগুণ space = std::pow<দ্বিগুণ>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        ধ্রুবক দ্বিগুণ time  // max(_, 1), so we won't divide by zero
            = std::max<দ্বিগুণ>(chkvis.instr_গুনতি() * TABLE_BYTES_PER_INST + chkvis.তথ্য_গুনতি(), 1);
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
        যদি (m_যোগ_Bytes > TABLE_যোগ_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_গুনতি() << " Data=" << chkvis.তথ্য_গুনতি()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        যদি (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_যোগ_Bytes += space;
        }
        ফেরত chkvis.optimizable();
    }

    শূন্য replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_সারি* ধ্রুবক fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ধ্রুবক সূচক_Vscp = m_সূচক_Tmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.set_Table_আকার(
            nodep->findBitDType(m_outVarps.আকার_2(), m_outVarps.আকার_2(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        জন্য (TableOutputVar& tov : m_outVarps) tov.set_Table_আকার(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_গ্রন্থি* ধ্রুবক stmtsp = create_Lookup_প্রবেশ(fl, সূচক_Vscp);
        createOutputAssigns(nodep, stmtsp, সূচক_Vscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী()->delete_বৃক্ষ();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    শূন্য createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{এই};
        জন্য (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            ধ্রুবক uint32_t in_মান = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_মান);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            জন্য (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                ধ্রুবক AstConst cnst{invscp->fileline(), AstConst::Widthed_মান{}, invscp->width(),
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
            V3_সংখ্যা outputAssignedMask{nodep, static_cast<পূর্ণসংখ্যা>(m_outVarps.আকার_2()), 0};
            জন্য (TableOutputVar& tov : m_outVarps) {
                যদি (V3_সংখ্যা* ধ্রুবক outnump = simvis.fetch_Out_সংখ্যা_Null(tov.varScopep())) {
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

    Ast_গ্রন্থি* create_Lookup_প্রবেশ(File_সারি* fl, AstVarScope* সূচক_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_গ্রন্থি_রাশিমালা* concatp = nullptr;
        জন্য (AstVarScope* invscp : m_inVarps) {
            AstVarRef* ধ্রুবক refp = নতুন AstVarRef{fl, invscp, VAccess::READ};
            যদি (concatp) {
                concatp = নতুন AstConcat{fl, refp, concatp};
            } অন্যথা {
                concatp = refp;
            }
        }

        ফেরত নতুন AstAssign{fl, নতুন AstVarRef{fl, সূচক_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_সারি* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ধ্রুবক fromRefp = নতুন AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ধ্রুবক সূচক_Refp = নতুন AstVarRef{fl, indexp, VAccess::READ};
        ফেরত নতুন AstArraySel{fl, fromRefp, সূচক_Refp};
    }

    শূন্য createOutputAssigns(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* stmtsp, AstVarScope* সূচক_Vscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_সারি* ধ্রুবক fl = nodep->fileline();
        জন্য (TableOutputVar& tov : m_outVarps) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক alhsp = নতুন AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক arhsp = select(fl, tov.tabeVarScopep(), সূচক_Vscp);
            Ast_গ্রন্থি* outsetp = m_assignDly
                                   ? static_cast<Ast_গ্রন্থি*>(নতুন AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_গ্রন্থি*>(নতুন AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            যদি (tov.mayBeUnassigned()) {
                V3_সংখ্যা outputChgMask{nodep, static_cast<পূর্ণসংখ্যা>(m_outVarps.আকার_2()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp
                    = নতুন AstNeq{fl,
                                 নতুন AstAnd{fl, select(fl, outputAssignedTableVscp, সূচক_Vscp),
                                            নতুন AstConst{fl, outputChgMask}},
                                 নতুন AstConst{fl, AstConst::Widthed_মান{},
                                              static_cast<পূর্ণসংখ্যা>(m_outVarps.আকার_2()), 0}};
                outsetp = নতুন AstIf{fl, condp, outsetp};
            }

            stmtsp->add_পরবর্তী(outsetp);
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

সর্বজনীন:
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
