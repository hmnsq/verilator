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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3HierBlock.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cctype>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਸਥਿਰ string V3_Hier_Command_ਦਲੀਲਾਂ_Filename(ਅਚਲ string& ਅਗੇਤਰ, ਤਾਰਕਿਕ forMkJson) {
    ਵਾਪਸ v3Global.opt.makeDir() + "/" + ਅਗੇਤਰ
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ਸਥਿਰ string V3HierParametersFileName(ਅਚਲ string& ਅਗੇਤਰ) {
    ਵਾਪਸ v3Global.opt.makeDir() + "/" + ਅਗੇਤਰ + "__hierParameters.v";
}

ਸਥਿਰ ਖਾਲੀ V3HierWriteCommonInputs(ਅਚਲ V3_Hier_ਖੰਡ* hblockp, std::ostream* ਦਾ, ਤਾਰਕਿਕ forMkJson) {
    ਅਚਲ string ਸਿਖਰ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    ਜੇ (!forMkJson) {
        ਲਈ (ਅਚਲ string& filename : V3_Hier_ਗ੍ਰਾਫ::ਸਰੋਤ_Files(ਸਿਖਰ_Module_File))
            *ਦਾ << filename << "\n";
    }
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : v3Global.opt.libraryFiles()) {
        ਜੇ (V3Os::filename_Real_ਰਸਤਾ(i.filename()) != ਸਿਖਰ_Module_File)
            *ਦਾ << "-v " << i.filename() << "\n";
    }
}

//######################################################################

ਤਾਰਕਿਕ V3_Hier_ਖੰਡ::stringParamPassable(ਅਚਲ string& ਮੁੱਲ) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    ਜੇ (ਮੁੱਲ.ਲੱਭੋ_ਪਹਿਲਾ_ਦਾ("\n\"") != string::npos) ਵਾਪਸ ਝੂਠ;
    ਜੇ (ਮੁੱਲ.ਲੱਭੋ("/*") != string::npos) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਮਾਪ_t pos = ਮੁੱਲ.ਲੱਭੋ("//"); pos != string::npos; pos = ਮੁੱਲ.ਲੱਭੋ("//", pos + 1)) {
        ਜੇ (pos > 0 && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ>(ਮੁੱਲ[pos - 1]))) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}

V3_Hier_ਖੰਡ::StrGParams V3_Hier_ਖੰਡ::stringifyParams(ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& gparams,
                                                     ਤਾਰਕਿਕ forGOption) {
    StrGParams strParams;
    ਲਈ (ਅਚਲ AstVar* ਅਚਲ gparam : gparams) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            ਜੇ (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::ਸਦਿਸ਼<ਅੱਖਰ> hexFpStr(32, '\0');
                ਅਚਲ ਪੂਰਨ len = VL_SNPRINTF(hexFpStr.ਅੰਕੜੇ(), hexFpStr.ਮਾਪ(), "%a",
                                            constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Double());
                UASSERT_OBJ(0 < len && static_cast<ਮਾਪ_t>(len) < hexFpStr.ਮਾਪ(), constp,
                            " is not properly converted to string");
                s = hexFpStr.ਅੰਕੜੇ();
            } ਨਹੀਂਤਾਂ ਜੇ (constp->isString()) {
                s = constp->ਅੰਕ().ਮੰਜ਼ਿਲ_String();
                ਜੇ (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } ਨਹੀਂਤਾਂ {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_ਸੰਖਿਆ ਅੰਕ{constp->ਅੰਕ()};
                ਅੰਕ.isSigned(constp->isSigned());
                s = ਅੰਕ.ascii(ਸੱਚ, ਸੱਚ);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_ਪਿੱਠ(gparam->name(), s);
        }
    }
    ਵਾਪਸ strParams;
}

VStringList V3_Hier_ਖੰਡ::command_ਦਲੀਲਾਂ(ਤਾਰਕਿਕ forMkJson) ਅਚਲ {
    VStringList opts;
    ਅਚਲ string ਅਗੇਤਰ = hier_ਅਗੇਤਰ();
    ਜੇ (!forMkJson) {
        opts.ਧੱਕੋ_ਪਿੱਠ(" --prefix " + ਅਗੇਤਰ);
        opts.ਧੱਕੋ_ਪਿੱਠ(" --mod-prefix " + ਅਗੇਤਰ);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ਧੱਕੋ_ਪਿੱਠ(" --top-module-encoded " + modp()->name());
    }
    opts.ਧੱਕੋ_ਪਿੱਠ(" --lib-create " + modp()->name());  // possibly mangled name
    ਜੇ (v3Global.opt.protect_ਕੁੰਜੀ_Provided())
        opts.ਧੱਕੋ_ਪਿੱਠ(" --protect-key " + v3Global.opt.protect_ਕੁੰਜੀ_Defaulted());
    opts.ਧੱਕੋ_ਪਿੱਠ(" --hierarchical-child " + cvt_ਮੰਜ਼ਿਲ_Str(v3Global.opt.ਧਾਗੇ()));

    ਅਚਲ StrGParams gparamsStr = stringifyParams(m_params, ਸੱਚ);
    ਲਈ (ਅਚਲ StrGParam& param : gparamsStr) {
        opts.ਧੱਕੋ_ਪਿੱਠ("-G" + param.ਪਹਿਲਾ + "=" + param.ਦੂਜਾ + "");
    }
    ਜੇ (!m_typeParams.ਖਾਲੀ_ਥਾਵਾਂ()) {
        opts.ਧੱਕੋ_ਪਿੱਠ(" --hierarchical-params-file " + typeParametersFilename());
    }

    ਅਚਲ ਪੂਰਨ ਖੰਡ_ਧਾਗੇ = V3Control::getHierWorkers(m_modp->origName());
    ਜੇ (ਖੰਡ_ਧਾਗੇ > 1) {
        ਜੇ (!ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) {
            V3Control::get_Hier_Workers_File_ਸਤਰ(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } ਨਹੀਂਤਾਂ {
            ਜੇ (v3Global.opt.ਧਾਗੇ() < ਖੰਡ_ਧਾਗੇ) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.ਧਾਗੇ()
                                << " hierarchical block threads = " << ਖੰਡ_ਧਾਗੇ);
            }

            opts.ਧੱਕੋ_ਪਿੱਠ(" --threads " + std::ਮੰਜ਼ਿਲ_string(ਖੰਡ_ਧਾਗੇ));
        }
    }

    ਵਾਪਸ opts;
}

VStringList V3_Hier_ਖੰਡ::hier_ਖੰਡ_ਦਲੀਲਾਂ() ਅਚਲ {
    VStringList opts;
    ਅਚਲ StrGParams gparamsStr = stringifyParams(m_params, ਝੂਠ);
    opts.emplace_ਪਿੱਠ("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    ਲਈ (ਅਚਲ StrGParam& pair : gparamsStr) {
        s += "," + pair.ਪਹਿਲਾ;
        s += "," + pair.ਦੂਜਾ;
    }
    opts.ਪਿੱਠ() += s;
    ਵਾਪਸ opts;
}

string V3_Hier_ਖੰਡ::hier_ਅਗੇਤਰ() ਅਚਲ { ਵਾਪਸ "V" + modp()->name(); }

string V3_Hier_ਖੰਡ::hierSomeFilename(ਤਾਰਕਿਕ withDir, ਅਚਲ ਅੱਖਰ* ਅਗੇਤਰ, ਅਚਲ ਅੱਖਰ* suffix) ਅਚਲ {
    string s;
    ਜੇ (withDir) s = hier_ਅਗੇਤਰ() + '/';
    s += ਅਗੇਤਰ + modp()->name() + suffix;
    ਵਾਪਸ s;
}

string V3_Hier_ਖੰਡ::hierWrapperFilename(ਤਾਰਕਿਕ withDir) ਅਚਲ {
    ਵਾਪਸ hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_ਖੰਡ::hierMkFilename(ਤਾਰਕਿਕ withDir) ਅਚਲ {
    ਵਾਪਸ hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_ਖੰਡ::hierLibFilename(ਤਾਰਕਿਕ withDir) ਅਚਲ {
    ਵਾਪਸ hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_ਖੰਡ::hierGeneratedFilenames(ਤਾਰਕਿਕ withDir) ਅਚਲ {
    ਵਾਪਸ hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_ਖੰਡ::vFileIfNecessary() ਅਚਲ {
    string filename = V3Os::filename_Real_ਰਸਤਾ(m_modp->fileline()->filename());
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        ਜੇ (filename == V3Os::filename_Real_ਰਸਤਾ(v.filename())) ਵਾਪਸ "";
    }
    ਵਾਪਸ filename;
}

ਖਾਲੀ V3_Hier_ਖੰਡ::write_Command_ਦਲੀਲਾਂ_File(ਤਾਰਕਿਕ forMkJson) ਅਚਲ {
    ਅਚਲ std::unique_ptr<std::ofstream> ਦਾ{V3File::new_ofstream(command_ਦਲੀਲਾਂ_Filename(forMkJson))};
    *ਦਾ << "--cc\n";

    ਜੇ (!forMkJson) {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ dependencyp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Hier_ਖੰਡ>();
            *ਦਾ << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(ਸੱਚ) << "\n";
        }
        *ਦਾ << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_ਅਗੇਤਰ() << " \n";
    }
    V3HierWriteCommonInputs(ਇਹ, ਦਾ.get(), forMkJson);
    ਅਚਲ VStringList& commandOpts = command_ਦਲੀਲਾਂ(ਝੂਠ);
    ਲਈ (ਅਚਲ string& opt : commandOpts) *ਦਾ << opt << "\n";
    *ਦਾ << hier_ਖੰਡ_ਦਲੀਲਾਂ().front() << "\n";
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ dependencyp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Hier_ਖੰਡ>();
        *ਦਾ << dependencyp->hier_ਖੰਡ_ਦਲੀਲਾਂ().front() << "\n";
    }
    *ਦਾ << v3Global.opt.all_ਦਲੀਲਾਂ_String_For_Hier_ਖੰਡ(ਝੂਠ) << "\n";
}

string V3_Hier_ਖੰਡ::command_ਦਲੀਲਾਂ_Filename(ਤਾਰਕਿਕ forMkJson) ਅਚਲ {
    ਵਾਪਸ V3_Hier_Command_ਦਲੀਲਾਂ_Filename(hier_ਅਗੇਤਰ(), forMkJson);
}

string V3_Hier_ਖੰਡ::typeParametersFilename() ਅਚਲ {
    ਵਾਪਸ V3HierParametersFileName(hier_ਅਗੇਤਰ());
}

ਖਾਲੀ V3_Hier_ਖੰਡ::writeParametersFile() ਅਚਲ {
    ਜੇ (m_typeParams.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

    VHashSha512 hash{"type params"};
    ਅਚਲ string moduleName = "Vhsh" + hash.digestSymbol24();
    ਅਚਲ std::unique_ptr<std::ofstream> ਦਾ{V3File::new_ofstream(typeParametersFilename())};
    *ਦਾ << "module " << moduleName << ";\n";
    ਲਈ (AstParamTypeDType* ਅਚਲ gparam : m_typeParams) {
        AstTypedef* tdefp
            = ਨਵਾਂ AstTypedef{ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::built_ਵਿੱਚ_Filename()}, gparam->name(), ਖਾਲੀ_ਸੂਚਕ,
                             VFlag_ਬੱਚਾ_DType{}, gparam->skipRefp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
        V3EmitV::verilog_For_ਰੁੱਖ(tdefp, *ਦਾ);
        VL_DO_DANGLING(tdefp->delete_ਰੁੱਖ(), tdefp);
    }
    *ਦਾ << "endmodule\n\n";
    *ਦਾ << "`verilator_config\n";
    *ਦਾ << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
ਵਰਗ Hier_ਖੰਡ_Usage_Collect_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    V3_Hier_ਗ੍ਰਾਫ* ਅਚਲ m_graphp = ਨਵਾਂ V3_Hier_ਗ੍ਰਾਫ{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ਅਚਲ AstModule*, V3_Hier_ਖੰਡ*> m_mod2vtx;
    AstModule* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // The current module
    std::ਸਦਿਸ਼<AstVar*> m_params;  // Overridden value parameters of current module
    std::ਸਦਿਸ਼<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::ਸਦਿਸ਼<V3_Hier_ਖੰਡ*> m_childrenp;

    // VISITORSs
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module*) override {}  // Ignore all non-AstModule
    ਖਾਲੀ ਦੇਖੋ(AstModule* nodep) override {
        // Visit each module once
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        ਜੇ (!nodep->hier_ਖੰਡ()) {
            iterateChildrenConst(nodep);
            ਵਾਪਸ;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_ਖੰਡ* ਅਚਲ blockp = ਨਵਾਂ V3_Hier_ਖੰਡ{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        ਲਈ (V3_Hier_ਖੰਡ* ਅਚਲ childp : m_childrenp) ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, blockp, childp, 1};
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ਅਚਲ modp = VN_CAST(nodep->modp(), Module);
        ਜੇ (!modp) ਵਾਪਸ;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        ਜੇ (modp->hier_ਖੰਡ()) m_childrenp.emplace_ਪਿੱਠ(m_mod2vtx.at(modp));
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (!m_modp) ਵਾਪਸ;
        ਜੇ (!m_modp->hier_ਖੰਡ()) ਵਾਪਸ;
        // Can't handle interface port on hier block
        ਜੇ (nodep->isIfaceRef() && !nodep->is_Iface_ਮਾਪਾ()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        ਜੇ (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ਧੱਕੋ_ਪਿੱਠ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        ਜੇ (!m_modp->hier_ਖੰਡ()) ਵਾਪਸ;
        // Record type parameter of this hier block
        m_typeParams.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_Type_ਸਾਰਣੀ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    ਸਪਸ਼ਟ Hier_ਖੰਡ_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_ਕਿਨਾਰੇ_ਜੋੜ(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->ਕ੍ਰਮ();  // This is a bit heavy weight, but does produce a topological ordering
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

ਜਨਤਕ:
    ਸਥਿਰ V3_Hier_ਗ੍ਰਾਫ* apply(AstNetlist* netlistp) {
        ਵਾਪਸ Hier_ਖੰਡ_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_ਗ੍ਰਾਫ::ਸਰੋਤ_Files(ਅਚਲ string& ਸਿਖਰ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().ਮਾਪ() + 1);
    ਲਈ (ਅਚਲ VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_ਪਿੱਠ(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    ਜੇ (!ਸਿਖਰ_Module_File.ਖਾਲੀ_ਥਾਵਾਂ()) sources.emplace_ਪਿੱਠ(ਸਿਖਰ_Module_File);
    ਵਾਪਸ sources;
}

ਖਾਲੀ V3_Hier_ਗ੍ਰਾਫ::write_Command_ਦਲੀਲਾਂ_Files(ਤਾਰਕਿਕ forMkJson) ਅਚਲ {

    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        vtx.as<V3_Hier_ਖੰਡ>()->write_Command_ਦਲੀਲਾਂ_File(forMkJson);
    }
    // For the top module
    ਅਚਲ std::unique_ptr<std::ofstream> ਦਾ{
        V3File::new_ofstream(ਸਿਖਰ_Command_ਦਲੀਲਾਂ_Filename(forMkJson))};
    ਜੇ (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
            *ਦਾ << vtx.as<V3_Hier_ਖੰਡ>()->hierWrapperFilename(ਸੱਚ) << "\n";
        }
    }
    V3HierWriteCommonInputs(ਖਾਲੀ_ਸੂਚਕ, ਦਾ.get(), forMkJson);
    ਜੇ (!forMkJson) {
        ਅਚਲ VString_ਸੈੱਟ& cppFiles = v3Global.opt.cppFiles();
        ਲਈ (ਅਚਲ string& i : cppFiles) *ਦਾ << i << "\n";
        *ਦਾ << "--top-module " << v3Global.rootp()->ਸਿਖਰ_Modulep()->name() << "\n";
        *ਦਾ << "--prefix " << v3Global.opt.ਅਗੇਤਰ() << "\n";
        *ਦਾ << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *ਦਾ << "--mod-prefix " << v3Global.opt.mod_ਅਗੇਤਰ() << "\n";
    }
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        *ਦਾ << vtx.as<V3_Hier_ਖੰਡ>()->hier_ਖੰਡ_ਦਲੀਲਾਂ().front() << "\n";
    }

    ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
        *ਦਾ << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    ਜੇ (v3Global.opt.protect_ਕੁੰਜੀ_Provided()) {
        *ਦਾ << "--protect-key " << v3Global.opt.protect_ਕੁੰਜੀ_Defaulted() << "\n";
    }
    *ਦਾ << "--threads " << cvt_ਮੰਜ਼ਿਲ_Str(v3Global.opt.ਧਾਗੇ()) << "\n";
    *ਦਾ << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *ਦਾ << v3Global.opt.all_ਦਲੀਲਾਂ_String_For_Hier_ਖੰਡ(ਸੱਚ) << "\n";
}

string V3_Hier_ਗ੍ਰਾਫ::ਸਿਖਰ_Command_ਦਲੀਲਾਂ_Filename(ਤਾਰਕਿਕ forMkJson) {
    ਵਾਪਸ V3_Hier_Command_ਦਲੀਲਾਂ_Filename(v3Global.opt.ਅਗੇਤਰ(), forMkJson);
}

ਖਾਲੀ V3_Hier_ਗ੍ਰਾਫ::writeParametersFiles() ਅਚਲ {
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) { vtx.as<V3_Hier_ਖੰਡ>()->writeParametersFile(); }
}

//######################################################################

ਖਾਲੀ V3_ਦਰਜਾਵਾਰ::create_ਗ੍ਰਾਫ(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ਗੰਢ_Module* ਅਚਲ modp = netlistp->ਸਿਖਰ_Modulep();
    ਜੇ (modp->hier_ਖੰਡ()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_ਹੋਰ()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_ਖੰਡ(ਝੂਠ);
    }

    V3_Hier_ਗ੍ਰਾਫ* ਅਚਲ graphp = Hier_ਖੰਡ_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::ਜੋੜੋ_Stat("HierBlock, Hierarchical blocks", graphp->vertices().ਮਾਪ());
    // No hierarchical block is found, nothing to do.
    ਜੇ (graphp->ਖਾਲੀ_ਥਾਵਾਂ()) {
        VL_DO_DANGLING(ਮਿਟਾਓ graphp, graphp);
        ਵਾਪਸ;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
