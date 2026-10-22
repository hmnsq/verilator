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

#અન્તર્ભાવ "V3LinkDotIfaceCapture.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3SymTable.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
તાર્કિક V3LinkDotIfaceCapture::s_enabled = સાચું;

// LCOV_EXCL_START
શૂન્ય V3LinkDotIfaceCapture::enable(તાર્કિક flag) {
    s_enabled = flag;
    જો (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

શૂન્ય V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
નામાકાશ {
સંરચના StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_ગાંઠ*>> m_byName;
};
std::unordered_map<Ast_ગાંઠ_Module*, StmtNameMap> s_moduleCache;

અચલ StmtNameMap& getOrBuild(Ast_ગાંઠ_Module* modp) {
    auto it = s_moduleCache.find(modp);
    જો (it != s_moduleCache.end()) પાછા it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        અચલ string& nm = stmtp->name();
        જો (!nm.empty()) cache.m_byName[nm].ધકેલો_back(stmtp);
    }
    પાછા cache;
}
}  // namespace

શૂન્ય V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_ગાંઠ_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    માટે (Ast_ગાંઠ_DType* curp = dtypep; curp;) {
        જો (AstIfaceRefDType* અચલ irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            તોડો;
        } નહીંતર જો (AstBracketArrayDType* અચલ bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } નહીંતર જો (AstUnpackArrayDType* અચલ unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } નહીંતર {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    પાછા resultp;
}

નામાકાશ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(અચલ string& hint, Ast_ગાંઠ* nodep) {
    જો (!hint.empty()) પાછા hint;
    જો (!nodep) પાછા "";
    Ast_ગાંઠ_Module* અચલ ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    પાછા ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_ગાંઠ_Module* modp, અચલ string& name) {
    AstTypedef* resultp = nullptr;
    અચલ StmtNameMap& cache = getOrBuild(modp);
    અચલ auto it = cache.m_byName.find(name);
    જો (!(it == cache.m_byName.end())) {
        માટે (Ast_ગાંઠ* nodep : it->second) {
            જો (AstTypedef* અચલ tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                તોડો;
            }
        }
    }
    પાછા resultp;
}
Ast_ગાંઠ_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_ગાંઠ_Module* modp, અચલ string& name,
                                                       VNType type) {

    Ast_ગાંઠ_DType* resultp = nullptr;
    અચલ StmtNameMap& cache = getOrBuild(modp);
    અચલ auto it = cache.m_byName.find(name);
    જો (!(it == cache.m_byName.end())) {
        માટે (Ast_ગાંઠ* nodep : it->second) {
            જો (Ast_ગાંઠ_DType* અચલ dtp = VN_CAST(nodep, ગાંઠ_DType)) {
                જો (dtp->type() == type) {
                    resultp = dtp;
                    તોડો;
                }
            }
        }
    }
    પાછા resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_ગાંઠ_Module* modp,
                                                                અચલ string& name) {

    AstParamTypeDType* resultp = nullptr;
    અચલ StmtNameMap& cache = getOrBuild(modp);
    અચલ auto it = cache.m_byName.find(name);
    જો (!(it == cache.m_byName.end())) {
        માટે (Ast_ગાંઠ* nodep : it->second) {
            જો (AstParamTypeDType* અચલ ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                તોડો;
            }
        }
    }
    પાછા resultp;
}

તાર્કિક V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_ગાંઠ_Module* targetModp) {
    અચલ VIfaceCaptureTag* અચલ tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    જો (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* અચલ paramTypep = findParamTypeInModule(targetModp, refp->name());
        જો (!paramTypep) પાછા ખોટું;
        retargetRefToParamType(refp, paramTypep);
        પાછા સાચું;
    }

    AstTypedef* અચલ typedefp = findTypedefInModule(targetModp, refp->name());
    જો (!typedefp) પાછા ખોટું;
    retargetRefToTypedef(refp, typedefp);
    પાછા સાચું;
}

શૂન્ય V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

શૂન્ય V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    જો (Ast_ગાંઠ_DType* અચલ dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

નામાકાશ {
using LiveNodes = std::unordered_set<અચલ Ast_ગાંઠ*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_ગાંઠ* nodep) { liveNodes.ઉમેરો(nodep); });
    પાછા liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_ગાંઠ_Module* findOwnerModuleImpl(Ast_ગાંઠ* nodep, અચલ LiveNodes* liveNodesp) {
    માટે (Ast_ગાંઠ* curp = nodep; curp; curp = curp->backp()) {
        જો (liveNodesp && !liveNodesp->ગણતરી(curp)) પાછા nullptr;
        જો (Ast_ગાંઠ_Module* અચલ modp = VN_CAST(curp, ગાંઠ_Module)) પાછા modp;
    }
    પાછા nullptr;
}

Ast_ગાંઠ_Module* findOwnerModuleIfLive(Ast_ગાંઠ* nodep, અચલ LiveNodes& liveNodes) {
    પાછા findOwnerModuleImpl(nodep, &liveNodes);
}

તાર્કિક moduleMatchesOwner(અચલ Ast_ગાંઠ_Module* modp, અચલ string& ownerName) {
    જો (!modp || ownerName.empty()) પાછા ખોટું;
    પાછા modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_ગાંઠ_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_ગાંઠ* nodep) {
    પાછા findOwnerModuleImpl(nodep, nullptr);
}

શૂન્ય V3LinkDotIfaceCapture::dumpEntries(અચલ string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    પૂર્ણાંક idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        અચલ VIfaceCaptureTag* અચલ tagp = captureTag(refp);
        જો (!tagp) પાછા;
        અચલ Ast_ગાંઠ_Module* અચલ ownerModp = findOwnerModule(refp);
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

શૂન્ય V3LinkDotIfaceCapture::tag(AstRefDType* refp, અચલ Ast_ગાંઠ_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, અચલ string& cellPath,
                                અચલ string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

અચલ VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    અચલ VIfaceCaptureTag* અચલ tagp = refp->captureTagp();
    જો (!tagp) પાછા nullptr;
    અચલ Ast_ગાંઠ_Module* અચલ ownerp = findOwnerModule(refp);
    જો (!ownerp || ownerp->origName() != tagp->m_capturedInName) પાછા nullptr;
    પાછા tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_ગાંઠ_Module* V3LinkDotIfaceCapture::followCellPath(Ast_ગાંઠ_Module* startModp,
                                                     અચલ string& cellPath) {
    જો (cellPath.empty() || !startModp) પાછા nullptr;
    Ast_ગાંઠ_Module* curModp = startModp;
    string remaining = cellPath;
    જ્યારે (!remaining.empty() && curModp) {
        string component;
        અચલ size_t dotPos = remaining.find('.');
        જો (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } નહીંતર {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        અચલ string componentBase = Ast_ગાંઠ::nameNoArray(component);
        Ast_ગાંઠ_Module* nextModp = nullptr;
        માટે (Ast_ગાંઠ* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            જો (AstCell* અચલ cellp = VN_CAST(sp, Cell)) {
                જો ((cellp->name() == component
                     || Ast_ગાંઠ::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    તોડો;
                }
            }
            જો (AstVar* અચલ varp = VN_CAST(sp, Var)) {
                જો (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    અચલ size_t viftopPos = varBaseName.find("__Viftop");
                    જો (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    જો (varBaseName == component
                        || Ast_ગાંઠ::nameNoArray(varBaseName) == componentBase) {
                        જો (AstIfaceRefDType* અચલ irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* અચલ ifacep = irefp->ifaceViaCellp();
                            જો (ifacep) {
                                nextModp = ifacep;
                                તોડો;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    પાછા curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
શૂન્ય V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, અચલ અક્ષર* stageLabel,
                                                  પૂર્ણાંક dotPos, અચલ std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_ગાંઠ_Module* modp,
                                                  અચલ std::function<std::string()>& indentFn) {
    જો (!enabled() || !refp) પાછા;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    અચલ AstCell* ifaceCellp = nullptr;
    જો (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        અચલ AstCell* અચલ cellp = VN_AS(dotSymp->nodep(), Cell);
        જો (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    જો (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        પાછા;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    જો (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        પાછા;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    અચલ string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    જો (AstParamTypeDType* અચલ paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } નહીંતર {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

શૂન્ય V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, અચલ string& cellPath,
                                         Ast_ગાંઠ_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         અચલ string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    અચલ string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    જો (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

પૂર્ણાંક V3LinkDotIfaceCapture::resolveCapturedRefs() {
    પૂર્ણાંક fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        અચલ VIfaceCaptureTag* અચલ tagp = captureTag(refp);
        જો (!tagp) પાછા;
        Ast_ગાંઠ_Module* અચલ ownerModp = findOwnerModule(refp);
        જો (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) પાછા;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_ગાંઠ_Module* correctModp = nullptr;
        જો (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } નહીંતર {
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
        refp->user3(સાચું);
        ++fixed;
    });

    પાછા fixed;
}

શૂન્ય V3LinkDotIfaceCapture::verifyNoDeadRefs(અચલ LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    માટે (Ast_ગાંઠ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        જો (Ast_ગાંઠ_Module* અચલ modp = VN_CAST(nodep, ગાંઠ_Module)) {
            જો (modp->dead()) ચાલુ;
            modp->foreach([&](AstRefDType* refp) {
                જો (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.ગણતરી(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_ગાંઠ_Module* અચલ ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                જો (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.ગણતરી(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_ગાંઠ_Module* અચલ ownerModp
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
    જો (v3Global.rootp()->typeTablep()) {
        માટે (Ast_ગાંઠ* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                જો (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.ગણતરી(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_ગાંઠ_Module* અચલ ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                જો (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.ગણતરી(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_ગાંઠ_Module* અચલ ownerModp
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

શૂન્ય V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    જો (!s_enabled) પાછા;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    જો (!v3Global.rootp()) પાછા;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    અચલ પૂર્ણાંક capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    જો (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    જો (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
