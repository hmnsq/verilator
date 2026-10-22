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

#समावेश "V3LinkDotIfaceCapture.h"

#समावेश "V3Error.h"
#समावेश "V3Global.h"
#समावेश "V3Stats.h"
#समावेश "V3SymTable.h"

#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
bool V3LinkDotIfaceCapture::s_enabled = true;

// LCOV_EXCL_START
व्योम V3LinkDotIfaceCapture::enable(bool flag) {
    s_enabled = flag;
    अगर (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

व्योम V3LinkDotIfaceCapture::reset() {
    s_tags.सफा();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
namespace {
काष्ठा StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_गाँठ*>> m_byName;
};
std::unordered_map<Ast_गाँठ_Module*, StmtNameMap> s_moduleCache;

स्थिर StmtNameMap& getOrBuild(Ast_गाँठ_Module* modp) {
    स्वतः it = s_moduleCache.find(modp);
    अगर (it != s_moduleCache.end()) वापस it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        स्थिर string& nm = stmtp->name();
        अगर (!nm.empty()) cache.m_byName[nm].धकेलो_back(stmtp);
    }
    वापस cache;
}
}  // namespace

व्योम V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.सफा(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_गाँठ_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    क्रम (Ast_गाँठ_DType* curp = dtypep; curp;) {
        अगर (AstIfaceRefDType* स्थिर irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            अवरोध;
        } अन्यथा अगर (AstBracketArrayDType* स्थिर bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } अन्यथा अगर (AstUnpackArrayDType* स्थिर unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } अन्यथा {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    वापस resultp;
}

namespace {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(स्थिर string& hint, Ast_गाँठ* nodep) {
    अगर (!hint.empty()) वापस hint;
    अगर (!nodep) वापस "";
    Ast_गाँठ_Module* स्थिर ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    वापस ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_गाँठ_Module* modp, स्थिर string& name) {
    AstTypedef* resultp = nullptr;
    स्थिर StmtNameMap& cache = getOrBuild(modp);
    स्थिर स्वतः it = cache.m_byName.find(name);
    अगर (!(it == cache.m_byName.end())) {
        क्रम (Ast_गाँठ* nodep : it->second) {
            अगर (AstTypedef* स्थिर tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                अवरोध;
            }
        }
    }
    वापस resultp;
}
Ast_गाँठ_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_गाँठ_Module* modp, स्थिर string& name,
                                                       VNType type) {

    Ast_गाँठ_DType* resultp = nullptr;
    स्थिर StmtNameMap& cache = getOrBuild(modp);
    स्थिर स्वतः it = cache.m_byName.find(name);
    अगर (!(it == cache.m_byName.end())) {
        क्रम (Ast_गाँठ* nodep : it->second) {
            अगर (Ast_गाँठ_DType* स्थिर dtp = VN_CAST(nodep, गाँठ_DType)) {
                अगर (dtp->type() == type) {
                    resultp = dtp;
                    अवरोध;
                }
            }
        }
    }
    वापस resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_गाँठ_Module* modp,
                                                                स्थिर string& name) {

    AstParamTypeDType* resultp = nullptr;
    स्थिर StmtNameMap& cache = getOrBuild(modp);
    स्थिर स्वतः it = cache.m_byName.find(name);
    अगर (!(it == cache.m_byName.end())) {
        क्रम (Ast_गाँठ* nodep : it->second) {
            अगर (AstParamTypeDType* स्थिर ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                अवरोध;
            }
        }
    }
    वापस resultp;
}

bool V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_गाँठ_Module* targetModp) {
    स्थिर VIfaceCaptureTag* स्थिर tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    अगर (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* स्थिर paramTypep = findParamTypeInModule(targetModp, refp->name());
        अगर (!paramTypep) वापस false;
        retargetRefToParamType(refp, paramTypep);
        वापस true;
    }

    AstTypedef* स्थिर typedefp = findTypedefInModule(targetModp, refp->name());
    अगर (!typedefp) वापस false;
    retargetRefToTypedef(refp, typedefp);
    वापस true;
}

व्योम V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

व्योम V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    अगर (Ast_गाँठ_DType* स्थिर dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

namespace {
using LiveNodes = std::unordered_set<स्थिर Ast_गाँठ*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_गाँठ* nodep) { liveNodes.जोड़ो(nodep); });
    वापस liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_गाँठ_Module* findOwnerModuleImpl(Ast_गाँठ* nodep, स्थिर LiveNodes* liveNodesp) {
    क्रम (Ast_गाँठ* curp = nodep; curp; curp = curp->backp()) {
        अगर (liveNodesp && !liveNodesp->गिनती(curp)) वापस nullptr;
        अगर (Ast_गाँठ_Module* स्थिर modp = VN_CAST(curp, गाँठ_Module)) वापस modp;
    }
    वापस nullptr;
}

Ast_गाँठ_Module* findOwnerModuleIfLive(Ast_गाँठ* nodep, स्थिर LiveNodes& liveNodes) {
    वापस findOwnerModuleImpl(nodep, &liveNodes);
}

bool moduleMatchesOwner(स्थिर Ast_गाँठ_Module* modp, स्थिर string& ownerName) {
    अगर (!modp || ownerName.empty()) वापस false;
    वापस modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_गाँठ_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_गाँठ* nodep) {
    वापस findOwnerModuleImpl(nodep, nullptr);
}

व्योम V3LinkDotIfaceCapture::dumpEntries(स्थिर string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    पूर्णांक idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        स्थिर VIfaceCaptureTag* स्थिर tagp = captureTag(refp);
        अगर (!tagp) वापस;
        स्थिर Ast_गाँठ_Module* स्थिर ownerModp = findOwnerModule(refp);
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

व्योम V3LinkDotIfaceCapture::tag(AstRefDType* refp, स्थिर Ast_गाँठ_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, स्थिर string& cellPath,
                                स्थिर string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

स्थिर VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    स्थिर VIfaceCaptureTag* स्थिर tagp = refp->captureTagp();
    अगर (!tagp) वापस nullptr;
    स्थिर Ast_गाँठ_Module* स्थिर ownerp = findOwnerModule(refp);
    अगर (!ownerp || ownerp->origName() != tagp->m_capturedInName) वापस nullptr;
    वापस tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_गाँठ_Module* V3LinkDotIfaceCapture::followCellPath(Ast_गाँठ_Module* startModp,
                                                     स्थिर string& cellPath) {
    अगर (cellPath.empty() || !startModp) वापस nullptr;
    Ast_गाँठ_Module* curModp = startModp;
    string remaining = cellPath;
    जबतक (!remaining.empty() && curModp) {
        string component;
        स्थिर माप_प्रकार dotPos = remaining.find('.');
        अगर (dotPos == string::npos) {
            component = remaining;
            remaining.सफा();
        } अन्यथा {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        स्थिर string componentBase = Ast_गाँठ::nameNoArray(component);
        Ast_गाँठ_Module* अग्र_Modp = nullptr;
        क्रम (Ast_गाँठ* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            अगर (AstCell* स्थिर cellp = VN_CAST(sp, Cell)) {
                अगर ((cellp->name() == component
                     || Ast_गाँठ::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    अग्र_Modp = cellp->modp();
                    अवरोध;
                }
            }
            अगर (AstVar* स्थिर varp = VN_CAST(sp, Var)) {
                अगर (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    स्थिर माप_प्रकार viftopPos = varBaseName.find("__Viftop");
                    अगर (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    अगर (varBaseName == component
                        || Ast_गाँठ::nameNoArray(varBaseName) == componentBase) {
                        अगर (AstIfaceRefDType* स्थिर irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* स्थिर ifacep = irefp->ifaceViaCellp();
                            अगर (ifacep) {
                                अग्र_Modp = ifacep;
                                अवरोध;
                            }
                        }
                    }
                }
            }
        }
        curModp = अग्र_Modp;
    }
    वापस curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
व्योम V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, स्थिर अक्षर* stageLabel,
                                                  पूर्णांक dotPos, स्थिर std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_गाँठ_Module* modp,
                                                  स्थिर std::function<std::string()>& indentFn) {
    अगर (!enabled() || !refp) वापस;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    स्थिर AstCell* ifaceCellp = nullptr;
    अगर (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        स्थिर AstCell* स्थिर cellp = VN_AS(dotSymp->nodep(), Cell);
        अगर (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    अगर (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        वापस;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    अगर (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        वापस;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    स्थिर string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    अगर (AstParamTypeDType* स्थिर paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } अन्यथा {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

व्योम V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, स्थिर string& cellPath,
                                         Ast_गाँठ_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         स्थिर string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    स्थिर string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    अगर (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

पूर्णांक V3LinkDotIfaceCapture::resolveCapturedRefs() {
    पूर्णांक जड़ = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        स्थिर VIfaceCaptureTag* स्थिर tagp = captureTag(refp);
        अगर (!tagp) वापस;
        Ast_गाँठ_Module* स्थिर ownerModp = findOwnerModule(refp);
        अगर (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) वापस;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_गाँठ_Module* correctModp = nullptr;
        अगर (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } अन्यथा {
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
        ++जड़;
    });

    वापस जड़;
}

व्योम V3LinkDotIfaceCapture::verifyNoDeadRefs(स्थिर LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    क्रम (Ast_गाँठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        अगर (Ast_गाँठ_Module* स्थिर modp = VN_CAST(nodep, गाँठ_Module)) {
            अगर (modp->dead()) जारी;
            modp->foreach([&](AstRefDType* refp) {
                अगर (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.गिनती(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_गाँठ_Module* स्थिर ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                अगर (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.गिनती(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_गाँठ_Module* स्थिर ownerModp
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
    अगर (v3Global.rootp()->typeTablep()) {
        क्रम (Ast_गाँठ* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                अगर (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.गिनती(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_गाँठ_Module* स्थिर ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                अगर (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.गिनती(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_गाँठ_Module* स्थिर ownerModp
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

व्योम V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    अगर (!s_enabled) वापस;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    अगर (!v3Global.rootp()) वापस;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    स्थिर पूर्णांक capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    अगर (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.आकार());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    अगर (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
