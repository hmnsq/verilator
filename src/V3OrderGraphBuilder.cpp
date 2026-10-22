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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3OrderGraph.h"
#அந்தர்பாவ "V3OrderInternal.h"
#அந்தர்பாவ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

வகுப்பு OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
பொது:
    // TYPES
    எண்ணிக்கை வகுப்பு VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

தனிப்பட்ட:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

பொது:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        மாறிலி unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        எனில் (!vertexp) {
            தேர்வு (type) {
            நிலை VarVertexType::STD: vertexp = புதிய OrderVarStdVertex{graphp, varscp}; நிறுத்து;
            நிலை VarVertexType::PRE: vertexp = புதிய OrderVarPreVertex{graphp, varscp}; நிறுத்து;
            நிலை VarVertexType::PORD: vertexp = புதிய OrderVarPordVertex{graphp, varscp}; நிறுத்து;
            நிலை VarVertexType::POST: vertexp = புதிய OrderVarPostVertex{graphp, varscp}; நிறுத்து;
            }
            m_vertexps[idx] = vertexp;
        }
        திரும்பு vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = இயல்பு;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

வகுப்பு OrderGraphBuilder final : பொது VNVisitor {
    // TYPES
    எண்ணிக்கை VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    எண்ணிக்கை VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    மாறிலி VNUser1InUse user1InUse;
    மாறிலி VNUser2InUse user2InUse;
    மாறிலி VNUser3InUse user3InUse;
    மாறிலி VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* மாறிலி m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    மாறிலி OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    மாறிலி OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    மாறிலி OrderDomain* m_hybridp = nullptr;

    தருக்க m_inClocked = பொய்;  // Underneath clocked AstActive
    தருக்க m_inPre = பொய்;  // Underneath AlwaysPre
    தருக்க m_inPost = பொய்;  // Underneath AstAlwaysPost
    std::function<தருக்க(மாறிலி AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    மாறிலி தருக்க m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    மாறிலி V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    மாறிலி V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    வெறுமை iterateLogic(Ast_முனை* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_முனை::user2_Clear_மரம்();
        Ast_முனை::user4_Clear_மரம்();
        m_forceReadEdgeIgnores.clear();
        எனில் (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = புதிய OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        எனில் (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            க்கு (AstVarScope* மாறிலி vscp : m_accessedVscps) {
                மாறிலி முழு recorded = vscp->user4();
                மாறிலி VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        திரும்பு m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    வெறுமை visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_மரம்_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        மாறிலி Ast_Sen_மரம்* மாறிலி senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        எனில் (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_முனை::user3_Clear_மரம்();
            senTreep->foreach([](மாறிலி AstVarRef* refp) {  //
                refp->varScopep()->user3(உண்மை);
            });
            m_readTriggersCombLogic = [](மாறிலி AstVarScope* vscp) { திரும்பு !vscp->user3(); };
        } இல்லையெனில் {
            எனில் (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](மாறிலி AstVarScope*) { திரும்பு உண்மை; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* மாறிலி varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        மாறிலி AstVar* மாறிலி varp = nodep->varp();
        எனில் (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            க்கு (AstVarScope* மாறிலி boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } இல்லையெனில் {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    வெறுமை recordRawAccess(AstVarScope* varscp, மாறிலி VAccess& access, Ast_முனை* nodep) {
        எனில் (!m_parallel) திரும்பு;
        uint8_t recorded = 0;
        எனில் (access.isWriteOrRW()) recorded |= VA_WRITE;
        எனில் (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        எனில் (!varscp->user4Or(recorded)) m_accessedVscps.தள்ளு_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    வெறுமை accountVarAccess(AstVarScope* varscp, மாறிலி VAccess& access, Ast_முனை* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        மாறிலி தருக்க prevGen = varscp->user2() & VU_GEN;
        மாறிலி தருக்க prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        மாறிலி தருக்க gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        தருக்க con = பொய்;
        எனில் (!prevCon && access.isReadOrRW()) {
            con = உண்மை;
            எனில் (prevGen && !m_inClocked) {
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
                con = பொய்;
            }
            எனில் (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                எனில் (m_forceReadEdgeIgnores.மொத்தம்(varscp) || m_cgRefBoundps) con = பொய்;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        எனில் (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            எனில் (m_inPost) {
                எனில் (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* மாறிலி varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* மாறிலி postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } இல்லையெனில் எனில் (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* மாறிலி varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* மாறிலி postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } இல்லையெனில் எனில் (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* மாறிலி ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* மாறிலி varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } இல்லையெனில் {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* மாறிலி ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* மாறிலி varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        எனில் (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            எனில் (m_inPost) {
                // Combinational logic
                எனில் (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* மாறிலி varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } இல்லையெனில் எனில் (!m_inClocked) {  // Combinational logic
                எனில் (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* மாறிலி varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } இல்லையெனில் எனில் (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* மாறிலி preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } இல்லையெனில் {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* மாறிலி preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* மாறிலி postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    வெறுமை visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* மாறிலி funcp = nodep->funcp();
        எனில் (!funcp->isCovergroupSample()) திரும்பு;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        மாறிலி AstVarScope* instp = nullptr;
        எனில் (மாறிலி AstVarRef* மாறிலி fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    வெறுமை visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    வெறுமை visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    வெறுமை visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    வெறுமை visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    வெறுமை visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = உண்மை;
        iterateLogic(nodep);
    }
    வெறுமை visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = உண்மை;
        iterateLogic(nodep);
    }
    வெறுமை visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    வெறுமை visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    வெறுமை visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    வெறுமை visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    வெறுமை visit(AstVar*) override {}
    வெறுமை visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    வெறுமை visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    வெறுமை visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    வெறுமை visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    வெறுமை visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(மாறிலி std::vector<V3Sched::LogicByScope*>& coll,
                      மாறிலி V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      மாறிலி V3Sched::CovergroupRefBindings& cgRefBindings, தருக்க parallel)
        : m_graphp{புதிய OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        க்கு (மாறிலி V3Sched::LogicByScope* மாறிலி lbsp : coll) {
            க்கு (மாறிலி auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = இயல்பு;

பொது:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    நிலையான std::unique_ptr<OrderGraph> apply(மாறிலி std::vector<V3Sched::LogicByScope*>& coll,
                                             மாறிலி V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             மாறிலி V3Sched::CovergroupRefBindings& cgRefBindings,
                                             தருக்க parallel) {
        திரும்பு std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(மாறிலி V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         மாறிலி std::vector<V3Sched::LogicByScope*>& coll,  //
                         மாறிலி V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         தருக்க parallel) {
    திரும்பு OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
