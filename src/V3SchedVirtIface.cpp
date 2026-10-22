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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3AstNodeExpr.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ନାମାକାଶ {

ଶ୍ରେଣୀ VirtIfaceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
ବ୍ୟକ୍ତିଗତ:
    // STATE
    using IfaceMember = std::pair<ଅଚଳ AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ଅଚଳ AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ସ୍ଥିର ତାର୍କିକ writesToVirtIface(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        ଫେରନ୍ତୁ nodep->exists([](ଅଚଳ AstMemberSel* ଅଚଳ memberSelp) {
            ଯଦି (!memberSelp->access().isWriteOrRW()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            AstIfaceRefDType* ଅଚଳ dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ଫେରନ୍ତୁ dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        ଯଦି (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ଅଚଳ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ଯଦି (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCMethodCall* nodep) override {
        ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            ଯଦି (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } ଅନ୍ୟଥା {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (varp->isTemp()) ଫେରନ୍ତୁ;
        m_vscpsByVar[varp].ଠେଲ_back(nodep);
        ଯଦି (ଅଚଳ AstIface* ଅଚଳ ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        ଯଦି (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(ମିଥ୍ୟା); });
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    ଶୂନ୍ୟ addCandidateVscp(ଅଚଳ AstIface* ଅଚଳ ifacep, AstVarScope* ଅଚଳ vscp) {
        ଅଚଳ IfaceMember member{ifacep, vscp->varp()->name()};
        ଯଦି (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].ଠେଲ_back(vscp);
    }

    ଶୂନ୍ୟ addReachableIfaceCallable(AstIface* ଅଚଳ ifacep, AstCFunc* ଅଚଳ funcp) {
        ଅଚଳ IfaceCallable callable{ifacep, funcp};
        ଯଦି (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.ଠେଲ_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    ଶୂନ୍ୟ buildTriggers() {
        ପାଇଁ (ଅଚଳ auto& written : m_vifWrittenMembers) {
            ଅଚଳ auto it = m_candidateVscps.find(written);
            ଯଦି (it == m_candidateVscps.end()) ଜାରି;
            ପାଇଁ (AstVarScope* ଅଚଳ vscp : it->second) {
                ଅଚଳ AstIface* ଅଚଳ ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        ପାଇଁ (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ଅଚଳ IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([ଏହି, callable](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep) {
                ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ଅଚଳ varp = refp->varp();
                    ଯଦି (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        ଫେରନ୍ତୁ;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ଅଚଳ auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    ପାଇଁ (AstVarScope* ଅଚଳ vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_CCall* ଅଚଳ callp = VN_CAST(nodep, ଗଣ୍ଠି_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    VirtIfaceTriggers take_triggers() { ଫେରନ୍ତୁ std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    ଯଦି (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_ବୃକ୍ଷ("sched_vif", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
    }
    ଫେରନ୍ତୁ triggers;
}

}  //namespace V3Sched
