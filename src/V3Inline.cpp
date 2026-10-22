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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Inline.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
స్థిర స్థిరరాశి పూర్ణాంకం INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

తరగతి Inline_Mod_Module_శీర్షం;
తరగతి Inline_Mod_Cell_శీర్షం;

తరగతి Inline_Mod_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_లోపల_Use m_user4_లోపల_Use;

బహిరంగ:
    Inline_Mod_లేఖాచిత్రం()
        : V3_లేఖాచిత్రం{} {}
    ~Inline_Mod_లేఖాచిత్రం() override = అప్రమేయ;

    // METHODS
    Inline_Mod_Module_శీర్షం* getInlineModModuleVertexp(Ast_కణుపు_Module* modp);
    Inline_Mod_Cell_శీర్షం* getInlineModCellVertexp(AstCell* cellp);
    శూన్య కూడు_అంచు(Inline_Mod_Module_శీర్షం& from, Inline_Mod_Cell_శీర్షం& గమ్యం);
    శూన్య కూడు_అంచు(Inline_Mod_Cell_శీర్షం& from, Inline_Mod_Module_శీర్షం& గమ్యం);

    // debug
    std::string dotRankDir() స్థిరరాశి override { తిరిగి "LR"; }
};

తరగతి Inline_Mod_Either_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Inline_Mod_Either_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
రక్షిత:
    స్పష్టమైన Inline_Mod_Either_శీర్షం(Inline_Mod_లేఖాచిత్రం& లేఖాచిత్రం)
        : V3_లేఖాచిత్రం_శీర్షం{&లేఖాచిత్రం} {}
};

తరగతి Inline_Mod_Module_శీర్షం final : బహిరంగ Inline_Mod_Either_శీర్షం {
    VL_RTTI_IMPL(Inline_Mod_Module_శీర్షం, Inline_Mod_Either_శీర్షం)
    Ast_కణుపు_Module* స్థిరరాశి m_modp;  // The module
    స్థిరరాశి అక్షరం* m_noInlineHardWyp = శూన్యసూచిక;  // First reason the module can never be inlined
    స్థిరరాశి అక్షరం* m_noInlineSoftWyp = శూన్యసూచిక;  // First reason not to inline unless forced
    స్థిరరాశి అక్షరం* m_shouldInlineWhyp = శూన్యసూచిక;  // First reason why this module should be inlined
    కొలత_t m_కొలత = 0;  // The size (statement count) of the module
    కొలత_t మార్చదగిన m_flattened_కొలత = 0;  // The size of the module if flattened
    తార్కిక మార్చదగిన m_flattened_కొలత_Valid = అబద్ధం;  // Whether the flattened size is valid
    కొలత_t మార్చదగిన m_instance_లెక్క = 0;  // The number of total instances of this module
    తార్కిక మార్చదగిన m_instance_లెక్క_Valid = అబద్ధం;  // Whether the instance count is valid

బహిరంగ:
    Inline_Mod_Module_శీర్షం(Inline_Mod_లేఖాచిత్రం& లేఖాచిత్రం, Ast_కణుపు_Module* modp)
        : Inline_Mod_Either_శీర్షం{లేఖాచిత్రం}
        , m_modp{modp} {}
    ~Inline_Mod_Module_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    Ast_కణుపు_Module* modp() స్థిరరాశి { తిరిగి m_modp; }
    కొలత_t కొలత() స్థిరరాశి { తిరిగి m_కొలత; }
    శూన్య కొలత(కొలత_t విలువ) { m_కొలత = విలువ; }
    శూన్య కొలత_Inc(కొలత_t విలువ = 1) { m_కొలత += విలువ; }
    తార్కిక noInlineHard() స్థిరరాశి { తిరిగి m_noInlineHardWyp; }
    శూన్య సమితి_No_Inline_Hard(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    తార్కిక noInlineSoft() స్థిరరాశి { తిరిగి m_noInlineSoftWyp; }
    శూన్య సమితి_No_Inline_Soft(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    తార్కిక shouldInline() స్థిరరాశి { తిరిగి m_shouldInlineWhyp; }
    శూన్య సమితి_Should_Inline(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    శూన్య సమితి_Flatten();

    // Total size of module, with all hierarchy below flattened
    కొలత_t flattened_కొలత() స్థిరరాశి {
        ఒకవేళ (!m_flattened_కొలత_Valid) {
            m_flattened_కొలత_Valid = నిజం;
            m_flattened_కొలత = m_కొలత;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& e1 : బహిర్గతం_అంచులు()) {
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& e2 : e1.శిఖరం()->బహిర్గతం_అంచులు()) {
                    Inline_Mod_Module_శీర్షం* స్థిరరాశి mVtxp = e2.శిఖరం()->as<Inline_Mod_Module_శీర్షం>();
                    m_flattened_కొలత += mVtxp->flattened_కొలత();
                }
            }
        }
        తిరిగి m_flattened_కొలత;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    కొలత_t instance_లెక్క() స్థిరరాశి {
        ఒకవేళ (!m_instance_లెక్క_Valid) {
            m_instance_లెక్క_Valid = నిజం;
            m_instance_లెక్క = 0;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& e1 : లోపల_అంచులు()) {
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& e2 : e1.fromp()->లోపల_అంచులు()) {
                    Inline_Mod_Module_శీర్షం* స్థిరరాశి mVtxp = e2.fromp()->as<Inline_Mod_Module_శీర్షం>();
                    m_instance_లెక్క += mVtxp->instance_లెక్క();
                }
            }
            ఒకవేళ (!m_instance_లెక్క) {
                UASSERT_OBJ(m_modp->is_శిఖరం(), m_modp, "non-top level module should have instances");
                m_instance_లెక్క = 1;
            }
        }
        తిరిగి m_instance_లెక్క;
    }

    // debug
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి m_modp->fileline(); }
    std::string dotShape() స్థిరరాశి override { తిరిగి "box"; }
    std::string dotColor() స్థిరరాశి override {
        తిరిగి m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() స్థిరరాశి override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_గమ్యం_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::గమ్యం_string(instance_లెక్క());
        str += "\nsize: " + std::గమ్యం_string(m_కొలత);
        str += "\nflattenedSize: " + std::గమ్యం_string(flattened_కొలత());
        ఒకవేళ (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        ఒకవేళ (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        ఒకవేళ (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        తిరిగి str;
    }
};

తరగతి Inline_Mod_Cell_శీర్షం final : బహిరంగ Inline_Mod_Either_శీర్షం {
    VL_RTTI_IMPL(Inline_Mod_Cell_శీర్షం, Inline_Mod_Either_శీర్షం)
    AstCell* స్థిరరాశి m_cellp;  // The cell (instance)
    స్థిరరాశి అక్షరం* m_doInlineWyp = శూన్యసూచిక;  // First reason this instance should be inlined
    తార్కిక m_flatten = అబద్ధం;  // Whether this cell and below already flattened (avoid O(n^2))

బహిరంగ:
    Inline_Mod_Cell_శీర్షం(Inline_Mod_లేఖాచిత్రం& లేఖాచిత్రం, AstCell* cellp)
        : Inline_Mod_Either_శీర్షం{లేఖాచిత్రం}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    AstCell* cellp() స్థిరరాశి { తిరిగి m_cellp; }
    తార్కిక doInline() స్థిరరాశి { తిరిగి m_doInlineWyp; }
    శూన్య సమితి_Do_Inline(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    తార్కిక flatten() స్థిరరాశి { తిరిగి m_flatten; }
    శూన్య సమితి_Flatten() { m_flatten = నిజం; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_శీర్షం& instance_యొక్క() స్థిరరాశి {
        UASSERT_OBJ(బహిర్గతం_Size1(), ఇది, "Cell should have exactly one outgoing edge");
        తిరిగి *బహిర్గతం_అంచులు().frontp()->శిఖరం()->as<Inline_Mod_Module_శీర్షం>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_శీర్షం& instance_లోపల() స్థిరరాశి {
        UASSERT_OBJ(లోపల_Size1(), ఇది, "Cell should have exactly one incoming edge");
        తిరిగి *లోపల_అంచులు().frontp()->fromp()->as<Inline_Mod_Module_శీర్షం>();
    }

    // debug
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి m_cellp->fileline(); }
    std::string dotColor() స్థిరరాశి override { తిరిగి m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() స్థిరరాశి override { తిరిగి "ellipse"; }
    std::string name() స్థిరరాశి override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_గమ్యం_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        ఒకవేళ (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        తిరిగి str;
    }
};

Inline_Mod_Module_శీర్షం* Inline_Mod_లేఖాచిత్రం::getInlineModModuleVertexp(Ast_కణుపు_Module* modp) {
    ఒకవేళ (!modp->user4p()) modp->user4p(కొత్త Inline_Mod_Module_శీర్షం{*ఇది, modp});
    తిరిగి modp->user4u().గమ్యం<Inline_Mod_Module_శీర్షం*>();
}
Inline_Mod_Cell_శీర్షం* Inline_Mod_లేఖాచిత్రం::getInlineModCellVertexp(AstCell* cellp) {
    ఒకవేళ (!cellp->user4p()) cellp->user4p(కొత్త Inline_Mod_Cell_శీర్షం{*ఇది, cellp});
    తిరిగి cellp->user4u().గమ్యం<Inline_Mod_Cell_శీర్షం*>();
}

శూన్య Inline_Mod_లేఖాచిత్రం::కూడు_అంచు(Inline_Mod_Module_శీర్షం& మాతృ, Inline_Mod_Cell_శీర్షం& cell) {
    UASSERT_OBJ(cell.లోపల_ఖాళీలు(), &cell, "Cell should have at most one incoming edge");
    కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, &మాతృ, &cell, 1, /* cutable: */ అబద్ధం};
}
శూన్య Inline_Mod_లేఖాచిత్రం::కూడు_అంచు(Inline_Mod_Cell_శీర్షం& cell, Inline_Mod_Module_శీర్షం& submodule) {
    UASSERT_OBJ(cell.బహిర్గతం_ఖాళీలు(), &cell, "Cell should have at most one outgoing edge");
    కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, &cell, &submodule, 1, /* cutable: */ అబద్ధం};
}

శూన్య Inline_Mod_Module_శీర్షం::సమితి_Flatten() {
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : బహిర్గతం_అంచులు()) {
        Inline_Mod_Cell_శీర్షం& cVtx = *అంచు.శిఖరం()->as<Inline_Mod_Cell_శీర్షం>();
        ఒకవేళ (cVtx.flatten()) కొనసాగు;
        cVtx.సమితి_Flatten();
        Inline_Mod_Module_శీర్షం& iVtx = cVtx.instance_యొక్క();
        ఒకవేళ (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.సమితి_Do_Inline("flatten parent");
        iVtx.సమితి_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

తరగతి Inline_Mod_లేఖాచిత్రం_Builder final : బహిరంగ VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_లేఖాచిత్రం> m_graphp{కొత్త Inline_Mod_లేఖాచిత్రం};  // The graph being built
    Inline_Mod_Module_శీర్షం* m_modVtxp = శూన్యసూచిక;  // Vertex of module currently being iterated
    స్థిరరాశి AstScope* m_sizedScopep = శూన్యసూచిక;  // The scope of current module measured for size

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_శీర్షం* స్థిరరాశి vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        ఒకవేళ (VN_IS(nodep, Iface)) vtxp->సమితి_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        ఒకవేళ (nodep->isConstPool()) vtxp->సమితి_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        ఒకవేళ (VN_IS(nodep, Package)) vtxp->సమితి_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        ఒకవేళ (nodep->verilatorLib()) vtxp->సమితి_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        ఒకవేళ (nodep->mod_ప్రజా()) vtxp->సమితి_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = శూన్యసూచిక;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        ఒకవేళ (m_modVtxp) m_modVtxp->సమితి_No_Inline_Hard("Contains class");
    }

    శూన్య సందర్శించు(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        ఒకవేళ (!m_sizedScopep) m_sizedScopep = nodep;
        ఒకవేళ (m_sizedScopep != nodep) తిరిగి;
        ఒకవేళ (m_modVtxp) m_modVtxp->కొలత_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    శూన్య సందర్శించు(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_శీర్షం* స్థిరరాశి vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->కూడు_అంచు(*m_modVtxp, *vtxp);
        m_graphp->కూడు_అంచు(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstPragma* nodep) override {
        ఒకవేళ (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            ఒకవేళ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } లేకపోతే {
                m_modVtxp->సమితి_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        ఒకవేళ (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            ఒకవేళ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } లేకపోతే {
                m_modVtxp->సమితి_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (m_modVtxp) m_modVtxp->కొలత_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    స్పష్టమైన Inline_Mod_లేఖాచిత్రం_Builder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->క్రమం();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_శీర్షం>()->modp()->is_శిఖరం(),
                    nodep, "First vertex should be top level module");
#నిర్వచితమైతే VL_DEBUG
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            ఒకవేళ (&vtx == m_graphp->vertices().frontp()) కొనసాగు;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.లోపల_ఖాళీలు(), &vtx, "Should have edges from root");
        }
#అయితే_ముగింపు
    }
    ~Inline_Mod_లేఖాచిత్రం_Builder() override = అప్రమేయ;

బహిరంగ:
    స్థిర std::unique_ptr<Inline_Mod_లేఖాచిత్రం> apply(AstNetlist* nodep) {
        తిరిగి std::move(Inline_Mod_లేఖాచిత్రం_Builder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

నామాకాశ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
ఉపయోగించి ScopeChildren = std::unordered_map<స్థిరరాశి AstScope*, std::సదిశ<AstScope*>>;

// Record downward links from parent scopes to their child scopes
శూన్య gatherScopes(స్థిరరాశి Ast_కణుపు_Module* modp, ScopeChildren& children) {
    కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ఒకవేళ (AstScope* స్థిరరాశి scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].నెట్టు_వెనుక(scopep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి subModp = VN_CAST(nodep, కణుపు_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
శూన్య renameScopes(AstScope* scopep, స్థిరరాశి std::string& old_ఉపసర్గ, స్థిరరాశి std::string& new_ఉపసర్గ,
                  స్థిరరాశి ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_ఉపసర్గ), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_ఉపసర్గ + scopep->name().substr(old_ఉపసర్గ.కొలత()));
    స్థిరరాశి స్వయంచాలక it = children.కనుగొను(scopep);
    ఒకవేళ (it == children.చివర()) తిరిగి;
    కొరకు (AstScope* స్థిరరాశి childp : it->రెండవ) {
        renameScopes(childp, old_ఉపసర్గ, new_ఉపసర్గ, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
శూన్య inlineScope(AstScope* scopep, AstCell* cellp, స్థిరరాశి std::string& ఉపసర్గ,
                 ScopeChildren& children) {
    AstScope* స్థిరరాశి మాతృ_Scopep = scopep->aboveScopep();
    UASSERT_OBJ(మాతృ_Scopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << మాతృ_Scopep);

    // Move the variables of the inlined scope into the scope above
    కొరకు (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        ఒకవేళ (vscp->scopep() == scopep) vscp->scopep(మాతృ_Scopep);
        // If the module was cloned, point to the cloned variable
        ఒకవేళ (AstVar* స్థిరరాశి newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        ఒకవేళ (!cellp->isTrace()) vscp->trace(అబద్ధం);
        మాతృ_Scopep->కూడు_Varsp(vscp->unlink_Fr_వెనుక());
    }

    // Move the logic of the inlined scope into the scope above
    కొరకు (Ast_కణుపు *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_వెనుక();
        ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp = VN_CAST(nodep, కణుపు_FTask)) {
            ftaskp->name(ఉపసర్గ + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        ఒకవేళ (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_కణుపు_Cover_Decl* స్థిరరాశి declp = incp->declp();
                ఒకవేళ (declp->చొప్పున_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    ఒకవేళ (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    తిరిగి;
                }
                ఒకవేళ (Ast_కణుపు_Cover_Decl* స్థిరరాశి newDeclp = VN_CAST(declp->user3p(), కణుపు_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        మాతృ_Scopep->కూడు_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    కొరకు (Ast_కణుపు *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* స్థిరరాశి cisp = VN_AS(nodep->unlink_Fr_వెనుక(), CellInlineScope);
        cisp->name(ఉపసర్గ + cisp->name());
        మాతృ_Scopep->కూడు_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    ఒకవేళ (v3Global.opt.vpi()) {
        మాతృ_Scopep->కూడు_Inlinesp(
            కొత్త AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    స్థిరరాశి std::string old_ఉపసర్గ = scopep->name() + ".";
    స్థిరరాశి std::string new_ఉపసర్గ = scopep->name() + "__DOT__";
    std::సదిశ<AstScope*> బిడ్డ_Scopeps;
    {
        స్థిరరాశి స్వయంచాలక it = children.కనుగొను(scopep);
        ఒకవేళ (it != children.చివర()) {
            బిడ్డ_Scopeps = std::move(it->రెండవ);
            children.erase(it);
        }
    }
    కొరకు (AstScope* స్థిరరాశి బిడ్డ_Scopep : బిడ్డ_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(బిడ్డ_Scopep->modp(), Class), బిడ్డ_Scopep,
                    "Inlined scope should not contain a class scope");
        ఒకవేళ (AstCell* స్థిరరాశి newCellp = VN_CAST(బిడ్డ_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            బిడ్డ_Scopep->aboveCellp(newCellp);
        }
        బిడ్డ_Scopep->aboveScopep(మాతృ_Scopep);
        renameScopes(బిడ్డ_Scopep, old_ఉపసర్గ, new_ఉపసర్గ, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::సదిశ<AstScope*>& మాతృ_Childps = children[మాతృ_Scopep];
    మాతృ_Childps.erase(std::remove(మాతృ_Childps.begin(), మాతృ_Childps.చివర(), scopep),
                        మాతృ_Childps.చివర());
    మాతృ_Childps.చొప్పించు(మాతృ_Childps.చివర(), బిడ్డ_Scopeps.begin(), బిడ్డ_Scopeps.చివర());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
శూన్య inlineCell(Ast_కణుపు_Module* modp, AstCell* cellp, తార్కిక చివరి, Inline_Mod_లేఖాచిత్రం& లేఖాచిత్రం,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;

    VNDeleter deleter;
    deleter.నెట్టు_Deletep(cellp->unlink_Fr_వెనుక());

    Ast_కణుపు_Module* స్థిరరాశి subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::సదిశ<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::సదిశ<AstScope*> otherScopeps;  // Scopes under some other instance
    కొరకు (Ast_కణుపు *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* స్థిరరాశి scopep = VN_CAST(nodep, Scope);
        ఒకవేళ (!scopep) కొనసాగు;
        scopep->unlink_Fr_వెనుక();
        ఒకవేళ (scopep->aboveCellp() == cellp) {
            inlineScopeps.నెట్టు_వెనుక(scopep);
            deleter.నెట్టు_Deletep(scopep);
        } లేకపోతే {
            otherScopeps.నెట్టు_వెనుక(scopep);
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
    Ast_కణుపు_Module* inlinedp;
    ఒకవేళ (చివరి) {
        inlinedp = subModp->unlink_Fr_వెనుక();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.ఖాళీలు(), cellp, "Last instance, but has other scopes");
    } లేకపోతే {
        inlinedp = subModp->clone_వృక్షం(అబద్ధం);
        // Compute map from the original module items to their clones
        కొరకు (Ast_కణుపు *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        ఒకవేళ (v3Global.opt.coverageFsm()) {
            కొరకు (Ast_కణుపు *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                స్థిరరాశి Ast_కణుపు_Cover_Decl* స్థిరరాశి declp = VN_CAST(nodep, కణుపు_Cover_Decl);
                ఒకవేళ (declp && declp->చొప్పున_Instance()) {
                    VL_DO_DANGLING(deleter.నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        కొరకు (AstScope* స్థిరరాశి scopep : otherScopeps) subModp->కూడు_Stmtsp(scopep);
    }
    deleter.నెట్టు_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    స్థిరరాశి std::string ఉపసర్గ = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    ఒకవేళ (AstSystemCSection* స్థిరరాశి sectionsp = inlinedp->scSectionsp()) {
        modp->కూడు_Sc_Sectionsp(sectionsp->unlink_Fr_వెనుక_With_తదుపరి());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_శీర్షం* స్థిరరాశి mVtxp = లేఖాచిత్రం.getInlineModModuleVertexp(modp);
    కొరకు (Ast_కణుపు *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_వెనుక();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
            varp->name(ఉపసర్గ + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            ఒకవేళ (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            ఒకవేళ (!cellp->isTrace()) varp->trace(అబద్ధం);
        } లేకపోతే ఒకవేళ (AstCell* స్థిరరాశి subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(ఉపసర్గ + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(శూన్యసూచిక);
            Inline_Mod_Cell_శీర్షం* స్థిరరాశి vtxp = లేఖాచిత్రం.getInlineModCellVertexp(subCellp);
            లేఖాచిత్రం.కూడు_అంచు(*mVtxp, *vtxp);
            లేఖాచిత్రం.కూడు_అంచు(*vtxp, *లేఖాచిత్రం.getInlineModModuleVertexp(subCellp->modp()));
        } లేకపోతే ఒకవేళ (AstTypedef* స్థిరరాశి typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(ఉపసర్గ + typedefp->name());
        } లేకపోతే ఒకవేళ (Ast_కణుపు_Cover_Decl* స్థిరరాశి declp = VN_CAST(nodep, కణుపు_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            ఒకవేళ (!declp->చొప్పున_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->కూడు_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    కొరకు (AstScope* స్థిరరాశి scopep : inlineScopeps) inlineScope(scopep, cellp, ఉపసర్గ, children);
}

// Apply all inlining decisions
శూన్య process(AstNetlist* netlistp, Inline_Mod_లేఖాచిత్రం& లేఖాచిత్రం) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    కొరకు (Ast_కణుపు_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), కణుపు_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::సదిశ<Inline_Mod_Cell_శీర్షం*> cVtxps;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) {
        Inline_Mod_Cell_శీర్షం* స్థిరరాశి cVtxp = vtx.cast<Inline_Mod_Cell_శీర్షం>();
        ఒకవేళ (!cVtxp) కొనసాగు;
        ఒకవేళ (!cVtxp->doInline()) కొనసాగు;
        cVtxps.నెట్టు_వెనుక(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    కొరకు (Inline_Mod_Cell_శీర్షం* స్థిరరాశి cVtxp : vlstd::తిరగేయి_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_శీర్షం& mVtx = cVtxp->instance_లోపల();
        Inline_Mod_Module_శీర్షం* స్థిరరాశి iVtxp = &cVtxp->instance_యొక్క();
        AstCell* స్థిరరాశి cellp = cVtxp->cellp();
        స్థిరరాశి తార్కిక చివరి = iVtxp->లోపల_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.కొలత_Inc(iVtxp->కొలత());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&లేఖాచిత్రం), cVtxp);
        // Delete the module we are inlining if this is the last instance
        ఒకవేళ (చివరి) {
            అయితే (!iVtxp->బహిర్గతం_ఖాళీలు()) {
                Inline_Mod_Cell_శీర్షం* స్థిరరాశి tVtxp
                    = iVtxp->బహిర్గతం_అంచులు().frontp()->శిఖరం()->as<Inline_Mod_Cell_శీర్షం>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&లేఖాచిత్రం), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&లేఖాచిత్రం), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, చివరి, లేఖాచిత్రం, children);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) లేఖాచిత్రం.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_కణుపు_Var_Ref* refp) {
        AstVarScope* స్థిరరాశి vscp = refp->varScopep();
        ఒకవేళ (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::కూడు_Stat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

శూన్య V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<Inline_Mod_లేఖాచిత్రం> graphp = Inline_Mod_లేఖాచిత్రం_Builder::apply(nodep);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    స్థిరరాశి కొలత_t design_కొలత
        = graphp->vertices().frontp()->as<Inline_Mod_Module_శీర్షం>()->flattened_కొలత();
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        ఒకవేళ (Inline_Mod_Module_శీర్షం* స్థిరరాశి mVtxp = vtx.cast<Inline_Mod_Module_శీర్షం>()) {
            // If this module is less than 10% of the design, flatten this module
            ఒకవేళ (mVtxp->flattened_కొలత() * 10 < design_కొలత) mVtxp->సమితి_Flatten();
            // Don't inline if can't inline
            ఒకవేళ (mVtxp->noInlineHard()) కొనసాగు;
            // Don't inline if soft off
            ఒకవేళ (mVtxp->noInlineSoft()) కొనసాగు;
            // If all instances of this module combined are less than 20% of the design, inline all
            కొలత_t మొత్తం_కొలత = mVtxp->flattened_కొలత() * mVtxp->instance_లెక్క();
            ఒకవేళ (మొత్తం_కొలత * 5 < design_కొలత) {
                కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mVtxp->లోపల_అంచులు()) {
                    Inline_Mod_Cell_శీర్షం* స్థిరరాశి cVtxp = అంచు.fromp()->as<Inline_Mod_Cell_శీర్షం>();
                    cVtxp->సమితి_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            కొనసాగు;
        }

        // The instantiation
        Inline_Mod_Cell_శీర్షం& cVtx = *vtx.as<Inline_Mod_Cell_శీర్షం>();
        // The module instantiated by this cell
        Inline_Mod_Module_శీర్షం& mVtx = cVtx.instance_యొక్క();

        // Don't inline if can't inline, duh!
        ఒకవేళ (mVtx.noInlineHard()) కొనసాగు;

        // If it should be inlined, inlined it
        ఒకవేళ (mVtx.shouldInline()) cVtx.సమితి_Do_Inline("should inline");
        // If --flatten, inline it
        ఒకవేళ (v3Global.opt.flatten()) cVtx.సమితి_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        ఒకవేళ (mVtx.noInlineSoft()) కొనసాగు;

        // If instatiated in exactly one static site, inline it
        ఒకవేళ (mVtx.లోపల_Size1()) cVtx.సమితి_Do_Inline("Single static instance");
        // If small, inline it
        ఒకవేళ (mVtx.కొలత() < INLINE_MODS_SMALLER) cVtx.సమితి_Do_Inline("Small");
        // If inlineMult is 0, inline it
        ఒకవేళ (v3Global.opt.inlineMult() < 1) cVtx.సమితి_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        స్థిరరాశి కొలత_t inlined_కొలత = mVtx.లోపల_అంచులు().కొలత() * mVtx.కొలత();
        స్థిరరాశి కొలత_t హద్దు = v3Global.opt.inlineMult();
        ఒకవేళ (inlined_కొలత < హద్దు) cVtx.సమితి_Do_Inline("inlinedSize < inlineMult");
    }
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_వృక్షం("inline", 0, dump_వృక్షం_Either_Level() >= 3);
}
