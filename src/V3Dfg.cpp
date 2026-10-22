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

DfgGraph::DfgGraph(V3DfgContext& ctx, स्थिरांक string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*हे); });
}

शून्य DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    जर (otherps.empty()) परत;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    स्थिरांक VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    साठी (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    साठी (स्थिरांक std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        साठी (DfgVertexVar* स्थिरांक vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            जर (DfgVertexVar* स्थिरांक altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* स्थिरांक srcp = vtxp->srcp();
                DfgVertex* स्थिरांक defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                जर (srcp) altp->srcp(srcp);
                जर (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                सुरूठेवा;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = हे;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        साठी (DfgVertexAst* स्थिरांक vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = हे;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        साठी (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = हे;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        साठी (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = हे;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_ओळ* flp, स्थिरांक std::string& prefix,
                                   स्थिरांक DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* स्थिरांक vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    जर (dtype.isArray()) परत नवीन DfgVarArray{*हे, vscp};
    परत नवीन DfgVarPacked{*हे, vscp};
}

स्थिर स्थिरांक std::string toDotId(स्थिरांक DfgVertex& vtx) { परत '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
स्थिर शून्य dumpDotVertex(std::ostream& os, स्थिरांक DfgVertex& vtx) {
    जर (स्थिरांक DfgVertexVar* स्थिरांक varVtxp = vtx.cast<DfgVertexVar>()) {
        स्थिरांक AstVarScope* स्थिरांक vscp = varVtxp->vscp();
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
        जर (स्थिरांक AstVarScope* स्थिरांक tmpForp = varVtxp->tmpForp()) {
            जर (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        स्थिर स्थिरांक अक्षर* स्थिरांक rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[असत्य][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        जर (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } अन्यथा जर (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } अन्यथा {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        स्थिरांक अक्षर* स्थिरांक colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        परत;
    }

    जर (स्थिरांक DfgPrev* स्थिरांक prevVtxp = vtx.cast<DfgPrev>()) {
        स्थिरांक AstVarScope* स्थिरांक vscp = prevVtxp->vscp();
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
        जर (prevVtxp->isPacked()) {
            os << ", shape=box";
        } अन्यथा जर (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } अन्यथा {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        स्थिरांक अक्षर* स्थिरांक colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        परत;
    }

    जर (स्थिरांक DfgConst* स्थिरांक constVtxp = vtx.cast<DfgConst>()) {
        स्थिरांक V3_संख्या& अंक = constVtxp->अंक();

        os << toDotId(vtx);
        os << " [label=\"";
        जर (अंक.width() <= 32 && !अंक.isSigned()) {
            os << constVtxp->width() << "'d" << अंक.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << अंक.toUInt() << std::dec << '\n';
        } अन्यथा {
            os << अंक.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        परत;
    }

    जर (स्थिरांक DfgSel* स्थिरांक selVtxp = vtx.cast<DfgSel>()) {
        स्थिरांक uint32_t lsb = selVtxp->lsb();
        स्थिरांक uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        जर (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } अन्यथा {
            os << ", shape=circle";
        }
        os << "]\n";
        परत;
    }

    जर (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        जर (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } अन्यथा {
            os << ", shape=octagon";
        }
        os << "]\n";
        परत;
    }

    जर (स्थिरांक DfgLogic* स्थिरांक logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्ष(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        स्थिरांक अक्षर* स्थिरांक colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        परत;
    }

    जर (स्थिरांक DfgVertexAst* स्थिरांक astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्ष(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        स्थिरांक DfgAstRd* स्थिरांक astRdVtxp = astVtxp->cast<DfgAstRd>();
        स्थिरांक अक्षर* स्थिरांक colorp = astRdVtxp ? "#80ff80"  // Green
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
        परत;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    जर (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } अन्यथा {
        os << ", shape=circle";
    }
    os << "]\n";
}

शून्य DfgGraph::dumpDot(std::ostream& os, स्थिरांक std::string& label,
                       std::function<तार्किक(स्थिरांक DfgVertex&)> p) स्थिरांक {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    जर (!p) p = [](स्थिरांक DfgVertex&) { परत सत्य; };

    std::unordered_set<स्थिरांक DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](स्थिरांक DfgVertex& vtx) {
        स्थिरांक DfgLogic* स्थिरांक logicp = vtx.cast<DfgLogic>();
        जर (!logicp) परत;
        जर (logicp->synth().empty()) परत;
        जर (!p(vtx)) परत;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.घाला(logicp);
        साठी (DfgVertex* स्थिरांक vtxp : logicp->synth()) {
            जर (!p(*vtxp)) सुरूठेवा;
            dumpDotVertex(os, *vtxp);
            emitted.घाला(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](स्थिरांक DfgVertex& vtx) {
        जर (emitted.मोजणी(&vtx)) परत;
        जर (!p(vtx)) परत;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](स्थिरांक DfgVertex& vtx) {
        जर (!p(vtx)) परत;
        साठी (माप_प्रकार i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* स्थिरांक srcp = vtx.inputp(i);
            जर (!srcp) सुरूठेवा;
            जर (!p(*srcp)) सुरूठेवा;
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

std::string DfgGraph::dumpDotString(स्थिरांक std::string& label,
                                    std::function<तार्किक(स्थिरांक DfgVertex&)> p) स्थिरांक {
    std::stringstream ss;
    dumpDot(ss, label, p);
    परत ss.str();
}

शून्य DfgGraph::dumpDotFile(स्थिरांक std::string& filename, स्थिरांक std::string& label,
                           std::function<तार्किक(स्थिरांक DfgVertex&)> p) स्थिरांक {
    स्थिरांक std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    जर (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->बंद();
}

शून्य DfgGraph::dumpDotFilePrefixed(स्थिरांक std::string& label,
                                   std::function<तार्किक(स्थिरांक DfgVertex&)> p) स्थिरांक {
    std::string filename = name();
    जर (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

ढाँचा <तार्किक T_SinksNotSources>
स्थिर std::unique_ptr<std::unordered_set<स्थिरांक DfgVertex*>>
dfgGraphCollectCone(स्थिरांक std::vector<स्थिरांक DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<स्थिरांक DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<स्थिरांक DfgVertex*>> resp{
        नवीन std::unordered_set<स्थिरांक DfgVertex*>{}};
    // Depth first traversal
    पर्यंत (!queue.empty()) {
        // Pop next work item
        स्थिरांक DfgVertex* स्थिरांक vtxp = queue.back();
        queue.काढा_back();
        // Mark vertex as visited, move on if already visited
        जर (!resp->घाला(vtxp).second) सुरूठेवा;
        // Enqueue all siblings of this vertex.
        जर VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](स्थिरांक DfgVertex& sink) {
                queue.ढकला_back(&sink);
                परत असत्य;
            });
        } अन्यथा {
            vtxp->foreachSource([&](स्थिरांक DfgVertex& src) {
                queue.ढकला_back(&src);
                परत असत्य;
            });
        }
    }
    // Done
    परत resp;
}

std::unique_ptr<std::unordered_set<स्थिरांक DfgVertex*>>
DfgGraph::sourceCone(स्थिरांक std::vector<स्थिरांक DfgVertex*>& vtxps) स्थिरांक {
    परत dfgGraphCollectCone<असत्य>(vtxps);
}

std::unique_ptr<std::unordered_set<स्थिरांक DfgVertex*>>
DfgGraph::sinkCone(स्थिरांक std::vector<स्थिरांक DfgVertex*>& vtxps) स्थिरांक {
    परत dfgGraphCollectCone<सत्य>(vtxps);
}

std::unique_ptr<std::unordered_set<स्थिरांक DfgVertex*>>
DfgGraph::neighborhood(स्थिरांक std::vector<स्थिरांक DfgVertex*>& vtxps, माप_प्रकार n) स्थिरांक {
    // Neighborhood
    std::vector<स्थिरांक DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<स्थिरांक DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    माप_प्रकार begin = 0;
    माप_प्रकार end = vec.size();
    साठी (माप_प्रकार hops = 1; hops <= n; ++hops) {
        साठी (माप_प्रकार i = begin; i < end; ++i) {
            स्थिरांक DfgVertex* स्थिरांक vtxp = vec[i];
            vtxp->foreachSink([&](स्थिरांक DfgVertex& dst) {
                जर (res.घाला(&dst).second) vec.ढकला_back(&dst);
                परत असत्य;
            });
            vtxp->foreachSource([&](स्थिरांक DfgVertex& src) {
                जर (res.घाला(&src).second) vec.ढकला_back(&src);
                परत असत्य;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    परत std::make_unique<std::unordered_set<स्थिरांक DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_ओळ* flp, स्थिरांक DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*हे);
}

तार्किक DfgVertex::unsafe() स्थिरांक {
    जर (is<DfgMux>()) परत सत्य;
    जर (is<DfgArraySel>()) परत !as<DfgArraySel>()->bitp()->is<DfgConst>();
    परत असत्य;
}

शून्य DfgVertex::typeCheck(स्थिरांक DfgGraph& dfg) स्थिरांक {

#define CHECK(cond, संदेश) \
    UASSERT_OBJ(cond, हे, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << संदेश);
    निवड (type()) {
    स्थिती VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        परत;
    }
    स्थिती VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        परत;
    }
    स्थिती VDfgType::AstRd: {
        स्थिरांक DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        परत;
    }
    स्थिती VDfgType::VarArray:
    स्थिती VDfgType::VarPacked: {
        स्थिरांक DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        परत;
    }
    स्थिती VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        परत;
    }
    स्थिती VDfgType::SpliceArray:
    स्थिती VDfgType::SplicePacked: {
        स्थिरांक DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](स्थिरांक DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            परत असत्य;
        });
        परत;
    }
    स्थिती VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        परत;
    }
    स्थिती VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        परत;
    }
    स्थिती VDfgType::UnitArray: {
        स्थिरांक DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        परत;
    }
    स्थिती VDfgType::Sel: {
        स्थिरांक DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        परत;
    }
    स्थिती VDfgType::MatchMasked: {
        स्थिरांक DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        परत;
    }
    स्थिती VDfgType::Mux: {
        स्थिरांक DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        परत;
    }
    स्थिती VDfgType::ArraySel: {
        स्थिरांक DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        परत;
    }

    स्थिती VDfgType::Add:
    स्थिती VDfgType::And:
    स्थिती VDfgType::Div:
    स्थिती VDfgType::DivS:
    स्थिती VDfgType::ModDiv:
    स्थिती VDfgType::ModDivS:
    स्थिती VDfgType::Mul:
    स्थिती VDfgType::MulS:
    स्थिती VDfgType::Or:
    स्थिती VDfgType::Sub:
    स्थिती VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        परत;
    }

    स्थिती VDfgType::Negate:
    स्थिती VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        परत;
    }

    स्थिती VDfgType::ShiftL:
    स्थिती VDfgType::ShiftR:
    स्थिती VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        परत;
    }

    स्थिती VDfgType::Concat: {
        स्थिरांक DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        परत;
    }

    स्थिती VDfgType::Rep: {
        स्थिरांक DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        परत;
    }

    स्थिती VDfgType::StreamL:
    स्थिती VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        परत;
    }

    स्थिती VDfgType::Eq:
    स्थिती VDfgType::EqCase:
    स्थिती VDfgType::EqWild:
    स्थिती VDfgType::Neq:
    स्थिती VDfgType::NeqCase:
    स्थिती VDfgType::NeqWild:
    स्थिती VDfgType::Gt:
    स्थिती VDfgType::GtS:
    स्थिती VDfgType::Gte:
    स्थिती VDfgType::GteS:
    स्थिती VDfgType::Lt:
    स्थिती VDfgType::LtS:
    स्थिती VDfgType::Lte:
    स्थिती VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        परत;
    }

    स्थिती VDfgType::Extend:
    स्थिती VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        परत;
    }

    स्थिती VDfgType::LogAnd:
    स्थिती VDfgType::LogEq:
    स्थिती VDfgType::LogIf:
    स्थिती VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        परत;
    }

    स्थिती VDfgType::LogNot:
    स्थिती VDfgType::RedAnd:
    स्थिती VDfgType::RedOr:
    स्थिती VDfgType::RedXor:
    स्थिती VDfgType::OneHot:
    स्थिती VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        परत;
    }

    स्थिती VDfgType::Cond: {
        स्थिरांक DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        परत;
    }

    स्थिती VDfgType::मोजणी_Ones_2: {
        स्थिरांक Dfg_मोजणी_Ones& v = *as<Dfg_मोजणी_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        परत;
    }

    स्थिती VDfgType::Pow:
    स्थिती VDfgType::PowSS:
    स्थिती VDfgType::PowSU:
    स्थिती VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        परत;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() स्थिरांक {
    uint32_t result = 0;
    foreachSink([&](स्थिरांक DfgVertex&) {
        ++result;
        परत असत्य;
    });
    परत result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    जर (DfgVertexVar* स्थिरांक varp = हे->cast<DfgVertexVar>()) परत varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    हे->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* स्थिरांक varp = sink.cast<DfgVertexVar>();
        जर (!varp) परत असत्य;

        // Do not use it if value might differ from its drivers
        जर (varp->isVolatile()) परत असत्य;

        // First variable found
        जर (!resp) {
            resp = varp;
            परत असत्य;
        }

        // Prefer those variables that must be kept anyway
        जर (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            जर (!resp->hasExtRdRefs()) resp = varp;
            परत असत्य;
        }
        जर (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            जर (!resp->hasDfgRefs()) resp = varp;
            परत असत्य;
        }

        // Prefer real variables over temporaries
        स्थिरांक तार्किक resIsTemp = resp->vscp()->varp()->isTemp();
        स्थिरांक तार्किक varIsTemp = varp->vscp()->varp()->isTemp();
        जर (resIsTemp != varIsTemp) {
            जर (resIsTemp) resp = varp;
            परत असत्य;
        }
        जर (!resp->tmpForp() != !varp->tmpForp()) {
            जर (resp->tmpForp()) resp = varp;
            परत असत्य;
        }

        // Prefer the earlier one in source order
        स्थिरांक File_ओळ& oldFlp = *(resp->fileline());
        स्थिरांक File_ओळ& newFlp = *(varp->fileline());
        जर (स्थिरांक पूर्णांक cmp = oldFlp.operatorCompare(newFlp)) {
            जर (cmp > 0) resp = varp;
            परत असत्य;
        }
        // Prefer the one with the lexically smaller name
        जर (स्थिरांक पूर्णांक cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            जर (cmp > 0) resp = varp;
            परत असत्य;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        परत असत्य;
    });
    परत resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, तार्किक tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    जर (स्थिरांक DfgVertexVar* स्थिरांक varp = हे->cast<DfgVertexVar>()) {
        परत varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    जर (tryResultVar) {
        जर (स्थिरांक DfgVertexVar* स्थिरांक varp = हे->getResultVar()) {
            परत varp->vscp()->scopep();
        }
    }

    AstScope* स्थिरांक rootp = v3Global.rootp()->शीर्ष_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[हे];
    जर (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* स्थिरांक scp = src.scopep(cache, सत्य);
            जर (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                परत सत्य;
            }
            परत असत्य;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), हे,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    परत resultr;
}

शून्य DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    पर्यंत (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*हे);
    // Delete - this will unlink sources
    delete हे;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* स्थिरांक srcp = हे->srcp();
    // Not driven at all
    जर (!srcp) परत {nullptr, 0};
    // If volatile, can have other drivers
    जर (isVolatile()) परत {nullptr, 0};
    // Don't inline CReset
    जर (srcp->is<DfgCReset>()) परत {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* स्थिरांक splicep = srcp->cast<DfgVertexSplice>();
    जर (!splicep) परत {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    स्थिरांक uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    तार्किक useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, स्थिरांक uint32_t dLo) {
        स्थिरांक uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        जर (dLo <= hi && lo <= dHi) useDefault = असत्य;
        // If it does not cover the whole searched range, move on
        जर (lo < dLo || dHi < hi) परत असत्य;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        परत सत्य;
    });

    // If a single driver covers the searched range, it is the one
    जर (driverp) परत {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    जर (useDefault) परत {defaultp(), lo};
    // Not driven by a single vertex
    परत {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

वर्ग DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<स्थिरांक DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<स्थिरांक DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    स्थिर std::string toLetters(माप_प्रकार मूल्य, तार्किक lowerCase = असत्य) {
        स्थिरांक अक्षर base = lowerCase ? 'a' : 'A';
        std::string s;
        करा { s += static_cast<अक्षर>(base + मूल्य % 26); } पर्यंत (मूल्य /= 26);
        परत s;
    }

    स्थिरांक std::string& internConst(स्थिरांक DfgConst& vtx) {
        स्थिरांक स्वतः pair = m_internedConsts.emplace(vtx.अंक().ascii(असत्य), "");
        जर (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        परत pair.first->second;
    }

    स्थिरांक std::string& internSelLsb(uint32_t मूल्य) {
        स्थिरांक स्वतः pair = m_internedSelLsbs.emplace(मूल्य, "");
        जर (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        परत pair.first->second;
    }

    स्थिरांक std::string& internWordWidth(uint32_t मूल्य) {
        स्थिरांक स्वतः pair = m_internedWordWidths.emplace(मूल्य, "");
        जर (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, सत्य);
        परत pair.first->second;
    }

    स्थिरांक std::string& internWideWidth(uint32_t मूल्य) {
        स्थिरांक स्वतः pair = m_internedWideWidths.emplace(मूल्य, "");
        जर (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        परत pair.first->second;
    }

    स्थिरांक std::string& internVertex(स्थिरांक DfgVertex& vtx) {
        स्थिरांक स्वतः pair = m_internedVertices.emplace(&vtx, "");
        जर (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        परत pair.first->second;
    }

    शून्य recordMultiplicityAndDepth(स्थिरांक DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& मूल्य = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        मूल्य.first += 1;
        मूल्य.second = std::max(मूल्य.second, depth);
        जर (!depth) परत;
        vtx.foreachSource([&](स्थिरांक DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            परत असत्य;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    शून्य render(स्थिरांक DfgVertex& vtx, uint32_t depth, तार्किक is_मूळ_2 = सत्य) {
        जर (स्थिरांक DfgConst* स्थिरांक constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            जर (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अन्यथा जर (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } अन्यथा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अन्यथा जर (!is_मूळ_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } अन्यथा जर (!vtx.foreachSource([&](स्थिरांक DfgVertex&) { परत सत्य; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } अन्यथा जर (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } अन्यथा {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            जर (स्थिरांक DfgSel* स्थिरांक selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                जर (selp->lsb() == 0) {
                    m_os << '0';
                } अन्यथा {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](स्थिरांक DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, असत्य);
                परत असत्य;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        जर (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } अन्यथा {
            स्थिरांक uint32_t width = vtx.size();
            जर (width == 1) {
                m_os << '1';
            } अन्यथा जर (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अन्यथा {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        जर (vtx.hasMultipleSinks()) m_os << '*';
    }

सार्वजनिक:
    DfgPatternString(std::ostream& os, स्थिरांक DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, असत्य);
        using Pair = std::pair<std::string, स्थिरांक DfgVertex*>;
        std::vector<Pair> vertices;
        साठी (स्थिरांक स्वतः& pair : m_multiplicityAndDepth) {
            जर (pair.second.first == 1) सुरूठेवा;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](स्थिरांक Pair& a, स्थिरांक Pair& b) {  //
            परत a.first < b.first;
        });
        साठी (स्थिरांक Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) स्थिरांक {
    std::ostringstream oss;
    DfgPatternString{oss, *हे, depth};
    परत oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#अन्तर्भाव "V3Dfg__gen_visitor_defns.h"  // From ./astgen
