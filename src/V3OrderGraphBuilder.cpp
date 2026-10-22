// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Block code ordering
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
//
//  Initial graph dependency builder for ordering
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

ವರ್ಗ ಕ್ರಮ_User final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ಎಣಿಕೆ ವರ್ಗ Var_ಶೃಂಗ_Type : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

ಖಾಸಗಿ:
    // Vertex of each type (if non-nullptr)
    std::array<ಕ್ರಮ_Var_ಶೃಂಗ*, static_cast<ಗಾತ್ರ_t>(Var_ಶೃಂಗ_Type::POST) + 1> m_vertexps;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಕ್ರಮ_Var_ಶೃಂಗ* get_Var_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstVarScope* varscp, Var_ಶೃಂಗ_Type type) {
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ idx = static_cast<ಚಿಹ್ನೆರಹಿತ>(type);
        ಕ್ರಮ_Var_ಶೃಂಗ* vertexp = m_vertexps[idx];
        ಆದರೆ (!vertexp) {
            ಆಯ್ಕೆ (type) {
            ಸ್ಥಿತಿ Var_ಶೃಂಗ_Type::STD: vertexp = ಹೊಸ ಕ್ರಮ_Var_Std_ಶೃಂಗ{graphp, varscp}; ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ Var_ಶೃಂಗ_Type::PRE: vertexp = ಹೊಸ ಕ್ರಮ_Var_Pre_ಶೃಂಗ{graphp, varscp}; ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ Var_ಶೃಂಗ_Type::PORD: vertexp = ಹೊಸ ಕ್ರಮ_Var_Pord_ಶೃಂಗ{graphp, varscp}; ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ Var_ಶೃಂಗ_Type::POST: vertexp = ಹೊಸ ಕ್ರಮ_Var_Post_ಶೃಂಗ{graphp, varscp}; ನಿಲ್ಲಿಸು;
            }
            m_vertexps[idx] = vertexp;
        }
        ಹಿಂತಿರುಗಿ vertexp;
    }

    // CONSTRUCTORS
    ಕ್ರಮ_User() { m_vertexps.fill(ಶೂನ್ಯಸೂಚಕ); }
    ~ಕ್ರಮ_User() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

ವರ್ಗ ಕ್ರಮ_ನಕ್ಷೆ_Builder final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    ಎಣಿಕೆ VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    ಎಣಿಕೆ VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    ಬಳಸಿ Var_ಶೃಂಗ_Type = ಕ್ರಮ_User::Var_ಶೃಂಗ_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use user4_ಒಳ_Use;
    AstUser1Allocator<AstVarScope, ಕ್ರಮ_User> m_ಕ್ರಮ_User;

    // STATE
    ಕ್ರಮ_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_graphp;  // The ordering graph built by this visitor
    ಕ್ರಮ_Logic_ಶೃಂಗ* m_logicVxp = ಶೂನ್ಯಸೂಚಕ;  // Current logic block being analyzed
    std::ಸದಿಶ<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;
    // Domain of the triggers of the current AstActive being processed
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* m_ಸಕ್ರಿಯ_Domainp = ಶೂನ್ಯಸೂಚಕ;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* m_domainp = ಶೂನ್ಯಸೂಚಕ;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* m_hybridp = ಶೂನ್ಯಸೂಚಕ;

    ತಾರ್ಕಿಕ m_ಒಳ_Clocked = ಸುಳ್ಳು;  // Underneath clocked AstActive
    ತಾರ್ಕಿಕ m_ಒಳ_Pre = ಸುಳ್ಳು;  // Underneath AlwaysPre
    ತಾರ್ಕಿಕ m_ಒಳ_Post = ಸುಳ್ಳು;  // Underneath AstAlwaysPost
    std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_ಗಣ m_force_Read_ಅಂಚು_Ignores;
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = ಶೂನ್ಯಸೂಚಕ;

    // METHODS

    ಶೂನ್ಯ iterateLogic(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        Ast_ಗಂಟು::user4_Clear_ವೃಕ್ಷ();
        m_force_Read_ಅಂಚು_Ignores.clear();
        ಆದರೆ (!m_ಒಳ_Clocked)
            V3Sched::util::collect_Force_Read_ಅಂಚು_Ignores(nodep, m_force_Read_ಅಂಚು_Ignores);
        // Create LogicVertex for this logic node
        m_logicVxp = ಹೊಸ ಕ್ರಮ_Logic_ಶೃಂಗ{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        ಆದರೆ (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_accessedVscps) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ recorded = vscp->user4();
                ಸ್ಥಿರಾಂಕ VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->ಕೂಡಿಸು_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = ಶೂನ್ಯಸೂಚಕ;
        m_force_Read_ಅಂಚು_Ignores.clear();
    }

    ಕ್ರಮ_Var_ಶೃಂಗ* get_Var_ಶೃಂಗ(AstVarScope* varscp, Var_ಶೃಂಗ_Type type) {
        ಹಿಂತಿರುಗಿ m_ಕ್ರಮ_User(varscp).get_Var_ಶೃಂಗ(m_graphp, varscp, type);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        UASSERT_OBJ(!nodep->sen_ವೃಕ್ಷ_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_ಒಳ_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_ಒಳ_Clocked);

        ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = nodep->sentreep();

        m_ಒಳ_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        ಆದರೆ (senTreep->hasHybrid()) {
            m_hybridp = m_ಸಕ್ರಿಯ_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_ಗಂಟು::user3_Clear_ವೃಕ್ಷ();
            senTreep->foreach([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {  //
                refp->varScopep()->user3(ನಿಜ);
            });
            m_readTriggersCombLogic = [](ಸ್ಥಿರಾಂಕ AstVarScope* vscp) { ಹಿಂತಿರುಗಿ !vscp->user3(); };
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!senTreep->hasCombo()) m_domainp = m_ಸಕ್ರಿಯ_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ಸ್ಥಿರಾಂಕ AstVarScope*) { ಹಿಂತಿರುಗಿ ನಿಜ; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ಸ್ಥಿರಾಂಕ varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    ಶೂನ್ಯ ದಾಖಲೆ_Raw_Access(AstVarScope* varscp, ಸ್ಥಿರಾಂಕ VAccess& access, Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!m_parallel) ಹಿಂತಿರುಗಿ;
        uint8_t recorded = 0;
        ಆದರೆ (access.isWriteOrRW()) recorded |= VA_WRITE;
        ಆದರೆ (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        ಆದರೆ (!varscp->user4Or(recorded)) m_accessedVscps.ತಳ್ಳು_ಹಿಂಭಾಗ(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    ಶೂನ್ಯ accountVarAccess(AstVarScope* varscp, ಸ್ಥಿರಾಂಕ VAccess& access, Ast_ಗಂಟು* nodep) {
        // Variable reference in logic. Add data dependency.
        ದಾಖಲೆ_Raw_Access(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prevGen = varscp->user2() & VU_GEN;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        ತಾರ್ಕಿಕ con = ಸುಳ್ಳು;
        ಆದರೆ (!prevCon && access.isReadOrRW()) {
            con = ನಿಜ;
            ಆದರೆ (prevGen && !m_ಒಳ_Clocked) {
                // Dangerous assumption:
                // If a variable is consumed in the same combinational process that produced it
                // earlier, consider it something like:
                //      foo = 1
                //      foo = foo + 1
                // and still optimize. Note this will break though:
                //      if (sometimes) foo = 1
                //      foo = foo + 1
                // TODO: Do this properly with liveness analysis (i.e.: if live, it's consumed)
                //       Note however that this construct is not nicely synthesizable (yields
                //       latch?).
                con = ಸುಳ್ಳು;
            }
            ಆದರೆ (!m_ಒಳ_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                ಆದರೆ (m_force_Read_ಅಂಚು_Ignores.ಎಣಿಕೆ_2(varscp) || m_cgRefBoundps) con = ಸುಳ್ಳು;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        ಆದರೆ (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            ಆದರೆ (m_ಒಳ_Post) {
                ಆದರೆ (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::STD);
                    m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, varVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
                }
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ postVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(postVxp, m_logicVxp, ತೂಕ_POST);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_ಒಳ_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::STD);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, varVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ postVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::POST);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(postVxp, m_logicVxp, ತೂಕ_POST);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ordVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::PORD);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, ordVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::STD);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, varVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ordVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::PORD);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(ordVxp, m_logicVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::STD);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, varVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
            }
        }

        // Variable is consumed
        ಆದರೆ (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            ಆದರೆ (m_ಒಳ_Post) {
                // Combinational logic
                ಆದರೆ (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(varVxp, m_logicVxp, ತೂಕ_MEDIUM);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_ಒಳ_Clocked) {  // Combinational logic
                ಆದರೆ (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(varVxp, m_logicVxp, ತೂಕ_MEDIUM);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ preVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::PRE);
                m_graphp->ಕೂಡಿಸು_Soft_ಅಂಚು(preVxp, m_logicVxp, ತೂಕ_PRE);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ preVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::PRE);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, preVxp, ತೂಕ_ಸಾಮಾನ್ಯ);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ postVxp = get_Var_ಶೃಂಗ(varscp, Var_ಶೃಂಗ_Type::POST);
                m_graphp->ಕೂಡಿಸು_Hard_ಅಂಚು(m_logicVxp, postVxp, ತೂಕ_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();
        ಆದರೆ (!funcp->isCovergroupSample()) ಹಿಂತಿರುಗಿ;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ಸ್ಥಿರಾಂಕ AstVarScope* instp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_ಒಳ_Pre, nodep, "Should not nest");
        VL_RESTORER(m_ಒಳ_Pre);
        m_ಒಳ_Pre = ನಿಜ;
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_ಒಳ_Post, nodep, "Should not nest");
        VL_RESTORER(m_ಒಳ_Post);
        m_ಒಳ_Post = ನಿಜ;
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Type_ಕೋಷ್ಟಕ* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ಕ್ರಮ_ನಕ್ಷೆ_Builder(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& coll,
                      ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& cgRefBindings, ತಾರ್ಕಿಕ parallel)
        : m_graphp{ಹೊಸ ಕ್ರಮ_ನಕ್ಷೆ{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3Sched::LogicByScope* ಸ್ಥಿರಾಂಕ lbsp : coll) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : *lbsp) {
                m_scopep = ನಮೂದು.m_scopep;
                m_ಸಕ್ರಿಯ_Domainp = m_graphp->domainp(ನಮೂದು.m_triggers);
                iterate(ನಮೂದು.m_activep);
                m_scopep = ಶೂನ್ಯಸೂಚಕ;
                m_ಸಕ್ರಿಯ_Domainp = ಶೂನ್ಯಸೂಚಕ;
            }
        }
    }
    ~ಕ್ರಮ_ನಕ್ಷೆ_Builder() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ಸ್ಥಿರ std::unique_ptr<ಕ್ರಮ_ನಕ್ಷೆ> apply(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& coll,
                                             ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& cgRefBindings,
                                             ತಾರ್ಕಿಕ parallel) {
        ಹಿಂತಿರುಗಿ std::unique_ptr<ಕ್ರಮ_ನಕ್ಷೆ>{
            ಕ್ರಮ_ನಕ್ಷೆ_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<ಕ್ರಮ_ನಕ್ಷೆ>
V3_ಕ್ರಮ::build_ಕ್ರಮ_ನಕ್ಷೆ(ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& coll,  //
                         ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         ತಾರ್ಕಿಕ parallel) {
    ಹಿಂತಿರುಗಿ ಕ್ರಮ_ನಕ್ಷೆ_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
