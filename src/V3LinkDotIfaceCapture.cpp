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

#ضمن "V3LinkDotIfaceCapture.h"

#ضمن "V3Error.h"
#ضمن "V3Global.h"
#ضمن "V3Stats.h"
#ضمن "V3SymTable.h"

#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIface_Capture_ٹیگ> V3LinkDotIfaceCapture::s_ٹیگ_سوچی{};
منطقي V3LinkDotIfaceCapture::s_enabled = سچ;

// LCOV_EXCL_START
فراغ V3LinkDotIfaceCapture::enable(منطقي flag) {
    s_enabled = flag;
    إذا (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

فراغ V3LinkDotIfaceCapture::reset() {
    s_ٹیگ_سوچی.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
نطاق {
بنية StmtNameMap final {
    std::unordered_map<string, std::سمتیہ<Ast_گرہ*>> m_byName;
};
std::unordered_map<Ast_گرہ_Module*, StmtNameMap> s_moduleCache;

ثابت StmtNameMap& getOrBuild(Ast_گرہ_Module* modp) {
    خودکار it = s_moduleCache.لبھو(modp);
    إذا (it != s_moduleCache.سرا()) أرجع it->دوجا;
    StmtNameMap& cache = s_moduleCache[modp];
    لكل (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ثابت string& nm = stmtp->name();
        إذا (!nm.خالی_تھاواں()) cache.m_byName[nm].ڈالو_پٹھ(stmtp);
    }
    أرجع cache;
}
}  // namespace

فراغ V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_گرہ_DType* dtypep) {
    AstIfaceRefDType* resultp = خالی_سوچک;
    لكل (Ast_گرہ_DType* curp = dtypep; curp;) {
        إذا (AstIfaceRefDType* ثابت irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            اكسر;
        } وإلا إذا (AstBracketArrayDType* ثابت bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } وإلا إذا (AstUnpackArrayDType* ثابت unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } وإلا {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    أرجع resultp;
}

نطاق {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ثابت string& hint, Ast_گرہ* nodep) {
    إذا (!hint.خالی_تھاواں()) أرجع hint;
    إذا (!nodep) أرجع "";
    Ast_گرہ_Module* ثابت ownerp = V3LinkDotIfaceCapture::لبھو_Owner_Module(nodep);
    أرجع ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::لبھو_Typedef_وچ_Module(Ast_گرہ_Module* modp, ثابت string& name) {
    AstTypedef* resultp = خالی_سوچک;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت خودکار it = cache.m_byName.لبھو(name);
    إذا (!(it == cache.m_byName.سرا())) {
        لكل (Ast_گرہ* nodep : it->دوجا) {
            إذا (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                اكسر;
            }
        }
    }
    أرجع resultp;
}
Ast_گرہ_DType* V3LinkDotIfaceCapture::لبھو_DType_وچ_Module(Ast_گرہ_Module* modp, ثابت string& name,
                                                       VNType type) {

    Ast_گرہ_DType* resultp = خالی_سوچک;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت خودکار it = cache.m_byName.لبھو(name);
    إذا (!(it == cache.m_byName.سرا())) {
        لكل (Ast_گرہ* nodep : it->دوجا) {
            إذا (Ast_گرہ_DType* ثابت dtp = VN_CAST(nodep, گرہ_DType)) {
                إذا (dtp->type() == type) {
                    resultp = dtp;
                    اكسر;
                }
            }
        }
    }
    أرجع resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::لبھو_Param_Type_وچ_Module(Ast_گرہ_Module* modp,
                                                                ثابت string& name) {

    AstParamTypeDType* resultp = خالی_سوچک;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت خودکار it = cache.m_byName.لبھو(name);
    إذا (!(it == cache.m_byName.سرا())) {
        لكل (Ast_گرہ* nodep : it->دوجا) {
            إذا (AstParamTypeDType* ثابت ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                اكسر;
            }
        }
    }
    أرجع resultp;
}

منطقي V3LinkDotIfaceCapture::retarget_Ref_منزل_Module(AstRefDType* refp, Ast_گرہ_Module* نشانہ_Modp) {
    ثابت VIface_Capture_ٹیگ* ثابت tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(نشانہ_Modp, refp, "Retarget to a null module");

    إذا (tagp->m_قسم == VIface_Capture_ٹیگ::قسم_2::PARAM_TYPE) {
        AstParamTypeDType* ثابت paramTypep = لبھو_Param_Type_وچ_Module(نشانہ_Modp, refp->name());
        إذا (!paramTypep) أرجع خطأ;
        retarget_Ref_منزل_Param_Type(refp, paramTypep);
        أرجع سچ;
    }

    AstTypedef* ثابت typedefp = لبھو_Typedef_وچ_Module(نشانہ_Modp, refp->name());
    إذا (!typedefp) أرجع خطأ;
    retarget_Ref_منزل_Typedef(refp, typedefp);
    أرجع سچ;
}

فراغ V3LinkDotIfaceCapture::retarget_Ref_منزل_Param_Type(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

فراغ V3LinkDotIfaceCapture::retarget_Ref_منزل_Typedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    إذا (Ast_گرہ_DType* ثابت dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

نطاق {
ورتوں جیوندا_Nodes = std::unordered_سیٹ<ثابت Ast_گرہ*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
جیوندا_Nodes collect_جیوندا_Nodes() {
    جیوندا_Nodes جیوندا_Nodes_2;
    v3Global.rootp()->foreach([&](Ast_گرہ* nodep) { جیوندا_Nodes_2.داخل_کرو(nodep); });
    أرجع جیوندا_Nodes_2;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_گرہ_Module* لبھو_Owner_Module_Impl(Ast_گرہ* nodep, ثابت جیوندا_Nodes* جیوندا_Nodesp) {
    لكل (Ast_گرہ* curp = nodep; curp; curp = curp->backp()) {
        إذا (جیوندا_Nodesp && !جیوندا_Nodesp->گنتی(curp)) أرجع خالی_سوچک;
        إذا (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) أرجع modp;
    }
    أرجع خالی_سوچک;
}

Ast_گرہ_Module* لبھو_Owner_Module_If_جیوندا(Ast_گرہ* nodep, ثابت جیوندا_Nodes& جیوندا_Nodes_2) {
    أرجع لبھو_Owner_Module_Impl(nodep, &جیوندا_Nodes_2);
}

منطقي moduleMatchesOwner(ثابت Ast_گرہ_Module* modp, ثابت string& ownerName) {
    إذا (!modp || ownerName.خالی_تھاواں()) أرجع خطأ;
    أرجع modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_گرہ_Module* V3LinkDotIfaceCapture::لبھو_Owner_Module(Ast_گرہ* nodep) {
    أرجع لبھو_Owner_Module_Impl(nodep, خالی_سوچک);
}

فراغ V3LinkDotIfaceCapture::dumpEntries(ثابت string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    صحيح idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIface_Capture_ٹیگ* ثابت tagp = capture_ٹیگ(refp);
        إذا (!tagp) أرجع;
        ثابت Ast_گرہ_Module* ثابت ownerModp = لبھو_Owner_Module(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvt_منزل_Hex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cell_رستہ << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_قسم == VIface_Capture_ٹیگ::قسم_2::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

فراغ V3LinkDotIfaceCapture::ٹیگ(AstRefDType* refp, ثابت Ast_گرہ_Module* capturedInp,
                                VIface_Capture_ٹیگ::قسم_2 قسم, ثابت string& cell_رستہ,
                                ثابت string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cell_رستہ.خالی_تھاواں(), refp, "Captured reference has no cell path");
    s_ٹیگ_سوچی.emplace_پٹھ(VIface_Capture_ٹیگ{قسم, cell_رستہ, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_ٹیگ_سوچی.پٹھ());
}

ثابت VIface_Capture_ٹیگ* V3LinkDotIfaceCapture::capture_ٹیگ(AstRefDType* refp) {
    ثابت VIface_Capture_ٹیگ* ثابت tagp = refp->captureTagp();
    إذا (!tagp) أرجع خالی_سوچک;
    ثابت Ast_گرہ_Module* ثابت ownerp = لبھو_Owner_Module(refp);
    إذا (!ownerp || ownerp->origName() != tagp->m_captured_وچ_Name) أرجع خالی_سوچک;
    أرجع tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_گرہ_Module* V3LinkDotIfaceCapture::follow_Cell_رستہ(Ast_گرہ_Module* شروع_Modp,
                                                     ثابت string& cell_رستہ) {
    إذا (cell_رستہ.خالی_تھاواں() || !شروع_Modp) أرجع خالی_سوچک;
    Ast_گرہ_Module* curModp = شروع_Modp;
    string بچیا = cell_رستہ;
    طالما (!بچیا.خالی_تھاواں() && curModp) {
        string component;
        ثابت ناپ_t dotPos = بچیا.لبھو('.');
        إذا (dotPos == string::npos) {
            component = بچیا;
            بچیا.clear();
        } وإلا {
            component = بچیا.substr(0, dotPos);
            بچیا = بچیا.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ثابت string component_ادھار = Ast_گرہ::nameNoArray(component);
        Ast_گرہ_Module* اگلا_Modp = خالی_سوچک;
        لكل (Ast_گرہ* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(sp, Cell)) {
                إذا ((cellp->name() == component
                     || Ast_گرہ::nameNoArray(cellp->name()) == component_ادھار)
                    && cellp->modp()) {
                    اگلا_Modp = cellp->modp();
                    اكسر;
                }
            }
            إذا (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                إذا (varp->isIfaceRef() && varp->subDTypep()) {
                    string var_ادھار_Name = varp->name();
                    ثابت ناپ_t viftopPos = var_ادھار_Name.لبھو("__Viftop");
                    إذا (viftopPos != string::npos) {
                        var_ادھار_Name = var_ادھار_Name.substr(0, viftopPos);
                    }
                    إذا (var_ادھار_Name == component
                        || Ast_گرہ::nameNoArray(var_ادھار_Name) == component_ادھار) {
                        إذا (AstIfaceRefDType* ثابت irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ثابت ifacep = irefp->iface_راہیں_Cellp();
                            إذا (ifacep) {
                                اگلا_Modp = ifacep;
                                اكسر;
                            }
                        }
                    }
                }
            }
        }
        curModp = اگلا_Modp;
    }
    أرجع curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
فراغ V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ثابت حرف* stageLabel,
                                                  صحيح dotPos, ثابت std::string& dot_متن,
                                                  VSymEnt* dotSymp, Ast_گرہ_Module* modp,
                                                  ثابت std::function<std::string()>& indentFn) {
    إذا (!enabled() || !refp) أرجع;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dot_متن << "' dotSym=" << dotSymp);

    ثابت AstCell* ifaceCellp = خالی_سوچک;
    إذا (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ثابت AstCell* ثابت cellp = VN_AS(dotSymp->nodep(), Cell);
        إذا (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    إذا (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        أرجع;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    إذا (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        أرجع;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dot_متن.خالی_تھاواں(), refp, "captureTypedefContext: dotText empty");
    ثابت string cell_رستہ = dot_متن;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    إذا (AstParamTypeDType* ثابت paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::جوڑو_Param_Type(refp, cell_رستہ, modp, paramTypep, "");
    } وإلا {
        ٹیگ(refp, modp, VIface_Capture_ٹیگ::قسم_2::TYPEDEF, cell_رستہ,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cell_رستہ << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

فراغ V3LinkDotIfaceCapture::جوڑو_Param_Type(AstRefDType* refp, ثابت string& cell_رستہ,
                                         Ast_گرہ_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ثابت string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ثابت string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cell_رستہ << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    إذا (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    ٹیگ(refp, ownerModp, VIface_Capture_ٹیگ::قسم_2::PARAM_TYPE, cell_رستہ, ptOwnerName);
}

صحيح V3LinkDotIfaceCapture::resolveCapturedRefs() {
    صحيح fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIface_Capture_ٹیگ* ثابت tagp = capture_ٹیگ(refp);
        إذا (!tagp) أرجع;
        Ast_گرہ_Module* ثابت ownerModp = لبھو_Owner_Module(refp);
        إذا (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) أرجع;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvt_منزل_Hex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cell_رستہ << "' targetKind="
                     << (tagp->m_قسم == VIface_Capture_ٹیگ::قسم_2::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_گرہ_Module* correctModp = خالی_سوچک;
        إذا (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } وإلا {
            correctModp = follow_Cell_رستہ(ownerModp, tagp->m_cell_رستہ);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cell_رستہ
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cell_رستہ << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retarget_Ref_منزل_Module(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_قسم == VIface_Capture_ٹیگ::قسم_2::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(سچ);
        ++fixed;
    });

    أرجع fixed;
}

فراغ V3LinkDotIfaceCapture::verifyNoDeadRefs(ثابت جیوندا_Nodes& جیوندا_Nodes_2) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    لكل (Ast_گرہ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        إذا (Ast_گرہ_Module* ثابت modp = VN_CAST(nodep, گرہ_Module)) {
            إذا (modp->dead()) استمر;
            modp->foreach([&](AstRefDType* refp) {
                إذا (refp->typedefp()) {
                    UASSERT_OBJ(جیوندا_Nodes_2.گنتی(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_گرہ_Module* ثابت ownerModp
                        = لبھو_Owner_Module_If_جیوندا(refp->typedefp(), جیوندا_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                إذا (refp->refDTypep()) {
                    UASSERT_OBJ(جیوندا_Nodes_2.گنتی(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_گرہ_Module* ثابت ownerModp
                        = لبھو_Owner_Module_If_جیوندا(refp->refDTypep(), جیوندا_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    إذا (v3Global.rootp()->typeTablep()) {
        لكل (Ast_گرہ* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                إذا (refp->typedefp()) {
                    UASSERT_OBJ(جیوندا_Nodes_2.گنتی(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_گرہ_Module* ثابت ownerModp
                        = لبھو_Owner_Module_If_جیوندا(refp->typedefp(), جیوندا_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                إذا (refp->refDTypep()) {
                    UASSERT_OBJ(جیوندا_Nodes_2.گنتی(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_گرہ_Module* ثابت ownerModp
                        = لبھو_Owner_Module_If_جیوندا(refp->refDTypep(), جیوندا_Nodes_2);
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

فراغ V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    إذا (!s_enabled) أرجع;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    إذا (!v3Global.rootp()) أرجع;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ثابت صحيح capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    إذا (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::جوڑو_Stat("IfaceCapture, Captured refs", s_ٹیگ_سوچی.ناپ());
    V3Stats::جوڑو_Stat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    إذا (v3Global.opt.debugCheck()) verifyNoDeadRefs(collect_جیوندا_Nodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(خالی_سوچک); });
    reset();
}
