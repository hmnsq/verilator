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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3HierBlock.h"

#ضمن "V3Control.h"
#ضمن "V3EmitV.h"
#ضمن "V3File.h"
#ضمن "V3Os.h"
#ضمن "V3Stats.h"
#ضمن "V3String.h"

#ضمن <cctype>
#ضمن <memory>
#ضمن <sstream>
#ضمن <utility>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ساكن string V3_Hier_Command_دلیلونه_Filename(ثابت string& مختاړی, منطقي forMkJson) {
    أرجع v3Global.opt.makeDir() + "/" + مختاړی
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ساكن string V3HierParametersFileName(ثابت string& مختاړی) {
    أرجع v3Global.opt.makeDir() + "/" + مختاړی + "__hierParameters.v";
}

ساكن فراغ V3HierWriteCommonInputs(ثابت V3_Hier_بلاک* hblockp, std::ostream* د, منطقي forMkJson) {
    ثابت string سر_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    إذا (!forMkJson) {
        لكل (ثابت string& filename : V3_Hier_ګراف::سرچینه_Files(سر_Module_File))
            *د << filename << "\n";
    }
    لكل (ثابت خپلکاری& i : v3Global.opt.libraryFiles()) {
        إذا (V3Os::filename_Real_لار(i.filename()) != سر_Module_File)
            *د << "-v " << i.filename() << "\n";
    }
}

//######################################################################

منطقي V3_Hier_بلاک::stringParamPassable(ثابت string& ارزښت) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    إذا (ارزښت.پیدا_کړه_لومړی_د("\n\"") != string::npos) أرجع خطأ;
    إذا (ارزښت.پیدا_کړه("/*") != string::npos) أرجع خطأ;
    لكل (اندازه_t pos = ارزښت.پیدا_کړه("//"); pos != string::npos; pos = ارزښت.پیدا_کړه("//", pos + 1)) {
        إذا (pos > 0 && std::که_تشه(static_cast<بینښه حرف>(ارزښت[pos - 1]))) أرجع خطأ;
    }
    أرجع ریښتیا;
}

V3_Hier_بلاک::StrGParams V3_Hier_بلاک::stringifyParams(ثابت std::ویکټور<AstVar*>& gparams,
                                                     منطقي forGOption) {
    StrGParams strParams;
    لكل (ثابت AstVar* ثابت gparam : gparams) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            إذا (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::ویکټور<حرف> hexFpStr(32, '\0');
                ثابت صحيح len = VL_SNPRINTF(hexFpStr.معلومات(), hexFpStr.اندازه(), "%a",
                                            constp->عدد().منزل_Double());
                UASSERT_OBJ(0 < len && static_cast<اندازه_t>(len) < hexFpStr.اندازه(), constp,
                            " is not properly converted to string");
                s = hexFpStr.معلومات();
            } وإلا إذا (constp->isString()) {
                s = constp->عدد().منزل_String();
                إذا (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } وإلا {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_شمېره عدد{constp->عدد()};
                عدد.isSigned(constp->isSigned());
                s = عدد.ascii(ریښتیا, ریښتیا);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_شا(gparam->name(), s);
        }
    }
    أرجع strParams;
}

VStringList V3_Hier_بلاک::command_دلیلونه(منطقي forMkJson) ثابت {
    VStringList opts;
    ثابت string مختاړی = hier_مختاړی();
    إذا (!forMkJson) {
        opts.ټېل_شا(" --prefix " + مختاړی);
        opts.ټېل_شا(" --mod-prefix " + مختاړی);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ټېل_شا(" --top-module-encoded " + modp()->name());
    }
    opts.ټېل_شا(" --lib-create " + modp()->name());  // possibly mangled name
    إذا (v3Global.opt.protect_کلی_Provided())
        opts.ټېل_شا(" --protect-key " + v3Global.opt.protect_کلی_Defaulted());
    opts.ټېل_شا(" --hierarchical-child " + cvt_منزل_Str(v3Global.opt.تارونه()));

    ثابت StrGParams gparamsStr = stringifyParams(m_params, ریښتیا);
    لكل (ثابت StrGParam& param : gparamsStr) {
        opts.ټېل_شا("-G" + param.لومړی + "=" + param.دوهم + "");
    }
    إذا (!m_typeParams.تشه()) {
        opts.ټېل_شا(" --hierarchical-params-file " + typeParametersFilename());
    }

    ثابت صحيح بلاک_تارونه = V3Control::getHierWorkers(m_modp->origName());
    إذا (بلاک_تارونه > 1) {
        إذا (!دننه_تشه()) {
            V3Control::get_Hier_Workers_File_کرښه(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } وإلا {
            إذا (v3Global.opt.تارونه() < بلاک_تارونه) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.تارونه()
                                << " hierarchical block threads = " << بلاک_تارونه);
            }

            opts.ټېل_شا(" --threads " + std::منزل_string(بلاک_تارونه));
        }
    }

    أرجع opts;
}

VStringList V3_Hier_بلاک::hier_بلاک_دلیلونه() ثابت {
    VStringList opts;
    ثابت StrGParams gparamsStr = stringifyParams(m_params, خطأ);
    opts.emplace_شا("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    لكل (ثابت StrGParam& pair : gparamsStr) {
        s += "," + pair.لومړی;
        s += "," + pair.دوهم;
    }
    opts.شا() += s;
    أرجع opts;
}

string V3_Hier_بلاک::hier_مختاړی() ثابت { أرجع "V" + modp()->name(); }

string V3_Hier_بلاک::hierSomeFilename(منطقي withDir, ثابت حرف* مختاړی, ثابت حرف* suffix) ثابت {
    string s;
    إذا (withDir) s = hier_مختاړی() + '/';
    s += مختاړی + modp()->name() + suffix;
    أرجع s;
}

string V3_Hier_بلاک::hierWrapperFilename(منطقي withDir) ثابت {
    أرجع hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_بلاک::hierMkFilename(منطقي withDir) ثابت {
    أرجع hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_بلاک::hierLibFilename(منطقي withDir) ثابت {
    أرجع hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_بلاک::hierGeneratedFilenames(منطقي withDir) ثابت {
    أرجع hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_بلاک::vFileIfNecessary() ثابت {
    string filename = V3Os::filename_Real_لار(m_modp->fileline()->filename());
    لكل (ثابت خپلکاری& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        إذا (filename == V3Os::filename_Real_لار(v.filename())) أرجع "";
    }
    أرجع filename;
}

فراغ V3_Hier_بلاک::write_Command_دلیلونه_File(منطقي forMkJson) ثابت {
    ثابت std::unique_ptr<std::ofstream> د{V3File::new_ofstream(command_دلیلونه_Filename(forMkJson))};
    *د << "--cc\n";

    إذا (!forMkJson) {
        لكل (ثابت V3_ګراف_څنډه& څنډه : بهر_څنډې()) {
            ثابت V3_Hier_بلاک* ثابت dependencyp = څنډه.سر()->as<V3_Hier_بلاک>();
            *د << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(ریښتیا) << "\n";
        }
        *د << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_مختاړی() << " \n";
    }
    V3HierWriteCommonInputs(هذا, د.get(), forMkJson);
    ثابت VStringList& commandOpts = command_دلیلونه(خطأ);
    لكل (ثابت string& opt : commandOpts) *د << opt << "\n";
    *د << hier_بلاک_دلیلونه().front() << "\n";
    لكل (ثابت V3_ګراف_څنډه& څنډه : بهر_څنډې()) {
        ثابت V3_Hier_بلاک* ثابت dependencyp = څنډه.سر()->as<V3_Hier_بلاک>();
        *د << dependencyp->hier_بلاک_دلیلونه().front() << "\n";
    }
    *د << v3Global.opt.all_دلیلونه_String_For_Hier_بلاک(خطأ) << "\n";
}

string V3_Hier_بلاک::command_دلیلونه_Filename(منطقي forMkJson) ثابت {
    أرجع V3_Hier_Command_دلیلونه_Filename(hier_مختاړی(), forMkJson);
}

string V3_Hier_بلاک::typeParametersFilename() ثابت {
    أرجع V3HierParametersFileName(hier_مختاړی());
}

فراغ V3_Hier_بلاک::writeParametersFile() ثابت {
    إذا (m_typeParams.تشه()) أرجع;

    VHashSha512 hash{"type params"};
    ثابت string moduleName = "Vhsh" + hash.digestSymbol24();
    ثابت std::unique_ptr<std::ofstream> د{V3File::new_ofstream(typeParametersFilename())};
    *د << "module " << moduleName << ";\n";
    لكل (AstParamTypeDType* ثابت gparam : m_typeParams) {
        AstTypedef* tdefp
            = جديد AstTypedef{جديد File_کرښه{File_کرښه::built_دننه_Filename()}, gparam->name(), تش_اشاره,
                             VFlag_ماشوم_DType{}, gparam->skipRefp()->clone_ونه_خالص(ریښتیا)};
        V3EmitV::verilog_For_ونه(tdefp, *د);
        VL_DO_DANGLING(tdefp->delete_ونه(), tdefp);
    }
    *د << "endmodule\n\n";
    *د << "`verilator_config\n";
    *د << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
صنف Hier_بلاک_Usage_Collect_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE
    V3_Hier_ګراف* ثابت m_graphp = جديد V3_Hier_ګراف{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ثابت AstModule*, V3_Hier_بلاک*> m_mod2vtx;
    AstModule* m_modp = تش_اشاره;  // The current module
    std::ویکټور<AstVar*> m_params;  // Overridden value parameters of current module
    std::ویکټور<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::ویکټور<V3_Hier_بلاک*> m_childrenp;

    // VISITORSs
    فراغ لیدنه(Ast_غوټه_Module*) override {}  // Ignore all non-AstModule
    فراغ لیدنه(AstModule* nodep) override {
        // Visit each module once
        إذا (nodep->user1_سېټ_Once()) أرجع;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        إذا (!nodep->hier_بلاک()) {
            iterateChildrenConst(nodep);
            أرجع;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_بلاک* ثابت blockp = جديد V3_Hier_بلاک{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        لكل (V3_Hier_بلاک* ثابت childp : m_childrenp) جديد V3_ګراف_څنډه{m_graphp, blockp, childp, 1};
    }
    فراغ لیدنه(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ثابت modp = VN_CAST(nodep->modp(), Module);
        إذا (!modp) أرجع;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        إذا (modp->hier_بلاک()) m_childrenp.emplace_شا(m_mod2vtx.at(modp));
    }
    فراغ لیدنه(AstVar* nodep) override {
        إذا (!m_modp) أرجع;
        إذا (!m_modp->hier_بلاک()) أرجع;
        // Can't handle interface port on hier block
        إذا (nodep->isIfaceRef() && !nodep->is_Iface_پلار()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        إذا (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ټېل_شا(nodep);
        }
    }
    فراغ لیدنه(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        إذا (!m_modp->hier_بلاک()) أرجع;
        // Record type parameter of this hier block
        m_typeParams.ټېل_شا(nodep);
    }

    فراغ لیدنه(Ast_غوټه_Stmt*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه_بیان*) override {}  // Accelerate
    فراغ لیدنه(Ast_Type_جدول*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    څرګند Hier_بلاک_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        إذا (dump_ګراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_څنډې_مجموعه(&V3_ګراف_څنډه::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->ترتیب();  // This is a bit heavy weight, but does produce a topological ordering
        إذا (dump_ګراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

عام:
    ساكن V3_Hier_ګراف* apply(AstNetlist* netlistp) {
        أرجع Hier_بلاک_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_ګراف::سرچینه_Files(ثابت string& سر_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().اندازه() + 1);
    لكل (ثابت VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_شا(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    إذا (!سر_Module_File.تشه()) sources.emplace_شا(سر_Module_File);
    أرجع sources;
}

فراغ V3_Hier_ګراف::write_Command_دلیلونه_Files(منطقي forMkJson) ثابت {

    لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
        vtx.as<V3_Hier_بلاک>()->write_Command_دلیلونه_File(forMkJson);
    }
    // For the top module
    ثابت std::unique_ptr<std::ofstream> د{
        V3File::new_ofstream(سر_Command_دلیلونه_Filename(forMkJson))};
    إذا (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
            *د << vtx.as<V3_Hier_بلاک>()->hierWrapperFilename(ریښتیا) << "\n";
        }
    }
    V3HierWriteCommonInputs(تش_اشاره, د.get(), forMkJson);
    إذا (!forMkJson) {
        ثابت VString_سېټ& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت string& i : cppFiles) *د << i << "\n";
        *د << "--top-module " << v3Global.rootp()->سر_Modulep()->name() << "\n";
        *د << "--prefix " << v3Global.opt.مختاړی() << "\n";
        *د << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *د << "--mod-prefix " << v3Global.opt.mod_مختاړی() << "\n";
    }
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
        *د << vtx.as<V3_Hier_بلاک>()->hier_بلاک_دلیلونه().front() << "\n";
    }

    إذا (!v3Global.opt.libCreate().تشه()) {
        *د << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    إذا (v3Global.opt.protect_کلی_Provided()) {
        *د << "--protect-key " << v3Global.opt.protect_کلی_Defaulted() << "\n";
    }
    *د << "--threads " << cvt_منزل_Str(v3Global.opt.تارونه()) << "\n";
    *د << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *د << v3Global.opt.all_دلیلونه_String_For_Hier_بلاک(ریښتیا) << "\n";
}

string V3_Hier_ګراف::سر_Command_دلیلونه_Filename(منطقي forMkJson) {
    أرجع V3_Hier_Command_دلیلونه_Filename(v3Global.opt.مختاړی(), forMkJson);
}

فراغ V3_Hier_ګراف::writeParametersFiles() ثابت {
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) { vtx.as<V3_Hier_بلاک>()->writeParametersFile(); }
}

//######################################################################

فراغ V3_پوړیز::create_ګراف(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_غوټه_Module* ثابت modp = netlistp->سر_Modulep();
    إذا (modp->hier_بلاک()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_نور()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_بلاک(خطأ);
    }

    V3_Hier_ګراف* ثابت graphp = Hier_بلاک_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::جمع_کړه_Stat("HierBlock, Hierarchical blocks", graphp->vertices().اندازه());
    // No hierarchical block is found, nothing to do.
    إذا (graphp->تشه()) {
        VL_DO_DANGLING(ړنګول graphp, graphp);
        أرجع;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
