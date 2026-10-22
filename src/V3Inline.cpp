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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Inline.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
ਸਥਿਰ ਅਚਲ ਪੂਰਨ INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

ਵਰਗ Inline_Mod_Module_ਸ਼ੀਰਸ਼;
ਵਰਗ Inline_Mod_Cell_ਸ਼ੀਰਸ਼;

ਵਰਗ Inline_Mod_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;

ਜਨਤਕ:
    Inline_Mod_ਗ੍ਰਾਫ()
        : V3_ਗ੍ਰਾਫ{} {}
    ~Inline_Mod_ਗ੍ਰਾਫ() override = ਮੂਲ;

    // METHODS
    Inline_Mod_Module_ਸ਼ੀਰਸ਼* getInlineModModuleVertexp(Ast_ਗੰਢ_Module* modp);
    Inline_Mod_Cell_ਸ਼ੀਰਸ਼* getInlineModCellVertexp(AstCell* cellp);
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_Mod_Module_ਸ਼ੀਰਸ਼& from, Inline_Mod_Cell_ਸ਼ੀਰਸ਼& ਮੰਜ਼ਿਲ);
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_Mod_Cell_ਸ਼ੀਰਸ਼& from, Inline_Mod_Module_ਸ਼ੀਰਸ਼& ਮੰਜ਼ਿਲ);

    // debug
    std::string dotRankDir() ਅਚਲ override { ਵਾਪਸ "LR"; }
};

ਵਰਗ Inline_Mod_Either_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Inline_Mod_Either_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
ਸੁਰੱਖਿਅਤ:
    ਸਪਸ਼ਟ Inline_Mod_Either_ਸ਼ੀਰਸ਼(Inline_Mod_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{&ਗ੍ਰਾਫ} {}
};

ਵਰਗ Inline_Mod_Module_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Inline_Mod_Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Inline_Mod_Module_ਸ਼ੀਰਸ਼, Inline_Mod_Either_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // The module
    ਅਚਲ ਅੱਖਰ* m_noInlineHardWyp = ਖਾਲੀ_ਸੂਚਕ;  // First reason the module can never be inlined
    ਅਚਲ ਅੱਖਰ* m_noInlineSoftWyp = ਖਾਲੀ_ਸੂਚਕ;  // First reason not to inline unless forced
    ਅਚਲ ਅੱਖਰ* m_shouldInlineWhyp = ਖਾਲੀ_ਸੂਚਕ;  // First reason why this module should be inlined
    ਮਾਪ_t m_ਮਾਪ = 0;  // The size (statement count) of the module
    ਮਾਪ_t ਬਦਲਣਯੋਗ m_flattened_ਮਾਪ = 0;  // The size of the module if flattened
    ਤਾਰਕਿਕ ਬਦਲਣਯੋਗ m_flattened_ਮਾਪ_Valid = ਝੂਠ;  // Whether the flattened size is valid
    ਮਾਪ_t ਬਦਲਣਯੋਗ m_instance_ਗਿਣਤੀ = 0;  // The number of total instances of this module
    ਤਾਰਕਿਕ ਬਦਲਣਯੋਗ m_instance_ਗਿਣਤੀ_Valid = ਝੂਠ;  // Whether the instance count is valid

ਜਨਤਕ:
    Inline_Mod_Module_ਸ਼ੀਰਸ਼(Inline_Mod_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, Ast_ਗੰਢ_Module* modp)
        : Inline_Mod_Either_ਸ਼ੀਰਸ਼{ਗ੍ਰਾਫ}
        , m_modp{modp} {}
    ~Inline_Mod_Module_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    Ast_ਗੰਢ_Module* modp() ਅਚਲ { ਵਾਪਸ m_modp; }
    ਮਾਪ_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਮਾਪ; }
    ਖਾਲੀ ਮਾਪ(ਮਾਪ_t ਮੁੱਲ) { m_ਮਾਪ = ਮੁੱਲ; }
    ਖਾਲੀ ਮਾਪ_Inc(ਮਾਪ_t ਮੁੱਲ = 1) { m_ਮਾਪ += ਮੁੱਲ; }
    ਤਾਰਕਿਕ noInlineHard() ਅਚਲ { ਵਾਪਸ m_noInlineHardWyp; }
    ਖਾਲੀ ਸੈੱਟ_No_Inline_Hard(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    ਤਾਰਕਿਕ noInlineSoft() ਅਚਲ { ਵਾਪਸ m_noInlineSoftWyp; }
    ਖਾਲੀ ਸੈੱਟ_No_Inline_Soft(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    ਤਾਰਕਿਕ shouldInline() ਅਚਲ { ਵਾਪਸ m_shouldInlineWhyp; }
    ਖਾਲੀ ਸੈੱਟ_Should_Inline(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    ਖਾਲੀ ਸੈੱਟ_Flatten();

    // Total size of module, with all hierarchy below flattened
    ਮਾਪ_t flattened_ਮਾਪ() ਅਚਲ {
        ਜੇ (!m_flattened_ਮਾਪ_Valid) {
            m_flattened_ਮਾਪ_Valid = ਸੱਚ;
            m_flattened_ਮਾਪ = m_ਮਾਪ;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e1 : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e2 : e1.ਸਿਖਰ()->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    Inline_Mod_Module_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = e2.ਸਿਖਰ()->as<Inline_Mod_Module_ਸ਼ੀਰਸ਼>();
                    m_flattened_ਮਾਪ += mVtxp->flattened_ਮਾਪ();
                }
            }
        }
        ਵਾਪਸ m_flattened_ਮਾਪ;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    ਮਾਪ_t instance_ਗਿਣਤੀ() ਅਚਲ {
        ਜੇ (!m_instance_ਗਿਣਤੀ_Valid) {
            m_instance_ਗਿਣਤੀ_Valid = ਸੱਚ;
            m_instance_ਗਿਣਤੀ = 0;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e1 : ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e2 : e1.fromp()->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                    Inline_Mod_Module_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = e2.fromp()->as<Inline_Mod_Module_ਸ਼ੀਰਸ਼>();
                    m_instance_ਗਿਣਤੀ += mVtxp->instance_ਗਿਣਤੀ();
                }
            }
            ਜੇ (!m_instance_ਗਿਣਤੀ) {
                UASSERT_OBJ(m_modp->is_ਸਿਖਰ(), m_modp, "non-top level module should have instances");
                m_instance_ਗਿਣਤੀ = 1;
            }
        }
        ਵਾਪਸ m_instance_ਗਿਣਤੀ;
    }

    // debug
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ m_modp->fileline(); }
    std::string dotShape() ਅਚਲ override { ਵਾਪਸ "box"; }
    std::string dotColor() ਅਚਲ override {
        ਵਾਪਸ m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ਅਚਲ override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_ਮੰਜ਼ਿਲ_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::ਮੰਜ਼ਿਲ_string(instance_ਗਿਣਤੀ());
        str += "\nsize: " + std::ਮੰਜ਼ਿਲ_string(m_ਮਾਪ);
        str += "\nflattenedSize: " + std::ਮੰਜ਼ਿਲ_string(flattened_ਮਾਪ());
        ਜੇ (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        ਜੇ (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        ਜੇ (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        ਵਾਪਸ str;
    }
};

ਵਰਗ Inline_Mod_Cell_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Inline_Mod_Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Inline_Mod_Cell_ਸ਼ੀਰਸ਼, Inline_Mod_Either_ਸ਼ੀਰਸ਼)
    AstCell* ਅਚਲ m_cellp;  // The cell (instance)
    ਅਚਲ ਅੱਖਰ* m_doInlineWyp = ਖਾਲੀ_ਸੂਚਕ;  // First reason this instance should be inlined
    ਤਾਰਕਿਕ m_flatten = ਝੂਠ;  // Whether this cell and below already flattened (avoid O(n^2))

ਜਨਤਕ:
    Inline_Mod_Cell_ਸ਼ੀਰਸ਼(Inline_Mod_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, AstCell* cellp)
        : Inline_Mod_Either_ਸ਼ੀਰਸ਼{ਗ੍ਰਾਫ}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    AstCell* cellp() ਅਚਲ { ਵਾਪਸ m_cellp; }
    ਤਾਰਕਿਕ doInline() ਅਚਲ { ਵਾਪਸ m_doInlineWyp; }
    ਖਾਲੀ ਸੈੱਟ_Do_Inline(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    ਤਾਰਕਿਕ flatten() ਅਚਲ { ਵਾਪਸ m_flatten; }
    ਖਾਲੀ ਸੈੱਟ_Flatten() { m_flatten = ਸੱਚ; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_ਸ਼ੀਰਸ਼& instance_ਦਾ() ਅਚਲ {
        UASSERT_OBJ(ਬਾਹਰਲਾ_Size1(), ਇਹ, "Cell should have exactly one outgoing edge");
        ਵਾਪਸ *ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp()->ਸਿਖਰ()->as<Inline_Mod_Module_ਸ਼ੀਰਸ਼>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_ਸ਼ੀਰਸ਼& instance_ਵਿੱਚ() ਅਚਲ {
        UASSERT_OBJ(ਵਿੱਚ_Size1(), ਇਹ, "Cell should have exactly one incoming edge");
        ਵਾਪਸ *ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()->fromp()->as<Inline_Mod_Module_ਸ਼ੀਰਸ਼>();
    }

    // debug
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ m_cellp->fileline(); }
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ਅਚਲ override { ਵਾਪਸ "ellipse"; }
    std::string name() ਅਚਲ override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_ਮੰਜ਼ਿਲ_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        ਜੇ (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        ਵਾਪਸ str;
    }
};

Inline_Mod_Module_ਸ਼ੀਰਸ਼* Inline_Mod_ਗ੍ਰਾਫ::getInlineModModuleVertexp(Ast_ਗੰਢ_Module* modp) {
    ਜੇ (!modp->user4p()) modp->user4p(ਨਵਾਂ Inline_Mod_Module_ਸ਼ੀਰਸ਼{*ਇਹ, modp});
    ਵਾਪਸ modp->user4u().ਮੰਜ਼ਿਲ<Inline_Mod_Module_ਸ਼ੀਰਸ਼*>();
}
Inline_Mod_Cell_ਸ਼ੀਰਸ਼* Inline_Mod_ਗ੍ਰਾਫ::getInlineModCellVertexp(AstCell* cellp) {
    ਜੇ (!cellp->user4p()) cellp->user4p(ਨਵਾਂ Inline_Mod_Cell_ਸ਼ੀਰਸ਼{*ਇਹ, cellp});
    ਵਾਪਸ cellp->user4u().ਮੰਜ਼ਿਲ<Inline_Mod_Cell_ਸ਼ੀਰਸ਼*>();
}

ਖਾਲੀ Inline_Mod_ਗ੍ਰਾਫ::ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_Mod_Module_ਸ਼ੀਰਸ਼& ਮਾਪਾ, Inline_Mod_Cell_ਸ਼ੀਰਸ਼& cell) {
    UASSERT_OBJ(cell.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ(), &cell, "Cell should have at most one incoming edge");
    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, &ਮਾਪਾ, &cell, 1, /* cutable: */ ਝੂਠ};
}
ਖਾਲੀ Inline_Mod_ਗ੍ਰਾਫ::ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_Mod_Cell_ਸ਼ੀਰਸ਼& cell, Inline_Mod_Module_ਸ਼ੀਰਸ਼& submodule) {
    UASSERT_OBJ(cell.ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ(), &cell, "Cell should have at most one outgoing edge");
    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, &cell, &submodule, 1, /* cutable: */ ਝੂਠ};
}

ਖਾਲੀ Inline_Mod_Module_ਸ਼ੀਰਸ਼::ਸੈੱਟ_Flatten() {
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        Inline_Mod_Cell_ਸ਼ੀਰਸ਼& cVtx = *ਕਿਨਾਰਾ.ਸਿਖਰ()->as<Inline_Mod_Cell_ਸ਼ੀਰਸ਼>();
        ਜੇ (cVtx.flatten()) ਜਾਰੀ;
        cVtx.ਸੈੱਟ_Flatten();
        Inline_Mod_Module_ਸ਼ੀਰਸ਼& iVtx = cVtx.instance_ਦਾ();
        ਜੇ (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.ਸੈੱਟ_Do_Inline("flatten parent");
        iVtx.ਸੈੱਟ_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

ਵਰਗ Inline_Mod_ਗ੍ਰਾਫ_Builder final : ਜਨਤਕ VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_ਗ੍ਰਾਫ> m_graphp{ਨਵਾਂ Inline_Mod_ਗ੍ਰਾਫ};  // The graph being built
    Inline_Mod_Module_ਸ਼ੀਰਸ਼* m_modVtxp = ਖਾਲੀ_ਸੂਚਕ;  // Vertex of module currently being iterated
    ਅਚਲ AstScope* m_sizedScopep = ਖਾਲੀ_ਸੂਚਕ;  // The scope of current module measured for size

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        ਜੇ (VN_IS(nodep, Iface)) vtxp->ਸੈੱਟ_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        ਜੇ (nodep->isConstPool()) vtxp->ਸੈੱਟ_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        ਜੇ (VN_IS(nodep, Package)) vtxp->ਸੈੱਟ_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        ਜੇ (nodep->verilatorLib()) vtxp->ਸੈੱਟ_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        ਜੇ (nodep->mod_ਜਨਤਕ()) vtxp->ਸੈੱਟ_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        ਜੇ (m_modVtxp) m_modVtxp->ਸੈੱਟ_No_Inline_Hard("Contains class");
    }

    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        ਜੇ (!m_sizedScopep) m_sizedScopep = nodep;
        ਜੇ (m_sizedScopep != nodep) ਵਾਪਸ;
        ਜੇ (m_modVtxp) m_modVtxp->ਮਾਪ_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->ਜੋੜੋ_ਕਿਨਾਰਾ(*m_modVtxp, *vtxp);
        m_graphp->ਜੋੜੋ_ਕਿਨਾਰਾ(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstPragma* nodep) override {
        ਜੇ (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            ਜੇ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ਨਹੀਂਤਾਂ {
                m_modVtxp->ਸੈੱਟ_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        ਜੇ (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            ਜੇ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ਨਹੀਂਤਾਂ {
                m_modVtxp->ਸੈੱਟ_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (m_modVtxp) m_modVtxp->ਮਾਪ_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ Inline_Mod_ਗ੍ਰਾਫ_Builder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->ਕ੍ਰਮ();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_ਸ਼ੀਰਸ਼>()->modp()->is_ਸਿਖਰ(),
                    nodep, "First vertex should be top level module");
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            ਜੇ (&vtx == m_graphp->vertices().frontp()) ਜਾਰੀ;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ(), &vtx, "Should have edges from root");
        }
#ਜੇ_ਅੰਤ
    }
    ~Inline_Mod_ਗ੍ਰਾਫ_Builder() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ std::unique_ptr<Inline_Mod_ਗ੍ਰਾਫ> apply(AstNetlist* nodep) {
        ਵਾਪਸ std::move(Inline_Mod_ਗ੍ਰਾਫ_Builder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

ਨਾਮਾਕਾਸ਼ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
ਵਰਤੋਂ ScopeChildren = std::unordered_map<ਅਚਲ AstScope*, std::ਸਦਿਸ਼<AstScope*>>;

// Record downward links from parent scopes to their child scopes
ਖਾਲੀ gatherScopes(ਅਚਲ Ast_ਗੰਢ_Module* modp, ScopeChildren& children) {
    ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ਜੇ (AstScope* ਅਚਲ scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ਧੱਕੋ_ਪਿੱਠ(scopep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ subModp = VN_CAST(nodep, ਗੰਢ_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
ਖਾਲੀ renameScopes(AstScope* scopep, ਅਚਲ std::string& old_ਅਗੇਤਰ, ਅਚਲ std::string& new_ਅਗੇਤਰ,
                  ਅਚਲ ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_ਅਗੇਤਰ), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_ਅਗੇਤਰ + scopep->name().substr(old_ਅਗੇਤਰ.ਮਾਪ()));
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = children.ਲੱਭੋ(scopep);
    ਜੇ (it == children.ਸਿਰਾ()) ਵਾਪਸ;
    ਲਈ (AstScope* ਅਚਲ childp : it->ਦੂਜਾ) {
        renameScopes(childp, old_ਅਗੇਤਰ, new_ਅਗੇਤਰ, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
ਖਾਲੀ inlineScope(AstScope* scopep, AstCell* cellp, ਅਚਲ std::string& ਅਗੇਤਰ,
                 ScopeChildren& children) {
    AstScope* ਅਚਲ ਮਾਪਾ_Scopep = scopep->aboveScopep();
    UASSERT_OBJ(ਮਾਪਾ_Scopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << ਮਾਪਾ_Scopep);

    // Move the variables of the inlined scope into the scope above
    ਲਈ (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        ਜੇ (vscp->scopep() == scopep) vscp->scopep(ਮਾਪਾ_Scopep);
        // If the module was cloned, point to the cloned variable
        ਜੇ (AstVar* ਅਚਲ newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        ਜੇ (!cellp->isTrace()) vscp->trace(ਝੂਠ);
        ਮਾਪਾ_Scopep->ਜੋੜੋ_Varsp(vscp->unlink_Fr_ਪਿੱਠ());
    }

    // Move the logic of the inlined scope into the scope above
    ਲਈ (Ast_ਗੰਢ *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_ਪਿੱਠ();
        ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = VN_CAST(nodep, ਗੰਢ_FTask)) {
            ftaskp->name(ਅਗੇਤਰ + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        ਜੇ (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp = incp->declp();
                ਜੇ (declp->ਪ੍ਰਤੀ_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    ਜੇ (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    ਵਾਪਸ;
                }
                ਜੇ (Ast_ਗੰਢ_Cover_Decl* ਅਚਲ newDeclp = VN_CAST(declp->user3p(), ਗੰਢ_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        ਮਾਪਾ_Scopep->ਜੋੜੋ_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    ਲਈ (Ast_ਗੰਢ *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ਅਚਲ cisp = VN_AS(nodep->unlink_Fr_ਪਿੱਠ(), CellInlineScope);
        cisp->name(ਅਗੇਤਰ + cisp->name());
        ਮਾਪਾ_Scopep->ਜੋੜੋ_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    ਜੇ (v3Global.opt.vpi()) {
        ਮਾਪਾ_Scopep->ਜੋੜੋ_Inlinesp(
            ਨਵਾਂ AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ਅਚਲ std::string old_ਅਗੇਤਰ = scopep->name() + ".";
    ਅਚਲ std::string new_ਅਗੇਤਰ = scopep->name() + "__DOT__";
    std::ਸਦਿਸ਼<AstScope*> ਬੱਚਾ_Scopeps;
    {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = children.ਲੱਭੋ(scopep);
        ਜੇ (it != children.ਸਿਰਾ()) {
            ਬੱਚਾ_Scopeps = std::move(it->ਦੂਜਾ);
            children.erase(it);
        }
    }
    ਲਈ (AstScope* ਅਚਲ ਬੱਚਾ_Scopep : ਬੱਚਾ_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(ਬੱਚਾ_Scopep->modp(), Class), ਬੱਚਾ_Scopep,
                    "Inlined scope should not contain a class scope");
        ਜੇ (AstCell* ਅਚਲ newCellp = VN_CAST(ਬੱਚਾ_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            ਬੱਚਾ_Scopep->aboveCellp(newCellp);
        }
        ਬੱਚਾ_Scopep->aboveScopep(ਮਾਪਾ_Scopep);
        renameScopes(ਬੱਚਾ_Scopep, old_ਅਗੇਤਰ, new_ਅਗੇਤਰ, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::ਸਦਿਸ਼<AstScope*>& ਮਾਪਾ_Childps = children[ਮਾਪਾ_Scopep];
    ਮਾਪਾ_Childps.erase(std::remove(ਮਾਪਾ_Childps.begin(), ਮਾਪਾ_Childps.ਸਿਰਾ(), scopep),
                        ਮਾਪਾ_Childps.ਸਿਰਾ());
    ਮਾਪਾ_Childps.ਪਾਓ(ਮਾਪਾ_Childps.ਸਿਰਾ(), ਬੱਚਾ_Scopeps.begin(), ਬੱਚਾ_Scopeps.ਸਿਰਾ());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
ਖਾਲੀ inlineCell(Ast_ਗੰਢ_Module* modp, AstCell* cellp, ਤਾਰਕਿਕ ਆਖਰੀ, Inline_Mod_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;

    VNDeleter deleter;
    deleter.ਧੱਕੋ_Deletep(cellp->unlink_Fr_ਪਿੱਠ());

    Ast_ਗੰਢ_Module* ਅਚਲ subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::ਸਦਿਸ਼<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::ਸਦਿਸ਼<AstScope*> otherScopeps;  // Scopes under some other instance
    ਲਈ (Ast_ਗੰਢ *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ਅਚਲ scopep = VN_CAST(nodep, Scope);
        ਜੇ (!scopep) ਜਾਰੀ;
        scopep->unlink_Fr_ਪਿੱਠ();
        ਜੇ (scopep->aboveCellp() == cellp) {
            inlineScopeps.ਧੱਕੋ_ਪਿੱਠ(scopep);
            deleter.ਧੱਕੋ_Deletep(scopep);
        } ਨਹੀਂਤਾਂ {
            otherScopeps.ਧੱਕੋ_ਪਿੱਠ(scopep);
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
    Ast_ਗੰਢ_Module* inlinedp;
    ਜੇ (ਆਖਰੀ) {
        inlinedp = subModp->unlink_Fr_ਪਿੱਠ();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.ਖਾਲੀ_ਥਾਵਾਂ(), cellp, "Last instance, but has other scopes");
    } ਨਹੀਂਤਾਂ {
        inlinedp = subModp->clone_ਰੁੱਖ(ਝੂਠ);
        // Compute map from the original module items to their clones
        ਲਈ (Ast_ਗੰਢ *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        ਜੇ (v3Global.opt.coverageFsm()) {
            ਲਈ (Ast_ਗੰਢ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ਅਚਲ Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp = VN_CAST(nodep, ਗੰਢ_Cover_Decl);
                ਜੇ (declp && declp->ਪ੍ਰਤੀ_Instance()) {
                    VL_DO_DANGLING(deleter.ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        ਲਈ (AstScope* ਅਚਲ scopep : otherScopeps) subModp->ਜੋੜੋ_Stmtsp(scopep);
    }
    deleter.ਧੱਕੋ_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ਅਚਲ std::string ਅਗੇਤਰ = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    ਜੇ (AstSystemCSection* ਅਚਲ sectionsp = inlinedp->scSectionsp()) {
        modp->ਜੋੜੋ_Sc_Sectionsp(sectionsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = ਗ੍ਰਾਫ.getInlineModModuleVertexp(modp);
    ਲਈ (Ast_ਗੰਢ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_ਪਿੱਠ();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
            varp->name(ਅਗੇਤਰ + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            ਜੇ (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            ਜੇ (!cellp->isTrace()) varp->trace(ਝੂਠ);
        } ਨਹੀਂਤਾਂ ਜੇ (AstCell* ਅਚਲ subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(ਅਗੇਤਰ + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(ਖਾਲੀ_ਸੂਚਕ);
            Inline_Mod_Cell_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਗ੍ਰਾਫ.getInlineModCellVertexp(subCellp);
            ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(*mVtxp, *vtxp);
            ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(*vtxp, *ਗ੍ਰਾਫ.getInlineModModuleVertexp(subCellp->modp()));
        } ਨਹੀਂਤਾਂ ਜੇ (AstTypedef* ਅਚਲ typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(ਅਗੇਤਰ + typedefp->name());
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp = VN_CAST(nodep, ਗੰਢ_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            ਜੇ (!declp->ਪ੍ਰਤੀ_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->ਜੋੜੋ_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    ਲਈ (AstScope* ਅਚਲ scopep : inlineScopeps) inlineScope(scopep, cellp, ਅਗੇਤਰ, children);
}

// Apply all inlining decisions
ਖਾਲੀ process(AstNetlist* netlistp, Inline_Mod_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    ਲਈ (Ast_ਗੰਢ_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::ਸਦਿਸ਼<Inline_Mod_Cell_ਸ਼ੀਰਸ਼*> cVtxps;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) {
        Inline_Mod_Cell_ਸ਼ੀਰਸ਼* ਅਚਲ cVtxp = vtx.cast<Inline_Mod_Cell_ਸ਼ੀਰਸ਼>();
        ਜੇ (!cVtxp) ਜਾਰੀ;
        ਜੇ (!cVtxp->doInline()) ਜਾਰੀ;
        cVtxps.ਧੱਕੋ_ਪਿੱਠ(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    ਲਈ (Inline_Mod_Cell_ਸ਼ੀਰਸ਼* ਅਚਲ cVtxp : vlstd::ਉਲਟ_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_ਸ਼ੀਰਸ਼& mVtx = cVtxp->instance_ਵਿੱਚ();
        Inline_Mod_Module_ਸ਼ੀਰਸ਼* ਅਚਲ iVtxp = &cVtxp->instance_ਦਾ();
        AstCell* ਅਚਲ cellp = cVtxp->cellp();
        ਅਚਲ ਤਾਰਕਿਕ ਆਖਰੀ = iVtxp->ਵਿੱਚ_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.ਮਾਪ_Inc(iVtxp->ਮਾਪ());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&ਗ੍ਰਾਫ), cVtxp);
        // Delete the module we are inlining if this is the last instance
        ਜੇ (ਆਖਰੀ) {
            ਜਦੋਂ (!iVtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) {
                Inline_Mod_Cell_ਸ਼ੀਰਸ਼* ਅਚਲ tVtxp
                    = iVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp()->ਸਿਖਰ()->as<Inline_Mod_Cell_ਸ਼ੀਰਸ਼>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&ਗ੍ਰਾਫ), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&ਗ੍ਰਾਫ), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, ਆਖਰੀ, ਗ੍ਰਾਫ, children);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) ਗ੍ਰਾਫ.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ਗੰਢ_Var_Ref* refp) {
        AstVarScope* ਅਚਲ vscp = refp->varScopep();
        ਜੇ (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::ਜੋੜੋ_Stat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

ਖਾਲੀ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<Inline_Mod_ਗ੍ਰਾਫ> graphp = Inline_Mod_ਗ੍ਰਾਫ_Builder::apply(nodep);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ਅਚਲ ਮਾਪ_t design_ਮਾਪ
        = graphp->vertices().frontp()->as<Inline_Mod_Module_ਸ਼ੀਰਸ਼>()->flattened_ਮਾਪ();
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        ਜੇ (Inline_Mod_Module_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = vtx.cast<Inline_Mod_Module_ਸ਼ੀਰਸ਼>()) {
            // If this module is less than 10% of the design, flatten this module
            ਜੇ (mVtxp->flattened_ਮਾਪ() * 10 < design_ਮਾਪ) mVtxp->ਸੈੱਟ_Flatten();
            // Don't inline if can't inline
            ਜੇ (mVtxp->noInlineHard()) ਜਾਰੀ;
            // Don't inline if soft off
            ਜੇ (mVtxp->noInlineSoft()) ਜਾਰੀ;
            // If all instances of this module combined are less than 20% of the design, inline all
            ਮਾਪ_t ਕੁੱਲ_ਮਾਪ = mVtxp->flattened_ਮਾਪ() * mVtxp->instance_ਗਿਣਤੀ();
            ਜੇ (ਕੁੱਲ_ਮਾਪ * 5 < design_ਮਾਪ) {
                ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                    Inline_Mod_Cell_ਸ਼ੀਰਸ਼* ਅਚਲ cVtxp = ਕਿਨਾਰਾ.fromp()->as<Inline_Mod_Cell_ਸ਼ੀਰਸ਼>();
                    cVtxp->ਸੈੱਟ_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            ਜਾਰੀ;
        }

        // The instantiation
        Inline_Mod_Cell_ਸ਼ੀਰਸ਼& cVtx = *vtx.as<Inline_Mod_Cell_ਸ਼ੀਰਸ਼>();
        // The module instantiated by this cell
        Inline_Mod_Module_ਸ਼ੀਰਸ਼& mVtx = cVtx.instance_ਦਾ();

        // Don't inline if can't inline, duh!
        ਜੇ (mVtx.noInlineHard()) ਜਾਰੀ;

        // If it should be inlined, inlined it
        ਜੇ (mVtx.shouldInline()) cVtx.ਸੈੱਟ_Do_Inline("should inline");
        // If --flatten, inline it
        ਜੇ (v3Global.opt.flatten()) cVtx.ਸੈੱਟ_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        ਜੇ (mVtx.noInlineSoft()) ਜਾਰੀ;

        // If instatiated in exactly one static site, inline it
        ਜੇ (mVtx.ਵਿੱਚ_Size1()) cVtx.ਸੈੱਟ_Do_Inline("Single static instance");
        // If small, inline it
        ਜੇ (mVtx.ਮਾਪ() < INLINE_MODS_SMALLER) cVtx.ਸੈੱਟ_Do_Inline("Small");
        // If inlineMult is 0, inline it
        ਜੇ (v3Global.opt.inlineMult() < 1) cVtx.ਸੈੱਟ_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ਅਚਲ ਮਾਪ_t inlined_ਮਾਪ = mVtx.ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ() * mVtx.ਮਾਪ();
        ਅਚਲ ਮਾਪ_t ਹੱਦਬੰਦੀ = v3Global.opt.inlineMult();
        ਜੇ (inlined_ਮਾਪ < ਹੱਦਬੰਦੀ) cVtx.ਸੈੱਟ_Do_Inline("inlinedSize < inlineMult");
    }
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_ਰੁੱਖ("inline", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
