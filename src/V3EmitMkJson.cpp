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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3EmitMkJson.h"

#समावेश "V3EmitCBase.h"
#समावेश "V3HierBlock.h"
#समावेश "V3Os.h"

#समावेश <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

श्रेणी V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    अटल व्योम emitManifest() {
        स्थिर std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        क्रम (Ast_गाँठ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), गाँठ_File)) {
            स्थिर AstCFile* स्थिर cfilep = VN_CAST(nodep, CFile);
            अगर (cfilep && cfilep->source()) {
                स्थिर std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                अगर (cfilep->support()) {
                    अगर (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } अन्यथा {
                        supportFast.emplace_back(filename);
                    }
                } अन्यथा {
                    अगर (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } अन्यथा {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        स्थिर std::string verilator_मूल = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_मूल()));
        क्रम (स्थिर string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_मूल + "/include/" + cpp);
        अगर (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        क्रम (स्थिर स्वतः& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        क्रम (स्थिर स्वतः& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.दो("version", 1)
            .begin("system")
            .दो("perl", V3Options::getenvPERL())
            .दो("python3", V3Options::getenvPYTHON3())
            .दो("verilator_root", verilator_मूल)
            .दो("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .end()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .दो("system_c", v3Global.opt.systemC())
            .दो("coverage", v3Global.opt.coverage())
            .दो("dpi", v3Global.dpi())
            .दो("use_timing", v3Global.usesTiming())
            .दो("threads", v3Global.opt.threads())
            .दो("trace", v3Global.opt.trace())
            .दो("trace_fst", v3Global.opt.traceEnabledFst())
            .दो("trace_saif", v3Global.opt.traceEnabledSaif())
            .दो("trace_vcd", v3Global.opt.traceEnabledVcd())
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

        अगर (स्थिर V3HierGraph* स्थिर graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            क्रम (स्थिर V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                स्थिर V3HierBlock* स्थिर hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                क्रम (स्थिर V3GraphEdge& edge : hblockp->outEdges()) {
                    स्थिर V3HierBlock* स्थिर dependencyp = edge.शीर्ष()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(true));
                }

                क्रम (स्थिर string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .दो("prefix", hblockp->hierPrefix())
                    .दो("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .दो("directory", makeDir + "/" + hblockp->hierPrefix())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .दो("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               hblockp->commandArgsFilename(true))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                क्रम (स्थिर V3GraphVertex& vtx : graphp->vertices()) {
                    स्थिर V3HierBlock* स्थिर blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(true));
                }

                क्रम (स्थिर string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .दो("prefix", v3Global.opt.prefix())
                    .दो("top", v3Global.rootp()->शीर्ष_Modulep()->name())
                    .दो("directory", makeDir)
                    .putList("sources", sources)
                    .दो("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->शीर्ष_Command_Args_Filename(true))))
                    .end();
            }

            of.end();  // submodules
        }
    }

खुला:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    भव ~V3EmitMkJsonEmitter() = शेष;
};

व्योम V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    स्थिर V3EmitMkJsonEmitter emitter;
}
