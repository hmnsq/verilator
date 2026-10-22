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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Inline.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
நிலையான மாறிலி முழு INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

வகுப்பு InlineModModuleVertex;
வகுப்பு InlineModCellVertex;

வகுப்பு InlineModGraph final : பொது V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

பொது:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = இயல்பு;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_முனை_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    வெறுமை addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    வெறுமை addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() மாறிலி override { திரும்பு "LR"; }
};

வகுப்பு InlineModEitherVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
பாதுகாக்கப்பட்ட:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

வகுப்பு InlineModModuleVertex final : பொது InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_முனை_Module* மாறிலி m_modp;  // The module
    மாறிலி எழுத்து* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    மாறிலி எழுத்து* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    மாறிலி எழுத்து* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    தருக்க mutable m_flattenedSizeValid = பொய்;  // Whether the flattened size is valid
    size_t mutable m_instance_மொத்தம் = 0;  // The number of total instances of this module
    தருக்க mutable m_instance_மொத்தம்_Valid = பொய்;  // Whether the instance count is valid

பொது:
    InlineModModuleVertex(InlineModGraph& graph, Ast_முனை_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = இயல்பு;

    // ACCESSORS
    Ast_முனை_Module* modp() மாறிலி { திரும்பு m_modp; }
    size_t size() மாறிலி { திரும்பு m_size; }
    வெறுமை size(size_t மதிப்பு) { m_size = மதிப்பு; }
    வெறுமை sizeInc(size_t மதிப்பு = 1) { m_size += மதிப்பு; }
    தருக்க noInlineHard() மாறிலி { திரும்பு m_noInlineHardWyp; }
    வெறுமை setNoInlineHard(மாறிலி எழுத்து* whyp) {
        எனில் (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    தருக்க noInlineSoft() மாறிலி { திரும்பு m_noInlineSoftWyp; }
    வெறுமை setNoInlineSoft(மாறிலி எழுத்து* whyp) {
        எனில் (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    தருக்க shouldInline() மாறிலி { திரும்பு m_shouldInlineWhyp; }
    வெறுமை setShouldInline(மாறிலி எழுத்து* whyp) {
        எனில் (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    வெறுமை setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() மாறிலி {
        எனில் (!m_flattenedSizeValid) {
            m_flattenedSizeValid = உண்மை;
            m_flattenedSize = m_size;
            க்கு (மாறிலி V3GraphEdge& e1 : outEdges()) {
                க்கு (மாறிலி V3GraphEdge& e2 : e1.உச்சி()->outEdges()) {
                    InlineModModuleVertex* மாறிலி mVtxp = e2.உச்சி()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        திரும்பு m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_மொத்தம்() மாறிலி {
        எனில் (!m_instance_மொத்தம்_Valid) {
            m_instance_மொத்தம்_Valid = உண்மை;
            m_instance_மொத்தம் = 0;
            க்கு (மாறிலி V3GraphEdge& e1 : inEdges()) {
                க்கு (மாறிலி V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* மாறிலி mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_மொத்தம் += mVtxp->instance_மொத்தம்();
                }
            }
            எனில் (!m_instance_மொத்தம்) {
                UASSERT_OBJ(m_modp->is_உச்சி(), m_modp, "non-top level module should have instances");
                m_instance_மொத்தம் = 1;
            }
        }
        திரும்பு m_instance_மொத்தம்;
    }

    // debug
    File_வரி* fileline() மாறிலி override { திரும்பு m_modp->fileline(); }
    std::string dotShape() மாறிலி override { திரும்பு "box"; }
    std::string dotColor() மாறிலி override {
        திரும்பு m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() மாறிலி override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_மொத்தம்());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        எனில் (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        எனில் (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        எனில் (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        திரும்பு str;
    }
};

வகுப்பு InlineModCellVertex final : பொது InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* மாறிலி m_cellp;  // The cell (instance)
    மாறிலி எழுத்து* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    தருக்க m_flatten = பொய்;  // Whether this cell and below already flattened (avoid O(n^2))

பொது:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = இயல்பு;

    // ACCESSORS
    AstCell* cellp() மாறிலி { திரும்பு m_cellp; }
    தருக்க doInline() மாறிலி { திரும்பு m_doInlineWyp; }
    வெறுமை setDoInline(மாறிலி எழுத்து* whyp) {
        எனில் (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    தருக்க flatten() மாறிலி { திரும்பு m_flatten; }
    வெறுமை setFlatten() { m_flatten = உண்மை; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() மாறிலி {
        UASSERT_OBJ(outSize1(), இது, "Cell should have exactly one outgoing edge");
        திரும்பு *outEdges().frontp()->உச்சி()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() மாறிலி {
        UASSERT_OBJ(inSize1(), இது, "Cell should have exactly one incoming edge");
        திரும்பு *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_வரி* fileline() மாறிலி override { திரும்பு m_cellp->fileline(); }
    std::string dotColor() மாறிலி override { திரும்பு m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() மாறிலி override { திரும்பு "ellipse"; }
    std::string name() மாறிலி override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        எனில் (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        திரும்பு str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_முனை_Module* modp) {
    எனில் (!modp->user4p()) modp->user4p(புதிய InlineModModuleVertex{*இது, modp});
    திரும்பு modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    எனில் (!cellp->user4p()) cellp->user4p(புதிய InlineModCellVertex{*இது, cellp});
    திரும்பு cellp->user4u().to<InlineModCellVertex*>();
}

வெறுமை InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    புதிய V3GraphEdge{இது, &parent, &cell, 1, /* cutable: */ பொய்};
}
வெறுமை InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    புதிய V3GraphEdge{இது, &cell, &submodule, 1, /* cutable: */ பொய்};
}

வெறுமை InlineModModuleVertex::setFlatten() {
    க்கு (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.உச்சி()->as<InlineModCellVertex>();
        எனில் (cVtx.flatten()) தொடர்;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        எனில் (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

வகுப்பு InlineModGraphBuilder final : பொது VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{புதிய InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    மாறிலி AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* மாறிலி vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        எனில் (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        எனில் (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        எனில் (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        எனில் (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        எனில் (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        எனில் (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    வெறுமை visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        எனில் (!m_sizedScopep) m_sizedScopep = nodep;
        எனில் (m_sizedScopep != nodep) திரும்பு;
        எனில் (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    வெறுமை visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* மாறிலி vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstPragma* nodep) override {
        எனில் (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            எனில் (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } இல்லையெனில் {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }

        எனில் (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            எனில் (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } இல்லையெனில் {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (m_modVtxp) m_modVtxp->sizeInc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_உச்சி(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        க்கு (மாறிலி V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            எனில் (&vtx == m_graphp->vertices().frontp()) தொடர்;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = இயல்பு;

பொது:
    நிலையான std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        திரும்பு std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

நாமாகாஶ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<மாறிலி AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
வெறுமை gatherScopes(மாறிலி Ast_முனை_Module* modp, ScopeChildren& children) {
    க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        எனில் (AstScope* மாறிலி scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].தள்ளு_back(scopep);
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_Module* மாறிலி subModp = VN_CAST(nodep, முனை_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
வெறுமை renameScopes(AstScope* scopep, மாறிலி std::string& oldPrefix, மாறிலி std::string& newPrefix,
                  மாறிலி ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    மாறிலி auto it = children.find(scopep);
    எனில் (it == children.end()) திரும்பு;
    க்கு (AstScope* மாறிலி childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
வெறுமை inlineScope(AstScope* scopep, AstCell* cellp, மாறிலி std::string& prefix,
                 ScopeChildren& children) {
    AstScope* மாறிலி parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    க்கு (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        எனில் (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        எனில் (AstVar* மாறிலி newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        எனில் (!cellp->isTrace()) vscp->trace(பொய்);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    க்கு (Ast_முனை *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        எனில் (Ast_முனை_FTask* மாறிலி ftaskp = VN_CAST(nodep, முனை_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        எனில் (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_முனை_Cover_Decl* மாறிலி declp = incp->declp();
                எனில் (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    எனில் (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    திரும்பு;
                }
                எனில் (Ast_முனை_Cover_Decl* மாறிலி newDeclp = VN_CAST(declp->user3p(), முனை_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    க்கு (Ast_முனை *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* மாறிலி cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    எனில் (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            புதிய AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    மாறிலி std::string oldPrefix = scopep->name() + ".";
    மாறிலி std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        மாறிலி auto it = children.find(scopep);
        எனில் (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    க்கு (AstScope* மாறிலி childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        எனில் (AstCell* மாறிலி newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.செருகு(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
வெறுமை inlineCell(Ast_முனை_Module* modp, AstCell* cellp, தருக்க last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    மாறிலி VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.தள்ளு_Deletep(cellp->unlinkFrBack());

    Ast_முனை_Module* மாறிலி subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    க்கு (Ast_முனை *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* மாறிலி scopep = VN_CAST(nodep, Scope);
        எனில் (!scopep) தொடர்;
        scopep->unlinkFrBack();
        எனில் (scopep->aboveCellp() == cellp) {
            inlineScopeps.தள்ளு_back(scopep);
            deleter.தள்ளு_Deletep(scopep);
        } இல்லையெனில் {
            otherScopeps.தள்ளு_back(scopep);
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
    Ast_முனை_Module* inlinedp;
    எனில் (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } இல்லையெனில் {
        inlinedp = subModp->clone_மரம்(பொய்);
        // Compute map from the original module items to their clones
        க்கு (Ast_முனை *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        எனில் (v3Global.opt.coverageFsm()) {
            க்கு (Ast_முனை *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                மாறிலி Ast_முனை_Cover_Decl* மாறிலி declp = VN_CAST(nodep, முனை_Cover_Decl);
                எனில் (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        க்கு (AstScope* மாறிலி scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.தள்ளு_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    மாறிலி std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    எனில் (AstSystemCSection* மாறிலி sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* மாறிலி mVtxp = graph.getInlineModModuleVertexp(modp);
    க்கு (Ast_முனை *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            எனில் (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            எனில் (!cellp->isTrace()) varp->trace(பொய்);
        } இல்லையெனில் எனில் (AstCell* மாறிலி subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* மாறிலி vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } இல்லையெனில் எனில் (AstTypedef* மாறிலி typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } இல்லையெனில் எனில் (Ast_முனை_Cover_Decl* மாறிலி declp = VN_CAST(nodep, முனை_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            எனில் (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    க்கு (AstScope* மாறிலி scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
வெறுமை process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    க்கு (Ast_முனை_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), முனை_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    க்கு (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* மாறிலி cVtxp = vtx.cast<InlineModCellVertex>();
        எனில் (!cVtxp) தொடர்;
        எனில் (!cVtxp->doInline()) தொடர்;
        cVtxps.தள்ளு_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    க்கு (InlineModCellVertex* மாறிலி cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* மாறிலி iVtxp = &cVtxp->instanceOf();
        AstCell* மாறிலி cellp = cVtxp->cellp();
        மாறிலி தருக்க last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        எனில் (last) {
            போது (!iVtxp->outEmpty()) {
                InlineModCellVertex* மாறிலி tVtxp
                    = iVtxp->outEdges().frontp()->உச்சி()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        எனில் (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_முனை_Var_Ref* refp) {
        AstVarScope* மாறிலி vscp = refp->varScopep();
        எனில் (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

வெறுமை V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    மாறிலி size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        எனில் (InlineModModuleVertex* மாறிலி mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            எனில் (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            எனில் (mVtxp->noInlineHard()) தொடர்;
            // Don't inline if soft off
            எனில் (mVtxp->noInlineSoft()) தொடர்;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_மொத்தம்();
            எனில் (totalSize * 5 < designSize) {
                க்கு (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* மாறிலி cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            தொடர்;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        எனில் (mVtx.noInlineHard()) தொடர்;

        // If it should be inlined, inlined it
        எனில் (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        எனில் (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        எனில் (mVtx.noInlineSoft()) தொடர்;

        // If instatiated in exactly one static site, inline it
        எனில் (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        எனில் (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        எனில் (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        மாறிலி size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        மாறிலி size_t limit = v3Global.opt.inlineMult();
        எனில் (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_மரம்("inline", 0, dump_மரம்_Either_Level() >= 3);
}
