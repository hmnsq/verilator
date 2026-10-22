// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create triggers necessary for scheduling across
//                         virtual interfaces
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
// V3SchedVirtIface's Transformations:
//
// Identify interface members written through virtual interface variables (VIF writes).
// For each such member, collect all VarScopes across all instantiations of that
// interface type. Each VarScope gets its own value-change trigger in the scheduler.
//
// Also disables lifetime optimization for variables in AlwaysPost blocks that
// write through virtual interfaces.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3AstNodeExpr.h"
#అన్తర్భావ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

నామాకాశ {

తరగతి VirtIfaceVisitor final : బహిరంగ VNVisitor {
అంతరంగ:
    // STATE
    ఉపయోగించి IfaceMember = std::pair<స్థిరరాశి AstIface*, std::string>;
    ఉపయోగించి IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::సమితి<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::సదిశ<AstVarScope*>> m_candidateVscps;
    std::సమితి<std::pair<IfaceMember, AstVarScope*>> m_చూసిన_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<స్థిరరాశి AstVar*, std::సదిశ<AstVarScope*>> m_vscpsByVar;
    std::సదిశ<IfaceCallable> m_reachableIfaceCallables;
    std::సమితి<IfaceCallable> m_చూసిన_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    స్థిర తార్కిక writes_గమ్యం_Virt_Iface(స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) {
        తిరిగి nodep->exists([](స్థిరరాశి AstMemberSel* స్థిరరాశి memberSelp) {
            ఒకవేళ (!memberSelp->access().isWriteOrRW()) తిరిగి అబద్ధం;
            AstIfaceRefDType* స్థిరరాశి dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            తిరిగి dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        ఒకవేళ (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* స్థిరరాశి dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ఒకవేళ (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCMethodCall* nodep) override {
        ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            ఒకవేళ (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } లేకపోతే {
                కూడు_Reachable_Iface_Callable(dtypep->iface_ద్వారా_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (varp->is_తాత్కాలిక()) తిరిగి;
        m_vscpsByVar[varp].నెట్టు_వెనుక(nodep);
        ఒకవేళ (స్థిరరాశి AstIface* స్థిరరాశి ifacep = varp->sensIfacep()) కూడు_Candidate_Vscp(ifacep, nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        ఒకవేళ (VN_IS(nodep, AlwaysPost) && writes_గమ్యం_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(అబద్ధం); });
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    శూన్య కూడు_Candidate_Vscp(స్థిరరాశి AstIface* స్థిరరాశి ifacep, AstVarScope* స్థిరరాశి vscp) {
        స్థిరరాశి IfaceMember member{ifacep, vscp->varp()->name()};
        ఒకవేళ (m_చూసిన_Candidate_Vscps.emplace(member, vscp).రెండవ)
            m_candidateVscps[member].నెట్టు_వెనుక(vscp);
    }

    శూన్య కూడు_Reachable_Iface_Callable(AstIface* స్థిరరాశి ifacep, AstCFunc* స్థిరరాశి funcp) {
        స్థిరరాశి IfaceCallable callable{ifacep, funcp};
        ఒకవేళ (m_చూసిన_Reachable_Iface_Callables.emplace(callable).రెండవ) {
            m_reachableIfaceCallables.నెట్టు_వెనుక(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    శూన్య buildTriggers() {
        కొరకు (స్థిరరాశి స్వయంచాలక& written : m_vifWrittenMembers) {
            స్థిరరాశి స్వయంచాలక it = m_candidateVscps.కనుగొను(written);
            ఒకవేళ (it == m_candidateVscps.చివర()) కొనసాగు;
            కొరకు (AstVarScope* స్థిరరాశి vscp : it->రెండవ) {
                స్థిరరాశి AstIface* స్థిరరాశి ifacep = written.మొదటి;
                m_triggers.కూడు_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        కొరకు (కొలత_t i = 0; i < m_reachableIfaceCallables.కొలత(); ++i) {
            స్థిరరాశి IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.రెండవ->foreach([ఇది, callable](Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
                ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* స్థిరరాశి varp = refp->varp();
                    ఒకవేళ (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_తాత్కాలిక()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        తిరిగి;
                    }
                    varp->sensIfacep(callable.మొదటి);
                    m_vifWrittenMembers.emplace(callable.మొదటి, varp->name());
                    స్థిరరాశి స్వయంచాలక it = m_vscpsByVar.కనుగొను(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.చివర(), varp,
                                "No VarScope for interface member");
                    కొరకు (AstVarScope* స్థిరరాశి vscp : it->రెండవ)
                        కూడు_Candidate_Vscp(callable.మొదటి, vscp);
                } లేకపోతే ఒకవేళ (Ast_కణుపు_CCall* స్థిరరాశి callp = VN_CAST(nodep, కణుపు_CCall)) {
                    కూడు_Reachable_Iface_Callable(callable.మొదటి, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = అప్రమేయ;

    // METHODS
    VirtIfaceTriggers take_triggers() { తిరిగి std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    ఒకవేళ (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_వృక్షం("sched_vif", 0, dump_వృక్షం_Either_Level() >= 6);
    }
    తిరిగి triggers;
}

}  //namespace V3Sched
