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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3OrderGraph.h"
#અન્તર્ભાવ "V3OrderInternal.h"
#અન્તર્ભાવ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

વર્ગ OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
જાહેર:
    // TYPES
    ગણના વર્ગ VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

ખાનગી:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

જાહેર:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        અચલ unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        જો (!vertexp) {
            પસંદગી (type) {
            સ્થિતિ VarVertexType::STD: vertexp = નવું OrderVarStdVertex{graphp, varscp}; તોડો;
            સ્થિતિ VarVertexType::PRE: vertexp = નવું OrderVarPreVertex{graphp, varscp}; તોડો;
            સ્થિતિ VarVertexType::PORD: vertexp = નવું OrderVarPordVertex{graphp, varscp}; તોડો;
            સ્થિતિ VarVertexType::POST: vertexp = નવું OrderVarPostVertex{graphp, varscp}; તોડો;
            }
            m_vertexps[idx] = vertexp;
        }
        પાછા vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = મૂળભૂત;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

વર્ગ OrderGraphBuilder final : જાહેર VNVisitor {
    // TYPES
    ગણના VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    ગણના VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    અચલ VNUser1InUse user1InUse;
    અચલ VNUser2InUse user2InUse;
    અચલ VNUser3InUse user3InUse;
    અચલ VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* અચલ m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    અચલ OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    અચલ OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    અચલ OrderDomain* m_hybridp = nullptr;

    તાર્કિક m_inClocked = ખોટું;  // Underneath clocked AstActive
    તાર્કિક m_inPre = ખોટું;  // Underneath AlwaysPre
    તાર્કિક m_inPost = ખોટું;  // Underneath AstAlwaysPost
    std::function<તાર્કિક(અચલ AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    અચલ તાર્કિક m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    અચલ V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    અચલ V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    શૂન્ય iterateLogic(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_ગાંઠ::user2_Clear_વૃક્ષ();
        Ast_ગાંઠ::user4_Clear_વૃક્ષ();
        m_forceReadEdgeIgnores.clear();
        જો (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = નવું OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        જો (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            માટે (AstVarScope* અચલ vscp : m_accessedVscps) {
                અચલ પૂર્ણાંક recorded = vscp->user4();
                અચલ VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->addVarAccess(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = nullptr;
        m_forceReadEdgeIgnores.clear();
    }

    OrderVarVertex* getVarVertex(AstVarScope* varscp, VarVertexType type) {
        પાછા m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    શૂન્ય visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_વૃક્ષ_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        અચલ Ast_Sen_વૃક્ષ* અચલ senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        જો (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_ગાંઠ::user3_Clear_વૃક્ષ();
            senTreep->foreach([](અચલ AstVarRef* refp) {  //
                refp->varScopep()->user3(સાચું);
            });
            m_readTriggersCombLogic = [](અચલ AstVarScope* vscp) { પાછા !vscp->user3(); };
        } નહીંતર {
            જો (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](અચલ AstVarScope*) { પાછા સાચું; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* અચલ varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        અચલ AstVar* અચલ varp = nodep->varp();
        જો (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            માટે (AstVarScope* અચલ boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } નહીંતર {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    શૂન્ય recordRawAccess(AstVarScope* varscp, અચલ VAccess& access, Ast_ગાંઠ* nodep) {
        જો (!m_parallel) પાછા;
        uint8_t recorded = 0;
        જો (access.isWriteOrRW()) recorded |= VA_WRITE;
        જો (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        જો (!varscp->user4Or(recorded)) m_accessedVscps.ધકેલો_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    શૂન્ય accountVarAccess(AstVarScope* varscp, અચલ VAccess& access, Ast_ગાંઠ* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        અચલ તાર્કિક prevGen = varscp->user2() & VU_GEN;
        અચલ તાર્કિક prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        અચલ તાર્કિક gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        તાર્કિક con = ખોટું;
        જો (!prevCon && access.isReadOrRW()) {
            con = સાચું;
            જો (prevGen && !m_inClocked) {
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
                con = ખોટું;
            }
            જો (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                જો (m_forceReadEdgeIgnores.ગણતરી(varscp) || m_cgRefBoundps) con = ખોટું;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        જો (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            જો (m_inPost) {
                જો (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* અચલ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* અચલ postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } નહીંતર જો (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* અચલ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* અચલ postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } નહીંતર જો (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* અચલ ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* અચલ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } નહીંતર {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* અચલ ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* અચલ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        જો (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            જો (m_inPost) {
                // Combinational logic
                જો (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* અચલ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } નહીંતર જો (!m_inClocked) {  // Combinational logic
                જો (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* અચલ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } નહીંતર જો (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* અચલ preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } નહીંતર {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* અચલ preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* અચલ postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    શૂન્ય visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* અચલ funcp = nodep->funcp();
        જો (!funcp->isCovergroupSample()) પાછા;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        અચલ AstVarScope* instp = nullptr;
        જો (અચલ AstVarRef* અચલ fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    શૂન્ય visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    શૂન્ય visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    શૂન્ય visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = સાચું;
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = સાચું;
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    શૂન્ય visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    શૂન્ય visit(AstVar*) override {}
    શૂન્ય visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    શૂન્ય visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    શૂન્ય visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    શૂન્ય visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    શૂન્ય visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(અચલ std::vector<V3Sched::LogicByScope*>& coll,
                      અચલ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      અચલ V3Sched::CovergroupRefBindings& cgRefBindings, તાર્કિક parallel)
        : m_graphp{નવું OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        માટે (અચલ V3Sched::LogicByScope* અચલ lbsp : coll) {
            માટે (અચલ auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = મૂળભૂત;

જાહેર:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    સ્થિર std::unique_ptr<OrderGraph> apply(અચલ std::vector<V3Sched::LogicByScope*>& coll,
                                             અચલ V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             અચલ V3Sched::CovergroupRefBindings& cgRefBindings,
                                             તાર્કિક parallel) {
        પાછા std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(અચલ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         અચલ std::vector<V3Sched::LogicByScope*>& coll,  //
                         અચલ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         તાર્કિક parallel) {
    પાછા OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
