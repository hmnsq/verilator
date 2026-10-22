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
        ಧ್ರುವ std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        ಪ್ರತಿ (Ast_ಗಂಟು_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ಗಂಟು_File)) {
            ಧ್ರುವ AstCFile* ಧ್ರುವ cfilep = VN_CAST(nodep, CFile);
            ಯದಿ (cfilep && cfilep->source()) {
                ಧ್ರುವ std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                ಯದಿ (cfilep->support()) {
                    ಯದಿ (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } ಅನ್ಯಥಾ {
                        supportFast.emplace_back(filename);
                    }
                } ಅನ್ಯಥಾ {
                    ಯದಿ (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } ಅನ್ಯಥಾ {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        ಧ್ರುವ std::string verilator_ಬೇರು = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_ಬೇರು()));
        ಪ್ರತಿ (ಧ್ರುವ string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_ಬೇರು + "/include/" + cpp);
        ಯದಿ (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        ಪ್ರತಿ (ಧ್ರುವ auto& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        ಪ್ರತಿ (ಧ್ರುವ auto& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_ಬೇರು)
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

        ಯದಿ (ಧ್ರುವ V3HierGraph* ಧ್ರುವ graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                ಧ್ರುವ V3HierBlock* ಧ್ರುವ hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : hblockp->outEdges()) {
                    ಧ್ರುವ V3HierBlock* ಧ್ರುವ dependencyp = edge.ತುದಿ()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(ಸತ್ಯ));
                }

                ಪ್ರತಿ (ಧ್ರುವ string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
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
                                               hblockp->commandArgsFilename(ಸತ್ಯ))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : graphp->vertices()) {
                    ಧ್ರುವ V3HierBlock* ಧ್ರುವ blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(ಸತ್ಯ));
                }

                ಪ್ರತಿ (ಧ್ರುವ string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", v3Global.opt.prefix())
                    .put("top", v3Global.rootp()->ತುದಿ_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->ತುದಿ_Command_Args_Filename(ಸತ್ಯ))))
                    .end();
            }

            of.end();  // submodules
        }
    }

ಸಾರ್ವಜನಿಕ:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    ಆಭಾಸ ~V3EmitMkJsonEmitter() = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ಧ್ರುವ V3EmitMkJsonEmitter emitter;
}
