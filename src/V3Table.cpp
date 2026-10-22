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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Table.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Simulate.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <cmath>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ TABLE_MIN_കണ്ണി_എണ്ണം = 32;
// Assume an instruction is 4 bytes
സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ TABLE_BYTES_PER_INST = 4;

//######################################################################

വർഗ്ഗം TableVisitor;

വർഗ്ഗം TableSimulateVisitor final : പൊതു SimulateVisitor {
    // MEMBERS
    TableVisitor* സ്ഥിരം m_cbthis;  ///< Class for callback

പൊതു:
    ///< Call other-this function on all new var references
    ശൂന്യം varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Class for holding lookup table state during construction

വർഗ്ഗം TableBuilder final {
    File_വരി* സ്ഥിരം m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

പൊതു:
    explicit TableBuilder(File_വരി* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        എങ്കിൽ (m_consp) VL_DO_DANGLING(m_consp->delete_വൃക്ഷം(), m_consp);
    }

    ശൂന്യം setTableSize(Ast_കണ്ണി_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = elemDType->width();
        Ast_കണ്ണി_DType* സ്ഥിരം subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* സ്ഥിരം tableDTypep = പുതിയ AstUnpackArrayDType{
            m_fl, subDTypep, പുതിയ AstRange{m_fl, static_cast<പൂർണ്ണസംഖ്യ>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* സ്ഥിരം defaultp
            = elemDType->isString()   ? പുതിയ AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? പുതിയ AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : പുതിയ AstConst{m_fl, AstConst::Widthed_മൂല്യം{}, width, 0};
        m_consp = പുതിയ AstConsArray{m_fl, tableDTypep, defaultp};
    }

    ശൂന്യം add_മൂല്യം(unsigned index, സ്ഥിരം V3_സംഖ്യ& മൂല്യം) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        എങ്കിൽ (മൂല്യം.isString() ? മൂല്യം.toString().empty() : മൂല്യം.isEqZero()) തിരികെ;
        m_consp->valuep(index, പുതിയ AstConst{m_fl, മൂല്യം});
    }

    AstVarScope* varScopep() {
        എങ്കിൽ (!m_varScopep) {
            AstVarRef* സ്ഥിരം refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
        }
        തിരികെ m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

വർഗ്ഗം TableOutputVar final {
    AstVarScope* സ്ഥിരം m_varScopep;  // The output variable
    സ്ഥിരം unsigned m_ord;  // Output ordinal number in this block
    തർക്കം m_mayBeUnassigned = അസത്യം;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

പൊതു:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() സ്ഥിരം { തിരികെ m_varScopep; }
    string name() സ്ഥിരം { തിരികെ varScopep()->varp()->name(); }
    unsigned ord() സ്ഥിരം { തിരികെ m_ord; }
    ശൂന്യം setMayBeUnassigned() { m_mayBeUnassigned = സത്യം; }
    തർക്കം mayBeUnassigned() സ്ഥിരം { തിരികെ m_mayBeUnassigned; }
    ശൂന്യം setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    ശൂന്യം add_മൂല്യം(unsigned index, സ്ഥിരം V3_സംഖ്യ& മൂല്യം) { m_tableBuilder.add_മൂല്യം(index, മൂല്യം); }
    AstVarScope* tabeVarScopep() { തിരികെ m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

വർഗ്ഗം TableVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    ഇരട്ട m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    തർക്കം m_assignDly = അസത്യം;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

പൊതു:
    ശൂന്യം simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        എങ്കിൽ (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.തള്ളുക_back(vscp);
        }
    }

സ്വകാര്യ:
    തർക്കം വൃക്ഷം_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{ഇത്};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        സ്ഥിരം size_t chgWidth = m_outVarps.size();
        സ്ഥിരം ഇരട്ട space = std::pow<ഇരട്ട>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        സ്ഥിരം ഇരട്ട time  // max(_, 1), so we won't divide by zero
            = std::max<ഇരട്ട>(chkvis.instr_എണ്ണം() * TABLE_BYTES_PER_INST + chkvis.data_എണ്ണം(), 1);
        എങ്കിൽ (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        എങ്കിൽ (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        എങ്കിൽ (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        എങ്കിൽ (chkvis.instr_എണ്ണം() < TABLE_MIN_കണ്ണി_എണ്ണം) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        എങ്കിൽ (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        എങ്കിൽ (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        എങ്കിൽ (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_എണ്ണം() << " Data=" << chkvis.data_എണ്ണം()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        എങ്കിൽ (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        തിരികെ chkvis.optimizable();
    }

    ശൂന്യം replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_വരി* സ്ഥിരം fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* സ്ഥിരം indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        വേണ്ടി (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_കണ്ണി* സ്ഥിരം stmtsp = create_Lookup_നിവേശം(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    ശൂന്യം createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{ഇത്};
        വേണ്ടി (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            സ്ഥിരം uint32_t in_മൂല്യം = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_മൂല്യം);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            വേണ്ടി (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                സ്ഥിരം AstConst cnst{invscp->fileline(), AstConst::Widthed_മൂല്യം{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_മൂല്യം >> shift)};
                simvis.new_മൂല്യം(invscp, &cnst);
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
            V3_സംഖ്യ outputAssignedMask{nodep, static_cast<പൂർണ്ണസംഖ്യ>(m_outVarps.size()), 0};
            വേണ്ടി (TableOutputVar& tov : m_outVarps) {
                എങ്കിൽ (V3_സംഖ്യ* സ്ഥിരം outnump = simvis.fetch_Out_സംഖ്യ_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_മൂല്യം(in_മൂല്യം, *outnump);
                } അല്ലെങ്കിൽ {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_മൂല്യം(in_മൂല്യം, outputAssignedMask);
        }  // each value
    }

    Ast_കണ്ണി* create_Lookup_നിവേശം(File_വരി* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_കണ്ണി_വ്യഞ്ജകം* concatp = nullptr;
        വേണ്ടി (AstVarScope* invscp : m_inVarps) {
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{fl, invscp, VAccess::READ};
            എങ്കിൽ (concatp) {
                concatp = പുതിയ AstConcat{fl, refp, concatp};
            } അല്ലെങ്കിൽ {
                concatp = refp;
            }
        }

        തിരികെ പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_വരി* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* സ്ഥിരം fromRefp = പുതിയ AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* സ്ഥിരം indexRefp = പുതിയ AstVarRef{fl, indexp, VAccess::READ};
        തിരികെ പുതിയ AstArraySel{fl, fromRefp, indexRefp};
    }

    ശൂന്യം createOutputAssigns(Ast_കണ്ണി* nodep, Ast_കണ്ണി* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        വേണ്ടി (TableOutputVar& tov : m_outVarps) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം alhsp = പുതിയ AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_കണ്ണി* outsetp = m_assignDly
                                   ? static_cast<Ast_കണ്ണി*>(പുതിയ AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_കണ്ണി*>(പുതിയ AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            എങ്കിൽ (tov.mayBeUnassigned()) {
                V3_സംഖ്യ outputChgMask{nodep, static_cast<പൂർണ്ണസംഖ്യ>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp
                    = പുതിയ AstNeq{fl,
                                 പുതിയ AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            പുതിയ AstConst{fl, outputChgMask}},
                                 പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{},
                                              static_cast<പൂർണ്ണസംഖ്യ>(m_outVarps.size()), 0}};
                outsetp = പുതിയ AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        എങ്കിൽ (വൃക്ഷം_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

പൊതു:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

ശൂന്യം TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

ശൂന്യം V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("table", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
