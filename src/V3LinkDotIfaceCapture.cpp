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

#অন্তর্ভাব "V3LinkDotIfaceCapture.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3SymTable.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
তার্কিক V3LinkDotIfaceCapture::s_enabled = সত্য;

// LCOV_EXCL_START
শূন্য V3LinkDotIfaceCapture::enable(তার্কিক flag) {
    s_enabled = flag;
    যদি (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

শূন্য V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
নামাকাশ {
সংরচনা StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_গ্রন্থি*>> m_byName;
};
std::unordered_map<Ast_গ্রন্থি_Module*, StmtNameMap> s_moduleCache;

ধ্রুব StmtNameMap& getOrBuild(Ast_গ্রন্থি_Module* modp) {
    auto it = s_moduleCache.find(modp);
    যদি (it != s_moduleCache.end()) প্রত্যাগম it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    প্রতি (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ধ্রুব string& nm = stmtp->name();
        যদি (!nm.empty()) cache.m_byName[nm].ঠেলো_back(stmtp);
    }
    প্রত্যাগম cache;
}
}  // namespace

শূন্য V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_গ্রন্থি_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    প্রতি (Ast_গ্রন্থি_DType* curp = dtypep; curp;) {
        যদি (AstIfaceRefDType* ধ্রুব irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            ভঙ্গ;
        } অন্যথা যদি (AstBracketArrayDType* ধ্রুব bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } অন্যথা যদি (AstUnpackArrayDType* ধ্রুব unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } অন্যথা {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    প্রত্যাগম resultp;
}

নামাকাশ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ধ্রুব string& hint, Ast_গ্রন্থি* nodep) {
    যদি (!hint.empty()) প্রত্যাগম hint;
    যদি (!nodep) প্রত্যাগম "";
    Ast_গ্রন্থি_Module* ধ্রুব ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    প্রত্যাগম ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_গ্রন্থি_Module* modp, ধ্রুব string& name) {
    AstTypedef* resultp = nullptr;
    ধ্রুব StmtNameMap& cache = getOrBuild(modp);
    ধ্রুব auto it = cache.m_byName.find(name);
    যদি (!(it == cache.m_byName.end())) {
        প্রতি (Ast_গ্রন্থি* nodep : it->second) {
            যদি (AstTypedef* ধ্রুব tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                ভঙ্গ;
            }
        }
    }
    প্রত্যাগম resultp;
}
Ast_গ্রন্থি_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_গ্রন্থি_Module* modp, ধ্রুব string& name,
                                                       VNType type) {

    Ast_গ্রন্থি_DType* resultp = nullptr;
    ধ্রুব StmtNameMap& cache = getOrBuild(modp);
    ধ্রুব auto it = cache.m_byName.find(name);
    যদি (!(it == cache.m_byName.end())) {
        প্রতি (Ast_গ্রন্থি* nodep : it->second) {
            যদি (Ast_গ্রন্থি_DType* ধ্রুব dtp = VN_CAST(nodep, গ্রন্থি_DType)) {
                যদি (dtp->type() == type) {
                    resultp = dtp;
                    ভঙ্গ;
                }
            }
        }
    }
    প্রত্যাগম resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_গ্রন্থি_Module* modp,
                                                                ধ্রুব string& name) {

    AstParamTypeDType* resultp = nullptr;
    ধ্রুব StmtNameMap& cache = getOrBuild(modp);
    ধ্রুব auto it = cache.m_byName.find(name);
    যদি (!(it == cache.m_byName.end())) {
        প্রতি (Ast_গ্রন্থি* nodep : it->second) {
            যদি (AstParamTypeDType* ধ্রুব ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                ভঙ্গ;
            }
        }
    }
    প্রত্যাগম resultp;
}

তার্কিক V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_গ্রন্থি_Module* targetModp) {
    ধ্রুব VIfaceCaptureTag* ধ্রুব tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    যদি (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* ধ্রুব paramTypep = findParamTypeInModule(targetModp, refp->name());
        যদি (!paramTypep) প্রত্যাগম অসত্য;
        retargetRefToParamType(refp, paramTypep);
        প্রত্যাগম সত্য;
    }

    AstTypedef* ধ্রুব typedefp = findTypedefInModule(targetModp, refp->name());
    যদি (!typedefp) প্রত্যাগম অসত্য;
    retargetRefToTypedef(refp, typedefp);
    প্রত্যাগম সত্য;
}

শূন্য V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

শূন্য V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    যদি (Ast_গ্রন্থি_DType* ধ্রুব dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

নামাকাশ {
using LiveNodes = std::unordered_set<ধ্রুব Ast_গ্রন্থি*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_গ্রন্থি* nodep) { liveNodes.ঢোকাও(nodep); });
    প্রত্যাগম liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_গ্রন্থি_Module* findOwnerModuleImpl(Ast_গ্রন্থি* nodep, ধ্রুব LiveNodes* liveNodesp) {
    প্রতি (Ast_গ্রন্থি* curp = nodep; curp; curp = curp->backp()) {
        যদি (liveNodesp && !liveNodesp->গুনতি(curp)) প্রত্যাগম nullptr;
        যদি (Ast_গ্রন্থি_Module* ধ্রুব modp = VN_CAST(curp, গ্রন্থি_Module)) প্রত্যাগম modp;
    }
    প্রত্যাগম nullptr;
}

Ast_গ্রন্থি_Module* findOwnerModuleIfLive(Ast_গ্রন্থি* nodep, ধ্রুব LiveNodes& liveNodes) {
    প্রত্যাগম findOwnerModuleImpl(nodep, &liveNodes);
}

তার্কিক moduleMatchesOwner(ধ্রুব Ast_গ্রন্থি_Module* modp, ধ্রুব string& ownerName) {
    যদি (!modp || ownerName.empty()) প্রত্যাগম অসত্য;
    প্রত্যাগম modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_গ্রন্থি_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_গ্রন্থি* nodep) {
    প্রত্যাগম findOwnerModuleImpl(nodep, nullptr);
}

শূন্য V3LinkDotIfaceCapture::dumpEntries(ধ্রুব string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    পূর্ণাঙ্ক idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ধ্রুব VIfaceCaptureTag* ধ্রুব tagp = captureTag(refp);
        যদি (!tagp) প্রত্যাগম;
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব ownerModp = findOwnerModule(refp);
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

শূন্য V3LinkDotIfaceCapture::tag(AstRefDType* refp, ধ্রুব Ast_গ্রন্থি_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, ধ্রুব string& cellPath,
                                ধ্রুব string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

ধ্রুব VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    ধ্রুব VIfaceCaptureTag* ধ্রুব tagp = refp->captureTagp();
    যদি (!tagp) প্রত্যাগম nullptr;
    ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব ownerp = findOwnerModule(refp);
    যদি (!ownerp || ownerp->origName() != tagp->m_capturedInName) প্রত্যাগম nullptr;
    প্রত্যাগম tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_গ্রন্থি_Module* V3LinkDotIfaceCapture::followCellPath(Ast_গ্রন্থি_Module* startModp,
                                                     ধ্রুব string& cellPath) {
    যদি (cellPath.empty() || !startModp) প্রত্যাগম nullptr;
    Ast_গ্রন্থি_Module* curModp = startModp;
    string remaining = cellPath;
    যাবত্ (!remaining.empty() && curModp) {
        string component;
        ধ্রুব size_t dotPos = remaining.find('.');
        যদি (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } অন্যথা {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ধ্রুব string componentBase = Ast_গ্রন্থি::nameNoArray(component);
        Ast_গ্রন্থি_Module* nextModp = nullptr;
        প্রতি (Ast_গ্রন্থি* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            যদি (AstCell* ধ্রুব cellp = VN_CAST(sp, Cell)) {
                যদি ((cellp->name() == component
                     || Ast_গ্রন্থি::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    ভঙ্গ;
                }
            }
            যদি (AstVar* ধ্রুব varp = VN_CAST(sp, Var)) {
                যদি (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    ধ্রুব size_t viftopPos = varBaseName.find("__Viftop");
                    যদি (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    যদি (varBaseName == component
                        || Ast_গ্রন্থি::nameNoArray(varBaseName) == componentBase) {
                        যদি (AstIfaceRefDType* ধ্রুব irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ধ্রুব ifacep = irefp->ifaceViaCellp();
                            যদি (ifacep) {
                                nextModp = ifacep;
                                ভঙ্গ;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    প্রত্যাগম curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
শূন্য V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ধ্রুব বর্ণ* stageLabel,
                                                  পূর্ণাঙ্ক dotPos, ধ্রুব std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_গ্রন্থি_Module* modp,
                                                  ধ্রুব std::function<std::string()>& indentFn) {
    যদি (!enabled() || !refp) প্রত্যাগম;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    ধ্রুব AstCell* ifaceCellp = nullptr;
    যদি (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ধ্রুব AstCell* ধ্রুব cellp = VN_AS(dotSymp->nodep(), Cell);
        যদি (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    যদি (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        প্রত্যাগম;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    যদি (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        প্রত্যাগম;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    ধ্রুব string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    যদি (AstParamTypeDType* ধ্রুব paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } অন্যথা {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

শূন্য V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, ধ্রুব string& cellPath,
                                         Ast_গ্রন্থি_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ধ্রুব string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ধ্রুব string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    যদি (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

পূর্ণাঙ্ক V3LinkDotIfaceCapture::resolveCapturedRefs() {
    পূর্ণাঙ্ক fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ধ্রুব VIfaceCaptureTag* ধ্রুব tagp = captureTag(refp);
        যদি (!tagp) প্রত্যাগম;
        Ast_গ্রন্থি_Module* ধ্রুব ownerModp = findOwnerModule(refp);
        যদি (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) প্রত্যাগম;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_গ্রন্থি_Module* correctModp = nullptr;
        যদি (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } অন্যথা {
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
        refp->user3(সত্য);
        ++fixed;
    });

    প্রত্যাগম fixed;
}

শূন্য V3LinkDotIfaceCapture::verifyNoDeadRefs(ধ্রুব LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    প্রতি (Ast_গ্রন্থি* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        যদি (Ast_গ্রন্থি_Module* ধ্রুব modp = VN_CAST(nodep, গ্রন্থি_Module)) {
            যদি (modp->dead()) অনুবর্ত;
            modp->foreach([&](AstRefDType* refp) {
                যদি (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.গুনতি(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_গ্রন্থি_Module* ধ্রুব ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                যদি (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.গুনতি(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_গ্রন্থি_Module* ধ্রুব ownerModp
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
    যদি (v3Global.rootp()->typeTablep()) {
        প্রতি (Ast_গ্রন্থি* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                যদি (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.গুনতি(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_গ্রন্থি_Module* ধ্রুব ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                যদি (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.গুনতি(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_গ্রন্থি_Module* ধ্রুব ownerModp
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

শূন্য V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    যদি (!s_enabled) প্রত্যাগম;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    যদি (!v3Global.rootp()) প্রত্যাগম;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ধ্রুব পূর্ণাঙ্ক capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    যদি (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    যদি (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
