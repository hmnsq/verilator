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

शून्य DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    जइ (otherps.empty()) प्रत्यागम;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ध्रुव VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    प्रति (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    प्रति (ध्रुव std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        प्रति (DfgVertexVar* ध्रुव vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            जइ (DfgVertexVar* ध्रुव altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ध्रुव srcp = vtxp->srcp();
                DfgVertex* ध्रुव defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                जइ (srcp) altp->srcp(srcp);
                जइ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                अनुवर्त;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = एतत्;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        प्रति (DfgVertexAst* ध्रुव vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = एतत्;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        प्रति (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = एतत्;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        प्रति (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = एतत्;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_पङ्क्तिः* flp, ध्रुव std::string& prefix,
                                   ध्रुव DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ध्रुव vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    जइ (dtype.isArray()) प्रत्यागम नूतन DfgVarArray{*एतत्, vscp};
    प्रत्यागम नूतन DfgVarPacked{*एतत्, vscp};
}

स्थिर ध्रुव std::string toDotId(ध्रुव DfgVertex& vtx) { प्रत्यागम '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
स्थिर शून्य dumpDotVertex(std::ostream& os, ध्रुव DfgVertex& vtx) {
    जइ (ध्रुव DfgVertexVar* ध्रुव varVtxp = vtx.cast<DfgVertexVar>()) {
        ध्रुव AstVarScope* ध्रुव vscp = varVtxp->vscp();
        os << toDotId(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        // Address
        os << '\n' << cvtToHex(varVtxp);
        // Original variable, if any
        जइ (ध्रुव AstVarScope* ध्रुव tmpForp = varVtxp->tmpForp()) {
            जइ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        स्थिर ध्रुव वर्ण* ध्रुव rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[मिच्छ][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        जइ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } अण्णहा जइ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } अण्णहा {
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

    जइ (ध्रुव DfgPrev* ध्रुव prevVtxp = vtx.cast<DfgPrev>()) {
        ध्रुव AstVarScope* ध्रुव vscp = prevVtxp->vscp();
        os << toDotId(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        // Address
        os << '\n' << cvtToHex(prevVtxp);
        // Type and fanout
        os << '\n';
        prevVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << prevVtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        जइ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } अण्णहा जइ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } अण्णहा {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ध्रुव वर्ण* ध्रुव colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        प्रत्यागम;
    }

    जइ (ध्रुव DfgConst* ध्रुव constVtxp = vtx.cast<DfgConst>()) {
        ध्रुव V3_सङ्ख्या& अङ्कः = constVtxp->अङ्कः();

        os << toDotId(vtx);
        os << " [label=\"";
        जइ (अङ्कः.width() <= 32 && !अङ्कः.isSigned()) {
            os << constVtxp->width() << "'d" << अङ्कः.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << अङ्कः.toUInt() << std::dec << '\n';
        } अण्णहा {
            os << अङ्कः.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        प्रत्यागम;
    }

    जइ (ध्रुव DfgSel* ध्रुव selVtxp = vtx.cast<DfgSel>()) {
        ध्रुव uint32_t lsb = selVtxp->lsb();
        ध्रुव uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        जइ (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } अण्णहा {
            os << ", shape=circle";
        }
        os << "]\n";
        प्रत्यागम;
    }

    जइ (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        जइ (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } अण्णहा {
            os << ", shape=octagon";
        }
        os << "]\n";
        प्रत्यागम;
    }

    जइ (ध्रुव DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्षः(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ध्रुव वर्ण* ध्रुव colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvtToHex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        प्रत्यागम;
    }

    जइ (ध्रुव DfgVertexAst* ध्रुव astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्षः(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ध्रुव DfgAstRd* ध्रुव astRdVtxp = astVtxp->cast<DfgAstRd>();
        ध्रुव वर्ण* ध्रुव colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->inSenItem()];
        os << "_L"[astRdVtxp->inLoop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        प्रत्यागम;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    जइ (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } अण्णहा {
        os << ", shape=circle";
    }
    os << "]\n";
}

शून्य DfgGraph::dumpDot(std::ostream& os, ध्रुव std::string& label,
                       std::function<तार्किक(ध्रुव DfgVertex&)> p) ध्रुव {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    जइ (!p) p = [](ध्रुव DfgVertex&) { प्रत्यागम सच्च; };

    std::unordered_set<ध्रुव DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ध्रुव DfgVertex& vtx) {
        ध्रुव DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>();
        जइ (!logicp) प्रत्यागम;
        जइ (logicp->synth().empty()) प्रत्यागम;
        जइ (!p(vtx)) प्रत्यागम;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.निवेशय(logicp);
        प्रति (DfgVertex* ध्रुव vtxp : logicp->synth()) {
            जइ (!p(*vtxp)) अनुवर्त;
            dumpDotVertex(os, *vtxp);
            emitted.निवेशय(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ध्रुव DfgVertex& vtx) {
        जइ (emitted.गणनम्(&vtx)) प्रत्यागम;
        जइ (!p(vtx)) प्रत्यागम;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ध्रुव DfgVertex& vtx) {
        जइ (!p(vtx)) प्रत्यागम;
        प्रति (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ध्रुव srcp = vtx.inputp(i);
            जइ (!srcp) अनुवर्त;
            जइ (!p(*srcp)) अनुवर्त;
            os << toDotId(*srcp) << " -> " << toDotId(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.empty() ? "" : "-" + label) << "\"\n";
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
    जइ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

शून्य DfgGraph::dumpDotFilePrefixed(ध्रुव std::string& label,
                                   std::function<तार्किक(ध्रुव DfgVertex&)> p) ध्रुव {
    std::string filename = name();
    जइ (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <तार्किक T_SinksNotSources>
स्थिर std::unique_ptr<std::unordered_set<ध्रुव DfgVertex*>>
dfgGraphCollectCone(ध्रुव std::vector<ध्रुव DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ध्रुव DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ध्रुव DfgVertex*>> resp{
        नूतन std::unordered_set<ध्रुव DfgVertex*>{}};
    // Depth first traversal
    जाव (!queue.empty()) {
        // Pop next work item
        ध्रुव DfgVertex* ध्रुव vtxp = queue.back();
        queue.निष्कासय_back();
        // Mark vertex as visited, move on if already visited
        जइ (!resp->निवेशय(vtxp).second) अनुवर्त;
        // Enqueue all siblings of this vertex.
        जइ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ध्रुव DfgVertex& sink) {
                queue.प्रक्षिप_back(&sink);
                प्रत्यागम मिच्छ;
            });
        } अण्णहा {
            vtxp->foreachSource([&](ध्रुव DfgVertex& src) {
                queue.प्रक्षिप_back(&src);
                प्रत्यागम मिच्छ;
            });
        }
    }
    // Done
    प्रत्यागम resp;
}

std::unique_ptr<std::unordered_set<ध्रुव DfgVertex*>>
DfgGraph::sourceCone(ध्रुव std::vector<ध्रुव DfgVertex*>& vtxps) ध्रुव {
    प्रत्यागम dfgGraphCollectCone<मिच्छ>(vtxps);
}

std::unique_ptr<std::unordered_set<ध्रुव DfgVertex*>>
DfgGraph::sinkCone(ध्रुव std::vector<ध्रुव DfgVertex*>& vtxps) ध्रुव {
    प्रत्यागम dfgGraphCollectCone<सच्च>(vtxps);
}

std::unique_ptr<std::unordered_set<ध्रुव DfgVertex*>>
DfgGraph::neighborhood(ध्रुव std::vector<ध्रुव DfgVertex*>& vtxps, size_t n) ध्रुव {
    // Neighborhood
    std::vector<ध्रुव DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ध्रुव DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    प्रति (size_t hops = 1; hops <= n; ++hops) {
        प्रति (size_t i = begin; i < end; ++i) {
            ध्रुव DfgVertex* ध्रुव vtxp = vec[i];
            vtxp->foreachSink([&](ध्रुव DfgVertex& dst) {
                जइ (res.निवेशय(&dst).second) vec.प्रक्षिप_back(&dst);
                प्रत्यागम मिच्छ;
            });
            vtxp->foreachSource([&](ध्रुव DfgVertex& src) {
                जइ (res.निवेशय(&src).second) vec.प्रक्षिप_back(&src);
                प्रत्यागम मिच्छ;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    प्रत्यागम std::make_unique<std::unordered_set<ध्रुव DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_पङ्क्तिः* flp, ध्रुव DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*एतत्);
}

तार्किक DfgVertex::unsafe() ध्रुव {
    जइ (is<DfgMux>()) प्रत्यागम सच्च;
    जइ (is<DfgArraySel>()) प्रत्यागम !as<DfgArraySel>()->bitp()->is<DfgConst>();
    प्रत्यागम मिच्छ;
}

शून्य DfgVertex::typeCheck(ध्रुव DfgGraph& dfg) ध्रुव {

#define CHECK(cond, सन्देशः) \
    UASSERT_OBJ(cond, एतत्, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << सन्देशः);
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
    पक्ष VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        प्रत्यागम;
    }
    पक्ष VDfgType::SpliceArray:
    पक्ष VDfgType::SplicePacked: {
        ध्रुव DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ध्रुव DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            प्रत्यागम मिच्छ;
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
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        प्रत्यागम;
    }
    पक्ष VDfgType::Sel: {
        ध्रुव DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        प्रत्यागम;
    }
    पक्ष VDfgType::MatchMasked: {
        ध्रुव DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        प्रत्यागम;
    }
    पक्ष VDfgType::Mux: {
        ध्रुव DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        प्रत्यागम;
    }
    पक्ष VDfgType::ArraySel: {
        ध्रुव DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        प्रत्यागम;
    }

    पक्ष VDfgType::Add:
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

    पक्ष VDfgType::ShiftL:
    पक्ष VDfgType::ShiftR:
    पक्ष VDfgType::ShiftRS: {
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
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        प्रत्यागम;
    }

    पक्ष VDfgType::Rep: {
        ध्रुव DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        प्रत्यागम;
    }

    पक्ष VDfgType::StreamL:
    पक्ष VDfgType::StreamR: {
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
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        प्रत्यागम;
    }

    पक्ष VDfgType::Extend:
    पक्ष VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        प्रत्यागम;
    }

    पक्ष VDfgType::LogAnd:
    पक्ष VDfgType::LogEq:
    पक्ष VDfgType::LogIf:
    पक्ष VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
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
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
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
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
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
#undef CHECK
}

uint32_t DfgVertex::fanout() ध्रुव {
    uint32_t result = 0;
    foreachSink([&](ध्रुव DfgVertex&) {
        ++result;
        प्रत्यागम मिच्छ;
    });
    प्रत्यागम result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    जइ (DfgVertexVar* ध्रुव varp = एतत्->cast<DfgVertexVar>()) प्रत्यागम varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    एतत्->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ध्रुव varp = sink.cast<DfgVertexVar>();
        जइ (!varp) प्रत्यागम मिच्छ;

        // Do not use it if value might differ from its drivers
        जइ (varp->isVolatile()) प्रत्यागम मिच्छ;

        // First variable found
        जइ (!resp) {
            resp = varp;
            प्रत्यागम मिच्छ;
        }

        // Prefer those variables that must be kept anyway
        जइ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            जइ (!resp->hasExtRdRefs()) resp = varp;
            प्रत्यागम मिच्छ;
        }
        जइ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            जइ (!resp->hasDfgRefs()) resp = varp;
            प्रत्यागम मिच्छ;
        }

        // Prefer real variables over temporaries
        ध्रुव तार्किक resIsTemp = resp->vscp()->varp()->isTemp();
        ध्रुव तार्किक varIsTemp = varp->vscp()->varp()->isTemp();
        जइ (resIsTemp != varIsTemp) {
            जइ (resIsTemp) resp = varp;
            प्रत्यागम मिच्छ;
        }
        जइ (!resp->tmpForp() != !varp->tmpForp()) {
            जइ (resp->tmpForp()) resp = varp;
            प्रत्यागम मिच्छ;
        }

        // Prefer the earlier one in source order
        ध्रुव File_पङ्क्तिः& oldFlp = *(resp->fileline());
        ध्रुव File_पङ्क्तिः& newFlp = *(varp->fileline());
        जइ (ध्रुव पूर्णाङ्क cmp = oldFlp.operatorCompare(newFlp)) {
            जइ (cmp > 0) resp = varp;
            प्रत्यागम मिच्छ;
        }
        // Prefer the one with the lexically smaller name
        जइ (ध्रुव पूर्णाङ्क cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            जइ (cmp > 0) resp = varp;
            प्रत्यागम मिच्छ;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        प्रत्यागम मिच्छ;
    });
    प्रत्यागम resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, तार्किक tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    जइ (ध्रुव DfgVertexVar* ध्रुव varp = एतत्->cast<DfgVertexVar>()) {
        प्रत्यागम varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    जइ (tryResultVar) {
        जइ (ध्रुव DfgVertexVar* ध्रुव varp = एतत्->getResultVar()) {
            प्रत्यागम varp->vscp()->scopep();
        }
    }

    AstScope* ध्रुव rootp = v3Global.rootp()->शिखरम्_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[एतत्];
    जइ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ध्रुव scp = src.scopep(cache, सच्च);
            जइ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                प्रत्यागम सच्च;
            }
            प्रत्यागम मिच्छ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), एतत्,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    प्रत्यागम resultr;
}

शून्य DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    जाव (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*एतत्);
    // Delete - this will unlink sources
    delete एतत्;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ध्रुव srcp = एतत्->srcp();
    // Not driven at all
    जइ (!srcp) प्रत्यागम {nullptr, 0};
    // If volatile, can have other drivers
    जइ (isVolatile()) प्रत्यागम {nullptr, 0};
    // Don't inline CReset
    जइ (srcp->is<DfgCReset>()) प्रत्यागम {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ध्रुव splicep = srcp->cast<DfgVertexSplice>();
    जइ (!splicep) प्रत्यागम {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ध्रुव uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    तार्किक useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ध्रुव uint32_t dLo) {
        ध्रुव uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        जइ (dLo <= hi && lo <= dHi) useDefault = मिच्छ;
        // If it does not cover the whole searched range, move on
        जइ (lo < dLo || dHi < hi) प्रत्यागम मिच्छ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        प्रत्यागम सच्च;
    });

    // If a single driver covers the searched range, it is the one
    जइ (driverp) प्रत्यागम {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    जइ (useDefault) प्रत्यागम {defaultp(), lo};
    // Not driven by a single vertex
    प्रत्यागम {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

वर्ग DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ध्रुव DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ध्रुव DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    स्थिर std::string toLetters(size_t मानम्, तार्किक lowerCase = मिच्छ) {
        ध्रुव वर्ण base = lowerCase ? 'a' : 'A';
        std::string s;
        करेहि { s += static_cast<वर्ण>(base + मानम् % 26); } जाव (मानम् /= 26);
        प्रत्यागम s;
    }

    ध्रुव std::string& internConst(ध्रुव DfgConst& vtx) {
        ध्रुव auto pair = m_internedConsts.emplace(vtx.अङ्कः().ascii(मिच्छ), "");
        जइ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internSelLsb(uint32_t मानम्) {
        ध्रुव auto pair = m_internedSelLsbs.emplace(मानम्, "");
        जइ (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internWordWidth(uint32_t मानम्) {
        ध्रुव auto pair = m_internedWordWidths.emplace(मानम्, "");
        जइ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, सच्च);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internWideWidth(uint32_t मानम्) {
        ध्रुव auto pair = m_internedWideWidths.emplace(मानम्, "");
        जइ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internVertex(ध्रुव DfgVertex& vtx) {
        ध्रुव auto pair = m_internedVertices.emplace(&vtx, "");
        जइ (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        प्रत्यागम pair.first->second;
    }

    शून्य recordMultiplicityAndDepth(ध्रुव DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& मानम् = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        मानम्.first += 1;
        मानम्.second = std::max(मानम्.second, depth);
        जइ (!depth) प्रत्यागम;
        vtx.foreachSource([&](ध्रुव DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            प्रत्यागम मिच्छ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    शून्य render(ध्रुव DfgVertex& vtx, uint32_t depth, तार्किक is_मूलम्_2 = सच्च) {
        जइ (ध्रुव DfgConst* ध्रुव constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            जइ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अण्णहा जइ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } अण्णहा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अण्णहा जइ (!is_मूलम्_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } अण्णहा जइ (!vtx.foreachSource([&](ध्रुव DfgVertex&) { प्रत्यागम सच्च; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } अण्णहा जइ (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } अण्णहा {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            जइ (ध्रुव DfgSel* ध्रुव selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                जइ (selp->lsb() == 0) {
                    m_os << '0';
                } अण्णहा {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ध्रुव DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, मिच्छ);
                प्रत्यागम मिच्छ;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        जइ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } अण्णहा {
            ध्रुव uint32_t width = vtx.size();
            जइ (width == 1) {
                m_os << '1';
            } अण्णहा जइ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अण्णहा {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        जइ (vtx.hasMultipleSinks()) m_os << '*';
    }

सार्वजनिक:
    DfgPatternString(std::ostream& os, ध्रुव DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, मिच्छ);
        using Pair = std::pair<std::string, ध्रुव DfgVertex*>;
        std::vector<Pair> vertices;
        प्रति (ध्रुव auto& pair : m_multiplicityAndDepth) {
            जइ (pair.second.first == 1) अनुवर्त;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ध्रुव Pair& a, ध्रुव Pair& b) {  //
            प्रत्यागम a.first < b.first;
        });
        प्रति (ध्रुव Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ध्रुव {
    std::ostringstream oss;
    DfgPatternString{oss, *एतत्, depth};
    प्रत्यागम oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#अन्तर्भाव "V3Dfg__gen_visitor_defns.h"  // From ./astgen
