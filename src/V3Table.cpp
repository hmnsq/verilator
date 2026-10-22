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
ساكن constexpr صحيح جدول_MAX_بائٹاں = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
ساكن constexpr صحيح جدول_کل_بائٹاں = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
ساكن constexpr صحيح جدول_SPACE_ویلا_MULT = 8;
// If < 32 instructions, not worth the effort
ساكن constexpr صحيح جدول_MIN_گرہ_گنتی = 32;
// Assume an instruction is 4 bytes
ساكن constexpr صحيح جدول_بائٹاں_فی_INST = 4;

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
    File_سطر* ثابت m_fl;  // FileLine used during construction
    AstConsArray* m_consp = خالی_سوچک;  // The lookup table initializer values
    AstVarScope* m_varScopep = خالی_سوچک;  // The scoped variable holding the table

عام:
    واضح جدول_Builder(File_سطر* fl)
        : m_fl{fl} {}

    ~جدول_Builder() {
        إذا (m_consp) VL_DO_DANGLING(m_consp->delete_درخت(), m_consp);
    }

    فراغ سیٹ_جدول_ناپ(Ast_گرہ_DType* elemDType, بنانشانی ناپ) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(ناپ > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ثابت صحيح چوڑائی = elemDType->چوڑائی();
        Ast_گرہ_DType* ثابت subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->لبھو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED);
        AstUnpackArrayDType* ثابت جدول_DTypep = جديد AstUnpackArrayDType{
            m_fl, subDTypep, جديد AstRange{m_fl, static_cast<صحيح>(ناپ), 0}};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(جدول_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ثابت defaultp
            = elemDType->isString()   ? جديد AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? جديد AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : جديد AstConst{m_fl, AstConst::Widthed_قدر{}, چوڑائی, 0};
        m_consp = جديد AstConsArray{m_fl, جدول_DTypep, defaultp};
    }

    فراغ جوڑو_قدر(بنانشانی اشاریہ, ثابت V3_نمبر& قدر) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        إذا (قدر.isString() ? قدر.منزل_String().خالی_تھاواں() : قدر.isEqZero()) أرجع;
        m_consp->valuep(اشاریہ, جديد AstConst{m_fl, قدر});
    }

    AstVarScope* varScopep() {
        إذا (!m_varScopep) {
            AstVarRef* ثابت refp = V3ConstPool::لبھو(m_consp);
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
    ثابت بنانشانی m_ord;  // Output ordinal number in this block
    منطقي m_mayBeUnassigned = خطأ;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    جدول_Builder m_جدول_Builder;

عام:
    جدول_Output_Var(AstVarScope* varScopep, بنانشانی ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_جدول_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    string name() ثابت { أرجع varScopep()->varp()->name(); }
    بنانشانی ord() ثابت { أرجع m_ord; }
    فراغ سیٹ_May_Be_Unassigned() { m_mayBeUnassigned = سچ; }
    منطقي mayBeUnassigned() ثابت { أرجع m_mayBeUnassigned; }
    فراغ سیٹ_جدول_ناپ(بنانشانی ناپ) { m_جدول_Builder.سیٹ_جدول_ناپ(varScopep()->dtypep(), ناپ); }
    فراغ جوڑو_قدر(بنانشانی اشاریہ, ثابت V3_نمبر& قدر) { m_جدول_Builder.جوڑو_قدر(اشاریہ, قدر); }
    AstVarScope* tabeVarScopep() { أرجع m_جدول_Builder.varScopep(); }
};

//######################################################################
// Table class functions

صنف جدول_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    مزدوج m_کل_بائٹاں = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_اشاریہ_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = خالی_سوچک;  // Current SCOPE

    //  State cleared on each always/assignw
    منطقي m_سونپو_Dly = خطأ;  // Consists of delayed assignments instead of normal assignments
    بنانشانی m_وچ_چوڑائی_بٹ = 0;  // Input table width - in bits
    بنانشانی m_باہرلا_چوڑائی_بائٹاں = 0;  // Output table width - in bytes
    std::سمتیہ<AstVarScope*> m_وچ_Varps;  // Input variable list
    std::سمتیہ<جدول_Output_Var> m_باہرلا_Varps;  // Output variable list

    // METHODS

عام:
    فراغ simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_باہرلا_چوڑائی_بائٹاں += nodep->varp()->dtypeSkipRefp()->چوڑائی_کل_بائٹاں();
            m_باہرلا_Varps.emplace_پٹھ(vscp, static_cast<بنانشانی>(m_باہرلا_Varps.ناپ()));
        }
        إذا (nodep->access().isReadOrRW()) {
            m_وچ_چوڑائی_بٹ += nodep->varp()->چوڑائی();
            m_وچ_Varps.ڈالو_پٹھ(vscp);
        }
    }

خاص:
    منطقي درخت_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_وچ_چوڑائی_بٹ = 0;
        m_باہرلا_چوڑائی_بائٹاں = 0;
        m_وچ_Varps.clear();
        m_باہرلا_Varps.clear();

        // Collect stats
        جدول_Simulate_Visitor chkvis{هذا};
        chkvis.main_جدول_Check(nodep);
        m_سونپو_Dly = chkvis.is_سونپو_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ثابت ناپ_t chg_چوڑائی = m_باہرلا_Varps.ناپ();
        ثابت مزدوج space = std::قوت<مزدوج>(2.0, m_وچ_چوڑائی_بٹ) * (m_باہرلا_چوڑائی_بائٹاں + chg_چوڑائی);
        // Instruction count bytes (ok, it's space also not time :)
        ثابت مزدوج ویلا_2  // max(_, 1), so we won't divide by zero
            = std::max<مزدوج>(chkvis.instr_گنتی() * جدول_بائٹاں_فی_INST + chkvis.انکڑے_گنتی(), 1);
        إذا (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        إذا (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        إذا (!m_باہرلا_چوڑائی_بائٹاں || !m_وچ_چوڑائی_بٹ) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        إذا (chkvis.instr_گنتی() < جدول_MIN_گرہ_گنتی) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        إذا (space > جدول_MAX_بائٹاں) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        إذا (space > ویلا_2 * جدول_SPACE_ویلا_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        إذا (m_کل_بائٹاں > جدول_کل_بائٹاں) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_گنتی() << " Data=" << chkvis.انکڑے_گنتی()
                                << " in width (bits)=" << m_وچ_چوڑائی_بٹ << " out width (bytes)="
                                << m_باہرلا_چوڑائی_بائٹاں << " Spacetime=" << (space / ویلا_2) << "("
                                << space << "/" << ویلا_2 << ")" << ": " << nodep);
        إذا (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / ویلا_2) << " " << nodep);
            m_کل_بائٹاں += space;
        }
        أرجع chkvis.optimizable();
    }

    فراغ replace_With_جدول(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_سطر* ثابت fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ثابت اشاریہ_Vscp = m_اشاریہ_Tmps.make(fl, m_scopep, m_وچ_چوڑائی_بٹ);

        // The 'output assigned' table builder
        جدول_Builder output_Assigned_جدول_Builder{fl};
        output_Assigned_جدول_Builder.سیٹ_جدول_ناپ(
            nodep->لبھو_ثنائی_ہندسہ_DType(m_باہرلا_Varps.ناپ(), m_باہرلا_Varps.ناپ(), VSigning::UNSIGNED),
            VL_نقاب_I(m_وچ_چوڑائی_بٹ));

        // Set sizes of output tables
        لكل (جدول_Output_Var& tov : m_باہرلا_Varps) tov.سیٹ_جدول_ناپ(VL_نقاب_I(m_وچ_چوڑائی_بٹ));

        // Populate the tables
        createTables(nodep, output_Assigned_جدول_Builder);

        Ast_گرہ* ثابت stmtsp = create_Lookup_داخلہ(fl, اشاریہ_Vscp);
        createOutputAssigns(nodep, stmtsp, اشاریہ_Vscp, output_Assigned_جدول_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا()->delete_درخت();
        nodep->جوڑو_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    فراغ createTables(AstAlways* nodep, جدول_Builder& output_Assigned_جدول_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        جدول_Simulate_Visitor simvis{هذا};
        لكل (uint32_t i = 0; i <= VL_نقاب_I(m_وچ_چوڑائی_بٹ); ++i) {
            ثابت uint32_t وچ_قدر = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << وچ_قدر);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t کھسکاؤ_2 = 0;
            لكل (AstVarScope* invscp : m_وچ_Varps) {
                // LSB is first variable, so extract it that way
                ثابت AstConst cnst{invscp->fileline(), AstConst::Widthed_قدر{}, invscp->چوڑائی(),
                                    VL_نقاب_I(invscp->چوڑائی()) & (وچ_قدر >> کھسکاؤ_2)};
                simvis.new_قدر(invscp, &cnst);
                کھسکاؤ_2 += invscp->چوڑائی();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(کھسکاؤ_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_جدول_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_سنیہا());

            // Build output value tables and the assigned flags table
            V3_نمبر output_Assigned_نقاب{nodep, static_cast<صحيح>(m_باہرلا_Varps.ناپ()), 0};
            لكل (جدول_Output_Var& tov : m_باہرلا_Varps) {
                إذا (V3_نمبر* ثابت outnump = simvis.fetch_باہرلا_نمبر_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_نقاب.سیٹ_ثنائی_ہندسہ(tov.ord(), 1);  // Mark output as assigned
                    tov.جوڑو_قدر(وچ_قدر, *outnump);
                } وإلا {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.سیٹ_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_جدول_Builder.جوڑو_قدر(وچ_قدر, output_Assigned_نقاب);
        }  // each value
    }

    Ast_گرہ* create_Lookup_داخلہ(File_سطر* fl, AstVarScope* اشاریہ_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_گرہ_اظہاریہ* concatp = خالی_سوچک;
        لكل (AstVarScope* invscp : m_وچ_Varps) {
            AstVarRef* ثابت refp = جديد AstVarRef{fl, invscp, VAccess::READ};
            إذا (concatp) {
                concatp = جديد AstConcat{fl, refp, concatp};
            } وإلا {
                concatp = refp;
            }
        }

        أرجع جديد Ast_سونپو{fl, جديد AstVarRef{fl, اشاریہ_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_سطر* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ثابت fromRefp = جديد AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ثابت اشاریہ_Refp = جديد AstVarRef{fl, indexp, VAccess::READ};
        أرجع جديد AstArraySel{fl, fromRefp, اشاریہ_Refp};
    }

    فراغ createOutputAssigns(Ast_گرہ* nodep, Ast_گرہ* stmtsp, AstVarScope* اشاریہ_Vscp,
                             AstVarScope* output_Assigned_جدول_Vscp) {
        File_سطر* ثابت fl = nodep->fileline();
        لكل (جدول_Output_Var& tov : m_باہرلا_Varps) {
            Ast_گرہ_اظہاریہ* ثابت alhsp = جديد AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_گرہ_اظہاریہ* ثابت arhsp = select(fl, tov.tabeVarScopep(), اشاریہ_Vscp);
            Ast_گرہ* outsetp = m_سونپو_Dly
                                   ? static_cast<Ast_گرہ*>(جديد Ast_سونپو_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_گرہ*>(جديد Ast_سونپو{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            إذا (tov.mayBeUnassigned()) {
                V3_نمبر output_Chg_نقاب{nodep, static_cast<صحيح>(m_باہرلا_Varps.ناپ()), 0};
                output_Chg_نقاب.سیٹ_ثنائی_ہندسہ(tov.ord(), 1);
                Ast_گرہ_اظہاریہ* ثابت condp
                    = جديد AstNeq{fl,
                                 جديد AstAnd{fl, select(fl, output_Assigned_جدول_Vscp, اشاریہ_Vscp),
                                            جديد AstConst{fl, output_Chg_نقاب}},
                                 جديد AstConst{fl, AstConst::Widthed_قدر{},
                                              static_cast<صحيح>(m_باہرلا_Varps.ناپ()), 0}};
                outsetp = جديد AstIf{fl, condp, outsetp};
            }

            stmtsp->جوڑو_اگلا(outsetp);
        }
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }
    فراغ ویکھو(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        إذا (درخت_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_جدول(nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

عام:
    // CONSTRUCTORS
    واضح جدول_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~جدول_Visitor() override {  //
        V3Stats::جوڑو_Stat("Optimizations, Tables created", m_statTablesCre);
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
