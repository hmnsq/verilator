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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCFunc.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ EmitCModel final : ਜਨਤਕ EmitCFunc {
    // TYPES
    ਵਰਤੋਂ CFunc_ਸਦਿਸ਼ = std::ਸਦਿਸ਼<ਅਚਲ AstCFunc*>;

    // MEMBERS
    V3_Unique_ਨਾਂ m_unique_ਨਾਂ;  // For generating unique file names

    // METHODS

    CFunc_ਸਦਿਸ਼ ਲੱਭੋ_Funcps(std::function<ਤਾਰਕਿਕ(ਅਚਲ AstCFunc*)> cb) {
        CFunc_ਸਦਿਸ਼ funcps;
        ਲਈ (Ast_ਗੰਢ* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc)) {
                ਜੇ (cb(funcp)) funcps.ਧੱਕੋ_ਪਿੱਠ(funcp);
            }
        }
        stable_ਛਾਂਟੋ(funcps.begin(), funcps.ਸਿਰਾ(),
                    [](ਅਚਲ Ast_ਗੰਢ* ap, ਅਚਲ Ast_ਗੰਢ* bp) { ਵਾਪਸ ap->name() < bp->name(); });
        ਵਾਪਸ funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    ਸਥਿਰ ਤਾਰਕਿਕ isDpiPassingStructs(ਅਚਲ AstCFunc* funcp) {
        ਜੇ (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) ਵਾਪਸ ਝੂਠ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(argp, Var);
            ਜੇ (varp && varp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ putSectionDelimiter(ਅਚਲ string& name) {
        puts("\n");
        puts("//============================================================\n");
        puts("// " + name + "\n");
    }

    ਖਾਲੀ emit_ਸਿਰਲੇਖ(Ast_ਗੰਢ_Module* modp) {
        open_New_Output_ਸਿਰਲੇਖ_File(EmitCUtil::ਸਿਖਰ_Class_Name(), "Primary model header");
        puts("//\n");
        puts("// This header should be included by all source files instantiating the design.\n");
        puts("// The class here is then constructed to instantiate the design.\n");
        puts("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ਸਿਖਰ_Include();

        puts("#include \"verilated.h\"\n");
        ਜੇ (v3Global.opt.systemC()) puts("#include \"verilated_sc.h\"\n");
        ਜੇ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ਜੇ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ਜੇ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ਜੇ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ਜੇ (v3Global.dpi()) puts("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        ਜੇ (!ਲੱਭੋ_Funcps(isDpiPassingStructs).ਖਾਲੀ_ਥਾਵਾਂ()) {
            puts("#include \"" + EmitCUtil::ਸਿਖਰ_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        puts("\n");
        puts("class " + EmitCUtil::symClassName() + ";\n");
        puts("class " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp) + ";\n");  // For rootp pointer only
        ਲਈ (ਅਚਲ string& ਅਧਾਰ : v3Global.opt.traceClassLangs()) puts("class " + ਅਧਾਰ + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        puts("\n");

        puts("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::ਸਿਖਰ_Class_Name()
                        + " VL_NOT_FINAL : ");
        ਜੇ (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            puts("public ::sc_core::sc_module, ");
        }
        puts("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        puts("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->reset_ਨਿੱਜੀ();
        ofp()->puts_ਨਿੱਜੀ(ਸੱਚ);  // private:

        puts("// Symbol table holding complete model state (owned by this class)\n");
        puts(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        puts("// Evaluation loop\n");
        puts("VerilatedEvalLoop m_evalLoop;\n");

        puts("\n");
        ofp()->puts_ਨਿੱਜੀ(ਝੂਠ);  // public:

        puts("\n// CONSTEXPR CAPABILITIES\n");
        puts("// Verilated with --trace?\n");
        puts("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        puts("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ ਸੱਚ);
                }
            }
        }
        ਜੇ (v3Global.opt.emitAccessors()) {
            puts("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                    ਜੇ (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        puts("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell)) {
                ਜੇ (cellp->modp()->isConstPool()) ਜਾਰੀ;  // Special emit rules
                putns(cellp, EmitCUtil::ਅਗੇਤਰ_Name_Protect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        puts("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        puts(EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp) + "* const rootp;\n");

        puts("\n");
        ofp()->puts_ਨਿੱਜੀ(ਝੂਠ);  // public:
        puts("// CONSTRUCTORS\n");
        ਜੇ (optSystemC()) {
            puts("SC_CTOR(" + EmitCUtil::ਸਿਖਰ_Class_Name() + ");\n");
            puts("virtual ~" + EmitCUtil::ਸਿਖਰ_Class_Name() + "();\n");
        } ਨਹੀਂਤਾਂ {
            puts("/// Construct the model; called by application code\n");
            puts("/// If contextp is null, then the model will use the default global "
                 "context\n");
            puts("/// If name is \"\", then makes a wrapper with a\n");
            puts("/// single model invisible with respect to DPI scope names.\n");
            puts("explicit " + EmitCUtil::ਸਿਖਰ_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            puts("explicit " + EmitCUtil::ਸਿਖਰ_Class_Name() + "(const char* name = \"TOP\");\n");
            puts("/// Destroy the model; called (often implicitly) by application code\n");
            puts("virtual ~" + EmitCUtil::ਸਿਖਰ_Class_Name() + "();\n");
        }
        ofp()->puts_ਨਿੱਜੀ(ਸੱਚ);
        puts("VL_UNCOPYABLE(" + EmitCUtil::ਸਿਖਰ_Class_Name() + ");  ///< Copying not allowed\n");

        puts("\n");
        ofp()->puts_ਨਿੱਜੀ(ਝੂਠ);  // public:
        puts("// API METHODS\n");
        ਅਚਲ string call_Eval_ਸਿਰਾ_ਕਦਮ
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        ਜੇ (optSystemC()) {
            ofp()->puts_ਨਿੱਜੀ(ਸੱਚ);  ///< eval() is invoked by our sensitive() calls.
        }
        ਜੇ (!optSystemC()) {
            puts("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        ਜੇ (optSystemC() && v3Global.ਵਰਤਦਾ_Timing()) {
            puts("void eval();\n");
        } ਨਹੀਂਤਾਂ {
            puts("void eval() { eval_step(); " + call_Eval_ਸਿਰਾ_ਕਦਮ + "}\n");
        }
        ਜੇ (!optSystemC()) {
            puts("/// Evaluate when calling multiple units/models per time step.\n");
        }
        puts("void eval_step();\n");
        ਜੇ (!optSystemC()) {
            puts("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            puts("/// Application must call after all eval() and before time changes.\n");
            puts("void eval_end_step()");
            ਜੇ (call_Eval_ਸਿਰਾ_ਕਦਮ == "") {
                puts(" {}\n");
            } ਨਹੀਂਤਾਂ {
                puts(";\n");
            }
        }
        ਜੇ (!optSystemC()) {
            puts("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->puts_ਨਿੱਜੀ(ਝੂਠ);  // public:
        puts("void final();\n");

        puts("/// Are there scheduled events to handle?\n");
        puts("bool eventsPending();\n");
        puts("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        puts("uint64_t nextTimeSlot();\n");

        ਜੇ (v3Global.opt.trace() || !optSystemC()) {
            puts("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            puts("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        ਜੇ (v3Global.opt.trace() && optSystemC()) {
            puts("/// SC tracing; avoid overloaded virtual function lint warning\n");
            puts("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        ਜੇ (!optSystemC()) {
            puts("/// Retrieve name of this model instance (as passed to constructor).\n");
            puts("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            ਅਚਲ CFunc_ਸਦਿਸ਼ funcps
                = ਲੱਭੋ_Funcps([](ਅਚਲ AstCFunc* nodep) { ਵਾਪਸ nodep->dpiExportDispatcher(); });
            ਜੇ (!funcps.ਖਾਲੀ_ਥਾਵਾਂ()) {
                puts("\n/// DPI Export functions\n");
                ਲਈ (ਅਚਲ AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        ਜੇ (v3Global.opt.savable()) {
            puts("\n");
            puts("// Serialization functions\n");
            puts("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::ਸਿਖਰ_Class_Name() + "& rhs);\n");
            puts("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::ਸਿਖਰ_Class_Name() + "& rhs);\n");
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
        ਜੇ (v3Global.opt.trace()) {
            puts("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->puts_ਨਿੱਜੀ(ਸੱਚ);  // private:
        puts("\n// Internal functions - the model's evaluation entry points\n");
        puts("void evalBegin() override final;\n");
        puts("void evalEnd() override final;\n");
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            ਅਚਲ VEval eval{i};
            // Only the iterated regions report whether they did any work
            puts(eval.isIterated() ? "bool "s : "void "s);
            puts(eval.eval_ਵਿਧੀ() + (eval.ਪਹਿਲਾ_Iteration() ? "(bool firstIteration)"s : "()"s));
            puts(" override final;\n");
        }
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            ਅਚਲ VEval eval{i};
            ਜੇ (!eval.hasTriggers()) ਜਾਰੀ;
            puts("void " + eval.dump_Triggers_ਵਿਧੀ() + "() override final;\n");
        }

        puts("\n// Internal functions - trace registration\n");
        puts("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        puts("};\n");

        ofp()->puts_ਸਿਰਾ_Guard();

        closeOutputFile();
    }

    ਖਾਲੀ emitConstructorImplementation(Ast_ਗੰਢ_Module* modp) {
        putSectionDelimiter("Constructors");

        puts("\n");
        putns(modp, EmitCUtil::ਸਿਖਰ_Class_Name() + "::" + EmitCUtil::ਸਿਖਰ_Class_Name());
        ਜੇ (optSystemC()) {
            puts("(sc_core::sc_module_name /* unused */)\n");
            puts("    : VerilatedModel{*Verilated::threadContextp()}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } ਨਹੀਂਤਾਂ {
            puts("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            puts("    : VerilatedModel{*_vcontextp__}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        puts("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvt_ਮੰਜ਼ਿਲ_Str(v3Global.opt.converge_ਹੱਦਬੰਦੀ()) + "}\n");

        // Set up IO references
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isPrimaryIO()) {
                    ਅਚਲ string protName = varp->nameProtect();
                    puts("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell)) {
                ਜੇ (cellp->modp()->isConstPool()) ਜਾਰੀ;  // Special emit rules
                ਅਚਲ string protName = cellp->nameProtect();
                puts("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        puts("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        puts("{\n");
        puts("// Register model with the context\n");
        puts("contextp()->addModel(this);\n");
        ਜੇ (v3Global.opt.profExec()) {
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "// Profile the evaluation loop's region loops\n");
            ਅਚਲ ਤਾਰਕਿਕ ਸਿਖਰ_Level = !v3Global.opt.hier_ਬੱਚਾ() && v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ();
            puts("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (ਸਿਖਰ_Level ? "true" : "false") + ");\n");
        }
        ਜੇ (v3Global.opt.trace())
            puts("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        ਜੇ (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "// Sensitivities on all clocks and combinational inputs\n");
            puts("SC_METHOD(eval);\n");
            ਜੇ (v3Global.ਵਰਤਦਾ_Timing()) {
                putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "// Notified by pending time delays/DPI exports\n");
                puts("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                    ਜੇ (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        ਪੂਰਨ vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        ਲਈ (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            ਅਚਲ ਪੂਰਨ vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            ਅਚਲ string ivar = "__Vi"s + cvt_ਮੰਜ਼ਿਲ_Str(vecnum);
                            putns(varp, "for (int __Vi" + cvt_ਮੰਜ਼ਿਲ_Str(vecnum) + " = "
                                            + cvt_ਮੰਜ਼ਿਲ_Str(arrayp->lo()));
                            puts("; " + ivar + " <= " + cvt_ਮੰਜ਼ਿਲ_Str(arrayp->hi()));
                            puts("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        ਲਈ (ਪੂਰਨ v = 0; v < vects; ++v) puts("[__Vi" + cvt_ਮੰਜ਼ਿਲ_Str(v) + "]");
                        puts(";\n");
                        ਲਈ (ਪੂਰਨ v = 0; v < vects; ++v) puts("}\n");
                    }
                }
            }
            puts("\n");
        }

        puts("}\n");

        ਜੇ (!optSystemC()) {
            puts("\n");
            puts(EmitCUtil::ਸਿਖਰ_Class_Name() + "::" + EmitCUtil::ਸਿਖਰ_Class_Name()
                 + "(const char* _vcname__)\n");
            puts("    : " + EmitCUtil::ਸਿਖਰ_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    ਖਾਲੀ emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        puts("\n");
        puts(EmitCUtil::ਸਿਖਰ_Class_Name() + "::~" + EmitCUtil::ਸਿਖਰ_Class_Name() + "() {\n");
        puts("delete vlSymsp;\n");
        puts("}\n");
    }

    ਖਾਲੀ emitStandardMethods1(Ast_ਗੰਢ_Module* modp) {
        UASSERT_OBJ(modp->is_ਸਿਖਰ(), modp, "Attempting to emitWrapEval for non-top class");

        ਅਚਲ string ਸਿਖਰ_Mod_Name_Protected = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
        ਅਚਲ string selfDecl = "(" + ਸਿਖਰ_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        puts("\n");
        puts("#ifdef VL_DEBUG\n");
        puts("void " + ਸਿਖਰ_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        puts("#endif  // VL_DEBUG\n");
        AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            ਅਚਲ VEval eval{i};
            ਜੇ (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        ਜੇ (optSystemC() && v3Global.ਵਰਤਦਾ_Timing()) {
            // ::eval
            puts("\nvoid " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::eval() {\n");
            puts("eval_step();\n");
            puts("if (eventsPending()) {\n");
            puts("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            puts("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            puts("}\n");
            puts("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        puts("\nvoid " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::eval_step() {\n");
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::ਸਿਖਰ_Class_Name()
             + "::eval_step\\n\"); );\n");
        puts("m_evalLoop.eval();\n");
        puts("}\n");

        // ::evalBegin - prepare the model for a time step
        puts("\nvoid " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::evalBegin() {\n");
        puts("#ifdef VL_DEBUG\n");
        putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "// Debug assertions\n");
        puts(ਸਿਖਰ_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        puts("#endif  // VL_DEBUG\n");

        ਜੇ (v3Global.opt.trace()) puts("vlSymsp->__Vm_activity = true;\n");

        ਜੇ (v3Global.hasEvents()) puts("vlSymsp->clearTriggeredEvents();\n");
        ਜੇ (v3Global.hasClasses()) puts("vlSymsp->__Vm_deleter.deleteAll();\n");

        puts("}\n");

        // ::evalEnd - the time step is complete
        puts("\nvoid " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::evalEnd() {\n");
        putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "// Evaluate cleanup\n");
        puts("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        ਜੇ (AstVar* ਅਚਲ delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            puts("vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".cleanupForevered();\n");
        }

        puts("}\n");

        // Evaluation entry points
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            ਅਚਲ VEval eval{i};
            AstCFunc* ਅਚਲ funcp = v3Global.rootp()->evalFuncp(eval);
            puts(eval.isIterated() ? "\nbool " : "\nvoid ");
            puts(EmitCUtil::ਸਿਖਰ_Class_Name() + "::" + eval.eval_ਵਿਧੀ() + "(");
            ਜੇ (eval.ਪਹਿਲਾ_Iteration()) puts("bool firstIteration");
            puts(") {\n");
            ਜੇ (eval.isIterated()) puts("return ");
            puts(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            ਜੇ (eval.ਪਹਿਲਾ_Iteration()) puts(", firstIteration");
            puts(");\n");
            puts("}\n");
        }

        // The trigger dump of each region
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            ਅਚਲ VEval eval{i};
            ਜੇ (!eval.hasTriggers()) ਜਾਰੀ;
            puts("\nVL_ATTR_COLD void " + EmitCUtil::ਸਿਖਰ_Class_Name()
                 + "::" + eval.dump_Triggers_ਵਿਧੀ() + "() {\n");
            puts(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            puts("}\n");
        }
    }

    ਖਾਲੀ emitStandardMethods2(Ast_ਗੰਢ_Module* modp) {
        // ::eval_end_step
        ਜੇ (v3Global.needTraceDumper() && !optSystemC()) {
            puts("\n");
            putns(modp, "void " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::eval_end_step() {\n");
            puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::ਸਿਖਰ_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            puts("#ifdef VM_TRACE\n");
            putsDecoration(ਖਾਲੀ_ਸੂਚਕ, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            puts("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            puts("#endif  // VM_TRACE\n");
            puts("}\n");
        }

        putSectionDelimiter("Events and timing");
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::ਸਿਖਰ_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::ਸਿਖਰ_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".nextTimeSlot(); }\n");
        } ਨਹੀਂਤਾਂ {
            putns(modp,
                  "bool " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::eventsPending() { return false; }\n\n");
            puts("uint64_t " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::nextTimeSlot() {\n");
            puts("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            puts("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        ਜੇ (!optSystemC()) {
            // ::name
            puts("\n");
            putns(modp, "const char* " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::name() const {\n");
            puts(/**/ "return vlSymsp->name();\n");
            puts("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        puts("\nVL_ATTR_COLD void " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::final() {\n");
        puts("contextp()->executingFinal(true);\n");
        puts("evalFinal();\n");
        puts("contextp()->executingFinal(false);\n");
        puts("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::ਸਿਖਰ_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::ਸਿਖਰ_Class_Name() + "\"; }\n");
        ਅਚਲ ਪੂਰਨ ਧਾਗੇ = v3Global.opt.hier_ਬੱਚਾ()
                                ? v3Global.opt.ਧਾਗੇ()
                                : std::max(v3Global.opt.ਧਾਗੇ(), v3Global.opt.hier_ਧਾਗੇ());
        putns(modp, "unsigned " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::threads() const { return "
                        + cvt_ਮੰਜ਼ਿਲ_Str(ਧਾਗੇ) + "; }\n");
        putns(modp, "void " + EmitCUtil::ਸਿਖਰ_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::atClone() const {\n");
        ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) {
            puts("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        puts("contextp()->threadPoolpOnClone()");
        ਜੇ (v3Global.opt.ਧਾਗੇ() > 1) puts(")");
        puts(";\n}\n");

        ਜੇ (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::ਸਿਖਰ_Class_Name()
                            + "::traceConfig() const {\n");
            puts("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            puts(v3Global.opt.useTraceParallel() ? "true" : "false");
            puts("}};\n");
            puts("};\n");
        }
    }

    ਖਾਲੀ emitTraceMethods(Ast_ਗੰਢ_Module* modp) {
        ਅਚਲ string ਸਿਖਰ_Mod_Name_Protected = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
        ਅਚਲ string ਸਿਖਰ_Trace_Name
            = V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(v3Global.rootp()->trace_Lib_ਸਿਖਰ_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            putns(modp, "\nvoid " + ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + ਸਿਖਰ_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_ਅਧਾਰ()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep);\n");
        putns(modp, "\nvoid " + ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + ਸਿਖਰ_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_ਅਧਾਰ()
                        + "* tracep);\n");

        // Static helper function
        puts("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        puts(EmitCUtil::void_Self_ਸੌਂਪੋ(modp));
        puts(EmitCUtil::sym_Class_ਸੌਂਪੋ());
        puts("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        puts("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        puts("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        puts("}\n");
        puts("vlSymsp->__Vm_baseCode = code;\n");
        ਜੇ (v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } ਨਹੀਂਤਾਂ {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts(ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            puts("tracep->pushPrefix(\"" + ਸਿਖਰ_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts("}\n");
        }
        puts(ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        puts(ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        ਜੇ (v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {  //
            puts("tracep->popPrefix();\n");
        } ਨਹੀਂਤਾਂ {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->popPrefix();\n");
            puts("tracep->popPrefix();\n");
            puts("}\n");
        }
        puts("}\n");

        // Forward declaration
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + ਸਿਖਰ_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_ਅਧਾਰ()
                        + "* tracep);\n");

        // ::traceRegisterModel
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::ਸਿਖਰ_Class_Name() + "::traceBaseModel(");
        puts("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        ਜੇ (optSystemC()) {
            puts(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            puts(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::ਸਿਖਰ_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            puts(/**/ "}");
        }
        puts(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        puts(/**/ v3Global.opt.trace_Class_ਅਧਾਰ() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.trace_Class_ਅਧਾਰ() + "C*>(tfp);\n");
        puts(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        puts(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::ਸਿਖਰ_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.trace_Class_ਅਧਾਰ() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        puts(/**/ "}\n");
        puts(/**/ "stfp->spTrace()->addModel(this);\n");
        puts(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ() ? "false" : "true")  //
             + ", " + std::ਮੰਜ਼ਿਲ_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        puts(/**/ ਸਿਖਰ_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        puts("}\n");
    }

    ਖਾਲੀ emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        puts("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::ਸਿਖਰ_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");

        puts("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::ਸਿਖਰ_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");
    }

    ਖਾਲੀ emitImplementation(Ast_ਗੰਢ_Module* modp) {
        open_New_Output_ਸਰੋਤ_File(EmitCUtil::ਸਿਖਰ_Class_Name(), ਝੂਠ, ਝੂਠ,
                                "Model implementation (design independent parts)");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        ਲਈ (ਅਚਲ string& ਅਧਾਰ : v3Global.opt.trace_ਸਰੋਤ_Langs())
            puts("#include \"" + ਅਧਾਰ + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        ਜੇ (v3Global.opt.trace()) emitTraceMethods(modp);
        ਜੇ (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    ਖਾਲੀ emitDpiExportDispatchers(Ast_ਗੰਢ_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3_Unique_ਨਾਂ unique_ਨਾਂ;
        ਅਚਲ std::string file_ਅਧਾਰ_Name = EmitCUtil::ਸਿਖਰ_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        ਲਈ (Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc);
            ਜੇ (!funcp || !funcp->dpiExportDispatcher()) ਜਾਰੀ;

            ਜੇ (ਵੰਡੋ_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(ਸੱਚ);
                // Close old file
                closeOutputFile();
            }

            ਜੇ (!ofp()) {
                open_New_Output_ਸਰੋਤ_File(unique_ਨਾਂ.get(file_ਅਧਾਰ_Name), ਝੂਠ, ਝੂਠ,
                                        "Implementation of DPI export functions.");
                puts("\n");
                puts("#include \"" + EmitCUtil::ਸਿਖਰ_Class_Name() + ".h\"\n");
                puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                puts("#include \"verilated_dpi.h\"\n");
                puts("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        ਜੇ (ofp()) closeOutputFile();
    }

    ਖਾਲੀ ਮੁੱਖ(Ast_ਗੰਢ_Module* modp) {
        m_modp = modp;
        emit_ਸਿਰਲੇਖ(modp);
        emitImplementation(modp);
        ਜੇ (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

ਜਨਤਕ:
    ਸਪਸ਼ਟ EmitCModel(Ast_ਗੰਢ_Module* ਸਿਖਰ_Modulep) { ਮੁੱਖ(ਸਿਖਰ_Modulep); }
};

//######################################################################
// EmitC class functions

ਖਾਲੀ V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->ਜੋੜੋ_Eval_Stats("emitc");
    { EmitCModel{v3Global.rootp()->ਸਿਖਰ_Modulep()}; }
}
