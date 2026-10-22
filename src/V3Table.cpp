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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Table.h"

#شامل "V3ConstPool.h"
#شامل "V3SharedTmps.h"
#شامل "V3Simulate.h"
#شامل "V3Stats.h"

#شامل <cmath>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Table class functions

// CONFIG
// 1MB is max table size (better be lots of instructs to be worth it!)
ایستا constexpr صحیح TABLE_MAX_BYTES = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ایستا constexpr صحیح TABLE_TOTAL_BYTES = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ایستا constexpr صحیح TABLE_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
ایستا constexpr صحیح TABLE_MIN_گره_شمار = 32;
// Assume an instruction is 4 bytes
ایستا constexpr صحیح TABLE_BYTES_PER_INST = 4;

//######################################################################

رده TableVisitor;

رده TableSimulateVisitor final : عمومی SimulateVisitor {
    // MEMBERS
    TableVisitor* ثابت m_cbthis;  ///< Class for callback

عمومی:
    ///< Call other-this function on all new var references
    تهی varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    explicit TableSimulateVisitor(TableVisitor* cbthis)
        : m_cbthis{cbthis} {}
    ~TableSimulateVisitor() override = پیشفرض;
};

//######################################################################
// Class for holding lookup table state during construction

رده TableBuilder final {
    File_سطر* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = nullptr;  // The lookup table initializer values
    AstVarScope* m_varScopep = nullptr;  // The scoped variable holding the table

عمومی:
    explicit TableBuilder(File_سطر* fl)
        : m_fl{fl} {}

    ~TableBuilder() {
        اگر (m_consp) VL_DO_DANGLING(m_consp->delete_درخت(), m_consp);
    }

    تهی setTableSize(Ast_گره_DType* elemDType, unsigned size) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(size > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحیح width = elemDType->width();
        Ast_گره_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(width, width, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت tableDTypep = جدید AstUnpackArrayDType{
            m_fl, subDTypep, جدید AstRange{m_fl, static_cast<صحیح>(size), 0}};
        v3Global.rootp()->typeTablep()->addTypesp(tableDTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? جدید AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? جدید AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : جدید AstConst{m_fl, AstConst::Widthed_مقدار{}, width, 0};
        m_consp = جدید AstConsArray{m_fl, tableDTypep, defaultp};
    }

    تهی add_مقدار(unsigned index, ثابت V3_عدد& مقدار) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        اگر (مقدار.isString() ? مقدار.toString().empty() : مقدار.isEqZero()) بازگشت;
        m_consp->valuep(index, جدید AstConst{m_fl, مقدار});
    }

    AstVarScope* varScopep() {
        اگر (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_درخت(), refp);
        }
        بازگشت m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

رده TableOutputVar final {
    AstVarScope* ثابت m_varScopep;  // The output variable
    ثابت unsigned m_ord;  // Output ordinal number in this block
    منطقی m_mayBeUnassigned = نادرست;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    TableBuilder m_tableBuilder;

عمومی:
    TableOutputVar(AstVarScope* varScopep, unsigned ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_tableBuilder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { بازگشت m_varScopep; }
    string name() ثابت { بازگشت varScopep()->varp()->name(); }
    unsigned ord() ثابت { بازگشت m_ord; }
    تهی setMayBeUnassigned() { m_mayBeUnassigned = درست; }
    منطقی mayBeUnassigned() ثابت { بازگشت m_mayBeUnassigned; }
    تهی setTableSize(unsigned size) { m_tableBuilder.setTableSize(varScopep()->dtypep(), size); }
    تهی add_مقدار(unsigned index, ثابت V3_عدد& مقدار) { m_tableBuilder.add_مقدار(index, مقدار); }
    AstVarScope* tabeVarScopep() { بازگشت m_tableBuilder.varScopep(); }
};

//######################################################################
// Table class functions

رده TableVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    دوگانه m_totalBytes = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_indexTmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = nullptr;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقی m_assignDly = نادرست;  // Consists of delayed assignments instead of normal assignments
    unsigned m_inWidthBits = 0;  // Input table width - in bits
    unsigned m_outWidthBytes = 0;  // Output table width - in bytes
    std::vector<AstVarScope*> m_inVarps;  // Input variable list
    std::vector<TableOutputVar> m_outVarps;  // Output variable list

    // METHODS

عمومی:
    تهی simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_outWidthBytes += nodep->varp()->dtypeSkipRefp()->widthTotalBytes();
            m_outVarps.emplace_back(vscp, static_cast<unsigned>(m_outVarps.size()));
        }
        اگر (nodep->access().isReadOrRW()) {
            m_inWidthBits += nodep->varp()->width();
            m_inVarps.افزودن_back(vscp);
        }
    }

خصوصی:
    منطقی درخت_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_inWidthBits = 0;
        m_outWidthBytes = 0;
        m_inVarps.clear();
        m_outVarps.clear();

        // Collect stats
        TableSimulateVisitor chkvis{این};
        chkvis.mainTableCheck(nodep);
        m_assignDly = chkvis.isAssignDly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت size_t chgWidth = m_outVarps.size();
        ثابت دوگانه space = std::pow<دوگانه>(2.0, m_inWidthBits) * (m_outWidthBytes + chgWidth);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت دوگانه time  // max(_, 1), so we won't divide by zero
            = std::max<دوگانه>(chkvis.instr_شمار() * TABLE_BYTES_PER_INST + chkvis.data_شمار(), 1);
        اگر (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        اگر (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        اگر (!m_outWidthBytes || !m_inWidthBits) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        اگر (chkvis.instr_شمار() < TABLE_MIN_گره_شمار) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        اگر (space > TABLE_MAX_BYTES) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        اگر (space > time * TABLE_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        اگر (m_totalBytes > TABLE_TOTAL_BYTES) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_شمار() << " Data=" << chkvis.data_شمار()
                                << " in width (bits)=" << m_inWidthBits << " out width (bytes)="
                                << m_outWidthBytes << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        اگر (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_totalBytes += space;
        }
        بازگشت chkvis.optimizable();
    }

    تهی replaceWithTable(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_سطر* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت indexVscp = m_indexTmps.make(fl, m_scopep, m_inWidthBits);

        // The 'output assigned' table builder
        TableBuilder outputAssignedTableBuilder{fl};
        outputAssignedTableBuilder.setTableSize(
            nodep->findBitDType(m_outVarps.size(), m_outVarps.size(), VSigning::UNSIGNED),
            VL_MASK_I(m_inWidthBits));

        // Set sizes of output tables
        برای (TableOutputVar& tov : m_outVarps) tov.setTableSize(VL_MASK_I(m_inWidthBits));

        // Populate the tables
        createTables(nodep, outputAssignedTableBuilder);

        Ast_گره* ثابت stmtsp = create_Lookup_درون_داد(fl, indexVscp);
        createOutputAssigns(nodep, stmtsp, indexVscp, outputAssignedTableBuilder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlinkFrBackWithNext()->delete_درخت();
        nodep->addStmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    تهی createTables(AstAlways* nodep, TableBuilder& outputAssignedTableBuilder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        TableSimulateVisitor simvis{این};
        برای (uint32_t i = 0; i <= VL_MASK_I(m_inWidthBits); ++i) {
            ثابت uint32_t in_مقدار = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << in_مقدار);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t shift = 0;
            برای (AstVarScope* invscp : m_inVarps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_مقدار{}, invscp->width(),
                                    VL_MASK_I(invscp->width()) & (in_مقدار >> shift)};
                simvis.new_مقدار(invscp, &cnst);
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
            V3_عدد outputAssignedMask{nodep, static_cast<صحیح>(m_outVarps.size()), 0};
            برای (TableOutputVar& tov : m_outVarps) {
                اگر (V3_عدد* ثابت outnump = simvis.fetch_Out_عدد_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    outputAssignedMask.setBit(tov.ord(), 1);  // Mark output as assigned
                    tov.add_مقدار(in_مقدار, *outnump);
                } وگرنه {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.setMayBeUnassigned();
                }
            }

            // Set changed table
            outputAssignedTableBuilder.add_مقدار(in_مقدار, outputAssignedMask);
        }  // each value
    }

    Ast_گره* create_Lookup_درون_داد(File_سطر* fl, AstVarScope* indexVscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_گره_عبارت* concatp = nullptr;
        برای (AstVarScope* invscp : m_inVarps) {
            AstVarRef* ثابت refp = جدید AstVarRef{fl, invscp, VAccess::READ};
            اگر (concatp) {
                concatp = جدید AstConcat{fl, refp, concatp};
            } وگرنه {
                concatp = refp;
            }
        }

        بازگشت جدید AstAssign{fl, جدید AstVarRef{fl, indexVscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_سطر* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = جدید AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت indexRefp = جدید AstVarRef{fl, indexp, VAccess::READ};
        بازگشت جدید AstArraySel{fl, fromRefp, indexRefp};
    }

    تهی createOutputAssigns(Ast_گره* nodep, Ast_گره* stmtsp, AstVarScope* indexVscp,
                             AstVarScope* outputAssignedTableVscp) {
        File_سطر* ثابت fl = nodep->fileline();
        برای (TableOutputVar& tov : m_outVarps) {
            Ast_گره_عبارت* ثابت alhsp = جدید AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_گره_عبارت* ثابت arhsp = select(fl, tov.tabeVarScopep(), indexVscp);
            Ast_گره* outsetp = m_assignDly
                                   ? static_cast<Ast_گره*>(جدید AstAssignDly{fl, alhsp, arhsp})
                                   : static_cast<Ast_گره*>(جدید AstAssign{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            اگر (tov.mayBeUnassigned()) {
                V3_عدد outputChgMask{nodep, static_cast<صحیح>(m_outVarps.size()), 0};
                outputChgMask.setBit(tov.ord(), 1);
                Ast_گره_عبارت* ثابت condp
                    = جدید AstNeq{fl,
                                 جدید AstAnd{fl, select(fl, outputAssignedTableVscp, indexVscp),
                                            جدید AstConst{fl, outputChgMask}},
                                 جدید AstConst{fl, AstConst::Widthed_مقدار{},
                                              static_cast<صحیح>(m_outVarps.size()), 0}};
                outsetp = جدید AstIf{fl, condp, outsetp};
            }

            stmtsp->addNext(outsetp);
        }
    }

    // VISITORS
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }
    تهی visit(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        اگر (درخت_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replaceWithTable(nodep);
        }
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عمومی:
    // CONSTRUCTORS
    explicit TableVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TableVisitor() override {  //
        V3Stats::addStat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

تهی TableSimulateVisitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

تهی V3Table::tableAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TableVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("table", 0, dump_درخت_Either_Level() >= 3);
}
