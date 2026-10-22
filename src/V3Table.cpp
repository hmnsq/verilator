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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Table.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Simulate.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <cmath>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
स्थिर constexpr पूर्णांक TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
स्थिर constexpr पूर्णांक TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
स्थिर constexpr पूर्णांक TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
स्थिर constexpr पूर्णांक TABLE_MIN_गाठ_मोजणी = 32;
// Assume an instruction is 4 bytes
स्थिर constexpr पूर्णांक TABLE_BYTES_PER_INST = 4;

//######################################################################

वर्ग TableVisitor;

वर्ग TableSimulateVisitor final : सार्वजनिक SimulateVisitor {
    // MEMBERS
    TableVisitor* स्थिरांक m_cbthis;  ///< Class for callback

सार्वजनिक:
    ///< Call other-this function on all new var references
    शून्य varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Class for holding lookup table state during construction

वर्ग TableBuilder final {
    File_ओळ* स्थिरांक m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

सार्वजनिक:
    explicit TableBuilder(File_ओळ* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        जर (m_consp) VL_DO_DANGLING(m_consp->delete_वृक्ष(), m_consp);
    }

    शून्य setTableSize(Ast_गाठ_DType* elemDType, अचिन्हित size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        स्थिरांक पूर्णांक width = elemDType->width();
        Ast_गाठ_DType* स्थिरांक subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* स्थिरांक tableDTypep = नवीन AstUnpackArrayDType{
            m_fl, subDTypep, नवीन AstRange{m_fl, static_cast<पूर्णांक>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* स्थिरांक defaultp
            = elemDType->isString()   ? नवीन AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? नवीन AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : नवीन AstConst{m_fl, AstConst::Widthed_मूल्य{}, width, 0};
        m_consp = नवीन AstConsArray{m_fl, tableDTypep, defaultp};
    }

    शून्य add_मूल्य(अचिन्हित index, स्थिरांक V3_संख्या& मूल्य) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        जर (मूल्य.isString() ? मूल्य.toString().empty() : मूल्य.isEqZero()) परत;
        m_consp->valuep(index, नवीन AstConst{m_fl, मूल्य});
    }

    AstVarScope* varScopep() {
        जर (!m_varScopep) {
            AstVarRef* स्थिरांक refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
        }
        परत m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

वर्ग TableOutputVar final {
    AstVarScope* स्थिरांक m_varScopep;  // The output variable
    स्थिरांक अचिन्हित m_ord;  // Output ordinal number in this block
    तार्किक m_mayBeUnassigned = असत्य;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

सार्वजनिक:
    TableOutputVar(AstVarScope* varScopep, अचिन्हित ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() स्थिरांक { परत m_varScopep; }
    string name() स्थिरांक { परत varScopep()->varp()->name(); }
    अचिन्हित ord() स्थिरांक { परत m_ord; }
    शून्य setMayBeUnassigned() { m_mayBeUnassigned = सत्य; }
    तार्किक mayBeUnassigned() स्थिरांक { परत m_mayBeUnassigned; }
    शून्य setTableSize(अचिन्हित size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    शून्य add_मूल्य(अचिन्हित index, स्थिरांक V3_संख्या& मूल्य) { m_tableBuilder.add_मूल्य(index, मूल्य); }
    AstVarScope* tabeVarScopep() { परत m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

वर्ग TableVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    द्विगुण m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    तार्किक m_assignDly = असत्य;  // Consists of delayed assignments instead of normal assignments
    अचिन्हित m_inWidthBits = 0;  // Input table width - in bits
    अचिन्हित m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

सार्वजनिक:
    शून्य simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* स्थिरांक vscp = nodep->varScopep();
        जर (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<अचिन्हित>(m_outVarps.size()));
        }
        जर (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.ढकला_back(vscp);
        }
    }

खाजगी:
    तार्किक वृक्ष_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{हे};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        स्थिरांक माप_प्रकार chgWidth = m_outVarps.size();
        स्थिरांक द्विगुण space = std::घात<द्विगुण>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        स्थिरांक द्विगुण समय  // max(_, 1), so we won't divide by zero
            = std::max<द्विगुण>(chkvis.instr_मोजणी() * TABLE_BYTES_PER_INST + chkvis.data_मोजणी(), 1);
        जर (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        जर (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        जर (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        जर (chkvis.instr_मोजणी() < TABLE_MIN_गाठ_मोजणी) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        जर (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        जर (space > समय * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        जर (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_मोजणी() << " Data=" << chkvis.data_मोजणी()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / समय) << "("
                                << space << "/" << समय << ")" << ": " << nodep);
        जर (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / समय) << " " << nodep);
            m_totalBytes += space;
        }
        परत chkvis.optimizable();
    }

    शून्य replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_ओळ* स्थिरांक fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* स्थिरांक indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        साठी (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_गाठ* स्थिरांक stmtsp = create_Lookup_आदान(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_वृक्ष();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    शून्य createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{हे};
        साठी (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            स्थिरांक uint32_t in_मूल्य = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_मूल्य);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            साठी (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                स्थिरांक AstConst cnst{invscp->fileline(), AstConst::Widthed_मूल्य{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_मूल्य >> shift)};
                simvis.new_मूल्य(invscp, &cnst);
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
            V3_संख्या outputAssignedMask{nodep, static_cast<पूर्णांक>(m_outVarps.size()), 0};
            साठी (TableOutputVar& tov : m_outVarps) {
                जर (V3_संख्या* स्थिरांक outnump = simvis.fetch_Out_संख्या_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_मूल्य(in_मूल्य, *outnump);
                } अन्यथा {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_मूल्य(in_मूल्य, outputAssignedMask);
        }  // each value
    }

    Ast_गाठ* create_Lookup_आदान(File_ओळ* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_गाठ_अभिव्यक्ती* concatp = nullptr;
        साठी (AstVarScope* invscp : m_inVarps) {
            AstVarRef* स्थिरांक refp = नवीन AstVarRef{fl, invscp, VAccess::READ};
            जर (concatp) {
                concatp = नवीन AstConcat{fl, refp, concatp};
            } अन्यथा {
                concatp = refp;
            }
        }

        परत नवीन AstAssign{fl, नवीन AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_ओळ* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* स्थिरांक fromRefp = नवीन AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* स्थिरांक indexRefp = नवीन AstVarRef{fl, indexp, VAccess::READ};
        परत नवीन AstArraySel{fl, fromRefp, indexRefp};
    }

    शून्य createOutputAssigns(Ast_गाठ* nodep, Ast_गाठ* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_ओळ* स्थिरांक fl = nodep->fileline();
        साठी (TableOutputVar& tov : m_outVarps) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक alhsp = नवीन AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_गाठ* outsetp = m_assignDly
                                   ? static_cast<Ast_गाठ*>(नवीन AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_गाठ*>(नवीन AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            जर (tov.mayBeUnassigned()) {
                V3_संख्या outputChgMask{nodep, static_cast<पूर्णांक>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक condp
                    = नवीन AstNeq{fl,
                                 नवीन AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            नवीन AstConst{fl, outputChgMask}},
                                 नवीन AstConst{fl, AstConst::Widthed_मूल्य{},
                                              static_cast<पूर्णांक>(m_outVarps.size()), 0}};
                outsetp = नवीन AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        जर (वृक्ष_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

शून्य TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

शून्य V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("table", 0, dump_वृक्ष_Either_Level() >= 3);
}
