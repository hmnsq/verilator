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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3TraceDecl.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Number.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <cstdint>
#अन्तर्भाव <functional>
#अन्तर्भाव <limits>
#अन्तर्भाव <tuple>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

वर्ग PathAdjustor final {
    File_ओळ* स्थिरांक m_flp;  // FileLine used for created nodes
    std::function<शून्य(Ast_गाठ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_चळत{""};  // Stack of current paths

    स्थिर constexpr अक्षर SEPARATOR = ' ';

सार्वजनिक:
    explicit PathAdjustor(File_ओळ* flp, std::function<शून्य(Ast_गाठ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    शून्य adjust(स्थिरांक string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        पर्यंत (!VString::startsWith(newPath, m_चळत.back())) {
            m_emit(नवीन Ast_Trace_काढा_Prefix{m_flp});
            m_चळत.काढा_back();
        }

        जर (newPath == m_चळत.back()) परत;

        स्थिरांक VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        स्थिरांक std::string extraPrefix = newPath.substr(m_चळत.back().size());
        माप_प्रकार begin = 0;
        स्थिरांक माप_प्रकार last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        पर्यंत (सत्य) {
            स्थिरांक माप_प्रकार end = extraPrefix.find(SEPARATOR, begin);
            जर (end == string::npos) थांबा;
            स्थिरांक string& extra = extraPrefix.substr(begin, end - begin);
            जर (end == last) {
                m_emit(नवीन Ast_Trace_ढकला_Prefix{m_flp, extra, lastScopeType});
            } अन्यथा {
                m_emit(नवीन Ast_Trace_ढकला_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_चळत.ढकला_back(m_चळत.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    शून्य unwind() {
        पर्यंत (m_चळत.size() > 1) {
            m_emit(नवीन Ast_Trace_काढा_Prefix{m_flp});
            m_चळत.काढा_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

वर्ग TraceDeclVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_शीर्ष_Scope* स्थिरांक m_शीर्ष_Scopep;  // The singleton AstTopScope
    स्थिरांक AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_शीर्ष_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<स्थिरांक AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    संरचना Dtype_Func_किल्ली final {
        स्थिरांक Ast_गाठ_DType* dtypep;
        VVarType varType;
        तार्किक चालक==(स्थिरांक Dtype_Func_किल्ली& other) स्थिरांक {
            परत dtypep == other.dtypep && varType == other.varType;
        }
    };
    संरचना Dtype_Func_किल्ली_Hash final {
        माप_प्रकार चालक()(स्थिरांक Dtype_Func_किल्ली& किल्ली) स्थिरांक {
            परत std::hash<स्थिरांक Ast_गाठ_DType*>{}(किल्ली.dtypep)
                   ^ (std::hash<uint8_t>{}(किल्ली.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_किल्ली, AstCFunc*, Dtype_Func_किल्ली_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    स्थिरांक string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    तार्किक m_skipDtypeFunc = असत्य;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    पूर्णांक m_शीर्ष_Func_Size = 0;  // Size of the top function currently being built
    पूर्णांक m_subFuncSize = 0;  // Size of the sub function currently being built
    तार्किक m_forceNewLeaf = असत्य;  // Force next addToSubFunc to start a fresh leaf
    माप_प्रकार m_शीर्ष_Scope_मूळ_Func_मोजणी = 0;  // Top-scope init functions used only for wrapper IOs
    तार्किक m_शीर्ष_Scope_मूळ_Phase = असत्य;  // Emitting top-scope wrapper IO declarations
    स्थिरांक पूर्णांक m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<पूर्णांक>::max();
    // Trace init functions to for each scope
    std::unordered_map<स्थिरांक AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, स्थिरांक AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_गाठ_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_गाठ_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    वर्ग TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        शून्य init(स्थिरांक std::string& name, Ast_गाठ* nodep, तार्किक in_शीर्ष_Scope) {
            // Compute path in hierarchy and item name
            स्थिरांक std::string& vcdName = Ast_गाठ::vcdName(name);
            AstVar* स्थिरांक varp = VN_CAST(nodep, Var);
            जर (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } अन्यथा जर (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } अन्यथा {
                // Other Variable
                स्थिरांक माप_प्रकार pos = vcdName.rfind(' ');
                स्थिरांक माप_प्रकार pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            जर (in_शीर्ष_Scope && !v3Global.opt.libCreate().empty()) {
                स्थिरांक माप_प्रकार start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                स्थिरांक std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                जर (prefix == "$rootio") परत;
                m_path = m_path.substr(start + 1);
                जर (v3Global.rootp()->trace_Lib_शीर्ष_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    सार्वजनिक:
        explicit TraceEntry(स्थिरांक AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_शीर्ष());
        }
        explicit TraceEntry(स्थिरांक AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_शीर्ष());
        }
        पूर्णांक operatorCompare(स्थिरांक TraceEntry& b) स्थिरांक {
            जर (स्थिरांक पूर्णांक cmp = path().compare(b.path())) परत cmp < 0;
            जर (स्थिरांक पूर्णांक cmp = fileline().operatorCompare(b.fileline())) परत cmp < 0;
            परत name() < b.name();
        }
        AstVarScope* vscp() स्थिरांक { परत m_vscp; }
        AstCell* cellp() स्थिरांक { परत m_cellp; }
        स्थिरांक std::string& path() स्थिरांक { परत m_path; }
        शून्य path(स्थिरांक std::string& path) { m_path = path; }
        स्थिरांक std::string& name() स्थिरांक { परत m_name; }
        File_ओळ& fileline() स्थिरांक { परत m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_गाठ_अभिव्यक्ती* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(स्थिरांक AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        स्थिरांक AstVar* स्थिरांक varp = nodep->varp();
        जर (!varp->isTrace()) परत "Verilator trace_off";
        जर (!nodep->isTrace()) परत "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        जर (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            परत "Automatic variable";

        स्थिरांक पूर्णांक width = recurseDTypeWidth(nodep->varp()->dtypep());
        जर (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            परत "Width " + cvtToStr(width) + " > --trace-max-width";

        स्थिरांक string prettyName = nodep->prettyName();
        जर (!V3Control::getScopeTraceOn(prettyName)) परत "Vlt scope trace_off";
        जर (!v3Global.opt.traceUnderscore()) {
            जर (!prettyName.empty() && prettyName[0] == '_') परत "Leading underscore";
            जर (prettyName.find("._") != string::npos) परत "Inlined leading underscore";
        }
        परत ""s;
    }

    पूर्णांक recurseDTypeWidth(स्थिरांक Ast_गाठ_DType* nodep) {
        जर (स्थिरांक Ast_गाठ_Array_DType* adtypep = VN_CAST(nodep, गाठ_Array_DType))
            परत recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        परत nodep->width();
    }

    AstCFunc* newCFunc(File_ओळ* flp, स्थिरांक string& name) {
        AstScope* स्थिरांक शीर्ष_Scopep = m_शीर्ष_Scopep->scopep();
        AstCFunc* स्थिरांक funcp = नवीन AstCFunc{flp, name, शीर्ष_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(सत्य);
        funcp->isStatic(असत्य);
        funcp->isLoose(सत्य);
        funcp->slow(सत्य);
        शीर्ष_Scopep->addBlocksp(funcp);
        परत funcp;
    }

    शून्य add_To_शीर्ष_Func(Ast_गाठ_Stmt* stmtp) {
        जर (m_शीर्ष_Func_Size > m_funcSizeLimit || m_शीर्ष_Funcps.empty()) {
            m_शीर्ष_Func_Size = 0;
            //
            स्थिरांक string n = cvtToStr(m_शीर्ष_Funcps.size());
            स्थिरांक string name{"trace_init_top__" + n};
            AstCFunc* स्थिरांक funcp = newCFunc(m_शीर्ष_Scopep->fileline(), name);
            m_शीर्ष_Funcps.ढकला_back(funcp);
        }
        m_शीर्ष_Funcps.back()->addStmtsp(stmtp);
        m_शीर्ष_Func_Size += stmtp->गाठ_मोजणी();
    }

    शून्य addToSubFunc(Ast_गाठ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        जर (m_dtypeSubFuncp) {
            जर (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->गाठ_मोजणी();
            परत;
        }
        // Defer trace splitting until V3Trace
        जर (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = असत्य;
            File_ओळ* स्थिरांक flp = m_currScopep->fileline();
            स्थिरांक string n = cvtToStr(m_subFuncps.size());
            स्थिरांक string name
                = m_currScopep == m_शीर्ष_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_शीर्ष_Scope_मूळ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* स्थिरांक funcp = newCFunc(flp, name);
            funcp->addStmtsp(नवीन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ढकला_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->गाठ_मोजणी();
    }

    AstTraceDecl* addTraceDecl(स्थिरांक VNumRange& arrayRange,
                               पूर्णांक widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* स्थिरांक dtypeCallp = nullptr) {
        VNumRange bitRange;
        जर (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } अन्यथा जर (स्थिरांक AstBasicDType* स्थिरांक bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_ओळ* स्थिरांक flp = m_traVscp->fileline();
        Ast_गाठ_अभिव्यक्ती* valuep = m_traValuep->clone_वृक्ष(असत्य);
        स्थिरांक तार्किक validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* स्थिरांक newp
            = नवीन AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        जर (validOffset) {
            newp->code(m_offset);
            जर (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* स्थिरांक refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(नवीन AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
            });
        } अन्यथा {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        परत newp;
    }

    शून्य addIgnore(स्थिरांक string& why) {
        ++m_statIgnSigs;
        स्थिरांक std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        जर (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    शून्य fixupPlaceholder(स्थिरांक std::string& path, Ast_गाठ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        स्थिरांक स्वतः it = m_pathToScopep.find(Ast_गाठ::prettyName(path));
        जर (it != m_pathToScopep.end()) {
            स्थिरांक AstScope* स्थिरांक scopep = it->second;
            File_ओळ* स्थिरांक flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_गाठ* stmtp = nullptr;
            साठी (AstCFunc* स्थिरांक subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* स्थिरांक callp = नवीन AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_गाठ::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            जर (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_वृक्ष(), placeholderp);
    }

    शून्य fixupLibStub(स्थिरांक std::string& path, Ast_गाठ_Stmt* placeholderp) {
        File_ओळ* स्थिरांक flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_गाठ::prettyName(path);
        जर (!v3Global.opt.libCreate().empty()) {
            स्थिरांक std::string शीर्ष_Prefix = v3Global.rootp()->trace_Lib_शीर्ष_Name() + ".";
            जर (VString::startsWith(name, शीर्ष_Prefix)) name = name.substr(शीर्ष_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* स्थिरांक initp = नवीन AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(नवीन AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_वृक्ष(), placeholderp);
    }

    शून्य fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        साठी (स्थिरांक स्वतः& item : m_cellInitPlaceholders) {
            स्थिरांक AstScope* स्थिरांक parentp = std::get<0>(item);
            स्थिरांक AstCell* स्थिरांक cellp = std::get<1>(item);
            Ast_गाठ_Stmt* स्थिरांक placeholderp = std::get<2>(item);
            स्थिरांक std::string path = parentp->name() + "__DOT__" + cellp->name();
            जर (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } अन्यथा {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        साठी (स्थिरांक स्वतः& item : m_ifaceRefInitPlaceholders) {
            स्थिरांक AstVarScope* स्थिरांक vscp = std::get<0>(item);
            Ast_गाठ_Stmt* स्थिरांक placeholderp = std::get<1>(item);
            स्थिरांक std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    शून्य remove_Redundant_Prefix_ढकला_काढा() {
        साठी (स्थिरांक स्वतः& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            साठी (AstCFunc* स्थिरांक funcp : pair.second) {
                Ast_गाठ* prevp = nullptr;
                Ast_गाठ* currp = funcp->stmtsp();
                पर्यंत (currp) {
                    Ast_गाठ* स्थिरांक nextp = currp->nextp();
                    जर (VN_IS(prevp, Trace_ढकला_Prefix) && VN_IS(currp, Trace_काढा_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_वृक्ष(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_वृक्ष(), currp);
                    }
                    जर (!nextp) थांबा;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    शून्य checkCalls(स्थिरांक AstCFunc* funcp) {
        जर (!v3Global.opt.debugCheck()) परत;
        checkCallsRecurse(funcp);
        जर (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            साठी (स्वतः tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    शून्य checkCallsRecurse(स्थिरांक AstCFunc* funcp) {
        funcp->foreach([हे](स्थिरांक Ast_गाठ* nodep) {
            जर (स्थिरांक AstTraceDecl* स्थिरांक declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } अन्यथा जर (स्थिरांक AstCCall* स्थिरांक ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    तार्किक isBasicIO() { परत m_traVscp->varp()->isVLIO(); }
    शून्य newDeclSubFunc() {
        File_ओळ* स्थिरांक flp = m_dtypeFuncp->fileline();
        AstCFunc* स्थिरांक subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* स्थिरांक subCallp = नवीन AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    शून्य newDeclFunc(Ast_गाठ_DType* nodep) {
        Ast_गाठ_DType* स्थिरांक skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_ओळ* स्थिरांक flp = skipTypep->fileline();

        स्थिरांक Dtype_Func_किल्ली dtype_किल्ली{skipTypep, m_traVscp->varp()->varType()};
        स्वतः pair = m_dtypeFuncs.emplace(dtype_किल्ली, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        जर (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* स्थिरांक callp = नवीन AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        जर (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            जर (AstStructDType* स्थिरांक dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, सत्य);
            } अन्यथा जर (AstUnpackArrayDType* स्थिरांक dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, सत्य);
            } अन्यथा जर (AstPackArrayDType* स्थिरांक dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, सत्य);
            } अन्यथा {
                UASSERT_OBJ(असत्य, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    शून्य declUnpackedArray(AstUnpackArrayDType* स्थिरांक nodep, तार्किक newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_ओळ* स्थिरांक flp = nodep->fileline();

        addToSubFunc(नवीन Ast_Trace_ढकला_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->डावा(), nodep->उजवा(), !newFunc});

        जर (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            जर (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } अन्यथा {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } अन्यथा {
            Ast_गाठ_DType* स्थिरांक subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            साठी (स्थिरांक पूर्णांक i : nodep->declRange().seq_डावा_To_उजवा()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_वृक्ष(असत्य);
                m_traValuep = नवीन AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
            }
        }

        addToSubFunc(नवीन Ast_Trace_काढा_Prefix{flp});
    }
    शून्य declPackedArray(AstPackArrayDType* स्थिरांक nodep, तार्किक newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_गाठ_DType* स्थिरांक subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_ओळ* स्थिरांक flp = nodep->fileline();

        addToSubFunc(नवीन Ast_Trace_ढकला_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->डावा(), nodep->उजवा(), !newFunc});

        // Always iterate left index to right index
        साठी (स्थिरांक पूर्णांक i : nodep->declRange().seq_डावा_To_उजवा()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            स्थिरांक पूर्णांक lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_वृक्ष(असत्य);
            m_traValuep = नवीन AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_वृक्ष(), m_traValuep = nullptr);
        }

        addToSubFunc(नवीन Ast_Trace_काढा_Prefix{flp});
    }
    शून्य declStruct(AstStructDType* स्थिरांक nodep, तार्किक newFunc) {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        पूर्णांक nMembers = 0;
        साठी (Ast_गाठ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        जर (!nodep->packed()) {
            addToSubFunc(नवीन Ast_Trace_ढकला_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            साठी (स्थिरांक AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाठ_DType* स्थिरांक subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_वृक्ष(असत्य);
                m_traValuep = नवीन AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
            }
            addToSubFunc(नवीन Ast_Trace_काढा_Prefix{flp});
        } अन्यथा {
            addToSubFunc(नवीन Ast_Trace_ढकला_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            साठी (स्थिरांक AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाठ_DType* स्थिरांक subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_वृक्ष(असत्य);
                m_traValuep = नवीन AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
            }
            addToSubFunc(नवीन Ast_Trace_काढा_Prefix{flp});
        }
    }

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        जर (nodep->modp()->verilatorLib()) परत;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        साठी (Ast_गाठ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstCell* स्थिरांक cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        जर (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](स्थिरांक TraceEntry& a, स्थिरांक TraceEntry& b) { परत a.operatorCompare(b); });

            File_ओळ* स्थिरांक flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_गाठ_Stmt* stmtp) { addToSubFunc(stmtp); }};
            स्थिरांक तार्किक split_मूळ_Primary_Ios = nodep->is_शीर्ष() && !v3Global.opt.libCreate().empty();
            स्थिरांक स्वतः emitEntry = [&](स्थिरांक TraceEntry& entry) {
                AstVarScope* स्थिरांक vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_गाठ*>(entry.cellp())
                                                    : static_cast<Ast_गाठ*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                जर (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    स्थिरांक string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    जर (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } अन्यथा {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = नवीन AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        जर (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_वृक्ष(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } अन्यथा {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* स्थिरांक cellp = entry.cellp();
                    Ast_गाठ_Stmt* स्थिरांक stmtp = नवीन AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            जर (split_मूळ_Primary_Ios) {
                m_शीर्ष_Scope_मूळ_Phase = सत्य;
                साठी (स्थिरांक TraceEntry& entry : m_entries) {
                    AstVarScope* स्थिरांक vscp = entry.vscp();
                    जर (!(vscp && vscp->varp()->isPrimaryIO())) सुरूठेवा;
                    emitEntry(entry);
                }
                m_शीर्ष_Scope_मूळ_Phase = असत्य;
                pathAdjustor.unwind();
                m_शीर्ष_Scope_मूळ_Func_मोजणी = m_subFuncps.size();
                जर (m_शीर्ष_Scope_मूळ_Func_मोजणी) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = सत्य;
                }
            }
            साठी (स्थिरांक TraceEntry& entry : m_entries) {
                AstVarScope* स्थिरांक vscp = entry.vscp();
                जर (split_मूळ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) सुरूठेवा;
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
        स्थिरांक std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        स्थिरांक AstCell* स्थिरांक cellp = nodep->aboveCellp();
        जर (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            स्थिरांक माप_प्रकार lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            स्थिरांक std::string parentPath = path.substr(0, lastDot + 1);

            साठी (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                स्थिरांक std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                जर (!VString::startsWith(refName, parentPath)) सुरूठेवा;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                जर (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_वृक्ष(), intfRefp);
                }
            }
        }
    }
    शून्य visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        जर (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) परत;
        जर (nodep->varp()->isClassMember()) परत;
        जर (nodep->varp()->isFuncLocal()) परत;

        // When creating a --lib-create library ...
        जर (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            जर (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) परत;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    शून्य visit(AstConstDType* nodep) override {
        जर (!m_traVscp) परत;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = सत्य;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    शून्य visit(AstRefDType* nodep) override {
        जर (!m_traVscp) परत;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    शून्य visit(AstIfaceRefDType* /*nodep*/) override {
        जर (!m_traVscp) परत;
        // Insert a placeholder to be fixed up later
        File_ओळ* स्थिरांक flp = m_traVscp->fileline();
        Ast_गाठ_Stmt* स्थिरांक stmtp
            = नवीन AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    शून्य visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        जर (!m_traVscp) परत;

        जर (v3Global.opt.traceMaxArray()
            && static_cast<पूर्णांक>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            परत;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        जर (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            परत;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        जर (isBasicIO()) m_skipDtypeFunc = सत्य;

        जर (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, असत्य);
    }
    शून्य visit(AstPackArrayDType* nodep) override {
        जर (!m_traVscp) परत;

        जर (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            परत;
        }

        Ast_गाठ_DType* स्थिरांक subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        जर (AstBasicDType* स्थिरांक basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            जर (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                परत;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        जर (isBasicIO()) m_skipDtypeFunc = सत्य;

        जर (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, असत्य);
    }
    शून्य visit(AstStructDType* nodep) override {
        जर (!m_traVscp) परत;

        जर (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            परत;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        जर (isBasicIO()) m_skipDtypeFunc = सत्य;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        जर (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, असत्य);
    }
    शून्य visit(AstUnionDType* nodep) override {
        जर (!m_traVscp) परत;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = सत्य;

        जर (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            परत;
        }

        VL_RESTORER_COPY(m_traName);
        File_ओळ* स्थिरांक flp = nodep->fileline();

        पूर्णांक nMembers = 0;
        साठी (Ast_गाठ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        जर (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } अन्यथा {
            addToSubFunc(नवीन Ast_Trace_ढकला_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            साठी (स्थिरांक AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाठ_DType* स्थिरांक subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(नवीन Ast_Trace_काढा_Prefix{flp});
        }
    }
    शून्य visit(AstBasicDType* nodep) override {
        जर (!m_traVscp) परत;
        जर (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } अन्यथा {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    शून्य visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    शून्य visit(Ast_गाठ_DType*) override {
        // Note more specific dtypes above
        जर (!m_traVscp) परत;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_शीर्ष_Scopep{nodep->शीर्ष_Scopep()} {
        File_ओळ* स्थिरांक flp = nodep->fileline();

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
        remove_Redundant_Prefix_ढकला_काढा();

        स्थिरांक std::vector<AstCFunc*>& शीर्ष_Scope_Funcps = m_scopeInitFuncps.at(m_शीर्ष_Scopep->scopep());
        AstCFunc* मूळ_Funcp = nullptr;
        जर (!v3Global.opt.libCreate().empty()) {
            मूळ_Funcp = newCFunc(flp, "trace_init_root");
            मूळ_Funcp->entryPoint(सत्य);
            साठी (माप_प्रकार i = 0; i < m_शीर्ष_Scope_मूळ_Func_मोजणी; ++i) {
                AstCCall* स्थिरांक callp = नवीन AstCCall{flp, शीर्ष_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                मूळ_Funcp->addStmtsp(callp->makeStmt());
            }
            जर (!m_शीर्ष_Scope_मूळ_Func_मोजणी) मूळ_Funcp->addStmtsp(नवीन AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        साठी (माप_प्रकार i = m_शीर्ष_Scope_मूळ_Func_मोजणी; i < शीर्ष_Scope_Funcps.size(); ++i) {
            AstCFunc* स्थिरांक funcp = शीर्ष_Scope_Funcps.at(i);
            AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_शीर्ष_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        जर (m_शीर्ष_Funcps.empty()) add_To_शीर्ष_Func(नवीन AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        जर (m_शीर्ष_Funcps.size() > 1) {
            AstCFunc* स्थिरांक शीर्ष_Funcp = newCFunc(flp, "");
            साठी (AstCFunc* funcp : m_शीर्ष_Funcps) {
                AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                शीर्ष_Funcp->addStmtsp(callp->makeStmt());
            }
            m_शीर्ष_Funcps.clear();
            m_शीर्ष_Funcps.ढकला_back(शीर्ष_Funcp);
        }

        // Set name of top level function
        AstCFunc* स्थिरांक शीर्ष_Funcp = m_शीर्ष_Funcps.front();
        शीर्ष_Funcp->name("trace_init_top");
        शीर्ष_Funcp->entryPoint(सत्य);

        जर (मूळ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(मूळ_Funcp);
        checkCalls(शीर्ष_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

शून्य V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("tracedecl", 0, dump_वृक्ष_Either_Level() >= 3);
}
