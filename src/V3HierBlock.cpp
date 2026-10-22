// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Hierarchical Verilation for large designs
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
// Hierarchical Verilation is useful for large designs.
// It reduces
//   - time and memory for Verilation
//   - compilation time especially when a hierarchical block is used many times
//
// Hierarchical Verilation internally uses --lib-create for each
// hierarchical block.  Upper modules read the wrapper from --lib-create
// instead of the Verilog design.
//
// Hierarchical Verilation runs as the following step
// 1) Find modules marked by /*verilator hier_block*/ metacomment
// 2) Generate ${prefix}_hier.mk to create protected-lib for hierarchical blocks and
//    final Verilation to process the top module, that refers wrappers
// 3) Call child Verilator process via ${prefix}_hier.mk
//
// There are 3 kinds of Verilator run.
// a) To create ${prefix}_hier.mk (--hierarchical)
// b) To --lib-create on each hierarchical block (--hierarchical-child)
// c) To load wrappers and Verilate the top module (... what primary flags?)
//
// Then user can build Verilated module as usual.
//
// Here is more detailed internal process.
// 1) Parser adds VPragmaType::HIER_BLOCK of AstPragma to modules
//    that are marked with /*verilator hier_block*/ metacomment in Verilator run a).
// 2) If module type parameters are present, V3Control marks hier param modules
// (marked with hier_params verilator config pragma) as modp->hierParams(true).
// This is done in run b), de-parameterized modules are mapped with their params one-to-one.
// 3) AstModule with HIER_BLOCK pragma is marked modp->hierBlock(true)
//    in V3LinkResolve.cpp during run a).
// 4) In V3LinkCells.cpp, the following things are done during run b) and c).
//    4-1) Delete the upper modules of the hierarchical block because the top module in run b) is
//         hierarchical block, not the top module of run c).
//    4-2) If the top module of the run b) or c) instantiates other hierarchical blocks that is
//         parameterized,
//         module and task names are renamed to the original name to be compatible with the
//         hier module to be called.
//
//         Parameterized modules have unique name by V3Param.cpp. The unique name contains '__' and
//         Verilator encodes '__' when loading such symbols.
// 5) In V3LinkDot.cpp,
//    5-1) Dotted access across hierarchical block boundary is checked. Currently hierarchical
//    block references are not supported.
//    5-2) If present, parameters in hier params module replace parameter values of
//    de-parameterized module in run b).
// 6) In V3Dead.cpp, some parameters of parameterized modules are protected not to be deleted even
//    if the parameter is not referred. This protection is necessary to match step 6) below.
// 7) In V3Param.cpp, use --lib-create wrapper of the parameterized module made in b) and c).
//    If a hierarchical block is a parameterized module and instantiated in multiple locations,
//    all parameters must exactly match.
// 8) In V3HierBlock.cpp, relationships among hierarchical blocks are checked in run a).
//    (which block uses other blocks..)
// 9) In V3EmitMk.cpp, ${prefix}_hier.mk is created in run a).
//
// There are three hidden command options:
//   --hierarchical-child is added to Verilator run b).
//   --hierarchical-block module_name,mangled_name,name0,value0,name1,value1,...
//       module_name  :The original modulename
//       mangled_name :Mangled name of parameterized modules (named in V3Param.cpp).
//                     Same as module_name for non-parameterized hierarchical block.
//       name         :The name of the parameter
//       value        :Overridden value of the parameter
//
//       Used for b) and c).
//       These options are repeated for all instantiated hierarchical blocks.
//   --hierarchical-params-file filename
//      filename    :Name of a hierarchical parameters file
//
//      Added in a), used for b).
//      Each de-parameterized module version has exactly one hier params file specified.

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3HierBlock.h"

#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <cctype>
#అన్తర్భావ <memory>
#అన్తర్భావ <sstream>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

స్థిర string V3_Hier_Command_వాదనలు_Filename(స్థిరరాశి string& ఉపసర్గ, తార్కిక forMkJson) {
    తిరిగి v3Global.opt.makeDir() + "/" + ఉపసర్గ
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

స్థిర string V3HierParametersFileName(స్థిరరాశి string& ఉపసర్గ) {
    తిరిగి v3Global.opt.makeDir() + "/" + ఉపసర్గ + "__hierParameters.v";
}

స్థిర శూన్య V3HierWriteCommonInputs(స్థిరరాశి V3_Hier_ఖండం* hblockp, std::ostream* యొక్క, తార్కిక forMkJson) {
    స్థిరరాశి string శిఖరం_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    ఒకవేళ (!forMkJson) {
        కొరకు (స్థిరరాశి string& filename : V3_Hier_లేఖాచిత్రం::ఆధారం_Files(శిఖరం_Module_File))
            *యొక్క << filename << "\n";
    }
    కొరకు (స్థిరరాశి స్వయంచాలక& i : v3Global.opt.libraryFiles()) {
        ఒకవేళ (V3Os::filename_Real_మార్గం(i.filename()) != శిఖరం_Module_File)
            *యొక్క << "-v " << i.filename() << "\n";
    }
}

//######################################################################

తార్కిక V3_Hier_ఖండం::stringParamPassable(స్థిరరాశి string& విలువ) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    ఒకవేళ (విలువ.కనుగొను_మొదటి_యొక్క("\n\"") != string::npos) తిరిగి అబద్ధం;
    ఒకవేళ (విలువ.కనుగొను("/*") != string::npos) తిరిగి అబద్ధం;
    కొరకు (కొలత_t pos = విలువ.కనుగొను("//"); pos != string::npos; pos = విలువ.కనుగొను("//", pos + 1)) {
        ఒకవేళ (pos > 0 && std::ఖాళీనా(static_cast<గుర్తులేని అక్షరం>(విలువ[pos - 1]))) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}

V3_Hier_ఖండం::StrGParams V3_Hier_ఖండం::stringifyParams(స్థిరరాశి std::సదిశ<AstVar*>& gparams,
                                                     తార్కిక forGOption) {
    StrGParams strParams;
    కొరకు (స్థిరరాశి AstVar* స్థిరరాశి gparam : gparams) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            ఒకవేళ (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::సదిశ<అక్షరం> hexFpStr(32, '\0');
                స్థిరరాశి పూర్ణాంకం len = VL_SNPRINTF(hexFpStr.దత్తాంశం(), hexFpStr.కొలత(), "%a",
                                            constp->అంకె().గమ్యం_Double());
                UASSERT_OBJ(0 < len && static_cast<కొలత_t>(len) < hexFpStr.కొలత(), constp,
                            " is not properly converted to string");
                s = hexFpStr.దత్తాంశం();
            } లేకపోతే ఒకవేళ (constp->isString()) {
                s = constp->అంకె().గమ్యం_String();
                ఒకవేళ (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } లేకపోతే {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_సంఖ్య అంకె{constp->అంకె()};
                అంకె.isSigned(constp->isSigned());
                s = అంకె.ascii(నిజం, నిజం);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_వెనుక(gparam->name(), s);
        }
    }
    తిరిగి strParams;
}

VStringList V3_Hier_ఖండం::command_వాదనలు(తార్కిక forMkJson) స్థిరరాశి {
    VStringList opts;
    స్థిరరాశి string ఉపసర్గ = hier_ఉపసర్గ();
    ఒకవేళ (!forMkJson) {
        opts.నెట్టు_వెనుక(" --prefix " + ఉపసర్గ);
        opts.నెట్టు_వెనుక(" --mod-prefix " + ఉపసర్గ);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.నెట్టు_వెనుక(" --top-module-encoded " + modp()->name());
    }
    opts.నెట్టు_వెనుక(" --lib-create " + modp()->name());  // possibly mangled name
    ఒకవేళ (v3Global.opt.protect_కీలకం_Provided())
        opts.నెట్టు_వెనుక(" --protect-key " + v3Global.opt.protect_కీలకం_Defaulted());
    opts.నెట్టు_వెనుక(" --hierarchical-child " + cvt_గమ్యం_Str(v3Global.opt.దారాలు()));

    స్థిరరాశి StrGParams gparamsStr = stringifyParams(m_params, నిజం);
    కొరకు (స్థిరరాశి StrGParam& param : gparamsStr) {
        opts.నెట్టు_వెనుక("-G" + param.మొదటి + "=" + param.రెండవ + "");
    }
    ఒకవేళ (!m_typeParams.ఖాళీలు()) {
        opts.నెట్టు_వెనుక(" --hierarchical-params-file " + typeParametersFilename());
    }

    స్థిరరాశి పూర్ణాంకం ఖండం_దారాలు = V3Control::getHierWorkers(m_modp->origName());
    ఒకవేళ (ఖండం_దారాలు > 1) {
        ఒకవేళ (!లోపల_ఖాళీలు()) {
            V3Control::get_Hier_Workers_File_పంక్తి(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } లేకపోతే {
            ఒకవేళ (v3Global.opt.దారాలు() < ఖండం_దారాలు) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.దారాలు()
                                << " hierarchical block threads = " << ఖండం_దారాలు);
            }

            opts.నెట్టు_వెనుక(" --threads " + std::గమ్యం_string(ఖండం_దారాలు));
        }
    }

    తిరిగి opts;
}

VStringList V3_Hier_ఖండం::hier_ఖండం_వాదనలు() స్థిరరాశి {
    VStringList opts;
    స్థిరరాశి StrGParams gparamsStr = stringifyParams(m_params, అబద్ధం);
    opts.emplace_వెనుక("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    కొరకు (స్థిరరాశి StrGParam& pair : gparamsStr) {
        s += "," + pair.మొదటి;
        s += "," + pair.రెండవ;
    }
    opts.వెనుక() += s;
    తిరిగి opts;
}

string V3_Hier_ఖండం::hier_ఉపసర్గ() స్థిరరాశి { తిరిగి "V" + modp()->name(); }

string V3_Hier_ఖండం::hierSomeFilename(తార్కిక withDir, స్థిరరాశి అక్షరం* ఉపసర్గ, స్థిరరాశి అక్షరం* suffix) స్థిరరాశి {
    string s;
    ఒకవేళ (withDir) s = hier_ఉపసర్గ() + '/';
    s += ఉపసర్గ + modp()->name() + suffix;
    తిరిగి s;
}

string V3_Hier_ఖండం::hierWrapperFilename(తార్కిక withDir) స్థిరరాశి {
    తిరిగి hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_ఖండం::hierMkFilename(తార్కిక withDir) స్థిరరాశి {
    తిరిగి hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_ఖండం::hierLibFilename(తార్కిక withDir) స్థిరరాశి {
    తిరిగి hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_ఖండం::hierGeneratedFilenames(తార్కిక withDir) స్థిరరాశి {
    తిరిగి hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_ఖండం::vFileIfNecessary() స్థిరరాశి {
    string filename = V3Os::filename_Real_మార్గం(m_modp->fileline()->filename());
    కొరకు (స్థిరరాశి స్వయంచాలక& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        ఒకవేళ (filename == V3Os::filename_Real_మార్గం(v.filename())) తిరిగి "";
    }
    తిరిగి filename;
}

శూన్య V3_Hier_ఖండం::write_Command_వాదనలు_File(తార్కిక forMkJson) స్థిరరాశి {
    స్థిరరాశి std::unique_ptr<std::ofstream> యొక్క{V3File::new_ofstream(command_వాదనలు_Filename(forMkJson))};
    *యొక్క << "--cc\n";

    ఒకవేళ (!forMkJson) {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : బహిర్గతం_అంచులు()) {
            స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి dependencyp = అంచు.శిఖరం()->as<V3_Hier_ఖండం>();
            *యొక్క << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(నిజం) << "\n";
        }
        *యొక్క << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_ఉపసర్గ() << " \n";
    }
    V3HierWriteCommonInputs(ఇది, యొక్క.get(), forMkJson);
    స్థిరరాశి VStringList& commandOpts = command_వాదనలు(అబద్ధం);
    కొరకు (స్థిరరాశి string& opt : commandOpts) *యొక్క << opt << "\n";
    *యొక్క << hier_ఖండం_వాదనలు().front() << "\n";
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : బహిర్గతం_అంచులు()) {
        స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి dependencyp = అంచు.శిఖరం()->as<V3_Hier_ఖండం>();
        *యొక్క << dependencyp->hier_ఖండం_వాదనలు().front() << "\n";
    }
    *యొక్క << v3Global.opt.all_వాదనలు_String_For_Hier_ఖండం(అబద్ధం) << "\n";
}

string V3_Hier_ఖండం::command_వాదనలు_Filename(తార్కిక forMkJson) స్థిరరాశి {
    తిరిగి V3_Hier_Command_వాదనలు_Filename(hier_ఉపసర్గ(), forMkJson);
}

string V3_Hier_ఖండం::typeParametersFilename() స్థిరరాశి {
    తిరిగి V3HierParametersFileName(hier_ఉపసర్గ());
}

శూన్య V3_Hier_ఖండం::writeParametersFile() స్థిరరాశి {
    ఒకవేళ (m_typeParams.ఖాళీలు()) తిరిగి;

    VHashSha512 hash{"type params"};
    స్థిరరాశి string moduleName = "Vhsh" + hash.digestSymbol24();
    స్థిరరాశి std::unique_ptr<std::ofstream> యొక్క{V3File::new_ofstream(typeParametersFilename())};
    *యొక్క << "module " << moduleName << ";\n";
    కొరకు (AstParamTypeDType* స్థిరరాశి gparam : m_typeParams) {
        AstTypedef* tdefp
            = కొత్త AstTypedef{కొత్త File_పంక్తి{File_పంక్తి::built_లోపల_Filename()}, gparam->name(), శూన్యసూచిక,
                             VFlag_బిడ్డ_DType{}, gparam->skipRefp()->clone_వృక్షం_శుద్ధ(నిజం)};
        V3EmitV::verilog_For_వృక్షం(tdefp, *యొక్క);
        VL_DO_DANGLING(tdefp->delete_వృక్షం(), tdefp);
    }
    *యొక్క << "endmodule\n\n";
    *యొక్క << "`verilator_config\n";
    *యొక్క << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
తరగతి Hier_ఖండం_Usage_Collect_Visitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    V3_Hier_లేఖాచిత్రం* స్థిరరాశి m_graphp = కొత్త V3_Hier_లేఖాచిత్రం{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<స్థిరరాశి AstModule*, V3_Hier_ఖండం*> m_mod2vtx;
    AstModule* m_modp = శూన్యసూచిక;  // The current module
    std::సదిశ<AstVar*> m_params;  // Overridden value parameters of current module
    std::సదిశ<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::సదిశ<V3_Hier_ఖండం*> m_childrenp;

    // VISITORSs
    శూన్య సందర్శించు(Ast_కణుపు_Module*) override {}  // Ignore all non-AstModule
    శూన్య సందర్శించు(AstModule* nodep) override {
        // Visit each module once
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        ఒకవేళ (!nodep->hier_ఖండం()) {
            iterateChildrenConst(nodep);
            తిరిగి;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_ఖండం* స్థిరరాశి blockp = కొత్త V3_Hier_ఖండం{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        కొరకు (V3_Hier_ఖండం* స్థిరరాశి childp : m_childrenp) కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, blockp, childp, 1};
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* స్థిరరాశి modp = VN_CAST(nodep->modp(), Module);
        ఒకవేళ (!modp) తిరిగి;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        ఒకవేళ (modp->hier_ఖండం()) m_childrenp.emplace_వెనుక(m_mod2vtx.at(modp));
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (!m_modp) తిరిగి;
        ఒకవేళ (!m_modp->hier_ఖండం()) తిరిగి;
        // Can't handle interface port on hier block
        ఒకవేళ (nodep->isIfaceRef() && !nodep->is_Iface_మాతృ()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        ఒకవేళ (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.నెట్టు_వెనుక(nodep);
        }
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        ఒకవేళ (!m_modp->hier_ఖండం()) తిరిగి;
        // Record type parameter of this hier block
        m_typeParams.నెట్టు_వెనుక(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Stmt*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_Type_పట్టిక*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    స్పష్టమైన Hier_ఖండం_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_అంచులు_కూడిక(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->క్రమం();  // This is a bit heavy weight, but does produce a topological ordering
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

బహిరంగ:
    స్థిర V3_Hier_లేఖాచిత్రం* apply(AstNetlist* netlistp) {
        తిరిగి Hier_ఖండం_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_లేఖాచిత్రం::ఆధారం_Files(స్థిరరాశి string& శిఖరం_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().కొలత() + 1);
    కొరకు (స్థిరరాశి VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_వెనుక(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    ఒకవేళ (!శిఖరం_Module_File.ఖాళీలు()) sources.emplace_వెనుక(శిఖరం_Module_File);
    తిరిగి sources;
}

శూన్య V3_Hier_లేఖాచిత్రం::write_Command_వాదనలు_Files(తార్కిక forMkJson) స్థిరరాశి {

    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        vtx.as<V3_Hier_ఖండం>()->write_Command_వాదనలు_File(forMkJson);
    }
    // For the top module
    స్థిరరాశి std::unique_ptr<std::ofstream> యొక్క{
        V3File::new_ofstream(శిఖరం_Command_వాదనలు_Filename(forMkJson))};
    ఒకవేళ (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
            *యొక్క << vtx.as<V3_Hier_ఖండం>()->hierWrapperFilename(నిజం) << "\n";
        }
    }
    V3HierWriteCommonInputs(శూన్యసూచిక, యొక్క.get(), forMkJson);
    ఒకవేళ (!forMkJson) {
        స్థిరరాశి VString_సమితి& cppFiles = v3Global.opt.cppFiles();
        కొరకు (స్థిరరాశి string& i : cppFiles) *యొక్క << i << "\n";
        *యొక్క << "--top-module " << v3Global.rootp()->శిఖరం_Modulep()->name() << "\n";
        *యొక్క << "--prefix " << v3Global.opt.ఉపసర్గ() << "\n";
        *యొక్క << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *యొక్క << "--mod-prefix " << v3Global.opt.mod_ఉపసర్గ() << "\n";
    }
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        *యొక్క << vtx.as<V3_Hier_ఖండం>()->hier_ఖండం_వాదనలు().front() << "\n";
    }

    ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
        *యొక్క << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    ఒకవేళ (v3Global.opt.protect_కీలకం_Provided()) {
        *యొక్క << "--protect-key " << v3Global.opt.protect_కీలకం_Defaulted() << "\n";
    }
    *యొక్క << "--threads " << cvt_గమ్యం_Str(v3Global.opt.దారాలు()) << "\n";
    *యొక్క << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *యొక్క << v3Global.opt.all_వాదనలు_String_For_Hier_ఖండం(నిజం) << "\n";
}

string V3_Hier_లేఖాచిత్రం::శిఖరం_Command_వాదనలు_Filename(తార్కిక forMkJson) {
    తిరిగి V3_Hier_Command_వాదనలు_Filename(v3Global.opt.ఉపసర్గ(), forMkJson);
}

శూన్య V3_Hier_లేఖాచిత్రం::writeParametersFiles() స్థిరరాశి {
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) { vtx.as<V3_Hier_ఖండం>()->writeParametersFile(); }
}

//######################################################################

శూన్య V3_క్రమానుగత::create_లేఖాచిత్రం(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_కణుపు_Module* స్థిరరాశి modp = netlistp->శిఖరం_Modulep();
    ఒకవేళ (modp->hier_ఖండం()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_మరిన్ని()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_ఖండం(అబద్ధం);
    }

    V3_Hier_లేఖాచిత్రం* స్థిరరాశి graphp = Hier_ఖండం_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::కూడు_Stat("HierBlock, Hierarchical blocks", graphp->vertices().కొలత());
    // No hierarchical block is found, nothing to do.
    ఒకవేళ (graphp->ఖాళీలు()) {
        VL_DO_DANGLING(తొలగించు graphp, graphp);
        తిరిగి;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
