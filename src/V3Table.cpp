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
स्थिर constexpr पूर्णाङ्क सारणी_MAX_अष्टकानि = 1 * 1024 * 1024;
// 64MB is close to max memory of some systems (256MB or so), so don't get out of control
स्थिर constexpr पूर्णाङ्क सारणी_योग_अष्टकानि = 64 * 1024 * 1024;
// Worth no more than 8 bytes of data to replace an instruction
स्थिर constexpr पूर्णाङ्क सारणी_SPACE_TIME_MULT = 8;
// If < 32 instructions, not worth the effort
स्थिर constexpr पूर्णाङ्क सारणी_MIN_ग्रन्थिः_गणनम् = 32;
// Assume an instruction is 4 bytes
स्थिर constexpr पूर्णाङ्क सारणी_अष्टकानि_प्रत्येकम्_INST = 4;

//######################################################################

वर्ग सारणी_Visitor;

वर्ग सारणी_Simulate_Visitor final : सार्वजनिक SimulateVisitor {
    // MEMBERS
    सारणी_Visitor* ध्रुव m_cbthis;  ///< Class for callback

सार्वजनिक:
    ///< Call other-this function on all new var references
    शून्य varRefCb(AstVarRef* nodep) override;

    // CONSTRUCTORS
    स्पष्ट सारणी_Simulate_Visitor(सारणी_Visitor* cbthis)
        : m_cbthis{cbthis} {}
    ~सारणी_Simulate_Visitor() override = स्वभाव;
};

//######################################################################
// Class for holding lookup table state during construction

वर्ग सारणी_Builder final {
    File_पङ्क्तिः* ध्रुव m_fl;  // FileLine used during construction
    AstConsArray* m_consp = शून्यसूचकः;  // The lookup table initializer values
    AstVarScope* m_varScopep = शून्यसूचकः;  // The scoped variable holding the table

सार्वजनिक:
    स्पष्ट सारणी_Builder(File_पङ्क्तिः* fl)
        : m_fl{fl} {}

    ~सारणी_Builder() {
        यदि (m_consp) VL_DO_DANGLING(m_consp->delete_वृक्षः(), m_consp);
    }

    शून्य समुच्चयः_सारणी_आकार(Ast_ग्रन्थिः_DType* elemDType, अचिह्नित आकार) {
        UASSERT_OBJ(!m_consp, m_fl, "Table size already set");
        UASSERT_OBJ(आकार > 0, m_fl, "Size zero");
        // TODO: Assert elemDType is a packed type
        // Create data type
        ध्रुव पूर्णाङ्क विस्तारः = elemDType->विस्तारः();
        Ast_ग्रन्थिः_DType* ध्रुव subDTypep
            = elemDType->isString() || elemDType->isDouble()
                  ? elemDType
                  : v3Global.rootp()->findBitDType(विस्तारः, विस्तारः, VSigning::UNSIGNED);
        AstUnpackArrayDType* ध्रुव सारणी_DTypep = नूतन AstUnpackArrayDType{
            m_fl, subDTypep, नूतन AstRange{m_fl, static_cast<पूर्णाङ्क>(आकार), 0}};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(सारणी_DTypep);
        // Create table initializer (with default value 0)
        AstConst* ध्रुव defaultp
            = elemDType->isString()   ? नूतन AstConst{m_fl, AstConst::String{}, ""}
              : elemDType->isDouble() ? नूतन AstConst{m_fl, AstConst::RealDouble{}, 0.0}
                                      : नूतन AstConst{m_fl, AstConst::Widthed_मानम्{}, विस्तारः, 0};
        m_consp = नूतन AstConsArray{m_fl, सारणी_DTypep, defaultp};
    }

    शून्य सङ्कलय_मानम्(अचिह्नित सूचकांक, ध्रुव V3_सङ्ख्या& मानम्) {
        UASSERT_OBJ(!m_varScopep, m_fl, "Table variable already created");
        // Default value is zero/empty string so don't add it
        यदि (मानम्.isString() ? मानम्.गन्तव्यम्_String().रिक्तानि() : मानम्.isEqZero()) प्रत्यागम;
        m_consp->valuep(सूचकांक, नूतन AstConst{m_fl, मानम्});
    }

    AstVarScope* varScopep() {
        यदि (!m_varScopep) {
            AstVarRef* ध्रुव refp = V3ConstPool::find(m_consp);
            m_varScopep = refp->varScopep();
            VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
        }
        प्रत्यागम m_varScopep;
    }
};

//######################################################################
// Class for holding output variable state during table conversion of logic

वर्ग सारणी_Output_Var final {
    AstVarScope* ध्रुव m_varScopep;  // The output variable
    ध्रुव अचिह्नित m_ord;  // Output ordinal number in this block
    तार्किक m_mayBeUnassigned = असत्य;  // If true, then this variable may be unassigned through
                                     // some path through the block being table converted
    सारणी_Builder m_सारणी_Builder;

सार्वजनिक:
    सारणी_Output_Var(AstVarScope* varScopep, अचिह्नित ord)
        : m_varScopep{varScopep}
        , m_ord{ord}
        , m_सारणी_Builder{varScopep->fileline()} {}

    AstVarScope* varScopep() ध्रुव { प्रत्यागम m_varScopep; }
    string name() ध्रुव { प्रत्यागम varScopep()->varp()->name(); }
    अचिह्नित ord() ध्रुव { प्रत्यागम m_ord; }
    शून्य समुच्चयः_May_Be_Unassigned() { m_mayBeUnassigned = सत्य; }
    तार्किक mayBeUnassigned() ध्रुव { प्रत्यागम m_mayBeUnassigned; }
    शून्य समुच्चयः_सारणी_आकार(अचिह्नित आकार) { m_सारणी_Builder.समुच्चयः_सारणी_आकार(varScopep()->dtypep(), आकार); }
    शून्य सङ्कलय_मानम्(अचिह्नित सूचकांक, ध्रुव V3_सङ्ख्या& मानम्) { m_सारणी_Builder.सङ्कलय_मानम्(सूचकांक, मानम्); }
    AstVarScope* tabeVarScopep() { प्रत्यागम m_सारणी_Builder.varScopep(); }
};

//######################################################################
// Table class functions

वर्ग सारणी_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on each always/assignw

    // STATE
    द्विगुण m_योग_अष्टकानि = 0;  // Total bytes in tables created
    VDouble0 m_statTablesCre;  // Statistic tracking
    V3SharedTmps m_सूचकांक_Tmps{"__Vtableidx", VVarType::BLOCKTEMP};  // Table index variables

    //  State cleared on each scope
    AstScope* m_scopep = शून्यसूचकः;  // Current SCOPE

    //  State cleared on each always/assignw
    तार्किक m_नियोजय_Dly = असत्य;  // Consists of delayed assignments instead of normal assignments
    अचिह्नित m_अन्तर्_विस्तारः_द्वयङ्काः = 0;  // Input table width - in bits
    अचिह्नित m_बहिर्गमनम्_विस्तारः_अष्टकानि = 0;  // Output table width - in bytes
    std::सदिशम्<AstVarScope*> m_अन्तर्_Varps;  // Input variable list
    std::सदिशम्<सारणी_Output_Var> m_बहिर्गमनम्_Varps;  // Output variable list

    // METHODS

सार्वजनिक:
    शून्य simulateVarRefCb(AstVarRef* nodep) {
        // Called by TableSimulateVisitor on each unique varref encountered
        UINFO(9, "   SimVARREF " << nodep);
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        यदि (nodep->access().isWriteOrRW()) {
            // We'll make the table with a separate natural alignment for each output var, so
            // always have 8, 16 or 32 bit widths, so use widthTotalBytes
            m_बहिर्गमनम्_विस्तारः_अष्टकानि += nodep->varp()->dtypeSkipRefp()->विस्तारः_योग_अष्टकानि();
            m_बहिर्गमनम्_Varps.emplace_पृष्ठभागः(vscp, static_cast<अचिह्नित>(m_बहिर्गमनम्_Varps.आकार()));
        }
        यदि (nodep->access().isReadOrRW()) {
            m_अन्तर्_विस्तारः_द्वयङ्काः += nodep->varp()->विस्तारः();
            m_अन्तर्_Varps.प्रक्षिप_पृष्ठभागः(vscp);
        }
    }

गुप्त:
    तार्किक वृक्षः_Test(AstAlways* nodep) {
        // Process alw/assign tree
        m_अन्तर्_विस्तारः_द्वयङ्काः = 0;
        m_बहिर्गमनम्_विस्तारः_अष्टकानि = 0;
        m_अन्तर्_Varps.clear();
        m_बहिर्गमनम्_Varps.clear();

        // Collect stats
        सारणी_Simulate_Visitor chkvis{एतत्};
        chkvis.main_सारणी_Check(nodep);
        m_नियोजय_Dly = chkvis.is_नियोजय_Dly();
        // Also sets m_inWidthBits
        // Also sets m_outWidthBytes
        // Also sets m_inVarps
        // Also sets m_outVarps

        // Calc data storage in bytes
        ध्रुव आकार_t chg_विस्तारः = m_बहिर्गमनम्_Varps.आकार();
        ध्रुव द्विगुण space = std::घातः<द्विगुण>(2.0, m_अन्तर्_विस्तारः_द्वयङ्काः) * (m_बहिर्गमनम्_विस्तारः_अष्टकानि + chg_विस्तारः);
        // Instruction count bytes (ok, it's space also not time :)
        ध्रुव द्विगुण time  // max(_, 1), so we won't divide by zero
            = std::max<द्विगुण>(chkvis.instr_गणनम्() * सारणी_अष्टकानि_प्रत्येकम्_INST + chkvis.दत्त_गणनम्(), 1);
        यदि (chkvis.isImpure()) chkvis.clearOptimizable(nodep, "Table creates side effects");
        यदि (chkvis.isCoverage()) {
            chkvis.clearOptimizable(nodep, "Table removes coverage points");
        }
        // cppcheck-suppress knownConditionTrueFalse // set by TableSimulateVisitor
        यदि (!m_बहिर्गमनम्_विस्तारः_अष्टकानि || !m_अन्तर्_विस्तारः_द्वयङ्काः) {
            chkvis.clearOptimizable(nodep, "Table has no outputs");
        }
        यदि (chkvis.instr_गणनम्() < सारणी_MIN_ग्रन्थिः_गणनम्) {
            chkvis.clearOptimizable(nodep, "Table has too few nodes involved");
        }
        // cppcheck-suppress knownConditionTrueFalse
        यदि (space > सारणी_MAX_अष्टकानि) {
            chkvis.clearOptimizable(nodep, "Table takes too much space");
        }
        यदि (space > time * सारणी_SPACE_TIME_MULT) {
            chkvis.clearOptimizable(nodep, "Table has bad tradeoff");
        }
        यदि (m_योग_अष्टकानि > सारणी_योग_अष्टकानि) {
            chkvis.clearOptimizable(nodep, "Table out of memory");
        }
        UINFO(4, "  Test: Opt=" << (chkvis.optimizable() ? "OK" : "NO") << ", Instrs="
                                << chkvis.instr_गणनम्() << " Data=" << chkvis.दत्त_गणनम्()
                                << " in width (bits)=" << m_अन्तर्_विस्तारः_द्वयङ्काः << " out width (bytes)="
                                << m_बहिर्गमनम्_विस्तारः_अष्टकानि << " Spacetime=" << (space / time) << "("
                                << space << "/" << time << ")" << ": " << nodep);
        यदि (chkvis.optimizable()) {
            UINFO(3, " Table Optimize spacetime=" << (space / time) << " " << nodep);
            m_योग_अष्टकानि += space;
        }
        प्रत्यागम chkvis.optimizable();
    }

    शून्य replace_With_सारणी(AstAlways* nodep) {
        // We've determined this table of nodes is optimizable, do it.
        ++m_statTablesCre;

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        // We will need a table index variable, create it here.
        AstVarScope* ध्रुव सूचकांक_Vscp = m_सूचकांक_Tmps.make(fl, m_scopep, m_अन्तर्_विस्तारः_द्वयङ्काः);

        // The 'output assigned' table builder
        सारणी_Builder output_Assigned_सारणी_Builder{fl};
        output_Assigned_सारणी_Builder.समुच्चयः_सारणी_आकार(
            nodep->findBitDType(m_बहिर्गमनम्_Varps.आकार(), m_बहिर्गमनम्_Varps.आकार(), VSigning::UNSIGNED),
            VL_आवरणम्_I(m_अन्तर्_विस्तारः_द्वयङ्काः));

        // Set sizes of output tables
        प्रति (सारणी_Output_Var& tov : m_बहिर्गमनम्_Varps) tov.समुच्चयः_सारणी_आकार(VL_आवरणम्_I(m_अन्तर्_विस्तारः_द्वयङ्काः));

        // Populate the tables
        createTables(nodep, output_Assigned_सारणी_Builder);

        Ast_ग्रन्थिः* ध्रुव stmtsp = create_Lookup_निवेशः(fl, सूचकांक_Vscp);
        createOutputAssigns(nodep, stmtsp, सूचकांक_Vscp, output_Assigned_सारणी_Builder.varScopep());

        // Link it in.
        // Replace the body, keeping the AstAlways itself under its AstActive
        nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः();
        nodep->सङ्कलय_Stmtsp(stmtsp);
        UINFOTREE(6, nodep, "", "table_new");
    }

    शून्य createTables(AstAlways* nodep, सारणी_Builder& output_Assigned_सारणी_Builder) {
        // Create table
        // There may be a simulation path by which the output doesn't change value.
        // We could bail on these cases, or we can have a "change it" boolean.
        // We've chosen the latter route, since recirc is common in large FSMs.
        सारणी_Simulate_Visitor simvis{एतत्};
        प्रति (uint32_t i = 0; i <= VL_आवरणम्_I(m_अन्तर्_विस्तारः_द्वयङ्काः); ++i) {
            ध्रुव uint32_t अन्तर्_मानम् = i;
            // Make a new simulation structure so we can set new input values
            UINFO(8, " Simulating " << std::hex << अन्तर्_मानम्);

            // Above simulateVisitor clears user 3, so
            // all outputs default to nullptr to mean 'recirculating'.
            simvis.clear();

            // Set all inputs to the constant
            uint32_t सरणम्_2 = 0;
            प्रति (AstVarScope* invscp : m_अन्तर्_Varps) {
                // LSB is first variable, so extract it that way
                ध्रुव AstConst cnst{invscp->fileline(), AstConst::Widthed_मानम्{}, invscp->विस्तारः(),
                                    VL_आवरणम्_I(invscp->विस्तारः()) & (अन्तर्_मानम् >> सरणम्_2)};
                simvis.new_मानम्(invscp, &cnst);
                सरणम्_2 += invscp->विस्तारः();
                // We are using 32 bit arithmetic, because there's no way the input table can be
                // 2^32 bytes!
                UASSERT_OBJ(सरणम्_2 <= 32, nodep, "shift overflow");
                UINFO(8, "   Input " << invscp->name() << " = " << cnst.name());
            }

            // Simulate
            simvis.main_सारणी_Emulate(nodep);
            UASSERT_OBJ(simvis.optimizable(), simvis.whyNotNodep(),
                        "Optimizable cleared, even though earlier test run said not: "
                            << simvis.why_Not_सन्देशः());

            // Build output value tables and the assigned flags table
            V3_सङ्ख्या output_Assigned_आवरणम्{nodep, static_cast<पूर्णाङ्क>(m_बहिर्गमनम्_Varps.आकार()), 0};
            प्रति (सारणी_Output_Var& tov : m_बहिर्गमनम्_Varps) {
                यदि (V3_सङ्ख्या* ध्रुव outnump = simvis.fetch_बहिर्गमनम्_सङ्ख्या_Null(tov.varScopep())) {
                    UINFO(8, "   Output " << tov.name() << " = " << *outnump);
                    UASSERT_OBJ(!outnump->isAnyXZ(), outnump, "Table should not contain X/Z");
                    output_Assigned_आवरणम्.समुच्चयः_Bit(tov.ord(), 1);  // Mark output as assigned
                    tov.सङ्कलय_मानम्(अन्तर्_मानम्, *outnump);
                } अन्यथा {
                    UINFO(8, "   Output " << tov.name() << " not set for this input");
                    tov.समुच्चयः_May_Be_Unassigned();
                }
            }

            // Set changed table
            output_Assigned_सारणी_Builder.सङ्कलय_मानम्(अन्तर्_मानम्, output_Assigned_आवरणम्);
        }  // each value
    }

    Ast_ग्रन्थिः* create_Lookup_निवेशः(File_पङ्क्तिः* fl, AstVarScope* सूचकांक_Vscp) {
        // Concat inputs into a single temp variable (inside always)
        // First var in inVars becomes the LSB of the concat
        Ast_ग्रन्थिः_व्यञ्जकम्* concatp = शून्यसूचकः;
        प्रति (AstVarScope* invscp : m_अन्तर्_Varps) {
            AstVarRef* ध्रुव refp = नूतन AstVarRef{fl, invscp, VAccess::READ};
            यदि (concatp) {
                concatp = नूतन AstConcat{fl, refp, concatp};
            } अन्यथा {
                concatp = refp;
            }
        }

        प्रत्यागम नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, सूचकांक_Vscp, VAccess::WRITE}, concatp};
    }

    AstArraySel* select(File_पङ्क्तिः* fl, AstVarScope* fromp, AstVarScope* indexp) {
        AstVarRef* ध्रुव fromRefp = नूतन AstVarRef{fl, fromp, VAccess::READ};
        AstVarRef* ध्रुव सूचकांक_Refp = नूतन AstVarRef{fl, indexp, VAccess::READ};
        प्रत्यागम नूतन AstArraySel{fl, fromRefp, सूचकांक_Refp};
    }

    शून्य createOutputAssigns(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* stmtsp, AstVarScope* सूचकांक_Vscp,
                             AstVarScope* output_Assigned_सारणी_Vscp) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        प्रति (सारणी_Output_Var& tov : m_बहिर्गमनम्_Varps) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव alhsp = नूतन AstVarRef{fl, tov.varScopep(), VAccess::WRITE};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arhsp = select(fl, tov.tabeVarScopep(), सूचकांक_Vscp);
            Ast_ग्रन्थिः* outsetp = m_नियोजय_Dly
                                   ? static_cast<Ast_ग्रन्थिः*>(नूतन Ast_नियोजय_Dly{fl, alhsp, arhsp})
                                   : static_cast<Ast_ग्रन्थिः*>(नूतन Ast_नियोजय{fl, alhsp, arhsp});

            // If this output is unassigned on some code paths, wrap the assignment in an If
            यदि (tov.mayBeUnassigned()) {
                V3_सङ्ख्या output_Chg_आवरणम्{nodep, static_cast<पूर्णाङ्क>(m_बहिर्गमनम्_Varps.आकार()), 0};
                output_Chg_आवरणम्.समुच्चयः_Bit(tov.ord(), 1);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
                    = नूतन AstNeq{fl,
                                 नूतन AstAnd{fl, select(fl, output_Assigned_सारणी_Vscp, सूचकांक_Vscp),
                                            नूतन AstConst{fl, output_Chg_आवरणम्}},
                                 नूतन AstConst{fl, AstConst::Widthed_मानम्{},
                                              static_cast<पूर्णाङ्क>(m_बहिर्गमनम्_Varps.आकार()), 0}};
                outsetp = नूतन AstIf{fl, condp, outsetp};
            }

            stmtsp->सङ्कलय_अग्र(outsetp);
        }
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstScope* nodep) override {
        UINFO(4, " SCOPE " << nodep);
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstAlways* nodep) override {
        UINFO(4, "  ALWAYS  " << nodep);
        यदि (वृक्षः_Test(nodep)) {
            // Well, then, I'll be a memory hog.
            replace_With_सारणी(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        // It's nearly impossible to have a large enough assign to make this worthwhile
        // For now we won't bother.
        // Accelerated: no iterate
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट सारणी_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~सारणी_Visitor() override {  //
        V3Stats::सङ्कलय_Stat("Optimizations, Tables created", m_statTablesCre);
    }
};

//######################################################################

शून्य सारणी_Simulate_Visitor::varRefCb(AstVarRef* nodep) {
    // Called by checking on each new varref encountered
    // We cross-call into a TableVisitor function.
    m_cbthis->simulateVarRefCb(nodep);
}

//######################################################################
// Table class functions

शून्य V3_सारणी::सारणी_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { सारणी_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("table", 0, dump_वृक्षः_Either_Level() >= 3);
}
