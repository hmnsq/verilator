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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderInternal.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

ଶ୍ରେଣୀ OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
ସର୍ବସାଧାରଣ:
    // TYPES
    ଗଣନା ଶ୍ରେଣୀ VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

ବ୍ୟକ୍ତିଗତ:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

ସର୍ବସାଧାରଣ:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        ଅଚଳ unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        ଯଦି (!vertexp) {
            ବାଛ (type) {
            ସ୍ଥିତି VarVertexType::STD: vertexp = ନୂତନ OrderVarStdVertex{graphp, varscp}; ଭାଙ୍ଗ;
            ସ୍ଥିତି VarVertexType::PRE: vertexp = ନୂତନ OrderVarPreVertex{graphp, varscp}; ଭାଙ୍ଗ;
            ସ୍ଥିତି VarVertexType::PORD: vertexp = ନୂତନ OrderVarPordVertex{graphp, varscp}; ଭାଙ୍ଗ;
            ସ୍ଥିତି VarVertexType::POST: vertexp = ନୂତନ OrderVarPostVertex{graphp, varscp}; ଭାଙ୍ଗ;
            }
            m_vertexps[idx] = vertexp;
        }
        ଫେରନ୍ତୁ vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

ଶ୍ରେଣୀ OrderGraphBuilder final : ସର୍ବସାଧାରଣ VNVisitor {
    // TYPES
    ଗଣନା VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    ଗଣନା VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ଅଚଳ VNUser1InUse user1InUse;
    ଅଚଳ VNUser2InUse user2InUse;
    ଅଚଳ VNUser3InUse user3InUse;
    ଅଚଳ VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* ଅଚଳ m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    ଅଚଳ OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ଅଚଳ OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ଅଚଳ OrderDomain* m_hybridp = nullptr;

    ତାର୍କିକ m_inClocked = ମିଥ୍ୟା;  // Underneath clocked AstActive
    ତାର୍କିକ m_inPre = ମିଥ୍ୟା;  // Underneath AlwaysPre
    ତାର୍କିକ m_inPost = ମିଥ୍ୟା;  // Underneath AstAlwaysPost
    std::function<ତାର୍କିକ(ଅଚଳ AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    ଅଚଳ ତାର୍କିକ m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ଅଚଳ V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ଅଚଳ V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    ଶୂନ୍ୟ iterateLogic(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();
        Ast_ଗଣ୍ଠି::user4_Clear_ବୃକ୍ଷ();
        m_forceReadEdgeIgnores.clear();
        ଯଦି (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = ନୂତନ OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        ଯଦି (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_accessedVscps) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା recorded = vscp->user4();
                ଅଚଳ VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        ଫେରନ୍ତୁ m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_ବୃକ୍ଷ_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        ଯଦି (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_ଗଣ୍ଠି::user3_Clear_ବୃକ୍ଷ();
            senTreep->foreach([](ଅଚଳ AstVarRef* refp) {  //
                refp->varScopep()->user3(ସତ୍ୟ);
            });
            m_readTriggersCombLogic = [](ଅଚଳ AstVarScope* vscp) { ଫେରନ୍ତୁ !vscp->user3(); };
        } ଅନ୍ୟଥା {
            ଯଦି (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](ଅଚଳ AstVarScope*) { ଫେରନ୍ତୁ ସତ୍ୟ; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ଅଚଳ varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ଅଚଳ AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            ପାଇଁ (AstVarScope* ଅଚଳ boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } ଅନ୍ୟଥା {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    ଶୂନ୍ୟ recordRawAccess(AstVarScope* varscp, ଅଚଳ VAccess& access, Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!m_parallel) ଫେରନ୍ତୁ;
        uint8_t recorded = 0;
        ଯଦି (access.isWriteOrRW()) recorded |= VA_WRITE;
        ଯଦି (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        ଯଦି (!varscp->user4Or(recorded)) m_accessedVscps.ଠେଲ_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    ଶୂନ୍ୟ accountVarAccess(AstVarScope* varscp, ଅଚଳ VAccess& access, Ast_ଗଣ୍ଠି* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ଅଚଳ ତାର୍କିକ prevGen = varscp->user2() & VU_GEN;
        ଅଚଳ ତାର୍କିକ prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ଅଚଳ ତାର୍କିକ gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        ତାର୍କିକ con = ମିଥ୍ୟା;
        ଯଦି (!prevCon && access.isReadOrRW()) {
            con = ସତ୍ୟ;
            ଯଦି (prevGen && !m_inClocked) {
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
                con = ମିଥ୍ୟା;
            }
            ଯଦି (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                ଯଦି (m_forceReadEdgeIgnores.ଗଣନା_2(varscp) || m_cgRefBoundps) con = ମିଥ୍ୟା;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        ଯଦି (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            ଯଦି (m_inPost) {
                ଯଦି (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* ଅଚଳ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* ଅଚଳ postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } ଅନ୍ୟଥା ଯଦି (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ଅଚଳ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* ଅଚଳ postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } ଅନ୍ୟଥା ଯଦି (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* ଅଚଳ ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ଅଚଳ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } ଅନ୍ୟଥା {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* ଅଚଳ ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* ଅଚଳ varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        ଯଦି (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            ଯଦି (m_inPost) {
                // Combinational logic
                ଯଦି (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ଅଚଳ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } ଅନ୍ୟଥା ଯଦି (!m_inClocked) {  // Combinational logic
                ଯଦି (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ଅଚଳ varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } ଅନ୍ୟଥା ଯଦି (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* ଅଚଳ preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } ଅନ୍ୟଥା {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* ଅଚଳ preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* ଅଚଳ postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    ଶୂନ୍ୟ visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ଅଚଳ funcp = nodep->funcp();
        ଯଦି (!funcp->isCovergroupSample()) ଫେରନ୍ତୁ;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ଅଚଳ AstVarScope* instp = nullptr;
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    ଶୂନ୍ୟ visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    ଶୂନ୍ୟ visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = ସତ୍ୟ;
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = ସତ୍ୟ;
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    ଶୂନ୍ୟ visit(AstVar*) override {}
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ଶୂନ୍ୟ visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ଶୂନ୍ୟ visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ଶୂନ୍ୟ visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ଶୂନ୍ୟ visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(ଅଚଳ std::vector<V3Sched::LogicByScope*>& coll,
                      ଅଚଳ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ଅଚଳ V3Sched::CovergroupRefBindings& cgRefBindings, ତାର୍କିକ parallel)
        : m_graphp{ନୂତନ OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        ପାଇଁ (ଅଚଳ V3Sched::LogicByScope* ଅଚଳ lbsp : coll) {
            ପାଇଁ (ଅଚଳ auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ସ୍ଥିର std::unique_ptr<OrderGraph> apply(ଅଚଳ std::vector<V3Sched::LogicByScope*>& coll,
                                             ଅଚଳ V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ଅଚଳ V3Sched::CovergroupRefBindings& cgRefBindings,
                                             ତାର୍କିକ parallel) {
        ଫେରନ୍ତୁ std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(ଅଚଳ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ଅଚଳ std::vector<V3Sched::LogicByScope*>& coll,  //
                         ଅଚଳ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         ତାର୍କିକ parallel) {
    ଫେରନ୍ତୁ OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
