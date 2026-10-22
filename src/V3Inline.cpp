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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Inline.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
স্থির ধ্রুবক পূর্ণসংখ্যা INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

শ্রেণী InlineModModuleVertex;
শ্রেণী InlineModCellVertex;

শ্রেণী InlineModGraph final : সর্বজনীন V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

সর্বজনীন:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = পূর্বনির্ধারিত;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_গ্রন্থি_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    শূন্য addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    শূন্য addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() ধ্রুবক override { ফেরত "LR"; }
};

শ্রেণী InlineModEitherVertex VL_NOT_FINAL : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
সুরক্ষিত:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

শ্রেণী InlineModModuleVertex final : সর্বজনীন InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // The module
    ধ্রুবক অক্ষর* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    ধ্রুবক অক্ষর* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    ধ্রুবক অক্ষর* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    আকার_t m_আকার = 0;  // The size (statement count) of the module
    আকার_t mutable m_flattened_আকার = 0;  // The size of the module if flattened
    যৌক্তিক mutable m_flattened_আকার_Valid = মিথ্যা;  // Whether the flattened size is valid
    আকার_t mutable m_instance_গুনতি = 0;  // The number of total instances of this module
    যৌক্তিক mutable m_instance_গুনতি_Valid = মিথ্যা;  // Whether the instance count is valid

সর্বজনীন:
    InlineModModuleVertex(InlineModGraph& graph, Ast_গ্রন্থি_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = পূর্বনির্ধারিত;

    // ACCESSORS
    Ast_গ্রন্থি_Module* modp() ধ্রুবক { ফেরত m_modp; }
    আকার_t আকার_2() ধ্রুবক { ফেরত m_আকার; }
    শূন্য আকার_2(আকার_t মান) { m_আকার = মান; }
    শূন্য আকার_Inc(আকার_t মান = 1) { m_আকার += মান; }
    যৌক্তিক noInlineHard() ধ্রুবক { ফেরত m_noInlineHardWyp; }
    শূন্য setNoInlineHard(ধ্রুবক অক্ষর* whyp) {
        যদি (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    যৌক্তিক noInlineSoft() ধ্রুবক { ফেরত m_noInlineSoftWyp; }
    শূন্য setNoInlineSoft(ধ্রুবক অক্ষর* whyp) {
        যদি (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    যৌক্তিক shouldInline() ধ্রুবক { ফেরত m_shouldInlineWhyp; }
    শূন্য setShouldInline(ধ্রুবক অক্ষর* whyp) {
        যদি (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    শূন্য setFlatten();

    // Total size of module, with all hierarchy below flattened
    আকার_t flattened_আকার() ধ্রুবক {
        যদি (!m_flattened_আকার_Valid) {
            m_flattened_আকার_Valid = সত্য;
            m_flattened_আকার = m_আকার;
            জন্য (ধ্রুবক V3GraphEdge& e1 : outEdges()) {
                জন্য (ধ্রুবক V3GraphEdge& e2 : e1.শীর্ষ()->outEdges()) {
                    InlineModModuleVertex* ধ্রুবক mVtxp = e2.শীর্ষ()->as<InlineModModuleVertex>();
                    m_flattened_আকার += mVtxp->flattened_আকার();
                }
            }
        }
        ফেরত m_flattened_আকার;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    আকার_t instance_গুনতি() ধ্রুবক {
        যদি (!m_instance_গুনতি_Valid) {
            m_instance_গুনতি_Valid = সত্য;
            m_instance_গুনতি = 0;
            জন্য (ধ্রুবক V3GraphEdge& e1 : inEdges()) {
                জন্য (ধ্রুবক V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* ধ্রুবক mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_গুনতি += mVtxp->instance_গুনতি();
                }
            }
            যদি (!m_instance_গুনতি) {
                UASSERT_OBJ(m_modp->is_শীর্ষ(), m_modp, "non-top level module should have instances");
                m_instance_গুনতি = 1;
            }
        }
        ফেরত m_instance_গুনতি;
    }

    // debug
    File_সারি* fileline() ধ্রুবক override { ফেরত m_modp->fileline(); }
    std::string dotShape() ধ্রুবক override { ফেরত "box"; }
    std::string dotColor() ধ্রুবক override {
        ফেরত m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ধ্রুবক override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_গুনতি());
        str += "\nsize: " + std::to_string(m_আকার);
        str += "\nflattenedSize: " + std::to_string(flattened_আকার());
        যদি (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        যদি (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        যদি (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        ফেরত str;
    }
};

শ্রেণী InlineModCellVertex final : সর্বজনীন InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* ধ্রুবক m_cellp;  // The cell (instance)
    ধ্রুবক অক্ষর* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    যৌক্তিক m_flatten = মিথ্যা;  // Whether this cell and below already flattened (avoid O(n^2))

সর্বজনীন:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = পূর্বনির্ধারিত;

    // ACCESSORS
    AstCell* cellp() ধ্রুবক { ফেরত m_cellp; }
    যৌক্তিক doInline() ধ্রুবক { ফেরত m_doInlineWyp; }
    শূন্য setDoInline(ধ্রুবক অক্ষর* whyp) {
        যদি (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    যৌক্তিক flatten() ধ্রুবক { ফেরত m_flatten; }
    শূন্য setFlatten() { m_flatten = সত্য; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() ধ্রুবক {
        UASSERT_OBJ(outSize1(), এই, "Cell should have exactly one outgoing edge");
        ফেরত *outEdges().frontp()->শীর্ষ()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() ধ্রুবক {
        UASSERT_OBJ(inSize1(), এই, "Cell should have exactly one incoming edge");
        ফেরত *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_সারি* fileline() ধ্রুবক override { ফেরত m_cellp->fileline(); }
    std::string dotColor() ধ্রুবক override { ফেরত m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ধ্রুবক override { ফেরত "ellipse"; }
    std::string name() ধ্রুবক override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        যদি (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        ফেরত str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_গ্রন্থি_Module* modp) {
    যদি (!modp->user4p()) modp->user4p(নতুন InlineModModuleVertex{*এই, modp});
    ফেরত modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    যদি (!cellp->user4p()) cellp->user4p(নতুন InlineModCellVertex{*এই, cellp});
    ফেরত cellp->user4u().to<InlineModCellVertex*>();
}

শূন্য InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    নতুন V3GraphEdge{এই, &parent, &cell, 1, /* cutable: */ মিথ্যা};
}
শূন্য InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    নতুন V3GraphEdge{এই, &cell, &submodule, 1, /* cutable: */ মিথ্যা};
}

শূন্য InlineModModuleVertex::setFlatten() {
    জন্য (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.শীর্ষ()->as<InlineModCellVertex>();
        যদি (cVtx.flatten()) চালু;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        যদি (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

শ্রেণী InlineModGraphBuilder final : সর্বজনীন VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{নতুন InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    ধ্রুবক AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* ধ্রুবক vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        যদি (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        যদি (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        যদি (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        যদি (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        যদি (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        যদি (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    শূন্য visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        যদি (!m_sizedScopep) m_sizedScopep = nodep;
        যদি (m_sizedScopep != nodep) ফেরত;
        যদি (m_modVtxp) m_modVtxp->আকার_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    শূন্য visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* ধ্রুবক vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstPragma* nodep) override {
        যদি (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            যদি (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } অন্যথা {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }

        যদি (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            যদি (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } অন্যথা {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (m_modVtxp) m_modVtxp->আকার_Inc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_শীর্ষ(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        জন্য (ধ্রুবক V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            যদি (&vtx == m_graphp->vertices().frontp()) চালু;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        ফেরত std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

নামাকাশ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<ধ্রুবক AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
শূন্য gatherScopes(ধ্রুবক Ast_গ্রন্থি_Module* modp, ScopeChildren& children) {
    জন্য (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        যদি (AstScope* ধ্রুবক scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ঠেলো_back(scopep);
        } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক subModp = VN_CAST(nodep, গ্রন্থি_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
শূন্য renameScopes(AstScope* scopep, ধ্রুবক std::string& oldPrefix, ধ্রুবক std::string& newPrefix,
                  ধ্রুবক ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.আকার_2()));
    ধ্রুবক auto it = children.find(scopep);
    যদি (it == children.end()) ফেরত;
    জন্য (AstScope* ধ্রুবক childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
শূন্য inlineScope(AstScope* scopep, AstCell* cellp, ধ্রুবক std::string& prefix,
                 ScopeChildren& children) {
    AstScope* ধ্রুবক parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    জন্য (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        যদি (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        যদি (AstVar* ধ্রুবক newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        যদি (!cellp->isTrace()) vscp->trace(মিথ্যা);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    জন্য (Ast_গ্রন্থি *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        যদি (Ast_গ্রন্থি_FTask* ধ্রুবক ftaskp = VN_CAST(nodep, গ্রন্থি_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        যদি (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_গ্রন্থি_Cover_Decl* ধ্রুবক declp = incp->declp();
                যদি (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    যদি (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    ফেরত;
                }
                যদি (Ast_গ্রন্থি_Cover_Decl* ধ্রুবক newDeclp = VN_CAST(declp->user3p(), গ্রন্থি_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    জন্য (Ast_গ্রন্থি *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ধ্রুবক cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    যদি (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            নতুন AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ধ্রুবক std::string oldPrefix = scopep->name() + ".";
    ধ্রুবক std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        ধ্রুবক auto it = children.find(scopep);
        যদি (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    জন্য (AstScope* ধ্রুবক childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        যদি (AstCell* ধ্রুবক newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.ঢোকাও(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
শূন্য inlineCell(Ast_গ্রন্থি_Module* modp, AstCell* cellp, যৌক্তিক last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ধ্রুবক VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.ঠেলো_Deletep(cellp->unlinkFrBack());

    Ast_গ্রন্থি_Module* ধ্রুবক subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    জন্য (Ast_গ্রন্থি *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ধ্রুবক scopep = VN_CAST(nodep, Scope);
        যদি (!scopep) চালু;
        scopep->unlinkFrBack();
        যদি (scopep->aboveCellp() == cellp) {
            inlineScopeps.ঠেলো_back(scopep);
            deleter.ঠেলো_Deletep(scopep);
        } অন্যথা {
            otherScopeps.ঠেলো_back(scopep);
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
    Ast_গ্রন্থি_Module* inlinedp;
    যদি (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } অন্যথা {
        inlinedp = subModp->clone_বৃক্ষ(মিথ্যা);
        // Compute map from the original module items to their clones
        জন্য (Ast_গ্রন্থি *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        যদি (v3Global.opt.coverageFsm()) {
            জন্য (Ast_গ্রন্থি *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ধ্রুবক Ast_গ্রন্থি_Cover_Decl* ধ্রুবক declp = VN_CAST(nodep, গ্রন্থি_Cover_Decl);
                যদি (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        জন্য (AstScope* ধ্রুবক scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.ঠেলো_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ধ্রুবক std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    যদি (AstSystemCSection* ধ্রুবক sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlink_Fr_Back_With_পরবর্তী());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* ধ্রুবক mVtxp = graph.getInlineModModuleVertexp(modp);
    জন্য (Ast_গ্রন্থি *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        যদি (AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            যদি (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            যদি (!cellp->isTrace()) varp->trace(মিথ্যা);
        } অন্যথা যদি (AstCell* ধ্রুবক subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* ধ্রুবক vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } অন্যথা যদি (AstTypedef* ধ্রুবক typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } অন্যথা যদি (Ast_গ্রন্থি_Cover_Decl* ধ্রুবক declp = VN_CAST(nodep, গ্রন্থি_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            যদি (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    জন্য (AstScope* ধ্রুবক scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
শূন্য process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    জন্য (Ast_গ্রন্থি_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    জন্য (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* ধ্রুবক cVtxp = vtx.cast<InlineModCellVertex>();
        যদি (!cVtxp) চালু;
        যদি (!cVtxp->doInline()) চালু;
        cVtxps.ঠেলো_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    জন্য (InlineModCellVertex* ধ্রুবক cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* ধ্রুবক iVtxp = &cVtxp->instanceOf();
        AstCell* ধ্রুবক cellp = cVtxp->cellp();
        ধ্রুবক যৌক্তিক last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.আকার_Inc(iVtxp->আকার_2());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        যদি (last) {
            যখন (!iVtxp->outEmpty()) {
                InlineModCellVertex* ধ্রুবক tVtxp
                    = iVtxp->outEdges().frontp()->শীর্ষ()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        যদি (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_গ্রন্থি_Var_Ref* refp) {
        AstVarScope* ধ্রুবক vscp = refp->varScopep();
        যদি (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

শূন্য V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ধ্রুবক আকার_t design_আকার
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattened_আকার();
    জন্য (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (InlineModModuleVertex* ধ্রুবক mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            যদি (mVtxp->flattened_আকার() * 10 < design_আকার) mVtxp->setFlatten();
            // Don't inline if can't inline
            যদি (mVtxp->noInlineHard()) চালু;
            // Don't inline if soft off
            যদি (mVtxp->noInlineSoft()) চালু;
            // If all instances of this module combined are less than 20% of the design, inline all
            আকার_t যোগ_আকার = mVtxp->flattened_আকার() * mVtxp->instance_গুনতি();
            যদি (যোগ_আকার * 5 < design_আকার) {
                জন্য (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* ধ্রুবক cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            চালু;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        যদি (mVtx.noInlineHard()) চালু;

        // If it should be inlined, inlined it
        যদি (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        যদি (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        যদি (mVtx.noInlineSoft()) চালু;

        // If instatiated in exactly one static site, inline it
        যদি (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        যদি (mVtx.আকার_2() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        যদি (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ধ্রুবক আকার_t inlined_আকার = mVtx.inEdges().আকার_2() * mVtx.আকার_2();
        ধ্রুবক আকার_t limit = v3Global.opt.inlineMult();
        যদি (inlined_আকার < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_বৃক্ষ("inline", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
