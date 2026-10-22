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
            = V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(v3Global.opt.makeDir()));

        V3_बहिर्गमनम्_Json_File सम्बन्धि{makeDir + "/" + v3Global.opt.उपसर्गः() + ".json"};

        std::सदिशम्<string> classesFast;
        std::सदिशम्<string> classesSlow;
        std::सदिशम्<string> supportFast;
        std::सदिशम्<string> supportSlow;
        std::सदिशम्<string> global;
        std::सदिशम्<string> deps;
        std::सदिशम्<string> cppFiles;

        प्रति (Ast_ग्रन्थिः_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ग्रन्थिः_File)) {
            ध्रुव AstCFile* ध्रुव cfilep = VN_CAST(nodep, CFile);
            यदि (cfilep && cfilep->source()) {
                ध्रुव std::string filename
                    = V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(cfilep->name()));
                यदि (cfilep->support()) {
                    यदि (cfilep->slow()) {
                        supportSlow.emplace_पृष्ठभागः(filename);
                    } अन्यथा {
                        supportFast.emplace_पृष्ठभागः(filename);
                    }
                } अन्यथा {
                    यदि (cfilep->slow()) {
                        classesSlow.emplace_पृष्ठभागः(filename);
                    } अन्यथा {
                        classesFast.emplace_पृष्ठभागः(filename);
                    }
                }
            }
        }

        ध्रुव std::string verilator_मूलम् = V3Os::filename_Slash_मार्गः(
            V3Os::filename_Absolute_मार्गः(V3Options::getenv_VERILATOR_मूलम्()));
        प्रति (ध्रुव string& cpp : v3Global.verilatedCppFiles())
            global.emplace_पृष्ठभागः(verilator_मूलम् + "/include/" + cpp);
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            global.emplace_पृष्ठभागः(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        प्रति (ध्रुव स्वचालित& dep : V3File::getAllDeps())
            deps.emplace_पृष्ठभागः(V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(dep)));
        प्रति (ध्रुव स्वचालित& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_पृष्ठभागः(V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(cppFile)));

        सम्बन्धि.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_मूलम्)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .अन्तः_2()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.उपयुङ्क्ते_Timing())
            .put("threads", v3Global.opt.तन्तवः())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .अन्तः_2()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .अन्तः_2();

        यदि (ध्रुव V3HierGraph* ध्रुव graphp = v3Global.hierGraphp()) {
            सम्बन्धि.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            प्रति (ध्रुव V3GraphVertex& vtx : vlstd::reverse_view(graphp->vertices())) {
                ध्रुव V3_Hier_खण्डम्* ध्रुव hblockp = vtx.as<V3_Hier_खण्डम्>();
                std::सदिशम्<std::string> hierDeps;
                std::सदिशम्<std::string> sources;
                प्रति (ध्रुव V3GraphEdge& edge : hblockp->बहिर्गमनम्_धाराः()) {
                    ध्रुव V3_Hier_खण्डम्* ध्रुव dependencyp = edge.शिखरम्()->as<V3_Hier_खण्डम्>();
                    hierDeps.emplace_पृष्ठभागः(dependencyp->hier_उपसर्गः());
                    sources.emplace_पृष्ठभागः(makeDir + "/" + dependencyp->hierWrapperFilename(सत्य));
                }

                प्रति (ध्रुव string& fname : V3HierGraph::sourceFiles(hblockp->vFileIfNecessary())) {
                    sources.emplace_पृष्ठभागः(
                        V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(fname)));
                }

                सम्बन्धि.begin()
                    .put("prefix", hblockp->hier_उपसर्गः())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_उपसर्गः())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(
                                               hblockp->command_तर्काः_Filename(सत्य))))
                    .अन्तः_2();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::सदिशम्<std::string> sources;
                प्रति (ध्रुव V3GraphVertex& vtx : graphp->vertices()) {
                    ध्रुव V3_Hier_खण्डम्* ध्रुव blockp = vtx.as<V3_Hier_खण्डम्>();
                    sources.emplace_पृष्ठभागः(makeDir + "/" + blockp->hierWrapperFilename(सत्य));
                }

                प्रति (ध्रुव string& fname : V3HierGraph::sourceFiles()) {
                    sources.emplace_पृष्ठभागः(
                        V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(fname)));
                }

                सम्बन्धि.begin()
                    .put("prefix", v3Global.opt.उपसर्गः())
                    .put("top", v3Global.rootp()->शिखरम्_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_मार्गः(V3Os::filename_Absolute_मार्गः(
                                               graphp->शिखरम्_Command_तर्काः_Filename(सत्य))))
                    .अन्तः_2();
            }

            सम्बन्धि.अन्तः_2();  // submodules
        }
    }

सार्वजनिक:
    स्पष्ट V3EmitMkJsonEmitter() { emitManifest(); }
    आभास ~V3EmitMkJsonEmitter() = स्वभाव;
};

शून्य V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ध्रुव V3EmitMkJsonEmitter emitter;
}
