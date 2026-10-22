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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3TraceDecl.h"

#समावेश "V3Ast.h"
#समावेश "V3Control.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3Error.h"
#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3Number.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

#समावेश <cstdint>
#समावेश <functional>
#समावेश <limits>
#समावेश <tuple>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

श्रेणी PathAdjustor final {
    File_पंक्ति* स्थिर m_flp;  // FileLine used for created nodes
    std::function<व्योम(Ast_गाँठ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_ढेर{""};  // Stack of current paths

    अटल constexpr अक्षर SEPARATOR = ' ';

खुला:
    explicit PathAdjustor(File_पंक्ति* flp, std::function<व्योम(Ast_गाँठ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    व्योम adjust(स्थिर string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        जबतक (!VString::startsWith(newPath, m_ढेर.back())) {
            m_emit(नया Ast_Trace_निकालो_Prefix{m_flp});
            m_ढेर.निकालो_back();
        }

        अगर (newPath == m_ढेर.back()) वापस;

        स्थिर VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        स्थिर std::string extraPrefix = newPath.substr(m_ढेर.back().आकार());
        माप_प्रकार begin = 0;
        स्थिर माप_प्रकार last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        जबतक (true) {
            स्थिर माप_प्रकार end = extraPrefix.find(SEPARATOR, begin);
            अगर (end == string::npos) अवरोध;
            स्थिर string& extra = extraPrefix.substr(begin, end - begin);
            अगर (end == last) {
                m_emit(नया Ast_Trace_धकेलो_Prefix{m_flp, extra, lastScopeType});
            } अन्यथा {
                m_emit(नया Ast_Trace_धकेलो_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_ढेर.धकेलो_back(m_ढेर.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.आकार(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    व्योम unwind() {
        जबतक (m_ढेर.आकार() > 1) {
            m_emit(नया Ast_Trace_निकालो_Prefix{m_flp});
            m_ढेर.निकालो_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

श्रेणी TraceDeclVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_शीर्ष_Scope* स्थिर m_शीर्ष_Scopep;  // The singleton AstTopScope
    स्थिर AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_शीर्ष_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<स्थिर AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    काष्ठा Dtype_Func_कुंजी final {
        स्थिर Ast_गाँठ_DType* dtypep;
        VVarType varType;
        bool चालक==(स्थिर Dtype_Func_कुंजी& other) स्थिर {
            वापस dtypep == other.dtypep && varType == other.varType;
        }
    };
    काष्ठा Dtype_Func_कुंजी_Hash final {
        माप_प्रकार चालक()(स्थिर Dtype_Func_कुंजी& कुंजी) स्थिर {
            वापस std::hash<स्थिर Ast_गाँठ_DType*>{}(कुंजी.dtypep)
                   ^ (std::hash<uint8_t>{}(कुंजी.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_कुंजी, AstCFunc*, Dtype_Func_कुंजी_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    स्थिर string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    bool m_skipDtypeFunc = false;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    पूर्णांक m_शीर्ष_Func_आकार = 0;  // Size of the top function currently being built
    पूर्णांक m_sub_Func_आकार = 0;  // Size of the sub function currently being built
    bool m_forceNewLeaf = false;  // Force next addToSubFunc to start a fresh leaf
    माप_प्रकार m_शीर्ष_Scope_मूल_Func_गिनती = 0;  // Top-scope init functions used only for wrapper IOs
    bool m_शीर्ष_Scope_मूल_Phase = false;  // Emitting top-scope wrapper IO declarations
    स्थिर पूर्णांक m_func_आकार_Limit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<पूर्णांक>::max();
    // Trace init functions to for each scope
    std::unordered_map<स्थिर AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, स्थिर AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_गाँठ_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_गाँठ_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    श्रेणी TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        व्योम init(स्थिर std::string& name, Ast_गाँठ* nodep, bool in_शीर्ष_Scope) {
            // Compute path in hierarchy and item name
            स्थिर std::string& vcdName = Ast_गाँठ::vcdName(name);
            AstVar* स्थिर varp = VN_CAST(nodep, Var);
            अगर (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.सफा();
            } अन्यथा अगर (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } अन्यथा {
                // Other Variable
                स्थिर माप_प्रकार pos = vcdName.rfind(' ');
                स्थिर माप_प्रकार pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            अगर (in_शीर्ष_Scope && !v3Global.opt.libCreate().empty()) {
                स्थिर माप_प्रकार start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                स्थिर std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                अगर (prefix == "$rootio") वापस;
                m_path = m_path.substr(start + 1);
                अगर (v3Global.rootp()->trace_Lib_शीर्ष_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    खुला:
        explicit TraceEntry(स्थिर AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_शीर्ष());
        }
        explicit TraceEntry(स्थिर AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_शीर्ष());
        }
        पूर्णांक operatorCompare(स्थिर TraceEntry& b) स्थिर {
            अगर (स्थिर पूर्णांक cmp = path().compare(b.path())) वापस cmp < 0;
            अगर (स्थिर पूर्णांक cmp = fileline().operatorCompare(b.fileline())) वापस cmp < 0;
            वापस name() < b.name();
        }
        AstVarScope* vscp() स्थिर { वापस m_vscp; }
        AstCell* cellp() स्थिर { वापस m_cellp; }
        स्थिर std::string& path() स्थिर { वापस m_path; }
        व्योम path(स्थिर std::string& path) { m_path = path; }
        स्थिर std::string& name() स्थिर { वापस m_name; }
        File_पंक्ति& fileline() स्थिर { वापस m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_गाँठ_व्यंजक* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(स्थिर AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        स्थिर AstVar* स्थिर varp = nodep->varp();
        अगर (!varp->isTrace()) वापस "Verilator trace_off";
        अगर (!nodep->isTrace()) वापस "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        अगर (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            वापस "Automatic variable";

        स्थिर पूर्णांक width = recurseDTypeWidth(nodep->varp()->dtypep());
        अगर (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            वापस "Width " + cvtToStr(width) + " > --trace-max-width";

        स्थिर string prettyName = nodep->prettyName();
        अगर (!V3Control::getScopeTraceOn(prettyName)) वापस "Vlt scope trace_off";
        अगर (!v3Global.opt.traceUnderscore()) {
            अगर (!prettyName.empty() && prettyName[0] == '_') वापस "Leading underscore";
            अगर (prettyName.find("._") != string::npos) वापस "Inlined leading underscore";
        }
        वापस ""s;
    }

    पूर्णांक recurseDTypeWidth(स्थिर Ast_गाँठ_DType* nodep) {
        अगर (स्थिर Ast_गाँठ_Array_DType* adtypep = VN_CAST(nodep, गाँठ_Array_DType))
            वापस recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        वापस nodep->width();
    }

    AstCFunc* newCFunc(File_पंक्ति* flp, स्थिर string& name) {
        AstScope* स्थिर शीर्ष_Scopep = m_शीर्ष_Scopep->scopep();
        AstCFunc* स्थिर funcp = नया AstCFunc{flp, name, शीर्ष_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(true);
        funcp->isStatic(false);
        funcp->isLoose(true);
        funcp->slow(true);
        शीर्ष_Scopep->addBlocksp(funcp);
        वापस funcp;
    }

    व्योम add_To_शीर्ष_Func(Ast_गाँठ_Stmt* stmtp) {
        अगर (m_शीर्ष_Func_आकार > m_func_आकार_Limit || m_शीर्ष_Funcps.empty()) {
            m_शीर्ष_Func_आकार = 0;
            //
            स्थिर string n = cvtToStr(m_शीर्ष_Funcps.आकार());
            स्थिर string name{"trace_init_top__" + n};
            AstCFunc* स्थिर funcp = newCFunc(m_शीर्ष_Scopep->fileline(), name);
            m_शीर्ष_Funcps.धकेलो_back(funcp);
        }
        m_शीर्ष_Funcps.back()->addStmtsp(stmtp);
        m_शीर्ष_Func_आकार += stmtp->गाँठ_गिनती();
    }

    व्योम addToSubFunc(Ast_गाँठ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        अगर (m_dtypeSubFuncp) {
            अगर (m_sub_Func_आकार > m_func_आकार_Limit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_sub_Func_आकार += stmtp->गाँठ_गिनती();
            वापस;
        }
        // Defer trace splitting until V3Trace
        अगर (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = false;
            File_पंक्ति* स्थिर flp = m_currScopep->fileline();
            स्थिर string n = cvtToStr(m_subFuncps.आकार());
            स्थिर string name
                = m_currScopep == m_शीर्ष_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_शीर्ष_Scope_मूल_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* स्थिर funcp = newCFunc(flp, name);
            funcp->addStmtsp(नया AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.धकेलो_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_sub_Func_आकार += stmtp->गाँठ_गिनती();
    }

    AstTraceDecl* addTraceDecl(स्थिर VNumRange& arrayRange,
                               पूर्णांक widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* स्थिर dtypeCallp = nullptr) {
        VNumRange bitRange;
        अगर (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } अन्यथा अगर (स्थिर AstBasicDType* स्थिर bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_पंक्ति* स्थिर flp = m_traVscp->fileline();
        Ast_गाँठ_व्यंजक* valuep = m_traValuep->clone_वृक्ष(false);
        स्थिर bool validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* स्थिर newp
            = नया AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        अगर (validOffset) {
            newp->code(m_offset);
            अगर (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* स्थिर refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(नया AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
            });
        } अन्यथा {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        वापस newp;
    }

    व्योम addIgnore(स्थिर string& why) {
        ++m_statIgnSigs;
        स्थिर std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        अगर (debug() > 3 && m_traVscp) std::अ_बाहर << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    व्योम fixupPlaceholder(स्थिर std::string& path, Ast_गाँठ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        स्थिर स्वतः it = m_pathToScopep.find(Ast_गाँठ::prettyName(path));
        अगर (it != m_pathToScopep.end()) {
            स्थिर AstScope* स्थिर scopep = it->second;
            File_पंक्ति* स्थिर flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_गाँठ* stmtp = nullptr;
            क्रम (AstCFunc* स्थिर subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* स्थिर callp = नया AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_गाँठ::add_अग्र(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            अगर (stmtp) placeholderp->add_अग्र_Here(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_वृक्ष(), placeholderp);
    }

    व्योम fixupLibStub(स्थिर std::string& path, Ast_गाँठ_Stmt* placeholderp) {
        File_पंक्ति* स्थिर flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_गाँठ::prettyName(path);
        अगर (!v3Global.opt.libCreate().empty()) {
            स्थिर std::string शीर्ष_Prefix = v3Global.rootp()->trace_Lib_शीर्ष_Name() + ".";
            अगर (VString::startsWith(name, शीर्ष_Prefix)) name = name.substr(शीर्ष_Prefix.आकार());
        }

        // Call the initialization function for the library instance
        AstCStmt* स्थिर initp = नया AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(नया AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->add_अग्र_Here(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_वृक्ष(), placeholderp);
    }

    व्योम fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        क्रम (स्थिर स्वतः& item : m_cellInitPlaceholders) {
            स्थिर AstScope* स्थिर parentp = std::लो<0>(item);
            स्थिर AstCell* स्थिर cellp = std::लो<1>(item);
            Ast_गाँठ_Stmt* स्थिर placeholderp = std::लो<2>(item);
            स्थिर std::string path = parentp->name() + "__DOT__" + cellp->name();
            अगर (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } अन्यथा {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        क्रम (स्थिर स्वतः& item : m_ifaceRefInitPlaceholders) {
            स्थिर AstVarScope* स्थिर vscp = std::लो<0>(item);
            Ast_गाँठ_Stmt* स्थिर placeholderp = std::लो<1>(item);
            स्थिर std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    व्योम remove_Redundant_Prefix_धकेलो_निकालो() {
        क्रम (स्थिर स्वतः& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            क्रम (AstCFunc* स्थिर funcp : pair.second) {
                Ast_गाँठ* prevp = nullptr;
                Ast_गाँठ* currp = funcp->stmtsp();
                जबतक (currp) {
                    Ast_गाँठ* स्थिर nextp = currp->nextp();
                    अगर (VN_IS(prevp, Trace_धकेलो_Prefix) && VN_IS(currp, Trace_निकालो_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_वृक्ष(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_वृक्ष(), currp);
                    }
                    अगर (!nextp) अवरोध;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    व्योम checkCalls(स्थिर AstCFunc* funcp) {
        अगर (!v3Global.opt.debugCheck()) वापस;
        checkCallsRecurse(funcp);
        अगर (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            क्रम (स्वतः tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    व्योम checkCallsRecurse(स्थिर AstCFunc* funcp) {
        funcp->foreach([यह](स्थिर Ast_गाँठ* nodep) {
            अगर (स्थिर AstTraceDecl* स्थिर declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } अन्यथा अगर (स्थिर AstCCall* स्थिर ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    bool isBasicIO() { वापस m_traVscp->varp()->isVLIO(); }
    व्योम newDeclSubFunc() {
        File_पंक्ति* स्थिर flp = m_dtypeFuncp->fileline();
        AstCFunc* स्थिर subFuncp = newCFunc(flp, m_dtypeNames.लो("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* स्थिर subCallp = नया AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_आकार = 0;
    }
    व्योम newDeclFunc(Ast_गाँठ_DType* nodep) {
        Ast_गाँठ_DType* स्थिर skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_पंक्ति* स्थिर flp = skipTypep->fileline();

        स्थिर Dtype_Func_कुंजी dtype_कुंजी{skipTypep, m_traVscp->varp()->varType()};
        स्वतः pair = m_dtypeFuncs.emplace(dtype_कुंजी, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        अगर (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.लो("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* स्थिर callp = नया AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        अगर (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_आकार);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            अगर (AstStructDType* स्थिर dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, true);
            } अन्यथा अगर (AstUnpackArrayDType* स्थिर dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, true);
            } अन्यथा अगर (AstPackArrayDType* स्थिर dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, true);
            } अन्यथा {
                UASSERT_OBJ(false, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    व्योम declUnpackedArray(AstUnpackArrayDType* स्थिर nodep, bool newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_पंक्ति* स्थिर flp = nodep->fileline();

        addToSubFunc(नया Ast_Trace_धकेलो_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->बाँए(), nodep->दाँए(), !newFunc});

        अगर (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            अगर (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } अन्यथा {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } अन्यथा {
            Ast_गाँठ_DType* स्थिर subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            क्रम (स्थिर पूर्णांक i : nodep->declRange().seq_बायाँ_To_दायाँ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_वृक्ष(false);
                m_traValuep = नया AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
            }
        }

        addToSubFunc(नया Ast_Trace_निकालो_Prefix{flp});
    }
    व्योम declPackedArray(AstPackArrayDType* स्थिर nodep, bool newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_गाँठ_DType* स्थिर subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_पंक्ति* स्थिर flp = nodep->fileline();

        addToSubFunc(नया Ast_Trace_धकेलो_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->बाँए(), nodep->दाँए(), !newFunc});

        // Always iterate left index to right index
        क्रम (स्थिर पूर्णांक i : nodep->declRange().seq_बायाँ_To_दायाँ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            स्थिर पूर्णांक lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_वृक्ष(false);
            m_traValuep = नया AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_वृक्ष(), m_traValuep = nullptr);
        }

        addToSubFunc(नया Ast_Trace_निकालो_Prefix{flp});
    }
    व्योम declStruct(AstStructDType* स्थिर nodep, bool newFunc) {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        पूर्णांक nMembers = 0;
        क्रम (Ast_गाँठ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        अगर (!nodep->packed()) {
            addToSubFunc(नया Ast_Trace_धकेलो_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            क्रम (स्थिर AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाँठ_DType* स्थिर subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_वृक्ष(false);
                m_traValuep = नया AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
            }
            addToSubFunc(नया Ast_Trace_निकालो_Prefix{flp});
        } अन्यथा {
            addToSubFunc(नया Ast_Trace_धकेलो_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            क्रम (स्थिर AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाँठ_DType* स्थिर subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_वृक्ष(false);
                m_traValuep = नया AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
            }
            addToSubFunc(नया Ast_Trace_निकालो_Prefix{flp});
        }
    }

    // VISITORS
    व्योम visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        अगर (nodep->modp()->verilatorLib()) वापस;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        क्रम (Ast_गाँठ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstCell* स्थिर cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        अगर (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](स्थिर TraceEntry& a, स्थिर TraceEntry& b) { वापस a.operatorCompare(b); });

            File_पंक्ति* स्थिर flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_गाँठ_Stmt* stmtp) { addToSubFunc(stmtp); }};
            स्थिर bool split_मूल_Primary_Ios = nodep->is_शीर्ष() && !v3Global.opt.libCreate().empty();
            स्थिर स्वतः emitEntry = [&](स्थिर TraceEntry& entry) {
                AstVarScope* स्थिर vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_गाँठ*>(entry.cellp())
                                                    : static_cast<Ast_गाँठ*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                अगर (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    स्थिर string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    अगर (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } अन्यथा {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = नया AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        अगर (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } अन्यथा {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* स्थिर cellp = entry.cellp();
                    Ast_गाँठ_Stmt* स्थिर stmtp = नया AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            अगर (split_मूल_Primary_Ios) {
                m_शीर्ष_Scope_मूल_Phase = true;
                क्रम (स्थिर TraceEntry& entry : m_entries) {
                    AstVarScope* स्थिर vscp = entry.vscp();
                    अगर (!(vscp && vscp->varp()->isPrimaryIO())) जारी;
                    emitEntry(entry);
                }
                m_शीर्ष_Scope_मूल_Phase = false;
                pathAdjustor.unwind();
                m_शीर्ष_Scope_मूल_Func_गिनती = m_subFuncps.आकार();
                अगर (m_शीर्ष_Scope_मूल_Func_गिनती) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_आकार = 0;
                    m_forceNewLeaf = true;
                }
            }
            क्रम (स्थिर TraceEntry& entry : m_entries) {
                AstVarScope* स्थिर vscp = entry.vscp();
                अगर (split_मूल_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) जारी;
                emitEntry(entry);
            }
            pathAdjustor.unwind();
            m_traVscp = nullptr;
            m_traName.सफा();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.सफा();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        स्थिर std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        स्थिर AstCell* स्थिर cellp = nodep->aboveCellp();
        अगर (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            स्थिर माप_प्रकार lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            स्थिर std::string parentPath = path.substr(0, lastDot + 1);

            क्रम (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                स्थिर std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                अगर (!VString::startsWith(refName, parentPath)) जारी;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                अगर (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_वृक्ष(), intfRefp);
                }
            }
        }
    }
    व्योम visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        अगर (nodep->varp()->is_अस्थायी() && !nodep->varp()->isTrace()) वापस;
        अगर (nodep->varp()->isClassMember()) वापस;
        अगर (nodep->varp()->isFuncLocal()) वापस;

        // When creating a --lib-create library ...
        अगर (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            अगर (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) वापस;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    व्योम visit(AstConstDType* nodep) override {
        अगर (!m_traVscp) वापस;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = true;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    व्योम visit(AstRefDType* nodep) override {
        अगर (!m_traVscp) वापस;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    व्योम visit(AstIfaceRefDType* /*nodep*/) override {
        अगर (!m_traVscp) वापस;
        // Insert a placeholder to be fixed up later
        File_पंक्ति* स्थिर flp = m_traVscp->fileline();
        Ast_गाँठ_Stmt* स्थिर stmtp
            = नया AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    व्योम visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        अगर (!m_traVscp) वापस;

        अगर (v3Global.opt.traceMaxArray()
            && static_cast<पूर्णांक>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            वापस;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        अगर (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            वापस;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        अगर (isBasicIO()) m_skipDtypeFunc = true;

        अगर (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, false);
    }
    व्योम visit(AstPackArrayDType* nodep) override {
        अगर (!m_traVscp) वापस;

        अगर (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            वापस;
        }

        Ast_गाँठ_DType* स्थिर subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        अगर (AstBasicDType* स्थिर basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            अगर (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                वापस;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        अगर (isBasicIO()) m_skipDtypeFunc = true;

        अगर (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, false);
    }
    व्योम visit(AstStructDType* nodep) override {
        अगर (!m_traVscp) वापस;

        अगर (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            वापस;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        अगर (isBasicIO()) m_skipDtypeFunc = true;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        अगर (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, false);
    }
    व्योम visit(AstUnionDType* nodep) override {
        अगर (!m_traVscp) वापस;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = true;

        अगर (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            वापस;
        }

        VL_RESTORER_COPY(m_traName);
        File_पंक्ति* स्थिर flp = nodep->fileline();

        पूर्णांक nMembers = 0;
        क्रम (Ast_गाँठ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        अगर (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } अन्यथा {
            addToSubFunc(नया Ast_Trace_धकेलो_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            क्रम (स्थिर AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाँठ_DType* स्थिर subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(नया Ast_Trace_निकालो_Prefix{flp});
        }
    }
    व्योम visit(AstBasicDType* nodep) override {
        अगर (!m_traVscp) वापस;
        अगर (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } अन्यथा {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    व्योम visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    व्योम visit(Ast_गाँठ_DType*) override {
        // Note more specific dtypes above
        अगर (!m_traVscp) वापस;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_शीर्ष_Scopep{nodep->शीर्ष_Scopep()} {
        File_पंक्ति* स्थिर flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_अग्र_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_Prefix_धकेलो_निकालो();

        स्थिर std::vector<AstCFunc*>& शीर्ष_Scope_Funcps = m_scopeInitFuncps.at(m_शीर्ष_Scopep->scopep());
        AstCFunc* मूल_Funcp = nullptr;
        अगर (!v3Global.opt.libCreate().empty()) {
            मूल_Funcp = newCFunc(flp, "trace_init_root");
            मूल_Funcp->entryPoint(true);
            क्रम (माप_प्रकार i = 0; i < m_शीर्ष_Scope_मूल_Func_गिनती; ++i) {
                AstCCall* स्थिर callp = नया AstCCall{flp, शीर्ष_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                मूल_Funcp->addStmtsp(callp->makeStmt());
            }
            अगर (!m_शीर्ष_Scope_मूल_Func_गिनती) मूल_Funcp->addStmtsp(नया AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        क्रम (माप_प्रकार i = m_शीर्ष_Scope_मूल_Func_गिनती; i < शीर्ष_Scope_Funcps.आकार(); ++i) {
            AstCFunc* स्थिर funcp = शीर्ष_Scope_Funcps.at(i);
            AstCCall* स्थिर callp = नया AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_शीर्ष_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        अगर (m_शीर्ष_Funcps.empty()) add_To_शीर्ष_Func(नया AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        अगर (m_शीर्ष_Funcps.आकार() > 1) {
            AstCFunc* स्थिर शीर्ष_Funcp = newCFunc(flp, "");
            क्रम (AstCFunc* funcp : m_शीर्ष_Funcps) {
                AstCCall* स्थिर callp = नया AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                शीर्ष_Funcp->addStmtsp(callp->makeStmt());
            }
            m_शीर्ष_Funcps.सफा();
            m_शीर्ष_Funcps.धकेलो_back(शीर्ष_Funcp);
        }

        // Set name of top level function
        AstCFunc* स्थिर शीर्ष_Funcp = m_शीर्ष_Funcps.front();
        शीर्ष_Funcp->name("trace_init_top");
        शीर्ष_Funcp->entryPoint(true);

        अगर (मूल_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(मूल_Funcp);
        checkCalls(शीर्ष_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

व्योम V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("tracedecl", 0, dump_वृक्ष_Either_Level() >= 3);
}
