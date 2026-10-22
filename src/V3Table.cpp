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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Table.h"

#समावेश "V3ConstPool.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Simulate.h"
#समावेश "V3Stats.h"

#समावेश <cmath>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
अटल constexpr पूर्णांक TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
अटल constexpr पूर्णांक TABLE_योग_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
अटल constexpr पूर्णांक TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
अटल constexpr पूर्णांक TABLE_MIN_गाँठ_गिनती = 32;
// Assume an instruction is 4 bytes
अटल constexpr पूर्णांक TABLE_BYTES_PER_INST = 4;

//######################################################################

श्रेणी TableVisitor;

श्रेणी TableSimulateVisitor final : खुला SimulateVisitor {
    // MEMBERS
    TableVisitor* स्थिर m_cbthis;  ///< Class for callback

खुला:
    ///< Call other-this function on all new var references
    व्योम varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = शेष;
};

//######################################################################
// Class for holding lookup table state during construction

श्रेणी TableBuilder final {
    File_पंक्ति* स्थिर m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

खुला:
    explicit TableBuilder(File_पंक्ति* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        अगर (m_consp) VL_DO_DANGLING(m_consp->delete_वृक्ष(), m_consp);
    }

    व्योम set_Table_आकार(Ast_गाँठ_DType* elemDType, अचिन्हित आकार) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(आकार > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        स्थिर पूर्णांक width = elemDType->width();
        Ast_गाँठ_DType* स्थिर subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* स्थिर tableDTypep = नया AstUnpackArrayDType{
            m_fl, subDTypep, नया AstRange{m_fl, static_cast<पूर्णांक>(आकार), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* स्थिर defaultp
            = elemDType->isString()   ? नया AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? नया AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : नया AstConst{m_fl, AstConst::Widthed_मूल्य{}, width, 0};
        m_consp = नया AstConsArray{m_fl, tableDTypep, defaultp};
    }

    व्योम add_मूल्य(अचिन्हित सूचकांक_2, स्थिर V3_संख्या& मूल्य) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        अगर (मूल्य.isString() ? मूल्य.toString().empty() : मूल्य.isEqZero()) वापस;
        m_consp->valuep(सूचकांक_2, नया AstConst{m_fl, मूल्य});
    }

    AstVarScope* varScopep() {
        अगर (!m_varScopep) {
            AstVarRef* स्थिर refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
        }
        वापस m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

श्रेणी TableOutputVar final {
    AstVarScope* स्थिर m_varScopep;  // The output variable
    स्थिर अचिन्हित m_ord;  // Output ordinal number in this block
    bool m_mayBeUnassigned = false;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

खुला:
    TableOutputVar(AstVarScope* varScopep, अचिन्हित ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() स्थिर { वापस m_varScopep; }
    string name() स्थिर { वापस varScopep()->varp()->name(); }
    अचिन्हित ord() स्थिर { वापस m_ord; }
    व्योम setMayBeUnassigned() { m_mayBeUnassigned = true; }
    bool mayBeUnassigned() स्थिर { वापस m_mayBeUnassigned; }
    व्योम set_Table_आकार(अचिन्हित आकार) { m_tableBuilder.set_Table_आकार(varScopep()->dtypep(), आकार); }
    व्योम add_मूल्य(अचिन्हित सूचकांक_2, स्थिर V3_संख्या& मूल्य) { m_tableBuilder.add_मूल्य(सूचकांक_2, मूल्य); }
    AstVarScope* tabeVarScopep() { वापस m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

श्रेणी TableVisitor final : खुला VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    द्विगुन m_योग_Bytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_सूचकांक_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    bool m_assignDly = false;  // Consists of delayed assignments instead of normal assignments
    अचिन्हित m_inWidthBits = 0;  // Input table width - in bits
    अचिन्हित m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

खुला:
    व्योम simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* स्थिर vscp = nodep->varScopep();
        अगर (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->width_योग_Bytes();
            m_outVarps.emplace_back(vscp, static_cast<अचिन्हित>(m_outVarps.आकार()));
        }
        अगर (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.धकेलो_back(vscp);
        }
    }

निजी:
    bool वृक्ष_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.सफा();
        m_outVarps.सफा();

        // Collect stats
        TableSimulateVisitor chkvis{यह};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        स्थिर माप_प्रकार chgWidth = m_outVarps.आकार();
        स्थिर द्विगुन space = std::घात<द्विगुन>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        स्थिर द्विगुन समय  // max(_, 1), so we won't divide by zero
            = std::max<द्विगुन>(chkvis.instr_गिनती() * TABLE_BYTES_PER_INST + chkvis.तथ्य_गिनती(), 1);
        अगर (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        अगर (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        अगर (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        अगर (chkvis.instr_गिनती() < TABLE_MIN_गाँठ_गिनती) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        अगर (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        अगर (space > समय * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        अगर (m_योग_Bytes > TABLE_योग_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_गिनती() << " Data=" << chkvis.तथ्य_गिनती()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / समय) << "("
                                << space << "/" << समय << ")" << ": " << nodep);
        अगर (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / समय) << " " << nodep);
            m_योग_Bytes += space;
        }
        वापस chkvis.optimizable();
    }

    व्योम replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_पंक्ति* स्थिर fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* स्थिर सूचकांक_Vscp = m_सूचकांक_Tmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.set_Table_आकार(
            nodep->findBitDType(m_outVarps.आकार(), m_outVarps.आकार(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        क्रम (TableOutputVar& tov : m_outVarps) tov.set_Table_आकार(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_गाँठ* स्थिर stmtsp = create_Lookup_निवेश(fl, सूचकांक_Vscp);
        createOutputAssigns(nodep, stmtsp, सूचकांक_Vscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    व्योम createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{यह};
        क्रम (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            स्थिर uint32_t in_मूल्य = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::षष्ठ << in_मूल्य);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.सफा();

            // Set all inputs to the constant
            uint32_t shift = 0;
            क्रम (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                स्थिर AstConst cnst{invscp->fileline(), AstConst::Widthed_मूल्य{}, invscp->width(),
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
            V3_संख्या outputAssignedMask{nodep, static_cast<पूर्णांक>(m_outVarps.आकार()), 0};
            क्रम (TableOutputVar& tov : m_outVarps) {
                अगर (V3_संख्या* स्थिर outnump = simvis.fetch_Out_संख्या_Null(tov.varScopep())) {
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

    Ast_गाँठ* create_Lookup_निवेश(File_पंक्ति* fl, AstVarScope* सूचकांक_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_गाँठ_व्यंजक* concatp = nullptr;
        क्रम (AstVarScope* invscp : m_inVarps) {
            AstVarRef* स्थिर refp = नया AstVarRef{fl, invscp, VAccess::READ};
            अगर (concatp) {
                concatp = नया AstConcat{fl, refp, concatp};
            } अन्यथा {
                concatp = refp;
            }
        }

        वापस नया AstAssign{fl, नया AstVarRef{fl, सूचकांक_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_पंक्ति* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* स्थिर fromRefp = नया AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* स्थिर सूचकांक_Refp = नया AstVarRef{fl, indexp, VAccess::READ};
        वापस नया AstArraySel{fl, fromRefp, सूचकांक_Refp};
    }

    व्योम createOutputAssigns(Ast_गाँठ* nodep, Ast_गाँठ* stmtsp, AstVarScope* सूचकांक_Vscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_पंक्ति* स्थिर fl = nodep->fileline();
        क्रम (TableOutputVar& tov : m_outVarps) {
            Ast_गाँठ_व्यंजक* स्थिर alhsp = नया AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_गाँठ_व्यंजक* स्थिर arhsp = select(fl, tov.tabeVarScopep(), सूचकांक_Vscp);
            Ast_गाँठ* outsetp = m_assignDly
                                   ? static_cast<Ast_गाँठ*>(नया AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_गाँठ*>(नया AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            अगर (tov.mayBeUnassigned()) {
                V3_संख्या outputChgMask{nodep, static_cast<पूर्णांक>(m_outVarps.आकार()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_गाँठ_व्यंजक* स्थिर condp
                    = नया AstNeq{fl,
                                 नया AstAnd{fl, select(fl, outputAssignedTableVscp, सूचकांक_Vscp),
                                            नया AstConst{fl, outputChgMask}},
                                 नया AstConst{fl, AstConst::Widthed_मूल्य{},
                                              static_cast<पूर्णांक>(m_outVarps.आकार()), 0}};
                outsetp = नया AstIf{fl, condp, outsetp};
            }

            stmtsp->add_अग्र(outsetp);
        }
    }

    // VISITORS
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        अगर (वृक्ष_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

खुला:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

व्योम TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

व्योम V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("table", 0, dump_वृक्ष_Either_Level() >= 3);
}
