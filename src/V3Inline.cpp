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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Inline.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// CONFIG
ساکن ثابت صحیح INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

جماعت Inline_Mod_Module_راس;
جماعت Inline_Mod_Cell_راس;

جماعت Inline_Mod_گراف final : عوامی V3_گراف {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4_اندر_Use m_user4_اندر_Use;

عوامی:
    Inline_Mod_گراف()
        : V3_گراف{} {}
    ~Inline_Mod_گراف() override = طےشدہ;

    // METHODS
    Inline_Mod_Module_راس* getInlineModModuleVertexp(Ast_گرہ_Module* modp);
    Inline_Mod_Cell_راس* getInlineModCellVertexp(AstCell* cellp);
    خالی جوڑو_کنارہ(Inline_Mod_Module_راس& from, Inline_Mod_Cell_راس& مقصد);
    خالی جوڑو_کنارہ(Inline_Mod_Cell_راس& from, Inline_Mod_Module_راس& مقصد);

    // debug
    std::string dotRankDir() ثابت override { واپس "LR"; }
};

جماعت Inline_Mod_Either_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Inline_Mod_Either_راس, V3_گراف_راس)
محفوظ:
    صریح Inline_Mod_Either_راس(Inline_Mod_گراف& گراف)
        : V3_گراف_راس{&گراف} {}
};

جماعت Inline_Mod_Module_راس final : عوامی Inline_Mod_Either_راس {
    VL_RTTI_IMPL(Inline_Mod_Module_راس, Inline_Mod_Either_راس)
    Ast_گرہ_Module* ثابت m_modp;  // The module
    ثابت حرف* m_noInlineHardWyp = خالی_اشاریہ;  // First reason the module can never be inlined
    ثابت حرف* m_noInlineSoftWyp = خالی_اشاریہ;  // First reason not to inline unless forced
    ثابت حرف* m_shouldInlineWhyp = خالی_اشاریہ;  // First reason why this module should be inlined
    سائز_t m_سائز = 0;  // The size (statement count) of the module
    سائز_t تغیرپذیر m_flattened_سائز = 0;  // The size of the module if flattened
    منطقی تغیرپذیر m_flattened_سائز_Valid = جھوٹ;  // Whether the flattened size is valid
    سائز_t تغیرپذیر m_instance_گنتی = 0;  // The number of total instances of this module
    منطقی تغیرپذیر m_instance_گنتی_Valid = جھوٹ;  // Whether the instance count is valid

عوامی:
    Inline_Mod_Module_راس(Inline_Mod_گراف& گراف, Ast_گرہ_Module* modp)
        : Inline_Mod_Either_راس{گراف}
        , m_modp{modp} {}
    ~Inline_Mod_Module_راس() override = طےشدہ;

    // ACCESSORS
    Ast_گرہ_Module* modp() ثابت { واپس m_modp; }
    سائز_t سائز() ثابت { واپس m_سائز; }
    خالی سائز(سائز_t قدر) { m_سائز = قدر; }
    خالی سائز_Inc(سائز_t قدر = 1) { m_سائز += قدر; }
    منطقی noInlineHard() ثابت { واپس m_noInlineHardWyp; }
    خالی سیٹ_No_Inline_Hard(ثابت حرف* whyp) {
        اگر (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    منطقی noInlineSoft() ثابت { واپس m_noInlineSoftWyp; }
    خالی سیٹ_No_Inline_Soft(ثابت حرف* whyp) {
        اگر (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    منطقی shouldInline() ثابت { واپس m_shouldInlineWhyp; }
    خالی سیٹ_Should_Inline(ثابت حرف* whyp) {
        اگر (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    خالی سیٹ_Flatten();

    // Total size of module, with all hierarchy below flattened
    سائز_t flattened_سائز() ثابت {
        اگر (!m_flattened_سائز_Valid) {
            m_flattened_سائز_Valid = سچ;
            m_flattened_سائز = m_سائز;
            برائے (ثابت V3_گراف_کنارہ& e1 : برآمد_کنارے()) {
                برائے (ثابت V3_گراف_کنارہ& e2 : e1.چوٹی()->برآمد_کنارے()) {
                    Inline_Mod_Module_راس* ثابت mVtxp = e2.چوٹی()->as<Inline_Mod_Module_راس>();
                    m_flattened_سائز += mVtxp->flattened_سائز();
                }
            }
        }
        واپس m_flattened_سائز;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    سائز_t instance_گنتی() ثابت {
        اگر (!m_instance_گنتی_Valid) {
            m_instance_گنتی_Valid = سچ;
            m_instance_گنتی = 0;
            برائے (ثابت V3_گراف_کنارہ& e1 : اندر_کنارے()) {
                برائے (ثابت V3_گراف_کنارہ& e2 : e1.fromp()->اندر_کنارے()) {
                    Inline_Mod_Module_راس* ثابت mVtxp = e2.fromp()->as<Inline_Mod_Module_راس>();
                    m_instance_گنتی += mVtxp->instance_گنتی();
                }
            }
            اگر (!m_instance_گنتی) {
                UASSERT_OBJ(m_modp->is_چوٹی(), m_modp, "non-top level module should have instances");
                m_instance_گنتی = 1;
            }
        }
        واپس m_instance_گنتی;
    }

    // debug
    File_سطر* fileline() ثابت override { واپس m_modp->fileline(); }
    std::string dotShape() ثابت override { واپس "box"; }
    std::string dotColor() ثابت override {
        واپس m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvt_مقصد_Hex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::مقصد_string(instance_گنتی());
        str += "\nsize: " + std::مقصد_string(m_سائز);
        str += "\nflattenedSize: " + std::مقصد_string(flattened_سائز());
        اگر (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        اگر (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        اگر (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        واپس str;
    }
};

جماعت Inline_Mod_Cell_راس final : عوامی Inline_Mod_Either_راس {
    VL_RTTI_IMPL(Inline_Mod_Cell_راس, Inline_Mod_Either_راس)
    AstCell* ثابت m_cellp;  // The cell (instance)
    ثابت حرف* m_doInlineWyp = خالی_اشاریہ;  // First reason this instance should be inlined
    منطقی m_flatten = جھوٹ;  // Whether this cell and below already flattened (avoid O(n^2))

عوامی:
    Inline_Mod_Cell_راس(Inline_Mod_گراف& گراف, AstCell* cellp)
        : Inline_Mod_Either_راس{گراف}
        , m_cellp{cellp} {}
    ~Inline_Mod_Cell_راس() override = طےشدہ;

    // ACCESSORS
    AstCell* cellp() ثابت { واپس m_cellp; }
    منطقی doInline() ثابت { واپس m_doInlineWyp; }
    خالی سیٹ_Do_Inline(ثابت حرف* whyp) {
        اگر (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    منطقی flatten() ثابت { واپس m_flatten; }
    خالی سیٹ_Flatten() { m_flatten = سچ; }

    // The module vertx this cell is instantiating
    Inline_Mod_Module_راس& instance_کا() ثابت {
        UASSERT_OBJ(برآمد_Size1(), یہ, "Cell should have exactly one outgoing edge");
        واپس *برآمد_کنارے().frontp()->چوٹی()->as<Inline_Mod_Module_راس>();
    }
    // The module vertex this cell is instantiated in
    Inline_Mod_Module_راس& instance_اندر() ثابت {
        UASSERT_OBJ(اندر_Size1(), یہ, "Cell should have exactly one incoming edge");
        واپس *اندر_کنارے().frontp()->fromp()->as<Inline_Mod_Module_راس>();
    }

    // debug
    File_سطر* fileline() ثابت override { واپس m_cellp->fileline(); }
    std::string dotColor() ثابت override { واپس m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ثابت override { واپس "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvt_مقصد_Hex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        اگر (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        واپس str;
    }
};

Inline_Mod_Module_راس* Inline_Mod_گراف::getInlineModModuleVertexp(Ast_گرہ_Module* modp) {
    اگر (!modp->user4p()) modp->user4p(نیا Inline_Mod_Module_راس{*یہ, modp});
    واپس modp->user4u().مقصد<Inline_Mod_Module_راس*>();
}
Inline_Mod_Cell_راس* Inline_Mod_گراف::getInlineModCellVertexp(AstCell* cellp) {
    اگر (!cellp->user4p()) cellp->user4p(نیا Inline_Mod_Cell_راس{*یہ, cellp});
    واپس cellp->user4u().مقصد<Inline_Mod_Cell_راس*>();
}

خالی Inline_Mod_گراف::جوڑو_کنارہ(Inline_Mod_Module_راس& والد, Inline_Mod_Cell_راس& cell) {
    UASSERT_OBJ(cell.اندر_خالی_خانے(), &cell, "Cell should have at most one incoming edge");
    نیا V3_گراف_کنارہ{یہ, &والد, &cell, 1, /* cutable: */ جھوٹ};
}
خالی Inline_Mod_گراف::جوڑو_کنارہ(Inline_Mod_Cell_راس& cell, Inline_Mod_Module_راس& submodule) {
    UASSERT_OBJ(cell.برآمد_خالی_خانے(), &cell, "Cell should have at most one outgoing edge");
    نیا V3_گراف_کنارہ{یہ, &cell, &submodule, 1, /* cutable: */ جھوٹ};
}

خالی Inline_Mod_Module_راس::سیٹ_Flatten() {
    برائے (V3_گراف_کنارہ& کنارہ : برآمد_کنارے()) {
        Inline_Mod_Cell_راس& cVtx = *کنارہ.چوٹی()->as<Inline_Mod_Cell_راس>();
        اگر (cVtx.flatten()) جاری;
        cVtx.سیٹ_Flatten();
        Inline_Mod_Module_راس& iVtx = cVtx.instance_کا();
        اگر (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.سیٹ_Do_Inline("flatten parent");
        iVtx.سیٹ_Flatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

جماعت Inline_Mod_گراف_Builder final : عوامی VNVisitor {
    // STATE
    std::unique_ptr<Inline_Mod_گراف> m_graphp{نیا Inline_Mod_گراف};  // The graph being built
    Inline_Mod_Module_راس* m_modVtxp = خالی_اشاریہ;  // Vertex of module currently being iterated
    ثابت AstScope* m_sizedScopep = خالی_اشاریہ;  // The scope of current module measured for size

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        Inline_Mod_Module_راس* ثابت vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        اگر (VN_IS(nodep, Iface)) vtxp->سیٹ_No_Inline_Hard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        اگر (nodep->isConstPool()) vtxp->سیٹ_No_Inline_Hard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        اگر (VN_IS(nodep, Package)) vtxp->سیٹ_No_Inline_Hard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        اگر (nodep->verilatorLib()) vtxp->سیٹ_No_Inline_Hard("verilatorLib");

        // Don't inline public modules by default
        اگر (nodep->mod_عوامی()) vtxp->سیٹ_No_Inline_Soft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = خالی_اشاریہ;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        اگر (m_modVtxp) m_modVtxp->سیٹ_No_Inline_Hard("Contains class");
    }

    خالی دورہ(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        اگر (!m_sizedScopep) m_sizedScopep = nodep;
        اگر (m_sizedScopep != nodep) واپس;
        اگر (m_modVtxp) m_modVtxp->سائز_Inc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    خالی دورہ(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        Inline_Mod_Cell_راس* ثابت vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->جوڑو_کنارہ(*m_modVtxp, *vtxp);
        m_graphp->جوڑو_کنارہ(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstPragma* nodep) override {
        اگر (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            اگر (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ورنہ {
                m_modVtxp->سیٹ_Should_Inline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }

        اگر (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            اگر (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } ورنہ {
                m_modVtxp->سیٹ_No_Inline_Soft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (m_modVtxp) m_modVtxp->سائز_Inc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<Inline_Mod_Module_راس>()->modp()->is_چوٹی(),
                    nodep, "First vertex should be top level module");
#اگر_تعریف_شدہ VL_DEBUG
        برائے (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            اگر (&vtx == m_graphp->vertices().frontp()) جاری;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.اندر_خالی_خانے(), &vtx, "Should have edges from root");
        }
#اگر_ختم
    }
    ~Inline_Mod_گراف_Builder() override = طےشدہ;

عوامی:
    ساکن std::unique_ptr<Inline_Mod_گراف> apply(AstNetlist* nodep) {
        واپس std::move(Inline_Mod_گراف_Builder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

نامخلا ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
استعمال ScopeChildren = std::unordered_map<ثابت AstScope*, std::سمتیہ<AstScope*>>;

// Record downward links from parent scopes to their child scopes
خالی gatherScopes(ثابت Ast_گرہ_Module* modp, ScopeChildren& children) {
    برائے (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        اگر (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ڈالو_عقب(scopep);
        } ورنہ اگر (ثابت Ast_گرہ_Module* ثابت subModp = VN_CAST(nodep, گرہ_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
خالی renameScopes(AstScope* scopep, ثابت std::string& old_سابقہ, ثابت std::string& new_سابقہ,
                  ثابت ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), old_سابقہ), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(new_سابقہ + scopep->name().substr(old_سابقہ.سائز()));
    ثابت خودکار it = children.ڈھونڈو(scopep);
    اگر (it == children.انتہا()) واپس;
    برائے (AstScope* ثابت childp : it->دوسرا) {
        renameScopes(childp, old_سابقہ, new_سابقہ, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
خالی inlineScope(AstScope* scopep, AstCell* cellp, ثابت std::string& سابقہ,
                 ScopeChildren& children) {
    AstScope* ثابت والد_Scopep = scopep->aboveScopep();
    UASSERT_OBJ(والد_Scopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << والد_Scopep);

    // Move the variables of the inlined scope into the scope above
    برائے (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        اگر (vscp->scopep() == scopep) vscp->scopep(والد_Scopep);
        // If the module was cloned, point to the cloned variable
        اگر (AstVar* ثابت newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        اگر (!cellp->isTrace()) vscp->trace(جھوٹ);
        والد_Scopep->جوڑو_Varsp(vscp->unlink_Fr_عقب());
    }

    // Move the logic of the inlined scope into the scope above
    برائے (Ast_گرہ *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_عقب();
        اگر (Ast_گرہ_FTask* ثابت ftaskp = VN_CAST(nodep, گرہ_FTask)) {
            ftaskp->name(سابقہ + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        اگر (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_گرہ_Cover_Decl* ثابت declp = incp->declp();
                اگر (declp->فی_Instance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    اگر (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    واپس;
                }
                اگر (Ast_گرہ_Cover_Decl* ثابت newDeclp = VN_CAST(declp->user3p(), گرہ_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        والد_Scopep->جوڑو_Blocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    برائے (Ast_گرہ *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ثابت cisp = VN_AS(nodep->unlink_Fr_عقب(), CellInlineScope);
        cisp->name(سابقہ + cisp->name());
        والد_Scopep->جوڑو_Inlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    اگر (v3Global.opt.vpi()) {
        والد_Scopep->جوڑو_Inlinesp(
            نیا AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ثابت std::string old_سابقہ = scopep->name() + ".";
    ثابت std::string new_سابقہ = scopep->name() + "__DOT__";
    std::سمتیہ<AstScope*> بچہ_Scopeps;
    {
        ثابت خودکار it = children.ڈھونڈو(scopep);
        اگر (it != children.انتہا()) {
            بچہ_Scopeps = std::move(it->دوسرا);
            children.erase(it);
        }
    }
    برائے (AstScope* ثابت بچہ_Scopep : بچہ_Scopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(بچہ_Scopep->modp(), Class), بچہ_Scopep,
                    "Inlined scope should not contain a class scope");
        اگر (AstCell* ثابت newCellp = VN_CAST(بچہ_Scopep->aboveCellp()->user3p(), Cell)) {
            // If the module was cloned, point to the cloned cell
            بچہ_Scopep->aboveCellp(newCellp);
        }
        بچہ_Scopep->aboveScopep(والد_Scopep);
        renameScopes(بچہ_Scopep, old_سابقہ, new_سابقہ, children);
    }
    // Children of the inlined scope are now children of the scope above. Note this must
    // come after the erase above, as inserting into 'children' can invalidate 'it'.
    std::سمتیہ<AstScope*>& والد_Childps = children[والد_Scopep];
    والد_Childps.erase(std::remove(والد_Childps.begin(), والد_Childps.انتہا(), scopep),
                        والد_Childps.انتہا());
    والد_Childps.داخل_کرو(والد_Childps.انتہا(), بچہ_Scopeps.begin(), بچہ_Scopeps.انتہا());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
خالی inlineCell(Ast_گرہ_Module* modp, AstCell* cellp, منطقی آخری, Inline_Mod_گراف& گراف,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ثابت VNUser3_اندر_Use user3_اندر_Use;

    VNDeleter deleter;
    deleter.ڈالو_Deletep(cellp->unlink_Fr_عقب());

    Ast_گرہ_Module* ثابت subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::سمتیہ<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::سمتیہ<AstScope*> otherScopeps;  // Scopes under some other instance
    برائے (Ast_گرہ *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ثابت scopep = VN_CAST(nodep, Scope);
        اگر (!scopep) جاری;
        scopep->unlink_Fr_عقب();
        اگر (scopep->aboveCellp() == cellp) {
            inlineScopeps.ڈالو_عقب(scopep);
            deleter.ڈالو_Deletep(scopep);
        } ورنہ {
            otherScopeps.ڈالو_عقب(scopep);
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
    Ast_گرہ_Module* inlinedp;
    اگر (آخری) {
        inlinedp = subModp->unlink_Fr_عقب();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.خالی_خانے(), cellp, "Last instance, but has other scopes");
    } ورنہ {
        inlinedp = subModp->clone_درخت(جھوٹ);
        // Compute map from the original module items to their clones
        برائے (Ast_گرہ *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        اگر (v3Global.opt.coverageFsm()) {
            برائے (Ast_گرہ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ثابت Ast_گرہ_Cover_Decl* ثابت declp = VN_CAST(nodep, گرہ_Cover_Decl);
                اگر (declp && declp->فی_Instance()) {
                    VL_DO_DANGLING(deleter.ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        برائے (AstScope* ثابت scopep : otherScopeps) subModp->جوڑو_Stmtsp(scopep);
    }
    deleter.ڈالو_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ثابت std::string سابقہ = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    اگر (AstSystemCSection* ثابت sectionsp = inlinedp->scSectionsp()) {
        modp->جوڑو_Sc_Sectionsp(sectionsp->unlink_Fr_عقب_With_اگلا());
    }

    // Move the module level declarations of the inlined module into 'modp'
    Inline_Mod_Module_راس* ثابت mVtxp = گراف.getInlineModModuleVertexp(modp);
    برائے (Ast_گرہ *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlink_Fr_عقب();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            varp->name(سابقہ + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            اگر (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            اگر (!cellp->isTrace()) varp->trace(جھوٹ);
        } ورنہ اگر (AstCell* ثابت subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(سابقہ + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(خالی_اشاریہ);
            Inline_Mod_Cell_راس* ثابت vtxp = گراف.getInlineModCellVertexp(subCellp);
            گراف.جوڑو_کنارہ(*mVtxp, *vtxp);
            گراف.جوڑو_کنارہ(*vtxp, *گراف.getInlineModModuleVertexp(subCellp->modp()));
        } ورنہ اگر (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(سابقہ + typedefp->name());
        } ورنہ اگر (Ast_گرہ_Cover_Decl* ثابت declp = VN_CAST(nodep, گرہ_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            اگر (!declp->فی_Instance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->جوڑو_Stmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    برائے (AstScope* ثابت scopep : inlineScopeps) inlineScope(scopep, cellp, سابقہ, children);
}

// Apply all inlining decisions
خالی process(AstNetlist* netlistp, Inline_Mod_گراف& گراف) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    برائے (Ast_گرہ_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گرہ_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::سمتیہ<Inline_Mod_Cell_راس*> cVtxps;
    برائے (V3_گراف_راس& vtx : گراف.vertices()) {
        Inline_Mod_Cell_راس* ثابت cVtxp = vtx.cast<Inline_Mod_Cell_راس>();
        اگر (!cVtxp) جاری;
        اگر (!cVtxp->doInline()) جاری;
        cVtxps.ڈالو_عقب(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    برائے (Inline_Mod_Cell_راس* ثابت cVtxp : vlstd::الٹ_view(cVtxps)) {
        // Pick up parts before deleting
        Inline_Mod_Module_راس& mVtx = cVtxp->instance_اندر();
        Inline_Mod_Module_راس* ثابت iVtxp = &cVtxp->instance_کا();
        AstCell* ثابت cellp = cVtxp->cellp();
        ثابت منطقی آخری = iVtxp->اندر_Size1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.سائز_Inc(iVtxp->سائز());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&گراف), cVtxp);
        // Delete the module we are inlining if this is the last instance
        اگر (آخری) {
            جب (!iVtxp->برآمد_خالی_خانے()) {
                Inline_Mod_Cell_راس* ثابت tVtxp
                    = iVtxp->برآمد_کنارے().frontp()->چوٹی()->as<Inline_Mod_Cell_راس>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&گراف), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&گراف), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, آخری, گراف, children);
        اگر (dump_گراف_Level() >= 9) گراف.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_گرہ_Var_Ref* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        اگر (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::جوڑو_Stat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

خالی V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<Inline_Mod_گراف> graphp = Inline_Mod_گراف_Builder::apply(nodep);
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ثابت سائز_t design_سائز
        = graphp->vertices().frontp()->as<Inline_Mod_Module_راس>()->flattened_سائز();
    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        اگر (Inline_Mod_Module_راس* ثابت mVtxp = vtx.cast<Inline_Mod_Module_راس>()) {
            // If this module is less than 10% of the design, flatten this module
            اگر (mVtxp->flattened_سائز() * 10 < design_سائز) mVtxp->سیٹ_Flatten();
            // Don't inline if can't inline
            اگر (mVtxp->noInlineHard()) جاری;
            // Don't inline if soft off
            اگر (mVtxp->noInlineSoft()) جاری;
            // If all instances of this module combined are less than 20% of the design, inline all
            سائز_t کل_سائز = mVtxp->flattened_سائز() * mVtxp->instance_گنتی();
            اگر (کل_سائز * 5 < design_سائز) {
                برائے (V3_گراف_کنارہ& کنارہ : mVtxp->اندر_کنارے()) {
                    Inline_Mod_Cell_راس* ثابت cVtxp = کنارہ.fromp()->as<Inline_Mod_Cell_راس>();
                    cVtxp->سیٹ_Do_Inline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            جاری;
        }

        // The instantiation
        Inline_Mod_Cell_راس& cVtx = *vtx.as<Inline_Mod_Cell_راس>();
        // The module instantiated by this cell
        Inline_Mod_Module_راس& mVtx = cVtx.instance_کا();

        // Don't inline if can't inline, duh!
        اگر (mVtx.noInlineHard()) جاری;

        // If it should be inlined, inlined it
        اگر (mVtx.shouldInline()) cVtx.سیٹ_Do_Inline("should inline");
        // If --flatten, inline it
        اگر (v3Global.opt.flatten()) cVtx.سیٹ_Do_Inline("--flatten");

        // Don't inline for other reasons if soft off
        اگر (mVtx.noInlineSoft()) جاری;

        // If instatiated in exactly one static site, inline it
        اگر (mVtx.اندر_Size1()) cVtx.سیٹ_Do_Inline("Single static instance");
        // If small, inline it
        اگر (mVtx.سائز() < INLINE_MODS_SMALLER) cVtx.سیٹ_Do_Inline("Small");
        // If inlineMult is 0, inline it
        اگر (v3Global.opt.inlineMult() < 1) cVtx.سیٹ_Do_Inline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ثابت سائز_t inlined_سائز = mVtx.اندر_کنارے().سائز() * mVtx.سائز();
        ثابت سائز_t حد_بندی = v3Global.opt.inlineMult();
        اگر (inlined_سائز < حد_بندی) cVtx.سیٹ_Do_Inline("inlinedSize < inlineMult");
    }
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_درخت("inline", 0, dump_درخت_Either_Level() >= 3);
}
