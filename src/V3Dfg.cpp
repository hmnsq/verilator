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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3DfgContext.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ধ্রুব string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*এতত্); });
}

শূন্য DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    যদি (otherps.empty()) প্রত্যাগম;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ধ্রুব VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    প্রতি (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    প্রতি (ধ্রুব std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        প্রতি (DfgVertexVar* ধ্রুব vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            যদি (DfgVertexVar* ধ্রুব altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ধ্রুব srcp = vtxp->srcp();
                DfgVertex* ধ্রুব defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                যদি (srcp) altp->srcp(srcp);
                যদি (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                অনুবর্ত;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = এতত্;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        প্রতি (DfgVertexAst* ধ্রুব vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = এতত্;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        প্রতি (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = এতত্;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        প্রতি (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = এতত্;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_সারি* flp, ধ্রুব std::string& prefix,
                                   ধ্রুব DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ধ্রুব vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    যদি (dtype.isArray()) প্রত্যাগম নূতন DfgVarArray{*এতত্, vscp};
    প্রত্যাগম নূতন DfgVarPacked{*এতত্, vscp};
}

স্থির ধ্রুব std::string toDotId(ধ্রুব DfgVertex& vtx) { প্রত্যাগম '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
স্থির শূন্য dumpDotVertex(std::ostream& os, ধ্রুব DfgVertex& vtx) {
    যদি (ধ্রুব DfgVertexVar* ধ্রুব varVtxp = vtx.cast<DfgVertexVar>()) {
        ধ্রুব AstVarScope* ধ্রুব vscp = varVtxp->vscp();
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
        যদি (ধ্রুব AstVarScope* ধ্রুব tmpForp = varVtxp->tmpForp()) {
            যদি (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        স্থির ধ্রুব বর্ণ* ধ্রুব rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[অসত্য][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        যদি (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } অন্যথা যদি (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } অন্যথা {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ধ্রুব বর্ণ* ধ্রুব colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        প্রত্যাগম;
    }

    যদি (ধ্রুব DfgPrev* ধ্রুব prevVtxp = vtx.cast<DfgPrev>()) {
        ধ্রুব AstVarScope* ধ্রুব vscp = prevVtxp->vscp();
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
        যদি (prevVtxp->isPacked()) {
            os << ", shape=box";
        } অন্যথা যদি (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } অন্যথা {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ধ্রুব বর্ণ* ধ্রুব colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        প্রত্যাগম;
    }

    যদি (ধ্রুব DfgConst* ধ্রুব constVtxp = vtx.cast<DfgConst>()) {
        ধ্রুব V3_সংখ্যা& অঙ্ক = constVtxp->অঙ্ক();

        os << toDotId(vtx);
        os << " [label=\"";
        যদি (অঙ্ক.width() <= 32 && !অঙ্ক.isSigned()) {
            os << constVtxp->width() << "'d" << অঙ্ক.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << অঙ্ক.toUInt() << std::dec << '\n';
        } অন্যথা {
            os << অঙ্ক.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        প্রত্যাগম;
    }

    যদি (ধ্রুব DfgSel* ধ্রুব selVtxp = vtx.cast<DfgSel>()) {
        ধ্রুব uint32_t lsb = selVtxp->lsb();
        ধ্রুব uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        যদি (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } অন্যথা {
            os << ", shape=circle";
        }
        os << "]\n";
        প্রত্যাগম;
    }

    যদি (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        যদি (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } অন্যথা {
            os << ", shape=octagon";
        }
        os << "]\n";
        প্রত্যাগম;
    }

    যদি (ধ্রুব DfgLogic* ধ্রুব logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_বৃক্ষ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ধ্রুব বর্ণ* ধ্রুব colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        প্রত্যাগম;
    }

    যদি (ধ্রুব DfgVertexAst* ধ্রুব astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_বৃক্ষ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ধ্রুব DfgAstRd* ধ্রুব astRdVtxp = astVtxp->cast<DfgAstRd>();
        ধ্রুব বর্ণ* ধ্রুব colorp = astRdVtxp ? "#80ff80"  // Green
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
        প্রত্যাগম;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    যদি (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } অন্যথা {
        os << ", shape=circle";
    }
    os << "]\n";
}

শূন্য DfgGraph::dumpDot(std::ostream& os, ধ্রুব std::string& label,
                       std::function<তার্কিক(ধ্রুব DfgVertex&)> p) ধ্রুব {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    যদি (!p) p = [](ধ্রুব DfgVertex&) { প্রত্যাগম সত্য; };

    std::unordered_set<ধ্রুব DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ধ্রুব DfgVertex& vtx) {
        ধ্রুব DfgLogic* ধ্রুব logicp = vtx.cast<DfgLogic>();
        যদি (!logicp) প্রত্যাগম;
        যদি (logicp->synth().empty()) প্রত্যাগম;
        যদি (!p(vtx)) প্রত্যাগম;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.ঢোকাও(logicp);
        প্রতি (DfgVertex* ধ্রুব vtxp : logicp->synth()) {
            যদি (!p(*vtxp)) অনুবর্ত;
            dumpDotVertex(os, *vtxp);
            emitted.ঢোকাও(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ধ্রুব DfgVertex& vtx) {
        যদি (emitted.গুনতি(&vtx)) প্রত্যাগম;
        যদি (!p(vtx)) প্রত্যাগম;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ধ্রুব DfgVertex& vtx) {
        যদি (!p(vtx)) প্রত্যাগম;
        প্রতি (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ধ্রুব srcp = vtx.inputp(i);
            যদি (!srcp) অনুবর্ত;
            যদি (!p(*srcp)) অনুবর্ত;
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

std::string DfgGraph::dumpDotString(ধ্রুব std::string& label,
                                    std::function<তার্কিক(ধ্রুব DfgVertex&)> p) ধ্রুব {
    std::stringstream ss;
    dumpDot(ss, label, p);
    প্রত্যাগম ss.str();
}

শূন্য DfgGraph::dumpDotFile(ধ্রুব std::string& filename, ধ্রুব std::string& label,
                           std::function<তার্কিক(ধ্রুব DfgVertex&)> p) ধ্রুব {
    ধ্রুব std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    যদি (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

শূন্য DfgGraph::dumpDotFilePrefixed(ধ্রুব std::string& label,
                                   std::function<তার্কিক(ধ্রুব DfgVertex&)> p) ধ্রুব {
    std::string filename = name();
    যদি (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <তার্কিক T_SinksNotSources>
স্থির std::unique_ptr<std::unordered_set<ধ্রুব DfgVertex*>>
dfgGraphCollectCone(ধ্রুব std::vector<ধ্রুব DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ধ্রুব DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ধ্রুব DfgVertex*>> resp{
        নূতন std::unordered_set<ধ্রুব DfgVertex*>{}};
    // Depth first traversal
    যাবত্ (!queue.empty()) {
        // Pop next work item
        ধ্রুব DfgVertex* ধ্রুব vtxp = queue.back();
        queue.বের_করো_back();
        // Mark vertex as visited, move on if already visited
        যদি (!resp->ঢোকাও(vtxp).second) অনুবর্ত;
        // Enqueue all siblings of this vertex.
        যদি VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ধ্রুব DfgVertex& sink) {
                queue.ঠেলো_back(&sink);
                প্রত্যাগম অসত্য;
            });
        } অন্যথা {
            vtxp->foreachSource([&](ধ্রুব DfgVertex& src) {
                queue.ঠেলো_back(&src);
                প্রত্যাগম অসত্য;
            });
        }
    }
    // Done
    প্রত্যাগম resp;
}

std::unique_ptr<std::unordered_set<ধ্রুব DfgVertex*>>
DfgGraph::sourceCone(ধ্রুব std::vector<ধ্রুব DfgVertex*>& vtxps) ধ্রুব {
    প্রত্যাগম dfgGraphCollectCone<অসত্য>(vtxps);
}

std::unique_ptr<std::unordered_set<ধ্রুব DfgVertex*>>
DfgGraph::sinkCone(ধ্রুব std::vector<ধ্রুব DfgVertex*>& vtxps) ধ্রুব {
    প্রত্যাগম dfgGraphCollectCone<সত্য>(vtxps);
}

std::unique_ptr<std::unordered_set<ধ্রুব DfgVertex*>>
DfgGraph::neighborhood(ধ্রুব std::vector<ধ্রুব DfgVertex*>& vtxps, size_t n) ধ্রুব {
    // Neighborhood
    std::vector<ধ্রুব DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ধ্রুব DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    প্রতি (size_t hops = 1; hops <= n; ++hops) {
        প্রতি (size_t i = begin; i < end; ++i) {
            ধ্রুব DfgVertex* ধ্রুব vtxp = vec[i];
            vtxp->foreachSink([&](ধ্রুব DfgVertex& dst) {
                যদি (res.ঢোকাও(&dst).second) vec.ঠেলো_back(&dst);
                প্রত্যাগম অসত্য;
            });
            vtxp->foreachSource([&](ধ্রুব DfgVertex& src) {
                যদি (res.ঢোকাও(&src).second) vec.ঠেলো_back(&src);
                প্রত্যাগম অসত্য;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    প্রত্যাগম std::make_unique<std::unordered_set<ধ্রুব DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_সারি* flp, ধ্রুব DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*এতত্);
}

তার্কিক DfgVertex::unsafe() ধ্রুব {
    যদি (is<DfgMux>()) প্রত্যাগম সত্য;
    যদি (is<DfgArraySel>()) প্রত্যাগম !as<DfgArraySel>()->bitp()->is<DfgConst>();
    প্রত্যাগম অসত্য;
}

শূন্য DfgVertex::typeCheck(ধ্রুব DfgGraph& dfg) ধ্রুব {

#define CHECK(cond, বার্তা) \
    UASSERT_OBJ(cond, এতত্, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << বার্তা);
    বিকল্প (type()) {
    পক্ষ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::AstRd: {
        ধ্রুব DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::VarArray:
    পক্ষ VDfgType::VarPacked: {
        ধ্রুব DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::SpliceArray:
    পক্ষ VDfgType::SplicePacked: {
        ধ্রুব DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ধ্রুব DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            প্রত্যাগম অসত্য;
        });
        প্রত্যাগম;
    }
    পক্ষ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::UnitArray: {
        ধ্রুব DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::Sel: {
        ধ্রুব DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::MatchMasked: {
        ধ্রুব DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::Mux: {
        ধ্রুব DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        প্রত্যাগম;
    }
    পক্ষ VDfgType::ArraySel: {
        ধ্রুব DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Add:
    পক্ষ VDfgType::And:
    পক্ষ VDfgType::Div:
    পক্ষ VDfgType::DivS:
    পক্ষ VDfgType::ModDiv:
    পক্ষ VDfgType::ModDivS:
    পক্ষ VDfgType::Mul:
    পক্ষ VDfgType::MulS:
    পক্ষ VDfgType::Or:
    পক্ষ VDfgType::Sub:
    পক্ষ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Negate:
    পক্ষ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::ShiftL:
    পক্ষ VDfgType::ShiftR:
    পক্ষ VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Concat: {
        ধ্রুব DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Rep: {
        ধ্রুব DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::StreamL:
    পক্ষ VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Eq:
    পক্ষ VDfgType::EqCase:
    পক্ষ VDfgType::EqWild:
    পক্ষ VDfgType::Neq:
    পক্ষ VDfgType::NeqCase:
    পক্ষ VDfgType::NeqWild:
    পক্ষ VDfgType::Gt:
    পক্ষ VDfgType::GtS:
    পক্ষ VDfgType::Gte:
    পক্ষ VDfgType::GteS:
    পক্ষ VDfgType::Lt:
    পক্ষ VDfgType::LtS:
    পক্ষ VDfgType::Lte:
    পক্ষ VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Extend:
    পক্ষ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::LogAnd:
    পক্ষ VDfgType::LogEq:
    পক্ষ VDfgType::LogIf:
    পক্ষ VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::LogNot:
    পক্ষ VDfgType::RedAnd:
    পক্ষ VDfgType::RedOr:
    পক্ষ VDfgType::RedXor:
    পক্ষ VDfgType::OneHot:
    পক্ষ VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Cond: {
        ধ্রুব DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::গুনতি_Ones_2: {
        ধ্রুব Dfg_গুনতি_Ones& v = *as<Dfg_গুনতি_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        প্রত্যাগম;
    }

    পক্ষ VDfgType::Pow:
    পক্ষ VDfgType::PowSS:
    পক্ষ VDfgType::PowSU:
    পক্ষ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        প্রত্যাগম;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ধ্রুব {
    uint32_t result = 0;
    foreachSink([&](ধ্রুব DfgVertex&) {
        ++result;
        প্রত্যাগম অসত্য;
    });
    প্রত্যাগম result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    যদি (DfgVertexVar* ধ্রুব varp = এতত্->cast<DfgVertexVar>()) প্রত্যাগম varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    এতত্->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ধ্রুব varp = sink.cast<DfgVertexVar>();
        যদি (!varp) প্রত্যাগম অসত্য;

        // Do not use it if value might differ from its drivers
        যদি (varp->isVolatile()) প্রত্যাগম অসত্য;

        // First variable found
        যদি (!resp) {
            resp = varp;
            প্রত্যাগম অসত্য;
        }

        // Prefer those variables that must be kept anyway
        যদি (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            যদি (!resp->hasExtRdRefs()) resp = varp;
            প্রত্যাগম অসত্য;
        }
        যদি (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            যদি (!resp->hasDfgRefs()) resp = varp;
            প্রত্যাগম অসত্য;
        }

        // Prefer real variables over temporaries
        ধ্রুব তার্কিক resIsTemp = resp->vscp()->varp()->isTemp();
        ধ্রুব তার্কিক varIsTemp = varp->vscp()->varp()->isTemp();
        যদি (resIsTemp != varIsTemp) {
            যদি (resIsTemp) resp = varp;
            প্রত্যাগম অসত্য;
        }
        যদি (!resp->tmpForp() != !varp->tmpForp()) {
            যদি (resp->tmpForp()) resp = varp;
            প্রত্যাগম অসত্য;
        }

        // Prefer the earlier one in source order
        ধ্রুব File_সারি& oldFlp = *(resp->fileline());
        ধ্রুব File_সারি& newFlp = *(varp->fileline());
        যদি (ধ্রুব পূর্ণাঙ্ক cmp = oldFlp.operatorCompare(newFlp)) {
            যদি (cmp > 0) resp = varp;
            প্রত্যাগম অসত্য;
        }
        // Prefer the one with the lexically smaller name
        যদি (ধ্রুব পূর্ণাঙ্ক cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            যদি (cmp > 0) resp = varp;
            প্রত্যাগম অসত্য;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        প্রত্যাগম অসত্য;
    });
    প্রত্যাগম resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, তার্কিক tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    যদি (ধ্রুব DfgVertexVar* ধ্রুব varp = এতত্->cast<DfgVertexVar>()) {
        প্রত্যাগম varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    যদি (tryResultVar) {
        যদি (ধ্রুব DfgVertexVar* ধ্রুব varp = এতত্->getResultVar()) {
            প্রত্যাগম varp->vscp()->scopep();
        }
    }

    AstScope* ধ্রুব rootp = v3Global.rootp()->শীর্ষ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[এতত্];
    যদি (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ধ্রুব scp = src.scopep(cache, সত্য);
            যদি (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                প্রত্যাগম সত্য;
            }
            প্রত্যাগম অসত্য;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), এতত্,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    প্রত্যাগম resultr;
}

শূন্য DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    যাবত্ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*এতত্);
    // Delete - this will unlink sources
    delete এতত্;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ধ্রুব srcp = এতত্->srcp();
    // Not driven at all
    যদি (!srcp) প্রত্যাগম {nullptr, 0};
    // If volatile, can have other drivers
    যদি (isVolatile()) প্রত্যাগম {nullptr, 0};
    // Don't inline CReset
    যদি (srcp->is<DfgCReset>()) প্রত্যাগম {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ধ্রুব splicep = srcp->cast<DfgVertexSplice>();
    যদি (!splicep) প্রত্যাগম {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ধ্রুব uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    তার্কিক useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ধ্রুব uint32_t dLo) {
        ধ্রুব uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        যদি (dLo <= hi && lo <= dHi) useDefault = অসত্য;
        // If it does not cover the whole searched range, move on
        যদি (lo < dLo || dHi < hi) প্রত্যাগম অসত্য;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        প্রত্যাগম সত্য;
    });

    // If a single driver covers the searched range, it is the one
    যদি (driverp) প্রত্যাগম {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    যদি (useDefault) প্রত্যাগম {defaultp(), lo};
    // Not driven by a single vertex
    প্রত্যাগম {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

বর্গ DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ধ্রুব DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ধ্রুব DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    স্থির std::string toLetters(size_t মান, তার্কিক lowerCase = অসত্য) {
        ধ্রুব বর্ণ base = lowerCase ? 'a' : 'A';
        std::string s;
        কুরু { s += static_cast<বর্ণ>(base + মান % 26); } যাবত্ (মান /= 26);
        প্রত্যাগম s;
    }

    ধ্রুব std::string& internConst(ধ্রুব DfgConst& vtx) {
        ধ্রুব auto pair = m_internedConsts.emplace(vtx.অঙ্ক().ascii(অসত্য), "");
        যদি (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        প্রত্যাগম pair.first->second;
    }

    ধ্রুব std::string& internSelLsb(uint32_t মান) {
        ধ্রুব auto pair = m_internedSelLsbs.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        প্রত্যাগম pair.first->second;
    }

    ধ্রুব std::string& internWordWidth(uint32_t মান) {
        ধ্রুব auto pair = m_internedWordWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, সত্য);
        প্রত্যাগম pair.first->second;
    }

    ধ্রুব std::string& internWideWidth(uint32_t মান) {
        ধ্রুব auto pair = m_internedWideWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        প্রত্যাগম pair.first->second;
    }

    ধ্রুব std::string& internVertex(ধ্রুব DfgVertex& vtx) {
        ধ্রুব auto pair = m_internedVertices.emplace(&vtx, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        প্রত্যাগম pair.first->second;
    }

    শূন্য recordMultiplicityAndDepth(ধ্রুব DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& মান = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        মান.first += 1;
        মান.second = std::max(মান.second, depth);
        যদি (!depth) প্রত্যাগম;
        vtx.foreachSource([&](ধ্রুব DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            প্রত্যাগম অসত্য;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    শূন্য render(ধ্রুব DfgVertex& vtx, uint32_t depth, তার্কিক is_মূল_2 = সত্য) {
        যদি (ধ্রুব DfgConst* ধ্রুব constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            যদি (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } অন্যথা যদি (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } অন্যথা {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } অন্যথা যদি (!is_মূল_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } অন্যথা যদি (!vtx.foreachSource([&](ধ্রুব DfgVertex&) { প্রত্যাগম সত্য; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } অন্যথা যদি (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } অন্যথা {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            যদি (ধ্রুব DfgSel* ধ্রুব selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                যদি (selp->lsb() == 0) {
                    m_os << '0';
                } অন্যথা {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ধ্রুব DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, অসত্য);
                প্রত্যাগম অসত্য;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        যদি (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } অন্যথা {
            ধ্রুব uint32_t width = vtx.size();
            যদি (width == 1) {
                m_os << '1';
            } অন্যথা যদি (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } অন্যথা {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        যদি (vtx.hasMultipleSinks()) m_os << '*';
    }

সার্বজনিক:
    DfgPatternString(std::ostream& os, ধ্রুব DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, অসত্য);
        using Pair = std::pair<std::string, ধ্রুব DfgVertex*>;
        std::vector<Pair> vertices;
        প্রতি (ধ্রুব auto& pair : m_multiplicityAndDepth) {
            যদি (pair.second.first == 1) অনুবর্ত;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ধ্রুব Pair& a, ধ্রুব Pair& b) {  //
            প্রত্যাগম a.first < b.first;
        });
        প্রতি (ধ্রুব Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ধ্রুব {
    std::ostringstream oss;
    DfgPatternString{oss, *এতত্, depth};
    প্রত্যাগম oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#অন্তর্ভাব "V3Dfg__gen_visitor_defns.h"  // From ./astgen
