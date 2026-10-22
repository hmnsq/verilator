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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Inline.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"

#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
अटल स्थिर पूर्णांक INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

श्रेणी InlineModModuleVertex;
श्रेणी InlineModCellVertex;

श्रेणी InlineModGraph final : खुला V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

खुला:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = शेष;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_गाँठ_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    व्योम addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    व्योम addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() स्थिर override { वापस "LR"; }
};

श्रेणी InlineModEitherVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
रक्षित:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

श्रेणी InlineModModuleVertex final : खुला InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_गाँठ_Module* स्थिर m_modp;  // The module
    स्थिर अक्षर* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    स्थिर अक्षर* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    स्थिर अक्षर* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    माप_प्रकार m_आकार = 0;  // The size (statement count) of the module
    माप_प्रकार mutable m_flattened_आकार = 0;  // The size of the module if flattened
    bool mutable m_flattened_आकार_Valid = false;  // Whether the flattened size is valid
    माप_प्रकार mutable m_instance_गिनती = 0;  // The number of total instances of this module
    bool mutable m_instance_गिनती_Valid = false;  // Whether the instance count is valid

खुला:
    InlineModModuleVertex(InlineModGraph& graph, Ast_गाँठ_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = शेष;

    // ACCESSORS
    Ast_गाँठ_Module* modp() स्थिर { वापस m_modp; }
    माप_प्रकार आकार() स्थिर { वापस m_आकार; }
    व्योम आकार(माप_प्रकार मूल्य) { m_आकार = मूल्य; }
    व्योम आकार_Inc(माप_प्रकार मूल्य = 1) { m_आकार += मूल्य; }
    bool noInlineHard() स्थिर { वापस m_noInlineHardWyp; }
    व्योम setNoInlineHard(स्थिर अक्षर* whyp) {
        अगर (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    bool noInlineSoft() स्थिर { वापस m_noInlineSoftWyp; }
    व्योम setNoInlineSoft(स्थिर अक्षर* whyp) {
        अगर (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    bool shouldInline() स्थिर { वापस m_shouldInlineWhyp; }
    व्योम setShouldInline(स्थिर अक्षर* whyp) {
        अगर (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    व्योम setFlatten();

    // Total size of module, with all hierarchy below flattened
    माप_प्रकार flattened_आकार() स्थिर {
        अगर (!m_flattened_आकार_Valid) {
            m_flattened_आकार_Valid = true;
            m_flattened_आकार = m_आकार;
            क्रम (स्थिर V3GraphEdge& e1 : outEdges()) {
                क्रम (स्थिर V3GraphEdge& e2 : e1.शीर्ष()->outEdges()) {
                    InlineModModuleVertex* स्थिर mVtxp = e2.शीर्ष()->as<InlineModModuleVertex>();
                    m_flattened_आकार += mVtxp->flattened_आकार();
                }
            }
        }
        वापस m_flattened_आकार;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    माप_प्रकार instance_गिनती() स्थिर {
        अगर (!m_instance_गिनती_Valid) {
            m_instance_गिनती_Valid = true;
            m_instance_गिनती = 0;
            क्रम (स्थिर V3GraphEdge& e1 : inEdges()) {
                क्रम (स्थिर V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* स्थिर mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_गिनती += mVtxp->instance_गिनती();
                }
            }
            अगर (!m_instance_गिनती) {
                UASSERT_OBJ(m_modp->is_शीर्ष(), m_modp, "non-top level module should have instances");
                m_instance_गिनती = 1;
            }
        }
        वापस m_instance_गिनती;
    }

    // debug
    File_पंक्ति* fileline() स्थिर override { वापस m_modp->fileline(); }
    std::string dotShape() स्थिर override { वापस "box"; }
    std::string dotColor() स्थिर override {
        वापस m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() स्थिर override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_गिनती());
        str += "\nsize: " + std::to_string(m_आकार);
        str += "\nflattenedSize: " + std::to_string(flattened_आकार());
        अगर (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        अगर (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        अगर (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        वापस str;
    }
};

श्रेणी InlineModCellVertex final : खुला InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* स्थिर m_cellp;  // The cell (instance)
    स्थिर अक्षर* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    bool m_flatten = false;  // Whether this cell and below already flattened (avoid O(n^2))

खुला:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = शेष;

    // ACCESSORS
    AstCell* cellp() स्थिर { वापस m_cellp; }
    bool doInline() स्थिर { वापस m_doInlineWyp; }
    व्योम setDoInline(स्थिर अक्षर* whyp) {
        अगर (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    bool flatten() स्थिर { वापस m_flatten; }
    व्योम setFlatten() { m_flatten = true; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() स्थिर {
        UASSERT_OBJ(outSize1(), यह, "Cell should have exactly one outgoing edge");
        वापस *outEdges().frontp()->शीर्ष()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() स्थिर {
        UASSERT_OBJ(inSize1(), यह, "Cell should have exactly one incoming edge");
        वापस *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_पंक्ति* fileline() स्थिर override { वापस m_cellp->fileline(); }
    std::string dotColor() स्थिर override { वापस m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() स्थिर override { वापस "ellipse"; }
    std::string name() स्थिर override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        अगर (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        वापस str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_गाँठ_Module* modp) {
    अगर (!modp->user4p()) modp->user4p(नया InlineModModuleVertex{*यह, modp});
    वापस modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    अगर (!cellp->user4p()) cellp->user4p(नया InlineModCellVertex{*यह, cellp});
    वापस cellp->user4u().to<InlineModCellVertex*>();
}

व्योम InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    नया V3GraphEdge{यह, &parent, &cell, 1, /* cutable: */ false};
}
व्योम InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    नया V3GraphEdge{यह, &cell, &submodule, 1, /* cutable: */ false};
}

व्योम InlineModModuleVertex::setFlatten() {
    क्रम (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.शीर्ष()->as<InlineModCellVertex>();
        अगर (cVtx.flatten()) जारी;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        अगर (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

श्रेणी InlineModGraphBuilder final : खुला VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{नया InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    स्थिर AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* स्थिर vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        अगर (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        अगर (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        अगर (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        अगर (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        अगर (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        अगर (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    व्योम visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        अगर (!m_sizedScopep) m_sizedScopep = nodep;
        अगर (m_sizedScopep != nodep) वापस;
        अगर (m_modVtxp) m_modVtxp->आकार_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    व्योम visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* स्थिर vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstPragma* nodep) override {
        अगर (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            अगर (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }

        अगर (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            अगर (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (m_modVtxp) m_modVtxp->आकार_Inc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_शीर्ष(),
                    nodep, "First vertex should be top level module");
#अगर_घोषित VL_DEBUG
        क्रम (स्थिर V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            अगर (&vtx == m_graphp->vertices().frontp()) जारी;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#पूर्ण_अगर
    }
    ~InlineModGraphBuilder() override = शेष;

खुला:
    अटल std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        वापस std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

namespace ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<स्थिर AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
व्योम gatherScopes(स्थिर Ast_गाँठ_Module* modp, ScopeChildren& children) {
    क्रम (Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        अगर (AstScope* स्थिर scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].धकेलो_back(scopep);
        } अन्यथा अगर (स्थिर Ast_गाँठ_Module* स्थिर subModp = VN_CAST(nodep, गाँठ_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
व्योम renameScopes(AstScope* scopep, स्थिर std::string& oldPrefix, स्थिर std::string& newPrefix,
                  स्थिर ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.आकार()));
    स्थिर स्वतः it = children.find(scopep);
    अगर (it == children.end()) वापस;
    क्रम (AstScope* स्थिर childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
व्योम inlineScope(AstScope* scopep, AstCell* cellp, स्थिर std::string& prefix,
                 ScopeChildren& children) {
    AstScope* स्थिर parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    क्रम (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        अगर (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        अगर (AstVar* स्थिर newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        अगर (!cellp->isTrace()) vscp->trace(false);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    क्रम (Ast_गाँठ *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        अगर (Ast_गाँठ_FTask* स्थिर ftaskp = VN_CAST(nodep, गाँठ_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        अगर (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_गाँठ_Cover_Decl* स्थिर declp = incp->declp();
                अगर (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    अगर (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    वापस;
                }
                अगर (Ast_गाँठ_Cover_Decl* स्थिर newDeclp = VN_CAST(declp->user3p(), गाँठ_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    क्रम (Ast_गाँठ *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* स्थिर cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    अगर (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            नया AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    स्थिर std::string oldPrefix = scopep->name() + ".";
    स्थिर std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        स्थिर स्वतः it = children.find(scopep);
        अगर (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    क्रम (AstScope* स्थिर childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        अगर (AstCell* स्थिर newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            childScopep->aboveCellp(newCellp);
        }
        childScopep->aboveScopep(parentScopep);
        renameScopes(childScopep, oldPrefix, newPrefix, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::vector<AstScope*>& parentChildps = children[parentScopep];
    parentChildps.erase(std::हटाओ(parentChildps.begin(), parentChildps.end(), scopep),
                        parentChildps.end());
    parentChildps.जोड़ो(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
व्योम inlineCell(Ast_गाँठ_Module* modp, AstCell* cellp, bool last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    स्थिर VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.धकेलो_Deletep(cellp->unlinkFrBack());

    Ast_गाँठ_Module* स्थिर subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    क्रम (Ast_गाँठ *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* स्थिर scopep = VN_CAST(nodep, Scope);
        अगर (!scopep) जारी;
        scopep->unlinkFrBack();
        अगर (scopep->aboveCellp() == cellp) {
            inlineScopeps.धकेलो_back(scopep);
            deleter.धकेलो_Deletep(scopep);
        } अन्यथा {
            otherScopeps.धकेलो_back(scopep);
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
    Ast_गाँठ_Module* inlinedp;
    अगर (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } अन्यथा {
        inlinedp = subModp->clone_वृक्ष(false);
        // Compute map from the original module items to their clones
        क्रम (Ast_गाँठ *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        अगर (v3Global.opt.coverageFsm()) {
            क्रम (Ast_गाँठ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                स्थिर Ast_गाँठ_Cover_Decl* स्थिर declp = VN_CAST(nodep, गाँठ_Cover_Decl);
                अगर (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        क्रम (AstScope* स्थिर scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.धकेलो_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    स्थिर std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    अगर (AstSystemCSection* स्थिर sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlink_Fr_Back_With_अग्र());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* स्थिर mVtxp = graph.getInlineModModuleVertexp(modp);
    क्रम (Ast_गाँठ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        अगर (AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            अगर (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            अगर (!cellp->isTrace()) varp->trace(false);
        } अन्यथा अगर (AstCell* स्थिर subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* स्थिर vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } अन्यथा अगर (AstTypedef* स्थिर typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } अन्यथा अगर (Ast_गाँठ_Cover_Decl* स्थिर declp = VN_CAST(nodep, गाँठ_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            अगर (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    क्रम (AstScope* स्थिर scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
व्योम process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    क्रम (Ast_गाँठ_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाँठ_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    क्रम (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* स्थिर cVtxp = vtx.cast<InlineModCellVertex>();
        अगर (!cVtxp) जारी;
        अगर (!cVtxp->doInline()) जारी;
        cVtxps.धकेलो_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    क्रम (InlineModCellVertex* स्थिर cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* स्थिर iVtxp = &cVtxp->instanceOf();
        AstCell* स्थिर cellp = cVtxp->cellp();
        स्थिर bool last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.आकार_Inc(iVtxp->आकार());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        अगर (last) {
            जबतक (!iVtxp->outEmpty()) {
                InlineModCellVertex* स्थिर tVtxp
                    = iVtxp->outEdges().frontp()->शीर्ष()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        अगर (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_गाँठ_Var_Ref* refp) {
        AstVarScope* स्थिर vscp = refp->varScopep();
        अगर (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

व्योम V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    स्थिर माप_प्रकार design_आकार
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattened_आकार();
    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        अगर (InlineModModuleVertex* स्थिर mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            अगर (mVtxp->flattened_आकार() * 10 < design_आकार) mVtxp->setFlatten();
            // Don't inline if can't inline
            अगर (mVtxp->noInlineHard()) जारी;
            // Don't inline if soft off
            अगर (mVtxp->noInlineSoft()) जारी;
            // If all instances of this module combined are less than 20% of the design, inline all
            माप_प्रकार योग_आकार = mVtxp->flattened_आकार() * mVtxp->instance_गिनती();
            अगर (योग_आकार * 5 < design_आकार) {
                क्रम (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* स्थिर cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            जारी;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        अगर (mVtx.noInlineHard()) जारी;

        // If it should be inlined, inlined it
        अगर (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        अगर (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        अगर (mVtx.noInlineSoft()) जारी;

        // If instatiated in exactly one static site, inline it
        अगर (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        अगर (mVtx.आकार() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        अगर (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        स्थिर माप_प्रकार inlined_आकार = mVtx.inEdges().आकार() * mVtx.आकार();
        स्थिर माप_प्रकार limit = v3Global.opt.inlineMult();
        अगर (inlined_आकार < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_वृक्ष("inline", 0, dump_वृक्ष_Either_Level() >= 3);
}
