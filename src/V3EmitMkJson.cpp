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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitMkJson.h"

#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3HierBlock.h"
#అన్తర్భావ "V3Os.h"

#అన్తర్భావ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

తరగతి V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    స్థిర శూన్య emitManifest() {
        స్థిరరాశి std::string makeDir
            = V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(v3Global.opt.makeDir()));

        V3_బహిర్గతం_Json_File యొక్క{makeDir + "/" + v3Global.opt.ఉపసర్గ() + ".json"};

        std::సదిశ<string> classesFast;
        std::సదిశ<string> classesSlow;
        std::సదిశ<string> supportFast;
        std::సదిశ<string> supportSlow;
        std::సదిశ<string> global;
        std::సదిశ<string> deps;
        std::సదిశ<string> cppFiles;

        కొరకు (Ast_కణుపు_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), కణుపు_File)) {
            స్థిరరాశి AstCFile* స్థిరరాశి cfilep = VN_CAST(nodep, CFile);
            ఒకవేళ (cfilep && cfilep->ఆధారం()) {
                స్థిరరాశి std::string filename
                    = V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(cfilep->name()));
                ఒకవేళ (cfilep->support()) {
                    ఒకవేళ (cfilep->slow()) {
                        supportSlow.emplace_వెనుక(filename);
                    } లేకపోతే {
                        supportFast.emplace_వెనుక(filename);
                    }
                } లేకపోతే {
                    ఒకవేళ (cfilep->slow()) {
                        classesSlow.emplace_వెనుక(filename);
                    } లేకపోతే {
                        classesFast.emplace_వెనుక(filename);
                    }
                }
            }
        }

        స్థిరరాశి std::string verilator_మూలం = V3Os::filename_Slash_మార్గం(
            V3Os::filename_Absolute_మార్గం(V3Options::getenv_VERILATOR_మూలం()));
        కొరకు (స్థిరరాశి string& cpp : v3Global.verilatedCppFiles())
            global.emplace_వెనుక(verilator_మూలం + "/include/" + cpp);
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            global.emplace_వెనుక(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        కొరకు (స్థిరరాశి స్వయంచాలక& dep : V3File::getAllDeps())
            deps.emplace_వెనుక(V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(dep)));
        కొరకు (స్థిరరాశి స్వయంచాలక& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_వెనుక(V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(cppFile)));

        యొక్క.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_మూలం)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .చివర()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.ఉపయోగిస్తుంది_Timing())
            .put("threads", v3Global.opt.దారాలు())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .చివర()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .చివర();

        ఒకవేళ (స్థిరరాశి V3_Hier_లేఖాచిత్రం* స్థిరరాశి graphp = v3Global.hierGraphp()) {
            యొక్క.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vlstd::తిరగేయి_view(graphp->vertices())) {
                స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి hblockp = vtx.as<V3_Hier_ఖండం>();
                std::సదిశ<std::string> hierDeps;
                std::సదిశ<std::string> sources;
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : hblockp->బహిర్గతం_అంచులు()) {
                    స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి dependencyp = అంచు.శిఖరం()->as<V3_Hier_ఖండం>();
                    hierDeps.emplace_వెనుక(dependencyp->hier_ఉపసర్గ());
                    sources.emplace_వెనుక(makeDir + "/" + dependencyp->hierWrapperFilename(నిజం));
                }

                కొరకు (స్థిరరాశి string& fname : V3_Hier_లేఖాచిత్రం::ఆధారం_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_వెనుక(
                        V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(fname)));
                }

                యొక్క.begin()
                    .put("prefix", hblockp->hier_ఉపసర్గ())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_ఉపసర్గ())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(
                                               hblockp->command_వాదనలు_Filename(నిజం))))
                    .చివర();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::సదిశ<std::string> sources;
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
                    స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి blockp = vtx.as<V3_Hier_ఖండం>();
                    sources.emplace_వెనుక(makeDir + "/" + blockp->hierWrapperFilename(నిజం));
                }

                కొరకు (స్థిరరాశి string& fname : V3_Hier_లేఖాచిత్రం::ఆధారం_Files()) {
                    sources.emplace_వెనుక(
                        V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(fname)));
                }

                యొక్క.begin()
                    .put("prefix", v3Global.opt.ఉపసర్గ())
                    .put("top", v3Global.rootp()->శిఖరం_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_మార్గం(V3Os::filename_Absolute_మార్గం(
                                               graphp->శిఖరం_Command_వాదనలు_Filename(నిజం))))
                    .చివర();
            }

            యొక్క.చివర();  // submodules
        }
    }

బహిరంగ:
    స్పష్టమైన V3EmitMkJsonEmitter() { emitManifest(); }
    మాయా ~V3EmitMkJsonEmitter() = అప్రమేయ;
};

శూన్య V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    స్థిరరాశి V3EmitMkJsonEmitter emitter;
}
