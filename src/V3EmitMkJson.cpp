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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitMkJson.h"

#شامل "V3EmitCBase.h"
#شامل "V3HierBlock.h"
#شامل "V3Os.h"

#شامل <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements

جماعت V3EmitMkJsonEmitter final {
    // METHODS

    // STATIC FUNCTIONS
    ساکن خالی emitManifest() {
        ثابت std::string makeDir
            = V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(v3Global.opt.makeDir()));

        V3_برآمد_Json_File کا{makeDir + "/" + v3Global.opt.سابقہ() + ".json"};

        std::سمتیہ<string> classesFast;
        std::سمتیہ<string> classesSlow;
        std::سمتیہ<string> supportFast;
        std::سمتیہ<string> supportSlow;
        std::سمتیہ<string> global;
        std::سمتیہ<string> deps;
        std::سمتیہ<string> cppFiles;

        برائے (Ast_گرہ_File* nodep = v3Global.rootp()->filesp(); nodep;
             nodep = VN_AS(nodep->nextp(), گرہ_File)) {
            ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
            اگر (cfilep && cfilep->ماخذ()) {
                ثابت std::string filename
                    = V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(cfilep->name()));
                اگر (cfilep->support()) {
                    اگر (cfilep->slow()) {
                        supportSlow.emplace_عقب(filename);
                    } ورنہ {
                        supportFast.emplace_عقب(filename);
                    }
                } ورنہ {
                    اگر (cfilep->slow()) {
                        classesSlow.emplace_عقب(filename);
                    } ورنہ {
                        classesFast.emplace_عقب(filename);
                    }
                }
            }
        }

        ثابت std::string verilator_جڑ = V3Os::filename_Slash_راستہ(
            V3Os::filename_Absolute_راستہ(V3Options::getenv_VERILATOR_جڑ()));
        برائے (ثابت string& cpp : v3Global.verilatedCppFiles())
            global.emplace_عقب(verilator_جڑ + "/include/" + cpp);
        اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            global.emplace_عقب(makeDir + "/" + v3Global.opt.libCreate() + ".cpp");
        }

        برائے (ثابت خودکار& dep : V3File::getAllDeps())
            deps.emplace_عقب(V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(dep)));
        برائے (ثابت خودکار& cppFile : v3Global.opt.cppFiles())
            cppFiles.emplace_عقب(V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(cppFile)));

        کا.put("version", 1)
            .begin("system")
            .put("perl", V3Options::getenvPERL())
            .put("python3", V3Options::getenvPYTHON3())
            .put("verilator_root", verilator_جڑ)
            .put("verilator_solver", V3Options::getenvVERILATOR_SOLVER())
            .انتہا()
            .begin("options")
            .putList("cflags", v3Global.opt.cFlags())
            .putList("ldflags", v3Global.opt.ldLibs())
            .put("system_c", v3Global.opt.systemC())
            .put("coverage", v3Global.opt.coverage())
            .put("dpi", v3Global.dpi())
            .put("use_timing", v3Global.استعمال_کرتا_Timing())
            .put("threads", v3Global.opt.دھاگے())
            .put("trace", v3Global.opt.trace())
            .put("trace_fst", v3Global.opt.traceEnabledFst())
            .put("trace_saif", v3Global.opt.traceEnabledSaif())
            .put("trace_vcd", v3Global.opt.traceEnabledVcd())
            .انتہا()
            .begin("sources")
            .putList("global", global)
            .putList("classes_slow", classesSlow)
            .putList("classes_fast", classesFast)
            .putList("support_slow", supportSlow)
            .putList("support_fast", supportFast)
            .putList("deps", deps)
            .putList("user_classes", cppFiles)
            .انتہا();

        اگر (ثابت V3_Hier_گراف* ثابت graphp = v3Global.hierGraphp()) {
            کا.begin("submodules", '[');

            // Enumerate in dependency order, leaves first. TODO: Shouldn't
            // really have to, but verilator-config.cmake.in depends on order.
            برائے (ثابت V3_گراف_راس& vtx : vlstd::الٹ_view(graphp->vertices())) {
                ثابت V3_Hier_بلاک* ثابت hblockp = vtx.as<V3_Hier_بلاک>();
                std::سمتیہ<std::string> hierDeps;
                std::سمتیہ<std::string> sources;
                برائے (ثابت V3_گراف_کنارہ& کنارہ : hblockp->برآمد_کنارے()) {
                    ثابت V3_Hier_بلاک* ثابت dependencyp = کنارہ.چوٹی()->as<V3_Hier_بلاک>();
                    hierDeps.emplace_عقب(dependencyp->hier_سابقہ());
                    sources.emplace_عقب(makeDir + "/" + dependencyp->hierWrapperFilename(سچ));
                }

                برائے (ثابت string& fname : V3_Hier_گراف::ماخذ_Files(hblockp->vFileIfNecessary())) {
                    sources.emplace_عقب(
                        V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(fname)));
                }

                کا.begin()
                    .put("prefix", hblockp->hier_سابقہ())
                    .put("top", hblockp->modp()->name())
                    .putList("deps", hierDeps)
                    .put("directory", makeDir + "/" + hblockp->hier_سابقہ())
                    .putList("sources", sources)
                    .putList("cflags", {"-fPIC"})
                    .put("verilator_args", V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(
                                               hblockp->command_دلائل_Filename(سچ))))
                    .انتہا();
            }

            // Top level reintegration
            {
                // TODO: When this reintegration "submodule" is built with the bundled
                // CMake script, it will overwrite the original json manifest containing the
                // "subodule" list we are creating here with one that doesn't have "submodule".
                // Good luck debugging, suggest 'message(${MANIFEST})' in verilated-config.cmake.
                std::سمتیہ<std::string> sources;
                برائے (ثابت V3_گراف_راس& vtx : graphp->vertices()) {
                    ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
                    sources.emplace_عقب(makeDir + "/" + blockp->hierWrapperFilename(سچ));
                }

                برائے (ثابت string& fname : V3_Hier_گراف::ماخذ_Files()) {
                    sources.emplace_عقب(
                        V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(fname)));
                }

                کا.begin()
                    .put("prefix", v3Global.opt.سابقہ())
                    .put("top", v3Global.rootp()->چوٹی_Modulep()->name())
                    .put("directory", makeDir)
                    .putList("sources", sources)
                    .put("verilator_args", V3Os::filename_Slash_راستہ(V3Os::filename_Absolute_راستہ(
                                               graphp->چوٹی_Command_دلائل_Filename(سچ))))
                    .انتہا();
            }

            کا.انتہا();  // submodules
        }
    }

عوامی:
    صریح V3EmitMkJsonEmitter() { emitManifest(); }
    مجازی ~V3EmitMkJsonEmitter() = طےشدہ;
};

خالی V3EmitMkJson::emit() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت V3EmitMkJsonEmitter emitter;
}
