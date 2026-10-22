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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3AstNodeExpr.h"
#અન્તર્ભાવ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

નામાકાશ {

વર્ગ VirtIfaceVisitor final : જાહેર VNVisitor {
ખાનગી:
    // STATE
    using IfaceMember = std::pair<અચલ AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<અચલ AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    સ્થિર તાર્કિક writesToVirtIface(અચલ Ast_ગાંઠ* અચલ nodep) {
        પાછા nodep->exists([](અચલ AstMemberSel* અચલ memberSelp) {
            જો (!memberSelp->access().isWriteOrRW()) પાછા ખોટું;
            AstIfaceRefDType* અચલ dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            પાછા dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    શૂન્ય visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        જો (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* અચલ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            જો (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCMethodCall* nodep) override {
        જો (અચલ AstIfaceRefDType* અચલ dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            જો (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } નહીંતર {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* અચલ varp = nodep->varp();
        જો (varp->isTemp()) પાછા;
        m_vscpsByVar[varp].ધકેલો_back(nodep);
        જો (અચલ AstIface* અચલ ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        જો (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(ખોટું); });
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    શૂન્ય addCandidateVscp(અચલ AstIface* અચલ ifacep, AstVarScope* અચલ vscp) {
        અચલ IfaceMember member{ifacep, vscp->varp()->name()};
        જો (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].ધકેલો_back(vscp);
    }

    શૂન્ય addReachableIfaceCallable(AstIface* અચલ ifacep, AstCFunc* અચલ funcp) {
        અચલ IfaceCallable callable{ifacep, funcp};
        જો (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.ધકેલો_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    શૂન્ય buildTriggers() {
        માટે (અચલ auto& written : m_vifWrittenMembers) {
            અચલ auto it = m_candidateVscps.find(written);
            જો (it == m_candidateVscps.end()) ચાલુ;
            માટે (AstVarScope* અચલ vscp : it->second) {
                અચલ AstIface* અચલ ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

જાહેર:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        માટે (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            અચલ IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([આ, callable](Ast_ગાંઠ_પદાવલિ* અચલ nodep) {
                જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* અચલ varp = refp->varp();
                    જો (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        પાછા;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    અચલ auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    માટે (AstVarScope* અચલ vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } નહીંતર જો (Ast_ગાંઠ_CCall* અચલ callp = VN_CAST(nodep, ગાંઠ_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = મૂળભૂત;

    // METHODS
    VirtIfaceTriggers take_triggers() { પાછા std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    જો (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_વૃક્ષ("sched_vif", 0, dump_વૃક્ષ_Either_Level() >= 6);
    }
    પાછા triggers;
}

}  //namespace V3Sched
