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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3DfgContext.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

Dfg_ನಕ್ಷೆ::Dfg_ನಕ್ಷೆ(V3DfgContext& ctx, ಸ್ಥಿರಾಂಕ string& name)
    : m_ctx{ctx}
    , m_name{name} {}

Dfg_ನಕ್ಷೆ::~Dfg_ನಕ್ಷೆ() {
    for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) { vtx.unlinkDelete(*ಇದು); });
}

ಶೂನ್ಯ Dfg_ನಕ್ಷೆ::ವಿಲೀನ_Graphs(std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>>&& otherps) {
    ಆದರೆ (otherps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::unique_ptr<Dfg_ನಕ್ಷೆ>& otherp : otherps) {
        // Process variables
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ altp = vtxp->vscp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ಆದರೆ (srcp) altp->srcp(srcp);
                ಆದರೆ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                ಮುಂದುವರಿ;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
            vtxp->m_dfgp = ಇದು;
#ಆದರೆ_ಅಂತ್ಯ
        }
        m_varVertices.splice(m_varVertices.ಕೊನೆ(), otherp->m_varVertices);
        // Process Ast references
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Ast* ಸ್ಥಿರಾಂಕ vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
            vtxp->m_dfgp = ಇದು;
#ಆದರೆ_ಅಂತ್ಯ
        }
        m_astVertices.splice(m_astVertices.ಕೊನೆ(), otherp->m_astVertices);
        // Process constants
        ಗಾಗಿ (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
            vtx.m_dfgp = ಇದು;
#ಆದರೆ_ಅಂತ್ಯ
        }
        m_constVertices.splice(m_constVertices.ಕೊನೆ(), otherp->m_constVertices);
        // Process operations
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
            vtx.m_dfgp = ಇದು;
#ಆದರೆ_ಅಂತ್ಯ
        }
        m_opVertices.splice(m_opVertices.ಕೊನೆ(), otherp->m_opVertices);
        // Update graph sizes
        m_ಗಾತ್ರ += otherp->m_ಗಾತ್ರ;
        otherp->m_ಗಾತ್ರ = 0;
    }
}

Dfg_ಶೃಂಗ_Var* Dfg_ನಕ್ಷೆ::makeNewVar(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ,
                                   ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, AstScope* scopep) {
    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), ಪೂರ್ವಪ್ರತ್ಯಯ);
    // Create and return the corresponding variable vertex
    ಆದರೆ (dtype.isArray()) ಹಿಂತಿರುಗಿ ಹೊಸ DfgVarArray{*ಇದು, vscp};
    ಹಿಂತಿರುಗಿ ಹೊಸ DfgVarPacked{*ಇದು, vscp};
}

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::string ಗಮ್ಯ_Dot_Id(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) { ಹಿಂತಿರುಗಿ '"' + cvt_ಗಮ್ಯ_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ಸ್ಥಿರ ಶೂನ್ಯ dump_Dot_ಶೃಂಗ(std::ostream& os, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
    ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varVtxp = vtx.cast<Dfg_ಶೃಂಗ_Var>()) {
        ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = varVtxp->vscp();
        os << ಗಮ್ಯ_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //ವಿಳಾಸ
        os << '\n' << cvt_ಗಮ್ಯ_Hex(varVtxp);
        // Original variable, if any
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ tmpForp = varVtxp->tmpForp()) {
            ಆದರೆ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[ಸುಳ್ಳು][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ಆದರೆ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ಇಲ್ಲದಿದ್ದರೆ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  //ಬೂದು
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgPrev* ಸ್ಥಿರಾಂಕ prevVtxp = vtx.cast<DfgPrev>()) {
        ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = prevVtxp->vscp();
        os << ಗಮ್ಯ_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //ವಿಳಾಸ
        os << '\n' << cvt_ಗಮ್ಯ_Hex(prevVtxp);
        // Type and fanout
        os << '\n';
        prevVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << prevVtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        ಆದರೆ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } ಇಲ್ಲದಿದ್ದರೆ {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constVtxp = vtx.cast<DfgConst>()) {
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constVtxp->ಅಂಕೆ();

        os << ಗಮ್ಯ_Dot_Id(vtx);
        os << " [label=\"";
        ಆದರೆ (ಅಂಕೆ.ಅಗಲ() <= 32 && !ಅಂಕೆ.isSigned()) {
            os << constVtxp->ಅಗಲ() << "'d" << ಅಂಕೆ.ಗಮ್ಯ_UInt() << '\n';
            os << constVtxp->ಅಗಲ() << "'h" << std::hex << ಅಂಕೆ.ಗಮ್ಯ_UInt() << std::dec << '\n';
        } ಇಲ್ಲದಿದ್ದರೆ {
            os << ಅಂಕೆ.ascii() << '\n';
        }
        os << cvt_ಗಮ್ಯ_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ selVtxp = vtx.cast<DfgSel>()) {
        ಸ್ಥಿರಾಂಕ uint32_t lsb = selVtxp->lsb();
        ಸ್ಥಿರಾಂಕ uint32_t msb = lsb + selVtxp->ಅಗಲ() - 1;
        os << ಗಮ್ಯ_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_ಗಮ್ಯ_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ಆದರೆ (vtx.has_ಅಪವರ್ತ್ಯ_Sinks()) {
            os << ", shape=doublecircle";
        } ಇಲ್ಲದಿದ್ದರೆ {
            os << ", shape=circle";
        }
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (vtx.is<Dfg_ಶೃಂಗ_Splice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << ಗಮ್ಯ_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_ಗಮ್ಯ_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ಆದರೆ (vtx.has_ಅಪವರ್ತ್ಯ_Sinks()) {
            os << ", shape=doubleoctagon";
        } ಇಲ್ಲದಿದ್ದರೆ {
            os << ", shape=octagon";
        }
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtx.cast<DfgLogic>()) {
        os << ಗಮ್ಯ_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_ಗಮ್ಯ_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_ಗಮ್ಯ_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ast* ಸ್ಥಿರಾಂಕ astVtxp = vtx.cast<Dfg_ಶೃಂಗ_Ast>()) {
        os << ಗಮ್ಯ_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_ಗಮ್ಯ_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ಸ್ಥಿರಾಂಕ DfgAstRd* ಸ್ಥಿರಾಂಕ astRdVtxp = astVtxp->cast<DfgAstRd>();
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_ಗಮ್ಯ_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->ಒಳ_Sen_ಅಂಶ()];
        os << "_L"[astRdVtxp->ಒಳ_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        ಹಿಂತಿರುಗಿ;
    }

    os << ಗಮ್ಯ_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_ಗಮ್ಯ_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ಆದರೆ (vtx.has_ಅಪವರ್ತ್ಯ_Sinks()) {
        os << ", shape=doublecircle";
    } ಇಲ್ಲದಿದ್ದರೆ {
        os << ", shape=circle";
    }
    os << "]\n";
}

ಶೂನ್ಯ Dfg_ನಕ್ಷೆ::dumpDot(std::ostream& os, ಸ್ಥಿರಾಂಕ std::string& label,
                       std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&)> p) ಸ್ಥಿರಾಂಕ {
    // This generates a graphviz dump, https://www.graphviz.org

    //ಶೀರ್ಷಿಕೆ
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ಆದರೆ (!p) p = [](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&) { ಹಿಂತಿರುಗಿ ನಿಜ; };

    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> emitted;
    // Emit all vertices associated with a DfgLogic
    for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        ಸ್ಥಿರಾಂಕ DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtx.cast<DfgLogic>();
        ಆದರೆ (!logicp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (logicp->synth().ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!p(vtx)) ಹಿಂತಿರುಗಿ;
        os << "subgraph cluster_" << cvt_ಗಮ್ಯ_Hex(logicp) << " {\n";
        dump_Dot_ಶೃಂಗ(os, *logicp);
        emitted.ಸೇರಿಸು(logicp);
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : logicp->synth()) {
            ಆದರೆ (!p(*vtxp)) ಮುಂದುವರಿ;
            dump_Dot_ಶೃಂಗ(os, *vtxp);
            emitted.ಸೇರಿಸು(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        ಆದರೆ (emitted.ಎಣಿಕೆ_2(&vtx)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!p(vtx)) ಹಿಂತಿರುಗಿ;
        dump_Dot_ಶೃಂಗ(os, vtx);
    });
    // Emit all edges
    for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        ಆದರೆ (!p(vtx)) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < vtx.nInputs(); ++i) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtx.inputp(i);
            ಆದರೆ (!srcp) ಮುಂದುವರಿ;
            ಆದರೆ (!p(*srcp)) ಮುಂದುವರಿ;
            os << ಗಮ್ಯ_Dot_Id(*srcp) << " -> " << ಗಮ್ಯ_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.ಖಾಲಿಗಳು() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string Dfg_ನಕ್ಷೆ::dumpDotString(ಸ್ಥಿರಾಂಕ std::string& label,
                                    std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&)> p) ಸ್ಥಿರಾಂಕ {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ಹಿಂತಿರುಗಿ ss.str();
}

ಶೂನ್ಯ Dfg_ನಕ್ಷೆ::dumpDotFile(ಸ್ಥಿರಾಂಕ std::string& filename, ಸ್ಥಿರಾಂಕ std::string& label,
                           std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&)> p) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ಆದರೆ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

ಶೂನ್ಯ Dfg_ನಕ್ಷೆ::dumpDotFilePrefixed(ಸ್ಥಿರಾಂಕ std::string& label,
                                   std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&)> p) ಸ್ಥಿರಾಂಕ {
    std::string filename = name();
    ಆದರೆ (!label.ಖಾಲಿಗಳು()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

ಮಾದರಿ <ತಾರ್ಕಿಕ T_SinksNotSources>
ಸ್ಥಿರ std::unique_ptr<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>>
dfg_ನಕ್ಷೆ_Collect_Cone(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> ಸರತಿ = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>> resp{
        ಹೊಸ std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>{}};
    // Depth first traversal
    ಆಗ (!ಸರತಿ.ಖಾಲಿಗಳು()) {
        // Pop next work item
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಸರತಿ.ಹಿಂಭಾಗ();
        ಸರತಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        // Mark vertex as visited, move on if already visited
        ಆದರೆ (!resp->ಸೇರಿಸು(vtxp).ಎರಡನೇ) ಮುಂದುವರಿ;
        // Enqueue all siblings of this vertex.
        ಆದರೆ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& sink) {
                ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&sink);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        } ಇಲ್ಲದಿದ್ದರೆ {
            vtxp->foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
                ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&src);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        }
    }
    //ಮುಗಿದ
    ಹಿಂತಿರುಗಿ resp;
}

std::unique_ptr<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>>
Dfg_ನಕ್ಷೆ::ಮೂಲ_Cone(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>& vtxps) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ dfg_ನಕ್ಷೆ_Collect_Cone<ಸುಳ್ಳು>(vtxps);
}

std::unique_ptr<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>>
Dfg_ನಕ್ಷೆ::sinkCone(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>& vtxps) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ dfg_ನಕ್ಷೆ_Collect_Cone<ನಿಜ>(vtxps);
}

std::unique_ptr<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>>
Dfg_ನಕ್ಷೆ::neighborhood(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>& vtxps, ಗಾತ್ರ_t n) ಸ್ಥಿರಾಂಕ {
    // Neighborhood
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> res{vec.begin(), vec.ಕೊನೆ()};
    // Expand neihborhood by 'n' hops
    ಗಾತ್ರ_t begin = 0;
    ಗಾತ್ರ_t ಕೊನೆ = vec.ಗಾತ್ರ_2();
    ಗಾಗಿ (ಗಾತ್ರ_t hops = 1; hops <= n; ++hops) {
        ಗಾಗಿ (ಗಾತ್ರ_t i = begin; i < ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = vec[i];
            vtxp->foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& dst) {
                ಆದರೆ (res.ಸೇರಿಸು(&dst).ಎರಡನೇ) vec.ತಳ್ಳು_ಹಿಂಭಾಗ(&dst);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            vtxp->foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
                ಆದರೆ (res.ಸೇರಿಸು(&src).ಎರಡನೇ) vec.ತಳ್ಳು_ಹಿಂಭಾಗ(&src);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        }
        begin = ಕೊನೆ;
        ಕೊನೆ = vec.ಗಾತ್ರ_2();
    }
    // Move out the results
    ಹಿಂತಿರುಗಿ std::make_unique<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

Dfg_ಶೃಂಗ::Dfg_ಶೃಂಗ(Dfg_ನಕ್ಷೆ& dfg, VDfgType type, File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dt,
                     Dfg_ಅಂಚು* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.ಕೂಡಿಸು_ಶೃಂಗ(*ಇದು);
}

ತಾರ್ಕಿಕ Dfg_ಶೃಂಗ::unsafe() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (is<DfgMux>()) ಹಿಂತಿರುಗಿ ನಿಜ;
    ಆದರೆ (is<DfgArraySel>()) ಹಿಂತಿರುಗಿ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

ಶೂನ್ಯ Dfg_ಶೃಂಗ::typeCheck(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg) ಸ್ಥಿರಾಂಕ {

#ವ್ಯಾಖ್ಯಾನಿಸು CHECK(cond, ಸಂದೇಶ_2) \
    UASSERT_OBJ(cond, ಇದು, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << ಸಂದೇಶ_2);
    ಆಯ್ಕೆ (type()) {
    ಸ್ಥಿತಿ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::AstRd: {
        ಸ್ಥಿರಾಂಕ DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::VarArray:
    ಸ್ಥಿತಿ VDfgType::VarPacked: {
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var& v = *as<Dfg_ಶೃಂಗ_Var>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::SpliceArray:
    ಸ್ಥಿತಿ VDfgType::SplicePacked: {
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Splice& v = *as<Dfg_ಶೃಂಗ_Splice>();
        v.foreachDriver([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_ದತ್ತಾಂಶ_Type::select(v.dtype(), lo, src.ಗಾತ್ರ_2()), "driver");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::UnitArray: {
        ಸ್ಥಿರಾಂಕ DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.ಗಾತ್ರ_2() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::Sel: {
        ಸ್ಥಿರಾಂಕ DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_ದತ್ತಾಂಶ_Type::select(v.srcp()->dtype(), v.lsb(), v.ಗಾತ್ರ_2()), "sel");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::ಹೊಂದಿಕೆ_Masked: {
        ಸ್ಥಿರಾಂಕ Dfg_ಹೊಂದಿಕೆ_Masked& v = *as<Dfg_ಹೊಂದಿಕೆ_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.ಗಾತ್ರ_2() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<Dfg_ಶೃಂಗ_Var>(), "Match should be a variable");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::Mux: {
        ಸ್ಥಿರಾಂಕ DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->ಗಾತ್ರ_2() >= v.ಗಾತ್ರ_2(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿತಿ VDfgType::ArraySel: {
        ಸ್ಥಿರಾಂಕ DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::ಕೂಡಿಸು_2:
    ಸ್ಥಿತಿ VDfgType::And:
    ಸ್ಥಿತಿ VDfgType::Div:
    ಸ್ಥಿತಿ VDfgType::DivS:
    ಸ್ಥಿತಿ VDfgType::ModDiv:
    ಸ್ಥಿತಿ VDfgType::ModDivS:
    ಸ್ಥಿತಿ VDfgType::Mul:
    ಸ್ಥಿತಿ VDfgType::MulS:
    ಸ್ಥಿತಿ VDfgType::Or:
    ಸ್ಥಿತಿ VDfgType::Sub:
    ಸ್ಥಿತಿ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Negate:
    ಸ್ಥಿತಿ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_L:
    ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_R:
    ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_RS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Concat: {
        ಸ್ಥಿರಾಂಕ DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.ಗಾತ್ರ_2() == v.rhsp()->ಗಾತ್ರ_2() + v.lhsp()->ಗಾತ್ರ_2(), "Concat result mismatch");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Rep: {
        ಸ್ಥಿರಾಂಕ DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.ಗಾತ್ರ_2() % v.srcp()->ಗಾತ್ರ_2() == 0, "Not a replicate");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::ಹರಿವು_L:
    ಸ್ಥಿತಿ VDfgType::ಹರಿವು_R: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Eq:
    ಸ್ಥಿತಿ VDfgType::EqCase:
    ಸ್ಥಿತಿ VDfgType::EqWild:
    ಸ್ಥಿತಿ VDfgType::Neq:
    ಸ್ಥಿತಿ VDfgType::NeqCase:
    ಸ್ಥಿತಿ VDfgType::NeqWild:
    ಸ್ಥಿತಿ VDfgType::Gt:
    ಸ್ಥಿತಿ VDfgType::GtS:
    ಸ್ಥಿತಿ VDfgType::Gte:
    ಸ್ಥಿತಿ VDfgType::GteS:
    ಸ್ಥಿತಿ VDfgType::Lt:
    ಸ್ಥಿತಿ VDfgType::LtS:
    ಸ್ಥಿತಿ VDfgType::Lte:
    ಸ್ಥಿತಿ VDfgType::LteS: {
        CHECK(dtype() == Dfg_ದತ್ತಾಂಶ_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Extend:
    ಸ್ಥಿತಿ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->ಗಾತ್ರ_2() < ಗಾತ್ರ_2(), "Operand should be narrower");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::LogAnd:
    ಸ್ಥಿತಿ VDfgType::LogEq:
    ಸ್ಥಿತಿ VDfgType::LogIf:
    ಸ್ಥಿತಿ VDfgType::LogOr: {
        CHECK(dtype() == Dfg_ದತ್ತಾಂಶ_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::LogNot:
    ಸ್ಥಿತಿ VDfgType::RedAnd:
    ಸ್ಥಿತಿ VDfgType::RedOr:
    ಸ್ಥಿತಿ VDfgType::RedXor:
    ಸ್ಥಿತಿ VDfgType::OneHot:
    ಸ್ಥಿತಿ VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_ದತ್ತಾಂಶ_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Cond: {
        ಸ್ಥಿರಾಂಕ DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::ಎಣಿಕೆ_Ones_2: {
        ಸ್ಥಿರಾಂಕ Dfg_ಎಣಿಕೆ_Ones& v = *as<Dfg_ಎಣಿಕೆ_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.ಗಾತ್ರ_2() == 32U, "Should yield a 32-bit result");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿತಿ VDfgType::Pow:
    ಸ್ಥಿತಿ VDfgType::PowSS:
    ಸ್ಥಿತಿ VDfgType::PowSU:
    ಸ್ಥಿತಿ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ಹಿಂತಿರುಗಿ;
    }
    }
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು CHECK
}

uint32_t Dfg_ಶೃಂಗ::fanout() ಸ್ಥಿರಾಂಕ {
    uint32_t ಫಲಿತಾಂಶ = 0;
    foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&) {
        ++ಫಲಿತಾಂಶ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    });
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

Dfg_ಶೃಂಗ_Var* Dfg_ಶೃಂಗ::get_ಫಲಿತಾಂಶ_Var() {
    // It's easy if the vertex is already a variable ...
    ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = ಇದು->cast<Dfg_ಶೃಂಗ_Var>()) ಹಿಂತಿರುಗಿ varp;

    // Inspect existing variables written by this vertex, and choose one
    Dfg_ಶೃಂಗ_Var* resp = ಶೂನ್ಯಸೂಚಕ;
    // cppcheck-has-bug-suppress constParameter
    ಇದು->foreachSink([&resp](Dfg_ಶೃಂಗ& sink) {
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = sink.cast<Dfg_ಶೃಂಗ_Var>();
        ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Do not use it if value might differ from its drivers
        ಆದರೆ (varp->isVolatile()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // First variable found
        ಆದರೆ (!resp) {
            resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Prefer those variables that must be kept anyway
        ಆದರೆ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ಆದರೆ (!resp->hasExtRdRefs()) resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ಆದರೆ (!resp->hasDfgRefs()) resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Prefer real variables over temporaries
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ res_Is_ತಾತ್ಕಾಲಿಕ = resp->vscp()->varp()->is_ತಾತ್ಕಾಲಿಕ();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ var_Is_ತಾತ್ಕಾಲಿಕ = varp->vscp()->varp()->is_ತಾತ್ಕಾಲಿಕ();
        ಆದರೆ (res_Is_ತಾತ್ಕಾಲಿಕ != var_Is_ತಾತ್ಕಾಲಿಕ) {
            ಆದರೆ (res_Is_ತಾತ್ಕಾಲಿಕ) resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (!resp->tmpForp() != !varp->tmpForp()) {
            ಆದರೆ (resp->tmpForp()) resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Prefer the earlier one in source order
        ಸ್ಥಿರಾಂಕ File_ಸಾಲು& oldFlp = *(resp->fileline());
        ಸ್ಥಿರಾಂಕ File_ಸಾಲು& newFlp = *(varp->fileline());
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ cmp = oldFlp.operatorCompare(newFlp)) {
            ಆದರೆ (cmp > 0) resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // Prefer the one with the lexically smaller name
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ಆದರೆ (cmp > 0) resp = varp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    });
    ಹಿಂತಿರುಗಿ resp;
}

AstScope* Dfg_ಶೃಂಗ::scopep(ScopeCache& cache, ತಾರ್ಕಿಕ try_ಫಲಿತಾಂಶ_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = ಇದು->cast<Dfg_ಶೃಂಗ_Var>()) {
        ಹಿಂತಿರುಗಿ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ಆದರೆ (try_ಫಲಿತಾಂಶ_Var) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = ಇದು->get_ಫಲಿತಾಂಶ_Var()) {
            ಹಿಂತಿರುಗಿ varp->vscp()->scopep();
        }
    }

    AstScope* ಸ್ಥಿರಾಂಕ rootp = v3Global.rootp()->ತುದಿ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[ಇದು];
    ಆದರೆ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = ಶೂನ್ಯಸೂಚಕ;
        foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            AstScope* ಸ್ಥಿರಾಂಕ scp = src.scopep(cache, ನಿಜ);
            ಆದರೆ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), ಇದು,
                "DfgVertex::scopep called on graph with circular operations");

    //ಮುಗಿದ
    ಹಿಂತಿರುಗಿ resultr;
}

ಶೂನ್ಯ Dfg_ಶೃಂಗ::unlinkDelete(Dfg_ನಕ್ಷೆ& dfg) {
    // Unlink sink edges
    ಆಗ (!m_sinks.ಖಾಲಿಗಳು()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.remove_ಶೃಂಗ(*ಇದು);
    // Delete - this will unlink sources
    ಅಳಿಸು ಇದು;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<Dfg_ಶೃಂಗ*, uint32_t> Dfg_ಶೃಂಗ_Var::driver_ದ_Range(uint32_t lo, uint32_t ಗಾತ್ರ_2) {
    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = ಇದು->srcp();
    // Not driven at all
    ಆದರೆ (!srcp) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
    // If volatile, can have other drivers
    ಆದರೆ (isVolatile()) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
    // Don't inline CReset
    ಆದರೆ (srcp->is<DfgCReset>()) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ splicep = srcp->cast<Dfg_ಶೃಂಗ_Splice>();
    ಆದರೆ (!splicep) ಹಿಂತಿರುಗಿ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ಸ್ಥಿರಾಂಕ uint32_t hi = lo + ಗಾತ್ರ_2 - 1;
    Dfg_ಶೃಂಗ* driverp = ಶೂನ್ಯಸೂಚಕ;
    uint32_t driverLo = 0;
    ತಾರ್ಕಿಕ useDefault = defaultp();
    splicep->foreachDriver([&](Dfg_ಶೃಂಗ& src, ಸ್ಥಿರಾಂಕ uint32_t dLo) {
        ಸ್ಥಿರಾಂಕ uint32_t dHi = dLo + src.ಗಾತ್ರ_2() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ಆದರೆ (dLo <= hi && lo <= dHi) useDefault = ಸುಳ್ಳು;
        // If it does not cover the whole searched range, move on
        ಆದರೆ (lo < dLo || dHi < hi) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    });

    // If a single driver covers the searched range, it is the one
    ಆದರೆ (driverp) ಹಿಂತಿರುಗಿ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ಆದರೆ (useDefault) ಹಿಂತಿರುಗಿ {defaultp(), lo};
    // Not driven by a single vertex
    ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

ವರ್ಗ Dfg_ನಮೂನೆ_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_interned_ಶಬ್ದ_Widths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_ಆಳ;

    ಸ್ಥಿರ std::string ಗಮ್ಯ_Letters(ಗಾತ್ರ_t ಮೌಲ್ಯ, ತಾರ್ಕಿಕ ಅಧೋ_Case = ಸುಳ್ಳು) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ತಳ = ಅಧೋ_Case ? 'a' : 'A';
        std::string s;
        ಮಾಡು { s += static_cast<ಅಕ್ಷರ>(ತಳ + ಮೌಲ್ಯ % 26); } ಆಗ (ಮೌಲ್ಯ /= 26);
        ಹಿಂತಿರುಗಿ s;
    }

    ಸ್ಥಿರಾಂಕ std::string& internConst(ಸ್ಥಿರಾಂಕ DfgConst& vtx) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_internedConsts.emplace(vtx.ಅಂಕೆ().ascii(ಸುಳ್ಳು), "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_internedConsts.ಗಾತ್ರ_2() - 1);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ std::string& internSelLsb(uint32_t ಮೌಲ್ಯ) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_internedSelLsbs.emplace(ಮೌಲ್ಯ, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_internedSelLsbs.ಗಾತ್ರ_2() - 1);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ std::string& intern_ಶಬ್ದ_ಅಗಲ(uint32_t ಮೌಲ್ಯ) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_interned_ಶಬ್ದ_Widths.emplace(ಮೌಲ್ಯ, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_interned_ಶಬ್ದ_Widths.ಗಾತ್ರ_2() - 1, ನಿಜ);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ std::string& intern_Wide_ಅಗಲ(uint32_t ಮೌಲ್ಯ) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_internedWideWidths.emplace(ಮೌಲ್ಯ, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_internedWideWidths.ಗಾತ್ರ_2() - 1);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ std::string& intern_ಶೃಂಗ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_internedVertices.emplace(&vtx, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_internedVertices.ಗಾತ್ರ_2() - 1);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಶೂನ್ಯ ದಾಖಲೆ_Multiplicity_And_ಆಳ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx, uint32_t ಆಳ) {
        std::pair<uint32_t, uint32_t>& ಮೌಲ್ಯ = m_multiplicity_And_ಆಳ
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, ಆಳ))
                                                   .ಮೊದಲ->ಎರಡನೇ;
        ಮೌಲ್ಯ.ಮೊದಲ += 1;
        ಮೌಲ್ಯ.ಎರಡನೇ = std::max(ಮೌಲ್ಯ.ಎರಡನೇ, ಆಳ);
        ಆದರೆ (!ಆಳ) ಹಿಂತಿರುಗಿ;
        vtx.foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
            ದಾಖಲೆ_Multiplicity_And_ಆಳ(src, ಆಳ - 1);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    ಶೂನ್ಯ render(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx, uint32_t ಆಳ, ತಾರ್ಕಿಕ is_ಬೇರು_2 = ನಿಜ) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ಆದರೆ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!is_ಬೇರು_2 && m_multiplicity_And_ಆಳ.at(&vtx).ಮೊದಲ > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << intern_ಶೃಂಗ(vtx);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!vtx.foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&) { ಹಿಂತಿರುಗಿ ನಿಜ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಆಳ == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ಆದರೆ (selp->lsb() == 0) {
                    m_os << '0';
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
                m_os << ' ';
                render(src, ಆಳ - 1, ಸುಳ್ಳು);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ಆದರೆ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtx.ಗಾತ್ರ_2();
            ಆದರೆ (ಅಗಲ == 1) {
                m_os << '1';
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ <= VL_QUADSIZE) {
                m_os << intern_ಶಬ್ದ_ಅಗಲ(ಅಗಲ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_os << intern_Wide_ಅಗಲ(ಅಗಲ);
            }
        }

        // Mark it if it has multiple sinks
        ಆದರೆ (vtx.has_ಅಪವರ್ತ್ಯ_Sinks()) m_os << '*';
    }

ಸಾರ್ವಜನಿಕ:
    Dfg_ನಮೂನೆ_String(std::ostream& os, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx, uint32_t ಆಳ)
        : m_os{os} {
        ದಾಖಲೆ_Multiplicity_And_ಆಳ(vtx, ಆಳ);
        render(vtx, ಆಳ, ಸುಳ್ಳು);
        ಬಳಸಿ Pair = std::pair<std::string, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>;
        std::ಸದಿಶ<Pair> vertices;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_multiplicity_And_ಆಳ) {
            ಆದರೆ (pair.ಎರಡನೇ.ಮೊದಲ == 1) ಮುಂದುವರಿ;
            vertices.emplace_ಹಿಂಭಾಗ(intern_ಶೃಂಗ(*pair.ಮೊದಲ), pair.ಮೊದಲ);
        }
        std::ವಿಂಗಡಿಸು(vertices.begin(), vertices.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Pair& a, ಸ್ಥಿರಾಂಕ Pair& b) {  //
            ಹಿಂತಿರುಗಿ a.ಮೊದಲ < b.ಮೊದಲ;
        });
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Pair& pair : vertices) {
            m_os << " | " << pair.ಮೊದಲ << " is ";
            render(*pair.ಎರಡನೇ, m_multiplicity_And_ಆಳ.at(pair.ಎರಡನೇ).ಎರಡನೇ);
        }
    }
};

std::string Dfg_ಶೃಂಗ::ನಮೂನೆ_String(uint32_t ಆಳ) ಸ್ಥಿರಾಂಕ {
    std::ostringstream oss;
    Dfg_ನಮೂನೆ_String{oss, *ಇದು, ಆಳ};
    ಹಿಂತಿರುಗಿ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ಅನ್ತರ್ಭಾವ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
