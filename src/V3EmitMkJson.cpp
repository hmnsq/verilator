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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitMkJson.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HierBlock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

ਵਰਗ V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    ਸਥਿਰ ਖਾਲੀ emitManifest() {
        ਅਚਲ std::string makeDir
            = V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(v3Global.opt.makeDir()));

        V3_ਬਾਹਰਲਾ_Json_File ਦਾ{makeDir + "/" + v3Global.opt.ਅਗੇਤਰ() + ".json"};

        std::ਸਦਿਸ਼<string> classesFast;
        std::ਸਦਿਸ਼<string> classesSlow;
        std::ਸਦਿਸ਼<string> supportFast;
        std::ਸਦਿਸ਼<string> supportSlow;
        std::ਸਦਿਸ਼<string> global;
        std::ਸਦਿਸ਼<string> deps;
        std::ਸਦਿਸ਼<string> cppFiles;

        ਲਈ (Ast_ਗੰਢ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ਗੰਢ_File)) {
            ਅਚਲ AstCFile* ਅਚਲ cfilep = VN_CAST(nodep, CFile);
            ਜੇ (cfilep && cfilep->ਸਰੋਤ()) {
                ਅਚਲ std::string filename
                    = V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(cfilep->name()));
                ਜੇ (cfilep->support()) {
                    ਜੇ (cfilep->slow()) {
                        supportSlow.emplace_ਪਿੱਠ(filename);
                    } ਨਹੀਂਤਾਂ {
                        supportFast.emplace_ਪਿੱਠ(filename);
                    }
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (cfilep->slow()) {
                        classesSlow.emplace_ਪਿੱਠ(filename);
                    } ਨਹੀਂਤਾਂ {
                        classesFast.emplace_ਪਿੱਠ(filename);
                    }
                }
            }
        }

        ਅਚਲ std::string verilator_ਜੜ੍ਹ = V3Os::filename_Slash_ਰਸਤਾ(
            V3Os::filename_Absolute_ਰਸਤਾ(V3Options::getenv_VERILATOR_ਜੜ੍ਹ()));
        ਲਈ (ਅਚਲ string& cpp : v3Global.verilatedCppFiles())
            global.emplace_ਪਿੱਠ(verilator_ਜੜ੍ਹ + "/include/" + cpp);
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            global.emplace_ਪਿੱਠ(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& dep : V3File::getAllDeps())
            deps.emplace_ਪਿੱਠ(V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(dep)));
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_ਪਿੱਠ(V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(cppFile)));

        ਦਾ.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_ਜੜ੍ਹ)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .ਸਿਰਾ()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.ਵਰਤਦਾ_Timing())
            .put("threads", v3Global.opt.ਧਾਗੇ())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .ਸਿਰਾ()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .ਸਿਰਾ();

        ਜੇ (ਅਚਲ V3_Hier_ਗ੍ਰਾਫ* ਅਚਲ graphp = v3Global.hierGraphp()) {
            ਦਾ.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vlstd::ਉਲਟ_view(graphp->vertices())) {
                ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ hblockp = vtx.as<V3_Hier_ਖੰਡ>();
                std::ਸਦਿਸ਼<std::string> hierDeps;
                std::ਸਦਿਸ਼<std::string> sources;
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : hblockp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ dependencyp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Hier_ਖੰਡ>();
                    hierDeps.emplace_ਪਿੱਠ(dependencyp->hier_ਅਗੇਤਰ());
                    sources.emplace_ਪਿੱਠ(makeDir + "/" + dependencyp->hierWrapperFilename(ਸੱਚ));
                }

                ਲਈ (ਅਚਲ string& fname : V3_Hier_ਗ੍ਰਾਫ::ਸਰੋਤ_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_ਪਿੱਠ(
                        V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(fname)));
                }

                ਦਾ.begin()
                    .put("prefix", hblockp->hier_ਅਗੇਤਰ())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_ਅਗੇਤਰ())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(
                                               hblockp->command_ਦਲੀਲਾਂ_Filename(ਸੱਚ))))
                    .ਸਿਰਾ();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::ਸਦਿਸ਼<std::string> sources;
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
                    ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ blockp = vtx.as<V3_Hier_ਖੰਡ>();
                    sources.emplace_ਪਿੱਠ(makeDir + "/" + blockp->hierWrapperFilename(ਸੱਚ));
                }

                ਲਈ (ਅਚਲ string& fname : V3_Hier_ਗ੍ਰਾਫ::ਸਰੋਤ_Files()) {
                    sources.emplace_ਪਿੱਠ(
                        V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(fname)));
                }

                ਦਾ.begin()
                    .put("prefix", v3Global.opt.ਅਗੇਤਰ())
                    .put("top", v3Global.rootp()->ਸਿਖਰ_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_ਰਸਤਾ(V3Os::filename_Absolute_ਰਸਤਾ(
                                               graphp->ਸਿਖਰ_Command_ਦਲੀਲਾਂ_Filename(ਸੱਚ))))
                    .ਸਿਰਾ();
            }

            ਦਾ.ਸਿਰਾ();  // submodules
        }
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ V3EmitMkJsonEmitter() { emitManifest(); }
    ਆਭਾਸੀ ~V3EmitMkJsonEmitter() = ਮੂਲ;
};

ਖਾਲੀ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ਅਚਲ V3EmitMkJsonEmitter emitter;
}
