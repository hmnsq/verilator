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
    std::array<ترتیب_Var_راس*, static_cast<اندازه_t>(Var_راس_Type::POST) + 1> m_vertexps;

عام:
    // METHODS
    ترتیب_Var_راس* get_Var_راس(ترتیب_ګراف* graphp, AstVarScope* varscp, Var_راس_Type type) {
        ثابت بینښه idx = static_cast<بینښه>(type);
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
    ترتیب_User() { m_vertexps.fill(تش_اشاره); }
    ~ترتیب_User() = افتراضي;
};

//######################################################################
// OrderBuildVisitor builds the ordering graph of the entire netlist, and
// removes any nodes that are no longer required once the graph is built

صنف ترتیب_ګراف_Builder final : عام VNVisitor {
    // TYPES
    تعداد VarUsage : uint8_t { VU_CON = 0x1, VU_GEN = 0x2 };
    تعداد VarAccess : uint8_t { VA_READ = 0x1, VA_WRITE = 0x2 };
    کارول Var_راس_Type = ترتیب_User::Var_راس_Type;

    // NODE STATE
    //  AstVarScope::user1    -> OrderUser instance for variable (via m_orderUser)
    //  AstVarScope::user2    -> VarUsage within logic blocks
    //  AstVarScope::user3    -> bool: Hybrid sensitivity
    //  AstVarScope::user4    -> VarAccess within logic blocks
    ثابت VNUser1_دننه_Use user1_دننه_Use;
    ثابت VNUser2_دننه_Use user2_دننه_Use;
    ثابت VNUser3_دننه_Use user3_دننه_Use;
    ثابت VNUser4_دننه_Use user4_دننه_Use;
    AstUser1Allocator<AstVarScope, ترتیب_User> m_ترتیب_User;

    // STATE
    ترتیب_ګراف* ثابت m_graphp;  // The ordering graph built by this visitor
    ترتیب_Logic_راس* m_logicVxp = تش_اشاره;  // Current logic block being analyzed
    std::ویکټور<AstVarScope*> m_accessedVscps;  // Variables accessed by the current logic block

    // Current AstScope being processed
    AstScope* m_scopep = تش_اشاره;
    // Domain of the triggers of the current AstActive being processed
    ثابت ترتیب_Domain* m_فعال_Domainp = تش_اشاره;
    // Sensitivity list for clocked logic, nullptr for combinational and hybrid logic
    ثابت ترتیب_Domain* m_domainp = تش_اشاره;
    // Sensitivity list for hybrid logic, nullptr for everything else
    ثابت ترتیب_Domain* m_hybridp = تش_اشاره;

    منطقي m_دننه_Clocked = خطأ;  // Underneath clocked AstActive
    منطقي m_دننه_Pre = خطأ;  // Underneath AlwaysPre
    منطقي m_دننه_Post = خطأ;  // Underneath AstAlwaysPost
    std::function<منطقي(ثابت AstVarScope*)> m_readTriggersCombLogic;
    V3Sched::util::Var_Scope_سېټ m_force_Read_څنډه_Ignores;
    ثابت منطقي m_parallel;  // Ordering for multi-threaded execution (record variable accesses)

    // What covergroup reference formal arguments are bound to at construction
    ثابت V3Sched::CovergroupRefBindings& m_cgRefBindings;
    // Bindings reachable from the covergroup sample() being walked, nullptr when not in one
    ثابت V3Sched::CovergroupRefBindings::Bindings* m_cgRefBoundps = تش_اشاره;

    // METHODS

    فراغ iterateLogic(Ast_غوټه* nodep) {
        UASSERT_OBJ(!m_logicVxp, nodep, "Should not nest");
        // Reset VarUsage and VarAccess
        Ast_غوټه::user2_Clear_ونه();
        Ast_غوټه::user4_Clear_ونه();
        m_force_Read_څنډه_Ignores.clear();
        إذا (!m_دننه_Clocked)
            V3Sched::util::collect_Force_Read_څنډه_Ignores(nodep, m_force_Read_څنډه_Ignores);
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
                m_logicVxp->جمع_کړه_Var_Access(vscp, access);
            }
            m_accessedVscps.clear();
        }
        // Finished with this logic
        m_logicVxp = تش_اشاره;
        m_force_Read_څنډه_Ignores.clear();
    }

    ترتیب_Var_راس* get_Var_راس(AstVarScope* varscp, Var_راس_Type type) {
        أرجع m_ترتیب_User(varscp).get_Var_راس(m_graphp, varscp, type);
    }

    // VISITORS
    فراغ لیدنه(Ast_فعال* nodep) override {
        UASSERT_OBJ(!nodep->sen_ونه_Storep(), nodep,
                    "AstSenTrees should have been made global in V3ActiveTop");
        UASSERT_OBJ(m_scopep, nodep, "AstActive not under AstScope");
        UASSERT_OBJ(!m_logicVxp, nodep, "AstActive under logic");
        UASSERT_OBJ(!m_دننه_Clocked && !m_domainp && !m_hybridp, nodep, "Should not nest");

        VL_RESTORER(m_domainp);
        VL_RESTORER(m_hybridp);
        VL_RESTORER(m_دننه_Clocked);

        ثابت Ast_Sen_ونه* ثابت senTreep = nodep->sentreep();

        m_دننه_Clocked = senTreep->hasClocked();

        // Note: We don't need to analyze the sensitivity list, as currently all sensitivity
        // lists simply reference an entry in a trigger vector, which are all set external to
        // the code being ordered.

        // Combinational and hybrid logic will have it's domain assigned based on the driver
        // domains. For clocked logic, we already know its domain. Hybrid logic also includes
        // additional sensitivities.
        إذا (senTreep->hasHybrid()) {
            m_hybridp = m_فعال_Domainp;
            // Mark AstVarScopes that are explicit sensitivities
            Ast_غوټه::user3_Clear_ونه();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user3(ریښتیا);
            });
            m_readTriggersCombLogic = [](ثابت AstVarScope* vscp) { أرجع !vscp->user3(); };
        } وإلا {
            إذا (!senTreep->hasCombo()) m_domainp = m_فعال_Domainp;
            // Always triggers
            m_readTriggersCombLogic = [](ثابت AstVarScope*) { أرجع ریښتیا; };
        }

        // Analyze logic underneath
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
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
    فراغ ریکارډ_Raw_Access(AstVarScope* varscp, ثابت VAccess& access, Ast_غوټه* nodep) {
        إذا (!m_parallel) أرجع;
        uint8_t recorded = 0;
        إذا (access.isWriteOrRW()) recorded |= VA_WRITE;
        إذا (access.isReadOrRW()) recorded |= VA_READ;
        UASSERT_OBJ(recorded, nodep, "Unknown variable access type");
        // Accumulate access type, record the variable on first access only
        إذا (!varscp->user4Or(recorded)) m_accessedVscps.ټېل_شا(varscp);
    }

    // Add the graph edges, and record the raw access, for one access of one variable
    فراغ accountVarAccess(AstVarScope* varscp, ثابت VAccess& access, Ast_غوټه* nodep) {
        // Variable reference in logic. Add data dependency.
        ریکارډ_Raw_Access(varscp, access, nodep);

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
            con = ریښتیا;
            إذا (prevGen && !m_دننه_Clocked) {
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
            إذا (!m_دننه_Clocked) {
                // Ignored reads and references from within covergroups do not
                // add to the combinational sensitivity of the block
                إذا (m_force_Read_څنډه_Ignores.شمېر(varscp) || m_cgRefBoundps) con = خطأ;
            }
        }

        // Note: See V3OrderGraph.h about the roles of the various vertex types

        // Variable is produced
        إذا (gen) {
            // Update VarUsage
            varscp->user2Or(VU_GEN);
            // Add edges for produced variables
            إذا (m_دننه_Post) {
                إذا (!varscp->varp()->ignorePostWrite()) {
                    // Add edge from producing LogicVertex -> produced VarStdVertex
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, varVxp, وزن_نورمال);
                }
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                m_graphp->جمع_کړه_Hard_څنډه(postVxp, m_logicVxp, وزن_POST);
            } وإلا إذا (!m_دننه_Clocked) {  // Combinational logic
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, varVxp, وزن_نورمال);
                // Add edge from produced VarPostVertex -> to producing LogicVertex
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                m_graphp->جمع_کړه_Hard_څنډه(postVxp, m_logicVxp, وزن_POST);
            } وإلا إذا (m_دننه_Pre) {  // AstAlwaysPre
                // Add edge from producing LogicVertex -> produced VarPordVertex
                ترتیب_Var_راس* ثابت ordVxp = get_Var_راس(varscp, Var_راس_Type::PORD);
                m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, ordVxp, وزن_نورمال);
                // Add edge from producing LogicVertex -> produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, varVxp, وزن_نورمال);
            } وإلا {
                // Sequential (clocked) logic
                // Add edge from produced VarPordVertex -> to producing LogicVertex
                ترتیب_Var_راس* ثابت ordVxp = get_Var_راس(varscp, Var_راس_Type::PORD);
                m_graphp->جمع_کړه_Hard_څنډه(ordVxp, m_logicVxp, وزن_نورمال);
                // Add edge from producing LogicVertex-> to produced VarStdVertex
                ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, varVxp, وزن_نورمال);
            }
        }

        // Variable is consumed
        إذا (con) {
            // Update VarUsage
            varscp->user2Or(VU_CON);
            // Add edges
            إذا (m_دننه_Post) {
                // Combinational logic
                إذا (!varscp->varp()->ignorePostRead() && m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جمع_کړه_Hard_څنډه(varVxp, m_logicVxp, وزن_MEDIUM);
                }
            } وإلا إذا (!m_دننه_Clocked) {  // Combinational logic
                إذا (m_readTriggersCombLogic(varscp)) {
                    // Ignore explicit sensitivities
                    ترتیب_Var_راس* ثابت varVxp = get_Var_راس(varscp, Var_راس_Type::STD);
                    // Add edge from consumed VarStdVertex -> to consuming LogicVertex
                    m_graphp->جمع_کړه_Hard_څنډه(varVxp, m_logicVxp, وزن_MEDIUM);
                }
            } وإلا إذا (m_دننه_Pre) {
                // AstAlwaysPre logic
                // Add edge from consumed VarPreVertex -> to consuming LogicVertex
                // This one is cutable (vs the producer) as there's only one such consumer,
                // but may be many producers
                ترتیب_Var_راس* ثابت preVxp = get_Var_راس(varscp, Var_راس_Type::PRE);
                m_graphp->جمع_کړه_Soft_څنډه(preVxp, m_logicVxp, وزن_PRE);
            } وإلا {
                // Sequential (clocked) logic
                // Add edge from consuming LogicVertex -> to consumed VarPreVertex
                // Generation of 'pre' because we want to indicate it should be before
                // AstAlwaysPre
                ترتیب_Var_راس* ثابت preVxp = get_Var_راس(varscp, Var_راس_Type::PRE);
                m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, preVxp, وزن_نورمال);
                // Add edge from consuming LogicVertex -> to consumed VarPostVertex
                ترتیب_Var_راس* ثابت postVxp = get_Var_راس(varscp, Var_راس_Type::POST);
                m_graphp->جمع_کړه_Hard_څنډه(m_logicVxp, postVxp, وزن_POST);
            }
        }
    }
    // A covergroup sample() is not inlined and may read design signals through cross-scope
    // references held by the covergroup. This attributes those references to the calling block.
    فراغ لیدنه(AstCMethodCall* nodep) override {
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
        ثابت AstVarScope* instp = تش_اشاره;
        إذا (ثابت AstVarRef* ثابت fromRefp = VN_CAST(nodep->fromp(), VarRef)) {
            instp = fromRefp->varScopep();
        }
        m_cgRefBoundps = &m_cgRefBindings.forSample(instp, VN_AS(funcp->scopep()->modp(), Class));
        iterateChildren(funcp);
    }

    //--- Logic akin to SystemVerilog Processes (AstNodeProcedure)
    فراغ لیدنه(AstInitial* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitial should not need ordering");
    }  // LCOV_EXCL_STOP
    فراغ لیدنه(AstInitialStatic* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstInitialStatic should not need ordering");
    }  // LCOV_EXCL_STOP
    فراغ لیدنه(AstInitialAutomatic* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ لیدنه(AstAlways* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ لیدنه(AstAlwaysPre* nodep) override {
        UASSERT_OBJ(!m_دننه_Pre, nodep, "Should not nest");
        VL_RESTORER(m_دننه_Pre);
        m_دننه_Pre = ریښتیا;
        iterateLogic(nodep);
    }
    فراغ لیدنه(AstAlwaysPost* nodep) override {
        UASSERT_OBJ(!m_دننه_Post, nodep, "Should not nest");
        VL_RESTORER(m_دننه_Post);
        m_دننه_Post = ریښتیا;
        iterateLogic(nodep);
    }
    فراغ لیدنه(AstAlwaysObserved* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ لیدنه(AstAlwaysReactive* nodep) override {  //
        iterateLogic(nodep);
    }
    فراغ لیدنه(AstFinal* nodep) override {  // LCOV_EXCL_START
        nodep->v3fatalSrc("AstFinal should not need ordering");
    }  // LCOV_EXCL_STOP

    //--- Verilator concoctions
    فراغ لیدنه(AstCoverToggle* nodep) override {  //
        iterateLogic(nodep);
    }

    //--- Ignored nodes
    فراغ لیدنه(AstVar*) override {}
    فراغ لیدنه(AstVarScope* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ لیدنه(AstCell* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ لیدنه(Ast_Type_جدول* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ لیدنه(AstClass* nodep) override { nodep->v3fatalSrc("Should not reach V3Order"); }
    فراغ لیدنه(AstCFunc*) override {
        // Calls to DPI exports handled with AstCCall. /* verilator public */ functions are
        // ignored for now (and hence potentially mis-ordered), but could use the same or
        // similar mechanism as DPI exports. Every other impure function (including those
        // that may set a non-local variable) must have been inlined in V3Task.
    }

    //---
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ترتیب_ګراف_Builder(ثابت std::ویکټور<V3Sched::LogicByScope*>& coll,
                      ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp,
                      ثابت V3Sched::CovergroupRefBindings& cgRefBindings, منطقي parallel)
        : m_graphp{جديد ترتیب_ګراف{trigKit, trigVscp}}
        , m_parallel{parallel}
        , m_cgRefBindings{cgRefBindings} {
        // Build the graph
        لكل (ثابت V3Sched::LogicByScope* ثابت lbsp : coll) {
            لكل (ثابت خپلکاری& ثبت : *lbsp) {
                m_scopep = ثبت.m_scopep;
                m_فعال_Domainp = m_graphp->domainp(ثبت.m_triggers);
                iterate(ثبت.m_activep);
                m_scopep = تش_اشاره;
                m_فعال_Domainp = تش_اشاره;
            }
        }
    }
    ~ترتیب_ګراف_Builder() override = افتراضي;

عام:
    // Process the netlist and return the constructed ordering graph. It's 'process' because
    // this visitor does change the tree (removes some nodes related to DPI export trigger).
    ساكن std::unique_ptr<ترتیب_ګراف> apply(ثابت std::ویکټور<V3Sched::LogicByScope*>& coll,
                                             ثابت V3Sched::TriggerKit& trigKit,
                                             AstVarScope* trigVscp,
                                             ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                             منطقي parallel) {
        أرجع std::unique_ptr<ترتیب_ګراف>{
            ترتیب_ګراف_Builder{coll, trigKit, trigVscp, cgRefBindings, parallel}.m_graphp};
    }
};

std::unique_ptr<ترتیب_ګراف>
V3_ترتیب::build_ترتیب_ګراف(ثابت V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ثابت std::ویکټور<V3Sched::LogicByScope*>& coll,  //
                         ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                         منطقي parallel) {
    أرجع ترتیب_ګراف_Builder::apply(coll, trigKit, trigVscp, cgRefBindings, parallel);
}
