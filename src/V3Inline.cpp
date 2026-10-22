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

صنف Inline_Mod_Module_راس;
صنف Inline_Mod_Cell_راس;

صنف Inline_Mod_گراف final : عام V3_گراف {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_اندريون_Use m_user4_اندريون_Use;

عام:
    Inline_Mod_گراف()
        : V3_گراف{} {}
    ~Inline_Mod_گراف() override = افتراضي;

    // METHODS
    Inline_Mod_Module_راس* getInlineModModuleVertexp(Ast_ڳنڍ_Module* modp);
    Inline_Mod_Cell_راس* getInlineModCellVertexp(AstCell* cellp);
    فراغ جوڙيو_ڪنارو(Inline_Mod_Module_راس& from, Inline_Mod_Cell_راس& منزل);
    فراغ جوڙيو_ڪنارو(Inline_Mod_Cell_راس& from, Inline_Mod_Module_راس& منزل);

    // debug
    std::string dotRankDir() ثابت override { أرجع "LR"; }
};

صنف Inline_Mod_Either_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(Inline_Mod_Either_راس, V3_گراف_راس)
محمي:
    واضح Inline_Mod_Either_راس(Inline_Mod_گراف& گراف)
        : V3_گراف_راس{&گراف} {}
};

صنف Inline_Mod_Module_راس final : عام Inline_Mod_Either_راس {
    VL_RTTI_IMPL(Inline_Mod_Module_راس, Inline_Mod_Either_راس)
    Ast_ڳنڍ_Module* ثابت m_modp;  // The module
    ثابت حرف* m_noInlineHardWyp = خالي_اشارو;  // First reason the module can never be inlined
    ثابت حرف* m_noInlineSoftWyp = خالي_اشارو;  // First reason not to inline unless forced
    ثابت حرف* m_shouldInlineWhyp = خالي_اشارو;  // First reason why this module should be inlined
    سائيز_t m_سائيز = 0;  // The size (statement count) of the module
    سائيز_t بدلجندڙ m_flattened_سائيز = 0;  // The size of the module if flattened
    منطقي بدلجندڙ m_flattened_سائيز_Valid = خطأ;  // Whether the flattened size is valid
    سائيز_t بدلجندڙ m_instance_ڳڻپ = 0;  // The number of total instances of this module
    منطقي بدلجندڙ m_instance_ڳڻپ_Valid = خطأ;  // Whether the instance count is valid

عام:
    Inline_Mod_Module_راس(Inline_Mod_گراف& گراف, Ast_ڳنڍ_Module* modp)
        : Inline_Mod_Either_راس{گراف}
        , m_modp{modp} {}
    ~Inline_Mod_Module_راس() override = افتراضي;

    // ACCESSORS
    Ast_ڳنڍ_Module* modp() ثابت { أرجع m_modp; }
    سائيز_t سائيز() ثابت { أرجع m_سائيز; }
    فراغ سائيز(سائيز_t قدر) { m_سائيز = قدر; }
    فراغ سائيز_Inc(سائيز_t قدر = 1) { m_سائيز += قدر; }
    منطقي noInlineHard() ثابت { أرجع m_noInlineHardWyp; }
    فراغ سيٽ_No_Inline_Hard(ثابت حرف* whyp) {
        إذا (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    منطقي noInlineSoft() ثابت { أرجع m_noInlineSoftWyp; }
    فراغ سيٽ_No_Inline_Soft(ثابت حرف* whyp) {
        إذا (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    منطقي shouldInline() ثابت { أرجع m_shouldInlineWhyp; }
    فراغ سيٽ_Should_Inline(ثابت حرف* whyp) {
        إذا (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    فراغ سيٽ_Flatten();

    // Total size of module, with all hierarchy below flattened
    سائيز_t flattened_سائيز() ثابت {
        إذا (!m_flattened_سائيز_Valid) {
            m_flattened_سائيز_Valid = سچ;
            m_flattened_سائيز = m_سائيز;
            لكل (ثابت V3_گراف_ڪنارو& e1 : ٻاهريون_ڪنارا()) {
                لكل (ثابت V3_گراف_ڪنارو& e2 : e1.چوٽي()->ٻاهريون_ڪنارا()) {
                    Inline_Mod_Module_راس* ثابت mVtxp = e2.چوٽي()->as<Inline_Mod_Module_راس>();
                    m_flattened_سائيز += mVtxp->flattened_سائيز();
                }
            }
        }
        أرجع m_flattened_سائيز;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    سائيز_t instance_ڳڻپ() ثابت {
        إذا (!m_instance_ڳڻپ_Valid) {
            m_instance_ڳڻپ_Valid = سچ;
            m_instance_ڳڻپ = 0;
            لكل (ثابت V3_گراف_ڪنارو& e1 : اندريون_ڪنارا()) {
                لكل (ثابت V3_گراف_ڪنارو& e2 : e1.fromp()->اندريون_ڪنارا()) {
                    Inline_Mod_Module_راس* ثابت mVtxp = e2.fromp()->as<Inline_Mod_Module_راس>();
                    m_instance_ڳڻپ += mVtxp->instance_ڳڻپ();
                }
            }
            إذا (!m_instance_ڳڻپ) {
                UASSERT_OBJ(m_modp->is_چوٽي(), m_modp, "non-top level module should have instances");
                m_instance_ڳڻپ = 1;
            }
        }
        أرجع m_instance_ڳڻپ;
    }

    // debug
    File_سٽ* fileline() ثابت override { أرجع m_modp->fileline(); }
    std::string dotShape() ثابت override { أرجع "box"; }
    std::string dotColor() ثابت override {
        أرجع m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_منزل_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::منزل_string(instance_ڳڻپ());
        str += "\nsize: " + std::منزل_string(m_سائيز);
        str += "\nflattenedSize: " + std::منزل_string(flattened_سائيز());
        إذا (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        إذا (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        إذا (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        أرجع str;
    }
};

صنف Inline_Mod_Cell_راس final : عام Inline_Mod_Either_راس {
    VL_RTTI_IMPL(Inline_Mod_Cell_راس, Inline_Mod_Either_راس)
    AstCell* ثابت m_cellp;  // The cell (instance)
    ثابت حرف* m_doInlineWyp = خالي_اشارو;  // First reason this instance should be inlined
    منطقي m_flatten = خطأ;  // Whether this cell and below already flattened (avoid O(n^2))

عام:
    Inline_Mod_Cell_راس(Inline_Mod_گراف& گراف, AstCell* cellp)
        : Inline_Mod_Either_راس{گراف}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_راس() override = افتراضي;

    // ACCESSORS
    AstCell* cellp() ثابت { أرجع m_cellp; }
    منطقي doInline() ثابت { أرجع m_doInlineWyp; }
    فراغ سيٽ_Do_Inline(ثابت حرف* whyp) {
        إذا (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    منطقي flatten() ثابت { أرجع m_flatten; }
    فراغ سيٽ_Flatten() { m_flatten = سچ; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_راس& instance_جو() ثابت {
        UASSERT_OBJ(ٻاهريون_Size1(), هذا, "Cell should have exactly one outgoing edge");
        أرجع *ٻاهريون_ڪنارا().frontp()->چوٽي()->as<Inline_Mod_Module_راس>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_راس& instance_اندريون() ثابت {
        UASSERT_OBJ(اندريون_Size1(), هذا, "Cell should have exactly one incoming edge");
        أرجع *اندريون_ڪنارا().frontp()->fromp()->as<Inline_Mod_Module_راس>();
    }

    // debug
    File_سٽ* fileline() ثابت override { أرجع m_cellp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_منزل_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        إذا (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        أرجع str;
    }
};

Inline_Mod_Module_راس* Inline_Mod_گراف::getInlineModModuleVertexp(Ast_ڳنڍ_Module* modp) {
    إذا (!modp->user4p()) modp->user4p(جديد Inline_Mod_Module_راس{*هذا, modp});
    أرجع modp->user4u().منزل<Inline_Mod_Module_راس*>();
}
Inline_Mod_Cell_راس* Inline_Mod_گراف::getInlineModCellVertexp(AstCell* cellp) {
    إذا (!cellp->user4p()) cellp->user4p(جديد Inline_Mod_Cell_راس{*هذا, cellp});
    أرجع cellp->user4u().منزل<Inline_Mod_Cell_راس*>();
}

فراغ Inline_Mod_گراف::جوڙيو_ڪنارو(Inline_Mod_Module_راس& والد, Inline_Mod_Cell_راس& cell) {
    UASSERT_OBJ(cell.اندريون_خالي_جايون(), &cell, "Cell should have at most one incoming edge");
    جديد V3_گراف_ڪنارو{هذا, &والد, &cell, 1, /* cutable: */ خطأ};
}
فراغ Inline_Mod_گراف::جوڙيو_ڪنارو(Inline_Mod_Cell_راس& cell, Inline_Mod_Module_راس& submodule) {
    UASSERT_OBJ(cell.ٻاهريون_خالي_جايون(), &cell, "Cell should have at most one outgoing edge");
    جديد V3_گراف_ڪنارو{هذا, &cell, &submodule, 1, /* cutable: */ خطأ};
}

فراغ Inline_Mod_Module_راس::سيٽ_Flatten() {
    لكل (V3_گراف_ڪنارو& ڪنارو : ٻاهريون_ڪنارا()) {
        Inline_Mod_Cell_راس& cVtx = *ڪنارو.چوٽي()->as<Inline_Mod_Cell_راس>();
        إذا (cVtx.flatten()) استمر;
        cVtx.سيٽ_Flatten();
        Inline_Mod_Module_راس& iVtx = cVtx.instance_جو();
        إذا (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.سيٽ_Do_Inline("flatten parent");
        iVtx.سيٽ_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

صنف Inline_Mod_گراف_Builder final : عام VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_گراف> m_graphp{جديد Inline_Mod_گراف};  // The graph being built
    Inline_Mod_Module_راس* m_modVtxp = خالي_اشارو;  // Vertex of module currently being iterated
    ثابت AstScope* m_sizedScopep = خالي_اشارو;  // The scope of current module measured for size

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_راس* ثابت vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        إذا (VN_IS(nodep, Iface)) vtxp->سيٽ_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        إذا (nodep->isConstPool()) vtxp->سيٽ_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        إذا (VN_IS(nodep, Package)) vtxp->سيٽ_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        إذا (nodep->verilatorLib()) vtxp->سيٽ_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        إذا (nodep->mod_عوامي()) vtxp->سيٽ_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = خالي_اشارو;
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        إذا (m_modVtxp) m_modVtxp->سيٽ_No_Inline_Hard("Contains class");
    }

    فراغ گهمو(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        إذا (!m_sizedScopep) m_sizedScopep = nodep;
        إذا (m_sizedScopep != nodep) أرجع;
        إذا (m_modVtxp) m_modVtxp->سائيز_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    فراغ گهمو(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_راس* ثابت vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->جوڙيو_ڪنارو(*m_modVtxp, *vtxp);
        m_graphp->جوڙيو_ڪنارو(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->سيٽ_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }

        إذا (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->سيٽ_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (m_modVtxp) m_modVtxp->سائيز_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    واضح Inline_Mod_گراف_Builder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->ترتيب();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_راس>()->modp()->is_چوٽي(),
                    nodep, "First vertex should be top level module");
#جيڪڏهن_وصف_ٿيل VL_DEBUG
        لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            إذا (&vtx == m_graphp->vertices().frontp()) استمر;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.اندريون_خالي_جايون(), &vtx, "Should have edges from root");
        }
#جيڪڏهن_پورو
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
استعمال ScopeChildren = std::unordered_map<ثابت AstScope*, std::ويڪٽر<AstScope*>>;

// Record downward links from parent scopes to their child scopes
فراغ gatherScopes(ثابت Ast_ڳنڍ_Module* modp, ScopeChildren& children) {
    لكل (Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ڌڪو_پٺ(scopep);
        } وإلا إذا (ثابت Ast_ڳنڍ_Module* ثابت subModp = VN_CAST(nodep, ڳنڍ_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
فراغ renameScopes(AstScope* scopep, ثابت std::string& old_اڳياڙي, ثابت std::string& new_اڳياڙي,
                  ثابت ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_اڳياڙي), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_اڳياڙي + scopep->name().substr(old_اڳياڙي.سائيز()));
    ثابت خودڪار it = children.ڳوليو(scopep);
    إذا (it == children.پڇاڙي()) أرجع;
    لكل (AstScope* ثابت childp : it->ٻيو) {
        renameScopes(childp, old_اڳياڙي, new_اڳياڙي, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
فراغ inlineScope(AstScope* scopep, AstCell* cellp, ثابت std::string& اڳياڙي,
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
        والد_Scopep->جوڙيو_Varsp(vscp->unlink_Fr_پٺ());
    }

    // Move the logic of the inlined scope into the scope above
    لكل (Ast_ڳنڍ *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_پٺ();
        إذا (Ast_ڳنڍ_FTask* ثابت ftaskp = VN_CAST(nodep, ڳنڍ_FTask)) {
            ftaskp->name(اڳياڙي + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        إذا (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_ڳنڍ_Cover_Decl* ثابت declp = incp->declp();
                إذا (declp->في_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    إذا (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    أرجع;
                }
                إذا (Ast_ڳنڍ_Cover_Decl* ثابت newDeclp = VN_CAST(declp->user3p(), ڳنڍ_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        والد_Scopep->جوڙيو_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    لكل (Ast_ڳنڍ *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ثابت cisp = VN_AS(nodep->unlink_Fr_پٺ(), CellInlineScope);
        cisp->name(اڳياڙي + cisp->name());
        والد_Scopep->جوڙيو_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    إذا (v3Global.opt.vpi()) {
        والد_Scopep->جوڙيو_Inlinesp(
            جديد AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ثابت std::string old_اڳياڙي = scopep->name() + ".";
    ثابت std::string new_اڳياڙي = scopep->name() + "__DOT__";
    std::ويڪٽر<AstScope*> ٻار_Scopeps;
    {
        ثابت خودڪار it = children.ڳوليو(scopep);
        إذا (it != children.پڇاڙي()) {
            ٻار_Scopeps = std::move(it->ٻيو);
            children.erase(it);
        }
    }
    لكل (AstScope* ثابت ٻار_Scopep : ٻار_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(ٻار_Scopep->modp(), Class), ٻار_Scopep,
                    "Inlined scope should not contain a class scope");
        إذا (AstCell* ثابت newCellp = VN_CAST(ٻار_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            ٻار_Scopep->aboveCellp(newCellp);
        }
        ٻار_Scopep->aboveScopep(والد_Scopep);
        renameScopes(ٻار_Scopep, old_اڳياڙي, new_اڳياڙي, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::ويڪٽر<AstScope*>& والد_Childps = children[والد_Scopep];
    والد_Childps.erase(std::remove(والد_Childps.begin(), والد_Childps.پڇاڙي(), scopep),
                        والد_Childps.پڇاڙي());
    والد_Childps.وجهو(والد_Childps.پڇاڙي(), ٻار_Scopeps.begin(), ٻار_Scopeps.پڇاڙي());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
فراغ inlineCell(Ast_ڳنڍ_Module* modp, AstCell* cellp, منطقي آخري, Inline_Mod_گراف& گراف,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ثابت VNUser3_اندريون_Use user3_اندريون_Use;

    VNDeleter deleter;
    deleter.ڌڪو_Deletep(cellp->unlink_Fr_پٺ());

    Ast_ڳنڍ_Module* ثابت subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::ويڪٽر<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::ويڪٽر<AstScope*> otherScopeps;  // Scopes under some other instance
    لكل (Ast_ڳنڍ *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ثابت scopep = VN_CAST(nodep, Scope);
        إذا (!scopep) استمر;
        scopep->unlink_Fr_پٺ();
        إذا (scopep->aboveCellp() == cellp) {
            inlineScopeps.ڌڪو_پٺ(scopep);
            deleter.ڌڪو_Deletep(scopep);
        } وإلا {
            otherScopeps.ڌڪو_پٺ(scopep);
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
    Ast_ڳنڍ_Module* inlinedp;
    إذا (آخري) {
        inlinedp = subModp->unlink_Fr_پٺ();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.خالي_جايون(), cellp, "Last instance, but has other scopes");
    } وإلا {
        inlinedp = subModp->clone_وڻ(خطأ);
        // Compute map from the original module items to their clones
        لكل (Ast_ڳنڍ *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        إذا (v3Global.opt.coverageFsm()) {
            لكل (Ast_ڳنڍ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ثابت Ast_ڳنڍ_Cover_Decl* ثابت declp = VN_CAST(nodep, ڳنڍ_Cover_Decl);
                إذا (declp && declp->في_Instance()) {
                    VL_DO_DANGLING(deleter.ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        لكل (AstScope* ثابت scopep : otherScopeps) subModp->جوڙيو_Stmtsp(scopep);
    }
    deleter.ڌڪو_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ثابت std::string اڳياڙي = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    إذا (AstSystemCSection* ثابت sectionsp = inlinedp->scSectionsp()) {
        modp->جوڙيو_Sc_Sectionsp(sectionsp->unlink_Fr_پٺ_With_ايندڙ());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_راس* ثابت mVtxp = گراف.getInlineModModuleVertexp(modp);
    لكل (Ast_ڳنڍ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_پٺ();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            varp->name(اڳياڙي + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            إذا (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            إذا (!cellp->isTrace()) varp->trace(خطأ);
        } وإلا إذا (AstCell* ثابت subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(اڳياڙي + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(خالي_اشارو);
            Inline_Mod_Cell_راس* ثابت vtxp = گراف.getInlineModCellVertexp(subCellp);
            گراف.جوڙيو_ڪنارو(*mVtxp, *vtxp);
            گراف.جوڙيو_ڪنارو(*vtxp, *گراف.getInlineModModuleVertexp(subCellp->modp()));
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(اڳياڙي + typedefp->name());
        } وإلا إذا (Ast_ڳنڍ_Cover_Decl* ثابت declp = VN_CAST(nodep, ڳنڍ_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            إذا (!declp->في_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->جوڙيو_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    لكل (AstScope* ثابت scopep : inlineScopeps) inlineScope(scopep, cellp, اڳياڙي, children);
}

// Apply all inlining decisions
فراغ process(AstNetlist* netlistp, Inline_Mod_گراف& گراف) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    لكل (Ast_ڳنڍ_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ڳنڍ_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::ويڪٽر<Inline_Mod_Cell_راس*> cVtxps;
    لكل (V3_گراف_راس& vtx : گراف.vertices()) {
        Inline_Mod_Cell_راس* ثابت cVtxp = vtx.cast<Inline_Mod_Cell_راس>();
        إذا (!cVtxp) استمر;
        إذا (!cVtxp->doInline()) استمر;
        cVtxps.ڌڪو_پٺ(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    لكل (Inline_Mod_Cell_راس* ثابت cVtxp : vlstd::ابتو_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_راس& mVtx = cVtxp->instance_اندريون();
        Inline_Mod_Module_راس* ثابت iVtxp = &cVtxp->instance_جو();
        AstCell* ثابت cellp = cVtxp->cellp();
        ثابت منطقي آخري = iVtxp->اندريون_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.سائيز_Inc(iVtxp->سائيز());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&گراف), cVtxp);
        // Delete the module we are inlining if this is the last instance
        إذا (آخري) {
            طالما (!iVtxp->ٻاهريون_خالي_جايون()) {
                Inline_Mod_Cell_راس* ثابت tVtxp
                    = iVtxp->ٻاهريون_ڪنارا().frontp()->چوٽي()->as<Inline_Mod_Cell_راس>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&گراف), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&گراف), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, آخري, گراف, children);
        إذا (dump_گراف_Level() >= 9) گراف.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_ڳنڍ_Var_Ref* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::جوڙيو_Stat("Optimizations, Inlined instances", m_nInlined);
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
    ثابت سائيز_t design_سائيز
        = graphp->vertices().frontp()->as<Inline_Mod_Module_راس>()->flattened_سائيز();
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (Inline_Mod_Module_راس* ثابت mVtxp = vtx.cast<Inline_Mod_Module_راس>()) {
            // If this module is less than 10% of the design, flatten this module
            إذا (mVtxp->flattened_سائيز() * 10 < design_سائيز) mVtxp->سيٽ_Flatten();
            // Don't inline if can't inline
            إذا (mVtxp->noInlineHard()) استمر;
            // Don't inline if soft off
            إذا (mVtxp->noInlineSoft()) استمر;
            // If all instances of this module combined are less than 20% of the design, inline all
            سائيز_t ڪل_سائيز = mVtxp->flattened_سائيز() * mVtxp->instance_ڳڻپ();
            إذا (ڪل_سائيز * 5 < design_سائيز) {
                لكل (V3_گراف_ڪنارو& ڪنارو : mVtxp->اندريون_ڪنارا()) {
                    Inline_Mod_Cell_راس* ثابت cVtxp = ڪنارو.fromp()->as<Inline_Mod_Cell_راس>();
                    cVtxp->سيٽ_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            استمر;
        }

        // The instantiation
        Inline_Mod_Cell_راس& cVtx = *vtx.as<Inline_Mod_Cell_راس>();
        // The module instantiated by this cell
        Inline_Mod_Module_راس& mVtx = cVtx.instance_جو();

        // Don't inline if can't inline, duh!
        إذا (mVtx.noInlineHard()) استمر;

        // If it should be inlined, inlined it
        إذا (mVtx.shouldInline()) cVtx.سيٽ_Do_Inline("should inline");
        // If --flatten, inline it
        إذا (v3Global.opt.flatten()) cVtx.سيٽ_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        إذا (mVtx.noInlineSoft()) استمر;

        // If instatiated in exactly one static site, inline it
        إذا (mVtx.اندريون_Size1()) cVtx.سيٽ_Do_Inline("Single static instance");
        // If small, inline it
        إذا (mVtx.سائيز() < INLINE_MODS_SMALLER) cVtx.سيٽ_Do_Inline("Small");
        // If inlineMult is 0, inline it
        إذا (v3Global.opt.inlineMult() < 1) cVtx.سيٽ_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ثابت سائيز_t inlined_سائيز = mVtx.اندريون_ڪنارا().سائيز() * mVtx.سائيز();
        ثابت سائيز_t حد_بندي = v3Global.opt.inlineMult();
        إذا (inlined_سائيز < حد_بندي) cVtx.سيٽ_Do_Inline("inlinedSize < inlineMult");
    }
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_وڻ("inline", 0, dump_وڻ_Either_Level() >= 3);
}
