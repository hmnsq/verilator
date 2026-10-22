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

ساكن string V3_Hier_Command_آرگومانها_Filename(ثابت string& پیشوند, منطقي forMkJson) {
    أرجع v3Global.opt.makeDir() + "/" + پیشوند
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ساكن string V3HierParametersFileName(ثابت string& پیشوند) {
    أرجع v3Global.opt.makeDir() + "/" + پیشوند + "__hierParameters.v";
}

ساكن فراغ V3HierWriteCommonInputs(ثابت V3_Hier_بلوک* hblockp, std::ostream* از, منطقي forMkJson) {
    ثابت string بالاترین_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    إذا (!forMkJson) {
        لكل (ثابت string& filename : V3_Hier_گراف::منبع_Files(بالاترین_Module_File))
            *از << filename << "\n";
    }
    لكل (ثابت خودکار& i : v3Global.opt.libraryFiles()) {
        إذا (V3Os::filename_Real_مسیر(i.filename()) != بالاترین_Module_File)
            *از << "-v " << i.filename() << "\n";
    }
}

//######################################################################

منطقي V3_Hier_بلوک::stringParamPassable(ثابت string& مقدار) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    إذا (مقدار.پیدا_کن_اولی_از("\n\"") != string::npos) أرجع خطأ;
    إذا (مقدار.پیدا_کن("/*") != string::npos) أرجع خطأ;
    لكل (اندازه_t pos = مقدار.پیدا_کن("//"); pos != string::npos; pos = مقدار.پیدا_کن("//", pos + 1)) {
        إذا (pos > 0 && std::آیا_فاصله(static_cast<بیعلامت حرف>(مقدار[pos - 1]))) أرجع خطأ;
    }
    أرجع درست;
}

V3_Hier_بلوک::StrGParams V3_Hier_بلوک::stringifyParams(ثابت std::وکتور<AstVar*>& gparams,
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
                std::وکتور<حرف> hexFpStr(32, '\0');
                ثابت صحيح len = VL_SNPRINTF(hexFpStr.داده(), hexFpStr.اندازه(), "%a",
                                            constp->رقم().مقصد_Double());
                UASSERT_OBJ(0 < len && static_cast<اندازه_t>(len) < hexFpStr.اندازه(), constp,
                            " is not properly converted to string");
                s = hexFpStr.داده();
            } وإلا إذا (constp->isString()) {
                s = constp->رقم().مقصد_String();
                إذا (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } وإلا {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_عدد رقم{constp->رقم()};
                رقم.isSigned(constp->isSigned());
                s = رقم.ascii(درست, درست);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_پشت(gparam->name(), s);
        }
    }
    أرجع strParams;
}

VStringList V3_Hier_بلوک::command_آرگومانها(منطقي forMkJson) ثابت {
    VStringList opts;
    ثابت string پیشوند = hier_پیشوند();
    إذا (!forMkJson) {
        opts.افزودن_پشت(" --prefix " + پیشوند);
        opts.افزودن_پشت(" --mod-prefix " + پیشوند);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.افزودن_پشت(" --top-module-encoded " + modp()->name());
    }
    opts.افزودن_پشت(" --lib-create " + modp()->name());  // possibly mangled name
    إذا (v3Global.opt.protect_کلید_Provided())
        opts.افزودن_پشت(" --protect-key " + v3Global.opt.protect_کلید_Defaulted());
    opts.افزودن_پشت(" --hierarchical-child " + cvt_مقصد_Str(v3Global.opt.نخها()));

    ثابت StrGParams gparamsStr = stringifyParams(m_params, درست);
    لكل (ثابت StrGParam& param : gparamsStr) {
        opts.افزودن_پشت("-G" + param.اولی + "=" + param.دومی + "");
    }
    إذا (!m_typeParams.خالی()) {
        opts.افزودن_پشت(" --hierarchical-params-file " + typeParametersFilename());
    }

    ثابت صحيح بلوک_نخها = V3Control::getHierWorkers(m_modp->origName());
    إذا (بلوک_نخها > 1) {
        إذا (!درون_خالی()) {
            V3Control::get_Hier_Workers_File_سطر(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } وإلا {
            إذا (v3Global.opt.نخها() < بلوک_نخها) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.نخها()
                                << " hierarchical block threads = " << بلوک_نخها);
            }

            opts.افزودن_پشت(" --threads " + std::مقصد_string(بلوک_نخها));
        }
    }

    أرجع opts;
}

VStringList V3_Hier_بلوک::hier_بلوک_آرگومانها() ثابت {
    VStringList opts;
    ثابت StrGParams gparamsStr = stringifyParams(m_params, خطأ);
    opts.emplace_پشت("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    لكل (ثابت StrGParam& pair : gparamsStr) {
        s += "," + pair.اولی;
        s += "," + pair.دومی;
    }
    opts.پشت() += s;
    أرجع opts;
}

string V3_Hier_بلوک::hier_پیشوند() ثابت { أرجع "V" + modp()->name(); }

string V3_Hier_بلوک::hierSomeFilename(منطقي withDir, ثابت حرف* پیشوند, ثابت حرف* suffix) ثابت {
    string s;
    إذا (withDir) s = hier_پیشوند() + '/';
    s += پیشوند + modp()->name() + suffix;
    أرجع s;
}

string V3_Hier_بلوک::hierWrapperFilename(منطقي withDir) ثابت {
    أرجع hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_بلوک::hierMkFilename(منطقي withDir) ثابت {
    أرجع hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_بلوک::hierLibFilename(منطقي withDir) ثابت {
    أرجع hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_بلوک::hierGeneratedFilenames(منطقي withDir) ثابت {
    أرجع hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_بلوک::vFileIfNecessary() ثابت {
    string filename = V3Os::filename_Real_مسیر(m_modp->fileline()->filename());
    لكل (ثابت خودکار& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        إذا (filename == V3Os::filename_Real_مسیر(v.filename())) أرجع "";
    }
    أرجع filename;
}

فراغ V3_Hier_بلوک::write_Command_آرگومانها_File(منطقي forMkJson) ثابت {
    ثابت std::unique_ptr<std::ofstream> از{V3File::new_ofstream(command_آرگومانها_Filename(forMkJson))};
    *از << "--cc\n";

    إذا (!forMkJson) {
        لكل (ثابت V3_گراف_یال& یال : بیرون_یالها()) {
            ثابت V3_Hier_بلوک* ثابت dependencyp = یال.بالاترین()->as<V3_Hier_بلوک>();
            *از << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(درست) << "\n";
        }
        *از << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_پیشوند() << " \n";
    }
    V3HierWriteCommonInputs(هذا, از.get(), forMkJson);
    ثابت VStringList& commandOpts = command_آرگومانها(خطأ);
    لكل (ثابت string& opt : commandOpts) *از << opt << "\n";
    *از << hier_بلوک_آرگومانها().front() << "\n";
    لكل (ثابت V3_گراف_یال& یال : بیرون_یالها()) {
        ثابت V3_Hier_بلوک* ثابت dependencyp = یال.بالاترین()->as<V3_Hier_بلوک>();
        *از << dependencyp->hier_بلوک_آرگومانها().front() << "\n";
    }
    *از << v3Global.opt.all_آرگومانها_String_For_Hier_بلوک(خطأ) << "\n";
}

string V3_Hier_بلوک::command_آرگومانها_Filename(منطقي forMkJson) ثابت {
    أرجع V3_Hier_Command_آرگومانها_Filename(hier_پیشوند(), forMkJson);
}

string V3_Hier_بلوک::typeParametersFilename() ثابت {
    أرجع V3HierParametersFileName(hier_پیشوند());
}

فراغ V3_Hier_بلوک::writeParametersFile() ثابت {
    إذا (m_typeParams.خالی()) أرجع;

    VHashSha512 hash{"type params"};
    ثابت string moduleName = "Vhsh" + hash.digestSymbol24();
    ثابت std::unique_ptr<std::ofstream> از{V3File::new_ofstream(typeParametersFilename())};
    *از << "module " << moduleName << ";\n";
    لكل (AstParamTypeDType* ثابت gparam : m_typeParams) {
        AstTypedef* tdefp
            = جديد AstTypedef{جديد File_سطر{File_سطر::built_درون_Filename()}, gparam->name(), اشاره_تهی,
                             VFlag_فرزند_DType{}, gparam->skipRefp()->clone_درخت_خالص(درست)};
        V3EmitV::verilog_For_درخت(tdefp, *از);
        VL_DO_DANGLING(tdefp->delete_درخت(), tdefp);
    }
    *از << "endmodule\n\n";
    *از << "`verilator_config\n";
    *از << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
صنف Hier_بلوک_Usage_Collect_Visitor final : عام VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE
    V3_Hier_گراف* ثابت m_graphp = جديد V3_Hier_گراف{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ثابت AstModule*, V3_Hier_بلوک*> m_mod2vtx;
    AstModule* m_modp = اشاره_تهی;  // The current module
    std::وکتور<AstVar*> m_params;  // Overridden value parameters of current module
    std::وکتور<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::وکتور<V3_Hier_بلوک*> m_childrenp;

    // VISITORSs
    فراغ بازدید(Ast_گره_Module*) override {}  // Ignore all non-AstModule
    فراغ بازدید(AstModule* nodep) override {
        // Visit each module once
        إذا (nodep->user1_مجموعه_Once()) أرجع;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        إذا (!nodep->hier_بلوک()) {
            iterateChildrenConst(nodep);
            أرجع;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_بلوک* ثابت blockp = جديد V3_Hier_بلوک{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        لكل (V3_Hier_بلوک* ثابت childp : m_childrenp) جديد V3_گراف_یال{m_graphp, blockp, childp, 1};
    }
    فراغ بازدید(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ثابت modp = VN_CAST(nodep->modp(), Module);
        إذا (!modp) أرجع;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        إذا (modp->hier_بلوک()) m_childrenp.emplace_پشت(m_mod2vtx.at(modp));
    }
    فراغ بازدید(AstVar* nodep) override {
        إذا (!m_modp) أرجع;
        إذا (!m_modp->hier_بلوک()) أرجع;
        // Can't handle interface port on hier block
        إذا (nodep->isIfaceRef() && !nodep->is_Iface_والد()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        إذا (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.افزودن_پشت(nodep);
        }
    }
    فراغ بازدید(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        إذا (!m_modp->hier_بلوک()) أرجع;
        // Record type parameter of this hier block
        m_typeParams.افزودن_پشت(nodep);
    }

    فراغ بازدید(Ast_گره_Stmt*) override {}  // Accelerate
    فراغ بازدید(Ast_گره_عبارت*) override {}  // Accelerate
    فراغ بازدید(Ast_Type_جدول*) override {}  // Accelerate
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    صریح Hier_بلوک_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        إذا (dump_گراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_یالها_جمع(&V3_گراف_یال::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->ترتیب();  // This is a bit heavy weight, but does produce a topological ordering
        إذا (dump_گراف_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

عام:
    ساكن V3_Hier_گراف* apply(AstNetlist* netlistp) {
        أرجع Hier_بلوک_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_گراف::منبع_Files(ثابت string& بالاترین_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().اندازه() + 1);
    لكل (ثابت VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_پشت(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    إذا (!بالاترین_Module_File.خالی()) sources.emplace_پشت(بالاترین_Module_File);
    أرجع sources;
}

فراغ V3_Hier_گراف::write_Command_آرگومانها_Files(منطقي forMkJson) ثابت {

    لكل (ثابت V3_گراف_رأس& vtx : vertices()) {
        vtx.as<V3_Hier_بلوک>()->write_Command_آرگومانها_File(forMkJson);
    }
    // For the top module
    ثابت std::unique_ptr<std::ofstream> از{
        V3File::new_ofstream(بالاترین_Command_آرگومانها_Filename(forMkJson))};
    إذا (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        لكل (ثابت V3_گراف_رأس& vtx : vertices()) {
            *از << vtx.as<V3_Hier_بلوک>()->hierWrapperFilename(درست) << "\n";
        }
    }
    V3HierWriteCommonInputs(اشاره_تهی, از.get(), forMkJson);
    إذا (!forMkJson) {
        ثابت VString_مجموعه& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت string& i : cppFiles) *از << i << "\n";
        *از << "--top-module " << v3Global.rootp()->بالاترین_Modulep()->name() << "\n";
        *از << "--prefix " << v3Global.opt.پیشوند() << "\n";
        *از << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *از << "--mod-prefix " << v3Global.opt.mod_پیشوند() << "\n";
    }
    لكل (ثابت V3_گراف_رأس& vtx : vertices()) {
        *از << vtx.as<V3_Hier_بلوک>()->hier_بلوک_آرگومانها().front() << "\n";
    }

    إذا (!v3Global.opt.libCreate().خالی()) {
        *از << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    إذا (v3Global.opt.protect_کلید_Provided()) {
        *از << "--protect-key " << v3Global.opt.protect_کلید_Defaulted() << "\n";
    }
    *از << "--threads " << cvt_مقصد_Str(v3Global.opt.نخها()) << "\n";
    *از << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *از << v3Global.opt.all_آرگومانها_String_For_Hier_بلوک(درست) << "\n";
}

string V3_Hier_گراف::بالاترین_Command_آرگومانها_Filename(منطقي forMkJson) {
    أرجع V3_Hier_Command_آرگومانها_Filename(v3Global.opt.پیشوند(), forMkJson);
}

فراغ V3_Hier_گراف::writeParametersFiles() ثابت {
    لكل (ثابت V3_گراف_رأس& vtx : vertices()) { vtx.as<V3_Hier_بلوک>()->writeParametersFile(); }
}

//######################################################################

فراغ V3_سلسلهمراتبی::create_گراف(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_گره_Module* ثابت modp = netlistp->بالاترین_Modulep();
    إذا (modp->hier_بلوک()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_بیشتر()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_بلوک(خطأ);
    }

    V3_Hier_گراف* ثابت graphp = Hier_بلوک_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::جمع_کن_Stat("HierBlock, Hierarchical blocks", graphp->vertices().اندازه());
    // No hierarchical block is found, nothing to do.
    إذا (graphp->خالی()) {
        VL_DO_DANGLING(حذف graphp, graphp);
        أرجع;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
