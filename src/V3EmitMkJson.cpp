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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3EmitMkJson.h"

#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3HierBlock.h"
#അന്തര്ഭാവ "V3Os.h"

#അന്തര്ഭാവ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

വർഗ്ഗം V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    സ്ഥിതം ശൂന്യം emitManifest() {
        സ്ഥിരം std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        വേണ്ടി (Ast_കണ്ണി_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), കണ്ണി_File)) {
            സ്ഥിരം AstCFile* സ്ഥിരം cfilep = VN_CAST(nodep, CFile);
            എങ്കിൽ (cfilep && cfilep->source()) {
                സ്ഥിരം std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                എങ്കിൽ (cfilep->support()) {
                    എങ്കിൽ (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } അല്ലെങ്കിൽ {
                        supportFast.emplace_back(filename);
                    }
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } അല്ലെങ്കിൽ {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        സ്ഥിരം std::string verilator_വേര് = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_വേര്()));
        വേണ്ടി (സ്ഥിരം string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_വേര് + "/include/" + cpp);
        എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        വേണ്ടി (സ്ഥിരം auto& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        വേണ്ടി (സ്ഥിരം auto& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_വേര്)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .end()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.usesTiming())
            .put("threads", v3Global.opt.threads())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .end()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .end();

        എങ്കിൽ (സ്ഥിരം V3HierGraph* സ്ഥിരം graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                സ്ഥിരം V3HierBlock* സ്ഥിരം hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : hblockp->outEdges()) {
                    സ്ഥിരം V3HierBlock* സ്ഥിരം dependencyp = edge.മുകൾ()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(സത്യം));
                }

                വേണ്ടി (സ്ഥിരം string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", hblockp->hierPrefix())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hierPrefix())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               hblockp->commandArgsFilename(സത്യം))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : graphp->vertices()) {
                    സ്ഥിരം V3HierBlock* സ്ഥിരം blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(സത്യം));
                }

                വേണ്ടി (സ്ഥിരം string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", v3Global.opt.prefix())
                    .put("top", v3Global.rootp()->മുകൾ_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->മുകൾ_Command_Args_Filename(സത്യം))))
                    .end();
            }

            of.end();  // submodules
        }
    }

പൊതു:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    മായാ ~V3EmitMkJsonEmitter() = സ്ഥിരസ്ഥിതി;
};

ശൂന്യം V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    സ്ഥിരം V3EmitMkJsonEmitter emitter;
}
