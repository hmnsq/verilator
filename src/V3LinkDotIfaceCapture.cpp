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

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
منطقي V3LinkDotIfaceCapture::s_enabled = true;

// LCOV_EXCL_START
فراغ V3LinkDotIfaceCapture::enable(منطقي flag) {
    s_enabled = flag;
    إذا (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

فراغ V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
نطاق {
بنية StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_عقدة*>> m_byName;
};
std::unordered_map<Ast_عقدة_Module*, StmtNameMap> s_moduleCache;

ثابت StmtNameMap& getOrBuild(Ast_عقدة_Module* modp) {
    auto it = s_moduleCache.find(modp);
    إذا (it != s_moduleCache.end()) إرجاع it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    لكل (Ast_عقدة* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ثابت string& nm = stmtp->name();
        إذا (!nm.empty()) cache.m_byName[nm].ادفع_back(stmtp);
    }
    إرجاع cache;
}
}  // namespace

فراغ V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_عقدة_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    لكل (Ast_عقدة_DType* curp = dtypep; curp;) {
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
    إرجاع resultp;
}

نطاق {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ثابت string& hint, Ast_عقدة* nodep) {
    إذا (!hint.empty()) إرجاع hint;
    إذا (!nodep) إرجاع "";
    Ast_عقدة_Module* ثابت ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    إرجاع ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_عقدة_Module* modp, ثابت string& name) {
    AstTypedef* resultp = nullptr;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت auto it = cache.m_byName.find(name);
    إذا (!(it == cache.m_byName.end())) {
        لكل (Ast_عقدة* nodep : it->second) {
            إذا (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                اكسر;
            }
        }
    }
    إرجاع resultp;
}
Ast_عقدة_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_عقدة_Module* modp, ثابت string& name,
                                                       VNType type) {

    Ast_عقدة_DType* resultp = nullptr;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت auto it = cache.m_byName.find(name);
    إذا (!(it == cache.m_byName.end())) {
        لكل (Ast_عقدة* nodep : it->second) {
            إذا (Ast_عقدة_DType* ثابت dtp = VN_CAST(nodep, عقدة_DType)) {
                إذا (dtp->type() == type) {
                    resultp = dtp;
                    اكسر;
                }
            }
        }
    }
    إرجاع resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_عقدة_Module* modp,
                                                                ثابت string& name) {

    AstParamTypeDType* resultp = nullptr;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت auto it = cache.m_byName.find(name);
    إذا (!(it == cache.m_byName.end())) {
        لكل (Ast_عقدة* nodep : it->second) {
            إذا (AstParamTypeDType* ثابت ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                اكسر;
            }
        }
    }
    إرجاع resultp;
}

منطقي V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_عقدة_Module* targetModp) {
    ثابت VIfaceCaptureTag* ثابت tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    إذا (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* ثابت paramTypep = findParamTypeInModule(targetModp, refp->name());
        إذا (!paramTypep) إرجاع خطأ;
        retargetRefToParamType(refp, paramTypep);
        إرجاع true;
    }

    AstTypedef* ثابت typedefp = findTypedefInModule(targetModp, refp->name());
    إذا (!typedefp) إرجاع خطأ;
    retargetRefToTypedef(refp, typedefp);
    إرجاع true;
}

فراغ V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

فراغ V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    إذا (Ast_عقدة_DType* ثابت dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

نطاق {
using LiveNodes = std::unordered_set<ثابت Ast_عقدة*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_عقدة* nodep) { liveNodes.أدرج(nodep); });
    إرجاع liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_عقدة_Module* findOwnerModuleImpl(Ast_عقدة* nodep, ثابت LiveNodes* liveNodesp) {
    لكل (Ast_عقدة* curp = nodep; curp; curp = curp->backp()) {
        إذا (liveNodesp && !liveNodesp->تعداد_2(curp)) إرجاع nullptr;
        إذا (Ast_عقدة_Module* ثابت modp = VN_CAST(curp, عقدة_Module)) إرجاع modp;
    }
    إرجاع nullptr;
}

Ast_عقدة_Module* findOwnerModuleIfLive(Ast_عقدة* nodep, ثابت LiveNodes& liveNodes) {
    إرجاع findOwnerModuleImpl(nodep, &liveNodes);
}

منطقي moduleMatchesOwner(ثابت Ast_عقدة_Module* modp, ثابت string& ownerName) {
    إذا (!modp || ownerName.empty()) إرجاع خطأ;
    إرجاع modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_عقدة_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_عقدة* nodep) {
    إرجاع findOwnerModuleImpl(nodep, nullptr);
}

فراغ V3LinkDotIfaceCapture::dumpEntries(ثابت string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    صحيح idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIfaceCaptureTag* ثابت tagp = captureTag(refp);
        إذا (!tagp) إرجاع;
        ثابت Ast_عقدة_Module* ثابت ownerModp = findOwnerModule(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvtToHex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cellPath << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

فراغ V3LinkDotIfaceCapture::tag(AstRefDType* refp, ثابت Ast_عقدة_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, ثابت string& cellPath,
                                ثابت string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

ثابت VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    ثابت VIfaceCaptureTag* ثابت tagp = refp->captureTagp();
    إذا (!tagp) إرجاع nullptr;
    ثابت Ast_عقدة_Module* ثابت ownerp = findOwnerModule(refp);
    إذا (!ownerp || ownerp->origName() != tagp->m_capturedInName) إرجاع nullptr;
    إرجاع tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_عقدة_Module* V3LinkDotIfaceCapture::followCellPath(Ast_عقدة_Module* startModp,
                                                     ثابت string& cellPath) {
    إذا (cellPath.empty() || !startModp) إرجاع nullptr;
    Ast_عقدة_Module* curModp = startModp;
    string remaining = cellPath;
    بينما (!remaining.empty() && curModp) {
        string component;
        ثابت size_t dotPos = remaining.find('.');
        إذا (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } وإلا {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ثابت string componentBase = Ast_عقدة::nameNoArray(component);
        Ast_عقدة_Module* nextModp = nullptr;
        لكل (Ast_عقدة* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(sp, Cell)) {
                إذا ((cellp->name() == component
                     || Ast_عقدة::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    اكسر;
                }
            }
            إذا (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                إذا (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    ثابت size_t viftopPos = varBaseName.find("__Viftop");
                    إذا (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    إذا (varBaseName == component
                        || Ast_عقدة::nameNoArray(varBaseName) == componentBase) {
                        إذا (AstIfaceRefDType* ثابت irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ثابت ifacep = irefp->ifaceViaCellp();
                            إذا (ifacep) {
                                nextModp = ifacep;
                                اكسر;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    إرجاع curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
فراغ V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ثابت حرف* stageLabel,
                                                  صحيح dotPos, ثابت std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_عقدة_Module* modp,
                                                  ثابت std::function<std::string()>& indentFn) {
    إذا (!enabled() || !refp) إرجاع;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    ثابت AstCell* ifaceCellp = nullptr;
    إذا (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ثابت AstCell* ثابت cellp = VN_AS(dotSymp->nodep(), Cell);
        إذا (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    إذا (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        إرجاع;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    إذا (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        إرجاع;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    ثابت string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    إذا (AstParamTypeDType* ثابت paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } وإلا {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

فراغ V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, ثابت string& cellPath,
                                         Ast_عقدة_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ثابت string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ثابت string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
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
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

صحيح V3LinkDotIfaceCapture::resolveCapturedRefs() {
    صحيح fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIfaceCaptureTag* ثابت tagp = captureTag(refp);
        إذا (!tagp) إرجاع;
        Ast_عقدة_Module* ثابت ownerModp = findOwnerModule(refp);
        إذا (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) إرجاع;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_عقدة_Module* correctModp = nullptr;
        إذا (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } وإلا {
            correctModp = followCellPath(ownerModp, tagp->m_cellPath);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cellPath
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cellPath << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retargetRefToModule(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(true);
        ++fixed;
    });

    إرجاع fixed;
}

فراغ V3LinkDotIfaceCapture::verifyNoDeadRefs(ثابت LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    لكل (Ast_عقدة* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        إذا (Ast_عقدة_Module* ثابت modp = VN_CAST(nodep, عقدة_Module)) {
            إذا (modp->dead()) استمر;
            modp->foreach([&](AstRefDType* refp) {
                إذا (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.تعداد_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_عقدة_Module* ثابت ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                إذا (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.تعداد_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_عقدة_Module* ثابت ownerModp
                        = findOwnerModuleIfLive(refp->refDTypep(), liveNodes);
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
        لكل (Ast_عقدة* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                إذا (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.تعداد_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_عقدة_Module* ثابت ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                إذا (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.تعداد_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_عقدة_Module* ثابت ownerModp
                        = findOwnerModuleIfLive(refp->refDTypep(), liveNodes);
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
    إذا (!s_enabled) إرجاع;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    إذا (!v3Global.rootp()) إرجاع;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ثابت صحيح capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    إذا (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    إذا (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
