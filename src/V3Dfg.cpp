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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"

#شامل "V3Ast.h"
#شامل "V3DfgContext.h"
#شامل "V3EmitV.h"
#شامل "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ثابت string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*این); });
}

تهی DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    اگر (otherps.empty()) بازگشت;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ثابت VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    برای (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    برای (ثابت std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        برای (DfgVertexVar* ثابت vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            اگر (DfgVertexVar* ثابت altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ثابت srcp = vtxp->srcp();
                DfgVertex* ثابت defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                اگر (srcp) altp->srcp(srcp);
                اگر (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                ادامه;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = این;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        برای (DfgVertexAst* ثابت vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = این;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        برای (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = این;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        برای (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = این;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_سطر* flp, ثابت std::string& prefix,
                                   ثابت DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ثابت vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    اگر (dtype.isArray()) بازگشت جدید DfgVarArray{*این, vscp};
    بازگشت جدید DfgVarPacked{*این, vscp};
}

ایستا ثابت std::string toDotId(ثابت DfgVertex& vtx) { بازگشت '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ایستا تهی dumpDotVertex(std::ostream& os, ثابت DfgVertex& vtx) {
    اگر (ثابت DfgVertexVar* ثابت varVtxp = vtx.cast<DfgVertexVar>()) {
        ثابت AstVarScope* ثابت vscp = varVtxp->vscp();
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
        اگر (ثابت AstVarScope* ثابت tmpForp = varVtxp->tmpForp()) {
            اگر (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ایستا ثابت نویسه* ثابت rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[نادرست][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        اگر (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } وگرنه اگر (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } وگرنه {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ثابت نویسه* ثابت colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        بازگشت;
    }

    اگر (ثابت DfgPrev* ثابت prevVtxp = vtx.cast<DfgPrev>()) {
        ثابت AstVarScope* ثابت vscp = prevVtxp->vscp();
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
        اگر (prevVtxp->isPacked()) {
            os << ", shape=box";
        } وگرنه اگر (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } وگرنه {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ثابت نویسه* ثابت colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        بازگشت;
    }

    اگر (ثابت DfgConst* ثابت constVtxp = vtx.cast<DfgConst>()) {
        ثابت V3_عدد& رقم = constVtxp->رقم();

        os << toDotId(vtx);
        os << " [label=\"";
        اگر (رقم.width() <= 32 && !رقم.isSigned()) {
            os << constVtxp->width() << "'d" << رقم.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << رقم.toUInt() << std::dec << '\n';
        } وگرنه {
            os << رقم.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        بازگشت;
    }

    اگر (ثابت DfgSel* ثابت selVtxp = vtx.cast<DfgSel>()) {
        ثابت uint32_t lsb = selVtxp->lsb();
        ثابت uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        اگر (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } وگرنه {
            os << ", shape=circle";
        }
        os << "]\n";
        بازگشت;
    }

    اگر (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        اگر (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } وگرنه {
            os << ", shape=octagon";
        }
        os << "]\n";
        بازگشت;
    }

    اگر (ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_درخت(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت نویسه* ثابت colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        بازگشت;
    }

    اگر (ثابت DfgVertexAst* ثابت astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_درخت(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت DfgAstRd* ثابت astRdVtxp = astVtxp->cast<DfgAstRd>();
        ثابت نویسه* ثابت colorp = astRdVtxp ? "#80ff80"  // Green
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
        بازگشت;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    اگر (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } وگرنه {
        os << ", shape=circle";
    }
    os << "]\n";
}

تهی DfgGraph::dumpDot(std::ostream& os, ثابت std::string& label,
                       std::function<منطقی(ثابت DfgVertex&)> p) ثابت {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    اگر (!p) p = [](ثابت DfgVertex&) { بازگشت درست; };

    std::unordered_set<ثابت DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ثابت DfgVertex& vtx) {
        ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        اگر (!logicp) بازگشت;
        اگر (logicp->synth().empty()) بازگشت;
        اگر (!p(vtx)) بازگشت;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.درج(logicp);
        برای (DfgVertex* ثابت vtxp : logicp->synth()) {
            اگر (!p(*vtxp)) ادامه;
            dumpDotVertex(os, *vtxp);
            emitted.درج(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ثابت DfgVertex& vtx) {
        اگر (emitted.شمار(&vtx)) بازگشت;
        اگر (!p(vtx)) بازگشت;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ثابت DfgVertex& vtx) {
        اگر (!p(vtx)) بازگشت;
        برای (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ثابت srcp = vtx.inputp(i);
            اگر (!srcp) ادامه;
            اگر (!p(*srcp)) ادامه;
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

std::string DfgGraph::dumpDotString(ثابت std::string& label,
                                    std::function<منطقی(ثابت DfgVertex&)> p) ثابت {
    std::stringstream ss;
    dumpDot(ss, label, p);
    بازگشت ss.str();
}

تهی DfgGraph::dumpDotFile(ثابت std::string& filename, ثابت std::string& label,
                           std::function<منطقی(ثابت DfgVertex&)> p) ثابت {
    ثابت std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    اگر (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

تهی DfgGraph::dumpDotFilePrefixed(ثابت std::string& label,
                                   std::function<منطقی(ثابت DfgVertex&)> p) ثابت {
    std::string filename = name();
    اگر (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <منطقی T_SinksNotSources>
ایستا std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
dfgGraphCollectCone(ثابت std::vector<ثابت DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ثابت DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ثابت DfgVertex*>> resp{
        جدید std::unordered_set<ثابت DfgVertex*>{}};
    // Depth first traversal
    وقتی (!queue.empty()) {
        // Pop next work item
        ثابت DfgVertex* ثابت vtxp = queue.back();
        queue.برداشت_back();
        // Mark vertex as visited, move on if already visited
        اگر (!resp->درج(vtxp).second) ادامه;
        // Enqueue all siblings of this vertex.
        اگر VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ثابت DfgVertex& sink) {
                queue.افزودن_back(&sink);
                بازگشت نادرست;
            });
        } وگرنه {
            vtxp->foreachSource([&](ثابت DfgVertex& src) {
                queue.افزودن_back(&src);
                بازگشت نادرست;
            });
        }
    }
    // Done
    بازگشت resp;
}

std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
DfgGraph::sourceCone(ثابت std::vector<ثابت DfgVertex*>& vtxps) ثابت {
    بازگشت dfgGraphCollectCone<نادرست>(vtxps);
}

std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
DfgGraph::sinkCone(ثابت std::vector<ثابت DfgVertex*>& vtxps) ثابت {
    بازگشت dfgGraphCollectCone<درست>(vtxps);
}

std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
DfgGraph::neighborhood(ثابت std::vector<ثابت DfgVertex*>& vtxps, size_t n) ثابت {
    // Neighborhood
    std::vector<ثابت DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ثابت DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    برای (size_t hops = 1; hops <= n; ++hops) {
        برای (size_t i = begin; i < end; ++i) {
            ثابت DfgVertex* ثابت vtxp = vec[i];
            vtxp->foreachSink([&](ثابت DfgVertex& dst) {
                اگر (res.درج(&dst).second) vec.افزودن_back(&dst);
                بازگشت نادرست;
            });
            vtxp->foreachSource([&](ثابت DfgVertex& src) {
                اگر (res.درج(&src).second) vec.افزودن_back(&src);
                بازگشت نادرست;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    بازگشت std::make_unique<std::unordered_set<ثابت DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_سطر* flp, ثابت DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*این);
}

منطقی DfgVertex::unsafe() ثابت {
    اگر (is<DfgMux>()) بازگشت درست;
    اگر (is<DfgArraySel>()) بازگشت !as<DfgArraySel>()->bitp()->is<DfgConst>();
    بازگشت نادرست;
}

تهی DfgVertex::typeCheck(ثابت DfgGraph& dfg) ثابت {

#define CHECK(cond, پیام) \
    UASSERT_OBJ(cond, این, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << پیام);
    انتخاب (type()) {
    حالت VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        بازگشت;
    }
    حالت VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        بازگشت;
    }
    حالت VDfgType::AstRd: {
        ثابت DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        بازگشت;
    }
    حالت VDfgType::VarArray:
    حالت VDfgType::VarPacked: {
        ثابت DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        بازگشت;
    }
    حالت VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        بازگشت;
    }
    حالت VDfgType::SpliceArray:
    حالت VDfgType::SplicePacked: {
        ثابت DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ثابت DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            بازگشت نادرست;
        });
        بازگشت;
    }
    حالت VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        بازگشت;
    }
    حالت VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        بازگشت;
    }
    حالت VDfgType::UnitArray: {
        ثابت DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        بازگشت;
    }
    حالت VDfgType::Sel: {
        ثابت DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        بازگشت;
    }
    حالت VDfgType::MatchMasked: {
        ثابت DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        بازگشت;
    }
    حالت VDfgType::Mux: {
        ثابت DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        بازگشت;
    }
    حالت VDfgType::ArraySel: {
        ثابت DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        بازگشت;
    }

    حالت VDfgType::Add:
    حالت VDfgType::And:
    حالت VDfgType::Div:
    حالت VDfgType::DivS:
    حالت VDfgType::ModDiv:
    حالت VDfgType::ModDivS:
    حالت VDfgType::Mul:
    حالت VDfgType::MulS:
    حالت VDfgType::Or:
    حالت VDfgType::Sub:
    حالت VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        بازگشت;
    }

    حالت VDfgType::Negate:
    حالت VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        بازگشت;
    }

    حالت VDfgType::ShiftL:
    حالت VDfgType::ShiftR:
    حالت VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        بازگشت;
    }

    حالت VDfgType::Concat: {
        ثابت DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        بازگشت;
    }

    حالت VDfgType::Rep: {
        ثابت DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        بازگشت;
    }

    حالت VDfgType::StreamL:
    حالت VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        بازگشت;
    }

    حالت VDfgType::Eq:
    حالت VDfgType::EqCase:
    حالت VDfgType::EqWild:
    حالت VDfgType::Neq:
    حالت VDfgType::NeqCase:
    حالت VDfgType::NeqWild:
    حالت VDfgType::Gt:
    حالت VDfgType::GtS:
    حالت VDfgType::Gte:
    حالت VDfgType::GteS:
    حالت VDfgType::Lt:
    حالت VDfgType::LtS:
    حالت VDfgType::Lte:
    حالت VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        بازگشت;
    }

    حالت VDfgType::Extend:
    حالت VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        بازگشت;
    }

    حالت VDfgType::LogAnd:
    حالت VDfgType::LogEq:
    حالت VDfgType::LogIf:
    حالت VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        بازگشت;
    }

    حالت VDfgType::LogNot:
    حالت VDfgType::RedAnd:
    حالت VDfgType::RedOr:
    حالت VDfgType::RedXor:
    حالت VDfgType::OneHot:
    حالت VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        بازگشت;
    }

    حالت VDfgType::Cond: {
        ثابت DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        بازگشت;
    }

    حالت VDfgType::شمار_Ones_2: {
        ثابت Dfg_شمار_Ones& v = *as<Dfg_شمار_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        بازگشت;
    }

    حالت VDfgType::Pow:
    حالت VDfgType::PowSS:
    حالت VDfgType::PowSU:
    حالت VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        بازگشت;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ثابت {
    uint32_t result = 0;
    foreachSink([&](ثابت DfgVertex&) {
        ++result;
        بازگشت نادرست;
    });
    بازگشت result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    اگر (DfgVertexVar* ثابت varp = این->cast<DfgVertexVar>()) بازگشت varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    این->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ثابت varp = sink.cast<DfgVertexVar>();
        اگر (!varp) بازگشت نادرست;

        // Do not use it if value might differ from its drivers
        اگر (varp->isVolatile()) بازگشت نادرست;

        // First variable found
        اگر (!resp) {
            resp = varp;
            بازگشت نادرست;
        }

        // Prefer those variables that must be kept anyway
        اگر (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            اگر (!resp->hasExtRdRefs()) resp = varp;
            بازگشت نادرست;
        }
        اگر (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            اگر (!resp->hasDfgRefs()) resp = varp;
            بازگشت نادرست;
        }

        // Prefer real variables over temporaries
        ثابت منطقی resIsTemp = resp->vscp()->varp()->isTemp();
        ثابت منطقی varIsTemp = varp->vscp()->varp()->isTemp();
        اگر (resIsTemp != varIsTemp) {
            اگر (resIsTemp) resp = varp;
            بازگشت نادرست;
        }
        اگر (!resp->tmpForp() != !varp->tmpForp()) {
            اگر (resp->tmpForp()) resp = varp;
            بازگشت نادرست;
        }

        // Prefer the earlier one in source order
        ثابت File_سطر& oldFlp = *(resp->fileline());
        ثابت File_سطر& newFlp = *(varp->fileline());
        اگر (ثابت صحیح cmp = oldFlp.operatorCompare(newFlp)) {
            اگر (cmp > 0) resp = varp;
            بازگشت نادرست;
        }
        // Prefer the one with the lexically smaller name
        اگر (ثابت صحیح cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            اگر (cmp > 0) resp = varp;
            بازگشت نادرست;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        بازگشت نادرست;
    });
    بازگشت resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, منطقی tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    اگر (ثابت DfgVertexVar* ثابت varp = این->cast<DfgVertexVar>()) {
        بازگشت varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    اگر (tryResultVar) {
        اگر (ثابت DfgVertexVar* ثابت varp = این->getResultVar()) {
            بازگشت varp->vscp()->scopep();
        }
    }

    AstScope* ثابت rootp = v3Global.rootp()->بالاترین_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[این];
    اگر (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ثابت scp = src.scopep(cache, درست);
            اگر (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                بازگشت درست;
            }
            بازگشت نادرست;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), این,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    بازگشت resultr;
}

تهی DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    وقتی (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*این);
    // Delete - this will unlink sources
    delete این;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ثابت srcp = این->srcp();
    // Not driven at all
    اگر (!srcp) بازگشت {nullptr, 0};
    // If volatile, can have other drivers
    اگر (isVolatile()) بازگشت {nullptr, 0};
    // Don't inline CReset
    اگر (srcp->is<DfgCReset>()) بازگشت {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ثابت splicep = srcp->cast<DfgVertexSplice>();
    اگر (!splicep) بازگشت {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ثابت uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    منطقی useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ثابت uint32_t dLo) {
        ثابت uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        اگر (dLo <= hi && lo <= dHi) useDefault = نادرست;
        // If it does not cover the whole searched range, move on
        اگر (lo < dLo || dHi < hi) بازگشت نادرست;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        بازگشت درست;
    });

    // If a single driver covers the searched range, it is the one
    اگر (driverp) بازگشت {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    اگر (useDefault) بازگشت {defaultp(), lo};
    // Not driven by a single vertex
    بازگشت {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

رده DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ثابت DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ثابت DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    ایستا std::string toLetters(size_t مقدار, منطقی lowerCase = نادرست) {
        ثابت نویسه base = lowerCase ? 'a' : 'A';
        std::string s;
        انجامبده { s += static_cast<نویسه>(base + مقدار % 26); } وقتی (مقدار /= 26);
        بازگشت s;
    }

    ثابت std::string& internConst(ثابت DfgConst& vtx) {
        ثابت auto pair = m_internedConsts.emplace(vtx.رقم().ascii(نادرست), "");
        اگر (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        بازگشت pair.first->second;
    }

    ثابت std::string& internSelLsb(uint32_t مقدار) {
        ثابت auto pair = m_internedSelLsbs.emplace(مقدار, "");
        اگر (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        بازگشت pair.first->second;
    }

    ثابت std::string& internWordWidth(uint32_t مقدار) {
        ثابت auto pair = m_internedWordWidths.emplace(مقدار, "");
        اگر (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, درست);
        بازگشت pair.first->second;
    }

    ثابت std::string& internWideWidth(uint32_t مقدار) {
        ثابت auto pair = m_internedWideWidths.emplace(مقدار, "");
        اگر (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        بازگشت pair.first->second;
    }

    ثابت std::string& internVertex(ثابت DfgVertex& vtx) {
        ثابت auto pair = m_internedVertices.emplace(&vtx, "");
        اگر (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        بازگشت pair.first->second;
    }

    تهی recordMultiplicityAndDepth(ثابت DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& مقدار = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        مقدار.first += 1;
        مقدار.second = std::max(مقدار.second, depth);
        اگر (!depth) بازگشت;
        vtx.foreachSource([&](ثابت DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            بازگشت نادرست;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    تهی render(ثابت DfgVertex& vtx, uint32_t depth, منطقی is_ریشه_2 = درست) {
        اگر (ثابت DfgConst* ثابت constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            اگر (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وگرنه اگر (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } وگرنه {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وگرنه اگر (!is_ریشه_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } وگرنه اگر (!vtx.foreachSource([&](ثابت DfgVertex&) { بازگشت درست; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } وگرنه اگر (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } وگرنه {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            اگر (ثابت DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                اگر (selp->lsb() == 0) {
                    m_os << '0';
                } وگرنه {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ثابت DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, نادرست);
                بازگشت نادرست;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        اگر (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } وگرنه {
            ثابت uint32_t width = vtx.size();
            اگر (width == 1) {
                m_os << '1';
            } وگرنه اگر (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } وگرنه {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        اگر (vtx.hasMultipleSinks()) m_os << '*';
    }

عمومی:
    DfgPatternString(std::ostream& os, ثابت DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, نادرست);
        using Pair = std::pair<std::string, ثابت DfgVertex*>;
        std::vector<Pair> vertices;
        برای (ثابت auto& pair : m_multiplicityAndDepth) {
            اگر (pair.second.first == 1) ادامه;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ثابت Pair& a, ثابت Pair& b) {  //
            بازگشت a.first < b.first;
        });
        برای (ثابت Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ثابت {
    std::ostringstream oss;
    DfgPatternString{oss, *این, depth};
    بازگشت oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#شامل "V3Dfg__gen_visitor_defns.h"  // From ./astgen
