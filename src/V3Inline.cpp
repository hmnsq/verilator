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
स्थिर ध्रुव पूर्णाङ्क INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

वर्ग InlineModModuleVertex;
वर्ग InlineModCellVertex;

वर्ग InlineModGraph final : सार्वजनिक V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_अन्तर्_Use m_user4_अन्तर्_Use;

सार्वजनिक:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = स्वभाव;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_ग्रन्थिः_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    शून्य सङ्कलय_Edge(InlineModModuleVertex& from, InlineModCellVertex& गन्तव्यम्);
    शून्य सङ्कलय_Edge(InlineModCellVertex& from, InlineModModuleVertex& गन्तव्यम्);

    // debug
    std::string dotRankDir() ध्रुव override { प्रत्यागम "LR"; }
};

वर्ग InlineModEitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
रक्षित:
    स्पष्ट InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

वर्ग InlineModModuleVertex final : सार्वजनिक InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // The module
    ध्रुव वर्ण* m_noInlineHardWyp = शून्यसूचकः;  // First reason the module can never be inlined
    ध्रुव वर्ण* m_noInlineSoftWyp = शून्यसूचकः;  // First reason not to inline unless forced
    ध्रुव वर्ण* m_shouldInlineWhyp = शून्यसूचकः;  // First reason why this module should be inlined
    आकार_t m_आकार = 0;  // The size (statement count) of the module
    आकार_t परिवर्तनीय m_flattened_आकार = 0;  // The size of the module if flattened
    तार्किक परिवर्तनीय m_flattened_आकार_Valid = असत्य;  // Whether the flattened size is valid
    आकार_t परिवर्तनीय m_instance_गणनम् = 0;  // The number of total instances of this module
    तार्किक परिवर्तनीय m_instance_गणनम्_Valid = असत्य;  // Whether the instance count is valid

सार्वजनिक:
    InlineModModuleVertex(InlineModGraph& graph, Ast_ग्रन्थिः_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = स्वभाव;

    // ACCESSORS
    Ast_ग्रन्थिः_Module* modp() ध्रुव { प्रत्यागम m_modp; }
    आकार_t आकार() ध्रुव { प्रत्यागम m_आकार; }
    शून्य आकार(आकार_t मानम्) { m_आकार = मानम्; }
    शून्य आकार_Inc(आकार_t मानम् = 1) { m_आकार += मानम्; }
    तार्किक noInlineHard() ध्रुव { प्रत्यागम m_noInlineHardWyp; }
    शून्य समुच्चयः_No_Inline_Hard(ध्रुव वर्ण* whyp) {
        यदि (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    तार्किक noInlineSoft() ध्रुव { प्रत्यागम m_noInlineSoftWyp; }
    शून्य समुच्चयः_No_Inline_Soft(ध्रुव वर्ण* whyp) {
        यदि (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    तार्किक shouldInline() ध्रुव { प्रत्यागम m_shouldInlineWhyp; }
    शून्य समुच्चयः_Should_Inline(ध्रुव वर्ण* whyp) {
        यदि (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    शून्य समुच्चयः_Flatten();

    // Total size of module, with all hierarchy below flattened
    आकार_t flattened_आकार() ध्रुव {
        यदि (!m_flattened_आकार_Valid) {
            m_flattened_आकार_Valid = सत्य;
            m_flattened_आकार = m_आकार;
            प्रति (ध्रुव V3GraphEdge& e1 : बहिर्गमनम्_धाराः()) {
                प्रति (ध्रुव V3GraphEdge& e2 : e1.शिखरम्()->बहिर्गमनम्_धाराः()) {
                    InlineModModuleVertex* ध्रुव mVtxp = e2.शिखरम्()->as<InlineModModuleVertex>();
                    m_flattened_आकार += mVtxp->flattened_आकार();
                }
            }
        }
        प्रत्यागम m_flattened_आकार;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    आकार_t instance_गणनम्() ध्रुव {
        यदि (!m_instance_गणनम्_Valid) {
            m_instance_गणनम्_Valid = सत्य;
            m_instance_गणनम् = 0;
            प्रति (ध्रुव V3GraphEdge& e1 : अन्तर्_धाराः()) {
                प्रति (ध्रुव V3GraphEdge& e2 : e1.fromp()->अन्तर्_धाराः()) {
                    InlineModModuleVertex* ध्रुव mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_गणनम् += mVtxp->instance_गणनम्();
                }
            }
            यदि (!m_instance_गणनम्) {
                UASSERT_OBJ(m_modp->is_शिखरम्(), m_modp, "non-top level module should have instances");
                m_instance_गणनम् = 1;
            }
        }
        प्रत्यागम m_instance_गणनम्;
    }

    // debug
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम m_modp->fileline(); }
    std::string dotShape() ध्रुव override { प्रत्यागम "box"; }
    std::string dotColor() ध्रुव override {
        प्रत्यागम m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ध्रुव override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_गन्तव्यम्_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::गन्तव्यम्_string(instance_गणनम्());
        str += "\nsize: " + std::गन्तव्यम्_string(m_आकार);
        str += "\nflattenedSize: " + std::गन्तव्यम्_string(flattened_आकार());
        यदि (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        यदि (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        यदि (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        प्रत्यागम str;
    }
};

वर्ग InlineModCellVertex final : सार्वजनिक InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* ध्रुव m_cellp;  // The cell (instance)
    ध्रुव वर्ण* m_doInlineWyp = शून्यसूचकः;  // First reason this instance should be inlined
    तार्किक m_flatten = असत्य;  // Whether this cell and below already flattened (avoid O(n^2))

सार्वजनिक:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = स्वभाव;

    // ACCESSORS
    AstCell* cellp() ध्रुव { प्रत्यागम m_cellp; }
    तार्किक doInline() ध्रुव { प्रत्यागम m_doInlineWyp; }
    शून्य समुच्चयः_Do_Inline(ध्रुव वर्ण* whyp) {
        यदि (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    तार्किक flatten() ध्रुव { प्रत्यागम m_flatten; }
    शून्य समुच्चयः_Flatten() { m_flatten = सत्य; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instance_सम्बन्धि() ध्रुव {
        UASSERT_OBJ(बहिर्गमनम्_Size1(), एतत्, "Cell should have exactly one outgoing edge");
        प्रत्यागम *बहिर्गमनम्_धाराः().frontp()->शिखरम्()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instance_अन्तर्() ध्रुव {
        UASSERT_OBJ(अन्तर्_Size1(), एतत्, "Cell should have exactly one incoming edge");
        प्रत्यागम *अन्तर्_धाराः().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम m_cellp->fileline(); }
    std::string dotColor() ध्रुव override { प्रत्यागम m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ध्रुव override { प्रत्यागम "ellipse"; }
    std::string name() ध्रुव override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_गन्तव्यम्_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        यदि (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        प्रत्यागम str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_ग्रन्थिः_Module* modp) {
    यदि (!modp->user4p()) modp->user4p(नूतन InlineModModuleVertex{*एतत्, modp});
    प्रत्यागम modp->user4u().गन्तव्यम्<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    यदि (!cellp->user4p()) cellp->user4p(नूतन InlineModCellVertex{*एतत्, cellp});
    प्रत्यागम cellp->user4u().गन्तव्यम्<InlineModCellVertex*>();
}

शून्य InlineModGraph::सङ्कलय_Edge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.अन्तर्_रिक्तानि(), &cell, "Cell should have at most one incoming edge");
    नूतन V3GraphEdge{एतत्, &parent, &cell, 1, /* cutable: */ असत्य};
}
शून्य InlineModGraph::सङ्कलय_Edge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.बहिर्गमनम्_रिक्तानि(), &cell, "Cell should have at most one outgoing edge");
    नूतन V3GraphEdge{एतत्, &cell, &submodule, 1, /* cutable: */ असत्य};
}

शून्य InlineModModuleVertex::समुच्चयः_Flatten() {
    प्रति (V3GraphEdge& edge : बहिर्गमनम्_धाराः()) {
        InlineModCellVertex& cVtx = *edge.शिखरम्()->as<InlineModCellVertex>();
        यदि (cVtx.flatten()) अनुवर्त;
        cVtx.समुच्चयः_Flatten();
        InlineModModuleVertex& iVtx = cVtx.instance_सम्बन्धि();
        यदि (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.समुच्चयः_Do_Inline("flatten parent");
        iVtx.समुच्चयः_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

वर्ग InlineModGraphBuilder final : सार्वजनिक VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{नूतन InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = शून्यसूचकः;  // Vertex of module currently being iterated
    ध्रुव AstScope* m_sizedScopep = शून्यसूचकः;  // The scope of current module measured for size

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* ध्रुव vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        यदि (VN_IS(nodep, Iface)) vtxp->समुच्चयः_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        यदि (nodep->isConstPool()) vtxp->समुच्चयः_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        यदि (VN_IS(nodep, Package)) vtxp->समुच्चयः_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        यदि (nodep->verilatorLib()) vtxp->समुच्चयः_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        यदि (nodep->mod_लौकिकम्()) vtxp->समुच्चयः_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = शून्यसूचकः;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        यदि (m_modVtxp) m_modVtxp->समुच्चयः_No_Inline_Hard("Contains class");
    }

    शून्य दर्शय(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        यदि (!m_sizedScopep) m_sizedScopep = nodep;
        यदि (m_sizedScopep != nodep) प्रत्यागम;
        यदि (m_modVtxp) m_modVtxp->आकार_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    शून्य दर्शय(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* ध्रुव vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->सङ्कलय_Edge(*m_modVtxp, *vtxp);
        m_graphp->सङ्कलय_Edge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstPragma* nodep) override {
        यदि (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            यदि (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->समुच्चयः_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        यदि (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            यदि (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } अन्यथा {
                m_modVtxp->समुच्चयः_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (m_modVtxp) m_modVtxp->आकार_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    स्पष्ट InlineModGraphBuilder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->क्रमः();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_शिखरम्(),
                    nodep, "First vertex should be top level module");
#यदि_परिभाषितम् VL_DEBUG
        प्रति (ध्रुव V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            यदि (&vtx == m_graphp->vertices().frontp()) अनुवर्त;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.अन्तर्_रिक्तानि(), &vtx, "Should have edges from root");
        }
#यदि_अन्तः
    }
    ~InlineModGraphBuilder() override = स्वभाव;

सार्वजनिक:
    स्थिर std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        प्रत्यागम std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

नामाकाश ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
उपयुज्य ScopeChildren = std::unordered_map<ध्रुव AstScope*, std::सदिशम्<AstScope*>>;

// Record downward links from parent scopes to their child scopes
शून्य gatherScopes(ध्रुव Ast_ग्रन्थिः_Module* modp, ScopeChildren& children) {
    प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (AstScope* ध्रुव scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].प्रक्षिप_पृष्ठभागः(scopep);
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_Module* ध्रुव subModp = VN_CAST(nodep, ग्रन्थिः_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
शून्य renameScopes(AstScope* scopep, ध्रुव std::string& old_उपसर्गः, ध्रुव std::string& new_उपसर्गः,
                  ध्रुव ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_उपसर्गः), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_उपसर्गः + scopep->name().substr(old_उपसर्गः.आकार()));
    ध्रुव स्वचालित it = children.find(scopep);
    यदि (it == children.अन्तः_2()) प्रत्यागम;
    प्रति (AstScope* ध्रुव childp : it->second) {
        renameScopes(childp, old_उपसर्गः, new_उपसर्गः, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
शून्य inlineScope(AstScope* scopep, AstCell* cellp, ध्रुव std::string& उपसर्गः,
                 ScopeChildren& children) {
    AstScope* ध्रुव parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    प्रति (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        यदि (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        यदि (AstVar* ध्रुव newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        यदि (!cellp->isTrace()) vscp->trace(असत्य);
        parentScopep->सङ्कलय_Varsp(vscp->unlink_Fr_पृष्ठभागः());
    }

    // Move the logic of the inlined scope into the scope above
    प्रति (Ast_ग्रन्थिः *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_पृष्ठभागः();
        यदि (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp = VN_CAST(nodep, ग्रन्थिः_FTask)) {
            ftaskp->name(उपसर्गः + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        यदि (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp = incp->declp();
                यदि (declp->प्रत्येकम्_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    यदि (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    प्रत्यागम;
                }
                यदि (Ast_ग्रन्थिः_Cover_Decl* ध्रुव newDeclp = VN_CAST(declp->user3p(), ग्रन्थिः_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->सङ्कलय_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    प्रति (Ast_ग्रन्थिः *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ध्रुव cisp = VN_AS(nodep->unlink_Fr_पृष्ठभागः(), CellInlineScope);
        cisp->name(उपसर्गः + cisp->name());
        parentScopep->सङ्कलय_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    यदि (v3Global.opt.vpi()) {
        parentScopep->सङ्कलय_Inlinesp(
            नूतन AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ध्रुव std::string old_उपसर्गः = scopep->name() + ".";
    ध्रुव std::string new_उपसर्गः = scopep->name() + "__DOT__";
    std::सदिशम्<AstScope*> childScopeps;
    {
        ध्रुव स्वचालित it = children.find(scopep);
        यदि (it != children.अन्तः_2()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    प्रति (AstScope* ध्रुव childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        यदि (AstCell* ध्रुव newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            childScopep->aboveCellp(newCellp);
        }
        childScopep->aboveScopep(parentScopep);
        renameScopes(childScopep, old_उपसर्गः, new_उपसर्गः, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::सदिशम्<AstScope*>& parentChildps = children[parentScopep];
    parentChildps.erase(std::remove(parentChildps.begin(), parentChildps.अन्तः_2(), scopep),
                        parentChildps.अन्तः_2());
    parentChildps.निवेशय(parentChildps.अन्तः_2(), childScopeps.begin(), childScopeps.अन्तः_2());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
शून्य inlineCell(Ast_ग्रन्थिः_Module* modp, AstCell* cellp, तार्किक चरमः, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;

    VNDeleter deleter;
    deleter.प्रक्षिप_Deletep(cellp->unlink_Fr_पृष्ठभागः());

    Ast_ग्रन्थिः_Module* ध्रुव subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::सदिशम्<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::सदिशम्<AstScope*> otherScopeps;  // Scopes under some other instance
    प्रति (Ast_ग्रन्थिः *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ध्रुव scopep = VN_CAST(nodep, Scope);
        यदि (!scopep) अनुवर्त;
        scopep->unlink_Fr_पृष्ठभागः();
        यदि (scopep->aboveCellp() == cellp) {
            inlineScopeps.प्रक्षिप_पृष्ठभागः(scopep);
            deleter.प्रक्षिप_Deletep(scopep);
        } अन्यथा {
            otherScopeps.प्रक्षिप_पृष्ठभागः(scopep);
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
    Ast_ग्रन्थिः_Module* inlinedp;
    यदि (चरमः) {
        inlinedp = subModp->unlink_Fr_पृष्ठभागः();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.रिक्तानि(), cellp, "Last instance, but has other scopes");
    } अन्यथा {
        inlinedp = subModp->clone_वृक्षः(असत्य);
        // Compute map from the original module items to their clones
        प्रति (Ast_ग्रन्थिः *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        यदि (v3Global.opt.coverageFsm()) {
            प्रति (Ast_ग्रन्थिः *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ध्रुव Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp = VN_CAST(nodep, ग्रन्थिः_Cover_Decl);
                यदि (declp && declp->प्रत्येकम्_Instance()) {
                    VL_DO_DANGLING(deleter.प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        प्रति (AstScope* ध्रुव scopep : otherScopeps) subModp->सङ्कलय_Stmtsp(scopep);
    }
    deleter.प्रक्षिप_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ध्रुव std::string उपसर्गः = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    यदि (AstSystemCSection* ध्रुव sectionsp = inlinedp->scSectionsp()) {
        modp->सङ्कलय_Sc_Sectionsp(sectionsp->unlink_Fr_पृष्ठभागः_With_अग्र());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* ध्रुव mVtxp = graph.getInlineModModuleVertexp(modp);
    प्रति (Ast_ग्रन्थिः *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_पृष्ठभागः();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
            varp->name(उपसर्गः + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            यदि (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            यदि (!cellp->isTrace()) varp->trace(असत्य);
        } अन्यथा यदि (AstCell* ध्रुव subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(उपसर्गः + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(शून्यसूचकः);
            InlineModCellVertex* ध्रुव vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.सङ्कलय_Edge(*mVtxp, *vtxp);
            graph.सङ्कलय_Edge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } अन्यथा यदि (AstTypedef* ध्रुव typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(उपसर्गः + typedefp->name());
        } अन्यथा यदि (Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp = VN_CAST(nodep, ग्रन्थिः_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            यदि (!declp->प्रत्येकम्_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->सङ्कलय_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    प्रति (AstScope* ध्रुव scopep : inlineScopeps) inlineScope(scopep, cellp, उपसर्गः, children);
}

// Apply all inlining decisions
शून्य process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    प्रति (Ast_ग्रन्थिः_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::सदिशम्<InlineModCellVertex*> cVtxps;
    प्रति (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* ध्रुव cVtxp = vtx.cast<InlineModCellVertex>();
        यदि (!cVtxp) अनुवर्त;
        यदि (!cVtxp->doInline()) अनुवर्त;
        cVtxps.प्रक्षिप_पृष्ठभागः(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    प्रति (InlineModCellVertex* ध्रुव cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instance_अन्तर्();
        InlineModModuleVertex* ध्रुव iVtxp = &cVtxp->instance_सम्बन्धि();
        AstCell* ध्रुव cellp = cVtxp->cellp();
        ध्रुव तार्किक चरमः = iVtxp->अन्तर्_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.आकार_Inc(iVtxp->आकार());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        यदि (चरमः) {
            यावत् (!iVtxp->बहिर्गमनम्_रिक्तानि()) {
                InlineModCellVertex* ध्रुव tVtxp
                    = iVtxp->बहिर्गमनम्_धाराः().frontp()->शिखरम्()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, चरमः, graph, children);
        यदि (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ग्रन्थिः_Var_Ref* refp) {
        AstVarScope* ध्रुव vscp = refp->varScopep();
        यदि (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::सङ्कलय_Stat("Optimizations, Inlined instances", m_nInlined);
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
    ध्रुव आकार_t design_आकार
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattened_आकार();
    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (InlineModModuleVertex* ध्रुव mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            यदि (mVtxp->flattened_आकार() * 10 < design_आकार) mVtxp->समुच्चयः_Flatten();
            // Don't inline if can't inline
            यदि (mVtxp->noInlineHard()) अनुवर्त;
            // Don't inline if soft off
            यदि (mVtxp->noInlineSoft()) अनुवर्त;
            // If all instances of this module combined are less than 20% of the design, inline all
            आकार_t योग_आकार = mVtxp->flattened_आकार() * mVtxp->instance_गणनम्();
            यदि (योग_आकार * 5 < design_आकार) {
                प्रति (V3GraphEdge& edge : mVtxp->अन्तर्_धाराः()) {
                    InlineModCellVertex* ध्रुव cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->समुच्चयः_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            अनुवर्त;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instance_सम्बन्धि();

        // Don't inline if can't inline, duh!
        यदि (mVtx.noInlineHard()) अनुवर्त;

        // If it should be inlined, inlined it
        यदि (mVtx.shouldInline()) cVtx.समुच्चयः_Do_Inline("should inline");
        // If --flatten, inline it
        यदि (v3Global.opt.flatten()) cVtx.समुच्चयः_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        यदि (mVtx.noInlineSoft()) अनुवर्त;

        // If instatiated in exactly one static site, inline it
        यदि (mVtx.अन्तर्_Size1()) cVtx.समुच्चयः_Do_Inline("Single static instance");
        // If small, inline it
        यदि (mVtx.आकार() < INLINE_MODS_SMALLER) cVtx.समुच्चयः_Do_Inline("Small");
        // If inlineMult is 0, inline it
        यदि (v3Global.opt.inlineMult() < 1) cVtx.समुच्चयः_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ध्रुव आकार_t inlined_आकार = mVtx.अन्तर्_धाराः().आकार() * mVtx.आकार();
        ध्रुव आकार_t सीमा = v3Global.opt.inlineMult();
        यदि (inlined_आकार < सीमा) cVtx.समुच्चयः_Do_Inline("inlinedSize < inlineMult");
    }
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_वृक्षः("inline", 0, dump_वृक्षः_Either_Level() >= 3);
}
