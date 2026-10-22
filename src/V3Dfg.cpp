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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3DfgContext.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ध्रुव string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*एतत्); });
}

शून्य DfgGraph::mergeGraphs(std::सदिशम्<std::unique_ptr<DfgGraph>>&& otherps) {
    यदि (otherps.रिक्तानि()) प्रत्यागम;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    प्रति (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    प्रति (ध्रुव std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        प्रति (DfgVertexVar* ध्रुव vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            यदि (DfgVertexVar* ध्रुव altp = vtxp->vscp()->user2u().गन्तव्यम्<DfgVertexVar*>()) {
                DfgVertex* ध्रुव srcp = vtxp->srcp();
                DfgVertex* ध्रुव defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                यदि (srcp) altp->srcp(srcp);
                यदि (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                अनुवर्त;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#यदि_परिभाषितम् VL_DEBUG
            vtxp->m_dfgp = एतत्;
#यदि_अन्तः
        }
        m_varVertices.splice(m_varVertices.अन्तः_2(), otherp->m_varVertices);
        // Process Ast references
        प्रति (DfgVertexAst* ध्रुव vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#यदि_परिभाषितम् VL_DEBUG
            vtxp->m_dfgp = एतत्;
#यदि_अन्तः
        }
        m_astVertices.splice(m_astVertices.अन्तः_2(), otherp->m_astVertices);
        // Process constants
        प्रति (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#यदि_परिभाषितम् VL_DEBUG
            vtx.m_dfgp = एतत्;
#यदि_अन्तः
        }
        m_constVertices.splice(m_constVertices.अन्तः_2(), otherp->m_constVertices);
        // Process operations
        प्रति (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#यदि_परिभाषितम् VL_DEBUG
            vtx.m_dfgp = एतत्;
#यदि_अन्तः
        }
        m_opVertices.splice(m_opVertices.अन्तः_2(), otherp->m_opVertices);
        // Update graph sizes
        m_आकार += otherp->m_आकार;
        otherp->m_आकार = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_पङ्क्तिः* flp, ध्रुव std::string& उपसर्गः,
                                   ध्रुव Dfg_दत्त_Type& dtype, AstScope* scopep) {
    AstVarScope* ध्रुव vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), उपसर्गः);
    // Create and return the corresponding variable vertex
    यदि (dtype.isArray()) प्रत्यागम नूतन DfgVarArray{*एतत्, vscp};
    प्रत्यागम नूतन DfgVarPacked{*एतत्, vscp};
}

स्थिर ध्रुव std::string गन्तव्यम्_Dot_Id(ध्रुव DfgVertex& vtx) { प्रत्यागम '"' + cvt_गन्तव्यम्_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
स्थिर शून्य dumpDotVertex(std::ostream& os, ध्रुव DfgVertex& vtx) {
    यदि (ध्रुव DfgVertexVar* ध्रुव varVtxp = vtx.cast<DfgVertexVar>()) {
        ध्रुव AstVarScope* ध्रुव vscp = varVtxp->vscp();
        os << गन्तव्यम्_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //स्थानाङ्कः
        os << '\n' << cvt_गन्तव्यम्_Hex(varVtxp);
        // Original variable, if any
        यदि (ध्रुव AstVarScope* ध्रुव tmpForp = varVtxp->tmpForp()) {
            यदि (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        स्थिर ध्रुव वर्ण* ध्रुव rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[असत्य][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        यदि (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } अन्यथा यदि (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } अन्यथा {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ध्रुव वर्ण* ध्रुव colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        प्रत्यागम;
    }

    यदि (ध्रुव Dfg_पूर्व* ध्रुव पूर्व_Vtxp = vtx.cast<Dfg_पूर्व>()) {
        ध्रुव AstVarScope* ध्रुव vscp = पूर्व_Vtxp->vscp();
        os << गन्तव्यम्_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //स्थानाङ्कः
        os << '\n' << cvt_गन्तव्यम्_Hex(पूर्व_Vtxp);
        // Type and fanout
        os << '\n';
        पूर्व_Vtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << पूर्व_Vtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        यदि (पूर्व_Vtxp->isPacked()) {
            os << ", shape=box";
        } अन्यथा यदि (पूर्व_Vtxp->isArray()) {
            os << ", shape=box3d";
        } अन्यथा {
            पूर्व_Vtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ध्रुव वर्ण* ध्रुव colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        प्रत्यागम;
    }

    यदि (ध्रुव DfgConst* ध्रुव constVtxp = vtx.cast<DfgConst>()) {
        ध्रुव V3_सङ्ख्या& अङ्कः = constVtxp->अङ्कः();

        os << गन्तव्यम्_Dot_Id(vtx);
        os << " [label=\"";
        यदि (अङ्कः.विस्तारः() <= 32 && !अङ्कः.isSigned()) {
            os << constVtxp->विस्तारः() << "'d" << अङ्कः.गन्तव्यम्_UInt() << '\n';
            os << constVtxp->विस्तारः() << "'h" << std::hex << अङ्कः.गन्तव्यम्_UInt() << std::dec << '\n';
        } अन्यथा {
            os << अङ्कः.ascii() << '\n';
        }
        os << cvt_गन्तव्यम्_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        प्रत्यागम;
    }

    यदि (ध्रुव DfgSel* ध्रुव selVtxp = vtx.cast<DfgSel>()) {
        ध्रुव uint32_t lsb = selVtxp->lsb();
        ध्रुव uint32_t msb = lsb + selVtxp->विस्तारः() - 1;
        os << गन्तव्यम्_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_गन्तव्यम्_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        यदि (vtx.has_गुणजः_Sinks()) {
            os << ", shape=doublecircle";
        } अन्यथा {
            os << ", shape=circle";
        }
        os << "]\n";
        प्रत्यागम;
    }

    यदि (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << गन्तव्यम्_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_गन्तव्यम्_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        यदि (vtx.has_गुणजः_Sinks()) {
            os << ", shape=doubleoctagon";
        } अन्यथा {
            os << ", shape=octagon";
        }
        os << "]\n";
        प्रत्यागम;
    }

    यदि (ध्रुव DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>()) {
        os << गन्तव्यम्_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्षः(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_गन्तव्यम्_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ध्रुव वर्ण* ध्रुव colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_गन्तव्यम्_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        प्रत्यागम;
    }

    यदि (ध्रुव DfgVertexAst* ध्रुव astVtxp = vtx.cast<DfgVertexAst>()) {
        os << गन्तव्यम्_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्षः(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_गन्तव्यम्_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ध्रुव DfgAstRd* ध्रुव astRdVtxp = astVtxp->cast<DfgAstRd>();
        ध्रुव वर्ण* ध्रुव colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_गन्तव्यम्_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->अन्तर्_Sen_Item()];
        os << "_L"[astRdVtxp->अन्तर्_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        प्रत्यागम;
    }

    os << गन्तव्यम्_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_गन्तव्यम्_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    यदि (vtx.has_गुणजः_Sinks()) {
        os << ", shape=doublecircle";
    } अन्यथा {
        os << ", shape=circle";
    }
    os << "]\n";
}

शून्य DfgGraph::dumpDot(std::ostream& os, ध्रुव std::string& label,
                       std::function<तार्किक(ध्रुव DfgVertex&)> p) ध्रुव {
    // This generates a graphviz dump, https://www.graphviz.org

    //शीर्षकम्
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    यदि (!p) p = [](ध्रुव DfgVertex&) { प्रत्यागम सत्य; };

    std::unordered_समुच्चयः<ध्रुव DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ध्रुव DfgVertex& vtx) {
        ध्रुव DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>();
        यदि (!logicp) प्रत्यागम;
        यदि (logicp->synth().रिक्तानि()) प्रत्यागम;
        यदि (!p(vtx)) प्रत्यागम;
        os << "subgraph cluster_" << cvt_गन्तव्यम्_Hex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.निवेशय(logicp);
        प्रति (DfgVertex* ध्रुव vtxp : logicp->synth()) {
            यदि (!p(*vtxp)) अनुवर्त;
            dumpDotVertex(os, *vtxp);
            emitted.निवेशय(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ध्रुव DfgVertex& vtx) {
        यदि (emitted.गणनम्(&vtx)) प्रत्यागम;
        यदि (!p(vtx)) प्रत्यागम;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ध्रुव DfgVertex& vtx) {
        यदि (!p(vtx)) प्रत्यागम;
        प्रति (आकार_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ध्रुव srcp = vtx.inputp(i);
            यदि (!srcp) अनुवर्त;
            यदि (!p(*srcp)) अनुवर्त;
            os << गन्तव्यम्_Dot_Id(*srcp) << " -> " << गन्तव्यम्_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.रिक्तानि() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string DfgGraph::dumpDotString(ध्रुव std::string& label,
                                    std::function<तार्किक(ध्रुव DfgVertex&)> p) ध्रुव {
    std::stringstream ss;
    dumpDot(ss, label, p);
    प्रत्यागम ss.str();
}

शून्य DfgGraph::dumpDotFile(ध्रुव std::string& filename, ध्रुव std::string& label,
                           std::function<तार्किक(ध्रुव DfgVertex&)> p) ध्रुव {
    ध्रुव std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    यदि (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

शून्य DfgGraph::dumpDotFilePrefixed(ध्रुव std::string& label,
                                   std::function<तार्किक(ध्रुव DfgVertex&)> p) ध्रुव {
    std::string filename = name();
    यदि (!label.रिक्तानि()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

प्रतिरूपम् <तार्किक T_SinksNotSources>
स्थिर std::unique_ptr<std::unordered_समुच्चयः<ध्रुव DfgVertex*>>
dfgGraphCollectCone(ध्रुव std::सदिशम्<ध्रुव DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::सदिशम्<ध्रुव DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_समुच्चयः<ध्रुव DfgVertex*>> resp{
        नूतन std::unordered_समुच्चयः<ध्रुव DfgVertex*>{}};
    // Depth first traversal
    यावत् (!queue.रिक्तानि()) {
        // Pop next work item
        ध्रुव DfgVertex* ध्रुव vtxp = queue.पृष्ठभागः();
        queue.निष्कासय_पृष्ठभागः();
        // Mark vertex as visited, move on if already visited
        यदि (!resp->निवेशय(vtxp).second) अनुवर्त;
        // Enqueue all siblings of this vertex.
        यदि VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ध्रुव DfgVertex& sink) {
                queue.प्रक्षिप_पृष्ठभागः(&sink);
                प्रत्यागम असत्य;
            });
        } अन्यथा {
            vtxp->foreachSource([&](ध्रुव DfgVertex& src) {
                queue.प्रक्षिप_पृष्ठभागः(&src);
                प्रत्यागम असत्य;
            });
        }
    }
    //समाप्तम्
    प्रत्यागम resp;
}

std::unique_ptr<std::unordered_समुच्चयः<ध्रुव DfgVertex*>>
DfgGraph::sourceCone(ध्रुव std::सदिशम्<ध्रुव DfgVertex*>& vtxps) ध्रुव {
    प्रत्यागम dfgGraphCollectCone<असत्य>(vtxps);
}

std::unique_ptr<std::unordered_समुच्चयः<ध्रुव DfgVertex*>>
DfgGraph::sinkCone(ध्रुव std::सदिशम्<ध्रुव DfgVertex*>& vtxps) ध्रुव {
    प्रत्यागम dfgGraphCollectCone<सत्य>(vtxps);
}

std::unique_ptr<std::unordered_समुच्चयः<ध्रुव DfgVertex*>>
DfgGraph::neighborhood(ध्रुव std::सदिशम्<ध्रुव DfgVertex*>& vtxps, आकार_t n) ध्रुव {
    // Neighborhood
    std::सदिशम्<ध्रुव DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_समुच्चयः<ध्रुव DfgVertex*> res{vec.begin(), vec.अन्तः_2()};
    // Expand neihborhood by 'n' hops
    आकार_t begin = 0;
    आकार_t अन्तः_2 = vec.आकार();
    प्रति (आकार_t hops = 1; hops <= n; ++hops) {
        प्रति (आकार_t i = begin; i < अन्तः_2; ++i) {
            ध्रुव DfgVertex* ध्रुव vtxp = vec[i];
            vtxp->foreachSink([&](ध्रुव DfgVertex& dst) {
                यदि (res.निवेशय(&dst).second) vec.प्रक्षिप_पृष्ठभागः(&dst);
                प्रत्यागम असत्य;
            });
            vtxp->foreachSource([&](ध्रुव DfgVertex& src) {
                यदि (res.निवेशय(&src).second) vec.प्रक्षिप_पृष्ठभागः(&src);
                प्रत्यागम असत्य;
            });
        }
        begin = अन्तः_2;
        अन्तः_2 = vec.आकार();
    }
    // Move out the results
    प्रत्यागम std::make_unique<std::unordered_समुच्चयः<ध्रुव DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_पङ्क्तिः* flp, ध्रुव Dfg_दत्त_Type& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.सङ्कलय_Vertex(*एतत्);
}

तार्किक DfgVertex::unsafe() ध्रुव {
    यदि (is<DfgMux>()) प्रत्यागम सत्य;
    यदि (is<DfgArraySel>()) प्रत्यागम !as<DfgArraySel>()->bitp()->is<DfgConst>();
    प्रत्यागम असत्य;
}

शून्य DfgVertex::typeCheck(ध्रुव DfgGraph& dfg) ध्रुव {

#परिभाषय CHECK(cond, सन्देशः_2) \
    UASSERT_OBJ(cond, एतत्, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << सन्देशः_2);
    विकल्प (type()) {
    पक्ष VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        प्रत्यागम;
    }
    पक्ष VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        प्रत्यागम;
    }
    पक्ष VDfgType::AstRd: {
        ध्रुव DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        प्रत्यागम;
    }
    पक्ष VDfgType::VarArray:
    पक्ष VDfgType::VarPacked: {
        ध्रुव DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        प्रत्यागम;
    }
    पक्ष VDfgType::पूर्व: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        प्रत्यागम;
    }
    पक्ष VDfgType::SpliceArray:
    पक्ष VDfgType::SplicePacked: {
        ध्रुव DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ध्रुव DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_दत्त_Type::select(v.dtype(), lo, src.आकार()), "driver");
            प्रत्यागम असत्य;
        });
        प्रत्यागम;
    }
    पक्ष VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        प्रत्यागम;
    }
    पक्ष VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        प्रत्यागम;
    }
    पक्ष VDfgType::UnitArray: {
        ध्रुव DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.आकार() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        प्रत्यागम;
    }
    पक्ष VDfgType::Sel: {
        ध्रुव DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_दत्त_Type::select(v.srcp()->dtype(), v.lsb(), v.आकार()), "sel");
        प्रत्यागम;
    }
    पक्ष VDfgType::मेलनम्_Masked: {
        ध्रुव Dfg_मेलनम्_Masked& v = *as<Dfg_मेलनम्_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.आकार() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        प्रत्यागम;
    }
    पक्ष VDfgType::Mux: {
        ध्रुव DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->आकार() >= v.आकार(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        प्रत्यागम;
    }
    पक्ष VDfgType::ArraySel: {
        ध्रुव DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        प्रत्यागम;
    }

    पक्ष VDfgType::सङ्कलय_2:
    पक्ष VDfgType::And:
    पक्ष VDfgType::Div:
    पक्ष VDfgType::DivS:
    पक्ष VDfgType::ModDiv:
    पक्ष VDfgType::ModDivS:
    पक्ष VDfgType::Mul:
    पक्ष VDfgType::MulS:
    पक्ष VDfgType::Or:
    पक्ष VDfgType::Sub:
    पक्ष VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        प्रत्यागम;
    }

    पक्ष VDfgType::Negate:
    पक्ष VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        प्रत्यागम;
    }

    पक्ष VDfgType::सरणम्_L:
    पक्ष VDfgType::सरणम्_R:
    पक्ष VDfgType::सरणम्_RS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        प्रत्यागम;
    }

    पक्ष VDfgType::Concat: {
        ध्रुव DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.आकार() == v.rhsp()->आकार() + v.lhsp()->आकार(), "Concat result mismatch");
        प्रत्यागम;
    }

    पक्ष VDfgType::Rep: {
        ध्रुव DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.आकार() % v.srcp()->आकार() == 0, "Not a replicate");
        प्रत्यागम;
    }

    पक्ष VDfgType::प्रवाहः_L:
    पक्ष VDfgType::प्रवाहः_R: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        प्रत्यागम;
    }

    पक्ष VDfgType::Eq:
    पक्ष VDfgType::EqCase:
    पक्ष VDfgType::EqWild:
    पक्ष VDfgType::Neq:
    पक्ष VDfgType::NeqCase:
    पक्ष VDfgType::NeqWild:
    पक्ष VDfgType::Gt:
    पक्ष VDfgType::GtS:
    पक्ष VDfgType::Gte:
    पक्ष VDfgType::GteS:
    पक्ष VDfgType::Lt:
    पक्ष VDfgType::LtS:
    पक्ष VDfgType::Lte:
    पक्ष VDfgType::LteS: {
        CHECK(dtype() == Dfg_दत्त_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        प्रत्यागम;
    }

    पक्ष VDfgType::Extend:
    पक्ष VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->आकार() < आकार(), "Operand should be narrower");
        प्रत्यागम;
    }

    पक्ष VDfgType::LogAnd:
    पक्ष VDfgType::LogEq:
    पक्ष VDfgType::LogIf:
    पक्ष VDfgType::LogOr: {
        CHECK(dtype() == Dfg_दत्त_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        प्रत्यागम;
    }

    पक्ष VDfgType::LogNot:
    पक्ष VDfgType::RedAnd:
    पक्ष VDfgType::RedOr:
    पक्ष VDfgType::RedXor:
    पक्ष VDfgType::OneHot:
    पक्ष VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_दत्त_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        प्रत्यागम;
    }

    पक्ष VDfgType::Cond: {
        ध्रुव DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        प्रत्यागम;
    }

    पक्ष VDfgType::गणनम्_Ones_2: {
        ध्रुव Dfg_गणनम्_Ones& v = *as<Dfg_गणनम्_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.आकार() == 32U, "Should yield a 32-bit result");
        प्रत्यागम;
    }

    पक्ष VDfgType::Pow:
    पक्ष VDfgType::PowSS:
    पक्ष VDfgType::PowSU:
    पक्ष VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        प्रत्यागम;
    }
    }
#अपरिभाषय CHECK
}

uint32_t DfgVertex::fanout() ध्रुव {
    uint32_t परिणाम = 0;
    foreachSink([&](ध्रुव DfgVertex&) {
        ++परिणाम;
        प्रत्यागम असत्य;
    });
    प्रत्यागम परिणाम;
}

DfgVertexVar* DfgVertex::get_परिणाम_Var() {
    // It's easy if the vertex is already a variable ...
    यदि (DfgVertexVar* ध्रुव varp = एतत्->cast<DfgVertexVar>()) प्रत्यागम varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = शून्यसूचकः;
    // cppcheck-has-bug-suppress constParameter
    एतत्->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ध्रुव varp = sink.cast<DfgVertexVar>();
        यदि (!varp) प्रत्यागम असत्य;

        // Do not use it if value might differ from its drivers
        यदि (varp->isVolatile()) प्रत्यागम असत्य;

        // First variable found
        यदि (!resp) {
            resp = varp;
            प्रत्यागम असत्य;
        }

        // Prefer those variables that must be kept anyway
        यदि (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            यदि (!resp->hasExtRdRefs()) resp = varp;
            प्रत्यागम असत्य;
        }
        यदि (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            यदि (!resp->hasDfgRefs()) resp = varp;
            प्रत्यागम असत्य;
        }

        // Prefer real variables over temporaries
        ध्रुव तार्किक res_Is_अस्थायी = resp->vscp()->varp()->is_अस्थायी();
        ध्रुव तार्किक var_Is_अस्थायी = varp->vscp()->varp()->is_अस्थायी();
        यदि (res_Is_अस्थायी != var_Is_अस्थायी) {
            यदि (res_Is_अस्थायी) resp = varp;
            प्रत्यागम असत्य;
        }
        यदि (!resp->tmpForp() != !varp->tmpForp()) {
            यदि (resp->tmpForp()) resp = varp;
            प्रत्यागम असत्य;
        }

        // Prefer the earlier one in source order
        ध्रुव File_पङ्क्तिः& oldFlp = *(resp->fileline());
        ध्रुव File_पङ्क्तिः& newFlp = *(varp->fileline());
        यदि (ध्रुव पूर्णाङ्क cmp = oldFlp.operatorCompare(newFlp)) {
            यदि (cmp > 0) resp = varp;
            प्रत्यागम असत्य;
        }
        // Prefer the one with the lexically smaller name
        यदि (ध्रुव पूर्णाङ्क cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            यदि (cmp > 0) resp = varp;
            प्रत्यागम असत्य;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        प्रत्यागम असत्य;
    });
    प्रत्यागम resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, तार्किक try_परिणाम_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    यदि (ध्रुव DfgVertexVar* ध्रुव varp = एतत्->cast<DfgVertexVar>()) {
        प्रत्यागम varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    यदि (try_परिणाम_Var) {
        यदि (ध्रुव DfgVertexVar* ध्रुव varp = एतत्->get_परिणाम_Var()) {
            प्रत्यागम varp->vscp()->scopep();
        }
    }

    AstScope* ध्रुव rootp = v3Global.rootp()->शिखरम्_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[एतत्];
    यदि (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = शून्यसूचकः;
        foreachSource([&](DfgVertex& src) {
            AstScope* ध्रुव scp = src.scopep(cache, सत्य);
            यदि (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                प्रत्यागम सत्य;
            }
            प्रत्यागम असत्य;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), एतत्,
                "DfgVertex::scopep called on graph with circular operations");

    //समाप्तम्
    प्रत्यागम resultr;
}

शून्य DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    यावत् (!m_sinks.रिक्तानि()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*एतत्);
    // Delete - this will unlink sources
    विलोपय एतत्;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driver_सम्बन्धि_Range(uint32_t lo, uint32_t आकार) {
    DfgVertex* ध्रुव srcp = एतत्->srcp();
    // Not driven at all
    यदि (!srcp) प्रत्यागम {शून्यसूचकः, 0};
    // If volatile, can have other drivers
    यदि (isVolatile()) प्रत्यागम {शून्यसूचकः, 0};
    // Don't inline CReset
    यदि (srcp->is<DfgCReset>()) प्रत्यागम {शून्यसूचकः, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ध्रुव splicep = srcp->cast<DfgVertexSplice>();
    यदि (!splicep) प्रत्यागम {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ध्रुव uint32_t hi = lo + आकार - 1;
    DfgVertex* driverp = शून्यसूचकः;
    uint32_t driverLo = 0;
    तार्किक useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ध्रुव uint32_t dLo) {
        ध्रुव uint32_t dHi = dLo + src.आकार() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        यदि (dLo <= hi && lo <= dHi) useDefault = असत्य;
        // If it does not cover the whole searched range, move on
        यदि (lo < dLo || dHi < hi) प्रत्यागम असत्य;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        प्रत्यागम सत्य;
    });

    // If a single driver covers the searched range, it is the one
    यदि (driverp) प्रत्यागम {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    यदि (useDefault) प्रत्यागम {defaultp(), lo};
    // Not driven by a single vertex
    प्रत्यागम {शून्यसूचकः, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

वर्ग Dfg_आदर्शः_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ध्रुव DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ध्रुव DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_गभीरता;

    स्थिर std::string गन्तव्यम्_Letters(आकार_t मानम्, तार्किक अधस्तनम्_Case = असत्य) {
        ध्रुव वर्ण आधारः = अधस्तनम्_Case ? 'a' : 'A';
        std::string s;
        कुरु { s += static_cast<वर्ण>(आधारः + मानम् % 26); } यावत् (मानम् /= 26);
        प्रत्यागम s;
    }

    ध्रुव std::string& internConst(ध्रुव DfgConst& vtx) {
        ध्रुव स्वचालित pair = m_internedConsts.emplace(vtx.अङ्कः().ascii(असत्य), "");
        यदि (pair.second) pair.first->second += गन्तव्यम्_Letters(m_internedConsts.आकार() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internSelLsb(uint32_t मानम्) {
        ध्रुव स्वचालित pair = m_internedSelLsbs.emplace(मानम्, "");
        यदि (pair.second) pair.first->second += गन्तव्यम्_Letters(m_internedSelLsbs.आकार() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& intern_Word_विस्तारः(uint32_t मानम्) {
        ध्रुव स्वचालित pair = m_internedWordWidths.emplace(मानम्, "");
        यदि (pair.second) pair.first->second += गन्तव्यम्_Letters(m_internedWordWidths.आकार() - 1, सत्य);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& intern_Wide_विस्तारः(uint32_t मानम्) {
        ध्रुव स्वचालित pair = m_internedWideWidths.emplace(मानम्, "");
        यदि (pair.second) pair.first->second += गन्तव्यम्_Letters(m_internedWideWidths.आकार() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internVertex(ध्रुव DfgVertex& vtx) {
        ध्रुव स्वचालित pair = m_internedVertices.emplace(&vtx, "");
        यदि (pair.second) pair.first->second += गन्तव्यम्_Letters(m_internedVertices.आकार() - 1);
        प्रत्यागम pair.first->second;
    }

    शून्य लेखः_Multiplicity_And_गभीरता(ध्रुव DfgVertex& vtx, uint32_t गभीरता) {
        std::pair<uint32_t, uint32_t>& मानम् = m_multiplicity_And_गभीरता
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, गभीरता))
                                                   .first->second;
        मानम्.first += 1;
        मानम्.second = std::max(मानम्.second, गभीरता);
        यदि (!गभीरता) प्रत्यागम;
        vtx.foreachSource([&](ध्रुव DfgVertex& src) {
            लेखः_Multiplicity_And_गभीरता(src, गभीरता - 1);
            प्रत्यागम असत्य;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    शून्य render(ध्रुव DfgVertex& vtx, uint32_t गभीरता, तार्किक is_मूलम्_2 = सत्य) {
        यदि (ध्रुव DfgConst* ध्रुव constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            यदि (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अन्यथा यदि (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } अन्यथा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अन्यथा यदि (!is_मूलम्_2 && m_multiplicity_And_गभीरता.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } अन्यथा यदि (!vtx.foreachSource([&](ध्रुव DfgVertex&) { प्रत्यागम सत्य; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } अन्यथा यदि (गभीरता == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } अन्यथा {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            यदि (ध्रुव DfgSel* ध्रुव selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                यदि (selp->lsb() == 0) {
                    m_os << '0';
                } अन्यथा {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ध्रुव DfgVertex& src) {
                m_os << ' ';
                render(src, गभीरता - 1, असत्य);
                प्रत्यागम असत्य;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        यदि (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } अन्यथा {
            ध्रुव uint32_t विस्तारः = vtx.आकार();
            यदि (विस्तारः == 1) {
                m_os << '1';
            } अन्यथा यदि (विस्तारः <= VL_QUADSIZE) {
                m_os << intern_Word_विस्तारः(विस्तारः);
            } अन्यथा {
                m_os << intern_Wide_विस्तारः(विस्तारः);
            }
        }

        // Mark it if it has multiple sinks
        यदि (vtx.has_गुणजः_Sinks()) m_os << '*';
    }

सार्वजनिक:
    Dfg_आदर्शः_String(std::ostream& os, ध्रुव DfgVertex& vtx, uint32_t गभीरता)
        : m_os{os} {
        लेखः_Multiplicity_And_गभीरता(vtx, गभीरता);
        render(vtx, गभीरता, असत्य);
        उपयुज्य Pair = std::pair<std::string, ध्रुव DfgVertex*>;
        std::सदिशम्<Pair> vertices;
        प्रति (ध्रुव स्वचालित& pair : m_multiplicity_And_गभीरता) {
            यदि (pair.second.first == 1) अनुवर्त;
            vertices.emplace_पृष्ठभागः(internVertex(*pair.first), pair.first);
        }
        std::क्रमय(vertices.begin(), vertices.अन्तः_2(), [](ध्रुव Pair& a, ध्रुव Pair& b) {  //
            प्रत्यागम a.first < b.first;
        });
        प्रति (ध्रुव Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicity_And_गभीरता.at(pair.second).second);
        }
    }
};

std::string DfgVertex::आदर्शः_String(uint32_t गभीरता) ध्रुव {
    std::ostringstream oss;
    Dfg_आदर्शः_String{oss, *एतत्, गभीरता};
    प्रत्यागम oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#अन्तर्भाव "V3Dfg__gen_visitor_defns.h"  // From ./astgen
