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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Inline.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
ସ୍ଥିର ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

ଶ୍ରେଣୀ InlineModModuleVertex;
ଶ୍ରେଣୀ InlineModCellVertex;

ଶ୍ରେଣୀ InlineModGraph final : ସର୍ବସାଧାରଣ V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

ସର୍ବସାଧାରଣ:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_ଗଣ୍ଠି_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    ଶୂନ୍ୟ addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    ଶୂନ୍ୟ addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() ଅଚଳ override { ଫେରନ୍ତୁ "LR"; }
};

ଶ୍ରେଣୀ InlineModEitherVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
ସୁରକ୍ଷିତ:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

ଶ୍ରେଣୀ InlineModModuleVertex final : ସର୍ବସାଧାରଣ InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // The module
    ଅଚଳ ଅକ୍ଷର* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    ଅଚଳ ଅକ୍ଷର* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    ଅଚଳ ଅକ୍ଷର* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    ତାର୍କିକ mutable m_flattenedSizeValid = ମିଥ୍ୟା;  // Whether the flattened size is valid
    size_t mutable m_instance_ଗଣନା = 0;  // The number of total instances of this module
    ତାର୍କିକ mutable m_instance_ଗଣନା_Valid = ମିଥ୍ୟା;  // Whether the instance count is valid

ସର୍ବସାଧାରଣ:
    InlineModModuleVertex(InlineModGraph& graph, Ast_ଗଣ୍ଠି_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    Ast_ଗଣ୍ଠି_Module* modp() ଅଚଳ { ଫେରନ୍ତୁ m_modp; }
    size_t size() ଅଚଳ { ଫେରନ୍ତୁ m_size; }
    ଶୂନ୍ୟ size(size_t ମୂଲ୍ୟ) { m_size = ମୂଲ୍ୟ; }
    ଶୂନ୍ୟ sizeInc(size_t ମୂଲ୍ୟ = 1) { m_size += ମୂଲ୍ୟ; }
    ତାର୍କିକ noInlineHard() ଅଚଳ { ଫେରନ୍ତୁ m_noInlineHardWyp; }
    ଶୂନ୍ୟ setNoInlineHard(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    ତାର୍କିକ noInlineSoft() ଅଚଳ { ଫେରନ୍ତୁ m_noInlineSoftWyp; }
    ଶୂନ୍ୟ setNoInlineSoft(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    ତାର୍କିକ shouldInline() ଅଚଳ { ଫେରନ୍ତୁ m_shouldInlineWhyp; }
    ଶୂନ୍ୟ setShouldInline(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    ଶୂନ୍ୟ setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() ଅଚଳ {
        ଯଦି (!m_flattenedSizeValid) {
            m_flattenedSizeValid = ସତ୍ୟ;
            m_flattenedSize = m_size;
            ପାଇଁ (ଅଚଳ V3GraphEdge& e1 : outEdges()) {
                ପାଇଁ (ଅଚଳ V3GraphEdge& e2 : e1.ଶୀର୍ଷ()->outEdges()) {
                    InlineModModuleVertex* ଅଚଳ mVtxp = e2.ଶୀର୍ଷ()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        ଫେରନ୍ତୁ m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_ଗଣନା() ଅଚଳ {
        ଯଦି (!m_instance_ଗଣନା_Valid) {
            m_instance_ଗଣନା_Valid = ସତ୍ୟ;
            m_instance_ଗଣନା = 0;
            ପାଇଁ (ଅଚଳ V3GraphEdge& e1 : inEdges()) {
                ପାଇଁ (ଅଚଳ V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* ଅଚଳ mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_ଗଣନା += mVtxp->instance_ଗଣନା();
                }
            }
            ଯଦି (!m_instance_ଗଣନା) {
                UASSERT_OBJ(m_modp->is_ଶୀର୍ଷ(), m_modp, "non-top level module should have instances");
                m_instance_ଗଣନା = 1;
            }
        }
        ଫେରନ୍ତୁ m_instance_ଗଣନା;
    }

    // debug
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ m_modp->fileline(); }
    std::string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "box"; }
    std::string dotColor() ଅଚଳ override {
        ଫେରନ୍ତୁ m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ଅଚଳ override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_ଗଣନା());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        ଯଦି (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        ଯଦି (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        ଯଦି (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        ଫେରନ୍ତୁ str;
    }
};

ଶ୍ରେଣୀ InlineModCellVertex final : ସର୍ବସାଧାରଣ InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* ଅଚଳ m_cellp;  // The cell (instance)
    ଅଚଳ ଅକ୍ଷର* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    ତାର୍କିକ m_flatten = ମିଥ୍ୟା;  // Whether this cell and below already flattened (avoid O(n^2))

ସର୍ବସାଧାରଣ:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    AstCell* cellp() ଅଚଳ { ଫେରନ୍ତୁ m_cellp; }
    ତାର୍କିକ doInline() ଅଚଳ { ଫେରନ୍ତୁ m_doInlineWyp; }
    ଶୂନ୍ୟ setDoInline(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    ତାର୍କିକ flatten() ଅଚଳ { ଫେରନ୍ତୁ m_flatten; }
    ଶୂନ୍ୟ setFlatten() { m_flatten = ସତ୍ୟ; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() ଅଚଳ {
        UASSERT_OBJ(outSize1(), ଏହି, "Cell should have exactly one outgoing edge");
        ଫେରନ୍ତୁ *outEdges().frontp()->ଶୀର୍ଷ()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() ଅଚଳ {
        UASSERT_OBJ(inSize1(), ଏହି, "Cell should have exactly one incoming edge");
        ଫେରନ୍ତୁ *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ m_cellp->fileline(); }
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "ellipse"; }
    std::string name() ଅଚଳ override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        ଯଦି (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        ଫେରନ୍ତୁ str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_ଗଣ୍ଠି_Module* modp) {
    ଯଦି (!modp->user4p()) modp->user4p(ନୂତନ InlineModModuleVertex{*ଏହି, modp});
    ଫେରନ୍ତୁ modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    ଯଦି (!cellp->user4p()) cellp->user4p(ନୂତନ InlineModCellVertex{*ଏହି, cellp});
    ଫେରନ୍ତୁ cellp->user4u().to<InlineModCellVertex*>();
}

ଶୂନ୍ୟ InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    ନୂତନ V3GraphEdge{ଏହି, &parent, &cell, 1, /* cutable: */ ମିଥ୍ୟା};
}
ଶୂନ୍ୟ InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    ନୂତନ V3GraphEdge{ଏହି, &cell, &submodule, 1, /* cutable: */ ମିଥ୍ୟା};
}

ଶୂନ୍ୟ InlineModModuleVertex::setFlatten() {
    ପାଇଁ (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.ଶୀର୍ଷ()->as<InlineModCellVertex>();
        ଯଦି (cVtx.flatten()) ଜାରି;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        ଯଦି (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

ଶ୍ରେଣୀ InlineModGraphBuilder final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{ନୂତନ InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    ଅଚଳ AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* ଅଚଳ vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        ଯଦି (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        ଯଦି (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        ଯଦି (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        ଯଦି (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        ଯଦି (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        ଯଦି (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        ଯଦି (!m_sizedScopep) m_sizedScopep = nodep;
        ଯଦି (m_sizedScopep != nodep) ଫେରନ୍ତୁ;
        ଯଦି (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* ଅଚଳ vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstPragma* nodep) override {
        ଯଦି (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            ଯଦି (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ଅନ୍ୟଥା {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            ଯଦି (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ଅନ୍ୟଥା {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (m_modVtxp) m_modVtxp->sizeInc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_ଶୀର୍ଷ(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            ଯଦି (&vtx == m_graphp->vertices().frontp()) ଜାରି;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        ଫେରନ୍ତୁ std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

ନାମାକାଶ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<ଅଚଳ AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
ଶୂନ୍ୟ gatherScopes(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ScopeChildren& children) {
    ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ଯଦି (AstScope* ଅଚଳ scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ଠେଲ_back(scopep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ subModp = VN_CAST(nodep, ଗଣ୍ଠି_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
ଶୂନ୍ୟ renameScopes(AstScope* scopep, ଅଚଳ std::string& oldPrefix, ଅଚଳ std::string& newPrefix,
                  ଅଚଳ ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    ଅଚଳ auto it = children.find(scopep);
    ଯଦି (it == children.end()) ଫେରନ୍ତୁ;
    ପାଇଁ (AstScope* ଅଚଳ childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
ଶୂନ୍ୟ inlineScope(AstScope* scopep, AstCell* cellp, ଅଚଳ std::string& prefix,
                 ScopeChildren& children) {
    AstScope* ଅଚଳ parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    ପାଇଁ (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        ଯଦି (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        ଯଦି (AstVar* ଅଚଳ newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        ଯଦି (!cellp->isTrace()) vscp->trace(ମିଥ୍ୟା);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = VN_CAST(nodep, ଗଣ୍ଠି_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        ଯଦି (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp = incp->declp();
                ଯଦି (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    ଯଦି (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    ଫେରନ୍ତୁ;
                }
                ଯଦି (Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ newDeclp = VN_CAST(declp->user3p(), ଗଣ୍ଠି_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ଅଚଳ cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    ଯଦି (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            ନୂତନ AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ଅଚଳ std::string oldPrefix = scopep->name() + ".";
    ଅଚଳ std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        ଅଚଳ auto it = children.find(scopep);
        ଯଦି (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    ପାଇଁ (AstScope* ଅଚଳ childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        ଯଦି (AstCell* ଅଚଳ newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.ଭର୍ତ୍ତି(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
ଶୂନ୍ୟ inlineCell(Ast_ଗଣ୍ଠି_Module* modp, AstCell* cellp, ତାର୍କିକ last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ଅଚଳ VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.ଠେଲ_Deletep(cellp->unlinkFrBack());

    Ast_ଗଣ୍ଠି_Module* ଅଚଳ subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ଅଚଳ scopep = VN_CAST(nodep, Scope);
        ଯଦି (!scopep) ଜାରି;
        scopep->unlinkFrBack();
        ଯଦି (scopep->aboveCellp() == cellp) {
            inlineScopeps.ଠେଲ_back(scopep);
            deleter.ଠେଲ_Deletep(scopep);
        } ଅନ୍ୟଥା {
            otherScopeps.ଠେଲ_back(scopep);
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
    Ast_ଗଣ୍ଠି_Module* inlinedp;
    ଯଦି (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } ଅନ୍ୟଥା {
        inlinedp = subModp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        // Compute map from the original module items to their clones
        ପାଇଁ (Ast_ଗଣ୍ଠି *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        ଯଦି (v3Global.opt.coverageFsm()) {
            ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ଅଚଳ Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp = VN_CAST(nodep, ଗଣ୍ଠି_Cover_Decl);
                ଯଦି (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        ପାଇଁ (AstScope* ଅଚଳ scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.ଠେଲ_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ଅଚଳ std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    ଯଦି (AstSystemCSection* ଅଚଳ sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* ଅଚଳ mVtxp = graph.getInlineModModuleVertexp(modp);
    ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            ଯଦି (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            ଯଦି (!cellp->isTrace()) varp->trace(ମିଥ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (AstCell* ଅଚଳ subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* ଅଚଳ vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } ଅନ୍ୟଥା ଯଦି (AstTypedef* ଅଚଳ typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp = VN_CAST(nodep, ଗଣ୍ଠି_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            ଯଦି (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    ପାଇଁ (AstScope* ଅଚଳ scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
ଶୂନ୍ୟ process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* ଅଚଳ cVtxp = vtx.cast<InlineModCellVertex>();
        ଯଦି (!cVtxp) ଜାରି;
        ଯଦି (!cVtxp->doInline()) ଜାରି;
        cVtxps.ଠେଲ_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    ପାଇଁ (InlineModCellVertex* ଅଚଳ cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* ଅଚଳ iVtxp = &cVtxp->instanceOf();
        AstCell* ଅଚଳ cellp = cVtxp->cellp();
        ଅଚଳ ତାର୍କିକ last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        ଯଦି (last) {
            ଯେତେବେଳେ (!iVtxp->outEmpty()) {
                InlineModCellVertex* ଅଚଳ tVtxp
                    = iVtxp->outEdges().frontp()->ଶୀର୍ଷ()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        ଯଦି (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ଗଣ୍ଠି_Var_Ref* refp) {
        AstVarScope* ଅଚଳ vscp = refp->varScopep();
        ଯଦି (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

ଶୂନ୍ୟ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ଅଚଳ size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        ଯଦି (InlineModModuleVertex* ଅଚଳ mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            ଯଦି (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            ଯଦି (mVtxp->noInlineHard()) ଜାରି;
            // Don't inline if soft off
            ଯଦି (mVtxp->noInlineSoft()) ଜାରି;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_ଗଣନା();
            ଯଦି (totalSize * 5 < designSize) {
                ପାଇଁ (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* ଅଚଳ cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            ଜାରି;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        ଯଦି (mVtx.noInlineHard()) ଜାରି;

        // If it should be inlined, inlined it
        ଯଦି (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        ଯଦି (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        ଯଦି (mVtx.noInlineSoft()) ଜାରି;

        // If instatiated in exactly one static site, inline it
        ଯଦି (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        ଯଦି (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        ଯଦି (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ଅଚଳ size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        ଅଚଳ size_t limit = v3Global.opt.inlineMult();
        ଯଦି (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_ବୃକ୍ଷ("inline", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
