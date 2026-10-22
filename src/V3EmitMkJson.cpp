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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3EmitMkJson.h"

#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3HierBlock.h"
#அந்தர்பாவ "V3Os.h"

#அந்தர்பாவ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

வகுப்பு V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    நிலையான வெறுமை emitManifest() {
        மாறிலி std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        க்கு (Ast_முனை_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), முனை_File)) {
            மாறிலி AstCFile* மாறிலி cfilep = VN_CAST(nodep, CFile);
            எனில் (cfilep && cfilep->source()) {
                மாறிலி std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                எனில் (cfilep->support()) {
                    எனில் (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } இல்லையெனில் {
                        supportFast.emplace_back(filename);
                    }
                } இல்லையெனில் {
                    எனில் (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } இல்லையெனில் {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        மாறிலி std::string verilator_வேர் = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_வேர்()));
        க்கு (மாறிலி string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_வேர் + "/include/" + cpp);
        எனில் (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        க்கு (மாறிலி auto& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        க்கு (மாறிலி auto& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_வேர்)
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

        எனில் (மாறிலி V3HierGraph* மாறிலி graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            க்கு (மாறிலி V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                மாறிலி V3HierBlock* மாறிலி hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                க்கு (மாறிலி V3GraphEdge& edge : hblockp->outEdges()) {
                    மாறிலி V3HierBlock* மாறிலி dependencyp = edge.உச்சி()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(உண்மை));
                }

                க்கு (மாறிலி string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
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
                                               hblockp->commandArgsFilename(உண்மை))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                க்கு (மாறிலி V3GraphVertex& vtx : graphp->vertices()) {
                    மாறிலி V3HierBlock* மாறிலி blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(உண்மை));
                }

                க்கு (மாறிலி string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", v3Global.opt.prefix())
                    .put("top", v3Global.rootp()->உச்சி_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->உச்சி_Command_Args_Filename(உண்மை))))
                    .end();
            }

            of.end();  // submodules
        }
    }

பொது:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    மெய்நிகர் ~V3EmitMkJsonEmitter() = இயல்பு;
};

வெறுமை V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    மாறிலி V3EmitMkJsonEmitter emitter;
}
