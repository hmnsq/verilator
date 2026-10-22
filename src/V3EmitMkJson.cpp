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
            = V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(v3Global.opt.makeDir()));

        V3_باہرلا_Json_File دا{makeDir + "/" + v3Global.opt.اگیتر() + ".json"};

        std::سمتیہ<string> classesFast;
        std::سمتیہ<string> classesSlow;
        std::سمتیہ<string> supportFast;
        std::سمتیہ<string> supportSlow;
        std::سمتیہ<string> global;
        std::سمتیہ<string> deps;
        std::سمتیہ<string> cppFiles;

        لكل (Ast_گرہ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گرہ_File)) {
            ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
            إذا (cfilep && cfilep->سروت()) {
                ثابت std::string filename
                    = V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(cfilep->name()));
                إذا (cfilep->support()) {
                    إذا (cfilep->slow()) {
                        supportSlow.emplace_پٹھ(filename);
                    } وإلا {
                        supportFast.emplace_پٹھ(filename);
                    }
                } وإلا {
                    إذا (cfilep->slow()) {
                        classesSlow.emplace_پٹھ(filename);
                    } وإلا {
                        classesFast.emplace_پٹھ(filename);
                    }
                }
            }
        }

        ثابت std::string verilator_جڑ = V3Os::filename_Slash_رستہ(
            V3Os::filename_Absolute_رستہ(V3Options::getenv_VERILATOR_جڑ()));
        لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
            global.emplace_پٹھ(verilator_جڑ + "/include/" + cpp);
        إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            global.emplace_پٹھ(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        لكل (ثابت خودکار& dep : V3File::getAllDeps())
            deps.emplace_پٹھ(V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(dep)));
        لكل (ثابت خودکار& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_پٹھ(V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(cppFile)));

        دا.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_جڑ)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .سرا()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.ورتدا_Timing())
            .put("threads", v3Global.opt.دھاگے())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .سرا()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .سرا();

        إذا (ثابت V3_Hier_گراف* ثابت graphp = v3Global.hierGraphp()) {
            دا.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            لكل (ثابت V3_گراف_راس& vtx : vlstd::الٹ_view(graphp->vertices())) {
                ثابت V3_Hier_بلاک* ثابت hblockp = vtx.as<V3_Hier_بلاک>();
                std::سمتیہ<std::string> hierDeps;
                std::سمتیہ<std::string> sources;
                لكل (ثابت V3_گراف_کنارا& کنارا : hblockp->باہرلا_کنارے()) {
                    ثابت V3_Hier_بلاک* ثابت dependencyp = کنارا.چوٹی()->as<V3_Hier_بلاک>();
                    hierDeps.emplace_پٹھ(dependencyp->hier_اگیتر());
                    sources.emplace_پٹھ(makeDir + "/" + dependencyp->hierWrapperFilename(سچ));
                }

                لكل (ثابت string& fname : V3_Hier_گراف::سروت_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_پٹھ(
                        V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(fname)));
                }

                دا.begin()
                    .put("prefix", hblockp->hier_اگیتر())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_اگیتر())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(
                                               hblockp->command_دلیلاں_Filename(سچ))))
                    .سرا();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::سمتیہ<std::string> sources;
                لكل (ثابت V3_گراف_راس& vtx : graphp->vertices()) {
                    ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
                    sources.emplace_پٹھ(makeDir + "/" + blockp->hierWrapperFilename(سچ));
                }

                لكل (ثابت string& fname : V3_Hier_گراف::سروت_Files()) {
                    sources.emplace_پٹھ(
                        V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(fname)));
                }

                دا.begin()
                    .put("prefix", v3Global.opt.اگیتر())
                    .put("top", v3Global.rootp()->چوٹی_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_رستہ(V3Os::filename_Absolute_رستہ(
                                               graphp->چوٹی_Command_دلیلاں_Filename(سچ))))
                    .سرا();
            }

            دا.سرا();  // submodules
        }
    }

عام:
    واضح V3EmitMkJsonEmitter() { emitManifest(); }
    مجازی ~V3EmitMkJsonEmitter() = افتراضي;
};

فراغ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت V3EmitMkJsonEmitter emitter;
}
