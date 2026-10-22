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

ವರ್ಗ OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ಗಣನಾ ವರ್ಗ VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

ಗುಪ್ತ:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        ಧ್ರುವ unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        ಯದಿ (!vertexp) {
            ವಿಕಲ್ಪ (type) {
            ಪಕ್ಷ VarVertexType::STD: vertexp = ನೂತನ OrderVarStdVertex{graphp, varscp}; ಭಙ್ಗ;
            ಪಕ್ಷ VarVertexType::PRE: vertexp = ನೂತನ OrderVarPreVertex{graphp, varscp}; ಭಙ್ಗ;
            ಪಕ್ಷ VarVertexType::PORD: vertexp = ನೂತನ OrderVarPordVertex{graphp, varscp}; ಭಙ್ಗ;
            ಪಕ್ಷ VarVertexType::POST: vertexp = ನೂತನ OrderVarPostVertex{graphp, varscp}; ಭಙ್ಗ;
            }
            m_vertexps[idx] = vertexp;
        }
        ಪ್ರತ್ಯಾಗಮ vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = ಸ್ವಭಾವ;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

ವರ್ಗ OrderGraphBuilder final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    ಗಣನಾ VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    ಗಣನಾ VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ಧ್ರುವ VNUser1InUse user1InUse;
    ಧ್ರುವ VNUser2InUse user2InUse;
    ಧ್ರುವ VNUser3InUse user3InUse;
    ಧ್ರುವ VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* ಧ್ರುವ m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    ಧ್ರುವ OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ಧ್ರುವ OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ಧ್ರುವ OrderDomain* m_hybridp = nullptr;

    ತಾರ್ಕಿಕ m_inClocked = ಅಸತ್ಯ;  // Underneath clocked AstActive
    ತಾರ್ಕಿಕ m_inPre = ಅಸತ್ಯ;  // Underneath AlwaysPre
    ತಾರ್ಕಿಕ m_inPost = ಅಸತ್ಯ;  // Underneath AstAlwaysPost
    std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    ಧ್ರುವ ತಾರ್ಕಿಕ m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ಧ್ರುವ V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ಧ್ರುವ V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    ಶೂನ್ಯ iterateLogic(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        Ast_ಗಂಟು::user4_Clear_ವೃಕ್ಷ();
        m_forceReadEdgeIgnores.clear();
        ಯದಿ (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = ನೂತನ OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        ಯದಿ (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_accessedVscps) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ recorded = vscp->user4();
                ಧ್ರುವ VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        ಪ್ರತ್ಯಾಗಮ m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_ವೃಕ್ಷ_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        ಯದಿ (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_ಗಂಟು::user3_Clear_ವೃಕ್ಷ();
            senTreep->foreach([](ಧ್ರುವ AstVarRef* refp) {  //
                refp->varScopep()->user3(ಸತ್ಯ);
            });
            m_readTriggersCombLogic = [](ಧ್ರುವ AstVarScope* vscp) { ಪ್ರತ್ಯಾಗಮ !vscp->user3(); };
        } ಅನ್ಯಥಾ {
            ಯದಿ (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](ಧ್ರುವ AstVarScope*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ಧ್ರುವ varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            ಪ್ರತಿ (AstVarScope* ಧ್ರುವ boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } ಅನ್ಯಥಾ {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    ಶೂನ್ಯ recordRawAccess(AstVarScope* varscp, ಧ್ರುವ VAccess& access, Ast_ಗಂಟು* nodep) {
        ಯದಿ (!m_parallel) ಪ್ರತ್ಯಾಗಮ;
        uint8_t recorded = 0;
        ಯದಿ (access.isWriteOrRW()) recorded |= VA_WRITE;
        ಯದಿ (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        ಯದಿ (!varscp->user4Or(recorded)) m_accessedVscps.ತಳ್ಳು_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    ಶೂನ್ಯ accountVarAccess(AstVarScope* varscp, ಧ್ರುವ VAccess& access, Ast_ಗಂಟು* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ಧ್ರುವ ತಾರ್ಕಿಕ prevGen = varscp->user2() & VU_GEN;
        ಧ್ರುವ ತಾರ್ಕಿಕ prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ಧ್ರುವ ತಾರ್ಕಿಕ gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        ತಾರ್ಕಿಕ con = ಅಸತ್ಯ;
        ಯದಿ (!prevCon && access.isReadOrRW()) {
            con = ಸತ್ಯ;
            ಯದಿ (prevGen && !m_inClocked) {
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
                con = ಅಸತ್ಯ;
            }
            ಯದಿ (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                ಯದಿ (m_forceReadEdgeIgnores.ಎಣಿಕೆ(varscp) || m_cgRefBoundps) con = ಅಸತ್ಯ;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        ಯದಿ (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            ಯದಿ (m_inPost) {
                ಯದಿ (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* ಧ್ರುವ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* ಧ್ರುವ postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } ಅನ್ಯಥಾ ಯದಿ (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ಧ್ರುವ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* ಧ್ರುವ postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } ಅನ್ಯಥಾ ಯದಿ (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* ಧ್ರುವ ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ಧ್ರುವ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } ಅನ್ಯಥಾ {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* ಧ್ರುವ ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* ಧ್ರುವ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        ಯದಿ (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            ಯದಿ (m_inPost) {
                // Combinational logic
                ಯದಿ (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ಧ್ರುವ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } ಅನ್ಯಥಾ ಯದಿ (!m_inClocked) {  // Combinational logic
                ಯದಿ (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ಧ್ರುವ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } ಅನ್ಯಥಾ ಯದಿ (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* ಧ್ರುವ preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } ಅನ್ಯಥಾ {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* ಧ್ರುವ preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* ಧ್ರುವ postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    ಶೂನ್ಯ visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ಧ್ರುವ funcp = nodep->funcp();
        ಯದಿ (!funcp->isCovergroupSample()) ಪ್ರತ್ಯಾಗಮ;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ಧ್ರುವ AstVarScope* instp = nullptr;
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    ಶೂನ್ಯ visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    ಶೂನ್ಯ visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    ಶೂನ್ಯ visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = ಸತ್ಯ;
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = ಸತ್ಯ;
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    ಶೂನ್ಯ visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    ಶೂನ್ಯ visit(AstVar*) override {}
    ಶೂನ್ಯ visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ಶೂನ್ಯ visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(ಧ್ರುವ std::vector<V3Sched::LogicByScope*>& coll,
                      ಧ್ರುವ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ಧ್ರುವ V3Sched::CovergroupRefBindings& cgRefBindings, ತಾರ್ಕಿಕ parallel)
        : m_graphp{ನೂತನ OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        ಪ್ರತಿ (ಧ್ರುವ V3Sched::LogicByScope* ಧ್ರುವ lbsp : coll) {
            ಪ್ರತಿ (ಧ್ರುವ auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ಸ್ಥಿರ std::unique_ptr<OrderGraph> apply(ಧ್ರುವ std::vector<V3Sched::LogicByScope*>& coll,
                                             ಧ್ರುವ V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ಧ್ರುವ V3Sched::CovergroupRefBindings& cgRefBindings,
                                             ತಾರ್ಕಿಕ parallel) {
        ಪ್ರತ್ಯಾಗಮ std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(ಧ್ರುವ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ಧ್ರುವ std::vector<V3Sched::LogicByScope*>& coll,  //
                         ಧ್ರುವ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         ತಾರ್ಕಿಕ parallel) {
    ಪ್ರತ್ಯಾಗಮ OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
