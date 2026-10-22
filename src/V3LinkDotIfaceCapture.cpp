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

#அந்தர்பாவ "V3LinkDotIfaceCapture.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3SymTable.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
தருக்க V3LinkDotIfaceCapture::s_enabled = உண்மை;

// LCOV_EXCL_START
வெறுமை V3LinkDotIfaceCapture::enable(தருக்க flag) {
    s_enabled = flag;
    எனில் (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

வெறுமை V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
நாமாகாஶ {
கட்டமைப்பு StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_முனை*>> m_byName;
};
std::unordered_map<Ast_முனை_Module*, StmtNameMap> s_moduleCache;

மாறிலி StmtNameMap& getOrBuild(Ast_முனை_Module* modp) {
    auto it = s_moduleCache.find(modp);
    எனில் (it != s_moduleCache.end()) திரும்பு it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        மாறிலி string& nm = stmtp->name();
        எனில் (!nm.empty()) cache.m_byName[nm].தள்ளு_back(stmtp);
    }
    திரும்பு cache;
}
}  // namespace

வெறுமை V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_முனை_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    க்கு (Ast_முனை_DType* curp = dtypep; curp;) {
        எனில் (AstIfaceRefDType* மாறிலி irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            நிறுத்து;
        } இல்லையெனில் எனில் (AstBracketArrayDType* மாறிலி bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } இல்லையெனில் எனில் (AstUnpackArrayDType* மாறிலி unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } இல்லையெனில் {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    திரும்பு resultp;
}

நாமாகாஶ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(மாறிலி string& hint, Ast_முனை* nodep) {
    எனில் (!hint.empty()) திரும்பு hint;
    எனில் (!nodep) திரும்பு "";
    Ast_முனை_Module* மாறிலி ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    திரும்பு ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_முனை_Module* modp, மாறிலி string& name) {
    AstTypedef* resultp = nullptr;
    மாறிலி StmtNameMap& cache = getOrBuild(modp);
    மாறிலி auto it = cache.m_byName.find(name);
    எனில் (!(it == cache.m_byName.end())) {
        க்கு (Ast_முனை* nodep : it->second) {
            எனில் (AstTypedef* மாறிலி tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                நிறுத்து;
            }
        }
    }
    திரும்பு resultp;
}
Ast_முனை_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_முனை_Module* modp, மாறிலி string& name,
                                                       VNType type) {

    Ast_முனை_DType* resultp = nullptr;
    மாறிலி StmtNameMap& cache = getOrBuild(modp);
    மாறிலி auto it = cache.m_byName.find(name);
    எனில் (!(it == cache.m_byName.end())) {
        க்கு (Ast_முனை* nodep : it->second) {
            எனில் (Ast_முனை_DType* மாறிலி dtp = VN_CAST(nodep, முனை_DType)) {
                எனில் (dtp->type() == type) {
                    resultp = dtp;
                    நிறுத்து;
                }
            }
        }
    }
    திரும்பு resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_முனை_Module* modp,
                                                                மாறிலி string& name) {

    AstParamTypeDType* resultp = nullptr;
    மாறிலி StmtNameMap& cache = getOrBuild(modp);
    மாறிலி auto it = cache.m_byName.find(name);
    எனில் (!(it == cache.m_byName.end())) {
        க்கு (Ast_முனை* nodep : it->second) {
            எனில் (AstParamTypeDType* மாறிலி ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                நிறுத்து;
            }
        }
    }
    திரும்பு resultp;
}

தருக்க V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_முனை_Module* targetModp) {
    மாறிலி VIfaceCaptureTag* மாறிலி tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    எனில் (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* மாறிலி paramTypep = findParamTypeInModule(targetModp, refp->name());
        எனில் (!paramTypep) திரும்பு பொய்;
        retargetRefToParamType(refp, paramTypep);
        திரும்பு உண்மை;
    }

    AstTypedef* மாறிலி typedefp = findTypedefInModule(targetModp, refp->name());
    எனில் (!typedefp) திரும்பு பொய்;
    retargetRefToTypedef(refp, typedefp);
    திரும்பு உண்மை;
}

வெறுமை V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

வெறுமை V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    எனில் (Ast_முனை_DType* மாறிலி dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

நாமாகாஶ {
using LiveNodes = std::unordered_set<மாறிலி Ast_முனை*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_முனை* nodep) { liveNodes.செருகு(nodep); });
    திரும்பு liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_முனை_Module* findOwnerModuleImpl(Ast_முனை* nodep, மாறிலி LiveNodes* liveNodesp) {
    க்கு (Ast_முனை* curp = nodep; curp; curp = curp->backp()) {
        எனில் (liveNodesp && !liveNodesp->மொத்தம்(curp)) திரும்பு nullptr;
        எனில் (Ast_முனை_Module* மாறிலி modp = VN_CAST(curp, முனை_Module)) திரும்பு modp;
    }
    திரும்பு nullptr;
}

Ast_முனை_Module* findOwnerModuleIfLive(Ast_முனை* nodep, மாறிலி LiveNodes& liveNodes) {
    திரும்பு findOwnerModuleImpl(nodep, &liveNodes);
}

தருக்க moduleMatchesOwner(மாறிலி Ast_முனை_Module* modp, மாறிலி string& ownerName) {
    எனில் (!modp || ownerName.empty()) திரும்பு பொய்;
    திரும்பு modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_முனை_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_முனை* nodep) {
    திரும்பு findOwnerModuleImpl(nodep, nullptr);
}

வெறுமை V3LinkDotIfaceCapture::dumpEntries(மாறிலி string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    முழு idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        மாறிலி VIfaceCaptureTag* மாறிலி tagp = captureTag(refp);
        எனில் (!tagp) திரும்பு;
        மாறிலி Ast_முனை_Module* மாறிலி ownerModp = findOwnerModule(refp);
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

வெறுமை V3LinkDotIfaceCapture::tag(AstRefDType* refp, மாறிலி Ast_முனை_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, மாறிலி string& cellPath,
                                மாறிலி string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

மாறிலி VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    மாறிலி VIfaceCaptureTag* மாறிலி tagp = refp->captureTagp();
    எனில் (!tagp) திரும்பு nullptr;
    மாறிலி Ast_முனை_Module* மாறிலி ownerp = findOwnerModule(refp);
    எனில் (!ownerp || ownerp->origName() != tagp->m_capturedInName) திரும்பு nullptr;
    திரும்பு tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_முனை_Module* V3LinkDotIfaceCapture::followCellPath(Ast_முனை_Module* startModp,
                                                     மாறிலி string& cellPath) {
    எனில் (cellPath.empty() || !startModp) திரும்பு nullptr;
    Ast_முனை_Module* curModp = startModp;
    string remaining = cellPath;
    போது (!remaining.empty() && curModp) {
        string component;
        மாறிலி size_t dotPos = remaining.find('.');
        எனில் (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } இல்லையெனில் {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        மாறிலி string componentBase = Ast_முனை::nameNoArray(component);
        Ast_முனை_Module* nextModp = nullptr;
        க்கு (Ast_முனை* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            எனில் (AstCell* மாறிலி cellp = VN_CAST(sp, Cell)) {
                எனில் ((cellp->name() == component
                     || Ast_முனை::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    நிறுத்து;
                }
            }
            எனில் (AstVar* மாறிலி varp = VN_CAST(sp, Var)) {
                எனில் (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    மாறிலி size_t viftopPos = varBaseName.find("__Viftop");
                    எனில் (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    எனில் (varBaseName == component
                        || Ast_முனை::nameNoArray(varBaseName) == componentBase) {
                        எனில் (AstIfaceRefDType* மாறிலி irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* மாறிலி ifacep = irefp->ifaceViaCellp();
                            எனில் (ifacep) {
                                nextModp = ifacep;
                                நிறுத்து;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    திரும்பு curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
வெறுமை V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, மாறிலி எழுத்து* stageLabel,
                                                  முழு dotPos, மாறிலி std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_முனை_Module* modp,
                                                  மாறிலி std::function<std::string()>& indentFn) {
    எனில் (!enabled() || !refp) திரும்பு;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    மாறிலி AstCell* ifaceCellp = nullptr;
    எனில் (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        மாறிலி AstCell* மாறிலி cellp = VN_AS(dotSymp->nodep(), Cell);
        எனில் (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    எனில் (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        திரும்பு;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    எனில் (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        திரும்பு;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    மாறிலி string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    எனில் (AstParamTypeDType* மாறிலி paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } இல்லையெனில் {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

வெறுமை V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, மாறிலி string& cellPath,
                                         Ast_முனை_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         மாறிலி string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    மாறிலி string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    எனில் (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

முழு V3LinkDotIfaceCapture::resolveCapturedRefs() {
    முழு fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        மாறிலி VIfaceCaptureTag* மாறிலி tagp = captureTag(refp);
        எனில் (!tagp) திரும்பு;
        Ast_முனை_Module* மாறிலி ownerModp = findOwnerModule(refp);
        எனில் (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) திரும்பு;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_முனை_Module* correctModp = nullptr;
        எனில் (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } இல்லையெனில் {
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
        refp->user3(உண்மை);
        ++fixed;
    });

    திரும்பு fixed;
}

வெறுமை V3LinkDotIfaceCapture::verifyNoDeadRefs(மாறிலி LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    க்கு (Ast_முனை* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        எனில் (Ast_முனை_Module* மாறிலி modp = VN_CAST(nodep, முனை_Module)) {
            எனில் (modp->dead()) தொடர்;
            modp->foreach([&](AstRefDType* refp) {
                எனில் (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.மொத்தம்(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_முனை_Module* மாறிலி ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                எனில் (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.மொத்தம்(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_முனை_Module* மாறிலி ownerModp
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
    எனில் (v3Global.rootp()->typeTablep()) {
        க்கு (Ast_முனை* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                எனில் (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.மொத்தம்(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_முனை_Module* மாறிலி ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                எனில் (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.மொத்தம்(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_முனை_Module* மாறிலி ownerModp
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

வெறுமை V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    எனில் (!s_enabled) திரும்பு;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    எனில் (!v3Global.rootp()) திரும்பு;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    மாறிலி முழு capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    எனில் (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    எனில் (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
