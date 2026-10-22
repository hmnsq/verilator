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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AstNodeExpr.h"
#अन्तर्भाव "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

वर्ग VirtIfaceVisitor final : सार्वजनिक VNVisitor {
खाजगी:
    // STATE
    using IfaceMember = std::pair<स्थिरांक AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<स्थिरांक AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    स्थिर तार्किक writesToVirtIface(स्थिरांक Ast_गाठ* स्थिरांक nodep) {
        परत nodep->exists([](स्थिरांक AstMemberSel* स्थिरांक memberSelp) {
            जर (!memberSelp->access().isWriteOrRW()) परत असत्य;
            AstIfaceRefDType* स्थिरांक dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            परत dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    शून्य visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        जर (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* स्थिरांक dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            जर (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCMethodCall* nodep) override {
        जर (स्थिरांक AstIfaceRefDType* स्थिरांक dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            जर (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } अन्यथा {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* स्थिरांक varp = nodep->varp();
        जर (varp->isTemp()) परत;
        m_vscpsByVar[varp].ढकला_back(nodep);
        जर (स्थिरांक AstIface* स्थिरांक ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        जर (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(असत्य); });
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

    शून्य addCandidateVscp(स्थिरांक AstIface* स्थिरांक ifacep, AstVarScope* स्थिरांक vscp) {
        स्थिरांक IfaceMember member{ifacep, vscp->varp()->name()};
        जर (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].ढकला_back(vscp);
    }

    शून्य addReachableIfaceCallable(AstIface* स्थिरांक ifacep, AstCFunc* स्थिरांक funcp) {
        स्थिरांक IfaceCallable callable{ifacep, funcp};
        जर (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.ढकला_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    शून्य buildTriggers() {
        साठी (स्थिरांक स्वतः& written : m_vifWrittenMembers) {
            स्थिरांक स्वतः it = m_candidateVscps.find(written);
            जर (it == m_candidateVscps.end()) सुरूठेवा;
            साठी (AstVarScope* स्थिरांक vscp : it->second) {
                स्थिरांक AstIface* स्थिरांक ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        साठी (माप_प्रकार i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            स्थिरांक IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([हे, callable](Ast_गाठ_अभिव्यक्ती* स्थिरांक nodep) {
                जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* स्थिरांक varp = refp->varp();
                    जर (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        परत;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    स्थिरांक स्वतः it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    साठी (AstVarScope* स्थिरांक vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } अन्यथा जर (Ast_गाठ_CCall* स्थिरांक callp = VN_CAST(nodep, गाठ_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = पूर्वनिर्धारित;

    // METHODS
    VirtIfaceTriggers take_triggers() { परत std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    जर (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_वृक्ष("sched_vif", 0, dump_वृक्ष_Either_Level() >= 6);
    }
    परत triggers;
}

}  //namespace V3Sched
