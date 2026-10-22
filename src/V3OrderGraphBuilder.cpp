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

वर्ग क्रमः_User final {
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
    std::array<क्रमः_Var_Vertex*, static_cast<आकार_t>(VarVertexType::POST) + 1> m_vertexps;

सार्वजनिक:
    // METHODS
    क्रमः_Var_Vertex* getVarVertex(क्रमः_Graph* graphp, AstVarScope* varscp, VarVertexType type) {
        ध्रुव अचिह्नित idx = static_cast<अचिह्नित>(type);
        क्रमः_Var_Vertex* vertexp = m_vertexps[idx];
        यदि (!vertexp) {
            विकल्प (type) {
            पक्ष VarVertexType::STD: vertexp = नूतन क्रमः_Var_Std_Vertex{graphp, varscp}; भङ्ग;
            पक्ष VarVertexType::PRE: vertexp = नूतन क्रमः_Var_Pre_Vertex{graphp, varscp}; भङ्ग;
            पक्ष VarVertexType::PORD: vertexp = नूतन क्रमः_Var_Pord_Vertex{graphp, varscp}; भङ्ग;
            पक्ष VarVertexType::POST: vertexp = नूतन क्रमः_Var_Post_Vertex{graphp, varscp}; भङ्ग;
            }
            m_vertexps[idx] = vertexp;
        }
        प्रत्यागम vertexp;
    }

    // CONSTRUCTORS
    क्रमः_User() { m_vertexps.fill(शून्यसूचकः); }
    ~क्रमः_User() = स्वभाव;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

वर्ग क्रमः_Graph_Builder final : सार्वजनिक VNVisitor {
    // TYPES
    गणना VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    गणना VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    उपयुज्य VarVertexType = क्रमः_User::VarVertexType;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
    ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
    ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
    ध्रुव VNUser4_अन्तर्_Use user4_अन्तर्_Use;
    AstUser1Allocator<AstVarScope, क्रमः_User> m_क्रमः_User;

    // STATE
    क्रमः_Graph* ध्रुव m_graphp;  // The ordering graph built by this visitor
    क्रमः_Logic_Vertex* m_logicVxp = शून्यसूचकः;  // Current logic block being analyzed
    std::सदिशम्<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = शून्यसूचकः;
    // Domain of the triggers of the current AstActive being processed
    ध्रुव क्रमः_Domain* m_सक्रिय_Domainp = शून्यसूचकः;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ध्रुव क्रमः_Domain* m_domainp = शून्यसूचकः;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ध्रुव क्रमः_Domain* m_hybridp = शून्यसूचकः;

    तार्किक m_अन्तर्_Clocked = असत्य;  // Underneath clocked AstActive
    तार्किक m_अन्तर्_Pre = असत्य;  // Underneath AlwaysPre
    तार्किक m_अन्तर्_Post = असत्य;  // Underneath AstAlwaysPost
    std::function<तार्किक(ध्रुव AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_समुच्चयः m_forceReadEdgeIgnores;
    ध्रुव तार्किक m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ध्रुव V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ध्रुव V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = शून्यसूचकः;

    // METHODS

    शून्य iterateLogic(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_ग्रन्थिः::user2_Clear_वृक्षः();
        Ast_ग्रन्थिः::user4_Clear_वृक्षः();
        m_forceReadEdgeIgnores.clear();
        यदि (!m_अन्तर्_Clocked)
            V3Sched::util::collectForceReadEdgeIgnores(nodep, m_forceReadEdgeIgnores);
        // Create LogicVertex for this logic node
        m_logicVxp = नूतन क्रमः_Logic_Vertex{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        यदि (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            प्रति (AstVarScope* ध्रुव vscp : m_accessedVscps) {
                ध्रुव पूर्णाङ्क recorded = vscp->user4();
                ध्रुव VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->सङ्कलय_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = शून्यसूचकः;
        m_forceReadEdgeIgnores.clear();
    }

    क्रमः_Var_Vertex* getVarVertex(AstVarScope* varscp, VarVertexType type) {
        प्रत्यागम m_क्रमः_User(varscp).getVarVertex(m_graphp, varscp, type);
    }

    // VISITORS
    शून्य दर्शय(Ast_सक्रिय* nodep) override {
        UASSERT_OBJ(!nodep->sen_वृक्षः_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_अन्तर्_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_अन्तर्_Clocked);

        ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep = nodep->sentreep();

        m_अन्तर्_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        यदि (senTreep->hasHybrid()) {
            m_hybridp = m_सक्रिय_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_ग्रन्थिः::user3_Clear_वृक्षः();
            senTreep->foreach([](ध्रुव AstVarRef* refp) {  //
                refp->varScopep()->user3(सत्य);
            });
            m_readTriggersCombLogic = [](ध्रुव AstVarScope* vscp) { प्रत्यागम !vscp->user3(); };
        } अन्यथा {
            यदि (!senTreep->hasCombo()) m_domainp = m_सक्रिय_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ध्रुव AstVarScope*) { प्रत्यागम सत्य; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* ध्रुव varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        यदि (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            प्रति (AstVarScope* ध्रुव boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } अन्यथा {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    शून्य लेखः_Raw_Access(AstVarScope* varscp, ध्रुव VAccess& access, Ast_ग्रन्थिः* nodep) {
        यदि (!m_parallel) प्रत्यागम;
        uint8_t recorded = 0;
        यदि (access.isWriteOrRW()) recorded |= VA_WRITE;
        यदि (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        यदि (!varscp->user4Or(recorded)) m_accessedVscps.प्रक्षिप_पृष्ठभागः(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    शून्य accountVarAccess(AstVarScope* varscp, ध्रुव VAccess& access, Ast_ग्रन्थिः* nodep) {
        // Variable reference in logic. Add data dependency.
        लेखः_Raw_Access(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ध्रुव तार्किक पूर्व_Gen = varscp->user2() & VU_GEN;
        ध्रुव तार्किक पूर्व_Con = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ध्रुव तार्किक gen = !पूर्व_Gen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        तार्किक con = असत्य;
        यदि (!पूर्व_Con && access.isReadOrRW()) {
            con = सत्य;
            यदि (पूर्व_Gen && !m_अन्तर्_Clocked) {
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
            यदि (!m_अन्तर्_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                यदि (m_forceReadEdgeIgnores.गणनम्(varscp) || m_cgRefBoundps) con = असत्य;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        यदि (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            यदि (m_अन्तर्_Post) {
                यदि (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    क्रमः_Var_Vertex* ध्रुव varVxp = getVarVertex(varscp, VarVertexType::STD);
                    m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, varVxp, WEIGHT_प्रसामान्यम्);
                }
                क्रमः_Var_Vertex* ध्रुव postVxp = getVarVertex(varscp, VarVertexType::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->सङ्कलय_Hard_Edge(postVxp, m_logicVxp, WEIGHT_POST);
            } अन्यथा यदि (!m_अन्तर्_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                क्रमः_Var_Vertex* ध्रुव varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, varVxp, WEIGHT_प्रसामान्यम्);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                क्रमः_Var_Vertex* ध्रुव postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->सङ्कलय_Hard_Edge(postVxp, m_logicVxp, WEIGHT_POST);
            } अन्यथा यदि (m_अन्तर्_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                क्रमः_Var_Vertex* ध्रुव ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, ordVxp, WEIGHT_प्रसामान्यम्);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                क्रमः_Var_Vertex* ध्रुव varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, varVxp, WEIGHT_प्रसामान्यम्);
            } अन्यथा {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                क्रमः_Var_Vertex* ध्रुव ordVxp = getVarVertex(varscp, VarVertexType::PORD);
                m_graphp->सङ्कलय_Hard_Edge(ordVxp, m_logicVxp, WEIGHT_प्रसामान्यम्);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                क्रमः_Var_Vertex* ध्रुव varVxp = getVarVertex(varscp, VarVertexType::STD);
                m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, varVxp, WEIGHT_प्रसामान्यम्);
            }
        }

        // Variable is consumed
        यदि (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            यदि (m_अन्तर्_Post) {
                // Combinational logic
                यदि (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    क्रमः_Var_Vertex* ध्रुव varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->सङ्कलय_Hard_Edge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } अन्यथा यदि (!m_अन्तर्_Clocked) {  // Combinational logic
                यदि (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    क्रमः_Var_Vertex* ध्रुव varVxp = getVarVertex(varscp, VarVertexType::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->सङ्कलय_Hard_Edge(varVxp, m_logicVxp, WEIGHT_MEDIUM);
                }
            } अन्यथा यदि (m_अन्तर्_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                क्रमः_Var_Vertex* ध्रुव preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->सङ्कलय_Soft_Edge(preVxp, m_logicVxp, WEIGHT_PRE);
            } अन्यथा {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                क्रमः_Var_Vertex* ध्रुव preVxp = getVarVertex(varscp, VarVertexType::PRE);
                m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, preVxp, WEIGHT_प्रसामान्यम्);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                क्रमः_Var_Vertex* ध्रुव postVxp = getVarVertex(varscp, VarVertexType::POST);
                m_graphp->सङ्कलय_Hard_Edge(m_logicVxp, postVxp, WEIGHT_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    शून्य दर्शय(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ध्रुव funcp = nodep->funcp();
        यदि (!funcp->isCovergroupSample()) प्रत्यागम;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ध्रुव AstVarScope* instp = शून्यसूचकः;
        यदि (ध्रुव AstVarRef* ध्रुव fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    शून्य दर्शय(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    शून्य दर्शय(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    शून्य दर्शय(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_अन्तर्_Pre, nodep, "Should not nest");
        VL_RESTORER(m_अन्तर्_Pre);
        m_अन्तर्_Pre = सत्य;
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_अन्तर्_Post, nodep, "Should not nest");
        VL_RESTORER(m_अन्तर्_Post);
        m_अन्तर्_Post = सत्य;
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    शून्य दर्शय(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    शून्य दर्शय(AstVar*) override {}
    शून्य दर्शय(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य दर्शय(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य दर्शय(Ast_Type_सारणी* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य दर्शय(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    शून्य दर्शय(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    क्रमः_Graph_Builder(ध्रुव std::सदिशम्<V3Sched::LogicByScope*>& coll,
                      ध्रुव V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ध्रुव V3Sched::CovergroupRefBindings& cgRefBindings, तार्किक parallel)
        : m_graphp{नूतन क्रमः_Graph{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        प्रति (ध्रुव V3Sched::LogicByScope* ध्रुव lbsp : coll) {
            प्रति (ध्रुव स्वचालित& प्रविष्टिः : *lbsp) {
                m_scopep = प्रविष्टिः.m_scopep;
                m_सक्रिय_Domainp = m_graphp->domainp(प्रविष्टिः.m_triggers);
                iterate(प्रविष्टिः.m_activep);
                m_scopep = शून्यसूचकः;
                m_सक्रिय_Domainp = शून्यसूचकः;
            }
        }
    }
    ~क्रमः_Graph_Builder() override = स्वभाव;

सार्वजनिक:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    स्थिर std::unique_ptr<क्रमः_Graph> apply(ध्रुव std::सदिशम्<V3Sched::LogicByScope*>& coll,
                                             ध्रुव V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ध्रुव V3Sched::CovergroupRefBindings& cgRefBindings,
                                             तार्किक parallel) {
        प्रत्यागम std::unique_ptr<क्रमः_Graph>{
            क्रमः_Graph_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<क्रमः_Graph>
V3_क्रमः::build_क्रमः_Graph(ध्रुव V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ध्रुव std::सदिशम्<V3Sched::LogicByScope*>& coll,  //
                         ध्रुव V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         तार्किक parallel) {
    प्रत्यागम क्रमः_Graph_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
