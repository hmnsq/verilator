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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Inline.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
ಸ್ಥಿರ ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

ವರ್ಗ InlineModModuleVertex;
ವರ್ಗ InlineModCellVertex;

ವರ್ಗ InlineModGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

ಸಾರ್ವಜನಿಕ:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = ಸ್ವಭಾವ;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_ಗಂಟು_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    ಶೂನ್ಯ addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    ಶೂನ್ಯ addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "LR"; }
};

ವರ್ಗ InlineModEitherVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
ರಕ್ಷಿತ:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

ವರ್ಗ InlineModModuleVertex final : ಸಾರ್ವಜನಿಕ InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // The module
    ಧ್ರುವ ವರ್ಣ* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    ಧ್ರುವ ವರ್ಣ* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    ಧ್ರುವ ವರ್ಣ* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    ತಾರ್ಕಿಕ mutable m_flattenedSizeValid = ಅಸತ್ಯ;  // Whether the flattened size is valid
    size_t mutable m_instance_ಎಣಿಕೆ = 0;  // The number of total instances of this module
    ತಾರ್ಕಿಕ mutable m_instance_ಎಣಿಕೆ_Valid = ಅಸತ್ಯ;  // Whether the instance count is valid

ಸಾರ್ವಜನಿಕ:
    InlineModModuleVertex(InlineModGraph& graph, Ast_ಗಂಟು_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    Ast_ಗಂಟು_Module* modp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modp; }
    size_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_size; }
    ಶೂನ್ಯ size(size_t ಮೌಲ್ಯ) { m_size = ಮೌಲ್ಯ; }
    ಶೂನ್ಯ sizeInc(size_t ಮೌಲ್ಯ = 1) { m_size += ಮೌಲ್ಯ; }
    ತಾರ್ಕಿಕ noInlineHard() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noInlineHardWyp; }
    ಶೂನ್ಯ setNoInlineHard(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    ತಾರ್ಕಿಕ noInlineSoft() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noInlineSoftWyp; }
    ಶೂನ್ಯ setNoInlineSoft(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    ತಾರ್ಕಿಕ shouldInline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_shouldInlineWhyp; }
    ಶೂನ್ಯ setShouldInline(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    ಶೂನ್ಯ setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() ಧ್ರುವ {
        ಯದಿ (!m_flattenedSizeValid) {
            m_flattenedSizeValid = ಸತ್ಯ;
            m_flattenedSize = m_size;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& e1 : outEdges()) {
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& e2 : e1.ತುದಿ()->outEdges()) {
                    InlineModModuleVertex* ಧ್ರುವ mVtxp = e2.ತುದಿ()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_ಎಣಿಕೆ() ಧ್ರುವ {
        ಯದಿ (!m_instance_ಎಣಿಕೆ_Valid) {
            m_instance_ಎಣಿಕೆ_Valid = ಸತ್ಯ;
            m_instance_ಎಣಿಕೆ = 0;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& e1 : inEdges()) {
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* ಧ್ರುವ mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_ಎಣಿಕೆ += mVtxp->instance_ಎಣಿಕೆ();
                }
            }
            ಯದಿ (!m_instance_ಎಣಿಕೆ) {
                UASSERT_OBJ(m_modp->is_ತುದಿ(), m_modp, "non-top level module should have instances");
                m_instance_ಎಣಿಕೆ = 1;
            }
        }
        ಪ್ರತ್ಯಾಗಮ m_instance_ಎಣಿಕೆ;
    }

    // debug
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_modp->fileline(); }
    std::string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "box"; }
    std::string dotColor() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ಧ್ರುವ override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_ಎಣಿಕೆ());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        ಯದಿ (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        ಯದಿ (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        ಯದಿ (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        ಪ್ರತ್ಯಾಗಮ str;
    }
};

ವರ್ಗ InlineModCellVertex final : ಸಾರ್ವಜನಿಕ InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* ಧ್ರುವ m_cellp;  // The cell (instance)
    ಧ್ರುವ ವರ್ಣ* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    ತಾರ್ಕಿಕ m_flatten = ಅಸತ್ಯ;  // Whether this cell and below already flattened (avoid O(n^2))

ಸಾರ್ವಜನಿಕ:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    AstCell* cellp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cellp; }
    ತಾರ್ಕಿಕ doInline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_doInlineWyp; }
    ಶೂನ್ಯ setDoInline(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    ತಾರ್ಕಿಕ flatten() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_flatten; }
    ಶೂನ್ಯ setFlatten() { m_flatten = ಸತ್ಯ; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() ಧ್ರುವ {
        UASSERT_OBJ(outSize1(), ಏತತ್, "Cell should have exactly one outgoing edge");
        ಪ್ರತ್ಯಾಗಮ *outEdges().frontp()->ತುದಿ()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() ಧ್ರುವ {
        UASSERT_OBJ(inSize1(), ಏತತ್, "Cell should have exactly one incoming edge");
        ಪ್ರತ್ಯಾಗಮ *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_cellp->fileline(); }
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "ellipse"; }
    std::string name() ಧ್ರುವ override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        ಯದಿ (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        ಪ್ರತ್ಯಾಗಮ str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_ಗಂಟು_Module* modp) {
    ಯದಿ (!modp->user4p()) modp->user4p(ನೂತನ InlineModModuleVertex{*ಏತತ್, modp});
    ಪ್ರತ್ಯಾಗಮ modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    ಯದಿ (!cellp->user4p()) cellp->user4p(ನೂತನ InlineModCellVertex{*ಏತತ್, cellp});
    ಪ್ರತ್ಯಾಗಮ cellp->user4u().to<InlineModCellVertex*>();
}

ಶೂನ್ಯ InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    ನೂತನ V3GraphEdge{ಏತತ್, &parent, &cell, 1, /* cutable: */ ಅಸತ್ಯ};
}
ಶೂನ್ಯ InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    ನೂತನ V3GraphEdge{ಏತತ್, &cell, &submodule, 1, /* cutable: */ ಅಸತ್ಯ};
}

ಶೂನ್ಯ InlineModModuleVertex::setFlatten() {
    ಪ್ರತಿ (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.ತುದಿ()->as<InlineModCellVertex>();
        ಯದಿ (cVtx.flatten()) ಅನುವರ್ತ;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        ಯದಿ (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

ವರ್ಗ InlineModGraphBuilder final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{ನೂತನ InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    ಧ್ರುವ AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* ಧ್ರುವ vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        ಯದಿ (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        ಯದಿ (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        ಯದಿ (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        ಯದಿ (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        ಯದಿ (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        ಯದಿ (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    ಶೂನ್ಯ visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        ಯದಿ (!m_sizedScopep) m_sizedScopep = nodep;
        ಯದಿ (m_sizedScopep != nodep) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* ಧ್ರುವ vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstPragma* nodep) override {
        ಯದಿ (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            ಯದಿ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ಅನ್ಯಥಾ {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            ಯದಿ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ಅನ್ಯಥಾ {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (m_modVtxp) m_modVtxp->sizeInc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_ತುದಿ(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            ಯದಿ (&vtx == m_graphp->vertices().frontp()) ಅನುವರ್ತ;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        ಪ್ರತ್ಯಾಗಮ std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

ನಾಮಾಕಾಶ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<ಧ್ರುವ AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
ಶೂನ್ಯ gatherScopes(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ScopeChildren& children) {
    ಪ್ರತಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ಯದಿ (AstScope* ಧ್ರುವ scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ತಳ್ಳು_back(scopep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ subModp = VN_CAST(nodep, ಗಂಟು_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
ಶೂನ್ಯ renameScopes(AstScope* scopep, ಧ್ರುವ std::string& oldPrefix, ಧ್ರುವ std::string& newPrefix,
                  ಧ್ರುವ ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    ಧ್ರುವ auto it = children.find(scopep);
    ಯದಿ (it == children.end()) ಪ್ರತ್ಯಾಗಮ;
    ಪ್ರತಿ (AstScope* ಧ್ರುವ childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
ಶೂನ್ಯ inlineScope(AstScope* scopep, AstCell* cellp, ಧ್ರುವ std::string& prefix,
                 ScopeChildren& children) {
    AstScope* ಧ್ರುವ parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    ಪ್ರತಿ (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        ಯದಿ (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        ಯದಿ (AstVar* ಧ್ರುವ newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        ಯದಿ (!cellp->isTrace()) vscp->trace(ಅಸತ್ಯ);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    ಪ್ರತಿ (Ast_ಗಂಟು *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        ಯದಿ (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp = incp->declp();
                ಯದಿ (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    ಯದಿ (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    ಪ್ರತ್ಯಾಗಮ;
                }
                ಯದಿ (Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ newDeclp = VN_CAST(declp->user3p(), ಗಂಟು_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    ಪ್ರತಿ (Ast_ಗಂಟು *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ಧ್ರುವ cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    ಯದಿ (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            ನೂತನ AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ಧ್ರುವ std::string oldPrefix = scopep->name() + ".";
    ಧ್ರುವ std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        ಧ್ರುವ auto it = children.find(scopep);
        ಯದಿ (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    ಪ್ರತಿ (AstScope* ಧ್ರುವ childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        ಯದಿ (AstCell* ಧ್ರುವ newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.ಸೇರಿಸು(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
ಶೂನ್ಯ inlineCell(Ast_ಗಂಟು_Module* modp, AstCell* cellp, ತಾರ್ಕಿಕ last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ಧ್ರುವ VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.ತಳ್ಳು_Deletep(cellp->unlinkFrBack());

    Ast_ಗಂಟು_Module* ಧ್ರುವ subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    ಪ್ರತಿ (Ast_ಗಂಟು *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ಧ್ರುವ scopep = VN_CAST(nodep, Scope);
        ಯದಿ (!scopep) ಅನುವರ್ತ;
        scopep->unlinkFrBack();
        ಯದಿ (scopep->aboveCellp() == cellp) {
            inlineScopeps.ತಳ್ಳು_back(scopep);
            deleter.ತಳ್ಳು_Deletep(scopep);
        } ಅನ್ಯಥಾ {
            otherScopeps.ತಳ್ಳು_back(scopep);
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
    Ast_ಗಂಟು_Module* inlinedp;
    ಯದಿ (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } ಅನ್ಯಥಾ {
        inlinedp = subModp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        // Compute map from the original module items to their clones
        ಪ್ರತಿ (Ast_ಗಂಟು *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        ಯದಿ (v3Global.opt.coverageFsm()) {
            ಪ್ರತಿ (Ast_ಗಂಟು *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ಧ್ರುವ Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp = VN_CAST(nodep, ಗಂಟು_Cover_Decl);
                ಯದಿ (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        ಪ್ರತಿ (AstScope* ಧ್ರುವ scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.ತಳ್ಳು_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ಧ್ರುವ std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    ಯದಿ (AstSystemCSection* ಧ್ರುವ sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* ಧ್ರುವ mVtxp = graph.getInlineModModuleVertexp(modp);
    ಪ್ರತಿ (Ast_ಗಂಟು *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            ಯದಿ (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            ಯದಿ (!cellp->isTrace()) varp->trace(ಅಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (AstCell* ಧ್ರುವ subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* ಧ್ರುವ vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } ಅನ್ಯಥಾ ಯದಿ (AstTypedef* ಧ್ರುವ typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp = VN_CAST(nodep, ಗಂಟು_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            ಯದಿ (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    ಪ್ರತಿ (AstScope* ಧ್ರುವ scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
ಶೂನ್ಯ process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* ಧ್ರುವ cVtxp = vtx.cast<InlineModCellVertex>();
        ಯದಿ (!cVtxp) ಅನುವರ್ತ;
        ಯದಿ (!cVtxp->doInline()) ಅನುವರ್ತ;
        cVtxps.ತಳ್ಳು_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    ಪ್ರತಿ (InlineModCellVertex* ಧ್ರುವ cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* ಧ್ರುವ iVtxp = &cVtxp->instanceOf();
        AstCell* ಧ್ರುವ cellp = cVtxp->cellp();
        ಧ್ರುವ ತಾರ್ಕಿಕ last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        ಯದಿ (last) {
            ಯಾವತ್ (!iVtxp->outEmpty()) {
                InlineModCellVertex* ಧ್ರುವ tVtxp
                    = iVtxp->outEdges().frontp()->ತುದಿ()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        ಯದಿ (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ಗಂಟು_Var_Ref* refp) {
        AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
        ಯದಿ (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

ಶೂನ್ಯ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ಧ್ರುವ size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        ಯದಿ (InlineModModuleVertex* ಧ್ರುವ mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            ಯದಿ (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            ಯದಿ (mVtxp->noInlineHard()) ಅನುವರ್ತ;
            // Don't inline if soft off
            ಯದಿ (mVtxp->noInlineSoft()) ಅನುವರ್ತ;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_ಎಣಿಕೆ();
            ಯದಿ (totalSize * 5 < designSize) {
                ಪ್ರತಿ (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* ಧ್ರುವ cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            ಅನುವರ್ತ;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        ಯದಿ (mVtx.noInlineHard()) ಅನುವರ್ತ;

        // If it should be inlined, inlined it
        ಯದಿ (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        ಯದಿ (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        ಯದಿ (mVtx.noInlineSoft()) ಅನುವರ್ತ;

        // If instatiated in exactly one static site, inline it
        ಯದಿ (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        ಯದಿ (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        ಯದಿ (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ಧ್ರುವ size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        ಧ್ರುವ size_t limit = v3Global.opt.inlineMult();
        ಯದಿ (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_ವೃಕ್ಷ("inline", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
