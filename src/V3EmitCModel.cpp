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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCFunc.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு EmitCModel final : பொது EmitCFunc {
    // TYPES
    using CFuncVector = std::vector<மாறிலி AstCFunc*>;

    // MEMBERS
    V3UniqueNames m_uniqueNames;  // For generating unique file names

    // METHODS

    CFuncVector findFuncps(std::function<தருக்க(மாறிலி AstCFunc*)> cb) {
        CFuncVector funcps;
        க்கு (Ast_முனை* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc)) {
                எனில் (cb(funcp)) funcps.தள்ளு_back(funcp);
            }
        }
        stable_sort(funcps.begin(), funcps.end(),
                    [](மாறிலி Ast_முனை* ap, மாறிலி Ast_முனை* bp) { திரும்பு ap->name() < bp->name(); });
        திரும்பு funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    நிலையான தருக்க isDpiPassingStructs(மாறிலி AstCFunc* funcp) {
        எனில் (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) திரும்பு பொய்;
        க்கு (மாறிலி Ast_முனை* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            மாறிலி AstVar* மாறிலி varp = VN_CAST(argp, Var);
            எனில் (varp && varp->dtypep()->containsUnpackedStruct()) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    வெறுமை putSectionDelimiter(மாறிலி string& name) {
        puts("\n");
        puts("//============================================================\n");
        puts("// " + name + "\n");
    }

    வெறுமை emitHeader(Ast_முனை_Module* modp) {
        openNewOutputHeaderFile(EmitCUtil::உச்சி_Class_Name(), "Primary model header");
        puts("//\n");
        puts("// This header should be included by all source files instantiating the design.\n");
        puts("// The class here is then constructed to instantiate the design.\n");
        puts("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_உச்சி_Include();

        puts("#include \"verilated.h\"\n");
        எனில் (v3Global.opt.systemC()) puts("#include \"verilated_sc.h\"\n");
        எனில் (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        எனில் (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        எனில் (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        எனில் (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        எனில் (v3Global.dpi()) puts("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        எனில் (!findFuncps(isDpiPassingStructs).empty()) {
            puts("#include \"" + EmitCUtil::உச்சி_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        puts("\n");
        puts("class " + EmitCUtil::symClassName() + ";\n");
        puts("class " + EmitCUtil::prefixNameProtect(modp) + ";\n");  // For rootp pointer only
        க்கு (மாறிலி string& base : v3Global.opt.traceClassLangs()) puts("class " + base + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        puts("\n");

        puts("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::உச்சி_Class_Name()
                        + " VL_NOT_FINAL : ");
        எனில் (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            puts("public ::sc_core::sc_module, ");
        }
        puts("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        puts("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->resetPrivate();
        ofp()->putsPrivate(உண்மை);  // private:

        puts("// Symbol table holding complete model state (owned by this class)\n");
        puts(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        puts("// Evaluation loop\n");
        puts("VerilatedEvalLoop m_evalLoop;\n");

        puts("\n");
        ofp()->putsPrivate(பொய்);  // public:

        puts("\n// CONSTEXPR CAPABILITIES\n");
        puts("// Verilated with --trace?\n");
        puts("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        puts("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ உண்மை);
                }
            }
        }
        எனில் (v3Global.opt.emitAccessors()) {
            puts("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                    எனில் (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        puts("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstCell* மாறிலி cellp = VN_CAST(nodep, Cell)) {
                எனில் (cellp->modp()->isConstPool()) தொடர்;  // Special emit rules
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        puts("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        puts(EmitCUtil::prefixNameProtect(modp) + "* const rootp;\n");

        puts("\n");
        ofp()->putsPrivate(பொய்);  // public:
        puts("// CONSTRUCTORS\n");
        எனில் (optSystemC()) {
            puts("SC_CTOR(" + EmitCUtil::உச்சி_Class_Name() + ");\n");
            puts("virtual ~" + EmitCUtil::உச்சி_Class_Name() + "();\n");
        } இல்லையெனில் {
            puts("/// Construct the model; called by application code\n");
            puts("/// If contextp is null, then the model will use the default global "
                 "context\n");
            puts("/// If name is \"\", then makes a wrapper with a\n");
            puts("/// single model invisible with respect to DPI scope names.\n");
            puts("explicit " + EmitCUtil::உச்சி_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            puts("explicit " + EmitCUtil::உச்சி_Class_Name() + "(const char* name = \"TOP\");\n");
            puts("/// Destroy the model; called (often implicitly) by application code\n");
            puts("virtual ~" + EmitCUtil::உச்சி_Class_Name() + "();\n");
        }
        ofp()->putsPrivate(உண்மை);
        puts("VL_UNCOPYABLE(" + EmitCUtil::உச்சி_Class_Name() + ");  ///< Copying not allowed\n");

        puts("\n");
        ofp()->putsPrivate(பொய்);  // public:
        puts("// API METHODS\n");
        மாறிலி string callEvalEndStep
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        எனில் (optSystemC()) {
            ofp()->putsPrivate(உண்மை);  ///< eval() is invoked by our sensitive() calls.
        }
        எனில் (!optSystemC()) {
            puts("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        எனில் (optSystemC() && v3Global.usesTiming()) {
            puts("void eval();\n");
        } இல்லையெனில் {
            puts("void eval() { eval_step(); " + callEvalEndStep + "}\n");
        }
        எனில் (!optSystemC()) {
            puts("/// Evaluate when calling multiple units/models per time step.\n");
        }
        puts("void eval_step();\n");
        எனில் (!optSystemC()) {
            puts("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            puts("/// Application must call after all eval() and before time changes.\n");
            puts("void eval_end_step()");
            எனில் (callEvalEndStep == "") {
                puts(" {}\n");
            } இல்லையெனில் {
                puts(";\n");
            }
        }
        எனில் (!optSystemC()) {
            puts("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->putsPrivate(பொய்);  // public:
        puts("void final();\n");

        puts("/// Are there scheduled events to handle?\n");
        puts("bool eventsPending();\n");
        puts("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        puts("uint64_t nextTimeSlot();\n");

        எனில் (v3Global.opt.trace() || !optSystemC()) {
            puts("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            puts("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        எனில் (v3Global.opt.trace() && optSystemC()) {
            puts("/// SC tracing; avoid overloaded virtual function lint warning\n");
            puts("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        எனில் (!optSystemC()) {
            puts("/// Retrieve name of this model instance (as passed to constructor).\n");
            puts("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            மாறிலி CFuncVector funcps
                = findFuncps([](மாறிலி AstCFunc* nodep) { திரும்பு nodep->dpiExportDispatcher(); });
            எனில் (!funcps.empty()) {
                puts("\n/// DPI Export functions\n");
                க்கு (மாறிலி AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        எனில் (v3Global.opt.savable()) {
            puts("\n");
            puts("// Serialization functions\n");
            puts("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::உச்சி_Class_Name() + "& rhs);\n");
            puts("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::உச்சி_Class_Name() + "& rhs);\n");
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
        எனில் (v3Global.opt.trace()) {
            puts("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->putsPrivate(உண்மை);  // private:
        puts("\n// Internal functions - the model's evaluation entry points\n");
        puts("void evalBegin() override final;\n");
        puts("void evalEnd() override final;\n");
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            மாறிலி VEval eval{i};
            // Only the iterated regions report whether they did any work
            puts(eval.isIterated() ? "bool "s : "void "s);
            puts(eval.evalMethod() + (eval.firstIteration() ? "(bool firstIteration)"s : "()"s));
            puts(" override final;\n");
        }
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            மாறிலி VEval eval{i};
            எனில் (!eval.hasTriggers()) தொடர்;
            puts("void " + eval.dumpTriggersMethod() + "() override final;\n");
        }

        puts("\n// Internal functions - trace registration\n");
        puts("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        puts("};\n");

        ofp()->putsEndGuard();

        closeOutputFile();
    }

    வெறுமை emitConstructorImplementation(Ast_முனை_Module* modp) {
        putSectionDelimiter("Constructors");

        puts("\n");
        putns(modp, EmitCUtil::உச்சி_Class_Name() + "::" + EmitCUtil::உச்சி_Class_Name());
        எனில் (optSystemC()) {
            puts("(sc_core::sc_module_name /* unused */)\n");
            puts("    : VerilatedModel{*Verilated::threadContextp()}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } இல்லையெனில் {
            puts("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            puts("    : VerilatedModel{*_vcontextp__}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        puts("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvtToStr(v3Global.opt.convergeLimit()) + "}\n");

        // Set up IO references
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isPrimaryIO()) {
                    மாறிலி string protName = varp->nameProtect();
                    puts("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstCell* மாறிலி cellp = VN_CAST(nodep, Cell)) {
                எனில் (cellp->modp()->isConstPool()) தொடர்;  // Special emit rules
                மாறிலி string protName = cellp->nameProtect();
                puts("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        puts("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        puts("{\n");
        puts("// Register model with the context\n");
        puts("contextp()->addModel(this);\n");
        எனில் (v3Global.opt.profExec()) {
            putsDecoration(nullptr, "// Profile the evaluation loop's region loops\n");
            மாறிலி தருக்க உச்சி_Level = !v3Global.opt.hierChild() && v3Global.opt.libCreate().empty();
            puts("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (உச்சி_Level ? "true" : "false") + ");\n");
        }
        எனில் (v3Global.opt.trace())
            puts("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        எனில் (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(nullptr, "// Sensitivities on all clocks and combinational inputs\n");
            puts("SC_METHOD(eval);\n");
            எனில் (v3Global.usesTiming()) {
                putsDecoration(nullptr, "// Notified by pending time delays/DPI exports\n");
                puts("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                    எனில் (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        முழு vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        க்கு (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            மாறிலி முழு vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            மாறிலி string ivar = "__Vi"s + cvtToStr(vecnum);
                            putns(varp, "for (int __Vi" + cvtToStr(vecnum) + " = "
                                            + cvtToStr(arrayp->lo()));
                            puts("; " + ivar + " <= " + cvtToStr(arrayp->hi()));
                            puts("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        க்கு (முழு v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                        puts(";\n");
                        க்கு (முழு v = 0; v < vects; ++v) puts("}\n");
                    }
                }
            }
            puts("\n");
        }

        puts("}\n");

        எனில் (!optSystemC()) {
            puts("\n");
            puts(EmitCUtil::உச்சி_Class_Name() + "::" + EmitCUtil::உச்சி_Class_Name()
                 + "(const char* _vcname__)\n");
            puts("    : " + EmitCUtil::உச்சி_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    வெறுமை emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        puts("\n");
        puts(EmitCUtil::உச்சி_Class_Name() + "::~" + EmitCUtil::உச்சி_Class_Name() + "() {\n");
        puts("delete vlSymsp;\n");
        puts("}\n");
    }

    வெறுமை emitStandardMethods1(Ast_முனை_Module* modp) {
        UASSERT_OBJ(modp->is_உச்சி(), modp, "Attempting to emitWrapEval for non-top class");

        மாறிலி string உச்சி_Mod_Name_Protected = EmitCUtil::prefixNameProtect(modp);
        மாறிலி string selfDecl = "(" + உச்சி_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        puts("\n");
        puts("#ifdef VL_DEBUG\n");
        puts("void " + உச்சி_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        puts("#endif  // VL_DEBUG\n");
        AstNetlist* மாறிலி netlistp = v3Global.rootp();
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            மாறிலி VEval eval{i};
            எனில் (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        எனில் (optSystemC() && v3Global.usesTiming()) {
            // ::eval
            puts("\nvoid " + EmitCUtil::உச்சி_Class_Name() + "::eval() {\n");
            puts("eval_step();\n");
            puts("if (eventsPending()) {\n");
            puts("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            puts("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            puts("}\n");
            puts("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        puts("\nvoid " + EmitCUtil::உச்சி_Class_Name() + "::eval_step() {\n");
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::உச்சி_Class_Name()
             + "::eval_step\\n\"); );\n");
        puts("m_evalLoop.eval();\n");
        puts("}\n");

        // ::evalBegin - prepare the model for a time step
        puts("\nvoid " + EmitCUtil::உச்சி_Class_Name() + "::evalBegin() {\n");
        puts("#ifdef VL_DEBUG\n");
        putsDecoration(nullptr, "// Debug assertions\n");
        puts(உச்சி_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        puts("#endif  // VL_DEBUG\n");

        எனில் (v3Global.opt.trace()) puts("vlSymsp->__Vm_activity = true;\n");

        எனில் (v3Global.hasEvents()) puts("vlSymsp->clearTriggeredEvents();\n");
        எனில் (v3Global.hasClasses()) puts("vlSymsp->__Vm_deleter.deleteAll();\n");

        puts("}\n");

        // ::evalEnd - the time step is complete
        puts("\nvoid " + EmitCUtil::உச்சி_Class_Name() + "::evalEnd() {\n");
        putsDecoration(nullptr, "// Evaluate cleanup\n");
        puts("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        எனில் (AstVar* மாறிலி delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            puts("vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".cleanupForevered();\n");
        }

        puts("}\n");

        // Evaluation entry points
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            மாறிலி VEval eval{i};
            AstCFunc* மாறிலி funcp = v3Global.rootp()->evalFuncp(eval);
            puts(eval.isIterated() ? "\nbool " : "\nvoid ");
            puts(EmitCUtil::உச்சி_Class_Name() + "::" + eval.evalMethod() + "(");
            எனில் (eval.firstIteration()) puts("bool firstIteration");
            puts(") {\n");
            எனில் (eval.isIterated()) puts("return ");
            puts(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            எனில் (eval.firstIteration()) puts(", firstIteration");
            puts(");\n");
            puts("}\n");
        }

        // The trigger dump of each region
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            மாறிலி VEval eval{i};
            எனில் (!eval.hasTriggers()) தொடர்;
            puts("\nVL_ATTR_COLD void " + EmitCUtil::உச்சி_Class_Name()
                 + "::" + eval.dumpTriggersMethod() + "() {\n");
            puts(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            puts("}\n");
        }
    }

    வெறுமை emitStandardMethods2(Ast_முனை_Module* modp) {
        // ::eval_end_step
        எனில் (v3Global.needTraceDumper() && !optSystemC()) {
            puts("\n");
            putns(modp, "void " + EmitCUtil::உச்சி_Class_Name() + "::eval_end_step() {\n");
            puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::உச்சி_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            puts("#ifdef VM_TRACE\n");
            putsDecoration(nullptr, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            puts("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            puts("#endif  // VM_TRACE\n");
            puts("}\n");
        }

        putSectionDelimiter("Events and timing");
        எனில் (auto* மாறிலி delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::உச்சி_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::உச்சி_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".nextTimeSlot(); }\n");
        } இல்லையெனில் {
            putns(modp,
                  "bool " + EmitCUtil::உச்சி_Class_Name() + "::eventsPending() { return false; }\n\n");
            puts("uint64_t " + EmitCUtil::உச்சி_Class_Name() + "::nextTimeSlot() {\n");
            puts("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            puts("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        எனில் (!optSystemC()) {
            // ::name
            puts("\n");
            putns(modp, "const char* " + EmitCUtil::உச்சி_Class_Name() + "::name() const {\n");
            puts(/**/ "return vlSymsp->name();\n");
            puts("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        puts("\nVL_ATTR_COLD void " + EmitCUtil::உச்சி_Class_Name() + "::final() {\n");
        puts("contextp()->executingFinal(true);\n");
        puts("evalFinal();\n");
        puts("contextp()->executingFinal(false);\n");
        puts("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::உச்சி_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::உச்சி_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::உச்சி_Class_Name() + "\"; }\n");
        மாறிலி முழு threads = v3Global.opt.hierChild()
                                ? v3Global.opt.threads()
                                : std::max(v3Global.opt.threads(), v3Global.opt.hierThreads());
        putns(modp, "unsigned " + EmitCUtil::உச்சி_Class_Name() + "::threads() const { return "
                        + cvtToStr(threads) + "; }\n");
        putns(modp, "void " + EmitCUtil::உச்சி_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::உச்சி_Class_Name() + "::atClone() const {\n");
        எனில் (v3Global.opt.threads() > 1) {
            puts("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        puts("contextp()->threadPoolpOnClone()");
        எனில் (v3Global.opt.threads() > 1) puts(")");
        puts(";\n}\n");

        எனில் (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::உச்சி_Class_Name()
                            + "::traceConfig() const {\n");
            puts("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            puts(v3Global.opt.useTraceParallel() ? "true" : "false");
            puts("}};\n");
            puts("};\n");
        }
    }

    வெறுமை emitTraceMethods(Ast_முனை_Module* modp) {
        மாறிலி string உச்சி_Mod_Name_Protected = EmitCUtil::prefixNameProtect(modp);
        மாறிலி string உச்சி_Trace_Name
            = V3OutFormatter::quoteNameControls(v3Global.rootp()->trace_Lib_உச்சி_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        எனில் (!v3Global.opt.libCreate().empty()) {
            putns(modp, "\nvoid " + உச்சி_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + உச்சி_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + உச்சி_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.traceClassBase() + "* tracep);\n");
        putns(modp, "\nvoid " + உச்சி_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + உச்சி_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                        + "* tracep);\n");

        // Static helper function
        puts("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.traceClassBase() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        puts(EmitCUtil::voidSelfAssign(modp));
        puts(EmitCUtil::symClassAssign());
        puts("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        puts("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        puts("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        puts("}\n");
        puts("vlSymsp->__Vm_baseCode = code;\n");
        எனில் (v3Global.opt.libCreate().empty()) {
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } இல்லையெனில் {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts(உச்சி_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            puts("tracep->pushPrefix(\"" + உச்சி_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts("}\n");
        }
        puts(உச்சி_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        puts(உச்சி_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        எனில் (v3Global.opt.libCreate().empty()) {  //
            puts("tracep->popPrefix();\n");
        } இல்லையெனில் {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->popPrefix();\n");
            puts("tracep->popPrefix();\n");
            puts("}\n");
        }
        puts("}\n");

        // Forward declaration
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + உச்சி_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + உச்சி_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                        + "* tracep);\n");

        // ::traceRegisterModel
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::உச்சி_Class_Name() + "::traceBaseModel(");
        puts("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        எனில் (optSystemC()) {
            puts(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            puts(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::உச்சி_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            puts(/**/ "}");
        }
        puts(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        puts(/**/ v3Global.opt.traceClassBase() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.traceClassBase() + "C*>(tfp);\n");
        puts(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        puts(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::உச்சி_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.traceClassBase() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        puts(/**/ "}\n");
        puts(/**/ "stfp->spTrace()->addModel(this);\n");
        puts(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().empty() ? "false" : "true")  //
             + ", " + std::to_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        puts(/**/ உச்சி_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        puts("}\n");
    }

    வெறுமை emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        puts("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::உச்சி_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");

        puts("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::உச்சி_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");
    }

    வெறுமை emitImplementation(Ast_முனை_Module* modp) {
        openNewOutputSourceFile(EmitCUtil::உச்சி_Class_Name(), பொய், பொய்,
                                "Model implementation (design independent parts)");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        க்கு (மாறிலி string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        எனில் (v3Global.opt.trace()) emitTraceMethods(modp);
        எனில் (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    வெறுமை emitDpiExportDispatchers(Ast_முனை_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3UniqueNames uniqueNames;
        மாறிலி std::string fileBaseName = EmitCUtil::உச்சி_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc);
            எனில் (!funcp || !funcp->dpiExportDispatcher()) தொடர்;

            எனில் (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(உண்மை);
                // Close old file
                closeOutputFile();
            }

            எனில் (!ofp()) {
                openNewOutputSourceFile(uniqueNames.get(fileBaseName), பொய், பொய்,
                                        "Implementation of DPI export functions.");
                puts("\n");
                puts("#include \"" + EmitCUtil::உச்சி_Class_Name() + ".h\"\n");
                puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                puts("#include \"verilated_dpi.h\"\n");
                puts("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        எனில் (ofp()) closeOutputFile();
    }

    வெறுமை முதன்மை(Ast_முனை_Module* modp) {
        m_modp = modp;
        emitHeader(modp);
        emitImplementation(modp);
        எனில் (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

பொது:
    explicit EmitCModel(Ast_முனை_Module* உச்சி_Modulep) { முதன்மை(உச்சி_Modulep); }
};

//######################################################################
// EmitC class functions

வெறுமை V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->addEvalStats("emitc");
    { EmitCModel{v3Global.rootp()->உச்சி_Modulep()}; }
}
