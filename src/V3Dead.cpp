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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dead.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <queue>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

വർഗ്ഗം DeadVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_കണ്ണി* സ്ഥിരം m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    തർക്കം m_removable = അസത്യം;  // Subject to dead removal
    തർക്കം m_isVirtual = അസത്യം;  // Virtual node, indirection

പൊതു:
    DeadVertex(V3Graph* graphp, Ast_കണ്ണി* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = സ്ഥിരസ്ഥിതി;
    string dotShape() സ്ഥിരം override {
        തിരികെ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_കണ്ണി* nodep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_nodep; }
    string name() സ്ഥിരം override VL_MT_STABLE {
        തിരികെ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    തർക്കം removable() സ്ഥിരം { തിരികെ m_removable; }
    ശൂന്യം removable(തർക്കം flag) { m_removable = flag; }
    തർക്കം isVirtual() സ്ഥിരം { തിരികെ m_isVirtual; }
    ശൂന്യം isVirtual(തർക്കം flag) { m_isVirtual = flag; }
    uint64_t workPos() സ്ഥിരം { തിരികെ m_workPos; }
    ശൂന്യം workPos(uint64_t മൂല്യം) { m_workPos = മൂല്യം; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
വർഗ്ഗം DeadWorkList final {
    // MEMBERS
    സ്ഥിതം uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
പൊതു:
    // METHODS
    തർക്കം empty() സ്ഥിരം { തിരികെ m_works.empty(); }
    ശൂന്യം തള്ളുക(DeadVertex* vtxp) {
        എങ്കിൽ (vtxp->workPos()) തിരികെ;  // Already on list
        സ്ഥിരം uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    ശൂന്യം erase(DeadVertex* vtxp) {
        സ്ഥിരം uint64_t id = vtxp->workPos();
        എങ്കിൽ (!id) തിരികെ;
        UINFO(9, "Worklist.erase " << vtxp);
        സ്ഥിരം auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_എടുക്കുക_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        സ്ഥിരം auto it = m_works.begin();
        DeadVertex* സ്ഥിരം vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        തിരികെ vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

വർഗ്ഗം DeadGraph final : പൊതു V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    ശൂന്യം newEdge(DeadVertex* fromp, DeadVertex* മുകൾ) {
        പുതിയ V3GraphEdge{ഇത്, fromp, മുകൾ, 1, അസത്യം};
        എങ്കിൽ (മുകൾ->removable()) m_funcs.erase(മുകൾ);  // Now has an input edge
    }
    ശൂന്യം തള്ളുക_Work_Maybe(DeadVertex* vtxp, തർക്കം allowSize1) {
        എങ്കിൽ (vtxp->removable()) {
            എങ്കിൽ (VN_IS(vtxp->nodep(), കണ്ണി_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.തള്ളുക(vtxp);
        }
    }

പൊതു:
    // METHODS
    DeadGraph() = സ്ഥിരസ്ഥിതി;
    ~DeadGraph() override = സ്ഥിരസ്ഥിതി;
    DeadVertex* findNewVertex(Ast_കണ്ണി* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        എങ്കിൽ (!vtxp) {
            vtxp = പുതിയ DeadVertex{ഇത്, nodep};
            nodep->user2p(vtxp);
            തള്ളുക_Work_Maybe(vtxp, അസത്യം);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        തിരികെ vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_കണ്ണി* nodep) {
        സ്ഥിരം auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        എങ്കിൽ (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } അല്ലെങ്കിൽ {
            vtxp = പുതിയ DeadVertex{ഇത്, nodep};
            vtxp->removable(സത്യം);
            vtxp->isVirtual(സത്യം);
            തള്ളുക_Work_Maybe(vtxp, അസത്യം);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        തിരികെ vtxp;
    }
    ശൂന്യം findNewRemovableVertex(Ast_കണ്ണി* nodep, തർക്കം removable) {
        DeadVertex* സ്ഥിരം vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        എങ്കിൽ (removable && !vtxp->removable()) {
            vtxp->removable(സത്യം);
            തള്ളുക_Work_Maybe(vtxp, അസത്യം);
        }
    }
    ശൂന്യം delete_കണ്ണി_Vertex(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (DeadVertex* സ്ഥിരം vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    ശൂന്യം deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        വേണ്ടി (സ്ഥിരം V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* സ്ഥിരം toVtxp = static_cast<DeadVertex*>(oedge.മുകൾ());
            തള്ളുക_Work_Maybe(toVtxp, സത്യം);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        എങ്കിൽ (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        എങ്കിൽ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(ഇത്), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    ശൂന്യം needs(Ast_കണ്ണി* nodep, Ast_കണ്ണി* parentp) {
        എങ്കിൽ (parentp == nodep) തിരികെ;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* സ്ഥിരം parentVtxp = findNewVertex(parentp);
        DeadVertex* സ്ഥിരം കണ്ണി_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << കണ്ണി_Vtxp);
        newEdge(parentVtxp, കണ്ണി_Vtxp);
    }
    ശൂന്യം needsVirtual(Ast_കണ്ണി_FTask* nodep, Ast_കണ്ണി* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        എങ്കിൽ (parentp == nodep) തിരികെ;  // No need for tracking needs itself (recursion)
        DeadVertex* സ്ഥിരം parentVtxp = findNewVertex(parentp);
        DeadVertex* സ്ഥിരം virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    ശൂന്യം funcVirtual(Ast_കണ്ണി_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* സ്ഥിരം virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* സ്ഥിരം കണ്ണി_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << കണ്ണി_Vtxp);
        newEdge(virtualVtxp, കണ്ണി_Vtxp);
    }
    തർക്കം funcsEmpty() സ്ഥിരം { തിരികെ m_funcs.empty(); }
    Ast_കണ്ണി* funcs_Get_എടുക്കുക_Front() {
        DeadVertex* സ്ഥിരം vtxp = m_funcs.get_എടുക്കുക_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        എങ്കിൽ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            തിരികെ nullptr;  // Caller will search again
        }
        Ast_കണ്ണി* സ്ഥിരം nodep = vtxp->nodep();
        തിരികെ nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

വർഗ്ഗം DeadVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_കണ്ണി_Assign*>;

    // STATE - across all visitors
    സ്ഥിരം തർക്കം m_elimUserVars;  // Allow removal of user's vars
    സ്ഥിരം തർക്കം m_elimDTypes;  // Allow removal of DTypes
    സ്ഥിരം തർക്കം m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_കണ്ണി*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_കണ്ണി_DType*, Ast_കണ്ണി_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    തർക്കം m_sideEffect = അസത്യം;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    തർക്കം m_inAssign = അസത്യം;  // Currently in an assign
    Ast_കണ്ണി_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_കണ്ണി* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    തർക്കം m_inFTask = അസത്യം;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    ശൂന്യം deleting(Ast_കണ്ണി* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_കണ്ണി_Vertex(nodep);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    ശൂന്യം checkAll(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം subnodep = nodep->dtypep()) {
            എങ്കിൽ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    ശൂന്യം checkVarRef(Ast_കണ്ണി_Var_Ref* nodep) സ്ഥിരം {
        എങ്കിൽ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    ശൂന്യം checkDType(Ast_കണ്ണി_DType* nodep) {
        എങ്കിൽ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.തള്ളുക_back(nodep);
        }
        എങ്കിൽ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep = VN_CAST(nodep, കണ്ണി_UOr_Struct_DType)) {
            എങ്കിൽ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    ശൂന്യം needsTask(Ast_കണ്ണി_FTask* taskp, Ast_കണ്ണി* containerp) {
        എങ്കിൽ (!taskp) തിരികെ;  // Unlinked
        എങ്കിൽ (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } അല്ലെങ്കിൽ {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        എങ്കിൽ (nodep->dead()) തിരികെ;
        എങ്കിൽ (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) {
            എങ്കിൽ (classp->extendsp()) classp->extendsp()->user1Inc();
            എങ്കിൽ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.തള്ളുക_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        വേണ്ടി (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    ശൂന്യം visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    ശൂന്യം visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        എങ്കിൽ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        എങ്കിൽ (m_modp->isConstPool()) nodep->user1Inc();
        എങ്കിൽ (!nodep->is_മുകൾ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.തള്ളുക_back(nodep);
        }
    }
    ശൂന്യം visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.തള്ളുക_back(nodep);
        nodep->modp()->user1Inc();
    }

    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        എങ്കിൽ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        എങ്കിൽ (nodep->varp()) nodep->varp()->user1Inc();
        എങ്കിൽ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!m_sideEffect && !nodep->isPure()) m_sideEffect = സത്യം;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        എങ്കിൽ (nodep->classOrPackagep()) {
            എങ്കിൽ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } അല്ലെങ്കിൽ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ശൂന്യം visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    ശൂന്യം visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->varp()) nodep->varp()->user1Inc();
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        എങ്കിൽ (nodep->classOrPackagep()) {
            എങ്കിൽ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } അല്ലെങ്കിൽ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->classOrPackagep()) {
            എങ്കിൽ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } അല്ലെങ്കിൽ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        എങ്കിൽ (nodep->classp()) nodep->classp()->user1Inc();
    }
    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->modportp()) {
            എങ്കിൽ (m_elimCells) {
                nodep->modportp(nullptr);
            } അല്ലെങ്കിൽ {
                nodep->modportp()->user1Inc();
            }
        }
        എങ്കിൽ (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    ശൂന്യം visit(Ast_കണ്ണി_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->classOrPackagep()) {
            എങ്കിൽ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } അല്ലെങ്കിൽ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->varp()) nodep->varp()->user1Inc();
        എങ്കിൽ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    ശൂന്യം visit(AstModport* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_elimCells) {
            എങ്കിൽ (!nodep->varsp()) {
                deleting(nodep);
                തിരികെ;
            }
        }
        checkAll(nodep);
    }
    ശൂന്യം visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    ശൂന്യം visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.തള്ളുക_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        എങ്കിൽ (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->scopep()) nodep->scopep()->user1Inc();
        എങ്കിൽ (mightElimVar(nodep->varp())) m_vscsp.തള്ളുക_back(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        എങ്കിൽ (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        എങ്കിൽ (mightElimVar(nodep)) {
            m_varsp.തള്ളുക_back(nodep);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        സ്ഥിരം തർക്കം assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = സത്യം;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->lhsp(), VarRef);
            എങ്കിൽ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } അല്ലെങ്കിൽ {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        എങ്കിൽ (assignInAssign) m_sideEffect = സത്യം;  // Parent assign shouldn't optimize
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        സ്ഥിരം തർക്കം removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = സത്യം;
        iterateChildren(nodep);
        checkAll(nodep);
        എങ്കിൽ (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        എങ്കിൽ (!removable) {
            എങ്കിൽ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        എങ്കിൽ (nodep->classOrPackagep()) {
            എങ്കിൽ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } അല്ലെങ്കിൽ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    ശൂന്യം visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!m_sideEffect && !nodep->isPure()) m_sideEffect = സത്യം;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    ശൂന്യം deadCheckTypedefs() {
        വേണ്ടി (AstTypedef* typedefp : m_typedefsp) {
            എങ്കിൽ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                തുടരുക;
            }
            checkAll(typedefp);
        }
    }
    തർക്കം shouldDeleteTypedef(സ്ഥിരം AstTypedef* typedefp) സ്ഥിരം {
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം structp = VN_CAST(typedefp->subDTypep(), കണ്ണി_UOr_Struct_DType)) {
            എങ്കിൽ (structp->user1() && !structp->packed()) തിരികെ അസത്യം;
        }
        തിരികെ m_elimCells && !typedefp->attrPublic();
    }

    ശൂന്യം deadCheckTasks() {
        ആകുമ്പോൾ (!m_graph.funcsEmpty()) {
            Ast_കണ്ണി* സ്ഥിരം nodep = m_graph.funcs_Get_എടുക്കുക_Front();
            എങ്കിൽ (!nodep) തുടരുക;
            UINFO(9, "Dead " << nodep);
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം taskp = VN_CAST(nodep, കണ്ണി_FTask)) {
                എങ്കിൽ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } അല്ലെങ്കിൽ എങ്കിൽ (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } അല്ലെങ്കിൽ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    ശൂന്യം deadCheckDemote() {
        വേണ്ടി (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* സ്ഥിരം vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            എങ്കിൽ (!vtxp->outSize1()) തുടരുക;
            എങ്കിൽ (!vtxp->isVirtual()) തുടരുക;
            വേണ്ടി (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_കണ്ണി* സ്ഥിരം nodep = edge.മുകൾ()->as<DeadVertex>()->nodep();
                Ast_കണ്ണി_FTask* സ്ഥിരം funcp = VN_AS(nodep, കണ്ണി_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                എങ്കിൽ (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(അസത്യം);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    ശൂന്യം deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        വേണ്ടി (തർക്കം retry = സത്യം; retry;) {
            retry = അസത്യം;
            Ast_കണ്ണി_Module* nextmodp;
            വേണ്ടി (Ast_കണ്ണി_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), കണ്ണി_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                സ്ഥിരം തർക്കം keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                എങ്കിൽ (modp->dead() || (!modp->is_മുകൾ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    എങ്കിൽ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](സ്ഥിരം AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    എങ്കിൽ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = സത്യം;
                }
            }
        }
    }
    തർക്കം mightElimVar(സ്ഥിരം AstVar* nodep) സ്ഥിരം {
        എങ്കിൽ (nodep->isSigPublic()) തിരികെ അസത്യം;  // Can't elim publics!
        എങ്കിൽ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->isTemp() && !nodep->isTrace()) തിരികെ സത്യം;
        തിരികെ m_elimUserVars;  // Post-Trace can kill most anything
    }

    ശൂന്യം deadCheckScope() {
        വേണ്ടി (തർക്കം retry = സത്യം; retry;) {
            retry = അസത്യം;
            വേണ്ടി (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* സ്ഥിരം scp = *it;
                എങ്കിൽ (!scp) തുടരുക;
                എങ്കിൽ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    എങ്കിൽ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = സത്യം;
                }
            }
        }
    }

    ശൂന്യം deadCheckCells() {
        വേണ്ടി (AstCell* cellp : m_cellsp) {
            എങ്കിൽ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    ശൂന്യം deadCheckClasses() {
        വേണ്ടി (തർക്കം retry = സത്യം; retry;) {
            retry = അസത്യം;
            വേണ്ടി (auto& itr : m_classesp) {
                എങ്കിൽ (AstClass* സ്ഥിരം nodep = itr) {  // nullptr if deleted earlier
                    എങ്കിൽ (nodep->user1() == 0) {
                        എങ്കിൽ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        എങ്കിൽ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = സത്യം;
                    }
                }
            }
        }
    }

    ശൂന്യം deadCheckVar() {
        // Delete any unused varscopes
        വേണ്ടി (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            എങ്കിൽ (vscp->varp()->user2()) തുടരുക;
            എങ്കിൽ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                സ്ഥിരം std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                വേണ്ടി (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_കണ്ണി_Assign* സ്ഥിരം assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                എങ്കിൽ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        വേണ്ടി (തർക്കം retry = സത്യം; retry;) {
            retry = അസത്യം;
            വേണ്ടി (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* സ്ഥിരം varp = *it;
                എങ്കിൽ (!varp) തുടരുക;
                എങ്കിൽ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    എങ്കിൽ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = സത്യം;
                }
            }
        }
        വേണ്ടി (std::vector<Ast_കണ്ണി*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            എങ്കിൽ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം classp
                    = VN_CAST((*it), കണ്ണി_UOr_Struct_DType)) {
                    തർക്കം cont = സത്യം;
                    വേണ്ടി (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        എങ്കിൽ (memberp->user1() != 0) {
                            cont = അസത്യം;
                            നിർത്തുക;
                        }
                    }
                    എങ്കിൽ (!cont) തുടരുക;
                }
                deleting(*it);
            }
        }
    }

പൊതു:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, തർക്കം elimUserVars, തർക്കം elimDTypes, തർക്കം elimScopes,
                തർക്കം elimCells, തർക്കം elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        എങ്കിൽ (AstVarScope* സ്ഥിരം vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        വേണ്ടി (auto& itr : m_dtypePkgsp) {
            എങ്കിൽ (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        എങ്കിൽ (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", അസത്യം);

        എങ്കിൽ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        എങ്കിൽ (elimScopes) deadCheckScope();
        എങ്കിൽ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        എങ്കിൽ (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_വൃക്ഷം();  // Removing assignments may affect isPure
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

ശൂന്യം V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, അസത്യം, അസത്യം, അസത്യം, അസത്യം, അസത്യം};
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("deadModules", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}

ശൂന്യം V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, അസത്യം, സത്യം, അസത്യം, അസത്യം, സത്യം}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("deadDtypes", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, അസത്യം, സത്യം, സത്യം, അസത്യം, അസത്യം}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("deadDtypesScoped", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, സത്യം, സത്യം, അസത്യം, സത്യം, സത്യം}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("deadAll", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, സത്യം, സത്യം, സത്യം, സത്യം, സത്യം}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("deadAllScoped", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
