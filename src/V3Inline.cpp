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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Inline.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
സ്ഥിതം സ്ഥിരം പൂർണ്ണസംഖ്യ INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

വർഗ്ഗം InlineModModuleVertex;
വർഗ്ഗം InlineModCellVertex;

വർഗ്ഗം InlineModGraph final : പൊതു V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

പൊതു:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = സ്ഥിരസ്ഥിതി;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_കണ്ണി_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    ശൂന്യം addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    ശൂന്യം addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() സ്ഥിരം override { തിരികെ "LR"; }
};

വർഗ്ഗം InlineModEitherVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
സംരക്ഷിത:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

വർഗ്ഗം InlineModModuleVertex final : പൊതു InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // The module
    സ്ഥിരം അക്ഷരം* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    സ്ഥിരം അക്ഷരം* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    സ്ഥിരം അക്ഷരം* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    തർക്കം mutable m_flattenedSizeValid = അസത്യം;  // Whether the flattened size is valid
    size_t mutable m_instance_എണ്ണം = 0;  // The number of total instances of this module
    തർക്കം mutable m_instance_എണ്ണം_Valid = അസത്യം;  // Whether the instance count is valid

പൊതു:
    InlineModModuleVertex(InlineModGraph& graph, Ast_കണ്ണി_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    Ast_കണ്ണി_Module* modp() സ്ഥിരം { തിരികെ m_modp; }
    size_t size() സ്ഥിരം { തിരികെ m_size; }
    ശൂന്യം size(size_t മൂല്യം) { m_size = മൂല്യം; }
    ശൂന്യം sizeInc(size_t മൂല്യം = 1) { m_size += മൂല്യം; }
    തർക്കം noInlineHard() സ്ഥിരം { തിരികെ m_noInlineHardWyp; }
    ശൂന്യം setNoInlineHard(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    തർക്കം noInlineSoft() സ്ഥിരം { തിരികെ m_noInlineSoftWyp; }
    ശൂന്യം setNoInlineSoft(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    തർക്കം shouldInline() സ്ഥിരം { തിരികെ m_shouldInlineWhyp; }
    ശൂന്യം setShouldInline(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    ശൂന്യം setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() സ്ഥിരം {
        എങ്കിൽ (!m_flattenedSizeValid) {
            m_flattenedSizeValid = സത്യം;
            m_flattenedSize = m_size;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& e1 : outEdges()) {
                വേണ്ടി (സ്ഥിരം V3GraphEdge& e2 : e1.മുകൾ()->outEdges()) {
                    InlineModModuleVertex* സ്ഥിരം mVtxp = e2.മുകൾ()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        തിരികെ m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_എണ്ണം() സ്ഥിരം {
        എങ്കിൽ (!m_instance_എണ്ണം_Valid) {
            m_instance_എണ്ണം_Valid = സത്യം;
            m_instance_എണ്ണം = 0;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& e1 : inEdges()) {
                വേണ്ടി (സ്ഥിരം V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* സ്ഥിരം mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_എണ്ണം += mVtxp->instance_എണ്ണം();
                }
            }
            എങ്കിൽ (!m_instance_എണ്ണം) {
                UASSERT_OBJ(m_modp->is_മുകൾ(), m_modp, "non-top level module should have instances");
                m_instance_എണ്ണം = 1;
            }
        }
        തിരികെ m_instance_എണ്ണം;
    }

    // debug
    File_വരി* fileline() സ്ഥിരം override { തിരികെ m_modp->fileline(); }
    std::string dotShape() സ്ഥിരം override { തിരികെ "box"; }
    std::string dotColor() സ്ഥിരം override {
        തിരികെ m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() സ്ഥിരം override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_എണ്ണം());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        എങ്കിൽ (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        എങ്കിൽ (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        എങ്കിൽ (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        തിരികെ str;
    }
};

വർഗ്ഗം InlineModCellVertex final : പൊതു InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* സ്ഥിരം m_cellp;  // The cell (instance)
    സ്ഥിരം അക്ഷരം* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    തർക്കം m_flatten = അസത്യം;  // Whether this cell and below already flattened (avoid O(n^2))

പൊതു:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    AstCell* cellp() സ്ഥിരം { തിരികെ m_cellp; }
    തർക്കം doInline() സ്ഥിരം { തിരികെ m_doInlineWyp; }
    ശൂന്യം setDoInline(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    തർക്കം flatten() സ്ഥിരം { തിരികെ m_flatten; }
    ശൂന്യം setFlatten() { m_flatten = സത്യം; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() സ്ഥിരം {
        UASSERT_OBJ(outSize1(), ഇത്, "Cell should have exactly one outgoing edge");
        തിരികെ *outEdges().frontp()->മുകൾ()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() സ്ഥിരം {
        UASSERT_OBJ(inSize1(), ഇത്, "Cell should have exactly one incoming edge");
        തിരികെ *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_വരി* fileline() സ്ഥിരം override { തിരികെ m_cellp->fileline(); }
    std::string dotColor() സ്ഥിരം override { തിരികെ m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() സ്ഥിരം override { തിരികെ "ellipse"; }
    std::string name() സ്ഥിരം override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        എങ്കിൽ (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        തിരികെ str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_കണ്ണി_Module* modp) {
    എങ്കിൽ (!modp->user4p()) modp->user4p(പുതിയ InlineModModuleVertex{*ഇത്, modp});
    തിരികെ modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    എങ്കിൽ (!cellp->user4p()) cellp->user4p(പുതിയ InlineModCellVertex{*ഇത്, cellp});
    തിരികെ cellp->user4u().to<InlineModCellVertex*>();
}

ശൂന്യം InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    പുതിയ V3GraphEdge{ഇത്, &parent, &cell, 1, /* cutable: */ അസത്യം};
}
ശൂന്യം InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    പുതിയ V3GraphEdge{ഇത്, &cell, &submodule, 1, /* cutable: */ അസത്യം};
}

ശൂന്യം InlineModModuleVertex::setFlatten() {
    വേണ്ടി (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.മുകൾ()->as<InlineModCellVertex>();
        എങ്കിൽ (cVtx.flatten()) തുടരുക;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        എങ്കിൽ (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

വർഗ്ഗം InlineModGraphBuilder final : പൊതു VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{പുതിയ InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    സ്ഥിരം AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* സ്ഥിരം vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        എങ്കിൽ (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        എങ്കിൽ (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        എങ്കിൽ (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        എങ്കിൽ (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        എങ്കിൽ (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        എങ്കിൽ (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    ശൂന്യം visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        എങ്കിൽ (!m_sizedScopep) m_sizedScopep = nodep;
        എങ്കിൽ (m_sizedScopep != nodep) തിരികെ;
        എങ്കിൽ (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    ശൂന്യം visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* സ്ഥിരം vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstPragma* nodep) override {
        എങ്കിൽ (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            എങ്കിൽ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } അല്ലെങ്കിൽ {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        എങ്കിൽ (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            എങ്കിൽ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } അല്ലെങ്കിൽ {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (m_modVtxp) m_modVtxp->sizeInc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_മുകൾ(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            എങ്കിൽ (&vtx == m_graphp->vertices().frontp()) തുടരുക;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        തിരികെ std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

നാമാകാശ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<സ്ഥിരം AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
ശൂന്യം gatherScopes(സ്ഥിരം Ast_കണ്ണി_Module* modp, ScopeChildren& children) {
    വേണ്ടി (Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        എങ്കിൽ (AstScope* സ്ഥിരം scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].തള്ളുക_back(scopep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം subModp = VN_CAST(nodep, കണ്ണി_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
ശൂന്യം renameScopes(AstScope* scopep, സ്ഥിരം std::string& oldPrefix, സ്ഥിരം std::string& newPrefix,
                  സ്ഥിരം ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    സ്ഥിരം auto it = children.find(scopep);
    എങ്കിൽ (it == children.end()) തിരികെ;
    വേണ്ടി (AstScope* സ്ഥിരം childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
ശൂന്യം inlineScope(AstScope* scopep, AstCell* cellp, സ്ഥിരം std::string& prefix,
                 ScopeChildren& children) {
    AstScope* സ്ഥിരം parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    വേണ്ടി (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        എങ്കിൽ (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        എങ്കിൽ (AstVar* സ്ഥിരം newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        എങ്കിൽ (!cellp->isTrace()) vscp->trace(അസത്യം);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    വേണ്ടി (Ast_കണ്ണി *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = VN_CAST(nodep, കണ്ണി_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        എങ്കിൽ (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp = incp->declp();
                എങ്കിൽ (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    എങ്കിൽ (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    തിരികെ;
                }
                എങ്കിൽ (Ast_കണ്ണി_Cover_Decl* സ്ഥിരം newDeclp = VN_CAST(declp->user3p(), കണ്ണി_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    വേണ്ടി (Ast_കണ്ണി *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* സ്ഥിരം cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    എങ്കിൽ (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            പുതിയ AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    സ്ഥിരം std::string oldPrefix = scopep->name() + ".";
    സ്ഥിരം std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        സ്ഥിരം auto it = children.find(scopep);
        എങ്കിൽ (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    വേണ്ടി (AstScope* സ്ഥിരം childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        എങ്കിൽ (AstCell* സ്ഥിരം newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.തിരുകുക(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
ശൂന്യം inlineCell(Ast_കണ്ണി_Module* modp, AstCell* cellp, തർക്കം last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    സ്ഥിരം VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.തള്ളുക_Deletep(cellp->unlinkFrBack());

    Ast_കണ്ണി_Module* സ്ഥിരം subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    വേണ്ടി (Ast_കണ്ണി *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* സ്ഥിരം scopep = VN_CAST(nodep, Scope);
        എങ്കിൽ (!scopep) തുടരുക;
        scopep->unlinkFrBack();
        എങ്കിൽ (scopep->aboveCellp() == cellp) {
            inlineScopeps.തള്ളുക_back(scopep);
            deleter.തള്ളുക_Deletep(scopep);
        } അല്ലെങ്കിൽ {
            otherScopeps.തള്ളുക_back(scopep);
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
    Ast_കണ്ണി_Module* inlinedp;
    എങ്കിൽ (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } അല്ലെങ്കിൽ {
        inlinedp = subModp->clone_വൃക്ഷം(അസത്യം);
        // Compute map from the original module items to their clones
        വേണ്ടി (Ast_കണ്ണി *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        എങ്കിൽ (v3Global.opt.coverageFsm()) {
            വേണ്ടി (Ast_കണ്ണി *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                സ്ഥിരം Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp = VN_CAST(nodep, കണ്ണി_Cover_Decl);
                എങ്കിൽ (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        വേണ്ടി (AstScope* സ്ഥിരം scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.തള്ളുക_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    സ്ഥിരം std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    എങ്കിൽ (AstSystemCSection* സ്ഥിരം sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* സ്ഥിരം mVtxp = graph.getInlineModModuleVertexp(modp);
    വേണ്ടി (Ast_കണ്ണി *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            എങ്കിൽ (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            എങ്കിൽ (!cellp->isTrace()) varp->trace(അസത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCell* സ്ഥിരം subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* സ്ഥിരം vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } അല്ലെങ്കിൽ എങ്കിൽ (AstTypedef* സ്ഥിരം typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp = VN_CAST(nodep, കണ്ണി_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            എങ്കിൽ (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    വേണ്ടി (AstScope* സ്ഥിരം scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
ശൂന്യം process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    വേണ്ടി (Ast_കണ്ണി_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* സ്ഥിരം cVtxp = vtx.cast<InlineModCellVertex>();
        എങ്കിൽ (!cVtxp) തുടരുക;
        എങ്കിൽ (!cVtxp->doInline()) തുടരുക;
        cVtxps.തള്ളുക_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    വേണ്ടി (InlineModCellVertex* സ്ഥിരം cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* സ്ഥിരം iVtxp = &cVtxp->instanceOf();
        AstCell* സ്ഥിരം cellp = cVtxp->cellp();
        സ്ഥിരം തർക്കം last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        എങ്കിൽ (last) {
            ആകുമ്പോൾ (!iVtxp->outEmpty()) {
                InlineModCellVertex* സ്ഥിരം tVtxp
                    = iVtxp->outEdges().frontp()->മുകൾ()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        എങ്കിൽ (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_കണ്ണി_Var_Ref* refp) {
        AstVarScope* സ്ഥിരം vscp = refp->varScopep();
        എങ്കിൽ (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

ശൂന്യം V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    സ്ഥിരം size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    വേണ്ടി (V3GraphVertex& vtx : graphp->vertices()) {
        എങ്കിൽ (InlineModModuleVertex* സ്ഥിരം mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            എങ്കിൽ (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            എങ്കിൽ (mVtxp->noInlineHard()) തുടരുക;
            // Don't inline if soft off
            എങ്കിൽ (mVtxp->noInlineSoft()) തുടരുക;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_എണ്ണം();
            എങ്കിൽ (totalSize * 5 < designSize) {
                വേണ്ടി (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* സ്ഥിരം cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            തുടരുക;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        എങ്കിൽ (mVtx.noInlineHard()) തുടരുക;

        // If it should be inlined, inlined it
        എങ്കിൽ (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        എങ്കിൽ (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        എങ്കിൽ (mVtx.noInlineSoft()) തുടരുക;

        // If instatiated in exactly one static site, inline it
        എങ്കിൽ (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        എങ്കിൽ (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        എങ്കിൽ (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        സ്ഥിരം size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        സ്ഥിരം size_t limit = v3Global.opt.inlineMult();
        എങ്കിൽ (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_വൃക്ഷം("inline", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
