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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3EmitMkJson.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3HierBlock.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"

#ଅନ୍ତର୍ଭାଵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

ଶ୍ରେଣୀ V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    ସ୍ଥିର ଶୂନ୍ୟ emitManifest() {
        ଅଚଳ std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        ପାଇଁ (Ast_ଗଣ୍ଠି_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ଗଣ୍ଠି_File)) {
            ଅଚଳ AstCFile* ଅଚଳ cfilep = VN_CAST(nodep, CFile);
            ଯଦି (cfilep && cfilep->source()) {
                ଅଚଳ std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                ଯଦି (cfilep->support()) {
                    ଯଦି (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } ଅନ୍ୟଥା {
                        supportFast.emplace_back(filename);
                    }
                } ଅନ୍ୟଥା {
                    ଯଦି (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } ଅନ୍ୟଥା {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        ଅଚଳ std::string verilator_ମୂଳ = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_ମୂଳ()));
        ପାଇଁ (ଅଚଳ string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_ମୂଳ + "/include/" + cpp);
        ଯଦି (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        ପାଇଁ (ଅଚଳ auto& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        ପାଇଁ (ଅଚଳ auto& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_ମୂଳ)
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

        ଯଦି (ଅଚଳ V3HierGraph* ଅଚଳ graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                ଅଚଳ V3HierBlock* ଅଚଳ hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                ପାଇଁ (ଅଚଳ V3GraphEdge& edge : hblockp->outEdges()) {
                    ଅଚଳ V3HierBlock* ଅଚଳ dependencyp = edge.ଶୀର୍ଷ()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(ସତ୍ୟ));
                }

                ପାଇଁ (ଅଚଳ string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
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
                                               hblockp->commandArgsFilename(ସତ୍ୟ))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : graphp->vertices()) {
                    ଅଚଳ V3HierBlock* ଅଚଳ blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(ସତ୍ୟ));
                }

                ପାଇଁ (ଅଚଳ string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", v3Global.opt.prefix())
                    .put("top", v3Global.rootp()->ଶୀର୍ଷ_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->ଶୀର୍ଷ_Command_Args_Filename(ସତ୍ୟ))))
                    .end();
            }

            of.end();  // submodules
        }
    }

ସର୍ବସାଧାରଣ:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    ଆଭାସୀ ~V3EmitMkJsonEmitter() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ଅଚଳ V3EmitMkJsonEmitter emitter;
}
