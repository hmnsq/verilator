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
ساكن constexpr صحيح جدول_MAX_بائيٽون = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ساكن constexpr صحيح جدول_ڪل_بائيٽون = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ساكن constexpr صحيح جدول_SPACE_وقت_MULT = 8;
// If < 32 instructions, not worth the effort
ساكن constexpr صحيح جدول_MIN_ڳنڍ_ڳڻپ = 32;
// Assume an instruction is 4 bytes
ساكن constexpr صحيح جدول_بائيٽون_في_INST = 4;

//######################################################################

صنف جدول_Visitor;

صنف جدول_Simulate_Visitor final : عام SimulateVisitor {
    // MEMBERS
    جدول_Visitor* ثابت m_cbthis;  ///< Class for callback

عام:
    ///< Call other-this function on all new var references
    فراغ varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    واضح جدول_Simulate_Visitor(جدول_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~جدول_Simulate_Visitor() override = افتراضي;
};

//######################################################################
// Class for holding lookup table state during construction

صنف جدول_Builder final {
    File_سٽ* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = خالي_اشارو;  // The lookup table initializer values
    AstVarScope* m_varScopep = خالي_اشارو;  // The scoped variable holding the table

عام:
    واضح جدول_Builder(File_سٽ* fl)
        : m_fl{fl} {}

    ~جدول_Builder() {
        إذا (m_consp) VL_DO_DANGLING(m_consp->delete_وڻ(), m_consp);
    }

    فراغ سيٽ_جدول_سائيز(Ast_ڳنڍ_DType* elemDType, بنانشان سائيز) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(سائيز > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحيح ويڪر = elemDType->ويڪر();
        Ast_ڳنڍ_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->ڳوليو_ٻٽو_انگ_DType(ويڪر, ويڪر, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت جدول_DTypep = جديد AstUnpackArrayDType{
            m_fl, subDTypep, جديد AstRange{m_fl, static_cast<صحيح>(سائيز), 0}};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(جدول_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? جديد AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? جديد AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : جديد AstConst{m_fl, AstConst::Widthed_قدر{}, ويڪر, 0};
        m_consp = جديد AstConsArray{m_fl, جدول_DTypep, defaultp};
    }

    فراغ جوڙيو_قدر(بنانشان اشاريو, ثابت V3_عدد& قدر) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        إذا (قدر.isString() ? قدر.منزل_String().خالي_جايون() : قدر.isEqZero()) أرجع;
        m_consp->valuep(اشاريو, جديد AstConst{m_fl, قدر});
    }

    AstVarScope* varScopep() {
        إذا (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::ڳوليو(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_وڻ(), refp);
        }
        أرجع m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

صنف جدول_Output_Var final {
    AstVarScope* ثابت m_varScopep;  // The output variable
    ثابت بنانشان m_ord;  // Output ordinal number in this block
    منطقي m_mayBeUnassigned = خطأ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    جدول_Builder m_جدول_Builder;

عام:
    جدول_Output_Var(AstVarScope* varScopep, بنانشان ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_جدول_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    string name() ثابت { أرجع varScopep()->varp()->name(); }
    بنانشان ord() ثابت { أرجع m_ord; }
    فراغ سيٽ_May_Be_Unassigned() { m_mayBeUnassigned = سچ; }
    منطقي mayBeUnassigned() ثابت { أرجع m_mayBeUnassigned; }
    فراغ سيٽ_جدول_سائيز(بنانشان سائيز) { m_جدول_Builder.سيٽ_جدول_سائيز(varScopep()->dtypep(), سائيز); }
    فراغ جوڙيو_قدر(بنانشان اشاريو, ثابت V3_عدد& قدر) { m_جدول_Builder.جوڙيو_قدر(اشاريو, قدر); }
    AstVarScope* tabeVarScopep() { أرجع m_جدول_Builder.varScopep(); }
};

//######################################################################
// Table class functions

صنف جدول_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    مزدوج m_ڪل_بائيٽون = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_اشاريو_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = خالي_اشارو;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقي m_سونپيو_Dly = خطأ;  // Consists of delayed assignments instead of normal assignments
    بنانشان m_اندريون_ويڪر_بٽ = 0;  // Input table width - in bits
    بنانشان m_ٻاهريون_ويڪر_بائيٽون = 0;  // Output table width - in bytes
    std::ويڪٽر<AstVarScope*> m_اندريون_Varps;  // Input variable list
    std::ويڪٽر<جدول_Output_Var> m_ٻاهريون_Varps;  // Output variable list

    // METHODS

عام:
    فراغ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_ٻاهريون_ويڪر_بائيٽون += nodep->varp()->dtypeSkipRefp()->ويڪر_ڪل_بائيٽون();
            m_ٻاهريون_Varps.emplace_پٺ(vscp, static_cast<بنانشان>(m_ٻاهريون_Varps.سائيز()));
        }
        إذا (nodep->access().isReadOrRW()) {
            m_اندريون_ويڪر_بٽ += nodep->varp()->ويڪر();
            m_اندريون_Varps.ڌڪو_پٺ(vscp);
        }
    }

خاص:
    منطقي وڻ_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_اندريون_ويڪر_بٽ = 0;
        m_ٻاهريون_ويڪر_بائيٽون = 0;
        m_اندريون_Varps.clear();
        m_ٻاهريون_Varps.clear();

        // Collect stats
        جدول_Simulate_Visitor chkvis{هذا};
        chkvis.main_جدول_Check(nodep);
        m_سونپيو_Dly = chkvis.is_سونپيو_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت سائيز_t chg_ويڪر = m_ٻاهريون_Varps.سائيز();
        ثابت مزدوج space = std::قوت<مزدوج>(2.0, m_اندريون_ويڪر_بٽ) * (m_ٻاهريون_ويڪر_بائيٽون + chg_ويڪر);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت مزدوج وقت_2  // max(_, 1), so we won't divide by zero
            = std::max<مزدوج>(chkvis.instr_ڳڻپ() * جدول_بائيٽون_في_INST + chkvis.معلومات_ڳڻپ(), 1);
        إذا (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        إذا (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        إذا (!m_ٻاهريون_ويڪر_بائيٽون || !m_اندريون_ويڪر_بٽ) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        إذا (chkvis.instr_ڳڻپ() < جدول_MIN_ڳنڍ_ڳڻپ) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        إذا (space > جدول_MAX_بائيٽون) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        إذا (space > وقت_2 * جدول_SPACE_وقت_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        إذا (m_ڪل_بائيٽون > جدول_ڪل_بائيٽون) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_ڳڻپ() << " Data=" << chkvis.معلومات_ڳڻپ()
                                << " in width (bits)=" << m_اندريون_ويڪر_بٽ << " out width (bytes)="
                                << m_ٻاهريون_ويڪر_بائيٽون << " Spacetime=" << (space / وقت_2) << "("
                                << space << "/" << وقت_2 << ")" << ": " << nodep);
        إذا (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / وقت_2) << " " << nodep);
            m_ڪل_بائيٽون += space;
        }
        أرجع chkvis.optimizable();
    }

    فراغ replace_With_جدول(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_سٽ* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت اشاريو_Vscp = m_اشاريو_Tmps.make(fl, m_scopep, m_اندريون_ويڪر_بٽ);

        // The 'output assigned' table builder
        جدول_Builder output_Assigned_جدول_Builder{fl};
        output_Assigned_جدول_Builder.سيٽ_جدول_سائيز(
            nodep->ڳوليو_ٻٽو_انگ_DType(m_ٻاهريون_Varps.سائيز(), m_ٻاهريون_Varps.سائيز(), VSigning::UNSIGNED),
            VL_نقاب_I(m_اندريون_ويڪر_بٽ));

        // Set sizes of output tables
        لكل (جدول_Output_Var& tov : m_ٻاهريون_Varps) tov.سيٽ_جدول_سائيز(VL_نقاب_I(m_اندريون_ويڪر_بٽ));

        // Populate the tables
        createTables(nodep, output_Assigned_جدول_Builder);

        Ast_ڳنڍ* ثابت stmtsp = create_Lookup_داخلا(fl, اشاريو_Vscp);
        createOutputAssigns(nodep, stmtsp, اشاريو_Vscp, output_Assigned_جدول_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ()->delete_وڻ();
        nodep->جوڙيو_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    فراغ createTables(AstAlways* nodep, جدول_Builder& output_Assigned_جدول_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        جدول_Simulate_Visitor simvis{هذا};
        لكل (uint32_t i = 0; i <= VL_نقاب_I(m_اندريون_ويڪر_بٽ); ++i) {
            ثابت uint32_t اندريون_قدر = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << اندريون_قدر);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t سرڪايو_2 = 0;
            لكل (AstVarScope* invscp : m_اندريون_Varps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_قدر{}, invscp->ويڪر(),
                                    VL_نقاب_I(invscp->ويڪر()) & (اندريون_قدر >> سرڪايو_2)};
                simvis.new_قدر(invscp, &cnst);
                سرڪايو_2 += invscp->ويڪر();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(سرڪايو_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_جدول_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_پيغام());

            // Build output value tables and the assigned flags table
            V3_عدد output_Assigned_نقاب{nodep, static_cast<صحيح>(m_ٻاهريون_Varps.سائيز()), 0};
            لكل (جدول_Output_Var& tov : m_ٻاهريون_Varps) {
                إذا (V3_عدد* ثابت outnump = simvis.fetch_ٻاهريون_عدد_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_نقاب.سيٽ_ٻٽو_انگ(tov.ord(), 1);  // Mark output as assigned
                    tov.جوڙيو_قدر(اندريون_قدر, *outnump);
                } وإلا {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.سيٽ_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_جدول_Builder.جوڙيو_قدر(اندريون_قدر, output_Assigned_نقاب);
        }  // each value
    }

    Ast_ڳنڍ* create_Lookup_داخلا(File_سٽ* fl, AstVarScope* اشاريو_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ڳنڍ_اظهار* concatp = خالي_اشارو;
        لكل (AstVarScope* invscp : m_اندريون_Varps) {
            AstVarRef* ثابت refp = جديد AstVarRef{fl, invscp, VAccess::READ};
            إذا (concatp) {
                concatp = جديد AstConcat{fl, refp, concatp};
            } وإلا {
                concatp = refp;
            }
        }

        أرجع جديد Ast_سونپيو{fl, جديد AstVarRef{fl, اشاريو_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_سٽ* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = جديد AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت اشاريو_Refp = جديد AstVarRef{fl, indexp, VAccess::READ};
        أرجع جديد AstArraySel{fl, fromRefp, اشاريو_Refp};
    }

    فراغ createOutputAssigns(Ast_ڳنڍ* nodep, Ast_ڳنڍ* stmtsp, AstVarScope* اشاريو_Vscp,
                             AstVarScope* output_Assigned_جدول_Vscp) {
        File_سٽ* ثابت fl = nodep->fileline();
        لكل (جدول_Output_Var& tov : m_ٻاهريون_Varps) {
            Ast_ڳنڍ_اظهار* ثابت alhsp = جديد AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ڳنڍ_اظهار* ثابت arhsp = select(fl, tov.tabeVarScopep(), اشاريو_Vscp);
            Ast_ڳنڍ* outsetp = m_سونپيو_Dly
                                   ? static_cast<Ast_ڳنڍ*>(جديد Ast_سونپيو_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ڳنڍ*>(جديد Ast_سونپيو{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            إذا (tov.mayBeUnassigned()) {
                V3_عدد output_Chg_نقاب{nodep, static_cast<صحيح>(m_ٻاهريون_Varps.سائيز()), 0};
                output_Chg_نقاب.سيٽ_ٻٽو_انگ(tov.ord(), 1);
                Ast_ڳنڍ_اظهار* ثابت condp
                    = جديد AstNeq{fl,
                                 جديد AstAnd{fl, select(fl, output_Assigned_جدول_Vscp, اشاريو_Vscp),
                                            جديد AstConst{fl, output_Chg_نقاب}},
                                 جديد AstConst{fl, AstConst::Widthed_قدر{},
                                              static_cast<صحيح>(m_ٻاهريون_Varps.سائيز()), 0}};
                outsetp = جديد AstIf{fl, condp, outsetp};
            }

            stmtsp->جوڙيو_ايندڙ(outsetp);
        }
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        إذا (وڻ_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_جدول(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عام:
    // CONSTRUCTORS
    واضح جدول_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~جدول_Visitor() override {  //
        V3Stats::جوڙيو_Stat("Optimizations, Tables created", m_statTablesCre);
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
    V3Global::dump_Check_Global_وڻ("table", 0, dump_وڻ_Either_Level() >= 3);
}
