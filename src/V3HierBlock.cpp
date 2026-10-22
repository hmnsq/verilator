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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3HierBlock.h"

#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <cctype>
#अन्तर्भाव <memory>
#अन्तर्भाव <sstream>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

स्थिर string V3_Hier_Command_तर्काः_Filename(ध्रुव string& उपसर्गः, तार्किक forMkJson) {
    प्रत्यागम v3Global.opt.makeDir() + "/" + उपसर्गः
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

स्थिर string V3HierParametersFileName(ध्रुव string& उपसर्गः) {
    प्रत्यागम v3Global.opt.makeDir() + "/" + उपसर्गः + "__hierParameters.v";
}

स्थिर शून्य V3HierWriteCommonInputs(ध्रुव V3_Hier_खण्डम्* hblockp, std::ostream* सम्बन्धि, तार्किक forMkJson) {
    ध्रुव string शिखरम्_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    यदि (!forMkJson) {
        प्रति (ध्रुव string& filename : V3HierGraph::sourceFiles(शिखरम्_Module_File))
            *सम्बन्धि << filename << "\n";
    }
    प्रति (ध्रुव स्वचालित& i : v3Global.opt.libraryFiles()) {
        यदि (V3Os::filename_Real_मार्गः(i.filename()) != शिखरम्_Module_File)
            *सम्बन्धि << "-v " << i.filename() << "\n";
    }
}

//######################################################################

तार्किक V3_Hier_खण्डम्::stringParamPassable(ध्रुव string& मानम्) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    यदि (मानम्.find_first_सम्बन्धि("\n\"") != string::npos) प्रत्यागम असत्य;
    यदि (मानम्.find("/*") != string::npos) प्रत्यागम असत्य;
    प्रति (आकार_t pos = मानम्.find("//"); pos != string::npos; pos = मानम्.find("//", pos + 1)) {
        यदि (pos > 0 && std::रिक्तम्_वा(static_cast<अचिह्नित वर्ण>(मानम्[pos - 1]))) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

V3_Hier_खण्डम्::StrGParams V3_Hier_खण्डम्::stringifyParams(ध्रुव std::सदिशम्<AstVar*>& gparams,
                                                     तार्किक forGOption) {
    StrGParams strParams;
    प्रति (ध्रुव AstVar* ध्रुव gparam : gparams) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            यदि (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::सदिशम्<वर्ण> hexFpStr(32, '\0');
                ध्रुव पूर्णाङ्क len = VL_SNPRINTF(hexFpStr.दत्त(), hexFpStr.आकार(), "%a",
                                            constp->अङ्कः().गन्तव्यम्_Double());
                UASSERT_OBJ(0 < len && static_cast<आकार_t>(len) < hexFpStr.आकार(), constp,
                            " is not properly converted to string");
                s = hexFpStr.दत्त();
            } अन्यथा यदि (constp->isString()) {
                s = constp->अङ्कः().गन्तव्यम्_String();
                यदि (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } अन्यथा {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_सङ्ख्या अङ्कः{constp->अङ्कः()};
                अङ्कः.isSigned(constp->isSigned());
                s = अङ्कः.ascii(सत्य, सत्य);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_पृष्ठभागः(gparam->name(), s);
        }
    }
    प्रत्यागम strParams;
}

VStringList V3_Hier_खण्डम्::command_तर्काः(तार्किक forMkJson) ध्रुव {
    VStringList opts;
    ध्रुव string उपसर्गः = hier_उपसर्गः();
    यदि (!forMkJson) {
        opts.प्रक्षिप_पृष्ठभागः(" --prefix " + उपसर्गः);
        opts.प्रक्षिप_पृष्ठभागः(" --mod-prefix " + उपसर्गः);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.प्रक्षिप_पृष्ठभागः(" --top-module-encoded " + modp()->name());
    }
    opts.प्रक्षिप_पृष्ठभागः(" --lib-create " + modp()->name());  // possibly mangled name
    यदि (v3Global.opt.protect_कुञ्चिका_Provided())
        opts.प्रक्षिप_पृष्ठभागः(" --protect-key " + v3Global.opt.protect_कुञ्चिका_Defaulted());
    opts.प्रक्षिप_पृष्ठभागः(" --hierarchical-child " + cvt_गन्तव्यम्_Str(v3Global.opt.तन्तवः()));

    ध्रुव StrGParams gparamsStr = stringifyParams(m_params, सत्य);
    प्रति (ध्रुव StrGParam& param : gparamsStr) {
        opts.प्रक्षिप_पृष्ठभागः("-G" + param.first + "=" + param.second + "");
    }
    यदि (!m_typeParams.रिक्तानि()) {
        opts.प्रक्षिप_पृष्ठभागः(" --hierarchical-params-file " + typeParametersFilename());
    }

    ध्रुव पूर्णाङ्क खण्डम्_तन्तवः = V3Control::getHierWorkers(m_modp->origName());
    यदि (खण्डम्_तन्तवः > 1) {
        यदि (!अन्तर्_रिक्तानि()) {
            V3Control::get_Hier_Workers_File_पङ्क्तिः(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } अन्यथा {
            यदि (v3Global.opt.तन्तवः() < खण्डम्_तन्तवः) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.तन्तवः()
                                << " hierarchical block threads = " << खण्डम्_तन्तवः);
            }

            opts.प्रक्षिप_पृष्ठभागः(" --threads " + std::गन्तव्यम्_string(खण्डम्_तन्तवः));
        }
    }

    प्रत्यागम opts;
}

VStringList V3_Hier_खण्डम्::hier_खण्डम्_तर्काः() ध्रुव {
    VStringList opts;
    ध्रुव StrGParams gparamsStr = stringifyParams(m_params, असत्य);
    opts.emplace_पृष्ठभागः("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    प्रति (ध्रुव StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.पृष्ठभागः() += s;
    प्रत्यागम opts;
}

string V3_Hier_खण्डम्::hier_उपसर्गः() ध्रुव { प्रत्यागम "V" + modp()->name(); }

string V3_Hier_खण्डम्::hierSomeFilename(तार्किक withDir, ध्रुव वर्ण* उपसर्गः, ध्रुव वर्ण* suffix) ध्रुव {
    string s;
    यदि (withDir) s = hier_उपसर्गः() + '/';
    s += उपसर्गः + modp()->name() + suffix;
    प्रत्यागम s;
}

string V3_Hier_खण्डम्::hierWrapperFilename(तार्किक withDir) ध्रुव {
    प्रत्यागम hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_खण्डम्::hierMkFilename(तार्किक withDir) ध्रुव {
    प्रत्यागम hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_खण्डम्::hierLibFilename(तार्किक withDir) ध्रुव {
    प्रत्यागम hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_खण्डम्::hierGeneratedFilenames(तार्किक withDir) ध्रुव {
    प्रत्यागम hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_खण्डम्::vFileIfNecessary() ध्रुव {
    string filename = V3Os::filename_Real_मार्गः(m_modp->fileline()->filename());
    प्रति (ध्रुव स्वचालित& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        यदि (filename == V3Os::filename_Real_मार्गः(v.filename())) प्रत्यागम "";
    }
    प्रत्यागम filename;
}

शून्य V3_Hier_खण्डम्::write_Command_तर्काः_File(तार्किक forMkJson) ध्रुव {
    ध्रुव std::unique_ptr<std::ofstream> सम्बन्धि{V3File::new_ofstream(command_तर्काः_Filename(forMkJson))};
    *सम्बन्धि << "--cc\n";

    यदि (!forMkJson) {
        प्रति (ध्रुव V3GraphEdge& edge : बहिर्गमनम्_धाराः()) {
            ध्रुव V3_Hier_खण्डम्* ध्रुव dependencyp = edge.शिखरम्()->as<V3_Hier_खण्डम्>();
            *सम्बन्धि << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(सत्य) << "\n";
        }
        *सम्बन्धि << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_उपसर्गः() << " \n";
    }
    V3HierWriteCommonInputs(एतत्, सम्बन्धि.get(), forMkJson);
    ध्रुव VStringList& commandOpts = command_तर्काः(असत्य);
    प्रति (ध्रुव string& opt : commandOpts) *सम्बन्धि << opt << "\n";
    *सम्बन्धि << hier_खण्डम्_तर्काः().front() << "\n";
    प्रति (ध्रुव V3GraphEdge& edge : बहिर्गमनम्_धाराः()) {
        ध्रुव V3_Hier_खण्डम्* ध्रुव dependencyp = edge.शिखरम्()->as<V3_Hier_खण्डम्>();
        *सम्बन्धि << dependencyp->hier_खण्डम्_तर्काः().front() << "\n";
    }
    *सम्बन्धि << v3Global.opt.all_तर्काः_String_For_Hier_खण्डम्(असत्य) << "\n";
}

string V3_Hier_खण्डम्::command_तर्काः_Filename(तार्किक forMkJson) ध्रुव {
    प्रत्यागम V3_Hier_Command_तर्काः_Filename(hier_उपसर्गः(), forMkJson);
}

string V3_Hier_खण्डम्::typeParametersFilename() ध्रुव {
    प्रत्यागम V3HierParametersFileName(hier_उपसर्गः());
}

शून्य V3_Hier_खण्डम्::writeParametersFile() ध्रुव {
    यदि (m_typeParams.रिक्तानि()) प्रत्यागम;

    VHashSha512 hash{"type params"};
    ध्रुव string moduleName = "Vhsh" + hash.digestSymbol24();
    ध्रुव std::unique_ptr<std::ofstream> सम्बन्धि{V3File::new_ofstream(typeParametersFilename())};
    *सम्बन्धि << "module " << moduleName << ";\n";
    प्रति (AstParamTypeDType* ध्रुव gparam : m_typeParams) {
        AstTypedef* tdefp
            = नूतन AstTypedef{नूतन File_पङ्क्तिः{File_पङ्क्तिः::built_अन्तर्_Filename()}, gparam->name(), शून्यसूचकः,
                             VFlagChildDType{}, gparam->skipRefp()->clone_वृक्षः_विशुद्धम्(सत्य)};
        V3EmitV::verilog_For_वृक्षः(tdefp, *सम्बन्धि);
        VL_DO_DANGLING(tdefp->delete_वृक्षः(), tdefp);
    }
    *सम्बन्धि << "endmodule\n\n";
    *सम्बन्धि << "`verilator_config\n";
    *सम्बन्धि << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
वर्ग Hier_खण्डम्_Usage_Collect_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
    V3HierGraph* ध्रुव m_graphp = नूतन V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ध्रुव AstModule*, V3_Hier_खण्डम्*> m_mod2vtx;
    AstModule* m_modp = शून्यसूचकः;  // The current module
    std::सदिशम्<AstVar*> m_params;  // Overridden value parameters of current module
    std::सदिशम्<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::सदिशम्<V3_Hier_खण्डम्*> m_childrenp;

    // VISITORSs
    शून्य दर्शय(Ast_ग्रन्थिः_Module*) override {}  // Ignore all non-AstModule
    शून्य दर्शय(AstModule* nodep) override {
        // Visit each module once
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        यदि (!nodep->hier_खण्डम्()) {
            iterateChildrenConst(nodep);
            प्रत्यागम;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_खण्डम्* ध्रुव blockp = नूतन V3_Hier_खण्डम्{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        प्रति (V3_Hier_खण्डम्* ध्रुव childp : m_childrenp) नूतन V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    शून्य दर्शय(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ध्रुव modp = VN_CAST(nodep->modp(), Module);
        यदि (!modp) प्रत्यागम;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        यदि (modp->hier_खण्डम्()) m_childrenp.emplace_पृष्ठभागः(m_mod2vtx.at(modp));
    }
    शून्य दर्शय(AstVar* nodep) override {
        यदि (!m_modp) प्रत्यागम;
        यदि (!m_modp->hier_खण्डम्()) प्रत्यागम;
        // Can't handle interface port on hier block
        यदि (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        यदि (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.प्रक्षिप_पृष्ठभागः(nodep);
        }
    }
    शून्य दर्शय(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        यदि (!m_modp->hier_खण्डम्()) प्रत्यागम;
        // Record type parameter of this hier block
        m_typeParams.प्रक्षिप_पृष्ठभागः(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Stmt*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य दर्शय(Ast_Type_सारणी*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    स्पष्ट Hier_खण्डम्_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        यदि (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_धाराः_योग(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->क्रमः();  // This is a bit heavy weight, but does produce a topological ordering
        यदि (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

सार्वजनिक:
    स्थिर V3HierGraph* apply(AstNetlist* netlistp) {
        प्रत्यागम Hier_खण्डम्_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(ध्रुव string& शिखरम्_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().आकार() + 1);
    प्रति (ध्रुव VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_पृष्ठभागः(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    यदि (!शिखरम्_Module_File.रिक्तानि()) sources.emplace_पृष्ठभागः(शिखरम्_Module_File);
    प्रत्यागम sources;
}

शून्य V3HierGraph::write_Command_तर्काः_Files(तार्किक forMkJson) ध्रुव {

    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        vtx.as<V3_Hier_खण्डम्>()->write_Command_तर्काः_File(forMkJson);
    }
    // For the top module
    ध्रुव std::unique_ptr<std::ofstream> सम्बन्धि{
        V3File::new_ofstream(शिखरम्_Command_तर्काः_Filename(forMkJson))};
    यदि (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
            *सम्बन्धि << vtx.as<V3_Hier_खण्डम्>()->hierWrapperFilename(सत्य) << "\n";
        }
    }
    V3HierWriteCommonInputs(शून्यसूचकः, सम्बन्धि.get(), forMkJson);
    यदि (!forMkJson) {
        ध्रुव VString_समुच्चयः& cppFiles = v3Global.opt.cppFiles();
        प्रति (ध्रुव string& i : cppFiles) *सम्बन्धि << i << "\n";
        *सम्बन्धि << "--top-module " << v3Global.rootp()->शिखरम्_Modulep()->name() << "\n";
        *सम्बन्धि << "--prefix " << v3Global.opt.उपसर्गः() << "\n";
        *सम्बन्धि << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *सम्बन्धि << "--mod-prefix " << v3Global.opt.mod_उपसर्गः() << "\n";
    }
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        *सम्बन्धि << vtx.as<V3_Hier_खण्डम्>()->hier_खण्डम्_तर्काः().front() << "\n";
    }

    यदि (!v3Global.opt.libCreate().रिक्तानि()) {
        *सम्बन्धि << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    यदि (v3Global.opt.protect_कुञ्चिका_Provided()) {
        *सम्बन्धि << "--protect-key " << v3Global.opt.protect_कुञ्चिका_Defaulted() << "\n";
    }
    *सम्बन्धि << "--threads " << cvt_गन्तव्यम्_Str(v3Global.opt.तन्तवः()) << "\n";
    *सम्बन्धि << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *सम्बन्धि << v3Global.opt.all_तर्काः_String_For_Hier_खण्डम्(सत्य) << "\n";
}

string V3HierGraph::शिखरम्_Command_तर्काः_Filename(तार्किक forMkJson) {
    प्रत्यागम V3_Hier_Command_तर्काः_Filename(v3Global.opt.उपसर्गः(), forMkJson);
}

शून्य V3HierGraph::writeParametersFiles() ध्रुव {
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) { vtx.as<V3_Hier_खण्डम्>()->writeParametersFile(); }
}

//######################################################################

शून्य V3_श्रेणीबद्धम्::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ग्रन्थिः_Module* ध्रुव modp = netlistp->शिखरम्_Modulep();
    यदि (modp->hier_खण्डम्()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_अधिकम्()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_खण्डम्(असत्य);
    }

    V3HierGraph* ध्रुव graphp = Hier_खण्डम्_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::सङ्कलय_Stat("HierBlock, Hierarchical blocks", graphp->vertices().आकार());
    // No hierarchical block is found, nothing to do.
    यदि (graphp->रिक्तानि()) {
        VL_DO_DANGLING(विलोपय graphp, graphp);
        प्रत्यागम;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
