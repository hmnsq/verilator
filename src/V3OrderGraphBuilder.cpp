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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3OrderGraph.h"
#অন্তর্ভাব "V3OrderInternal.h"
#অন্তর্ভাব "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

বর্গ OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
সার্বজনিক:
    // TYPES
    গণনা বর্গ VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

গুপ্ত:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

সার্বজনিক:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        ধ্রুব unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        যদি (!vertexp) {
            বিকল্প (type) {
            পক্ষ VarVertexType::STD: vertexp = নূতন OrderVarStdVertex{graphp, varscp}; ভঙ্গ;
            পক্ষ VarVertexType::PRE: vertexp = নূতন OrderVarPreVertex{graphp, varscp}; ভঙ্গ;
            পক্ষ VarVertexType::PORD: vertexp = নূতন OrderVarPordVertex{graphp, varscp}; ভঙ্গ;
            পক্ষ VarVertexType::POST: vertexp = নূতন OrderVarPostVertex{graphp, varscp}; ভঙ্গ;
            }
            m_vertexps[idx] = vertexp;
        }
        প্রত্যাগম vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = স্বভাব;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

বর্গ OrderGraphBuilder final : সার্বজনিক VNVisitor {
    // TYPES
    গণনা VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    গণনা VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ধ্রুব VNUser1InUse user1InUse;
    ধ্রুব VNUser2InUse user2InUse;
    ধ্রুব VNUser3InUse user3InUse;
    ধ্রুব VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* ধ্রুব m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    ধ্রুব OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ধ্রুব OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ধ্রুব OrderDomain* m_hybridp = nullptr;

    তার্কিক m_inClocked = অসত্য;  // Underneath clocked AstActive
    তার্কিক m_inPre = অসত্য;  // Underneath AlwaysPre
    তার্কিক m_inPost = অসত্য;  // Underneath AstAlwaysPost
    std::function<তার্কিক(ধ্রুব AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    ধ্রুব তার্কিক m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ধ্রুব V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ধ্রুব V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    শূন্য iterateLogic(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_গ্রন্থি::user2_Clear_বৃক্ষ();
        Ast_গ্রন্থি::user4_Clear_বৃক্ষ();
        m_forceReadEdgeIgnores.clear();
        যদি (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = নূতন OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        যদি (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            প্রতি (AstVarScope* ধ্রুব vscp : m_accessedVscps) {
                ধ্রুব পূর্ণাঙ্ক recorded = vscp->user4();
                ধ্রুব VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        প্রত্যাগম m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    শূন্য visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_বৃক্ষ_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        ধ্রুব Ast_Sen_বৃক্ষ* ধ্রুব senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        যদি (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_গ্রন্থি::user3_Clear_বৃক্ষ();
            senTreep->foreach([](ধ্রুব AstVarRef* refp) {  //
                refp->varScopep()->user3(সত্য);
            });
            m_readTriggersCombLogic = [](ধ্রুব AstVarScope* vscp) { প্রত্যাগম !vscp->user3(); };
        } অন্যথা {
            যদি (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](ধ্রুব AstVarScope*) { প্রত্যাগম সত্য; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ধ্রুব varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ধ্রুব AstVar* ধ্রুব varp = nodep->varp();
        যদি (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            প্রতি (AstVarScope* ধ্রুব boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } অন্যথা {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    শূন্য recordRawAccess(AstVarScope* varscp, ধ্রুব VAccess& access, Ast_গ্রন্থি* nodep) {
        যদি (!m_parallel) প্রত্যাগম;
        uint8_t recorded = 0;
        যদি (access.isWriteOrRW()) recorded |= VA_WRITE;
        যদি (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        যদি (!varscp->user4Or(recorded)) m_accessedVscps.ঠেলো_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    শূন্য accountVarAccess(AstVarScope* varscp, ধ্রুব VAccess& access, Ast_গ্রন্থি* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ধ্রুব তার্কিক prevGen = varscp->user2() & VU_GEN;
        ধ্রুব তার্কিক prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ধ্রুব তার্কিক gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        তার্কিক con = অসত্য;
        যদি (!prevCon && access.isReadOrRW()) {
            con = সত্য;
            যদি (prevGen && !m_inClocked) {
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
                con = অসত্য;
            }
            যদি (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                যদি (m_forceReadEdgeIgnores.গুনতি(varscp) || m_cgRefBoundps) con = অসত্য;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        যদি (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            যদি (m_inPost) {
                যদি (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* ধ্রুব varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* ধ্রুব postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } অন্যথা যদি (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ধ্রুব varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* ধ্রুব postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } অন্যথা যদি (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* ধ্রুব ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ধ্রুব varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } অন্যথা {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* ধ্রুব ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* ধ্রুব varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        যদি (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            যদি (m_inPost) {
                // Combinational logic
                যদি (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ধ্রুব varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } অন্যথা যদি (!m_inClocked) {  // Combinational logic
                যদি (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ধ্রুব varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } অন্যথা যদি (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* ধ্রুব preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } অন্যথা {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* ধ্রুব preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* ধ্রুব postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    শূন্য visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ধ্রুব funcp = nodep->funcp();
        যদি (!funcp->isCovergroupSample()) প্রত্যাগম;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ধ্রুব AstVarScope* instp = nullptr;
        যদি (ধ্রুব AstVarRef* ধ্রুব fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    শূন্য visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    শূন্য visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    শূন্য visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    শূন্য visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    শূন্য visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = সত্য;
        iterateLogic(nodep);
    }
    শূন্য visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = সত্য;
        iterateLogic(nodep);
    }
    শূন্য visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    শূন্য visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    শূন্য visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    শূন্য visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    শূন্য visit(AstVar*) override {}
    শূন্য visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    শূন্য visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    শূন্য visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    শূন্য visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    শূন্য visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(ধ্রুব std::vector<V3Sched::LogicByScope*>& coll,
                      ধ্রুব V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ধ্রুব V3Sched::CovergroupRefBindings& cgRefBindings, তার্কিক parallel)
        : m_graphp{নূতন OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        প্রতি (ধ্রুব V3Sched::LogicByScope* ধ্রুব lbsp : coll) {
            প্রতি (ধ্রুব auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = স্বভাব;

সার্বজনিক:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    স্থির std::unique_ptr<OrderGraph> apply(ধ্রুব std::vector<V3Sched::LogicByScope*>& coll,
                                             ধ্রুব V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ধ্রুব V3Sched::CovergroupRefBindings& cgRefBindings,
                                             তার্কিক parallel) {
        প্রত্যাগম std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(ধ্রুব V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ধ্রুব std::vector<V3Sched::LogicByScope*>& coll,  //
                         ধ্রুব V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         তার্কিক parallel) {
    প্রত্যাগম OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
