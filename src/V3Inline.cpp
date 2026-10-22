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

صنف InlineModModuleVertex;
صنف InlineModCellVertex;

صنف InlineModGraph final : عام V3Graph {
    // NODE STATE
    // AstNodeModule::user4p() -> InlineModModuleVertex*, the module vertex
    // AstCell::user4p()       -> InlineModCellVertex*, the cell vertex

    VNUser4InUse m_user4InUse;

عام:
    InlineModGraph()
        : V3Graph{} {}
    ~InlineModGraph() override = افتراضي;

    // METHODS
    InlineModModuleVertex* getInlineModModuleVertexp(Ast_عقدة_Module* modp);
    InlineModCellVertex* getInlineModCellVertexp(AstCell* cellp);
    فراغ addEdge(InlineModModuleVertex& from, InlineModCellVertex& to);
    فراغ addEdge(InlineModCellVertex& from, InlineModModuleVertex& to);

    // debug
    std::string dotRankDir() ثابت override { أرجع "LR"; }
};

صنف InlineModEitherVertex VL_NOT_FINAL : عام V3GraphVertex {
    VL_RTTI_IMPL(InlineModEitherVertex, V3GraphVertex)
محمي:
    explicit InlineModEitherVertex(InlineModGraph& graph)
        : V3GraphVertex{&graph} {}
};

صنف InlineModModuleVertex final : عام InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModModuleVertex, InlineModEitherVertex)
    Ast_عقدة_Module* ثابت m_modp;  // The module
    ثابت حرف* m_noInlineHardWyp = nullptr;  // First reason the module can never be inlined
    ثابت حرف* m_noInlineSoftWyp = nullptr;  // First reason not to inline unless forced
    ثابت حرف* m_shouldInlineWhyp = nullptr;  // First reason why this module should be inlined
    size_t m_size = 0;  // The size (statement count) of the module
    size_t mutable m_flattenedSize = 0;  // The size of the module if flattened
    منطقي mutable m_flattenedSizeValid = خطأ;  // Whether the flattened size is valid
    size_t mutable m_instance_عدد = 0;  // The number of total instances of this module
    منطقي mutable m_instance_عدد_Valid = خطأ;  // Whether the instance count is valid

عام:
    InlineModModuleVertex(InlineModGraph& graph, Ast_عقدة_Module* modp)
        : InlineModEitherVertex{graph}
        , m_modp{modp} {}
    ~InlineModModuleVertex() override = افتراضي;

    // ACCESSORS
    Ast_عقدة_Module* modp() ثابت { أرجع m_modp; }
    size_t size() ثابت { أرجع m_size; }
    فراغ size(size_t قيمة) { m_size = قيمة; }
    فراغ sizeInc(size_t قيمة = 1) { m_size += قيمة; }
    منطقي noInlineHard() ثابت { أرجع m_noInlineHardWyp; }
    فراغ setNoInlineHard(ثابت حرف* whyp) {
        إذا (!m_noInlineHardWyp) m_noInlineHardWyp = whyp;
    }
    منطقي noInlineSoft() ثابت { أرجع m_noInlineSoftWyp; }
    فراغ setNoInlineSoft(ثابت حرف* whyp) {
        إذا (!m_noInlineSoftWyp) m_noInlineSoftWyp = whyp;
    }
    منطقي shouldInline() ثابت { أرجع m_shouldInlineWhyp; }
    فراغ setShouldInline(ثابت حرف* whyp) {
        إذا (!m_shouldInlineWhyp) m_shouldInlineWhyp = whyp;
    }
    // Mark every instance below this module for inlining
    فراغ setFlatten();

    // Total size of module, with all hierarchy below flattened
    size_t flattenedSize() ثابت {
        إذا (!m_flattenedSizeValid) {
            m_flattenedSizeValid = true;
            m_flattenedSize = m_size;
            لكل (ثابت V3GraphEdge& e1 : outEdges()) {
                لكل (ثابت V3GraphEdge& e2 : e1.قمة()->outEdges()) {
                    InlineModModuleVertex* ثابت mVtxp = e2.قمة()->as<InlineModModuleVertex>();
                    m_flattenedSize += mVtxp->flattenedSize();
                }
            }
        }
        أرجع m_flattenedSize;
    }
    // Total number of instances of this module in the whole hierarchy of the design
    // Note this is the same as the number of AstScopes the module has.
    size_t instance_عدد() ثابت {
        إذا (!m_instance_عدد_Valid) {
            m_instance_عدد_Valid = true;
            m_instance_عدد = 0;
            لكل (ثابت V3GraphEdge& e1 : inEdges()) {
                لكل (ثابت V3GraphEdge& e2 : e1.fromp()->inEdges()) {
                    InlineModModuleVertex* ثابت mVtxp = e2.fromp()->as<InlineModModuleVertex>();
                    m_instance_عدد += mVtxp->instance_عدد();
                }
            }
            إذا (!m_instance_عدد) {
                UASSERT_OBJ(m_modp->is_قمة(), m_modp, "non-top level module should have instances");
                m_instance_عدد = 1;
            }
        }
        أرجع m_instance_عدد;
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
        std::string str = m_modp->typeName() + " "s + cvtToHex(m_modp);
        str += "\n" + m_modp->name() + " @ " + fileline()->ascii();
        str += "\ninstanceCount: " + std::to_string(instance_عدد());
        str += "\nsize: " + std::to_string(m_size);
        str += "\nflattenedSize: " + std::to_string(flattenedSize());
        إذا (m_shouldInlineWhyp) str += "\nShouldInline: "s + m_shouldInlineWhyp;
        إذا (m_noInlineHardWyp) str += "\nNoInlineHard: "s + m_noInlineHardWyp;
        إذا (m_noInlineSoftWyp) str += "\nNoInlineSoft: "s + m_noInlineSoftWyp;
        str += "\n";
        أرجع str;
    }
};

صنف InlineModCellVertex final : عام InlineModEitherVertex {
    VL_RTTI_IMPL(InlineModCellVertex, InlineModEitherVertex)
    AstCell* ثابت m_cellp;  // The cell (instance)
    ثابت حرف* m_doInlineWyp = nullptr;  // First reason this instance should be inlined
    منطقي m_flatten = خطأ;  // Whether this cell and below already flattened (avoid O(n^2))

عام:
    InlineModCellVertex(InlineModGraph& graph, AstCell* cellp)
        : InlineModEitherVertex{graph}
        , m_cellp{cellp} {}
    ~InlineModCellVertex() override = افتراضي;

    // ACCESSORS
    AstCell* cellp() ثابت { أرجع m_cellp; }
    منطقي doInline() ثابت { أرجع m_doInlineWyp; }
    فراغ setDoInline(ثابت حرف* whyp) {
        إذا (!m_doInlineWyp) m_doInlineWyp = whyp;
    }
    منطقي flatten() ثابت { أرجع m_flatten; }
    فراغ setFlatten() { m_flatten = true; }

    // The module vertx this cell is instantiating
    InlineModModuleVertex& instanceOf() ثابت {
        UASSERT_OBJ(outSize1(), هذا, "Cell should have exactly one outgoing edge");
        أرجع *outEdges().frontp()->قمة()->as<InlineModModuleVertex>();
    }
    // The module vertex this cell is instantiated in
    InlineModModuleVertex& instanceIn() ثابت {
        UASSERT_OBJ(inSize1(), هذا, "Cell should have exactly one incoming edge");
        أرجع *inEdges().frontp()->fromp()->as<InlineModModuleVertex>();
    }

    // debug
    File_سطر* fileline() ثابت override { أرجع m_cellp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_doInlineWyp ? "green" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = m_cellp->typeName() + " "s + cvtToHex(m_cellp);
        str += "\n" + m_cellp->name() + " @ " + fileline()->ascii();
        إذا (m_doInlineWyp) str += "\nDoInline: "s + m_doInlineWyp;
        str += "\n";
        أرجع str;
    }
};

InlineModModuleVertex* InlineModGraph::getInlineModModuleVertexp(Ast_عقدة_Module* modp) {
    إذا (!modp->user4p()) modp->user4p(جديد InlineModModuleVertex{*هذا, modp});
    أرجع modp->user4u().to<InlineModModuleVertex*>();
}
InlineModCellVertex* InlineModGraph::getInlineModCellVertexp(AstCell* cellp) {
    إذا (!cellp->user4p()) cellp->user4p(جديد InlineModCellVertex{*هذا, cellp});
    أرجع cellp->user4u().to<InlineModCellVertex*>();
}

فراغ InlineModGraph::addEdge(InlineModModuleVertex& parent, InlineModCellVertex& cell) {
    UASSERT_OBJ(cell.inEmpty(), &cell, "Cell should have at most one incoming edge");
    جديد V3GraphEdge{هذا, &parent, &cell, 1, /* cutable: */ خطأ};
}
فراغ InlineModGraph::addEdge(InlineModCellVertex& cell, InlineModModuleVertex& submodule) {
    UASSERT_OBJ(cell.outEmpty(), &cell, "Cell should have at most one outgoing edge");
    جديد V3GraphEdge{هذا, &cell, &submodule, 1, /* cutable: */ خطأ};
}

فراغ InlineModModuleVertex::setFlatten() {
    لكل (V3GraphEdge& edge : outEdges()) {
        InlineModCellVertex& cVtx = *edge.قمة()->as<InlineModCellVertex>();
        إذا (cVtx.flatten()) استمر;
        cVtx.setFlatten();
        InlineModModuleVertex& iVtx = cVtx.instanceOf();
        إذا (!iVtx.noInlineHard() && !iVtx.noInlineSoft()) cVtx.setDoInline("flatten parent");
        iVtx.setFlatten();
    }
}

//######################################################################
// Visitor that builds the bipartite module instantiation graph

صنف InlineModGraphBuilder final : عام VNVisitor {
    // STATE
    std::unique_ptr<InlineModGraph> m_graphp{جديد InlineModGraph};  // The graph being built
    InlineModModuleVertex* m_modVtxp = nullptr;  // Vertex of module currently being iterated
    ثابت AstScope* m_sizedScopep = nullptr;  // The scope of current module measured for size

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        UASSERT_OBJ(!m_modVtxp, nodep, "Unsupported: Nested modules");

        // Create the module vertex
        InlineModModuleVertex* ثابت vtxp = m_graphp->getInlineModModuleVertexp(nodep);

        // Check if the module itself is not inlineable

        // TODO: All references are resolved by now, but AstIfaceRefDType::cellp and
        // the AstIntfRef records still name the interface instance, so keep it.
        إذا (VN_IS(nodep, Iface)) vtxp->setNoInlineHard("Interface");
        // Never inline the constant pool, even if packages become inlineable
        إذا (nodep->isConstPool()) vtxp->setNoInlineHard("Constant pool");
        // Never inline packages - TODO: conceptually fine, but why not?
        إذا (VN_IS(nodep, Package)) vtxp->setNoInlineHard("Package");
        // Never inline the wrapper of a --lib-create library, its instances
        // create the library models at construction (see V3EmitCSyms)
        إذا (nodep->verilatorLib()) vtxp->setNoInlineHard("verilatorLib");

        // Don't inline public modules by default
        إذا (nodep->modPublic()) vtxp->setNoInlineSoft("Public module");

        // Iterate children
        VL_RESTORER(m_modVtxp);
        VL_RESTORER(m_sizedScopep);
        m_modVtxp = vtxp;
        m_sizedScopep = nullptr;
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstClass* nodep) override {
        // TODO allow inlining of modules that contain classes
        إذا (m_modVtxp) m_modVtxp->setNoInlineHard("Contains class");
    }

    فراغ visit(AstScope* nodep) override {
        // Every instance of a module holds an identical copy of the module body under
        // its own AstScope, so only measure the size of one of them.
        إذا (!m_sizedScopep) m_sizedScopep = nodep;
        إذا (m_sizedScopep != nodep) أرجع;
        إذا (m_modVtxp) m_modVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

    // Cells instantiate modules
    فراغ visit(AstCell* nodep) override {
        UASSERT_OBJ(m_modVtxp, nodep, "Cell should be under a module");

        // Create the cell vertex
        InlineModCellVertex* ثابت vtxp = m_graphp->getInlineModCellVertexp(nodep);

        // Add containing-module/instantiated-module edges
        m_graphp->addEdge(*m_modVtxp, *vtxp);
        m_graphp->addEdge(*vtxp, *m_graphp->getInlineModModuleVertexp(nodep->modp()));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->setShouldInline("Pragma INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
            أرجع;
        }

        إذا (nodep->pragType() == VPragmaType::NO_INLINE_MODULE) {
            إذا (!m_modVtxp) {
                nodep->v3error("Inline pragma not under a module");  // LCOV_EXCL_LINE
            } وإلا {
                m_modVtxp->setNoInlineSoft("Pragma NO_INLINE_MODULE");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
            أرجع;
        }

        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (m_modVtxp) m_modVtxp->sizeInc();
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
        UASSERT_OBJ(m_graphp->vertices().frontp()->as<InlineModModuleVertex>()->modp()->is_قمة(),
                    nodep, "First vertex should be top level module");
#ifdef VL_DEBUG
        لكل (ثابت V3GraphVertex& vtx : m_graphp->vertices()) {
            // First vertex is the top levelmodule, we checked above
            إذا (&vtx == m_graphp->vertices().frontp()) استمر;
            // Otherwise it should have instantiations
            UASSERT_OBJ(!vtx.inEmpty(), &vtx, "Should have edges from root");
        }
#endif
    }
    ~InlineModGraphBuilder() override = افتراضي;

عام:
    ساكن std::unique_ptr<InlineModGraph> apply(AstNetlist* nodep) {
        أرجع std::move(InlineModGraphBuilder{nodep}.m_graphp);
    }
};

//######################################################################
// Module inliner

نطاق ModuleInliner {

// The scopes instantiated directly under each scope (that is parent -> children links)
using ScopeChildren = std::unordered_map<ثابت AstScope*, std::vector<AstScope*>>;

// Record downward links from parent scopes to their child scopes
فراغ gatherScopes(ثابت Ast_عقدة_Module* modp, ScopeChildren& children) {
    لكل (Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            // Note the top scope is held under the AstTopScope, so is not seen here
            UASSERT_OBJ(scopep->aboveScopep(), scopep, "Instance scope should have a scope above");
            children[scopep->aboveScopep()].ادفع_back(scopep);
        } وإلا إذا (ثابت Ast_عقدة_Module* ثابت subModp = VN_CAST(nodep, عقدة_Module)) {
            // An AstClass holds its scopes under itself
            UASSERT_OBJ(VN_IS(subModp, Class), subModp, "Nested module should be a class");
            gatherScopes(subModp, children);
        }
    }
}

// Rename the given scope, and all scopes below it, after the scope named by 'oldPrefix'
// (the original parent of 'scopep') has been inlined into the scope above it
فراغ renameScopes(AstScope* scopep, ثابت std::string& oldPrefix, ثابت std::string& newPrefix,
                  ثابت ScopeChildren& children) {
    UASSERT_OBJ(VString::startsWith(scopep->name(), oldPrefix), scopep,
                "Scope name should start with the name of the scope above it");
    scopep->name(newPrefix + scopep->name().substr(oldPrefix.size()));
    ثابت auto it = children.find(scopep);
    إذا (it == children.end()) أرجع;
    لكل (AstScope* ثابت childp : it->second) {
        renameScopes(childp, oldPrefix, newPrefix, children);
    }
}

// Merge the given scope (instance) of the inlined cell into the scope above it
فراغ inlineScope(AstScope* scopep, AstCell* cellp, ثابت std::string& prefix,
                 ScopeChildren& children) {
    AstScope* ثابت parentScopep = scopep->aboveScopep();
    UASSERT_OBJ(parentScopep, scopep, "Inlined scope should have a scope above");
    UINFO(6, "  Inline Scope " << scopep);
    UINFO(6, "     into      " << parentScopep);

    // Move the variables of the inlined scope into the scope above
    لكل (AstVarScope *vscp = scopep->varsp(), *nextp; vscp; vscp = nextp) {
        nextp = VN_AS(vscp->nextp(), VarScope);
        // Note V3Scope attaches variables of non-virtual interface references to the
        // scope of the interface instance, so only update if it is this scope
        إذا (vscp->scopep() == scopep) vscp->scopep(parentScopep);
        // If the module was cloned, point to the cloned variable
        إذا (AstVar* ثابت newVarp = VN_CAST(vscp->varp()->user3p(), Var)) {
            vscp->varp(newVarp);
            vscp->dtypeFrom(newVarp);
        }
        إذا (!cellp->isTrace()) vscp->trace(خطأ);
        parentScopep->addVarsp(vscp->unlinkFrBack());
    }

    // Move the logic of the inlined scope into the scope above
    لكل (Ast_عقدة *nodep = scopep->blocksp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        إذا (Ast_عقدة_FTask* ثابت ftaskp = VN_CAST(nodep, عقدة_FTask)) {
            ftaskp->name(prefix + ftaskp->name());
        }
        // If the module was cloned, point coverage increments to the cloned declarations
        إذا (v3Global.opt.coverage()) {
            nodep->foreach([&](AstCoverInc* incp) {
                Ast_عقدة_Cover_Decl* ثابت declp = incp->declp();
                إذا (declp->perInstance()) {
                    // Not cloned, fix up the path here, as only this scope refers to it
                    إذا (!incp->isDuplicate()) {
                        declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
                    }
                    أرجع;
                }
                إذا (Ast_عقدة_Cover_Decl* ثابت newDeclp = VN_CAST(declp->user3p(), عقدة_Cover_Decl)) {
                    incp->declp(newDeclp);
                }
            });
        }
        parentScopep->addBlocksp(nodep);
    }

    // Move the inline records of the inlined scope into the scope above
    لكل (Ast_عقدة *nodep = scopep->inlinesp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstCellInlineScope* ثابت cisp = VN_AS(nodep->unlinkFrBack(), CellInlineScope);
        cisp->name(prefix + cisp->name());
        parentScopep->addInlinesp(cisp);
    }
    // ... and add one for the instance we are inlining now
    إذا (v3Global.opt.vpi()) {
        parentScopep->addInlinesp(
            جديد AstCellInlineScope{cellp->fileline(), cellp->name(), cellp->modp()->origName()});
    }

    // Reparent and rename the scopes instantiated under the inlined scope
    ثابت std::string oldPrefix = scopep->name() + ".";
    ثابت std::string newPrefix = scopep->name() + "__DOT__";
    std::vector<AstScope*> childScopeps;
    {
        ثابت auto it = children.find(scopep);
        إذا (it != children.end()) {
            childScopeps = std::move(it->second);
            children.erase(it);
        }
    }
    لكل (AstScope* ثابت childScopep : childScopeps) {
        // A class scope would hang off the scope of the module declaring it, but modules
        // containing classes are never inlined
        UASSERT_OBJ(!VN_IS(childScopep->modp(), Class), childScopep,
                    "Inlined scope should not contain a class scope");
        إذا (AstCell* ثابت newCellp = VN_CAST(childScopep->aboveCellp()->user3p(), Cell)) {
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
    parentChildps.أدرج(parentChildps.end(), childScopeps.begin(), childScopeps.end());

    UASSERT_OBJ(!scopep->varsp() && !scopep->blocksp() && !scopep->inlinesp(), scopep,
                "Inlined scope should be empty");
}

// Inline 'cellp' into 'modp'. 'last' indicatest this is tha last instance of the inlined module
فراغ inlineCell(Ast_عقدة_Module* modp, AstCell* cellp, منطقي last, InlineModGraph& graph,
                ScopeChildren& children) {
    UINFO(5, " Inline Cell  " << cellp);
    UINFO(5, " into Module  " << modp);

    // NODE STATE
    //  AstNode::user3p()  -> AstNode*. The clone of this module level declaration
    ثابت VNUser3InUse user3InUse;

    VNDeleter deleter;
    deleter.ادفع_Deletep(cellp->unlinkFrBack());

    Ast_عقدة_Module* ثابت subModp = cellp->modp();  // The module being inlined

    // Unlink all scopes of the instantiated module, so they are not cloned with it
    std::vector<AstScope*> inlineScopeps;  // Scopes under 'cellp'
    std::vector<AstScope*> otherScopeps;  // Scopes under some other instance
    لكل (Ast_عقدة *nodep = subModp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        AstScope* ثابت scopep = VN_CAST(nodep, Scope);
        إذا (!scopep) استمر;
        scopep->unlinkFrBack();
        إذا (scopep->aboveCellp() == cellp) {
            inlineScopeps.ادفع_back(scopep);
            deleter.ادفع_Deletep(scopep);
        } وإلا {
            otherScopeps.ادفع_back(scopep);
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
    Ast_عقدة_Module* inlinedp;
    إذا (last) {
        inlinedp = subModp->unlinkFrBack();
        // This is the only instantiation, so all scopes are being inlined
        UASSERT_OBJ(otherScopeps.empty(), cellp, "Last instance, but has other scopes");
    } وإلا {
        inlinedp = subModp->clone_شجرة(خطأ);
        // Compute map from the original module items to their clones
        لكل (Ast_عقدة *ap = subModp->stmtsp(), *bp = inlinedp->stmtsp(); ap || bp;
             ap = ap->nextp(), bp = bp->nextp()) {
            UASSERT_OBJ(ap && bp, ap ? ap : bp, "Clone has different number of children");
            ap->user3p(bp);
        }
        // Per instance coverage declarations must not be duplicated, drop the clones
        إذا (v3Global.opt.coverageFsm()) {
            لكل (Ast_عقدة *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ثابت Ast_عقدة_Cover_Decl* ثابت declp = VN_CAST(nodep, عقدة_Cover_Decl);
                إذا (declp && declp->perInstance()) {
                    VL_DO_DANGLING(deleter.ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
        // Put back the scopes of the instances we are not inlining this time
        لكل (AstScope* ثابت scopep : otherScopeps) subModp->addStmtsp(scopep);
    }
    deleter.ادفع_Deletep(inlinedp);

    // Prefix for renaming inlined declarations
    ثابت std::string prefix = cellp->name() + "__DOT__";

    // Move the `systemc_* blocks of the inlined module into 'modp'
    إذا (AstSystemCSection* ثابت sectionsp = inlinedp->scSectionsp()) {
        modp->addScSectionsp(sectionsp->unlinkFrBackWithNext());
    }

    // Move the module level declarations of the inlined module into 'modp'
    InlineModModuleVertex* ثابت mVtxp = graph.getInlineModModuleVertexp(modp);
    لكل (Ast_عقدة *nodep = inlinedp->stmtsp(), *nextp; nodep; nodep = nextp) {
        nextp = nodep->nextp();
        nodep->unlinkFrBack();
        UASSERT_OBJ(!VN_IS(nodep, Class), nodep,
                    "Module containing a class should not be inlined");
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            varp->name(prefix + varp->name());
            // Variable is now local to 'modp', rename to avoid conflicts and clear I/O bits
            إذا (varp->direction() == VDirection::INOUT && varp->varType() == VVarType::WIRE) {
                varp->varType(VVarType::TRIWIRE);
            }
            varp->direction(VDirection::NONE);
            إذا (!cellp->isTrace()) varp->trace(خطأ);
        } وإلا إذا (AstCell* ثابت subCellp = VN_CAST(nodep, Cell)) {
            subCellp->name(prefix + subCellp->name());
            // Need to update graph. Note the vertex of the original cell was either
            // deleted (if 'last'), or user4p is a copy made by cloneTree, so reset it.
            subCellp->user4p(nullptr);
            InlineModCellVertex* ثابت vtxp = graph.getInlineModCellVertexp(subCellp);
            graph.addEdge(*mVtxp, *vtxp);
            graph.addEdge(*vtxp, *graph.getInlineModModuleVertexp(subCellp->modp()));
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            typedefp->name(prefix + typedefp->name());
        } وإلا إذا (Ast_عقدة_Cover_Decl* ثابت declp = VN_CAST(nodep, عقدة_Cover_Decl)) {
            // Fix path in coverage statements. Per instance ones are fixed in inlineScope.
            إذا (!declp->perInstance()) {
                declp->hier(VString::dot(cellp->prettyName(), ".", declp->hier()));
            }
        }
        modp->addStmtsp(nodep);
    }

    // Merge each scope (instance) of the inlined cell into the scope above it
    لكل (AstScope* ثابت scopep : inlineScopeps) inlineScope(scopep, cellp, prefix, children);
}

// Apply all inlining decisions
فراغ process(AstNetlist* netlistp, InlineModGraph& graph) {
    // Number of inlined instances, for statistics
    VDouble0 m_nInlined;

    // Record the scope hierarchy - we need the downward links
    ScopeChildren children;
    لكل (Ast_عقدة_Module* modp = netlistp->modulesp(); modp;
         modp = VN_AS(modp->nextp(), عقدة_Module)) {
        gatherScopes(modp, children);
    }

    // Gather all cells that need to be inlined (this is in topological order)
    std::vector<InlineModCellVertex*> cVtxps;
    لكل (V3GraphVertex& vtx : graph.vertices()) {
        InlineModCellVertex* ثابت cVtxp = vtx.cast<InlineModCellVertex>();
        إذا (!cVtxp) استمر;
        إذا (!cVtxp->doInline()) استمر;
        cVtxps.ادفع_back(cVtxp);
    }

    // Inline cells bottom up (leaves into roots)
    لكل (InlineModCellVertex* ثابت cVtxp : vlstd::reverse_view(cVtxps)) {
        // Pick up parts before deleting
        InlineModModuleVertex& mVtx = cVtxp->instanceIn();
        InlineModModuleVertex* ثابت iVtxp = &cVtxp->instanceOf();
        AstCell* ثابت cellp = cVtxp->cellp();
        ثابت منطقي last = iVtxp->inSize1();
        UASSERT_OBJ(!iVtxp->noInlineHard(), cellp, "Should not be inlining if not possible");

        // Update
        ++m_nInlined;
        mVtx.sizeInc(iVtxp->size());  // For debug dump only

        // Delete the cell we are inlining
        VL_DO_DANGLING(cVtxp->unlinkDelete(&graph), cVtxp);
        // Delete the module we are inlining if this is the last instance
        إذا (last) {
            طالما (!iVtxp->outEmpty()) {
                InlineModCellVertex* ثابت tVtxp
                    = iVtxp->outEdges().frontp()->قمة()->as<InlineModCellVertex>();
                // Bottom up ordering ensures this
                UASSERT_OBJ(!tVtxp->doInline(), tVtxp, "Should have been inlined");
                VL_DO_DANGLING(tVtxp->unlinkDelete(&graph), tVtxp);
            }
            VL_DO_DANGLING(iVtxp->unlinkDelete(&graph), iVtxp);
        }

        // Do it
        inlineCell(mVtx.modp(), cellp, last, graph, children);
        إذا (dumpGraphLevel() >= 9) graph.dumpDotFilePrefixed("inlinemod-cell");
    }

    // Restore varp() == varScopep()->varp() on all references, as cloning modules for
    // inlining repointed some AstVarScopes. Hierarchical references can be anywhere.
    netlistp->foreach([](Ast_عقدة_Var_Ref* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (vscp && refp->varp() != vscp->varp()) refp->varp(vscp->varp());
    });

    V3Stats::addStat("Optimizations, Inlined instances", m_nInlined);
}

}  //namespace ModuleInliner

//######################################################################
// V3Inline class functions

فراغ V3Inline::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the bipartite module instantiation graph
    std::unique_ptr<InlineModGraph> graphp = InlineModGraphBuilder::apply(nodep);
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-graph");

    // Decide which instances to inline
    ثابت size_t designSize
        = graphp->vertices().frontp()->as<InlineModModuleVertex>()->flattenedSize();
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        إذا (InlineModModuleVertex* ثابت mVtxp = vtx.cast<InlineModModuleVertex>()) {
            // If this module is less than 10% of the design, flatten this module
            إذا (mVtxp->flattenedSize() * 10 < designSize) mVtxp->setFlatten();
            // Don't inline if can't inline
            إذا (mVtxp->noInlineHard()) استمر;
            // Don't inline if soft off
            إذا (mVtxp->noInlineSoft()) استمر;
            // If all instances of this module combined are less than 20% of the design, inline all
            size_t totalSize = mVtxp->flattenedSize() * mVtxp->instance_عدد();
            إذا (totalSize * 5 < designSize) {
                لكل (V3GraphEdge& edge : mVtxp->inEdges()) {
                    InlineModCellVertex* ثابت cVtxp = edge.fromp()->as<InlineModCellVertex>();
                    cVtxp->setDoInline("< 20% of design");
                }
            }
            // No more decisions based on module vertex
            استمر;
        }

        // The instantiation
        InlineModCellVertex& cVtx = *vtx.as<InlineModCellVertex>();
        // The module instantiated by this cell
        InlineModModuleVertex& mVtx = cVtx.instanceOf();

        // Don't inline if can't inline, duh!
        إذا (mVtx.noInlineHard()) استمر;

        // If it should be inlined, inlined it
        إذا (mVtx.shouldInline()) cVtx.setDoInline("should inline");
        // If --flatten, inline it
        إذا (v3Global.opt.flatten()) cVtx.setDoInline("--flatten");

        // Don't inline for other reasons if soft off
        إذا (mVtx.noInlineSoft()) استمر;

        // If instatiated in exactly one static site, inline it
        إذا (mVtx.inSize1()) cVtx.setDoInline("Single static instance");
        // If small, inline it
        إذا (mVtx.size() < INLINE_MODS_SMALLER) cVtx.setDoInline("Small");
        // If inlineMult is 0, inline it
        إذا (v3Global.opt.inlineMult() < 1) cVtx.setDoInline("inlineMult < 1");
        // If it would yield less than the given number of ops, inline it
        ثابت size_t inlinedSize = mVtx.inEdges().size() * mVtx.size();
        ثابت size_t limit = v3Global.opt.inlineMult();
        إذا (inlinedSize < limit) cVtx.setDoInline("inlinedSize < inlineMult");
    }
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-decision");

    // Inline the modules we decided to inline
    ModuleInliner::process(nodep, *graphp);
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("inlinemod-inlined");

    V3Global::dump_Check_Global_شجرة("inline", 0, dump_شجرة_Either_Level() >= 3);
}
