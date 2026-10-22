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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AstUserAllocator.h"
#شامل "V3Graph.h"
#شامل "V3OrderGraph.h"
#شامل "V3OrderInternal.h"
#شامل "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

رده OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
عمومی:
    // TYPES
    شمارش رده VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

خصوصی:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

عمومی:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        ثابت unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        اگر (!vertexp) {
            انتخاب (type) {
            حالت VarVertexType::STD: vertexp = جدید OrderVarStdVertex{graphp, varscp}; بشکن;
            حالت VarVertexType::PRE: vertexp = جدید OrderVarPreVertex{graphp, varscp}; بشکن;
            حالت VarVertexType::PORD: vertexp = جدید OrderVarPordVertex{graphp, varscp}; بشکن;
            حالت VarVertexType::POST: vertexp = جدید OrderVarPostVertex{graphp, varscp}; بشکن;
            }
            m_vertexps[idx] = vertexp;
        }
        بازگشت vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = پیشفرض;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

رده OrderGraphBuilder final : عمومی VNVisitor {
    // TYPES
    شمارش VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    شمارش VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ثابت VNUser1InUse user1InUse;
    ثابت VNUser2InUse user2InUse;
    ثابت VNUser3InUse user3InUse;
    ثابت VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* ثابت m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    ثابت OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ثابت OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ثابت OrderDomain* m_hybridp = nullptr;

    منطقی m_inClocked = نادرست;  // Underneath clocked AstActive
    منطقی m_inPre = نادرست;  // Underneath AlwaysPre
    منطقی m_inPost = نادرست;  // Underneath AstAlwaysPost
    std::function<منطقی(ثابت AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    ثابت منطقی m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ثابت V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ثابت V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    تهی iterateLogic(Ast_گره* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_گره::user2_Clear_درخت();
        Ast_گره::user4_Clear_درخت();
        m_forceReadEdgeIgnores.clear();
        اگر (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = جدید OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        اگر (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            برای (AstVarScope* ثابت vscp : m_accessedVscps) {
                ثابت صحیح recorded = vscp->user4();
                ثابت VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        بازگشت m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    تهی visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_درخت_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        ثابت Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        اگر (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_گره::user3_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user3(درست);
            });
            m_readTriggersCombLogic = [](ثابت AstVarScope* vscp) { بازگشت !vscp->user3(); };
        } وگرنه {
            اگر (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](ثابت AstVarScope*) { بازگشت درست; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ثابت varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ثابت AstVar* ثابت varp = nodep->varp();
        اگر (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            برای (AstVarScope* ثابت boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } وگرنه {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    تهی recordRawAccess(AstVarScope* varscp, ثابت VAccess& access, Ast_گره* nodep) {
        اگر (!m_parallel) بازگشت;
        uint8_t recorded = 0;
        اگر (access.isWriteOrRW()) recorded |= VA_WRITE;
        اگر (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        اگر (!varscp->user4Or(recorded)) m_accessedVscps.افزودن_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    تهی accountVarAccess(AstVarScope* varscp, ثابت VAccess& access, Ast_گره* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ثابت منطقی prevGen = varscp->user2() & VU_GEN;
        ثابت منطقی prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ثابت منطقی gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        منطقی con = نادرست;
        اگر (!prevCon && access.isReadOrRW()) {
            con = درست;
            اگر (prevGen && !m_inClocked) {
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
                con = نادرست;
            }
            اگر (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                اگر (m_forceReadEdgeIgnores.شمار(varscp) || m_cgRefBoundps) con = نادرست;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        اگر (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            اگر (m_inPost) {
                اگر (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* ثابت varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* ثابت postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } وگرنه اگر (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ثابت varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* ثابت postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } وگرنه اگر (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* ثابت ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* ثابت varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } وگرنه {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* ثابت ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* ثابت varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        اگر (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            اگر (m_inPost) {
                // Combinational logic
                اگر (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ثابت varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } وگرنه اگر (!m_inClocked) {  // Combinational logic
                اگر (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* ثابت varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } وگرنه اگر (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* ثابت preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } وگرنه {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* ثابت preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* ثابت postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    تهی visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ثابت funcp = nodep->funcp();
        اگر (!funcp->isCovergroupSample()) بازگشت;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ثابت AstVarScope* instp = nullptr;
        اگر (ثابت AstVarRef* ثابت fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    تهی visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    تهی visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    تهی visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    تهی visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    تهی visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = درست;
        iterateLogic(nodep);
    }
    تهی visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = درست;
        iterateLogic(nodep);
    }
    تهی visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    تهی visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    تهی visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    تهی visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    تهی visit(AstVar*) override {}
    تهی visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    تهی visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    تهی visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    تهی visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    تهی visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(ثابت std::vector<V3Sched::LogicByScope*>& coll,
                      ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ثابت V3Sched::CovergroupRefBindings& cgRefBindings, منطقی parallel)
        : m_graphp{جدید OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        برای (ثابت V3Sched::LogicByScope* ثابت lbsp : coll) {
            برای (ثابت auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = پیشفرض;

عمومی:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ایستا std::unique_ptr<OrderGraph> apply(ثابت std::vector<V3Sched::LogicByScope*>& coll,
                                             ثابت V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                             منطقی parallel) {
        بازگشت std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(ثابت V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ثابت std::vector<V3Sched::LogicByScope*>& coll,  //
                         ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         منطقی parallel) {
    بازگشت OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
