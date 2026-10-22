// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Module inlining
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
// V3Inline's Transformations:
//
// Each module:
//      Look for CELL... PRAGMA INLINE_MODULE
//          Replicate the cell's module
//              Rename vars to include cell name
//          Insert cell's module declarations into the upper module
//          Merge each SCOPE of the cell's module into the SCOPE above it
//          Reparent and rename the SCOPEs below the inlined instance
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Inline.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
સ્થિર અચલ પૂર્ણાંક INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

વર્ગ InlineModModuleVertex;
વર્ગ InlineModCellVertex;

વર્ગ InlineModGraph final : જાહેર V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

જાહેર:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = મૂળભૂત;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_ગાંઠ_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    શૂન્ય addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    શૂન્ય addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() અચલ override { પાછા "LR"; }
};

વર્ગ InlineModEitherVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
સુરક્ષિત:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

વર્ગ InlineModModuleVertex final : જાહેર InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_ગાંઠ_Module* અચલ m_modp;  // The module
    અચલ અક્ષર* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    અચલ અક્ષર* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    અચલ અક્ષર* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    તાર્કિક mutable m_flattenedSizeValid = ખોટું;  // Whether the flattened size is valid
    size_t mutable m_instance_ગણતરી = 0;  // The number of total instances of this module
    તાર્કિક mutable m_instance_ગણતરી_Valid = ખોટું;  // Whether the instance count is valid

જાહેર:
    InlineModModuleVertex(InlineModGraph& graph, Ast_ગાંઠ_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = મૂળભૂત;

    // ACCESSORS
    Ast_ગાંઠ_Module* modp() અચલ { પાછા m_modp; }
    size_t size() અચલ { પાછા m_size; }
    શૂન્ય size(size_t કિંમત) { m_size = કિંમત; }
    શૂન્ય sizeInc(size_t કિંમત = 1) { m_size += કિંમત; }
    તાર્કિક noInlineHard() અચલ { પાછા m_noInlineHardWyp; }
    શૂન્ય setNoInlineHard(અચલ અક્ષર* whyp) {
        જો (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    તાર્કિક noInlineSoft() અચલ { પાછા m_noInlineSoftWyp; }
    શૂન્ય setNoInlineSoft(અચલ અક્ષર* whyp) {
        જો (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    તાર્કિક shouldInline() અચલ { પાછા m_shouldInlineWhyp; }
    શૂન્ય setShouldInline(અચલ અક્ષર* whyp) {
        જો (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    શૂન્ય setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() અચલ {
        જો (!m_flattenedSizeValid) {
            m_flattenedSizeValid = સાચું;
            m_flattenedSize = m_size;
            માટે (અચલ V3GraphEdge& e1 : outEdges()) {
                માટે (અચલ V3GraphEdge& e2 : e1.ટોચ()->outEdges()) {
                    InlineModModuleVertex* અચલ mVtxp = e2.ટોચ()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        પાછા m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_ગણતરી() અચલ {
        જો (!m_instance_ગણતરી_Valid) {
            m_instance_ગણતરી_Valid = સાચું;
            m_instance_ગણતરી = 0;
            માટે (અચલ V3GraphEdge& e1 : inEdges()) {
                માટે (અચલ V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* અચલ mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_ગણતરી += mVtxp->instance_ગણતરી();
                }
            }
            જો (!m_instance_ગણતરી) {
                UASSERT_OBJ(m_modp->is_ટોચ(), m_modp, "non-top level module should have instances");
                m_instance_ગણતરી = 1;
            }
        }
        પાછા m_instance_ગણતરી;
    }

    // debug
    File_લીટી* fileline() અચલ override { પાછા m_modp->fileline(); }
    std::string dotShape() અચલ override { પાછા "box"; }
    std::string dotColor() અચલ override {
        પાછા m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() અચલ override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_ગણતરી());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        જો (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        જો (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        જો (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        પાછા str;
    }
};

વર્ગ InlineModCellVertex final : જાહેર InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* અચલ m_cellp;  // The cell (instance)
    અચલ અક્ષર* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    તાર્કિક m_flatten = ખોટું;  // Whether this cell and below already flattened (avoid O(n^2))

જાહેર:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = મૂળભૂત;

    // ACCESSORS
    AstCell* cellp() અચલ { પાછા m_cellp; }
    તાર્કિક doInline() અચલ { પાછા m_doInlineWyp; }
    શૂન્ય setDoInline(અચલ અક્ષર* whyp) {
        જો (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    તાર્કિક flatten() અચલ { પાછા m_flatten; }
    શૂન્ય setFlatten() { m_flatten = સાચું; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() અચલ {
        UASSERT_OBJ(outSize1(), આ, "Cell should have exactly one outgoing edge");
        પાછા *outEdges().frontp()->ટોચ()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() અચલ {
        UASSERT_OBJ(inSize1(), આ, "Cell should have exactly one incoming edge");
        પાછા *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_લીટી* fileline() અચલ override { પાછા m_cellp->fileline(); }
    std::string dotColor() અચલ override { પાછા m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() અચલ override { પાછા "ellipse"; }
    std::string name() અચલ override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        જો (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        પાછા str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_ગાંઠ_Module* modp) {
    જો (!modp->user4p()) modp->user4p(નવું InlineModModuleVertex{*આ, modp});
    પાછા modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    જો (!cellp->user4p()) cellp->user4p(નવું InlineModCellVertex{*આ, cellp});
    પાછા cellp->user4u().to<InlineModCellVertex*>();
}

શૂન્ય InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    નવું V3GraphEdge{આ, &parent, &cell, 1, /* cutable: */ ખોટું};
}
શૂન્ય InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    નવું V3GraphEdge{આ, &cell, &submodule, 1, /* cutable: */ ખોટું};
}

શૂન્ય InlineModModuleVertex::setFlatten() {
    માટે (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.ટોચ()->as<InlineModCellVertex>();
        જો (cVtx.flatten()) ચાલુ;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        જો (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

વર્ગ InlineModGraphBuilder final : જાહેર VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{નવું InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    અચલ AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* અચલ vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        જો (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        જો (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        જો (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        જો (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        જો (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        જો (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    શૂન્ય visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        જો (!m_sizedScopep) m_sizedScopep = nodep;
        જો (m_sizedScopep != nodep) પાછા;
        જો (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    શૂન્ય visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* અચલ vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstPragma* nodep) override {
        જો (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            જો (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } નહીંતર {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }

        જો (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            જો (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } નહીંતર {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit InlineModGraphBuilder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->order();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_ટોચ(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        માટે (અચલ V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            જો (&vtx == m_graphp->vertices().frontp()) ચાલુ;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = મૂળભૂત;

જાહેર:
    સ્થિર std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        પાછા std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

નામાકાશ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<અચલ AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
શૂન્ય gatherScopes(અચલ Ast_ગાંઠ_Module* modp, ScopeChildren& children) {
    માટે (Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        જો (AstScope* અચલ scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ધકેલો_back(scopep);
        } નહીંતર જો (અચલ Ast_ગાંઠ_Module* અચલ subModp = VN_CAST(nodep, ગાંઠ_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
શૂન્ય renameScopes(AstScope* scopep, અચલ std::string& oldPrefix, અચલ std::string& newPrefix,
                  અચલ ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    અચલ auto it = children.find(scopep);
    જો (it == children.end()) પાછા;
    માટે (AstScope* અચલ childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
શૂન્ય inlineScope(AstScope* scopep, AstCell* cellp, અચલ std::string& prefix,
                 ScopeChildren& children) {
    AstScope* અચલ parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    માટે (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        જો (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        જો (AstVar* અચલ newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        જો (!cellp->isTrace()) vscp->trace(ખોટું);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    માટે (Ast_ગાંઠ *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        જો (Ast_ગાંઠ_FTask* અચલ ftaskp = VN_CAST(nodep, ગાંઠ_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        જો (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ગાંઠ_Cover_Decl* અચલ declp = incp->declp();
                જો (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    જો (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    પાછા;
                }
                જો (Ast_ગાંઠ_Cover_Decl* અચલ newDeclp = VN_CAST(declp->user3p(), ગાંઠ_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    માટે (Ast_ગાંઠ *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* અચલ cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    જો (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            નવું AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    અચલ std::string oldPrefix = scopep->name() + ".";
    અચલ std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        અચલ auto it = children.find(scopep);
        જો (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    માટે (AstScope* અચલ childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        જો (AstCell* અચલ newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            childScopep->aboveCellp(newCellp);
        }
        childScopep->aboveScopep(parentScopep);
        renameScopes(childScopep, oldPrefix, newPrefix, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::vector<AstScope*>& parentChildps = children[parentScopep];
    parentChildps.erase(std::remove(parentChildps.begin(), parentChildps.end(), scopep),
                        parentChildps.end());
    parentChildps.ઉમેરો(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
શૂન્ય inlineCell(Ast_ગાંઠ_Module* modp, AstCell* cellp, તાર્કિક last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    અચલ VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.ધકેલો_Deletep(cellp->unlinkFrBack());

    Ast_ગાંઠ_Module* અચલ subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    માટે (Ast_ગાંઠ *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* અચલ scopep = VN_CAST(nodep, Scope);
        જો (!scopep) ચાલુ;
        scopep->unlinkFrBack();
        જો (scopep->aboveCellp() == cellp) {
            inlineScopeps.ધકેલો_back(scopep);
            deleter.ધકેલો_Deletep(scopep);
        } નહીંતર {
            otherScopeps.ધકેલો_back(scopep);
        }
    }

    // Important: If this is the last cell, then don't clone the instantiated module but
    // inline the original directly. While this requires some special casing, doing so
    // saves us having to temporarily clone the module for the last cell, which
    // significantly reduces Verilator memory usage. This is especially true as often the
    // top few levels of the hierarchy are singleton wrapper modules, which we always
    // inline. In this case this special casing saves us from having to clone essentially
    // the entire netlist, which would in effect double Verilator memory consumption, or
    // worse if we put off deleting the inlined modules until the end. Not having to clone
    // large trees also improves speed.

    // The module we will yank the declarations out of and put into 'modp'
    Ast_ગાંઠ_Module* inlinedp;
    જો (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } નહીંતર {
        inlinedp = subModp->clone_વૃક્ષ(ખોટું);
        // Compute map from the original module items to their clones
        માટે (Ast_ગાંઠ *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        જો (v3Global.opt.coverageFsm()) {
            માટે (Ast_ગાંઠ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                અચલ Ast_ગાંઠ_Cover_Decl* અચલ declp = VN_CAST(nodep, ગાંઠ_Cover_Decl);
                જો (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        માટે (AstScope* અચલ scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.ધકેલો_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    અચલ std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    જો (AstSystemCSection* અચલ sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* અચલ mVtxp = graph.getInlineModModuleVertexp(modp);
    માટે (Ast_ગાંઠ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            જો (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            જો (!cellp->isTrace()) varp->trace(ખોટું);
        } નહીંતર જો (AstCell* અચલ subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* અચલ vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } નહીંતર જો (AstTypedef* અચલ typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } નહીંતર જો (Ast_ગાંઠ_Cover_Decl* અચલ declp = VN_CAST(nodep, ગાંઠ_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            જો (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    માટે (AstScope* અચલ scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
શૂન્ય process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    માટે (Ast_ગાંઠ_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    માટે (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* અચલ cVtxp = vtx.cast<InlineModCellVertex>();
        જો (!cVtxp) ચાલુ;
        જો (!cVtxp->doInline()) ચાલુ;
        cVtxps.ધકેલો_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    માટે (InlineModCellVertex* અચલ cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* અચલ iVtxp = &cVtxp->instanceOf();
        AstCell* અચલ cellp = cVtxp->cellp();
        અચલ તાર્કિક last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        જો (last) {
            જ્યારે (!iVtxp->outEmpty()) {
                InlineModCellVertex* અચલ tVtxp
                    = iVtxp->outEdges().frontp()->ટોચ()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        જો (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ગાંઠ_Var_Ref* refp) {
        AstVarScope* અચલ vscp = refp->varScopep();
        જો (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

શૂન્ય V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    અચલ size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        જો (InlineModModuleVertex* અચલ mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            જો (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            જો (mVtxp->noInlineHard()) ચાલુ;
            // Don't inline if soft off
            જો (mVtxp->noInlineSoft()) ચાલુ;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_ગણતરી();
            જો (totalSize * 5 < designSize) {
                માટે (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* અચલ cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            ચાલુ;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        જો (mVtx.noInlineHard()) ચાલુ;

        // If it should be inlined, inlined it
        જો (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        જો (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        જો (mVtx.noInlineSoft()) ચાલુ;

        // If instatiated in exactly one static site, inline it
        જો (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        જો (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        જો (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        અચલ size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        અચલ size_t limit = v3Global.opt.inlineMult();
        જો (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_વૃક્ષ("inline", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
