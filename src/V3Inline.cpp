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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Inline.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

ವರ್ಗ Inline_Mod_Module_ಶೃಂಗ;
ವರ್ಗ Inline_Mod_Cell_ಶೃಂಗ;

ವರ್ಗ Inline_Mod_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;

ಸಾರ್ವಜನಿಕ:
    Inline_Mod_ನಕ್ಷೆ()
        : V3_ನಕ್ಷೆ{} {}
    ~Inline_Mod_ನಕ್ಷೆ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    Inline_Mod_Module_ಶೃಂಗ* getInlineModModuleVertexp(Ast_ಗಂಟು_Module* modp);
    Inline_Mod_Cell_ಶೃಂಗ* getInlineModCellVertexp(AstCell* cellp);
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(Inline_Mod_Module_ಶೃಂಗ& from, Inline_Mod_Cell_ಶೃಂಗ& ಗಮ್ಯ);
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(Inline_Mod_Cell_ಶೃಂಗ& from, Inline_Mod_Module_ಶೃಂಗ& ಗಮ್ಯ);

    // debug
    std::string dotRankDir() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "LR"; }
};

ವರ್ಗ Inline_Mod_Either_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Inline_Mod_Either_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
ಸಂರಕ್ಷಿತ:
    ಸ್ಪಷ್ಟ Inline_Mod_Either_ಶೃಂಗ(Inline_Mod_ನಕ್ಷೆ& ನಕ್ಷೆ)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{&ನಕ್ಷೆ} {}
};

ವರ್ಗ Inline_Mod_Module_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Inline_Mod_Either_ಶೃಂಗ {
    VL_RTTI_IMPL(Inline_Mod_Module_ಶೃಂಗ, Inline_Mod_Either_ಶೃಂಗ)
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // The module
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_noInlineHardWyp = ಶೂನ್ಯಸೂಚಕ;  // First reason the module can never be inlined
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_noInlineSoftWyp = ಶೂನ್ಯಸೂಚಕ;  // First reason not to inline unless forced
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_shouldInlineWhyp = ಶೂನ್ಯಸೂಚಕ;  // First reason why this module should be inlined
    ಗಾತ್ರ_t m_ಗಾತ್ರ = 0;  // The size (statement count) of the module
    ಗಾತ್ರ_t ಬದಲಾಯಿಸಬಹುದಾದ m_flattened_ಗಾತ್ರ = 0;  // The size of the module if flattened
    ತಾರ್ಕಿಕ ಬದಲಾಯಿಸಬಹುದಾದ m_flattened_ಗಾತ್ರ_Valid = ಸುಳ್ಳು;  // Whether the flattened size is valid
    ಗಾತ್ರ_t ಬದಲಾಯಿಸಬಹುದಾದ m_instance_ಎಣಿಕೆ = 0;  // The number of total instances of this module
    ತಾರ್ಕಿಕ ಬದಲಾಯಿಸಬಹುದಾದ m_instance_ಎಣಿಕೆ_Valid = ಸುಳ್ಳು;  // Whether the instance count is valid

ಸಾರ್ವಜನಿಕ:
    Inline_Mod_Module_ಶೃಂಗ(Inline_Mod_ನಕ್ಷೆ& ನಕ್ಷೆ, Ast_ಗಂಟು_Module* modp)
        : Inline_Mod_Either_ಶೃಂಗ{ನಕ್ಷೆ}
        , m_modp{modp} {}
    ~Inline_Mod_Module_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    Ast_ಗಂಟು_Module* modp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modp; }
    ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಾತ್ರ; }
    ಶೂನ್ಯ ಗಾತ್ರ_2(ಗಾತ್ರ_t ಮೌಲ್ಯ) { m_ಗಾತ್ರ = ಮೌಲ್ಯ; }
    ಶೂನ್ಯ ಗಾತ್ರ_Inc(ಗಾತ್ರ_t ಮೌಲ್ಯ = 1) { m_ಗಾತ್ರ += ಮೌಲ್ಯ; }
    ತಾರ್ಕಿಕ noInlineHard() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noInlineHardWyp; }
    ಶೂನ್ಯ ಗಣ_No_Inline_Hard(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    ತಾರ್ಕಿಕ noInlineSoft() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noInlineSoftWyp; }
    ಶೂನ್ಯ ಗಣ_No_Inline_Soft(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    ತಾರ್ಕಿಕ shouldInline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_shouldInlineWhyp; }
    ಶೂನ್ಯ ಗಣ_Should_Inline(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    ಶೂನ್ಯ ಗಣ_Flatten();

    // Total size of module, with all hierarchy below flattened
    ಗಾತ್ರ_t flattened_ಗಾತ್ರ() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!m_flattened_ಗಾತ್ರ_Valid) {
            m_flattened_ಗಾತ್ರ_Valid = ನಿಜ;
            m_flattened_ಗಾತ್ರ = m_ಗಾತ್ರ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& e1 : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& e2 : e1.ತುದಿ()->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    Inline_Mod_Module_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = e2.ತುದಿ()->as<Inline_Mod_Module_ಶೃಂಗ>();
                    m_flattened_ಗಾತ್ರ += mVtxp->flattened_ಗಾತ್ರ();
                }
            }
        }
        ಹಿಂತಿರುಗಿ m_flattened_ಗಾತ್ರ;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    ಗಾತ್ರ_t instance_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!m_instance_ಎಣಿಕೆ_Valid) {
            m_instance_ಎಣಿಕೆ_Valid = ನಿಜ;
            m_instance_ಎಣಿಕೆ = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& e1 : ಒಳ_ಅಂಚುಗಳು()) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& e2 : e1.fromp()->ಒಳ_ಅಂಚುಗಳು()) {
                    Inline_Mod_Module_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = e2.fromp()->as<Inline_Mod_Module_ಶೃಂಗ>();
                    m_instance_ಎಣಿಕೆ += mVtxp->instance_ಎಣಿಕೆ();
                }
            }
            ಆದರೆ (!m_instance_ಎಣಿಕೆ) {
                UASSERT_OBJ(m_modp->is_ತುದಿ(), m_modp, "non-top level module should have instances");
                m_instance_ಎಣಿಕೆ = 1;
            }
        }
        ಹಿಂತಿರುಗಿ m_instance_ಎಣಿಕೆ;
    }

    // debug
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_modp->fileline(); }
    std::string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "box"; }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_ಗಮ್ಯ_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::ಗಮ್ಯ_string(instance_ಎಣಿಕೆ());
        str += "\nsize: " + std::ಗಮ್ಯ_string(m_ಗಾತ್ರ);
        str += "\nflattenedSize: " + std::ಗಮ್ಯ_string(flattened_ಗಾತ್ರ());
        ಆದರೆ (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        ಆದರೆ (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        ಆದರೆ (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        ಹಿಂತಿರುಗಿ str;
    }
};

ವರ್ಗ Inline_Mod_Cell_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Inline_Mod_Either_ಶೃಂಗ {
    VL_RTTI_IMPL(Inline_Mod_Cell_ಶೃಂಗ, Inline_Mod_Either_ಶೃಂಗ)
    AstCell* ಸ್ಥಿರಾಂಕ m_cellp;  // The cell (instance)
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_doInlineWyp = ಶೂನ್ಯಸೂಚಕ;  // First reason this instance should be inlined
    ತಾರ್ಕಿಕ m_flatten = ಸುಳ್ಳು;  // Whether this cell and below already flattened (avoid O(n^2))

ಸಾರ್ವಜನಿಕ:
    Inline_Mod_Cell_ಶೃಂಗ(Inline_Mod_ನಕ್ಷೆ& ನಕ್ಷೆ, AstCell* cellp)
        : Inline_Mod_Either_ಶೃಂಗ{ನಕ್ಷೆ}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    AstCell* cellp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cellp; }
    ತಾರ್ಕಿಕ doInline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_doInlineWyp; }
    ಶೂನ್ಯ ಗಣ_Do_Inline(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    ತಾರ್ಕಿಕ flatten() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flatten; }
    ಶೂನ್ಯ ಗಣ_Flatten() { m_flatten = ನಿಜ; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_ಶೃಂಗ& instance_ದ() ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(ಹೊರಹೋಗು_Size1(), ಇದು, "Cell should have exactly one outgoing edge");
        ಹಿಂತಿರುಗಿ *ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp()->ತುದಿ()->as<Inline_Mod_Module_ಶೃಂಗ>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_ಶೃಂಗ& instance_ಒಳ() ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(ಒಳ_Size1(), ಇದು, "Cell should have exactly one incoming edge");
        ಹಿಂತಿರುಗಿ *ಒಳ_ಅಂಚುಗಳು().frontp()->fromp()->as<Inline_Mod_Module_ಶೃಂಗ>();
    }

    // debug
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_cellp->fileline(); }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "ellipse"; }
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_ಗಮ್ಯ_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        ಆದರೆ (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        ಹಿಂತಿರುಗಿ str;
    }
};

Inline_Mod_Module_ಶೃಂಗ* Inline_Mod_ನಕ್ಷೆ::getInlineModModuleVertexp(Ast_ಗಂಟು_Module* modp) {
    ಆದರೆ (!modp->user4p()) modp->user4p(ಹೊಸ Inline_Mod_Module_ಶೃಂಗ{*ಇದು, modp});
    ಹಿಂತಿರುಗಿ modp->user4u().ಗಮ್ಯ<Inline_Mod_Module_ಶೃಂಗ*>();
}
Inline_Mod_Cell_ಶೃಂಗ* Inline_Mod_ನಕ್ಷೆ::getInlineModCellVertexp(AstCell* cellp) {
    ಆದರೆ (!cellp->user4p()) cellp->user4p(ಹೊಸ Inline_Mod_Cell_ಶೃಂಗ{*ಇದು, cellp});
    ಹಿಂತಿರುಗಿ cellp->user4u().ಗಮ್ಯ<Inline_Mod_Cell_ಶೃಂಗ*>();
}

ಶೂನ್ಯ Inline_Mod_ನಕ್ಷೆ::ಕೂಡಿಸು_ಅಂಚು(Inline_Mod_Module_ಶೃಂಗ& ಪೋಷಕ, Inline_Mod_Cell_ಶೃಂಗ& cell) {
    UASSERT_OBJ(cell.ಒಳ_ಖಾಲಿಗಳು(), &cell, "Cell should have at most one incoming edge");
    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, &ಪೋಷಕ, &cell, 1, /* cutable: */ ಸುಳ್ಳು};
}
ಶೂನ್ಯ Inline_Mod_ನಕ್ಷೆ::ಕೂಡಿಸು_ಅಂಚು(Inline_Mod_Cell_ಶೃಂಗ& cell, Inline_Mod_Module_ಶೃಂಗ& submodule) {
    UASSERT_OBJ(cell.ಹೊರಹೋಗು_ಖಾಲಿಗಳು(), &cell, "Cell should have at most one outgoing edge");
    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, &cell, &submodule, 1, /* cutable: */ ಸುಳ್ಳು};
}

ಶೂನ್ಯ Inline_Mod_Module_ಶೃಂಗ::ಗಣ_Flatten() {
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        Inline_Mod_Cell_ಶೃಂಗ& cVtx = *ಅಂಚು.ತುದಿ()->as<Inline_Mod_Cell_ಶೃಂಗ>();
        ಆದರೆ (cVtx.flatten()) ಮುಂದುವರಿ;
        cVtx.ಗಣ_Flatten();
        Inline_Mod_Module_ಶೃಂಗ& iVtx = cVtx.instance_ದ();
        ಆದರೆ (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.ಗಣ_Do_Inline("flatten parent");
        iVtx.ಗಣ_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

ವರ್ಗ Inline_Mod_ನಕ್ಷೆ_Builder final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_ನಕ್ಷೆ> m_graphp{ಹೊಸ Inline_Mod_ನಕ್ಷೆ};  // The graph being built
    Inline_Mod_Module_ಶೃಂಗ* m_modVtxp = ಶೂನ್ಯಸೂಚಕ;  // Vertex of module currently being iterated
    ಸ್ಥಿರಾಂಕ AstScope* m_sizedScopep = ಶೂನ್ಯಸೂಚಕ;  // The scope of current module measured for size

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        ಆದರೆ (VN_IS(nodep, Iface)) vtxp->ಗಣ_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        ಆದರೆ (nodep->isConstPool()) vtxp->ಗಣ_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        ಆದರೆ (VN_IS(nodep, Package)) vtxp->ಗಣ_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        ಆದರೆ (nodep->verilatorLib()) vtxp->ಗಣ_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        ಆದರೆ (nodep->mod_ಬಹಿರಂಗ()) vtxp->ಗಣ_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = ಶೂನ್ಯಸೂಚಕ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        ಆದರೆ (m_modVtxp) m_modVtxp->ಗಣ_No_Inline_Hard("Contains class");
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        ಆದರೆ (!m_sizedScopep) m_sizedScopep = nodep;
        ಆದರೆ (m_sizedScopep != nodep) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_modVtxp) m_modVtxp->ಗಾತ್ರ_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->ಕೂಡಿಸು_ಅಂಚು(*m_modVtxp, *vtxp);
        m_graphp->ಕೂಡಿಸು_ಅಂಚು(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPragma* nodep) override {
        ಆದರೆ (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            ಆದರೆ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_modVtxp->ಗಣ_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            ಆದರೆ (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_modVtxp->ಗಣ_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (m_modVtxp) m_modVtxp->ಗಾತ್ರ_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Inline_Mod_ನಕ್ಷೆ_Builder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->ಕ್ರಮ();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_ಶೃಂಗ>()->modp()->is_ತುದಿ(),
                    nodep, "First vertex should be top level module");
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            ಆದರೆ (&vtx == m_graphp->vertices().frontp()) ಮುಂದುವರಿ;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.ಒಳ_ಖಾಲಿಗಳು(), &vtx, "Should have edges from root");
        }
#ಆದರೆ_ಅಂತ್ಯ
    }
    ~Inline_Mod_ನಕ್ಷೆ_Builder() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<Inline_Mod_ನಕ್ಷೆ> apply(AstNetlist* nodep) {
        ಹಿಂತಿರುಗಿ std::move(Inline_Mod_ನಕ್ಷೆ_Builder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

ನಾಮಾಕಾಶ ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
ಬಳಸಿ ScopeChildren = std::unordered_map<ಸ್ಥಿರಾಂಕ AstScope*, std::ಸದಿಶ<AstScope*>>;

// Record downward links from parent scopes to their child scopes
ಶೂನ್ಯ gatherScopes(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ScopeChildren& children) {
    ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ಆದರೆ (AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ತಳ್ಳು_ಹಿಂಭಾಗ(scopep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ subModp = VN_CAST(nodep, ಗಂಟು_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
ಶೂನ್ಯ renameScopes(AstScope* scopep, ಸ್ಥಿರಾಂಕ std::string& old_ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ std::string& new_ಪೂರ್ವಪ್ರತ್ಯಯ,
                  ಸ್ಥಿರಾಂಕ ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_ಪೂರ್ವಪ್ರತ್ಯಯ), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_ಪೂರ್ವಪ್ರತ್ಯಯ + scopep->name().substr(old_ಪೂರ್ವಪ್ರತ್ಯಯ.ಗಾತ್ರ_2()));
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = children.ಕಂಡುಹಿಡಿ(scopep);
    ಆದರೆ (it == children.ಕೊನೆ()) ಹಿಂತಿರುಗಿ;
    ಗಾಗಿ (AstScope* ಸ್ಥಿರಾಂಕ childp : it->ಎರಡನೇ) {
        renameScopes(childp, old_ಪೂರ್ವಪ್ರತ್ಯಯ, new_ಪೂರ್ವಪ್ರತ್ಯಯ, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
ಶೂನ್ಯ inlineScope(AstScope* scopep, AstCell* cellp, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ,
                 ScopeChildren& children) {
    AstScope* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Scopep = scopep->aboveScopep();
    UASSERT_OBJ(ಪೋಷಕ_Scopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << ಪೋಷಕ_Scopep);

    // Move the variables of the inlined scope into the scope above
    ಗಾಗಿ (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        ಆದರೆ (vscp->scopep() == scopep) vscp->scopep(ಪೋಷಕ_Scopep);
        // If the module was cloned, point to the cloned variable
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        ಆದರೆ (!cellp->isTrace()) vscp->trace(ಸುಳ್ಳು);
        ಪೋಷಕ_Scopep->ಕೂಡಿಸು_Varsp(vscp->unlink_Fr_ಹಿಂಭಾಗ());
    }

    // Move the logic of the inlined scope into the scope above
    ಗಾಗಿ (Ast_ಗಂಟು *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            ftaskp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        ಆದರೆ (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp = incp->declp();
                ಆದರೆ (declp->ಪ್ರತಿ_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    ಆದರೆ (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    ಹಿಂತಿರುಗಿ;
                }
                ಆದರೆ (Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ newDeclp = VN_CAST(declp->user3p(), ಗಂಟು_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        ಪೋಷಕ_Scopep->ಕೂಡಿಸು_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    ಗಾಗಿ (Ast_ಗಂಟು *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ಸ್ಥಿರಾಂಕ cisp = VN_AS(nodep->unlink_Fr_ಹಿಂಭಾಗ(), CellInlineScope);
        cisp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + cisp->name());
        ಪೋಷಕ_Scopep->ಕೂಡಿಸು_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    ಆದರೆ (v3Global.opt.vpi()) {
        ಪೋಷಕ_Scopep->ಕೂಡಿಸು_Inlinesp(
            ಹೊಸ AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ಸ್ಥಿರಾಂಕ std::string old_ಪೂರ್ವಪ್ರತ್ಯಯ = scopep->name() + ".";
    ಸ್ಥಿರಾಂಕ std::string new_ಪೂರ್ವಪ್ರತ್ಯಯ = scopep->name() + "__DOT__";
    std::ಸದಿಶ<AstScope*> ಮಗು_Scopeps;
    {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = children.ಕಂಡುಹಿಡಿ(scopep);
        ಆದರೆ (it != children.ಕೊನೆ()) {
            ಮಗು_Scopeps = std::move(it->ಎರಡನೇ);
            children.erase(it);
        }
    }
    ಗಾಗಿ (AstScope* ಸ್ಥಿರಾಂಕ ಮಗು_Scopep : ಮಗು_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(ಮಗು_Scopep->modp(), Class), ಮಗು_Scopep,
                    "Inlined scope should not contain a class scope");
        ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ newCellp = VN_CAST(ಮಗು_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            ಮಗು_Scopep->aboveCellp(newCellp);
        }
        ಮಗು_Scopep->aboveScopep(ಪೋಷಕ_Scopep);
        renameScopes(ಮಗು_Scopep, old_ಪೂರ್ವಪ್ರತ್ಯಯ, new_ಪೂರ್ವಪ್ರತ್ಯಯ, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::ಸದಿಶ<AstScope*>& ಪೋಷಕ_Childps = children[ಪೋಷಕ_Scopep];
    ಪೋಷಕ_Childps.erase(std::remove(ಪೋಷಕ_Childps.begin(), ಪೋಷಕ_Childps.ಕೊನೆ(), scopep),
                        ಪೋಷಕ_Childps.ಕೊನೆ());
    ಪೋಷಕ_Childps.ಸೇರಿಸು(ಪೋಷಕ_Childps.ಕೊನೆ(), ಮಗು_Scopeps.begin(), ಮಗು_Scopeps.ಕೊನೆ());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
ಶೂನ್ಯ inlineCell(Ast_ಗಂಟು_Module* modp, AstCell* cellp, ತಾರ್ಕಿಕ ಕೊನೆಯ, Inline_Mod_ನಕ್ಷೆ& ನಕ್ಷೆ,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;

    VNDeleter deleter;
    deleter.ತಳ್ಳು_Deletep(cellp->unlink_Fr_ಹಿಂಭಾಗ());

    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::ಸದಿಶ<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::ಸದಿಶ<AstScope*> otherScopeps;  // Scopes under some other instance
    ಗಾಗಿ (Ast_ಗಂಟು *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_CAST(nodep, Scope);
        ಆದರೆ (!scopep) ಮುಂದುವರಿ;
        scopep->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (scopep->aboveCellp() == cellp) {
            inlineScopeps.ತಳ್ಳು_ಹಿಂಭಾಗ(scopep);
            deleter.ತಳ್ಳು_Deletep(scopep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            otherScopeps.ತಳ್ಳು_ಹಿಂಭಾಗ(scopep);
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
    Ast_ಗಂಟು_Module* inlinedp;
    ಆದರೆ (ಕೊನೆಯ) {
        inlinedp = subModp->unlink_Fr_ಹಿಂಭಾಗ();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.ಖಾಲಿಗಳು(), cellp, "Last instance, but has other scopes");
    } ಇಲ್ಲದಿದ್ದರೆ {
        inlinedp = subModp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        // Compute map from the original module items to their clones
        ಗಾಗಿ (Ast_ಗಂಟು *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        ಆದರೆ (v3Global.opt.coverageFsm()) {
            ಗಾಗಿ (Ast_ಗಂಟು *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp = VN_CAST(nodep, ಗಂಟು_Cover_Decl);
                ಆದರೆ (declp && declp->ಪ್ರತಿ_Instance()) {
                    VL_DO_DANGLING(deleter.ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        ಗಾಗಿ (AstScope* ಸ್ಥಿರಾಂಕ scopep : otherScopeps) subModp->ಕೂಡಿಸು_Stmtsp(scopep);
    }
    deleter.ತಳ್ಳು_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ಸ್ಥಿರಾಂಕ std::string ಪೂರ್ವಪ್ರತ್ಯಯ = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    ಆದರೆ (AstSystemCSection* ಸ್ಥಿರಾಂಕ sectionsp = inlinedp->scSectionsp()) {
        modp->ಕೂಡಿಸು_Sc_Sectionsp(sectionsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = ನಕ್ಷೆ.getInlineModModuleVertexp(modp);
    ಗಾಗಿ (Ast_ಗಂಟು *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
            varp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            ಆದರೆ (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            ಆದರೆ (!cellp->isTrace()) varp->trace(ಸುಳ್ಳು);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(ಶೂನ್ಯಸೂಚಕ);
            Inline_Mod_Cell_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ನಕ್ಷೆ.getInlineModCellVertexp(subCellp);
            ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(*mVtxp, *vtxp);
            ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(*vtxp, *ನಕ್ಷೆ.getInlineModModuleVertexp(subCellp->modp()));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(ಪೂರ್ವಪ್ರತ್ಯಯ + typedefp->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp = VN_CAST(nodep, ಗಂಟು_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            ಆದರೆ (!declp->ಪ್ರತಿ_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->ಕೂಡಿಸು_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    ಗಾಗಿ (AstScope* ಸ್ಥಿರಾಂಕ scopep : inlineScopeps) inlineScope(scopep, cellp, ಪೂರ್ವಪ್ರತ್ಯಯ, children);
}

// Apply all inlining decisions
ಶೂನ್ಯ process(AstNetlist* netlistp, Inline_Mod_ನಕ್ಷೆ& ನಕ್ಷೆ) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::ಸದಿಶ<Inline_Mod_Cell_ಶೃಂಗ*> cVtxps;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) {
        Inline_Mod_Cell_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cVtxp = vtx.cast<Inline_Mod_Cell_ಶೃಂಗ>();
        ಆದರೆ (!cVtxp) ಮುಂದುವರಿ;
        ಆದರೆ (!cVtxp->doInline()) ಮುಂದುವರಿ;
        cVtxps.ತಳ್ಳು_ಹಿಂಭಾಗ(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    ಗಾಗಿ (Inline_Mod_Cell_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cVtxp : vlstd::ಹಿಮ್ಮುಖ_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_ಶೃಂಗ& mVtx = cVtxp->instance_ಒಳ();
        Inline_Mod_Module_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ iVtxp = &cVtxp->instance_ದ();
        AstCell* ಸ್ಥಿರಾಂಕ cellp = cVtxp->cellp();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಕೊನೆಯ = iVtxp->ಒಳ_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.ಗಾತ್ರ_Inc(iVtxp->ಗಾತ್ರ_2());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&ನಕ್ಷೆ), cVtxp);
        // Delete the module we are inlining if this is the last instance
        ಆದರೆ (ಕೊನೆಯ) {
            ಆಗ (!iVtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) {
                Inline_Mod_Cell_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ tVtxp
                    = iVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp()->ತುದಿ()->as<Inline_Mod_Cell_ಶೃಂಗ>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&ನಕ್ಷೆ), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&ನಕ್ಷೆ), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, ಕೊನೆಯ, ನಕ್ಷೆ, children);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) ನಕ್ಷೆ.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ಗಂಟು_Var_Ref* refp) {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
        ಆದರೆ (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

ಶೂನ್ಯ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<Inline_Mod_ನಕ್ಷೆ> graphp = Inline_Mod_ನಕ್ಷೆ_Builder::apply(nodep);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t design_ಗಾತ್ರ
        = graphp->vertices().frontp()->as<Inline_Mod_Module_ಶೃಂಗ>()->flattened_ಗಾತ್ರ();
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        ಆದರೆ (Inline_Mod_Module_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = vtx.cast<Inline_Mod_Module_ಶೃಂಗ>()) {
            // If this module is less than 10% of the design, flatten this module
            ಆದರೆ (mVtxp->flattened_ಗಾತ್ರ() * 10 < design_ಗಾತ್ರ) mVtxp->ಗಣ_Flatten();
            // Don't inline if can't inline
            ಆದರೆ (mVtxp->noInlineHard()) ಮುಂದುವರಿ;
            // Don't inline if soft off
            ಆದರೆ (mVtxp->noInlineSoft()) ಮುಂದುವರಿ;
            // If all instances of this module combined are less than 20% of the design, inline all
            ಗಾತ್ರ_t ಒಟ್ಟು_ಗಾತ್ರ = mVtxp->flattened_ಗಾತ್ರ() * mVtxp->instance_ಎಣಿಕೆ();
            ಆದರೆ (ಒಟ್ಟು_ಗಾತ್ರ * 5 < design_ಗಾತ್ರ) {
                ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mVtxp->ಒಳ_ಅಂಚುಗಳು()) {
                    Inline_Mod_Cell_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cVtxp = ಅಂಚು.fromp()->as<Inline_Mod_Cell_ಶೃಂಗ>();
                    cVtxp->ಗಣ_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            ಮುಂದುವರಿ;
        }

        // The instantiation
        Inline_Mod_Cell_ಶೃಂಗ& cVtx = *vtx.as<Inline_Mod_Cell_ಶೃಂಗ>();
        // The module instantiated by this cell
        Inline_Mod_Module_ಶೃಂಗ& mVtx = cVtx.instance_ದ();

        // Don't inline if can't inline, duh!
        ಆದರೆ (mVtx.noInlineHard()) ಮುಂದುವರಿ;

        // If it should be inlined, inlined it
        ಆದರೆ (mVtx.shouldInline()) cVtx.ಗಣ_Do_Inline("should inline");
        // If --flatten, inline it
        ಆದರೆ (v3Global.opt.flatten()) cVtx.ಗಣ_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        ಆದರೆ (mVtx.noInlineSoft()) ಮುಂದುವರಿ;

        // If instatiated in exactly one static site, inline it
        ಆದರೆ (mVtx.ಒಳ_Size1()) cVtx.ಗಣ_Do_Inline("Single static instance");
        // If small, inline it
        ಆದರೆ (mVtx.ಗಾತ್ರ_2() < INLINE_MODS_SMALLER) cVtx.ಗಣ_Do_Inline("Small");
        // If inlineMult is 0, inline it
        ಆದರೆ (v3Global.opt.inlineMult() < 1) cVtx.ಗಣ_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t inlined_ಗಾತ್ರ = mVtx.ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2() * mVtx.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಎಲ್ಲೆ = v3Global.opt.inlineMult();
        ಆದರೆ (inlined_ಗಾತ್ರ < ಎಲ್ಲೆ) cVtx.ಗಣ_Do_Inline("inlinedSize < inlineMult");
    }
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_ವೃಕ್ಷ("inline", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
