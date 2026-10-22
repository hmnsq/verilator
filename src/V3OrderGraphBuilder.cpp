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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Graph.h"
#समावेश "V3OrderGraph.h"
#समावेश "V3OrderInternal.h"
#समावेश "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

श्रेणी OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
खुला:
    // TYPES
    क्रमागत श्रेणी VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

निजी:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<माप_प्रकार>(VarVertexType::POST) + 1> m_vertexps;

खुला:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        स्थिर अचिन्हित idx = static_cast<अचिन्हित>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        अगर (!vertexp) {
            चयन (type) {
            हाल VarVertexType::STD: vertexp = नया OrderVarStdVertex{graphp, varscp}; अवरोध;
            हाल VarVertexType::PRE: vertexp = नया OrderVarPreVertex{graphp, varscp}; अवरोध;
            हाल VarVertexType::PORD: vertexp = नया OrderVarPordVertex{graphp, varscp}; अवरोध;
            हाल VarVertexType::POST: vertexp = नया OrderVarPostVertex{graphp, varscp}; अवरोध;
            }
            m_vertexps[idx] = vertexp;
        }
        वापस vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = शेष;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

श्रेणी OrderGraphBuilder final : खुला VNVisitor {
    // TYPES
    क्रमागत VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    क्रमागत VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    स्थिर VNUser1InUse user1InUse;
    स्थिर VNUser2InUse user2InUse;
    स्थिर VNUser3InUse user3InUse;
    स्थिर VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* स्थिर m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    स्थिर OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    स्थिर OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    स्थिर OrderDomain* m_hybridp = nullptr;

    bool m_inClocked = false;  // Underneath clocked AstActive
    bool m_inPre = false;  // Underneath AlwaysPre
    bool m_inPost = false;  // Underneath AstAlwaysPost
    std::function<bool(स्थिर AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    स्थिर bool m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    स्थिर V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    स्थिर V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    व्योम iterateLogic(Ast_गाँठ* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_गाँठ::user2_Clear_वृक्ष();
        Ast_गाँठ::user4_Clear_वृक्ष();
        m_forceReadEdgeIgnores.सफा();
        अगर (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = नया OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        अगर (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            क्रम (AstVarScope* स्थिर vscp : m_accessedVscps) {
                स्थिर पूर्णांक recorded = vscp->user4();
                स्थिर VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->addVarAccess(vscp, access);
            }
            m_accessedVscps.सफा();
        }
        // Finished with this logic
        m_logicVxp = nullptr;
        m_forceReadEdgeIgnores.सफा();
    }

    OrderVarVertex* getVarVertex(AstVarScope* varscp, VarVertexType type) {
        वापस m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    व्योम visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_वृक्ष_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        स्थिर Ast_Sen_वृक्ष* स्थिर senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        अगर (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_गाँठ::user3_Clear_वृक्ष();
            senTreep->foreach([](स्थिर AstVarRef* refp) {  //
                refp->varScopep()->user3(true);
            });
            m_readTriggersCombLogic = [](स्थिर AstVarScope* vscp) { वापस !vscp->user3(); };
        } अन्यथा {
            अगर (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](स्थिर AstVarScope*) { वापस true; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* स्थिर varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        स्थिर AstVar* स्थिर varp = nodep->varp();
        अगर (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            क्रम (AstVarScope* स्थिर boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } अन्यथा {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    व्योम recordRawAccess(AstVarScope* varscp, स्थिर VAccess& access, Ast_गाँठ* nodep) {
        अगर (!m_parallel) वापस;
        uint8_t recorded = 0;
        अगर (access.isWriteOrRW()) recorded |= VA_WRITE;
        अगर (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        अगर (!varscp->user4Or(recorded)) m_accessedVscps.धकेलो_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    व्योम accountVarAccess(AstVarScope* varscp, स्थिर VAccess& access, Ast_गाँठ* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        स्थिर bool पूर्व_Gen = varscp->user2() & VU_GEN;
        स्थिर bool पूर्व_Con = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        स्थिर bool gen = !पूर्व_Gen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        bool con = false;
        अगर (!पूर्व_Con && access.isReadOrRW()) {
            con = true;
            अगर (पूर्व_Gen && !m_inClocked) {
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
                con = false;
            }
            अगर (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                अगर (m_forceReadEdgeIgnores.गिनती(varscp) || m_cgRefBoundps) con = false;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        अगर (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            अगर (m_inPost) {
                अगर (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* स्थिर varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* स्थिर postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } अन्यथा अगर (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* स्थिर varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* स्थिर postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } अन्यथा अगर (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* स्थिर ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* स्थिर varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } अन्यथा {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* स्थिर ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* स्थिर varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        अगर (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            अगर (m_inPost) {
                // Combinational logic
                अगर (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* स्थिर varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } अन्यथा अगर (!m_inClocked) {  // Combinational logic
                अगर (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* स्थिर varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } अन्यथा अगर (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* स्थिर preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } अन्यथा {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* स्थिर preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* स्थिर postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    व्योम visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* स्थिर funcp = nodep->funcp();
        अगर (!funcp->isCovergroupSample()) वापस;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        स्थिर AstVarScope* instp = nullptr;
        अगर (स्थिर AstVarRef* स्थिर fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    व्योम visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    व्योम visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    व्योम visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    व्योम visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    व्योम visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = true;
        iterateLogic(nodep);
    }
    व्योम visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = true;
        iterateLogic(nodep);
    }
    व्योम visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    व्योम visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    व्योम visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    व्योम visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    व्योम visit(AstVar*) override {}
    व्योम visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    व्योम visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    व्योम visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    व्योम visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    व्योम visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(स्थिर std::vector<V3Sched::LogicByScope*>& coll,
                      स्थिर V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      स्थिर V3Sched::CovergroupRefBindings& cgRefBindings, bool parallel)
        : m_graphp{नया OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        क्रम (स्थिर V3Sched::LogicByScope* स्थिर lbsp : coll) {
            क्रम (स्थिर स्वतः& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = शेष;

खुला:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    अटल std::unique_ptr<OrderGraph> apply(स्थिर std::vector<V3Sched::LogicByScope*>& coll,
                                             स्थिर V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             स्थिर V3Sched::CovergroupRefBindings& cgRefBindings,
                                             bool parallel) {
        वापस std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(स्थिर V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         स्थिर std::vector<V3Sched::LogicByScope*>& coll,  //
                         स्थिर V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         bool parallel) {
    वापस OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
