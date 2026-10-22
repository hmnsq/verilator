// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert AstModule to DfgGraph
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
//
// Convert and AstModule (before V3Scope), or the entire AstNetlist
// (after V3Scope) to an initial DfgGraph composed onlyof DfgLogic,
// DfgUnresolved and DfgVertexVar vertices. This will later be synthesized
// into primitive operations by V3DfgPasses::synthesize.
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Cfg.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग Ast_गन्तव्यम्_Dfg_सङ्कलय_Ast_Refs final : सार्वजनिक VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ध्रुव std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    तार्किक m_अन्तर्_Sen_Item = असत्य;  // Inside an AstSenItem
    तार्किक m_अन्तर्_Loop = असत्य;  // Inside an AstLoop

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    शून्य दर्शय(AstSenItem* nodep) override {
        VL_RESTORER(m_अन्तर्_Sen_Item);
        m_अन्तर्_Sen_Item = सत्य;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstLoop* nodep) override {
        VL_RESTORER(m_अन्तर्_Loop);
        m_अन्तर्_Loop = सत्य;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        यदि (nodep->classOrPackagep()) प्रत्यागम;
        // If target is not supported, ignore
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        यदि (!V3Dfg::isSupported(vscp)) प्रत्यागम;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ध्रुव varp = m_getVarVertex(vscp);
        यदि (!varp) प्रत्यागम;
        // Create Ast reference vertices
        यदि (nodep->access().isReadOnly()) {
            DfgAstRd* ध्रुव astp = नूतन DfgAstRd{m_dfg, nodep, m_अन्तर्_Sen_Item, m_अन्तर्_Loop};
            astp->srcp(varp);
            प्रत्यागम;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::समुच्चयः_Has_Mod_Wr_Refs(vscp);
    }

    Ast_गन्तव्यम्_Dfg_सङ्कलय_Ast_Refs(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        Ast_गन्तव्यम्_Dfg_सङ्कलय_Ast_Refs{dfg, nodep, getVarVertex};
    }
};

शून्य V3DfgPasses::सङ्कलय_Ast_Refs(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    Ast_गन्तव्यम्_Dfg_सङ्कलय_Ast_Refs::apply(dfg, nodep, getVarVertex);
}

वर्ग Ast_गन्तव्यम्_Dfg_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ध्रुव VNUser2_अन्तर्_Use m_user2_अन्तर्_Use;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3_Dfg_Ast_गन्तव्यम्_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = शून्यसूचकः;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    शून्य markReferenced(Ast_ग्रन्थिः* nodep) {
        V3DfgPasses::सङ्कलय_Ast_Refs(m_dfg, nodep, [एतत्](AstVarScope* vscp) {  //
            प्रत्यागम getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        यदि (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ध्रुव vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(नूतन DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(नूतन DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        प्रत्यागम varp->user2u().प्रतिरूपम् गन्तव्यम्<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::सदिशम्<DfgVertexVar*>> gatherWritten(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
        std::unique_ptr<std::सदिशम्<DfgVertexVar*>> resp{नूतन std::सदिशम्<DfgVertexVar*>{}};
        ध्रुव तार्किक abort = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक {
            यदि (refp->access().isReadOnly()) प्रत्यागम असत्य;
            AstVarScope* ध्रुव vscp = refp->varScopep();
            यदि (!V3Dfg::isSupported(vscp)) प्रत्यागम सत्य;
            यदि (!vscp->user3_समुच्चयः_Once()) resp->emplace_पृष्ठभागः(getVarVertex(vscp));
            प्रत्यागम असत्य;
        });
        यदि (abort) {
            ++m_ctx.m_nonRepVar;
            प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::सदिशम्<DfgVertexVar*>> gatherRead(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
        std::unique_ptr<std::सदिशम्<DfgVertexVar*>> resp{नूतन std::सदिशम्<DfgVertexVar*>{}};
        ध्रुव तार्किक abort = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक {
            यदि (refp->access().isWriteOnly()) प्रत्यागम असत्य;
            AstVarScope* ध्रुव vscp = refp->varScopep();
            यदि (!V3Dfg::isSupported(vscp)) प्रत्यागम सत्य;
            यदि (!vscp->user3_समुच्चयः_Once()) resp->emplace_पृष्ठभागः(getVarVertex(vscp));
            प्रत्यागम असत्य;
        });
        यदि (abort) {
            ++m_ctx.m_nonRepVar;
            प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::सदिशम्<DfgVertexVar*>> gather_जीवितम्(ध्रुव CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::सदिशम्<AstVarScope*>> varps = V3Cfg::जीवितम्_Var_Scopes(cfg);
        यदि (!varps) {
            ++m_ctx.m_non_Rep_जीवितम्;
            प्रत्यागम शून्यसूचकः;
        }

        // Convert to vertics
        ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
        std::unique_ptr<std::सदिशम्<DfgVertexVar*>> resp{नूतन std::सदिशम्<DfgVertexVar*>{}};
        resp->reserve(varps->आकार());
        प्रति (AstVarScope* ध्रुव varp : *varps) {
            यदि (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                प्रत्यागम शून्यसूचकः;
            }
            UASSERT_OBJ(!varp->user3_समुच्चयः_Once(), varp, "Live variables should be unique");
            resp->emplace_पृष्ठभागः(getVarVertex(varp));
        }
        प्रत्यागम resp;
    }

    // Connect inputs and outputs of a DfgLogic
    शून्य connect(DfgLogic& vtx, ध्रुव std::सदिशम्<DfgVertexVar*>& iVarps,
                 ध्रुव std::सदिशम्<DfgVertexVar*>& oVarps) {
        // Connect inputs
        प्रति (DfgVertexVar* ध्रुव iVarp : iVarps) vtx.सङ्कलय_निवेशः(iVarp);
        // Connect outputs
        प्रति (DfgVertexVar* ध्रुव oVarp : oVarps) {
            यदि (!oVarp->srcp()) oVarp->srcp(नूतन DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->सङ्कलय_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    तार्किक convert(AstAlways* nodep) {
        ध्रुव VAlwaysKwd kwd = nodep->keyword();
        यदि (kwd == VAlwaysKwd::CONT_नियोजय) {
            // TODO: simplify once CFG analysis can handle arrays
            यदि (ध्रुव Ast_नियोजय_W* ध्रुव ap = VN_CAST(nodep->stmtsp(), नियोजय_W)) {
                यदि (ap->nextp()) प्रत्यागम असत्य;
                // Cannot handle assignment with timing control
                यदि (ap->timingControlp()) प्रत्यागम असत्य;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ध्रुव std::unique_ptr<std::सदिशम्<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                यदि (!oVarpsp) प्रत्यागम असत्य;
                // Gather read variables, give up if any are not supported
                ध्रुव std::unique_ptr<std::सदिशम्<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                यदि (!iVarpsp) प्रत्यागम असत्य;
                // Create the DfgLogic
                DfgLogic* ध्रुव logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, शून्यसूचकः};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //समाप्तम्
                ++m_ctx.m_representable;
                प्रत्यागम सत्य;
            }
        }

        // Can only handle combinational logic
        यदि (nodep->sentreep()) प्रत्यागम असत्य;
        यदि (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_नियोजय) {
            प्रत्यागम असत्य;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        यदि (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            प्रत्यागम असत्य;
        }
        // Gather written variables, give up if any are not supported
        ध्रुव std::unique_ptr<std::सदिशम्<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        यदि (!oVarpsp) प्रत्यागम असत्य;
        // Gather read variables, give up if any are not supported
        ध्रुव std::unique_ptr<std::सदिशम्<DfgVertexVar*>> iVarpsp = gather_जीवितम्(*cfgp);
        यदि (!iVarpsp) प्रत्यागम असत्य;
        // Create the DfgLogic
        DfgLogic* ध्रुव logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //समाप्तम्
        ++m_ctx.m_representable;
        प्रत्यागम सत्य;
    }

    // VISITORS
    // Unhandled node
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    शून्य दर्शय(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }
    शून्य दर्शय(AstModule* nodep) override { iterate_And_अग्र_Null(nodep->stmtsp()); }
    शून्य दर्शय(AstIface* nodep) override {
        यदि (!nodep->hasVirtualRef()) {
            iterate_And_अग्र_Null(nodep->stmtsp());
        } अन्यथा {
            markReferenced(nodep);
        }
    }
    शून्य दर्शय(Ast_शिखरम्_Scope* nodep) override { iterate(nodep->scopep()); }
    शून्य दर्शय(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_सक्रिय* nodep) override {
        यदि (nodep->hasCombo()) {
            iterateChildren(nodep);
        } अन्यथा {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    शून्य दर्शय(AstCell* nodep) override { markReferenced(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_गन्तव्यम्_Dfg_Visitor(DfgGraph& dfg, AstNetlist& मूलम्, V3_Dfg_Ast_गन्तव्यम्_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&मूलम्);
    }
    VL_UNCOPYABLE(Ast_गन्तव्यम्_Dfg_Visitor);
    VL_UNMOVABLE(Ast_गन्तव्यम्_Dfg_Visitor);

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, AstNetlist& मूलम्, V3_Dfg_Ast_गन्तव्यम्_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_गन्तव्यम्_Dfg_Visitor{dfg, मूलम्, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        प्रति (DfgVertexVar* ध्रुव varp : dfg.varVertices().unlinkable()) {
            यदि (varp->srcp()) अनुवर्त;
            ध्रुव तार्किक keep = varp->foreachSink([&](DfgVertex& d) { प्रत्यागम !d.is<DfgAstRd>(); });
            यदि (!keep) {
                यावत् (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::ast_गन्तव्यम्_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ध्रुव dfgp = नूतन DfgGraph{ctx, "netlist"};
    Ast_गन्तव्यम्_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    प्रत्यागम std::unique_ptr<DfgGraph>{dfgp};
}
