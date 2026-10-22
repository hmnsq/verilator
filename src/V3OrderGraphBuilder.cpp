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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Graph.h"
#ضمن "V3OrderGraph.h"
#ضمن "V3OrderInternal.h"
#ضمن "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Order information stored under each AstNode::user1p()...

صنف ترتیب_User final {
    // Stored in AstVarScope::user1p, a list of all the various vertices
    // that can exist for one given scoped variable
عام:
    // TYPES
    تعداد صنف Var_راس_Type : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

خاص:
    // Vertex of each type (if non-nullptr)
    std::array<ترتیب_Var_راس*, static_cast<ناپ_t>(Var_راس_Type::POST) + 1> m_vertexps;

عام:
    // METHODS
    ترتیب_Var_راس* get_Var_راس(ترتیب_گراف* graphp, AstVarScope* varscp, Var_راس_Type type) {
        ثابت بنانشانی idx = static_cast<بنانشانی>(type);
        ترتیب_Var_راس* vertexp = m_vertexps[idx];
        إذا (!vertexp) {
            بدل (type) {
            حالة Var_راس_Type::STD: vertexp = جديد ترتیب_Var_Std_راس{graphp, varscp}; اكسر;
            حالة Var_راس_Type::PRE: vertexp = جديد ترتیب_Var_Pre_راس{graphp, varscp}; اكسر;
            حالة Var_راس_Type::PORD: vertexp = جديد ترتیب_Var_Pord_راس{graphp, varscp}; اكسر;
            حالة Var_راس_Type::POST: vertexp = جديد ترتیب_Var_Post_راس{graphp, varscp}; اكسر;
            }
            m_vertexps[idx] = vertexp;
        }
        أرجع vertexp;
    }

    // CONSTRUCTORS
    ترتیب_User() { m_vertexps.fill(خالی_سوچک); }
    ~ترتیب_User() = افتراضي;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

صنف ترتیب_گراف_Builder final : عام VNVisitor {
    // TYPES
    تعداد VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    تعداد VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    ورتوں Var_راس_Type = ترتیب_User::Var_راس_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ثابت VNUser1_وچ_Use user1_وچ_Use;
    ثابت VNUser2_وچ_Use user2_وچ_Use;
    ثابت VNUser3_وچ_Use user3_وچ_Use;
    ثابت VNUser4_وچ_Use user4_وچ_Use;
    AstUser1Allocator<AstVarScope, ترتیب_User> m_ترتیب_User;

    // STATE
    ترتیب_گراف* ثابت m_graphp;  // The ordering graph built by this visitor
    ترتیب_Logic_راس* m_logicVxp = خالی_سوچک;  // Current logic block being analyzed
    std::سمتیہ<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = خالی_سوچک;
    // Domain of the triggers of the current AstActive being processed
    ثابت ترتیب_Domain* m_سرگرم_Domainp = خالی_سوچک;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ثابت ترتیب_Domain* m_domainp = خالی_سوچک;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ثابت ترتیب_Domain* m_hybridp = خالی_سوچک;

    منطقي m_وچ_Clocked = خطأ;  // Underneath clocked AstActive
    منطقي m_وچ_Pre = خطأ;  // Underneath AlwaysPre
    منطقي m_وچ_Post = خطأ;  // Underneath AstAlwaysPost
    std::function<منطقي(ثابت AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_سیٹ m_force_Read_کنارا_Ignores;
    ثابت منطقي m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ثابت V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ثابت V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = خالی_سوچک;

    // METHODS

    فراغ iterateLogic(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_گرہ::user2_Clear_درخت();
        Ast_گرہ::user4_Clear_درخت();
        m_force_Read_کنارا_Ignores.clear();
        إذا (!m_وچ_Clocked)
            V3Sched::util::collect_Force_Read_کنارا_Ignores(nodep, m_force_Read_کنارا_Ignores);
        // Create LogicVertex for this logic node
        m_logicVxp = جديد ترتیب_Logic_راس{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        إذا (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            لكل (AstVarScope* ثابت vscp : m_accessedVscps) {
                ثابت صحيح recorded = vscp->user4();
                ثابت VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->جوڑو_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = خالی_سوچک;
        m_force_Read_کنارا_Ignores.clear();
    }

    ترتیب_Var_راس* get_Var_راس(AstVarScope* varscp, Var_راس_Type type) {
        أرجع m_ترتیب_User(varscp).get_Var_راس(m_graphp, varscp, type);
    }

    // VISITORS
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        UASSERT_OBJ(!nodep->sen_درخت_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_وچ_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_وچ_Clocked);

        ثابت Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();

        m_وچ_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        إذا (senTreep->hasHybrid()) {
            m_hybridp = m_سرگرم_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_گرہ::user3_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user3(سچ);
            });
            m_readTriggersCombLogic = [](ثابت AstVarScope* vscp) { أرجع !vscp->user3(); };
        } وإلا {
            إذا (!senTreep->hasCombo()) m_domainp = m_سرگرم_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ثابت AstVarScope*) { أرجع سچ; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
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
        إذا (m_cgRefBoundps && varp->covergroupRefMember()) {
            // Covergroup params are considered const-ref
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "covergroup ref argument is written");
            لكل (AstVarScope* ثابت boundp : *m_cgRefBoundps) {
                accountVarAccess(boundp, VAccess::READ, nodep);
            }
        } وإلا {
            accountVarAccess(varscp, nodep->access(), nodep);
        }
    }

    // Record the raw access for the multi-threaded data hazard fixer
    فراغ ریکارڈ_Raw_Access(AstVarScope* varscp, ثابت VAccess& access, Ast_گرہ* nodep) {
        إذا (!m_parallel) أرجع;
        uint8_t recorded = 0;
        إذا (access.isWriteOrRW()) recorded |= VA_WRITE;
        إذا (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        إذا (!varscp->user4Or(recorded)) m_accessedVscps.ڈالو_پٹھ(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    فراغ accountVarAccess(AstVarScope* varscp, ثابت VAccess& access, Ast_گرہ* nodep) {
        // Variable reference in logic. Add data dependency.
        ریکارڈ_Raw_Access(varscp, access, nodep);

        // Check whether this variable was already generated/consumed in the same logic. We
        // don't want to add extra edges if the logic has many usages of the same variable,
        // so only proceed on first encounter.
        ثابت منطقي prevGen = varscp->user2() & VU_GEN;
        ثابت منطقي prevCon = varscp->user2() & VU_CON;

        // Compute whether the variable is produced (written) here
        ثابت منطقي gen = !prevGen && access.isWriteOrRW() && !varscp->varp()->ignoreSchedWrite();

        // Compute whether the value is consumed (read) here
        منطقي con = خطأ;
        إذا (!prevCon && access.isReadOrRW()) {
            con = سچ;
            إذا (prevGen && !m_وچ_Clocked) {
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
                con = خطأ;
            }
            إذا (!m_وچ_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                إذا (m_force_Read_کنارا_Ignores.گنتی(varscp) || m_cgRefBoundps) con = خطأ;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        إذا (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            إذا (m_وچ_Post) {
                إذا (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    m_graphp->جوڑو_Hard_کنارا(m_logicVxp, varVxp, بھار_عام);
                }
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->جوڑو_Hard_کنارا(postVxp, m_logicVxp, بھار_POST);
            } وإلا إذا (!m_وچ_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جوڑو_Hard_کنارا(m_logicVxp, varVxp, بھار_عام);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                m_graphp->جوڑو_Hard_کنارا(postVxp, m_logicVxp, بھار_POST);
            } وإلا إذا (m_وچ_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                ترتیب_Var_راس* ثابت ordVxp = get_Var_راس(varscp, Var_راس_Type::PORD);
                m_graphp->جوڑو_Hard_کنارا(m_logicVxp, ordVxp, بھار_عام);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جوڑو_Hard_کنارا(m_logicVxp, varVxp, بھار_عام);
            } وإلا {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                ترتیب_Var_راس* ثابت ordVxp = get_Var_راس(varscp, Var_راس_Type::PORD);
                m_graphp->جوڑو_Hard_کنارا(ordVxp, m_logicVxp, بھار_عام);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جوڑو_Hard_کنارا(m_logicVxp, varVxp, بھار_عام);
            }
        }

        // Variable is consumed
        إذا (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            إذا (m_وچ_Post) {
                // Combinational logic
                إذا (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جوڑو_Hard_کنارا(varVxp, m_logicVxp, بھار_MEDIUM);
                }
            } وإلا إذا (!m_وچ_Clocked) {  // Combinational logic
                إذا (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جوڑو_Hard_کنارا(varVxp, m_logicVxp, بھار_MEDIUM);
                }
            } وإلا إذا (m_وچ_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                ترتیب_Var_راس* ثابت preVxp = get_Var_راس(varscp, Var_راس_Type::PRE);
                m_graphp->جوڑو_Soft_کنارا(preVxp, m_logicVxp, بھار_PRE);
            } وإلا {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                ترتیب_Var_راس* ثابت preVxp = get_Var_راس(varscp, Var_راس_Type::PRE);
                m_graphp->جوڑو_Hard_کنارا(m_logicVxp, preVxp, بھار_عام);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                m_graphp->جوڑو_Hard_کنارا(m_logicVxp, postVxp, بھار_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    فراغ ویکھو(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        AstCFunc* ثابت funcp = nodep->funcp();
        إذا (!funcp->isCovergroupSample()) أرجع;
        // Since sample is a built-in, we never expect recursion.
        UASSERT_OBJ(!m_cgRefBoundps, nodep, "Covergroup sample() calls another sample()");
        VL_RESTORER(m_cgRefBoundps);
        // Reference formals are bound per covergroup object. If the call handle matches
        // one that we recorded, use that info. If the call handle isn't something we
        // recorded (eg array-element construction), use the union of references across
        // the covergroup type.
        ثابت AstVarScope* instp = خالی_سوچک;
        إذا (ثابت AstVarRef* ثابت fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    فراغ ویکھو(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    فراغ ویکھو(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    فراغ ویکھو(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ ویکھو(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ ویکھو(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_وچ_Pre, nodep, "Should not nest");
        VL_RESTORER(m_وچ_Pre);
        m_وچ_Pre = سچ;
        iterateLogic(nodep);
    }
    فراغ ویکھو(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_وچ_Post, nodep, "Should not nest");
        VL_RESTORER(m_وچ_Post);
        m_وچ_Post = سچ;
        iterateLogic(nodep);
    }
    فراغ ویکھو(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ ویکھو(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ ویکھو(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    فراغ ویکھو(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    فراغ ویکھو(AstVar*) override {}
    فراغ ویکھو(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ ویکھو(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ ویکھو(Ast_Type_جدول* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ ویکھو(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ ویکھو(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ترتیب_گراف_Builder(ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,
                      ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ثابت V3Sched::CovergroupRefBindings& cgRefBindings, منطقي parallel)
        : m_graphp{جديد ترتیب_گراف{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        لكل (ثابت V3Sched::LogicByScope* ثابت lbsp : coll) {
            لكل (ثابت خودکار& اندراج : *lbsp) {
                m_scopep = اندراج.m_scopep;
                m_سرگرم_Domainp = m_graphp->domainp(اندراج.m_triggers);
                iterate(اندراج.m_activep);
                m_scopep = خالی_سوچک;
                m_سرگرم_Domainp = خالی_سوچک;
            }
        }
    }
    ~ترتیب_گراف_Builder() override = افتراضي;

عام:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ساكن std::unique_ptr<ترتیب_گراف> apply(ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,
                                             ثابت V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                             منطقي parallel) {
        أرجع std::unique_ptr<ترتیب_گراف>{
            ترتیب_گراف_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<ترتیب_گراف>
V3_ترتیب::build_ترتیب_گراف(ثابت V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,  //
                         ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         منطقي parallel) {
    أرجع ترتیب_گراف_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
