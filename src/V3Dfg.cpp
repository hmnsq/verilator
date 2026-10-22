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

Dfg_گراف::Dfg_گراف(V3DfgContext& ctx, ثابت string& name)
    : m_ctx{ctx}
    , m_name{name} {}

Dfg_گراف::~Dfg_گراف() {
    for_Each_راس([&](Dfg_راس& vtx) { vtx.unlinkDelete(*یہ); });
}

خالی Dfg_گراف::ملاؤ_Graphs(std::سمتیہ<std::unique_ptr<Dfg_گراف>>&& otherps) {
    اگر (otherps.خالی_خانے()) واپس;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ثابت VNUser2_اندر_Use user2_اندر_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    برائے (Dfg_راس_Var& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    برائے (ثابت std::unique_ptr<Dfg_گراف>& otherp : otherps) {
        // Process variables
        برائے (Dfg_راس_Var* ثابت vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            اگر (Dfg_راس_Var* ثابت altp = vtxp->vscp()->user2u().مقصد<Dfg_راس_Var*>()) {
                Dfg_راس* ثابت srcp = vtxp->srcp();
                Dfg_راس* ثابت defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                اگر (srcp) altp->srcp(srcp);
                اگر (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                جاری;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#اگر_تعریف_شدہ VL_DEBUG
            vtxp->m_dfgp = یہ;
#اگر_ختم
        }
        m_varVertices.splice(m_varVertices.انتہا(), otherp->m_varVertices);
        // Process Ast references
        برائے (Dfg_راس_Ast* ثابت vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#اگر_تعریف_شدہ VL_DEBUG
            vtxp->m_dfgp = یہ;
#اگر_ختم
        }
        m_astVertices.splice(m_astVertices.انتہا(), otherp->m_astVertices);
        // Process constants
        برائے (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#اگر_تعریف_شدہ VL_DEBUG
            vtx.m_dfgp = یہ;
#اگر_ختم
        }
        m_constVertices.splice(m_constVertices.انتہا(), otherp->m_constVertices);
        // Process operations
        برائے (Dfg_راس& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#اگر_تعریف_شدہ VL_DEBUG
            vtx.m_dfgp = یہ;
#اگر_ختم
        }
        m_opVertices.splice(m_opVertices.انتہا(), otherp->m_opVertices);
        // Update graph sizes
        m_سائز += otherp->m_سائز;
        otherp->m_سائز = 0;
    }
}

Dfg_راس_Var* Dfg_گراف::makeNewVar(File_سطر* flp, ثابت std::string& سابقہ,
                                   ثابت Dfg_ڈیٹا_Type& dtype, AstScope* scopep) {
    AstVarScope* ثابت vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), سابقہ);
    // Create and return the corresponding variable vertex
    اگر (dtype.isArray()) واپس نیا DfgVarArray{*یہ, vscp};
    واپس نیا DfgVarPacked{*یہ, vscp};
}

ساکن ثابت std::string مقصد_Dot_Id(ثابت Dfg_راس& vtx) { واپس '"' + cvt_مقصد_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ساکن خالی dump_Dot_راس(std::ostream& os, ثابت Dfg_راس& vtx) {
    اگر (ثابت Dfg_راس_Var* ثابت varVtxp = vtx.cast<Dfg_راس_Var>()) {
        ثابت AstVarScope* ثابت vscp = varVtxp->vscp();
        os << مقصد_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //پتہ
        os << '\n' << cvt_مقصد_Hex(varVtxp);
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
        ساکن ثابت حرف* ثابت rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[جھوٹ][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        اگر (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ورنہ اگر (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ورنہ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ثابت حرف* ثابت colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  //سرمئی
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        واپس;
    }

    اگر (ثابت Dfg_پچھلا* ثابت پچھلا_Vtxp = vtx.cast<Dfg_پچھلا>()) {
        ثابت AstVarScope* ثابت vscp = پچھلا_Vtxp->vscp();
        os << مقصد_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //پتہ
        os << '\n' << cvt_مقصد_Hex(پچھلا_Vtxp);
        // Type and fanout
        os << '\n';
        پچھلا_Vtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << پچھلا_Vtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        اگر (پچھلا_Vtxp->isPacked()) {
            os << ", shape=box";
        } ورنہ اگر (پچھلا_Vtxp->isArray()) {
            os << ", shape=box3d";
        } ورنہ {
            پچھلا_Vtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ثابت حرف* ثابت colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        واپس;
    }

    اگر (ثابت DfgConst* ثابت constVtxp = vtx.cast<DfgConst>()) {
        ثابت V3_نمبر& عدد = constVtxp->عدد();

        os << مقصد_Dot_Id(vtx);
        os << " [label=\"";
        اگر (عدد.چوڑائی() <= 32 && !عدد.isSigned()) {
            os << constVtxp->چوڑائی() << "'d" << عدد.مقصد_UInt() << '\n';
            os << constVtxp->چوڑائی() << "'h" << std::hex << عدد.مقصد_UInt() << std::dec << '\n';
        } ورنہ {
            os << عدد.ascii() << '\n';
        }
        os << cvt_مقصد_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        واپس;
    }

    اگر (ثابت DfgSel* ثابت selVtxp = vtx.cast<DfgSel>()) {
        ثابت uint32_t lsb = selVtxp->lsb();
        ثابت uint32_t msb = lsb + selVtxp->چوڑائی() - 1;
        os << مقصد_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_مقصد_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        اگر (vtx.has_مضرب_Sinks()) {
            os << ", shape=doublecircle";
        } ورنہ {
            os << ", shape=circle";
        }
        os << "]\n";
        واپس;
    }

    اگر (vtx.is<Dfg_راس_Splice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << مقصد_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_مقصد_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        اگر (vtx.has_مضرب_Sinks()) {
            os << ", shape=doubleoctagon";
        } ورنہ {
            os << ", shape=octagon";
        }
        os << "]\n";
        واپس;
    }

    اگر (ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) {
        os << مقصد_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_درخت(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_مقصد_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت حرف* ثابت colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_مقصد_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        واپس;
    }

    اگر (ثابت Dfg_راس_Ast* ثابت astVtxp = vtx.cast<Dfg_راس_Ast>()) {
        os << مقصد_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_درخت(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_مقصد_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت DfgAstRd* ثابت astRdVtxp = astVtxp->cast<DfgAstRd>();
        ثابت حرف* ثابت colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_مقصد_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->اندر_Sen_شے()];
        os << "_L"[astRdVtxp->اندر_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        واپس;
    }

    os << مقصد_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_مقصد_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    اگر (vtx.has_مضرب_Sinks()) {
        os << ", shape=doublecircle";
    } ورنہ {
        os << ", shape=circle";
    }
    os << "]\n";
}

خالی Dfg_گراف::dumpDot(std::ostream& os, ثابت std::string& label,
                       std::function<منطقی(ثابت Dfg_راس&)> p) ثابت {
    // This generates a graphviz dump, https://www.graphviz.org

    //سرخی
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    اگر (!p) p = [](ثابت Dfg_راس&) { واپس سچ; };

    std::unordered_سیٹ<ثابت Dfg_راس*> emitted;
    // Emit all vertices associated with a DfgLogic
    for_Each_راس([&](ثابت Dfg_راس& vtx) {
        ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        اگر (!logicp) واپس;
        اگر (logicp->synth().خالی_خانے()) واپس;
        اگر (!p(vtx)) واپس;
        os << "subgraph cluster_" << cvt_مقصد_Hex(logicp) << " {\n";
        dump_Dot_راس(os, *logicp);
        emitted.داخل_کرو(logicp);
        برائے (Dfg_راس* ثابت vtxp : logicp->synth()) {
            اگر (!p(*vtxp)) جاری;
            dump_Dot_راس(os, *vtxp);
            emitted.داخل_کرو(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    for_Each_راس([&](ثابت Dfg_راس& vtx) {
        اگر (emitted.گنتی(&vtx)) واپس;
        اگر (!p(vtx)) واپس;
        dump_Dot_راس(os, vtx);
    });
    // Emit all edges
    for_Each_راس([&](ثابت Dfg_راس& vtx) {
        اگر (!p(vtx)) واپس;
        برائے (سائز_t i = 0; i < vtx.nInputs(); ++i) {
            Dfg_راس* ثابت srcp = vtx.inputp(i);
            اگر (!srcp) جاری;
            اگر (!p(*srcp)) جاری;
            os << مقصد_Dot_Id(*srcp) << " -> " << مقصد_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.خالی_خانے() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string Dfg_گراف::dumpDotString(ثابت std::string& label,
                                    std::function<منطقی(ثابت Dfg_راس&)> p) ثابت {
    std::stringstream ss;
    dumpDot(ss, label, p);
    واپس ss.str();
}

خالی Dfg_گراف::dumpDotFile(ثابت std::string& filename, ثابت std::string& label,
                           std::function<منطقی(ثابت Dfg_راس&)> p) ثابت {
    ثابت std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    اگر (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

خالی Dfg_گراف::dumpDotFilePrefixed(ثابت std::string& label,
                                   std::function<منطقی(ثابت Dfg_راس&)> p) ثابت {
    std::string filename = name();
    اگر (!label.خالی_خانے()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

سانچہ <منطقی T_SinksNotSources>
ساکن std::unique_ptr<std::unordered_سیٹ<ثابت Dfg_راس*>>
dfg_گراف_Collect_Cone(ثابت std::سمتیہ<ثابت Dfg_راس*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::سمتیہ<ثابت Dfg_راس*> قطار = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_سیٹ<ثابت Dfg_راس*>> resp{
        نیا std::unordered_سیٹ<ثابت Dfg_راس*>{}};
    // Depth first traversal
    جب (!قطار.خالی_خانے()) {
        // Pop next work item
        ثابت Dfg_راس* ثابت vtxp = قطار.عقب();
        قطار.نکالو_عقب();
        // Mark vertex as visited, move on if already visited
        اگر (!resp->داخل_کرو(vtxp).دوسرا) جاری;
        // Enqueue all siblings of this vertex.
        اگر VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ثابت Dfg_راس& sink) {
                قطار.ڈالو_عقب(&sink);
                واپس جھوٹ;
            });
        } ورنہ {
            vtxp->foreach_ماخذ([&](ثابت Dfg_راس& src) {
                قطار.ڈالو_عقب(&src);
                واپس جھوٹ;
            });
        }
    }
    //مکمل
    واپس resp;
}

std::unique_ptr<std::unordered_سیٹ<ثابت Dfg_راس*>>
Dfg_گراف::ماخذ_Cone(ثابت std::سمتیہ<ثابت Dfg_راس*>& vtxps) ثابت {
    واپس dfg_گراف_Collect_Cone<جھوٹ>(vtxps);
}

std::unique_ptr<std::unordered_سیٹ<ثابت Dfg_راس*>>
Dfg_گراف::sinkCone(ثابت std::سمتیہ<ثابت Dfg_راس*>& vtxps) ثابت {
    واپس dfg_گراف_Collect_Cone<سچ>(vtxps);
}

std::unique_ptr<std::unordered_سیٹ<ثابت Dfg_راس*>>
Dfg_گراف::neighborhood(ثابت std::سمتیہ<ثابت Dfg_راس*>& vtxps, سائز_t n) ثابت {
    // Neighborhood
    std::سمتیہ<ثابت Dfg_راس*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_سیٹ<ثابت Dfg_راس*> res{vec.begin(), vec.انتہا()};
    // Expand neihborhood by 'n' hops
    سائز_t begin = 0;
    سائز_t انتہا = vec.سائز();
    برائے (سائز_t hops = 1; hops <= n; ++hops) {
        برائے (سائز_t i = begin; i < انتہا; ++i) {
            ثابت Dfg_راس* ثابت vtxp = vec[i];
            vtxp->foreachSink([&](ثابت Dfg_راس& dst) {
                اگر (res.داخل_کرو(&dst).دوسرا) vec.ڈالو_عقب(&dst);
                واپس جھوٹ;
            });
            vtxp->foreach_ماخذ([&](ثابت Dfg_راس& src) {
                اگر (res.داخل_کرو(&src).دوسرا) vec.ڈالو_عقب(&src);
                واپس جھوٹ;
            });
        }
        begin = انتہا;
        انتہا = vec.سائز();
    }
    // Move out the results
    واپس std::make_unique<std::unordered_سیٹ<ثابت Dfg_راس*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

Dfg_راس::Dfg_راس(Dfg_گراف& dfg, VDfgType type, File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dt,
                     Dfg_کنارہ* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.جوڑو_راس(*یہ);
}

منطقی Dfg_راس::unsafe() ثابت {
    اگر (is<DfgMux>()) واپس سچ;
    اگر (is<DfgArraySel>()) واپس !as<DfgArraySel>()->bitp()->is<DfgConst>();
    واپس جھوٹ;
}

خالی Dfg_راس::typeCheck(ثابت Dfg_گراف& dfg) ثابت {

#تعریف_کرو CHECK(cond, پیغام_2) \
    UASSERT_OBJ(cond, یہ, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << پیغام_2);
    انتخاب (type()) {
    حالت VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        واپس;
    }
    حالت VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        واپس;
    }
    حالت VDfgType::AstRd: {
        ثابت DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        واپس;
    }
    حالت VDfgType::VarArray:
    حالت VDfgType::VarPacked: {
        ثابت Dfg_راس_Var& v = *as<Dfg_راس_Var>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        واپس;
    }
    حالت VDfgType::پچھلا: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        واپس;
    }
    حالت VDfgType::SpliceArray:
    حالت VDfgType::SplicePacked: {
        ثابت Dfg_راس_Splice& v = *as<Dfg_راس_Splice>();
        v.foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_ڈیٹا_Type::select(v.dtype(), lo, src.سائز()), "driver");
            واپس جھوٹ;
        });
        واپس;
    }
    حالت VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        واپس;
    }
    حالت VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        واپس;
    }
    حالت VDfgType::UnitArray: {
        ثابت DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.سائز() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        واپس;
    }
    حالت VDfgType::Sel: {
        ثابت DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_ڈیٹا_Type::select(v.srcp()->dtype(), v.lsb(), v.سائز()), "sel");
        واپس;
    }
    حالت VDfgType::مطابقت_Masked: {
        ثابت Dfg_مطابقت_Masked& v = *as<Dfg_مطابقت_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.سائز() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<Dfg_راس_Var>(), "Match should be a variable");
        واپس;
    }
    حالت VDfgType::Mux: {
        ثابت DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->سائز() >= v.سائز(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        واپس;
    }
    حالت VDfgType::ArraySel: {
        ثابت DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        واپس;
    }

    حالت VDfgType::جوڑو_2:
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
        واپس;
    }

    حالت VDfgType::Negate:
    حالت VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        واپس;
    }

    حالت VDfgType::سرکاؤ_L:
    حالت VDfgType::سرکاؤ_R:
    حالت VDfgType::سرکاؤ_RS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        واپس;
    }

    حالت VDfgType::Concat: {
        ثابت DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.سائز() == v.rhsp()->سائز() + v.lhsp()->سائز(), "Concat result mismatch");
        واپس;
    }

    حالت VDfgType::Rep: {
        ثابت DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.سائز() % v.srcp()->سائز() == 0, "Not a replicate");
        واپس;
    }

    حالت VDfgType::دھارا_L:
    حالت VDfgType::دھارا_R: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        واپس;
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
        CHECK(dtype() == Dfg_ڈیٹا_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        واپس;
    }

    حالت VDfgType::Extend:
    حالت VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->سائز() < سائز(), "Operand should be narrower");
        واپس;
    }

    حالت VDfgType::LogAnd:
    حالت VDfgType::LogEq:
    حالت VDfgType::LogIf:
    حالت VDfgType::LogOr: {
        CHECK(dtype() == Dfg_ڈیٹا_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        واپس;
    }

    حالت VDfgType::LogNot:
    حالت VDfgType::RedAnd:
    حالت VDfgType::RedOr:
    حالت VDfgType::RedXor:
    حالت VDfgType::OneHot:
    حالت VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_ڈیٹا_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        واپس;
    }

    حالت VDfgType::Cond: {
        ثابت DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        واپس;
    }

    حالت VDfgType::گنتی_Ones_2: {
        ثابت Dfg_گنتی_Ones& v = *as<Dfg_گنتی_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.سائز() == 32U, "Should yield a 32-bit result");
        واپس;
    }

    حالت VDfgType::Pow:
    حالت VDfgType::PowSS:
    حالت VDfgType::PowSU:
    حالت VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        واپس;
    }
    }
#تعریف_منسوخ CHECK
}

uint32_t Dfg_راس::fanout() ثابت {
    uint32_t نتیجہ = 0;
    foreachSink([&](ثابت Dfg_راس&) {
        ++نتیجہ;
        واپس جھوٹ;
    });
    واپس نتیجہ;
}

Dfg_راس_Var* Dfg_راس::get_نتیجہ_Var() {
    // It's easy if the vertex is already a variable ...
    اگر (Dfg_راس_Var* ثابت varp = یہ->cast<Dfg_راس_Var>()) واپس varp;

    // Inspect existing variables written by this vertex, and choose one
    Dfg_راس_Var* resp = خالی_اشاریہ;
    // cppcheck-has-bug-suppress constParameter
    یہ->foreachSink([&resp](Dfg_راس& sink) {
        Dfg_راس_Var* ثابت varp = sink.cast<Dfg_راس_Var>();
        اگر (!varp) واپس جھوٹ;

        // Do not use it if value might differ from its drivers
        اگر (varp->isVolatile()) واپس جھوٹ;

        // First variable found
        اگر (!resp) {
            resp = varp;
            واپس جھوٹ;
        }

        // Prefer those variables that must be kept anyway
        اگر (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            اگر (!resp->hasExtRdRefs()) resp = varp;
            واپس جھوٹ;
        }
        اگر (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            اگر (!resp->hasDfgRefs()) resp = varp;
            واپس جھوٹ;
        }

        // Prefer real variables over temporaries
        ثابت منطقی res_Is_عارضی = resp->vscp()->varp()->is_عارضی();
        ثابت منطقی var_Is_عارضی = varp->vscp()->varp()->is_عارضی();
        اگر (res_Is_عارضی != var_Is_عارضی) {
            اگر (res_Is_عارضی) resp = varp;
            واپس جھوٹ;
        }
        اگر (!resp->tmpForp() != !varp->tmpForp()) {
            اگر (resp->tmpForp()) resp = varp;
            واپس جھوٹ;
        }

        // Prefer the earlier one in source order
        ثابت File_سطر& oldFlp = *(resp->fileline());
        ثابت File_سطر& newFlp = *(varp->fileline());
        اگر (ثابت صحیح cmp = oldFlp.operatorCompare(newFlp)) {
            اگر (cmp > 0) resp = varp;
            واپس جھوٹ;
        }
        // Prefer the one with the lexically smaller name
        اگر (ثابت صحیح cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            اگر (cmp > 0) resp = varp;
            واپس جھوٹ;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        واپس جھوٹ;
    });
    واپس resp;
}

AstScope* Dfg_راس::scopep(ScopeCache& cache, منطقی try_نتیجہ_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    اگر (ثابت Dfg_راس_Var* ثابت varp = یہ->cast<Dfg_راس_Var>()) {
        واپس varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    اگر (try_نتیجہ_Var) {
        اگر (ثابت Dfg_راس_Var* ثابت varp = یہ->get_نتیجہ_Var()) {
            واپس varp->vscp()->scopep();
        }
    }

    AstScope* ثابت rootp = v3Global.rootp()->چوٹی_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[یہ];
    اگر (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = خالی_اشاریہ;
        foreach_ماخذ([&](Dfg_راس& src) {
            AstScope* ثابت scp = src.scopep(cache, سچ);
            اگر (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                واپس سچ;
            }
            واپس جھوٹ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), یہ,
                "DfgVertex::scopep called on graph with circular operations");

    //مکمل
    واپس resultr;
}

خالی Dfg_راس::unlinkDelete(Dfg_گراف& dfg) {
    // Unlink sink edges
    جب (!m_sinks.خالی_خانے()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.remove_راس(*یہ);
    // Delete - this will unlink sources
    حذف یہ;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<Dfg_راس*, uint32_t> Dfg_راس_Var::driver_کا_Range(uint32_t lo, uint32_t سائز) {
    Dfg_راس* ثابت srcp = یہ->srcp();
    // Not driven at all
    اگر (!srcp) واپس {خالی_اشاریہ, 0};
    // If volatile, can have other drivers
    اگر (isVolatile()) واپس {خالی_اشاریہ, 0};
    // Don't inline CReset
    اگر (srcp->is<DfgCReset>()) واپس {خالی_اشاریہ, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    Dfg_راس_Splice* ثابت splicep = srcp->cast<Dfg_راس_Splice>();
    اگر (!splicep) واپس {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ثابت uint32_t hi = lo + سائز - 1;
    Dfg_راس* driverp = خالی_اشاریہ;
    uint32_t driverLo = 0;
    منطقی useDefault = defaultp();
    splicep->foreachDriver([&](Dfg_راس& src, ثابت uint32_t dLo) {
        ثابت uint32_t dHi = dLo + src.سائز() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        اگر (dLo <= hi && lo <= dHi) useDefault = جھوٹ;
        // If it does not cover the whole searched range, move on
        اگر (lo < dLo || dHi < hi) واپس جھوٹ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        واپس سچ;
    });

    // If a single driver covers the searched range, it is the one
    اگر (driverp) واپس {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    اگر (useDefault) واپس {defaultp(), lo};
    // Not driven by a single vertex
    واپس {خالی_اشاریہ, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

جماعت Dfg_نمونہ_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_interned_لفظ_Widths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ثابت Dfg_راس*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ثابت Dfg_راس*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_گہرائی;

    ساکن std::string مقصد_Letters(سائز_t قدر, منطقی زیریں_Case = جھوٹ) {
        ثابت حرف بنیاد = زیریں_Case ? 'a' : 'A';
        std::string s;
        کرو { s += static_cast<حرف>(بنیاد + قدر % 26); } جب (قدر /= 26);
        واپس s;
    }

    ثابت std::string& internConst(ثابت DfgConst& vtx) {
        ثابت خودکار pair = m_internedConsts.emplace(vtx.عدد().ascii(جھوٹ), "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_internedConsts.سائز() - 1);
        واپس pair.پہلا->دوسرا;
    }

    ثابت std::string& internSelLsb(uint32_t قدر) {
        ثابت خودکار pair = m_internedSelLsbs.emplace(قدر, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_internedSelLsbs.سائز() - 1);
        واپس pair.پہلا->دوسرا;
    }

    ثابت std::string& intern_لفظ_چوڑائی(uint32_t قدر) {
        ثابت خودکار pair = m_interned_لفظ_Widths.emplace(قدر, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_interned_لفظ_Widths.سائز() - 1, سچ);
        واپس pair.پہلا->دوسرا;
    }

    ثابت std::string& intern_Wide_چوڑائی(uint32_t قدر) {
        ثابت خودکار pair = m_internedWideWidths.emplace(قدر, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_internedWideWidths.سائز() - 1);
        واپس pair.پہلا->دوسرا;
    }

    ثابت std::string& intern_راس(ثابت Dfg_راس& vtx) {
        ثابت خودکار pair = m_internedVertices.emplace(&vtx, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_internedVertices.سائز() - 1);
        واپس pair.پہلا->دوسرا;
    }

    خالی ریکارڈ_Multiplicity_And_گہرائی(ثابت Dfg_راس& vtx, uint32_t گہرائی) {
        std::pair<uint32_t, uint32_t>& قدر = m_multiplicity_And_گہرائی
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, گہرائی))
                                                   .پہلا->دوسرا;
        قدر.پہلا += 1;
        قدر.دوسرا = std::max(قدر.دوسرا, گہرائی);
        اگر (!گہرائی) واپس;
        vtx.foreach_ماخذ([&](ثابت Dfg_راس& src) {
            ریکارڈ_Multiplicity_And_گہرائی(src, گہرائی - 1);
            واپس جھوٹ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    خالی render(ثابت Dfg_راس& vtx, uint32_t گہرائی, منطقی is_جڑ_2 = سچ) {
        اگر (ثابت DfgConst* ثابت constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            اگر (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ورنہ اگر (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ورنہ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ورنہ اگر (!is_جڑ_2 && m_multiplicity_And_گہرائی.at(&vtx).پہلا > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << intern_راس(vtx);
        } ورنہ اگر (!vtx.foreach_ماخذ([&](ثابت Dfg_راس&) { واپس سچ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ورنہ اگر (گہرائی == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ورنہ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            اگر (ثابت DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                اگر (selp->lsb() == 0) {
                    m_os << '0';
                } ورنہ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreach_ماخذ([&](ثابت Dfg_راس& src) {
                m_os << ' ';
                render(src, گہرائی - 1, جھوٹ);
                واپس جھوٹ;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        اگر (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ورنہ {
            ثابت uint32_t چوڑائی = vtx.سائز();
            اگر (چوڑائی == 1) {
                m_os << '1';
            } ورنہ اگر (چوڑائی <= VL_QUADSIZE) {
                m_os << intern_لفظ_چوڑائی(چوڑائی);
            } ورنہ {
                m_os << intern_Wide_چوڑائی(چوڑائی);
            }
        }

        // Mark it if it has multiple sinks
        اگر (vtx.has_مضرب_Sinks()) m_os << '*';
    }

عوامی:
    Dfg_نمونہ_String(std::ostream& os, ثابت Dfg_راس& vtx, uint32_t گہرائی)
        : m_os{os} {
        ریکارڈ_Multiplicity_And_گہرائی(vtx, گہرائی);
        render(vtx, گہرائی, جھوٹ);
        استعمال Pair = std::pair<std::string, ثابت Dfg_راس*>;
        std::سمتیہ<Pair> vertices;
        برائے (ثابت خودکار& pair : m_multiplicity_And_گہرائی) {
            اگر (pair.دوسرا.پہلا == 1) جاری;
            vertices.emplace_عقب(intern_راس(*pair.پہلا), pair.پہلا);
        }
        std::ترتیب_دو(vertices.begin(), vertices.انتہا(), [](ثابت Pair& a, ثابت Pair& b) {  //
            واپس a.پہلا < b.پہلا;
        });
        برائے (ثابت Pair& pair : vertices) {
            m_os << " | " << pair.پہلا << " is ";
            render(*pair.دوسرا, m_multiplicity_And_گہرائی.at(pair.دوسرا).دوسرا);
        }
    }
};

std::string Dfg_راس::نمونہ_String(uint32_t گہرائی) ثابت {
    std::ostringstream oss;
    Dfg_نمونہ_String{oss, *یہ, گہرائی};
    واپس oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#شامل "V3Dfg__gen_visitor_defns.h"  // From ./astgen
