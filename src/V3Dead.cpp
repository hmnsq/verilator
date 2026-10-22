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
    Ast_गाठ* स्थिरांक m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    तार्किक m_removable = असत्य;  // Subject to dead removal
    तार्किक m_isVirtual = असत्य;  // Virtual node, indirection

सार्वजनिक:
    DeadVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = पूर्वनिर्धारित;
    string dotShape() स्थिरांक override {
        परत isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_गाठ* nodep() स्थिरांक VL_MT_STABLE { परत m_nodep; }
    string name() स्थिरांक override VL_MT_STABLE {
        परत (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    तार्किक removable() स्थिरांक { परत m_removable; }
    शून्य removable(तार्किक flag) { m_removable = flag; }
    तार्किक isVirtual() स्थिरांक { परत m_isVirtual; }
    शून्य isVirtual(तार्किक flag) { m_isVirtual = flag; }
    uint64_t workPos() स्थिरांक { परत m_workPos; }
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
    तार्किक empty() स्थिरांक { परत m_works.empty(); }
    शून्य ढकला(DeadVertex* vtxp) {
        जर (vtxp->workPos()) परत;  // Already on list
        स्थिरांक uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    शून्य erase(DeadVertex* vtxp) {
        स्थिरांक uint64_t id = vtxp->workPos();
        जर (!id) परत;
        UINFO(9, "Worklist.erase " << vtxp);
        स्थिरांक स्वतः it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_काढा_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        स्थिरांक स्वतः it = m_works.begin();
        DeadVertex* स्थिरांक vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        परत vtxp;
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
        नवीन V3GraphEdge{हे, fromp, शीर्ष, 1, असत्य};
        जर (शीर्ष->removable()) m_funcs.erase(शीर्ष);  // Now has an input edge
    }
    शून्य ढकला_Work_Maybe(DeadVertex* vtxp, तार्किक allowSize1) {
        जर (vtxp->removable()) {
            जर (VN_IS(vtxp->nodep(), गाठ_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.ढकला(vtxp);
        }
    }

सार्वजनिक:
    // METHODS
    DeadGraph() = पूर्वनिर्धारित;
    ~DeadGraph() override = पूर्वनिर्धारित;
    DeadVertex* findNewVertex(Ast_गाठ* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        जर (!vtxp) {
            vtxp = नवीन DeadVertex{हे, nodep};
            nodep->user2p(vtxp);
            ढकला_Work_Maybe(vtxp, असत्य);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        परत vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_गाठ* nodep) {
        स्थिरांक स्वतः it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        जर (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } अन्यथा {
            vtxp = नवीन DeadVertex{हे, nodep};
            vtxp->removable(सत्य);
            vtxp->isVirtual(सत्य);
            ढकला_Work_Maybe(vtxp, असत्य);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        परत vtxp;
    }
    शून्य findNewRemovableVertex(Ast_गाठ* nodep, तार्किक removable) {
        DeadVertex* स्थिरांक vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        जर (removable && !vtxp->removable()) {
            vtxp->removable(सत्य);
            ढकला_Work_Maybe(vtxp, असत्य);
        }
    }
    शून्य delete_गाठ_Vertex(Ast_गाठ* nodep) {
        जर (DeadVertex* स्थिरांक vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    शून्य deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        साठी (स्थिरांक V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* स्थिरांक toVtxp = static_cast<DeadVertex*>(oedge.शीर्ष());
            ढकला_Work_Maybe(toVtxp, सत्य);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        जर (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        जर (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(हे), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    शून्य needs(Ast_गाठ* nodep, Ast_गाठ* parentp) {
        जर (parentp == nodep) परत;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* स्थिरांक parentVtxp = findNewVertex(parentp);
        DeadVertex* स्थिरांक गाठ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << गाठ_Vtxp);
        newEdge(parentVtxp, गाठ_Vtxp);
    }
    शून्य needsVirtual(Ast_गाठ_FTask* nodep, Ast_गाठ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        जर (parentp == nodep) परत;  // No need for tracking needs itself (recursion)
        DeadVertex* स्थिरांक parentVtxp = findNewVertex(parentp);
        DeadVertex* स्थिरांक virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    शून्य funcVirtual(Ast_गाठ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* स्थिरांक virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* स्थिरांक गाठ_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << गाठ_Vtxp);
        newEdge(virtualVtxp, गाठ_Vtxp);
    }
    तार्किक funcsEmpty() स्थिरांक { परत m_funcs.empty(); }
    Ast_गाठ* funcs_Get_काढा_Front() {
        DeadVertex* स्थिरांक vtxp = m_funcs.get_काढा_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        जर (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            परत nullptr;  // Caller will search again
        }
        Ast_गाठ* स्थिरांक nodep = vtxp->nodep();
        परत nodep;
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
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_गाठ_Assign*>;

    // STATE - across all visitors
    स्थिरांक तार्किक m_elimUserVars;  // Allow removal of user's vars
    स्थिरांक तार्किक m_elimDTypes;  // Allow removal of DTypes
    स्थिरांक तार्किक m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_गाठ*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_गाठ_DType*, Ast_गाठ_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    तार्किक m_sideEffect = असत्य;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_inAssign = असत्य;  // Currently in an assign
    Ast_गाठ_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_गाठ* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    तार्किक m_inFTask = असत्य;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    शून्य deleting(Ast_गाठ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_गाठ_Vertex(nodep);
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    शून्य checkAll(Ast_गाठ* nodep) {
        जर (Ast_गाठ* स्थिरांक subnodep = nodep->dtypep()) {
            जर (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        जर (Ast_गाठ* स्थिरांक subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    शून्य checkVarRef(Ast_गाठ_Var_Ref* nodep) स्थिरांक {
        जर (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    शून्य checkDType(Ast_गाठ_DType* nodep) {
        जर (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ढकला_back(nodep);
        }
        जर (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक sdtypep = VN_CAST(nodep, गाठ_UOr_Struct_DType)) {
            जर (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        जर (Ast_गाठ* स्थिरांक subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        जर (Ast_गाठ* स्थिरांक subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    शून्य needsTask(Ast_गाठ_FTask* taskp, Ast_गाठ* containerp) {
        जर (!taskp) परत;  // Unlinked
        जर (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } अन्यथा {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        जर (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        जर (nodep->dead()) परत;
        जर (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        जर (AstClass* स्थिरांक classp = VN_CAST(nodep, Class)) {
            जर (classp->extendsp()) classp->extendsp()->user1Inc();
            जर (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ढकला_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        जर (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        साठी (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    शून्य visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        जर (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    शून्य visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        जर (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        जर (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        जर (m_modp->isConstPool()) nodep->user1Inc();
        जर (!nodep->is_शीर्ष() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ढकला_back(nodep);
        }
    }
    शून्य visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ढकला_back(nodep);
        nodep->modp()->user1Inc();
    }

    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        जर (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        जर (nodep->varp()) nodep->varp()->user1Inc();
        जर (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        जर (!m_sideEffect && !nodep->isPure()) m_sideEffect = सत्य;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        जर (nodep->classOrPackagep()) {
            जर (m_elimCells) {
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
        जर (nodep->varp()) nodep->varp()->user1Inc();
    }
    शून्य visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        जर (nodep->classOrPackagep()) {
            जर (m_elimCells) {
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
        जर (nodep->classOrPackagep()) {
            जर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        जर (nodep->classp()) nodep->classp()->user1Inc();
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        जर (nodep->modportp()) {
            जर (m_elimCells) {
                nodep->modportp(nullptr);
            } अन्यथा {
                nodep->modportp()->user1Inc();
            }
        }
        जर (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    शून्य visit(Ast_गाठ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        जर (nodep->classOrPackagep()) {
            जर (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } अन्यथा {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        जर (nodep->varp()) nodep->varp()->user1Inc();
        जर (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        जर (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    शून्य visit(AstModport* nodep) override {
        iterateChildren(nodep);
        जर (m_elimCells) {
            जर (!nodep->varsp()) {
                deleting(nodep);
                परत;
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
        m_typedefsp.ढकला_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        जर (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    शून्य visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        जर (nodep->scopep()) nodep->scopep()->user1Inc();
        जर (mightElimVar(nodep->varp())) m_vscsp.ढकला_back(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        जर (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        जर (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        जर (mightElimVar(nodep)) {
            m_varsp.ढकला_back(nodep);
        } अन्यथा {
            जर (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        स्थिरांक तार्किक assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = सत्य;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* स्थिरांक varrefp = VN_CAST(nodep->lhsp(), VarRef);
            जर (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } अन्यथा {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        जर (assignInAssign) m_sideEffect = सत्य;  // Parent assign shouldn't optimize
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        स्थिरांक तार्किक removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
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
        जर (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        जर (!removable) {
            जर (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        जर (nodep->classOrPackagep()) {
            जर (m_elimCells) {
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
    शून्य visit(Ast_गाठ* nodep) override {
        जर (!m_sideEffect && !nodep->isPure()) m_sideEffect = सत्य;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    शून्य deadCheckTypedefs() {
        साठी (AstTypedef* typedefp : m_typedefsp) {
            जर (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                सुरूठेवा;
            }
            checkAll(typedefp);
        }
    }
    तार्किक shouldDeleteTypedef(स्थिरांक AstTypedef* typedefp) स्थिरांक {
        जर (स्थिरांक स्वतः* स्थिरांक structp = VN_CAST(typedefp->subDTypep(), गाठ_UOr_Struct_DType)) {
            जर (structp->user1() && !structp->packed()) परत असत्य;
        }
        परत m_elimCells && !typedefp->attrPublic();
    }

    शून्य deadCheckTasks() {
        पर्यंत (!m_graph.funcsEmpty()) {
            Ast_गाठ* स्थिरांक nodep = m_graph.funcs_Get_काढा_Front();
            जर (!nodep) सुरूठेवा;
            UINFO(9, "Dead " << nodep);
            जर (Ast_गाठ_FTask* स्थिरांक taskp = VN_CAST(nodep, गाठ_FTask)) {
                जर (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } अन्यथा जर (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } अन्यथा {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    शून्य deadCheckDemote() {
        साठी (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* स्थिरांक vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            जर (!vtxp->outSize1()) सुरूठेवा;
            जर (!vtxp->isVirtual()) सुरूठेवा;
            साठी (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_गाठ* स्थिरांक nodep = edge.शीर्ष()->as<DeadVertex>()->nodep();
                Ast_गाठ_FTask* स्थिरांक funcp = VN_AS(nodep, गाठ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                जर (v3Global.opt.fDeadMethods()) {
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
        साठी (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            Ast_गाठ_Module* nextmodp;
            साठी (Ast_गाठ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), गाठ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                स्थिरांक तार्किक keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                जर (modp->dead() || (!modp->is_शीर्ष() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    जर (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](स्थिरांक AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    जर (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = सत्य;
                }
            }
        }
    }
    तार्किक mightElimVar(स्थिरांक AstVar* nodep) स्थिरांक {
        जर (nodep->isSigPublic()) परत असत्य;  // Can't elim publics!
        जर (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) परत असत्य;
        जर (nodep->isTemp() && !nodep->isTrace()) परत सत्य;
        परत m_elimUserVars;  // Post-Trace can kill most anything
    }

    शून्य deadCheckScope() {
        साठी (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            साठी (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* स्थिरांक scp = *it;
                जर (!scp) सुरूठेवा;
                जर (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    जर (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = सत्य;
                }
            }
        }
    }

    शून्य deadCheckCells() {
        साठी (AstCell* cellp : m_cellsp) {
            जर (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    शून्य deadCheckClasses() {
        साठी (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            साठी (स्वतः& itr : m_classesp) {
                जर (AstClass* स्थिरांक nodep = itr) {  // nullptr if deleted earlier
                    जर (nodep->user1() == 0) {
                        जर (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        जर (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
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
        साठी (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            जर (vscp->varp()->user2()) सुरूठेवा;
            जर (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                स्थिरांक std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                साठी (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_गाठ_Assign* स्थिरांक assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                जर (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        साठी (तार्किक retry = सत्य; retry;) {
            retry = असत्य;
            साठी (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* स्थिरांक varp = *it;
                जर (!varp) सुरूठेवा;
                जर (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    जर (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = सत्य;
                }
            }
        }
        साठी (std::vector<Ast_गाठ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            जर ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक classp
                    = VN_CAST((*it), गाठ_UOr_Struct_DType)) {
                    तार्किक cont = सत्य;
                    साठी (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        जर (memberp->user1() != 0) {
                            cont = असत्य;
                            थांबा;
                        }
                    }
                    जर (!cont) सुरूठेवा;
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

        जर (AstVarScope* स्थिरांक vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        साठी (स्वतः& itr : m_dtypePkgsp) {
            जर (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        जर (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", असत्य);

        जर (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        जर (elimScopes) deadCheckScope();
        जर (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        जर (elimTasks) deadCheckDemote();

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
