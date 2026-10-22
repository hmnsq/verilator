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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCFunc.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <functional>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి EmitCModel final : బహిరంగ EmitCFunc {
    // TYPES
    ఉపయోగించి CFunc_సదిశ = std::సదిశ<స్థిరరాశి AstCFunc*>;

    // MEMBERS
    V3_Unique_పేర్లు m_unique_పేర్లు;  // For generating unique file names

    // METHODS

    CFunc_సదిశ కనుగొను_Funcps(std::function<తార్కిక(స్థిరరాశి AstCFunc*)> cb) {
        CFunc_సదిశ funcps;
        కొరకు (Ast_కణుపు* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc)) {
                ఒకవేళ (cb(funcp)) funcps.నెట్టు_వెనుక(funcp);
            }
        }
        stable_క్రమీకరించు(funcps.begin(), funcps.చివర(),
                    [](స్థిరరాశి Ast_కణుపు* ap, స్థిరరాశి Ast_కణుపు* bp) { తిరిగి ap->name() < bp->name(); });
        తిరిగి funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    స్థిర తార్కిక isDpiPassingStructs(స్థిరరాశి AstCFunc* funcp) {
        ఒకవేళ (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) తిరిగి అబద్ధం;
        కొరకు (స్థిరరాశి Ast_కణుపు* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(argp, Var);
            ఒకవేళ (varp && varp->dtypep()->కలిగియుంది_Unpacked_Struct()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    శూన్య putSectionDelimiter(స్థిరరాశి string& name) {
        puts("\n");
        puts("//============================================================\n");
        puts("// " + name + "\n");
    }

    శూన్య emit_శీర్షిక(Ast_కణుపు_Module* modp) {
        open_New_Output_శీర్షిక_File(EmitCUtil::శిఖరం_Class_Name(), "Primary model header");
        puts("//\n");
        puts("// This header should be included by all source files instantiating the design.\n");
        puts("// The class here is then constructed to instantiate the design.\n");
        puts("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_శిఖరం_Include();

        puts("#include \"verilated.h\"\n");
        ఒకవేళ (v3Global.opt.systemC()) puts("#include \"verilated_sc.h\"\n");
        ఒకవేళ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ఒకవేళ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ఒకవేళ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ఒకవేళ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ఒకవేళ (v3Global.dpi()) puts("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        ఒకవేళ (!కనుగొను_Funcps(isDpiPassingStructs).ఖాళీలు()) {
            puts("#include \"" + EmitCUtil::శిఖరం_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        puts("\n");
        puts("class " + EmitCUtil::symClassName() + ";\n");
        puts("class " + EmitCUtil::ఉపసర్గ_Name_Protect(modp) + ";\n");  // For rootp pointer only
        కొరకు (స్థిరరాశి string& పునాది : v3Global.opt.traceClassLangs()) puts("class " + పునాది + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        puts("\n");

        puts("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::శిఖరం_Class_Name()
                        + " VL_NOT_FINAL : ");
        ఒకవేళ (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            puts("public ::sc_core::sc_module, ");
        }
        puts("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        puts("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->reset_వ్యక్తిగత();
        ofp()->puts_వ్యక్తిగత(నిజం);  // private:

        puts("// Symbol table holding complete model state (owned by this class)\n");
        puts(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        puts("// Evaluation loop\n");
        puts("VerilatedEvalLoop m_evalLoop;\n");

        puts("\n");
        ofp()->puts_వ్యక్తిగత(అబద్ధం);  // public:

        puts("\n// CONSTEXPR CAPABILITIES\n");
        puts("// Verilated with --trace?\n");
        puts("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        puts("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ నిజం);
                }
            }
        }
        ఒకవేళ (v3Global.opt.emitAccessors()) {
            puts("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                    ఒకవేళ (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        puts("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell)) {
                ఒకవేళ (cellp->modp()->isConstPool()) కొనసాగు;  // Special emit rules
                putns(cellp, EmitCUtil::ఉపసర్గ_Name_Protect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        puts("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        puts(EmitCUtil::ఉపసర్గ_Name_Protect(modp) + "* const rootp;\n");

        puts("\n");
        ofp()->puts_వ్యక్తిగత(అబద్ధం);  // public:
        puts("// CONSTRUCTORS\n");
        ఒకవేళ (optSystemC()) {
            puts("SC_CTOR(" + EmitCUtil::శిఖరం_Class_Name() + ");\n");
            puts("virtual ~" + EmitCUtil::శిఖరం_Class_Name() + "();\n");
        } లేకపోతే {
            puts("/// Construct the model; called by application code\n");
            puts("/// If contextp is null, then the model will use the default global "
                 "context\n");
            puts("/// If name is \"\", then makes a wrapper with a\n");
            puts("/// single model invisible with respect to DPI scope names.\n");
            puts("explicit " + EmitCUtil::శిఖరం_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            puts("explicit " + EmitCUtil::శిఖరం_Class_Name() + "(const char* name = \"TOP\");\n");
            puts("/// Destroy the model; called (often implicitly) by application code\n");
            puts("virtual ~" + EmitCUtil::శిఖరం_Class_Name() + "();\n");
        }
        ofp()->puts_వ్యక్తిగత(నిజం);
        puts("VL_UNCOPYABLE(" + EmitCUtil::శిఖరం_Class_Name() + ");  ///< Copying not allowed\n");

        puts("\n");
        ofp()->puts_వ్యక్తిగత(అబద్ధం);  // public:
        puts("// API METHODS\n");
        స్థిరరాశి string call_Eval_చివర_అడుగు
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        ఒకవేళ (optSystemC()) {
            ofp()->puts_వ్యక్తిగత(నిజం);  ///< eval() is invoked by our sensitive() calls.
        }
        ఒకవేళ (!optSystemC()) {
            puts("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        ఒకవేళ (optSystemC() && v3Global.ఉపయోగిస్తుంది_Timing()) {
            puts("void eval();\n");
        } లేకపోతే {
            puts("void eval() { eval_step(); " + call_Eval_చివర_అడుగు + "}\n");
        }
        ఒకవేళ (!optSystemC()) {
            puts("/// Evaluate when calling multiple units/models per time step.\n");
        }
        puts("void eval_step();\n");
        ఒకవేళ (!optSystemC()) {
            puts("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            puts("/// Application must call after all eval() and before time changes.\n");
            puts("void eval_end_step()");
            ఒకవేళ (call_Eval_చివర_అడుగు == "") {
                puts(" {}\n");
            } లేకపోతే {
                puts(";\n");
            }
        }
        ఒకవేళ (!optSystemC()) {
            puts("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->puts_వ్యక్తిగత(అబద్ధం);  // public:
        puts("void final();\n");

        puts("/// Are there scheduled events to handle?\n");
        puts("bool eventsPending();\n");
        puts("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        puts("uint64_t nextTimeSlot();\n");

        ఒకవేళ (v3Global.opt.trace() || !optSystemC()) {
            puts("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            puts("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        ఒకవేళ (v3Global.opt.trace() && optSystemC()) {
            puts("/// SC tracing; avoid overloaded virtual function lint warning\n");
            puts("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        ఒకవేళ (!optSystemC()) {
            puts("/// Retrieve name of this model instance (as passed to constructor).\n");
            puts("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            స్థిరరాశి CFunc_సదిశ funcps
                = కనుగొను_Funcps([](స్థిరరాశి AstCFunc* nodep) { తిరిగి nodep->dpiExportDispatcher(); });
            ఒకవేళ (!funcps.ఖాళీలు()) {
                puts("\n/// DPI Export functions\n");
                కొరకు (స్థిరరాశి AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        ఒకవేళ (v3Global.opt.savable()) {
            puts("\n");
            puts("// Serialization functions\n");
            puts("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::శిఖరం_Class_Name() + "& rhs);\n");
            puts("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::శిఖరం_Class_Name() + "& rhs);\n");
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
        ఒకవేళ (v3Global.opt.trace()) {
            puts("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->puts_వ్యక్తిగత(నిజం);  // private:
        puts("\n// Internal functions - the model's evaluation entry points\n");
        puts("void evalBegin() override final;\n");
        puts("void evalEnd() override final;\n");
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            స్థిరరాశి VEval eval{i};
            // Only the iterated regions report whether they did any work
            puts(eval.isIterated() ? "bool "s : "void "s);
            puts(eval.eval_పద్ధతి() + (eval.మొదటి_Iteration() ? "(bool firstIteration)"s : "()"s));
            puts(" override final;\n");
        }
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            స్థిరరాశి VEval eval{i};
            ఒకవేళ (!eval.hasTriggers()) కొనసాగు;
            puts("void " + eval.dump_Triggers_పద్ధతి() + "() override final;\n");
        }

        puts("\n// Internal functions - trace registration\n");
        puts("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        puts("};\n");

        ofp()->puts_చివర_Guard();

        closeOutputFile();
    }

    శూన్య emitConstructorImplementation(Ast_కణుపు_Module* modp) {
        putSectionDelimiter("Constructors");

        puts("\n");
        putns(modp, EmitCUtil::శిఖరం_Class_Name() + "::" + EmitCUtil::శిఖరం_Class_Name());
        ఒకవేళ (optSystemC()) {
            puts("(sc_core::sc_module_name /* unused */)\n");
            puts("    : VerilatedModel{*Verilated::threadContextp()}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } లేకపోతే {
            puts("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            puts("    : VerilatedModel{*_vcontextp__}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        puts("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvt_గమ్యం_Str(v3Global.opt.converge_హద్దు()) + "}\n");

        // Set up IO references
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isPrimaryIO()) {
                    స్థిరరాశి string protName = varp->nameProtect();
                    puts("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell)) {
                ఒకవేళ (cellp->modp()->isConstPool()) కొనసాగు;  // Special emit rules
                స్థిరరాశి string protName = cellp->nameProtect();
                puts("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        puts("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        puts("{\n");
        puts("// Register model with the context\n");
        puts("contextp()->addModel(this);\n");
        ఒకవేళ (v3Global.opt.profExec()) {
            putsDecoration(శూన్యసూచిక, "// Profile the evaluation loop's region loops\n");
            స్థిరరాశి తార్కిక శిఖరం_Level = !v3Global.opt.hier_బిడ్డ() && v3Global.opt.libCreate().ఖాళీలు();
            puts("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (శిఖరం_Level ? "true" : "false") + ");\n");
        }
        ఒకవేళ (v3Global.opt.trace())
            puts("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        ఒకవేళ (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(శూన్యసూచిక, "// Sensitivities on all clocks and combinational inputs\n");
            puts("SC_METHOD(eval);\n");
            ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Timing()) {
                putsDecoration(శూన్యసూచిక, "// Notified by pending time delays/DPI exports\n");
                puts("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                    ఒకవేళ (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        పూర్ణాంకం vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        కొరకు (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            స్థిరరాశి పూర్ణాంకం vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            స్థిరరాశి string ivar = "__Vi"s + cvt_గమ్యం_Str(vecnum);
                            putns(varp, "for (int __Vi" + cvt_గమ్యం_Str(vecnum) + " = "
                                            + cvt_గమ్యం_Str(arrayp->lo()));
                            puts("; " + ivar + " <= " + cvt_గమ్యం_Str(arrayp->hi()));
                            puts("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        కొరకు (పూర్ణాంకం v = 0; v < vects; ++v) puts("[__Vi" + cvt_గమ్యం_Str(v) + "]");
                        puts(";\n");
                        కొరకు (పూర్ణాంకం v = 0; v < vects; ++v) puts("}\n");
                    }
                }
            }
            puts("\n");
        }

        puts("}\n");

        ఒకవేళ (!optSystemC()) {
            puts("\n");
            puts(EmitCUtil::శిఖరం_Class_Name() + "::" + EmitCUtil::శిఖరం_Class_Name()
                 + "(const char* _vcname__)\n");
            puts("    : " + EmitCUtil::శిఖరం_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    శూన్య emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        puts("\n");
        puts(EmitCUtil::శిఖరం_Class_Name() + "::~" + EmitCUtil::శిఖరం_Class_Name() + "() {\n");
        puts("delete vlSymsp;\n");
        puts("}\n");
    }

    శూన్య emitStandardMethods1(Ast_కణుపు_Module* modp) {
        UASSERT_OBJ(modp->is_శిఖరం(), modp, "Attempting to emitWrapEval for non-top class");

        స్థిరరాశి string శిఖరం_Mod_Name_Protected = EmitCUtil::ఉపసర్గ_Name_Protect(modp);
        స్థిరరాశి string selfDecl = "(" + శిఖరం_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        puts("\n");
        puts("#ifdef VL_DEBUG\n");
        puts("void " + శిఖరం_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        puts("#endif  // VL_DEBUG\n");
        AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            స్థిరరాశి VEval eval{i};
            ఒకవేళ (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        ఒకవేళ (optSystemC() && v3Global.ఉపయోగిస్తుంది_Timing()) {
            // ::eval
            puts("\nvoid " + EmitCUtil::శిఖరం_Class_Name() + "::eval() {\n");
            puts("eval_step();\n");
            puts("if (eventsPending()) {\n");
            puts("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            puts("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            puts("}\n");
            puts("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        puts("\nvoid " + EmitCUtil::శిఖరం_Class_Name() + "::eval_step() {\n");
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::శిఖరం_Class_Name()
             + "::eval_step\\n\"); );\n");
        puts("m_evalLoop.eval();\n");
        puts("}\n");

        // ::evalBegin - prepare the model for a time step
        puts("\nvoid " + EmitCUtil::శిఖరం_Class_Name() + "::evalBegin() {\n");
        puts("#ifdef VL_DEBUG\n");
        putsDecoration(శూన్యసూచిక, "// Debug assertions\n");
        puts(శిఖరం_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        puts("#endif  // VL_DEBUG\n");

        ఒకవేళ (v3Global.opt.trace()) puts("vlSymsp->__Vm_activity = true;\n");

        ఒకవేళ (v3Global.hasEvents()) puts("vlSymsp->clearTriggeredEvents();\n");
        ఒకవేళ (v3Global.hasClasses()) puts("vlSymsp->__Vm_deleter.deleteAll();\n");

        puts("}\n");

        // ::evalEnd - the time step is complete
        puts("\nvoid " + EmitCUtil::శిఖరం_Class_Name() + "::evalEnd() {\n");
        putsDecoration(శూన్యసూచిక, "// Evaluate cleanup\n");
        puts("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        ఒకవేళ (AstVar* స్థిరరాశి delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            puts("vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".cleanupForevered();\n");
        }

        puts("}\n");

        // Evaluation entry points
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            స్థిరరాశి VEval eval{i};
            AstCFunc* స్థిరరాశి funcp = v3Global.rootp()->evalFuncp(eval);
            puts(eval.isIterated() ? "\nbool " : "\nvoid ");
            puts(EmitCUtil::శిఖరం_Class_Name() + "::" + eval.eval_పద్ధతి() + "(");
            ఒకవేళ (eval.మొదటి_Iteration()) puts("bool firstIteration");
            puts(") {\n");
            ఒకవేళ (eval.isIterated()) puts("return ");
            puts(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            ఒకవేళ (eval.మొదటి_Iteration()) puts(", firstIteration");
            puts(");\n");
            puts("}\n");
        }

        // The trigger dump of each region
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            స్థిరరాశి VEval eval{i};
            ఒకవేళ (!eval.hasTriggers()) కొనసాగు;
            puts("\nVL_ATTR_COLD void " + EmitCUtil::శిఖరం_Class_Name()
                 + "::" + eval.dump_Triggers_పద్ధతి() + "() {\n");
            puts(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            puts("}\n");
        }
    }

    శూన్య emitStandardMethods2(Ast_కణుపు_Module* modp) {
        // ::eval_end_step
        ఒకవేళ (v3Global.needTraceDumper() && !optSystemC()) {
            puts("\n");
            putns(modp, "void " + EmitCUtil::శిఖరం_Class_Name() + "::eval_end_step() {\n");
            puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::శిఖరం_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            puts("#ifdef VM_TRACE\n");
            putsDecoration(శూన్యసూచిక, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            puts("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            puts("#endif  // VM_TRACE\n");
            puts("}\n");
        }

        putSectionDelimiter("Events and timing");
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::శిఖరం_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::శిఖరం_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".nextTimeSlot(); }\n");
        } లేకపోతే {
            putns(modp,
                  "bool " + EmitCUtil::శిఖరం_Class_Name() + "::eventsPending() { return false; }\n\n");
            puts("uint64_t " + EmitCUtil::శిఖరం_Class_Name() + "::nextTimeSlot() {\n");
            puts("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            puts("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        ఒకవేళ (!optSystemC()) {
            // ::name
            puts("\n");
            putns(modp, "const char* " + EmitCUtil::శిఖరం_Class_Name() + "::name() const {\n");
            puts(/**/ "return vlSymsp->name();\n");
            puts("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        puts("\nVL_ATTR_COLD void " + EmitCUtil::శిఖరం_Class_Name() + "::final() {\n");
        puts("contextp()->executingFinal(true);\n");
        puts("evalFinal();\n");
        puts("contextp()->executingFinal(false);\n");
        puts("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::శిఖరం_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::శిఖరం_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::శిఖరం_Class_Name() + "\"; }\n");
        స్థిరరాశి పూర్ణాంకం దారాలు = v3Global.opt.hier_బిడ్డ()
                                ? v3Global.opt.దారాలు()
                                : std::max(v3Global.opt.దారాలు(), v3Global.opt.hier_దారాలు());
        putns(modp, "unsigned " + EmitCUtil::శిఖరం_Class_Name() + "::threads() const { return "
                        + cvt_గమ్యం_Str(దారాలు) + "; }\n");
        putns(modp, "void " + EmitCUtil::శిఖరం_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::శిఖరం_Class_Name() + "::atClone() const {\n");
        ఒకవేళ (v3Global.opt.దారాలు() > 1) {
            puts("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        puts("contextp()->threadPoolpOnClone()");
        ఒకవేళ (v3Global.opt.దారాలు() > 1) puts(")");
        puts(";\n}\n");

        ఒకవేళ (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::శిఖరం_Class_Name()
                            + "::traceConfig() const {\n");
            puts("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            puts(v3Global.opt.useTraceParallel() ? "true" : "false");
            puts("}};\n");
            puts("};\n");
        }
    }

    శూన్య emitTraceMethods(Ast_కణుపు_Module* modp) {
        స్థిరరాశి string శిఖరం_Mod_Name_Protected = EmitCUtil::ఉపసర్గ_Name_Protect(modp);
        స్థిరరాశి string శిఖరం_Trace_Name
            = V3_బహిర్గతం_Formatter::quoteNameControls(v3Global.rootp()->trace_Lib_శిఖరం_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            putns(modp, "\nvoid " + శిఖరం_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + శిఖరం_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_పునాది()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + శిఖరం_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.trace_Class_పునాది() + "* tracep);\n");
        putns(modp, "\nvoid " + శిఖరం_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + శిఖరం_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_పునాది()
                        + "* tracep);\n");

        // Static helper function
        puts("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.trace_Class_పునాది() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        puts(EmitCUtil::void_Self_కేటాయించు(modp));
        puts(EmitCUtil::sym_Class_కేటాయించు());
        puts("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        puts("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        puts("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        puts("}\n");
        puts("vlSymsp->__Vm_baseCode = code;\n");
        ఒకవేళ (v3Global.opt.libCreate().ఖాళీలు()) {
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } లేకపోతే {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts(శిఖరం_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            puts("tracep->pushPrefix(\"" + శిఖరం_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts("}\n");
        }
        puts(శిఖరం_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        puts(శిఖరం_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        ఒకవేళ (v3Global.opt.libCreate().ఖాళీలు()) {  //
            puts("tracep->popPrefix();\n");
        } లేకపోతే {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->popPrefix();\n");
            puts("tracep->popPrefix();\n");
            puts("}\n");
        }
        puts("}\n");

        // Forward declaration
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + శిఖరం_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + శిఖరం_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_పునాది()
                        + "* tracep);\n");

        // ::traceRegisterModel
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::శిఖరం_Class_Name() + "::traceBaseModel(");
        puts("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        ఒకవేళ (optSystemC()) {
            puts(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            puts(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::శిఖరం_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            puts(/**/ "}");
        }
        puts(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        puts(/**/ v3Global.opt.trace_Class_పునాది() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.trace_Class_పునాది() + "C*>(tfp);\n");
        puts(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        puts(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::శిఖరం_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.trace_Class_పునాది() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        puts(/**/ "}\n");
        puts(/**/ "stfp->spTrace()->addModel(this);\n");
        puts(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().ఖాళీలు() ? "false" : "true")  //
             + ", " + std::గమ్యం_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        puts(/**/ శిఖరం_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        puts("}\n");
    }

    శూన్య emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        puts("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::శిఖరం_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");

        puts("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::శిఖరం_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");
    }

    శూన్య emitImplementation(Ast_కణుపు_Module* modp) {
        open_New_Output_ఆధారం_File(EmitCUtil::శిఖరం_Class_Name(), అబద్ధం, అబద్ధం,
                                "Model implementation (design independent parts)");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        కొరకు (స్థిరరాశి string& పునాది : v3Global.opt.trace_ఆధారం_Langs())
            puts("#include \"" + పునాది + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        ఒకవేళ (v3Global.opt.trace()) emitTraceMethods(modp);
        ఒకవేళ (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    శూన్య emitDpiExportDispatchers(Ast_కణుపు_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3_Unique_పేర్లు unique_పేర్లు;
        స్థిరరాశి std::string file_పునాది_Name = EmitCUtil::శిఖరం_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc);
            ఒకవేళ (!funcp || !funcp->dpiExportDispatcher()) కొనసాగు;

            ఒకవేళ (విభజించు_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(నిజం);
                // Close old file
                closeOutputFile();
            }

            ఒకవేళ (!ofp()) {
                open_New_Output_ఆధారం_File(unique_పేర్లు.get(file_పునాది_Name), అబద్ధం, అబద్ధం,
                                        "Implementation of DPI export functions.");
                puts("\n");
                puts("#include \"" + EmitCUtil::శిఖరం_Class_Name() + ".h\"\n");
                puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                puts("#include \"verilated_dpi.h\"\n");
                puts("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        ఒకవేళ (ofp()) closeOutputFile();
    }

    శూన్య ప్రధాన(Ast_కణుపు_Module* modp) {
        m_modp = modp;
        emit_శీర్షిక(modp);
        emitImplementation(modp);
        ఒకవేళ (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

బహిరంగ:
    స్పష్టమైన EmitCModel(Ast_కణుపు_Module* శిఖరం_Modulep) { ప్రధాన(శిఖరం_Modulep); }
};

//######################################################################
// EmitC class functions

శూన్య V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->కూడు_Eval_Stats("emitc");
    { EmitCModel{v3Global.rootp()->శిఖరం_Modulep()}; }
}
