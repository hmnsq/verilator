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
स्थिर अचर पूर्णाङ्क INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

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
    ~InlineModGraph() override = स्वभाव;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_गाँठो_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    शून्य addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    शून्य addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() अचर override { फर्काऊ "LR"; }
};

वर्ग InlineModEitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
रक्षित:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

वर्ग InlineModModuleVertex final : सार्वजनिक InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_गाँठो_Module* अचर m_modp;  // The module
    अचर वर्ण* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    अचर वर्ण* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    अचर वर्ण* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    माप_प्रकार m_size = 0;  // The size (statement count) of the module
    माप_प्रकार mutable m_flattenedSize = 0;  // The size of the module if flattened
    तार्किक mutable m_flattenedSizeValid = असत्य;  // Whether the flattened size is valid
    माप_प्रकार mutable m_instance_गन्ती = 0;  // The number of total instances of this module
    तार्किक mutable m_instance_गन्ती_Valid = असत्य;  // Whether the instance count is valid

सार्वजनिक:
    InlineModModuleVertex(InlineModGraph& graph, Ast_गाँठो_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = स्वभाव;

    // ACCESSORS
    Ast_गाँठो_Module* modp() अचर { फर्काऊ m_modp; }
    माप_प्रकार size() अचर { फर्काऊ m_size; }
    शून्य size(माप_प्रकार मान) { m_size = मान; }
    शून्य sizeInc(माप_प्रकार मान = 1) { m_size += मान; }
    तार्किक noInlineHard() अचर { फर्काऊ m_noInlineHardWyp; }
    शून्य setNoInlineHard(अचर वर्ण* whyp) {
        यदि (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    तार्किक noInlineSoft() अचर { फर्काऊ m_noInlineSoftWyp; }
    शून्य setNoInlineSoft(अचर वर्ण* whyp) {
        यदि (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    तार्किक shouldInline() अचर { फर्काऊ m_shouldInlineWhyp; }
    शून्य setShouldInline(अचर वर्ण* whyp) {
        यदि (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    शून्य setFlatten();

    // Total size of module, with all hierarchy below flattened
    माप_प्रकार flattenedSize() अचर {
        यदि (!m_flattenedSizeValid) {
            m_flattenedSizeValid = सत्य;
            m_flattenedSize = m_size;
            लागि (अचर V3GraphEdge& e1 : outEdges()) {
                लागि (अचर V3GraphEdge& e2 : e1.शीर्ष()->outEdges()) {
                    InlineModModuleVertex* अचर mVtxp = e2.शीर्ष()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        फर्काऊ m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    माप_प्रकार instance_गन्ती() अचर {
        यदि (!m_instance_गन्ती_Valid) {
            m_instance_गन्ती_Valid = सत्य;
            m_instance_गन्ती = 0;
            लागि (अचर V3GraphEdge& e1 : inEdges()) {
                लागि (अचर V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* अचर mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_गन्ती += mVtxp->instance_गन्ती();
                }
            }
            यदि (!m_instance_गन्ती) {
                UASSERT_OBJ(m_modp->is_शीर्ष(), m_modp, "non-top level module should have instances");
                m_instance_गन्ती = 1;
            }
        }
        फर्काऊ m_instance_गन्ती;
    }

    // debug
    File_पङ्क्ति* fileline() अचर override { फर्काऊ m_modp->fileline(); }
    std::string dotShape() अचर override { फर्काऊ "box"; }
    std::string dotColor() अचर override {
        फर्काऊ m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() अचर override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_गन्ती());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        यदि (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        यदि (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        यदि (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        फर्काऊ str;
    }
};

वर्ग InlineModCellVertex final : सार्वजनिक InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* अचर m_cellp;  // The cell (instance)
    अचर वर्ण* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    तार्किक m_flatten = असत्य;  // Whether this cell and below already flattened (avoid O(n^2))

सार्वजनिक:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = स्वभाव;

    // ACCESSORS
    AstCell* cellp() अचर { फर्काऊ m_cellp; }
    तार्किक doInline() अचर { फर्काऊ m_doInlineWyp; }
    शून्य setDoInline(अचर वर्ण* whyp) {
        यदि (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    तार्किक flatten() अचर { फर्काऊ m_flatten; }
    शून्य setFlatten() { m_flatten = सत्य; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() अचर {
        UASSERT_OBJ(outSize1(), एतत्, "Cell should have exactly one outgoing edge");
        फर्काऊ *outEdges().frontp()->शीर्ष()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() अचर {
        UASSERT_OBJ(inSize1(), एतत्, "Cell should have exactly one incoming edge");
        फर्काऊ *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_पङ्क्ति* fileline() अचर override { फर्काऊ m_cellp->fileline(); }
    std::string dotColor() अचर override { फर्काऊ m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() अचर override { फर्काऊ "ellipse"; }
    std::string name() अचर override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        यदि (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        फर्काऊ str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_गाँठो_Module* modp) {
    यदि (!modp->user4p()) modp->user4p(नूतन InlineModModuleVertex{*एतत्, modp});
    फर्काऊ modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    यदि (!cellp->user4p()) cellp->user4p(नूतन InlineModCellVertex{*एतत्, cellp});
    फर्काऊ cellp->user4u().to<InlineModCellVertex*>();
}

शून्य InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    नूतन V3GraphEdge{एतत्, &parent, &cell, 1, /* cutable: */ असत्य};
}
शून्य InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    नूतन V3GraphEdge{एतत्, &cell, &submodule, 1, /* cutable: */ असत्य};
}

शून्य InlineModModuleVertex::setFlatten() {
    लागि (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.शीर्ष()->as<InlineModCellVertex>();
        यदि (cVtx.flatten()) जारी;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        यदि (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

वर्ग InlineModGraphBuilder final : सार्वजनिक VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{नूतन InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    अचर AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* अचर vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        यदि (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        यदि (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        यदि (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        यदि (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        यदि (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        यदि (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    शून्य visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        यदि (!m_sizedScopep) m_sizedScopep = nodep;
        यदि (m_sizedScopep != nodep) फर्काऊ;
        यदि (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    शून्य visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* अचर vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstPragma* nodep) override {
        यदि (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            यदि (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
            फर्काऊ;
        }

        यदि (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            यदि (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
            फर्काऊ;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (m_modVtxp) m_modVtxp->sizeInc();
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
        लागि (अचर V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            यदि (&vtx == m_graphp->vertices().frontp()) जारी;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = स्वभाव;

सार्वजनिक:
    स्थिर std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        फर्काऊ std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

नामाकाश ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<अचर AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
शून्य gatherScopes(अचर Ast_गाँठो_Module* modp, ScopeChildren& children) {
    लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (AstScope* अचर scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].थप_back(scopep);
        } अन्यथा यदि (अचर Ast_गाँठो_Module* अचर subModp = VN_CAST(nodep, गाँठो_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
शून्य renameScopes(AstScope* scopep, अचर std::string& oldPrefix, अचर std::string& newPrefix,
                  अचर ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    अचर स्वतः it = children.find(scopep);
    यदि (it == children.end()) फर्काऊ;
    लागि (AstScope* अचर childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
शून्य inlineScope(AstScope* scopep, AstCell* cellp, अचर std::string& prefix,
                 ScopeChildren& children) {
    AstScope* अचर parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    लागि (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        यदि (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        यदि (AstVar* अचर newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        यदि (!cellp->isTrace()) vscp->trace(असत्य);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    लागि (Ast_गाँठो *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        यदि (Ast_गाँठो_FTask* अचर ftaskp = VN_CAST(nodep, गाँठो_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        यदि (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_गाँठो_Cover_Decl* अचर declp = incp->declp();
                यदि (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    यदि (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    फर्काऊ;
                }
                यदि (Ast_गाँठो_Cover_Decl* अचर newDeclp = VN_CAST(declp->user3p(), गाँठो_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    लागि (Ast_गाँठो *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* अचर cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    यदि (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            नूतन AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    अचर std::string oldPrefix = scopep->name() + ".";
    अचर std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        अचर स्वतः it = children.find(scopep);
        यदि (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    लागि (AstScope* अचर childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        यदि (AstCell* अचर newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.घुसाउ(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
शून्य inlineCell(Ast_गाँठो_Module* modp, AstCell* cellp, तार्किक last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    अचर VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.थप_Deletep(cellp->unlinkFrBack());

    Ast_गाँठो_Module* अचर subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    लागि (Ast_गाँठो *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* अचर scopep = VN_CAST(nodep, Scope);
        यदि (!scopep) जारी;
        scopep->unlinkFrBack();
        यदि (scopep->aboveCellp() == cellp) {
            inlineScopeps.थप_back(scopep);
            deleter.थप_Deletep(scopep);
        } अन्यथा {
            otherScopeps.थप_back(scopep);
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
    Ast_गाँठो_Module* inlinedp;
    यदि (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } अन्यथा {
        inlinedp = subModp->clone_रूख(असत्य);
        // Compute map from the original module items to their clones
        लागि (Ast_गाँठो *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        यदि (v3Global.opt.coverageFsm()) {
            लागि (Ast_गाँठो *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                अचर Ast_गाँठो_Cover_Decl* अचर declp = VN_CAST(nodep, गाँठो_Cover_Decl);
                यदि (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.थप_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        लागि (AstScope* अचर scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.थप_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    अचर std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    यदि (AstSystemCSection* अचर sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* अचर mVtxp = graph.getInlineModModuleVertexp(modp);
    लागि (Ast_गाँठो *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        यदि (AstVar* अचर varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            यदि (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            यदि (!cellp->isTrace()) varp->trace(असत्य);
        } अन्यथा यदि (AstCell* अचर subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* अचर vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } अन्यथा यदि (AstTypedef* अचर typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } अन्यथा यदि (Ast_गाँठो_Cover_Decl* अचर declp = VN_CAST(nodep, गाँठो_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            यदि (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    लागि (AstScope* अचर scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
शून्य process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    लागि (Ast_गाँठो_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाँठो_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    लागि (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* अचर cVtxp = vtx.cast<InlineModCellVertex>();
        यदि (!cVtxp) जारी;
        यदि (!cVtxp->doInline()) जारी;
        cVtxps.थप_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    लागि (InlineModCellVertex* अचर cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* अचर iVtxp = &cVtxp->instanceOf();
        AstCell* अचर cellp = cVtxp->cellp();
        अचर तार्किक last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        यदि (last) {
            जबसम्म (!iVtxp->outEmpty()) {
                InlineModCellVertex* अचर tVtxp
                    = iVtxp->outEdges().frontp()->शीर्ष()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        यदि (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_गाँठो_Var_Ref* refp) {
        AstVarScope* अचर vscp = refp->varScopep();
        यदि (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
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
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    अचर माप_प्रकार designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    लागि (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (InlineModModuleVertex* अचर mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            यदि (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            यदि (mVtxp->noInlineHard()) जारी;
            // Don't inline if soft off
            यदि (mVtxp->noInlineSoft()) जारी;
            // If all instances of this module combined are less than 20% of the design, inline all
            माप_प्रकार totalSize = mVtxp->flattenedSize() * mVtxp->instance_गन्ती();
            यदि (totalSize * 5 < designSize) {
                लागि (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* अचर cVtxp = edge.fromp()->as<InlineModCellVertex>();
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
        यदि (mVtx.noInlineHard()) जारी;

        // If it should be inlined, inlined it
        यदि (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        यदि (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        यदि (mVtx.noInlineSoft()) जारी;

        // If instatiated in exactly one static site, inline it
        यदि (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        यदि (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        यदि (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        अचर माप_प्रकार inlinedSize = mVtx.inEdges().size() * mVtx.size();
        अचर माप_प्रकार limit = v3Global.opt.inlineMult();
        यदि (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_रूख("inline", 0, dump_रूख_Either_Level() >= 3);
}
