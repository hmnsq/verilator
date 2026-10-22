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

#include "V3LinkDotIfaceCapture.h"

#include "V3Error.h"
#include "V3Global.h"
#include "V3Stats.h"
#include "V3SymTable.h"

#include <unordered_map>
#include <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
bool V3LinkDotIfaceCapture::s_enabled = true;

// LCOV_EXCL_START
void V3LinkDotIfaceCapture::enable(bool flag) {
    s_enabled = flag;
    གལ་ཏེ (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

void V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
namespace {
sa_nracanaa StmtNameMap final {
    std::unordered_map<string, std::vector<AstNode*>> m_byName;
};
std::unordered_map<AstNodeModule*, StmtNameMap> s_moduleCache;

const StmtNameMap& getOrBuild(AstNodeModule* modp) {
    auto it = s_moduleCache.find(modp);
    གལ་ཏེ (it != s_moduleCache.end()) ཕྱིར it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    ཆེད་དུ (AstNode* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        const string& nm = stmtp->name();
        གལ་ཏེ (!nm.empty()) cache.m_byName[nm].push_back(stmtp);
    }
    ཕྱིར cache;
}
}  // namespace

void V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(AstNodeDType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    ཆེད་དུ (AstNodeDType* curp = dtypep; curp;) {
        གལ་ཏེ (AstIfaceRefDType* const irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            bha_nga;
        } དེ་མིན གལ་ཏེ (AstBracketArrayDType* const bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } དེ་མིན གལ་ཏེ (AstUnpackArrayDType* const unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } དེ་མིན {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    ཕྱིར resultp;
}

namespace {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(const string& hint, AstNode* nodep) {
    གལ་ཏེ (!hint.empty()) ཕྱིར hint;
    གལ་ཏེ (!nodep) ཕྱིར "";
    AstNodeModule* const ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    ཕྱིར ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(AstNodeModule* modp, const string& name) {
    AstTypedef* resultp = nullptr;
    const StmtNameMap& cache = getOrBuild(modp);
    const auto it = cache.m_byName.find(name);
    གལ་ཏེ (!(it == cache.m_byName.end())) {
        ཆེད་དུ (AstNode* nodep : it->second) {
            གལ་ཏེ (AstTypedef* const tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                bha_nga;
            }
        }
    }
    ཕྱིར resultp;
}
AstNodeDType* V3LinkDotIfaceCapture::findDTypeInModule(AstNodeModule* modp, const string& name,
                                                       VNType type) {

    AstNodeDType* resultp = nullptr;
    const StmtNameMap& cache = getOrBuild(modp);
    const auto it = cache.m_byName.find(name);
    གལ་ཏེ (!(it == cache.m_byName.end())) {
        ཆེད་དུ (AstNode* nodep : it->second) {
            གལ་ཏེ (AstNodeDType* const dtp = VN_CAST(nodep, NodeDType)) {
                གལ་ཏེ (dtp->type() == type) {
                    resultp = dtp;
                    bha_nga;
                }
            }
        }
    }
    ཕྱིར resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(AstNodeModule* modp,
                                                                const string& name) {

    AstParamTypeDType* resultp = nullptr;
    const StmtNameMap& cache = getOrBuild(modp);
    const auto it = cache.m_byName.find(name);
    གལ་ཏེ (!(it == cache.m_byName.end())) {
        ཆེད་དུ (AstNode* nodep : it->second) {
            གལ་ཏེ (AstParamTypeDType* const ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                bha_nga;
            }
        }
    }
    ཕྱིར resultp;
}

bool V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, AstNodeModule* targetModp) {
    const VIfaceCaptureTag* const tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    གལ་ཏེ (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* const paramTypep = findParamTypeInModule(targetModp, refp->name());
        གལ་ཏེ (!paramTypep) ཕྱིར false;
        retargetRefToParamType(refp, paramTypep);
        ཕྱིར true;
    }

    AstTypedef* const typedefp = findTypedefInModule(targetModp, refp->name());
    གལ་ཏེ (!typedefp) ཕྱིར false;
    retargetRefToTypedef(refp, typedefp);
    ཕྱིར true;
}

void V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

void V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    གལ་ཏེ (AstNodeDType* const dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

namespace {
using LiveNodes = std::unordered_set<const AstNode*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](AstNode* nodep) { liveNodes.insert(nodep); });
    ཕྱིར liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
AstNodeModule* findOwnerModuleImpl(AstNode* nodep, const LiveNodes* liveNodesp) {
    ཆེད་དུ (AstNode* curp = nodep; curp; curp = curp->backp()) {
        གལ་ཏེ (liveNodesp && !liveNodesp->count(curp)) ཕྱིར nullptr;
        གལ་ཏེ (AstNodeModule* const modp = VN_CAST(curp, NodeModule)) ཕྱིར modp;
    }
    ཕྱིར nullptr;
}

AstNodeModule* findOwnerModuleIfLive(AstNode* nodep, const LiveNodes& liveNodes) {
    ཕྱིར findOwnerModuleImpl(nodep, &liveNodes);
}

bool moduleMatchesOwner(const AstNodeModule* modp, const string& ownerName) {
    གལ་ཏེ (!modp || ownerName.empty()) ཕྱིར false;
    ཕྱིར modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

AstNodeModule* V3LinkDotIfaceCapture::findOwnerModule(AstNode* nodep) {
    ཕྱིར findOwnerModuleImpl(nodep, nullptr);
}

void V3LinkDotIfaceCapture::dumpEntries(const string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    ཧྲིལ idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        const VIfaceCaptureTag* const tagp = captureTag(refp);
        གལ་ཏེ (!tagp) ཕྱིར;
        const AstNodeModule* const ownerModp = findOwnerModule(refp);
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

void V3LinkDotIfaceCapture::tag(AstRefDType* refp, const AstNodeModule* capturedInp,
                                VIfaceCaptureTag::Kind kind, const string& cellPath,
                                const string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

const VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    const VIfaceCaptureTag* const tagp = refp->captureTagp();
    གལ་ཏེ (!tagp) ཕྱིར nullptr;
    const AstNodeModule* const ownerp = findOwnerModule(refp);
    གལ་ཏེ (!ownerp || ownerp->origName() != tagp->m_capturedInName) ཕྱིར nullptr;
    ཕྱིར tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
AstNodeModule* V3LinkDotIfaceCapture::followCellPath(AstNodeModule* startModp,
                                                     const string& cellPath) {
    གལ་ཏེ (cellPath.empty() || !startModp) ཕྱིར nullptr;
    AstNodeModule* curModp = startModp;
    string remaining = cellPath;
    སྐབས (!remaining.empty() && curModp) {
        string component;
        const size_t dotPos = remaining.find('.');
        གལ་ཏེ (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } དེ་མིན {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        const string componentBase = AstNode::nameNoArray(component);
        AstNodeModule* nextModp = nullptr;
        ཆེད་དུ (AstNode* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            གལ་ཏེ (AstCell* const cellp = VN_CAST(sp, Cell)) {
                གལ་ཏེ ((cellp->name() == component
                     || AstNode::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    bha_nga;
                }
            }
            གལ་ཏེ (AstVar* const varp = VN_CAST(sp, Var)) {
                གལ་ཏེ (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    const size_t viftopPos = varBaseName.find("__Viftop");
                    གལ་ཏེ (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    གལ་ཏེ (varBaseName == component
                        || AstNode::nameNoArray(varBaseName) == componentBase) {
                        གལ་ཏེ (AstIfaceRefDType* const irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* const ifacep = irefp->ifaceViaCellp();
                            གལ་ཏེ (ifacep) {
                                nextModp = ifacep;
                                bha_nga;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    ཕྱིར curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
void V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, const var_na* stageLabel,
                                                  ཧྲིལ dotPos, const std::string& dotText,
                                                  VSymEnt* dotSymp, AstNodeModule* modp,
                                                  const std::function<std::string()>& indentFn) {
    གལ་ཏེ (!enabled() || !refp) ཕྱིར;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    const AstCell* ifaceCellp = nullptr;
    གལ་ཏེ (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        const AstCell* const cellp = VN_AS(dotSymp->nodep(), Cell);
        གལ་ཏེ (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    གལ་ཏེ (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        ཕྱིར;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    གལ་ཏེ (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        ཕྱིར;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    const string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    གལ་ཏེ (AstParamTypeDType* const paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } དེ་མིན {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

void V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, const string& cellPath,
                                         AstNodeModule* ownerModp, AstParamTypeDType* paramTypep,
                                         const string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    const string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    གལ་ཏེ (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

ཧྲིལ V3LinkDotIfaceCapture::resolveCapturedRefs() {
    ཧྲིལ fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        const VIfaceCaptureTag* const tagp = captureTag(refp);
        གལ་ཏེ (!tagp) ཕྱིར;
        AstNodeModule* const ownerModp = findOwnerModule(refp);
        གལ་ཏེ (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) ཕྱིར;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        AstNodeModule* correctModp = nullptr;
        གལ་ཏེ (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } དེ་མིན {
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

    ཕྱིར fixed;
}

void V3LinkDotIfaceCapture::verifyNoDeadRefs(const LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    ཆེད་དུ (AstNode* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        གལ་ཏེ (AstNodeModule* const modp = VN_CAST(nodep, NodeModule)) {
            གལ་ཏེ (modp->dead()) continue;
            modp->foreach([&](AstRefDType* refp) {
                གལ་ཏེ (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.count(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    AstNodeModule* const ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                གལ་ཏེ (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.count(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    AstNodeModule* const ownerModp
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
    གལ་ཏེ (v3Global.rootp()->typeTablep()) {
        ཆེད་དུ (AstNode* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                གལ་ཏེ (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.count(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    AstNodeModule* const ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                གལ་ཏེ (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.count(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    AstNodeModule* const ownerModp
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

void V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    གལ་ཏེ (!s_enabled) ཕྱིར;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    གལ་ཏེ (!v3Global.rootp()) ཕྱིར;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    const ཧྲིལ capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    གལ་ཏེ (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    གལ་ཏེ (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
