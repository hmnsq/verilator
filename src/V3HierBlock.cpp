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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ಸ್ಥಿರ string V3_Hier_Command_ವಾದಗಳು_Filename(ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ, ತಾರ್ಕಿಕ forMkJson) {
    ಹಿಂತಿರುಗಿ v3Global.opt.makeDir() + "/" + ಪೂರ್ವಪ್ರತ್ಯಯ
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

ಸ್ಥಿರ string V3HierParametersFileName(ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
    ಹಿಂತಿರುಗಿ v3Global.opt.makeDir() + "/" + ಪೂರ್ವಪ್ರತ್ಯಯ + "__hierParameters.v";
}

ಸ್ಥಿರ ಶೂನ್ಯ V3HierWriteCommonInputs(ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* hblockp, std::ostream* ದ, ತಾರ್ಕಿಕ forMkJson) {
    ಸ್ಥಿರಾಂಕ string ತುದಿ_Module_File = hblockp ? hblockp->vFileIfNecessary() : "";
    ಆದರೆ (!forMkJson) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& filename : V3_Hier_ನಕ್ಷೆ::ಮೂಲ_Files(ತುದಿ_Module_File))
            *ದ << filename << "\n";
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : v3Global.opt.libraryFiles()) {
        ಆದರೆ (V3Os::filename_Real_ಮಾರ್ಗ(i.filename()) != ತುದಿ_Module_File)
            *ದ << "-v " << i.filename() << "\n";
    }
}

//######################################################################

ತಾರ್ಕಿಕ V3_Hier_ಖಂಡ::stringParamPassable(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    ಆದರೆ (ಮೌಲ್ಯ.ಕಂಡುಹಿಡಿ_ಮೊದಲ_ದ("\n\"") != string::npos) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (ಮೌಲ್ಯ.ಕಂಡುಹಿಡಿ("/*") != string::npos) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಗಾತ್ರ_t pos = ಮೌಲ್ಯ.ಕಂಡುಹಿಡಿ("//"); pos != string::npos; pos = ಮೌಲ್ಯ.ಕಂಡುಹಿಡಿ("//", pos + 1)) {
        ಆದರೆ (pos > 0 && std::ಖಾಲಿಯೇ(static_cast<ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ>(ಮೌಲ್ಯ[pos - 1]))) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

V3_Hier_ಖಂಡ::StrGParams V3_Hier_ಖಂಡ::stringifyParams(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& gparams,
                                                     ತಾರ್ಕಿಕ forGOption) {
    StrGParams strParams;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ gparam : gparams) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            ಆದರೆ (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::ಸದಿಶ<ಅಕ್ಷರ> hexFpStr(32, '\0');
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ len = VL_SNPRINTF(hexFpStr.ದತ್ತಾಂಶ(), hexFpStr.ಗಾತ್ರ_2(), "%a",
                                            constp->ಅಂಕೆ().ಗಮ್ಯ_Double());
                UASSERT_OBJ(0 < len && static_cast<ಗಾತ್ರ_t>(len) < hexFpStr.ಗಾತ್ರ_2(), constp,
                            " is not properly converted to string");
                s = hexFpStr.ದತ್ತಾಂಶ();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->isString()) {
                s = constp->ಅಂಕೆ().ಗಮ್ಯ_String();
                ಆದರೆ (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } ಇಲ್ಲದಿದ್ದರೆ {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp->ಅಂಕೆ()};
                ಅಂಕೆ.isSigned(constp->isSigned());
                s = ಅಂಕೆ.ascii(ನಿಜ, ನಿಜ);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_ಹಿಂಭಾಗ(gparam->name(), s);
        }
    }
    ಹಿಂತಿರುಗಿ strParams;
}

VStringList V3_Hier_ಖಂಡ::command_ವಾದಗಳು(ತಾರ್ಕಿಕ forMkJson) ಸ್ಥಿರಾಂಕ {
    VStringList opts;
    ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ = hier_ಪೂರ್ವಪ್ರತ್ಯಯ();
    ಆದರೆ (!forMkJson) {
        opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --prefix " + ಪೂರ್ವಪ್ರತ್ಯಯ);
        opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --mod-prefix " + ಪೂರ್ವಪ್ರತ್ಯಯ);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --top-module-encoded " + modp()->name());
    }
    opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --lib-create " + modp()->name());  // possibly mangled name
    ಆದರೆ (v3Global.opt.protect_ಕೀಲಿ_Provided())
        opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --protect-key " + v3Global.opt.protect_ಕೀಲಿ_Defaulted());
    opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --hierarchical-child " + cvt_ಗಮ್ಯ_Str(v3Global.opt.ಎಳೆಗಳು()));

    ಸ್ಥಿರಾಂಕ StrGParams gparamsStr = stringifyParams(m_params, ನಿಜ);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ StrGParam& param : gparamsStr) {
        opts.ತಳ್ಳು_ಹಿಂಭಾಗ("-G" + param.ಮೊದಲ + "=" + param.ಎರಡನೇ + "");
    }
    ಆದರೆ (!m_typeParams.ಖಾಲಿಗಳು()) {
        opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --hierarchical-params-file " + typeParametersFilename());
    }

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಖಂಡ_ಎಳೆಗಳು = V3Control::getHierWorkers(m_modp->origName());
    ಆದರೆ (ಖಂಡ_ಎಳೆಗಳು > 1) {
        ಆದರೆ (!ಒಳ_ಖಾಲಿಗಳು()) {
            V3Control::get_Hier_Workers_File_ಸಾಲು(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() < ಖಂಡ_ಎಳೆಗಳು) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.ಎಳೆಗಳು()
                                << " hierarchical block threads = " << ಖಂಡ_ಎಳೆಗಳು);
            }

            opts.ತಳ್ಳು_ಹಿಂಭಾಗ(" --threads " + std::ಗಮ್ಯ_string(ಖಂಡ_ಎಳೆಗಳು));
        }
    }

    ಹಿಂತಿರುಗಿ opts;
}

VStringList V3_Hier_ಖಂಡ::hier_ಖಂಡ_ವಾದಗಳು() ಸ್ಥಿರಾಂಕ {
    VStringList opts;
    ಸ್ಥಿರಾಂಕ StrGParams gparamsStr = stringifyParams(m_params, ಸುಳ್ಳು);
    opts.emplace_ಹಿಂಭಾಗ("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ StrGParam& pair : gparamsStr) {
        s += "," + pair.ಮೊದಲ;
        s += "," + pair.ಎರಡನೇ;
    }
    opts.ಹಿಂಭಾಗ() += s;
    ಹಿಂತಿರುಗಿ opts;
}

string V3_Hier_ಖಂಡ::hier_ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "V" + modp()->name(); }

string V3_Hier_ಖಂಡ::hierSomeFilename(ತಾರ್ಕಿಕ withDir, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* suffix) ಸ್ಥಿರಾಂಕ {
    string s;
    ಆದರೆ (withDir) s = hier_ಪೂರ್ವಪ್ರತ್ಯಯ() + '/';
    s += ಪೂರ್ವಪ್ರತ್ಯಯ + modp()->name() + suffix;
    ಹಿಂತಿರುಗಿ s;
}

string V3_Hier_ಖಂಡ::hierWrapperFilename(ತಾರ್ಕಿಕ withDir) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ hierSomeFilename(withDir, "", ".sv");
}

string V3_Hier_ಖಂಡ::hierMkFilename(ತಾರ್ಕಿಕ withDir) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ hierSomeFilename(withDir, "V", ".mk");
}

string V3_Hier_ಖಂಡ::hierLibFilename(ತಾರ್ಕಿಕ withDir) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ hierSomeFilename(withDir, "lib", ".a");
}

string V3_Hier_ಖಂಡ::hierGeneratedFilenames(ತಾರ್ಕಿಕ withDir) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3_Hier_ಖಂಡ::vFileIfNecessary() ಸ್ಥಿರಾಂಕ {
    string filename = V3Os::filename_Real_ಮಾರ್ಗ(m_modp->fileline()->filename());
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        ಆದರೆ (filename == V3Os::filename_Real_ಮಾರ್ಗ(v.filename())) ಹಿಂತಿರುಗಿ "";
    }
    ಹಿಂತಿರುಗಿ filename;
}

ಶೂನ್ಯ V3_Hier_ಖಂಡ::write_Command_ವಾದಗಳು_File(ತಾರ್ಕಿಕ forMkJson) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ದ{V3File::new_ofstream(command_ವಾದಗಳು_Filename(forMkJson))};
    *ದ << "--cc\n";

    ಆದರೆ (!forMkJson) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ dependencyp = ಅಂಚು.ತುದಿ()->as<V3_Hier_ಖಂಡ>();
            *ದ << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(ನಿಜ) << "\n";
        }
        *ದ << "-Mdir " << v3Global.opt.makeDir() << "/" << hier_ಪೂರ್ವಪ್ರತ್ಯಯ() << " \n";
    }
    V3HierWriteCommonInputs(ಇದು, ದ.get(), forMkJson);
    ಸ್ಥಿರಾಂಕ VStringList& commandOpts = command_ವಾದಗಳು(ಸುಳ್ಳು);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& opt : commandOpts) *ದ << opt << "\n";
    *ದ << hier_ಖಂಡ_ವಾದಗಳು().front() << "\n";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ dependencyp = ಅಂಚು.ತುದಿ()->as<V3_Hier_ಖಂಡ>();
        *ದ << dependencyp->hier_ಖಂಡ_ವಾದಗಳು().front() << "\n";
    }
    *ದ << v3Global.opt.all_ವಾದಗಳು_String_For_Hier_ಖಂಡ(ಸುಳ್ಳು) << "\n";
}

string V3_Hier_ಖಂಡ::command_ವಾದಗಳು_Filename(ತಾರ್ಕಿಕ forMkJson) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ V3_Hier_Command_ವಾದಗಳು_Filename(hier_ಪೂರ್ವಪ್ರತ್ಯಯ(), forMkJson);
}

string V3_Hier_ಖಂಡ::typeParametersFilename() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ V3HierParametersFileName(hier_ಪೂರ್ವಪ್ರತ್ಯಯ());
}

ಶೂನ್ಯ V3_Hier_ಖಂಡ::writeParametersFile() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (m_typeParams.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

    VHashSha512 hash{"type params"};
    ಸ್ಥಿರಾಂಕ string moduleName = "Vhsh" + hash.digestSymbol24();
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ದ{V3File::new_ofstream(typeParametersFilename())};
    *ದ << "module " << moduleName << ";\n";
    ಗಾಗಿ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ gparam : m_typeParams) {
        AstTypedef* tdefp
            = ಹೊಸ AstTypedef{ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::built_ಒಳ_Filename()}, gparam->name(), ಶೂನ್ಯಸೂಚಕ,
                             VFlag_ಮಗು_DType{}, gparam->skipRefp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)};
        V3EmitV::verilog_For_ವೃಕ್ಷ(tdefp, *ದ);
        VL_DO_DANGLING(tdefp->delete_ವೃಕ್ಷ(), tdefp);
    }
    *ದ << "endmodule\n\n";
    *ದ << "`verilator_config\n";
    *ದ << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
ವರ್ಗ Hier_ಖಂಡ_Usage_Collect_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    V3_Hier_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_graphp = ಹೊಸ V3_Hier_ನಕ್ಷೆ{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstModule*, V3_Hier_ಖಂಡ*> m_mod2vtx;
    AstModule* m_modp = ಶೂನ್ಯಸೂಚಕ;  // The current module
    std::ಸದಿಶ<AstVar*> m_params;  // Overridden value parameters of current module
    std::ಸದಿಶ<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::ಸದಿಶ<V3_Hier_ಖಂಡ*> m_childrenp;

    // VISITORSs
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module*) override {}  // Ignore all non-AstModule
    ಶೂನ್ಯ ಭೇಟಿ(AstModule* nodep) override {
        // Visit each module once
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        ಆದರೆ (!nodep->hier_ಖಂಡ()) {
            iterateChildrenConst(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = ಹೊಸ V3_Hier_ಖಂಡ{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        ಗಾಗಿ (V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ childp : m_childrenp) ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, blockp, childp, 1};
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* ಸ್ಥಿರಾಂಕ modp = VN_CAST(nodep->modp(), Module);
        ಆದರೆ (!modp) ಹಿಂತಿರುಗಿ;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        ಆದರೆ (modp->hier_ಖಂಡ()) m_childrenp.emplace_ಹಿಂಭಾಗ(m_mod2vtx.at(modp));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (!m_modp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_modp->hier_ಖಂಡ()) ಹಿಂತಿರುಗಿ;
        // Can't handle interface port on hier block
        ಆದರೆ (nodep->isIfaceRef() && !nodep->is_Iface_ಪೋಷಕ()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        ಆದರೆ (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        ಆದರೆ (!m_modp->hier_ಖಂಡ()) ಹಿಂತಿರುಗಿ;
        // Record type parameter of this hier block
        m_typeParams.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Type_ಕೋಷ್ಟಕ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    ಸ್ಪಷ್ಟ Hier_ಖಂಡ_Usage_Collect_Visitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->remove_Redundant_ಅಂಚುಗಳು_ಮೊತ್ತ(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->ಕ್ರಮ();  // This is a bit heavy weight, but does produce a topological ordering
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3_Hier_ನಕ್ಷೆ* apply(AstNetlist* netlistp) {
        ಹಿಂತಿರುಗಿ Hier_ಖಂಡ_Usage_Collect_Visitor{netlistp}.m_graphp;
    }
};

VStringList V3_Hier_ನಕ್ಷೆ::ಮೂಲ_Files(ಸ್ಥಿರಾಂಕ string& ತುದಿ_Module_File) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().ಗಾತ್ರ_2() + 1);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_ಹಿಂಭಾಗ(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    ಆದರೆ (!ತುದಿ_Module_File.ಖಾಲಿಗಳು()) sources.emplace_ಹಿಂಭಾಗ(ತುದಿ_Module_File);
    ಹಿಂತಿರುಗಿ sources;
}

ಶೂನ್ಯ V3_Hier_ನಕ್ಷೆ::write_Command_ವಾದಗಳು_Files(ತಾರ್ಕಿಕ forMkJson) ಸ್ಥಿರಾಂಕ {

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        vtx.as<V3_Hier_ಖಂಡ>()->write_Command_ವಾದಗಳು_File(forMkJson);
    }
    // For the top module
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ದ{
        V3File::new_ofstream(ತುದಿ_Command_ವಾದಗಳು_Filename(forMkJson))};
    ಆದರೆ (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
            *ದ << vtx.as<V3_Hier_ಖಂಡ>()->hierWrapperFilename(ನಿಜ) << "\n";
        }
    }
    V3HierWriteCommonInputs(ಶೂನ್ಯಸೂಚಕ, ದ.get(), forMkJson);
    ಆದರೆ (!forMkJson) {
        ಸ್ಥಿರಾಂಕ VString_ಗಣ& cppFiles = v3Global.opt.cppFiles();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : cppFiles) *ದ << i << "\n";
        *ದ << "--top-module " << v3Global.rootp()->ತುದಿ_Modulep()->name() << "\n";
        *ದ << "--prefix " << v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() << "\n";
        *ದ << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *ದ << "--mod-prefix " << v3Global.opt.mod_ಪೂರ್ವಪ್ರತ್ಯಯ() << "\n";
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        *ದ << vtx.as<V3_Hier_ಖಂಡ>()->hier_ಖಂಡ_ವಾದಗಳು().front() << "\n";
    }

    ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
        *ದ << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    ಆದರೆ (v3Global.opt.protect_ಕೀಲಿ_Provided()) {
        *ದ << "--protect-key " << v3Global.opt.protect_ಕೀಲಿ_Defaulted() << "\n";
    }
    *ದ << "--threads " << cvt_ಗಮ್ಯ_Str(v3Global.opt.ಎಳೆಗಳು()) << "\n";
    *ದ << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *ದ << v3Global.opt.all_ವಾದಗಳು_String_For_Hier_ಖಂಡ(ನಿಜ) << "\n";
}

string V3_Hier_ನಕ್ಷೆ::ತುದಿ_Command_ವಾದಗಳು_Filename(ತಾರ್ಕಿಕ forMkJson) {
    ಹಿಂತಿರುಗಿ V3_Hier_Command_ವಾದಗಳು_Filename(v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ(), forMkJson);
}

ಶೂನ್ಯ V3_Hier_ನಕ್ಷೆ::writeParametersFiles() ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) { vtx.as<V3_Hier_ಖಂಡ>()->writeParametersFile(); }
}

//######################################################################

ಶೂನ್ಯ V3_ಶ್ರೇಣೀಕೃತ::create_ನಕ್ಷೆ(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = netlistp->ತುದಿ_Modulep();
    ಆದರೆ (modp->hier_ಖಂಡ()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warn_ಇನ್ನಷ್ಟು()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hier_ಖಂಡ(ಸುಳ್ಳು);
    }

    V3_Hier_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ graphp = Hier_ಖಂಡ_Usage_Collect_Visitor::apply(netlistp);
    V3Stats::ಕೂಡಿಸು_Stat("HierBlock, Hierarchical blocks", graphp->vertices().ಗಾತ್ರ_2());
    // No hierarchical block is found, nothing to do.
    ಆದರೆ (graphp->ಖಾಲಿಗಳು()) {
        VL_DO_DANGLING(ಅಳಿಸು graphp, graphp);
        ಹಿಂತಿರುಗಿ;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
