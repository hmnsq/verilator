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
ساكن constexpr صحيح جدول_MAX_بایتها = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ساكن constexpr صحيح جدول_مجموع_بایتها = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ساكن constexpr صحيح جدول_SPACE_زمان_MULT = 8;
// If < 32 instructions, not worth the effort
ساكن constexpr صحيح جدول_MIN_گره_شمار = 32;
// Assume an instruction is 4 bytes
ساكن constexpr صحيح جدول_بایتها_در_هر_INST = 4;

//######################################################################

صنف جدول_Visitor;

صنف جدول_Simulate_Visitor final : عام SimulateVisitor {
    // MEMBERS
    جدول_Visitor* ثابت m_cbthis;  ///< Class for callback

عام:
    ///< Call other-this function on all new var references
    فراغ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    صریح جدول_Simulate_Visitor(جدول_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~جدول_Simulate_Visitor() override = افتراضي;
};

//######################################################################
// Class for holding lookup table state during construction

صنف جدول_Builder final {
    File_سطر* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = اشاره_تهی;  // The lookup table initializer values
    AstVarScope* m_varScopep = اشاره_تهی;  // The scoped variable holding the table

عام:
    صریح جدول_Builder(File_سطر* fl)
        : m_fl{fl} {}

    ~جدول_Builder() {
        إذا (m_consp) VL_DO_DANGLING(m_consp->delete_درخت(), m_consp);
    }

    فراغ مجموعه_جدول_اندازه(Ast_گره_DType* elemDType, بیعلامت اندازه) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(اندازه > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحيح عرض = elemDType->عرض();
        Ast_گره_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->پیدا_کن_بیت_DType(عرض, عرض, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت جدول_DTypep = جديد AstUnpackArrayDType{
            m_fl, subDTypep, جديد AstRange{m_fl, static_cast<صحيح>(اندازه), 0}};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(جدول_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? جديد AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? جديد AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : جديد AstConst{m_fl, AstConst::Widthed_مقدار{}, عرض, 0};
        m_consp = جديد AstConsArray{m_fl, جدول_DTypep, defaultp};
    }

    فراغ جمع_کن_مقدار(بیعلامت شاخص, ثابت V3_عدد& مقدار) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        إذا (مقدار.isString() ? مقدار.مقصد_String().خالی() : مقدار.isEqZero()) أرجع;
        m_consp->valuep(شاخص, جديد AstConst{m_fl, مقدار});
    }

    AstVarScope* varScopep() {
        إذا (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::پیدا_کن(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_درخت(), refp);
        }
        أرجع m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

صنف جدول_Output_Var final {
    AstVarScope* ثابت m_varScopep;  // The output variable
    ثابت بیعلامت m_ord;  // Output ordinal number in this block
    منطقي m_mayBeUnassigned = خطأ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    جدول_Builder m_جدول_Builder;

عام:
    جدول_Output_Var(AstVarScope* varScopep, بیعلامت ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_جدول_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    string name() ثابت { أرجع varScopep()->varp()->name(); }
    بیعلامت ord() ثابت { أرجع m_ord; }
    فراغ مجموعه_May_Be_Unassigned() { m_mayBeUnassigned = درست; }
    منطقي mayBeUnassigned() ثابت { أرجع m_mayBeUnassigned; }
    فراغ مجموعه_جدول_اندازه(بیعلامت اندازه) { m_جدول_Builder.مجموعه_جدول_اندازه(varScopep()->dtypep(), اندازه); }
    فراغ جمع_کن_مقدار(بیعلامت شاخص, ثابت V3_عدد& مقدار) { m_جدول_Builder.جمع_کن_مقدار(شاخص, مقدار); }
    AstVarScope* tabeVarScopep() { أرجع m_جدول_Builder.varScopep(); }
};

//######################################################################
// Table class functions

صنف جدول_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    مزدوج m_مجموع_بایتها = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_شاخص_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = اشاره_تهی;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقي m_تخصیص_بده_Dly = خطأ;  // Consists of delayed assignments instead of normal assignments
    بیعلامت m_درون_عرض_بیتها = 0;  // Input table width - in bits
    بیعلامت m_بیرون_عرض_بایتها = 0;  // Output table width - in bytes
    std::وکتور<AstVarScope*> m_درون_Varps;  // Input variable list
    std::وکتور<جدول_Output_Var> m_بیرون_Varps;  // Output variable list

    // METHODS

عام:
    فراغ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_بیرون_عرض_بایتها += nodep->varp()->dtypeSkipRefp()->عرض_مجموع_بایتها();
            m_بیرون_Varps.emplace_پشت(vscp, static_cast<بیعلامت>(m_بیرون_Varps.اندازه()));
        }
        إذا (nodep->access().isReadOrRW()) {
            m_درون_عرض_بیتها += nodep->varp()->عرض();
            m_درون_Varps.افزودن_پشت(vscp);
        }
    }

خاص:
    منطقي درخت_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_درون_عرض_بیتها = 0;
        m_بیرون_عرض_بایتها = 0;
        m_درون_Varps.clear();
        m_بیرون_Varps.clear();

        // Collect stats
        جدول_Simulate_Visitor chkvis{هذا};
        chkvis.main_جدول_Check(nodep);
        m_تخصیص_بده_Dly = chkvis.is_تخصیص_بده_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت اندازه_t chg_عرض = m_بیرون_Varps.اندازه();
        ثابت مزدوج space = std::توان<مزدوج>(2.0, m_درون_عرض_بیتها) * (m_بیرون_عرض_بایتها + chg_عرض);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت مزدوج زمان_2  // max(_, 1), so we won't divide by zero
            = std::max<مزدوج>(chkvis.instr_شمار() * جدول_بایتها_در_هر_INST + chkvis.داده_شمار(), 1);
        إذا (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        إذا (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        إذا (!m_بیرون_عرض_بایتها || !m_درون_عرض_بیتها) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        إذا (chkvis.instr_شمار() < جدول_MIN_گره_شمار) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        إذا (space > جدول_MAX_بایتها) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        إذا (space > زمان_2 * جدول_SPACE_زمان_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        إذا (m_مجموع_بایتها > جدول_مجموع_بایتها) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_شمار() << " Data=" << chkvis.داده_شمار()
                                << " in width (bits)=" << m_درون_عرض_بیتها << " out width (bytes)="
                                << m_بیرون_عرض_بایتها << " Spacetime=" << (space / زمان_2) << "("
                                << space << "/" << زمان_2 << ")" << ": " << nodep);
        إذا (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / زمان_2) << " " << nodep);
            m_مجموع_بایتها += space;
        }
        أرجع chkvis.optimizable();
    }

    فراغ replace_With_جدول(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_سطر* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت شاخص_Vscp = m_شاخص_Tmps.make(fl, m_scopep, m_درون_عرض_بیتها);

        // The 'output assigned' table builder
        جدول_Builder output_Assigned_جدول_Builder{fl};
        output_Assigned_جدول_Builder.مجموعه_جدول_اندازه(
            nodep->پیدا_کن_بیت_DType(m_بیرون_Varps.اندازه(), m_بیرون_Varps.اندازه(), VSigning::UNSIGNED),
            VL_ماسک_I(m_درون_عرض_بیتها));

        // Set sizes of output tables
        لكل (جدول_Output_Var& tov : m_بیرون_Varps) tov.مجموعه_جدول_اندازه(VL_ماسک_I(m_درون_عرض_بیتها));

        // Populate the tables
        createTables(nodep, output_Assigned_جدول_Builder);

        Ast_گره* ثابت stmtsp = create_Lookup_درون_داد(fl, شاخص_Vscp);
        createOutputAssigns(nodep, stmtsp, شاخص_Vscp, output_Assigned_جدول_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_پشت_With_بعدی()->delete_درخت();
        nodep->جمع_کن_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    فراغ createTables(AstAlways* nodep, جدول_Builder& output_Assigned_جدول_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        جدول_Simulate_Visitor simvis{هذا};
        لكل (uint32_t i = 0; i <= VL_ماسک_I(m_درون_عرض_بیتها); ++i) {
            ثابت uint32_t درون_مقدار = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << درون_مقدار);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t لغزش_2 = 0;
            لكل (AstVarScope* invscp : m_درون_Varps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_مقدار{}, invscp->عرض(),
                                    VL_ماسک_I(invscp->عرض()) & (درون_مقدار >> لغزش_2)};
                simvis.new_مقدار(invscp, &cnst);
                لغزش_2 += invscp->عرض();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(لغزش_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_جدول_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_پیام());

            // Build output value tables and the assigned flags table
            V3_عدد output_Assigned_ماسک{nodep, static_cast<صحيح>(m_بیرون_Varps.اندازه()), 0};
            لكل (جدول_Output_Var& tov : m_بیرون_Varps) {
                إذا (V3_عدد* ثابت outnump = simvis.fetch_بیرون_عدد_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_ماسک.مجموعه_بیت(tov.ord(), 1);  // Mark output as assigned
                    tov.جمع_کن_مقدار(درون_مقدار, *outnump);
                } وإلا {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.مجموعه_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_جدول_Builder.جمع_کن_مقدار(درون_مقدار, output_Assigned_ماسک);
        }  // each value
    }

    Ast_گره* create_Lookup_درون_داد(File_سطر* fl, AstVarScope* شاخص_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_گره_عبارت* concatp = اشاره_تهی;
        لكل (AstVarScope* invscp : m_درون_Varps) {
            AstVarRef* ثابت refp = جديد AstVarRef{fl, invscp, VAccess::READ};
            إذا (concatp) {
                concatp = جديد AstConcat{fl, refp, concatp};
            } وإلا {
                concatp = refp;
            }
        }

        أرجع جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, شاخص_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_سطر* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = جديد AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت شاخص_Refp = جديد AstVarRef{fl, indexp, VAccess::READ};
        أرجع جديد AstArraySel{fl, fromRefp, شاخص_Refp};
    }

    فراغ createOutputAssigns(Ast_گره* nodep, Ast_گره* stmtsp, AstVarScope* شاخص_Vscp,
                             AstVarScope* output_Assigned_جدول_Vscp) {
        File_سطر* ثابت fl = nodep->fileline();
        لكل (جدول_Output_Var& tov : m_بیرون_Varps) {
            Ast_گره_عبارت* ثابت alhsp = جديد AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_گره_عبارت* ثابت arhsp = select(fl, tov.tabeVarScopep(), شاخص_Vscp);
            Ast_گره* outsetp = m_تخصیص_بده_Dly
                                   ? static_cast<Ast_گره*>(جديد Ast_تخصیص_بده_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_گره*>(جديد Ast_تخصیص_بده{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            إذا (tov.mayBeUnassigned()) {
                V3_عدد output_Chg_ماسک{nodep, static_cast<صحيح>(m_بیرون_Varps.اندازه()), 0};
                output_Chg_ماسک.مجموعه_بیت(tov.ord(), 1);
                Ast_گره_عبارت* ثابت condp
                    = جديد AstNeq{fl,
                                 جديد AstAnd{fl, select(fl, output_Assigned_جدول_Vscp, شاخص_Vscp),
                                            جديد AstConst{fl, output_Chg_ماسک}},
                                 جديد AstConst{fl, AstConst::Widthed_مقدار{},
                                              static_cast<صحيح>(m_بیرون_Varps.اندازه()), 0}};
                outsetp = جديد AstIf{fl, condp, outsetp};
            }

            stmtsp->جمع_کن_بعدی(outsetp);
        }
    }

    // VISITORS
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        إذا (درخت_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_جدول(nodep);
        }
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عام:
    // CONSTRUCTORS
    صریح جدول_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~جدول_Visitor() override {  //
        V3Stats::جمع_کن_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

فراغ جدول_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

فراغ V3_جدول::جدول_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { جدول_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("table", 0, dump_درخت_Either_Level() >= 3);
}
