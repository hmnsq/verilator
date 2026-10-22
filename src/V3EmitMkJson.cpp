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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitMkJson.h"

#ضمن "V3EmitCBase.h"
#ضمن "V3HierBlock.h"
#ضمن "V3Os.h"

#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

صنف V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    ساكن فراغ emitManifest() {
        ثابت std::string makeDir
            = V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(v3Global.opt.makeDir()));

        V3_بهر_Json_File د{makeDir + "/" + v3Global.opt.مختاړی() + ".json"};

        std::ویکټور<string> classesFast;
        std::ویکټور<string> classesSlow;
        std::ویکټور<string> supportFast;
        std::ویکټور<string> supportSlow;
        std::ویکټور<string> global;
        std::ویکټور<string> deps;
        std::ویکټور<string> cppFiles;

        لكل (Ast_غوټه_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), غوټه_File)) {
            ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
            إذا (cfilep && cfilep->سرچینه()) {
                ثابت std::string filename
                    = V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(cfilep->name()));
                إذا (cfilep->support()) {
                    إذا (cfilep->slow()) {
                        supportSlow.emplace_شا(filename);
                    } وإلا {
                        supportFast.emplace_شا(filename);
                    }
                } وإلا {
                    إذا (cfilep->slow()) {
                        classesSlow.emplace_شا(filename);
                    } وإلا {
                        classesFast.emplace_شا(filename);
                    }
                }
            }
        }

        ثابت std::string verilator_ریښه = V3Os::filename_Slash_لار(
            V3Os::filename_Absolute_لار(V3Options::getenv_VERILATOR_ریښه()));
        لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
            global.emplace_شا(verilator_ریښه + "/include/" + cpp);
        إذا (!v3Global.opt.libCreate().تشه()) {
            global.emplace_شا(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        لكل (ثابت خپلکاری& dep : V3File::getAllDeps())
            deps.emplace_شا(V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(dep)));
        لكل (ثابت خپلکاری& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_شا(V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(cppFile)));

        د.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_ریښه)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .پای_ټکی()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.کاروي_Timing())
            .put("threads", v3Global.opt.تارونه())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .پای_ټکی()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .پای_ټکی();

        إذا (ثابت V3_Hier_ګراف* ثابت graphp = v3Global.hierGraphp()) {
            د.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            لكل (ثابت V3_ګراف_راس& vtx : vlstd::سرچپه_view(graphp->vertices())) {
                ثابت V3_Hier_بلاک* ثابت hblockp = vtx.as<V3_Hier_بلاک>();
                std::ویکټور<std::string> hierDeps;
                std::ویکټور<std::string> sources;
                لكل (ثابت V3_ګراف_څنډه& څنډه : hblockp->بهر_څنډې()) {
                    ثابت V3_Hier_بلاک* ثابت dependencyp = څنډه.سر()->as<V3_Hier_بلاک>();
                    hierDeps.emplace_شا(dependencyp->hier_مختاړی());
                    sources.emplace_شا(makeDir + "/" + dependencyp->hierWrapperFilename(ریښتیا));
                }

                لكل (ثابت string& fname : V3_Hier_ګراف::سرچینه_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_شا(
                        V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(fname)));
                }

                د.begin()
                    .put("prefix", hblockp->hier_مختاړی())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_مختاړی())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(
                                               hblockp->command_دلیلونه_Filename(ریښتیا))))
                    .پای_ټکی();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::ویکټور<std::string> sources;
                لكل (ثابت V3_ګراف_راس& vtx : graphp->vertices()) {
                    ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
                    sources.emplace_شا(makeDir + "/" + blockp->hierWrapperFilename(ریښتیا));
                }

                لكل (ثابت string& fname : V3_Hier_ګراف::سرچینه_Files()) {
                    sources.emplace_شا(
                        V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(fname)));
                }

                د.begin()
                    .put("prefix", v3Global.opt.مختاړی())
                    .put("top", v3Global.rootp()->سر_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_لار(V3Os::filename_Absolute_لار(
                                               graphp->سر_Command_دلیلونه_Filename(ریښتیا))))
                    .پای_ټکی();
            }

            د.پای_ټکی();  // submodules
        }
    }

عام:
    څرګند V3EmitMkJsonEmitter() { emitManifest(); }
    مجازي ~V3EmitMkJsonEmitter() = افتراضي;
};

فراغ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت V3EmitMkJsonEmitter emitter;
}
