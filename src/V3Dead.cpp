// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dead code elimination
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
// DEAD TRANSFORMATIONS:
//      Remove any unreferenced modules
//      Remove any unreferenced variables
//
// TODO: A graph would make the process of circular and interlinked
// dependencies easier to resolve.
// NOTE: If redo this, consider using maybePointedTo()/broken() ish scheme
// instead of needing as many visitors.
//
// The following nodes have package pointers and are cleaned up here:
// AstRefDType, AstEnumItemRef, AstNodeVarRef, AstNodeFTask
// These have packagep but will not exist at this stage
// AstPackageImport, AstDot, AstClassOrPackageRef
//
// Note on packagep: After the V3Scope/V3LinkDotScoped stage, package links
// are no longer used, but their presence prevents us from removing empty
// packages. As the links as no longer used after V3Scope, we remove them
// here after scoping to allow more dead node removal.
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dead.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <queue>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

ਵਰਗ Dead_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Dead_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    ਤਾਰਕਿਕ m_removable = ਝੂਠ;  // Subject to dead removal
    ਤਾਰਕਿਕ m_isVirtual = ਝੂਠ;  // Virtual node, indirection

ਜਨਤਕ:
    Dead_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Dead_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string dotShape() ਅਚਲ override {
        ਵਾਪਸ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_ਗੰਢ* nodep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_nodep; }
    string name() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvt_ਮੰਜ਼ਿਲ_Hex(nodep()) + ' ' + nodep()->name();
    }
    ਤਾਰਕਿਕ removable() ਅਚਲ { ਵਾਪਸ m_removable; }
    ਖਾਲੀ removable(ਤਾਰਕਿਕ flag) { m_removable = flag; }
    ਤਾਰਕਿਕ isVirtual() ਅਚਲ { ਵਾਪਸ m_isVirtual; }
    ਖਾਲੀ isVirtual(ਤਾਰਕਿਕ flag) { m_isVirtual = flag; }
    uint64_t workPos() ਅਚਲ { ਵਾਪਸ m_workPos; }
    ਖਾਲੀ workPos(uint64_t ਮੁੱਲ) { m_workPos = ਮੁੱਲ; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
ਵਰਗ DeadWorkList final {
    // MEMBERS
    ਸਥਿਰ uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, Dead_ਸ਼ੀਰਸ਼*> m_ਚੱਲਦਾ;  // Work list by sequence
ਜਨਤਕ:
    // METHODS
    ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_ਚੱਲਦਾ.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਖਾਲੀ ਧੱਕੋ(Dead_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (vtxp->workPos()) ਵਾਪਸ;  // Already on list
        ਅਚਲ uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_ਚੱਲਦਾ.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    ਖਾਲੀ erase(Dead_ਸ਼ੀਰਸ਼* vtxp) {
        ਅਚਲ uint64_t id = vtxp->workPos();
        ਜੇ (!id) ਵਾਪਸ;
        UINFO(9, "Worklist.erase " << vtxp);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਚੱਲਦਾ.ਲੱਭੋ(id);
        UASSERT_OBJ(it != m_ਚੱਲਦਾ.ਸਿਰਾ(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_ਚੱਲਦਾ.erase(it);
        vtxp->workPos(0);
    }
    Dead_ਸ਼ੀਰਸ਼* get_ਕੱਢੋ_Front() {
        UDEBUGONLY(UASSERT(!ਖਾਲੀ_ਥਾਵਾਂ(), "Front invalid to call on empty list"););
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਚੱਲਦਾ.begin();
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = it->ਦੂਜਾ;
        m_ਚੱਲਦਾ.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        ਵਾਪਸ vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

ਵਰਗ Dead_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, Dead_ਸ਼ੀਰਸ਼*> m_virtualVtxsp;

    ਖਾਲੀ new_ਕਿਨਾਰਾ(Dead_ਸ਼ੀਰਸ਼* fromp, Dead_ਸ਼ੀਰਸ਼* ਸਿਖਰ) {
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, fromp, ਸਿਖਰ, 1, ਝੂਠ};
        ਜੇ (ਸਿਖਰ->removable()) m_funcs.erase(ਸਿਖਰ);  // Now has an input edge
    }
    ਖਾਲੀ ਧੱਕੋ_Work_Maybe(Dead_ਸ਼ੀਰਸ਼* vtxp, ਤਾਰਕਿਕ allowSize1) {
        ਜੇ (vtxp->removable()) {
            ਜੇ (VN_IS(vtxp->nodep(), ਗੰਢ_FTask)
                && (vtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ() || (allowSize1 && vtxp->ਵਿੱਚ_Size1())))
                m_funcs.ਧੱਕੋ(vtxp);
        }
    }

ਜਨਤਕ:
    // METHODS
    Dead_ਗ੍ਰਾਫ() = ਮੂਲ;
    ~Dead_ਗ੍ਰਾਫ() override = ਮੂਲ;
    Dead_ਸ਼ੀਰਸ਼* ਲੱਭੋ_New_ਸ਼ੀਰਸ਼(Ast_ਗੰਢ* nodep) {
        Dead_ਸ਼ੀਰਸ਼* vtxp = nodep->user2u().ਮੰਜ਼ਿਲ<Dead_ਸ਼ੀਰਸ਼*>();
        ਜੇ (!vtxp) {
            vtxp = ਨਵਾਂ Dead_ਸ਼ੀਰਸ਼{ਇਹ, nodep};
            nodep->user2p(vtxp);
            ਧੱਕੋ_Work_Maybe(vtxp, ਝੂਠ);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ਵਾਪਸ vtxp;
    }
    Dead_ਸ਼ੀਰਸ਼* ਲੱਭੋ_Virtual_ਸ਼ੀਰਸ਼(Ast_ਗੰਢ* nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_virtualVtxsp.ਲੱਭੋ(nodep->name());
        Dead_ਸ਼ੀਰਸ਼* vtxp;
        ਜੇ (it != m_virtualVtxsp.ਸਿਰਾ()) {
            vtxp = it->ਦੂਜਾ;
        } ਨਹੀਂਤਾਂ {
            vtxp = ਨਵਾਂ Dead_ਸ਼ੀਰਸ਼{ਇਹ, nodep};
            vtxp->removable(ਸੱਚ);
            vtxp->isVirtual(ਸੱਚ);
            ਧੱਕੋ_Work_Maybe(vtxp, ਝੂਠ);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ਵਾਪਸ vtxp;
    }
    ਖਾਲੀ ਲੱਭੋ_New_Removable_ਸ਼ੀਰਸ਼(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ removable) {
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਲੱਭੋ_New_ਸ਼ੀਰਸ਼(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ਜੇ (removable && !vtxp->removable()) {
            vtxp->removable(ਸੱਚ);
            ਧੱਕੋ_Work_Maybe(vtxp, ਝੂਠ);
        }
    }
    ਖਾਲੀ delete_ਗੰਢ_ਸ਼ੀਰਸ਼(Ast_ਗੰਢ* nodep) {
        ਜੇ (Dead_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = nodep->user2u().ਮੰਜ਼ਿਲ<Dead_ਸ਼ੀਰਸ਼*>()) delete_ਸ਼ੀਰਸ਼(vtxp);
    }
    ਖਾਲੀ delete_ਸ਼ੀਰਸ਼(Dead_ਸ਼ੀਰਸ਼* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& oedge : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            Dead_ਸ਼ੀਰਸ਼* ਅਚਲ ਮੰਜ਼ਿਲ_Vtxp = static_cast<Dead_ਸ਼ੀਰਸ਼*>(oedge.ਸਿਖਰ());
            ਧੱਕੋ_Work_Maybe(ਮੰਜ਼ਿਲ_Vtxp, ਸੱਚ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ਜੇ (!vtxp->isVirtual()) vtxp->nodep()->user2p(ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(ਇਹ), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    ਖਾਲੀ needs(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* parentp) {
        ਜੇ (parentp == nodep) ਵਾਪਸ;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਾਪਾ_Vtxp = ਲੱਭੋ_New_ਸ਼ੀਰਸ਼(parentp);
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ ਗੰਢ_Vtxp = ਲੱਭੋ_New_ਸ਼ੀਰਸ਼(nodep);
        UINFO(9, "Edge need " << ਮਾਪਾ_Vtxp << " -> " << ਗੰਢ_Vtxp);
        new_ਕਿਨਾਰਾ(ਮਾਪਾ_Vtxp, ਗੰਢ_Vtxp);
    }
    ਖਾਲੀ needsVirtual(Ast_ਗੰਢ_FTask* nodep, Ast_ਗੰਢ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ਜੇ (parentp == nodep) ਵਾਪਸ;  // No need for tracking needs itself (recursion)
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਾਪਾ_Vtxp = ਲੱਭੋ_New_ਸ਼ੀਰਸ਼(parentp);
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ virtualVtxp = ਲੱਭੋ_Virtual_ਸ਼ੀਰਸ਼(nodep);
        UINFO(9, "Edge needVirtual " << ਮਾਪਾ_Vtxp << " -> " << virtualVtxp);
        new_ਕਿਨਾਰਾ(ਮਾਪਾ_Vtxp, virtualVtxp);
    }
    ਖਾਲੀ funcVirtual(Ast_ਗੰਢ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ virtualVtxp = ਲੱਭੋ_Virtual_ਸ਼ੀਰਸ਼(nodep);
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ ਗੰਢ_Vtxp = ਲੱਭੋ_New_ਸ਼ੀਰਸ਼(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << ਗੰਢ_Vtxp);
        new_ਕਿਨਾਰਾ(virtualVtxp, ਗੰਢ_Vtxp);
    }
    ਤਾਰਕਿਕ funcs_ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_funcs.ਖਾਲੀ_ਥਾਵਾਂ(); }
    Ast_ਗੰਢ* funcs_Get_ਕੱਢੋ_Front() {
        Dead_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = m_funcs.get_ਕੱਢੋ_Front();
        UASSERT_OBJ(vtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ਜੇ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            delete_ਸ਼ੀਰਸ਼(vtxp);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Caller will search again
        }
        Ast_ਗੰਢ* ਅਚਲ nodep = vtxp->nodep();
        ਵਾਪਸ nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

ਵਰਗ DeadVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;  // For usage information see DeadGraph

    // TYPES
    ਵਰਤੋਂ ਸੌਂਪੋ_Map = std::multimap<AstVarScope*, Ast_ਗੰਢ_ਸੌਂਪੋ*>;

    // STATE - across all visitors
    ਅਚਲ ਤਾਰਕਿਕ m_elimUserVars;  // Allow removal of user's vars
    ਅਚਲ ਤਾਰਕਿਕ m_elimDTypes;  // Allow removal of DTypes
    ਅਚਲ ਤਾਰਕਿਕ m_elimCells;  // Allow removal of Cells

    Dead_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::ਸਦਿਸ਼<AstVar*> m_varsp;
    std::ਸਦਿਸ਼<Ast_ਗੰਢ*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_ਗੰਢ_DType*, Ast_ਗੰਢ_Module*> m_dtypePkgsp;  // Data type's containing package
    std::ਸਦਿਸ਼<AstVarScope*> m_vscsp;
    std::ਸਦਿਸ਼<AstScope*> m_scopesp;
    std::ਸਦਿਸ਼<AstCell*> m_cellsp;
    std::ਸਦਿਸ਼<AstClass*> m_classesp;
    std::ਸਦਿਸ਼<AstTypedef*> m_typedefsp;
    ਸੌਂਪੋ_Map m_ਸੌਂਪੋ_Map;  // List of all simple assignments for each variable
    ਤਾਰਕਿਕ m_sideEffect = ਝੂਠ;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    ਤਾਰਕਿਕ m_ਵਿੱਚ_ਸੌਂਪੋ = ਝੂਠ;  // Currently in an assign
    Ast_ਗੰਢ_DType* m_curDTypep = ਖਾਲੀ_ਸੂਚਕ;  // Current NodeDType
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_Foreach_ਸਿਰਲੇਖ* m_foreachHeaderp = ਖਾਲੀ_ਸੂਚਕ;  // Current foreach header
    Ast_ਗੰਢ* m_containingFTaskRefp = ਖਾਲੀ_ਸੂਚਕ;  // Parent of ftaskref (e.g. task/module)
    ਤਾਰਕਿਕ m_ਵਿੱਚ_FTask = ਝੂਠ;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    ਖਾਲੀ deleting(Ast_ਗੰਢ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_ਗ੍ਰਾਫ.delete_ਗੰਢ_ਸ਼ੀਰਸ਼(nodep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    ਖਾਲੀ checkAll(Ast_ਗੰਢ* nodep) {
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ subnodep = nodep->dtypep()) {
            ਜੇ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ subnodep = nodep->get_ਬੱਚਾ_DTypep()) subnodep->user1Inc();
    }
    ਖਾਲੀ checkVarRef(Ast_ਗੰਢ_Var_Ref* nodep) ਅਚਲ {
        ਜੇ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ checkDType(Ast_ਗੰਢ_DType* nodep) {
        ਜੇ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ਧੱਕੋ_ਪਿੱਠ(nodep);
        }
        ਜੇ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep = VN_CAST(nodep, ਗੰਢ_UOr_Struct_DType)) {
            ਜੇ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    ਖਾਲੀ needsTask(Ast_ਗੰਢ_FTask* taskp, Ast_ਗੰਢ* containerp) {
        ਜੇ (!taskp) ਵਾਪਸ;  // Unlinked
        ਜੇ (taskp->isVirtual()) {
            m_ਗ੍ਰਾਫ.needsVirtual(taskp, containerp);
        } ਨਹੀਂਤਾਂ {
            m_ਗ੍ਰਾਫ.needs(taskp, containerp);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ਜੇ (nodep->dead()) ਵਾਪਸ;
        ਜੇ (nodep->mod_ਜਨਤਕ()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) {
            ਜੇ (classp->extendsp()) classp->extendsp()->user1Inc();
            ਜੇ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ਧੱਕੋ_ਪਿੱਠ(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        ਲਈ (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ਜੇ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ਜੇ (m_modp->isConstPool()) nodep->user1Inc();
        ਜੇ (!nodep->is_ਸਿਖਰ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ਧੱਕੋ_ਪਿੱਠ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ਧੱਕੋ_ਪਿੱਠ(nodep);
        nodep->modp()->user1Inc();
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ਜੇ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ਜੇ (nodep->varp()) nodep->varp()->user1Inc();
        ਜੇ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!m_sideEffect && !nodep->is_ਸ਼ੁੱਧ()) m_sideEffect = ਸੱਚ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ਜੇ (nodep->classOrPackagep()) {
            ਜੇ (m_elimCells) {
                nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->varp()) nodep->varp()->user1Inc();
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ਜੇ (nodep->classOrPackagep()) {
            ਜੇ (m_elimCells) {
                nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ਜੇ (nodep->classOrPackagep()) {
            ਜੇ (m_elimCells) {
                nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ਜੇ (nodep->classp()) nodep->classp()->user1Inc();
    }
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ਜੇ (nodep->modportp()) {
            ਜੇ (m_elimCells) {
                nodep->modportp(ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                nodep->modportp()->user1Inc();
            }
        }
        ਜੇ (nodep->iface_ਰਾਹੀਂ_Cellp()) nodep->iface_ਰਾਹੀਂ_Cellp()->user1Inc();
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->classOrPackagep()) {
            ਜੇ (m_elimCells) {
                nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->varp()) nodep->varp()->user1Inc();
        ਜੇ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstModport* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_elimCells) {
            ਜੇ (!nodep->varsp()) {
                deleting(nodep);
                ਵਾਪਸ;
            }
        }
        checkAll(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Foreach_ਸਿਰਲੇਖ* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ਧੱਕੋ_ਪਿੱਠ(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ਜੇ (nodep->attr_ਜਨਤਕ() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->scopep()) nodep->scopep()->user1Inc();
        ਜੇ (mightElimVar(nodep->varp())) m_vscsp.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ਜੇ (m_ਵਿੱਚ_FTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ਜੇ (mightElimVar(nodep)) {
            m_varsp.ਧੱਕੋ_ਪਿੱਠ(nodep);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ਅਚਲ ਤਾਰਕਿਕ ਸੌਂਪੋ_ਵਿੱਚ_ਸੌਂਪੋ = m_ਵਿੱਚ_ਸੌਂਪੋ;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_ਵਿੱਚ_ਸੌਂਪੋ);
            VL_RESTORER(m_sideEffect);
            m_ਵਿੱਚ_ਸੌਂਪੋ = ਸੱਚ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ਜੇ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_ਸੌਂਪੋ_Map.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ਨਹੀਂਤਾਂ {  // Track like any other statement
                iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ਜੇ (ਸੌਂਪੋ_ਵਿੱਚ_ਸੌਂਪੋ) m_sideEffect = ਸੱਚ;  // Parent assign shouldn't optimize
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        ਅਚਲ ਤਾਰਕਿਕ removable = !(nodep->task_ਜਨਤਕ() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->class_ਵਿਧੀ()));
        m_ਗ੍ਰਾਫ.ਲੱਭੋ_New_Removable_ਸ਼ੀਰਸ਼(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_ਵਿੱਚ_FTask);
        m_containingFTaskRefp = nodep;
        m_ਵਿੱਚ_FTask = ਸੱਚ;
        iterateChildren(nodep);
        checkAll(nodep);
        ਜੇ (nodep->isVirtual()) m_ਗ੍ਰਾਫ.funcVirtual(nodep);
        ਜੇ (!removable) {
            ਜੇ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ਜੇ (nodep->classOrPackagep()) {
            ਜੇ (m_elimCells) {
                nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    ਖਾਲੀ ਦੇਖੋ(Ast_Clocking_ਵਸਤੂ* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!m_sideEffect && !nodep->is_ਸ਼ੁੱਧ()) m_sideEffect = ਸੱਚ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    ਖਾਲੀ deadCheckTypedefs() {
        ਲਈ (AstTypedef* typedefp : m_typedefsp) {
            ਜੇ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                ਜਾਰੀ;
            }
            checkAll(typedefp);
        }
    }
    ਤਾਰਕਿਕ shouldDeleteTypedef(ਅਚਲ AstTypedef* typedefp) ਅਚਲ {
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ structp = VN_CAST(typedefp->subDTypep(), ਗੰਢ_UOr_Struct_DType)) {
            ਜੇ (structp->user1() && !structp->packed()) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ m_elimCells && !typedefp->attr_ਜਨਤਕ();
    }

    ਖਾਲੀ deadCheckTasks() {
        ਜਦੋਂ (!m_ਗ੍ਰਾਫ.funcs_ਖਾਲੀ_ਥਾਵਾਂ()) {
            Ast_ਗੰਢ* ਅਚਲ nodep = m_ਗ੍ਰਾਫ.funcs_Get_ਕੱਢੋ_Front();
            ਜੇ (!nodep) ਜਾਰੀ;
            UINFO(9, "Dead " << nodep);
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ taskp = VN_CAST(nodep, ਗੰਢ_FTask)) {
                ਜੇ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ਨਹੀਂਤਾਂ ਜੇ (taskp->class_ਵਿਧੀ()) {
                    ++m_statFTasksMDeadified;
                } ਨਹੀਂਤਾਂ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    ਖਾਲੀ deadCheckDemote() {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& gvtx : m_ਗ੍ਰਾਫ.vertices()) {
            Dead_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = gvtx.cast<Dead_ਸ਼ੀਰਸ਼>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ਜੇ (!vtxp->ਬਾਹਰਲਾ_Size1()) ਜਾਰੀ;
            ਜੇ (!vtxp->isVirtual()) ਜਾਰੀ;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {  // Always a single one
                Ast_ਗੰਢ* ਅਚਲ nodep = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<Dead_ਸ਼ੀਰਸ਼>()->nodep();
                Ast_ਗੰਢ_FTask* ਅਚਲ funcp = VN_AS(nodep, ਗੰਢ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ਜੇ (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(ਝੂਠ);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    ਖਾਲੀ deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        ਲਈ (ਤਾਰਕਿਕ retry = ਸੱਚ; retry;) {
            retry = ਝੂਠ;
            Ast_ਗੰਢ_Module* nextmodp;
            ਲਈ (Ast_ਗੰਢ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), ਗੰਢ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ਅਚਲ ਤਾਰਕਿਕ keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ਜੇ (modp->dead() || (!modp->is_ਸਿਖਰ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ਜੇ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ਅਚਲ AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ਜੇ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(ਖਾਲੀ_ਸੂਚਕ);
                    deleting(modp);
                    retry = ਸੱਚ;
                }
            }
        }
    }
    ਤਾਰਕਿਕ mightElimVar(ਅਚਲ AstVar* nodep) ਅਚਲ {
        ਜੇ (nodep->is_Sig_ਜਨਤਕ()) ਵਾਪਸ ਝੂਠ;  // Can't elim publics!
        ਜੇ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->is_ਆਰਜ਼ੀ() && !nodep->isTrace()) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ m_elimUserVars;  // Post-Trace can kill most anything
    }

    ਖਾਲੀ deadCheckScope() {
        ਲਈ (ਤਾਰਕਿਕ retry = ਸੱਚ; retry;) {
            retry = ਝੂਠ;
            ਲਈ (std::ਸਦਿਸ਼<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.ਸਿਰਾ();
                 ++it) {
                AstScope* ਅਚਲ scp = *it;
                ਜੇ (!scp) ਜਾਰੀ;
                ਜੇ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ਜੇ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = ਖਾਲੀ_ਸੂਚਕ;
                    retry = ਸੱਚ;
                }
            }
        }
    }

    ਖਾਲੀ deadCheckCells() {
        ਲਈ (AstCell* cellp : m_cellsp) {
            ਜੇ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    ਖਾਲੀ deadCheckClasses() {
        ਲਈ (ਤਾਰਕਿਕ retry = ਸੱਚ; retry;) {
            retry = ਝੂਠ;
            ਲਈ (ਸਵੈਚਾਲਿਤ& itr : m_classesp) {
                ਜੇ (AstClass* ਅਚਲ nodep = itr) {  // nullptr if deleted earlier
                    ਜੇ (nodep->user1() == 0) {
                        ਜੇ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ਜੇ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = ਖਾਲੀ_ਸੂਚਕ;
                        retry = ਸੱਚ;
                    }
                }
            }
        }
    }

    ਖਾਲੀ deadCheckVar() {
        // Delete any unused varscopes
        ਲਈ (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ਜੇ (vscp->varp()->user2()) ਜਾਰੀ;
            ਜੇ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ਅਚਲ std::pair<ਸੌਂਪੋ_Map::iterator, ਸੌਂਪੋ_Map::iterator> eqrange
                    = m_ਸੌਂਪੋ_Map.equal_range(vscp);
                ਲਈ (ਸੌਂਪੋ_Map::iterator itr = eqrange.ਪਹਿਲਾ; itr != eqrange.ਦੂਜਾ; ++itr) {
                    Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = itr->ਦੂਜਾ;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ਜੇ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        ਲਈ (ਤਾਰਕਿਕ retry = ਸੱਚ; retry;) {
            retry = ਝੂਠ;
            ਲਈ (std::ਸਦਿਸ਼<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.ਸਿਰਾ(); ++it) {
                AstVar* ਅਚਲ varp = *it;
                ਜੇ (!varp) ਜਾਰੀ;
                ਜੇ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ਜੇ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = ਖਾਲੀ_ਸੂਚਕ;
                    retry = ਸੱਚ;
                }
            }
        }
        ਲਈ (std::ਸਦਿਸ਼<Ast_ਗੰਢ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.ਸਿਰਾ();
             ++it) {
            ਜੇ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ classp
                    = VN_CAST((*it), ਗੰਢ_UOr_Struct_DType)) {
                    ਤਾਰਕਿਕ cont = ਸੱਚ;
                    ਲਈ (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ਜੇ (memberp->user1() != 0) {
                            cont = ਝੂਠ;
                            ਰੋਕੋ;
                        }
                    }
                    ਜੇ (!cont) ਜਾਰੀ;
                }
                deleting(*it);
            }
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, ਤਾਰਕਿਕ elimUserVars, ਤਾਰਕਿਕ elimDTypes, ਤਾਰਕਿਕ elimScopes,
                ਤਾਰਕਿਕ elimCells, ਤਾਰਕਿਕ elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        ਜੇ (AstVarScope* ਅਚਲ vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : m_dtypePkgsp) {
            ਜੇ (itr.ਪਹਿਲਾ->user1()) itr.ਦੂਜਾ->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_ਗ੍ਰਾਫ.remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9 || debug() >= 9)
            m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("dead_graph", ਝੂਠ);

        ਜੇ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ਜੇ (elimScopes) deadCheckScope();
        ਜੇ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ਜੇ (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_ਰੁੱਖ();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

ਖਾਲੀ V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, ਝੂਠ, ਝੂਠ, ਝੂਠ, ਝੂਠ, ਝੂਠ};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("deadModules", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}

ਖਾਲੀ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ਝੂਠ, ਸੱਚ, ਝੂਠ, ਝੂਠ, ਸੱਚ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("deadDtypes", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ਝੂਠ, ਸੱਚ, ਸੱਚ, ਝੂਠ, ਝੂਠ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("deadDtypesScoped", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ਸੱਚ, ਸੱਚ, ਝੂਠ, ਸੱਚ, ਸੱਚ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("deadAll", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ਸੱਚ, ਸੱਚ, ਸੱਚ, ਸੱਚ, ਸੱਚ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("deadAllScoped", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
