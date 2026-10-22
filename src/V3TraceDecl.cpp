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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3TraceDecl.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Number.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <cstdint>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <limits>
#அந்தர்பாவ <tuple>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

வகுப்பு PathAdjustor final {
    File_வரி* மாறிலி m_flp;  // FileLine used for created nodes
    std::function<வெறுமை(Ast_முனை_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_அடுக்கு{""};  // Stack of current paths

    நிலையான constexpr எழுத்து SEPARATOR = ' ';

பொது:
    explicit PathAdjustor(File_வரி* flp, std::function<வெறுமை(Ast_முனை_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    வெறுமை adjust(மாறிலி string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        போது (!VString::startsWith(newPath, m_அடுக்கு.back())) {
            m_emit(புதிய Ast_Trace_எடு_Prefix{m_flp});
            m_அடுக்கு.எடு_back();
        }

        எனில் (newPath == m_அடுக்கு.back()) திரும்பு;

        மாறிலி VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        மாறிலி std::string extraPrefix = newPath.substr(m_அடுக்கு.back().size());
        size_t begin = 0;
        மாறிலி size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        போது (உண்மை) {
            மாறிலி size_t end = extraPrefix.find(SEPARATOR, begin);
            எனில் (end == string::npos) நிறுத்து;
            மாறிலி string& extra = extraPrefix.substr(begin, end - begin);
            எனில் (end == last) {
                m_emit(புதிய Ast_Trace_தள்ளு_Prefix{m_flp, extra, lastScopeType});
            } இல்லையெனில் {
                m_emit(புதிய Ast_Trace_தள்ளு_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_அடுக்கு.தள்ளு_back(m_அடுக்கு.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    வெறுமை unwind() {
        போது (m_அடுக்கு.size() > 1) {
            m_emit(புதிய Ast_Trace_எடு_Prefix{m_flp});
            m_அடுக்கு.எடு_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

வகுப்பு TraceDeclVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_உச்சி_Scope* மாறிலி m_உச்சி_Scopep;  // The singleton AstTopScope
    மாறிலி AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_உச்சி_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<மாறிலி AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    கட்டமைப்பு Dtype_Func_சாவி final {
        மாறிலி Ast_முனை_DType* dtypep;
        VVarType varType;
        தருக்க operator==(மாறிலி Dtype_Func_சாவி& other) மாறிலி {
            திரும்பு dtypep == other.dtypep && varType == other.varType;
        }
    };
    கட்டமைப்பு Dtype_Func_சாவி_Hash final {
        size_t operator()(மாறிலி Dtype_Func_சாவி& சாவி) மாறிலி {
            திரும்பு std::hash<மாறிலி Ast_முனை_DType*>{}(சாவி.dtypep)
                   ^ (std::hash<uint8_t>{}(சாவி.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_சாவி, AstCFunc*, Dtype_Func_சாவி_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    மாறிலி string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    தருக்க m_skipDtypeFunc = பொய்;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    முழு m_உச்சி_Func_Size = 0;  // Size of the top function currently being built
    முழு m_subFuncSize = 0;  // Size of the sub function currently being built
    தருக்க m_forceNewLeaf = பொய்;  // Force next addToSubFunc to start a fresh leaf
    size_t m_உச்சி_Scope_வேர்_Func_மொத்தம் = 0;  // Top-scope init functions used only for wrapper IOs
    தருக்க m_உச்சி_Scope_வேர்_Phase = பொய்;  // Emitting top-scope wrapper IO declarations
    மாறிலி முழு m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<முழு>::max();
    // Trace init functions to for each scope
    std::unordered_map<மாறிலி AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, மாறிலி AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_முனை_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_முனை_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    வகுப்பு TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        வெறுமை init(மாறிலி std::string& name, Ast_முனை* nodep, தருக்க in_உச்சி_Scope) {
            // Compute path in hierarchy and item name
            மாறிலி std::string& vcdName = Ast_முனை::vcdName(name);
            AstVar* மாறிலி varp = VN_CAST(nodep, Var);
            எனில் (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } இல்லையெனில் எனில் (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } இல்லையெனில் {
                // Other Variable
                மாறிலி size_t pos = vcdName.rfind(' ');
                மாறிலி size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            எனில் (in_உச்சி_Scope && !v3Global.opt.libCreate().empty()) {
                மாறிலி size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                மாறிலி std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                எனில் (prefix == "$rootio") திரும்பு;
                m_path = m_path.substr(start + 1);
                எனில் (v3Global.rootp()->trace_Lib_உச்சி_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    பொது:
        explicit TraceEntry(மாறிலி AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_உச்சி());
        }
        explicit TraceEntry(மாறிலி AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_உச்சி());
        }
        முழு operatorCompare(மாறிலி TraceEntry& b) மாறிலி {
            எனில் (மாறிலி முழு cmp = path().compare(b.path())) திரும்பு cmp < 0;
            எனில் (மாறிலி முழு cmp = fileline().operatorCompare(b.fileline())) திரும்பு cmp < 0;
            திரும்பு name() < b.name();
        }
        AstVarScope* vscp() மாறிலி { திரும்பு m_vscp; }
        AstCell* cellp() மாறிலி { திரும்பு m_cellp; }
        மாறிலி std::string& path() மாறிலி { திரும்பு m_path; }
        வெறுமை path(மாறிலி std::string& path) { m_path = path; }
        மாறிலி std::string& name() மாறிலி { திரும்பு m_name; }
        File_வரி& fileline() மாறிலி { திரும்பு m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_முனை_கோவை* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(மாறிலி AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        மாறிலி AstVar* மாறிலி varp = nodep->varp();
        எனில் (!varp->isTrace()) திரும்பு "Verilator trace_off";
        எனில் (!nodep->isTrace()) திரும்பு "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        எனில் (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            திரும்பு "Automatic variable";

        மாறிலி முழு width = recurseDTypeWidth(nodep->varp()->dtypep());
        எனில் (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            திரும்பு "Width " + cvtToStr(width) + " > --trace-max-width";

        மாறிலி string prettyName = nodep->prettyName();
        எனில் (!V3Control::getScopeTraceOn(prettyName)) திரும்பு "Vlt scope trace_off";
        எனில் (!v3Global.opt.traceUnderscore()) {
            எனில் (!prettyName.empty() && prettyName[0] == '_') திரும்பு "Leading underscore";
            எனில் (prettyName.find("._") != string::npos) திரும்பு "Inlined leading underscore";
        }
        திரும்பு ""s;
    }

    முழு recurseDTypeWidth(மாறிலி Ast_முனை_DType* nodep) {
        எனில் (மாறிலி Ast_முனை_Array_DType* adtypep = VN_CAST(nodep, முனை_Array_DType))
            திரும்பு recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        திரும்பு nodep->width();
    }

    AstCFunc* newCFunc(File_வரி* flp, மாறிலி string& name) {
        AstScope* மாறிலி உச்சி_Scopep = m_உச்சி_Scopep->scopep();
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{flp, name, உச்சி_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(உண்மை);
        funcp->isStatic(பொய்);
        funcp->isLoose(உண்மை);
        funcp->slow(உண்மை);
        உச்சி_Scopep->addBlocksp(funcp);
        திரும்பு funcp;
    }

    வெறுமை add_To_உச்சி_Func(Ast_முனை_Stmt* stmtp) {
        எனில் (m_உச்சி_Func_Size > m_funcSizeLimit || m_உச்சி_Funcps.empty()) {
            m_உச்சி_Func_Size = 0;
            //
            மாறிலி string n = cvtToStr(m_உச்சி_Funcps.size());
            மாறிலி string name{"trace_init_top__" + n};
            AstCFunc* மாறிலி funcp = newCFunc(m_உச்சி_Scopep->fileline(), name);
            m_உச்சி_Funcps.தள்ளு_back(funcp);
        }
        m_உச்சி_Funcps.back()->addStmtsp(stmtp);
        m_உச்சி_Func_Size += stmtp->முனை_மொத்தம்();
    }

    வெறுமை addToSubFunc(Ast_முனை_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        எனில் (m_dtypeSubFuncp) {
            எனில் (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->முனை_மொத்தம்();
            திரும்பு;
        }
        // Defer trace splitting until V3Trace
        எனில் (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = பொய்;
            File_வரி* மாறிலி flp = m_currScopep->fileline();
            மாறிலி string n = cvtToStr(m_subFuncps.size());
            மாறிலி string name
                = m_currScopep == m_உச்சி_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_உச்சி_Scope_வேர்_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* மாறிலி funcp = newCFunc(flp, name);
            funcp->addStmtsp(புதிய AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.தள்ளு_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->முனை_மொத்தம்();
    }

    AstTraceDecl* addTraceDecl(மாறிலி VNumRange& arrayRange,
                               முழு widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* மாறிலி dtypeCallp = nullptr) {
        VNumRange bitRange;
        எனில் (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } இல்லையெனில் எனில் (மாறிலி AstBasicDType* மாறிலி bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_வரி* மாறிலி flp = m_traVscp->fileline();
        Ast_முனை_கோவை* valuep = m_traValuep->clone_மரம்(பொய்);
        மாறிலி தருக்க validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* மாறிலி newp
            = புதிய AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        எனில் (validOffset) {
            newp->code(m_offset);
            எனில் (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* மாறிலி refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(புதிய AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_மரம்(), refp);
            });
        } இல்லையெனில் {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        திரும்பு newp;
    }

    வெறுமை addIgnore(மாறிலி string& why) {
        ++m_statIgnSigs;
        மாறிலி std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        எனில் (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    வெறுமை fixupPlaceholder(மாறிலி std::string& path, Ast_முனை_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        மாறிலி auto it = m_pathToScopep.find(Ast_முனை::prettyName(path));
        எனில் (it != m_pathToScopep.end()) {
            மாறிலி AstScope* மாறிலி scopep = it->second;
            File_வரி* மாறிலி flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_முனை* stmtp = nullptr;
            க்கு (AstCFunc* மாறிலி subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* மாறிலி callp = புதிய AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_முனை::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            எனில் (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_மரம்(), placeholderp);
    }

    வெறுமை fixupLibStub(மாறிலி std::string& path, Ast_முனை_Stmt* placeholderp) {
        File_வரி* மாறிலி flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_முனை::prettyName(path);
        எனில் (!v3Global.opt.libCreate().empty()) {
            மாறிலி std::string உச்சி_Prefix = v3Global.rootp()->trace_Lib_உச்சி_Name() + ".";
            எனில் (VString::startsWith(name, உச்சி_Prefix)) name = name.substr(உச்சி_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* மாறிலி initp = புதிய AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(புதிய AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_மரம்(), placeholderp);
    }

    வெறுமை fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        க்கு (மாறிலி auto& item : m_cellInitPlaceholders) {
            மாறிலி AstScope* மாறிலி parentp = std::get<0>(item);
            மாறிலி AstCell* மாறிலி cellp = std::get<1>(item);
            Ast_முனை_Stmt* மாறிலி placeholderp = std::get<2>(item);
            மாறிலி std::string path = parentp->name() + "__DOT__" + cellp->name();
            எனில் (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } இல்லையெனில் {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        க்கு (மாறிலி auto& item : m_ifaceRefInitPlaceholders) {
            மாறிலி AstVarScope* மாறிலி vscp = std::get<0>(item);
            Ast_முனை_Stmt* மாறிலி placeholderp = std::get<1>(item);
            மாறிலி std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    வெறுமை remove_Redundant_Prefix_தள்ளு_எடு() {
        க்கு (மாறிலி auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            க்கு (AstCFunc* மாறிலி funcp : pair.second) {
                Ast_முனை* prevp = nullptr;
                Ast_முனை* currp = funcp->stmtsp();
                போது (currp) {
                    Ast_முனை* மாறிலி nextp = currp->nextp();
                    எனில் (VN_IS(prevp, Trace_தள்ளு_Prefix) && VN_IS(currp, Trace_எடு_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_மரம்(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_மரம்(), currp);
                    }
                    எனில் (!nextp) நிறுத்து;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    வெறுமை checkCalls(மாறிலி AstCFunc* funcp) {
        எனில் (!v3Global.opt.debugCheck()) திரும்பு;
        checkCallsRecurse(funcp);
        எனில் (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            க்கு (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    வெறுமை checkCallsRecurse(மாறிலி AstCFunc* funcp) {
        funcp->foreach([இது](மாறிலி Ast_முனை* nodep) {
            எனில் (மாறிலி AstTraceDecl* மாறிலி declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } இல்லையெனில் எனில் (மாறிலி AstCCall* மாறிலி ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    தருக்க isBasicIO() { திரும்பு m_traVscp->varp()->isVLIO(); }
    வெறுமை newDeclSubFunc() {
        File_வரி* மாறிலி flp = m_dtypeFuncp->fileline();
        AstCFunc* மாறிலி subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* மாறிலி subCallp = புதிய AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    வெறுமை newDeclFunc(Ast_முனை_DType* nodep) {
        Ast_முனை_DType* மாறிலி skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_வரி* மாறிலி flp = skipTypep->fileline();

        மாறிலி Dtype_Func_சாவி dtype_சாவி{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_சாவி, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        எனில் (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* மாறிலி callp = புதிய AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        எனில் (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            எனில் (AstStructDType* மாறிலி dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, உண்மை);
            } இல்லையெனில் எனில் (AstUnpackArrayDType* மாறிலி dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, உண்மை);
            } இல்லையெனில் எனில் (AstPackArrayDType* மாறிலி dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, உண்மை);
            } இல்லையெனில் {
                UASSERT_OBJ(பொய், skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    வெறுமை declUnpackedArray(AstUnpackArrayDType* மாறிலி nodep, தருக்க newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_வரி* மாறிலி flp = nodep->fileline();

        addToSubFunc(புதிய Ast_Trace_தள்ளு_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->இடது(), nodep->வலது(), !newFunc});

        எனில் (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            எனில் (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } இல்லையெனில் {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } இல்லையெனில் {
            Ast_முனை_DType* மாறிலி subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            க்கு (மாறிலி முழு i : nodep->declRange().seq_இடது_To_வலது()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_மரம்(பொய்);
                m_traValuep = புதிய AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_மரம்(), m_traValuep);
            }
        }

        addToSubFunc(புதிய Ast_Trace_எடு_Prefix{flp});
    }
    வெறுமை declPackedArray(AstPackArrayDType* மாறிலி nodep, தருக்க newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_முனை_DType* மாறிலி subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_வரி* மாறிலி flp = nodep->fileline();

        addToSubFunc(புதிய Ast_Trace_தள்ளு_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->இடது(), nodep->வலது(), !newFunc});

        // Always iterate left index to right index
        க்கு (மாறிலி முழு i : nodep->declRange().seq_இடது_To_வலது()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            மாறிலி முழு lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_மரம்(பொய்);
            m_traValuep = புதிய AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_மரம்(), m_traValuep = nullptr);
        }

        addToSubFunc(புதிய Ast_Trace_எடு_Prefix{flp});
    }
    வெறுமை declStruct(AstStructDType* மாறிலி nodep, தருக்க newFunc) {
        File_வரி* மாறிலி flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        முழு nMembers = 0;
        க்கு (Ast_முனை* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        எனில் (!nodep->packed()) {
            addToSubFunc(புதிய Ast_Trace_தள்ளு_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            க்கு (மாறிலி AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_முனை_DType* மாறிலி subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_மரம்(பொய்);
                m_traValuep = புதிய AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_மரம்(), m_traValuep);
            }
            addToSubFunc(புதிய Ast_Trace_எடு_Prefix{flp});
        } இல்லையெனில் {
            addToSubFunc(புதிய Ast_Trace_தள்ளு_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            க்கு (மாறிலி AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_முனை_DType* மாறிலி subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_மரம்(பொய்);
                m_traValuep = புதிய AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_மரம்(), m_traValuep);
            }
            addToSubFunc(புதிய Ast_Trace_எடு_Prefix{flp});
        }
    }

    // VISITORS
    வெறுமை visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        எனில் (nodep->modp()->verilatorLib()) திரும்பு;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        க்கு (Ast_முனை* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstCell* மாறிலி cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        எனில் (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](மாறிலி TraceEntry& a, மாறிலி TraceEntry& b) { திரும்பு a.operatorCompare(b); });

            File_வரி* மாறிலி flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_முனை_Stmt* stmtp) { addToSubFunc(stmtp); }};
            மாறிலி தருக்க split_வேர்_Primary_Ios = nodep->is_உச்சி() && !v3Global.opt.libCreate().empty();
            மாறிலி auto emitEntry = [&](மாறிலி TraceEntry& entry) {
                AstVarScope* மாறிலி vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_முனை*>(entry.cellp())
                                                    : static_cast<Ast_முனை*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                எனில் (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    மாறிலி string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    எனில் (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } இல்லையெனில் {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = புதிய AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        எனில் (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_மரம்(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } இல்லையெனில் {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* மாறிலி cellp = entry.cellp();
                    Ast_முனை_Stmt* மாறிலி stmtp = புதிய AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            எனில் (split_வேர்_Primary_Ios) {
                m_உச்சி_Scope_வேர்_Phase = உண்மை;
                க்கு (மாறிலி TraceEntry& entry : m_entries) {
                    AstVarScope* மாறிலி vscp = entry.vscp();
                    எனில் (!(vscp && vscp->varp()->isPrimaryIO())) தொடர்;
                    emitEntry(entry);
                }
                m_உச்சி_Scope_வேர்_Phase = பொய்;
                pathAdjustor.unwind();
                m_உச்சி_Scope_வேர்_Func_மொத்தம் = m_subFuncps.size();
                எனில் (m_உச்சி_Scope_வேர்_Func_மொத்தம்) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = உண்மை;
                }
            }
            க்கு (மாறிலி TraceEntry& entry : m_entries) {
                AstVarScope* மாறிலி vscp = entry.vscp();
                எனில் (split_வேர்_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) தொடர்;
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
        மாறிலி std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        மாறிலி AstCell* மாறிலி cellp = nodep->aboveCellp();
        எனில் (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            மாறிலி size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            மாறிலி std::string parentPath = path.substr(0, lastDot + 1);

            க்கு (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                மாறிலி std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                எனில் (!VString::startsWith(refName, parentPath)) தொடர்;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                எனில் (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_மரம்(), intfRefp);
                }
            }
        }
    }
    வெறுமை visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        எனில் (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) திரும்பு;
        எனில் (nodep->varp()->isClassMember()) திரும்பு;
        எனில் (nodep->varp()->isFuncLocal()) திரும்பு;

        // When creating a --lib-create library ...
        எனில் (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            எனில் (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) திரும்பு;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    வெறுமை visit(AstConstDType* nodep) override {
        எனில் (!m_traVscp) திரும்பு;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = உண்மை;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    வெறுமை visit(AstRefDType* nodep) override {
        எனில் (!m_traVscp) திரும்பு;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    வெறுமை visit(AstIfaceRefDType* /*nodep*/) override {
        எனில் (!m_traVscp) திரும்பு;
        // Insert a placeholder to be fixed up later
        File_வரி* மாறிலி flp = m_traVscp->fileline();
        Ast_முனை_Stmt* மாறிலி stmtp
            = புதிய AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    வெறுமை visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        எனில் (!m_traVscp) திரும்பு;

        எனில் (v3Global.opt.traceMaxArray()
            && static_cast<முழு>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            திரும்பு;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        எனில் (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            திரும்பு;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        எனில் (isBasicIO()) m_skipDtypeFunc = உண்மை;

        எனில் (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, பொய்);
    }
    வெறுமை visit(AstPackArrayDType* nodep) override {
        எனில் (!m_traVscp) திரும்பு;

        எனில் (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            திரும்பு;
        }

        Ast_முனை_DType* மாறிலி subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        எனில் (AstBasicDType* மாறிலி basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            எனில் (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                திரும்பு;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        எனில் (isBasicIO()) m_skipDtypeFunc = உண்மை;

        எனில் (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, பொய்);
    }
    வெறுமை visit(AstStructDType* nodep) override {
        எனில் (!m_traVscp) திரும்பு;

        எனில் (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            திரும்பு;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        எனில் (isBasicIO()) m_skipDtypeFunc = உண்மை;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        எனில் (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, பொய்);
    }
    வெறுமை visit(AstUnionDType* nodep) override {
        எனில் (!m_traVscp) திரும்பு;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = உண்மை;

        எனில் (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            திரும்பு;
        }

        VL_RESTORER_COPY(m_traName);
        File_வரி* மாறிலி flp = nodep->fileline();

        முழு nMembers = 0;
        க்கு (Ast_முனை* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        எனில் (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } இல்லையெனில் {
            addToSubFunc(புதிய Ast_Trace_தள்ளு_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            க்கு (மாறிலி AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_முனை_DType* மாறிலி subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(புதிய Ast_Trace_எடு_Prefix{flp});
        }
    }
    வெறுமை visit(AstBasicDType* nodep) override {
        எனில் (!m_traVscp) திரும்பு;
        எனில் (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } இல்லையெனில் {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    வெறுமை visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    வெறுமை visit(Ast_முனை_DType*) override {
        // Note more specific dtypes above
        எனில் (!m_traVscp) திரும்பு;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_உச்சி_Scopep{nodep->உச்சி_Scopep()} {
        File_வரி* மாறிலி flp = nodep->fileline();

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
        remove_Redundant_Prefix_தள்ளு_எடு();

        மாறிலி std::vector<AstCFunc*>& உச்சி_Scope_Funcps = m_scopeInitFuncps.at(m_உச்சி_Scopep->scopep());
        AstCFunc* வேர்_Funcp = nullptr;
        எனில் (!v3Global.opt.libCreate().empty()) {
            வேர்_Funcp = newCFunc(flp, "trace_init_root");
            வேர்_Funcp->entryPoint(உண்மை);
            க்கு (size_t i = 0; i < m_உச்சி_Scope_வேர்_Func_மொத்தம்; ++i) {
                AstCCall* மாறிலி callp = புதிய AstCCall{flp, உச்சி_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                வேர்_Funcp->addStmtsp(callp->makeStmt());
            }
            எனில் (!m_உச்சி_Scope_வேர்_Func_மொத்தம்) வேர்_Funcp->addStmtsp(புதிய AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        க்கு (size_t i = m_உச்சி_Scope_வேர்_Func_மொத்தம்; i < உச்சி_Scope_Funcps.size(); ++i) {
            AstCFunc* மாறிலி funcp = உச்சி_Scope_Funcps.at(i);
            AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_உச்சி_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        எனில் (m_உச்சி_Funcps.empty()) add_To_உச்சி_Func(புதிய AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        எனில் (m_உச்சி_Funcps.size() > 1) {
            AstCFunc* மாறிலி உச்சி_Funcp = newCFunc(flp, "");
            க்கு (AstCFunc* funcp : m_உச்சி_Funcps) {
                AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                உச்சி_Funcp->addStmtsp(callp->makeStmt());
            }
            m_உச்சி_Funcps.clear();
            m_உச்சி_Funcps.தள்ளு_back(உச்சி_Funcp);
        }

        // Set name of top level function
        AstCFunc* மாறிலி உச்சி_Funcp = m_உச்சி_Funcps.front();
        உச்சி_Funcp->name("trace_init_top");
        உச்சி_Funcp->entryPoint(உண்மை);

        எனில் (வேர்_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(வேர்_Funcp);
        checkCalls(உச்சி_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

வெறுமை V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("tracedecl", 0, dump_மரம்_Either_Level() >= 3);
}
