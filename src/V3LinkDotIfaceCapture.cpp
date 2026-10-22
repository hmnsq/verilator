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

#അന്തര്ഭാവ "V3LinkDotIfaceCapture.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3SymTable.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
തർക്കം V3LinkDotIfaceCapture::s_enabled = സത്യം;

// LCOV_EXCL_START
ശൂന്യം V3LinkDotIfaceCapture::enable(തർക്കം flag) {
    s_enabled = flag;
    എങ്കിൽ (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

ശൂന്യം V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
നാമാകാശ {
ഘടന StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_കണ്ണി*>> m_byName;
};
std::unordered_map<Ast_കണ്ണി_Module*, StmtNameMap> s_moduleCache;

സ്ഥിരം StmtNameMap& getOrBuild(Ast_കണ്ണി_Module* modp) {
    auto it = s_moduleCache.find(modp);
    എങ്കിൽ (it != s_moduleCache.end()) തിരികെ it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        സ്ഥിരം string& nm = stmtp->name();
        എങ്കിൽ (!nm.empty()) cache.m_byName[nm].തള്ളുക_back(stmtp);
    }
    തിരികെ cache;
}
}  // namespace

ശൂന്യം V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_കണ്ണി_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    വേണ്ടി (Ast_കണ്ണി_DType* curp = dtypep; curp;) {
        എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            നിർത്തുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstBracketArrayDType* സ്ഥിരം bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } അല്ലെങ്കിൽ {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    തിരികെ resultp;
}

നാമാകാശ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(സ്ഥിരം string& hint, Ast_കണ്ണി* nodep) {
    എങ്കിൽ (!hint.empty()) തിരികെ hint;
    എങ്കിൽ (!nodep) തിരികെ "";
    Ast_കണ്ണി_Module* സ്ഥിരം ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    തിരികെ ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_കണ്ണി_Module* modp, സ്ഥിരം string& name) {
    AstTypedef* resultp = nullptr;
    സ്ഥിരം StmtNameMap& cache = getOrBuild(modp);
    സ്ഥിരം auto it = cache.m_byName.find(name);
    എങ്കിൽ (!(it == cache.m_byName.end())) {
        വേണ്ടി (Ast_കണ്ണി* nodep : it->second) {
            എങ്കിൽ (AstTypedef* സ്ഥിരം tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                നിർത്തുക;
            }
        }
    }
    തിരികെ resultp;
}
Ast_കണ്ണി_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_കണ്ണി_Module* modp, സ്ഥിരം string& name,
                                                       VNType type) {

    Ast_കണ്ണി_DType* resultp = nullptr;
    സ്ഥിരം StmtNameMap& cache = getOrBuild(modp);
    സ്ഥിരം auto it = cache.m_byName.find(name);
    എങ്കിൽ (!(it == cache.m_byName.end())) {
        വേണ്ടി (Ast_കണ്ണി* nodep : it->second) {
            എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtp = VN_CAST(nodep, കണ്ണി_DType)) {
                എങ്കിൽ (dtp->type() == type) {
                    resultp = dtp;
                    നിർത്തുക;
                }
            }
        }
    }
    തിരികെ resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_കണ്ണി_Module* modp,
                                                                സ്ഥിരം string& name) {

    AstParamTypeDType* resultp = nullptr;
    സ്ഥിരം StmtNameMap& cache = getOrBuild(modp);
    സ്ഥിരം auto it = cache.m_byName.find(name);
    എങ്കിൽ (!(it == cache.m_byName.end())) {
        വേണ്ടി (Ast_കണ്ണി* nodep : it->second) {
            എങ്കിൽ (AstParamTypeDType* സ്ഥിരം ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                നിർത്തുക;
            }
        }
    }
    തിരികെ resultp;
}

തർക്കം V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_കണ്ണി_Module* targetModp) {
    സ്ഥിരം VIfaceCaptureTag* സ്ഥിരം tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    എങ്കിൽ (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* സ്ഥിരം paramTypep = findParamTypeInModule(targetModp, refp->name());
        എങ്കിൽ (!paramTypep) തിരികെ അസത്യം;
        retargetRefToParamType(refp, paramTypep);
        തിരികെ സത്യം;
    }

    AstTypedef* സ്ഥിരം typedefp = findTypedefInModule(targetModp, refp->name());
    എങ്കിൽ (!typedefp) തിരികെ അസത്യം;
    retargetRefToTypedef(refp, typedefp);
    തിരികെ സത്യം;
}

ശൂന്യം V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

ശൂന്യം V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

നാമാകാശ {
using LiveNodes = std::unordered_set<സ്ഥിരം Ast_കണ്ണി*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_കണ്ണി* nodep) { liveNodes.തിരുകുക(nodep); });
    തിരികെ liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_കണ്ണി_Module* findOwnerModuleImpl(Ast_കണ്ണി* nodep, സ്ഥിരം LiveNodes* liveNodesp) {
    വേണ്ടി (Ast_കണ്ണി* curp = nodep; curp; curp = curp->backp()) {
        എങ്കിൽ (liveNodesp && !liveNodesp->എണ്ണം(curp)) തിരികെ nullptr;
        എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(curp, കണ്ണി_Module)) തിരികെ modp;
    }
    തിരികെ nullptr;
}

Ast_കണ്ണി_Module* findOwnerModuleIfLive(Ast_കണ്ണി* nodep, സ്ഥിരം LiveNodes& liveNodes) {
    തിരികെ findOwnerModuleImpl(nodep, &liveNodes);
}

തർക്കം moduleMatchesOwner(സ്ഥിരം Ast_കണ്ണി_Module* modp, സ്ഥിരം string& ownerName) {
    എങ്കിൽ (!modp || ownerName.empty()) തിരികെ അസത്യം;
    തിരികെ modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_കണ്ണി_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_കണ്ണി* nodep) {
    തിരികെ findOwnerModuleImpl(nodep, nullptr);
}

ശൂന്യം V3LinkDotIfaceCapture::dumpEntries(സ്ഥിരം string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    പൂർണ്ണസംഖ്യ idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        സ്ഥിരം VIfaceCaptureTag* സ്ഥിരം tagp = captureTag(refp);
        എങ്കിൽ (!tagp) തിരികെ;
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം ownerModp = findOwnerModule(refp);
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

ശൂന്യം V3LinkDotIfaceCapture::tag(AstRefDType* refp, സ്ഥിരം Ast_കണ്ണി_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, സ്ഥിരം string& cellPath,
                                സ്ഥിരം string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

സ്ഥിരം VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    സ്ഥിരം VIfaceCaptureTag* സ്ഥിരം tagp = refp->captureTagp();
    എങ്കിൽ (!tagp) തിരികെ nullptr;
    സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം ownerp = findOwnerModule(refp);
    എങ്കിൽ (!ownerp || ownerp->origName() != tagp->m_capturedInName) തിരികെ nullptr;
    തിരികെ tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_കണ്ണി_Module* V3LinkDotIfaceCapture::followCellPath(Ast_കണ്ണി_Module* startModp,
                                                     സ്ഥിരം string& cellPath) {
    എങ്കിൽ (cellPath.empty() || !startModp) തിരികെ nullptr;
    Ast_കണ്ണി_Module* curModp = startModp;
    string remaining = cellPath;
    ആകുമ്പോൾ (!remaining.empty() && curModp) {
        string component;
        സ്ഥിരം size_t dotPos = remaining.find('.');
        എങ്കിൽ (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } അല്ലെങ്കിൽ {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        സ്ഥിരം string componentBase = Ast_കണ്ണി::nameNoArray(component);
        Ast_കണ്ണി_Module* nextModp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            എങ്കിൽ (AstCell* സ്ഥിരം cellp = VN_CAST(sp, Cell)) {
                എങ്കിൽ ((cellp->name() == component
                     || Ast_കണ്ണി::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    നിർത്തുക;
                }
            }
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(sp, Var)) {
                എങ്കിൽ (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    സ്ഥിരം size_t viftopPos = varBaseName.find("__Viftop");
                    എങ്കിൽ (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    എങ്കിൽ (varBaseName == component
                        || Ast_കണ്ണി::nameNoArray(varBaseName) == componentBase) {
                        എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* സ്ഥിരം ifacep = irefp->ifaceViaCellp();
                            എങ്കിൽ (ifacep) {
                                nextModp = ifacep;
                                നിർത്തുക;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    തിരികെ curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
ശൂന്യം V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, സ്ഥിരം അക്ഷരം* stageLabel,
                                                  പൂർണ്ണസംഖ്യ dotPos, സ്ഥിരം std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_കണ്ണി_Module* modp,
                                                  സ്ഥിരം std::function<std::string()>& indentFn) {
    എങ്കിൽ (!enabled() || !refp) തിരികെ;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    സ്ഥിരം AstCell* ifaceCellp = nullptr;
    എങ്കിൽ (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        സ്ഥിരം AstCell* സ്ഥിരം cellp = VN_AS(dotSymp->nodep(), Cell);
        എങ്കിൽ (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    എങ്കിൽ (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        തിരികെ;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    എങ്കിൽ (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        തിരികെ;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    സ്ഥിരം string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    എങ്കിൽ (AstParamTypeDType* സ്ഥിരം paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } അല്ലെങ്കിൽ {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

ശൂന്യം V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, സ്ഥിരം string& cellPath,
                                         Ast_കണ്ണി_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         സ്ഥിരം string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    സ്ഥിരം string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    എങ്കിൽ (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

പൂർണ്ണസംഖ്യ V3LinkDotIfaceCapture::resolveCapturedRefs() {
    പൂർണ്ണസംഖ്യ fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        സ്ഥിരം VIfaceCaptureTag* സ്ഥിരം tagp = captureTag(refp);
        എങ്കിൽ (!tagp) തിരികെ;
        Ast_കണ്ണി_Module* സ്ഥിരം ownerModp = findOwnerModule(refp);
        എങ്കിൽ (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) തിരികെ;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_കണ്ണി_Module* correctModp = nullptr;
        എങ്കിൽ (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } അല്ലെങ്കിൽ {
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
        refp->user3(സത്യം);
        ++fixed;
    });

    തിരികെ fixed;
}

ശൂന്യം V3LinkDotIfaceCapture::verifyNoDeadRefs(സ്ഥിരം LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    വേണ്ടി (Ast_കണ്ണി* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(nodep, കണ്ണി_Module)) {
            എങ്കിൽ (modp->dead()) തുടരുക;
            modp->foreach([&](AstRefDType* refp) {
                എങ്കിൽ (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.എണ്ണം(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_കണ്ണി_Module* സ്ഥിരം ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                എങ്കിൽ (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.എണ്ണം(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_കണ്ണി_Module* സ്ഥിരം ownerModp
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
    എങ്കിൽ (v3Global.rootp()->typeTablep()) {
        വേണ്ടി (Ast_കണ്ണി* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                എങ്കിൽ (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.എണ്ണം(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_കണ്ണി_Module* സ്ഥിരം ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                എങ്കിൽ (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.എണ്ണം(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_കണ്ണി_Module* സ്ഥിരം ownerModp
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

ശൂന്യം V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    എങ്കിൽ (!s_enabled) തിരികെ;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    എങ്കിൽ (!v3Global.rootp()) തിരികെ;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    സ്ഥിരം പൂർണ്ണസംഖ്യ capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    എങ്കിൽ (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    എങ്കിൽ (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
