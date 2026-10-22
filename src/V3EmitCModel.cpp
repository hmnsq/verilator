// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for model entry point class
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCFunc.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ EmitCModel final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    // TYPES
    ಬಳಸಿ CFunc_ಸದಿಶ = std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ AstCFunc*>;

    // MEMBERS
    V3_Unique_ಹೆಸರುಗಳು m_unique_ಹೆಸರುಗಳು;  // For generating unique file names

    // METHODS

    CFunc_ಸದಿಶ ಕಂಡುಹಿಡಿ_Funcps(std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ AstCFunc*)> cb) {
        CFunc_ಸದಿಶ funcps;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc)) {
                ಆದರೆ (cb(funcp)) funcps.ತಳ್ಳು_ಹಿಂಭಾಗ(funcp);
            }
        }
        stable_ವಿಂಗಡಿಸು(funcps.begin(), funcps.ಕೊನೆ(),
                    [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ap, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* bp) { ಹಿಂತಿರುಗಿ ap->name() < bp->name(); });
        ಹಿಂತಿರುಗಿ funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDpiPassingStructs(ಸ್ಥಿರಾಂಕ AstCFunc* funcp) {
        ಆದರೆ (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(argp, Var);
            ಆದರೆ (varp && varp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ putSectionDelimiter(ಸ್ಥಿರಾಂಕ string& name) {
        puts("\n");
        puts("//============================================================\n");
        puts("// " + name + "\n");
    }

    ಶೂನ್ಯ emit_ಶೀರ್ಷಿಕೆ(Ast_ಗಂಟು_Module* modp) {
        open_New_Output_ಶೀರ್ಷಿಕೆ_File(EmitCUtil::ತುದಿ_Class_Name(), "Primary model header");
        puts("//\n");
        puts("// This header should be included by all source files instantiating the design.\n");
        puts("// The class here is then constructed to instantiate the design.\n");
        puts("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ತುದಿ_Include();

        puts("#include \"verilated.h\"\n");
        ಆದರೆ (v3Global.opt.systemC()) puts("#include \"verilated_sc.h\"\n");
        ಆದರೆ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ಆದರೆ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ಆದರೆ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ಆದರೆ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ಆದರೆ (v3Global.dpi()) puts("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        ಆದರೆ (!ಕಂಡುಹಿಡಿ_Funcps(isDpiPassingStructs).ಖಾಲಿಗಳು()) {
            puts("#include \"" + EmitCUtil::ತುದಿ_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        puts("\n");
        puts("class " + EmitCUtil::symClassName() + ";\n");
        puts("class " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp) + ";\n");  // For rootp pointer only
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ತಳ : v3Global.opt.traceClassLangs()) puts("class " + ತಳ + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        puts("\n");

        puts("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::ತುದಿ_Class_Name()
                        + " VL_NOT_FINAL : ");
        ಆದರೆ (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            puts("public ::sc_core::sc_module, ");
        }
        puts("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        puts("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->reset_ಖಾಸಗಿ();
        ofp()->puts_ಖಾಸಗಿ(ನಿಜ);  // private:

        puts("// Symbol table holding complete model state (owned by this class)\n");
        puts(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        puts("// Evaluation loop\n");
        puts("VerilatedEvalLoop m_evalLoop;\n");

        puts("\n");
        ofp()->puts_ಖಾಸಗಿ(ಸುಳ್ಳು);  // public:

        puts("\n// CONSTEXPR CAPABILITIES\n");
        puts("// Verilated with --trace?\n");
        puts("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        puts("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ ನಿಜ);
                }
            }
        }
        ಆದರೆ (v3Global.opt.emitAccessors()) {
            puts("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                    ಆದರೆ (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        puts("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell)) {
                ಆದರೆ (cellp->modp()->isConstPool()) ಮುಂದುವರಿ;  // Special emit rules
                putns(cellp, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        puts("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        puts(EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp) + "* const rootp;\n");

        puts("\n");
        ofp()->puts_ಖಾಸಗಿ(ಸುಳ್ಳು);  // public:
        puts("// CONSTRUCTORS\n");
        ಆದರೆ (optSystemC()) {
            puts("SC_CTOR(" + EmitCUtil::ತುದಿ_Class_Name() + ");\n");
            puts("virtual ~" + EmitCUtil::ತುದಿ_Class_Name() + "();\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("/// Construct the model; called by application code\n");
            puts("/// If contextp is null, then the model will use the default global "
                 "context\n");
            puts("/// If name is \"\", then makes a wrapper with a\n");
            puts("/// single model invisible with respect to DPI scope names.\n");
            puts("explicit " + EmitCUtil::ತುದಿ_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            puts("explicit " + EmitCUtil::ತುದಿ_Class_Name() + "(const char* name = \"TOP\");\n");
            puts("/// Destroy the model; called (often implicitly) by application code\n");
            puts("virtual ~" + EmitCUtil::ತುದಿ_Class_Name() + "();\n");
        }
        ofp()->puts_ಖಾಸಗಿ(ನಿಜ);
        puts("VL_UNCOPYABLE(" + EmitCUtil::ತುದಿ_Class_Name() + ");  ///< Copying not allowed\n");

        puts("\n");
        ofp()->puts_ಖಾಸಗಿ(ಸುಳ್ಳು);  // public:
        puts("// API METHODS\n");
        ಸ್ಥಿರಾಂಕ string call_Eval_ಕೊನೆ_ಹೆಜ್ಜೆ
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        ಆದರೆ (optSystemC()) {
            ofp()->puts_ಖಾಸಗಿ(ನಿಜ);  ///< eval() is invoked by our sensitive() calls.
        }
        ಆದರೆ (!optSystemC()) {
            puts("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        ಆದರೆ (optSystemC() && v3Global.ಬಳಸುತ್ತದೆ_Timing()) {
            puts("void eval();\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("void eval() { eval_step(); " + call_Eval_ಕೊನೆ_ಹೆಜ್ಜೆ + "}\n");
        }
        ಆದರೆ (!optSystemC()) {
            puts("/// Evaluate when calling multiple units/models per time step.\n");
        }
        puts("void eval_step();\n");
        ಆದರೆ (!optSystemC()) {
            puts("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            puts("/// Application must call after all eval() and before time changes.\n");
            puts("void eval_end_step()");
            ಆದರೆ (call_Eval_ಕೊನೆ_ಹೆಜ್ಜೆ == "") {
                puts(" {}\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts(";\n");
            }
        }
        ಆದರೆ (!optSystemC()) {
            puts("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->puts_ಖಾಸಗಿ(ಸುಳ್ಳು);  // public:
        puts("void final();\n");

        puts("/// Are there scheduled events to handle?\n");
        puts("bool eventsPending();\n");
        puts("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        puts("uint64_t nextTimeSlot();\n");

        ಆದರೆ (v3Global.opt.trace() || !optSystemC()) {
            puts("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            puts("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        ಆದರೆ (v3Global.opt.trace() && optSystemC()) {
            puts("/// SC tracing; avoid overloaded virtual function lint warning\n");
            puts("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        ಆದರೆ (!optSystemC()) {
            puts("/// Retrieve name of this model instance (as passed to constructor).\n");
            puts("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            ಸ್ಥಿರಾಂಕ CFunc_ಸದಿಶ funcps
                = ಕಂಡುಹಿಡಿ_Funcps([](ಸ್ಥಿರಾಂಕ AstCFunc* nodep) { ಹಿಂತಿರುಗಿ nodep->dpiExportDispatcher(); });
            ಆದರೆ (!funcps.ಖಾಲಿಗಳು()) {
                puts("\n/// DPI Export functions\n");
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        ಆದರೆ (v3Global.opt.savable()) {
            puts("\n");
            puts("// Serialization functions\n");
            puts("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::ತುದಿ_Class_Name() + "& rhs);\n");
            puts("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::ತುದಿ_Class_Name() + "& rhs);\n");
        }

        puts("\n// Abstract methods from VerilatedModel\n");
        puts("const char* hierName() const override final;\n");
        puts("const char* modelName() const override final;\n");
        puts("unsigned threads() const override final;\n");
        puts("/// Prepare for cloning the model at the process level (e.g. fork in Linux)\n");
        puts("/// Release necessary resources. Called before cloning.\n");
        puts("void prepareClone() const;\n");
        puts("/// Re-init after cloning the model at the process level (e.g. fork in Linux)\n");
        puts("/// Re-allocate necessary resources. Called after cloning.\n");
        puts("void atClone() const;\n");
        ಆದರೆ (v3Global.opt.trace()) {
            puts("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->puts_ಖಾಸಗಿ(ನಿಜ);  // private:
        puts("\n// Internal functions - the model's evaluation entry points\n");
        puts("void evalBegin() override final;\n");
        puts("void evalEnd() override final;\n");
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ VEval eval{i};
            // Only the iterated regions report whether they did any work
            puts(eval.isIterated() ? "bool "s : "void "s);
            puts(eval.eval_ವಿಧಾನ() + (eval.ಮೊದಲ_Iteration() ? "(bool firstIteration)"s : "()"s));
            puts(" override final;\n");
        }
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ VEval eval{i};
            ಆದರೆ (!eval.hasTriggers()) ಮುಂದುವರಿ;
            puts("void " + eval.dump_Triggers_ವಿಧಾನ() + "() override final;\n");
        }

        puts("\n// Internal functions - trace registration\n");
        puts("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        puts("};\n");

        ofp()->puts_ಕೊನೆ_Guard();

        closeOutputFile();
    }

    ಶೂನ್ಯ emitConstructorImplementation(Ast_ಗಂಟು_Module* modp) {
        putSectionDelimiter("Constructors");

        puts("\n");
        putns(modp, EmitCUtil::ತುದಿ_Class_Name() + "::" + EmitCUtil::ತುದಿ_Class_Name());
        ಆದರೆ (optSystemC()) {
            puts("(sc_core::sc_module_name /* unused */)\n");
            puts("    : VerilatedModel{*Verilated::threadContextp()}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            puts("    : VerilatedModel{*_vcontextp__}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        puts("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvt_ಗಮ್ಯ_Str(v3Global.opt.converge_ಎಲ್ಲೆ()) + "}\n");

        // Set up IO references
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isPrimaryIO()) {
                    ಸ್ಥಿರಾಂಕ string protName = varp->nameProtect();
                    puts("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell)) {
                ಆದರೆ (cellp->modp()->isConstPool()) ಮುಂದುವರಿ;  // Special emit rules
                ಸ್ಥಿರಾಂಕ string protName = cellp->nameProtect();
                puts("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        puts("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        puts("{\n");
        puts("// Register model with the context\n");
        puts("contextp()->addModel(this);\n");
        ಆದರೆ (v3Global.opt.profExec()) {
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "// Profile the evaluation loop's region loops\n");
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ತುದಿ_Level = !v3Global.opt.hier_ಮಗು() && v3Global.opt.libCreate().ಖಾಲಿಗಳು();
            puts("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (ತುದಿ_Level ? "true" : "false") + ");\n");
        }
        ಆದರೆ (v3Global.opt.trace())
            puts("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        ಆದರೆ (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "// Sensitivities on all clocks and combinational inputs\n");
            puts("SC_METHOD(eval);\n");
            ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Timing()) {
                putsDecoration(ಶೂನ್ಯಸೂಚಕ, "// Notified by pending time delays/DPI exports\n");
                puts("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                    ಆದರೆ (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        ಪೂರ್ಣಾಂಕ vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        ಗಾಗಿ (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            ಸ್ಥಿರಾಂಕ string ivar = "__Vi"s + cvt_ಗಮ್ಯ_Str(vecnum);
                            putns(varp, "for (int __Vi" + cvt_ಗಮ್ಯ_Str(vecnum) + " = "
                                            + cvt_ಗಮ್ಯ_Str(arrayp->lo()));
                            puts("; " + ivar + " <= " + cvt_ಗಮ್ಯ_Str(arrayp->hi()));
                            puts("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ v = 0; v < vects; ++v) puts("[__Vi" + cvt_ಗಮ್ಯ_Str(v) + "]");
                        puts(";\n");
                        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ v = 0; v < vects; ++v) puts("}\n");
                    }
                }
            }
            puts("\n");
        }

        puts("}\n");

        ಆದರೆ (!optSystemC()) {
            puts("\n");
            puts(EmitCUtil::ತುದಿ_Class_Name() + "::" + EmitCUtil::ತುದಿ_Class_Name()
                 + "(const char* _vcname__)\n");
            puts("    : " + EmitCUtil::ತುದಿ_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    ಶೂನ್ಯ emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        puts("\n");
        puts(EmitCUtil::ತುದಿ_Class_Name() + "::~" + EmitCUtil::ತುದಿ_Class_Name() + "() {\n");
        puts("delete vlSymsp;\n");
        puts("}\n");
    }

    ಶೂನ್ಯ emitStandardMethods1(Ast_ಗಂಟು_Module* modp) {
        UASSERT_OBJ(modp->is_ತುದಿ(), modp, "Attempting to emitWrapEval for non-top class");

        ಸ್ಥಿರಾಂಕ string ತುದಿ_Mod_Name_Protected = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
        ಸ್ಥಿರಾಂಕ string selfDecl = "(" + ತುದಿ_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        puts("\n");
        puts("#ifdef VL_DEBUG\n");
        puts("void " + ತುದಿ_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        puts("#endif  // VL_DEBUG\n");
        AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ VEval eval{i};
            ಆದರೆ (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        ಆದರೆ (optSystemC() && v3Global.ಬಳಸುತ್ತದೆ_Timing()) {
            // ::eval
            puts("\nvoid " + EmitCUtil::ತುದಿ_Class_Name() + "::eval() {\n");
            puts("eval_step();\n");
            puts("if (eventsPending()) {\n");
            puts("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            puts("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            puts("}\n");
            puts("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        puts("\nvoid " + EmitCUtil::ತುದಿ_Class_Name() + "::eval_step() {\n");
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::ತುದಿ_Class_Name()
             + "::eval_step\\n\"); );\n");
        puts("m_evalLoop.eval();\n");
        puts("}\n");

        // ::evalBegin - prepare the model for a time step
        puts("\nvoid " + EmitCUtil::ತುದಿ_Class_Name() + "::evalBegin() {\n");
        puts("#ifdef VL_DEBUG\n");
        putsDecoration(ಶೂನ್ಯಸೂಚಕ, "// Debug assertions\n");
        puts(ತುದಿ_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        puts("#endif  // VL_DEBUG\n");

        ಆದರೆ (v3Global.opt.trace()) puts("vlSymsp->__Vm_activity = true;\n");

        ಆದರೆ (v3Global.hasEvents()) puts("vlSymsp->clearTriggeredEvents();\n");
        ಆದರೆ (v3Global.hasClasses()) puts("vlSymsp->__Vm_deleter.deleteAll();\n");

        puts("}\n");

        // ::evalEnd - the time step is complete
        puts("\nvoid " + EmitCUtil::ತುದಿ_Class_Name() + "::evalEnd() {\n");
        putsDecoration(ಶೂನ್ಯಸೂಚಕ, "// Evaluate cleanup\n");
        puts("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            puts("vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".cleanupForevered();\n");
        }

        puts("}\n");

        // Evaluation entry points
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ VEval eval{i};
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = v3Global.rootp()->evalFuncp(eval);
            puts(eval.isIterated() ? "\nbool " : "\nvoid ");
            puts(EmitCUtil::ತುದಿ_Class_Name() + "::" + eval.eval_ವಿಧಾನ() + "(");
            ಆದರೆ (eval.ಮೊದಲ_Iteration()) puts("bool firstIteration");
            puts(") {\n");
            ಆದರೆ (eval.isIterated()) puts("return ");
            puts(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            ಆದರೆ (eval.ಮೊದಲ_Iteration()) puts(", firstIteration");
            puts(");\n");
            puts("}\n");
        }

        // The trigger dump of each region
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ VEval eval{i};
            ಆದರೆ (!eval.hasTriggers()) ಮುಂದುವರಿ;
            puts("\nVL_ATTR_COLD void " + EmitCUtil::ತುದಿ_Class_Name()
                 + "::" + eval.dump_Triggers_ವಿಧಾನ() + "() {\n");
            puts(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            puts("}\n");
        }
    }

    ಶೂನ್ಯ emitStandardMethods2(Ast_ಗಂಟು_Module* modp) {
        // ::eval_end_step
        ಆದರೆ (v3Global.needTraceDumper() && !optSystemC()) {
            puts("\n");
            putns(modp, "void " + EmitCUtil::ತುದಿ_Class_Name() + "::eval_end_step() {\n");
            puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::ತುದಿ_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            puts("#ifdef VM_TRACE\n");
            putsDecoration(ಶೂನ್ಯಸೂಚಕ, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            puts("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            puts("#endif  // VM_TRACE\n");
            puts("}\n");
        }

        putSectionDelimiter("Events and timing");
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::ತುದಿ_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::ತುದಿ_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".nextTimeSlot(); }\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(modp,
                  "bool " + EmitCUtil::ತುದಿ_Class_Name() + "::eventsPending() { return false; }\n\n");
            puts("uint64_t " + EmitCUtil::ತುದಿ_Class_Name() + "::nextTimeSlot() {\n");
            puts("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            puts("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        ಆದರೆ (!optSystemC()) {
            // ::name
            puts("\n");
            putns(modp, "const char* " + EmitCUtil::ತುದಿ_Class_Name() + "::name() const {\n");
            puts(/**/ "return vlSymsp->name();\n");
            puts("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        puts("\nVL_ATTR_COLD void " + EmitCUtil::ತುದಿ_Class_Name() + "::final() {\n");
        puts("contextp()->executingFinal(true);\n");
        puts("evalFinal();\n");
        puts("contextp()->executingFinal(false);\n");
        puts("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::ತುದಿ_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::ತುದಿ_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::ತುದಿ_Class_Name() + "\"; }\n");
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಎಳೆಗಳು = v3Global.opt.hier_ಮಗು()
                                ? v3Global.opt.ಎಳೆಗಳು()
                                : std::max(v3Global.opt.ಎಳೆಗಳು(), v3Global.opt.hier_ಎಳೆಗಳು());
        putns(modp, "unsigned " + EmitCUtil::ತುದಿ_Class_Name() + "::threads() const { return "
                        + cvt_ಗಮ್ಯ_Str(ಎಳೆಗಳು) + "; }\n");
        putns(modp, "void " + EmitCUtil::ತುದಿ_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::ತುದಿ_Class_Name() + "::atClone() const {\n");
        ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) {
            puts("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        puts("contextp()->threadPoolpOnClone()");
        ಆದರೆ (v3Global.opt.ಎಳೆಗಳು() > 1) puts(")");
        puts(";\n}\n");

        ಆದರೆ (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::ತುದಿ_Class_Name()
                            + "::traceConfig() const {\n");
            puts("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            puts(v3Global.opt.useTraceParallel() ? "true" : "false");
            puts("}};\n");
            puts("};\n");
        }
    }

    ಶೂನ್ಯ emitTraceMethods(Ast_ಗಂಟು_Module* modp) {
        ಸ್ಥಿರಾಂಕ string ತುದಿ_Mod_Name_Protected = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
        ಸ್ಥಿರಾಂಕ string ತುದಿ_Trace_Name
            = V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(v3Global.rootp()->trace_Lib_ತುದಿ_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            putns(modp, "\nvoid " + ತುದಿ_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + ತುದಿ_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_ತಳ()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + ತುದಿ_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.trace_Class_ತಳ() + "* tracep);\n");
        putns(modp, "\nvoid " + ತುದಿ_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + ತುದಿ_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_ತಳ()
                        + "* tracep);\n");

        // Static helper function
        puts("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.trace_Class_ತಳ() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        puts(EmitCUtil::void_Self_ನಿಯೋಜಿಸು(modp));
        puts(EmitCUtil::sym_Class_ನಿಯೋಜಿಸು());
        puts("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        puts("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        puts("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        puts("}\n");
        puts("vlSymsp->__Vm_baseCode = code;\n");
        ಆದರೆ (v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts(ತುದಿ_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            puts("tracep->pushPrefix(\"" + ತುದಿ_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts("}\n");
        }
        puts(ತುದಿ_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        puts(ತುದಿ_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        ಆದರೆ (v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {  //
            puts("tracep->popPrefix();\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->popPrefix();\n");
            puts("tracep->popPrefix();\n");
            puts("}\n");
        }
        puts("}\n");

        // Forward declaration
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + ತುದಿ_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + ತುದಿ_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_ತಳ()
                        + "* tracep);\n");

        // ::traceRegisterModel
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::ತುದಿ_Class_Name() + "::traceBaseModel(");
        puts("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        ಆದರೆ (optSystemC()) {
            puts(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            puts(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::ತುದಿ_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            puts(/**/ "}");
        }
        puts(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        puts(/**/ v3Global.opt.trace_Class_ತಳ() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.trace_Class_ತಳ() + "C*>(tfp);\n");
        puts(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        puts(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::ತುದಿ_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.trace_Class_ತಳ() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        puts(/**/ "}\n");
        puts(/**/ "stfp->spTrace()->addModel(this);\n");
        puts(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().ಖಾಲಿಗಳು() ? "false" : "true")  //
             + ", " + std::ಗಮ್ಯ_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        puts(/**/ ತುದಿ_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        puts("}\n");
    }

    ಶೂನ್ಯ emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        puts("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::ತುದಿ_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");

        puts("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::ತುದಿ_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");
    }

    ಶೂನ್ಯ emitImplementation(Ast_ಗಂಟು_Module* modp) {
        open_New_Output_ಮೂಲ_File(EmitCUtil::ತುದಿ_Class_Name(), ಸುಳ್ಳು, ಸುಳ್ಳು,
                                "Model implementation (design independent parts)");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ತಳ : v3Global.opt.trace_ಮೂಲ_Langs())
            puts("#include \"" + ತಳ + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        ಆದರೆ (v3Global.opt.trace()) emitTraceMethods(modp);
        ಆದರೆ (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    ಶೂನ್ಯ emitDpiExportDispatchers(Ast_ಗಂಟು_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3_Unique_ಹೆಸರುಗಳು unique_ಹೆಸರುಗಳು;
        ಸ್ಥಿರಾಂಕ std::string file_ತಳ_Name = EmitCUtil::ತುದಿ_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc);
            ಆದರೆ (!funcp || !funcp->dpiExportDispatcher()) ಮುಂದುವರಿ;

            ಆದರೆ (ವಿಭಜಿಸು_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ನಿಜ);
                // Close old file
                closeOutputFile();
            }

            ಆದರೆ (!ofp()) {
                open_New_Output_ಮೂಲ_File(unique_ಹೆಸರುಗಳು.get(file_ತಳ_Name), ಸುಳ್ಳು, ಸುಳ್ಳು,
                                        "Implementation of DPI export functions.");
                puts("\n");
                puts("#include \"" + EmitCUtil::ತುದಿ_Class_Name() + ".h\"\n");
                puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                puts("#include \"verilated_dpi.h\"\n");
                puts("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        ಆದರೆ (ofp()) closeOutputFile();
    }

    ಶೂನ್ಯ ಮುಖ್ಯ(Ast_ಗಂಟು_Module* modp) {
        m_modp = modp;
        emit_ಶೀರ್ಷಿಕೆ(modp);
        emitImplementation(modp);
        ಆದರೆ (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ EmitCModel(Ast_ಗಂಟು_Module* ತುದಿ_Modulep) { ಮುಖ್ಯ(ತುದಿ_Modulep); }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->ಕೂಡಿಸು_Eval_Stats("emitc");
    { EmitCModel{v3Global.rootp()->ತುದಿ_Modulep()}; }
}
