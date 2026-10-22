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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitMkJson.h"

#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3HierBlock.h"
#अन्तर्भाव "V3Os.h"

#अन्तर्भाव <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

वर्ग V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    स्थिर शून्य emitManifest() {
        ध्रुव std::string makeDir
            = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(v3Global.opt.makeDir()));

        V3OutJsonFile of{makeDir + "/" + v3Global.opt.prefix() + ".json"};

        std::vector<string> classesFast;
        std::vector<string> classesSlow;
        std::vector<string> supportFast;
        std::vector<string> supportSlow;
        std::vector<string> global;
        std::vector<string> deps;
        std::vector<string> cppFiles;

        प्रति (Ast_गाँठ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), गाँठ_File)) {
            ध्रुव AstCFile* ध्रुव cfilep = VN_CAST(nodep, CFile);
            यदि (cfilep && cfilep->source()) {
                ध्रुव std::string filename
                    = V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cfilep->name()));
                यदि (cfilep->support()) {
                    यदि (cfilep->slow()) {
                        supportSlow.emplace_back(filename);
                    } अन्यथा {
                        supportFast.emplace_back(filename);
                    }
                } अन्यथा {
                    यदि (cfilep->slow()) {
                        classesSlow.emplace_back(filename);
                    } अन्यथा {
                        classesFast.emplace_back(filename);
                    }
                }
            }
        }

        ध्रुव std::string verilator_मूल = V3Os::filenameSlashPath(
            V3Os::filenameAbsolutePath(V3Options::getenv_VERILATOR_मूल()));
        प्रति (ध्रुव string& cpp : v3Global.verilatedCppFiles())
            global.emplace_back(verilator_मूल + "/include/" + cpp);
        यदि (!v3Global.opt.libCreate().empty()) {
            global.emplace_back(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        प्रति (ध्रुव स्वतः& dep : V3File::getAllDeps())
            deps.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(dep)));
        प्रति (ध्रुव स्वतः& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_back(V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(cppFile)));

        of.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_मूल)
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

        यदि (ध्रुव V3HierGraph* ध्रुव graphp = v3Global.hierGraphp()) {
            of.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            प्रति (ध्रुव V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                ध्रुव V3HierBlock* ध्रुव hblockp = vtx.as<V3HierBlock>();
                std::vector<std::string> hierDeps;
                std::vector<std::string> sources;
                प्रति (ध्रुव V3GraphEdge& edge : hblockp->outEdges()) {
                    ध्रुव V3HierBlock* ध्रुव dependencyp = edge.शीर्ष()->as<V3HierBlock>();
                    hierDeps.emplace_back(dependencyp->hierPrefix());
                    sources.emplace_back(makeDir + "/" + dependencyp->hierWrapperFilename(सत्य));
                }

                प्रति (ध्रुव string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
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
                                               hblockp->commandArgsFilename(सत्य))))
                    .end();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::vector<std::string> sources;
                प्रति (ध्रुव V3GraphVertex& vtx : graphp->vertices()) {
                    ध्रुव V3HierBlock* ध्रुव blockp = vtx.as<V3HierBlock>();
                    sources.emplace_back(makeDir + "/" + blockp->hierWrapperFilename(सत्य));
                }

                प्रति (ध्रुव string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_back(
                        V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(fname)));
                }

                of.begin()
                    .put("prefix", v3Global.opt.prefix())
                    .put("top", v3Global.rootp()->शीर्ष_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filenameSlashPath(V3Os::filenameAbsolutePath(
                                               graphp->शीर्ष_Command_Args_Filename(सत्य))))
                    .end();
            }

            of.end();  // submodules
        }
    }

सार्वजनिक:
    explicit V3EmitMkJsonEmitter() { emitManifest(); }
    आभास ~V3EmitMkJsonEmitter() = स्वभाव;
};

शून्य V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ध्रुव V3EmitMkJsonEmitter emitter;
}
