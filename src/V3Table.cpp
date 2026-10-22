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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Table.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Simulate.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <cmath>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
நிலையான constexpr முழு TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
நிலையான constexpr முழு TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
நிலையான constexpr முழு TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
நிலையான constexpr முழு TABLE_MIN_முனை_மொத்தம் = 32;
// Assume an instruction is 4 bytes
நிலையான constexpr முழு TABLE_BYTES_PER_INST = 4;

//######################################################################

வகுப்பு TableVisitor;

வகுப்பு TableSimulateVisitor final : பொது SimulateVisitor {
    // MEMBERS
    TableVisitor* மாறிலி m_cbthis;  ///< Class for callback

பொது:
    ///< Call other-this function on all new var references
    வெறுமை varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = இயல்பு;
};

//######################################################################
// Class for holding lookup table state during construction

வகுப்பு TableBuilder final {
    File_வரி* மாறிலி m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

பொது:
    explicit TableBuilder(File_வரி* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        எனில் (m_consp) VL_DO_DANGLING(m_consp->delete_மரம்(), m_consp);
    }

    வெறுமை setTableSize(Ast_முனை_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        மாறிலி முழு width = elemDType->width();
        Ast_முனை_DType* மாறிலி subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* மாறிலி tableDTypep = புதிய AstUnpackArrayDType{
            m_fl, subDTypep, புதிய AstRange{m_fl, static_cast<முழு>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* மாறிலி defaultp
            = elemDType->isString()   ? புதிய AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? புதிய AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : புதிய AstConst{m_fl, AstConst::Widthed_மதிப்பு{}, width, 0};
        m_consp = புதிய AstConsArray{m_fl, tableDTypep, defaultp};
    }

    வெறுமை add_மதிப்பு(unsigned index, மாறிலி V3_எண்& மதிப்பு) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        எனில் (மதிப்பு.isString() ? மதிப்பு.toString().empty() : மதிப்பு.isEqZero()) திரும்பு;
        m_consp->valuep(index, புதிய AstConst{m_fl, மதிப்பு});
    }

    AstVarScope* varScopep() {
        எனில் (!m_varScopep) {
            AstVarRef* மாறிலி refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_மரம்(), refp);
        }
        திரும்பு m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

வகுப்பு TableOutputVar final {
    AstVarScope* மாறிலி m_varScopep;  // The output variable
    மாறிலி unsigned m_ord;  // Output ordinal number in this block
    தருக்க m_mayBeUnassigned = பொய்;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

பொது:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() மாறிலி { திரும்பு m_varScopep; }
    string name() மாறிலி { திரும்பு varScopep()->varp()->name(); }
    unsigned ord() மாறிலி { திரும்பு m_ord; }
    வெறுமை setMayBeUnassigned() { m_mayBeUnassigned = உண்மை; }
    தருக்க mayBeUnassigned() மாறிலி { திரும்பு m_mayBeUnassigned; }
    வெறுமை setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    வெறுமை add_மதிப்பு(unsigned index, மாறிலி V3_எண்& மதிப்பு) { m_tableBuilder.add_மதிப்பு(index, மதிப்பு); }
    AstVarScope* tabeVarScopep() { திரும்பு m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

வகுப்பு TableVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    இரட்டை m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    தருக்க m_assignDly = பொய்;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

பொது:
    வெறுமை simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* மாறிலி vscp = nodep->varScopep();
        எனில் (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        எனில் (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.தள்ளு_back(vscp);
        }
    }

தனிப்பட்ட:
    தருக்க மரம்_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{இது};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        மாறிலி size_t chgWidth = m_outVarps.size();
        மாறிலி இரட்டை space = std::pow<இரட்டை>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        மாறிலி இரட்டை time  // max(_, 1), so we won't divide by zero
            = std::max<இரட்டை>(chkvis.instr_மொத்தம்() * TABLE_BYTES_PER_INST + chkvis.data_மொத்தம்(), 1);
        எனில் (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        எனில் (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        எனில் (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        எனில் (chkvis.instr_மொத்தம்() < TABLE_MIN_முனை_மொத்தம்) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        எனில் (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        எனில் (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        எனில் (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_மொத்தம்() << " Data=" << chkvis.data_மொத்தம்()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        எனில் (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        திரும்பு chkvis.optimizable();
    }

    வெறுமை replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_வரி* மாறிலி fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* மாறிலி indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        க்கு (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_முனை* மாறிலி stmtsp = create_Lookup_உள்ளீடு(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_மரம்();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    வெறுமை createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{இது};
        க்கு (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            மாறிலி uint32_t in_மதிப்பு = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_மதிப்பு);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            க்கு (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                மாறிலி AstConst cnst{invscp->fileline(), AstConst::Widthed_மதிப்பு{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_மதிப்பு >> shift)};
                simvis.new_மதிப்பு(invscp, &cnst);
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
            V3_எண் outputAssignedMask{nodep, static_cast<முழு>(m_outVarps.size()), 0};
            க்கு (TableOutputVar& tov : m_outVarps) {
                எனில் (V3_எண்* மாறிலி outnump = simvis.fetch_Out_எண்_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_மதிப்பு(in_மதிப்பு, *outnump);
                } இல்லையெனில் {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_மதிப்பு(in_மதிப்பு, outputAssignedMask);
        }  // each value
    }

    Ast_முனை* create_Lookup_உள்ளீடு(File_வரி* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_முனை_கோவை* concatp = nullptr;
        க்கு (AstVarScope* invscp : m_inVarps) {
            AstVarRef* மாறிலி refp = புதிய AstVarRef{fl, invscp, VAccess::READ};
            எனில் (concatp) {
                concatp = புதிய AstConcat{fl, refp, concatp};
            } இல்லையெனில் {
                concatp = refp;
            }
        }

        திரும்பு புதிய AstAssign{fl, புதிய AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_வரி* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* மாறிலி fromRefp = புதிய AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* மாறிலி indexRefp = புதிய AstVarRef{fl, indexp, VAccess::READ};
        திரும்பு புதிய AstArraySel{fl, fromRefp, indexRefp};
    }

    வெறுமை createOutputAssigns(Ast_முனை* nodep, Ast_முனை* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_வரி* மாறிலி fl = nodep->fileline();
        க்கு (TableOutputVar& tov : m_outVarps) {
            Ast_முனை_கோவை* மாறிலி alhsp = புதிய AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_முனை_கோவை* மாறிலி arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_முனை* outsetp = m_assignDly
                                   ? static_cast<Ast_முனை*>(புதிய AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_முனை*>(புதிய AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            எனில் (tov.mayBeUnassigned()) {
                V3_எண் outputChgMask{nodep, static_cast<முழு>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_முனை_கோவை* மாறிலி condp
                    = புதிய AstNeq{fl,
                                 புதிய AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            புதிய AstConst{fl, outputChgMask}},
                                 புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{},
                                              static_cast<முழு>(m_outVarps.size()), 0}};
                outsetp = புதிய AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        எனில் (மரம்_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

பொது:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

வெறுமை TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

வெறுமை V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("table", 0, dump_மரம்_Either_Level() >= 3);
}
