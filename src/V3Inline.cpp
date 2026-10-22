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
ایستا ثابت صحیح INLINE_MODS_SMALLER = 100;  // If a mod is < this # nodes, can always inline it

//######################################################################
// Bipartite module instantiation graph containing module and cell vertices

رده InlineModModuleVertex;
رده InlineModCellVertex;

رده InlineModGraph final : عمومی V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

عمومی:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = پیشفرض;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_گره_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    تهی addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    تهی addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() ثابت override { بازگشت "LR"; }
};

رده InlineModEitherVertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
محافظتشده:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

رده InlineModModuleVertex final : عمومی InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_گره_Module* ثابت m_modp;  // The module
    ثابت نویسه* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    ثابت نویسه* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    ثابت نویسه* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    منطقی mutable m_flattenedSizeValid = نادرست;  // Whether the flattened size is valid
    size_t mutable m_instance_شمار = 0;  // The number of total instances of this module
    منطقی mutable m_instance_شمار_Valid = نادرست;  // Whether the instance count is valid

عمومی:
    InlineModModuleVertex(InlineModGraph& graph, Ast_گره_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = پیشفرض;

    // ACCESSORS
    Ast_گره_Module* modp() ثابت { بازگشت m_modp; }
    size_t size() ثابت { بازگشت m_size; }
    تهی size(size_t مقدار) { m_size = مقدار; }
    تهی sizeInc(size_t مقدار = 1) { m_size += مقدار; }
    منطقی noInlineHard() ثابت { بازگشت m_noInlineHardWyp; }
    تهی setNoInlineHard(ثابت نویسه* whyp) {
        اگر (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    منطقی noInlineSoft() ثابت { بازگشت m_noInlineSoftWyp; }
    تهی setNoInlineSoft(ثابت نویسه* whyp) {
        اگر (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    منطقی shouldInline() ثابت { بازگشت m_shouldInlineWhyp; }
    تهی setShouldInline(ثابت نویسه* whyp) {
        اگر (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    تهی setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() ثابت {
        اگر (!m_flattenedSizeValid) {
            m_flattenedSizeValid = درست;
            m_flattenedSize = m_size;
            برای (ثابت V3GraphEdge& e1 : outEdges()) {
                برای (ثابت V3GraphEdge& e2 : e1.بالاترین()->outEdges()) {
                    InlineModModuleVertex* ثابت mVtxp = e2.بالاترین()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        بازگشت m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_شمار() ثابت {
        اگر (!m_instance_شمار_Valid) {
            m_instance_شمار_Valid = درست;
            m_instance_شمار = 0;
            برای (ثابت V3GraphEdge& e1 : inEdges()) {
                برای (ثابت V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* ثابت mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_شمار += mVtxp->instance_شمار();
                }
            }
            اگر (!m_instance_شمار) {
                UASSERT_OBJ(m_modp->is_بالاترین(), m_modp, "non-top level module should have instances");
                m_instance_شمار = 1;
            }
        }
        بازگشت m_instance_شمار;
    }

    // debug
    File_سطر* fileline() ثابت override { بازگشت m_modp->fileline(); }
    std::string dotShape() ثابت override { بازگشت "box"; }
    std::string dotColor() ثابت override {
        بازگشت m_noInlineHardWyp    ? "red"
               : m_shouldInlineWhyp ? "blue"
               : m_noInlineSoftWyp  ? "orange"
                                    : "black";
    }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_شمار());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        اگر (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        اگر (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        اگر (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        بازگشت str;
    }
};

رده InlineModCellVertex final : عمومی InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* ثابت m_cellp;  // The cell (instance)
    ثابت نویسه* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    منطقی m_flatten = نادرست;  // Whether this cell and below already flattened (avoid O(n^2))

عمومی:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = پیشفرض;

    // ACCESSORS
    AstCell* cellp() ثابت { بازگشت m_cellp; }
    منطقی doInline() ثابت { بازگشت m_doInlineWyp; }
    تهی setDoInline(ثابت نویسه* whyp) {
        اگر (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    منطقی flatten() ثابت { بازگشت m_flatten; }
    تهی setFlatten() { m_flatten = درست; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() ثابت {
        UASSERT_OBJ(outSize1(), این, "Cell should have exactly one outgoing edge");
        بازگشت *outEdges().frontp()->بالاترین()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() ثابت {
        UASSERT_OBJ(inSize1(), این, "Cell should have exactly one incoming edge");
        بازگشت *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_سطر* fileline() ثابت override { بازگشت m_cellp->fileline(); }
    std::string dotColor() ثابت override { بازگشت m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ثابت override { بازگشت "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        اگر (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        بازگشت str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_گره_Module* modp) {
    اگر (!modp->user4p()) modp->user4p(جدید InlineModModuleVertex{*این, modp});
    بازگشت modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    اگر (!cellp->user4p()) cellp->user4p(جدید InlineModCellVertex{*این, cellp});
    بازگشت cellp->user4u().to<InlineModCellVertex*>();
}

تهی InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    جدید V3GraphEdge{این, &parent, &cell, 1, /* cutable: */ نادرست};
}
تهی InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    جدید V3GraphEdge{این, &cell, &submodule, 1, /* cutable: */ نادرست};
}

تهی InlineModModuleVertex::setFlatten() {
    برای (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.بالاترین()->as<InlineModCellVertex>();
        اگر (cVtx.flatten()) ادامه;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        اگر (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

رده InlineModGraphBuilder final : عمومی VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{جدید InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    ثابت AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* ثابت vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        اگر (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        اگر (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        اگر (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        اگر (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        اگر (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    تهی visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        اگر (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    تهی visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        اگر (!m_sizedScopep) m_sizedScopep = nodep;
        اگر (m_sizedScopep != nodep) بازگشت;
        اگر (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    تهی visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* ثابت vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    تهی visit(AstPragma* nodep) override {
        اگر (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            اگر (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وگرنه {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }

        اگر (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            اگر (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وگرنه {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    تهی visit(Ast_گره* nodep) override {
        اگر (m_modVtxp) m_modVtxp->sizeInc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_بالاترین(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        برای (ثابت V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            اگر (&vtx == m_graphp->vertices().frontp()) ادامه;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = پیشفرض;

عمومی:
    ایستا std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        بازگشت std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

فضاینام ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<ثابت AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
تهی gatherScopes(ثابت Ast_گره_Module* modp, ScopeChildren& children) {
    برای (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        اگر (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].افزودن_back(scopep);
        } وگرنه اگر (ثابت Ast_گره_Module* ثابت subModp = VN_CAST(nodep, گره_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
تهی renameScopes(AstScope* scopep, ثابت std::string& oldPrefix, ثابت std::string& newPrefix,
                  ثابت ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    ثابت auto it = children.find(scopep);
    اگر (it == children.end()) بازگشت;
    برای (AstScope* ثابت childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
تهی inlineScope(AstScope* scopep, AstCell* cellp, ثابت std::string& prefix,
                 ScopeChildren& children) {
    AstScope* ثابت parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    برای (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        اگر (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        اگر (AstVar* ثابت newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        اگر (!cellp->isTrace()) vscp->trace(نادرست);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    برای (Ast_گره *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        اگر (Ast_گره_FTask* ثابت ftaskp = VN_CAST(nodep, گره_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        اگر (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_گره_Cover_Decl* ثابت declp = incp->declp();
                اگر (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    اگر (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    بازگشت;
                }
                اگر (Ast_گره_Cover_Decl* ثابت newDeclp = VN_CAST(declp->user3p(), گره_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    برای (Ast_گره *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ثابت cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    اگر (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            جدید AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ثابت std::string oldPrefix = scopep->name() + ".";
    ثابت std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        ثابت auto it = children.find(scopep);
        اگر (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    برای (AstScope* ثابت childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        اگر (AstCell* ثابت newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.درج(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
تهی inlineCell(Ast_گره_Module* modp, AstCell* cellp, منطقی last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ثابت VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.افزودن_Deletep(cellp->unlinkFrBack());

    Ast_گره_Module* ثابت subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    برای (Ast_گره *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ثابت scopep = VN_CAST(nodep, Scope);
        اگر (!scopep) ادامه;
        scopep->unlinkFrBack();
        اگر (scopep->aboveCellp() == cellp) {
            inlineScopeps.افزودن_back(scopep);
            deleter.افزودن_Deletep(scopep);
        } وگرنه {
            otherScopeps.افزودن_back(scopep);
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
    اگر (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } وگرنه {
        inlinedp = subModp->clone_درخت(نادرست);
        // Compute map from the original module items to their clones
        برای (Ast_گره *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        اگر (v3Global.opt.coverageFsm()) {
            برای (Ast_گره *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ثابت Ast_گره_Cover_Decl* ثابت declp = VN_CAST(nodep, گره_Cover_Decl);
                اگر (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        برای (AstScope* ثابت scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.افزودن_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ثابت std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    اگر (AstSystemCSection* ثابت sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* ثابت mVtxp = graph.getInlineModModuleVertexp(modp);
    برای (Ast_گره *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            اگر (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            اگر (!cellp->isTrace()) varp->trace(نادرست);
        } وگرنه اگر (AstCell* ثابت subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* ثابت vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } وگرنه اگر (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } وگرنه اگر (Ast_گره_Cover_Decl* ثابت declp = VN_CAST(nodep, گره_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            اگر (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    برای (AstScope* ثابت scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
تهی process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    برای (Ast_گره_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گره_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    برای (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* ثابت cVtxp = vtx.cast<InlineModCellVertex>();
        اگر (!cVtxp) ادامه;
        اگر (!cVtxp->doInline()) ادامه;
        cVtxps.افزودن_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    برای (InlineModCellVertex* ثابت cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* ثابت iVtxp = &cVtxp->instanceOf();
        AstCell* ثابت cellp = cVtxp->cellp();
        ثابت منطقی last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        اگر (last) {
            وقتی (!iVtxp->outEmpty()) {
                InlineModCellVertex* ثابت tVtxp
                    = iVtxp->outEdges().frontp()->بالاترین()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        اگر (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_گره_Var_Ref* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        اگر (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

تهی V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ثابت size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    برای (V3GraphVertex& vtx : graphp->vertices()) {
        اگر (InlineModModuleVertex* ثابت mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            اگر (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            اگر (mVtxp->noInlineHard()) ادامه;
            // Don't inline if soft off
            اگر (mVtxp->noInlineSoft()) ادامه;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_شمار();
            اگر (totalSize * 5 < designSize) {
                برای (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* ثابت cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            ادامه;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        اگر (mVtx.noInlineHard()) ادامه;

        // If it should be inlined, inlined it
        اگر (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        اگر (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        اگر (mVtx.noInlineSoft()) ادامه;

        // If instatiated in exactly one static site, inline it
        اگر (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        اگر (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        اگر (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ثابت size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        ثابت size_t limit = v3Global.opt.inlineMult();
        اگر (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_درخت("inline", 0, dump_درخت_Either_Level() >= 3);
}
