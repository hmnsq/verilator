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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dead.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <queue>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

વર્ગ DeadVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_ગાંઠ* અચલ m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    તાર્કિક m_removable = ખોટું;  // Subject to dead removal
    તાર્કિક m_isVirtual = ખોટું;  // Virtual node, indirection

જાહેર:
    DeadVertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = મૂળભૂત;
    string dotShape() અચલ override {
        પાછા isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_ગાંઠ* nodep() અચલ VL_MT_STABLE { પાછા m_nodep; }
    string name() અચલ override VL_MT_STABLE {
        પાછા (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    તાર્કિક removable() અચલ { પાછા m_removable; }
    શૂન્ય removable(તાર્કિક flag) { m_removable = flag; }
    તાર્કિક isVirtual() અચલ { પાછા m_isVirtual; }
    શૂન્ય isVirtual(તાર્કિક flag) { m_isVirtual = flag; }
    uint64_t workPos() અચલ { પાછા m_workPos; }
    શૂન્ય workPos(uint64_t કિંમત) { m_workPos = કિંમત; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
વર્ગ DeadWorkList final {
    // MEMBERS
    સ્થિર uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
જાહેર:
    // METHODS
    તાર્કિક empty() અચલ { પાછા m_works.empty(); }
    શૂન્ય ધકેલો(DeadVertex* vtxp) {
        જો (vtxp->workPos()) પાછા;  // Already on list
        અચલ uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    શૂન્ય erase(DeadVertex* vtxp) {
        અચલ uint64_t id = vtxp->workPos();
        જો (!id) પાછા;
        UINFO(9, "Worklist.erase " << vtxp);
        અચલ auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_કાઢો_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        અચલ auto it = m_works.begin();
        DeadVertex* અચલ vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        પાછા vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

વર્ગ DeadGraph final : જાહેર V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    શૂન્ય newEdge(DeadVertex* fromp, DeadVertex* ટોચ) {
        નવું V3GraphEdge{આ, fromp, ટોચ, 1, ખોટું};
        જો (ટોચ->removable()) m_funcs.erase(ટોચ);  // Now has an input edge
    }
    શૂન્ય ધકેલો_Work_Maybe(DeadVertex* vtxp, તાર્કિક allowSize1) {
        જો (vtxp->removable()) {
            જો (VN_IS(vtxp->nodep(), ગાંઠ_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.ધકેલો(vtxp);
        }
    }

જાહેર:
    // METHODS
    DeadGraph() = મૂળભૂત;
    ~DeadGraph() override = મૂળભૂત;
    DeadVertex* findNewVertex(Ast_ગાંઠ* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        જો (!vtxp) {
            vtxp = નવું DeadVertex{આ, nodep};
            nodep->user2p(vtxp);
            ધકેલો_Work_Maybe(vtxp, ખોટું);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        પાછા vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_ગાંઠ* nodep) {
        અચલ auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        જો (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } નહીંતર {
            vtxp = નવું DeadVertex{આ, nodep};
            vtxp->removable(સાચું);
            vtxp->isVirtual(સાચું);
            ધકેલો_Work_Maybe(vtxp, ખોટું);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        પાછા vtxp;
    }
    શૂન્ય findNewRemovableVertex(Ast_ગાંઠ* nodep, તાર્કિક removable) {
        DeadVertex* અચલ vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        જો (removable && !vtxp->removable()) {
            vtxp->removable(સાચું);
            ધકેલો_Work_Maybe(vtxp, ખોટું);
        }
    }
    શૂન્ય delete_ગાંઠ_Vertex(Ast_ગાંઠ* nodep) {
        જો (DeadVertex* અચલ vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    શૂન્ય deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        માટે (અચલ V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* અચલ toVtxp = static_cast<DeadVertex*>(oedge.ટોચ());
            ધકેલો_Work_Maybe(toVtxp, સાચું);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        જો (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        જો (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(આ), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    શૂન્ય needs(Ast_ગાંઠ* nodep, Ast_ગાંઠ* parentp) {
        જો (parentp == nodep) પાછા;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* અચલ parentVtxp = findNewVertex(parentp);
        DeadVertex* અચલ ગાંઠ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << ગાંઠ_Vtxp);
        newEdge(parentVtxp, ગાંઠ_Vtxp);
    }
    શૂન્ય needsVirtual(Ast_ગાંઠ_FTask* nodep, Ast_ગાંઠ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        જો (parentp == nodep) પાછા;  // No need for tracking needs itself (recursion)
        DeadVertex* અચલ parentVtxp = findNewVertex(parentp);
        DeadVertex* અચલ virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    શૂન્ય funcVirtual(Ast_ગાંઠ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* અચલ virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* અચલ ગાંઠ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << ગાંઠ_Vtxp);
        newEdge(virtualVtxp, ગાંઠ_Vtxp);
    }
    તાર્કિક funcsEmpty() અચલ { પાછા m_funcs.empty(); }
    Ast_ગાંઠ* funcs_Get_કાઢો_Front() {
        DeadVertex* અચલ vtxp = m_funcs.get_કાઢો_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        જો (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            પાછા nullptr;  // Caller will search again
        }
        Ast_ગાંઠ* અચલ nodep = vtxp->nodep();
        પાછા nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

વર્ગ DeadVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_ગાંઠ_Assign*>;

    // STATE - across all visitors
    અચલ તાર્કિક m_elimUserVars;  // Allow removal of user's vars
    અચલ તાર્કિક m_elimDTypes;  // Allow removal of DTypes
    અચલ તાર્કિક m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_ગાંઠ*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_ગાંઠ_DType*, Ast_ગાંઠ_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    તાર્કિક m_sideEffect = ખોટું;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    તાર્કિક m_inAssign = ખોટું;  // Currently in an assign
    Ast_ગાંઠ_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_ગાંઠ* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    તાર્કિક m_inFTask = ખોટું;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    શૂન્ય deleting(Ast_ગાંઠ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_ગાંઠ_Vertex(nodep);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    શૂન્ય checkAll(Ast_ગાંઠ* nodep) {
        જો (Ast_ગાંઠ* અચલ subnodep = nodep->dtypep()) {
            જો (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        જો (Ast_ગાંઠ* અચલ subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    શૂન્ય checkVarRef(Ast_ગાંઠ_Var_Ref* nodep) અચલ {
        જો (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    શૂન્ય checkDType(Ast_ગાંઠ_DType* nodep) {
        જો (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ધકેલો_back(nodep);
        }
        જો (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep = VN_CAST(nodep, ગાંઠ_UOr_Struct_DType)) {
            જો (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        જો (Ast_ગાંઠ* અચલ subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        જો (Ast_ગાંઠ* અચલ subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    શૂન્ય needsTask(Ast_ગાંઠ_FTask* taskp, Ast_ગાંઠ* containerp) {
        જો (!taskp) પાછા;  // Unlinked
        જો (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } નહીંતર {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        જો (nodep->dead()) પાછા;
        જો (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        જો (AstClass* અચલ classp = VN_CAST(nodep, Class)) {
            જો (classp->extendsp()) classp->extendsp()->user1Inc();
            જો (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ધકેલો_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        માટે (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    શૂન્ય visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    શૂન્ય visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        જો (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        જો (m_modp->isConstPool()) nodep->user1Inc();
        જો (!nodep->is_ટોચ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ધકેલો_back(nodep);
        }
    }
    શૂન્ય visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ધકેલો_back(nodep);
        nodep->modp()->user1Inc();
    }

    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        જો (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        જો (nodep->varp()) nodep->varp()->user1Inc();
        જો (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        જો (!m_sideEffect && !nodep->isPure()) m_sideEffect = સાચું;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        જો (nodep->classOrPackagep()) {
            જો (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } નહીંતર {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    શૂન્ય visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    શૂન્ય visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->varp()) nodep->varp()->user1Inc();
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        જો (nodep->classOrPackagep()) {
            જો (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } નહીંતર {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    શૂન્ય visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        જો (nodep->classOrPackagep()) {
            જો (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } નહીંતર {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        જો (nodep->classp()) nodep->classp()->user1Inc();
    }
    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        જો (nodep->modportp()) {
            જો (m_elimCells) {
                nodep->modportp(nullptr);
            } નહીંતર {
                nodep->modportp()->user1Inc();
            }
        }
        જો (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    શૂન્ય visit(Ast_ગાંઠ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->classOrPackagep()) {
            જો (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } નહીંતર {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        જો (nodep->varp()) nodep->varp()->user1Inc();
        જો (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        જો (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    શૂન્ય visit(AstModport* nodep) override {
        iterateChildren(nodep);
        જો (m_elimCells) {
            જો (!nodep->varsp()) {
                deleting(nodep);
                પાછા;
            }
        }
        checkAll(nodep);
    }
    શૂન્ય visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    શૂન્ય visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ધકેલો_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        જો (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->scopep()) nodep->scopep()->user1Inc();
        જો (mightElimVar(nodep->varp())) m_vscsp.ધકેલો_back(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        જો (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        જો (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        જો (mightElimVar(nodep)) {
            m_varsp.ધકેલો_back(nodep);
        } નહીંતર {
            જો (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        અચલ તાર્કિક assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = સાચું;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* અચલ varrefp = VN_CAST(nodep->lhsp(), VarRef);
            જો (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } નહીંતર {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        જો (assignInAssign) m_sideEffect = સાચું;  // Parent assign shouldn't optimize
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        અચલ તાર્કિક removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = સાચું;
        iterateChildren(nodep);
        checkAll(nodep);
        જો (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        જો (!removable) {
            જો (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        જો (nodep->classOrPackagep()) {
            જો (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } નહીંતર {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    શૂન્ય visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (!m_sideEffect && !nodep->isPure()) m_sideEffect = સાચું;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    શૂન્ય deadCheckTypedefs() {
        માટે (AstTypedef* typedefp : m_typedefsp) {
            જો (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                ચાલુ;
            }
            checkAll(typedefp);
        }
    }
    તાર્કિક shouldDeleteTypedef(અચલ AstTypedef* typedefp) અચલ {
        જો (અચલ auto* અચલ structp = VN_CAST(typedefp->subDTypep(), ગાંઠ_UOr_Struct_DType)) {
            જો (structp->user1() && !structp->packed()) પાછા ખોટું;
        }
        પાછા m_elimCells && !typedefp->attrPublic();
    }

    શૂન્ય deadCheckTasks() {
        જ્યારે (!m_graph.funcsEmpty()) {
            Ast_ગાંઠ* અચલ nodep = m_graph.funcs_Get_કાઢો_Front();
            જો (!nodep) ચાલુ;
            UINFO(9, "Dead " << nodep);
            જો (Ast_ગાંઠ_FTask* અચલ taskp = VN_CAST(nodep, ગાંઠ_FTask)) {
                જો (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } નહીંતર જો (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } નહીંતર {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    શૂન્ય deadCheckDemote() {
        માટે (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* અચલ vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            જો (!vtxp->outSize1()) ચાલુ;
            જો (!vtxp->isVirtual()) ચાલુ;
            માટે (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_ગાંઠ* અચલ nodep = edge.ટોચ()->as<DeadVertex>()->nodep();
                Ast_ગાંઠ_FTask* અચલ funcp = VN_AS(nodep, ગાંઠ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                જો (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(ખોટું);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    શૂન્ય deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        માટે (તાર્કિક retry = સાચું; retry;) {
            retry = ખોટું;
            Ast_ગાંઠ_Module* nextmodp;
            માટે (Ast_ગાંઠ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), ગાંઠ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                અચલ તાર્કિક keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                જો (modp->dead() || (!modp->is_ટોચ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    જો (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](અચલ AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    જો (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = સાચું;
                }
            }
        }
    }
    તાર્કિક mightElimVar(અચલ AstVar* nodep) અચલ {
        જો (nodep->isSigPublic()) પાછા ખોટું;  // Can't elim publics!
        જો (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) પાછા ખોટું;
        જો (nodep->isTemp() && !nodep->isTrace()) પાછા સાચું;
        પાછા m_elimUserVars;  // Post-Trace can kill most anything
    }

    શૂન્ય deadCheckScope() {
        માટે (તાર્કિક retry = સાચું; retry;) {
            retry = ખોટું;
            માટે (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* અચલ scp = *it;
                જો (!scp) ચાલુ;
                જો (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    જો (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = સાચું;
                }
            }
        }
    }

    શૂન્ય deadCheckCells() {
        માટે (AstCell* cellp : m_cellsp) {
            જો (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    શૂન્ય deadCheckClasses() {
        માટે (તાર્કિક retry = સાચું; retry;) {
            retry = ખોટું;
            માટે (auto& itr : m_classesp) {
                જો (AstClass* અચલ nodep = itr) {  // nullptr if deleted earlier
                    જો (nodep->user1() == 0) {
                        જો (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        જો (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = સાચું;
                    }
                }
            }
        }
    }

    શૂન્ય deadCheckVar() {
        // Delete any unused varscopes
        માટે (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            જો (vscp->varp()->user2()) ચાલુ;
            જો (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                અચલ std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                માટે (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_ગાંઠ_Assign* અચલ assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                જો (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        માટે (તાર્કિક retry = સાચું; retry;) {
            retry = ખોટું;
            માટે (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* અચલ varp = *it;
                જો (!varp) ચાલુ;
                જો (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    જો (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = સાચું;
                }
            }
        }
        માટે (std::vector<Ast_ગાંઠ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            જો ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ classp
                    = VN_CAST((*it), ગાંઠ_UOr_Struct_DType)) {
                    તાર્કિક cont = સાચું;
                    માટે (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        જો (memberp->user1() != 0) {
                            cont = ખોટું;
                            તોડો;
                        }
                    }
                    જો (!cont) ચાલુ;
                }
                deleting(*it);
            }
        }
    }

જાહેર:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, તાર્કિક elimUserVars, તાર્કિક elimDTypes, તાર્કિક elimScopes,
                તાર્કિક elimCells, તાર્કિક elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        જો (AstVarScope* અચલ vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        માટે (auto& itr : m_dtypePkgsp) {
            જો (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        જો (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", ખોટું);

        જો (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        જો (elimScopes) deadCheckScope();
        જો (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        જો (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_વૃક્ષ();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::addStatSum("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::addStatSum("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::addStatSum("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::addStatSum("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

શૂન્ય V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, ખોટું, ખોટું, ખોટું, ખોટું, ખોટું};
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("deadModules", 0, dump_વૃક્ષ_Either_Level() >= 6);
}

શૂન્ય V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ખોટું, સાચું, ખોટું, ખોટું, સાચું}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("deadDtypes", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ખોટું, સાચું, સાચું, ખોટું, ખોટું}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("deadDtypesScoped", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, સાચું, સાચું, ખોટું, સાચું, સાચું}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("deadAll", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, સાચું, સાચું, સાચું, સાચું, સાચું}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("deadAllScoped", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
