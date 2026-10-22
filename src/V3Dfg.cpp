// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Data flow graph (DFG) representation of logic
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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3DfgContext.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

Dfg_లేఖాచిత్రం::Dfg_లేఖాచిత్రం(V3DfgContext& ctx, స్థిరరాశి string& name)
    : m_ctx{ctx}
    , m_name{name} {}

Dfg_లేఖాచిత్రం::~Dfg_లేఖాచిత్రం() {
    for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) { vtx.unlinkDelete(*ఇది); });
}

శూన్య Dfg_లేఖాచిత్రం::విలీనం_Graphs(std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>>&& otherps) {
    ఒకవేళ (otherps.ఖాళీలు()) తిరిగి;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    కొరకు (Dfg_శీర్షం_Var& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    కొరకు (స్థిరరాశి std::unique_ptr<Dfg_లేఖాచిత్రం>& otherp : otherps) {
        // Process variables
        కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి altp = vtxp->vscp()->user2u().గమ్యం<Dfg_శీర్షం_Var*>()) {
                Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
                Dfg_శీర్షం* స్థిరరాశి defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ఒకవేళ (srcp) altp->srcp(srcp);
                ఒకవేళ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                కొనసాగు;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#నిర్వచితమైతే VL_DEBUG
            vtxp->m_dfgp = ఇది;
#అయితే_ముగింపు
        }
        m_varVertices.splice(m_varVertices.చివర(), otherp->m_varVertices);
        // Process Ast references
        కొరకు (Dfg_శీర్షం_Ast* స్థిరరాశి vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#నిర్వచితమైతే VL_DEBUG
            vtxp->m_dfgp = ఇది;
#అయితే_ముగింపు
        }
        m_astVertices.splice(m_astVertices.చివర(), otherp->m_astVertices);
        // Process constants
        కొరకు (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#నిర్వచితమైతే VL_DEBUG
            vtx.m_dfgp = ఇది;
#అయితే_ముగింపు
        }
        m_constVertices.splice(m_constVertices.చివర(), otherp->m_constVertices);
        // Process operations
        కొరకు (Dfg_శీర్షం& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#నిర్వచితమైతే VL_DEBUG
            vtx.m_dfgp = ఇది;
#అయితే_ముగింపు
        }
        m_opVertices.splice(m_opVertices.చివర(), otherp->m_opVertices);
        // Update graph sizes
        m_కొలత += otherp->m_కొలత;
        otherp->m_కొలత = 0;
    }
}

Dfg_శీర్షం_Var* Dfg_లేఖాచిత్రం::makeNewVar(File_పంక్తి* flp, స్థిరరాశి std::string& ఉపసర్గ,
                                   స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, AstScope* scopep) {
    AstVarScope* స్థిరరాశి vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), ఉపసర్గ);
    // Create and return the corresponding variable vertex
    ఒకవేళ (dtype.isArray()) తిరిగి కొత్త DfgVarArray{*ఇది, vscp};
    తిరిగి కొత్త DfgVarPacked{*ఇది, vscp};
}

స్థిర స్థిరరాశి std::string గమ్యం_Dot_Id(స్థిరరాశి Dfg_శీర్షం& vtx) { తిరిగి '"' + cvt_గమ్యం_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
స్థిర శూన్య dump_Dot_శీర్షం(std::ostream& os, స్థిరరాశి Dfg_శీర్షం& vtx) {
    ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varVtxp = vtx.cast<Dfg_శీర్షం_Var>()) {
        స్థిరరాశి AstVarScope* స్థిరరాశి vscp = varVtxp->vscp();
        os << గమ్యం_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //చిరునామా
        os << '\n' << cvt_గమ్యం_Hex(varVtxp);
        // Original variable, if any
        ఒకవేళ (స్థిరరాశి AstVarScope* స్థిరరాశి tmpForp = varVtxp->tmpForp()) {
            ఒకవేళ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[అబద్ధం][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ఒకవేళ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } లేకపోతే ఒకవేళ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } లేకపోతే {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        స్థిరరాశి అక్షరం* స్థిరరాశి colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  //బూడిద
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        తిరిగి;
    }

    ఒకవేళ (స్థిరరాశి DfgPrev* స్థిరరాశి prevVtxp = vtx.cast<DfgPrev>()) {
        స్థిరరాశి AstVarScope* స్థిరరాశి vscp = prevVtxp->vscp();
        os << గమ్యం_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //చిరునామా
        os << '\n' << cvt_గమ్యం_Hex(prevVtxp);
        // Type and fanout
        os << '\n';
        prevVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << prevVtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        ఒకవేళ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } లేకపోతే ఒకవేళ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } లేకపోతే {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        స్థిరరాశి అక్షరం* స్థిరరాశి colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        తిరిగి;
    }

    ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి constVtxp = vtx.cast<DfgConst>()) {
        స్థిరరాశి V3_సంఖ్య& అంకె = constVtxp->అంకె();

        os << గమ్యం_Dot_Id(vtx);
        os << " [label=\"";
        ఒకవేళ (అంకె.వెడల్పు() <= 32 && !అంకె.isSigned()) {
            os << constVtxp->వెడల్పు() << "'d" << అంకె.గమ్యం_UInt() << '\n';
            os << constVtxp->వెడల్పు() << "'h" << std::hex << అంకె.గమ్యం_UInt() << std::dec << '\n';
        } లేకపోతే {
            os << అంకె.ascii() << '\n';
        }
        os << cvt_గమ్యం_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        తిరిగి;
    }

    ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి selVtxp = vtx.cast<DfgSel>()) {
        స్థిరరాశి uint32_t lsb = selVtxp->lsb();
        స్థిరరాశి uint32_t msb = lsb + selVtxp->వెడల్పు() - 1;
        os << గమ్యం_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_గమ్యం_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ఒకవేళ (vtx.has_గుణిజం_Sinks()) {
            os << ", shape=doublecircle";
        } లేకపోతే {
            os << ", shape=circle";
        }
        os << "]\n";
        తిరిగి;
    }

    ఒకవేళ (vtx.is<Dfg_శీర్షం_Splice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << గమ్యం_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_గమ్యం_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ఒకవేళ (vtx.has_గుణిజం_Sinks()) {
            os << ", shape=doubleoctagon";
        } లేకపోతే {
            os << ", shape=octagon";
        }
        os << "]\n";
        తిరిగి;
    }

    ఒకవేళ (స్థిరరాశి DfgLogic* స్థిరరాశి logicp = vtx.cast<DfgLogic>()) {
        os << గమ్యం_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_వృక్షం(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_గమ్యం_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        స్థిరరాశి అక్షరం* స్థిరరాశి colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_గమ్యం_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        తిరిగి;
    }

    ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Ast* స్థిరరాశి astVtxp = vtx.cast<Dfg_శీర్షం_Ast>()) {
        os << గమ్యం_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_వృక్షం(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_గమ్యం_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        స్థిరరాశి DfgAstRd* స్థిరరాశి astRdVtxp = astVtxp->cast<DfgAstRd>();
        స్థిరరాశి అక్షరం* స్థిరరాశి colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_గమ్యం_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->లోపల_Sen_అంశం()];
        os << "_L"[astRdVtxp->లోపల_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        తిరిగి;
    }

    os << గమ్యం_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_గమ్యం_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ఒకవేళ (vtx.has_గుణిజం_Sinks()) {
        os << ", shape=doublecircle";
    } లేకపోతే {
        os << ", shape=circle";
    }
    os << "]\n";
}

శూన్య Dfg_లేఖాచిత్రం::dumpDot(std::ostream& os, స్థిరరాశి std::string& label,
                       std::function<తార్కిక(స్థిరరాశి Dfg_శీర్షం&)> p) స్థిరరాశి {
    // This generates a graphviz dump, https://www.graphviz.org

    //శీర్షిక
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ఒకవేళ (!p) p = [](స్థిరరాశి Dfg_శీర్షం&) { తిరిగి నిజం; };

    std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*> emitted;
    // Emit all vertices associated with a DfgLogic
    for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
        స్థిరరాశి DfgLogic* స్థిరరాశి logicp = vtx.cast<DfgLogic>();
        ఒకవేళ (!logicp) తిరిగి;
        ఒకవేళ (logicp->synth().ఖాళీలు()) తిరిగి;
        ఒకవేళ (!p(vtx)) తిరిగి;
        os << "subgraph cluster_" << cvt_గమ్యం_Hex(logicp) << " {\n";
        dump_Dot_శీర్షం(os, *logicp);
        emitted.చొప్పించు(logicp);
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : logicp->synth()) {
            ఒకవేళ (!p(*vtxp)) కొనసాగు;
            dump_Dot_శీర్షం(os, *vtxp);
            emitted.చొప్పించు(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
        ఒకవేళ (emitted.లెక్క(&vtx)) తిరిగి;
        ఒకవేళ (!p(vtx)) తిరిగి;
        dump_Dot_శీర్షం(os, vtx);
    });
    // Emit all edges
    for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
        ఒకవేళ (!p(vtx)) తిరిగి;
        కొరకు (కొలత_t i = 0; i < vtx.nInputs(); ++i) {
            Dfg_శీర్షం* స్థిరరాశి srcp = vtx.inputp(i);
            ఒకవేళ (!srcp) కొనసాగు;
            ఒకవేళ (!p(*srcp)) కొనసాగు;
            os << గమ్యం_Dot_Id(*srcp) << " -> " << గమ్యం_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.ఖాళీలు() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string Dfg_లేఖాచిత్రం::dumpDotString(స్థిరరాశి std::string& label,
                                    std::function<తార్కిక(స్థిరరాశి Dfg_శీర్షం&)> p) స్థిరరాశి {
    std::stringstream ss;
    dumpDot(ss, label, p);
    తిరిగి ss.str();
}

శూన్య Dfg_లేఖాచిత్రం::dumpDotFile(స్థిరరాశి std::string& filename, స్థిరరాశి std::string& label,
                           std::function<తార్కిక(స్థిరరాశి Dfg_శీర్షం&)> p) స్థిరరాశి {
    స్థిరరాశి std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ఒకవేళ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

శూన్య Dfg_లేఖాచిత్రం::dumpDotFilePrefixed(స్థిరరాశి std::string& label,
                                   std::function<తార్కిక(స్థిరరాశి Dfg_శీర్షం&)> p) స్థిరరాశి {
    std::string filename = name();
    ఒకవేళ (!label.ఖాళీలు()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

మూస <తార్కిక T_SinksNotSources>
స్థిర std::unique_ptr<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>>
dfg_లేఖాచిత్రం_Collect_Cone(స్థిరరాశి std::సదిశ<స్థిరరాశి Dfg_శీర్షం*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::సదిశ<స్థిరరాశి Dfg_శీర్షం*> వరుస = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>> resp{
        కొత్త std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>{}};
    // Depth first traversal
    అయితే (!వరుస.ఖాళీలు()) {
        // Pop next work item
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి vtxp = వరుస.వెనుక();
        వరుస.తీయి_వెనుక();
        // Mark vertex as visited, move on if already visited
        ఒకవేళ (!resp->చొప్పించు(vtxp).రెండవ) కొనసాగు;
        // Enqueue all siblings of this vertex.
        ఒకవేళ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](స్థిరరాశి Dfg_శీర్షం& sink) {
                వరుస.నెట్టు_వెనుక(&sink);
                తిరిగి అబద్ధం;
            });
        } లేకపోతే {
            vtxp->foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
                వరుస.నెట్టు_వెనుక(&src);
                తిరిగి అబద్ధం;
            });
        }
    }
    //పూర్తి
    తిరిగి resp;
}

std::unique_ptr<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>>
Dfg_లేఖాచిత్రం::ఆధారం_Cone(స్థిరరాశి std::సదిశ<స్థిరరాశి Dfg_శీర్షం*>& vtxps) స్థిరరాశి {
    తిరిగి dfg_లేఖాచిత్రం_Collect_Cone<అబద్ధం>(vtxps);
}

std::unique_ptr<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>>
Dfg_లేఖాచిత్రం::sinkCone(స్థిరరాశి std::సదిశ<స్థిరరాశి Dfg_శీర్షం*>& vtxps) స్థిరరాశి {
    తిరిగి dfg_లేఖాచిత్రం_Collect_Cone<నిజం>(vtxps);
}

std::unique_ptr<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>>
Dfg_లేఖాచిత్రం::neighborhood(స్థిరరాశి std::సదిశ<స్థిరరాశి Dfg_శీర్షం*>& vtxps, కొలత_t n) స్థిరరాశి {
    // Neighborhood
    std::సదిశ<స్థిరరాశి Dfg_శీర్షం*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*> res{vec.begin(), vec.చివర()};
    // Expand neihborhood by 'n' hops
    కొలత_t begin = 0;
    కొలత_t చివర = vec.కొలత();
    కొరకు (కొలత_t hops = 1; hops <= n; ++hops) {
        కొరకు (కొలత_t i = begin; i < చివర; ++i) {
            స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి vtxp = vec[i];
            vtxp->foreachSink([&](స్థిరరాశి Dfg_శీర్షం& dst) {
                ఒకవేళ (res.చొప్పించు(&dst).రెండవ) vec.నెట్టు_వెనుక(&dst);
                తిరిగి అబద్ధం;
            });
            vtxp->foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
                ఒకవేళ (res.చొప్పించు(&src).రెండవ) vec.నెట్టు_వెనుక(&src);
                తిరిగి అబద్ధం;
            });
        }
        begin = చివర;
        చివర = vec.కొలత();
    }
    // Move out the results
    తిరిగి std::make_unique<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

Dfg_శీర్షం::Dfg_శీర్షం(Dfg_లేఖాచిత్రం& dfg, VDfgType type, File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dt,
                     Dfg_అంచు* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.కూడు_శీర్షం(*ఇది);
}

తార్కిక Dfg_శీర్షం::unsafe() స్థిరరాశి {
    ఒకవేళ (is<DfgMux>()) తిరిగి నిజం;
    ఒకవేళ (is<DfgArraySel>()) తిరిగి !as<DfgArraySel>()->bitp()->is<DfgConst>();
    తిరిగి అబద్ధం;
}

శూన్య Dfg_శీర్షం::typeCheck(స్థిరరాశి Dfg_లేఖాచిత్రం& dfg) స్థిరరాశి {

#నిర్వచించు CHECK(cond, సందేశం_2) \
    UASSERT_OBJ(cond, ఇది, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << సందేశం_2);
    ఎంపిక (type()) {
    స్థితి VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        తిరిగి;
    }
    స్థితి VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        తిరిగి;
    }
    స్థితి VDfgType::AstRd: {
        స్థిరరాశి DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        తిరిగి;
    }
    స్థితి VDfgType::VarArray:
    స్థితి VDfgType::VarPacked: {
        స్థిరరాశి Dfg_శీర్షం_Var& v = *as<Dfg_శీర్షం_Var>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        తిరిగి;
    }
    స్థితి VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        తిరిగి;
    }
    స్థితి VDfgType::SpliceArray:
    స్థితి VDfgType::SplicePacked: {
        స్థిరరాశి Dfg_శీర్షం_Splice& v = *as<Dfg_శీర్షం_Splice>();
        v.foreachDriver([&](స్థిరరాశి Dfg_శీర్షం& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_దత్తాంశం_Type::select(v.dtype(), lo, src.కొలత()), "driver");
            తిరిగి అబద్ధం;
        });
        తిరిగి;
    }
    స్థితి VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        తిరిగి;
    }
    స్థితి VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        తిరిగి;
    }
    స్థితి VDfgType::UnitArray: {
        స్థిరరాశి DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.కొలత() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        తిరిగి;
    }
    స్థితి VDfgType::Sel: {
        స్థిరరాశి DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_దత్తాంశం_Type::select(v.srcp()->dtype(), v.lsb(), v.కొలత()), "sel");
        తిరిగి;
    }
    స్థితి VDfgType::సరిపోలిక_Masked: {
        స్థిరరాశి Dfg_సరిపోలిక_Masked& v = *as<Dfg_సరిపోలిక_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.కొలత() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<Dfg_శీర్షం_Var>(), "Match should be a variable");
        తిరిగి;
    }
    స్థితి VDfgType::Mux: {
        స్థిరరాశి DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->కొలత() >= v.కొలత(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        తిరిగి;
    }
    స్థితి VDfgType::ArraySel: {
        స్థిరరాశి DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        తిరిగి;
    }

    స్థితి VDfgType::కూడు_2:
    స్థితి VDfgType::And:
    స్థితి VDfgType::Div:
    స్థితి VDfgType::DivS:
    స్థితి VDfgType::ModDiv:
    స్థితి VDfgType::ModDivS:
    స్థితి VDfgType::Mul:
    స్థితి VDfgType::MulS:
    స్థితి VDfgType::Or:
    స్థితి VDfgType::Sub:
    స్థితి VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        తిరిగి;
    }

    స్థితి VDfgType::Negate:
    స్థితి VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        తిరిగి;
    }

    స్థితి VDfgType::జరుపు_L:
    స్థితి VDfgType::జరుపు_R:
    స్థితి VDfgType::జరుపు_RS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        తిరిగి;
    }

    స్థితి VDfgType::Concat: {
        స్థిరరాశి DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.కొలత() == v.rhsp()->కొలత() + v.lhsp()->కొలత(), "Concat result mismatch");
        తిరిగి;
    }

    స్థితి VDfgType::Rep: {
        స్థిరరాశి DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.కొలత() % v.srcp()->కొలత() == 0, "Not a replicate");
        తిరిగి;
    }

    స్థితి VDfgType::ప్రవాహం_L:
    స్థితి VDfgType::ప్రవాహం_R: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        తిరిగి;
    }

    స్థితి VDfgType::Eq:
    స్థితి VDfgType::EqCase:
    స్థితి VDfgType::EqWild:
    స్థితి VDfgType::Neq:
    స్థితి VDfgType::NeqCase:
    స్థితి VDfgType::NeqWild:
    స్థితి VDfgType::Gt:
    స్థితి VDfgType::GtS:
    స్థితి VDfgType::Gte:
    స్థితి VDfgType::GteS:
    స్థితి VDfgType::Lt:
    స్థితి VDfgType::LtS:
    స్థితి VDfgType::Lte:
    స్థితి VDfgType::LteS: {
        CHECK(dtype() == Dfg_దత్తాంశం_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        తిరిగి;
    }

    స్థితి VDfgType::Extend:
    స్థితి VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->కొలత() < కొలత(), "Operand should be narrower");
        తిరిగి;
    }

    స్థితి VDfgType::LogAnd:
    స్థితి VDfgType::LogEq:
    స్థితి VDfgType::LogIf:
    స్థితి VDfgType::LogOr: {
        CHECK(dtype() == Dfg_దత్తాంశం_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        తిరిగి;
    }

    స్థితి VDfgType::LogNot:
    స్థితి VDfgType::RedAnd:
    స్థితి VDfgType::RedOr:
    స్థితి VDfgType::RedXor:
    స్థితి VDfgType::OneHot:
    స్థితి VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_దత్తాంశం_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        తిరిగి;
    }

    స్థితి VDfgType::Cond: {
        స్థిరరాశి DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        తిరిగి;
    }

    స్థితి VDfgType::లెక్క_Ones_2: {
        స్థిరరాశి Dfg_లెక్క_Ones& v = *as<Dfg_లెక్క_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.కొలత() == 32U, "Should yield a 32-bit result");
        తిరిగి;
    }

    స్థితి VDfgType::Pow:
    స్థితి VDfgType::PowSS:
    స్థితి VDfgType::PowSU:
    స్థితి VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        తిరిగి;
    }
    }
#నిర్వచనం_రద్దు CHECK
}

uint32_t Dfg_శీర్షం::fanout() స్థిరరాశి {
    uint32_t ఫలితం = 0;
    foreachSink([&](స్థిరరాశి Dfg_శీర్షం&) {
        ++ఫలితం;
        తిరిగి అబద్ధం;
    });
    తిరిగి ఫలితం;
}

Dfg_శీర్షం_Var* Dfg_శీర్షం::get_ఫలితం_Var() {
    // It's easy if the vertex is already a variable ...
    ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి varp = ఇది->cast<Dfg_శీర్షం_Var>()) తిరిగి varp;

    // Inspect existing variables written by this vertex, and choose one
    Dfg_శీర్షం_Var* resp = శూన్యసూచిక;
    // cppcheck-has-bug-suppress constParameter
    ఇది->foreachSink([&resp](Dfg_శీర్షం& sink) {
        Dfg_శీర్షం_Var* స్థిరరాశి varp = sink.cast<Dfg_శీర్షం_Var>();
        ఒకవేళ (!varp) తిరిగి అబద్ధం;

        // Do not use it if value might differ from its drivers
        ఒకవేళ (varp->isVolatile()) తిరిగి అబద్ధం;

        // First variable found
        ఒకవేళ (!resp) {
            resp = varp;
            తిరిగి అబద్ధం;
        }

        // Prefer those variables that must be kept anyway
        ఒకవేళ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ఒకవేళ (!resp->hasExtRdRefs()) resp = varp;
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ఒకవేళ (!resp->hasDfgRefs()) resp = varp;
            తిరిగి అబద్ధం;
        }

        // Prefer real variables over temporaries
        స్థిరరాశి తార్కిక res_Is_తాత్కాలిక = resp->vscp()->varp()->is_తాత్కాలిక();
        స్థిరరాశి తార్కిక var_Is_తాత్కాలిక = varp->vscp()->varp()->is_తాత్కాలిక();
        ఒకవేళ (res_Is_తాత్కాలిక != var_Is_తాత్కాలిక) {
            ఒకవేళ (res_Is_తాత్కాలిక) resp = varp;
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (!resp->tmpForp() != !varp->tmpForp()) {
            ఒకవేళ (resp->tmpForp()) resp = varp;
            తిరిగి అబద్ధం;
        }

        // Prefer the earlier one in source order
        స్థిరరాశి File_పంక్తి& oldFlp = *(resp->fileline());
        స్థిరరాశి File_పంక్తి& newFlp = *(varp->fileline());
        ఒకవేళ (స్థిరరాశి పూర్ణాంకం cmp = oldFlp.operatorCompare(newFlp)) {
            ఒకవేళ (cmp > 0) resp = varp;
            తిరిగి అబద్ధం;
        }
        // Prefer the one with the lexically smaller name
        ఒకవేళ (స్థిరరాశి పూర్ణాంకం cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ఒకవేళ (cmp > 0) resp = varp;
            తిరిగి అబద్ధం;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        తిరిగి అబద్ధం;
    });
    తిరిగి resp;
}

AstScope* Dfg_శీర్షం::scopep(ScopeCache& cache, తార్కిక try_ఫలితం_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = ఇది->cast<Dfg_శీర్షం_Var>()) {
        తిరిగి varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ఒకవేళ (try_ఫలితం_Var) {
        ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = ఇది->get_ఫలితం_Var()) {
            తిరిగి varp->vscp()->scopep();
        }
    }

    AstScope* స్థిరరాశి rootp = v3Global.rootp()->శిఖరం_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[ఇది];
    ఒకవేళ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = శూన్యసూచిక;
        foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            AstScope* స్థిరరాశి scp = src.scopep(cache, నిజం);
            ఒకవేళ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                తిరిగి నిజం;
            }
            తిరిగి అబద్ధం;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), ఇది,
                "DfgVertex::scopep called on graph with circular operations");

    //పూర్తి
    తిరిగి resultr;
}

శూన్య Dfg_శీర్షం::unlinkDelete(Dfg_లేఖాచిత్రం& dfg) {
    // Unlink sink edges
    అయితే (!m_sinks.ఖాళీలు()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.remove_శీర్షం(*ఇది);
    // Delete - this will unlink sources
    తొలగించు ఇది;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<Dfg_శీర్షం*, uint32_t> Dfg_శీర్షం_Var::driver_యొక్క_Range(uint32_t lo, uint32_t కొలత) {
    Dfg_శీర్షం* స్థిరరాశి srcp = ఇది->srcp();
    // Not driven at all
    ఒకవేళ (!srcp) తిరిగి {శూన్యసూచిక, 0};
    // If volatile, can have other drivers
    ఒకవేళ (isVolatile()) తిరిగి {శూన్యసూచిక, 0};
    // Don't inline CReset
    ఒకవేళ (srcp->is<DfgCReset>()) తిరిగి {శూన్యసూచిక, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    Dfg_శీర్షం_Splice* స్థిరరాశి splicep = srcp->cast<Dfg_శీర్షం_Splice>();
    ఒకవేళ (!splicep) తిరిగి {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    స్థిరరాశి uint32_t hi = lo + కొలత - 1;
    Dfg_శీర్షం* driverp = శూన్యసూచిక;
    uint32_t driverLo = 0;
    తార్కిక useDefault = defaultp();
    splicep->foreachDriver([&](Dfg_శీర్షం& src, స్థిరరాశి uint32_t dLo) {
        స్థిరరాశి uint32_t dHi = dLo + src.కొలత() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ఒకవేళ (dLo <= hi && lo <= dHi) useDefault = అబద్ధం;
        // If it does not cover the whole searched range, move on
        ఒకవేళ (lo < dLo || dHi < hi) తిరిగి అబద్ధం;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        తిరిగి నిజం;
    });

    // If a single driver covers the searched range, it is the one
    ఒకవేళ (driverp) తిరిగి {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ఒకవేళ (useDefault) తిరిగి {defaultp(), lo};
    // Not driven by a single vertex
    తిరిగి {శూన్యసూచిక, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

తరగతి Dfg_నమూనా_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_interned_మాట_Widths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<స్థిరరాశి Dfg_శీర్షం*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<స్థిరరాశి Dfg_శీర్షం*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_లోతు;

    స్థిర std::string గమ్యం_Letters(కొలత_t విలువ, తార్కిక క్రింది_Case = అబద్ధం) {
        స్థిరరాశి అక్షరం పునాది = క్రింది_Case ? 'a' : 'A';
        std::string s;
        చేయి { s += static_cast<అక్షరం>(పునాది + విలువ % 26); } అయితే (విలువ /= 26);
        తిరిగి s;
    }

    స్థిరరాశి std::string& internConst(స్థిరరాశి DfgConst& vtx) {
        స్థిరరాశి స్వయంచాలక pair = m_internedConsts.emplace(vtx.అంకె().ascii(అబద్ధం), "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_internedConsts.కొలత() - 1);
        తిరిగి pair.మొదటి->రెండవ;
    }

    స్థిరరాశి std::string& internSelLsb(uint32_t విలువ) {
        స్థిరరాశి స్వయంచాలక pair = m_internedSelLsbs.emplace(విలువ, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_internedSelLsbs.కొలత() - 1);
        తిరిగి pair.మొదటి->రెండవ;
    }

    స్థిరరాశి std::string& intern_మాట_వెడల్పు(uint32_t విలువ) {
        స్థిరరాశి స్వయంచాలక pair = m_interned_మాట_Widths.emplace(విలువ, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_interned_మాట_Widths.కొలత() - 1, నిజం);
        తిరిగి pair.మొదటి->రెండవ;
    }

    స్థిరరాశి std::string& intern_Wide_వెడల్పు(uint32_t విలువ) {
        స్థిరరాశి స్వయంచాలక pair = m_internedWideWidths.emplace(విలువ, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_internedWideWidths.కొలత() - 1);
        తిరిగి pair.మొదటి->రెండవ;
    }

    స్థిరరాశి std::string& intern_శీర్షం(స్థిరరాశి Dfg_శీర్షం& vtx) {
        స్థిరరాశి స్వయంచాలక pair = m_internedVertices.emplace(&vtx, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_internedVertices.కొలత() - 1);
        తిరిగి pair.మొదటి->రెండవ;
    }

    శూన్య రికార్డు_Multiplicity_And_లోతు(స్థిరరాశి Dfg_శీర్షం& vtx, uint32_t లోతు) {
        std::pair<uint32_t, uint32_t>& విలువ = m_multiplicity_And_లోతు
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, లోతు))
                                                   .మొదటి->రెండవ;
        విలువ.మొదటి += 1;
        విలువ.రెండవ = std::max(విలువ.రెండవ, లోతు);
        ఒకవేళ (!లోతు) తిరిగి;
        vtx.foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
            రికార్డు_Multiplicity_And_లోతు(src, లోతు - 1);
            తిరిగి అబద్ధం;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    శూన్య render(స్థిరరాశి Dfg_శీర్షం& vtx, uint32_t లోతు, తార్కిక is_మూలం_2 = నిజం) {
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ఒకవేళ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } లేకపోతే ఒకవేళ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } లేకపోతే {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } లేకపోతే ఒకవేళ (!is_మూలం_2 && m_multiplicity_And_లోతు.at(&vtx).మొదటి > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << intern_శీర్షం(vtx);
        } లేకపోతే ఒకవేళ (!vtx.foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం&) { తిరిగి నిజం; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } లేకపోతే ఒకవేళ (లోతు == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } లేకపోతే {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ఒకవేళ (selp->lsb() == 0) {
                    m_os << '0';
                } లేకపోతే {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
                m_os << ' ';
                render(src, లోతు - 1, అబద్ధం);
                తిరిగి అబద్ధం;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ఒకవేళ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } లేకపోతే {
            స్థిరరాశి uint32_t వెడల్పు = vtx.కొలత();
            ఒకవేళ (వెడల్పు == 1) {
                m_os << '1';
            } లేకపోతే ఒకవేళ (వెడల్పు <= VL_QUADSIZE) {
                m_os << intern_మాట_వెడల్పు(వెడల్పు);
            } లేకపోతే {
                m_os << intern_Wide_వెడల్పు(వెడల్పు);
            }
        }

        // Mark it if it has multiple sinks
        ఒకవేళ (vtx.has_గుణిజం_Sinks()) m_os << '*';
    }

బహిరంగ:
    Dfg_నమూనా_String(std::ostream& os, స్థిరరాశి Dfg_శీర్షం& vtx, uint32_t లోతు)
        : m_os{os} {
        రికార్డు_Multiplicity_And_లోతు(vtx, లోతు);
        render(vtx, లోతు, అబద్ధం);
        ఉపయోగించి Pair = std::pair<std::string, స్థిరరాశి Dfg_శీర్షం*>;
        std::సదిశ<Pair> vertices;
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_multiplicity_And_లోతు) {
            ఒకవేళ (pair.రెండవ.మొదటి == 1) కొనసాగు;
            vertices.emplace_వెనుక(intern_శీర్షం(*pair.మొదటి), pair.మొదటి);
        }
        std::క్రమీకరించు(vertices.begin(), vertices.చివర(), [](స్థిరరాశి Pair& a, స్థిరరాశి Pair& b) {  //
            తిరిగి a.మొదటి < b.మొదటి;
        });
        కొరకు (స్థిరరాశి Pair& pair : vertices) {
            m_os << " | " << pair.మొదటి << " is ";
            render(*pair.రెండవ, m_multiplicity_And_లోతు.at(pair.రెండవ).రెండవ);
        }
    }
};

std::string Dfg_శీర్షం::నమూనా_String(uint32_t లోతు) స్థిరరాశి {
    std::ostringstream oss;
    Dfg_నమూనా_String{oss, *ఇది, లోతు};
    తిరిగి oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#అన్తర్భావ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
