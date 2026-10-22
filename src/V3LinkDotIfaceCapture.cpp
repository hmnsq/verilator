// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Interface typedef capture helper
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

// ARCHITECTURE - Separation of Concerns (do not change without reading):
//
//   Each captured REFDTYPE carries its own capture record (a VIfaceCaptureTag,
//   see AstRefDType::captureTagp): the cell path from its owner module to the
//   interface, the interface's name, and whether the target is a typedef or a
//   parameter type.  Nothing here points into the AST, so a freed reference
//   takes its record with it, and a cloned reference shares it.
//
//   1. CAPTURE (V3LinkDot, primary pass):
//      captureTypedefContext() / addParamType() tag the reference.
//
//   2. PARAMETERIZATION (V3Param):
//      After a module is cloned, and after an interface cell is specialized,
//      V3Param walks that one module and retargets its tagged references by
//      cell path, so widths computed during parameterization use the
//      specialized interface.
//
//   3. TARGET RESOLUTION (finalizeIfaceCapture, after V3Param):
//      Walks every tagged reference in a live module, follows its cell path
//      from its owner module and retargets it by name, then clears all tags.
//

#شامل "V3LinkDotIfaceCapture.h"

#شامل "V3Error.h"
#شامل "V3Global.h"
#شامل "V3Stats.h"
#شامل "V3SymTable.h"

#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIface_Capture_لیبل> V3LinkDotIfaceCapture::s_لیبل_فہرست{};
منطقی V3LinkDotIfaceCapture::s_enabled = سچ;

// LCOV_EXCL_START
خالی V3LinkDotIfaceCapture::enable(منطقی flag) {
    s_enabled = flag;
    اگر (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

خالی V3LinkDotIfaceCapture::reset() {
    s_لیبل_فہرست.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
نامخلا {
ساخت StmtNameMap final {
    std::unordered_map<string, std::سمتیہ<Ast_گرہ*>> m_byName;
};
std::unordered_map<Ast_گرہ_Module*, StmtNameMap> s_moduleCache;

ثابت StmtNameMap& getOrBuild(Ast_گرہ_Module* modp) {
    خودکار it = s_moduleCache.ڈھونڈو(modp);
    اگر (it != s_moduleCache.انتہا()) واپس it->دوسرا;
    StmtNameMap& cache = s_moduleCache[modp];
    برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ثابت string& nm = stmtp->name();
        اگر (!nm.خالی_خانے()) cache.m_byName[nm].ڈالو_عقب(stmtp);
    }
    واپس cache;
}
}  // namespace

خالی V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_گرہ_DType* dtypep) {
    AstIfaceRefDType* resultp = خالی_اشاریہ;
    برائے (Ast_گرہ_DType* curp = dtypep; curp;) {
        اگر (AstIfaceRefDType* ثابت irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            توڑو;
        } ورنہ اگر (AstBracketArrayDType* ثابت bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } ورنہ اگر (AstUnpackArrayDType* ثابت unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } ورنہ {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    واپس resultp;
}

نامخلا {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ثابت string& hint, Ast_گرہ* nodep) {
    اگر (!hint.خالی_خانے()) واپس hint;
    اگر (!nodep) واپس "";
    Ast_گرہ_Module* ثابت ownerp = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(nodep);
    واپس ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::ڈھونڈو_Typedef_اندر_Module(Ast_گرہ_Module* modp, ثابت string& name) {
    AstTypedef* resultp = خالی_اشاریہ;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت خودکار it = cache.m_byName.ڈھونڈو(name);
    اگر (!(it == cache.m_byName.انتہا())) {
        برائے (Ast_گرہ* nodep : it->دوسرا) {
            اگر (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                توڑو;
            }
        }
    }
    واپس resultp;
}
Ast_گرہ_DType* V3LinkDotIfaceCapture::ڈھونڈو_DType_اندر_Module(Ast_گرہ_Module* modp, ثابت string& name,
                                                       VNType type) {

    Ast_گرہ_DType* resultp = خالی_اشاریہ;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت خودکار it = cache.m_byName.ڈھونڈو(name);
    اگر (!(it == cache.m_byName.انتہا())) {
        برائے (Ast_گرہ* nodep : it->دوسرا) {
            اگر (Ast_گرہ_DType* ثابت dtp = VN_CAST(nodep, گرہ_DType)) {
                اگر (dtp->type() == type) {
                    resultp = dtp;
                    توڑو;
                }
            }
        }
    }
    واپس resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::ڈھونڈو_Param_Type_اندر_Module(Ast_گرہ_Module* modp,
                                                                ثابت string& name) {

    AstParamTypeDType* resultp = خالی_اشاریہ;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت خودکار it = cache.m_byName.ڈھونڈو(name);
    اگر (!(it == cache.m_byName.انتہا())) {
        برائے (Ast_گرہ* nodep : it->دوسرا) {
            اگر (AstParamTypeDType* ثابت ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                توڑو;
            }
        }
    }
    واپس resultp;
}

منطقی V3LinkDotIfaceCapture::retarget_Ref_مقصد_Module(AstRefDType* refp, Ast_گرہ_Module* ہدف_Modp) {
    ثابت VIface_Capture_لیبل* ثابت tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(ہدف_Modp, refp, "Retarget to a null module");

    اگر (tagp->m_قسم == VIface_Capture_لیبل::قسم_2::PARAM_TYPE) {
        AstParamTypeDType* ثابت paramTypep = ڈھونڈو_Param_Type_اندر_Module(ہدف_Modp, refp->name());
        اگر (!paramTypep) واپس جھوٹ;
        retarget_Ref_مقصد_Param_Type(refp, paramTypep);
        واپس سچ;
    }

    AstTypedef* ثابت typedefp = ڈھونڈو_Typedef_اندر_Module(ہدف_Modp, refp->name());
    اگر (!typedefp) واپس جھوٹ;
    retarget_Ref_مقصد_Typedef(refp, typedefp);
    واپس سچ;
}

خالی V3LinkDotIfaceCapture::retarget_Ref_مقصد_Param_Type(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

خالی V3LinkDotIfaceCapture::retarget_Ref_مقصد_Typedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    اگر (Ast_گرہ_DType* ثابت dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

نامخلا {
استعمال زندہ_Nodes = std::unordered_سیٹ<ثابت Ast_گرہ*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
زندہ_Nodes collect_زندہ_Nodes() {
    زندہ_Nodes زندہ_Nodes_2;
    v3Global.rootp()->foreach([&](Ast_گرہ* nodep) { زندہ_Nodes_2.داخل_کرو(nodep); });
    واپس زندہ_Nodes_2;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_گرہ_Module* ڈھونڈو_Owner_Module_Impl(Ast_گرہ* nodep, ثابت زندہ_Nodes* زندہ_Nodesp) {
    برائے (Ast_گرہ* curp = nodep; curp; curp = curp->backp()) {
        اگر (زندہ_Nodesp && !زندہ_Nodesp->گنتی(curp)) واپس خالی_اشاریہ;
        اگر (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) واپس modp;
    }
    واپس خالی_اشاریہ;
}

Ast_گرہ_Module* ڈھونڈو_Owner_Module_If_زندہ(Ast_گرہ* nodep, ثابت زندہ_Nodes& زندہ_Nodes_2) {
    واپس ڈھونڈو_Owner_Module_Impl(nodep, &زندہ_Nodes_2);
}

منطقی moduleMatchesOwner(ثابت Ast_گرہ_Module* modp, ثابت string& ownerName) {
    اگر (!modp || ownerName.خالی_خانے()) واپس جھوٹ;
    واپس modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_گرہ_Module* V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(Ast_گرہ* nodep) {
    واپس ڈھونڈو_Owner_Module_Impl(nodep, خالی_اشاریہ);
}

خالی V3LinkDotIfaceCapture::dumpEntries(ثابت string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    صحیح idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIface_Capture_لیبل* ثابت tagp = capture_لیبل(refp);
        اگر (!tagp) واپس;
        ثابت Ast_گرہ_Module* ثابت ownerModp = ڈھونڈو_Owner_Module(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvt_مقصد_Hex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cell_راستہ << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_قسم == VIface_Capture_لیبل::قسم_2::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

خالی V3LinkDotIfaceCapture::لیبل(AstRefDType* refp, ثابت Ast_گرہ_Module* capturedInp,
                                VIface_Capture_لیبل::قسم_2 قسم, ثابت string& cell_راستہ,
                                ثابت string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cell_راستہ.خالی_خانے(), refp, "Captured reference has no cell path");
    s_لیبل_فہرست.emplace_عقب(VIface_Capture_لیبل{قسم, cell_راستہ, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_لیبل_فہرست.عقب());
}

ثابت VIface_Capture_لیبل* V3LinkDotIfaceCapture::capture_لیبل(AstRefDType* refp) {
    ثابت VIface_Capture_لیبل* ثابت tagp = refp->captureTagp();
    اگر (!tagp) واپس خالی_اشاریہ;
    ثابت Ast_گرہ_Module* ثابت ownerp = ڈھونڈو_Owner_Module(refp);
    اگر (!ownerp || ownerp->origName() != tagp->m_captured_اندر_Name) واپس خالی_اشاریہ;
    واپس tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_گرہ_Module* V3LinkDotIfaceCapture::follow_Cell_راستہ(Ast_گرہ_Module* آغاز_Modp,
                                                     ثابت string& cell_راستہ) {
    اگر (cell_راستہ.خالی_خانے() || !آغاز_Modp) واپس خالی_اشاریہ;
    Ast_گرہ_Module* curModp = آغاز_Modp;
    string بقایا = cell_راستہ;
    جب (!بقایا.خالی_خانے() && curModp) {
        string component;
        ثابت سائز_t dotPos = بقایا.ڈھونڈو('.');
        اگر (dotPos == string::npos) {
            component = بقایا;
            بقایا.clear();
        } ورنہ {
            component = بقایا.substr(0, dotPos);
            بقایا = بقایا.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ثابت string component_بنیاد = Ast_گرہ::nameNoArray(component);
        Ast_گرہ_Module* اگلا_Modp = خالی_اشاریہ;
        برائے (Ast_گرہ* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            اگر (AstCell* ثابت cellp = VN_CAST(sp, Cell)) {
                اگر ((cellp->name() == component
                     || Ast_گرہ::nameNoArray(cellp->name()) == component_بنیاد)
                    && cellp->modp()) {
                    اگلا_Modp = cellp->modp();
                    توڑو;
                }
            }
            اگر (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                اگر (varp->isIfaceRef() && varp->subDTypep()) {
                    string var_بنیاد_Name = varp->name();
                    ثابت سائز_t viftopPos = var_بنیاد_Name.ڈھونڈو("__Viftop");
                    اگر (viftopPos != string::npos) {
                        var_بنیاد_Name = var_بنیاد_Name.substr(0, viftopPos);
                    }
                    اگر (var_بنیاد_Name == component
                        || Ast_گرہ::nameNoArray(var_بنیاد_Name) == component_بنیاد) {
                        اگر (AstIfaceRefDType* ثابت irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ثابت ifacep = irefp->iface_بذریعہ_Cellp();
                            اگر (ifacep) {
                                اگلا_Modp = ifacep;
                                توڑو;
                            }
                        }
                    }
                }
            }
        }
        curModp = اگلا_Modp;
    }
    واپس curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
خالی V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ثابت حرف* stageLabel,
                                                  صحیح dotPos, ثابت std::string& dot_متن,
                                                  VSymEnt* dotSymp, Ast_گرہ_Module* modp,
                                                  ثابت std::function<std::string()>& indentFn) {
    اگر (!enabled() || !refp) واپس;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dot_متن << "' dotSym=" << dotSymp);

    ثابت AstCell* ifaceCellp = خالی_اشاریہ;
    اگر (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ثابت AstCell* ثابت cellp = VN_AS(dotSymp->nodep(), Cell);
        اگر (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    اگر (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        واپس;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    اگر (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        واپس;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dot_متن.خالی_خانے(), refp, "captureTypedefContext: dotText empty");
    ثابت string cell_راستہ = dot_متن;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    اگر (AstParamTypeDType* ثابت paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::جوڑو_Param_Type(refp, cell_راستہ, modp, paramTypep, "");
    } ورنہ {
        لیبل(refp, modp, VIface_Capture_لیبل::قسم_2::TYPEDEF, cell_راستہ,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cell_راستہ << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

خالی V3LinkDotIfaceCapture::جوڑو_Param_Type(AstRefDType* refp, ثابت string& cell_راستہ,
                                         Ast_گرہ_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ثابت string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ثابت string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cell_راستہ << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    اگر (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    لیبل(refp, ownerModp, VIface_Capture_لیبل::قسم_2::PARAM_TYPE, cell_راستہ, ptOwnerName);
}

صحیح V3LinkDotIfaceCapture::resolveCapturedRefs() {
    صحیح fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIface_Capture_لیبل* ثابت tagp = capture_لیبل(refp);
        اگر (!tagp) واپس;
        Ast_گرہ_Module* ثابت ownerModp = ڈھونڈو_Owner_Module(refp);
        اگر (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) واپس;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvt_مقصد_Hex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cell_راستہ << "' targetKind="
                     << (tagp->m_قسم == VIface_Capture_لیبل::قسم_2::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_گرہ_Module* correctModp = خالی_اشاریہ;
        اگر (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } ورنہ {
            correctModp = follow_Cell_راستہ(ownerModp, tagp->m_cell_راستہ);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cell_راستہ
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cell_راستہ << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retarget_Ref_مقصد_Module(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_قسم == VIface_Capture_لیبل::قسم_2::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(سچ);
        ++fixed;
    });

    واپس fixed;
}

خالی V3LinkDotIfaceCapture::verifyNoDeadRefs(ثابت زندہ_Nodes& زندہ_Nodes_2) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    برائے (Ast_گرہ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        اگر (Ast_گرہ_Module* ثابت modp = VN_CAST(nodep, گرہ_Module)) {
            اگر (modp->dead()) جاری;
            modp->foreach([&](AstRefDType* refp) {
                اگر (refp->typedefp()) {
                    UASSERT_OBJ(زندہ_Nodes_2.گنتی(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_گرہ_Module* ثابت ownerModp
                        = ڈھونڈو_Owner_Module_If_زندہ(refp->typedefp(), زندہ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                اگر (refp->refDTypep()) {
                    UASSERT_OBJ(زندہ_Nodes_2.گنتی(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_گرہ_Module* ثابت ownerModp
                        = ڈھونڈو_Owner_Module_If_زندہ(refp->refDTypep(), زندہ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    اگر (v3Global.rootp()->typeTablep()) {
        برائے (Ast_گرہ* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                اگر (refp->typedefp()) {
                    UASSERT_OBJ(زندہ_Nodes_2.گنتی(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_گرہ_Module* ثابت ownerModp
                        = ڈھونڈو_Owner_Module_If_زندہ(refp->typedefp(), زندہ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                اگر (refp->refDTypep()) {
                    UASSERT_OBJ(زندہ_Nodes_2.گنتی(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_گرہ_Module* ثابت ownerModp
                        = ڈھونڈو_Owner_Module_If_زندہ(refp->refDTypep(), زندہ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has refDTypep pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
}

خالی V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    اگر (!s_enabled) واپس;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    اگر (!v3Global.rootp()) واپس;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ثابت صحیح capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    اگر (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::جوڑو_Stat("IfaceCapture, Captured refs", s_لیبل_فہرست.سائز());
    V3Stats::جوڑو_Stat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    اگر (v3Global.opt.debugCheck()) verifyNoDeadRefs(collect_زندہ_Nodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(خالی_اشاریہ); });
    reset();
}
