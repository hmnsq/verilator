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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dead.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <queue>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

वर्ग DeadVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_गाँठ* ध्रुव m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    तार्किक m_removable = असत्य;  // Subject to dead removal
    तार्किक m_isVirtual = असत्य;  // Virtual node, indirection

सार्वजनिक:
    DeadVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = स्वभाव;
    string dotShape() ध्रुव override {
        प्रत्यागम isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_गाँठ* nodep() ध्रुव VL_MT_STABLE { प्रत्यागम m_nodep; }
    string name() ध्रुव override VL_MT_STABLE {
        प्रत्यागम (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    तार्किक removable() ध्रुव { प्रत्यागम m_removable; }
    शून्य removable(तार्किक flag) { m_removable = flag; }
    तार्किक isVirtual() ध्रुव { प्रत्यागम m_isVirtual; }
    शून्य isVirtual(तार्किक flag) { m_isVirtual = flag; }
    uint64_t workPos() ध्रुव { प्रत्यागम m_workPos; }
    शून्य workPos(uint64_t मूल्य) { m_workPos = मूल्य; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
वर्ग DeadWorkList final {
    // MEMBERS
    स्थिर uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
सार्वजनिक:
    // METHODS
    तार्किक empty() ध्रुव { प्रत्यागम m_works.empty(); }
    शून्य धकेलो(DeadVertex* vtxp) {
        यदि (vtxp->workPos()) प्रत्यागम;  // Already on list
        ध्रुव uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    शून्य erase(DeadVertex* vtxp) {
        ध्रुव uint64_t id = vtxp->workPos();
        यदि (!id) प्रत्यागम;
        UINFO(9, "Worklist.erase " << vtxp);
        ध्रुव स्वतः it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_निकालो_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        ध्रुव स्वतः it = m_works.begin();
        DeadVertex* ध्रुव vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        प्रत्यागम vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

वर्ग DeadGraph final : सार्वजनिक V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    शून्य newEdge(DeadVertex* fromp, DeadVertex* शीर्ष) {
        नूतन V3GraphEdge{एतत्, fromp, शीर्ष, 1, असत्य};
        यदि (शीर्ष->removable()) m_funcs.erase(शीर्ष);  // Now has an input edge
    }
    शून्य धकेलो_Work_Maybe(DeadVertex* vtxp, तार्किक allowSize1) {
        यदि (vtxp->removable()) {
            यदि (VN_IS(vtxp->nodep(), गाँठ_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.धकेलो(vtxp);
        }
    }

सार्वजनिक:
    // METHODS
    DeadGraph() = स्वभाव;
    ~DeadGraph() override = स्वभाव;
    DeadVertex* findNewVertex(Ast_गाँठ* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        यदि (!vtxp) {
            vtxp = नूतन DeadVertex{एतत्, nodep};
            nodep->user2p(vtxp);
            धकेलो_Work_Maybe(vtxp, असत्य);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        प्रत्यागम vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_गाँठ* nodep) {
        ध्रुव स्वतः it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        यदि (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } अन्यथा {
            vtxp = नूतन DeadVertex{एतत्, nodep};
            vtxp->removable(सत्य);
            vtxp->isVirtual(सत्य);
            धकेलो_Work_Maybe(vtxp, असत्य);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        प्रत्यागम vtxp;
    }
    शून्य findNewRemovableVertex(Ast_गाँठ* nodep, तार्किक removable) {
        DeadVertex* ध्रुव vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        यदि (removable && !vtxp->removable()) {
            vtxp->removable(सत्य);
            धकेलो_Work_Maybe(vtxp, असत्य);
        }
    }
    शून्य delete_गाँठ_Vertex(Ast_गाँठ* nodep) {
        यदि (DeadVertex* ध्रुव vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    शून्य deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        प्रति (ध्रुव V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* ध्रुव toVtxp = static_cast<DeadVertex*>(oedge.शीर्ष());
            धकेलो_Work_Maybe(toVtxp, सत्य);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        यदि (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        यदि (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(एतत्), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    शून्य needs(Ast_गाँठ* nodep, Ast_गाँठ* parentp) {
        यदि (parentp == nodep) प्रत्यागम;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* ध्रुव parentVtxp = findNewVertex(parentp);
        DeadVertex* ध्रुव गाँठ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << गाँठ_Vtxp);
        newEdge(parentVtxp, गाँठ_Vtxp);
    }
    शून्य needsVirtual(Ast_गाँठ_FTask* nodep, Ast_गाँठ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        यदि (parentp == nodep) प्रत्यागम;  // No need for tracking needs itself (recursion)
        DeadVertex* ध्रुव parentVtxp = findNewVertex(parentp);
        DeadVertex* ध्रुव virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    शून्य funcVirtual(Ast_गाँठ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* ध्रुव virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* ध्रुव गाँठ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << गाँठ_Vtxp);
        newEdge(virtualVtxp, गाँठ_Vtxp);
    }
    तार्किक funcsEmpty() ध्रुव { प्रत्यागम m_funcs.empty(); }
    Ast_गाँठ* funcs_Get_निकालो_Front() {
        DeadVertex* ध्रुव vtxp = m_funcs.get_निकालो_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        यदि (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            प्रत्यागम nullptr;  // Caller will search again
        }
        Ast_गाँठ* ध्रुव nodep = vtxp->nodep();
        प्रत्यागम nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

वर्ग DeadVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_गाँठ_Assign*>;

    // STATE - across all visitors
    ध्रुव तार्किक m_elimUserVars;  // Allow removal of user's vars
    ध्रुव तार्किक m_elimDTypes;  // Allow removal of DTypes
    ध्रुव तार्किक m_elimCells;  // Allow removal of Cells

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
    तार्किक m_sideEffect = असत्य;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_inAssign = असत्य;  // Currently in an assign
    Ast_गाँठ_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_गाँठ* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    तार्किक m_inFTask = असत्य;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    शून्य deleting(Ast_गाँठ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_गाँठ_Vertex(nodep);
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    शून्य checkAll(Ast_गाँठ* nodep) {
        यदि (Ast_गाँठ* ध्रुव subnodep = nodep->dtypep()) {
            यदि (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        यदि (Ast_गाँठ* ध्रुव subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    शून्य checkVarRef(Ast_गाँठ_Var_Ref* nodep) ध्रुव {
        यदि (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    शून्य checkDType(Ast_गाँठ_DType* nodep) {
        यदि (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.धकेलो_back(nodep);
        }
        यदि (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        यदि (ध्रुव Ast_गाँठ_UOr_Struct_DType* ध्रुव sdtypep = VN_CAST(nodep, गाँठ_UOr_Struct_DType)) {
            यदि (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        यदि (Ast_गाँठ* ध्रुव subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        यदि (Ast_गाँठ* ध्रुव subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    शून्य needsTask(Ast_गाँठ_FTask* taskp, Ast_गाँठ* containerp) {
        यदि (!taskp) प्रत्यागम;  // Unlinked
        यदि (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } अन्यथा {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    शून्य visit(Ast_गाँठ_Module* nodep) override {
        यदि (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        यदि (nodep->dead()) प्रत्यागम;
        यदि (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            यदि (classp->extendsp()) classp->extendsp()->user1Inc();
            यदि (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.धकेलो_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        प्रति (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    शून्य visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    शून्य visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        यदि (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        यदि (m_modp->isConstPool()) nodep->user1Inc();
        यदि (!nodep->is_शीर्ष() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.धकेलो_back(nodep);
        }
    }
    शून्य visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.धकेलो_back(nodep);
        nodep->modp()->user1Inc();
    }

    शून्य visit(Ast_गाँठ_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        यदि (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        यदि (nodep->varp()) nodep->varp()->user1Inc();
        यदि (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    शून्य visit(Ast_गाँठ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        यदि (!m_sideEffect && !nodep->isPure()) m_sideEffect = सत्य;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        यदि (nodep->classOrPackagep()) {
            यदि (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    शून्य visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    शून्य visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->varp()) nodep->varp()->user1Inc();
    }
    शून्य visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        यदि (nodep->classOrPackagep()) {
            यदि (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    शून्य visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        यदि (nodep->classOrPackagep()) {
            यदि (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        यदि (nodep->classp()) nodep->classp()->user1Inc();
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        यदि (nodep->modportp()) {
            यदि (m_elimCells) {
                nodep->modportp(nullptr);
            } अन्यथा {
                nodep->modportp()->user1Inc();
            }
        }
        यदि (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    शून्य visit(Ast_गाँठ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->classOrPackagep()) {
            यदि (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->varp()) nodep->varp()->user1Inc();
        यदि (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    शून्य visit(AstModport* nodep) override {
        iterateChildren(nodep);
        यदि (m_elimCells) {
            यदि (!nodep->varsp()) {
                deleting(nodep);
                प्रत्यागम;
            }
        }
        checkAll(nodep);
    }
    शून्य visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    शून्य visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.धकेलो_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        यदि (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    शून्य visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->scopep()) nodep->scopep()->user1Inc();
        यदि (mightElimVar(nodep->varp())) m_vscsp.धकेलो_back(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        यदि (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        यदि (mightElimVar(nodep)) {
            m_varsp.धकेलो_back(nodep);
        } अन्यथा {
            यदि (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ध्रुव तार्किक assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = सत्य;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ध्रुव varrefp = VN_CAST(nodep->lhsp(), VarRef);
            यदि (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } अन्यथा {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        यदि (assignInAssign) m_sideEffect = सत्य;  // Parent assign shouldn't optimize
    }
    शून्य visit(Ast_गाँठ_FTask* nodep) override {
        ध्रुव तार्किक removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = सत्य;
        iterateChildren(nodep);
        checkAll(nodep);
        यदि (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        यदि (!removable) {
            यदि (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        यदि (nodep->classOrPackagep()) {
            यदि (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    शून्य visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (!m_sideEffect && !nodep->isPure()) m_sideEffect = सत्य;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    शून्य deadCheckTypedefs() {
        प्रति (AstTypedef* typedefp : m_typedefsp) {
            यदि (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                अनुवर्त;
            }
            checkAll(typedefp);
        }
    }
    तार्किक shouldDeleteTypedef(ध्रुव AstTypedef* typedefp) ध्रुव {
        यदि (ध्रुव स्वतः* ध्रुव structp = VN_CAST(typedefp->subDTypep(), गाँठ_UOr_Struct_DType)) {
            यदि (structp->user1() && !structp->packed()) प्रत्यागम असत्य;
        }
        प्रत्यागम m_elimCells && !typedefp->attrPublic();
    }

    शून्य deadCheckTasks() {
        यावत् (!m_graph.funcsEmpty()) {
            Ast_गाँठ* ध्रुव nodep = m_graph.funcs_Get_निकालो_Front();
            यदि (!nodep) अनुवर्त;
            UINFO(9, "Dead " << nodep);
            यदि (Ast_गाँठ_FTask* ध्रुव taskp = VN_CAST(nodep, गाँठ_FTask)) {
                यदि (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } अन्यथा यदि (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } अन्यथा {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    शून्य deadCheckDemote() {
        प्रति (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* ध्रुव vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            यदि (!vtxp->outSize1()) अनुवर्त;
            यदि (!vtxp->isVirtual()) अनुवर्त;
            प्रति (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_गाँठ* ध्रुव nodep = edge.शीर्ष()->as<DeadVertex>()->nodep();
                Ast_गाँठ_FTask* ध्रुव funcp = VN_AS(nodep, गाँठ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                यदि (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(असत्य);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    शून्य deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        प्रति (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            Ast_गाँठ_Module* nextmodp;
            प्रति (Ast_गाँठ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), गाँठ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ध्रुव तार्किक keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                यदि (modp->dead() || (!modp->is_शीर्ष() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    यदि (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ध्रुव AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    यदि (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = सत्य;
                }
            }
        }
    }
    तार्किक mightElimVar(ध्रुव AstVar* nodep) ध्रुव {
        यदि (nodep->isSigPublic()) प्रत्यागम असत्य;  // Can't elim publics!
        यदि (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) प्रत्यागम असत्य;
        यदि (nodep->isTemp() && !nodep->isTrace()) प्रत्यागम सत्य;
        प्रत्यागम m_elimUserVars;  // Post-Trace can kill most anything
    }

    शून्य deadCheckScope() {
        प्रति (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            प्रति (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* ध्रुव scp = *it;
                यदि (!scp) अनुवर्त;
                यदि (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    यदि (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = सत्य;
                }
            }
        }
    }

    शून्य deadCheckCells() {
        प्रति (AstCell* cellp : m_cellsp) {
            यदि (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    शून्य deadCheckClasses() {
        प्रति (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            प्रति (स्वतः& itr : m_classesp) {
                यदि (AstClass* ध्रुव nodep = itr) {  // nullptr if deleted earlier
                    यदि (nodep->user1() == 0) {
                        यदि (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        यदि (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = सत्य;
                    }
                }
            }
        }
    }

    शून्य deadCheckVar() {
        // Delete any unused varscopes
        प्रति (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            यदि (vscp->varp()->user2()) अनुवर्त;
            यदि (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ध्रुव std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                प्रति (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_गाँठ_Assign* ध्रुव assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                यदि (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        प्रति (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            प्रति (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* ध्रुव varp = *it;
                यदि (!varp) अनुवर्त;
                यदि (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    यदि (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = सत्य;
                }
            }
        }
        प्रति (std::vector<Ast_गाँठ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            यदि ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                यदि (ध्रुव Ast_गाँठ_UOr_Struct_DType* ध्रुव classp
                    = VN_CAST((*it), गाँठ_UOr_Struct_DType)) {
                    तार्किक cont = सत्य;
                    प्रति (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        यदि (memberp->user1() != 0) {
                            cont = असत्य;
                            भङ्ग;
                        }
                    }
                    यदि (!cont) अनुवर्त;
                }
                deleting(*it);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, तार्किक elimUserVars, तार्किक elimDTypes, तार्किक elimScopes,
                तार्किक elimCells, तार्किक elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        यदि (AstVarScope* ध्रुव vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        प्रति (स्वतः& itr : m_dtypePkgsp) {
            यदि (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", असत्य);

        यदि (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        यदि (elimScopes) deadCheckScope();
        यदि (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        यदि (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_वृक्ष();  // Removing assignments may affect isPure
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

शून्य V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, असत्य, असत्य, असत्य, असत्य, असत्य};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadModules", 0, dump_वृक्ष_Either_Level() >= 6);
}

शून्य V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, असत्य, सत्य, असत्य, असत्य, सत्य}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadDtypes", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, असत्य, सत्य, सत्य, असत्य, असत्य}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadDtypesScoped", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, सत्य, सत्य, असत्य, सत्य, सत्य}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadAll", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, सत्य, सत्य, सत्य, सत्य, सत्य}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deadAllScoped", 0, dump_वृक्ष_Either_Level() >= 3);
}
