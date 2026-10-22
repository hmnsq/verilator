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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeExpr.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਨਾਮਾਕਾਸ਼ {

ਵਰਗ VirtIfaceVisitor final : ਜਨਤਕ VNVisitor {
ਨਿਜੀ:
    // STATE
    ਵਰਤੋਂ IfaceMember = std::pair<ਅਚਲ AstIface*, std::string>;
    ਵਰਤੋਂ IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::ਸੈੱਟ<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::ਸਦਿਸ਼<AstVarScope*>> m_candidateVscps;
    std::ਸੈੱਟ<std::pair<IfaceMember, AstVarScope*>> m_ਵੇਖਿਆ_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ਅਚਲ AstVar*, std::ਸਦਿਸ਼<AstVarScope*>> m_vscpsByVar;
    std::ਸਦਿਸ਼<IfaceCallable> m_reachableIfaceCallables;
    std::ਸੈੱਟ<IfaceCallable> m_ਵੇਖਿਆ_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ਸਥਿਰ ਤਾਰਕਿਕ writes_ਮੰਜ਼ਿਲ_Virt_Iface(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) {
        ਵਾਪਸ nodep->exists([](ਅਚਲ AstMemberSel* ਅਚਲ memberSelp) {
            ਜੇ (!memberSelp->access().isWriteOrRW()) ਵਾਪਸ ਝੂਠ;
            AstIfaceRefDType* ਅਚਲ dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ਵਾਪਸ dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        ਜੇ (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ਅਚਲ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ਜੇ (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodCall* nodep) override {
        ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            ਜੇ (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } ਨਹੀਂਤਾਂ {
                ਜੋੜੋ_Reachable_Iface_Callable(dtypep->iface_ਰਾਹੀਂ_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (varp->is_ਆਰਜ਼ੀ()) ਵਾਪਸ;
        m_vscpsByVar[varp].ਧੱਕੋ_ਪਿੱਠ(nodep);
        ਜੇ (ਅਚਲ AstIface* ਅਚਲ ifacep = varp->sensIfacep()) ਜੋੜੋ_Candidate_Vscp(ifacep, nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        ਜੇ (VN_IS(nodep, AlwaysPost) && writes_ਮੰਜ਼ਿਲ_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(ਝੂਠ); });
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    ਖਾਲੀ ਜੋੜੋ_Candidate_Vscp(ਅਚਲ AstIface* ਅਚਲ ifacep, AstVarScope* ਅਚਲ vscp) {
        ਅਚਲ IfaceMember member{ifacep, vscp->varp()->name()};
        ਜੇ (m_ਵੇਖਿਆ_Candidate_Vscps.emplace(member, vscp).ਦੂਜਾ)
            m_candidateVscps[member].ਧੱਕੋ_ਪਿੱਠ(vscp);
    }

    ਖਾਲੀ ਜੋੜੋ_Reachable_Iface_Callable(AstIface* ਅਚਲ ifacep, AstCFunc* ਅਚਲ funcp) {
        ਅਚਲ IfaceCallable callable{ifacep, funcp};
        ਜੇ (m_ਵੇਖਿਆ_Reachable_Iface_Callables.emplace(callable).ਦੂਜਾ) {
            m_reachableIfaceCallables.ਧੱਕੋ_ਪਿੱਠ(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    ਖਾਲੀ buildTriggers() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& written : m_vifWrittenMembers) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_candidateVscps.ਲੱਭੋ(written);
            ਜੇ (it == m_candidateVscps.ਸਿਰਾ()) ਜਾਰੀ;
            ਲਈ (AstVarScope* ਅਚਲ vscp : it->ਦੂਜਾ) {
                ਅਚਲ AstIface* ਅਚਲ ifacep = written.ਪਹਿਲਾ;
                m_triggers.ਜੋੜੋ_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        ਲਈ (ਮਾਪ_t i = 0; i < m_reachableIfaceCallables.ਮਾਪ(); ++i) {
            ਅਚਲ IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.ਦੂਜਾ->foreach([ਇਹ, callable](Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
                ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ਅਚਲ varp = refp->varp();
                    ਜੇ (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_ਆਰਜ਼ੀ()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        ਵਾਪਸ;
                    }
                    varp->sensIfacep(callable.ਪਹਿਲਾ);
                    m_vifWrittenMembers.emplace(callable.ਪਹਿਲਾ, varp->name());
                    ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_vscpsByVar.ਲੱਭੋ(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.ਸਿਰਾ(), varp,
                                "No VarScope for interface member");
                    ਲਈ (AstVarScope* ਅਚਲ vscp : it->ਦੂਜਾ)
                        ਜੋੜੋ_Candidate_Vscp(callable.ਪਹਿਲਾ, vscp);
                } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_CCall* ਅਚਲ callp = VN_CAST(nodep, ਗੰਢ_CCall)) {
                    ਜੋੜੋ_Reachable_Iface_Callable(callable.ਪਹਿਲਾ, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = ਮੂਲ;

    // METHODS
    VirtIfaceTriggers take_triggers() { ਵਾਪਸ std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    ਜੇ (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_ਰੁੱਖ("sched_vif", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
    }
    ਵਾਪਸ triggers;
}

}  //namespace V3Sched
