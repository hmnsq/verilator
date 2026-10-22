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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Inline.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
स्थिर स्थिरांक पूर्णांक INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

वर्ग InlineModModuleVertex;
वर्ग InlineModCellVertex;

वर्ग InlineModGraph final : सार्वजनिक V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

सार्वजनिक:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = पूर्वनिर्धारित;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_गाठ_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    शून्य addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    शून्य addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() स्थिरांक override { परत "LR"; }
};

वर्ग InlineModEitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
संरक्षित:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

वर्ग InlineModModuleVertex final : सार्वजनिक InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_गाठ_Module* स्थिरांक m_modp;  // The module
    स्थिरांक अक्षर* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    स्थिरांक अक्षर* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    स्थिरांक अक्षर* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    माप_प्रकार m_size = 0;  // The size (statement count) of the module
    माप_प्रकार mutable m_flattenedSize = 0;  // The size of the module if flattened
    तार्किक mutable m_flattenedSizeValid = असत्य;  // Whether the flattened size is valid
    माप_प्रकार mutable m_instance_मोजणी = 0;  // The number of total instances of this module
    तार्किक mutable m_instance_मोजणी_Valid = असत्य;  // Whether the instance count is valid

सार्वजनिक:
    InlineModModuleVertex(InlineModGraph& graph, Ast_गाठ_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    Ast_गाठ_Module* modp() स्थिरांक { परत m_modp; }
    माप_प्रकार size() स्थिरांक { परत m_size; }
    शून्य size(माप_प्रकार मूल्य) { m_size = मूल्य; }
    शून्य sizeInc(माप_प्रकार मूल्य = 1) { m_size += मूल्य; }
    तार्किक noInlineHard() स्थिरांक { परत m_noInlineHardWyp; }
    शून्य setNoInlineHard(स्थिरांक अक्षर* whyp) {
        जर (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    तार्किक noInlineSoft() स्थिरांक { परत m_noInlineSoftWyp; }
    शून्य setNoInlineSoft(स्थिरांक अक्षर* whyp) {
        जर (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    तार्किक shouldInline() स्थिरांक { परत m_shouldInlineWhyp; }
    शून्य setShouldInline(स्थिरांक अक्षर* whyp) {
        जर (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    शून्य setFlatten();

    // Total size of module, with all hierarchy below flattened
    माप_प्रकार flattenedSize() स्थिरांक {
        जर (!m_flattenedSizeValid) {
            m_flattenedSizeValid = सत्य;
            m_flattenedSize = m_size;
            साठी (स्थिरांक V3GraphEdge& e1 : outEdges()) {
                साठी (स्थिरांक V3GraphEdge& e2 : e1.शीर्ष()->outEdges()) {
                    InlineModModuleVertex* स्थिरांक mVtxp = e2.शीर्ष()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        परत m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    माप_प्रकार instance_मोजणी() स्थिरांक {
        जर (!m_instance_मोजणी_Valid) {
            m_instance_मोजणी_Valid = सत्य;
            m_instance_मोजणी = 0;
            साठी (स्थिरांक V3GraphEdge& e1 : inEdges()) {
                साठी (स्थिरांक V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* स्थिरांक mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_मोजणी += mVtxp->instance_मोजणी();
                }
            }
            जर (!m_instance_मोजणी) {
                UASSERT_OBJ(m_modp->is_शीर्ष(), m_modp, "non-top level module should have instances");
                m_instance_मोजणी = 1;
            }
        }
        परत m_instance_मोजणी;
    }

    // debug
    File_ओळ* fileline() स्थिरांक override { परत m_modp->fileline(); }
    std::string dotShape() स्थिरांक override { परत "box"; }
    std::string dotColor() स्थिरांक override {
        परत m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() स्थिरांक override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_मोजणी());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        जर (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        जर (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        जर (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        परत str;
    }
};

वर्ग InlineModCellVertex final : सार्वजनिक InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* स्थिरांक m_cellp;  // The cell (instance)
    स्थिरांक अक्षर* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    तार्किक m_flatten = असत्य;  // Whether this cell and below already flattened (avoid O(n^2))

सार्वजनिक:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    AstCell* cellp() स्थिरांक { परत m_cellp; }
    तार्किक doInline() स्थिरांक { परत m_doInlineWyp; }
    शून्य setDoInline(स्थिरांक अक्षर* whyp) {
        जर (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    तार्किक flatten() स्थिरांक { परत m_flatten; }
    शून्य setFlatten() { m_flatten = सत्य; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() स्थिरांक {
        UASSERT_OBJ(outSize1(), हे, "Cell should have exactly one outgoing edge");
        परत *outEdges().frontp()->शीर्ष()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() स्थिरांक {
        UASSERT_OBJ(inSize1(), हे, "Cell should have exactly one incoming edge");
        परत *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_ओळ* fileline() स्थिरांक override { परत m_cellp->fileline(); }
    std::string dotColor() स्थिरांक override { परत m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() स्थिरांक override { परत "ellipse"; }
    std::string name() स्थिरांक override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        जर (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        परत str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_गाठ_Module* modp) {
    जर (!modp->user4p()) modp->user4p(नवीन InlineModModuleVertex{*हे, modp});
    परत modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    जर (!cellp->user4p()) cellp->user4p(नवीन InlineModCellVertex{*हे, cellp});
    परत cellp->user4u().to<InlineModCellVertex*>();
}

शून्य InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    नवीन V3GraphEdge{हे, &parent, &cell, 1, /* cutable: */ असत्य};
}
शून्य InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    नवीन V3GraphEdge{हे, &cell, &submodule, 1, /* cutable: */ असत्य};
}

शून्य InlineModModuleVertex::setFlatten() {
    साठी (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.शीर्ष()->as<InlineModCellVertex>();
        जर (cVtx.flatten()) सुरूठेवा;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        जर (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

वर्ग InlineModGraphBuilder final : सार्वजनिक VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{नवीन InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    स्थिरांक AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* स्थिरांक vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        जर (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        जर (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        जर (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        जर (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        जर (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        जर (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    शून्य visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        जर (!m_sizedScopep) m_sizedScopep = nodep;
        जर (m_sizedScopep != nodep) परत;
        जर (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    शून्य visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* स्थिरांक vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstPragma* nodep) override {
        जर (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            जर (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            परत;
        }

        जर (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            जर (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            परत;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    शून्य visit(Ast_गाठ* nodep) override {
        जर (m_modVtxp) m_modVtxp->sizeInc();
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
#ifdef VL_DEBUG
        साठी (स्थिरांक V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            जर (&vtx == m_graphp->vertices().frontp()) सुरूठेवा;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = पूर्वनिर्धारित;

सार्वजनिक:
    स्थिर std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        परत std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

नामाकाश ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<स्थिरांक AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
शून्य gatherScopes(स्थिरांक Ast_गाठ_Module* modp, ScopeChildren& children) {
    साठी (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        जर (AstScope* स्थिरांक scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ढकला_back(scopep);
        } अन्यथा जर (स्थिरांक Ast_गाठ_Module* स्थिरांक subModp = VN_CAST(nodep, गाठ_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
शून्य renameScopes(AstScope* scopep, स्थिरांक std::string& oldPrefix, स्थिरांक std::string& newPrefix,
                  स्थिरांक ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    स्थिरांक स्वतः it = children.find(scopep);
    जर (it == children.end()) परत;
    साठी (AstScope* स्थिरांक childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
शून्य inlineScope(AstScope* scopep, AstCell* cellp, स्थिरांक std::string& prefix,
                 ScopeChildren& children) {
    AstScope* स्थिरांक parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    साठी (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        जर (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        जर (AstVar* स्थिरांक newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        जर (!cellp->isTrace()) vscp->trace(असत्य);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    साठी (Ast_गाठ *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        जर (Ast_गाठ_FTask* स्थिरांक ftaskp = VN_CAST(nodep, गाठ_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        जर (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_गाठ_Cover_Decl* स्थिरांक declp = incp->declp();
                जर (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    जर (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    परत;
                }
                जर (Ast_गाठ_Cover_Decl* स्थिरांक newDeclp = VN_CAST(declp->user3p(), गाठ_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    साठी (Ast_गाठ *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* स्थिरांक cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    जर (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            नवीन AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    स्थिरांक std::string oldPrefix = scopep->name() + ".";
    स्थिरांक std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        स्थिरांक स्वतः it = children.find(scopep);
        जर (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    साठी (AstScope* स्थिरांक childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        जर (AstCell* स्थिरांक newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.घाला(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
शून्य inlineCell(Ast_गाठ_Module* modp, AstCell* cellp, तार्किक last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    स्थिरांक VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.ढकला_Deletep(cellp->unlinkFrBack());

    Ast_गाठ_Module* स्थिरांक subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    साठी (Ast_गाठ *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* स्थिरांक scopep = VN_CAST(nodep, Scope);
        जर (!scopep) सुरूठेवा;
        scopep->unlinkFrBack();
        जर (scopep->aboveCellp() == cellp) {
            inlineScopeps.ढकला_back(scopep);
            deleter.ढकला_Deletep(scopep);
        } अन्यथा {
            otherScopeps.ढकला_back(scopep);
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
    Ast_गाठ_Module* inlinedp;
    जर (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } अन्यथा {
        inlinedp = subModp->clone_वृक्ष(असत्य);
        // Compute map from the original module items to their clones
        साठी (Ast_गाठ *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        जर (v3Global.opt.coverageFsm()) {
            साठी (Ast_गाठ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                स्थिरांक Ast_गाठ_Cover_Decl* स्थिरांक declp = VN_CAST(nodep, गाठ_Cover_Decl);
                जर (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        साठी (AstScope* स्थिरांक scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.ढकला_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    स्थिरांक std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    जर (AstSystemCSection* स्थिरांक sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* स्थिरांक mVtxp = graph.getInlineModModuleVertexp(modp);
    साठी (Ast_गाठ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            जर (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            जर (!cellp->isTrace()) varp->trace(असत्य);
        } अन्यथा जर (AstCell* स्थिरांक subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* स्थिरांक vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } अन्यथा जर (AstTypedef* स्थिरांक typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } अन्यथा जर (Ast_गाठ_Cover_Decl* स्थिरांक declp = VN_CAST(nodep, गाठ_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            जर (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    साठी (AstScope* स्थिरांक scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
शून्य process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    साठी (Ast_गाठ_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाठ_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    साठी (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* स्थिरांक cVtxp = vtx.cast<InlineModCellVertex>();
        जर (!cVtxp) सुरूठेवा;
        जर (!cVtxp->doInline()) सुरूठेवा;
        cVtxps.ढकला_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    साठी (InlineModCellVertex* स्थिरांक cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* स्थिरांक iVtxp = &cVtxp->instanceOf();
        AstCell* स्थिरांक cellp = cVtxp->cellp();
        स्थिरांक तार्किक last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        जर (last) {
            पर्यंत (!iVtxp->outEmpty()) {
                InlineModCellVertex* स्थिरांक tVtxp
                    = iVtxp->outEdges().frontp()->शीर्ष()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        जर (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_गाठ_Var_Ref* refp) {
        AstVarScope* स्थिरांक vscp = refp->varScopep();
        जर (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

शून्य V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    स्थिरांक माप_प्रकार designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        जर (InlineModModuleVertex* स्थिरांक mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            जर (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            जर (mVtxp->noInlineHard()) सुरूठेवा;
            // Don't inline if soft off
            जर (mVtxp->noInlineSoft()) सुरूठेवा;
            // If all instances of this module combined are less than 20% of the design, inline all
            माप_प्रकार totalSize = mVtxp->flattenedSize() * mVtxp->instance_मोजणी();
            जर (totalSize * 5 < designSize) {
                साठी (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* स्थिरांक cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            सुरूठेवा;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        जर (mVtx.noInlineHard()) सुरूठेवा;

        // If it should be inlined, inlined it
        जर (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        जर (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        जर (mVtx.noInlineSoft()) सुरूठेवा;

        // If instatiated in exactly one static site, inline it
        जर (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        जर (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        जर (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        स्थिरांक माप_प्रकार inlinedSize = mVtx.inEdges().size() * mVtx.size();
        स्थिरांक माप_प्रकार limit = v3Global.opt.inlineMult();
        जर (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_वृक्ष("inline", 0, dump_वृक्ष_Either_Level() >= 3);
}
