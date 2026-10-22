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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dead.h"

#समावेश "V3ConstPool.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"

#समावेश <queue>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

श्रेणी DeadVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_गाँठ* स्थिर m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    bool m_removable = false;  // Subject to dead removal
    bool m_isVirtual = false;  // Virtual node, indirection

खुला:
    DeadVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = शेष;
    string dotShape() स्थिर override {
        वापस isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_गाँठ* nodep() स्थिर VL_MT_STABLE { वापस m_nodep; }
    string name() स्थिर override VL_MT_STABLE {
        वापस (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    bool removable() स्थिर { वापस m_removable; }
    व्योम removable(bool flag) { m_removable = flag; }
    bool isVirtual() स्थिर { वापस m_isVirtual; }
    व्योम isVirtual(bool flag) { m_isVirtual = flag; }
    uint64_t workPos() स्थिर { वापस m_workPos; }
    व्योम workPos(uint64_t मूल्य) { m_workPos = मूल्य; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
श्रेणी DeadWorkList final {
    // MEMBERS
    अटल uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
खुला:
    // METHODS
    bool empty() स्थिर { वापस m_works.empty(); }
    व्योम धकेलो(DeadVertex* vtxp) {
        अगर (vtxp->workPos()) वापस;  // Already on list
        स्थिर uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    व्योम erase(DeadVertex* vtxp) {
        स्थिर uint64_t id = vtxp->workPos();
        अगर (!id) वापस;
        UINFO(9, "Worklist.erase " << vtxp);
        स्थिर स्वतः it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_निकालो_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        स्थिर स्वतः it = m_works.begin();
        DeadVertex* स्थिर vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        वापस vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

श्रेणी DeadGraph final : खुला V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    व्योम newEdge(DeadVertex* fromp, DeadVertex* शीर्ष) {
        नया V3GraphEdge{यह, fromp, शीर्ष, 1, false};
        अगर (शीर्ष->removable()) m_funcs.erase(शीर्ष);  // Now has an input edge
    }
    व्योम धकेलो_Work_Maybe(DeadVertex* vtxp, bool allowSize1) {
        अगर (vtxp->removable()) {
            अगर (VN_IS(vtxp->nodep(), गाँठ_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.धकेलो(vtxp);
        }
    }

खुला:
    // METHODS
    DeadGraph() = शेष;
    ~DeadGraph() override = शेष;
    DeadVertex* findNewVertex(Ast_गाँठ* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        अगर (!vtxp) {
            vtxp = नया DeadVertex{यह, nodep};
            nodep->user2p(vtxp);
            धकेलो_Work_Maybe(vtxp, false);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        वापस vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_गाँठ* nodep) {
        स्थिर स्वतः it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        अगर (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } अन्यथा {
            vtxp = नया DeadVertex{यह, nodep};
            vtxp->removable(true);
            vtxp->isVirtual(true);
            धकेलो_Work_Maybe(vtxp, false);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        वापस vtxp;
    }
    व्योम findNewRemovableVertex(Ast_गाँठ* nodep, bool removable) {
        DeadVertex* स्थिर vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        अगर (removable && !vtxp->removable()) {
            vtxp->removable(true);
            धकेलो_Work_Maybe(vtxp, false);
        }
    }
    व्योम delete_गाँठ_Vertex(Ast_गाँठ* nodep) {
        अगर (DeadVertex* स्थिर vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    व्योम deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        क्रम (स्थिर V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* स्थिर toVtxp = static_cast<DeadVertex*>(oedge.शीर्ष());
            धकेलो_Work_Maybe(toVtxp, true);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        अगर (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        अगर (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(यह), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    व्योम needs(Ast_गाँठ* nodep, Ast_गाँठ* parentp) {
        अगर (parentp == nodep) वापस;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* स्थिर parentVtxp = findNewVertex(parentp);
        DeadVertex* स्थिर गाँठ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << गाँठ_Vtxp);
        newEdge(parentVtxp, गाँठ_Vtxp);
    }
    व्योम needsVirtual(Ast_गाँठ_FTask* nodep, Ast_गाँठ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        अगर (parentp == nodep) वापस;  // No need for tracking needs itself (recursion)
        DeadVertex* स्थिर parentVtxp = findNewVertex(parentp);
        DeadVertex* स्थिर virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    व्योम funcVirtual(Ast_गाँठ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* स्थिर virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* स्थिर गाँठ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << गाँठ_Vtxp);
        newEdge(virtualVtxp, गाँठ_Vtxp);
    }
    bool funcsEmpty() स्थिर { वापस m_funcs.empty(); }
    Ast_गाँठ* funcs_Get_निकालो_Front() {
        DeadVertex* स्थिर vtxp = m_funcs.get_निकालो_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        अगर (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            वापस nullptr;  // Caller will search again
        }
        Ast_गाँठ* स्थिर nodep = vtxp->nodep();
        वापस nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

श्रेणी DeadVisitor final : खुला VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_गाँठ_Assign*>;

    // STATE - across all visitors
    स्थिर bool m_elimUserVars;  // Allow removal of user's vars
    स्थिर bool m_elimDTypes;  // Allow removal of DTypes
    स्थिर bool m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_गाँठ*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_गाँठ_DType*, Ast_गाँठ_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    bool m_sideEffect = false;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    bool m_inAssign = false;  // Currently in an assign
    Ast_गाँठ_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_गाँठ* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    bool m_inFTask = false;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    व्योम deleting(Ast_गाँठ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_गाँठ_Vertex(nodep);
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    व्योम checkAll(Ast_गाँठ* nodep) {
        अगर (Ast_गाँठ* स्थिर subnodep = nodep->dtypep()) {
            अगर (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        अगर (Ast_गाँठ* स्थिर subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    व्योम checkVarRef(Ast_गाँठ_Var_Ref* nodep) स्थिर {
        अगर (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    व्योम checkDType(Ast_गाँठ_DType* nodep) {
        अगर (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.धकेलो_back(nodep);
        }
        अगर (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep = VN_CAST(nodep, गाँठ_UOr_Struct_DType)) {
            अगर (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        अगर (Ast_गाँठ* स्थिर subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        अगर (Ast_गाँठ* स्थिर subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    व्योम needsTask(Ast_गाँठ_FTask* taskp, Ast_गाँठ* containerp) {
        अगर (!taskp) वापस;  // Unlinked
        अगर (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } अन्यथा {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        अगर (nodep->dead()) वापस;
        अगर (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (AstClass* स्थिर classp = VN_CAST(nodep, Class)) {
            अगर (classp->extendsp()) classp->extendsp()->user1Inc();
            अगर (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.धकेलो_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    व्योम visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        क्रम (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    व्योम visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    व्योम visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        अगर (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        अगर (m_modp->isConstPool()) nodep->user1Inc();
        अगर (!nodep->is_शीर्ष() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.धकेलो_back(nodep);
        }
    }
    व्योम visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.धकेलो_back(nodep);
        nodep->modp()->user1Inc();
    }

    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        अगर (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        अगर (nodep->varp()) nodep->varp()->user1Inc();
        अगर (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        अगर (!m_sideEffect && !nodep->isPure()) m_sideEffect = true;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        अगर (nodep->classOrPackagep()) {
            अगर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    व्योम visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    व्योम visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->varp()) nodep->varp()->user1Inc();
    }
    व्योम visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        अगर (nodep->classOrPackagep()) {
            अगर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    व्योम visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        अगर (nodep->classOrPackagep()) {
            अगर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        अगर (nodep->classp()) nodep->classp()->user1Inc();
    }
    व्योम visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        अगर (nodep->modportp()) {
            अगर (m_elimCells) {
                nodep->modportp(nullptr);
            } अन्यथा {
                nodep->modportp()->user1Inc();
            }
        }
        अगर (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    व्योम visit(Ast_गाँठ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    व्योम visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->classOrPackagep()) {
            अगर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    व्योम visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->varp()) nodep->varp()->user1Inc();
        अगर (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    व्योम visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    व्योम visit(AstModport* nodep) override {
        iterateChildren(nodep);
        अगर (m_elimCells) {
            अगर (!nodep->varsp()) {
                deleting(nodep);
                वापस;
            }
        }
        checkAll(nodep);
    }
    व्योम visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    व्योम visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.धकेलो_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        अगर (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    व्योम visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->scopep()) nodep->scopep()->user1Inc();
        अगर (mightElimVar(nodep->varp())) m_vscsp.धकेलो_back(nodep);
    }
    व्योम visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        अगर (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        अगर (mightElimVar(nodep)) {
            m_varsp.धकेलो_back(nodep);
        } अन्यथा {
            अगर (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        स्थिर bool assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = true;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_अग्र_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* स्थिर varrefp = VN_CAST(nodep->lhsp(), VarRef);
            अगर (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } अन्यथा {  // Track like any other statement
                iterate_And_अग्र_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        अगर (assignInAssign) m_sideEffect = true;  // Parent assign shouldn't optimize
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        स्थिर bool removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = true;
        iterateChildren(nodep);
        checkAll(nodep);
        अगर (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        अगर (!removable) {
            अगर (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        अगर (nodep->classOrPackagep()) {
            अगर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    व्योम visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!m_sideEffect && !nodep->isPure()) m_sideEffect = true;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    व्योम deadCheckTypedefs() {
        क्रम (AstTypedef* typedefp : m_typedefsp) {
            अगर (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                जारी;
            }
            checkAll(typedefp);
        }
    }
    bool shouldDeleteTypedef(स्थिर AstTypedef* typedefp) स्थिर {
        अगर (स्थिर स्वतः* स्थिर structp = VN_CAST(typedefp->subDTypep(), गाँठ_UOr_Struct_DType)) {
            अगर (structp->user1() && !structp->packed()) वापस false;
        }
        वापस m_elimCells && !typedefp->attrPublic();
    }

    व्योम deadCheckTasks() {
        जबतक (!m_graph.funcsEmpty()) {
            Ast_गाँठ* स्थिर nodep = m_graph.funcs_Get_निकालो_Front();
            अगर (!nodep) जारी;
            UINFO(9, "Dead " << nodep);
            अगर (Ast_गाँठ_FTask* स्थिर taskp = VN_CAST(nodep, गाँठ_FTask)) {
                अगर (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } अन्यथा अगर (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } अन्यथा {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    व्योम deadCheckDemote() {
        क्रम (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* स्थिर vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            अगर (!vtxp->outSize1()) जारी;
            अगर (!vtxp->isVirtual()) जारी;
            क्रम (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_गाँठ* स्थिर nodep = edge.शीर्ष()->as<DeadVertex>()->nodep();
                Ast_गाँठ_FTask* स्थिर funcp = VN_AS(nodep, गाँठ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                अगर (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(false);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    व्योम deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        क्रम (bool retry = true; retry;) {
            retry = false;
            Ast_गाँठ_Module* nextmodp;
            क्रम (Ast_गाँठ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), गाँठ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                स्थिर bool keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                अगर (modp->dead() || (!modp->is_शीर्ष() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    अगर (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](स्थिर AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    अगर (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = true;
                }
            }
        }
    }
    bool mightElimVar(स्थिर AstVar* nodep) स्थिर {
        अगर (nodep->isSigPublic()) वापस false;  // Can't elim publics!
        अगर (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) वापस false;
        अगर (nodep->is_अस्थायी() && !nodep->isTrace()) वापस true;
        वापस m_elimUserVars;  // Post-Trace can kill most anything
    }

    व्योम deadCheckScope() {
        क्रम (bool retry = true; retry;) {
            retry = false;
            क्रम (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* स्थिर scp = *it;
                अगर (!scp) जारी;
                अगर (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    अगर (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = true;
                }
            }
        }
    }

    व्योम deadCheckCells() {
        क्रम (AstCell* cellp : m_cellsp) {
            अगर (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    व्योम deadCheckClasses() {
        क्रम (bool retry = true; retry;) {
            retry = false;
            क्रम (स्वतः& itr : m_classesp) {
                अगर (AstClass* स्थिर nodep = itr) {  // nullptr if deleted earlier
                    अगर (nodep->user1() == 0) {
                        अगर (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        अगर (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = true;
                    }
                }
            }
        }
    }

    व्योम deadCheckVar() {
        // Delete any unused varscopes
        क्रम (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            अगर (vscp->varp()->user2()) जारी;
            अगर (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                स्थिर std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                क्रम (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_गाँठ_Assign* स्थिर assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                अगर (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        क्रम (bool retry = true; retry;) {
            retry = false;
            क्रम (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* स्थिर varp = *it;
                अगर (!varp) जारी;
                अगर (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    अगर (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = true;
                }
            }
        }
        क्रम (std::vector<Ast_गाँठ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            अगर ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर classp
                    = VN_CAST((*it), गाँठ_UOr_Struct_DType)) {
                    bool cont = true;
                    क्रम (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        अगर (memberp->user1() != 0) {
                            cont = false;
                            अवरोध;
                        }
                    }
                    अगर (!cont) जारी;
                }
                deleting(*it);
            }
        }
    }

खुला:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, bool elimUserVars, bool elimDTypes, bool elimScopes,
                bool elimCells, bool elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        अगर (AstVarScope* स्थिर vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        क्रम (स्वतः& itr : m_dtypePkgsp) {
            अगर (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        अगर (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", false);

        अगर (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        अगर (elimScopes) deadCheckScope();
        अगर (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        अगर (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_वृक्ष();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::add_Stat_योग("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::add_Stat_योग("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::add_Stat_योग("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::add_Stat_योग("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

व्योम V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, false, false, false, false, false};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadModules", 0, dump_वृक्ष_Either_Level() >= 6);
}

व्योम V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, false, true, false, false, true}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadDtypes", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, false, true, true, false, false}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadDtypesScoped", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, true, true, false, true, true}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadAll", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, true, true, true, true, true}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadAllScoped", 0, dump_वृक्ष_Either_Level() >= 3);
}
