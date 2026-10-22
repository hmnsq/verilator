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

#ಅನ್ತರ್ಭಾವ "V3LinkDotIfaceCapture.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3SymTable.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
ತಾರ್ಕಿಕ V3LinkDotIfaceCapture::s_enabled = ಸತ್ಯ;

// LCOV_EXCL_START
ಶೂನ್ಯ V3LinkDotIfaceCapture::enable(ತಾರ್ಕಿಕ flag) {
    s_enabled = flag;
    ಯದಿ (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

ಶೂನ್ಯ V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
ನಾಮಾಕಾಶ {
ಸಂರಚನಾ StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_ಗಂಟು*>> m_byName;
};
std::unordered_map<Ast_ಗಂಟು_Module*, StmtNameMap> s_moduleCache;

ಧ್ರುವ StmtNameMap& getOrBuild(Ast_ಗಂಟು_Module* modp) {
    auto it = s_moduleCache.find(modp);
    ಯದಿ (it != s_moduleCache.end()) ಪ್ರತ್ಯಾಗಮ it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ಧ್ರುವ string& nm = stmtp->name();
        ಯದಿ (!nm.empty()) cache.m_byName[nm].ತಳ್ಳು_back(stmtp);
    }
    ಪ್ರತ್ಯಾಗಮ cache;
}
}  // namespace

ಶೂನ್ಯ V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_ಗಂಟು_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    ಪ್ರತಿ (Ast_ಗಂಟು_DType* curp = dtypep; curp;) {
        ಯದಿ (AstIfaceRefDType* ಧ್ರುವ irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            ಭಙ್ಗ;
        } ಅನ್ಯಥಾ ಯದಿ (AstBracketArrayDType* ಧ್ರುವ bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } ಅನ್ಯಥಾ ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } ಅನ್ಯಥಾ {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    ಪ್ರತ್ಯಾಗಮ resultp;
}

ನಾಮಾಕಾಶ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ಧ್ರುವ string& hint, Ast_ಗಂಟು* nodep) {
    ಯದಿ (!hint.empty()) ಪ್ರತ್ಯಾಗಮ hint;
    ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ "";
    Ast_ಗಂಟು_Module* ಧ್ರುವ ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    ಪ್ರತ್ಯಾಗಮ ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& name) {
    AstTypedef* resultp = nullptr;
    ಧ್ರುವ StmtNameMap& cache = getOrBuild(modp);
    ಧ್ರುವ auto it = cache.m_byName.find(name);
    ಯದಿ (!(it == cache.m_byName.end())) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep : it->second) {
            ಯದಿ (AstTypedef* ಧ್ರುವ tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                ಭಙ್ಗ;
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ resultp;
}
Ast_ಗಂಟು_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& name,
                                                       VNType type) {

    Ast_ಗಂಟು_DType* resultp = nullptr;
    ಧ್ರುವ StmtNameMap& cache = getOrBuild(modp);
    ಧ್ರುವ auto it = cache.m_byName.find(name);
    ಯದಿ (!(it == cache.m_byName.end())) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep : it->second) {
            ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = VN_CAST(nodep, ಗಂಟು_DType)) {
                ಯದಿ (dtp->type() == type) {
                    resultp = dtp;
                    ಭಙ್ಗ;
                }
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_ಗಂಟು_Module* modp,
                                                                ಧ್ರುವ string& name) {

    AstParamTypeDType* resultp = nullptr;
    ಧ್ರುವ StmtNameMap& cache = getOrBuild(modp);
    ಧ್ರುವ auto it = cache.m_byName.find(name);
    ಯದಿ (!(it == cache.m_byName.end())) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep : it->second) {
            ಯದಿ (AstParamTypeDType* ಧ್ರುವ ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                ಭಙ್ಗ;
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ resultp;
}

ತಾರ್ಕಿಕ V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_ಗಂಟು_Module* targetModp) {
    ಧ್ರುವ VIfaceCaptureTag* ಧ್ರುವ tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    ಯದಿ (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* ಧ್ರುವ paramTypep = findParamTypeInModule(targetModp, refp->name());
        ಯದಿ (!paramTypep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        retargetRefToParamType(refp, paramTypep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    AstTypedef* ಧ್ರುವ typedefp = findTypedefInModule(targetModp, refp->name());
    ಯದಿ (!typedefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    retargetRefToTypedef(refp, typedefp);
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

ನಾಮಾಕಾಶ {
using LiveNodes = std::unordered_set<ಧ್ರುವ Ast_ಗಂಟು*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_ಗಂಟು* nodep) { liveNodes.ಸೇರಿಸು(nodep); });
    ಪ್ರತ್ಯಾಗಮ liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_ಗಂಟು_Module* findOwnerModuleImpl(Ast_ಗಂಟು* nodep, ಧ್ರುವ LiveNodes* liveNodesp) {
    ಪ್ರತಿ (Ast_ಗಂಟು* curp = nodep; curp; curp = curp->backp()) {
        ಯದಿ (liveNodesp && !liveNodesp->ಎಣಿಕೆ(curp)) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(curp, ಗಂಟು_Module)) ಪ್ರತ್ಯಾಗಮ modp;
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

Ast_ಗಂಟು_Module* findOwnerModuleIfLive(Ast_ಗಂಟು* nodep, ಧ್ರುವ LiveNodes& liveNodes) {
    ಪ್ರತ್ಯಾಗಮ findOwnerModuleImpl(nodep, &liveNodes);
}

ತಾರ್ಕಿಕ moduleMatchesOwner(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& ownerName) {
    ಯದಿ (!modp || ownerName.empty()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_ಗಂಟು_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_ಗಂಟು* nodep) {
    ಪ್ರತ್ಯಾಗಮ findOwnerModuleImpl(nodep, nullptr);
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::dumpEntries(ಧ್ರುವ string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    ಪೂರ್ಣಾಙ್ಕ idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ಧ್ರುವ VIfaceCaptureTag* ಧ್ರುವ tagp = captureTag(refp);
        ಯದಿ (!tagp) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp = findOwnerModule(refp);
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

ಶೂನ್ಯ V3LinkDotIfaceCapture::tag(AstRefDType* refp, ಧ್ರುವ Ast_ಗಂಟು_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, ಧ್ರುವ string& cellPath,
                                ಧ್ರುವ string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

ಧ್ರುವ VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    ಧ್ರುವ VIfaceCaptureTag* ಧ್ರುವ tagp = refp->captureTagp();
    ಯದಿ (!tagp) ಪ್ರತ್ಯಾಗಮ nullptr;
    ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ ownerp = findOwnerModule(refp);
    ಯದಿ (!ownerp || ownerp->origName() != tagp->m_capturedInName) ಪ್ರತ್ಯಾಗಮ nullptr;
    ಪ್ರತ್ಯಾಗಮ tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_ಗಂಟು_Module* V3LinkDotIfaceCapture::followCellPath(Ast_ಗಂಟು_Module* startModp,
                                                     ಧ್ರುವ string& cellPath) {
    ಯದಿ (cellPath.empty() || !startModp) ಪ್ರತ್ಯಾಗಮ nullptr;
    Ast_ಗಂಟು_Module* curModp = startModp;
    string remaining = cellPath;
    ಯಾವತ್ (!remaining.empty() && curModp) {
        string component;
        ಧ್ರುವ size_t dotPos = remaining.find('.');
        ಯದಿ (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } ಅನ್ಯಥಾ {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ಧ್ರುವ string componentBase = Ast_ಗಂಟು::nameNoArray(component);
        Ast_ಗಂಟು_Module* nextModp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            ಯದಿ (AstCell* ಧ್ರುವ cellp = VN_CAST(sp, Cell)) {
                ಯದಿ ((cellp->name() == component
                     || Ast_ಗಂಟು::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    ಭಙ್ಗ;
                }
            }
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(sp, Var)) {
                ಯದಿ (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    ಧ್ರುವ size_t viftopPos = varBaseName.find("__Viftop");
                    ಯದಿ (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    ಯದಿ (varBaseName == component
                        || Ast_ಗಂಟು::nameNoArray(varBaseName) == componentBase) {
                        ಯದಿ (AstIfaceRefDType* ಧ್ರುವ irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ಧ್ರುವ ifacep = irefp->ifaceViaCellp();
                            ಯದಿ (ifacep) {
                                nextModp = ifacep;
                                ಭಙ್ಗ;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    ಪ್ರತ್ಯಾಗಮ curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
ಶೂನ್ಯ V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ಧ್ರುವ ವರ್ಣ* stageLabel,
                                                  ಪೂರ್ಣಾಙ್ಕ dotPos, ಧ್ರುವ std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_ಗಂಟು_Module* modp,
                                                  ಧ್ರುವ std::function<std::string()>& indentFn) {
    ಯದಿ (!enabled() || !refp) ಪ್ರತ್ಯಾಗಮ;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    ಧ್ರುವ AstCell* ifaceCellp = nullptr;
    ಯದಿ (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ಧ್ರುವ AstCell* ಧ್ರುವ cellp = VN_AS(dotSymp->nodep(), Cell);
        ಯದಿ (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    ಯದಿ (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        ಪ್ರತ್ಯಾಗಮ;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    ಯದಿ (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        ಪ್ರತ್ಯಾಗಮ;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    ಧ್ರುವ string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    ಯದಿ (AstParamTypeDType* ಧ್ರುವ paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } ಅನ್ಯಥಾ {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, ಧ್ರುವ string& cellPath,
                                         Ast_ಗಂಟು_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ಧ್ರುವ string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ಧ್ರುವ string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    ಯದಿ (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

ಪೂರ್ಣಾಙ್ಕ V3LinkDotIfaceCapture::resolveCapturedRefs() {
    ಪೂರ್ಣಾಙ್ಕ fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ಧ್ರುವ VIfaceCaptureTag* ಧ್ರುವ tagp = captureTag(refp);
        ಯದಿ (!tagp) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp = findOwnerModule(refp);
        ಯದಿ (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) ಪ್ರತ್ಯಾಗಮ;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_ಗಂಟು_Module* correctModp = nullptr;
        ಯದಿ (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } ಅನ್ಯಥಾ {
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
        refp->user3(ಸತ್ಯ);
        ++fixed;
    });

    ಪ್ರತ್ಯಾಗಮ fixed;
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::verifyNoDeadRefs(ಧ್ರುವ LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    ಪ್ರತಿ (Ast_ಗಂಟು* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(nodep, ಗಂಟು_Module)) {
            ಯದಿ (modp->dead()) ಅನುವರ್ತ;
            modp->foreach([&](AstRefDType* refp) {
                ಯದಿ (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.ಎಣಿಕೆ(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                ಯದಿ (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.ಎಣಿಕೆ(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp
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
    ಯದಿ (v3Global.rootp()->typeTablep()) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                ಯದಿ (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.ಎಣಿಕೆ(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                ಯದಿ (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.ಎಣಿಕೆ(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp
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

ಶೂನ್ಯ V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    ಯದಿ (!s_enabled) ಪ್ರತ್ಯಾಗಮ;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    ಯದಿ (!v3Global.rootp()) ಪ್ರತ್ಯಾಗಮ;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    ಯದಿ (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    ಯದಿ (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
