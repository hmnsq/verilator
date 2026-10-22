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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"

#समावेश "V3Ast.h"
#समावेश "V3DfgContext.h"
#समावेश "V3EmitV.h"
#समावेश "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, स्थिर string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*यह); });
}

व्योम DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    अगर (otherps.empty()) वापस;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    स्थिर VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    क्रम (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    क्रम (स्थिर std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        क्रम (DfgVertexVar* स्थिर vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            अगर (DfgVertexVar* स्थिर altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* स्थिर srcp = vtxp->srcp();
                DfgVertex* स्थिर defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                अगर (srcp) altp->srcp(srcp);
                अगर (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                जारी;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#अगर_घोषित VL_DEBUG
            vtxp->m_dfgp = यह;
#पूर्ण_अगर
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        क्रम (DfgVertexAst* स्थिर vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#अगर_घोषित VL_DEBUG
            vtxp->m_dfgp = यह;
#पूर्ण_अगर
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        क्रम (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#अगर_घोषित VL_DEBUG
            vtx.m_dfgp = यह;
#पूर्ण_अगर
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        क्रम (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#अगर_घोषित VL_DEBUG
            vtx.m_dfgp = यह;
#पूर्ण_अगर
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_आकार += otherp->m_आकार;
        otherp->m_आकार = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_पंक्ति* flp, स्थिर std::string& prefix,
                                   स्थिर Dfg_तथ्य_Type& dtype, AstScope* scopep) {
    AstVarScope* स्थिर vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    अगर (dtype.isArray()) वापस नया DfgVarArray{*यह, vscp};
    वापस नया DfgVarPacked{*यह, vscp};
}

अटल स्थिर std::string toDotId(स्थिर DfgVertex& vtx) { वापस '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
अटल व्योम dumpDotVertex(std::ब_प्रवाह& os, स्थिर DfgVertex& vtx) {
    अगर (स्थिर DfgVertexVar* स्थिर varVtxp = vtx.cast<DfgVertexVar>()) {
        स्थिर AstVarScope* स्थिर vscp = varVtxp->vscp();
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
        अगर (स्थिर AstVarScope* स्थिर tmpForp = varVtxp->tmpForp()) {
            अगर (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        अटल स्थिर अक्षर* स्थिर rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[false][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        अगर (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } अन्यथा अगर (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } अन्यथा {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        स्थिर अक्षर* स्थिर colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        वापस;
    }

    अगर (स्थिर Dfg_पूर्व* स्थिर पूर्व_Vtxp = vtx.cast<Dfg_पूर्व>()) {
        स्थिर AstVarScope* स्थिर vscp = पूर्व_Vtxp->vscp();
        os << toDotId(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        // Address
        os << '\n' << cvtToHex(पूर्व_Vtxp);
        // Type and fanout
        os << '\n';
        पूर्व_Vtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << पूर्व_Vtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        अगर (पूर्व_Vtxp->isPacked()) {
            os << ", shape=box";
        } अन्यथा अगर (पूर्व_Vtxp->isArray()) {
            os << ", shape=box3d";
        } अन्यथा {
            पूर्व_Vtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        स्थिर अक्षर* स्थिर colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        वापस;
    }

    अगर (स्थिर DfgConst* स्थिर constVtxp = vtx.cast<DfgConst>()) {
        स्थिर V3_संख्या& अंक = constVtxp->अंक();

        os << toDotId(vtx);
        os << " [label=\"";
        अगर (अंक.width() <= 32 && !अंक.isSigned()) {
            os << constVtxp->width() << "'d" << अंक.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::षष्ठ << अंक.toUInt() << std::डेसी << '\n';
        } अन्यथा {
            os << अंक.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        वापस;
    }

    अगर (स्थिर DfgSel* स्थिर selVtxp = vtx.cast<DfgSel>()) {
        स्थिर uint32_t lsb = selVtxp->lsb();
        स्थिर uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        अगर (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } अन्यथा {
            os << ", shape=circle";
        }
        os << "]\n";
        वापस;
    }

    अगर (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        अगर (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } अन्यथा {
            os << ", shape=octagon";
        }
        os << "]\n";
        वापस;
    }

    अगर (स्थिर DfgLogic* स्थिर logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्ष(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        स्थिर अक्षर* स्थिर colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        वापस;
    }

    अगर (स्थिर DfgVertexAst* स्थिर astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_वृक्ष(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        स्थिर DfgAstRd* स्थिर astRdVtxp = astVtxp->cast<DfgAstRd>();
        स्थिर अक्षर* स्थिर colorp = astRdVtxp ? "#80ff80"  // Green
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
        वापस;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    अगर (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } अन्यथा {
        os << ", shape=circle";
    }
    os << "]\n";
}

व्योम DfgGraph::dumpDot(std::ब_प्रवाह& os, स्थिर std::string& label,
                       std::function<bool(स्थिर DfgVertex&)> p) स्थिर {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    अगर (!p) p = [](स्थिर DfgVertex&) { वापस true; };

    std::unordered_set<स्थिर DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](स्थिर DfgVertex& vtx) {
        स्थिर DfgLogic* स्थिर logicp = vtx.cast<DfgLogic>();
        अगर (!logicp) वापस;
        अगर (logicp->synth().empty()) वापस;
        अगर (!p(vtx)) वापस;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.जोड़ो(logicp);
        क्रम (DfgVertex* स्थिर vtxp : logicp->synth()) {
            अगर (!p(*vtxp)) जारी;
            dumpDotVertex(os, *vtxp);
            emitted.जोड़ो(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](स्थिर DfgVertex& vtx) {
        अगर (emitted.गिनती(&vtx)) वापस;
        अगर (!p(vtx)) वापस;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](स्थिर DfgVertex& vtx) {
        अगर (!p(vtx)) वापस;
        क्रम (माप_प्रकार i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* स्थिर srcp = vtx.inputp(i);
            अगर (!srcp) जारी;
            अगर (!p(*srcp)) जारी;
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

std::string DfgGraph::dumpDotString(स्थिर std::string& label,
                                    std::function<bool(स्थिर DfgVertex&)> p) स्थिर {
    std::stringstream ss;
    dumpDot(ss, label, p);
    वापस ss.str();
}

व्योम DfgGraph::dumpDotFile(स्थिर std::string& filename, स्थिर std::string& label,
                           std::function<bool(स्थिर DfgVertex&)> p) स्थिर {
    स्थिर std::unique_ptr<std::बख_प्रवाह> os{V3File::new_ofstream(filename)};
    अगर (os->हार()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.लो(), label, p);
    os->बंद();
}

व्योम DfgGraph::dumpDotFilePrefixed(स्थिर std::string& label,
                                   std::function<bool(स्थिर DfgVertex&)> p) स्थिर {
    std::string filename = name();
    अगर (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

ढाँचा <bool T_SinksNotSources>
अटल std::unique_ptr<std::unordered_set<स्थिर DfgVertex*>>
dfgGraphCollectCone(स्थिर std::vector<स्थिर DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<स्थिर DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<स्थिर DfgVertex*>> resp{
        नया std::unordered_set<स्थिर DfgVertex*>{}};
    // Depth first traversal
    जबतक (!queue.empty()) {
        // Pop next work item
        स्थिर DfgVertex* स्थिर vtxp = queue.back();
        queue.निकालो_back();
        // Mark vertex as visited, move on if already visited
        अगर (!resp->जोड़ो(vtxp).second) जारी;
        // Enqueue all siblings of this vertex.
        अगर VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](स्थिर DfgVertex& sink) {
                queue.धकेलो_back(&sink);
                वापस false;
            });
        } अन्यथा {
            vtxp->foreachSource([&](स्थिर DfgVertex& src) {
                queue.धकेलो_back(&src);
                वापस false;
            });
        }
    }
    // Done
    वापस resp;
}

std::unique_ptr<std::unordered_set<स्थिर DfgVertex*>>
DfgGraph::sourceCone(स्थिर std::vector<स्थिर DfgVertex*>& vtxps) स्थिर {
    वापस dfgGraphCollectCone<false>(vtxps);
}

std::unique_ptr<std::unordered_set<स्थिर DfgVertex*>>
DfgGraph::sinkCone(स्थिर std::vector<स्थिर DfgVertex*>& vtxps) स्थिर {
    वापस dfgGraphCollectCone<true>(vtxps);
}

std::unique_ptr<std::unordered_set<स्थिर DfgVertex*>>
DfgGraph::neighborhood(स्थिर std::vector<स्थिर DfgVertex*>& vtxps, माप_प्रकार n) स्थिर {
    // Neighborhood
    std::vector<स्थिर DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<स्थिर DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    माप_प्रकार begin = 0;
    माप_प्रकार end = vec.आकार();
    क्रम (माप_प्रकार hops = 1; hops <= n; ++hops) {
        क्रम (माप_प्रकार i = begin; i < end; ++i) {
            स्थिर DfgVertex* स्थिर vtxp = vec[i];
            vtxp->foreachSink([&](स्थिर DfgVertex& dst) {
                अगर (res.जोड़ो(&dst).second) vec.धकेलो_back(&dst);
                वापस false;
            });
            vtxp->foreachSource([&](स्थिर DfgVertex& src) {
                अगर (res.जोड़ो(&src).second) vec.धकेलो_back(&src);
                वापस false;
            });
        }
        begin = end;
        end = vec.आकार();
    }
    // Move out the results
    वापस std::make_unique<std::unordered_set<स्थिर DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*यह);
}

bool DfgVertex::unsafe() स्थिर {
    अगर (is<DfgMux>()) वापस true;
    अगर (is<DfgArraySel>()) वापस !as<DfgArraySel>()->bitp()->is<DfgConst>();
    वापस false;
}

व्योम DfgVertex::typeCheck(स्थिर DfgGraph& dfg) स्थिर {

#घोषणा CHECK(cond, संदेश) \
    UASSERT_OBJ(cond, यह, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << संदेश);
    चयन (type()) {
    हाल VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        वापस;
    }
    हाल VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        वापस;
    }
    हाल VDfgType::AstRd: {
        स्थिर DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        वापस;
    }
    हाल VDfgType::VarArray:
    हाल VDfgType::VarPacked: {
        स्थिर DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        वापस;
    }
    हाल VDfgType::पूर्व: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        वापस;
    }
    हाल VDfgType::SpliceArray:
    हाल VDfgType::SplicePacked: {
        स्थिर DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](स्थिर DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_तथ्य_Type::select(v.dtype(), lo, src.आकार()), "driver");
            वापस false;
        });
        वापस;
    }
    हाल VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        वापस;
    }
    हाल VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        वापस;
    }
    हाल VDfgType::UnitArray: {
        स्थिर DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.आकार() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        वापस;
    }
    हाल VDfgType::Sel: {
        स्थिर DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_तथ्य_Type::select(v.srcp()->dtype(), v.lsb(), v.आकार()), "sel");
        वापस;
    }
    हाल VDfgType::MatchMasked: {
        स्थिर DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.आकार() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        वापस;
    }
    हाल VDfgType::Mux: {
        स्थिर DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->आकार() >= v.आकार(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        वापस;
    }
    हाल VDfgType::ArraySel: {
        स्थिर DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        वापस;
    }

    हाल VDfgType::Add:
    हाल VDfgType::And:
    हाल VDfgType::Div:
    हाल VDfgType::DivS:
    हाल VDfgType::ModDiv:
    हाल VDfgType::ModDivS:
    हाल VDfgType::Mul:
    हाल VDfgType::MulS:
    हाल VDfgType::Or:
    हाल VDfgType::Sub:
    हाल VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        वापस;
    }

    हाल VDfgType::Negate:
    हाल VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        वापस;
    }

    हाल VDfgType::ShiftL:
    हाल VDfgType::ShiftR:
    हाल VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        वापस;
    }

    हाल VDfgType::Concat: {
        स्थिर DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.आकार() == v.rhsp()->आकार() + v.lhsp()->आकार(), "Concat result mismatch");
        वापस;
    }

    हाल VDfgType::Rep: {
        स्थिर DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.आकार() % v.srcp()->आकार() == 0, "Not a replicate");
        वापस;
    }

    हाल VDfgType::StreamL:
    हाल VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        वापस;
    }

    हाल VDfgType::Eq:
    हाल VDfgType::EqCase:
    हाल VDfgType::EqWild:
    हाल VDfgType::Neq:
    हाल VDfgType::NeqCase:
    हाल VDfgType::NeqWild:
    हाल VDfgType::Gt:
    हाल VDfgType::GtS:
    हाल VDfgType::Gte:
    हाल VDfgType::GteS:
    हाल VDfgType::Lt:
    हाल VDfgType::LtS:
    हाल VDfgType::Lte:
    हाल VDfgType::LteS: {
        CHECK(dtype() == Dfg_तथ्य_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        वापस;
    }

    हाल VDfgType::Extend:
    हाल VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->आकार() < आकार(), "Operand should be narrower");
        वापस;
    }

    हाल VDfgType::LogAnd:
    हाल VDfgType::LogEq:
    हाल VDfgType::LogIf:
    हाल VDfgType::LogOr: {
        CHECK(dtype() == Dfg_तथ्य_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        वापस;
    }

    हाल VDfgType::LogNot:
    हाल VDfgType::RedAnd:
    हाल VDfgType::RedOr:
    हाल VDfgType::RedXor:
    हाल VDfgType::OneHot:
    हाल VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_तथ्य_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        वापस;
    }

    हाल VDfgType::Cond: {
        स्थिर DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        वापस;
    }

    हाल VDfgType::गिनती_Ones_2: {
        स्थिर Dfg_गिनती_Ones& v = *as<Dfg_गिनती_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.आकार() == 32U, "Should yield a 32-bit result");
        वापस;
    }

    हाल VDfgType::Pow:
    हाल VDfgType::PowSS:
    हाल VDfgType::PowSU:
    हाल VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        वापस;
    }
    }
#अघोषित CHECK
}

uint32_t DfgVertex::fanout() स्थिर {
    uint32_t परिणाम = 0;
    foreachSink([&](स्थिर DfgVertex&) {
        ++परिणाम;
        वापस false;
    });
    वापस परिणाम;
}

DfgVertexVar* DfgVertex::get_परिणाम_Var() {
    // It's easy if the vertex is already a variable ...
    अगर (DfgVertexVar* स्थिर varp = यह->cast<DfgVertexVar>()) वापस varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    यह->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* स्थिर varp = sink.cast<DfgVertexVar>();
        अगर (!varp) वापस false;

        // Do not use it if value might differ from its drivers
        अगर (varp->isVolatile()) वापस false;

        // First variable found
        अगर (!resp) {
            resp = varp;
            वापस false;
        }

        // Prefer those variables that must be kept anyway
        अगर (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            अगर (!resp->hasExtRdRefs()) resp = varp;
            वापस false;
        }
        अगर (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            अगर (!resp->hasDfgRefs()) resp = varp;
            वापस false;
        }

        // Prefer real variables over temporaries
        स्थिर bool res_Is_अस्थायी = resp->vscp()->varp()->is_अस्थायी();
        स्थिर bool var_Is_अस्थायी = varp->vscp()->varp()->is_अस्थायी();
        अगर (res_Is_अस्थायी != var_Is_अस्थायी) {
            अगर (res_Is_अस्थायी) resp = varp;
            वापस false;
        }
        अगर (!resp->tmpForp() != !varp->tmpForp()) {
            अगर (resp->tmpForp()) resp = varp;
            वापस false;
        }

        // Prefer the earlier one in source order
        स्थिर File_पंक्ति& oldFlp = *(resp->fileline());
        स्थिर File_पंक्ति& newFlp = *(varp->fileline());
        अगर (स्थिर पूर्णांक cmp = oldFlp.operatorCompare(newFlp)) {
            अगर (cmp > 0) resp = varp;
            वापस false;
        }
        // Prefer the one with the lexically smaller name
        अगर (स्थिर पूर्णांक cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            अगर (cmp > 0) resp = varp;
            वापस false;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        वापस false;
    });
    वापस resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, bool try_परिणाम_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    अगर (स्थिर DfgVertexVar* स्थिर varp = यह->cast<DfgVertexVar>()) {
        वापस varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    अगर (try_परिणाम_Var) {
        अगर (स्थिर DfgVertexVar* स्थिर varp = यह->get_परिणाम_Var()) {
            वापस varp->vscp()->scopep();
        }
    }

    AstScope* स्थिर rootp = v3Global.rootp()->शीर्ष_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[यह];
    अगर (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* स्थिर scp = src.scopep(cache, true);
            अगर (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                वापस true;
            }
            वापस false;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), यह,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    वापस resultr;
}

व्योम DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    जबतक (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*यह);
    // Delete - this will unlink sources
    मिटाओ यह;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t आकार) {
    DfgVertex* स्थिर srcp = यह->srcp();
    // Not driven at all
    अगर (!srcp) वापस {nullptr, 0};
    // If volatile, can have other drivers
    अगर (isVolatile()) वापस {nullptr, 0};
    // Don't inline CReset
    अगर (srcp->is<DfgCReset>()) वापस {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* स्थिर splicep = srcp->cast<DfgVertexSplice>();
    अगर (!splicep) वापस {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    स्थिर uint32_t hi = lo + आकार - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    bool useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, स्थिर uint32_t dLo) {
        स्थिर uint32_t dHi = dLo + src.आकार() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        अगर (dLo <= hi && lo <= dHi) useDefault = false;
        // If it does not cover the whole searched range, move on
        अगर (lo < dLo || dHi < hi) वापस false;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        वापस true;
    });

    // If a single driver covers the searched range, it is the one
    अगर (driverp) वापस {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    अगर (useDefault) वापस {defaultp(), lo};
    // Not driven by a single vertex
    वापस {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

श्रेणी DfgPatternString final {
    std::ब_प्रवाह& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<स्थिर DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<स्थिर DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    अटल std::string toLetters(माप_प्रकार मूल्य, bool lowerCase = false) {
        स्थिर अक्षर base = lowerCase ? 'a' : 'A';
        std::string s;
        करो { s += static_cast<अक्षर>(base + मूल्य % 26); } जबतक (मूल्य /= 26);
        वापस s;
    }

    स्थिर std::string& internConst(स्थिर DfgConst& vtx) {
        स्थिर स्वतः pair = m_internedConsts.emplace(vtx.अंक().ascii(false), "");
        अगर (pair.second) pair.first->second += toLetters(m_internedConsts.आकार() - 1);
        वापस pair.first->second;
    }

    स्थिर std::string& internSelLsb(uint32_t मूल्य) {
        स्थिर स्वतः pair = m_internedSelLsbs.emplace(मूल्य, "");
        अगर (pair.second) pair.first->second += toLetters(m_internedSelLsbs.आकार() - 1);
        वापस pair.first->second;
    }

    स्थिर std::string& internWordWidth(uint32_t मूल्य) {
        स्थिर स्वतः pair = m_internedWordWidths.emplace(मूल्य, "");
        अगर (pair.second) pair.first->second += toLetters(m_internedWordWidths.आकार() - 1, true);
        वापस pair.first->second;
    }

    स्थिर std::string& internWideWidth(uint32_t मूल्य) {
        स्थिर स्वतः pair = m_internedWideWidths.emplace(मूल्य, "");
        अगर (pair.second) pair.first->second += toLetters(m_internedWideWidths.आकार() - 1);
        वापस pair.first->second;
    }

    स्थिर std::string& internVertex(स्थिर DfgVertex& vtx) {
        स्थिर स्वतः pair = m_internedVertices.emplace(&vtx, "");
        अगर (pair.second) pair.first->second += toLetters(m_internedVertices.आकार() - 1);
        वापस pair.first->second;
    }

    व्योम recordMultiplicityAndDepth(स्थिर DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& मूल्य = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        मूल्य.first += 1;
        मूल्य.second = std::max(मूल्य.second, depth);
        अगर (!depth) वापस;
        vtx.foreachSource([&](स्थिर DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            वापस false;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    व्योम render(स्थिर DfgVertex& vtx, uint32_t depth, bool is_मूल_2 = true) {
        अगर (स्थिर DfgConst* स्थिर constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            अगर (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अन्यथा अगर (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } अन्यथा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अन्यथा अगर (!is_मूल_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } अन्यथा अगर (!vtx.foreachSource([&](स्थिर DfgVertex&) { वापस true; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } अन्यथा अगर (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } अन्यथा {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            अगर (स्थिर DfgSel* स्थिर selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                अगर (selp->lsb() == 0) {
                    m_os << '0';
                } अन्यथा {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](स्थिर DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, false);
                वापस false;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        अगर (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } अन्यथा {
            स्थिर uint32_t width = vtx.आकार();
            अगर (width == 1) {
                m_os << '1';
            } अन्यथा अगर (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अन्यथा {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        अगर (vtx.hasMultipleSinks()) m_os << '*';
    }

खुला:
    DfgPatternString(std::ब_प्रवाह& os, स्थिर DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, false);
        using Pair = std::pair<std::string, स्थिर DfgVertex*>;
        std::vector<Pair> vertices;
        क्रम (स्थिर स्वतः& pair : m_multiplicityAndDepth) {
            अगर (pair.second.first == 1) जारी;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](स्थिर Pair& a, स्थिर Pair& b) {  //
            वापस a.first < b.first;
        });
        क्रम (स्थिर Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) स्थिर {
    std::ostringstream oss;
    DfgPatternString{oss, *यह, depth};
    वापस oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#समावेश "V3Dfg__gen_visitor_defns.h"  // From ./astgen
