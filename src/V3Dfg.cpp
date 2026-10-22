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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3DfgContext.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, மாறிலி string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*இது); });
}

வெறுமை DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    எனில் (otherps.empty()) திரும்பு;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    மாறிலி VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    க்கு (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    க்கு (மாறிலி std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        க்கு (DfgVertexVar* மாறிலி vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            எனில் (DfgVertexVar* மாறிலி altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* மாறிலி srcp = vtxp->srcp();
                DfgVertex* மாறிலி defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                எனில் (srcp) altp->srcp(srcp);
                எனில் (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                தொடர்;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = இது;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        க்கு (DfgVertexAst* மாறிலி vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = இது;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        க்கு (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = இது;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        க்கு (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = இது;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_வரி* flp, மாறிலி std::string& prefix,
                                   மாறிலி DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* மாறிலி vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    எனில் (dtype.isArray()) திரும்பு புதிய DfgVarArray{*இது, vscp};
    திரும்பு புதிய DfgVarPacked{*இது, vscp};
}

நிலையான மாறிலி std::string toDotId(மாறிலி DfgVertex& vtx) { திரும்பு '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
நிலையான வெறுமை dumpDotVertex(std::ostream& os, மாறிலி DfgVertex& vtx) {
    எனில் (மாறிலி DfgVertexVar* மாறிலி varVtxp = vtx.cast<DfgVertexVar>()) {
        மாறிலி AstVarScope* மாறிலி vscp = varVtxp->vscp();
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
        எனில் (மாறிலி AstVarScope* மாறிலி tmpForp = varVtxp->tmpForp()) {
            எனில் (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        நிலையான மாறிலி எழுத்து* மாறிலி rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[பொய்][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        எனில் (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } இல்லையெனில் எனில் (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } இல்லையெனில் {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        மாறிலி எழுத்து* மாறிலி colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        திரும்பு;
    }

    எனில் (மாறிலி DfgPrev* மாறிலி prevVtxp = vtx.cast<DfgPrev>()) {
        மாறிலி AstVarScope* மாறிலி vscp = prevVtxp->vscp();
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
        எனில் (prevVtxp->isPacked()) {
            os << ", shape=box";
        } இல்லையெனில் எனில் (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } இல்லையெனில் {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        மாறிலி எழுத்து* மாறிலி colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        திரும்பு;
    }

    எனில் (மாறிலி DfgConst* மாறிலி constVtxp = vtx.cast<DfgConst>()) {
        மாறிலி V3_எண்& இலக்கம் = constVtxp->இலக்கம்();

        os << toDotId(vtx);
        os << " [label=\"";
        எனில் (இலக்கம்.width() <= 32 && !இலக்கம்.isSigned()) {
            os << constVtxp->width() << "'d" << இலக்கம்.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << இலக்கம்.toUInt() << std::dec << '\n';
        } இல்லையெனில் {
            os << இலக்கம்.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        திரும்பு;
    }

    எனில் (மாறிலி DfgSel* மாறிலி selVtxp = vtx.cast<DfgSel>()) {
        மாறிலி uint32_t lsb = selVtxp->lsb();
        மாறிலி uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        எனில் (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } இல்லையெனில் {
            os << ", shape=circle";
        }
        os << "]\n";
        திரும்பு;
    }

    எனில் (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        எனில் (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } இல்லையெனில் {
            os << ", shape=octagon";
        }
        os << "]\n";
        திரும்பு;
    }

    எனில் (மாறிலி DfgLogic* மாறிலி logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_மரம்(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        மாறிலி எழுத்து* மாறிலி colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        திரும்பு;
    }

    எனில் (மாறிலி DfgVertexAst* மாறிலி astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_மரம்(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        மாறிலி DfgAstRd* மாறிலி astRdVtxp = astVtxp->cast<DfgAstRd>();
        மாறிலி எழுத்து* மாறிலி colorp = astRdVtxp ? "#80ff80"  // Green
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
        திரும்பு;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    எனில் (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } இல்லையெனில் {
        os << ", shape=circle";
    }
    os << "]\n";
}

வெறுமை DfgGraph::dumpDot(std::ostream& os, மாறிலி std::string& label,
                       std::function<தருக்க(மாறிலி DfgVertex&)> p) மாறிலி {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    எனில் (!p) p = [](மாறிலி DfgVertex&) { திரும்பு உண்மை; };

    std::unordered_set<மாறிலி DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](மாறிலி DfgVertex& vtx) {
        மாறிலி DfgLogic* மாறிலி logicp = vtx.cast<DfgLogic>();
        எனில் (!logicp) திரும்பு;
        எனில் (logicp->synth().empty()) திரும்பு;
        எனில் (!p(vtx)) திரும்பு;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.செருகு(logicp);
        க்கு (DfgVertex* மாறிலி vtxp : logicp->synth()) {
            எனில் (!p(*vtxp)) தொடர்;
            dumpDotVertex(os, *vtxp);
            emitted.செருகு(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](மாறிலி DfgVertex& vtx) {
        எனில் (emitted.மொத்தம்(&vtx)) திரும்பு;
        எனில் (!p(vtx)) திரும்பு;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](மாறிலி DfgVertex& vtx) {
        எனில் (!p(vtx)) திரும்பு;
        க்கு (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* மாறிலி srcp = vtx.inputp(i);
            எனில் (!srcp) தொடர்;
            எனில் (!p(*srcp)) தொடர்;
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

std::string DfgGraph::dumpDotString(மாறிலி std::string& label,
                                    std::function<தருக்க(மாறிலி DfgVertex&)> p) மாறிலி {
    std::stringstream ss;
    dumpDot(ss, label, p);
    திரும்பு ss.str();
}

வெறுமை DfgGraph::dumpDotFile(மாறிலி std::string& filename, மாறிலி std::string& label,
                           std::function<தருக்க(மாறிலி DfgVertex&)> p) மாறிலி {
    மாறிலி std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    எனில் (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

வெறுமை DfgGraph::dumpDotFilePrefixed(மாறிலி std::string& label,
                                   std::function<தருக்க(மாறிலி DfgVertex&)> p) மாறிலி {
    std::string filename = name();
    எனில் (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <தருக்க T_SinksNotSources>
நிலையான std::unique_ptr<std::unordered_set<மாறிலி DfgVertex*>>
dfgGraphCollectCone(மாறிலி std::vector<மாறிலி DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<மாறிலி DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<மாறிலி DfgVertex*>> resp{
        புதிய std::unordered_set<மாறிலி DfgVertex*>{}};
    // Depth first traversal
    போது (!queue.empty()) {
        // Pop next work item
        மாறிலி DfgVertex* மாறிலி vtxp = queue.back();
        queue.எடு_back();
        // Mark vertex as visited, move on if already visited
        எனில் (!resp->செருகு(vtxp).second) தொடர்;
        // Enqueue all siblings of this vertex.
        எனில் VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](மாறிலி DfgVertex& sink) {
                queue.தள்ளு_back(&sink);
                திரும்பு பொய்;
            });
        } இல்லையெனில் {
            vtxp->foreachSource([&](மாறிலி DfgVertex& src) {
                queue.தள்ளு_back(&src);
                திரும்பு பொய்;
            });
        }
    }
    // Done
    திரும்பு resp;
}

std::unique_ptr<std::unordered_set<மாறிலி DfgVertex*>>
DfgGraph::sourceCone(மாறிலி std::vector<மாறிலி DfgVertex*>& vtxps) மாறிலி {
    திரும்பு dfgGraphCollectCone<பொய்>(vtxps);
}

std::unique_ptr<std::unordered_set<மாறிலி DfgVertex*>>
DfgGraph::sinkCone(மாறிலி std::vector<மாறிலி DfgVertex*>& vtxps) மாறிலி {
    திரும்பு dfgGraphCollectCone<உண்மை>(vtxps);
}

std::unique_ptr<std::unordered_set<மாறிலி DfgVertex*>>
DfgGraph::neighborhood(மாறிலி std::vector<மாறிலி DfgVertex*>& vtxps, size_t n) மாறிலி {
    // Neighborhood
    std::vector<மாறிலி DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<மாறிலி DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    க்கு (size_t hops = 1; hops <= n; ++hops) {
        க்கு (size_t i = begin; i < end; ++i) {
            மாறிலி DfgVertex* மாறிலி vtxp = vec[i];
            vtxp->foreachSink([&](மாறிலி DfgVertex& dst) {
                எனில் (res.செருகு(&dst).second) vec.தள்ளு_back(&dst);
                திரும்பு பொய்;
            });
            vtxp->foreachSource([&](மாறிலி DfgVertex& src) {
                எனில் (res.செருகு(&src).second) vec.தள்ளு_back(&src);
                திரும்பு பொய்;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    திரும்பு std::make_unique<std::unordered_set<மாறிலி DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_வரி* flp, மாறிலி DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*இது);
}

தருக்க DfgVertex::unsafe() மாறிலி {
    எனில் (is<DfgMux>()) திரும்பு உண்மை;
    எனில் (is<DfgArraySel>()) திரும்பு !as<DfgArraySel>()->bitp()->is<DfgConst>();
    திரும்பு பொய்;
}

வெறுமை DfgVertex::typeCheck(மாறிலி DfgGraph& dfg) மாறிலி {

#define CHECK(cond, செய்தி) \
    UASSERT_OBJ(cond, இது, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << செய்தி);
    தேர்வு (type()) {
    நிலை VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        திரும்பு;
    }
    நிலை VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        திரும்பு;
    }
    நிலை VDfgType::AstRd: {
        மாறிலி DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        திரும்பு;
    }
    நிலை VDfgType::VarArray:
    நிலை VDfgType::VarPacked: {
        மாறிலி DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        திரும்பு;
    }
    நிலை VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        திரும்பு;
    }
    நிலை VDfgType::SpliceArray:
    நிலை VDfgType::SplicePacked: {
        மாறிலி DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](மாறிலி DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            திரும்பு பொய்;
        });
        திரும்பு;
    }
    நிலை VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        திரும்பு;
    }
    நிலை VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        திரும்பு;
    }
    நிலை VDfgType::UnitArray: {
        மாறிலி DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        திரும்பு;
    }
    நிலை VDfgType::Sel: {
        மாறிலி DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        திரும்பு;
    }
    நிலை VDfgType::MatchMasked: {
        மாறிலி DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        திரும்பு;
    }
    நிலை VDfgType::Mux: {
        மாறிலி DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        திரும்பு;
    }
    நிலை VDfgType::ArraySel: {
        மாறிலி DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        திரும்பு;
    }

    நிலை VDfgType::Add:
    நிலை VDfgType::And:
    நிலை VDfgType::Div:
    நிலை VDfgType::DivS:
    நிலை VDfgType::ModDiv:
    நிலை VDfgType::ModDivS:
    நிலை VDfgType::Mul:
    நிலை VDfgType::MulS:
    நிலை VDfgType::Or:
    நிலை VDfgType::Sub:
    நிலை VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        திரும்பு;
    }

    நிலை VDfgType::Negate:
    நிலை VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        திரும்பு;
    }

    நிலை VDfgType::ShiftL:
    நிலை VDfgType::ShiftR:
    நிலை VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        திரும்பு;
    }

    நிலை VDfgType::Concat: {
        மாறிலி DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        திரும்பு;
    }

    நிலை VDfgType::Rep: {
        மாறிலி DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        திரும்பு;
    }

    நிலை VDfgType::StreamL:
    நிலை VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        திரும்பு;
    }

    நிலை VDfgType::Eq:
    நிலை VDfgType::EqCase:
    நிலை VDfgType::EqWild:
    நிலை VDfgType::Neq:
    நிலை VDfgType::NeqCase:
    நிலை VDfgType::NeqWild:
    நிலை VDfgType::Gt:
    நிலை VDfgType::GtS:
    நிலை VDfgType::Gte:
    நிலை VDfgType::GteS:
    நிலை VDfgType::Lt:
    நிலை VDfgType::LtS:
    நிலை VDfgType::Lte:
    நிலை VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        திரும்பு;
    }

    நிலை VDfgType::Extend:
    நிலை VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        திரும்பு;
    }

    நிலை VDfgType::LogAnd:
    நிலை VDfgType::LogEq:
    நிலை VDfgType::LogIf:
    நிலை VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        திரும்பு;
    }

    நிலை VDfgType::LogNot:
    நிலை VDfgType::RedAnd:
    நிலை VDfgType::RedOr:
    நிலை VDfgType::RedXor:
    நிலை VDfgType::OneHot:
    நிலை VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        திரும்பு;
    }

    நிலை VDfgType::Cond: {
        மாறிலி DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        திரும்பு;
    }

    நிலை VDfgType::மொத்தம்_Ones_2: {
        மாறிலி Dfg_மொத்தம்_Ones& v = *as<Dfg_மொத்தம்_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        திரும்பு;
    }

    நிலை VDfgType::Pow:
    நிலை VDfgType::PowSS:
    நிலை VDfgType::PowSU:
    நிலை VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        திரும்பு;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() மாறிலி {
    uint32_t result = 0;
    foreachSink([&](மாறிலி DfgVertex&) {
        ++result;
        திரும்பு பொய்;
    });
    திரும்பு result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    எனில் (DfgVertexVar* மாறிலி varp = இது->cast<DfgVertexVar>()) திரும்பு varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    இது->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* மாறிலி varp = sink.cast<DfgVertexVar>();
        எனில் (!varp) திரும்பு பொய்;

        // Do not use it if value might differ from its drivers
        எனில் (varp->isVolatile()) திரும்பு பொய்;

        // First variable found
        எனில் (!resp) {
            resp = varp;
            திரும்பு பொய்;
        }

        // Prefer those variables that must be kept anyway
        எனில் (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            எனில் (!resp->hasExtRdRefs()) resp = varp;
            திரும்பு பொய்;
        }
        எனில் (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            எனில் (!resp->hasDfgRefs()) resp = varp;
            திரும்பு பொய்;
        }

        // Prefer real variables over temporaries
        மாறிலி தருக்க resIsTemp = resp->vscp()->varp()->isTemp();
        மாறிலி தருக்க varIsTemp = varp->vscp()->varp()->isTemp();
        எனில் (resIsTemp != varIsTemp) {
            எனில் (resIsTemp) resp = varp;
            திரும்பு பொய்;
        }
        எனில் (!resp->tmpForp() != !varp->tmpForp()) {
            எனில் (resp->tmpForp()) resp = varp;
            திரும்பு பொய்;
        }

        // Prefer the earlier one in source order
        மாறிலி File_வரி& oldFlp = *(resp->fileline());
        மாறிலி File_வரி& newFlp = *(varp->fileline());
        எனில் (மாறிலி முழு cmp = oldFlp.operatorCompare(newFlp)) {
            எனில் (cmp > 0) resp = varp;
            திரும்பு பொய்;
        }
        // Prefer the one with the lexically smaller name
        எனில் (மாறிலி முழு cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            எனில் (cmp > 0) resp = varp;
            திரும்பு பொய்;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        திரும்பு பொய்;
    });
    திரும்பு resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, தருக்க tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    எனில் (மாறிலி DfgVertexVar* மாறிலி varp = இது->cast<DfgVertexVar>()) {
        திரும்பு varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    எனில் (tryResultVar) {
        எனில் (மாறிலி DfgVertexVar* மாறிலி varp = இது->getResultVar()) {
            திரும்பு varp->vscp()->scopep();
        }
    }

    AstScope* மாறிலி rootp = v3Global.rootp()->உச்சி_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[இது];
    எனில் (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* மாறிலி scp = src.scopep(cache, உண்மை);
            எனில் (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                திரும்பு உண்மை;
            }
            திரும்பு பொய்;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), இது,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    திரும்பு resultr;
}

வெறுமை DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    போது (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*இது);
    // Delete - this will unlink sources
    delete இது;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* மாறிலி srcp = இது->srcp();
    // Not driven at all
    எனில் (!srcp) திரும்பு {nullptr, 0};
    // If volatile, can have other drivers
    எனில் (isVolatile()) திரும்பு {nullptr, 0};
    // Don't inline CReset
    எனில் (srcp->is<DfgCReset>()) திரும்பு {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* மாறிலி splicep = srcp->cast<DfgVertexSplice>();
    எனில் (!splicep) திரும்பு {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    மாறிலி uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    தருக்க useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, மாறிலி uint32_t dLo) {
        மாறிலி uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        எனில் (dLo <= hi && lo <= dHi) useDefault = பொய்;
        // If it does not cover the whole searched range, move on
        எனில் (lo < dLo || dHi < hi) திரும்பு பொய்;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        திரும்பு உண்மை;
    });

    // If a single driver covers the searched range, it is the one
    எனில் (driverp) திரும்பு {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    எனில் (useDefault) திரும்பு {defaultp(), lo};
    // Not driven by a single vertex
    திரும்பு {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

வகுப்பு DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<மாறிலி DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<மாறிலி DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    நிலையான std::string toLetters(size_t மதிப்பு, தருக்க lowerCase = பொய்) {
        மாறிலி எழுத்து base = lowerCase ? 'a' : 'A';
        std::string s;
        செய் { s += static_cast<எழுத்து>(base + மதிப்பு % 26); } போது (மதிப்பு /= 26);
        திரும்பு s;
    }

    மாறிலி std::string& internConst(மாறிலி DfgConst& vtx) {
        மாறிலி auto pair = m_internedConsts.emplace(vtx.இலக்கம்().ascii(பொய்), "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        திரும்பு pair.first->second;
    }

    மாறிலி std::string& internSelLsb(uint32_t மதிப்பு) {
        மாறிலி auto pair = m_internedSelLsbs.emplace(மதிப்பு, "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        திரும்பு pair.first->second;
    }

    மாறிலி std::string& internWordWidth(uint32_t மதிப்பு) {
        மாறிலி auto pair = m_internedWordWidths.emplace(மதிப்பு, "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, உண்மை);
        திரும்பு pair.first->second;
    }

    மாறிலி std::string& internWideWidth(uint32_t மதிப்பு) {
        மாறிலி auto pair = m_internedWideWidths.emplace(மதிப்பு, "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        திரும்பு pair.first->second;
    }

    மாறிலி std::string& internVertex(மாறிலி DfgVertex& vtx) {
        மாறிலி auto pair = m_internedVertices.emplace(&vtx, "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        திரும்பு pair.first->second;
    }

    வெறுமை recordMultiplicityAndDepth(மாறிலி DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& மதிப்பு = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        மதிப்பு.first += 1;
        மதிப்பு.second = std::max(மதிப்பு.second, depth);
        எனில் (!depth) திரும்பு;
        vtx.foreachSource([&](மாறிலி DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            திரும்பு பொய்;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    வெறுமை render(மாறிலி DfgVertex& vtx, uint32_t depth, தருக்க is_வேர்_2 = உண்மை) {
        எனில் (மாறிலி DfgConst* மாறிலி constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            எனில் (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } இல்லையெனில் எனில் (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } இல்லையெனில் {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } இல்லையெனில் எனில் (!is_வேர்_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } இல்லையெனில் எனில் (!vtx.foreachSource([&](மாறிலி DfgVertex&) { திரும்பு உண்மை; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } இல்லையெனில் எனில் (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } இல்லையெனில் {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            எனில் (மாறிலி DfgSel* மாறிலி selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                எனில் (selp->lsb() == 0) {
                    m_os << '0';
                } இல்லையெனில் {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](மாறிலி DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, பொய்);
                திரும்பு பொய்;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        எனில் (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } இல்லையெனில் {
            மாறிலி uint32_t width = vtx.size();
            எனில் (width == 1) {
                m_os << '1';
            } இல்லையெனில் எனில் (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } இல்லையெனில் {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        எனில் (vtx.hasMultipleSinks()) m_os << '*';
    }

பொது:
    DfgPatternString(std::ostream& os, மாறிலி DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, பொய்);
        using Pair = std::pair<std::string, மாறிலி DfgVertex*>;
        std::vector<Pair> vertices;
        க்கு (மாறிலி auto& pair : m_multiplicityAndDepth) {
            எனில் (pair.second.first == 1) தொடர்;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](மாறிலி Pair& a, மாறிலி Pair& b) {  //
            திரும்பு a.first < b.first;
        });
        க்கு (மாறிலி Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) மாறிலி {
    std::ostringstream oss;
    DfgPatternString{oss, *இது, depth};
    திரும்பு oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#அந்தர்பாவ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
