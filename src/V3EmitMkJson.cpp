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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3EmitMkJson.h"

#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3HierBlock.h"
#અન્તર્ભાવ "V3Os.h"

#અન્તર્ભાવ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

વર્ગ V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    સ્થિર શૂન્ય emitManifest() {
        અચલ std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        માટે (Ast_ગાંઠ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ગાંઠ_File)) {
            અચલ AstCFile* અચલ cfilep = VN_CAST(nodep, CFile);
            જો (cfilep && cfilep->source()) {
                અચલ std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                જો (cfilep->support()) {
                    જો (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } નહીંતર {
                        supportFast.emplace_back(filename);
                    }
                } નહીંતર {
                    જો (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } નહીંતર {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        અચલ std::string verilator_મૂળ = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_મૂળ()));
        માટે (અચલ string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_મૂળ + "/include/" + cpp);
        જો (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        માટે (અચલ auto& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        માટે (અચલ auto& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_મૂળ)
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

        જો (અચલ V3HierGraph* અચલ graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            માટે (અચલ V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                અચલ V3HierBlock* અચલ hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                માટે (અચલ V3GraphEdge& edge : hblockp->outEdges()) {
                    અચલ V3HierBlock* અચલ dependencyp = edge.ટોચ()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(સાચું));
                }

                માટે (અચલ string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
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
                                               hblockp->commandArgsFilename(સાચું))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                માટે (અચલ V3GraphVertex& vtx : graphp->vertices()) {
                    અચલ V3HierBlock* અચલ blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(સાચું));
                }

                માટે (અચલ string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", v3Global.opt.prefix())
                    .put("top", v3Global.rootp()->ટોચ_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->ટોચ_Command_Args_Filename(સાચું))))
                    .end();
            }

            of.end();  // submodules
        }
    }

જાહેર:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    આભાસી ~V3EmitMkJsonEmitter() = મૂળભૂત;
};

શૂન્ય V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    અચલ V3EmitMkJsonEmitter emitter;
}
