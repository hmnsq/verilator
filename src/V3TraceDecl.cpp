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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3TraceDecl.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Number.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <cstdint>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <tuple>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

വർഗ്ഗം PathAdjustor final {
    File_വരി* സ്ഥിരം m_flp;  // FileLine used for created nodes
    std::function<ശൂന്യം(Ast_കണ്ണി_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_അട്ടി{""};  // Stack of current paths

    സ്ഥിതം constexpr അക്ഷരം SEPARATOR = ' ';

പൊതു:
    explicit PathAdjustor(File_വരി* flp, std::function<ശൂന്യം(Ast_കണ്ണി_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    ശൂന്യം adjust(സ്ഥിരം string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        ആകുമ്പോൾ (!VString::startsWith(newPath, m_അട്ടി.back())) {
            m_emit(പുതിയ Ast_Trace_എടുക്കുക_Prefix{m_flp});
            m_അട്ടി.എടുക്കുക_back();
        }

        എങ്കിൽ (newPath == m_അട്ടി.back()) തിരികെ;

        സ്ഥിരം VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        സ്ഥിരം std::string extraPrefix = newPath.substr(m_അട്ടി.back().size());
        size_t begin = 0;
        സ്ഥിരം size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        ആകുമ്പോൾ (സത്യം) {
            സ്ഥിരം size_t end = extraPrefix.find(SEPARATOR, begin);
            എങ്കിൽ (end == string::npos) നിർത്തുക;
            സ്ഥിരം string& extra = extraPrefix.substr(begin, end - begin);
            എങ്കിൽ (end == last) {
                m_emit(പുതിയ Ast_Trace_തള്ളുക_Prefix{m_flp, extra, lastScopeType});
            } അല്ലെങ്കിൽ {
                m_emit(പുതിയ Ast_Trace_തള്ളുക_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_അട്ടി.തള്ളുക_back(m_അട്ടി.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    ശൂന്യം unwind() {
        ആകുമ്പോൾ (m_അട്ടി.size() > 1) {
            m_emit(പുതിയ Ast_Trace_എടുക്കുക_Prefix{m_flp});
            m_അട്ടി.എടുക്കുക_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

വർഗ്ഗം TraceDeclVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_മുകൾ_Scope* സ്ഥിരം m_മുകൾ_Scopep;  // The singleton AstTopScope
    സ്ഥിരം AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_മുകൾ_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<സ്ഥിരം AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ഘടന Dtype_Func_താക്കോൽ final {
        സ്ഥിരം Ast_കണ്ണി_DType* dtypep;
        VVarType varType;
        തർക്കം operator==(സ്ഥിരം Dtype_Func_താക്കോൽ& other) സ്ഥിരം {
            തിരികെ dtypep == other.dtypep && varType == other.varType;
        }
    };
    ഘടന Dtype_Func_താക്കോൽ_Hash final {
        size_t operator()(സ്ഥിരം Dtype_Func_താക്കോൽ& താക്കോൽ) സ്ഥിരം {
            തിരികെ std::hash<സ്ഥിരം Ast_കണ്ണി_DType*>{}(താക്കോൽ.dtypep)
                   ^ (std::hash<uint8_t>{}(താക്കോൽ.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_താക്കോൽ, AstCFunc*, Dtype_Func_താക്കോൽ_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    സ്ഥിരം string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    തർക്കം m_skipDtypeFunc = അസത്യം;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    പൂർണ്ണസംഖ്യ m_മുകൾ_Func_Size = 0;  // Size of the top function currently being built
    പൂർണ്ണസംഖ്യ m_subFuncSize = 0;  // Size of the sub function currently being built
    തർക്കം m_forceNewLeaf = അസത്യം;  // Force next addToSubFunc to start a fresh leaf
    size_t m_മുകൾ_Scope_വേര്_Func_എണ്ണം = 0;  // Top-scope init functions used only for wrapper IOs
    തർക്കം m_മുകൾ_Scope_വേര്_Phase = അസത്യം;  // Emitting top-scope wrapper IO declarations
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
    // Trace init functions to for each scope
    std::unordered_map<സ്ഥിരം AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, സ്ഥിരം AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_കണ്ണി_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_കണ്ണി_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    വർഗ്ഗം TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        ശൂന്യം init(സ്ഥിരം std::string& name, Ast_കണ്ണി* nodep, തർക്കം in_മുകൾ_Scope) {
            // Compute path in hierarchy and item name
            സ്ഥിരം std::string& vcdName = Ast_കണ്ണി::vcdName(name);
            AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
            എങ്കിൽ (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } അല്ലെങ്കിൽ {
                // Other Variable
                സ്ഥിരം size_t pos = vcdName.rfind(' ');
                സ്ഥിരം size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            എങ്കിൽ (in_മുകൾ_Scope && !v3Global.opt.libCreate().empty()) {
                സ്ഥിരം size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                സ്ഥിരം std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                എങ്കിൽ (prefix == "$rootio") തിരികെ;
                m_path = m_path.substr(start + 1);
                എങ്കിൽ (v3Global.rootp()->trace_Lib_മുകൾ_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    പൊതു:
        explicit TraceEntry(സ്ഥിരം AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_മുകൾ());
        }
        explicit TraceEntry(സ്ഥിരം AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_മുകൾ());
        }
        പൂർണ്ണസംഖ്യ operatorCompare(സ്ഥിരം TraceEntry& b) സ്ഥിരം {
            എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ cmp = path().compare(b.path())) തിരികെ cmp < 0;
            എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ cmp = fileline().operatorCompare(b.fileline())) തിരികെ cmp < 0;
            തിരികെ name() < b.name();
        }
        AstVarScope* vscp() സ്ഥിരം { തിരികെ m_vscp; }
        AstCell* cellp() സ്ഥിരം { തിരികെ m_cellp; }
        സ്ഥിരം std::string& path() സ്ഥിരം { തിരികെ m_path; }
        ശൂന്യം path(സ്ഥിരം std::string& path) { m_path = path; }
        സ്ഥിരം std::string& name() സ്ഥിരം { തിരികെ m_name; }
        File_വരി& fileline() സ്ഥിരം { തിരികെ m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_കണ്ണി_വ്യഞ്ജകം* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(സ്ഥിരം AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        സ്ഥിരം AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (!varp->isTrace()) തിരികെ "Verilator trace_off";
        എങ്കിൽ (!nodep->isTrace()) തിരികെ "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        എങ്കിൽ (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            തിരികെ "Automatic variable";

        സ്ഥിരം പൂർണ്ണസംഖ്യ width = recurseDTypeWidth(nodep->varp()->dtypep());
        എങ്കിൽ (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            തിരികെ "Width " + cvtToStr(width) + " > --trace-max-width";

        സ്ഥിരം string prettyName = nodep->prettyName();
        എങ്കിൽ (!V3Control::getScopeTraceOn(prettyName)) തിരികെ "Vlt scope trace_off";
        എങ്കിൽ (!v3Global.opt.traceUnderscore()) {
            എങ്കിൽ (!prettyName.empty() && prettyName[0] == '_') തിരികെ "Leading underscore";
            എങ്കിൽ (prettyName.find("._") != string::npos) തിരികെ "Inlined leading underscore";
        }
        തിരികെ ""s;
    }

    പൂർണ്ണസംഖ്യ recurseDTypeWidth(സ്ഥിരം Ast_കണ്ണി_DType* nodep) {
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* adtypep = VN_CAST(nodep, കണ്ണി_Array_DType))
            തിരികെ recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        തിരികെ nodep->width();
    }

    AstCFunc* newCFunc(File_വരി* flp, സ്ഥിരം string& name) {
        AstScope* സ്ഥിരം മുകൾ_Scopep = m_മുകൾ_Scopep->scopep();
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{flp, name, മുകൾ_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(സത്യം);
        funcp->isStatic(അസത്യം);
        funcp->isLoose(സത്യം);
        funcp->slow(സത്യം);
        മുകൾ_Scopep->addBlocksp(funcp);
        തിരികെ funcp;
    }

    ശൂന്യം add_To_മുകൾ_Func(Ast_കണ്ണി_Stmt* stmtp) {
        എങ്കിൽ (m_മുകൾ_Func_Size > m_funcSizeLimit || m_മുകൾ_Funcps.empty()) {
            m_മുകൾ_Func_Size = 0;
            //
            സ്ഥിരം string n = cvtToStr(m_മുകൾ_Funcps.size());
            സ്ഥിരം string name{"trace_init_top__" + n};
            AstCFunc* സ്ഥിരം funcp = newCFunc(m_മുകൾ_Scopep->fileline(), name);
            m_മുകൾ_Funcps.തള്ളുക_back(funcp);
        }
        m_മുകൾ_Funcps.back()->addStmtsp(stmtp);
        m_മുകൾ_Func_Size += stmtp->കണ്ണി_എണ്ണം();
    }

    ശൂന്യം addToSubFunc(Ast_കണ്ണി_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        എങ്കിൽ (m_dtypeSubFuncp) {
            എങ്കിൽ (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->കണ്ണി_എണ്ണം();
            തിരികെ;
        }
        // Defer trace splitting until V3Trace
        എങ്കിൽ (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = അസത്യം;
            File_വരി* സ്ഥിരം flp = m_currScopep->fileline();
            സ്ഥിരം string n = cvtToStr(m_subFuncps.size());
            സ്ഥിരം string name
                = m_currScopep == m_മുകൾ_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_മുകൾ_Scope_വേര്_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* സ്ഥിരം funcp = newCFunc(flp, name);
            funcp->addStmtsp(പുതിയ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.തള്ളുക_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->കണ്ണി_എണ്ണം();
    }

    AstTraceDecl* addTraceDecl(സ്ഥിരം VNumRange& arrayRange,
                               പൂർണ്ണസംഖ്യ widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* സ്ഥിരം dtypeCallp = nullptr) {
        VNumRange bitRange;
        എങ്കിൽ (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_വരി* സ്ഥിരം flp = m_traVscp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = m_traValuep->clone_വൃക്ഷം(അസത്യം);
        സ്ഥിരം തർക്കം validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* സ്ഥിരം newp
            = പുതിയ AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        എങ്കിൽ (validOffset) {
            newp->code(m_offset);
            എങ്കിൽ (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* സ്ഥിരം refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(പുതിയ AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
            });
        } അല്ലെങ്കിൽ {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        തിരികെ newp;
    }

    ശൂന്യം addIgnore(സ്ഥിരം string& why) {
        ++m_statIgnSigs;
        സ്ഥിരം std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        എങ്കിൽ (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    ശൂന്യം fixupPlaceholder(സ്ഥിരം std::string& path, Ast_കണ്ണി_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        സ്ഥിരം auto it = m_pathToScopep.find(Ast_കണ്ണി::prettyName(path));
        എങ്കിൽ (it != m_pathToScopep.end()) {
            സ്ഥിരം AstScope* സ്ഥിരം scopep = it->second;
            File_വരി* സ്ഥിരം flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_കണ്ണി* stmtp = nullptr;
            വേണ്ടി (AstCFunc* സ്ഥിരം subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_കണ്ണി::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            എങ്കിൽ (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_വൃക്ഷം(), placeholderp);
    }

    ശൂന്യം fixupLibStub(സ്ഥിരം std::string& path, Ast_കണ്ണി_Stmt* placeholderp) {
        File_വരി* സ്ഥിരം flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_കണ്ണി::prettyName(path);
        എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            സ്ഥിരം std::string മുകൾ_Prefix = v3Global.rootp()->trace_Lib_മുകൾ_Name() + ".";
            എങ്കിൽ (VString::startsWith(name, മുകൾ_Prefix)) name = name.substr(മുകൾ_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* സ്ഥിരം initp = പുതിയ AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(പുതിയ AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_വൃക്ഷം(), placeholderp);
    }

    ശൂന്യം fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        വേണ്ടി (സ്ഥിരം auto& item : m_cellInitPlaceholders) {
            സ്ഥിരം AstScope* സ്ഥിരം parentp = std::get<0>(item);
            സ്ഥിരം AstCell* സ്ഥിരം cellp = std::get<1>(item);
            Ast_കണ്ണി_Stmt* സ്ഥിരം placeholderp = std::get<2>(item);
            സ്ഥിരം std::string path = parentp->name() + "__DOT__" + cellp->name();
            എങ്കിൽ (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } അല്ലെങ്കിൽ {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        വേണ്ടി (സ്ഥിരം auto& item : m_ifaceRefInitPlaceholders) {
            സ്ഥിരം AstVarScope* സ്ഥിരം vscp = std::get<0>(item);
            Ast_കണ്ണി_Stmt* സ്ഥിരം placeholderp = std::get<1>(item);
            സ്ഥിരം std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    ശൂന്യം remove_Redundant_Prefix_തള്ളുക_എടുക്കുക() {
        വേണ്ടി (സ്ഥിരം auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            വേണ്ടി (AstCFunc* സ്ഥിരം funcp : pair.second) {
                Ast_കണ്ണി* prevp = nullptr;
                Ast_കണ്ണി* currp = funcp->stmtsp();
                ആകുമ്പോൾ (currp) {
                    Ast_കണ്ണി* സ്ഥിരം nextp = currp->nextp();
                    എങ്കിൽ (VN_IS(prevp, Trace_തള്ളുക_Prefix) && VN_IS(currp, Trace_എടുക്കുക_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_വൃക്ഷം(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_വൃക്ഷം(), currp);
                    }
                    എങ്കിൽ (!nextp) നിർത്തുക;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    ശൂന്യം checkCalls(സ്ഥിരം AstCFunc* funcp) {
        എങ്കിൽ (!v3Global.opt.debugCheck()) തിരികെ;
        checkCallsRecurse(funcp);
        എങ്കിൽ (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            വേണ്ടി (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    ശൂന്യം checkCallsRecurse(സ്ഥിരം AstCFunc* funcp) {
        funcp->foreach([ഇത്](സ്ഥിരം Ast_കണ്ണി* nodep) {
            എങ്കിൽ (സ്ഥിരം AstTraceDecl* സ്ഥിരം declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCCall* സ്ഥിരം ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    തർക്കം isBasicIO() { തിരികെ m_traVscp->varp()->isVLIO(); }
    ശൂന്യം newDeclSubFunc() {
        File_വരി* സ്ഥിരം flp = m_dtypeFuncp->fileline();
        AstCFunc* സ്ഥിരം subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* സ്ഥിരം subCallp = പുതിയ AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    ശൂന്യം newDeclFunc(Ast_കണ്ണി_DType* nodep) {
        Ast_കണ്ണി_DType* സ്ഥിരം skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_വരി* സ്ഥിരം flp = skipTypep->fileline();

        സ്ഥിരം Dtype_Func_താക്കോൽ dtype_താക്കോൽ{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_താക്കോൽ, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        എങ്കിൽ (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        എങ്കിൽ (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            എങ്കിൽ (AstStructDType* സ്ഥിരം dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstPackArrayDType* സ്ഥിരം dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, സത്യം);
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(അസത്യം, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    ശൂന്യം declUnpackedArray(AstUnpackArrayDType* സ്ഥിരം nodep, തർക്കം newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_വരി* സ്ഥിരം flp = nodep->fileline();

        addToSubFunc(പുതിയ Ast_Trace_തള്ളുക_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->ഇടത്(), nodep->വലത്(), !newFunc});

        എങ്കിൽ (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            എങ്കിൽ (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } അല്ലെങ്കിൽ {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_DType* സ്ഥിരം subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ i : nodep->declRange().seq_ഇടത്_To_വലത്()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_വൃക്ഷം(അസത്യം);
                m_traValuep = പുതിയ AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_വൃക്ഷം(), m_traValuep);
            }
        }

        addToSubFunc(പുതിയ Ast_Trace_എടുക്കുക_Prefix{flp});
    }
    ശൂന്യം declPackedArray(AstPackArrayDType* സ്ഥിരം nodep, തർക്കം newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_കണ്ണി_DType* സ്ഥിരം subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_വരി* സ്ഥിരം flp = nodep->fileline();

        addToSubFunc(പുതിയ Ast_Trace_തള്ളുക_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->ഇടത്(), nodep->വലത്(), !newFunc});

        // Always iterate left index to right index
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ i : nodep->declRange().seq_ഇടത്_To_വലത്()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_വൃക്ഷം(അസത്യം);
            m_traValuep = പുതിയ AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_വൃക്ഷം(), m_traValuep = nullptr);
        }

        addToSubFunc(പുതിയ Ast_Trace_എടുക്കുക_Prefix{flp});
    }
    ശൂന്യം declStruct(AstStructDType* സ്ഥിരം nodep, തർക്കം newFunc) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        പൂർണ്ണസംഖ്യ nMembers = 0;
        വേണ്ടി (Ast_കണ്ണി* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        എങ്കിൽ (!nodep->packed()) {
            addToSubFunc(പുതിയ Ast_Trace_തള്ളുക_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            വേണ്ടി (സ്ഥിരം AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_കണ്ണി_DType* സ്ഥിരം subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_വൃക്ഷം(അസത്യം);
                m_traValuep = പുതിയ AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_വൃക്ഷം(), m_traValuep);
            }
            addToSubFunc(പുതിയ Ast_Trace_എടുക്കുക_Prefix{flp});
        } അല്ലെങ്കിൽ {
            addToSubFunc(പുതിയ Ast_Trace_തള്ളുക_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            വേണ്ടി (സ്ഥിരം AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_കണ്ണി_DType* സ്ഥിരം subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_വൃക്ഷം(അസത്യം);
                m_traValuep = പുതിയ AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_വൃക്ഷം(), m_traValuep);
            }
            addToSubFunc(പുതിയ Ast_Trace_എടുക്കുക_Prefix{flp});
        }
    }

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        എങ്കിൽ (nodep->modp()->verilatorLib()) തിരികെ;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstCell* സ്ഥിരം cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        എങ്കിൽ (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](സ്ഥിരം TraceEntry& a, സ്ഥിരം TraceEntry& b) { തിരികെ a.operatorCompare(b); });

            File_വരി* സ്ഥിരം flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_കണ്ണി_Stmt* stmtp) { addToSubFunc(stmtp); }};
            സ്ഥിരം തർക്കം split_വേര്_Primary_Ios = nodep->is_മുകൾ() && !v3Global.opt.libCreate().empty();
            സ്ഥിരം auto emitEntry = [&](സ്ഥിരം TraceEntry& entry) {
                AstVarScope* സ്ഥിരം vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_കണ്ണി*>(entry.cellp())
                                                    : static_cast<Ast_കണ്ണി*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                എങ്കിൽ (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    സ്ഥിരം string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    എങ്കിൽ (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } അല്ലെങ്കിൽ {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = പുതിയ AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        എങ്കിൽ (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_വൃക്ഷം(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } അല്ലെങ്കിൽ {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* സ്ഥിരം cellp = entry.cellp();
                    Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = പുതിയ AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            എങ്കിൽ (split_വേര്_Primary_Ios) {
                m_മുകൾ_Scope_വേര്_Phase = സത്യം;
                വേണ്ടി (സ്ഥിരം TraceEntry& entry : m_entries) {
                    AstVarScope* സ്ഥിരം vscp = entry.vscp();
                    എങ്കിൽ (!(vscp && vscp->varp()->isPrimaryIO())) തുടരുക;
                    emitEntry(entry);
                }
                m_മുകൾ_Scope_വേര്_Phase = അസത്യം;
                pathAdjustor.unwind();
                m_മുകൾ_Scope_വേര്_Func_എണ്ണം = m_subFuncps.size();
                എങ്കിൽ (m_മുകൾ_Scope_വേര്_Func_എണ്ണം) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = സത്യം;
                }
            }
            വേണ്ടി (സ്ഥിരം TraceEntry& entry : m_entries) {
                AstVarScope* സ്ഥിരം vscp = entry.vscp();
                എങ്കിൽ (split_വേര്_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) തുടരുക;
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
        സ്ഥിരം std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        സ്ഥിരം AstCell* സ്ഥിരം cellp = nodep->aboveCellp();
        എങ്കിൽ (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            സ്ഥിരം size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            സ്ഥിരം std::string parentPath = path.substr(0, lastDot + 1);

            വേണ്ടി (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                സ്ഥിരം std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                എങ്കിൽ (!VString::startsWith(refName, parentPath)) തുടരുക;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                എങ്കിൽ (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_വൃക്ഷം(), intfRefp);
                }
            }
        }
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        എങ്കിൽ (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) തിരികെ;
        എങ്കിൽ (nodep->varp()->isClassMember()) തിരികെ;
        എങ്കിൽ (nodep->varp()->isFuncLocal()) തിരികെ;

        // When creating a --lib-create library ...
        എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            എങ്കിൽ (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) തിരികെ;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    ശൂന്യം visit(AstConstDType* nodep) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = സത്യം;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    ശൂന്യം visit(AstIfaceRefDType* /*nodep*/) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;
        // Insert a placeholder to be fixed up later
        File_വരി* സ്ഥിരം flp = m_traVscp->fileline();
        Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp
            = പുതിയ AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    ശൂന്യം visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        എങ്കിൽ (!m_traVscp) തിരികെ;

        എങ്കിൽ (v3Global.opt.traceMaxArray()
            && static_cast<പൂർണ്ണസംഖ്യ>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            തിരികെ;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        എങ്കിൽ (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            തിരികെ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        എങ്കിൽ (isBasicIO()) m_skipDtypeFunc = സത്യം;

        എങ്കിൽ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, അസത്യം);
    }
    ശൂന്യം visit(AstPackArrayDType* nodep) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;

        എങ്കിൽ (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            തിരികെ;
        }

        Ast_കണ്ണി_DType* സ്ഥിരം subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        എങ്കിൽ (AstBasicDType* സ്ഥിരം basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            എങ്കിൽ (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                തിരികെ;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        എങ്കിൽ (isBasicIO()) m_skipDtypeFunc = സത്യം;

        എങ്കിൽ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, അസത്യം);
    }
    ശൂന്യം visit(AstStructDType* nodep) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;

        എങ്കിൽ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            തിരികെ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        എങ്കിൽ (isBasicIO()) m_skipDtypeFunc = സത്യം;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        എങ്കിൽ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, അസത്യം);
    }
    ശൂന്യം visit(AstUnionDType* nodep) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = സത്യം;

        എങ്കിൽ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            തിരികെ;
        }

        VL_RESTORER_COPY(m_traName);
        File_വരി* സ്ഥിരം flp = nodep->fileline();

        പൂർണ്ണസംഖ്യ nMembers = 0;
        വേണ്ടി (Ast_കണ്ണി* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        എങ്കിൽ (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } അല്ലെങ്കിൽ {
            addToSubFunc(പുതിയ Ast_Trace_തള്ളുക_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            വേണ്ടി (സ്ഥിരം AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_കണ്ണി_DType* സ്ഥിരം subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(പുതിയ Ast_Trace_എടുക്കുക_Prefix{flp});
        }
    }
    ശൂന്യം visit(AstBasicDType* nodep) override {
        എങ്കിൽ (!m_traVscp) തിരികെ;
        എങ്കിൽ (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } അല്ലെങ്കിൽ {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    ശൂന്യം visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    ശൂന്യം visit(Ast_കണ്ണി_DType*) override {
        // Note more specific dtypes above
        എങ്കിൽ (!m_traVscp) തിരികെ;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_മുകൾ_Scopep{nodep->മുകൾ_Scopep()} {
        File_വരി* സ്ഥിരം flp = nodep->fileline();

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
        remove_Redundant_Prefix_തള്ളുക_എടുക്കുക();

        സ്ഥിരം std::vector<AstCFunc*>& മുകൾ_Scope_Funcps = m_scopeInitFuncps.at(m_മുകൾ_Scopep->scopep());
        AstCFunc* വേര്_Funcp = nullptr;
        എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            വേര്_Funcp = newCFunc(flp, "trace_init_root");
            വേര്_Funcp->entryPoint(സത്യം);
            വേണ്ടി (size_t i = 0; i < m_മുകൾ_Scope_വേര്_Func_എണ്ണം; ++i) {
                AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, മുകൾ_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                വേര്_Funcp->addStmtsp(callp->makeStmt());
            }
            എങ്കിൽ (!m_മുകൾ_Scope_വേര്_Func_എണ്ണം) വേര്_Funcp->addStmtsp(പുതിയ AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        വേണ്ടി (size_t i = m_മുകൾ_Scope_വേര്_Func_എണ്ണം; i < മുകൾ_Scope_Funcps.size(); ++i) {
            AstCFunc* സ്ഥിരം funcp = മുകൾ_Scope_Funcps.at(i);
            AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_മുകൾ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        എങ്കിൽ (m_മുകൾ_Funcps.empty()) add_To_മുകൾ_Func(പുതിയ AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        എങ്കിൽ (m_മുകൾ_Funcps.size() > 1) {
            AstCFunc* സ്ഥിരം മുകൾ_Funcp = newCFunc(flp, "");
            വേണ്ടി (AstCFunc* funcp : m_മുകൾ_Funcps) {
                AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                മുകൾ_Funcp->addStmtsp(callp->makeStmt());
            }
            m_മുകൾ_Funcps.clear();
            m_മുകൾ_Funcps.തള്ളുക_back(മുകൾ_Funcp);
        }

        // Set name of top level function
        AstCFunc* സ്ഥിരം മുകൾ_Funcp = m_മുകൾ_Funcps.front();
        മുകൾ_Funcp->name("trace_init_top");
        മുകൾ_Funcp->entryPoint(സത്യം);

        എങ്കിൽ (വേര്_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(വേര്_Funcp);
        checkCalls(മുകൾ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

ശൂന്യം V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("tracedecl", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
