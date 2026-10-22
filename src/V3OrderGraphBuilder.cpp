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

جماعت ترتیب_User final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
عوامی:
    // TYPES
    شمار جماعت Var_راس_Type : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

نجی:
    // Vertex of each type (if non-nullptr)
    std::array<ترتیب_Var_راس*, static_cast<سائز_t>(Var_راس_Type::POST) + 1> m_vertexps;

عوامی:
    // METHODS
    ترتیب_Var_راس* get_Var_راس(ترتیب_گراف* graphp, AstVarScope* varscp, Var_راس_Type type) {
        ثابت غیرعلامتی idx = static_cast<غیرعلامتی>(type);
        ترتیب_Var_راس* vertexp = m_vertexps[idx];
        اگر (!vertexp) {
            انتخاب (type) {
            حالت Var_راس_Type::STD: vertexp = نیا ترتیب_Var_Std_راس{graphp, varscp}; توڑو;
            حالت Var_راس_Type::PRE: vertexp = نیا ترتیب_Var_Pre_راس{graphp, varscp}; توڑو;
            حالت Var_راس_Type::PORD: vertexp = نیا ترتیب_Var_Pord_راس{graphp, varscp}; توڑو;
            حالت Var_راس_Type::POST: vertexp = نیا ترتیب_Var_Post_راس{graphp, varscp}; توڑو;
            }
            m_vertexps[idx] = vertexp;
        }
        واپس vertexp;
    }

    // CONSTRUCTORS
    ترتیب_User() { m_vertexps.fill(خالی_اشاریہ); }
    ~ترتیب_User() = طےشدہ;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

جماعت ترتیب_گراف_Builder final : عوامی VNVisitor {
    // TYPES
    شمار VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    شمار VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    استعمال Var_راس_Type = ترتیب_User::Var_راس_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ثابت VNUser1_اندر_Use user1_اندر_Use;
    ثابت VNUser2_اندر_Use user2_اندر_Use;
    ثابت VNUser3_اندر_Use user3_اندر_Use;
    ثابت VNUser4_اندر_Use user4_اندر_Use;
    AstUser1Allocator<AstVarScope, ترتیب_User> m_ترتیب_User;

    // STATE
    ترتیب_گراف* ثابت m_graphp;  // The ordering graph built by this visitor
    ترتیب_Logic_راس* m_logicVxp = خالی_اشاریہ;  // Current logic block being analyzed
    std::سمتیہ<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = خالی_اشاریہ;
    // Domain of the triggers of the current AstActive being processed
    ثابت ترتیب_Domain* m_فعال_Domainp = خالی_اشاریہ;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ثابت ترتیب_Domain* m_domainp = خالی_اشاریہ;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ثابت ترتیب_Domain* m_hybridp = خالی_اشاریہ;

    منطقی m_اندر_Clocked = جھوٹ;  // Underneath clocked AstActive
    منطقی m_اندر_Pre = جھوٹ;  // Underneath AlwaysPre
    منطقی m_اندر_Post = جھوٹ;  // Underneath AstAlwaysPost
    std::function<منطقی(ثابت AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_سیٹ m_force_Read_کنارہ_Ignores;
    ثابت منطقی m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ثابت V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ثابت V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = خالی_اشاریہ;

    // METHODS

    خالی iterateLogic(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_گرہ::user2_Clear_درخت();
        Ast_گرہ::user4_Clear_درخت();
        m_force_Read_کنارہ_Ignores.clear();
        اگر (!m_اندر_Clocked)
            V3Sched::util::collect_Force_Read_کنارہ_Ignores(nodep, m_force_Read_کنارہ_Ignores);
        // Create LogicVertex for this logic node
        m_logicVxp = نیا ترتیب_Logic_راس{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        اگر (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            برائے (AstVarScope* ثابت vscp : m_accessedVscps) {
                ثابت صحیح recorded = vscp->user4();
                ثابت VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->جوڑو_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = خالی_اشاریہ;
        m_force_Read_کنارہ_Ignores.clear();
    }

    ترتیب_Var_راس* get_Var_راس(AstVarScope* varscp, Var_راس_Type type) {
        واپس m_ترتیب_User(varscp).get_Var_راس(m_graphp, varscp, type);
    }

    // VISITORS
    خالی دورہ(Ast_فعال* nodep) override {
        UASSERT_OBJ(!nodep->sen_درخت_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_اندر_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_اندر_Clocked);

        ثابت Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();

        m_اندر_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        اگر (senTreep->hasHybrid()) {
            m_hybridp = m_فعال_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_گرہ::user3_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user3(سچ);
            });
            m_readTriggersCombLogic = [](ثابت AstVarScope* vscp) { واپس !vscp->user3(); };
        } ورنہ {
            اگر (!senTreep->hasCombo()) m_domainp = m_فعال_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ثابت AstVarScope*) { واپس سچ; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
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
            برائے (AstVarScope* ثابت boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } ورنہ {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    خالی ریکارڈ_Raw_Access(AstVarScope* varscp, ثابت VAccess& access, Ast_گرہ* nodep) {
        اگر (!m_parallel) واپس;
        uint8_t recorded = 0;
        اگر (access.isWriteOrRW()) recorded |= VA_WRITE;
        اگر (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        اگر (!varscp->user4Or(recorded)) m_accessedVscps.ڈالو_عقب(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    خالی accountVarAccess(AstVarScope* varscp, ثابت VAccess& access, Ast_گرہ* nodep) {
        // Variable reference in logic. Add data dependency.
        ریکارڈ_Raw_Access(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ثابت منطقی پچھلا_Gen = varscp->user2() & VU_GEN;
        ثابت منطقی پچھلا_Con = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ثابت منطقی gen = !پچھلا_Gen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        منطقی con = جھوٹ;
        اگر (!پچھلا_Con && access.isReadOrRW()) {
            con = سچ;
            اگر (پچھلا_Gen && !m_اندر_Clocked) {
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
                con = جھوٹ;
            }
            اگر (!m_اندر_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                اگر (m_force_Read_کنارہ_Ignores.گنتی(varscp) || m_cgRefBoundps) con = جھوٹ;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        اگر (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            اگر (m_اندر_Post) {
                اگر (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, varVxp, وزن_معمول);
                }
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->جوڑو_Hard_کنارہ(postVxp, m_logicVxp, وزن_POST);
            } ورنہ اگر (!m_اندر_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, varVxp, وزن_معمول);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                m_graphp->جوڑو_Hard_کنارہ(postVxp, m_logicVxp, وزن_POST);
            } ورنہ اگر (m_اندر_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                ترتیب_Var_راس* ثابت ordVxp = get_Var_راس(varscp, Var_راس_Type::PORD);
                m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, ordVxp, وزن_معمول);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, varVxp, وزن_معمول);
            } ورنہ {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                ترتیب_Var_راس* ثابت ordVxp = get_Var_راس(varscp, Var_راس_Type::PORD);
                m_graphp->جوڑو_Hard_کنارہ(ordVxp, m_logicVxp, وزن_معمول);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, varVxp, وزن_معمول);
            }
        }

        // Variable is consumed
        اگر (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            اگر (m_اندر_Post) {
                // Combinational logic
                اگر (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جوڑو_Hard_کنارہ(varVxp, m_logicVxp, وزن_MEDIUM);
                }
            } ورنہ اگر (!m_اندر_Clocked) {  // Combinational logic
                اگر (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جوڑو_Hard_کنارہ(varVxp, m_logicVxp, وزن_MEDIUM);
                }
            } ورنہ اگر (m_اندر_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                ترتیب_Var_راس* ثابت preVxp = get_Var_راس(varscp, Var_راس_Type::PRE);
                m_graphp->جوڑو_Soft_کنارہ(preVxp, m_logicVxp, وزن_PRE);
            } ورنہ {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                ترتیب_Var_راس* ثابت preVxp = get_Var_راس(varscp, Var_راس_Type::PRE);
                m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, preVxp, وزن_معمول);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                m_graphp->جوڑو_Hard_کنارہ(m_logicVxp, postVxp, وزن_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    خالی دورہ(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ثابت funcp = nodep->funcp();
        اگر (!funcp->isCovergroupSample()) واپس;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ثابت AstVarScope* instp = خالی_اشاریہ;
        اگر (ثابت AstVarRef* ثابت fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    خالی دورہ(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    خالی دورہ(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    خالی دورہ(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    خالی دورہ(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    خالی دورہ(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_اندر_Pre, nodep, "Should not nest");
        VL_RESTORER(m_اندر_Pre);
        m_اندر_Pre = سچ;
        iterateLogic(nodep);
    }
    خالی دورہ(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_اندر_Post, nodep, "Should not nest");
        VL_RESTORER(m_اندر_Post);
        m_اندر_Post = سچ;
        iterateLogic(nodep);
    }
    خالی دورہ(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    خالی دورہ(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    خالی دورہ(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    خالی دورہ(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    خالی دورہ(AstVar*) override {}
    خالی دورہ(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    خالی دورہ(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    خالی دورہ(Ast_Type_جدول* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    خالی دورہ(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    خالی دورہ(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ترتیب_گراف_Builder(ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,
                      ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ثابت V3Sched::CovergroupRefBindings& cgRefBindings, منطقی parallel)
        : m_graphp{نیا ترتیب_گراف{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        برائے (ثابت V3Sched::LogicByScope* ثابت lbsp : coll) {
            برائے (ثابت خودکار& اندراج_2 : *lbsp) {
                m_scopep = اندراج_2.m_scopep;
                m_فعال_Domainp = m_graphp->domainp(اندراج_2.m_triggers);
                iterate(اندراج_2.m_activep);
                m_scopep = خالی_اشاریہ;
                m_فعال_Domainp = خالی_اشاریہ;
            }
        }
    }
    ~ترتیب_گراف_Builder() override = طےشدہ;

عوامی:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ساکن std::unique_ptr<ترتیب_گراف> apply(ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,
                                             ثابت V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                             منطقی parallel) {
        واپس std::unique_ptr<ترتیب_گراف>{
            ترتیب_گراف_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<ترتیب_گراف>
V3_ترتیب::build_ترتیب_گراف(ثابت V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,  //
                         ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         منطقی parallel) {
    واپس ترتیب_گراف_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
