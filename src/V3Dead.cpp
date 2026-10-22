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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dead.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <queue>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

வகுப்பு DeadVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_முனை* மாறிலி m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    தருக்க m_removable = பொய்;  // Subject to dead removal
    தருக்க m_isVirtual = பொய்;  // Virtual node, indirection

பொது:
    DeadVertex(V3Graph* graphp, Ast_முனை* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = இயல்பு;
    string dotShape() மாறிலி override {
        திரும்பு isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_முனை* nodep() மாறிலி VL_MT_STABLE { திரும்பு m_nodep; }
    string name() மாறிலி override VL_MT_STABLE {
        திரும்பு (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    தருக்க removable() மாறிலி { திரும்பு m_removable; }
    வெறுமை removable(தருக்க flag) { m_removable = flag; }
    தருக்க isVirtual() மாறிலி { திரும்பு m_isVirtual; }
    வெறுமை isVirtual(தருக்க flag) { m_isVirtual = flag; }
    uint64_t workPos() மாறிலி { திரும்பு m_workPos; }
    வெறுமை workPos(uint64_t மதிப்பு) { m_workPos = மதிப்பு; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
வகுப்பு DeadWorkList final {
    // MEMBERS
    நிலையான uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
பொது:
    // METHODS
    தருக்க empty() மாறிலி { திரும்பு m_works.empty(); }
    வெறுமை தள்ளு(DeadVertex* vtxp) {
        எனில் (vtxp->workPos()) திரும்பு;  // Already on list
        மாறிலி uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    வெறுமை erase(DeadVertex* vtxp) {
        மாறிலி uint64_t id = vtxp->workPos();
        எனில் (!id) திரும்பு;
        UINFO(9, "Worklist.erase " << vtxp);
        மாறிலி auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_எடு_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        மாறிலி auto it = m_works.begin();
        DeadVertex* மாறிலி vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        திரும்பு vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

வகுப்பு DeadGraph final : பொது V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    வெறுமை newEdge(DeadVertex* fromp, DeadVertex* உச்சி) {
        புதிய V3GraphEdge{இது, fromp, உச்சி, 1, பொய்};
        எனில் (உச்சி->removable()) m_funcs.erase(உச்சி);  // Now has an input edge
    }
    வெறுமை தள்ளு_Work_Maybe(DeadVertex* vtxp, தருக்க allowSize1) {
        எனில் (vtxp->removable()) {
            எனில் (VN_IS(vtxp->nodep(), முனை_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.தள்ளு(vtxp);
        }
    }

பொது:
    // METHODS
    DeadGraph() = இயல்பு;
    ~DeadGraph() override = இயல்பு;
    DeadVertex* findNewVertex(Ast_முனை* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        எனில் (!vtxp) {
            vtxp = புதிய DeadVertex{இது, nodep};
            nodep->user2p(vtxp);
            தள்ளு_Work_Maybe(vtxp, பொய்);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        திரும்பு vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_முனை* nodep) {
        மாறிலி auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        எனில் (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } இல்லையெனில் {
            vtxp = புதிய DeadVertex{இது, nodep};
            vtxp->removable(உண்மை);
            vtxp->isVirtual(உண்மை);
            தள்ளு_Work_Maybe(vtxp, பொய்);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        திரும்பு vtxp;
    }
    வெறுமை findNewRemovableVertex(Ast_முனை* nodep, தருக்க removable) {
        DeadVertex* மாறிலி vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        எனில் (removable && !vtxp->removable()) {
            vtxp->removable(உண்மை);
            தள்ளு_Work_Maybe(vtxp, பொய்);
        }
    }
    வெறுமை delete_முனை_Vertex(Ast_முனை* nodep) {
        எனில் (DeadVertex* மாறிலி vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    வெறுமை deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        க்கு (மாறிலி V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* மாறிலி toVtxp = static_cast<DeadVertex*>(oedge.உச்சி());
            தள்ளு_Work_Maybe(toVtxp, உண்மை);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        எனில் (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        எனில் (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(இது), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    வெறுமை needs(Ast_முனை* nodep, Ast_முனை* parentp) {
        எனில் (parentp == nodep) திரும்பு;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* மாறிலி parentVtxp = findNewVertex(parentp);
        DeadVertex* மாறிலி முனை_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << முனை_Vtxp);
        newEdge(parentVtxp, முனை_Vtxp);
    }
    வெறுமை needsVirtual(Ast_முனை_FTask* nodep, Ast_முனை* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        எனில் (parentp == nodep) திரும்பு;  // No need for tracking needs itself (recursion)
        DeadVertex* மாறிலி parentVtxp = findNewVertex(parentp);
        DeadVertex* மாறிலி virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    வெறுமை funcVirtual(Ast_முனை_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* மாறிலி virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* மாறிலி முனை_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << முனை_Vtxp);
        newEdge(virtualVtxp, முனை_Vtxp);
    }
    தருக்க funcsEmpty() மாறிலி { திரும்பு m_funcs.empty(); }
    Ast_முனை* funcs_Get_எடு_Front() {
        DeadVertex* மாறிலி vtxp = m_funcs.get_எடு_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        எனில் (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            திரும்பு nullptr;  // Caller will search again
        }
        Ast_முனை* மாறிலி nodep = vtxp->nodep();
        திரும்பு nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

வகுப்பு DeadVisitor final : பொது VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_முனை_Assign*>;

    // STATE - across all visitors
    மாறிலி தருக்க m_elimUserVars;  // Allow removal of user's vars
    மாறிலி தருக்க m_elimDTypes;  // Allow removal of DTypes
    மாறிலி தருக்க m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_முனை*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_முனை_DType*, Ast_முனை_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    தருக்க m_sideEffect = பொய்;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    தருக்க m_inAssign = பொய்;  // Currently in an assign
    Ast_முனை_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_முனை* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    தருக்க m_inFTask = பொய்;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    வெறுமை deleting(Ast_முனை* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_முனை_Vertex(nodep);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    வெறுமை checkAll(Ast_முனை* nodep) {
        எனில் (Ast_முனை* மாறிலி subnodep = nodep->dtypep()) {
            எனில் (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        எனில் (Ast_முனை* மாறிலி subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    வெறுமை checkVarRef(Ast_முனை_Var_Ref* nodep) மாறிலி {
        எனில் (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    வெறுமை checkDType(Ast_முனை_DType* nodep) {
        எனில் (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.தள்ளு_back(nodep);
        }
        எனில் (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep = VN_CAST(nodep, முனை_UOr_Struct_DType)) {
            எனில் (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        எனில் (Ast_முனை* மாறிலி subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        எனில் (Ast_முனை* மாறிலி subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    வெறுமை needsTask(Ast_முனை_FTask* taskp, Ast_முனை* containerp) {
        எனில் (!taskp) திரும்பு;  // Unlinked
        எனில் (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } இல்லையெனில் {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        எனில் (nodep->dead()) திரும்பு;
        எனில் (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (AstClass* மாறிலி classp = VN_CAST(nodep, Class)) {
            எனில் (classp->extendsp()) classp->extendsp()->user1Inc();
            எனில் (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.தள்ளு_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        க்கு (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    வெறுமை visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    வெறுமை visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        எனில் (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        எனில் (m_modp->isConstPool()) nodep->user1Inc();
        எனில் (!nodep->is_உச்சி() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.தள்ளு_back(nodep);
        }
    }
    வெறுமை visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.தள்ளு_back(nodep);
        nodep->modp()->user1Inc();
    }

    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        எனில் (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        எனில் (nodep->varp()) nodep->varp()->user1Inc();
        எனில் (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        எனில் (!m_sideEffect && !nodep->isPure()) m_sideEffect = உண்மை;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        எனில் (nodep->classOrPackagep()) {
            எனில் (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } இல்லையெனில் {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    வெறுமை visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    வெறுமை visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->varp()) nodep->varp()->user1Inc();
    }
    வெறுமை visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        எனில் (nodep->classOrPackagep()) {
            எனில் (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } இல்லையெனில் {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    வெறுமை visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        எனில் (nodep->classOrPackagep()) {
            எனில் (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } இல்லையெனில் {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        எனில் (nodep->classp()) nodep->classp()->user1Inc();
    }
    வெறுமை visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        எனில் (nodep->modportp()) {
            எனில் (m_elimCells) {
                nodep->modportp(nullptr);
            } இல்லையெனில் {
                nodep->modportp()->user1Inc();
            }
        }
        எனில் (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    வெறுமை visit(Ast_முனை_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->classOrPackagep()) {
            எனில் (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } இல்லையெனில் {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->varp()) nodep->varp()->user1Inc();
        எனில் (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    வெறுமை visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    வெறுமை visit(AstModport* nodep) override {
        iterateChildren(nodep);
        எனில் (m_elimCells) {
            எனில் (!nodep->varsp()) {
                deleting(nodep);
                திரும்பு;
            }
        }
        checkAll(nodep);
    }
    வெறுமை visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    வெறுமை visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.தள்ளு_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        எனில் (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    வெறுமை visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->scopep()) nodep->scopep()->user1Inc();
        எனில் (mightElimVar(nodep->varp())) m_vscsp.தள்ளு_back(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        எனில் (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        எனில் (mightElimVar(nodep)) {
            m_varsp.தள்ளு_back(nodep);
        } இல்லையெனில் {
            எனில் (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        மாறிலி தருக்க assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = உண்மை;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* மாறிலி varrefp = VN_CAST(nodep->lhsp(), VarRef);
            எனில் (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } இல்லையெனில் {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        எனில் (assignInAssign) m_sideEffect = உண்மை;  // Parent assign shouldn't optimize
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        மாறிலி தருக்க removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = உண்மை;
        iterateChildren(nodep);
        checkAll(nodep);
        எனில் (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        எனில் (!removable) {
            எனில் (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        எனில் (nodep->classOrPackagep()) {
            எனில் (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } இல்லையெனில் {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    வெறுமை visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!m_sideEffect && !nodep->isPure()) m_sideEffect = உண்மை;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    வெறுமை deadCheckTypedefs() {
        க்கு (AstTypedef* typedefp : m_typedefsp) {
            எனில் (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                தொடர்;
            }
            checkAll(typedefp);
        }
    }
    தருக்க shouldDeleteTypedef(மாறிலி AstTypedef* typedefp) மாறிலி {
        எனில் (மாறிலி auto* மாறிலி structp = VN_CAST(typedefp->subDTypep(), முனை_UOr_Struct_DType)) {
            எனில் (structp->user1() && !structp->packed()) திரும்பு பொய்;
        }
        திரும்பு m_elimCells && !typedefp->attrPublic();
    }

    வெறுமை deadCheckTasks() {
        போது (!m_graph.funcsEmpty()) {
            Ast_முனை* மாறிலி nodep = m_graph.funcs_Get_எடு_Front();
            எனில் (!nodep) தொடர்;
            UINFO(9, "Dead " << nodep);
            எனில் (Ast_முனை_FTask* மாறிலி taskp = VN_CAST(nodep, முனை_FTask)) {
                எனில் (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } இல்லையெனில் எனில் (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } இல்லையெனில் {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    வெறுமை deadCheckDemote() {
        க்கு (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* மாறிலி vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            எனில் (!vtxp->outSize1()) தொடர்;
            எனில் (!vtxp->isVirtual()) தொடர்;
            க்கு (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_முனை* மாறிலி nodep = edge.உச்சி()->as<DeadVertex>()->nodep();
                Ast_முனை_FTask* மாறிலி funcp = VN_AS(nodep, முனை_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                எனில் (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(பொய்);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    வெறுமை deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        க்கு (தருக்க retry = உண்மை; retry;) {
            retry = பொய்;
            Ast_முனை_Module* nextmodp;
            க்கு (Ast_முனை_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), முனை_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                மாறிலி தருக்க keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                எனில் (modp->dead() || (!modp->is_உச்சி() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    எனில் (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](மாறிலி AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    எனில் (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = உண்மை;
                }
            }
        }
    }
    தருக்க mightElimVar(மாறிலி AstVar* nodep) மாறிலி {
        எனில் (nodep->isSigPublic()) திரும்பு பொய்;  // Can't elim publics!
        எனில் (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) திரும்பு பொய்;
        எனில் (nodep->isTemp() && !nodep->isTrace()) திரும்பு உண்மை;
        திரும்பு m_elimUserVars;  // Post-Trace can kill most anything
    }

    வெறுமை deadCheckScope() {
        க்கு (தருக்க retry = உண்மை; retry;) {
            retry = பொய்;
            க்கு (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* மாறிலி scp = *it;
                எனில் (!scp) தொடர்;
                எனில் (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    எனில் (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = உண்மை;
                }
            }
        }
    }

    வெறுமை deadCheckCells() {
        க்கு (AstCell* cellp : m_cellsp) {
            எனில் (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    வெறுமை deadCheckClasses() {
        க்கு (தருக்க retry = உண்மை; retry;) {
            retry = பொய்;
            க்கு (auto& itr : m_classesp) {
                எனில் (AstClass* மாறிலி nodep = itr) {  // nullptr if deleted earlier
                    எனில் (nodep->user1() == 0) {
                        எனில் (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        எனில் (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = உண்மை;
                    }
                }
            }
        }
    }

    வெறுமை deadCheckVar() {
        // Delete any unused varscopes
        க்கு (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            எனில் (vscp->varp()->user2()) தொடர்;
            எனில் (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                மாறிலி std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                க்கு (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_முனை_Assign* மாறிலி assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                எனில் (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        க்கு (தருக்க retry = உண்மை; retry;) {
            retry = பொய்;
            க்கு (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* மாறிலி varp = *it;
                எனில் (!varp) தொடர்;
                எனில் (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    எனில் (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = உண்மை;
                }
            }
        }
        க்கு (std::vector<Ast_முனை*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            எனில் ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி classp
                    = VN_CAST((*it), முனை_UOr_Struct_DType)) {
                    தருக்க cont = உண்மை;
                    க்கு (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        எனில் (memberp->user1() != 0) {
                            cont = பொய்;
                            நிறுத்து;
                        }
                    }
                    எனில் (!cont) தொடர்;
                }
                deleting(*it);
            }
        }
    }

பொது:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, தருக்க elimUserVars, தருக்க elimDTypes, தருக்க elimScopes,
                தருக்க elimCells, தருக்க elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        எனில் (AstVarScope* மாறிலி vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        க்கு (auto& itr : m_dtypePkgsp) {
            எனில் (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        எனில் (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", பொய்);

        எனில் (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        எனில் (elimScopes) deadCheckScope();
        எனில் (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        எனில் (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_மரம்();  // Removing assignments may affect isPure
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

வெறுமை V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, பொய், பொய், பொய், பொய், பொய்};
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("deadModules", 0, dump_மரம்_Either_Level() >= 6);
}

வெறுமை V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, பொய், உண்மை, பொய், பொய், உண்மை}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("deadDtypes", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, பொய், உண்மை, உண்மை, பொய், பொய்}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("deadDtypesScoped", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, உண்மை, உண்மை, பொய், உண்மை, உண்மை}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("deadAll", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, உண்மை, உண்மை, உண்மை, உண்மை, உண்மை}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("deadAllScoped", 0, dump_மரம்_Either_Level() >= 3);
}
