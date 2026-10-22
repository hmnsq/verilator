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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3AstNodeExpr.h"
#അന്തര്ഭാവ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

നാമാകാശ {

വർഗ്ഗം VirtIfaceVisitor final : പൊതു VNVisitor {
സ്വകാര്യ:
    // STATE
    using IfaceMember = std::pair<സ്ഥിരം AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<സ്ഥിരം AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    സ്ഥിതം തർക്കം writesToVirtIface(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) {
        തിരികെ nodep->exists([](സ്ഥിരം AstMemberSel* സ്ഥിരം memberSelp) {
            എങ്കിൽ (!memberSelp->access().isWriteOrRW()) തിരികെ അസത്യം;
            AstIfaceRefDType* സ്ഥിരം dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            തിരികെ dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    ശൂന്യം visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* സ്ഥിരം dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            എങ്കിൽ (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCMethodCall* nodep) override {
        എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            എങ്കിൽ (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } അല്ലെങ്കിൽ {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (varp->isTemp()) തിരികെ;
        m_vscpsByVar[varp].തള്ളുക_back(nodep);
        എങ്കിൽ (സ്ഥിരം AstIface* സ്ഥിരം ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        എങ്കിൽ (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(അസത്യം); });
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    ശൂന്യം addCandidateVscp(സ്ഥിരം AstIface* സ്ഥിരം ifacep, AstVarScope* സ്ഥിരം vscp) {
        സ്ഥിരം IfaceMember member{ifacep, vscp->varp()->name()};
        എങ്കിൽ (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].തള്ളുക_back(vscp);
    }

    ശൂന്യം addReachableIfaceCallable(AstIface* സ്ഥിരം ifacep, AstCFunc* സ്ഥിരം funcp) {
        സ്ഥിരം IfaceCallable callable{ifacep, funcp};
        എങ്കിൽ (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.തള്ളുക_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    ശൂന്യം buildTriggers() {
        വേണ്ടി (സ്ഥിരം auto& written : m_vifWrittenMembers) {
            സ്ഥിരം auto it = m_candidateVscps.find(written);
            എങ്കിൽ (it == m_candidateVscps.end()) തുടരുക;
            വേണ്ടി (AstVarScope* സ്ഥിരം vscp : it->second) {
                സ്ഥിരം AstIface* സ്ഥിരം ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

പൊതു:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        വേണ്ടി (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            സ്ഥിരം IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([ഇത്, callable](Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
                എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* സ്ഥിരം varp = refp->varp();
                    എങ്കിൽ (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        തിരികെ;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    സ്ഥിരം auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    വേണ്ടി (AstVarScope* സ്ഥിരം vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_CCall* സ്ഥിരം callp = VN_CAST(nodep, കണ്ണി_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = സ്ഥിരസ്ഥിതി;

    // METHODS
    VirtIfaceTriggers take_triggers() { തിരികെ std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    എങ്കിൽ (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_വൃക്ഷം("sched_vif", 0, dump_വൃക്ഷം_Either_Level() >= 6);
    }
    തിരികെ triggers;
}

}  //namespace V3Sched
