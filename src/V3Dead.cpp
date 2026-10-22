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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dead.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <queue>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

తరగతి Dead_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Dead_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    తార్కిక m_removable = అబద్ధం;  // Subject to dead removal
    తార్కిక m_isVirtual = అబద్ధం;  // Virtual node, indirection

బహిరంగ:
    Dead_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Dead_శీర్షం() override = అప్రమేయ;
    string dotShape() స్థిరరాశి override {
        తిరిగి isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_కణుపు* nodep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_nodep; }
    string name() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvt_గమ్యం_Hex(nodep()) + ' ' + nodep()->name();
    }
    తార్కిక removable() స్థిరరాశి { తిరిగి m_removable; }
    శూన్య removable(తార్కిక flag) { m_removable = flag; }
    తార్కిక isVirtual() స్థిరరాశి { తిరిగి m_isVirtual; }
    శూన్య isVirtual(తార్కిక flag) { m_isVirtual = flag; }
    uint64_t workPos() స్థిరరాశి { తిరిగి m_workPos; }
    శూన్య workPos(uint64_t విలువ) { m_workPos = విలువ; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
తరగతి DeadWorkList final {
    // MEMBERS
    స్థిర uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, Dead_శీర్షం*> m_పనిచేస్తుంది;  // Work list by sequence
బహిరంగ:
    // METHODS
    తార్కిక ఖాళీలు() స్థిరరాశి { తిరిగి m_పనిచేస్తుంది.ఖాళీలు(); }
    శూన్య నెట్టు(Dead_శీర్షం* vtxp) {
        ఒకవేళ (vtxp->workPos()) తిరిగి;  // Already on list
        స్థిరరాశి uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_పనిచేస్తుంది.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    శూన్య erase(Dead_శీర్షం* vtxp) {
        స్థిరరాశి uint64_t id = vtxp->workPos();
        ఒకవేళ (!id) తిరిగి;
        UINFO(9, "Worklist.erase " << vtxp);
        స్థిరరాశి స్వయంచాలక it = m_పనిచేస్తుంది.కనుగొను(id);
        UASSERT_OBJ(it != m_పనిచేస్తుంది.చివర(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_పనిచేస్తుంది.erase(it);
        vtxp->workPos(0);
    }
    Dead_శీర్షం* get_తీయి_Front() {
        UDEBUGONLY(UASSERT(!ఖాళీలు(), "Front invalid to call on empty list"););
        స్థిరరాశి స్వయంచాలక it = m_పనిచేస్తుంది.begin();
        Dead_శీర్షం* స్థిరరాశి vtxp = it->రెండవ;
        m_పనిచేస్తుంది.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        తిరిగి vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

తరగతి Dead_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, Dead_శీర్షం*> m_virtualVtxsp;

    శూన్య new_అంచు(Dead_శీర్షం* fromp, Dead_శీర్షం* శిఖరం) {
        కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, fromp, శిఖరం, 1, అబద్ధం};
        ఒకవేళ (శిఖరం->removable()) m_funcs.erase(శిఖరం);  // Now has an input edge
    }
    శూన్య నెట్టు_Work_Maybe(Dead_శీర్షం* vtxp, తార్కిక allowSize1) {
        ఒకవేళ (vtxp->removable()) {
            ఒకవేళ (VN_IS(vtxp->nodep(), కణుపు_FTask)
                && (vtxp->లోపల_ఖాళీలు() || (allowSize1 && vtxp->లోపల_Size1())))
                m_funcs.నెట్టు(vtxp);
        }
    }

బహిరంగ:
    // METHODS
    Dead_లేఖాచిత్రం() = అప్రమేయ;
    ~Dead_లేఖాచిత్రం() override = అప్రమేయ;
    Dead_శీర్షం* కనుగొను_New_శీర్షం(Ast_కణుపు* nodep) {
        Dead_శీర్షం* vtxp = nodep->user2u().గమ్యం<Dead_శీర్షం*>();
        ఒకవేళ (!vtxp) {
            vtxp = కొత్త Dead_శీర్షం{ఇది, nodep};
            nodep->user2p(vtxp);
            నెట్టు_Work_Maybe(vtxp, అబద్ధం);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        తిరిగి vtxp;
    }
    Dead_శీర్షం* కనుగొను_Virtual_శీర్షం(Ast_కణుపు* nodep) {
        స్థిరరాశి స్వయంచాలక it = m_virtualVtxsp.కనుగొను(nodep->name());
        Dead_శీర్షం* vtxp;
        ఒకవేళ (it != m_virtualVtxsp.చివర()) {
            vtxp = it->రెండవ;
        } లేకపోతే {
            vtxp = కొత్త Dead_శీర్షం{ఇది, nodep};
            vtxp->removable(నిజం);
            vtxp->isVirtual(నిజం);
            నెట్టు_Work_Maybe(vtxp, అబద్ధం);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        తిరిగి vtxp;
    }
    శూన్య కనుగొను_New_Removable_శీర్షం(Ast_కణుపు* nodep, తార్కిక removable) {
        Dead_శీర్షం* స్థిరరాశి vtxp = కనుగొను_New_శీర్షం(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ఒకవేళ (removable && !vtxp->removable()) {
            vtxp->removable(నిజం);
            నెట్టు_Work_Maybe(vtxp, అబద్ధం);
        }
    }
    శూన్య delete_కణుపు_శీర్షం(Ast_కణుపు* nodep) {
        ఒకవేళ (Dead_శీర్షం* స్థిరరాశి vtxp = nodep->user2u().గమ్యం<Dead_శీర్షం*>()) delete_శీర్షం(vtxp);
    }
    శూన్య delete_శీర్షం(Dead_శీర్షం* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& oedge : vtxp->బహిర్గతం_అంచులు()) {
            Dead_శీర్షం* స్థిరరాశి గమ్యం_Vtxp = static_cast<Dead_శీర్షం*>(oedge.శిఖరం());
            నెట్టు_Work_Maybe(గమ్యం_Vtxp, నిజం);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ఒకవేళ (!vtxp->isVirtual()) vtxp->nodep()->user2p(శూన్యసూచిక);
        ఒకవేళ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(ఇది), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    శూన్య needs(Ast_కణుపు* nodep, Ast_కణుపు* parentp) {
        ఒకవేళ (parentp == nodep) తిరిగి;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        Dead_శీర్షం* స్థిరరాశి మాతృ_Vtxp = కనుగొను_New_శీర్షం(parentp);
        Dead_శీర్షం* స్థిరరాశి కణుపు_Vtxp = కనుగొను_New_శీర్షం(nodep);
        UINFO(9, "Edge need " << మాతృ_Vtxp << " -> " << కణుపు_Vtxp);
        new_అంచు(మాతృ_Vtxp, కణుపు_Vtxp);
    }
    శూన్య needsVirtual(Ast_కణుపు_FTask* nodep, Ast_కణుపు* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ఒకవేళ (parentp == nodep) తిరిగి;  // No need for tracking needs itself (recursion)
        Dead_శీర్షం* స్థిరరాశి మాతృ_Vtxp = కనుగొను_New_శీర్షం(parentp);
        Dead_శీర్షం* స్థిరరాశి virtualVtxp = కనుగొను_Virtual_శీర్షం(nodep);
        UINFO(9, "Edge needVirtual " << మాతృ_Vtxp << " -> " << virtualVtxp);
        new_అంచు(మాతృ_Vtxp, virtualVtxp);
    }
    శూన్య funcVirtual(Ast_కణుపు_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        Dead_శీర్షం* స్థిరరాశి virtualVtxp = కనుగొను_Virtual_శీర్షం(nodep);
        Dead_శీర్షం* స్థిరరాశి కణుపు_Vtxp = కనుగొను_New_శీర్షం(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << కణుపు_Vtxp);
        new_అంచు(virtualVtxp, కణుపు_Vtxp);
    }
    తార్కిక funcs_ఖాళీలు() స్థిరరాశి { తిరిగి m_funcs.ఖాళీలు(); }
    Ast_కణుపు* funcs_Get_తీయి_Front() {
        Dead_శీర్షం* స్థిరరాశి vtxp = m_funcs.get_తీయి_Front();
        UASSERT_OBJ(vtxp->లోపల_ఖాళీలు(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ఒకవేళ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            delete_శీర్షం(vtxp);
            తిరిగి శూన్యసూచిక;  // Caller will search again
        }
        Ast_కణుపు* స్థిరరాశి nodep = vtxp->nodep();
        తిరిగి nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

తరగతి DeadVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;  // For usage information see DeadGraph

    // TYPES
    ఉపయోగించి కేటాయించు_Map = std::multimap<AstVarScope*, Ast_కణుపు_కేటాయించు*>;

    // STATE - across all visitors
    స్థిరరాశి తార్కిక m_elimUserVars;  // Allow removal of user's vars
    స్థిరరాశి తార్కిక m_elimDTypes;  // Allow removal of DTypes
    స్థిరరాశి తార్కిక m_elimCells;  // Allow removal of Cells

    Dead_లేఖాచిత్రం m_లేఖాచిత్రం;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::సదిశ<AstVar*> m_varsp;
    std::సదిశ<Ast_కణుపు*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_కణుపు_DType*, Ast_కణుపు_Module*> m_dtypePkgsp;  // Data type's containing package
    std::సదిశ<AstVarScope*> m_vscsp;
    std::సదిశ<AstScope*> m_scopesp;
    std::సదిశ<AstCell*> m_cellsp;
    std::సదిశ<AstClass*> m_classesp;
    std::సదిశ<AstTypedef*> m_typedefsp;
    కేటాయించు_Map m_కేటాయించు_Map;  // List of all simple assignments for each variable
    తార్కిక m_sideEffect = అబద్ధం;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    తార్కిక m_లోపల_కేటాయించు = అబద్ధం;  // Currently in an assign
    Ast_కణుపు_DType* m_curDTypep = శూన్యసూచిక;  // Current NodeDType
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_Foreach_శీర్షిక* m_foreachHeaderp = శూన్యసూచిక;  // Current foreach header
    Ast_కణుపు* m_containingFTaskRefp = శూన్యసూచిక;  // Parent of ftaskref (e.g. task/module)
    తార్కిక m_లోపల_FTask = అబద్ధం;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    శూన్య deleting(Ast_కణుపు* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_లేఖాచిత్రం.delete_కణుపు_శీర్షం(nodep);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    శూన్య checkAll(Ast_కణుపు* nodep) {
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి subnodep = nodep->dtypep()) {
            ఒకవేళ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి subnodep = nodep->get_బిడ్డ_DTypep()) subnodep->user1Inc();
    }
    శూన్య checkVarRef(Ast_కణుపు_Var_Ref* nodep) స్థిరరాశి {
        ఒకవేళ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(శూన్యసూచిక);
    }
    శూన్య checkDType(Ast_కణుపు_DType* nodep) {
        ఒకవేళ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.నెట్టు_వెనుక(nodep);
        }
        ఒకవేళ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep = VN_CAST(nodep, కణుపు_UOr_Struct_DType)) {
            ఒకవేళ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    శూన్య needsTask(Ast_కణుపు_FTask* taskp, Ast_కణుపు* containerp) {
        ఒకవేళ (!taskp) తిరిగి;  // Unlinked
        ఒకవేళ (taskp->isVirtual()) {
            m_లేఖాచిత్రం.needsVirtual(taskp, containerp);
        } లేకపోతే {
            m_లేఖాచిత్రం.needs(taskp, containerp);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ఒకవేళ (nodep->dead()) తిరిగి;
        ఒకవేళ (nodep->mod_ప్రజా()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) {
            ఒకవేళ (classp->extendsp()) classp->extendsp()->user1Inc();
            ఒకవేళ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.నెట్టు_వెనుక(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        కొరకు (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    శూన్య సందర్శించు(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ఒకవేళ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ఒకవేళ (m_modp->isConstPool()) nodep->user1Inc();
        ఒకవేళ (!nodep->is_శిఖరం() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.నెట్టు_వెనుక(nodep);
        }
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.నెట్టు_వెనుక(nodep);
        nodep->modp()->user1Inc();
    }

    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ఒకవేళ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ఒకవేళ (nodep->varp()) nodep->varp()->user1Inc();
        ఒకవేళ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!m_sideEffect && !nodep->is_శుద్ధ()) m_sideEffect = నిజం;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ఒకవేళ (nodep->classOrPackagep()) {
            ఒకవేళ (m_elimCells) {
                nodep->classOrPackagep(శూన్యసూచిక);
            } లేకపోతే {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    శూన్య సందర్శించు(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    శూన్య సందర్శించు(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->varp()) nodep->varp()->user1Inc();
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ఒకవేళ (nodep->classOrPackagep()) {
            ఒకవేళ (m_elimCells) {
                nodep->classOrPackagep(శూన్యసూచిక);
            } లేకపోతే {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->classOrPackagep()) {
            ఒకవేళ (m_elimCells) {
                nodep->classOrPackagep(శూన్యసూచిక);
            } లేకపోతే {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ఒకవేళ (nodep->classp()) nodep->classp()->user1Inc();
    }
    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->modportp()) {
            ఒకవేళ (m_elimCells) {
                nodep->modportp(శూన్యసూచిక);
            } లేకపోతే {
                nodep->modportp()->user1Inc();
            }
        }
        ఒకవేళ (nodep->iface_ద్వారా_Cellp()) nodep->iface_ద్వారా_Cellp()->user1Inc();
    }
    శూన్య సందర్శించు(Ast_కణుపు_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->classOrPackagep()) {
            ఒకవేళ (m_elimCells) {
                nodep->classOrPackagep(శూన్యసూచిక);
            } లేకపోతే {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->varp()) nodep->varp()->user1Inc();
        ఒకవేళ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    శూన్య సందర్శించు(AstModport* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_elimCells) {
            ఒకవేళ (!nodep->varsp()) {
                deleting(nodep);
                తిరిగి;
            }
        }
        checkAll(nodep);
    }
    శూన్య సందర్శించు(Ast_Foreach_శీర్షిక* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.నెట్టు_వెనుక(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ఒకవేళ (nodep->attr_ప్రజా() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->scopep()) nodep->scopep()->user1Inc();
        ఒకవేళ (mightElimVar(nodep->varp())) m_vscsp.నెట్టు_వెనుక(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ఒకవేళ (m_లోపల_FTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ఒకవేళ (mightElimVar(nodep)) {
            m_varsp.నెట్టు_వెనుక(nodep);
        } లేకపోతే {
            ఒకవేళ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        స్థిరరాశి తార్కిక కేటాయించు_లోపల_కేటాయించు = m_లోపల_కేటాయించు;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_లోపల_కేటాయించు);
            VL_RESTORER(m_sideEffect);
            m_లోపల_కేటాయించు = నిజం;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_తదుపరి_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ఒకవేళ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_కేటాయించు_Map.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } లేకపోతే {  // Track like any other statement
                iterate_And_తదుపరి_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ఒకవేళ (కేటాయించు_లోపల_కేటాయించు) m_sideEffect = నిజం;  // Parent assign shouldn't optimize
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        స్థిరరాశి తార్కిక removable = !(nodep->task_ప్రజా() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->class_పద్ధతి()));
        m_లేఖాచిత్రం.కనుగొను_New_Removable_శీర్షం(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_లోపల_FTask);
        m_containingFTaskRefp = nodep;
        m_లోపల_FTask = నిజం;
        iterateChildren(nodep);
        checkAll(nodep);
        ఒకవేళ (nodep->isVirtual()) m_లేఖాచిత్రం.funcVirtual(nodep);
        ఒకవేళ (!removable) {
            ఒకవేళ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ఒకవేళ (nodep->classOrPackagep()) {
            ఒకవేళ (m_elimCells) {
                nodep->classOrPackagep(శూన్యసూచిక);
            } లేకపోతే {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    శూన్య సందర్శించు(Ast_Clocking_అంశం* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (!m_sideEffect && !nodep->is_శుద్ధ()) m_sideEffect = నిజం;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    శూన్య deadCheckTypedefs() {
        కొరకు (AstTypedef* typedefp : m_typedefsp) {
            ఒకవేళ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                కొనసాగు;
            }
            checkAll(typedefp);
        }
    }
    తార్కిక shouldDeleteTypedef(స్థిరరాశి AstTypedef* typedefp) స్థిరరాశి {
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి structp = VN_CAST(typedefp->subDTypep(), కణుపు_UOr_Struct_DType)) {
            ఒకవేళ (structp->user1() && !structp->packed()) తిరిగి అబద్ధం;
        }
        తిరిగి m_elimCells && !typedefp->attr_ప్రజా();
    }

    శూన్య deadCheckTasks() {
        అయితే (!m_లేఖాచిత్రం.funcs_ఖాళీలు()) {
            Ast_కణుపు* స్థిరరాశి nodep = m_లేఖాచిత్రం.funcs_Get_తీయి_Front();
            ఒకవేళ (!nodep) కొనసాగు;
            UINFO(9, "Dead " << nodep);
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి taskp = VN_CAST(nodep, కణుపు_FTask)) {
                ఒకవేళ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } లేకపోతే ఒకవేళ (taskp->class_పద్ధతి()) {
                    ++m_statFTasksMDeadified;
                } లేకపోతే {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    శూన్య deadCheckDemote() {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& gvtx : m_లేఖాచిత్రం.vertices()) {
            Dead_శీర్షం* స్థిరరాశి vtxp = gvtx.cast<Dead_శీర్షం>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ఒకవేళ (!vtxp->బహిర్గతం_Size1()) కొనసాగు;
            ఒకవేళ (!vtxp->isVirtual()) కొనసాగు;
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->బహిర్గతం_అంచులు()) {  // Always a single one
                Ast_కణుపు* స్థిరరాశి nodep = అంచు.శిఖరం()->as<Dead_శీర్షం>()->nodep();
                Ast_కణుపు_FTask* స్థిరరాశి funcp = VN_AS(nodep, కణుపు_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ఒకవేళ (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(అబద్ధం);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    శూన్య deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        కొరకు (తార్కిక retry = నిజం; retry;) {
            retry = అబద్ధం;
            Ast_కణుపు_Module* nextmodp;
            కొరకు (Ast_కణుపు_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), కణుపు_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                స్థిరరాశి తార్కిక keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ఒకవేళ (modp->dead() || (!modp->is_శిఖరం() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ఒకవేళ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](స్థిరరాశి AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ఒకవేళ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(శూన్యసూచిక);
                    deleting(modp);
                    retry = నిజం;
                }
            }
        }
    }
    తార్కిక mightElimVar(స్థిరరాశి AstVar* nodep) స్థిరరాశి {
        ఒకవేళ (nodep->is_Sig_ప్రజా()) తిరిగి అబద్ధం;  // Can't elim publics!
        ఒకవేళ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->is_తాత్కాలిక() && !nodep->isTrace()) తిరిగి నిజం;
        తిరిగి m_elimUserVars;  // Post-Trace can kill most anything
    }

    శూన్య deadCheckScope() {
        కొరకు (తార్కిక retry = నిజం; retry;) {
            retry = అబద్ధం;
            కొరకు (std::సదిశ<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.చివర();
                 ++it) {
                AstScope* స్థిరరాశి scp = *it;
                ఒకవేళ (!scp) కొనసాగు;
                ఒకవేళ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ఒకవేళ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = శూన్యసూచిక;
                    retry = నిజం;
                }
            }
        }
    }

    శూన్య deadCheckCells() {
        కొరకు (AstCell* cellp : m_cellsp) {
            ఒకవేళ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    శూన్య deadCheckClasses() {
        కొరకు (తార్కిక retry = నిజం; retry;) {
            retry = అబద్ధం;
            కొరకు (స్వయంచాలక& itr : m_classesp) {
                ఒకవేళ (AstClass* స్థిరరాశి nodep = itr) {  // nullptr if deleted earlier
                    ఒకవేళ (nodep->user1() == 0) {
                        ఒకవేళ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ఒకవేళ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = శూన్యసూచిక;
                        retry = నిజం;
                    }
                }
            }
        }
    }

    శూన్య deadCheckVar() {
        // Delete any unused varscopes
        కొరకు (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ఒకవేళ (vscp->varp()->user2()) కొనసాగు;
            ఒకవేళ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                స్థిరరాశి std::pair<కేటాయించు_Map::iterator, కేటాయించు_Map::iterator> eqrange
                    = m_కేటాయించు_Map.equal_range(vscp);
                కొరకు (కేటాయించు_Map::iterator itr = eqrange.మొదటి; itr != eqrange.రెండవ; ++itr) {
                    Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = itr->రెండవ;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ఒకవేళ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        కొరకు (తార్కిక retry = నిజం; retry;) {
            retry = అబద్ధం;
            కొరకు (std::సదిశ<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.చివర(); ++it) {
                AstVar* స్థిరరాశి varp = *it;
                ఒకవేళ (!varp) కొనసాగు;
                ఒకవేళ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ఒకవేళ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = శూన్యసూచిక;
                    retry = నిజం;
                }
            }
        }
        కొరకు (std::సదిశ<Ast_కణుపు*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.చివర();
             ++it) {
            ఒకవేళ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి classp
                    = VN_CAST((*it), కణుపు_UOr_Struct_DType)) {
                    తార్కిక cont = నిజం;
                    కొరకు (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ఒకవేళ (memberp->user1() != 0) {
                            cont = అబద్ధం;
                            ఆపు;
                        }
                    }
                    ఒకవేళ (!cont) కొనసాగు;
                }
                deleting(*it);
            }
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, తార్కిక elimUserVars, తార్కిక elimDTypes, తార్కిక elimScopes,
                తార్కిక elimCells, తార్కిక elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        ఒకవేళ (AstVarScope* స్థిరరాశి vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        కొరకు (స్వయంచాలక& itr : m_dtypePkgsp) {
            ఒకవేళ (itr.మొదటి->user1()) itr.రెండవ->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_లేఖాచిత్రం.remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9 || debug() >= 9)
            m_లేఖాచిత్రం.dumpDotFilePrefixed("dead_graph", అబద్ధం);

        ఒకవేళ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ఒకవేళ (elimScopes) deadCheckScope();
        ఒకవేళ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ఒకవేళ (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_వృక్షం();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::కూడు_Stat_కూడిక("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

శూన్య V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, అబద్ధం, అబద్ధం, అబద్ధం, అబద్ధం, అబద్ధం};
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("deadModules", 0, dump_వృక్షం_Either_Level() >= 6);
}

శూన్య V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, అబద్ధం, నిజం, అబద్ధం, అబద్ధం, నిజం}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("deadDtypes", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, అబద్ధం, నిజం, నిజం, అబద్ధం, అబద్ధం}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("deadDtypesScoped", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, నిజం, నిజం, అబద్ధం, నిజం, నిజం}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("deadAll", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, నిజం, నిజం, నిజం, నిజం, నిజం}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("deadAllScoped", 0, dump_వృక్షం_Either_Level() >= 3);
}
