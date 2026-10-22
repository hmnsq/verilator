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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgContext.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

Dfg_ਗ੍ਰਾਫ::Dfg_ਗ੍ਰਾਫ(V3DfgContext& ctx, ਅਚਲ string& name)
    : m_ctx{ctx}
    , m_name{name} {}

Dfg_ਗ੍ਰਾਫ::~Dfg_ਗ੍ਰਾਫ() {
    for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) { vtx.unlinkDelete(*ਇਹ); });
}

ਖਾਲੀ Dfg_ਗ੍ਰਾਫ::ਮਿਲਾਓ_Graphs(std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>>&& otherps) {
    ਜੇ (otherps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    ਲਈ (ਅਚਲ std::unique_ptr<Dfg_ਗ੍ਰਾਫ>& otherp : otherps) {
        // Process variables
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ altp = vtxp->vscp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ਜੇ (srcp) altp->srcp(srcp);
                ਜੇ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                ਜਾਰੀ;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
            vtxp->m_dfgp = ਇਹ;
#ਜੇ_ਅੰਤ
        }
        m_varVertices.splice(m_varVertices.ਸਿਰਾ(), otherp->m_varVertices);
        // Process Ast references
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Ast* ਅਚਲ vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
            vtxp->m_dfgp = ਇਹ;
#ਜੇ_ਅੰਤ
        }
        m_astVertices.splice(m_astVertices.ਸਿਰਾ(), otherp->m_astVertices);
        // Process constants
        ਲਈ (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
            vtx.m_dfgp = ਇਹ;
#ਜੇ_ਅੰਤ
        }
        m_constVertices.splice(m_constVertices.ਸਿਰਾ(), otherp->m_constVertices);
        // Process operations
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
            vtx.m_dfgp = ਇਹ;
#ਜੇ_ਅੰਤ
        }
        m_opVertices.splice(m_opVertices.ਸਿਰਾ(), otherp->m_opVertices);
        // Update graph sizes
        m_ਮਾਪ += otherp->m_ਮਾਪ;
        otherp->m_ਮਾਪ = 0;
    }
}

Dfg_ਸ਼ੀਰਸ਼_Var* Dfg_ਗ੍ਰਾਫ::makeNewVar(File_ਸਤਰ* flp, ਅਚਲ std::string& ਅਗੇਤਰ,
                                   ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, AstScope* scopep) {
    AstVarScope* ਅਚਲ vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), ਅਗੇਤਰ);
    // Create and return the corresponding variable vertex
    ਜੇ (dtype.isArray()) ਵਾਪਸ ਨਵਾਂ DfgVarArray{*ਇਹ, vscp};
    ਵਾਪਸ ਨਵਾਂ DfgVarPacked{*ਇਹ, vscp};
}

ਸਥਿਰ ਅਚਲ std::string ਮੰਜ਼ਿਲ_Dot_Id(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) { ਵਾਪਸ '"' + cvt_ਮੰਜ਼ਿਲ_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ਸਥਿਰ ਖਾਲੀ dump_Dot_ਸ਼ੀਰਸ਼(std::ostream& os, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
    ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varVtxp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
        ਅਚਲ AstVarScope* ਅਚਲ vscp = varVtxp->vscp();
        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //ਪਤਾ
        os << '\n' << cvt_ਮੰਜ਼ਿਲ_Hex(varVtxp);
        // Original variable, if any
        ਜੇ (ਅਚਲ AstVarScope* ਅਚਲ tmpForp = varVtxp->tmpForp()) {
            ਜੇ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[ਝੂਠ][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ਜੇ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ਨਹੀਂਤਾਂ ਜੇ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ਨਹੀਂਤਾਂ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ਅਚਲ ਅੱਖਰ* ਅਚਲ colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  //ਸਲੇਟੀ
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ਵਾਪਸ;
    }

    ਜੇ (ਅਚਲ DfgPrev* ਅਚਲ prevVtxp = vtx.cast<DfgPrev>()) {
        ਅਚਲ AstVarScope* ਅਚਲ vscp = prevVtxp->vscp();
        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //ਪਤਾ
        os << '\n' << cvt_ਮੰਜ਼ਿਲ_Hex(prevVtxp);
        // Type and fanout
        os << '\n';
        prevVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << prevVtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        ਜੇ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } ਨਹੀਂਤਾਂ ਜੇ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } ਨਹੀਂਤਾਂ {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ਅਚਲ ਅੱਖਰ* ਅਚਲ colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ਵਾਪਸ;
    }

    ਜੇ (ਅਚਲ DfgConst* ਅਚਲ constVtxp = vtx.cast<DfgConst>()) {
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = constVtxp->ਅੰਕ();

        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        os << " [label=\"";
        ਜੇ (ਅੰਕ.ਚੌੜਾਈ() <= 32 && !ਅੰਕ.isSigned()) {
            os << constVtxp->ਚੌੜਾਈ() << "'d" << ਅੰਕ.ਮੰਜ਼ਿਲ_UInt() << '\n';
            os << constVtxp->ਚੌੜਾਈ() << "'h" << std::hex << ਅੰਕ.ਮੰਜ਼ਿਲ_UInt() << std::dec << '\n';
        } ਨਹੀਂਤਾਂ {
            os << ਅੰਕ.ascii() << '\n';
        }
        os << cvt_ਮੰਜ਼ਿਲ_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ਵਾਪਸ;
    }

    ਜੇ (ਅਚਲ DfgSel* ਅਚਲ selVtxp = vtx.cast<DfgSel>()) {
        ਅਚਲ uint32_t lsb = selVtxp->lsb();
        ਅਚਲ uint32_t msb = lsb + selVtxp->ਚੌੜਾਈ() - 1;
        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_ਮੰਜ਼ਿਲ_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ਜੇ (vtx.has_ਗੁਣਜ_Sinks()) {
            os << ", shape=doublecircle";
        } ਨਹੀਂਤਾਂ {
            os << ", shape=circle";
        }
        os << "]\n";
        ਵਾਪਸ;
    }

    ਜੇ (vtx.is<Dfg_ਸ਼ੀਰਸ਼_Splice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_ਮੰਜ਼ਿਲ_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ਜੇ (vtx.has_ਗੁਣਜ_Sinks()) {
            os << ", shape=doubleoctagon";
        } ਨਹੀਂਤਾਂ {
            os << ", shape=octagon";
        }
        os << "]\n";
        ਵਾਪਸ;
    }

    ਜੇ (ਅਚਲ DfgLogic* ਅਚਲ logicp = vtx.cast<DfgLogic>()) {
        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ਰੁੱਖ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_ਮੰਜ਼ਿਲ_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ਅਚਲ ਅੱਖਰ* ਅਚਲ colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_ਮੰਜ਼ਿਲ_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        ਵਾਪਸ;
    }

    ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ast* ਅਚਲ astVtxp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Ast>()) {
        os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ਰੁੱਖ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_ਮੰਜ਼ਿਲ_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ਅਚਲ DfgAstRd* ਅਚਲ astRdVtxp = astVtxp->cast<DfgAstRd>();
        ਅਚਲ ਅੱਖਰ* ਅਚਲ colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_ਮੰਜ਼ਿਲ_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->ਵਿੱਚ_Sen_ਵਸਤੂ()];
        os << "_L"[astRdVtxp->ਵਿੱਚ_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        ਵਾਪਸ;
    }

    os << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_ਮੰਜ਼ਿਲ_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ਜੇ (vtx.has_ਗੁਣਜ_Sinks()) {
        os << ", shape=doublecircle";
    } ਨਹੀਂਤਾਂ {
        os << ", shape=circle";
    }
    os << "]\n";
}

ਖਾਲੀ Dfg_ਗ੍ਰਾਫ::dumpDot(std::ostream& os, ਅਚਲ std::string& label,
                       std::function<ਤਾਰਕਿਕ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&)> p) ਅਚਲ {
    // This generates a graphviz dump, https://www.graphviz.org

    //ਸਿਰਲੇਖ
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ਜੇ (!p) p = [](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&) { ਵਾਪਸ ਸੱਚ; };

    std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> emitted;
    // Emit all vertices associated with a DfgLogic
    for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਅਚਲ DfgLogic* ਅਚਲ logicp = vtx.cast<DfgLogic>();
        ਜੇ (!logicp) ਵਾਪਸ;
        ਜੇ (logicp->synth().ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਜੇ (!p(vtx)) ਵਾਪਸ;
        os << "subgraph cluster_" << cvt_ਮੰਜ਼ਿਲ_Hex(logicp) << " {\n";
        dump_Dot_ਸ਼ੀਰਸ਼(os, *logicp);
        emitted.ਪਾਓ(logicp);
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : logicp->synth()) {
            ਜੇ (!p(*vtxp)) ਜਾਰੀ;
            dump_Dot_ਸ਼ੀਰਸ਼(os, *vtxp);
            emitted.ਪਾਓ(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਜੇ (emitted.ਗਿਣਤੀ_2(&vtx)) ਵਾਪਸ;
        ਜੇ (!p(vtx)) ਵਾਪਸ;
        dump_Dot_ਸ਼ੀਰਸ਼(os, vtx);
    });
    // Emit all edges
    for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਜੇ (!p(vtx)) ਵਾਪਸ;
        ਲਈ (ਮਾਪ_t i = 0; i < vtx.nInputs(); ++i) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtx.inputp(i);
            ਜੇ (!srcp) ਜਾਰੀ;
            ਜੇ (!p(*srcp)) ਜਾਰੀ;
            os << ਮੰਜ਼ਿਲ_Dot_Id(*srcp) << " -> " << ਮੰਜ਼ਿਲ_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string Dfg_ਗ੍ਰਾਫ::dumpDotString(ਅਚਲ std::string& label,
                                    std::function<ਤਾਰਕਿਕ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&)> p) ਅਚਲ {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ਵਾਪਸ ss.str();
}

ਖਾਲੀ Dfg_ਗ੍ਰਾਫ::dumpDotFile(ਅਚਲ std::string& filename, ਅਚਲ std::string& label,
                           std::function<ਤਾਰਕਿਕ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&)> p) ਅਚਲ {
    ਅਚਲ std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ਜੇ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

ਖਾਲੀ Dfg_ਗ੍ਰਾਫ::dumpDotFilePrefixed(ਅਚਲ std::string& label,
                                   std::function<ਤਾਰਕਿਕ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&)> p) ਅਚਲ {
    std::string filename = name();
    ਜੇ (!label.ਖਾਲੀ_ਥਾਵਾਂ()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

ਸਾਂਚਾ <ਤਾਰਕਿਕ T_SinksNotSources>
ਸਥਿਰ std::unique_ptr<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>>
dfg_ਗ੍ਰਾਫ_Collect_Cone(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> ਕਤਾਰ = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>> resp{
        ਨਵਾਂ std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>{}};
    // Depth first traversal
    ਜਦੋਂ (!ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        // Pop next work item
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਕਤਾਰ.ਪਿੱਠ();
        ਕਤਾਰ.ਕੱਢੋ_ਪਿੱਠ();
        // Mark vertex as visited, move on if already visited
        ਜੇ (!resp->ਪਾਓ(vtxp).ਦੂਜਾ) ਜਾਰੀ;
        // Enqueue all siblings of this vertex.
        ਜੇ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& sink) {
                ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(&sink);
                ਵਾਪਸ ਝੂਠ;
            });
        } ਨਹੀਂਤਾਂ {
            vtxp->foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
                ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(&src);
                ਵਾਪਸ ਝੂਠ;
            });
        }
    }
    //ਪੂਰਾ
    ਵਾਪਸ resp;
}

std::unique_ptr<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>>
Dfg_ਗ੍ਰਾਫ::ਸਰੋਤ_Cone(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>& vtxps) ਅਚਲ {
    ਵਾਪਸ dfg_ਗ੍ਰਾਫ_Collect_Cone<ਝੂਠ>(vtxps);
}

std::unique_ptr<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>>
Dfg_ਗ੍ਰਾਫ::sinkCone(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>& vtxps) ਅਚਲ {
    ਵਾਪਸ dfg_ਗ੍ਰਾਫ_Collect_Cone<ਸੱਚ>(vtxps);
}

std::unique_ptr<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>>
Dfg_ਗ੍ਰਾਫ::neighborhood(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>& vtxps, ਮਾਪ_t n) ਅਚਲ {
    // Neighborhood
    std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> res{vec.begin(), vec.ਸਿਰਾ()};
    // Expand neihborhood by 'n' hops
    ਮਾਪ_t begin = 0;
    ਮਾਪ_t ਸਿਰਾ = vec.ਮਾਪ();
    ਲਈ (ਮਾਪ_t hops = 1; hops <= n; ++hops) {
        ਲਈ (ਮਾਪ_t i = begin; i < ਸਿਰਾ; ++i) {
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = vec[i];
            vtxp->foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& dst) {
                ਜੇ (res.ਪਾਓ(&dst).ਦੂਜਾ) vec.ਧੱਕੋ_ਪਿੱਠ(&dst);
                ਵਾਪਸ ਝੂਠ;
            });
            vtxp->foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
                ਜੇ (res.ਪਾਓ(&src).ਦੂਜਾ) vec.ਧੱਕੋ_ਪਿੱਠ(&src);
                ਵਾਪਸ ਝੂਠ;
            });
        }
        begin = ਸਿਰਾ;
        ਸਿਰਾ = vec.ਮਾਪ();
    }
    // Move out the results
    ਵਾਪਸ std::make_unique<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

Dfg_ਸ਼ੀਰਸ਼::Dfg_ਸ਼ੀਰਸ਼(Dfg_ਗ੍ਰਾਫ& dfg, VDfgType type, File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dt,
                     Dfg_ਕਿਨਾਰਾ* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.ਜੋੜੋ_ਸ਼ੀਰਸ਼(*ਇਹ);
}

ਤਾਰਕਿਕ Dfg_ਸ਼ੀਰਸ਼::unsafe() ਅਚਲ {
    ਜੇ (is<DfgMux>()) ਵਾਪਸ ਸੱਚ;
    ਜੇ (is<DfgArraySel>()) ਵਾਪਸ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ਵਾਪਸ ਝੂਠ;
}

ਖਾਲੀ Dfg_ਸ਼ੀਰਸ਼::typeCheck(ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg) ਅਚਲ {

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ CHECK(cond, ਸੁਨੇਹਾ_2) \
    UASSERT_OBJ(cond, ਇਹ, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << ਸੁਨੇਹਾ_2);
    ਚੋਣ (type()) {
    ਹਾਲਤ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::AstRd: {
        ਅਚਲ DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::VarArray:
    ਹਾਲਤ VDfgType::VarPacked: {
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var& v = *as<Dfg_ਸ਼ੀਰਸ਼_Var>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::SpliceArray:
    ਹਾਲਤ VDfgType::SplicePacked: {
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Splice& v = *as<Dfg_ਸ਼ੀਰਸ਼_Splice>();
        v.foreachDriver([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_ਅੰਕੜੇ_Type::select(v.dtype(), lo, src.ਮਾਪ()), "driver");
            ਵਾਪਸ ਝੂਠ;
        });
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::UnitArray: {
        ਅਚਲ DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.ਮਾਪ() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::Sel: {
        ਅਚਲ DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_ਅੰਕੜੇ_Type::select(v.srcp()->dtype(), v.lsb(), v.ਮਾਪ()), "sel");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::ਮੇਲ_Masked: {
        ਅਚਲ Dfg_ਮੇਲ_Masked& v = *as<Dfg_ਮੇਲ_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.ਮਾਪ() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<Dfg_ਸ਼ੀਰਸ਼_Var>(), "Match should be a variable");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::Mux: {
        ਅਚਲ DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->ਮਾਪ() >= v.ਮਾਪ(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ਵਾਪਸ;
    }
    ਹਾਲਤ VDfgType::ArraySel: {
        ਅਚਲ DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::ਜੋੜੋ_2:
    ਹਾਲਤ VDfgType::And:
    ਹਾਲਤ VDfgType::Div:
    ਹਾਲਤ VDfgType::DivS:
    ਹਾਲਤ VDfgType::ModDiv:
    ਹਾਲਤ VDfgType::ModDivS:
    ਹਾਲਤ VDfgType::Mul:
    ਹਾਲਤ VDfgType::MulS:
    ਹਾਲਤ VDfgType::Or:
    ਹਾਲਤ VDfgType::Sub:
    ਹਾਲਤ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Negate:
    ਹਾਲਤ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_L:
    ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_R:
    ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_RS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Concat: {
        ਅਚਲ DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.ਮਾਪ() == v.rhsp()->ਮਾਪ() + v.lhsp()->ਮਾਪ(), "Concat result mismatch");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Rep: {
        ਅਚਲ DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.ਮਾਪ() % v.srcp()->ਮਾਪ() == 0, "Not a replicate");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::ਧਾਰਾ_L:
    ਹਾਲਤ VDfgType::ਧਾਰਾ_R: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Eq:
    ਹਾਲਤ VDfgType::EqCase:
    ਹਾਲਤ VDfgType::EqWild:
    ਹਾਲਤ VDfgType::Neq:
    ਹਾਲਤ VDfgType::NeqCase:
    ਹਾਲਤ VDfgType::NeqWild:
    ਹਾਲਤ VDfgType::Gt:
    ਹਾਲਤ VDfgType::GtS:
    ਹਾਲਤ VDfgType::Gte:
    ਹਾਲਤ VDfgType::GteS:
    ਹਾਲਤ VDfgType::Lt:
    ਹਾਲਤ VDfgType::LtS:
    ਹਾਲਤ VDfgType::Lte:
    ਹਾਲਤ VDfgType::LteS: {
        CHECK(dtype() == Dfg_ਅੰਕੜੇ_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Extend:
    ਹਾਲਤ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->ਮਾਪ() < ਮਾਪ(), "Operand should be narrower");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::LogAnd:
    ਹਾਲਤ VDfgType::LogEq:
    ਹਾਲਤ VDfgType::LogIf:
    ਹਾਲਤ VDfgType::LogOr: {
        CHECK(dtype() == Dfg_ਅੰਕੜੇ_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::LogNot:
    ਹਾਲਤ VDfgType::RedAnd:
    ਹਾਲਤ VDfgType::RedOr:
    ਹਾਲਤ VDfgType::RedXor:
    ਹਾਲਤ VDfgType::OneHot:
    ਹਾਲਤ VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_ਅੰਕੜੇ_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Cond: {
        ਅਚਲ DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::ਗਿਣਤੀ_Ones_2: {
        ਅਚਲ Dfg_ਗਿਣਤੀ_Ones& v = *as<Dfg_ਗਿਣਤੀ_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.ਮਾਪ() == 32U, "Should yield a 32-bit result");
        ਵਾਪਸ;
    }

    ਹਾਲਤ VDfgType::Pow:
    ਹਾਲਤ VDfgType::PowSS:
    ਹਾਲਤ VDfgType::PowSU:
    ਹਾਲਤ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ਵਾਪਸ;
    }
    }
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ CHECK
}

uint32_t Dfg_ਸ਼ੀਰਸ਼::fanout() ਅਚਲ {
    uint32_t ਨਤੀਜਾ = 0;
    foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&) {
        ++ਨਤੀਜਾ;
        ਵਾਪਸ ਝੂਠ;
    });
    ਵਾਪਸ ਨਤੀਜਾ;
}

Dfg_ਸ਼ੀਰਸ਼_Var* Dfg_ਸ਼ੀਰਸ਼::get_ਨਤੀਜਾ_Var() {
    // It's easy if the vertex is already a variable ...
    ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = ਇਹ->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) ਵਾਪਸ varp;

    // Inspect existing variables written by this vertex, and choose one
    Dfg_ਸ਼ੀਰਸ਼_Var* resp = ਖਾਲੀ_ਸੂਚਕ;
    // cppcheck-has-bug-suppress constParameter
    ਇਹ->foreachSink([&resp](Dfg_ਸ਼ੀਰਸ਼& sink) {
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = sink.cast<Dfg_ਸ਼ੀਰਸ਼_Var>();
        ਜੇ (!varp) ਵਾਪਸ ਝੂਠ;

        // Do not use it if value might differ from its drivers
        ਜੇ (varp->isVolatile()) ਵਾਪਸ ਝੂਠ;

        // First variable found
        ਜੇ (!resp) {
            resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }

        // Prefer those variables that must be kept anyway
        ਜੇ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ਜੇ (!resp->hasExtRdRefs()) resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ਜੇ (!resp->hasDfgRefs()) resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }

        // Prefer real variables over temporaries
        ਅਚਲ ਤਾਰਕਿਕ res_Is_ਆਰਜ਼ੀ = resp->vscp()->varp()->is_ਆਰਜ਼ੀ();
        ਅਚਲ ਤਾਰਕਿਕ var_Is_ਆਰਜ਼ੀ = varp->vscp()->varp()->is_ਆਰਜ਼ੀ();
        ਜੇ (res_Is_ਆਰਜ਼ੀ != var_Is_ਆਰਜ਼ੀ) {
            ਜੇ (res_Is_ਆਰਜ਼ੀ) resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (!resp->tmpForp() != !varp->tmpForp()) {
            ਜੇ (resp->tmpForp()) resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }

        // Prefer the earlier one in source order
        ਅਚਲ File_ਸਤਰ& oldFlp = *(resp->fileline());
        ਅਚਲ File_ਸਤਰ& newFlp = *(varp->fileline());
        ਜੇ (ਅਚਲ ਪੂਰਨ cmp = oldFlp.operatorCompare(newFlp)) {
            ਜੇ (cmp > 0) resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }
        // Prefer the one with the lexically smaller name
        ਜੇ (ਅਚਲ ਪੂਰਨ cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ਜੇ (cmp > 0) resp = varp;
            ਵਾਪਸ ਝੂਠ;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ਵਾਪਸ ਝੂਠ;
    });
    ਵਾਪਸ resp;
}

AstScope* Dfg_ਸ਼ੀਰਸ਼::scopep(ScopeCache& cache, ਤਾਰਕਿਕ try_ਨਤੀਜਾ_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = ਇਹ->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
        ਵਾਪਸ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ਜੇ (try_ਨਤੀਜਾ_Var) {
        ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = ਇਹ->get_ਨਤੀਜਾ_Var()) {
            ਵਾਪਸ varp->vscp()->scopep();
        }
    }

    AstScope* ਅਚਲ rootp = v3Global.rootp()->ਸਿਖਰ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[ਇਹ];
    ਜੇ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = ਖਾਲੀ_ਸੂਚਕ;
        foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            AstScope* ਅਚਲ scp = src.scopep(cache, ਸੱਚ);
            ਜੇ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ਵਾਪਸ ਸੱਚ;
            }
            ਵਾਪਸ ਝੂਠ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), ਇਹ,
                "DfgVertex::scopep called on graph with circular operations");

    //ਪੂਰਾ
    ਵਾਪਸ resultr;
}

ਖਾਲੀ Dfg_ਸ਼ੀਰਸ਼::unlinkDelete(Dfg_ਗ੍ਰਾਫ& dfg) {
    // Unlink sink edges
    ਜਦੋਂ (!m_sinks.ਖਾਲੀ_ਥਾਵਾਂ()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.remove_ਸ਼ੀਰਸ਼(*ਇਹ);
    // Delete - this will unlink sources
    ਮਿਟਾਓ ਇਹ;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> Dfg_ਸ਼ੀਰਸ਼_Var::driver_ਦਾ_Range(uint32_t lo, uint32_t ਮਾਪ) {
    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = ਇਹ->srcp();
    // Not driven at all
    ਜੇ (!srcp) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
    // If volatile, can have other drivers
    ਜੇ (isVolatile()) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
    // Don't inline CReset
    ਜੇ (srcp->is<DfgCReset>()) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ splicep = srcp->cast<Dfg_ਸ਼ੀਰਸ਼_Splice>();
    ਜੇ (!splicep) ਵਾਪਸ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ਅਚਲ uint32_t hi = lo + ਮਾਪ - 1;
    Dfg_ਸ਼ੀਰਸ਼* driverp = ਖਾਲੀ_ਸੂਚਕ;
    uint32_t driverLo = 0;
    ਤਾਰਕਿਕ useDefault = defaultp();
    splicep->foreachDriver([&](Dfg_ਸ਼ੀਰਸ਼& src, ਅਚਲ uint32_t dLo) {
        ਅਚਲ uint32_t dHi = dLo + src.ਮਾਪ() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ਜੇ (dLo <= hi && lo <= dHi) useDefault = ਝੂਠ;
        // If it does not cover the whole searched range, move on
        ਜੇ (lo < dLo || dHi < hi) ਵਾਪਸ ਝੂਠ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ਵਾਪਸ ਸੱਚ;
    });

    // If a single driver covers the searched range, it is the one
    ਜੇ (driverp) ਵਾਪਸ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ਜੇ (useDefault) ਵਾਪਸ {defaultp(), lo};
    // Not driven by a single vertex
    ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

ਵਰਗ Dfg_ਨਮੂਨਾ_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_interned_ਸ਼ਬਦ_Widths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_ਡੂੰਘਾਈ;

    ਸਥਿਰ std::string ਮੰਜ਼ਿਲ_Letters(ਮਾਪ_t ਮੁੱਲ, ਤਾਰਕਿਕ ਹੇਠਲਾ_Case = ਝੂਠ) {
        ਅਚਲ ਅੱਖਰ ਅਧਾਰ = ਹੇਠਲਾ_Case ? 'a' : 'A';
        std::string s;
        ਕਰੋ { s += static_cast<ਅੱਖਰ>(ਅਧਾਰ + ਮੁੱਲ % 26); } ਜਦੋਂ (ਮੁੱਲ /= 26);
        ਵਾਪਸ s;
    }

    ਅਚਲ std::string& internConst(ਅਚਲ DfgConst& vtx) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_internedConsts.emplace(vtx.ਅੰਕ().ascii(ਝੂਠ), "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_internedConsts.ਮਾਪ() - 1);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਅਚਲ std::string& internSelLsb(uint32_t ਮੁੱਲ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_internedSelLsbs.emplace(ਮੁੱਲ, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_internedSelLsbs.ਮਾਪ() - 1);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਅਚਲ std::string& intern_ਸ਼ਬਦ_ਚੌੜਾਈ(uint32_t ਮੁੱਲ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_interned_ਸ਼ਬਦ_Widths.emplace(ਮੁੱਲ, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_interned_ਸ਼ਬਦ_Widths.ਮਾਪ() - 1, ਸੱਚ);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਅਚਲ std::string& intern_Wide_ਚੌੜਾਈ(uint32_t ਮੁੱਲ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_internedWideWidths.emplace(ਮੁੱਲ, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_internedWideWidths.ਮਾਪ() - 1);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਅਚਲ std::string& intern_ਸ਼ੀਰਸ਼(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_internedVertices.emplace(&vtx, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_internedVertices.ਮਾਪ() - 1);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਖਾਲੀ ਰਿਕਾਰਡ_Multiplicity_And_ਡੂੰਘਾਈ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx, uint32_t ਡੂੰਘਾਈ) {
        std::pair<uint32_t, uint32_t>& ਮੁੱਲ = m_multiplicity_And_ਡੂੰਘਾਈ
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, ਡੂੰਘਾਈ))
                                                   .ਪਹਿਲਾ->ਦੂਜਾ;
        ਮੁੱਲ.ਪਹਿਲਾ += 1;
        ਮੁੱਲ.ਦੂਜਾ = std::max(ਮੁੱਲ.ਦੂਜਾ, ਡੂੰਘਾਈ);
        ਜੇ (!ਡੂੰਘਾਈ) ਵਾਪਸ;
        vtx.foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
            ਰਿਕਾਰਡ_Multiplicity_And_ਡੂੰਘਾਈ(src, ਡੂੰਘਾਈ - 1);
            ਵਾਪਸ ਝੂਠ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    ਖਾਲੀ render(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx, uint32_t ਡੂੰਘਾਈ, ਤਾਰਕਿਕ is_ਜੜ੍ਹ_2 = ਸੱਚ) {
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ਜੇ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ਨਹੀਂਤਾਂ ਜੇ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ਨਹੀਂਤਾਂ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ਨਹੀਂਤਾਂ ਜੇ (!is_ਜੜ੍ਹ_2 && m_multiplicity_And_ਡੂੰਘਾਈ.at(&vtx).ਪਹਿਲਾ > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << intern_ਸ਼ੀਰਸ਼(vtx);
        } ਨਹੀਂਤਾਂ ਜੇ (!vtx.foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&) { ਵਾਪਸ ਸੱਚ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ਨਹੀਂਤਾਂ ਜੇ (ਡੂੰਘਾਈ == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ਨਹੀਂਤਾਂ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ਜੇ (ਅਚਲ DfgSel* ਅਚਲ selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ਜੇ (selp->lsb() == 0) {
                    m_os << '0';
                } ਨਹੀਂਤਾਂ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
                m_os << ' ';
                render(src, ਡੂੰਘਾਈ - 1, ਝੂਠ);
                ਵਾਪਸ ਝੂਠ;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ਜੇ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ uint32_t ਚੌੜਾਈ = vtx.ਮਾਪ();
            ਜੇ (ਚੌੜਾਈ == 1) {
                m_os << '1';
            } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ <= VL_QUADSIZE) {
                m_os << intern_ਸ਼ਬਦ_ਚੌੜਾਈ(ਚੌੜਾਈ);
            } ਨਹੀਂਤਾਂ {
                m_os << intern_Wide_ਚੌੜਾਈ(ਚੌੜਾਈ);
            }
        }

        // Mark it if it has multiple sinks
        ਜੇ (vtx.has_ਗੁਣਜ_Sinks()) m_os << '*';
    }

ਜਨਤਕ:
    Dfg_ਨਮੂਨਾ_String(std::ostream& os, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx, uint32_t ਡੂੰਘਾਈ)
        : m_os{os} {
        ਰਿਕਾਰਡ_Multiplicity_And_ਡੂੰਘਾਈ(vtx, ਡੂੰਘਾਈ);
        render(vtx, ਡੂੰਘਾਈ, ਝੂਠ);
        ਵਰਤੋਂ Pair = std::pair<std::string, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>;
        std::ਸਦਿਸ਼<Pair> vertices;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_multiplicity_And_ਡੂੰਘਾਈ) {
            ਜੇ (pair.ਦੂਜਾ.ਪਹਿਲਾ == 1) ਜਾਰੀ;
            vertices.emplace_ਪਿੱਠ(intern_ਸ਼ੀਰਸ਼(*pair.ਪਹਿਲਾ), pair.ਪਹਿਲਾ);
        }
        std::ਛਾਂਟੋ(vertices.begin(), vertices.ਸਿਰਾ(), [](ਅਚਲ Pair& a, ਅਚਲ Pair& b) {  //
            ਵਾਪਸ a.ਪਹਿਲਾ < b.ਪਹਿਲਾ;
        });
        ਲਈ (ਅਚਲ Pair& pair : vertices) {
            m_os << " | " << pair.ਪਹਿਲਾ << " is ";
            render(*pair.ਦੂਜਾ, m_multiplicity_And_ਡੂੰਘਾਈ.at(pair.ਦੂਜਾ).ਦੂਜਾ);
        }
    }
};

std::string Dfg_ਸ਼ੀਰਸ਼::ਨਮੂਨਾ_String(uint32_t ਡੂੰਘਾਈ) ਅਚਲ {
    std::ostringstream oss;
    Dfg_ਨਮੂਨਾ_String{oss, *ਇਹ, ਡੂੰਘਾਈ};
    ਵਾਪਸ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
