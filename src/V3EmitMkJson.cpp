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
            = V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(v3Global.opt.makeDir()));

        V3_بیرون_Json_File از{makeDir + "/" + v3Global.opt.پیشوند() + ".json"};

        std::وکتور<string> classesFast;
        std::وکتور<string> classesSlow;
        std::وکتور<string> supportFast;
        std::وکتور<string> supportSlow;
        std::وکتور<string> global;
        std::وکتور<string> deps;
        std::وکتور<string> cppFiles;

        لكل (Ast_گره_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گره_File)) {
            ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
            إذا (cfilep && cfilep->منبع()) {
                ثابت std::string filename
                    = V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(cfilep->name()));
                إذا (cfilep->support()) {
                    إذا (cfilep->slow()) {
                        supportSlow.emplace_پشت(filename);
                    } وإلا {
                        supportFast.emplace_پشت(filename);
                    }
                } وإلا {
                    إذا (cfilep->slow()) {
                        classesSlow.emplace_پشت(filename);
                    } وإلا {
                        classesFast.emplace_پشت(filename);
                    }
                }
            }
        }

        ثابت std::string verilator_ریشه = V3Os::filename_Slash_مسیر(
            V3Os::filename_Absolute_مسیر(V3Options::getenv_VERILATOR_ریشه()));
        لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
            global.emplace_پشت(verilator_ریشه + "/include/" + cpp);
        إذا (!v3Global.opt.libCreate().خالی()) {
            global.emplace_پشت(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        لكل (ثابت خودکار& dep : V3File::getAllDeps())
            deps.emplace_پشت(V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(dep)));
        لكل (ثابت خودکار& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_پشت(V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(cppFile)));

        از.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_ریشه)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .انتها()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.استفاده_میکند_Timing())
            .put("threads", v3Global.opt.نخها())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .انتها()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .انتها();

        إذا (ثابت V3_Hier_گراف* ثابت graphp = v3Global.hierGraphp()) {
            از.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            لكل (ثابت V3_گراف_رأس& vtx : vlstd::معکوس_view(graphp->vertices())) {
                ثابت V3_Hier_بلوک* ثابت hblockp = vtx.as<V3_Hier_بلوک>();
                std::وکتور<std::string> hierDeps;
                std::وکتور<std::string> sources;
                لكل (ثابت V3_گراف_یال& یال : hblockp->بیرون_یالها()) {
                    ثابت V3_Hier_بلوک* ثابت dependencyp = یال.بالاترین()->as<V3_Hier_بلوک>();
                    hierDeps.emplace_پشت(dependencyp->hier_پیشوند());
                    sources.emplace_پشت(makeDir + "/" + dependencyp->hierWrapperFilename(درست));
                }

                لكل (ثابت string& fname : V3_Hier_گراف::منبع_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_پشت(
                        V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(fname)));
                }

                از.begin()
                    .put("prefix", hblockp->hier_پیشوند())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_پیشوند())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(
                                               hblockp->command_آرگومانها_Filename(درست))))
                    .انتها();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::وکتور<std::string> sources;
                لكل (ثابت V3_گراف_رأس& vtx : graphp->vertices()) {
                    ثابت V3_Hier_بلوک* ثابت blockp = vtx.as<V3_Hier_بلوک>();
                    sources.emplace_پشت(makeDir + "/" + blockp->hierWrapperFilename(درست));
                }

                لكل (ثابت string& fname : V3_Hier_گراف::منبع_Files()) {
                    sources.emplace_پشت(
                        V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(fname)));
                }

                از.begin()
                    .put("prefix", v3Global.opt.پیشوند())
                    .put("top", v3Global.rootp()->بالاترین_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_مسیر(V3Os::filename_Absolute_مسیر(
                                               graphp->بالاترین_Command_آرگومانها_Filename(درست))))
                    .انتها();
            }

            از.انتها();  // submodules
        }
    }

عام:
    صریح V3EmitMkJsonEmitter() { emitManifest(); }
    مجازی ~V3EmitMkJsonEmitter() = افتراضي;
};

فراغ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت V3EmitMkJsonEmitter emitter;
}
