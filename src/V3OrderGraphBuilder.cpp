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
    تعداد صنف Var_رأس_Type : uint8_t {  // Types of vertices we can create
        STD = 0,
        PRE = 1,
        PORD = 2,
        POST = 3
    };

خاص:
    // Vertex of each type (if non-nullptr)
    std::array<ترتیب_Var_رأس*, static_cast<اندازه_t>(Var_رأس_Type::POST) + 1> m_vertexps;

عام:
    // METHODS
    ترتیب_Var_رأس* get_Var_رأس(ترتیب_گراف* graphp, AstVarScope* varscp, Var_رأس_Type type) {
        ثابت بیعلامت idx = static_cast<بیعلامت>(type);
        ترتیب_Var_رأس* vertexp = m_vertexps[idx];
        إذا (!vertexp) {
            بدل (type) {
            حالة Var_رأس_Type::STD: vertexp = جديد ترتیب_Var_Std_رأس{graphp, varscp}; اكسر;
            حالة Var_رأس_Type::PRE: vertexp = جديد ترتیب_Var_Pre_رأس{graphp, varscp}; اكسر;
            حالة Var_رأس_Type::PORD: vertexp = جديد ترتیب_Var_Pord_رأس{graphp, varscp}; اكسر;
            حالة Var_رأس_Type::POST: vertexp = جديد ترتیب_Var_Post_رأس{graphp, varscp}; اكسر;
            }
            m_vertexps[idx] = vertexp;
        }
        أرجع vertexp;
    }

    // CONSTRUCTORS
    ترتیب_User() { m_vertexps.fill(اشاره_تهی); }
    ~ترتیب_User() = افتراضي;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

صنف ترتیب_گراف_Builder final : عام VNVisitor {
    // TYPES
    تعداد VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    تعداد VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    استفاده Var_رأس_Type = ترتیب_User::Var_رأس_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ثابت VNUser1_درون_Use user1_درون_Use;
    ثابت VNUser2_درون_Use user2_درون_Use;
    ثابت VNUser3_درون_Use user3_درون_Use;
    ثابت VNUser4_درون_Use user4_درون_Use;
    AstUser1Allocator<AstVarScope, ترتیب_User> m_ترتیب_User;

    // STATE
    ترتیب_گراف* ثابت m_graphp;  // The ordering graph built by this visitor
    ترتیب_Logic_رأس* m_logicVxp = اشاره_تهی;  // Current logic block being analyzed
    std::وکتور<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = اشاره_تهی;
    // Domain of the triggers of the current AstActive being processed
    ثابت ترتیب_Domain* m_فعال_Domainp = اشاره_تهی;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ثابت ترتیب_Domain* m_domainp = اشاره_تهی;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ثابت ترتیب_Domain* m_hybridp = اشاره_تهی;

    منطقي m_درون_Clocked = خطأ;  // Underneath clocked AstActive
    منطقي m_درون_Pre = خطأ;  // Underneath AlwaysPre
    منطقي m_درون_Post = خطأ;  // Underneath AstAlwaysPost
    std::function<منطقي(ثابت AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_مجموعه m_force_Read_یال_Ignores;
    ثابت منطقي m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ثابت V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ثابت V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = اشاره_تهی;

    // METHODS

    فراغ iterateLogic(Ast_گره* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_گره::user2_Clear_درخت();
        Ast_گره::user4_Clear_درخت();
        m_force_Read_یال_Ignores.clear();
        إذا (!m_درون_Clocked)
            V3Sched::util::collect_Force_Read_یال_Ignores(nodep, m_force_Read_یال_Ignores);
        // Create LogicVertex for this logic node
        m_logicVxp = جديد ترتیب_Logic_رأس{m_graphp, m_scopep, m_domainp, m_hybridp, nodep};
        // Gather variable dependencies based on usage
        iterateChildren(nodep);
        إذا (m_parallel) {
            // Emit one access record for each variable this logic block accessed
            لكل (AstVarScope* ثابت vscp : m_accessedVscps) {
                ثابت صحيح recorded = vscp->user4();
                ثابت VAccess access = recorded == (VA_READ | VA_WRITE) ? VAccess::READWRITE
                                       : recorded == VA_WRITE           ? VAccess::WRITE
                                                                        : VAccess::READ;
                m_logicVxp->جمع_کن_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = اشاره_تهی;
        m_force_Read_یال_Ignores.clear();
    }

    ترتیب_Var_رأس* get_Var_رأس(AstVarScope* varscp, Var_رأس_Type type) {
        أرجع m_ترتیب_User(varscp).get_Var_رأس(m_graphp, varscp, type);
    }

    // VISITORS
    فراغ بازدید(Ast_فعال* nodep) override {
        UASSERT_OBJ(!nodep->sen_درخت_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_درون_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_درون_Clocked);

        ثابت Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();

        m_درون_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        إذا (senTreep->hasHybrid()) {
            m_hybridp = m_فعال_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_گره::user3_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user3(درست);
            });
            m_readTriggersCombLogic = [](ثابت AstVarScope* vscp) { أرجع !vscp->user3(); };
        } وإلا {
            إذا (!senTreep->hasCombo()) m_domainp = m_فعال_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ثابت AstVarScope*) { أرجع درست; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
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
    فراغ رکورد_Raw_Access(AstVarScope* varscp, ثابت VAccess& access, Ast_گره* nodep) {
        إذا (!m_parallel) أرجع;
        uint8_t recorded = 0;
        إذا (access.isWriteOrRW()) recorded |= VA_WRITE;
        إذا (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        إذا (!varscp->user4Or(recorded)) m_accessedVscps.افزودن_پشت(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    فراغ accountVarAccess(AstVarScope* varscp, ثابت VAccess& access, Ast_گره* nodep) {
        // Variable reference in logic. Add data dependency.
        رکورد_Raw_Access(varscp, access, nodep);

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
            con = درست;
            إذا (prevGen && !m_درون_Clocked) {
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
            إذا (!m_درون_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                إذا (m_force_Read_یال_Ignores.شمار(varscp) || m_cgRefBoundps) con = خطأ;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        إذا (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            إذا (m_درون_Post) {
                إذا (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    ترتیب_Var_رأس* ثابت varVxp = get_Var_رأس(varscp, Var_رأس_Type::STD);
                    m_graphp->جمع_کن_Hard_یال(m_logicVxp, varVxp, وزن_نرمال);
                }
                ترتیب_Var_رأس* ثابت postVxp = get_Var_رأس(varscp, Var_رأس_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->جمع_کن_Hard_یال(postVxp, m_logicVxp, وزن_POST);
            } وإلا إذا (!m_درون_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_رأس* ثابت varVxp = get_Var_رأس(varscp, Var_رأس_Type::STD);
                m_graphp->جمع_کن_Hard_یال(m_logicVxp, varVxp, وزن_نرمال);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                ترتیب_Var_رأس* ثابت postVxp = get_Var_رأس(varscp, Var_رأس_Type::POST);
                m_graphp->جمع_کن_Hard_یال(postVxp, m_logicVxp, وزن_POST);
            } وإلا إذا (m_درون_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                ترتیب_Var_رأس* ثابت ordVxp = get_Var_رأس(varscp, Var_رأس_Type::PORD);
                m_graphp->جمع_کن_Hard_یال(m_logicVxp, ordVxp, وزن_نرمال);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_رأس* ثابت varVxp = get_Var_رأس(varscp, Var_رأس_Type::STD);
                m_graphp->جمع_کن_Hard_یال(m_logicVxp, varVxp, وزن_نرمال);
            } وإلا {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                ترتیب_Var_رأس* ثابت ordVxp = get_Var_رأس(varscp, Var_رأس_Type::PORD);
                m_graphp->جمع_کن_Hard_یال(ordVxp, m_logicVxp, وزن_نرمال);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                ترتیب_Var_رأس* ثابت varVxp = get_Var_رأس(varscp, Var_رأس_Type::STD);
                m_graphp->جمع_کن_Hard_یال(m_logicVxp, varVxp, وزن_نرمال);
            }
        }

        // Variable is consumed
        إذا (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            إذا (m_درون_Post) {
                // Combinational logic
                إذا (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_رأس* ثابت varVxp = get_Var_رأس(varscp, Var_رأس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جمع_کن_Hard_یال(varVxp, m_logicVxp, وزن_MEDIUM);
                }
            } وإلا إذا (!m_درون_Clocked) {  // Combinational logic
                إذا (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_رأس* ثابت varVxp = get_Var_رأس(varscp, Var_رأس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جمع_کن_Hard_یال(varVxp, m_logicVxp, وزن_MEDIUM);
                }
            } وإلا إذا (m_درون_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                ترتیب_Var_رأس* ثابت preVxp = get_Var_رأس(varscp, Var_رأس_Type::PRE);
                m_graphp->جمع_کن_Soft_یال(preVxp, m_logicVxp, وزن_PRE);
            } وإلا {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                ترتیب_Var_رأس* ثابت preVxp = get_Var_رأس(varscp, Var_رأس_Type::PRE);
                m_graphp->جمع_کن_Hard_یال(m_logicVxp, preVxp, وزن_نرمال);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                ترتیب_Var_رأس* ثابت postVxp = get_Var_رأس(varscp, Var_رأس_Type::POST);
                m_graphp->جمع_کن_Hard_یال(m_logicVxp, postVxp, وزن_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    فراغ بازدید(AstCMethodCall* nodep) override {
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
        ثابت AstVarScope* instp = اشاره_تهی;
        إذا (ثابت AstVarRef* ثابت fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    فراغ بازدید(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    فراغ بازدید(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    فراغ بازدید(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ بازدید(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ بازدید(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_درون_Pre, nodep, "Should not nest");
        VL_RESTORER(m_درون_Pre);
        m_درون_Pre = درست;
        iterateLogic(nodep);
    }
    فراغ بازدید(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_درون_Post, nodep, "Should not nest");
        VL_RESTORER(m_درون_Post);
        m_درون_Post = درست;
        iterateLogic(nodep);
    }
    فراغ بازدید(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ بازدید(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ بازدید(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    فراغ بازدید(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    فراغ بازدید(AstVar*) override {}
    فراغ بازدید(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ بازدید(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ بازدید(Ast_Type_جدول* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ بازدید(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ بازدید(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ترتیب_گراف_Builder(ثابت std::وکتور<V3Sched::LogicByScope*>& coll,
                      ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ثابت V3Sched::CovergroupRefBindings& cgRefBindings, منطقي parallel)
        : m_graphp{جديد ترتیب_گراف{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        لكل (ثابت V3Sched::LogicByScope* ثابت lbsp : coll) {
            لكل (ثابت خودکار& مدخل : *lbsp) {
                m_scopep = مدخل.m_scopep;
                m_فعال_Domainp = m_graphp->domainp(مدخل.m_triggers);
                iterate(مدخل.m_activep);
                m_scopep = اشاره_تهی;
                m_فعال_Domainp = اشاره_تهی;
            }
        }
    }
    ~ترتیب_گراف_Builder() override = افتراضي;

عام:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ساكن std::unique_ptr<ترتیب_گراف> apply(ثابت std::وکتور<V3Sched::LogicByScope*>& coll,
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
                         ثابت std::وکتور<V3Sched::LogicByScope*>& coll,  //
                         ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         منطقي parallel) {
    أرجع ترتیب_گراف_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
