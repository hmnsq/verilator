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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Inline.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
ساكن ثابت صحيح INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

صنف Inline_Mod_Module_رأس;
صنف Inline_Mod_Cell_رأس;

صنف Inline_Mod_گراف final : عام V3_گراف {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_درون_Use m_user4_درون_Use;

عام:
    Inline_Mod_گراف()
        : V3_گراف{} {}
    ~Inline_Mod_گراف() override = افتراضي;

    // METHODS
    Inline_Mod_Module_رأس* getInlineModModuleVertexp(Ast_گره_Module* modp);
    Inline_Mod_Cell_رأس* getInlineModCellVertexp(AstCell* cellp);
    فراغ جمع_کن_یال(Inline_Mod_Module_رأس& from, Inline_Mod_Cell_رأس& مقصد);
    فراغ جمع_کن_یال(Inline_Mod_Cell_رأس& from, Inline_Mod_Module_رأس& مقصد);

    // debug
    std::string dotRankDir() ثابت override { أرجع "LR"; }
};

صنف Inline_Mod_Either_رأس VL_NOT_FINAL : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Inline_Mod_Either_رأس, V3_گراف_رأس)
محمي:
    صریح Inline_Mod_Either_رأس(Inline_Mod_گراف& گراف)
        : V3_گراف_رأس{&گراف} {}
};

صنف Inline_Mod_Module_رأس final : عام Inline_Mod_Either_رأس {
    VL_RTTI_IMPL(Inline_Mod_Module_رأس, Inline_Mod_Either_رأس)
    Ast_گره_Module* ثابت m_modp;  // The module
    ثابت حرف* m_noInlineHardWyp = اشاره_تهی;  // First reason the module can never be inlined
    ثابت حرف* m_noInlineSoftWyp = اشاره_تهی;  // First reason not to inline unless forced
    ثابت حرف* m_shouldInlineWhyp = اشاره_تهی;  // First reason why this module should be inlined
    اندازه_t m_اندازه = 0;  // The size (statement count) of the module
    اندازه_t تغییرپذیر m_flattened_اندازه = 0;  // The size of the module if flattened
    منطقي تغییرپذیر m_flattened_اندازه_Valid = خطأ;  // Whether the flattened size is valid
    اندازه_t تغییرپذیر m_instance_شمار = 0;  // The number of total instances of this module
    منطقي تغییرپذیر m_instance_شمار_Valid = خطأ;  // Whether the instance count is valid

عام:
    Inline_Mod_Module_رأس(Inline_Mod_گراف& گراف, Ast_گره_Module* modp)
        : Inline_Mod_Either_رأس{گراف}
        , m_modp{modp} {}
    ~Inline_Mod_Module_رأس() override = افتراضي;

    // ACCESSORS
    Ast_گره_Module* modp() ثابت { أرجع m_modp; }
    اندازه_t اندازه() ثابت { أرجع m_اندازه; }
    فراغ اندازه(اندازه_t مقدار) { m_اندازه = مقدار; }
    فراغ اندازه_Inc(اندازه_t مقدار = 1) { m_اندازه += مقدار; }
    منطقي noInlineHard() ثابت { أرجع m_noInlineHardWyp; }
    فراغ مجموعه_No_Inline_Hard(ثابت حرف* whyp) {
        إذا (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    منطقي noInlineSoft() ثابت { أرجع m_noInlineSoftWyp; }
    فراغ مجموعه_No_Inline_Soft(ثابت حرف* whyp) {
        إذا (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    منطقي shouldInline() ثابت { أرجع m_shouldInlineWhyp; }
    فراغ مجموعه_Should_Inline(ثابت حرف* whyp) {
        إذا (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    فراغ مجموعه_Flatten();

    // Total size of module, with all hierarchy below flattened
    اندازه_t flattened_اندازه() ثابت {
        إذا (!m_flattened_اندازه_Valid) {
            m_flattened_اندازه_Valid = درست;
            m_flattened_اندازه = m_اندازه;
            لكل (ثابت V3_گراف_یال& e1 : بیرون_یالها()) {
                لكل (ثابت V3_گراف_یال& e2 : e1.بالاترین()->بیرون_یالها()) {
                    Inline_Mod_Module_رأس* ثابت mVtxp = e2.بالاترین()->as<Inline_Mod_Module_رأس>();
                    m_flattened_اندازه += mVtxp->flattened_اندازه();
                }
            }
        }
        أرجع m_flattened_اندازه;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    اندازه_t instance_شمار() ثابت {
        إذا (!m_instance_شمار_Valid) {
            m_instance_شمار_Valid = درست;
            m_instance_شمار = 0;
            لكل (ثابت V3_گراف_یال& e1 : درون_یالها()) {
                لكل (ثابت V3_گراف_یال& e2 : e1.fromp()->درون_یالها()) {
                    Inline_Mod_Module_رأس* ثابت mVtxp = e2.fromp()->as<Inline_Mod_Module_رأس>();
                    m_instance_شمار += mVtxp->instance_شمار();
                }
            }
            إذا (!m_instance_شمار) {
                UASSERT_OBJ(m_modp->is_بالاترین(), m_modp, "non-top level module should have instances");
                m_instance_شمار = 1;
            }
        }
        أرجع m_instance_شمار;
    }

    // debug
    File_سطر* fileline() ثابت override { أرجع m_modp->fileline(); }
    std::string dotShape() ثابت override { أرجع "box"; }
    std::string dotColor() ثابت override {
        أرجع m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_مقصد_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::مقصد_string(instance_شمار());
        str += "\nsize: " + std::مقصد_string(m_اندازه);
        str += "\nflattenedSize: " + std::مقصد_string(flattened_اندازه());
        إذا (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        إذا (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        إذا (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        أرجع str;
    }
};

صنف Inline_Mod_Cell_رأس final : عام Inline_Mod_Either_رأس {
    VL_RTTI_IMPL(Inline_Mod_Cell_رأس, Inline_Mod_Either_رأس)
    AstCell* ثابت m_cellp;  // The cell (instance)
    ثابت حرف* m_doInlineWyp = اشاره_تهی;  // First reason this instance should be inlined
    منطقي m_flatten = خطأ;  // Whether this cell and below already flattened (avoid O(n^2))

عام:
    Inline_Mod_Cell_رأس(Inline_Mod_گراف& گراف, AstCell* cellp)
        : Inline_Mod_Either_رأس{گراف}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_رأس() override = افتراضي;

    // ACCESSORS
    AstCell* cellp() ثابت { أرجع m_cellp; }
    منطقي doInline() ثابت { أرجع m_doInlineWyp; }
    فراغ مجموعه_Do_Inline(ثابت حرف* whyp) {
        إذا (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    منطقي flatten() ثابت { أرجع m_flatten; }
    فراغ مجموعه_Flatten() { m_flatten = درست; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_رأس& instance_از() ثابت {
        UASSERT_OBJ(بیرون_Size1(), هذا, "Cell should have exactly one outgoing edge");
        أرجع *بیرون_یالها().frontp()->بالاترین()->as<Inline_Mod_Module_رأس>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_رأس& instance_درون() ثابت {
        UASSERT_OBJ(درون_Size1(), هذا, "Cell should have exactly one incoming edge");
        أرجع *درون_یالها().frontp()->fromp()->as<Inline_Mod_Module_رأس>();
    }

    // debug
    File_سطر* fileline() ثابت override { أرجع m_cellp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_مقصد_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        إذا (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        أرجع str;
    }
};

Inline_Mod_Module_رأس* Inline_Mod_گراف::getInlineModModuleVertexp(Ast_گره_Module* modp) {
    إذا (!modp->user4p()) modp->user4p(جديد Inline_Mod_Module_رأس{*هذا, modp});
    أرجع modp->user4u().مقصد<Inline_Mod_Module_رأس*>();
}
Inline_Mod_Cell_رأس* Inline_Mod_گراف::getInlineModCellVertexp(AstCell* cellp) {
    إذا (!cellp->user4p()) cellp->user4p(جديد Inline_Mod_Cell_رأس{*هذا, cellp});
    أرجع cellp->user4u().مقصد<Inline_Mod_Cell_رأس*>();
}

فراغ Inline_Mod_گراف::جمع_کن_یال(Inline_Mod_Module_رأس& والد, Inline_Mod_Cell_رأس& cell) {
    UASSERT_OBJ(cell.درون_خالی(), &cell, "Cell should have at most one incoming edge");
    جديد V3_گراف_یال{هذا, &والد, &cell, 1, /* cutable: */ خطأ};
}
فراغ Inline_Mod_گراف::جمع_کن_یال(Inline_Mod_Cell_رأس& cell, Inline_Mod_Module_رأس& submodule) {
    UASSERT_OBJ(cell.بیرون_خالی(), &cell, "Cell should have at most one outgoing edge");
    جديد V3_گراف_یال{هذا, &cell, &submodule, 1, /* cutable: */ خطأ};
}

فراغ Inline_Mod_Module_رأس::مجموعه_Flatten() {
    لكل (V3_گراف_یال& یال : بیرون_یالها()) {
        Inline_Mod_Cell_رأس& cVtx = *یال.بالاترین()->as<Inline_Mod_Cell_رأس>();
        إذا (cVtx.flatten()) استمر;
        cVtx.مجموعه_Flatten();
        Inline_Mod_Module_رأس& iVtx = cVtx.instance_از();
        إذا (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.مجموعه_Do_Inline("flatten parent");
        iVtx.مجموعه_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

صنف Inline_Mod_گراف_Builder final : عام VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_گراف> m_graphp{جديد Inline_Mod_گراف};  // The graph being built
    Inline_Mod_Module_رأس* m_modVtxp = اشاره_تهی;  // Vertex of module currently being iterated
    ثابت AstScope* m_sizedScopep = اشاره_تهی;  // The scope of current module measured for size

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_رأس* ثابت vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        إذا (VN_IS(nodep, Iface)) vtxp->مجموعه_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        إذا (nodep->isConstPool()) vtxp->مجموعه_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        إذا (VN_IS(nodep, Package)) vtxp->مجموعه_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        إذا (nodep->verilatorLib()) vtxp->مجموعه_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        إذا (nodep->mod_عمومی()) vtxp->مجموعه_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = اشاره_تهی;
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        إذا (m_modVtxp) m_modVtxp->مجموعه_No_Inline_Hard("Contains class");
    }

    فراغ بازدید(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        إذا (!m_sizedScopep) m_sizedScopep = nodep;
        إذا (m_sizedScopep != nodep) أرجع;
        إذا (m_modVtxp) m_modVtxp->اندازه_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    فراغ بازدید(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_رأس* ثابت vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->جمع_کن_یال(*m_modVtxp, *vtxp);
        m_graphp->جمع_کن_یال(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->مجموعه_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }

        إذا (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->مجموعه_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (m_modVtxp) m_modVtxp->اندازه_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    صریح Inline_Mod_گراف_Builder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->ترتیب();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_رأس>()->modp()->is_بالاترین(),
                    nodep, "First vertex should be top level module");
#اگر_تعریف_شده VL_DEBUG
        لكل (ثابت V3_گراف_رأس& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            إذا (&vtx == m_graphp->vertices().frontp()) استمر;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.درون_خالی(), &vtx, "Should have edges from root");
        }
#پایان_اگر
    }
    ~Inline_Mod_گراف_Builder() override = افتراضي;

عام:
    ساكن std::unique_ptr<Inline_Mod_گراف> apply(AstNetlist* nodep) {
        أرجع std::move(Inline_Mod_گراف_Builder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

نطاق ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
استفاده ScopeChildren = std::unordered_map<ثابت AstScope*, std::وکتور<AstScope*>>;

// Record downward links from parent scopes to their child scopes
فراغ gatherScopes(ثابت Ast_گره_Module* modp, ScopeChildren& children) {
    لكل (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].افزودن_پشت(scopep);
        } وإلا إذا (ثابت Ast_گره_Module* ثابت subModp = VN_CAST(nodep, گره_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
فراغ renameScopes(AstScope* scopep, ثابت std::string& old_پیشوند, ثابت std::string& new_پیشوند,
                  ثابت ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_پیشوند), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_پیشوند + scopep->name().substr(old_پیشوند.اندازه()));
    ثابت خودکار it = children.پیدا_کن(scopep);
    إذا (it == children.انتها()) أرجع;
    لكل (AstScope* ثابت childp : it->دومی) {
        renameScopes(childp, old_پیشوند, new_پیشوند, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
فراغ inlineScope(AstScope* scopep, AstCell* cellp, ثابت std::string& پیشوند,
                 ScopeChildren& children) {
    AstScope* ثابت والد_Scopep = scopep->aboveScopep();
    UASSERT_OBJ(والد_Scopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << والد_Scopep);

    // Move the variables of the inlined scope into the scope above
    لكل (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        إذا (vscp->scopep() == scopep) vscp->scopep(والد_Scopep);
        // If the module was cloned, point to the cloned variable
        إذا (AstVar* ثابت newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        إذا (!cellp->isTrace()) vscp->trace(خطأ);
        والد_Scopep->جمع_کن_Varsp(vscp->unlink_Fr_پشت());
    }

    // Move the logic of the inlined scope into the scope above
    لكل (Ast_گره *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_پشت();
        إذا (Ast_گره_FTask* ثابت ftaskp = VN_CAST(nodep, گره_FTask)) {
            ftaskp->name(پیشوند + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        إذا (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_گره_Cover_Decl* ثابت declp = incp->declp();
                إذا (declp->در_هر_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    إذا (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    أرجع;
                }
                إذا (Ast_گره_Cover_Decl* ثابت newDeclp = VN_CAST(declp->user3p(), گره_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        والد_Scopep->جمع_کن_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    لكل (Ast_گره *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ثابت cisp = VN_AS(nodep->unlink_Fr_پشت(), CellInlineScope);
        cisp->name(پیشوند + cisp->name());
        والد_Scopep->جمع_کن_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    إذا (v3Global.opt.vpi()) {
        والد_Scopep->جمع_کن_Inlinesp(
            جديد AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ثابت std::string old_پیشوند = scopep->name() + ".";
    ثابت std::string new_پیشوند = scopep->name() + "__DOT__";
    std::وکتور<AstScope*> فرزند_Scopeps;
    {
        ثابت خودکار it = children.پیدا_کن(scopep);
        إذا (it != children.انتها()) {
            فرزند_Scopeps = std::move(it->دومی);
            children.erase(it);
        }
    }
    لكل (AstScope* ثابت فرزند_Scopep : فرزند_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(فرزند_Scopep->modp(), Class), فرزند_Scopep,
                    "Inlined scope should not contain a class scope");
        إذا (AstCell* ثابت newCellp = VN_CAST(فرزند_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            فرزند_Scopep->aboveCellp(newCellp);
        }
        فرزند_Scopep->aboveScopep(والد_Scopep);
        renameScopes(فرزند_Scopep, old_پیشوند, new_پیشوند, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::وکتور<AstScope*>& والد_Childps = children[والد_Scopep];
    والد_Childps.erase(std::remove(والد_Childps.begin(), والد_Childps.انتها(), scopep),
                        والد_Childps.انتها());
    والد_Childps.درج(والد_Childps.انتها(), فرزند_Scopeps.begin(), فرزند_Scopeps.انتها());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
فراغ inlineCell(Ast_گره_Module* modp, AstCell* cellp, منطقي آخرین, Inline_Mod_گراف& گراف,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ثابت VNUser3_درون_Use user3_درون_Use;

    VNDeleter deleter;
    deleter.افزودن_Deletep(cellp->unlink_Fr_پشت());

    Ast_گره_Module* ثابت subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::وکتور<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::وکتور<AstScope*> otherScopeps;  // Scopes under some other instance
    لكل (Ast_گره *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ثابت scopep = VN_CAST(nodep, Scope);
        إذا (!scopep) استمر;
        scopep->unlink_Fr_پشت();
        إذا (scopep->aboveCellp() == cellp) {
            inlineScopeps.افزودن_پشت(scopep);
            deleter.افزودن_Deletep(scopep);
        } وإلا {
            otherScopeps.افزودن_پشت(scopep);
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
    Ast_گره_Module* inlinedp;
    إذا (آخرین) {
        inlinedp = subModp->unlink_Fr_پشت();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.خالی(), cellp, "Last instance, but has other scopes");
    } وإلا {
        inlinedp = subModp->clone_درخت(خطأ);
        // Compute map from the original module items to their clones
        لكل (Ast_گره *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        إذا (v3Global.opt.coverageFsm()) {
            لكل (Ast_گره *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ثابت Ast_گره_Cover_Decl* ثابت declp = VN_CAST(nodep, گره_Cover_Decl);
                إذا (declp && declp->در_هر_Instance()) {
                    VL_DO_DANGLING(deleter.افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        لكل (AstScope* ثابت scopep : otherScopeps) subModp->جمع_کن_Stmtsp(scopep);
    }
    deleter.افزودن_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ثابت std::string پیشوند = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    إذا (AstSystemCSection* ثابت sectionsp = inlinedp->scSectionsp()) {
        modp->جمع_کن_Sc_Sectionsp(sectionsp->unlink_Fr_پشت_With_بعدی());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_رأس* ثابت mVtxp = گراف.getInlineModModuleVertexp(modp);
    لكل (Ast_گره *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_پشت();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            varp->name(پیشوند + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            إذا (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            إذا (!cellp->isTrace()) varp->trace(خطأ);
        } وإلا إذا (AstCell* ثابت subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(پیشوند + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(اشاره_تهی);
            Inline_Mod_Cell_رأس* ثابت vtxp = گراف.getInlineModCellVertexp(subCellp);
            گراف.جمع_کن_یال(*mVtxp, *vtxp);
            گراف.جمع_کن_یال(*vtxp, *گراف.getInlineModModuleVertexp(subCellp->modp()));
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(پیشوند + typedefp->name());
        } وإلا إذا (Ast_گره_Cover_Decl* ثابت declp = VN_CAST(nodep, گره_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            إذا (!declp->در_هر_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->جمع_کن_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    لكل (AstScope* ثابت scopep : inlineScopeps) inlineScope(scopep, cellp, پیشوند, children);
}

// Apply all inlining decisions
فراغ process(AstNetlist* netlistp, Inline_Mod_گراف& گراف) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    لكل (Ast_گره_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گره_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::وکتور<Inline_Mod_Cell_رأس*> cVtxps;
    لكل (V3_گراف_رأس& vtx : گراف.vertices()) {
        Inline_Mod_Cell_رأس* ثابت cVtxp = vtx.cast<Inline_Mod_Cell_رأس>();
        إذا (!cVtxp) استمر;
        إذا (!cVtxp->doInline()) استمر;
        cVtxps.افزودن_پشت(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    لكل (Inline_Mod_Cell_رأس* ثابت cVtxp : vlstd::معکوس_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_رأس& mVtx = cVtxp->instance_درون();
        Inline_Mod_Module_رأس* ثابت iVtxp = &cVtxp->instance_از();
        AstCell* ثابت cellp = cVtxp->cellp();
        ثابت منطقي آخرین = iVtxp->درون_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.اندازه_Inc(iVtxp->اندازه());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&گراف), cVtxp);
        // Delete the module we are inlining if this is the last instance
        إذا (آخرین) {
            طالما (!iVtxp->بیرون_خالی()) {
                Inline_Mod_Cell_رأس* ثابت tVtxp
                    = iVtxp->بیرون_یالها().frontp()->بالاترین()->as<Inline_Mod_Cell_رأس>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&گراف), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&گراف), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, آخرین, گراف, children);
        إذا (dump_گراف_Level() >= 9) گراف.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_گره_Var_Ref* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::جمع_کن_Stat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

فراغ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<Inline_Mod_گراف> graphp = Inline_Mod_گراف_Builder::apply(nodep);
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ثابت اندازه_t design_اندازه
        = graphp->vertices().frontp()->as<Inline_Mod_Module_رأس>()->flattened_اندازه();
    لكل (V3_گراف_رأس& vtx : graphp->vertices()) {
        إذا (Inline_Mod_Module_رأس* ثابت mVtxp = vtx.cast<Inline_Mod_Module_رأس>()) {
            // If this module is less than 10% of the design, flatten this module
            إذا (mVtxp->flattened_اندازه() * 10 < design_اندازه) mVtxp->مجموعه_Flatten();
            // Don't inline if can't inline
            إذا (mVtxp->noInlineHard()) استمر;
            // Don't inline if soft off
            إذا (mVtxp->noInlineSoft()) استمر;
            // If all instances of this module combined are less than 20% of the design, inline all
            اندازه_t مجموع_اندازه = mVtxp->flattened_اندازه() * mVtxp->instance_شمار();
            إذا (مجموع_اندازه * 5 < design_اندازه) {
                لكل (V3_گراف_یال& یال : mVtxp->درون_یالها()) {
                    Inline_Mod_Cell_رأس* ثابت cVtxp = یال.fromp()->as<Inline_Mod_Cell_رأس>();
                    cVtxp->مجموعه_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            استمر;
        }

        // The instantiation
        Inline_Mod_Cell_رأس& cVtx = *vtx.as<Inline_Mod_Cell_رأس>();
        // The module instantiated by this cell
        Inline_Mod_Module_رأس& mVtx = cVtx.instance_از();

        // Don't inline if can't inline, duh!
        إذا (mVtx.noInlineHard()) استمر;

        // If it should be inlined, inlined it
        إذا (mVtx.shouldInline()) cVtx.مجموعه_Do_Inline("should inline");
        // If --flatten, inline it
        إذا (v3Global.opt.flatten()) cVtx.مجموعه_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        إذا (mVtx.noInlineSoft()) استمر;

        // If instatiated in exactly one static site, inline it
        إذا (mVtx.درون_Size1()) cVtx.مجموعه_Do_Inline("Single static instance");
        // If small, inline it
        إذا (mVtx.اندازه() < INLINE_MODS_SMALLER) cVtx.مجموعه_Do_Inline("Small");
        // If inlineMult is 0, inline it
        إذا (v3Global.opt.inlineMult() < 1) cVtx.مجموعه_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ثابت اندازه_t inlined_اندازه = mVtx.درون_یالها().اندازه() * mVtx.اندازه();
        ثابت اندازه_t حد = v3Global.opt.inlineMult();
        إذا (inlined_اندازه < حد) cVtx.مجموعه_Do_Inline("inlinedSize < inlineMult");
    }
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_درخت("inline", 0, dump_درخت_Either_Level() >= 3);
}
