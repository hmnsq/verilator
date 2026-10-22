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

صنف Inline_Mod_ګراف final : عام V3_ګراف {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_دننه_Use m_user4_دننه_Use;

عام:
    Inline_Mod_ګراف()
        : V3_ګراف{} {}
    ~Inline_Mod_ګراف() override = افتراضي;

    // METHODS
    Inline_Mod_Module_راس* getInlineModModuleVertexp(Ast_غوټه_Module* modp);
    Inline_Mod_Cell_راس* getInlineModCellVertexp(AstCell* cellp);
    فراغ جمع_کړه_څنډه(Inline_Mod_Module_راس& from, Inline_Mod_Cell_راس& منزل);
    فراغ جمع_کړه_څنډه(Inline_Mod_Cell_راس& from, Inline_Mod_Module_راس& منزل);

    // debug
    std::string dotRankDir() ثابت override { أرجع "LR"; }
};

صنف Inline_Mod_Either_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Inline_Mod_Either_راس, V3_ګراف_راس)
محمي:
    څرګند Inline_Mod_Either_راس(Inline_Mod_ګراف& ګراف)
        : V3_ګراف_راس{&ګراف} {}
};

صنف Inline_Mod_Module_راس final : عام Inline_Mod_Either_راس {
    VL_RTTI_IMPL(Inline_Mod_Module_راس, Inline_Mod_Either_راس)
    Ast_غوټه_Module* ثابت m_modp;  // The module
    ثابت حرف* m_noInlineHardWyp = تش_اشاره;  // First reason the module can never be inlined
    ثابت حرف* m_noInlineSoftWyp = تش_اشاره;  // First reason not to inline unless forced
    ثابت حرف* m_shouldInlineWhyp = تش_اشاره;  // First reason why this module should be inlined
    اندازه_t m_اندازه = 0;  // The size (statement count) of the module
    اندازه_t بدلیدونکی m_flattened_اندازه = 0;  // The size of the module if flattened
    منطقي بدلیدونکی m_flattened_اندازه_Valid = خطأ;  // Whether the flattened size is valid
    اندازه_t بدلیدونکی m_instance_شمېر = 0;  // The number of total instances of this module
    منطقي بدلیدونکی m_instance_شمېر_Valid = خطأ;  // Whether the instance count is valid

عام:
    Inline_Mod_Module_راس(Inline_Mod_ګراف& ګراف, Ast_غوټه_Module* modp)
        : Inline_Mod_Either_راس{ګراف}
        , m_modp{modp} {}
    ~Inline_Mod_Module_راس() override = افتراضي;

    // ACCESSORS
    Ast_غوټه_Module* modp() ثابت { أرجع m_modp; }
    اندازه_t اندازه() ثابت { أرجع m_اندازه; }
    فراغ اندازه(اندازه_t ارزښت) { m_اندازه = ارزښت; }
    فراغ اندازه_Inc(اندازه_t ارزښت = 1) { m_اندازه += ارزښت; }
    منطقي noInlineHard() ثابت { أرجع m_noInlineHardWyp; }
    فراغ سېټ_No_Inline_Hard(ثابت حرف* whyp) {
        إذا (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    منطقي noInlineSoft() ثابت { أرجع m_noInlineSoftWyp; }
    فراغ سېټ_No_Inline_Soft(ثابت حرف* whyp) {
        إذا (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    منطقي shouldInline() ثابت { أرجع m_shouldInlineWhyp; }
    فراغ سېټ_Should_Inline(ثابت حرف* whyp) {
        إذا (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    فراغ سېټ_Flatten();

    // Total size of module, with all hierarchy below flattened
    اندازه_t flattened_اندازه() ثابت {
        إذا (!m_flattened_اندازه_Valid) {
            m_flattened_اندازه_Valid = ریښتیا;
            m_flattened_اندازه = m_اندازه;
            لكل (ثابت V3_ګراف_څنډه& e1 : بهر_څنډې()) {
                لكل (ثابت V3_ګراف_څنډه& e2 : e1.سر()->بهر_څنډې()) {
                    Inline_Mod_Module_راس* ثابت mVtxp = e2.سر()->as<Inline_Mod_Module_راس>();
                    m_flattened_اندازه += mVtxp->flattened_اندازه();
                }
            }
        }
        أرجع m_flattened_اندازه;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    اندازه_t instance_شمېر() ثابت {
        إذا (!m_instance_شمېر_Valid) {
            m_instance_شمېر_Valid = ریښتیا;
            m_instance_شمېر = 0;
            لكل (ثابت V3_ګراف_څنډه& e1 : دننه_څنډې()) {
                لكل (ثابت V3_ګراف_څنډه& e2 : e1.fromp()->دننه_څنډې()) {
                    Inline_Mod_Module_راس* ثابت mVtxp = e2.fromp()->as<Inline_Mod_Module_راس>();
                    m_instance_شمېر += mVtxp->instance_شمېر();
                }
            }
            إذا (!m_instance_شمېر) {
                UASSERT_OBJ(m_modp->is_سر(), m_modp, "non-top level module should have instances");
                m_instance_شمېر = 1;
            }
        }
        أرجع m_instance_شمېر;
    }

    // debug
    File_کرښه* fileline() ثابت override { أرجع m_modp->fileline(); }
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
        str += "\ninstanceCount: " + std::منزل_string(instance_شمېر());
        str += "\nsize: " + std::منزل_string(m_اندازه);
        str += "\nflattenedSize: " + std::منزل_string(flattened_اندازه());
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
    ثابت حرف* m_doInlineWyp = تش_اشاره;  // First reason this instance should be inlined
    منطقي m_flatten = خطأ;  // Whether this cell and below already flattened (avoid O(n^2))

عام:
    Inline_Mod_Cell_راس(Inline_Mod_ګراف& ګراف, AstCell* cellp)
        : Inline_Mod_Either_راس{ګراف}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_راس() override = افتراضي;

    // ACCESSORS
    AstCell* cellp() ثابت { أرجع m_cellp; }
    منطقي doInline() ثابت { أرجع m_doInlineWyp; }
    فراغ سېټ_Do_Inline(ثابت حرف* whyp) {
        إذا (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    منطقي flatten() ثابت { أرجع m_flatten; }
    فراغ سېټ_Flatten() { m_flatten = ریښتیا; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_راس& instance_د() ثابت {
        UASSERT_OBJ(بهر_Size1(), هذا, "Cell should have exactly one outgoing edge");
        أرجع *بهر_څنډې().frontp()->سر()->as<Inline_Mod_Module_راس>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_راس& instance_دننه() ثابت {
        UASSERT_OBJ(دننه_Size1(), هذا, "Cell should have exactly one incoming edge");
        أرجع *دننه_څنډې().frontp()->fromp()->as<Inline_Mod_Module_راس>();
    }

    // debug
    File_کرښه* fileline() ثابت override { أرجع m_cellp->fileline(); }
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

Inline_Mod_Module_راس* Inline_Mod_ګراف::getInlineModModuleVertexp(Ast_غوټه_Module* modp) {
    إذا (!modp->user4p()) modp->user4p(جديد Inline_Mod_Module_راس{*هذا, modp});
    أرجع modp->user4u().منزل<Inline_Mod_Module_راس*>();
}
Inline_Mod_Cell_راس* Inline_Mod_ګراف::getInlineModCellVertexp(AstCell* cellp) {
    إذا (!cellp->user4p()) cellp->user4p(جديد Inline_Mod_Cell_راس{*هذا, cellp});
    أرجع cellp->user4u().منزل<Inline_Mod_Cell_راس*>();
}

فراغ Inline_Mod_ګراف::جمع_کړه_څنډه(Inline_Mod_Module_راس& پلار, Inline_Mod_Cell_راس& cell) {
    UASSERT_OBJ(cell.دننه_تشه(), &cell, "Cell should have at most one incoming edge");
    جديد V3_ګراف_څنډه{هذا, &پلار, &cell, 1, /* cutable: */ خطأ};
}
فراغ Inline_Mod_ګراف::جمع_کړه_څنډه(Inline_Mod_Cell_راس& cell, Inline_Mod_Module_راس& submodule) {
    UASSERT_OBJ(cell.بهر_تشه(), &cell, "Cell should have at most one outgoing edge");
    جديد V3_ګراف_څنډه{هذا, &cell, &submodule, 1, /* cutable: */ خطأ};
}

فراغ Inline_Mod_Module_راس::سېټ_Flatten() {
    لكل (V3_ګراف_څنډه& څنډه : بهر_څنډې()) {
        Inline_Mod_Cell_راس& cVtx = *څنډه.سر()->as<Inline_Mod_Cell_راس>();
        إذا (cVtx.flatten()) استمر;
        cVtx.سېټ_Flatten();
        Inline_Mod_Module_راس& iVtx = cVtx.instance_د();
        إذا (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.سېټ_Do_Inline("flatten parent");
        iVtx.سېټ_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

صنف Inline_Mod_ګراف_Builder final : عام VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_ګراف> m_graphp{جديد Inline_Mod_ګراف};  // The graph being built
    Inline_Mod_Module_راس* m_modVtxp = تش_اشاره;  // Vertex of module currently being iterated
    ثابت AstScope* m_sizedScopep = تش_اشاره;  // The scope of current module measured for size

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_راس* ثابت vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        إذا (VN_IS(nodep, Iface)) vtxp->سېټ_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        إذا (nodep->isConstPool()) vtxp->سېټ_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        إذا (VN_IS(nodep, Package)) vtxp->سېټ_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        إذا (nodep->verilatorLib()) vtxp->سېټ_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        إذا (nodep->mod_عامه()) vtxp->سېټ_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = تش_اشاره;
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        إذا (m_modVtxp) m_modVtxp->سېټ_No_Inline_Hard("Contains class");
    }

    فراغ لیدنه(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        إذا (!m_sizedScopep) m_sizedScopep = nodep;
        إذا (m_sizedScopep != nodep) أرجع;
        إذا (m_modVtxp) m_modVtxp->اندازه_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    فراغ لیدنه(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_راس* ثابت vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->جمع_کړه_څنډه(*m_modVtxp, *vtxp);
        m_graphp->جمع_کړه_څنډه(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->سېټ_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }

        إذا (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->سېټ_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (m_modVtxp) m_modVtxp->اندازه_Inc();
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    څرګند Inline_Mod_ګراف_Builder(AstNetlist* nodep) {
        // Build the module instantiation graph
        iterateConst(nodep);
        // Order vertices (any topological order is fine), can't be cyclic at this point
        m_graphp->ترتیب();
        // Check that the first vertex is the top level module (everything, including packages,
        // have a corresponding AstCell under the top level at this point).
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_راس>()->modp()->is_سر(),
                    nodep, "First vertex should be top level module");
#که_تعریف_شوی VL_DEBUG
        لكل (ثابت V3_ګراف_راس& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            إذا (&vtx == m_graphp->vertices().frontp()) استمر;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.دننه_تشه(), &vtx, "Should have edges from root");
        }
#د_که_پای
    }
    ~Inline_Mod_ګراف_Builder() override = افتراضي;

عام:
    ساكن std::unique_ptr<Inline_Mod_ګراف> apply(AstNetlist* nodep) {
        أرجع std::move(Inline_Mod_ګراف_Builder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

نطاق ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
کارول ScopeChildren = std::unordered_map<ثابت AstScope*, std::ویکټور<AstScope*>>;

// Record downward links from parent scopes to their child scopes
فراغ gatherScopes(ثابت Ast_غوټه_Module* modp, ScopeChildren& children) {
    لكل (Ast_غوټه* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ټېل_شا(scopep);
        } وإلا إذا (ثابت Ast_غوټه_Module* ثابت subModp = VN_CAST(nodep, غوټه_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
فراغ renameScopes(AstScope* scopep, ثابت std::string& old_مختاړی, ثابت std::string& new_مختاړی,
                  ثابت ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_مختاړی), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_مختاړی + scopep->name().substr(old_مختاړی.اندازه()));
    ثابت خپلکاری it = children.پیدا_کړه(scopep);
    إذا (it == children.پای_ټکی()) أرجع;
    لكل (AstScope* ثابت childp : it->دوهم) {
        renameScopes(childp, old_مختاړی, new_مختاړی, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
فراغ inlineScope(AstScope* scopep, AstCell* cellp, ثابت std::string& مختاړی,
                 ScopeChildren& children) {
    AstScope* ثابت پلار_Scopep = scopep->aboveScopep();
    UASSERT_OBJ(پلار_Scopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << پلار_Scopep);

    // Move the variables of the inlined scope into the scope above
    لكل (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        إذا (vscp->scopep() == scopep) vscp->scopep(پلار_Scopep);
        // If the module was cloned, point to the cloned variable
        إذا (AstVar* ثابت newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        إذا (!cellp->isTrace()) vscp->trace(خطأ);
        پلار_Scopep->جمع_کړه_Varsp(vscp->unlink_Fr_شا());
    }

    // Move the logic of the inlined scope into the scope above
    لكل (Ast_غوټه *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_شا();
        إذا (Ast_غوټه_FTask* ثابت ftaskp = VN_CAST(nodep, غوټه_FTask)) {
            ftaskp->name(مختاړی + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        إذا (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_غوټه_Cover_Decl* ثابت declp = incp->declp();
                إذا (declp->فی_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    إذا (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    أرجع;
                }
                إذا (Ast_غوټه_Cover_Decl* ثابت newDeclp = VN_CAST(declp->user3p(), غوټه_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        پلار_Scopep->جمع_کړه_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    لكل (Ast_غوټه *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ثابت cisp = VN_AS(nodep->unlink_Fr_شا(), CellInlineScope);
        cisp->name(مختاړی + cisp->name());
        پلار_Scopep->جمع_کړه_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    إذا (v3Global.opt.vpi()) {
        پلار_Scopep->جمع_کړه_Inlinesp(
            جديد AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ثابت std::string old_مختاړی = scopep->name() + ".";
    ثابت std::string new_مختاړی = scopep->name() + "__DOT__";
    std::ویکټور<AstScope*> ماشوم_Scopeps;
    {
        ثابت خپلکاری it = children.پیدا_کړه(scopep);
        إذا (it != children.پای_ټکی()) {
            ماشوم_Scopeps = std::move(it->دوهم);
            children.erase(it);
        }
    }
    لكل (AstScope* ثابت ماشوم_Scopep : ماشوم_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(ماشوم_Scopep->modp(), Class), ماشوم_Scopep,
                    "Inlined scope should not contain a class scope");
        إذا (AstCell* ثابت newCellp = VN_CAST(ماشوم_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            ماشوم_Scopep->aboveCellp(newCellp);
        }
        ماشوم_Scopep->aboveScopep(پلار_Scopep);
        renameScopes(ماشوم_Scopep, old_مختاړی, new_مختاړی, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::ویکټور<AstScope*>& پلار_Childps = children[پلار_Scopep];
    پلار_Childps.erase(std::remove(پلار_Childps.begin(), پلار_Childps.پای_ټکی(), scopep),
                        پلار_Childps.پای_ټکی());
    پلار_Childps.ورننباسه(پلار_Childps.پای_ټکی(), ماشوم_Scopeps.begin(), ماشوم_Scopeps.پای_ټکی());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
فراغ inlineCell(Ast_غوټه_Module* modp, AstCell* cellp, منطقي وروستی, Inline_Mod_ګراف& ګراف,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ثابت VNUser3_دننه_Use user3_دننه_Use;

    VNDeleter deleter;
    deleter.ټېل_Deletep(cellp->unlink_Fr_شا());

    Ast_غوټه_Module* ثابت subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::ویکټور<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::ویکټور<AstScope*> otherScopeps;  // Scopes under some other instance
    لكل (Ast_غوټه *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ثابت scopep = VN_CAST(nodep, Scope);
        إذا (!scopep) استمر;
        scopep->unlink_Fr_شا();
        إذا (scopep->aboveCellp() == cellp) {
            inlineScopeps.ټېل_شا(scopep);
            deleter.ټېل_Deletep(scopep);
        } وإلا {
            otherScopeps.ټېل_شا(scopep);
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
    Ast_غوټه_Module* inlinedp;
    إذا (وروستی) {
        inlinedp = subModp->unlink_Fr_شا();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.تشه(), cellp, "Last instance, but has other scopes");
    } وإلا {
        inlinedp = subModp->clone_ونه(خطأ);
        // Compute map from the original module items to their clones
        لكل (Ast_غوټه *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        إذا (v3Global.opt.coverageFsm()) {
            لكل (Ast_غوټه *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ثابت Ast_غوټه_Cover_Decl* ثابت declp = VN_CAST(nodep, غوټه_Cover_Decl);
                إذا (declp && declp->فی_Instance()) {
                    VL_DO_DANGLING(deleter.ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        لكل (AstScope* ثابت scopep : otherScopeps) subModp->جمع_کړه_Stmtsp(scopep);
    }
    deleter.ټېل_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ثابت std::string مختاړی = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    إذا (AstSystemCSection* ثابت sectionsp = inlinedp->scSectionsp()) {
        modp->جمع_کړه_Sc_Sectionsp(sectionsp->unlink_Fr_شا_With_بل());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_راس* ثابت mVtxp = ګراف.getInlineModModuleVertexp(modp);
    لكل (Ast_غوټه *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_شا();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            varp->name(مختاړی + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            إذا (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            إذا (!cellp->isTrace()) varp->trace(خطأ);
        } وإلا إذا (AstCell* ثابت subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(مختاړی + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(تش_اشاره);
            Inline_Mod_Cell_راس* ثابت vtxp = ګراف.getInlineModCellVertexp(subCellp);
            ګراف.جمع_کړه_څنډه(*mVtxp, *vtxp);
            ګراف.جمع_کړه_څنډه(*vtxp, *ګراف.getInlineModModuleVertexp(subCellp->modp()));
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(مختاړی + typedefp->name());
        } وإلا إذا (Ast_غوټه_Cover_Decl* ثابت declp = VN_CAST(nodep, غوټه_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            إذا (!declp->فی_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->جمع_کړه_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    لكل (AstScope* ثابت scopep : inlineScopeps) inlineScope(scopep, cellp, مختاړی, children);
}

// Apply all inlining decisions
فراغ process(AstNetlist* netlistp, Inline_Mod_ګراف& ګراف) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    لكل (Ast_غوټه_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), غوټه_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::ویکټور<Inline_Mod_Cell_راس*> cVtxps;
    لكل (V3_ګراف_راس& vtx : ګراف.vertices()) {
        Inline_Mod_Cell_راس* ثابت cVtxp = vtx.cast<Inline_Mod_Cell_راس>();
        إذا (!cVtxp) استمر;
        إذا (!cVtxp->doInline()) استمر;
        cVtxps.ټېل_شا(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    لكل (Inline_Mod_Cell_راس* ثابت cVtxp : vlstd::سرچپه_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_راس& mVtx = cVtxp->instance_دننه();
        Inline_Mod_Module_راس* ثابت iVtxp = &cVtxp->instance_د();
        AstCell* ثابت cellp = cVtxp->cellp();
        ثابت منطقي وروستی = iVtxp->دننه_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.اندازه_Inc(iVtxp->اندازه());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&ګراف), cVtxp);
        // Delete the module we are inlining if this is the last instance
        إذا (وروستی) {
            طالما (!iVtxp->بهر_تشه()) {
                Inline_Mod_Cell_راس* ثابت tVtxp
                    = iVtxp->بهر_څنډې().frontp()->سر()->as<Inline_Mod_Cell_راس>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&ګراف), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&ګراف), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, وروستی, ګراف, children);
        إذا (dump_ګراف_Level() >= 9) ګراف.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_غوټه_Var_Ref* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::جمع_کړه_Stat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

فراغ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<Inline_Mod_ګراف> graphp = Inline_Mod_ګراف_Builder::apply(nodep);
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ثابت اندازه_t design_اندازه
        = graphp->vertices().frontp()->as<Inline_Mod_Module_راس>()->flattened_اندازه();
    لكل (V3_ګراف_راس& vtx : graphp->vertices()) {
        إذا (Inline_Mod_Module_راس* ثابت mVtxp = vtx.cast<Inline_Mod_Module_راس>()) {
            // If this module is less than 10% of the design, flatten this module
            إذا (mVtxp->flattened_اندازه() * 10 < design_اندازه) mVtxp->سېټ_Flatten();
            // Don't inline if can't inline
            إذا (mVtxp->noInlineHard()) استمر;
            // Don't inline if soft off
            إذا (mVtxp->noInlineSoft()) استمر;
            // If all instances of this module combined are less than 20% of the design, inline all
            اندازه_t ټول_اندازه = mVtxp->flattened_اندازه() * mVtxp->instance_شمېر();
            إذا (ټول_اندازه * 5 < design_اندازه) {
                لكل (V3_ګراف_څنډه& څنډه : mVtxp->دننه_څنډې()) {
                    Inline_Mod_Cell_راس* ثابت cVtxp = څنډه.fromp()->as<Inline_Mod_Cell_راس>();
                    cVtxp->سېټ_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            استمر;
        }

        // The instantiation
        Inline_Mod_Cell_راس& cVtx = *vtx.as<Inline_Mod_Cell_راس>();
        // The module instantiated by this cell
        Inline_Mod_Module_راس& mVtx = cVtx.instance_د();

        // Don't inline if can't inline, duh!
        إذا (mVtx.noInlineHard()) استمر;

        // If it should be inlined, inlined it
        إذا (mVtx.shouldInline()) cVtx.سېټ_Do_Inline("should inline");
        // If --flatten, inline it
        إذا (v3Global.opt.flatten()) cVtx.سېټ_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        إذا (mVtx.noInlineSoft()) استمر;

        // If instatiated in exactly one static site, inline it
        إذا (mVtx.دننه_Size1()) cVtx.سېټ_Do_Inline("Single static instance");
        // If small, inline it
        إذا (mVtx.اندازه() < INLINE_MODS_SMALLER) cVtx.سېټ_Do_Inline("Small");
        // If inlineMult is 0, inline it
        إذا (v3Global.opt.inlineMult() < 1) cVtx.سېټ_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ثابت اندازه_t inlined_اندازه = mVtx.دننه_څنډې().اندازه() * mVtx.اندازه();
        ثابت اندازه_t حد = v3Global.opt.inlineMult();
        إذا (inlined_اندازه < حد) cVtx.سېټ_Do_Inline("inlinedSize < inlineMult");
    }
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_ونه("inline", 0, dump_ونه_Either_Level() >= 3);
}
