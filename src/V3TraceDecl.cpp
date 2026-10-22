// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Waves tracing
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
// V3TraceDecl's Transformations:
//      Create trace init CFunc
//      For each VarScope
//          If appropriate type of signal, create a TraceDecl
//
//*************************************************************************

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3TraceDecl.h"

#شامل "V3Ast.h"
#شامل "V3Control.h"
#شامل "V3EmitCBase.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Number.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

#شامل <cstdint>
#شامل <functional>
#شامل <limits>
#شامل <tuple>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

رده PathAdjustor final {
    File_سطر* ثابت m_flp;  // FileLine used for created nodes
    std::function<تهی(Ast_گره_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_پشته{""};  // Stack of current paths

    ایستا constexpr نویسه SEPARATOR = ' ';

عمومی:
    explicit PathAdjustor(File_سطر* flp, std::function<تهی(Ast_گره_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    تهی adjust(ثابت string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        وقتی (!VString::startsWith(newPath, m_پشته.back())) {
            m_emit(جدید Ast_Trace_برداشت_Prefix{m_flp});
            m_پشته.برداشت_back();
        }

        اگر (newPath == m_پشته.back()) بازگشت;

        ثابت VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        ثابت std::string extraPrefix = newPath.substr(m_پشته.back().size());
        size_t begin = 0;
        ثابت size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        وقتی (درست) {
            ثابت size_t end = extraPrefix.find(SEPARATOR, begin);
            اگر (end == string::npos) بشکن;
            ثابت string& extra = extraPrefix.substr(begin, end - begin);
            اگر (end == last) {
                m_emit(جدید Ast_Trace_افزودن_Prefix{m_flp, extra, lastScopeType});
            } وگرنه {
                m_emit(جدید Ast_Trace_افزودن_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_پشته.افزودن_back(m_پشته.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    تهی unwind() {
        وقتی (m_پشته.size() > 1) {
            m_emit(جدید Ast_Trace_برداشت_Prefix{m_flp});
            m_پشته.برداشت_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

رده TraceDeclVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_بالاترین_Scope* ثابت m_بالاترین_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_بالاترین_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ساختار Dtype_Func_کلید final {
        ثابت Ast_گره_DType* dtypep;
        VVarType varType;
        منطقی operator==(ثابت Dtype_Func_کلید& other) ثابت {
            بازگشت dtypep == other.dtypep && varType == other.varType;
        }
    };
    ساختار Dtype_Func_کلید_Hash final {
        size_t operator()(ثابت Dtype_Func_کلید& کلید) ثابت {
            بازگشت std::hash<ثابت Ast_گره_DType*>{}(کلید.dtypep)
                   ^ (std::hash<uint8_t>{}(کلید.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_کلید, AstCFunc*, Dtype_Func_کلید_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    ثابت string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    منطقی m_skipDtypeFunc = نادرست;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحیح m_بالاترین_Func_Size = 0;  // Size of the top function currently being built
    صحیح m_subFuncSize = 0;  // Size of the sub function currently being built
    منطقی m_forceNewLeaf = نادرست;  // Force next addToSubFunc to start a fresh leaf
    size_t m_بالاترین_Scope_ریشه_Func_شمار = 0;  // Top-scope init functions used only for wrapper IOs
    منطقی m_بالاترین_Scope_ریشه_Phase = نادرست;  // Emitting top-scope wrapper IO declarations
    ثابت صحیح m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<صحیح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_گره_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_گره_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    رده TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        تهی init(ثابت std::string& name, Ast_گره* nodep, منطقی in_بالاترین_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_گره::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } وگرنه اگر (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } وگرنه {
                // Other Variable
                ثابت size_t pos = vcdName.rfind(' ');
                ثابت size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            اگر (in_بالاترین_Scope && !v3Global.opt.libCreate().empty()) {
                ثابت size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                اگر (prefix == "$rootio") بازگشت;
                m_path = m_path.substr(start + 1);
                اگر (v3Global.rootp()->trace_Lib_بالاترین_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    عمومی:
        explicit TraceEntry(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_بالاترین());
        }
        explicit TraceEntry(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_بالاترین());
        }
        صحیح operatorCompare(ثابت TraceEntry& b) ثابت {
            اگر (ثابت صحیح cmp = path().compare(b.path())) بازگشت cmp < 0;
            اگر (ثابت صحیح cmp = fileline().operatorCompare(b.fileline())) بازگشت cmp < 0;
            بازگشت name() < b.name();
        }
        AstVarScope* vscp() ثابت { بازگشت m_vscp; }
        AstCell* cellp() ثابت { بازگشت m_cellp; }
        ثابت std::string& path() ثابت { بازگشت m_path; }
        تهی path(ثابت std::string& path) { m_path = path; }
        ثابت std::string& name() ثابت { بازگشت m_name; }
        File_سطر& fileline() ثابت { بازگشت m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_گره_عبارت* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ثابت AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ثابت AstVar* ثابت varp = nodep->varp();
        اگر (!varp->isTrace()) بازگشت "Verilator trace_off";
        اگر (!nodep->isTrace()) بازگشت "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        اگر (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            بازگشت "Automatic variable";

        ثابت صحیح width = recurseDTypeWidth(nodep->varp()->dtypep());
        اگر (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            بازگشت "Width " + cvtToStr(width) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        اگر (!V3Control::getScopeTraceOn(prettyName)) بازگشت "Vlt scope trace_off";
        اگر (!v3Global.opt.traceUnderscore()) {
            اگر (!prettyName.empty() && prettyName[0] == '_') بازگشت "Leading underscore";
            اگر (prettyName.find("._") != string::npos) بازگشت "Inlined leading underscore";
        }
        بازگشت ""s;
    }

    صحیح recurseDTypeWidth(ثابت Ast_گره_DType* nodep) {
        اگر (ثابت Ast_گره_Array_DType* adtypep = VN_CAST(nodep, گره_Array_DType))
            بازگشت recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        بازگشت nodep->width();
    }

    AstCFunc* newCFunc(File_سطر* flp, ثابت string& name) {
        AstScope* ثابت بالاترین_Scopep = m_بالاترین_Scopep->scopep();
        AstCFunc* ثابت funcp = جدید AstCFunc{flp, name, بالاترین_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(درست);
        funcp->isStatic(نادرست);
        funcp->isLoose(درست);
        funcp->slow(درست);
        بالاترین_Scopep->addBlocksp(funcp);
        بازگشت funcp;
    }

    تهی add_To_بالاترین_Func(Ast_گره_Stmt* stmtp) {
        اگر (m_بالاترین_Func_Size > m_funcSizeLimit || m_بالاترین_Funcps.empty()) {
            m_بالاترین_Func_Size = 0;
            //
            ثابت string n = cvtToStr(m_بالاترین_Funcps.size());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_بالاترین_Scopep->fileline(), name);
            m_بالاترین_Funcps.افزودن_back(funcp);
        }
        m_بالاترین_Funcps.back()->addStmtsp(stmtp);
        m_بالاترین_Func_Size += stmtp->گره_شمار();
    }

    تهی addToSubFunc(Ast_گره_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        اگر (m_dtypeSubFuncp) {
            اگر (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->گره_شمار();
            بازگشت;
        }
        // Defer trace splitting until V3Trace
        اگر (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = نادرست;
            File_سطر* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvtToStr(m_subFuncps.size());
            ثابت string name
                = m_currScopep == m_بالاترین_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_بالاترین_Scope_ریشه_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->addStmtsp(جدید AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.افزودن_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->گره_شمار();
    }

    AstTraceDecl* addTraceDecl(ثابت VNumRange& arrayRange,
                               صحیح widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = nullptr) {
        VNumRange bitRange;
        اگر (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } وگرنه اگر (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گره_عبارت* valuep = m_traValuep->clone_درخت(نادرست);
        ثابت منطقی validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = جدید AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        اگر (validOffset) {
            newp->code(m_offset);
            اگر (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(جدید AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_درخت(), refp);
            });
        } وگرنه {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        بازگشت newp;
    }

    تهی addIgnore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        اگر (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    تهی fixupPlaceholder(ثابت std::string& path, Ast_گره_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت auto it = m_pathToScopep.find(Ast_گره::prettyName(path));
        اگر (it != m_pathToScopep.end()) {
            ثابت AstScope* ثابت scopep = it->second;
            File_سطر* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_گره* stmtp = nullptr;
            برای (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = جدید AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_گره::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            اگر (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_درخت(), placeholderp);
    }

    تهی fixupLibStub(ثابت std::string& path, Ast_گره_Stmt* placeholderp) {
        File_سطر* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_گره::prettyName(path);
        اگر (!v3Global.opt.libCreate().empty()) {
            ثابت std::string بالاترین_Prefix = v3Global.rootp()->trace_Lib_بالاترین_Name() + ".";
            اگر (VString::startsWith(name, بالاترین_Prefix)) name = name.substr(بالاترین_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = جدید AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(جدید AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_درخت(), placeholderp);
    }

    تهی fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        برای (ثابت auto& item : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(item);
            ثابت AstCell* ثابت cellp = std::get<1>(item);
            Ast_گره_Stmt* ثابت placeholderp = std::get<2>(item);
            ثابت std::string path = parentp->name() + "__DOT__" + cellp->name();
            اگر (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } وگرنه {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        برای (ثابت auto& item : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(item);
            Ast_گره_Stmt* ثابت placeholderp = std::get<1>(item);
            ثابت std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    تهی remove_Redundant_Prefix_افزودن_برداشت() {
        برای (ثابت auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            برای (AstCFunc* ثابت funcp : pair.second) {
                Ast_گره* prevp = nullptr;
                Ast_گره* currp = funcp->stmtsp();
                وقتی (currp) {
                    Ast_گره* ثابت nextp = currp->nextp();
                    اگر (VN_IS(prevp, Trace_افزودن_Prefix) && VN_IS(currp, Trace_برداشت_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_درخت(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_درخت(), currp);
                    }
                    اگر (!nextp) بشکن;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    تهی checkCalls(ثابت AstCFunc* funcp) {
        اگر (!v3Global.opt.debugCheck()) بازگشت;
        checkCallsRecurse(funcp);
        اگر (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            برای (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    تهی checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([این](ثابت Ast_گره* nodep) {
            اگر (ثابت AstTraceDecl* ثابت declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } وگرنه اگر (ثابت AstCCall* ثابت ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    منطقی isBasicIO() { بازگشت m_traVscp->varp()->isVLIO(); }
    تهی newDeclSubFunc() {
        File_سطر* ثابت flp = m_dtypeFuncp->fileline();
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* ثابت subCallp = جدید AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    تهی newDeclFunc(Ast_گره_DType* nodep) {
        Ast_گره_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_کلید dtype_کلید{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_کلید, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        اگر (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = جدید AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        اگر (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            اگر (AstStructDType* ثابت dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, درست);
            } وگرنه اگر (AstUnpackArrayDType* ثابت dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, درست);
            } وگرنه اگر (AstPackArrayDType* ثابت dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, درست);
            } وگرنه {
                UASSERT_OBJ(نادرست, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    تهی declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقی newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        addToSubFunc(جدید Ast_Trace_افزودن_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->چپ(), nodep->راست(), !newFunc});

        اگر (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            اگر (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } وگرنه {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } وگرنه {
            Ast_گره_DType* ثابت subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            برای (ثابت صحیح i : nodep->declRange().seq_چپ_To_راست()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_درخت(نادرست);
                m_traValuep = جدید AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
        }

        addToSubFunc(جدید Ast_Trace_برداشت_Prefix{flp});
    }
    تهی declPackedArray(AstPackArrayDType* ثابت nodep, منطقی newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_گره_DType* ثابت subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        addToSubFunc(جدید Ast_Trace_افزودن_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->چپ(), nodep->راست(), !newFunc});

        // Always iterate left index to right index
        برای (ثابت صحیح i : nodep->declRange().seq_چپ_To_راست()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            ثابت صحیح lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_درخت(نادرست);
            m_traValuep = جدید AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_درخت(), m_traValuep = nullptr);
        }

        addToSubFunc(جدید Ast_Trace_برداشت_Prefix{flp});
    }
    تهی declStruct(AstStructDType* ثابت nodep, منطقی newFunc) {
        File_سطر* ثابت flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        صحیح nMembers = 0;
        برای (Ast_گره* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        اگر (!nodep->packed()) {
            addToSubFunc(جدید Ast_Trace_افزودن_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            برای (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(نادرست);
                m_traValuep = جدید AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            addToSubFunc(جدید Ast_Trace_برداشت_Prefix{flp});
        } وگرنه {
            addToSubFunc(جدید Ast_Trace_افزودن_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            برای (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(نادرست);
                m_traValuep = جدید AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            addToSubFunc(جدید Ast_Trace_برداشت_Prefix{flp});
        }
    }

    // VISITORS
    تهی visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        اگر (nodep->modp()->verilatorLib()) بازگشت;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        برای (Ast_گره* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        اگر (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](ثابت TraceEntry& a, ثابت TraceEntry& b) { بازگشت a.operatorCompare(b); });

            File_سطر* ثابت flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_گره_Stmt* stmtp) { addToSubFunc(stmtp); }};
            ثابت منطقی split_ریشه_Primary_Ios = nodep->is_بالاترین() && !v3Global.opt.libCreate().empty();
            ثابت auto emitEntry = [&](ثابت TraceEntry& entry) {
                AstVarScope* ثابت vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_گره*>(entry.cellp())
                                                    : static_cast<Ast_گره*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                اگر (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    اگر (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } وگرنه {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = جدید AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        اگر (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } وگرنه {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = entry.cellp();
                    Ast_گره_Stmt* ثابت stmtp = جدید AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            اگر (split_ریشه_Primary_Ios) {
                m_بالاترین_Scope_ریشه_Phase = درست;
                برای (ثابت TraceEntry& entry : m_entries) {
                    AstVarScope* ثابت vscp = entry.vscp();
                    اگر (!(vscp && vscp->varp()->isPrimaryIO())) ادامه;
                    emitEntry(entry);
                }
                m_بالاترین_Scope_ریشه_Phase = نادرست;
                pathAdjustor.unwind();
                m_بالاترین_Scope_ریشه_Func_شمار = m_subFuncps.size();
                اگر (m_بالاترین_Scope_ریشه_Func_شمار) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = درست;
                }
            }
            برای (ثابت TraceEntry& entry : m_entries) {
                AstVarScope* ثابت vscp = entry.vscp();
                اگر (split_ریشه_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) ادامه;
                emitEntry(entry);
            }
            pathAdjustor.unwind();
            m_traVscp = nullptr;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        اگر (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            ثابت std::string parentPath = path.substr(0, lastDot + 1);

            برای (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                اگر (!VString::startsWith(refName, parentPath)) ادامه;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                اگر (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_درخت(), intfRefp);
                }
            }
        }
    }
    تهی visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        اگر (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) بازگشت;
        اگر (nodep->varp()->isClassMember()) بازگشت;
        اگر (nodep->varp()->isFuncLocal()) بازگشت;

        // When creating a --lib-create library ...
        اگر (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            اگر (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) بازگشت;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    تهی visit(AstConstDType* nodep) override {
        اگر (!m_traVscp) بازگشت;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = درست;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    تهی visit(AstRefDType* nodep) override {
        اگر (!m_traVscp) بازگشت;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    تهی visit(AstIfaceRefDType* /*nodep*/) override {
        اگر (!m_traVscp) بازگشت;
        // Insert a placeholder to be fixed up later
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گره_Stmt* ثابت stmtp
            = جدید AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    تهی visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        اگر (!m_traVscp) بازگشت;

        اگر (v3Global.opt.traceMaxArray()
            && static_cast<صحیح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            بازگشت;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        اگر (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            بازگشت;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        اگر (isBasicIO()) m_skipDtypeFunc = درست;

        اگر (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, نادرست);
    }
    تهی visit(AstPackArrayDType* nodep) override {
        اگر (!m_traVscp) بازگشت;

        اگر (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            بازگشت;
        }

        Ast_گره_DType* ثابت subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        اگر (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            اگر (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                بازگشت;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        اگر (isBasicIO()) m_skipDtypeFunc = درست;

        اگر (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, نادرست);
    }
    تهی visit(AstStructDType* nodep) override {
        اگر (!m_traVscp) بازگشت;

        اگر (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            بازگشت;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        اگر (isBasicIO()) m_skipDtypeFunc = درست;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        اگر (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, نادرست);
    }
    تهی visit(AstUnionDType* nodep) override {
        اگر (!m_traVscp) بازگشت;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = درست;

        اگر (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            بازگشت;
        }

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        صحیح nMembers = 0;
        برای (Ast_گره* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        اگر (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } وگرنه {
            addToSubFunc(جدید Ast_Trace_افزودن_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            برای (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(جدید Ast_Trace_برداشت_Prefix{flp});
        }
    }
    تهی visit(AstBasicDType* nodep) override {
        اگر (!m_traVscp) بازگشت;
        اگر (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } وگرنه {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    تهی visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    تهی visit(Ast_گره_DType*) override {
        // Note more specific dtypes above
        اگر (!m_traVscp) بازگشت;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_بالاترین_Scopep{nodep->بالاترین_Scopep()} {
        File_سطر* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterateAndNextConstNull(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_Prefix_افزودن_برداشت();

        ثابت std::vector<AstCFunc*>& بالاترین_Scope_Funcps = m_scopeInitFuncps.at(m_بالاترین_Scopep->scopep());
        AstCFunc* ریشه_Funcp = nullptr;
        اگر (!v3Global.opt.libCreate().empty()) {
            ریشه_Funcp = newCFunc(flp, "trace_init_root");
            ریشه_Funcp->entryPoint(درست);
            برای (size_t i = 0; i < m_بالاترین_Scope_ریشه_Func_شمار; ++i) {
                AstCCall* ثابت callp = جدید AstCCall{flp, بالاترین_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                ریشه_Funcp->addStmtsp(callp->makeStmt());
            }
            اگر (!m_بالاترین_Scope_ریشه_Func_شمار) ریشه_Funcp->addStmtsp(جدید AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        برای (size_t i = m_بالاترین_Scope_ریشه_Func_شمار; i < بالاترین_Scope_Funcps.size(); ++i) {
            AstCFunc* ثابت funcp = بالاترین_Scope_Funcps.at(i);
            AstCCall* ثابت callp = جدید AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_بالاترین_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        اگر (m_بالاترین_Funcps.empty()) add_To_بالاترین_Func(جدید AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        اگر (m_بالاترین_Funcps.size() > 1) {
            AstCFunc* ثابت بالاترین_Funcp = newCFunc(flp, "");
            برای (AstCFunc* funcp : m_بالاترین_Funcps) {
                AstCCall* ثابت callp = جدید AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                بالاترین_Funcp->addStmtsp(callp->makeStmt());
            }
            m_بالاترین_Funcps.clear();
            m_بالاترین_Funcps.افزودن_back(بالاترین_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت بالاترین_Funcp = m_بالاترین_Funcps.front();
        بالاترین_Funcp->name("trace_init_top");
        بالاترین_Funcp->entryPoint(درست);

        اگر (ریشه_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ریشه_Funcp);
        checkCalls(بالاترین_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

تهی V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("tracedecl", 0, dump_درخت_Either_Level() >= 3);
}
