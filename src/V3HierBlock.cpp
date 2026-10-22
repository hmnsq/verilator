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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3HierBlock.h"

#شامل "V3Control.h"
#شامل "V3EmitV.h"
#شامل "V3File.h"
#شامل "V3Os.h"
#شامل "V3Stats.h"
#شامل "V3String.h"

#شامل <cctype>
#شامل <memory>
#شامل <sstream>
#شامل <utility>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ساکن string V3_Hier_Command_دلائل_Filename(ثابت string& سابقہ, منطقی forMkJson) {
    واپس v3Global.opt.makeDir() + "/" + سابقہ
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ساکن string V3HierParametersFileName(ثابت string& سابقہ) {
    واپس v3Global.opt.makeDir() + "/" + سابقہ + "__hierParameters.v";
}

ساکن خالی V3HierWriteCommonInputs(ثابت V3_Hier_بلاک* hblockp, std::ostream* کا, منطقی forMkJson) {
    ثابت string چوٹی_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    اگر (!forMkJson) {
        برائے (ثابت string& filename : V3_Hier_گراف::ماخذ_Files(چوٹی_Module_File))
            *کا << filename << "\n";
    }
    برائے (ثابت خودکار& i : v3Global.opt.libraryFiles()) {
        اگر (V3Os::filename_Real_راستہ(i.filename()) != چوٹی_Module_File)
            *کا << "-v " << i.filename() << "\n";
    }
}

//######################################################################

منطقی V3_Hier_بلاک::stringParamPassable(ثابت string& قدر) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    اگر (قدر.ڈھونڈو_پہلا_کا("\n\"") != string::npos) واپس جھوٹ;
    اگر (قدر.ڈھونڈو("/*") != string::npos) واپس جھوٹ;
    برائے (سائز_t pos = قدر.ڈھونڈو("//"); pos != string::npos; pos = قدر.ڈھونڈو("//", pos + 1)) {
        اگر (pos > 0 && std::خلا_ہے(static_cast<غیرعلامتی حرف>(قدر[pos - 1]))) واپس جھوٹ;
    }
    واپس سچ;
}

V3_Hier_بلاک::StrGParams V3_Hier_بلاک::stringifyParams(ثابت std::سمتیہ<AstVar*>& gparams,
                                                     منطقی forGOption) {
    StrGParams strParams;
    برائے (ثابت AstVar* ثابت gparam : gparams) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            اگر (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::سمتیہ<حرف> hexFpStr(32, '\0');
                ثابت صحیح len = VL_SNPRINTF(hexFpStr.ڈیٹا(), hexFpStr.سائز(), "%a",
                                            constp->عدد().مقصد_Double());
                UASSERT_OBJ(0 < len && static_cast<سائز_t>(len) < hexFpStr.سائز(), constp,
                            " is not properly converted to string");
                s = hexFpStr.ڈیٹا();
            } ورنہ اگر (constp->isString()) {
                s = constp->عدد().مقصد_String();
                اگر (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } ورنہ {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_نمبر عدد{constp->عدد()};
                عدد.isSigned(constp->isSigned());
                s = عدد.ascii(سچ, سچ);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_عقب(gparam->name(), s);
        }
    }
    واپس strParams;
}

VStringList V3_Hier_بلاک::command_دلائل(منطقی forMkJson) ثابت {
    VStringList opts;
    ثابت string سابقہ = hier_سابقہ();
    اگر (!forMkJson) {
        opts.ڈالو_عقب(" --prefix " + سابقہ);
        opts.ڈالو_عقب(" --mod-prefix " + سابقہ);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ڈالو_عقب(" --top-module-encoded " + modp()->name());
    }
    opts.ڈالو_عقب(" --lib-create " + modp()->name());  // possibly mangled name
    اگر (v3Global.opt.protect_کلید_Provided())
        opts.ڈالو_عقب(" --protect-key " + v3Global.opt.protect_کلید_Defaulted());
    opts.ڈالو_عقب(" --hierarchical-child " + cvt_مقصد_Str(v3Global.opt.دھاگے()));

    ثابت StrGParams gparamsStr = stringifyParams(m_params, سچ);
    برائے (ثابت StrGParam& param : gparamsStr) {
        opts.ڈالو_عقب("-G" + param.پہلا + "=" + param.دوسرا + "");
    }
    اگر (!m_typeParams.خالی_خانے()) {
        opts.ڈالو_عقب(" --hierarchical-params-file " + typeParametersFilename());
    }

    ثابت صحیح بلاک_دھاگے = V3Control::getHierWorkers(m_modp->origName());
    اگر (بلاک_دھاگے > 1) {
        اگر (!اندر_خالی_خانے()) {
            V3Control::get_Hier_Workers_File_سطر(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } ورنہ {
            اگر (v3Global.opt.دھاگے() < بلاک_دھاگے) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.دھاگے()
                                << " hierarchical block threads = " << بلاک_دھاگے);
            }

            opts.ڈالو_عقب(" --threads " + std::مقصد_string(بلاک_دھاگے));
        }
    }

    واپس opts;
}

VStringList V3_Hier_بلاک::hier_بلاک_دلائل() ثابت {
    VStringList opts;
    ثابت StrGParams gparamsStr = stringifyParams(m_params, جھوٹ);
    opts.emplace_عقب("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    برائے (ثابت StrGParam& pair : gparamsStr) {
        s += "," + pair.پہلا;
        s += "," + pair.دوسرا;
    }
    opts.عقب() += s;
    واپس opts;
}

string V3_Hier_بلاک::hier_سابقہ() ثابت { واپس "V" + modp()->name(); }

string V3_Hier_بلاک::hierSomeFilename(منطقی withDir, ثابت حرف* سابقہ, ثابت حرف* suffix) ثابت {
    string s;
    اگر (withDir) s = hier_سابقہ() + '/';
    s += سابقہ + modp()->name() + suffix;
    واپس s;
}

string V3_Hier_بلاک::hierWrapperFilename(منطقی withDir) ثابت {
    واپس hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_بلاک::hierMkFilename(منطقی withDir) ثابت {
    واپس hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_بلاک::hierLibFilename(منطقی withDir) ثابت {
    واپس hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_بلاک::hierGeneratedFilenames(منطقی withDir) ثابت {
    واپس hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_بلاک::vFileIfNecessary() ثابت {
    string filename = V3Os::filename_Real_راستہ(m_modp->fileline()->filename());
    برائے (ثابت خودکار& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        اگر (filename == V3Os::filename_Real_راستہ(v.filename())) واپس "";
    }
    واپس filename;
}

خالی V3_Hier_بلاک::write_Command_دلائل_File(منطقی forMkJson) ثابت {
    ثابت std::unique_ptr<std::ofstream> کا{V3File::new_ofstream(command_دلائل_Filename(forMkJson))};
    *کا << "--cc\n";

    اگر (!forMkJson) {
        برائے (ثابت V3_گراف_کنارہ& کنارہ : برآمد_کنارے()) {
            ثابت V3_Hier_بلاک* ثابت dependencyp = کنارہ.چوٹی()->as<V3_Hier_بلاک>();
            *کا << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(سچ) << "\n";
        }
        *کا << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_سابقہ() << " \n";
    }
    V3HierWriteCommonInputs(یہ, کا.get(), forMkJson);
    ثابت VStringList& commandOpts = command_دلائل(جھوٹ);
    برائے (ثابت string& opt : commandOpts) *کا << opt << "\n";
    *کا << hier_بلاک_دلائل().front() << "\n";
    برائے (ثابت V3_گراف_کنارہ& کنارہ : برآمد_کنارے()) {
        ثابت V3_Hier_بلاک* ثابت dependencyp = کنارہ.چوٹی()->as<V3_Hier_بلاک>();
        *کا << dependencyp->hier_بلاک_دلائل().front() << "\n";
    }
    *کا << v3Global.opt.all_دلائل_String_For_Hier_بلاک(جھوٹ) << "\n";
}

string V3_Hier_بلاک::command_دلائل_Filename(منطقی forMkJson) ثابت {
    واپس V3_Hier_Command_دلائل_Filename(hier_سابقہ(), forMkJson);
}

string V3_Hier_بلاک::typeParametersFilename() ثابت {
    واپس V3HierParametersFileName(hier_سابقہ());
}

خالی V3_Hier_بلاک::writeParametersFile() ثابت {
    اگر (m_typeParams.خالی_خانے()) واپس;

    VHashSha512 hash{"type params"};
    ثابت string moduleName = "Vhsh" + hash.digestSymbol24();
    ثابت std::unique_ptr<std::ofstream> کا{V3File::new_ofstream(typeParametersFilename())};
    *کا << "module " << moduleName << ";\n";
    برائے (AstParamTypeDType* ثابت gparam : m_typeParams) {
        AstTypedef* tdefp
            = نیا AstTypedef{نیا File_سطر{File_سطر::built_اندر_Filename()}, gparam->name(), خالی_اشاریہ,
                             VFlag_بچہ_DType{}, gparam->skipRefp()->clone_درخت_خالص(سچ)};
        V3EmitV::verilog_For_درخت(tdefp, *کا);
        VL_DO_DANGLING(tdefp->delete_درخت(), tdefp);
    }
    *کا << "endmodule\n\n";
    *کا << "`verilator_config\n";
    *کا << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
جماعت Hier_بلاک_Usage_Collect_Visitor final : عوامی VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    V3_Hier_گراف* ثابت m_graphp = نیا V3_Hier_گراف{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ثابت AstModule*, V3_Hier_بلاک*> m_mod2vtx;
    AstModule* m_modp = خالی_اشاریہ;  // The current module
    std::سمتیہ<AstVar*> m_params;  // Overridden value parameters of current module
    std::سمتیہ<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::سمتیہ<V3_Hier_بلاک*> m_childrenp;

    // VISITORSs
    خالی دورہ(Ast_گرہ_Module*) override {}  // Ignore all non-AstModule
    خالی دورہ(AstModule* nodep) override {
        // Visit each module once
        اگر (nodep->user1_سیٹ_Once()) واپس;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        اگر (!nodep->hier_بلاک()) {
            iterateChildrenConst(nodep);
            واپس;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_بلاک* ثابت blockp = نیا V3_Hier_بلاک{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        برائے (V3_Hier_بلاک* ثابت childp : m_childrenp) نیا V3_گراف_کنارہ{m_graphp, blockp, childp, 1};
    }
    خالی دورہ(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ثابت modp = VN_CAST(nodep->modp(), Module);
        اگر (!modp) واپس;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        اگر (modp->hier_بلاک()) m_childrenp.emplace_عقب(m_mod2vtx.at(modp));
    }
    خالی دورہ(AstVar* nodep) override {
        اگر (!m_modp) واپس;
        اگر (!m_modp->hier_بلاک()) واپس;
        // Can't handle interface port on hier block
        اگر (nodep->isIfaceRef() && !nodep->is_Iface_والد()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        اگر (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ڈالو_عقب(nodep);
        }
    }
    خالی دورہ(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        اگر (!m_modp->hier_بلاک()) واپس;
        // Record type parameter of this hier block
        m_typeParams.ڈالو_عقب(nodep);
    }

    خالی دورہ(Ast_گرہ_Stmt*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    خالی دورہ(Ast_Type_جدول*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    صریح Hier_بلاک_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        اگر (dump_گراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_کنارے_مجموع(&V3_گراف_کنارہ::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->ترتیب();  // This is a bit heavy weight, but does produce a topological ordering
        اگر (dump_گراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

عوامی:
    ساکن V3_Hier_گراف* apply(AstNetlist* netlistp) {
        واپس Hier_بلاک_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_گراف::ماخذ_Files(ثابت string& چوٹی_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().سائز() + 1);
    برائے (ثابت VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_عقب(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    اگر (!چوٹی_Module_File.خالی_خانے()) sources.emplace_عقب(چوٹی_Module_File);
    واپس sources;
}

خالی V3_Hier_گراف::write_Command_دلائل_Files(منطقی forMkJson) ثابت {

    برائے (ثابت V3_گراف_راس& vtx : vertices()) {
        vtx.as<V3_Hier_بلاک>()->write_Command_دلائل_File(forMkJson);
    }
    // For the top module
    ثابت std::unique_ptr<std::ofstream> کا{
        V3File::new_ofstream(چوٹی_Command_دلائل_Filename(forMkJson))};
    اگر (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        برائے (ثابت V3_گراف_راس& vtx : vertices()) {
            *کا << vtx.as<V3_Hier_بلاک>()->hierWrapperFilename(سچ) << "\n";
        }
    }
    V3HierWriteCommonInputs(خالی_اشاریہ, کا.get(), forMkJson);
    اگر (!forMkJson) {
        ثابت VString_سیٹ& cppFiles = v3Global.opt.cppFiles();
        برائے (ثابت string& i : cppFiles) *کا << i << "\n";
        *کا << "--top-module " << v3Global.rootp()->چوٹی_Modulep()->name() << "\n";
        *کا << "--prefix " << v3Global.opt.سابقہ() << "\n";
        *کا << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *کا << "--mod-prefix " << v3Global.opt.mod_سابقہ() << "\n";
    }
    برائے (ثابت V3_گراف_راس& vtx : vertices()) {
        *کا << vtx.as<V3_Hier_بلاک>()->hier_بلاک_دلائل().front() << "\n";
    }

    اگر (!v3Global.opt.libCreate().خالی_خانے()) {
        *کا << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    اگر (v3Global.opt.protect_کلید_Provided()) {
        *کا << "--protect-key " << v3Global.opt.protect_کلید_Defaulted() << "\n";
    }
    *کا << "--threads " << cvt_مقصد_Str(v3Global.opt.دھاگے()) << "\n";
    *کا << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *کا << v3Global.opt.all_دلائل_String_For_Hier_بلاک(سچ) << "\n";
}

string V3_Hier_گراف::چوٹی_Command_دلائل_Filename(منطقی forMkJson) {
    واپس V3_Hier_Command_دلائل_Filename(v3Global.opt.سابقہ(), forMkJson);
}

خالی V3_Hier_گراف::writeParametersFiles() ثابت {
    برائے (ثابت V3_گراف_راس& vtx : vertices()) { vtx.as<V3_Hier_بلاک>()->writeParametersFile(); }
}

//######################################################################

خالی V3_درجہ_وار::create_گراف(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_گرہ_Module* ثابت modp = netlistp->چوٹی_Modulep();
    اگر (modp->hier_بلاک()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_مزید()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_بلاک(جھوٹ);
    }

    V3_Hier_گراف* ثابت graphp = Hier_بلاک_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::جوڑو_Stat("HierBlock, Hierarchical blocks", graphp->vertices().سائز());
    // No hierarchical block is found, nothing to do.
    اگر (graphp->خالی_خانے()) {
        VL_DO_DANGLING(حذف graphp, graphp);
        واپس;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
