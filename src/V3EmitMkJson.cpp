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
            = V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(v3Global.opt.makeDir()));

        V3_ٻاهريون_Json_File جو{makeDir + "/" + v3Global.opt.اڳياڙي() + ".json"};

        std::ويڪٽر<string> classesFast;
        std::ويڪٽر<string> classesSlow;
        std::ويڪٽر<string> supportFast;
        std::ويڪٽر<string> supportSlow;
        std::ويڪٽر<string> global;
        std::ويڪٽر<string> deps;
        std::ويڪٽر<string> cppFiles;

        لكل (Ast_ڳنڍ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), ڳنڍ_File)) {
            ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
            إذا (cfilep && cfilep->ذريعو()) {
                ثابت std::string filename
                    = V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(cfilep->name()));
                إذا (cfilep->support()) {
                    إذا (cfilep->slow()) {
                        supportSlow.emplace_پٺ(filename);
                    } وإلا {
                        supportFast.emplace_پٺ(filename);
                    }
                } وإلا {
                    إذا (cfilep->slow()) {
                        classesSlow.emplace_پٺ(filename);
                    } وإلا {
                        classesFast.emplace_پٺ(filename);
                    }
                }
            }
        }

        ثابت std::string verilator_پاڙ = V3Os::filename_Slash_رستو(
            V3Os::filename_Absolute_رستو(V3Options::getenv_VERILATOR_پاڙ()));
        لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
            global.emplace_پٺ(verilator_پاڙ + "/include/" + cpp);
        إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            global.emplace_پٺ(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        لكل (ثابت خودڪار& dep : V3File::getAllDeps())
            deps.emplace_پٺ(V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(dep)));
        لكل (ثابت خودڪار& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_پٺ(V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(cppFile)));

        جو.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_پاڙ)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .پڇاڙي()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.استعمال_ڪري_Timing())
            .put("threads", v3Global.opt.ڌاڳا())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .پڇاڙي()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .پڇاڙي();

        إذا (ثابت V3_Hier_گراف* ثابت graphp = v3Global.hierGraphp()) {
            جو.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            لكل (ثابت V3_گراف_راس& vtx : vlstd::ابتو_view(graphp->vertices())) {
                ثابت V3_Hier_بلاڪ* ثابت hblockp = vtx.as<V3_Hier_بلاڪ>();
                std::ويڪٽر<std::string> hierDeps;
                std::ويڪٽر<std::string> sources;
                لكل (ثابت V3_گراف_ڪنارو& ڪنارو : hblockp->ٻاهريون_ڪنارا()) {
                    ثابت V3_Hier_بلاڪ* ثابت dependencyp = ڪنارو.چوٽي()->as<V3_Hier_بلاڪ>();
                    hierDeps.emplace_پٺ(dependencyp->hier_اڳياڙي());
                    sources.emplace_پٺ(makeDir + "/" + dependencyp->hierWrapperFilename(سچ));
                }

                لكل (ثابت string& fname : V3_Hier_گراف::ذريعو_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_پٺ(
                        V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(fname)));
                }

                جو.begin()
                    .put("prefix", hblockp->hier_اڳياڙي())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_اڳياڙي())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(
                                               hblockp->command_دليلون_Filename(سچ))))
                    .پڇاڙي();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::ويڪٽر<std::string> sources;
                لكل (ثابت V3_گراف_راس& vtx : graphp->vertices()) {
                    ثابت V3_Hier_بلاڪ* ثابت blockp = vtx.as<V3_Hier_بلاڪ>();
                    sources.emplace_پٺ(makeDir + "/" + blockp->hierWrapperFilename(سچ));
                }

                لكل (ثابت string& fname : V3_Hier_گراف::ذريعو_Files()) {
                    sources.emplace_پٺ(
                        V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(fname)));
                }

                جو.begin()
                    .put("prefix", v3Global.opt.اڳياڙي())
                    .put("top", v3Global.rootp()->چوٽي_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_رستو(V3Os::filename_Absolute_رستو(
                                               graphp->چوٽي_Command_دليلون_Filename(سچ))))
                    .پڇاڙي();
            }

            جو.پڇاڙي();  // submodules
        }
    }

عام:
    واضح V3EmitMkJsonEmitter() { emitManifest(); }
    مجازي ~V3EmitMkJsonEmitter() = افتراضي;
};

فراغ V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت V3EmitMkJsonEmitter emitter;
}
