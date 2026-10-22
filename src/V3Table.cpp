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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Table.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Simulate.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <cmath>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା TABLE_MIN_ଗଣ୍ଠି_ଗଣନା = 32;
// Assume an instruction is 4 bytes
ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା TABLE_BYTES_PER_INST = 4;

//######################################################################

ଶ୍ରେଣୀ TableVisitor;

ଶ୍ରେଣୀ TableSimulateVisitor final : ସର୍ବସାଧାରଣ SimulateVisitor {
    // MEMBERS
    TableVisitor* ଅଚଳ m_cbthis;  ///< Class for callback

ସର୍ବସାଧାରଣ:
    ///< Call other-this function on all new var references
    ଶୂନ୍ୟ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Class for holding lookup table state during construction

ଶ୍ରେଣୀ TableBuilder final {
    File_ଧାଡ଼ି* ଅଚଳ m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

ସର୍ବସାଧାରଣ:
    explicit TableBuilder(File_ଧାଡ଼ି* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        ଯଦି (m_consp) VL_DO_DANGLING(m_consp->delete_ବୃକ୍ଷ(), m_consp);
    }

    ଶୂନ୍ୟ setTableSize(Ast_ଗଣ୍ଠି_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = elemDType->width();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* ଅଚଳ tableDTypep = ନୂତନ AstUnpackArrayDType{
            m_fl, subDTypep, ନୂତନ AstRange{m_fl, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* ଅଚଳ defaultp
            = elemDType->isString()   ? ନୂତନ AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? ନୂତନ AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : ନୂତନ AstConst{m_fl, AstConst::Widthed_ମୂଲ୍ୟ{}, width, 0};
        m_consp = ନୂତନ AstConsArray{m_fl, tableDTypep, defaultp};
    }

    ଶୂନ୍ୟ add_ମୂଲ୍ୟ(unsigned index, ଅଚଳ V3_ସଂଖ୍ୟା& ମୂଲ୍ୟ) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        ଯଦି (ମୂଲ୍ୟ.isString() ? ମୂଲ୍ୟ.toString().empty() : ମୂଲ୍ୟ.isEqZero()) ଫେରନ୍ତୁ;
        m_consp->valuep(index, ନୂତନ AstConst{m_fl, ମୂଲ୍ୟ});
    }

    AstVarScope* varScopep() {
        ଯଦି (!m_varScopep) {
            AstVarRef* ଅଚଳ refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
        }
        ଫେରନ୍ତୁ m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

ଶ୍ରେଣୀ TableOutputVar final {
    AstVarScope* ଅଚଳ m_varScopep;  // The output variable
    ଅଚଳ unsigned m_ord;  // Output ordinal number in this block
    ତାର୍କିକ m_mayBeUnassigned = ମିଥ୍ୟା;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

ସର୍ବସାଧାରଣ:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() ଅଚଳ { ଫେରନ୍ତୁ m_varScopep; }
    string name() ଅଚଳ { ଫେରନ୍ତୁ varScopep()->varp()->name(); }
    unsigned ord() ଅଚଳ { ଫେରନ୍ତୁ m_ord; }
    ଶୂନ୍ୟ setMayBeUnassigned() { m_mayBeUnassigned = ସତ୍ୟ; }
    ତାର୍କିକ mayBeUnassigned() ଅଚଳ { ଫେରନ୍ତୁ m_mayBeUnassigned; }
    ଶୂନ୍ୟ setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    ଶୂନ୍ୟ add_ମୂଲ୍ୟ(unsigned index, ଅଚଳ V3_ସଂଖ୍ୟା& ମୂଲ୍ୟ) { m_tableBuilder.add_ମୂଲ୍ୟ(index, ମୂଲ୍ୟ); }
    AstVarScope* tabeVarScopep() { ଫେରନ୍ତୁ m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

ଶ୍ରେଣୀ TableVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    ଦ୍ୱିଗୁଣ m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    ତାର୍କିକ m_assignDly = ମିଥ୍ୟା;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ଅଚଳ vscp = nodep->varScopep();
        ଯଦି (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        ଯଦି (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ଠେଲ_back(vscp);
        }
    }

ବ୍ୟକ୍ତିଗତ:
    ତାର୍କିକ ବୃକ୍ଷ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{ଏହି};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ଅଚଳ size_t chgWidth = m_outVarps.size();
        ଅଚଳ ଦ୍ୱିଗୁଣ space = std::pow<ଦ୍ୱିଗୁଣ>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        ଅଚଳ ଦ୍ୱିଗୁଣ time  // max(_, 1), so we won't divide by zero
            = std::max<ଦ୍ୱିଗୁଣ>(chkvis.instr_ଗଣନା() * TABLE_BYTES_PER_INST + chkvis.data_ଗଣନା(), 1);
        ଯଦି (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        ଯଦି (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        ଯଦି (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        ଯଦି (chkvis.instr_ଗଣନା() < TABLE_MIN_ଗଣ୍ଠି_ଗଣନା) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        ଯଦି (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        ଯଦି (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        ଯଦି (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_ଗଣନା() << " Data=" << chkvis.data_ଗଣନା()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        ଯଦି (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        ଫେରନ୍ତୁ chkvis.optimizable();
    }

    ଶୂନ୍ୟ replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ଅଚଳ indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        ପାଇଁ (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = create_Lookup_ନିବେଶ(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    ଶୂନ୍ୟ createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{ଏହି};
        ପାଇଁ (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            ଅଚଳ uint32_t in_ମୂଲ୍ୟ = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_ମୂଲ୍ୟ);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            ପାଇଁ (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                ଅଚଳ AstConst cnst{invscp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_ମୂଲ୍ୟ >> shift)};
                simvis.new_ମୂଲ୍ୟ(invscp, &cnst);
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
            V3_ସଂଖ୍ୟା outputAssignedMask{nodep, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_outVarps.size()), 0};
            ପାଇଁ (TableOutputVar& tov : m_outVarps) {
                ଯଦି (V3_ସଂଖ୍ୟା* ଅଚଳ outnump = simvis.fetch_Out_ସଂଖ୍ୟା_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_ମୂଲ୍ୟ(in_ମୂଲ୍ୟ, *outnump);
                } ଅନ୍ୟଥା {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_ମୂଲ୍ୟ(in_ମୂଲ୍ୟ, outputAssignedMask);
        }  // each value
    }

    Ast_ଗଣ୍ଠି* create_Lookup_ନିବେଶ(File_ଧାଡ଼ି* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* concatp = nullptr;
        ପାଇଁ (AstVarScope* invscp : m_inVarps) {
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{fl, invscp, VAccess::READ};
            ଯଦି (concatp) {
                concatp = ନୂତନ AstConcat{fl, refp, concatp};
            } ଅନ୍ୟଥା {
                concatp = refp;
            }
        }

        ଫେରନ୍ତୁ ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_ଧାଡ଼ି* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ଅଚଳ fromRefp = ନୂତନ AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ଅଚଳ indexRefp = ନୂତନ AstVarRef{fl, indexp, VAccess::READ};
        ଫେରନ୍ତୁ ନୂତନ AstArraySel{fl, fromRefp, indexRefp};
    }

    ଶୂନ୍ୟ createOutputAssigns(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପାଇଁ (TableOutputVar& tov : m_outVarps) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ alhsp = ନୂତନ AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_ଗଣ୍ଠି* outsetp = m_assignDly
                                   ? static_cast<Ast_ଗଣ୍ଠି*>(ନୂତନ AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ଗଣ୍ଠି*>(ନୂତନ AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            ଯଦି (tov.mayBeUnassigned()) {
                V3_ସଂଖ୍ୟା outputChgMask{nodep, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp
                    = ନୂତନ AstNeq{fl,
                                 ନୂତନ AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            ନୂତନ AstConst{fl, outputChgMask}},
                                 ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{},
                                              static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_outVarps.size()), 0}};
                outsetp = ନୂତନ AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        ଯଦି (ବୃକ୍ଷ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

ଶୂନ୍ୟ TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

ଶୂନ୍ୟ V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("table", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
