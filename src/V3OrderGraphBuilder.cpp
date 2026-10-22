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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderInternal.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

ਵਰਗ ਕ੍ਰਮ_User final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
ਜਨਤਕ:
    // TYPES
    ਗਿਣਤੀ ਵਰਗ Var_ਸ਼ੀਰਸ਼_Type : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

ਨਿਜੀ:
    // Vertex of each type (if non-nullptr)
    std::array<ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼*, static_cast<ਮਾਪ_t>(Var_ਸ਼ੀਰਸ਼_Type::POST) + 1> m_vertexps;

ਜਨਤਕ:
    // METHODS
    ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* get_Var_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstVarScope* varscp, Var_ਸ਼ੀਰਸ਼_Type type) {
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ idx = static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(type);
        ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* vertexp = m_vertexps[idx];
        ਜੇ (!vertexp) {
            ਚੋਣ (type) {
            ਹਾਲਤ Var_ਸ਼ੀਰਸ਼_Type::STD: vertexp = ਨਵਾਂ ਕ੍ਰਮ_Var_Std_ਸ਼ੀਰਸ਼{graphp, varscp}; ਰੋਕੋ;
            ਹਾਲਤ Var_ਸ਼ੀਰਸ਼_Type::PRE: vertexp = ਨਵਾਂ ਕ੍ਰਮ_Var_Pre_ਸ਼ੀਰਸ਼{graphp, varscp}; ਰੋਕੋ;
            ਹਾਲਤ Var_ਸ਼ੀਰਸ਼_Type::PORD: vertexp = ਨਵਾਂ ਕ੍ਰਮ_Var_Pord_ਸ਼ੀਰਸ਼{graphp, varscp}; ਰੋਕੋ;
            ਹਾਲਤ Var_ਸ਼ੀਰਸ਼_Type::POST: vertexp = ਨਵਾਂ ਕ੍ਰਮ_Var_Post_ਸ਼ੀਰਸ਼{graphp, varscp}; ਰੋਕੋ;
            }
            m_vertexps[idx] = vertexp;
        }
        ਵਾਪਸ vertexp;
    }

    // CONSTRUCTORS
    ਕ੍ਰਮ_User() { m_vertexps.fill(ਖਾਲੀ_ਸੂਚਕ); }
    ~ਕ੍ਰਮ_User() = ਮੂਲ;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

ਵਰਗ ਕ੍ਰਮ_ਗ੍ਰਾਫ_Builder final : ਜਨਤਕ VNVisitor {
    // TYPES
    ਗਿਣਤੀ VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    ਗਿਣਤੀ VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    ਵਰਤੋਂ Var_ਸ਼ੀਰਸ਼_Type = ਕ੍ਰਮ_User::Var_ਸ਼ੀਰਸ਼_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use user4_ਵਿੱਚ_Use;
    AstUser1Allocator<AstVarScope, ਕ੍ਰਮ_User> m_ਕ੍ਰਮ_User;

    // STATE
    ਕ੍ਰਮ_ਗ੍ਰਾਫ* ਅਚਲ m_graphp;  // The ordering graph built by this visitor
    ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* m_logicVxp = ਖਾਲੀ_ਸੂਚਕ;  // Current logic block being analyzed
    std::ਸਦਿਸ਼<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;
    // Domain of the triggers of the current AstActive being processed
    ਅਚਲ ਕ੍ਰਮ_Domain* m_ਸਰਗਰਮ_Domainp = ਖਾਲੀ_ਸੂਚਕ;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ਅਚਲ ਕ੍ਰਮ_Domain* m_domainp = ਖਾਲੀ_ਸੂਚਕ;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ਅਚਲ ਕ੍ਰਮ_Domain* m_hybridp = ਖਾਲੀ_ਸੂਚਕ;

    ਤਾਰਕਿਕ m_ਵਿੱਚ_Clocked = ਝੂਠ;  // Underneath clocked AstActive
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Pre = ਝੂਠ;  // Underneath AlwaysPre
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Post = ਝੂਠ;  // Underneath AstAlwaysPost
    std::function<ਤਾਰਕਿਕ(ਅਚਲ AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_ਸੈੱਟ m_force_Read_ਕਿਨਾਰਾ_Ignores;
    ਅਚਲ ਤਾਰਕਿਕ m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ਅਚਲ V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ਅਚਲ V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = ਖਾਲੀ_ਸੂਚਕ;

    // METHODS

    ਖਾਲੀ iterateLogic(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();
        Ast_ਗੰਢ::user4_Clear_ਰੁੱਖ();
        m_force_Read_ਕਿਨਾਰਾ_Ignores.clear();
        ਜੇ (!m_ਵਿੱਚ_Clocked)
            V3Sched::util::collect_Force_Read_ਕਿਨਾਰਾ_Ignores(nodep, m_force_Read_ਕਿਨਾਰਾ_Ignores);
        // Create LogicVertex for this logic node
        m_logicVxp = ਨਵਾਂ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        ਜੇ (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            ਲਈ (AstVarScope* ਅਚਲ vscp : m_accessedVscps) {
                ਅਚਲ ਪੂਰਨ recorded = vscp->user4();
                ਅਚਲ VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->ਜੋੜੋ_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = ਖਾਲੀ_ਸੂਚਕ;
        m_force_Read_ਕਿਨਾਰਾ_Ignores.clear();
    }

    ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* get_Var_ਸ਼ੀਰਸ਼(AstVarScope* varscp, Var_ਸ਼ੀਰਸ਼_Type type) {
        ਵਾਪਸ m_ਕ੍ਰਮ_User(varscp).get_Var_ਸ਼ੀਰਸ਼(m_graphp, varscp, type);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        UASSERT_OBJ(!nodep->sen_ਰੁੱਖ_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_ਵਿੱਚ_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_ਵਿੱਚ_Clocked);

        ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = nodep->sentreep();

        m_ਵਿੱਚ_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        ਜੇ (senTreep->hasHybrid()) {
            m_hybridp = m_ਸਰਗਰਮ_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_ਗੰਢ::user3_Clear_ਰੁੱਖ();
            senTreep->foreach([](ਅਚਲ AstVarRef* refp) {  //
                refp->varScopep()->user3(ਸੱਚ);
            });
            m_readTriggersCombLogic = [](ਅਚਲ AstVarScope* vscp) { ਵਾਪਸ !vscp->user3(); };
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!senTreep->hasCombo()) m_domainp = m_ਸਰਗਰਮ_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ਅਚਲ AstVarScope*) { ਵਾਪਸ ਸੱਚ; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ਅਚਲ varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ਅਚਲ AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            ਲਈ (AstVarScope* ਅਚਲ boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } ਨਹੀਂਤਾਂ {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    ਖਾਲੀ ਰਿਕਾਰਡ_Raw_Access(AstVarScope* varscp, ਅਚਲ VAccess& access, Ast_ਗੰਢ* nodep) {
        ਜੇ (!m_parallel) ਵਾਪਸ;
        uint8_t recorded = 0;
        ਜੇ (access.isWriteOrRW()) recorded |= VA_WRITE;
        ਜੇ (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        ਜੇ (!varscp->user4Or(recorded)) m_accessedVscps.ਧੱਕੋ_ਪਿੱਠ(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    ਖਾਲੀ accountVarAccess(AstVarScope* varscp, ਅਚਲ VAccess& access, Ast_ਗੰਢ* nodep) {
        // Variable reference in logic. Add data dependency.
        ਰਿਕਾਰਡ_Raw_Access(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ਅਚਲ ਤਾਰਕਿਕ prevGen = varscp->user2() & VU_GEN;
        ਅਚਲ ਤਾਰਕਿਕ prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ਅਚਲ ਤਾਰਕਿਕ gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        ਤਾਰਕਿਕ con = ਝੂਠ;
        ਜੇ (!prevCon && access.isReadOrRW()) {
            con = ਸੱਚ;
            ਜੇ (prevGen && !m_ਵਿੱਚ_Clocked) {
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
                con = ਝੂਠ;
            }
            ਜੇ (!m_ਵਿੱਚ_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                ਜੇ (m_force_Read_ਕਿਨਾਰਾ_Ignores.ਗਿਣਤੀ_2(varscp) || m_cgRefBoundps) con = ਝੂਠ;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        ਜੇ (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            ਜੇ (m_ਵਿੱਚ_Post) {
                ਜੇ (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::STD);
                    m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, varVxp, ਭਾਰ_ਸਾਧਾਰਨ);
                }
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ postVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(postVxp, m_logicVxp, ਭਾਰ_POST);
            } ਨਹੀਂਤਾਂ ਜੇ (!m_ਵਿੱਚ_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::STD);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, varVxp, ਭਾਰ_ਸਾਧਾਰਨ);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ postVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::POST);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(postVxp, m_logicVxp, ਭਾਰ_POST);
            } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ ordVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::PORD);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, ordVxp, ਭਾਰ_ਸਾਧਾਰਨ);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::STD);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, varVxp, ਭਾਰ_ਸਾਧਾਰਨ);
            } ਨਹੀਂਤਾਂ {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ ordVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::PORD);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(ordVxp, m_logicVxp, ਭਾਰ_ਸਾਧਾਰਨ);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::STD);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, varVxp, ਭਾਰ_ਸਾਧਾਰਨ);
            }
        }

        // Variable is consumed
        ਜੇ (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            ਜੇ (m_ਵਿੱਚ_Post) {
                // Combinational logic
                ਜੇ (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(varVxp, m_logicVxp, ਭਾਰ_MEDIUM);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (!m_ਵਿੱਚ_Clocked) {  // Combinational logic
                ਜੇ (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(varVxp, m_logicVxp, ਭਾਰ_MEDIUM);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ preVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::PRE);
                m_graphp->ਜੋੜੋ_Soft_ਕਿਨਾਰਾ(preVxp, m_logicVxp, ਭਾਰ_PRE);
            } ਨਹੀਂਤਾਂ {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ preVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::PRE);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, preVxp, ਭਾਰ_ਸਾਧਾਰਨ);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ postVxp = get_Var_ਸ਼ੀਰਸ਼(varscp, Var_ਸ਼ੀਰਸ਼_Type::POST);
                m_graphp->ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(m_logicVxp, postVxp, ਭਾਰ_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    ਖਾਲੀ ਦੇਖੋ(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ਅਚਲ funcp = nodep->funcp();
        ਜੇ (!funcp->isCovergroupSample()) ਵਾਪਸ;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ਅਚਲ AstVarScope* instp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_ਵਿੱਚ_Pre, nodep, "Should not nest");
        VL_RESTORER(m_ਵਿੱਚ_Pre);
        m_ਵਿੱਚ_Pre = ਸੱਚ;
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_ਵਿੱਚ_Post, nodep, "Should not nest");
        VL_RESTORER(m_ਵਿੱਚ_Post);
        m_ਵਿੱਚ_Post = ਸੱਚ;
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Type_ਸਾਰਣੀ* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ਕ੍ਰਮ_ਗ੍ਰਾਫ_Builder(ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& coll,
                      ਅਚਲ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ਅਚਲ V3Sched::CovergroupRefBindings& cgRefBindings, ਤਾਰਕਿਕ parallel)
        : m_graphp{ਨਵਾਂ ਕ੍ਰਮ_ਗ੍ਰਾਫ{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        ਲਈ (ਅਚਲ V3Sched::LogicByScope* ਅਚਲ lbsp : coll) {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : *lbsp) {
                m_scopep = ਇੰਦਰਾਜ.m_scopep;
                m_ਸਰਗਰਮ_Domainp = m_graphp->domainp(ਇੰਦਰਾਜ.m_triggers);
                iterate(ਇੰਦਰਾਜ.m_activep);
                m_scopep = ਖਾਲੀ_ਸੂਚਕ;
                m_ਸਰਗਰਮ_Domainp = ਖਾਲੀ_ਸੂਚਕ;
            }
        }
    }
    ~ਕ੍ਰਮ_ਗ੍ਰਾਫ_Builder() override = ਮੂਲ;

ਜਨਤਕ:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ਸਥਿਰ std::unique_ptr<ਕ੍ਰਮ_ਗ੍ਰਾਫ> apply(ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& coll,
                                             ਅਚਲ V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ਅਚਲ V3Sched::CovergroupRefBindings& cgRefBindings,
                                             ਤਾਰਕਿਕ parallel) {
        ਵਾਪਸ std::unique_ptr<ਕ੍ਰਮ_ਗ੍ਰਾਫ>{
            ਕ੍ਰਮ_ਗ੍ਰਾਫ_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<ਕ੍ਰਮ_ਗ੍ਰਾਫ>
V3_ਕ੍ਰਮ::build_ਕ੍ਰਮ_ਗ੍ਰਾਫ(ਅਚਲ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& coll,  //
                         ਅਚਲ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         ਤਾਰਕਿਕ parallel) {
    ਵਾਪਸ ਕ੍ਰਮ_ਗ੍ਰਾਫ_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
