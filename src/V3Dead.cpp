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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dead.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <queue>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

ଶ୍ରେଣୀ DeadVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    ତାର୍କିକ m_removable = ମିଥ୍ୟା;  // Subject to dead removal
    ତାର୍କିକ m_isVirtual = ମିଥ୍ୟା;  // Virtual node, indirection

ସର୍ବସାଧାରଣ:
    DeadVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string dotShape() ଅଚଳ override {
        ଫେରନ୍ତୁ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_nodep; }
    string name() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    ତାର୍କିକ removable() ଅଚଳ { ଫେରନ୍ତୁ m_removable; }
    ଶୂନ୍ୟ removable(ତାର୍କିକ flag) { m_removable = flag; }
    ତାର୍କିକ isVirtual() ଅଚଳ { ଫେରନ୍ତୁ m_isVirtual; }
    ଶୂନ୍ୟ isVirtual(ତାର୍କିକ flag) { m_isVirtual = flag; }
    uint64_t workPos() ଅଚଳ { ଫେରନ୍ତୁ m_workPos; }
    ଶୂନ୍ୟ workPos(uint64_t ମୂଲ୍ୟ) { m_workPos = ମୂଲ୍ୟ; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
ଶ୍ରେଣୀ DeadWorkList final {
    // MEMBERS
    ସ୍ଥିର uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
ସର୍ବସାଧାରଣ:
    // METHODS
    ତାର୍କିକ empty() ଅଚଳ { ଫେରନ୍ତୁ m_works.empty(); }
    ଶୂନ୍ୟ ଠେଲ(DeadVertex* vtxp) {
        ଯଦି (vtxp->workPos()) ଫେରନ୍ତୁ;  // Already on list
        ଅଚଳ uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    ଶୂନ୍ୟ erase(DeadVertex* vtxp) {
        ଅଚଳ uint64_t id = vtxp->workPos();
        ଯଦି (!id) ଫେରନ୍ତୁ;
        UINFO(9, "Worklist.erase " << vtxp);
        ଅଚଳ auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_କାଢ଼_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        ଅଚଳ auto it = m_works.begin();
        DeadVertex* ଅଚଳ vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        ଫେରନ୍ତୁ vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

ଶ୍ରେଣୀ DeadGraph final : ସର୍ବସାଧାରଣ V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    ଶୂନ୍ୟ newEdge(DeadVertex* fromp, DeadVertex* ଶୀର୍ଷ) {
        ନୂତନ V3GraphEdge{ଏହି, fromp, ଶୀର୍ଷ, 1, ମିଥ୍ୟା};
        ଯଦି (ଶୀର୍ଷ->removable()) m_funcs.erase(ଶୀର୍ଷ);  // Now has an input edge
    }
    ଶୂନ୍ୟ ଠେଲ_Work_Maybe(DeadVertex* vtxp, ତାର୍କିକ allowSize1) {
        ଯଦି (vtxp->removable()) {
            ଯଦି (VN_IS(vtxp->nodep(), ଗଣ୍ଠି_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.ଠେଲ(vtxp);
        }
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    DeadGraph() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~DeadGraph() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    DeadVertex* findNewVertex(Ast_ଗଣ୍ଠି* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        ଯଦି (!vtxp) {
            vtxp = ନୂତନ DeadVertex{ଏହି, nodep};
            nodep->user2p(vtxp);
            ଠେଲ_Work_Maybe(vtxp, ମିଥ୍ୟା);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ଫେରନ୍ତୁ vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        ଯଦି (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } ଅନ୍ୟଥା {
            vtxp = ନୂତନ DeadVertex{ଏହି, nodep};
            vtxp->removable(ସତ୍ୟ);
            vtxp->isVirtual(ସତ୍ୟ);
            ଠେଲ_Work_Maybe(vtxp, ମିଥ୍ୟା);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ଫେରନ୍ତୁ vtxp;
    }
    ଶୂନ୍ୟ findNewRemovableVertex(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ removable) {
        DeadVertex* ଅଚଳ vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ଯଦି (removable && !vtxp->removable()) {
            vtxp->removable(ସତ୍ୟ);
            ଠେଲ_Work_Maybe(vtxp, ମିଥ୍ୟା);
        }
    }
    ଶୂନ୍ୟ delete_ଗଣ୍ଠି_Vertex(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (DeadVertex* ଅଚଳ vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    ଶୂନ୍ୟ deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        ପାଇଁ (ଅଚଳ V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* ଅଚଳ toVtxp = static_cast<DeadVertex*>(oedge.ଶୀର୍ଷ());
            ଠେଲ_Work_Maybe(toVtxp, ସତ୍ୟ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ଯଦି (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        ଯଦି (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(ଏହି), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    ଶୂନ୍ୟ needs(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* parentp) {
        ଯଦି (parentp == nodep) ଫେରନ୍ତୁ;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* ଅଚଳ parentVtxp = findNewVertex(parentp);
        DeadVertex* ଅଚଳ ଗଣ୍ଠି_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << ଗଣ୍ଠି_Vtxp);
        newEdge(parentVtxp, ଗଣ୍ଠି_Vtxp);
    }
    ଶୂନ୍ୟ needsVirtual(Ast_ଗଣ୍ଠି_FTask* nodep, Ast_ଗଣ୍ଠି* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ଯଦି (parentp == nodep) ଫେରନ୍ତୁ;  // No need for tracking needs itself (recursion)
        DeadVertex* ଅଚଳ parentVtxp = findNewVertex(parentp);
        DeadVertex* ଅଚଳ virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    ଶୂନ୍ୟ funcVirtual(Ast_ଗଣ୍ଠି_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* ଅଚଳ virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* ଅଚଳ ଗଣ୍ଠି_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << ଗଣ୍ଠି_Vtxp);
        newEdge(virtualVtxp, ଗଣ୍ଠି_Vtxp);
    }
    ତାର୍କିକ funcsEmpty() ଅଚଳ { ଫେରନ୍ତୁ m_funcs.empty(); }
    Ast_ଗଣ୍ଠି* funcs_Get_କାଢ଼_Front() {
        DeadVertex* ଅଚଳ vtxp = m_funcs.get_କାଢ଼_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ଯଦି (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            ଫେରନ୍ତୁ nullptr;  // Caller will search again
        }
        Ast_ଗଣ୍ଠି* ଅଚଳ nodep = vtxp->nodep();
        ଫେରନ୍ତୁ nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

ଶ୍ରେଣୀ DeadVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_ଗଣ୍ଠି_Assign*>;

    // STATE - across all visitors
    ଅଚଳ ତାର୍କିକ m_elimUserVars;  // Allow removal of user's vars
    ଅଚଳ ତାର୍କିକ m_elimDTypes;  // Allow removal of DTypes
    ଅଚଳ ତାର୍କିକ m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_ଗଣ୍ଠି*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_ଗଣ୍ଠି_DType*, Ast_ଗଣ୍ଠି_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    ତାର୍କିକ m_sideEffect = ମିଥ୍ୟା;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    ତାର୍କିକ m_inAssign = ମିଥ୍ୟା;  // Currently in an assign
    Ast_ଗଣ୍ଠି_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_ଗଣ୍ଠି* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    ତାର୍କିକ m_inFTask = ମିଥ୍ୟା;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    ଶୂନ୍ୟ deleting(Ast_ଗଣ୍ଠି* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_ଗଣ୍ଠି_Vertex(nodep);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    ଶୂନ୍ୟ checkAll(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ subnodep = nodep->dtypep()) {
            ଯଦି (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    ଶୂନ୍ୟ checkVarRef(Ast_ଗଣ୍ଠି_Var_Ref* nodep) ଅଚଳ {
        ଯଦି (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    ଶୂନ୍ୟ checkDType(Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯଦି (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ଠେଲ_back(nodep);
        }
        ଯଦି (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep = VN_CAST(nodep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଯଦି (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    ଶୂନ୍ୟ needsTask(Ast_ଗଣ୍ଠି_FTask* taskp, Ast_ଗଣ୍ଠି* containerp) {
        ଯଦି (!taskp) ଫେରନ୍ତୁ;  // Unlinked
        ଯଦି (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } ଅନ୍ୟଥା {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ଯଦି (nodep->dead()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) {
            ଯଦି (classp->extendsp()) classp->extendsp()->user1Inc();
            ଯଦି (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ଠେଲ_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        ପାଇଁ (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ଯଦି (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ଯଦି (m_modp->isConstPool()) nodep->user1Inc();
        ଯଦି (!nodep->is_ଶୀର୍ଷ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ଠେଲ_back(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ଠେଲ_back(nodep);
        nodep->modp()->user1Inc();
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ଯଦି (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ଯଦି (nodep->varp()) nodep->varp()->user1Inc();
        ଯଦି (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!m_sideEffect && !nodep->isPure()) m_sideEffect = ସତ୍ୟ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ଯଦି (nodep->classOrPackagep()) {
            ଯଦି (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ଅନ୍ୟଥା {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ଶୂନ୍ୟ visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    ଶୂନ୍ୟ visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->varp()) nodep->varp()->user1Inc();
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ଯଦି (nodep->classOrPackagep()) {
            ଯଦି (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ଅନ୍ୟଥା {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ଯଦି (nodep->classOrPackagep()) {
            ଯଦି (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ଅନ୍ୟଥା {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ଯଦି (nodep->classp()) nodep->classp()->user1Inc();
    }
    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ଯଦି (nodep->modportp()) {
            ଯଦି (m_elimCells) {
                nodep->modportp(nullptr);
            } ଅନ୍ୟଥା {
                nodep->modportp()->user1Inc();
            }
        }
        ଯଦି (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->classOrPackagep()) {
            ଯଦି (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ଅନ୍ୟଥା {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->varp()) nodep->varp()->user1Inc();
        ଯଦି (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    ଶୂନ୍ୟ visit(AstModport* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_elimCells) {
            ଯଦି (!nodep->varsp()) {
                deleting(nodep);
                ଫେରନ୍ତୁ;
            }
        }
        checkAll(nodep);
    }
    ଶୂନ୍ୟ visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ଠେଲ_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ଯଦି (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->scopep()) nodep->scopep()->user1Inc();
        ଯଦି (mightElimVar(nodep->varp())) m_vscsp.ଠେଲ_back(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ଯଦି (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ଯଦି (mightElimVar(nodep)) {
            m_varsp.ଠେଲ_back(nodep);
        } ଅନ୍ୟଥା {
            ଯଦି (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ଅଚଳ ତାର୍କିକ assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = ସତ୍ୟ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ଯଦି (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ଅନ୍ୟଥା {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ଯଦି (assignInAssign) m_sideEffect = ସତ୍ୟ;  // Parent assign shouldn't optimize
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        ଅଚଳ ତାର୍କିକ removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = ସତ୍ୟ;
        iterateChildren(nodep);
        checkAll(nodep);
        ଯଦି (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        ଯଦି (!removable) {
            ଯଦି (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ଯଦି (nodep->classOrPackagep()) {
            ଯଦି (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ଅନ୍ୟଥା {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    ଶୂନ୍ୟ visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!m_sideEffect && !nodep->isPure()) m_sideEffect = ସତ୍ୟ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    ଶୂନ୍ୟ deadCheckTypedefs() {
        ପାଇଁ (AstTypedef* typedefp : m_typedefsp) {
            ଯଦି (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                ଜାରି;
            }
            checkAll(typedefp);
        }
    }
    ତାର୍କିକ shouldDeleteTypedef(ଅଚଳ AstTypedef* typedefp) ଅଚଳ {
        ଯଦି (ଅଚଳ auto* ଅଚଳ structp = VN_CAST(typedefp->subDTypep(), ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଯଦି (structp->user1() && !structp->packed()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ m_elimCells && !typedefp->attrPublic();
    }

    ଶୂନ୍ୟ deadCheckTasks() {
        ଯେତେବେଳେ (!m_graph.funcsEmpty()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nodep = m_graph.funcs_Get_କାଢ଼_Front();
            ଯଦି (!nodep) ଜାରି;
            UINFO(9, "Dead " << nodep);
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ taskp = VN_CAST(nodep, ଗଣ୍ଠି_FTask)) {
                ଯଦି (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ଅନ୍ୟଥା ଯଦି (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } ଅନ୍ୟଥା {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    ଶୂନ୍ୟ deadCheckDemote() {
        ପାଇଁ (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* ଅଚଳ vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ଯଦି (!vtxp->outSize1()) ଜାରି;
            ଯଦି (!vtxp->isVirtual()) ଜାରି;
            ପାଇଁ (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_ଗଣ୍ଠି* ଅଚଳ nodep = edge.ଶୀର୍ଷ()->as<DeadVertex>()->nodep();
                Ast_ଗଣ୍ଠି_FTask* ଅଚଳ funcp = VN_AS(nodep, ଗଣ୍ଠି_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ଯଦି (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(ମିଥ୍ୟା);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    ଶୂନ୍ୟ deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        ପାଇଁ (ତାର୍କିକ retry = ସତ୍ୟ; retry;) {
            retry = ମିଥ୍ୟା;
            Ast_ଗଣ୍ଠି_Module* nextmodp;
            ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ଅଚଳ ତାର୍କିକ keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ଯଦି (modp->dead() || (!modp->is_ଶୀର୍ଷ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ଯଦି (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ଅଚଳ AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ଯଦି (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = ସତ୍ୟ;
                }
            }
        }
    }
    ତାର୍କିକ mightElimVar(ଅଚଳ AstVar* nodep) ଅଚଳ {
        ଯଦି (nodep->isSigPublic()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Can't elim publics!
        ଯଦି (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->isTemp() && !nodep->isTrace()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ m_elimUserVars;  // Post-Trace can kill most anything
    }

    ଶୂନ୍ୟ deadCheckScope() {
        ପାଇଁ (ତାର୍କିକ retry = ସତ୍ୟ; retry;) {
            retry = ମିଥ୍ୟା;
            ପାଇଁ (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* ଅଚଳ scp = *it;
                ଯଦି (!scp) ଜାରି;
                ଯଦି (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ଯଦି (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = ସତ୍ୟ;
                }
            }
        }
    }

    ଶୂନ୍ୟ deadCheckCells() {
        ପାଇଁ (AstCell* cellp : m_cellsp) {
            ଯଦି (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    ଶୂନ୍ୟ deadCheckClasses() {
        ପାଇଁ (ତାର୍କିକ retry = ସତ୍ୟ; retry;) {
            retry = ମିଥ୍ୟା;
            ପାଇଁ (auto& itr : m_classesp) {
                ଯଦି (AstClass* ଅଚଳ nodep = itr) {  // nullptr if deleted earlier
                    ଯଦି (nodep->user1() == 0) {
                        ଯଦି (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ଯଦି (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = ସତ୍ୟ;
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ deadCheckVar() {
        // Delete any unused varscopes
        ପାଇଁ (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ଯଦି (vscp->varp()->user2()) ଜାରି;
            ଯଦି (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ଅଚଳ std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                ପାଇଁ (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ଯଦି (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        ପାଇଁ (ତାର୍କିକ retry = ସତ୍ୟ; retry;) {
            retry = ମିଥ୍ୟା;
            ପାଇଁ (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* ଅଚଳ varp = *it;
                ଯଦି (!varp) ଜାରି;
                ଯଦି (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ଯଦି (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = ସତ୍ୟ;
                }
            }
        }
        ପାଇଁ (std::vector<Ast_ଗଣ୍ଠି*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            ଯଦି ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ classp
                    = VN_CAST((*it), ଗଣ୍ଠି_UOr_Struct_DType)) {
                    ତାର୍କିକ cont = ସତ୍ୟ;
                    ପାଇଁ (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ଯଦି (memberp->user1() != 0) {
                            cont = ମିଥ୍ୟା;
                            ଭାଙ୍ଗ;
                        }
                    }
                    ଯଦି (!cont) ଜାରି;
                }
                deleting(*it);
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, ତାର୍କିକ elimUserVars, ତାର୍କିକ elimDTypes, ତାର୍କିକ elimScopes,
                ତାର୍କିକ elimCells, ତାର୍କିକ elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        ଯଦି (AstVarScope* ଅଚଳ vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        ପାଇଁ (auto& itr : m_dtypePkgsp) {
            ଯଦି (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ଯଦି (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", ମିଥ୍ୟା);

        ଯଦି (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ଯଦି (elimScopes) deadCheckScope();
        ଯଦି (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ଯଦି (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_ବୃକ୍ଷ();  // Removing assignments may affect isPure
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

ଶୂନ୍ୟ V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, ମିଥ୍ୟା, ମିଥ୍ୟା, ମିଥ୍ୟା};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("deadModules", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}

ଶୂନ୍ୟ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ମିଥ୍ୟା, ସତ୍ୟ, ମିଥ୍ୟା, ମିଥ୍ୟା, ସତ୍ୟ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("deadDtypes", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ମିଥ୍ୟା, ସତ୍ୟ, ସତ୍ୟ, ମିଥ୍ୟା, ମିଥ୍ୟା}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("deadDtypesScoped", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ସତ୍ୟ, ସତ୍ୟ, ମିଥ୍ୟା, ସତ୍ୟ, ସତ୍ୟ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("deadAll", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ସତ୍ୟ, ସତ୍ୟ, ସତ୍ୟ, ସତ୍ୟ, ସତ୍ୟ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("deadAllScoped", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
