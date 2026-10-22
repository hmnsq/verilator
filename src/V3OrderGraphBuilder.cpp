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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3OrderGraph.h"
#അന്തര്ഭാവ "V3OrderInternal.h"
#അന്തര്ഭാവ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

വർഗ്ഗം OrderUser final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
പൊതു:
    // TYPES
    എണ്ണൽ വർഗ്ഗം VarVertexType : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

സ്വകാര്യ:
    // Vertex of each type (if non-nullptr)
    std::array<OrderVarVertex*, static_cast<size_t>(VarVertexType::POST) + 1> m_vertexps;

പൊതു:
    // METHODS
    OrderVarVertex* getVarVertex(OrderGraph* graphp, AstVarScope* varscp, VarVertexType type) {
        സ്ഥിരം unsigned idx = static_cast<unsigned>(type);
        OrderVarVertex* vertexp = m_vertexps[idx];
        എങ്കിൽ (!vertexp) {
            തിരഞ്ഞെടുപ്പ് (type) {
            അവസ്ഥ VarVertexType::STD: vertexp = പുതിയ OrderVarStdVertex{graphp, varscp}; നിർത്തുക;
            അവസ്ഥ VarVertexType::PRE: vertexp = പുതിയ OrderVarPreVertex{graphp, varscp}; നിർത്തുക;
            അവസ്ഥ VarVertexType::PORD: vertexp = പുതിയ OrderVarPordVertex{graphp, varscp}; നിർത്തുക;
            അവസ്ഥ VarVertexType::POST: vertexp = പുതിയ OrderVarPostVertex{graphp, varscp}; നിർത്തുക;
            }
            m_vertexps[idx] = vertexp;
        }
        തിരികെ vertexp;
    }

    // CONSTRUCTORS
    OrderUser() { m_vertexps.fill(nullptr); }
    ~OrderUser() = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

വർഗ്ഗം OrderGraphBuilder final : പൊതു VNVisitor {
    // TYPES
    എണ്ണൽ VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    എണ്ണൽ VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    using VarVertexType = OrderUser::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    സ്ഥിരം VNUser1InUse user1InUse;
    സ്ഥിരം VNUser2InUse user2InUse;
    സ്ഥിരം VNUser3InUse user3InUse;
    സ്ഥിരം VNUser4InUse user4InUse;
    AstUser1Allocator<AstVarScope, OrderUser> m_orderUser;

    // STATE
    OrderGraph* സ്ഥിരം m_graphp;  // The ordering graph built by this visitor
    OrderLogicVertex* m_logicVxp = nullptr;  // Current logic block being analyzed
    std::vector<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = nullptr;
    // Domain of the triggers of the current AstActive being processed
    സ്ഥിരം OrderDomain* m_activeDomainp = nullptr;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    സ്ഥിരം OrderDomain* m_domainp = nullptr;
    // Sensitivity list for hybrid logic, nullptr for everything else
    സ്ഥിരം OrderDomain* m_hybridp = nullptr;

    തർക്കം m_inClocked = അസത്യം;  // Underneath clocked AstActive
    തർക്കം m_inPre = അസത്യം;  // Underneath AlwaysPre
    തർക്കം m_inPost = അസത്യം;  // Underneath AstAlwaysPost
    std::function<തർക്കം(സ്ഥിരം AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::VarScopeSet m_forceReadEdgeIgnores;
    സ്ഥിരം തർക്കം m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    സ്ഥിരം V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    സ്ഥിരം V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = nullptr;

    // METHODS

    ശൂന്യം iterateLogic(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_കണ്ണി::user2_Clear_വൃക്ഷം();
        Ast_കണ്ണി::user4_Clear_വൃക്ഷം();
        m_forceReadEdgeIgnores.clear();
        എങ്കിൽ (!m_inClocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = പുതിയ OrderLogicVertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        എങ്കിൽ (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_accessedVscps) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ recorded = vscp->user4();
                സ്ഥിരം VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
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
        തിരികെ m_orderUser(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    ശൂന്യം visit(AstActive* nodep) override {
        UASSERT_OBJ(!nodep->sen_വൃക്ഷം_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_inClocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_inClocked);

        സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = nodep->sentreep();

        m_inClocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        എങ്കിൽ (senTreep->hasHybrid()) {
            m_hybridp = m_activeDomainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_കണ്ണി::user3_Clear_വൃക്ഷം();
            senTreep->foreach([](സ്ഥിരം AstVarRef* refp) {  //
                refp->varScopep()->user3(സത്യം);
            });
            m_readTriggersCombLogic = [](സ്ഥിരം AstVarScope* vscp) { തിരികെ !vscp->user3(); };
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!senTreep->hasCombo()) m_domainp = m_activeDomainp;
            // Always triggers
            m_readTriggersCombLogic = [](സ്ഥിരം AstVarScope*) { തിരികെ സത്യം; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* സ്ഥിരം varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            വേണ്ടി (AstVarScope* സ്ഥിരം boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } അല്ലെങ്കിൽ {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    ശൂന്യം recordRawAccess(AstVarScope* varscp, സ്ഥിരം VAccess& access, Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!m_parallel) തിരികെ;
        uint8_t recorded = 0;
        എങ്കിൽ (access.isWriteOrRW()) recorded |= VA_WRITE;
        എങ്കിൽ (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        എങ്കിൽ (!varscp->user4Or(recorded)) m_accessedVscps.തള്ളുക_back(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    ശൂന്യം accountVarAccess(AstVarScope* varscp, സ്ഥിരം VAccess& access, Ast_കണ്ണി* nodep) {
        // Variable reference in logic. Add data dependency.
        recordRawAccess(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        സ്ഥിരം തർക്കം prevGen = varscp->user2() & VU_GEN;
        സ്ഥിരം തർക്കം prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        സ്ഥിരം തർക്കം gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        തർക്കം con = അസത്യം;
        എങ്കിൽ (!prevCon && access.isReadOrRW()) {
            con = സത്യം;
            എങ്കിൽ (prevGen && !m_inClocked) {
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
                con = അസത്യം;
            }
            എങ്കിൽ (!m_inClocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                എങ്കിൽ (m_forceReadEdgeIgnores.എണ്ണം(varscp) || m_cgRefBoundps) con = അസത്യം;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        എങ്കിൽ (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            എങ്കിൽ (m_inPost) {
                എങ്കിൽ (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    OrderVarVertex* സ്ഥിരം varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                }
                OrderVarVertex* സ്ഥിരം postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } അല്ലെങ്കിൽ എങ്കിൽ (!m_inClocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* സ്ഥിരം varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                OrderVarVertex* സ്ഥിരം postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(postVxp, m_logicVxp, WEIGHT_POST);
            } അല്ലെങ്കിൽ എങ്കിൽ (m_inPre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                OrderVarVertex* സ്ഥിരം ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(m_logicVxp, ordVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                OrderVarVertex* സ്ഥിരം varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            } അല്ലെങ്കിൽ {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                OrderVarVertex* സ്ഥിരം ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->addHardEdge(ordVxp, m_logicVxp, WEIGHT_NORMAL);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                OrderVarVertex* സ്ഥിരം varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->addHardEdge(m_logicVxp, varVxp, WEIGHT_NORMAL);
            }
        }

        // Variable is consumed
        എങ്കിൽ (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            എങ്കിൽ (m_inPost) {
                // Combinational logic
                എങ്കിൽ (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* സ്ഥിരം varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (!m_inClocked) {  // Combinational logic
                എങ്കിൽ (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    OrderVarVertex* സ്ഥിരം varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->addHardEdge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (m_inPre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                OrderVarVertex* സ്ഥിരം preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addSoftEdge(preVxp, m_logicVxp, WEIGHT_PRE);
            } അല്ലെങ്കിൽ {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                OrderVarVertex* സ്ഥിരം preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->addHardEdge(m_logicVxp, preVxp, WEIGHT_NORMAL);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                OrderVarVertex* സ്ഥിരം postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->addHardEdge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    ശൂന്യം visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        എങ്കിൽ (!funcp->isCovergroupSample()) തിരികെ;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        സ്ഥിരം AstVarScope* instp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    ശൂന്യം visit(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    ശൂന്യം visit(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    ശൂന്യം visit(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_inPre, nodep, "Should not nest");
        VL_RESTORER(m_inPre);
        m_inPre = സത്യം;
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_inPost, nodep, "Should not nest");
        VL_RESTORER(m_inPost);
        m_inPost = സത്യം;
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    ശൂന്യം visit(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    ശൂന്യം visit(AstVar*) override {}
    ശൂന്യം visit(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ശൂന്യം visit(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ശൂന്യം visit(AstTypeTable* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ശൂന്യം visit(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    ശൂന്യം visit(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    OrderGraphBuilder(സ്ഥിരം std::vector<V3Sched::LogicByScope*>& coll,
                      സ്ഥിരം V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      സ്ഥിരം V3Sched::CovergroupRefBindings& cgRefBindings, തർക്കം parallel)
        : m_graphp{പുതിയ OrderGraph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        വേണ്ടി (സ്ഥിരം V3Sched::LogicByScope* സ്ഥിരം lbsp : coll) {
            വേണ്ടി (സ്ഥിരം auto& entry : *lbsp) {
                m_scopep = entry.m_scopep;
                m_activeDomainp = m_graphp->domainp(entry.m_triggers);
                iterate(entry.m_activep);
                m_scopep = nullptr;
                m_activeDomainp = nullptr;
            }
        }
    }
    ~OrderGraphBuilder() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    സ്ഥിതം std::unique_ptr<OrderGraph> apply(സ്ഥിരം std::vector<V3Sched::LogicByScope*>& coll,
                                             സ്ഥിരം V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             സ്ഥിരം V3Sched::CovergroupRefBindings& cgRefBindings,
                                             തർക്കം parallel) {
        തിരികെ std::unique_ptr<OrderGraph>{
            OrderGraphBuilder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<OrderGraph>
V3Order::buildOrderGraph(സ്ഥിരം V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         സ്ഥിരം std::vector<V3Sched::LogicByScope*>& coll,  //
                         സ്ഥിരം V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         തർക്കം parallel) {
    തിരികെ OrderGraphBuilder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
