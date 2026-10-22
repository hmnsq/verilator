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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3OrderGraph.h"
#అన్తర్భావ "V3OrderInternal.h"
#అన్తర్భావ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

తరగతి క్రమం_User final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
బహిరంగ:
    // TYPES
    గణన తరగతి Var_శీర్షం_Type : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

అంతరంగ:
    // Vertex of each type (if non-nullptr)
    std::array<క్రమం_Var_శీర్షం*, static_cast<కొలత_t>(Var_శీర్షం_Type::POST) + 1> m_vertexps;

బహిరంగ:
    // METHODS
    క్రమం_Var_శీర్షం* get_Var_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstVarScope* varscp, Var_శీర్షం_Type type) {
        స్థిరరాశి గుర్తులేని idx = static_cast<గుర్తులేని>(type);
        క్రమం_Var_శీర్షం* vertexp = m_vertexps[idx];
        ఒకవేళ (!vertexp) {
            ఎంపిక (type) {
            స్థితి Var_శీర్షం_Type::STD: vertexp = కొత్త క్రమం_Var_Std_శీర్షం{graphp, varscp}; ఆపు;
            స్థితి Var_శీర్షం_Type::PRE: vertexp = కొత్త క్రమం_Var_Pre_శీర్షం{graphp, varscp}; ఆపు;
            స్థితి Var_శీర్షం_Type::PORD: vertexp = కొత్త క్రమం_Var_Pord_శీర్షం{graphp, varscp}; ఆపు;
            స్థితి Var_శీర్షం_Type::POST: vertexp = కొత్త క్రమం_Var_Post_శీర్షం{graphp, varscp}; ఆపు;
            }
            m_vertexps[idx] = vertexp;
        }
        తిరిగి vertexp;
    }

    // CONSTRUCTORS
    క్రమం_User() { m_vertexps.fill(శూన్యసూచిక); }
    ~క్రమం_User() = అప్రమేయ;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

తరగతి క్రమం_లేఖాచిత్రం_Builder final : బహిరంగ VNVisitor {
    // TYPES
    గణన VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    గణన VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    ఉపయోగించి Var_శీర్షం_Type = క్రమం_User::Var_శీర్షం_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
    స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
    స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
    స్థిరరాశి VNUser4_లోపల_Use user4_లోపల_Use;
    AstUser1Allocator<AstVarScope, క్రమం_User> m_క్రమం_User;

    // STATE
    క్రమం_లేఖాచిత్రం* స్థిరరాశి m_graphp;  // The ordering graph built by this visitor
    క్రమం_Logic_శీర్షం* m_logicVxp = శూన్యసూచిక;  // Current logic block being analyzed
    std::సదిశ<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = శూన్యసూచిక;
    // Domain of the triggers of the current AstActive being processed
    స్థిరరాశి క్రమం_Domain* m_చురుకైన_Domainp = శూన్యసూచిక;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    స్థిరరాశి క్రమం_Domain* m_domainp = శూన్యసూచిక;
    // Sensitivity list for hybrid logic, nullptr for everything else
    స్థిరరాశి క్రమం_Domain* m_hybridp = శూన్యసూచిక;

    తార్కిక m_లోపల_Clocked = అబద్ధం;  // Underneath clocked AstActive
    తార్కిక m_లోపల_Pre = అబద్ధం;  // Underneath AlwaysPre
    తార్కిక m_లోపల_Post = అబద్ధం;  // Underneath AstAlwaysPost
    std::function<తార్కిక(స్థిరరాశి AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_సమితి m_force_Read_అంచు_Ignores;
    స్థిరరాశి తార్కిక m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    స్థిరరాశి V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    స్థిరరాశి V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = శూన్యసూచిక;

    // METHODS

    శూన్య iterateLogic(Ast_కణుపు* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_కణుపు::user2_Clear_వృక్షం();
        Ast_కణుపు::user4_Clear_వృక్షం();
        m_force_Read_అంచు_Ignores.clear();
        ఒకవేళ (!m_లోపల_Clocked)
            V3Sched::util::collect_Force_Read_అంచు_Ignores(nodep, m_force_Read_అంచు_Ignores);
        // Create LogicVertex for this logic node
        m_logicVxp = కొత్త క్రమం_Logic_శీర్షం{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        ఒకవేళ (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            కొరకు (AstVarScope* స్థిరరాశి vscp : m_accessedVscps) {
                స్థిరరాశి పూర్ణాంకం recorded = vscp->user4();
                స్థిరరాశి VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->కూడు_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = శూన్యసూచిక;
        m_force_Read_అంచు_Ignores.clear();
    }

    క్రమం_Var_శీర్షం* get_Var_శీర్షం(AstVarScope* varscp, Var_శీర్షం_Type type) {
        తిరిగి m_క్రమం_User(varscp).get_Var_శీర్షం(m_graphp, varscp, type);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        UASSERT_OBJ(!nodep->sen_వృక్షం_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_లోపల_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_లోపల_Clocked);

        స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి senTreep = nodep->sentreep();

        m_లోపల_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        ఒకవేళ (senTreep->hasHybrid()) {
            m_hybridp = m_చురుకైన_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_కణుపు::user3_Clear_వృక్షం();
            senTreep->foreach([](స్థిరరాశి AstVarRef* refp) {  //
                refp->varScopep()->user3(నిజం);
            });
            m_readTriggersCombLogic = [](స్థిరరాశి AstVarScope* vscp) { తిరిగి !vscp->user3(); };
        } లేకపోతే {
            ఒకవేళ (!senTreep->hasCombo()) m_domainp = m_చురుకైన_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](స్థిరరాశి AstVarScope*) { తిరిగి నిజం; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        // As we explicitly not visit (see ignored nodes below) any subtree that is not relevant
        // for ordering, we should be able to assert this:
        UASSERT_OBJ(m_scopep, nodep, "AstVarRef not under scope");
        UASSERT_OBJ(m_logicVxp, nodep, "AstVarRef not under logic");
        AstVarScope* స్థిరరాశి varscp = nodep->varScopep();
        UASSERT_OBJ(varscp, nodep, "Var didn't get varscoped in V3Scope.cpp");
        // Reading a covergroup 'ref' formal reads whatever it was bound to at construction.
        // The formal itself is a pointer member fixed at construction, so it is not itself
        // interesting to ordering.
        స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            కొరకు (AstVarScope* స్థిరరాశి boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } లేకపోతే {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    శూన్య రికార్డు_Raw_Access(AstVarScope* varscp, స్థిరరాశి VAccess& access, Ast_కణుపు* nodep) {
        ఒకవేళ (!m_parallel) తిరిగి;
        uint8_t recorded = 0;
        ఒకవేళ (access.isWriteOrRW()) recorded |= VA_WRITE;
        ఒకవేళ (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        ఒకవేళ (!varscp->user4Or(recorded)) m_accessedVscps.నెట్టు_వెనుక(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    శూన్య accountVarAccess(AstVarScope* varscp, స్థిరరాశి VAccess& access, Ast_కణుపు* nodep) {
        // Variable reference in logic. Add data dependency.
        రికార్డు_Raw_Access(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        స్థిరరాశి తార్కిక prevGen = varscp->user2() & VU_GEN;
        స్థిరరాశి తార్కిక prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        స్థిరరాశి తార్కిక gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        తార్కిక con = అబద్ధం;
        ఒకవేళ (!prevCon && access.isReadOrRW()) {
            con = నిజం;
            ఒకవేళ (prevGen && !m_లోపల_Clocked) {
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
                con = అబద్ధం;
            }
            ఒకవేళ (!m_లోపల_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                ఒకవేళ (m_force_Read_అంచు_Ignores.లెక్క(varscp) || m_cgRefBoundps) con = అబద్ధం;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        ఒకవేళ (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            ఒకవేళ (m_లోపల_Post) {
                ఒకవేళ (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    క్రమం_Var_శీర్షం* స్థిరరాశి varVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::STD);
                    m_graphp->కూడు_Hard_అంచు(m_logicVxp, varVxp, బరువు_సాధారణ);
                }
                క్రమం_Var_శీర్షం* స్థిరరాశి postVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->కూడు_Hard_అంచు(postVxp, m_logicVxp, బరువు_POST);
            } లేకపోతే ఒకవేళ (!m_లోపల_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి varVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::STD);
                m_graphp->కూడు_Hard_అంచు(m_logicVxp, varVxp, బరువు_సాధారణ);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి postVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::POST);
                m_graphp->కూడు_Hard_అంచు(postVxp, m_logicVxp, బరువు_POST);
            } లేకపోతే ఒకవేళ (m_లోపల_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి ordVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::PORD);
                m_graphp->కూడు_Hard_అంచు(m_logicVxp, ordVxp, బరువు_సాధారణ);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి varVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::STD);
                m_graphp->కూడు_Hard_అంచు(m_logicVxp, varVxp, బరువు_సాధారణ);
            } లేకపోతే {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి ordVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::PORD);
                m_graphp->కూడు_Hard_అంచు(ordVxp, m_logicVxp, బరువు_సాధారణ);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి varVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::STD);
                m_graphp->కూడు_Hard_అంచు(m_logicVxp, varVxp, బరువు_సాధారణ);
            }
        }

        // Variable is consumed
        ఒకవేళ (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            ఒకవేళ (m_లోపల_Post) {
                // Combinational logic
                ఒకవేళ (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    క్రమం_Var_శీర్షం* స్థిరరాశి varVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->కూడు_Hard_అంచు(varVxp, m_logicVxp, బరువు_MEDIUM);
                }
            } లేకపోతే ఒకవేళ (!m_లోపల_Clocked) {  // Combinational logic
                ఒకవేళ (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    క్రమం_Var_శీర్షం* స్థిరరాశి varVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->కూడు_Hard_అంచు(varVxp, m_logicVxp, బరువు_MEDIUM);
                }
            } లేకపోతే ఒకవేళ (m_లోపల_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                క్రమం_Var_శీర్షం* స్థిరరాశి preVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::PRE);
                m_graphp->కూడు_Soft_అంచు(preVxp, m_logicVxp, బరువు_PRE);
            } లేకపోతే {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                క్రమం_Var_శీర్షం* స్థిరరాశి preVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::PRE);
                m_graphp->కూడు_Hard_అంచు(m_logicVxp, preVxp, బరువు_సాధారణ);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                క్రమం_Var_శీర్షం* స్థిరరాశి postVxp = get_Var_శీర్షం(varscp, Var_శీర్షం_Type::POST);
                m_graphp->కూడు_Hard_అంచు(m_logicVxp, postVxp, బరువు_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    శూన్య సందర్శించు(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        ఒకవేళ (!funcp->isCovergroupSample()) తిరిగి;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        స్థిరరాశి AstVarScope* instp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    శూన్య సందర్శించు(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    శూన్య సందర్శించు(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    శూన్య సందర్శించు(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_లోపల_Pre, nodep, "Should not nest");
        VL_RESTORER(m_లోపల_Pre);
        m_లోపల_Pre = నిజం;
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_లోపల_Post, nodep, "Should not nest");
        VL_RESTORER(m_లోపల_Post);
        m_లోపల_Post = నిజం;
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    శూన్య సందర్శించు(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    శూన్య సందర్శించు(AstVar*) override {}
    శూన్య సందర్శించు(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    శూన్య సందర్శించు(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    శూన్య సందర్శించు(Ast_Type_పట్టిక* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    శూన్య సందర్శించు(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    శూన్య సందర్శించు(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    క్రమం_లేఖాచిత్రం_Builder(స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& coll,
                      స్థిరరాశి V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      స్థిరరాశి V3Sched::CovergroupRefBindings& cgRefBindings, తార్కిక parallel)
        : m_graphp{కొత్త క్రమం_లేఖాచిత్రం{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        కొరకు (స్థిరరాశి V3Sched::LogicByScope* స్థిరరాశి lbsp : coll) {
            కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : *lbsp) {
                m_scopep = నమోదు_2.m_scopep;
                m_చురుకైన_Domainp = m_graphp->domainp(నమోదు_2.m_triggers);
                iterate(నమోదు_2.m_activep);
                m_scopep = శూన్యసూచిక;
                m_చురుకైన_Domainp = శూన్యసూచిక;
            }
        }
    }
    ~క్రమం_లేఖాచిత్రం_Builder() override = అప్రమేయ;

బహిరంగ:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    స్థిర std::unique_ptr<క్రమం_లేఖాచిత్రం> apply(స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& coll,
                                             స్థిరరాశి V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             స్థిరరాశి V3Sched::CovergroupRefBindings& cgRefBindings,
                                             తార్కిక parallel) {
        తిరిగి std::unique_ptr<క్రమం_లేఖాచిత్రం>{
            క్రమం_లేఖాచిత్రం_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<క్రమం_లేఖాచిత్రం>
V3_క్రమం::build_క్రమం_లేఖాచిత్రం(స్థిరరాశి V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& coll,  //
                         స్థిరరాశి V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         తార్కిక parallel) {
    తిరిగి క్రమం_లేఖాచిత్రం_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
