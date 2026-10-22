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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3OrderGraph.h"
#अन्तर्भाव "V3OrderInternal.h"
#अन्तर्भाव "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

वर्ग OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
सार्वजनिक:
    // TYPES
    गणना वर्ग VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

गुप्त:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<माप_प्रकार>(VarVertexType::POST) + 1> m_vertexps;

सार्वजनिक:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        अचर अचिन्हित idx = static_cast<अचिन्हित>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        यदि (!vertexp) {
            विकल्प (type) {
            पक्ष VarVertexType::STD: vertexp = नूतन OrderVarStdVertex{graphp, varscp}; रोक;
            पक्ष VarVertexType::PRE: vertexp = नूतन OrderVarPreVertex{graphp, varscp}; रोक;
            पक्ष VarVertexType::PORD: vertexp = नूतन OrderVarPordVertex{graphp, varscp}; रोक;
            पक्ष VarVertexType::POST: vertexp = नूतन OrderVarPostVertex{graphp, varscp}; रोक;
            }
            m_vertexps[idx] = vertexp;
        }
        फर्काऊ vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = स्वभाव;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

वर्ग OrderGraphBuilder final : सार्वजनिक VNVisitor {
    // TYPES
    गणना VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    गणना VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    अचर VNUser1InUse user1InUse;
    अचर VNUser2InUse user2InUse;
    अचर VNUser3InUse user3InUse;
    अचर VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* अचर m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    अचर OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    अचर OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    अचर OrderDomain* m_hybridp = nullptr;

    तार्किक m_inClocked = असत्य;  // Underneath clocked AstActive
    तार्किक m_inPre = असत्य;  // Underneath AlwaysPre
    तार्किक m_inPost = असत्य;  // Underneath AstAlwaysPost
    std::function<तार्किक(अचर AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    अचर तार्किक m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    अचर V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    अचर V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    शून्य iterateLogic(Ast_गाँठो* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_गाँठो::user2_Clear_रूख();
        Ast_गाँठो::user4_Clear_रूख();
        m_forceReadEdgeIgnores.clear();
        यदि (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = नूतन OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        यदि (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            लागि (AstVarScope* अचर vscp : m_accessedVscps) {
                अचर पूर्णाङ्क recorded = vscp->user4();
                अचर VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        फर्काऊ m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    शून्य visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_रूख_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        अचर Ast_Sen_रूख* अचर senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        यदि (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_गाँठो::user3_Clear_रूख();
            senTreep->foreach([](अचर AstVarRef* refp) {  //
                refp->varScopep()->user3(सत्य);
            });
            m_readTriggersCombLogic = [](अचर AstVarScope* vscp) { फर्काऊ !vscp->user3(); };
        } अन्यथा {
            यदि (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](अचर AstVarScope*) { फर्काऊ सत्य; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* अचर varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        अचर AstVar* अचर varp = nodep->varp();
        यदि (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            लागि (AstVarScope* अचर boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } अन्यथा {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    शून्य recordRawAccess(AstVarScope* varscp, अचर VAccess& access, Ast_गाँठो* nodep) {
        यदि (!m_parallel) फर्काऊ;
        uint8_t recorded = 0;
        यदि (access.isWriteOrRW()) recorded |= VA_WRITE;
        यदि (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        यदि (!varscp->user4Or(recorded)) m_accessedVscps.थप_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    शून्य accountVarAccess(AstVarScope* varscp, अचर VAccess& access, Ast_गाँठो* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        अचर तार्किक prevGen = varscp->user2() & VU_GEN;
        अचर तार्किक prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        अचर तार्किक gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        तार्किक con = असत्य;
        यदि (!prevCon && access.isReadOrRW()) {
            con = सत्य;
            यदि (prevGen && !m_inClocked) {
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
                con = असत्य;
            }
            यदि (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                यदि (m_forceReadEdgeIgnores.गन्ती(varscp) || m_cgRefBoundps) con = असत्य;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        यदि (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            यदि (m_inPost) {
                यदि (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* अचर varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* अचर postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } अन्यथा यदि (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* अचर varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* अचर postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } अन्यथा यदि (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* अचर ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* अचर varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } अन्यथा {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* अचर ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* अचर varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        यदि (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            यदि (m_inPost) {
                // Combinational logic
                यदि (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* अचर varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } अन्यथा यदि (!m_inClocked) {  // Combinational logic
                यदि (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* अचर varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } अन्यथा यदि (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* अचर preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } अन्यथा {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* अचर preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* अचर postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    शून्य visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* अचर funcp = nodep->funcp();
        यदि (!funcp->isCovergroupSample()) फर्काऊ;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        अचर AstVarScope* instp = nullptr;
        यदि (अचर AstVarRef* अचर fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    शून्य visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    शून्य visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    शून्य visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = सत्य;
        iterateLogic(nodep);
    }
    शून्य visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = सत्य;
        iterateLogic(nodep);
    }
    शून्य visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    शून्य visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    शून्य visit(AstVar*) override {}
    शून्य visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(अचर std::vector<V3Sched::LogicByScope*>& coll,
                      अचर V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      अचर V3Sched::CovergroupRefBindings& cgRefBindings, तार्किक parallel)
        : m_graphp{नूतन OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        लागि (अचर V3Sched::LogicByScope* अचर lbsp : coll) {
            लागि (अचर स्वतः& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = स्वभाव;

सार्वजनिक:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    स्थिर std::unique_ptr<OrderGraph> apply(अचर std::vector<V3Sched::LogicByScope*>& coll,
                                             अचर V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             अचर V3Sched::CovergroupRefBindings& cgRefBindings,
                                             तार्किक parallel) {
        फर्काऊ std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(अचर V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         अचर std::vector<V3Sched::LogicByScope*>& coll,  //
                         अचर V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         तार्किक parallel) {
    फर्काऊ OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
