// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit JSON manifest file
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitMkJson.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"

#ಅನ್ತರ್ಭಾವ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

ವರ್ಗ V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    ಸ್ಥಿರ ಶೂನ್ಯ emitManifest() {
        ಸ್ಥಿರಾಂಕ std::string makeDir
            = V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(v3Global.opt.makeDir()));

        V3_ಹೊರಹೋಗು_Json_File ದ{makeDir + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".json"};

        std::ಸದಿಶ<string> classesFast;
        std::ಸದಿಶ<string> classesSlow;
        std::ಸದಿಶ<string> supportFast;
        std::ಸದಿಶ<string> supportSlow;
        std::ಸದಿಶ<string> global;
        std::ಸದಿಶ<string> deps;
        std::ಸದಿಶ<string> cppFiles;

        ಗಾಗಿ (Ast_ಗಂಟು_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ಗಂಟು_File)) {
            ಸ್ಥಿರಾಂಕ AstCFile* ಸ್ಥಿರಾಂಕ cfilep = VN_CAST(nodep, CFile);
            ಆದರೆ (cfilep && cfilep->ಮೂಲ()) {
                ಸ್ಥಿರಾಂಕ std::string filename
                    = V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(cfilep->name()));
                ಆದರೆ (cfilep->support()) {
                    ಆದರೆ (cfilep->slow()) {
                        supportSlow.emplace_ಹಿಂಭಾಗ(filename);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        supportFast.emplace_ಹಿಂಭಾಗ(filename);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (cfilep->slow()) {
                        classesSlow.emplace_ಹಿಂಭಾಗ(filename);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        classesFast.emplace_ಹಿಂಭಾಗ(filename);
                    }
                }
            }
        }

        ಸ್ಥಿರಾಂಕ std::string verilator_ಬೇರು = V3Os::filename_Slash_ಮಾರ್ಗ(
            V3Os::filename_Absolute_ಮಾರ್ಗ(V3Options::getenv_VERILATOR_ಬೇರು()));
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& cpp : v3Global.verilatedCppFiles())
            global.emplace_ಹಿಂಭಾಗ(verilator_ಬೇರು + "/include/" + cpp);
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            global.emplace_ಹಿಂಭಾಗ(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& dep : V3File::getAllDeps())
            deps.emplace_ಹಿಂಭಾಗ(V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(dep)));
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_ಹಿಂಭಾಗ(V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(cppFile)));

        ದ.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_ಬೇರು)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .ಕೊನೆ()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.ಬಳಸುತ್ತದೆ_Timing())
            .put("threads", v3Global.opt.ಎಳೆಗಳು())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .ಕೊನೆ()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .ಕೊನೆ();

        ಆದರೆ (ಸ್ಥಿರಾಂಕ V3_Hier_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ graphp = v3Global.hierGraphp()) {
            ದ.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vlstd::ಹಿಮ್ಮುಖ_view(graphp->vertices())) {
                ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ hblockp = vtx.as<V3_Hier_ಖಂಡ>();
                std::ಸದಿಶ<std::string> hierDeps;
                std::ಸದಿಶ<std::string> sources;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : hblockp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ dependencyp = ಅಂಚು.ತುದಿ()->as<V3_Hier_ಖಂಡ>();
                    hierDeps.emplace_ಹಿಂಭಾಗ(dependencyp->hier_ಪೂರ್ವಪ್ರತ್ಯಯ());
                    sources.emplace_ಹಿಂಭಾಗ(makeDir + "/" + dependencyp->hierWrapperFilename(ನಿಜ));
                }

                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& fname : V3_Hier_ನಕ್ಷೆ::ಮೂಲ_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_ಹಿಂಭಾಗ(
                        V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(fname)));
                }

                ದ.begin()
                    .put("prefix", hblockp->hier_ಪೂರ್ವಪ್ರತ್ಯಯ())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_ಪೂರ್ವಪ್ರತ್ಯಯ())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(
                                               hblockp->command_ವಾದಗಳು_Filename(ನಿಜ))))
                    .ಕೊನೆ();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::ಸದಿಶ<std::string> sources;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
                    ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = vtx.as<V3_Hier_ಖಂಡ>();
                    sources.emplace_ಹಿಂಭಾಗ(makeDir + "/" + blockp->hierWrapperFilename(ನಿಜ));
                }

                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& fname : V3_Hier_ನಕ್ಷೆ::ಮೂಲ_Files()) {
                    sources.emplace_ಹಿಂಭಾಗ(
                        V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(fname)));
                }

                ದ.begin()
                    .put("prefix", v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ())
                    .put("top", v3Global.rootp()->ತುದಿ_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_ಮಾರ್ಗ(V3Os::filename_Absolute_ಮಾರ್ಗ(
                                               graphp->ತುದಿ_Command_ವಾದಗಳು_Filename(ನಿಜ))))
                    .ಕೊನೆ();
            }

            ದ.ಕೊನೆ();  // submodules
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ V3EmitMkJsonEmitter() { emitManifest(); }
    ವಾಸ್ತವಿಕ ~V3EmitMkJsonEmitter() = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ಸ್ಥಿರಾಂಕ V3EmitMkJsonEmitter emitter;
}
