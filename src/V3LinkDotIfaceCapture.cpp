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

#अन्तर्भाव "V3LinkDotIfaceCapture.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3SymTable.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
तार्किक V3LinkDotIfaceCapture::s_enabled = सत्य;

// LCOV_EXCL_START
शून्य V3LinkDotIfaceCapture::enable(तार्किक flag) {
    s_enabled = flag;
    जर (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

शून्य V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
नामाकाश {
संरचना StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_गाठ*>> m_byName;
};
std::unordered_map<Ast_गाठ_Module*, StmtNameMap> s_moduleCache;

स्थिरांक StmtNameMap& getOrBuild(Ast_गाठ_Module* modp) {
    स्वतः it = s_moduleCache.find(modp);
    जर (it != s_moduleCache.end()) परत it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        स्थिरांक string& nm = stmtp->name();
        जर (!nm.empty()) cache.m_byName[nm].ढकला_back(stmtp);
    }
    परत cache;
}
}  // namespace

शून्य V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_गाठ_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    साठी (Ast_गाठ_DType* curp = dtypep; curp;) {
        जर (AstIfaceRefDType* स्थिरांक irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            थांबा;
        } अन्यथा जर (AstBracketArrayDType* स्थिरांक bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } अन्यथा जर (AstUnpackArrayDType* स्थिरांक unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } अन्यथा {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    परत resultp;
}

नामाकाश {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(स्थिरांक string& hint, Ast_गाठ* nodep) {
    जर (!hint.empty()) परत hint;
    जर (!nodep) परत "";
    Ast_गाठ_Module* स्थिरांक ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    परत ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_गाठ_Module* modp, स्थिरांक string& name) {
    AstTypedef* resultp = nullptr;
    स्थिरांक StmtNameMap& cache = getOrBuild(modp);
    स्थिरांक स्वतः it = cache.m_byName.find(name);
    जर (!(it == cache.m_byName.end())) {
        साठी (Ast_गाठ* nodep : it->second) {
            जर (AstTypedef* स्थिरांक tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                थांबा;
            }
        }
    }
    परत resultp;
}
Ast_गाठ_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_गाठ_Module* modp, स्थिरांक string& name,
                                                       VNType type) {

    Ast_गाठ_DType* resultp = nullptr;
    स्थिरांक StmtNameMap& cache = getOrBuild(modp);
    स्थिरांक स्वतः it = cache.m_byName.find(name);
    जर (!(it == cache.m_byName.end())) {
        साठी (Ast_गाठ* nodep : it->second) {
            जर (Ast_गाठ_DType* स्थिरांक dtp = VN_CAST(nodep, गाठ_DType)) {
                जर (dtp->type() == type) {
                    resultp = dtp;
                    थांबा;
                }
            }
        }
    }
    परत resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_गाठ_Module* modp,
                                                                स्थिरांक string& name) {

    AstParamTypeDType* resultp = nullptr;
    स्थिरांक StmtNameMap& cache = getOrBuild(modp);
    स्थिरांक स्वतः it = cache.m_byName.find(name);
    जर (!(it == cache.m_byName.end())) {
        साठी (Ast_गाठ* nodep : it->second) {
            जर (AstParamTypeDType* स्थिरांक ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                थांबा;
            }
        }
    }
    परत resultp;
}

तार्किक V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_गाठ_Module* targetModp) {
    स्थिरांक VIfaceCaptureTag* स्थिरांक tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    जर (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* स्थिरांक paramTypep = findParamTypeInModule(targetModp, refp->name());
        जर (!paramTypep) परत असत्य;
        retargetRefToParamType(refp, paramTypep);
        परत सत्य;
    }

    AstTypedef* स्थिरांक typedefp = findTypedefInModule(targetModp, refp->name());
    जर (!typedefp) परत असत्य;
    retargetRefToTypedef(refp, typedefp);
    परत सत्य;
}

शून्य V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

शून्य V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    जर (Ast_गाठ_DType* स्थिरांक dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

नामाकाश {
using LiveNodes = std::unordered_set<स्थिरांक Ast_गाठ*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_गाठ* nodep) { liveNodes.घाला(nodep); });
    परत liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_गाठ_Module* findOwnerModuleImpl(Ast_गाठ* nodep, स्थिरांक LiveNodes* liveNodesp) {
    साठी (Ast_गाठ* curp = nodep; curp; curp = curp->backp()) {
        जर (liveNodesp && !liveNodesp->मोजणी(curp)) परत nullptr;
        जर (Ast_गाठ_Module* स्थिरांक modp = VN_CAST(curp, गाठ_Module)) परत modp;
    }
    परत nullptr;
}

Ast_गाठ_Module* findOwnerModuleIfLive(Ast_गाठ* nodep, स्थिरांक LiveNodes& liveNodes) {
    परत findOwnerModuleImpl(nodep, &liveNodes);
}

तार्किक moduleMatchesOwner(स्थिरांक Ast_गाठ_Module* modp, स्थिरांक string& ownerName) {
    जर (!modp || ownerName.empty()) परत असत्य;
    परत modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_गाठ_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_गाठ* nodep) {
    परत findOwnerModuleImpl(nodep, nullptr);
}

शून्य V3LinkDotIfaceCapture::dumpEntries(स्थिरांक string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    पूर्णांक idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        स्थिरांक VIfaceCaptureTag* स्थिरांक tagp = captureTag(refp);
        जर (!tagp) परत;
        स्थिरांक Ast_गाठ_Module* स्थिरांक ownerModp = findOwnerModule(refp);
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

शून्य V3LinkDotIfaceCapture::tag(AstRefDType* refp, स्थिरांक Ast_गाठ_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, स्थिरांक string& cellPath,
                                स्थिरांक string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

स्थिरांक VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    स्थिरांक VIfaceCaptureTag* स्थिरांक tagp = refp->captureTagp();
    जर (!tagp) परत nullptr;
    स्थिरांक Ast_गाठ_Module* स्थिरांक ownerp = findOwnerModule(refp);
    जर (!ownerp || ownerp->origName() != tagp->m_capturedInName) परत nullptr;
    परत tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_गाठ_Module* V3LinkDotIfaceCapture::followCellPath(Ast_गाठ_Module* startModp,
                                                     स्थिरांक string& cellPath) {
    जर (cellPath.empty() || !startModp) परत nullptr;
    Ast_गाठ_Module* curModp = startModp;
    string remaining = cellPath;
    पर्यंत (!remaining.empty() && curModp) {
        string component;
        स्थिरांक माप_प्रकार dotPos = remaining.find('.');
        जर (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } अन्यथा {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        स्थिरांक string componentBase = Ast_गाठ::nameNoArray(component);
        Ast_गाठ_Module* nextModp = nullptr;
        साठी (Ast_गाठ* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            जर (AstCell* स्थिरांक cellp = VN_CAST(sp, Cell)) {
                जर ((cellp->name() == component
                     || Ast_गाठ::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    थांबा;
                }
            }
            जर (AstVar* स्थिरांक varp = VN_CAST(sp, Var)) {
                जर (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    स्थिरांक माप_प्रकार viftopPos = varBaseName.find("__Viftop");
                    जर (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    जर (varBaseName == component
                        || Ast_गाठ::nameNoArray(varBaseName) == componentBase) {
                        जर (AstIfaceRefDType* स्थिरांक irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* स्थिरांक ifacep = irefp->ifaceViaCellp();
                            जर (ifacep) {
                                nextModp = ifacep;
                                थांबा;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    परत curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
शून्य V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, स्थिरांक अक्षर* stageLabel,
                                                  पूर्णांक dotPos, स्थिरांक std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_गाठ_Module* modp,
                                                  स्थिरांक std::function<std::string()>& indentFn) {
    जर (!enabled() || !refp) परत;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    स्थिरांक AstCell* ifaceCellp = nullptr;
    जर (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        स्थिरांक AstCell* स्थिरांक cellp = VN_AS(dotSymp->nodep(), Cell);
        जर (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    जर (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        परत;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    जर (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        परत;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    स्थिरांक string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    जर (AstParamTypeDType* स्थिरांक paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
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

शून्य V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, स्थिरांक string& cellPath,
                                         Ast_गाठ_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         स्थिरांक string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    स्थिरांक string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    जर (debug())
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
    पूर्णांक fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        स्थिरांक VIfaceCaptureTag* स्थिरांक tagp = captureTag(refp);
        जर (!tagp) परत;
        Ast_गाठ_Module* स्थिरांक ownerModp = findOwnerModule(refp);
        जर (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) परत;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_गाठ_Module* correctModp = nullptr;
        जर (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
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
        refp->user3(सत्य);
        ++fixed;
    });

    परत fixed;
}

शून्य V3LinkDotIfaceCapture::verifyNoDeadRefs(स्थिरांक LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    साठी (Ast_गाठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        जर (Ast_गाठ_Module* स्थिरांक modp = VN_CAST(nodep, गाठ_Module)) {
            जर (modp->dead()) सुरूठेवा;
            modp->foreach([&](AstRefDType* refp) {
                जर (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.मोजणी(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_गाठ_Module* स्थिरांक ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                जर (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.मोजणी(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_गाठ_Module* स्थिरांक ownerModp
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
    जर (v3Global.rootp()->typeTablep()) {
        साठी (Ast_गाठ* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                जर (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.मोजणी(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_गाठ_Module* स्थिरांक ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                जर (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.मोजणी(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_गाठ_Module* स्थिरांक ownerModp
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

शून्य V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    जर (!s_enabled) परत;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    जर (!v3Global.rootp()) परत;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    स्थिरांक पूर्णांक capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    जर (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    जर (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
