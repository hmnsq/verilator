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
ساكن constexpr صحيح TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ساكن constexpr صحيح TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ساكن constexpr صحيح TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
ساكن constexpr صحيح TABLE_MIN_تۈگۈن_سانى = 32;
// Assume an instruction is 4 bytes
ساكن constexpr صحيح TABLE_BYTES_PER_INST = 4;

//######################################################################

صنف TableVisitor;

صنف TableSimulateVisitor final : عام SimulateVisitor {
    // MEMBERS
    TableVisitor* ثابت m_cbthis;  ///< Class for callback

عام:
    ///< Call other-this function on all new var references
    فراغ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = افتراضي;
};

//######################################################################
// Class for holding lookup table state during construction

صنف TableBuilder final {
    File_قۇر* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

عام:
    explicit TableBuilder(File_قۇر* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        إذا (m_consp) VL_DO_DANGLING(m_consp->delete_دەرەخ(), m_consp);
    }

    فراغ setTableSize(Ast_تۈگۈن_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحيح width = elemDType->width();
        Ast_تۈگۈن_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت tableDTypep = جديد AstUnpackArrayDType{
            m_fl, subDTypep, جديد AstRange{m_fl, static_cast<صحيح>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? جديد AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? جديد AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : جديد AstConst{m_fl, AstConst::Widthed_قىممەت{}, width, 0};
        m_consp = جديد AstConsArray{m_fl, tableDTypep, defaultp};
    }

    فراغ add_قىممەت(unsigned index, ثابت V3_سان& قىممەت) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        إذا (قىممەت.isString() ? قىممەت.toString().empty() : قىممەت.isEqZero()) أرجع;
        m_consp->valuep(index, جديد AstConst{m_fl, قىممەت});
    }

    AstVarScope* varScopep() {
        إذا (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_دەرەخ(), refp);
        }
        أرجع m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

صنف TableOutputVar final {
    AstVarScope* ثابت m_varScopep;  // The output variable
    ثابت unsigned m_ord;  // Output ordinal number in this block
    منطقي m_mayBeUnassigned = خطأ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

عام:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    string name() ثابت { أرجع varScopep()->varp()->name(); }
    unsigned ord() ثابت { أرجع m_ord; }
    فراغ setMayBeUnassigned() { m_mayBeUnassigned = true; }
    منطقي mayBeUnassigned() ثابت { أرجع m_mayBeUnassigned; }
    فراغ setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    فراغ add_قىممەت(unsigned index, ثابت V3_سان& قىممەت) { m_tableBuilder.add_قىممەت(index, قىممەت); }
    AstVarScope* tabeVarScopep() { أرجع m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

صنف TableVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    مزدوج m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقي m_assignDly = خطأ;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

عام:
    فراغ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        إذا (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ئىتتىرىش_back(vscp);
        }
    }

خاص:
    منطقي دەرەخ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{هذا};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت size_t chgWidth = m_outVarps.size();
        ثابت مزدوج space = std::pow<مزدوج>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت مزدوج time  // max(_, 1), so we won't divide by zero
            = std::max<مزدوج>(chkvis.instr_سانى() * TABLE_BYTES_PER_INST + chkvis.data_سانى(), 1);
        إذا (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        إذا (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        إذا (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        إذا (chkvis.instr_سانى() < TABLE_MIN_تۈگۈن_سانى) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        إذا (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        إذا (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        إذا (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_سانى() << " Data=" << chkvis.data_سانى()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        إذا (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        أرجع chkvis.optimizable();
    }

    فراغ replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_قۇر* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        لكل (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_تۈگۈن* ثابت stmtsp = create_Lookup_كىرگۈزۈش(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_دەرەخ();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    فراغ createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{هذا};
        لكل (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            ثابت uint32_t in_قىممەت = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_قىممەت);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            لكل (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_قىممەت{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_قىممەت >> shift)};
                simvis.new_قىممەت(invscp, &cnst);
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
            V3_سان outputAssignedMask{nodep, static_cast<صحيح>(m_outVarps.size()), 0};
            لكل (TableOutputVar& tov : m_outVarps) {
                إذا (V3_سان* ثابت outnump = simvis.fetch_Out_سان_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_قىممەت(in_قىممەت, *outnump);
                } وإلا {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_قىممەت(in_قىممەت, outputAssignedMask);
        }  // each value
    }

    Ast_تۈگۈن* create_Lookup_كىرگۈزۈش(File_قۇر* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_تۈگۈن_ئىپادە* concatp = nullptr;
        لكل (AstVarScope* invscp : m_inVarps) {
            AstVarRef* ثابت refp = جديد AstVarRef{fl, invscp, VAccess::READ};
            إذا (concatp) {
                concatp = جديد AstConcat{fl, refp, concatp};
            } وإلا {
                concatp = refp;
            }
        }

        أرجع جديد AstAssign{fl, جديد AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_قۇر* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = جديد AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت indexRefp = جديد AstVarRef{fl, indexp, VAccess::READ};
        أرجع جديد AstArraySel{fl, fromRefp, indexRefp};
    }

    فراغ createOutputAssigns(Ast_تۈگۈن* nodep, Ast_تۈگۈن* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_قۇر* ثابت fl = nodep->fileline();
        لكل (TableOutputVar& tov : m_outVarps) {
            Ast_تۈگۈن_ئىپادە* ثابت alhsp = جديد AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_تۈگۈن_ئىپادە* ثابت arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_تۈگۈن* outsetp = m_assignDly
                                   ? static_cast<Ast_تۈگۈن*>(جديد AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_تۈگۈن*>(جديد AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            إذا (tov.mayBeUnassigned()) {
                V3_سان outputChgMask{nodep, static_cast<صحيح>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_تۈگۈن_ئىپادە* ثابت condp
                    = جديد AstNeq{fl,
                                 جديد AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            جديد AstConst{fl, outputChgMask}},
                                 جديد AstConst{fl, AstConst::Widthed_قىممەت{},
                                              static_cast<صحيح>(m_outVarps.size()), 0}};
                outsetp = جديد AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        إذا (دەرەخ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    فراغ visit(Ast_تۈگۈن_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عام:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

فراغ TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

فراغ V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_دەرەخ("table", 0, dump_دەرەخ_Either_Level() >= 3);
}
