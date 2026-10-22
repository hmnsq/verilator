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

DfgGraph::DfgGraph(V3DfgContext& ctx, अचर string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*एतत्); });
}

शून्य DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    यदि (otherps.empty()) फर्काऊ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    अचर VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    लागि (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    लागि (अचर std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        लागि (DfgVertexVar* अचर vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            यदि (DfgVertexVar* अचर altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* अचर srcp = vtxp->srcp();
                DfgVertex* अचर defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                यदि (srcp) altp->srcp(srcp);
                यदि (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                जारी;
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
        लागि (DfgVertexAst* अचर vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = एतत्;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        लागि (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = एतत्;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        लागि (DfgVertex& vtx : otherp->m_opVertices) {
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

DfgVertexVar* DfgGraph::makeNewVar(File_पङ्क्ति* flp, अचर std::string& prefix,
                                   अचर DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* अचर vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    यदि (dtype.isArray()) फर्काऊ नूतन DfgVarArray{*एतत्, vscp};
    फर्काऊ नूतन DfgVarPacked{*एतत्, vscp};
}

स्थिर अचर std::string toDotId(अचर DfgVertex& vtx) { फर्काऊ '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
स्थिर शून्य dumpDotVertex(std::ostream& os, अचर DfgVertex& vtx) {
    यदि (अचर DfgVertexVar* अचर varVtxp = vtx.cast<DfgVertexVar>()) {
        अचर AstVarScope* अचर vscp = varVtxp->vscp();
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
        यदि (अचर AstVarScope* अचर tmpForp = varVtxp->tmpForp()) {
            यदि (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        स्थिर अचर वर्ण* अचर rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
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
        अचर वर्ण* अचर colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        फर्काऊ;
    }

    यदि (अचर DfgPrev* अचर prevVtxp = vtx.cast<DfgPrev>()) {
        अचर AstVarScope* अचर vscp = prevVtxp->vscp();
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
        यदि (prevVtxp->isPacked()) {
            os << ", shape=box";
        } अन्यथा यदि (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } अन्यथा {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        अचर वर्ण* अचर colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        फर्काऊ;
    }

    यदि (अचर DfgConst* अचर constVtxp = vtx.cast<DfgConst>()) {
        अचर V3_सङ्ख्या& अङ्क = constVtxp->अङ्क();

        os << toDotId(vtx);
        os << " [label=\"";
        यदि (अङ्क.width() <= 32 && !अङ्क.isSigned()) {
            os << constVtxp->width() << "'d" << अङ्क.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << अङ्क.toUInt() << std::dec << '\n';
        } अन्यथा {
            os << अङ्क.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        फर्काऊ;
    }

    यदि (अचर DfgSel* अचर selVtxp = vtx.cast<DfgSel>()) {
        अचर uint32_t lsb = selVtxp->lsb();
        अचर uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        यदि (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } अन्यथा {
            os << ", shape=circle";
        }
        os << "]\n";
        फर्काऊ;
    }

    यदि (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        यदि (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } अन्यथा {
            os << ", shape=octagon";
        }
        os << "]\n";
        फर्काऊ;
    }

    यदि (अचर DfgLogic* अचर logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_रूख(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        अचर वर्ण* अचर colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        फर्काऊ;
    }

    यदि (अचर DfgVertexAst* अचर astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_रूख(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        अचर DfgAstRd* अचर astRdVtxp = astVtxp->cast<DfgAstRd>();
        अचर वर्ण* अचर colorp = astRdVtxp ? "#80ff80"  // Green
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
        फर्काऊ;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    यदि (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } अन्यथा {
        os << ", shape=circle";
    }
    os << "]\n";
}

शून्य DfgGraph::dumpDot(std::ostream& os, अचर std::string& label,
                       std::function<तार्किक(अचर DfgVertex&)> p) अचर {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    यदि (!p) p = [](अचर DfgVertex&) { फर्काऊ सत्य; };

    std::unordered_set<अचर DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](अचर DfgVertex& vtx) {
        अचर DfgLogic* अचर logicp = vtx.cast<DfgLogic>();
        यदि (!logicp) फर्काऊ;
        यदि (logicp->synth().empty()) फर्काऊ;
        यदि (!p(vtx)) फर्काऊ;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.घुसाउ(logicp);
        लागि (DfgVertex* अचर vtxp : logicp->synth()) {
            यदि (!p(*vtxp)) जारी;
            dumpDotVertex(os, *vtxp);
            emitted.घुसाउ(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](अचर DfgVertex& vtx) {
        यदि (emitted.गन्ती(&vtx)) फर्काऊ;
        यदि (!p(vtx)) फर्काऊ;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](अचर DfgVertex& vtx) {
        यदि (!p(vtx)) फर्काऊ;
        लागि (माप_प्रकार i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* अचर srcp = vtx.inputp(i);
            यदि (!srcp) जारी;
            यदि (!p(*srcp)) जारी;
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

std::string DfgGraph::dumpDotString(अचर std::string& label,
                                    std::function<तार्किक(अचर DfgVertex&)> p) अचर {
    std::stringstream ss;
    dumpDot(ss, label, p);
    फर्काऊ ss.str();
}

शून्य DfgGraph::dumpDotFile(अचर std::string& filename, अचर std::string& label,
                           std::function<तार्किक(अचर DfgVertex&)> p) अचर {
    अचर std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    यदि (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->बंद();
}

शून्य DfgGraph::dumpDotFilePrefixed(अचर std::string& label,
                                   std::function<तार्किक(अचर DfgVertex&)> p) अचर {
    std::string filename = name();
    यदि (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

ढाँचा <तार्किक T_SinksNotSources>
स्थिर std::unique_ptr<std::unordered_set<अचर DfgVertex*>>
dfgGraphCollectCone(अचर std::vector<अचर DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<अचर DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<अचर DfgVertex*>> resp{
        नूतन std::unordered_set<अचर DfgVertex*>{}};
    // Depth first traversal
    जबसम्म (!queue.empty()) {
        // Pop next work item
        अचर DfgVertex* अचर vtxp = queue.back();
        queue.झिक_back();
        // Mark vertex as visited, move on if already visited
        यदि (!resp->घुसाउ(vtxp).second) जारी;
        // Enqueue all siblings of this vertex.
        यदि VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](अचर DfgVertex& sink) {
                queue.थप_back(&sink);
                फर्काऊ असत्य;
            });
        } अन्यथा {
            vtxp->foreachSource([&](अचर DfgVertex& src) {
                queue.थप_back(&src);
                फर्काऊ असत्य;
            });
        }
    }
    // Done
    फर्काऊ resp;
}

std::unique_ptr<std::unordered_set<अचर DfgVertex*>>
DfgGraph::sourceCone(अचर std::vector<अचर DfgVertex*>& vtxps) अचर {
    फर्काऊ dfgGraphCollectCone<असत्य>(vtxps);
}

std::unique_ptr<std::unordered_set<अचर DfgVertex*>>
DfgGraph::sinkCone(अचर std::vector<अचर DfgVertex*>& vtxps) अचर {
    फर्काऊ dfgGraphCollectCone<सत्य>(vtxps);
}

std::unique_ptr<std::unordered_set<अचर DfgVertex*>>
DfgGraph::neighborhood(अचर std::vector<अचर DfgVertex*>& vtxps, माप_प्रकार n) अचर {
    // Neighborhood
    std::vector<अचर DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<अचर DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    माप_प्रकार begin = 0;
    माप_प्रकार end = vec.size();
    लागि (माप_प्रकार hops = 1; hops <= n; ++hops) {
        लागि (माप_प्रकार i = begin; i < end; ++i) {
            अचर DfgVertex* अचर vtxp = vec[i];
            vtxp->foreachSink([&](अचर DfgVertex& dst) {
                यदि (res.घुसाउ(&dst).second) vec.थप_back(&dst);
                फर्काऊ असत्य;
            });
            vtxp->foreachSource([&](अचर DfgVertex& src) {
                यदि (res.घुसाउ(&src).second) vec.थप_back(&src);
                फर्काऊ असत्य;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    फर्काऊ std::make_unique<std::unordered_set<अचर DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_पङ्क्ति* flp, अचर DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*एतत्);
}

तार्किक DfgVertex::unsafe() अचर {
    यदि (is<DfgMux>()) फर्काऊ सत्य;
    यदि (is<DfgArraySel>()) फर्काऊ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    फर्काऊ असत्य;
}

शून्य DfgVertex::typeCheck(अचर DfgGraph& dfg) अचर {

#define CHECK(cond, सन्देश) \
    UASSERT_OBJ(cond, एतत्, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << सन्देश);
    विकल्प (type()) {
    पक्ष VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        फर्काऊ;
    }
    पक्ष VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        फर्काऊ;
    }
    पक्ष VDfgType::AstRd: {
        अचर DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        फर्काऊ;
    }
    पक्ष VDfgType::VarArray:
    पक्ष VDfgType::VarPacked: {
        अचर DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        फर्काऊ;
    }
    पक्ष VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        फर्काऊ;
    }
    पक्ष VDfgType::SpliceArray:
    पक्ष VDfgType::SplicePacked: {
        अचर DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](अचर DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            फर्काऊ असत्य;
        });
        फर्काऊ;
    }
    पक्ष VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        फर्काऊ;
    }
    पक्ष VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        फर्काऊ;
    }
    पक्ष VDfgType::UnitArray: {
        अचर DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        फर्काऊ;
    }
    पक्ष VDfgType::Sel: {
        अचर DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        फर्काऊ;
    }
    पक्ष VDfgType::MatchMasked: {
        अचर DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        फर्काऊ;
    }
    पक्ष VDfgType::Mux: {
        अचर DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        फर्काऊ;
    }
    पक्ष VDfgType::ArraySel: {
        अचर DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        फर्काऊ;
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
        फर्काऊ;
    }

    पक्ष VDfgType::Negate:
    पक्ष VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        फर्काऊ;
    }

    पक्ष VDfgType::ShiftL:
    पक्ष VDfgType::ShiftR:
    पक्ष VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        फर्काऊ;
    }

    पक्ष VDfgType::Concat: {
        अचर DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        फर्काऊ;
    }

    पक्ष VDfgType::Rep: {
        अचर DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        फर्काऊ;
    }

    पक्ष VDfgType::StreamL:
    पक्ष VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        फर्काऊ;
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
        फर्काऊ;
    }

    पक्ष VDfgType::Extend:
    पक्ष VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        फर्काऊ;
    }

    पक्ष VDfgType::LogAnd:
    पक्ष VDfgType::LogEq:
    पक्ष VDfgType::LogIf:
    पक्ष VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        फर्काऊ;
    }

    पक्ष VDfgType::LogNot:
    पक्ष VDfgType::RedAnd:
    पक्ष VDfgType::RedOr:
    पक्ष VDfgType::RedXor:
    पक्ष VDfgType::OneHot:
    पक्ष VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        फर्काऊ;
    }

    पक्ष VDfgType::Cond: {
        अचर DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        फर्काऊ;
    }

    पक्ष VDfgType::गन्ती_Ones_2: {
        अचर Dfg_गन्ती_Ones& v = *as<Dfg_गन्ती_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        फर्काऊ;
    }

    पक्ष VDfgType::Pow:
    पक्ष VDfgType::PowSS:
    पक्ष VDfgType::PowSU:
    पक्ष VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        फर्काऊ;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() अचर {
    uint32_t result = 0;
    foreachSink([&](अचर DfgVertex&) {
        ++result;
        फर्काऊ असत्य;
    });
    फर्काऊ result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    यदि (DfgVertexVar* अचर varp = एतत्->cast<DfgVertexVar>()) फर्काऊ varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    एतत्->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* अचर varp = sink.cast<DfgVertexVar>();
        यदि (!varp) फर्काऊ असत्य;

        // Do not use it if value might differ from its drivers
        यदि (varp->isVolatile()) फर्काऊ असत्य;

        // First variable found
        यदि (!resp) {
            resp = varp;
            फर्काऊ असत्य;
        }

        // Prefer those variables that must be kept anyway
        यदि (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            यदि (!resp->hasExtRdRefs()) resp = varp;
            फर्काऊ असत्य;
        }
        यदि (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            यदि (!resp->hasDfgRefs()) resp = varp;
            फर्काऊ असत्य;
        }

        // Prefer real variables over temporaries
        अचर तार्किक resIsTemp = resp->vscp()->varp()->isTemp();
        अचर तार्किक varIsTemp = varp->vscp()->varp()->isTemp();
        यदि (resIsTemp != varIsTemp) {
            यदि (resIsTemp) resp = varp;
            फर्काऊ असत्य;
        }
        यदि (!resp->tmpForp() != !varp->tmpForp()) {
            यदि (resp->tmpForp()) resp = varp;
            फर्काऊ असत्य;
        }

        // Prefer the earlier one in source order
        अचर File_पङ्क्ति& oldFlp = *(resp->fileline());
        अचर File_पङ्क्ति& newFlp = *(varp->fileline());
        यदि (अचर पूर्णाङ्क cmp = oldFlp.operatorCompare(newFlp)) {
            यदि (cmp > 0) resp = varp;
            फर्काऊ असत्य;
        }
        // Prefer the one with the lexically smaller name
        यदि (अचर पूर्णाङ्क cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            यदि (cmp > 0) resp = varp;
            फर्काऊ असत्य;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        फर्काऊ असत्य;
    });
    फर्काऊ resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, तार्किक tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    यदि (अचर DfgVertexVar* अचर varp = एतत्->cast<DfgVertexVar>()) {
        फर्काऊ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    यदि (tryResultVar) {
        यदि (अचर DfgVertexVar* अचर varp = एतत्->getResultVar()) {
            फर्काऊ varp->vscp()->scopep();
        }
    }

    AstScope* अचर rootp = v3Global.rootp()->शीर्ष_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[एतत्];
    यदि (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* अचर scp = src.scopep(cache, सत्य);
            यदि (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                फर्काऊ सत्य;
            }
            फर्काऊ असत्य;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), एतत्,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    फर्काऊ resultr;
}

शून्य DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    जबसम्म (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*एतत्);
    // Delete - this will unlink sources
    delete एतत्;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* अचर srcp = एतत्->srcp();
    // Not driven at all
    यदि (!srcp) फर्काऊ {nullptr, 0};
    // If volatile, can have other drivers
    यदि (isVolatile()) फर्काऊ {nullptr, 0};
    // Don't inline CReset
    यदि (srcp->is<DfgCReset>()) फर्काऊ {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* अचर splicep = srcp->cast<DfgVertexSplice>();
    यदि (!splicep) फर्काऊ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    अचर uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    तार्किक useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, अचर uint32_t dLo) {
        अचर uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        यदि (dLo <= hi && lo <= dHi) useDefault = असत्य;
        // If it does not cover the whole searched range, move on
        यदि (lo < dLo || dHi < hi) फर्काऊ असत्य;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        फर्काऊ सत्य;
    });

    // If a single driver covers the searched range, it is the one
    यदि (driverp) फर्काऊ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    यदि (useDefault) फर्काऊ {defaultp(), lo};
    // Not driven by a single vertex
    फर्काऊ {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

वर्ग DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<अचर DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<अचर DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    स्थिर std::string toLetters(माप_प्रकार मान, तार्किक lowerCase = असत्य) {
        अचर वर्ण base = lowerCase ? 'a' : 'A';
        std::string s;
        गर { s += static_cast<वर्ण>(base + मान % 26); } जबसम्म (मान /= 26);
        फर्काऊ s;
    }

    अचर std::string& internConst(अचर DfgConst& vtx) {
        अचर स्वतः pair = m_internedConsts.emplace(vtx.अङ्क().ascii(असत्य), "");
        यदि (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        फर्काऊ pair.first->second;
    }

    अचर std::string& internSelLsb(uint32_t मान) {
        अचर स्वतः pair = m_internedSelLsbs.emplace(मान, "");
        यदि (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        फर्काऊ pair.first->second;
    }

    अचर std::string& internWordWidth(uint32_t मान) {
        अचर स्वतः pair = m_internedWordWidths.emplace(मान, "");
        यदि (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, सत्य);
        फर्काऊ pair.first->second;
    }

    अचर std::string& internWideWidth(uint32_t मान) {
        अचर स्वतः pair = m_internedWideWidths.emplace(मान, "");
        यदि (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        फर्काऊ pair.first->second;
    }

    अचर std::string& internVertex(अचर DfgVertex& vtx) {
        अचर स्वतः pair = m_internedVertices.emplace(&vtx, "");
        यदि (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        फर्काऊ pair.first->second;
    }

    शून्य recordMultiplicityAndDepth(अचर DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& मान = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        मान.first += 1;
        मान.second = std::max(मान.second, depth);
        यदि (!depth) फर्काऊ;
        vtx.foreachSource([&](अचर DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            फर्काऊ असत्य;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    शून्य render(अचर DfgVertex& vtx, uint32_t depth, तार्किक is_जरा_2 = सत्य) {
        यदि (अचर DfgConst* अचर constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            यदि (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अन्यथा यदि (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } अन्यथा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अन्यथा यदि (!is_जरा_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } अन्यथा यदि (!vtx.foreachSource([&](अचर DfgVertex&) { फर्काऊ सत्य; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } अन्यथा यदि (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } अन्यथा {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            यदि (अचर DfgSel* अचर selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                यदि (selp->lsb() == 0) {
                    m_os << '0';
                } अन्यथा {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](अचर DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, असत्य);
                फर्काऊ असत्य;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        यदि (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } अन्यथा {
            अचर uint32_t width = vtx.size();
            यदि (width == 1) {
                m_os << '1';
            } अन्यथा यदि (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अन्यथा {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        यदि (vtx.hasMultipleSinks()) m_os << '*';
    }

सार्वजनिक:
    DfgPatternString(std::ostream& os, अचर DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, असत्य);
        using Pair = std::pair<std::string, अचर DfgVertex*>;
        std::vector<Pair> vertices;
        लागि (अचर स्वतः& pair : m_multiplicityAndDepth) {
            यदि (pair.second.first == 1) जारी;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](अचर Pair& a, अचर Pair& b) {  //
            फर्काऊ a.first < b.first;
        });
        लागि (अचर Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) अचर {
    std::ostringstream oss;
    DfgPatternString{oss, *एतत्, depth};
    फर्काऊ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#अन्तर्भाव "V3Dfg__gen_visitor_defns.h"  // From ./astgen
