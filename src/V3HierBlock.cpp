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

ساكن string V3_Hier_Command_دلیلاں_Filename(ثابت string& اگیتر, منطقي forMkJson) {
    أرجع v3Global.opt.makeDir() + "/" + اگیتر
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ساكن string V3HierParametersFileName(ثابت string& اگیتر) {
    أرجع v3Global.opt.makeDir() + "/" + اگیتر + "__hierParameters.v";
}

ساكن فراغ V3HierWriteCommonInputs(ثابت V3_Hier_بلاک* hblockp, std::ostream* دا, منطقي forMkJson) {
    ثابت string چوٹی_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    إذا (!forMkJson) {
        لكل (ثابت string& filename : V3_Hier_گراف::سروت_Files(چوٹی_Module_File))
            *دا << filename << "\n";
    }
    لكل (ثابت خودکار& i : v3Global.opt.libraryFiles()) {
        إذا (V3Os::filename_Real_رستہ(i.filename()) != چوٹی_Module_File)
            *دا << "-v " << i.filename() << "\n";
    }
}

//######################################################################

منطقي V3_Hier_بلاک::stringParamPassable(ثابت string& قدر) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    إذا (قدر.لبھو_پہلا_دا("\n\"") != string::npos) أرجع خطأ;
    إذا (قدر.لبھو("/*") != string::npos) أرجع خطأ;
    لكل (ناپ_t pos = قدر.لبھو("//"); pos != string::npos; pos = قدر.لبھو("//", pos + 1)) {
        إذا (pos > 0 && std::خلا_اے(static_cast<بنانشانی حرف>(قدر[pos - 1]))) أرجع خطأ;
    }
    أرجع سچ;
}

V3_Hier_بلاک::StrGParams V3_Hier_بلاک::stringifyParams(ثابت std::سمتیہ<AstVar*>& gparams,
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
                std::سمتیہ<حرف> hexFpStr(32, '\0');
                ثابت صحيح len = VL_SNPRINTF(hexFpStr.انکڑے(), hexFpStr.ناپ(), "%a",
                                            constp->عدد().منزل_Double());
                UASSERT_OBJ(0 < len && static_cast<ناپ_t>(len) < hexFpStr.ناپ(), constp,
                            " is not properly converted to string");
                s = hexFpStr.انکڑے();
            } وإلا إذا (constp->isString()) {
                s = constp->عدد().منزل_String();
                إذا (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } وإلا {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_نمبر عدد{constp->عدد()};
                عدد.isSigned(constp->isSigned());
                s = عدد.ascii(سچ, سچ);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_پٹھ(gparam->name(), s);
        }
    }
    أرجع strParams;
}

VStringList V3_Hier_بلاک::command_دلیلاں(منطقي forMkJson) ثابت {
    VStringList opts;
    ثابت string اگیتر = hier_اگیتر();
    إذا (!forMkJson) {
        opts.ڈالو_پٹھ(" --prefix " + اگیتر);
        opts.ڈالو_پٹھ(" --mod-prefix " + اگیتر);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ڈالو_پٹھ(" --top-module-encoded " + modp()->name());
    }
    opts.ڈالو_پٹھ(" --lib-create " + modp()->name());  // possibly mangled name
    إذا (v3Global.opt.protect_کلید_Provided())
        opts.ڈالو_پٹھ(" --protect-key " + v3Global.opt.protect_کلید_Defaulted());
    opts.ڈالو_پٹھ(" --hierarchical-child " + cvt_منزل_Str(v3Global.opt.دھاگے()));

    ثابت StrGParams gparamsStr = stringifyParams(m_params, سچ);
    لكل (ثابت StrGParam& param : gparamsStr) {
        opts.ڈالو_پٹھ("-G" + param.پہلا + "=" + param.دوجا + "");
    }
    إذا (!m_typeParams.خالی_تھاواں()) {
        opts.ڈالو_پٹھ(" --hierarchical-params-file " + typeParametersFilename());
    }

    ثابت صحيح بلاک_دھاگے = V3Control::getHierWorkers(m_modp->origName());
    إذا (بلاک_دھاگے > 1) {
        إذا (!وچ_خالی_تھاواں()) {
            V3Control::get_Hier_Workers_File_سطر(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } وإلا {
            إذا (v3Global.opt.دھاگے() < بلاک_دھاگے) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.دھاگے()
                                << " hierarchical block threads = " << بلاک_دھاگے);
            }

            opts.ڈالو_پٹھ(" --threads " + std::منزل_string(بلاک_دھاگے));
        }
    }

    أرجع opts;
}

VStringList V3_Hier_بلاک::hier_بلاک_دلیلاں() ثابت {
    VStringList opts;
    ثابت StrGParams gparamsStr = stringifyParams(m_params, خطأ);
    opts.emplace_پٹھ("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    لكل (ثابت StrGParam& pair : gparamsStr) {
        s += "," + pair.پہلا;
        s += "," + pair.دوجا;
    }
    opts.پٹھ() += s;
    أرجع opts;
}

string V3_Hier_بلاک::hier_اگیتر() ثابت { أرجع "V" + modp()->name(); }

string V3_Hier_بلاک::hierSomeFilename(منطقي withDir, ثابت حرف* اگیتر, ثابت حرف* suffix) ثابت {
    string s;
    إذا (withDir) s = hier_اگیتر() + '/';
    s += اگیتر + modp()->name() + suffix;
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
    string filename = V3Os::filename_Real_رستہ(m_modp->fileline()->filename());
    لكل (ثابت خودکار& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        إذا (filename == V3Os::filename_Real_رستہ(v.filename())) أرجع "";
    }
    أرجع filename;
}

فراغ V3_Hier_بلاک::write_Command_دلیلاں_File(منطقي forMkJson) ثابت {
    ثابت std::unique_ptr<std::ofstream> دا{V3File::new_ofstream(command_دلیلاں_Filename(forMkJson))};
    *دا << "--cc\n";

    إذا (!forMkJson) {
        لكل (ثابت V3_گراف_کنارا& کنارا : باہرلا_کنارے()) {
            ثابت V3_Hier_بلاک* ثابت dependencyp = کنارا.چوٹی()->as<V3_Hier_بلاک>();
            *دا << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(سچ) << "\n";
        }
        *دا << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_اگیتر() << " \n";
    }
    V3HierWriteCommonInputs(هذا, دا.get(), forMkJson);
    ثابت VStringList& commandOpts = command_دلیلاں(خطأ);
    لكل (ثابت string& opt : commandOpts) *دا << opt << "\n";
    *دا << hier_بلاک_دلیلاں().front() << "\n";
    لكل (ثابت V3_گراف_کنارا& کنارا : باہرلا_کنارے()) {
        ثابت V3_Hier_بلاک* ثابت dependencyp = کنارا.چوٹی()->as<V3_Hier_بلاک>();
        *دا << dependencyp->hier_بلاک_دلیلاں().front() << "\n";
    }
    *دا << v3Global.opt.all_دلیلاں_String_For_Hier_بلاک(خطأ) << "\n";
}

string V3_Hier_بلاک::command_دلیلاں_Filename(منطقي forMkJson) ثابت {
    أرجع V3_Hier_Command_دلیلاں_Filename(hier_اگیتر(), forMkJson);
}

string V3_Hier_بلاک::typeParametersFilename() ثابت {
    أرجع V3HierParametersFileName(hier_اگیتر());
}

فراغ V3_Hier_بلاک::writeParametersFile() ثابت {
    إذا (m_typeParams.خالی_تھاواں()) أرجع;

    VHashSha512 hash{"type params"};
    ثابت string moduleName = "Vhsh" + hash.digestSymbol24();
    ثابت std::unique_ptr<std::ofstream> دا{V3File::new_ofstream(typeParametersFilename())};
    *دا << "module " << moduleName << ";\n";
    لكل (AstParamTypeDType* ثابت gparam : m_typeParams) {
        AstTypedef* tdefp
            = جديد AstTypedef{جديد File_سطر{File_سطر::built_وچ_Filename()}, gparam->name(), خالی_سوچک,
                             VFlag_بچہ_DType{}, gparam->skipRefp()->clone_درخت_خالص(سچ)};
        V3EmitV::verilog_For_درخت(tdefp, *دا);
        VL_DO_DANGLING(tdefp->delete_درخت(), tdefp);
    }
    *دا << "endmodule\n\n";
    *دا << "`verilator_config\n";
    *دا << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
صنف Hier_بلاک_Usage_Collect_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE
    V3_Hier_گراف* ثابت m_graphp = جديد V3_Hier_گراف{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ثابت AstModule*, V3_Hier_بلاک*> m_mod2vtx;
    AstModule* m_modp = خالی_سوچک;  // The current module
    std::سمتیہ<AstVar*> m_params;  // Overridden value parameters of current module
    std::سمتیہ<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::سمتیہ<V3_Hier_بلاک*> m_childrenp;

    // VISITORSs
    فراغ ویکھو(Ast_گرہ_Module*) override {}  // Ignore all non-AstModule
    فراغ ویکھو(AstModule* nodep) override {
        // Visit each module once
        إذا (nodep->user1_سیٹ_Once()) أرجع;

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
        لكل (V3_Hier_بلاک* ثابت childp : m_childrenp) جديد V3_گراف_کنارا{m_graphp, blockp, childp, 1};
    }
    فراغ ویکھو(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ثابت modp = VN_CAST(nodep->modp(), Module);
        إذا (!modp) أرجع;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        إذا (modp->hier_بلاک()) m_childrenp.emplace_پٹھ(m_mod2vtx.at(modp));
    }
    فراغ ویکھو(AstVar* nodep) override {
        إذا (!m_modp) أرجع;
        إذا (!m_modp->hier_بلاک()) أرجع;
        // Can't handle interface port on hier block
        إذا (nodep->isIfaceRef() && !nodep->is_Iface_ماپا()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        إذا (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ڈالو_پٹھ(nodep);
        }
    }
    فراغ ویکھو(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        إذا (!m_modp->hier_بلاک()) أرجع;
        // Record type parameter of this hier block
        m_typeParams.ڈالو_پٹھ(nodep);
    }

    فراغ ویکھو(Ast_گرہ_Stmt*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    فراغ ویکھو(Ast_Type_جدول*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    واضح Hier_بلاک_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        إذا (dump_گراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_کنارے_جوڑ(&V3_گراف_کنارا::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->ترتیب();  // This is a bit heavy weight, but does produce a topological ordering
        إذا (dump_گراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

عام:
    ساكن V3_Hier_گراف* apply(AstNetlist* netlistp) {
        أرجع Hier_بلاک_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_گراف::سروت_Files(ثابت string& چوٹی_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().ناپ() + 1);
    لكل (ثابت VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_پٹھ(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    إذا (!چوٹی_Module_File.خالی_تھاواں()) sources.emplace_پٹھ(چوٹی_Module_File);
    أرجع sources;
}

فراغ V3_Hier_گراف::write_Command_دلیلاں_Files(منطقي forMkJson) ثابت {

    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        vtx.as<V3_Hier_بلاک>()->write_Command_دلیلاں_File(forMkJson);
    }
    // For the top module
    ثابت std::unique_ptr<std::ofstream> دا{
        V3File::new_ofstream(چوٹی_Command_دلیلاں_Filename(forMkJson))};
    إذا (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        لكل (ثابت V3_گراف_راس& vtx : vertices()) {
            *دا << vtx.as<V3_Hier_بلاک>()->hierWrapperFilename(سچ) << "\n";
        }
    }
    V3HierWriteCommonInputs(خالی_سوچک, دا.get(), forMkJson);
    إذا (!forMkJson) {
        ثابت VString_سیٹ& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت string& i : cppFiles) *دا << i << "\n";
        *دا << "--top-module " << v3Global.rootp()->چوٹی_Modulep()->name() << "\n";
        *دا << "--prefix " << v3Global.opt.اگیتر() << "\n";
        *دا << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *دا << "--mod-prefix " << v3Global.opt.mod_اگیتر() << "\n";
    }
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        *دا << vtx.as<V3_Hier_بلاک>()->hier_بلاک_دلیلاں().front() << "\n";
    }

    إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
        *دا << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    إذا (v3Global.opt.protect_کلید_Provided()) {
        *دا << "--protect-key " << v3Global.opt.protect_کلید_Defaulted() << "\n";
    }
    *دا << "--threads " << cvt_منزل_Str(v3Global.opt.دھاگے()) << "\n";
    *دا << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *دا << v3Global.opt.all_دلیلاں_String_For_Hier_بلاک(سچ) << "\n";
}

string V3_Hier_گراف::چوٹی_Command_دلیلاں_Filename(منطقي forMkJson) {
    أرجع V3_Hier_Command_دلیلاں_Filename(v3Global.opt.اگیتر(), forMkJson);
}

فراغ V3_Hier_گراف::writeParametersFiles() ثابت {
    لكل (ثابت V3_گراف_راس& vtx : vertices()) { vtx.as<V3_Hier_بلاک>()->writeParametersFile(); }
}

//######################################################################

فراغ V3_درجہ_وار::create_گراف(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_گرہ_Module* ثابت modp = netlistp->چوٹی_Modulep();
    إذا (modp->hier_بلاک()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_ہور()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_بلاک(خطأ);
    }

    V3_Hier_گراف* ثابت graphp = Hier_بلاک_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::جوڑو_Stat("HierBlock, Hierarchical blocks", graphp->vertices().ناپ());
    // No hierarchical block is found, nothing to do.
    إذا (graphp->خالی_تھاواں()) {
        VL_DO_DANGLING(مٹاؤ graphp, graphp);
        أرجع;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
