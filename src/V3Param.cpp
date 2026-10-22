// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Replicate modules for parameterization
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
// PARAM TRANSFORMATIONS:
//   Top down traversal:
//      For each cell:
//          If parameterized,
//              Determine all parameter widths, constant values.
//              (Interfaces also matter, as if a module is parameterized
//              this effectively changes the width behavior of all that
//              reference the iface.)
//
//              Clone module cell calls, renaming with __{par1}_{par2}_...
//              Substitute constants for cell's module's parameters.
//              Relink pins and cell and ifacerefdtype to point to new module.
//
//              For interface Parent's we have the AstIfaceRefDType::cellp()
//              pointing to this module.  If that parent cell's interface
//              module gets parameterized, AstIfaceRefDType::cloneRelink
//              will update AstIfaceRefDType::cellp(), and V3LinkDot will
//              see the new interface.
//
//              However if a submodule's AstIfaceRefDType::ifacep() points
//              to the old (unparameterized) interface and needs correction.
//              To detect this we must walk all pins looking for interfaces
//              that the parent has changed and propagate down.
//
//          Then process all modules called by that cell.
//          (Cells never referenced after parameters expanded must be ignored.)
//
//      Arrayed instances of modules and interfaces are expanded into their
//      elements before the above, each element knowing its position in the
//      array (for V3Width to select its part of the port connections).
//      Interface array ports are split into a variable for each element.
//
//   After we complete parameters, the varp's will be wrong (point to old module)
//   and must be relinked.
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Param.h"

#અન્તર્ભાવ "V3Case.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3HierBlock.h"
#અન્તર્ભાવ "V3LinkDotIfaceCapture.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3Parse.h"
#અન્તર્ભાવ "V3Simulate.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3Unroll.h"
#અન્તર્ભાવ "V3Width.h"

#અન્તર્ભાવ <cctype>
#અન્તર્ભાવ <cstdlib>
#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
સ્થિર AstClassRefDType* class_Ref_DType_Of_ગાંઠ(Ast_ગાંઠ* nodep) {
    જ્યારે (AstTypedef* અચલ tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ગાંઠ_DType* અચલ dtp = VN_CAST(nodep, ગાંઠ_DType);
    પાછા dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

વર્ગ ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, અચલ V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<અચલ std::string, Ast_ગાંઠ_Module*>;
    using ParamConstMap = std::map<અચલ std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<અચલ std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    અચલ તાર્કિક m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<અચલ V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<અચલ std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<અચલ AstVar*> m_untypedParams;

    // METHODS

જાહેર:
    ParameterizedHierBlocks(અચલ V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        માટે (અચલ auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            અચલ V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            માટે (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    નવું File_લીટી{File_લીટી::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                અચલ તાર્કિક inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ઉમેરો({hierOpt.second.origName(), {}});
        }
        માટે (Ast_ગાંઠ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
            જો (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            અચલ string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            અચલ auto defParamIt = m_modParams.find(actualModName);
            જો (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                        જો (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        અચલ AstBasicDType* અચલ dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        જો (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    તાર્કિક hierSubRun() અચલ { પાછા m_hierSubRun; }
    તાર્કિક isHierBlock(અચલ string& origName) અચલ {
        પાછા m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_ગાંઠ_Module* findByParams(અચલ string& origName, AstPin* firstPinp,
                                અચલ Ast_ગાંઠ_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        અચલ std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        અચલ auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        અચલ AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        માટે (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            તાર્કિક found = સાચું;
            size_t paramIdx = 0;
            અચલ ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            માટે (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (!pinp->exprp()) ચાલુ;
                જો (અચલ AstVar* અચલ modvarp = pinp->modVarp()) {
                    AstConst* અચલ constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    અચલ auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    અચલ તાર્કિક untyped = m_untypedParams.ગણતરી(paramIt->second);
                    AstConst* અચલ defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    જો (defValuep && areSame(constp, defValuep)) {
                        જો (!untyped || constp->same_કિંમત_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            ચાલુ;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    અચલ auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    જો (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = ખોટું;
                        તોડો;
                    }
                    જો (untyped && !constp->same_કિંમત_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = ખોટું;
                        તોડો;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            જો (found && paramIdx == hierIt->second->params().size()) તોડો;
        }
        જો (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ગાંઠ::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            પાછા nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        અચલ HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        અચલ auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        જો (it == m_hierBlockMod.end()) પાછા nullptr;
        પાછા it->second;
    }
    સ્થિર તાર્કિક areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        જો (pinValuep->isString()) {
            પાછા pinValuep->અંક().toString() == hierOptParamp->અંક().toString();
        }

        જો (hierOptParamp->isDouble()) {
            દ્વિગુણ var;
            જો (pinValuep->isDouble()) {
                var = pinValuep->અંક().toDouble();
            } નહીંતર {  // Cast from integer to real
                V3_સંખ્યા var_અંક{pinValuep, V3_સંખ્યા::Double{}, 0.0};
                var_અંક.opIToRD(pinValuep->અંક());
                var = var_અંક.toDouble();
            }
            પાછા V3_સંખ્યા::epsilonEqual(var, hierOptParamp->અંક().toDouble());
        } નહીંતર {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_સંખ્યા var_અંક{pinValuep, hierOptParamp->અંક().width()};
            જો (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_અંક.opRToIRoundS(pinValuep->અંક());
            } નહીંતર જો (pinValuep->isSigned()) {
                var_અંક.opExtendS(pinValuep->અંક(), pinValuep->અંક().width());
            } નહીંતર {
                var_અંક.opAssign(pinValuep->અંક());
            }
            V3_સંખ્યા isEq{pinValuep, 1};
            isEq.opEq(var_અંક, hierOptParamp->અંક());
            પાછા isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

વર્ગ ParamProcessor final {
    // NODE STATE - Local
    //   AstVar::user3()        // int    Global parameter number (for naming new module)
    //                          //        (0=not processed, 1=iterated, but no number,
    //                          //         65+ parameter numbered)
    // NODE STATE - Shared with ParamVisitor
    //   AstNodeModule::user3p()  // AstNodeModule* Unaltered copy of the parameterized module.
    //                               The module/class node may be modified according to parameter
    //                               values and an unchanged copy is needed to instantiate
    //                               modules/classes with different parameters.
    //   AstNodeModule::user2() // bool   True if processed
    //   AstGenFor::user2()     // bool   True if processed
    //   AstVar::user2()        // bool   True if constant propagated
    //   AstIfaceRefDType::user2()      // bool   True if visited
    //   AstClassRefDType::user2()      // bool   True if visited
    //   AstNodeModule:user4p() // AstNodeModule* Parametrized copy with default parameters
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser3InUse m_inuser3;
    અચલ VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<અચલ Ast_ગાંઠ*, Ast_ગાંઠ*>;
    સંરચના ModInfo final {
        Ast_ગાંઠ_Module* અચલ m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_ગાંઠ_Module* modp)
            : m_modp{modp} {}
    };
    std::map<અચલ std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<અચલ std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<અચલ std::string, std::string> m_કિંમત_Names;  // Parameter value text to its name
    std::set<std::string> m_કિંમત_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    સ્થિર constexpr std::string::size_type DIGEST_DIGITS = 8;

    અચલ Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_કિંમત_Map = std::map<std::string, Ast_ગાંઠ*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ગાંઠ_Module*, Default_કિંમત_Map> m_default_Parameter_કિંમતો;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<અચલ Ast_ગાંઠ_Module*, અચલ AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, પૂર્ણાંક>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, પૂર્ણાંક> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<અચલ AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    સ્થિર શૂન્ય makeSmallNames(Ast_ગાંઠ_Module* modp) {
        std::vector<પૂર્ણાંક> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->isGParam() || varp->isIfaceRef()) {
                    અક્ષર ch = varp->name()[0];
                    ch = std::toupper(ch);
                    જો (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<પૂર્ણાંક>(ch)] * 256 + ch);
                    usedLetter[static_cast<પૂર્ણાંક>(ch)]++;
                }
            } નહીંતર જો (AstParamTypeDType* અચલ typep = VN_CAST(stmtp, ParamTypeDType)) {
                અચલ અક્ષર ch = 'T';
                typep->user3(usedLetter[static_cast<પૂર્ણાંક>(ch)] * 256 + ch);
                usedLetter[static_cast<પૂર્ણાંક>(ch)]++;
            }
        }
    }
    સ્થિર string paramSmallName(Ast_ગાંઠ_Module* modp, Ast_ગાંઠ* varp) {
        જો (varp->user3() <= 1) makeSmallNames(modp);
        પૂર્ણાંક index = varp->user3() / 256;
        અચલ અક્ષર ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        જ્યારે (index) {
            st += cvtToStr(static_cast<અક્ષર>((index % 25) + 'A'));
            index /= 26;
        }
        પાછા st;
    }

    સ્થિર તાર્કિક is_Aggregate_Param_કિંમત(અચલ Ast_ગાંઠ* nodep) {
        પાછા VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    સ્થિર string param_કિંમત_String(અચલ Ast_ગાંઠ* nodep) {
        જો (અચલ AstRefDType* અચલ refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string ચાવી = nodep->name();
        જો (અચલ AstIfaceRefDType* અચલ ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            જો (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                ચાવી = ifrtp->cellp()->modp()->name();
            } નહીંતર જો (ifrtp->ifacep()) {
                ચાવી = ifrtp->ifacep()->name();
            } નહીંતર {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } નહીંતર જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ dtypep
                   = VN_CAST(nodep, ગાંઠ_UOr_Struct_DType)) {
            ચાવી += " ";
            ચાવી += dtypep->verilogKwd();
            ચાવી += " {";
            માટે (અચલ Ast_ગાંઠ* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                ચાવી += param_કિંમત_String(memberp);
                ચાવી += ";";
            }
            ચાવી += "}";
        } નહીંતર જો (અચલ AstMemberDType* અચલ dtypep = VN_CAST(nodep, MemberDType)) {
            ચાવી += " ";
            ચાવી += param_કિંમત_String(dtypep->subDTypep());
        } નહીંતર જો (અચલ AstBasicDType* અચલ dtypep = VN_CAST(nodep, BasicDType)) {
            જો (dtypep->isSigned()) ચાવી += " signed";
            જો (dtypep->isRanged()) {
                ચાવી += "[" + cvtToStr(dtypep->ડાબું()) + ":" + cvtToStr(dtypep->જમણું()) + "]";
            }
        } નહીંતર જો (અચલ AstPackArrayDType* અચલ dtypep = VN_CAST(nodep, PackArrayDType)) {
            ચાવી += "[";
            ચાવી += cvtToStr(dtypep->ડાબું());
            ચાવી += ":";
            ચાવી += cvtToStr(dtypep->જમણું());
            ચાવી += "] ";
            ચાવી += param_કિંમત_String(dtypep->subDTypep());
        } નહીંતર જો (અચલ AstPattern* અચલ patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            ચાવી += "'{";
            માટે (અચલ AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                જો (memp->isDefault()) ચાવી += "default";
                જો (અચલ AstText* અચલ textp = VN_CAST(memp->keyp(), Text)) {
                    ચાવી += textp->text();
                } નહીંતર જો (memp->keyp()) {
                    ચાવી += param_કિંમત_String(memp->keyp());
                }
                ચાવી += ":";
                જો (memp->repp()) ચાવી += param_કિંમત_String(memp->repp()) + "x";
                માટે (અચલ Ast_ગાંઠ_પદાવલિ* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ગાંઠ_પદાવલિ)) {
                    ચાવી += param_કિંમત_String(valuep) + ";";
                }
                ચાવી += ",";
            }
            ચાવી += "}";
        } નહીંતર જો (અચલ AstConsArray* અચલ consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            ચાવી += "{";
            માટે (અચલ AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                ચાવી += cvtToStr(itemp->index()) + ":" + param_કિંમત_String(itemp->valuep()) + ",";
            }
            જો (consArrayp->defaultp()) {
                ચાવી += "default:" + param_કિંમત_String(consArrayp->defaultp()) + ",";
            }
            ચાવી += "}";
        } નહીંતર જો (અચલ AstConsStruct* અચલ consp = VN_CAST(nodep, ConsStruct)) {
            ચાવી += "{";
            માટે (અચલ AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                ચાવી += param_કિંમત_String(itemp->valuep());
                ચાવી += ",";
            }
            ચાવી += "}";
        } નહીંતર જો (અચલ AstClassRefDType* અચલ classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            જો (classRefp->classp()) {
                અચલ string& className = classRefp->classp()->name();
                અચલ string& origName = classRefp->classp()->origName();
                અચલ તાર્કિક isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                જો (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } નહીંતર જો (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    ચાવી = className;
                } નહીંતર {
                    // Unspecialized class with no params - use origName
                    ચાવી = origName;
                }
            } નહીંતર {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            ચાવી = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } નહીંતર જો (અચલ AstQueueDType* અચલ dtypep = VN_CAST(nodep, QueueDType)) {
            ચાવી = paramElemString(dtypep->subDTypep()) + "$[$";
            જો (dtypep->boundConst()) ચાવી += ":" + cvtToStr(dtypep->boundConst());
            ચાવી += "]";
        } નહીંતર જો (અચલ AstDynArrayDType* અચલ dtypep = VN_CAST(nodep, DynArrayDType)) {
            ચાવી = paramElemString(dtypep->subDTypep()) + "$[]";
        } નહીંતર જો (અચલ AstAssocArrayDType* અચલ dtypep = VN_CAST(nodep, AssocArrayDType)) {
            ચાવી = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->ચાવી_DTypep()) + "]";
        } નહીંતર જો (અચલ AstWildcardArrayDType* અચલ dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            ચાવી = paramElemString(dtypep->subDTypep()) + "$[*]";
        } નહીંતર જો (અચલ AstEnumDType* અચલ dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            ચાવી += " enum {";
            માટે (અચલ AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                અચલ AstConst* અચલ constp = VN_CAST(itemp->valuep(), Const);
                ચાવી += itemp->name() + "=" + (constp ? constp->અંક().ascii() : "?") + ";";
            }
            ચાવી += "}";
        } નહીંતર જો (અચલ Ast_ગાંઠ_DType* અચલ dtypep = VN_CAST(nodep, ગાંઠ_DType)) {
            ચાવી += dtypep->prettyDTypeName(સાચું);
        }
        UASSERT_OBJ(!ચાવી.empty(), nodep, "Parameter yielded no value string");
        પાછા ચાવી;
    }
    // As paramValueString(), for the element or key type of a container
    સ્થિર string paramElemString(અચલ Ast_ગાંઠ_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        અચલ AstClassRefDType* અચલ classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        જો (classRefp && classRefp->paramsp()) પાછા classRefp->prettyDTypeName(સાચું);
        પાછા param_કિંમત_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    સ્થિર string digestSuffix(અચલ string& text, std::set<string>& usedr) {
        અચલ string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        જ્યારે (digits < hex.size() && !usedr.ઉમેરો(hex.substr(0, digits)).second) ++digits;
        પાછા hex.substr(0, digits);
    }
    string param_કિંમત_સંખ્યા(Ast_ગાંઠ* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        જો (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        અચલ string text = param_કિંમત_String(nodep);
        અચલ auto pair = m_કિંમત_Names.emplace(text, "");
        જો (pair.second) pair.first->second = "z" + digestSuffix(text, m_કિંમત_Suffixes);
        પાછા pair.first->second;
    }
    string moduleCalcName(અચલ Ast_ગાંઠ_Module* srcModp, અચલ string& longname) {
        string newname = longname;
        જો (longname.length() > 30) {
            અચલ auto pair = m_longMap.emplace(longname, "");
            જો (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        પાછા newname;
    }
    સ્થિર Ast_ગાંઠ_DType* arraySubDTypep(Ast_ગાંઠ_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        જો (અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            પાછા adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        જો (અચલ AstBracketArrayDType* અચલ adtypep = VN_CAST(nodep, BracketArrayDType)) {
            પાછા adtypep->subDTypep();
        }
        પાછા nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    સ્થિર Ast_ગાંઠ_DType* arraySubDTypeDeepp(Ast_ગાંઠ_DType* nodep) {
        જ્યારે (Ast_ગાંઠ_DType* અચલ subp = arraySubDTypep(nodep)) nodep = subp;
        પાછા nodep;
    }
    સ્થિર તાર્કિક isString(Ast_ગાંઠ_DType* nodep) {
        જો (AstBasicDType* અચલ basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            પાછા basicp->isString();
        પાછા ખોટું;
    }
    શૂન્ય collectPins(CloneMap* clonemapp, Ast_ગાંઠ_Module* modp, તાર્કિક originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            અચલ Ast_ગાંઠ* originalParamp = nullptr;
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } નહીંતર જો (AstParamTypeDType* અચલ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                જો (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            જો (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            જો (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    શૂન્ય relinkPins(અચલ CloneMap* clonemapp, AstPin* startpinp) {
        માટે (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                અચલ auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } નહીંતર જો (pinp->modPTypep()) {
                અચલ auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } નહીંતર {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    શૂન્ય relinkPinsByName(AstPin* startpinp, Ast_ગાંઠ_Module* modp) {
        std::map<અચલ string, AstVar*> nameToPin;
        માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        માટે (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (અચલ AstVar* અચલ varp = pinp->modVarp()) {
                અચલ auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    સ્થિર તાર્કિક hasDescendantDefparams(અચલ Ast_ગાંઠ_Module* modp,
                                       std::set<અચલ Ast_ગાંઠ_Module*>& visited) {
        જો (!visited.ઉમેરો(modp).second) પાછા ખોટું;
        માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* અચલ cellp = VN_CAST(stmtp, Cell);
            જો (!cellp) ચાલુ;
            માટે (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    પાછા સાચું;
                }
            }
            જો (Ast_ગાંઠ_Module* અચલ childModp = cellp->modp()) {
                જો (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    પાછા સાચું;
                }
            }
        }
        visited.erase(modp);
        પાછા ખોટું;
    }
    સ્થિર તાર્કિક hasDescendantDefparams(અચલ Ast_ગાંઠ_Module* modp) {
        std::set<અચલ Ast_ગાંઠ_Module*> visited;
        પાછા hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    અચલ AstPin* nestedDefparamp(અચલ Ast_ગાંઠ_Module* modp) {
        અચલ auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        જો (!pair.second) પાછા pair.first->second;  // Searched, or being searched
        અચલ AstPin* foundp = nullptr;
        modp->exists([&](અચલ AstCell* cellp) {
            માટે (અચલ AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (!pinp->paramPath().empty()) foundp = pinp;
            }
            જો (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            પાછા foundp != nullptr;
        });
        pair.first->second = foundp;
        પાછા foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    શૂન્ય checkSupportedParam(Ast_ગાંઠ_Module* modp, AstPin* pinp) અચલ {
        // ConsArray is not supported because that can not be set via -G
        // option.
        જો (pinp->modVarp()) {
            તાર્કિક supported = ખોટું;
            જો (અચલ AstConst* અચલ constp = VN_CAST(pinp->exprp(), Const)) {
                supported = સાચું;
                જો (constp->અંક().isString()
                    && !V3HierBlock::stringParamPassable(constp->અંક().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            જો (!supported) {
                pinp->v3error(
                    Ast_ગાંઠ::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } નહીંતર જો (અચલ AstParamTypeDType* અચલ typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ગાંઠ::prettyNameQ(modp->origName()));
        }
    }
    તાર્કિક moduleExists(અચલ string& modName) અચલ {
        જો (m_allModuleNames.find(modName) != m_allModuleNames.end()) પાછા સાચું;
        જો (m_modNameMap.find(modName) != m_modNameMap.end()) પાછા સાચું;
        પાછા ખોટું;
    }

    string parameterizedHierBlockName(Ast_ગાંઠ_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ગાંઠ*> pins;

        AstPin* pinp = paramPinsp;
        જ્યારે (pinp) {
            checkSupportedParam(modp, pinp);
            જો (અચલ AstVar* અચલ varp = pinp->modVarp()) {
                જો (!pinp->exprp()) ચાલુ;
                જો (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } નહીંતર જો (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ગાંઠ_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        અચલ auto pair = m_default_Parameter_કિંમતો.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        જો (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            જો (અચલ AstPin* અચલ defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ગાંઠ::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_કિંમત_Map params;
            માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                    જો (varp->isGParam()) {
                        AstConst* અચલ constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } નહીંતર જો (AstParamTypeDType* અચલ p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ગાંઠ* અચલ dtypep = static_cast<Ast_ગાંઠ*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        અચલ auto paramsIt = pair.first;
        જો (paramsIt->second.empty()) પાછા modp->origName();  // modp has no parameter

        string longname = modp->origName();
        માટે (auto&& default_કિંમત : paramsIt->second) {
            અચલ auto pinIt = pins.find(default_કિંમત.first);
            // If the pin does not have a value assigned, use the default one.
            અચલ Ast_ગાંઠ* અચલ nodep = pinIt == pins.end() ? default_કિંમત.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_કિંમત.first + "=";
            // constp can be nullptr

            જો (અચલ AstConst* અચલ p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->અંક().ascii(ખોટું);
            } નહીંતર જો (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_વૃક્ષ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        અચલ auto iter = m_longMap.find(longname);
        જો (iter != m_longMap.end()) પાછા iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ઉમેરો(longname);
        જ્યારે (સાચું) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            અચલ string hashStr = hashStrGen.digestHex();
            માટે (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                જો (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                જો (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    પાછા newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ઉમેરો(V3Os::trueRandom(64));
        }
    }
    શૂન્ય replaceRefsRecurse(Ast_ગાંઠ* અચલ nodep, અચલ AstClass* અચલ oldClassp,
                            AstClass* અચલ newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        જો (AstClassRefDType* અચલ classRefp = VN_CAST(nodep, ClassRefDType)) {
            જો (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } નહીંતર જો (AstClassOrPackageRef* અચલ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            જો (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } નહીંતર જો (AstTypedef* અચલ typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            જો (typedefp->subDTypep()) {
                જો (AstClassRefDType* અચલ classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    જો (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } નહીંતર જો (Ast_ગાંઠ_FTask_Ref* અચલ ftaskRefp = VN_CAST(nodep, ગાંઠ_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            જો (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            જો (Ast_ગાંઠ_FTask* અચલ oldTaskp = ftaskRefp->taskp()) {
                જો (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        જો (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        જો (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        જો (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        જો (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        જો (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    વર્ગ VarXRefRelinkVisitor final : જાહેર VNVisitor {
        Ast_ગાંઠ_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ગાંઠ_Module*> m_varModuleMap;  // Cache var->module lookups

    જાહેર:
        explicit VarXRefRelinkVisitor(Ast_ગાંઠ_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    ખાનગી:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ગાંઠ_Module* findVarModule(AstVar* varp) {
            અચલ auto it = m_varModuleMap.find(varp);
            જો (it != m_varModuleMap.end()) પાછા it->second;
            Ast_ગાંઠ_Module* varModp = nullptr;
            માટે (Ast_ગાંઠ* np = varp; np; np = np->backp()) {
                જો (Ast_ગાંઠ_Module* અચલ modp = VN_CAST(np, ગાંઠ_Module)) {
                    varModp = modp;
                    તોડો;
                }
            }
            m_varModuleMap[varp] = varModp;
            પાછા varModp;
        }

        શૂન્ય visit(AstVarXRef* nodep) override {
            AstVar* અચલ varp = nodep->varp();
            જો (!varp) {
                iterateChildren(nodep);
                પાછા;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            અચલ string& dotted = nodep->dotted();
            જો (dotted.empty()) {
                iterateChildren(nodep);
                પાછા;
            }

            અચલ size_t dotPos = dotted.find('.');
            અચલ string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            જો (portName.empty()) {
                iterateChildren(nodep);
                પાછા;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            માટે (Ast_ગાંઠ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (AstVar* અચલ varChkp = VN_CAST(stmtp, Var)) {
                    જો (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        તોડો;
                    }
                }
            }
            જો (!portVarp) {
                iterateChildren(nodep);
                પાછા;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* અચલ irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            જો (!irefp) {
                iterateChildren(nodep);
                પાછા;
            }

            Ast_ગાંઠ_Module* અચલ newIfacep = irefp->ifaceViaCellp();
            જો (!newIfacep) {
                iterateChildren(nodep);
                પાછા;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ગાંઠ_Module* અચલ varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            જો (varModp && varModp != newIfacep) {
                માટે (Ast_ગાંઠ* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    જો (AstVar* અચલ newVarp = VN_CAST(stmtp, Var)) {
                        જો (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            તોડો;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    શૂન્ય fixupCrossModuleRefDTypes(Ast_ગાંઠ_Module* newModp) {
        જો (!V3LinkDotIfaceCapture::enabled()) પાછા;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        પૂર્ણાંક fixed_ગણતરી = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            અચલ VIfaceCaptureTag* અચલ tagp = V3LinkDotIfaceCapture::captureTag(refp);
            જો (!tagp) પાછા;

            Ast_ગાંઠ_Module* અચલ correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            જો (!correctModp || correctModp->dead()) પાછા;
            જો (correctModp->parameterizedTemplate()) પાછા;

            તાર્કિક fixed = ખોટું;
            જો (refp->typedefp()) {
                જો (AstTypedef* અચલ newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    જો (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = સાચું;
                }
            }
            જો (!fixed && refp->refDTypep()) {
                જો (Ast_ગાંઠ_DType* અચલ newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = સાચું;
                }
            }
            જો (fixed) ++fixed_ગણતરી;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_ગણતરી);
    }

    // Return true on success, false on error
    તાર્કિક deepCloneModule(Ast_ગાંઠ_Module* srcModp, Ast_ગાંઠ* ifErrorp, AstPin* paramsp,
                         અચલ string& newname, અચલ IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ગાંઠ_Module* newModp;
        જો (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_વૃક્ષ(ખોટું), ગાંઠ_Module);
        } નહીંતર {
            newModp = srcModp->clone_વૃક્ષ(ખોટું);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        જો (AstIface* અચલ newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(સાચું);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(ખોટું);

        newModp->name(newname);
        newModp->user2(ખોટું);  // We need to re-recurse this module once changed
        newModp->recursive(ખોટું);
        newModp->recursiveClone(ખોટું);
        newModp->hasGParam(ખોટું);
        // Recursion may need level cleanups
        જો (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        જો ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_વૃક્ષ(), newModp);
            પાછા ખોટું;
        }

        જો (AstClass* અચલ newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ગાંઠ* insertp = srcModp;
        જ્યારે (VN_IS(insertp->nextp(), ગાંઠ_Module)
               && VN_AS(insertp->nextp(), ગાંઠ_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        અચલ auto iter = m_modNameMap.find(newname);
        CloneMap* અચલ clonemapp = &(iter->second.m_cloneMap);
        UINFO(4, "     De-parameterize to new: " << newModp);

        // Grab all I/O so we can remap our pins later
        // Note we allow multiple users of a parameterized model,
        // thus we need to stash this info.
        collectPins(clonemapp, newModp, srcModp->user3p());
        // Relink parameter vars to the new module
        // For interface ports (e.g., l3_if #(W, L0A_W) l3), the parameter pins may
        // reference variables from the enclosing module rather than from the interface
        // being cloned. In such cases, use relinkPinsByName to match by variable name.
        // Check if any parameter pins reference variables outside the cloned interface.
        // This is O(n) but acceptable since parameter pin lists are typically small (<10 pins).
        તાર્કિક needRelinkByName = ખોટું;
        જો (paramsp) {
            માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = સાચું;
                    તોડો;
                }
            }
        }
        જો (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } નહીંતર {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        માટે (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            અચલ AstIfaceRefDType* અચલ portIrefp = it->first;
            અચલ AstIfaceRefDType* અચલ pinIrefp = it->second;
            AstIfaceRefDType* અચલ cloneIrefp = portIrefp->clonep();
            UINFO(8, "     IfaceOld " << portIrefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, portIrefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->ifaceViaCellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        જો (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (pinp->exprp()) {
                જો (AstVar* અચલ modvarp = pinp->modVarp()) {
                    Ast_ગાંઠ* અચલ newp = pinp->exprp();  // Const or ConsArray
                    AstConst* અચલ exprp = VN_CAST(newp, Const);
                    AstConst* અચલ origp = VN_CAST(modvarp->valuep(), Const);
                    અચલ તાર્કિક overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    જો (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_વૃક્ષ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_વૃક્ષ(ખોટું));
                    modvarp->overriddenParam(overridden);
                } નહીંતર જો (AstParamTypeDType* અચલ modptp = pinp->modPTypep()) {
                    Ast_ગાંઠ_DType* અચલ dtypep = VN_AS(pinp->exprp(), ગાંઠ_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    જો (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_વૃક્ષ();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_વૃક્ષ(ખોટું));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        પાછા સાચું;
    }
    અચલ ModInfo* moduleFindOrClone(Ast_ગાંઠ_Module* srcModp, Ast_ગાંઠ* ifErrorp, AstPin* paramsp,
                                     અચલ string& newname, અચલ IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        જો (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } નહીંતર {
            જો (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                પાછા nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        અચલ ModInfo* અચલ modInfop = &(it->second);
        પાછા modInfop;
    }

    શૂન્ય convertToStringp(Ast_ગાંઠ* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* અચલ constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        જો (constp && !constp->અંક().isString()) {
            constp->replaceWith(
                નવું AstConst{constp->fileline(), AstConst::String{}, constp->અંક().toString()});
            VL_DO_DANGLING(constp->delete_વૃક્ષ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    શૂન્ય constify_Member_કિંમત(AstVar* varp, std::set<AstVar*>& inProgress) {
        જો (!varp->valuep() || VN_IS(varp->valuep(), Const)) પાછા;
        જો (!inProgress.ઉમેરો(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_વૃક્ષ();
            varp->valuep(નવું AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            પાછા;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](અચલ AstVarRef* refp) {
            AstVar* અચલ refVarp = refp->varp();
            જો (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ઉમેરો(refVarp);
            }
        });
        માટે (AstVar* અચલ sibp : siblings) constify_Member_કિંમત(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    સ્થિર AstParseRef* memberParseRef(Ast_ગાંઠ* nodep) {
        જો (AstParseRef* અચલ refp = VN_CAST(nodep, ParseRef)) પાછા refp;
        Ast_ગાંઠ_Pre_Sel* અચલ selp = VN_CAST(nodep, ગાંઠ_Pre_Sel);
        પાછા selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    સ્થિર શૂન્ય replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ગાંઠ_પદાવલિ* valuep) {
        જો (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } નહીંતર {
            Ast_ગાંઠ_Pre_Sel* અચલ selp = VN_AS(dotp->rhsp(), ગાંઠ_Pre_Sel);
            જો (AstAttrOf* અચલ attrp = selp->attrp()) {
                જો (Ast_ગાંઠ* અચલ oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_વૃક્ષ(ખોટું));
                    VL_DO_DANGLING(oldFromp->delete_વૃક્ષ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_વૃક્ષ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_વૃક્ષ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    સ્થિર તાર્કિક isDeferredMemberBase(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (VN_IS(nodep, MemberSel)) પાછા સાચું;
        જો (અચલ Ast_ગાંઠ_Pre_Sel* અચલ selp = VN_CAST(nodep, ગાંઠ_Pre_Sel)) {
            અચલ Ast_ગાંઠ_DType* અચલ fromDTypep = selp->fromp()->dtypep();
            અચલ Ast_ગાંઠ_Array_DType* અચલ arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ગાંઠ_Array_DType) : nullptr;
            જો (arrayDTypep) {
                અચલ Ast_ગાંઠ_DType* અચલ elemDTypep = arrayDTypep->subDTypep();
                પાછા elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ગાંઠ_UOr_Struct_DType);
            }
            પાછા isDeferredMemberBase(selp->fromp());
        }
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = nodep->dtypep();
        પાછા dtypep && VN_IS(dtypep->skipRefOrNullp(), ગાંઠ_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    સ્થિર શૂન્ય resolveMemberDot(AstDot* dotp) {
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = VN_CAST(dotp->lhsp(), ગાંઠ_પદાવલિ);
        AstParseRef* અચલ memberRefp = memberParseRef(dotp->rhsp());
        જો (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) પાછા;
        AstMemberSel* અચલ newp = નવું AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    શૂન્ય resolveDotToTypedef(Ast_ગાંઠ* exprp) {
        AstDot* અચલ dotp = VN_CAST(exprp, Dot);
        જો (!dotp) પાછા;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* અચલ classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        જો (!classRefp) {
            resolveMemberDot(dotp);
            પાછા;
        }
        AstParseRef* અચલ parseRefp = memberParseRef(dotp->rhsp());
        જો (!parseRefp) પાછા;

        અચલ AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* અચલ paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        જો (paramTypep) {
            Ast_ગાંઠ_DType* અચલ dtypep = paramTypep->subDTypep();
            AstClassRefDType* અચલ classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            જો (classRefDTypep) {
                AstClass* અચલ srcClassp = classRefDTypep->classp();
                જો (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    જો (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        જો (classRefp->paramsp()) {
            AstClass* અચલ srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            જો (srcClassp && srcClassp->hasGParam()) {
                જો (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        જો (AstClassRefDType* અચલ crp = class_Ref_DType_Of_ગાંઠ(classRefp->classOrPackageNodep())) {
            જો (AstClass* અચલ srcClassp = crp->classp()) {
                જો (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    જો (AstClass* અચલ newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        જો (!lhsClassp) પાછા;

        Ast_ગાંઠ* અચલ memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        જો (AstTypedef* અચલ tdefp = VN_CAST(memberp, Typedef)) {
            જો (parseRefp != dotp->rhsp()) પાછા;
            AstRefDType* અચલ refp = નવું AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_વૃક્ષ(), dotp);
        } નહીંતર જો (AstVar* અચલ varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    શૂન્ય substituteParamMember(AstDot* અચલ dotp, AstParseRef* અચલ memberRefp,
                               AstVar* અચલ varp) {
        જો (!varp->isParam() || !varp->valuep()) પાછા;
        જો (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_કિંમત(varp, inProgress);
        }
        AstConst* અચલ constp = VN_CAST(varp->valuep(), Const);
        જો (!constp) પાછા;
        AstConst* અચલ newp = constp->clone_વૃક્ષ(ખોટું);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    શૂન્ય resolveParamClassRefDType(AstRefDType* refp) {
        જો (refp->typedefp() || refp->refDTypep()) પાછા;

        AstClassOrPackageRef* અચલ classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        જો (!classRefp) પાછા;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        જો (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            પાછા;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ગાંઠ* targetp = classRefp->classOrPackageNodep();
        જ્યારે (અચલ AstTypedef* અચલ tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        જો (Ast_ગાંઠ_DType* અચલ dtargetp = VN_CAST(targetp, ગાંઠ_DType))
            targetp = dtargetp->skipRefOrNullp();
        જો (!targetp) પાછા;
        AstClassRefDType* અચલ classRefDTypep = VN_CAST(targetp, ClassRefDType);
        જો (!classRefDTypep) પાછા;
        srcClassp = classRefDTypep->classp();
        જો (!srcClassp) પાછા;

        AstClass* newClassp = srcClassp;
        જો (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* અચલ typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        જો (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    સ્થિર તાર્કિક defaultParamsHaveTypeQueryOnParamType(અચલ AstClassRefDType* nodep) {
        તાર્કિક found = ખોટું;
        nodep->foreach([&](અચલ AstAttrOf* attrp) {
            જો (found || !attrp->attrType().isTypeQuery()) પાછા;
            અચલ AstRefDType* અચલ refp = VN_CAST(attrp->fromp(), RefDType);
            જો (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = સાચું;
        });
        પાછા found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    તાર્કિક classTypeMatchesDefaultClone(અચલ Ast_ગાંઠ_DType* exprp, અચલ Ast_ગાંઠ_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        અચલ auto* અચલ પદાવલિ_Class_Refp = VN_CAST(exprp, ClassRefDType);
        અચલ auto* અચલ origClassRefp = VN_CAST(origp, ClassRefDType);
        જો (!પદાવલિ_Class_Refp || !origClassRefp) પાછા ખોટું;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        અચલ Ast_ગાંઠ_Module* અચલ defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        જો (defaultClonep && defaultClonep == પદાવલિ_Class_Refp->classp()) પાછા સાચું;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        જો (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) પાછા ખોટું;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        જો (!origClassRefp->classp()->hasGParam()) પાછા ખોટું;
        જો (origClassRefp->classp()->origName() != પદાવલિ_Class_Refp->classp()->origName())
            પાછા ખોટું;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        જો (!m_defaultCloneInProgress.ઉમેરો(origClassRefp->classp()).second) પાછા ખોટું;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* અચલ origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_વૃક્ષ(ખોટું));
        Ast_ગાંઠ_Module* અચલ resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        અચલ તાર્કિક match = resolvedModp && VN_CAST(resolvedModp, Class) == પદાવલિ_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_વૃક્ષ(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        પાછા match;
    }

    સ્થિર તાર્કિક paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        જો (exprp->અંક().width() == origp->અંક().width()) પાછા ખોટું;
        જો (exprp->અંક().isOpaque()) પાછા ખોટું;
        જો (origp->અંક().isOpaque()) પાછા ખોટું;
        અચલ પૂર્ણાંક maxWidth = std::max(exprp->અંક().width(), origp->અંક().width());
        V3_સંખ્યા પદાવલિ_અંક{exprp, maxWidth};
        જો (exprp->અંક().isSigned()) {
            પદાવલિ_અંક.opExtendS(exprp->અંક(), exprp->અંક().width());
        } નહીંતર {
            પદાવલિ_અંક.opAssign(exprp->અંક());
        }
        V3_સંખ્યા orig_અંક{origp, maxWidth};
        જો (origp->અંક().isSigned()) {
            orig_અંક.opExtendS(origp->અંક(), origp->અંક().width());
        } નહીંતર {
            orig_અંક.opAssign(origp->અંક());
        }
        V3_સંખ્યા isEq{exprp, 1};
        isEq.opEq(પદાવલિ_અંક, orig_અંક);
        પાછા isEq.isNeqZero();
    }

    // Rewrite placeholder __VGIfaceParam pins emitted by
    // LinkDotResolveVisitor::addImplicitParametersOfGenericIface for a nested
    // generic-interface forwarding chain (#7454). Must run here (not in
    // V3LinkDot's Param/Resolve visitor at LDS_PARAMED) because:
    //   * linkDotParamed runs AFTER V3Param, at which point cellDeparam has
    //     already consumed the pin via moduleFindOrClone (variant naming off
    //     pinp->exprp()'s IfaceRefDType) and genericInterfaceVarSetup (hard
    //     VN_AS cast to IfaceRefDType); and
    //   * the enclosing module's port only becomes a concrete IfaceRefDType
    //     after its own nodeDeparam has specialized it. That specialization
    //     happens in V3Param's top-down walk, so the window between "outer
    //     port resolved" and "inner cellDeparam starts" exists only here.
    શૂન્ય resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (!pinp->exprp()) ચાલુ;
            જો (!VString::startsWith(pinp->name(), "__VGIfaceParam")) ચાલુ;
            અચલ Ast_ગાંઠ_Var_Ref* અચલ fwdRefp = VN_CAST(pinp->exprp(), ગાંઠ_Var_Ref);
            જો (!fwdRefp) ચાલુ;
            અચલ AstIfaceRefDType* અચલ resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* અચલ newIrefp = નવું AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            જો (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_વૃક્ષ(સાચું));
            }
            pinp->exprp()->unlinkFrBack()->delete_વૃક્ષ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    શૂન્ય name_By_Pin_કિંમત(AstPin* pinp, Ast_ગાંઠ_Module* srcModp, AstVar* modvarp, string& longnamer,
                        તાર્કિક& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_કિંમત_સંખ્યા(pinp->exprp());
        any_overridesr = સાચું;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    શૂન્ય cellPinCleanup(Ast_ગાંઠ* nodep, AstPin* pinp, AstPin* paramsp, Ast_ગાંઠ_Module* srcModp,
                        string& longnamer, તાર્કિક& any_overridesr,
                        std::unordered_set<અચલ AstPin*>& overridingTypePinsr) {
        જો (!pinp->exprp()) પાછા;  // No-connect
        જો (AstVar* અચલ modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            જો (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } નહીંતર જો (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_કિંમત(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } નહીંતર {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                જો (is_Aggregate_Param_કિંમત(pinp->exprp())) {
                    name_By_Pin_કિંમત(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    પાછા;
                }
                // Cast/CastSize default values are not yet folded by V3Width.
                // Constify here so the comparison below sees a Const node.
                // Other node kinds are handled in the branches above.
                //
                // Only fold when the cast is self-contained.  modvarp is the
                // parameter on the shared module template and constifyParamsEdit
                // is destructive: a default that reads another parameter
                // (VarRef) or a type parameter (RefDType) must be evaluated per
                // instance, and folding it here would evaluate it against the
                // template's own defaults and bake that in for every later
                // instance that relies on the default.
                જો (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    અચલ તાર્કિક dependent = modvarp->valuep()->exists(
                        [](Ast_ગાંઠ* np) { પાછા VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    જો (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                જો (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* અચલ exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* અચલ origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                જો (exprp && !exprp->અંક().isDouble() && !exprp->અંક().isString()) {
                    AstVar* cloneVarp = modvarp->clone_વૃક્ષ(ખોટું);
                    તાર્કિક cloneVarpUnresolved = ખોટું;
                    જો (Ast_ગાંઠ* અચલ oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_વૃક્ષ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_વૃક્ષ(ખોટું));
                    જો (Ast_ગાંઠ_DType* અચલ origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        જો (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_વૃક્ષ();
                        cloneVarp->childDTypep(origDTypep->clone_વૃક્ષ(ખોટું));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr પૂર્ણાંક maxSubstIters = 1000;
                        માટે (પૂર્ણાંક it = 0; it < maxSubstIters; ++it) {
                            તાર્કિક any = ખોટું;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* અચલ targetp = varrefp->varp();
                                Ast_ગાંઠ* replacep = nullptr;
                                માટે (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    જો (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        જો (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_વૃક્ષ(ખોટું);
                                        }
                                        તોડો;
                                    }
                                }
                                જો (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_વૃક્ષ(ખોટું);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* અચલ patp = VN_CAST(replacep, Pattern);
                                જો (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_વૃક્ષ(ખોટું));
                                }
                                જો (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_વૃક્ષ(), varrefp);
                                    any = સાચું;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_ગાંઠ_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* અચલ ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                જો (!ptdp) પાછા;
                                AstPin* overridePinp = nullptr;
                                માટે (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    જો (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        તોડો;
                                    }
                                }
                                Ast_ગાંઠ_DType* અચલ substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ગાંઠ_DType)
                                                   : ptdp->subDTypep();
                                જો (substp) toReplace.emplace_back(refp, substp);
                            });
                            માટે (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* અચલ refp = it->first;
                                refp->replaceWith(it->second->clone_વૃક્ષ(ખોટું));
                                VL_DO_DANGLING(refp->delete_વૃક્ષ(), refp);
                                any = સાચું;
                            }
                            જો (!any) તોડો;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ગાંઠ* np) {
                            જો (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = સાચું;
                                પાછા;
                            }
                            જો (AstEnumItemRef* અચલ itemRefp = VN_CAST(np, EnumItemRef)) {
                                જો (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = સાચું;
                                }
                                પાછા;
                            }
                            AstRefDType* અચલ refp = VN_CAST(np, RefDType);
                            જો (!refp) પાછા;
                            જો (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = સાચું;
                            }
                            // A type declared in the template must not be widthed there
                            જો (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = સાચું;
                            }
                        });
                    }
                    જો (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        જો (AstConst* અચલ widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            જો (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_વૃક્ષ(), cloneVarp);
                }
                AstConst* અચલ namingExprp = normedNamep ? normedNamep : exprp;
                જો (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ગાંઠ* અચલ pinExprp = pinp->exprp();
                    pinExprp->replaceWith(નવું AstConst{pinp->fileline(), AstConst::Widthed_કિંમત{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_વૃક્ષ(), pinExprp);
                } નહીંતર જો (origp
                           && (exprp->same_વૃક્ષ(origp)
                               || (exprp->અંક().width() == origp->અંક().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } નહીંતર જો (namingExprp->અંક().isDouble() || namingExprp->અંક().isString()
                           || namingExprp->અંક().isFourState()
                           || namingExprp->અંક().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_કિંમત_સંખ્યા(namingExprp));
                    any_overridesr = સાચું;
                } નહીંતર {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->અંક().ascii(ખોટું));
                    any_overridesr = સાચું;
                }
                જો (normedNamep) VL_DO_DANGLING(normedNamep->delete_વૃક્ષ(), normedNamep);
            }
        } નહીંતર જો (AstParamTypeDType* અચલ modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ગાંઠ_DType* rawTypep = VN_CAST(pinp->exprp(), ગાંઠ_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            તાર્કિક skipWidthForTemplateStruct = ખોટું;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ગાંઠ_DType* અચલ resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                જો (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ગાંઠ_Module* અચલ ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    જો (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = સાચું;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                જો (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_ગાંઠ_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            અચલ Ast_ગાંઠ_DType* origp = modvarp->skipRefToNonRefp();
            જો (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } નહીંતર જો (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } નહીંતર {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                જો (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ગાંઠ_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                જો (!modvarp->fwdType().is_ગાંઠ_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                જો (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } નહીંતર {
                    જો (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ગાંઠ_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    જો (AstClassRefDType* અચલ classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        જો (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } નહીંતર {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    શૂન્ય cellInterfaceCleanup(AstPin* pinsp, Ast_ગાંઠ_Module* srcModp, string& longnamer,
                              તાર્કિક& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        માટે (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            અચલ AstVar* અચલ modvarp = pinp->modVarp();
            જો (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                ચાલુ;
            }
            જો (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* અચલ portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                અચલ Ast_ગાંઠ* અચલ exprp = pinp->exprp();
                જો (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    ચાલુ;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                જો (અચલ Ast_ગાંઠ_Var_Ref* અચલ vrp = VN_CAST(exprp, ગાંઠ_Var_Ref)) {
                    જો (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                જો (!pinIrefp && exprp) {
                    અચલ Ast_ગાંઠ* basep = exprp;
                    જ્યારે (અચલ Ast_ગાંઠ_Pre_Sel* અચલ selp = VN_CAST(basep, ગાંઠ_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    જો (અચલ AstVarRef* અચલ vrp = VN_CAST(basep, VarRef)) {
                        જો (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                જો (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } નહીંતર જો (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } નહીંતર {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    જો (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_કિંમત_સંખ્યા(pinIrefp));
                        any_overridesr = સાચું;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        જો (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ગાંઠ::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ગાંઠ::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    શૂન્ય storeOriginalParams(Ast_ગાંઠ_Module* અચલ classp) {
        માટે (Ast_ગાંઠ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ગાંઠ* originalParamp = nullptr;
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->isGParam()) originalParamp = varp->clonep();
            } નહીંતર જો (AstParamTypeDType* અચલ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                જો (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            જો (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    શૂન્ય genericInterfaceVarSetup(અચલ AstPin* અચલ paramsp, અચલ AstPin* અચલ pinsp) {
        std::unordered_map<string, અચલ AstPin*> paramspMap;
        માટે (અચલ AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ઉમેરો({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        જો (paramspMap.empty()) પાછા;

        માટે (અચલ Ast_ગાંઠ* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            જો (અચલ AstPin* અચલ pinp = VN_CAST(nodep, Pin)) {
                જો (AstVar* અચલ varp = pinp->modVarp()) {
                    Ast_ગાંઠ_DType* dtypep = varp->childDTypep()->elemDTypep();
                    જો (AstIfaceGenericDType* અચલ ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        અચલ auto iter = paramspMap.find(varp->name());
                        જો (iter == paramspMap.end()) ચાલુ;
                        Ast_ગાંઠ* અચલ backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        અચલ AstPin* અચલ paramp = iter->second;
                        paramspMap.erase(iter);
                        અચલ AstIfaceRefDType* અચલ ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* અચલ newIfacerefp = નવું AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        જો (auto* અચલ arrDtp = VN_CAST(backp, ગાંઠ_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } નહીંતર જો (auto* અચલ arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } નહીંતર {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ધકેલો_Deletep(ifaceGDTypep), ifaceGDTypep);
                        જો (paramspMap.empty()) પાછા;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    શૂન્ય resolveDefaultParams(Ast_ગાંઠ* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        જો (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            જો (!classp) પાછા;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } નહીંતર જો (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } નહીંતર {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        માટે (Ast_ગાંઠ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                જો (!paramTypep->isGParam()) ચાલુ;
                m_paramIndex.emplace(paramTypep, static_cast<પૂર્ણાંક>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_ગાંઠ_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        માટે (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* અચલ paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* અચલ reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            જો (!reqDtp) ચાલુ;

            // If default is a reference to another param type, record dependency
            જો (AstRefDType* અચલ refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                જો (AstParamTypeDType* અચલ sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    જો (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    ચાલુ;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            જો (Ast_ગાંઠ_DType* અચલ dtp = VN_CAST(reqDtp->lhsp(), ગાંઠ_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* અચલ typep = pinp->modPTypep();
            અચલ auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            જો (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } નહીંતર {
                pinsByIndex.ધકેલો_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        માટે (તાર્કિક resolvedSome = સાચું; resolvedSome;) {
            resolvedSome = ખોટું;
            માટે (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                જો (pinsByIndex[paramIdx]) ચાલુ;
                અચલ પૂર્ણાંક sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                જો (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_વૃક્ષ(ખોટું);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                જો (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ગાંઠ_DType* અચલ dtypep = defaultTypeNodes[paramIdx];
                    newPinp = નવું AstPin{dtypep->fileline(), static_cast<પૂર્ણાંક>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_વૃક્ષ(ખોટું)};
                }

                જો (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(સાચું);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    જો (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } નહીંતર જો (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    જો (!paramsp) paramsp = newPinp;
                    resolvedSome = સાચું;
                }
            }
        }
    }

    Ast_ગાંઠ_Module* ગાંઠ_Deparam_Common(Ast_ગાંઠ* nodep, Ast_ગાંઠ_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, તાર્કિક any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        જો (debug() >= 9 && paramsp) paramsp->dump_વૃક્ષ_And_Next(cout, "-  cellparams: ");

        જો (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } નહીંતર {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<અચલ AstPin*> overridingTypePins;
            માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (!pinp->exprp()) ચાલુ;  // An empty override keeps the default
                જો (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    ચાલુ;
                }
                AstVar* અચલ modvarp = pinp->modVarp();
                જો (!modvarp || !modvarp->isGParam()) ચાલુ;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                અચલ AstPattern* અચલ patternp = VN_CAST(pinp->exprp(), Pattern);
                જો (patternp && !patternp->childDTypep()) ચાલુ;
                જો (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_કિંમત(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (AstParamTypeDType* અચલ modptp = pinp->modPTypep()) {
                    જો (!overridingTypePins.ગણતરી(pinp)) ચાલુ;
                    Ast_ગાંઠ_DType* અચલ typep = VN_AS(pinp->exprp(), ગાંઠ_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_કિંમત_સંખ્યા(typep->skipRefToNonRefp());
                    any_overrides = સાચું;
                    ચાલુ;
                }
                cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                               any_overrides /*ref*/, overridingTypePins /*ref*/);
            }
        }
        IfaceRefRefs ifaceRefRefs;
        cellInterfaceCleanup(pinsp, srcModp, longname /*ref*/, any_overrides /*ref*/,
                             ifaceRefRefs /*ref*/);

        // Template classes with type parameters need specialization even when types match
        // defaults. This is required for UVM parameterized classes. However, interfaces should NOT
        // be specialized when type params match defaults (needed for nested interface ports).
        // Already-specialized clones (hasGParam=false) must not be re-cloned, otherwise
        // nested class type parameters cause unbounded re-deparameterization (Holder_ ->
        // Holder__).
        તાર્કિક defaultsResolved = ખોટું;
        જો (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            માટે (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                જો (pinp->modPTypep()) {
                    any_overrides = સાચું;
                    defaultsResolved = સાચું;
                    તોડો;
                }
            }
        }
        જો (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = સાચું;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ગાંઠ_Module* newModp = nullptr;
        જો (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_ગાંઠ_Module* અચલ paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            જો (!paramedModp) પાછા nullptr;  // Unsupported, already reported
            paramedModp->dead(ખોટું);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } નહીંતર જો (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            જો (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ગાંઠ_Module* ગાંઠ_Copyp = srcModp->clone_વૃક્ષ(ખોટું);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ગાંઠ_Copyp);
                m_deleter.ધકેલો_Deletep(ગાંઠ_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ગાંઠ_Copyp);
                storeOriginalParams(ગાંઠ_Copyp);
            }
            newModp = srcModp;
        } નહીંતર {
            અચલ string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            અચલ ModInfo* અચલ modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            જો (!modInfop) પાછા nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        અચલ તાર્કિક cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* અચલ newClassp = VN_CAST(newModp, Class);
        જો (newClassp && !newClassp->user2()) m_specializedClassps.ધકેલો_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        જો (defaultsResolved) srcModp->user4p(newModp);

        માટે (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                જો (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        જો (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_વૃક્ષ(), paramsp);
        પાછા newModp;
    }

    Ast_ગાંઠ_Module* cellDeparam(AstCell* nodep, Ast_ગાંઠ_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ગાંઠ_Module* અચલ newModp = ગાંઠ_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        જો (!newModp) પાછા nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(ખોટું);
        પાછા newModp;
    }
    Ast_ગાંઠ_Module* ifaceRefDeparam(AstIfaceRefDType* અચલ nodep, Ast_ગાંઠ_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        જો (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            અચલ string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            અચલ string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            અચલ size_t ownerPos = ownerBaseName.find("__");
            જો (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            અચલ size_t srcPos = srcBaseName.find("__");
            જો (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            જો (ownerBaseName == srcBaseName) {
                તાર્કિક allOwnParams = સાચું;
                માટે (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    જો (AstRefDType* અચલ refp = VN_CAST(pinp->exprp(), RefDType)) {
                        જો (AstParamTypeDType* અચલ ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ગાંઠ_Module* અચલ ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            જો (ptdOwnerp != m_modp) allOwnParams = ખોટું;
                        } નહીંતર {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } નહીંતર {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                જો (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    જો (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
                    પાછા ownerIfacep;
                }
            }
        }

        Ast_ગાંઠ_Module* અચલ newModp
            = ગાંઠ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ખોટું);
        જો (!newModp) પાછા nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        પાછા newModp;
    }
    Ast_ગાંઠ_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ગાંઠ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ગાંઠ_Module* અચલ newModp
            = ગાંઠ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ખોટું);
        જો (!newModp) પાછા nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* અચલ refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* અચલ newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        જો (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            જો (AstTypedef* અચલ typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        પાછા newModp;
    }
    Ast_ગાંઠ_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ગાંઠ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ગાંઠ_Module* અચલ newModp
            = ગાંઠ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ખોટું);
        જો (!newModp) પાછા nullptr;
        AstClass* અચલ newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        પાછા newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    વર્ગ DeferredResolverVisitor final : જાહેર VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<અચલ Ast_ગાંઠ*> m_reachedDecls;

        તાર્કિક firstReach(અચલ Ast_ગાંઠ* અચલ declp) { પાછા m_reachedDecls.ઉમેરો(declp).second; }

        શૂન્ય iterateParamType(AstParamTypeDType* અચલ paramTypep) {
            જો (firstReach(paramTypep)) iterate(paramTypep);
        }
        શૂન્ય iterateTypedef(AstTypedef* અચલ tdefp) {
            જો (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }
        શૂન્ય visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ગાંઠ* અચલ targetp = nodep->classOrPackageNodep();
            જો (AstTypedef* અચલ tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } નહીંતર જો (AstParamTypeDType* અચલ paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        શૂન્ય visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        શૂન્ય visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* અચલ tdefp = nodep->typedefp();
            જો (tdefp) {
                iterateTypedef(tdefp);
            } નહીંતર જો (AstParamTypeDType* અચલ paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        શૂન્ય visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* અચલ varp = nodep->varp();
            જો (firstReach(varp)) {
                iterate(varp->subDTypep());
                અચલ auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                જો (varp->varType() == VVarType::LPARAM && deferredVarps.ગણતરી(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    જાહેર:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ગાંઠ* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

જાહેર:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    શૂન્ય retargetIfaceRefs(Ast_ગાંઠ_Module* parentModp, અચલ string& cellName) {
        Ast_ગાંઠ_Module* અચલ correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        જો (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) પાછા;
        parentModp->foreach([&](AstRefDType* refp) {
            અચલ VIfaceCaptureTag* અચલ tagp = V3LinkDotIfaceCapture::captureTag(refp);
            જો (!tagp || tagp->m_cellPath != cellName) પાછા;
            // Only retarget REFDTYPEs owned by parentModp.
            જો (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) પાછા;
            જો (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    શૂન્ય resolveDeferredDotsReachableFrom(Ast_ગાંઠ* rootp, અચલ Ast_ગાંઠ_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*આ, rootp};
    }

    Ast_ગાંઠ_Module* ગાંઠ_Deparam(Ast_ગાંઠ* nodep, Ast_ગાંઠ_Module* srcModp, Ast_ગાંઠ_Module* modp,
                               અચલ string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ગાંઠ::ગાંઠ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ગાંઠ_Name = nodep->name();
        જો (AstIfaceRefDType* અચલ ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            જો (ગાંઠ_Name.empty()) ગાંઠ_Name = ifaceRefp->cellName();
        }
        અચલ string instanceName
            = ગાંઠ_Name.empty() ? someInstanceName : (someInstanceName + "." + ગાંઠ_Name);

        જો (AstCell* અચલ cellp = VN_CAST(nodep, Cell)) {
            માટે (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* અચલ nextp = VN_AS(pinp->nextp(), Pin);
                જો (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_વૃક્ષ(), pinp);
                }
                pinp = nextp;
            }
            // Nested generic-iface forwarding (#7454): rewrite VarRef placeholders
            // left by V3LinkDot to concrete IfaceRefDTypes now that the enclosing
            // module has been specialized.
            resolveGenericIfaceForwardingPins(cellp->paramsp());
        }
        // Create new module name with _'s between the constants
        UINFOTREE(10, nodep, "", "cell");
        // Resolve `class::member` Dots in pin values, and in any deferred
        // lparam reachable from the pin tree, so constify sees Consts.
        resolveDeferredDotsReachableFrom(nodep, modp);
        // Evaluate all module constants
        V3Const::constifyParamsEdit(nodep);
        // Set name for warnings for when we param propagate the module
        // For AstIfaceRefDType, name() returns the modport name (often empty),
        // so use cellName() which is the actual cell instance name.
        // If both are empty (interface port, not a cell), skip appending
        // to avoid double-dots in the path.
        srcModp->someInstanceName(instanceName);
        UINFO(9, "nodeDeparam SET-SRC-INST srcMod="
                     << srcModp->prettyNameQ() << " someInstanceName='"
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ગાંઠ::ગાંઠ_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ગાંઠ_Module* newModp = nullptr;
        જો (AstCell* અચલ cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } નહીંતર જો (AstIfaceRefDType* અચલ ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } નહીંતર જો (AstClassRefDType* અચલ classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } નહીંતર જો (AstClassOrPackageRef* અચલ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } નહીંતર {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        જો (!newModp) પાછા nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ગાંઠ::ગાંઠ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        પાછા newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        પાછા taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        માટે (Ast_ગાંઠ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
            m_allModuleNames.ઉમેરો(modp->name());
        }
    }
    ~ParamProcessor() = મૂળભૂત;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

વર્ગ ClassRefUnlinkerVisitor final : જાહેર VNVisitor {

જાહેર:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
        જો (nodep->paramsp()) {
            જો (AstClass* અચલ classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                જો (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* અચલ dotp = VN_CAST(nodep->backp(), Dot);
                    જો (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(ધકેલો_Deletep(dotp), dotp, nodep);
                    } નહીંતર {
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    શૂન્ય visit(AstClass* nodep) override {}  // don't iterate inside classes
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

વર્ગ ParamClassRefDTypeRelinkVisitor final : જાહેર VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<અચલ AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ગાંઠ_Module* m_ownerModp = nullptr;
    Ast_ગાંઠ_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    શૂન્ય ensureOwnerMap() {
        જો (m_ownerMapForp == m_ownerModp) પાછા;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        જો (!m_ownerModp) પાછા;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        જો (AstClass* અચલ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            જો (ownerClassp->hasGParam()) પાછા;
        }
        માટે (Ast_ગાંઠ* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* અચલ tdp = VN_CAST(sp, Typedef);
            જો (!tdp) ચાલુ;
            AstClassRefDType* અચલ crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            જો (!crdtp || !crdtp->classp()) ચાલુ;
            AstClass* અચલ targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            જો (targetp->hasGParam()) ચાલુ;
            અચલ std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            અચલ auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            જો (!pair.second && pair.first->second != targetp) m_ambiguous.ઉમેરો(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    સ્થિર string classOrigName(અચલ AstClass* classp) {
        પાછા classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    સ્થિર AstTypedef* findTypedefWithBases(AstClass* classp, અચલ string& name) {
        માટે (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            જો (AstTypedef* અચલ tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                જો (tdp->subDTypep()) પાછા tdp;
            }
        }
        પાછા nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    શૂન્ય retargetRefDType(AstRefDType* refp) {
        જો (!m_ownerModp) પાછા;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        જો (AstClass* અચલ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            જો (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                અચલ AstClass* refClassp = nullptr;
                જો (AstTypedef* અચલ tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } નહીંતર જો (અચલ AstClassOrPackageRef* અચલ classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                જો (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* અચલ selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    જો (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        પાછા;
                    }
                }
            }
        }

        AstTypedef* અચલ oldTdp = refp->typedefp();
        જો (!oldTdp) પાછા;
        AstClass* અચલ oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        જો (!oldOwnerp) પાછા;

        ensureOwnerMap();
        જો (m_origNameToClone.empty()) પાછા;
        અચલ std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        જો (m_ambiguous.ગણતરી(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            પાછા;
        }
        અચલ auto it = m_origNameToClone.find(origName);
        જો (it == m_origNameToClone.end()) પાછા;
        AstClass* અચલ correctClassp = it->second;
        જો (correctClassp == oldOwnerp) પાછા;
        AstTypedef* અચલ newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        જો (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            પાછા;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        જો (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            પાછા;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

જાહેર:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstParamTypeDType* અચલ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        જો (AstParamTypeDType* અચલ paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            અચલ auto it = m_paramTypeClassMap.find(paramtypep);
            જો (it != m_paramTypeClassMap.end()) {
                AstClass* અચલ origClassp = it->second;
                જો (origClassp->hasGParam() && !origClassp->user3p()) {
                    જો (AstClass* અચલ parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        અચલ string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        માટે (Ast_ગાંઠ* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            જો (AstParamTypeDType* અચલ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                જો (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    તોડો;
                                }
                            }
                        }
                        જો (newParamTypep) {
                            nodep->refDTypep(newParamTypep);
                            nodep->classOrPackagep(parametrizedClassp);
                        }
                    }
                }
            }
        }
        // Retarget REFDTYPE.typedefp using the current owner's own
        // resolved typedefs as ground truth (handles sibling-clone stomping).
        retargetRefDType(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

વર્ગ ParamState final {
જાહેર:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<તાર્કિક, પૂર્ણાંક>;
    std::multimap<WQKey, Ast_ગાંઠ_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ગાંઠ_Module*, std::unordered_set<Ast_ગાંઠ_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

વર્ગ ParamVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    તાર્કિક m_iterateModule = ખોટું;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<તાર્કિક, Ast_ગાંઠ*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<અચલ Ast_ગાંઠ*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<અચલ AstRefDType*, તાર્કિક>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<અચલ Ast_ગાંઠ_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    તાર્કિક m_modIfaceRefsDone = ખોટું;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    તાર્કિક m_inGenerateCond = ખોટું;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    શૂન્ય processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ગાંઠ_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = સાચું;

        // Visit all cells under module, recursively
        જ્યારે (સાચું) {
            // Classes specialized since the last pass still need their bodies elaborated
            માટે (AstClass* અચલ classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{સાચું, classp->level()}, classp);
            }
            જો (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            જો (workQueue.empty()) તોડો;

            અચલ auto itm = workQueue.cbegin();
            Ast_ગાંઠ_Module* અચલ modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            જો (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                જો (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = ખોટું;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            જ્યારે (!m_cellps.empty()) {
                અચલ auto itim = m_cellps.cbegin();
                Ast_ગાંઠ* અચલ cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                અચલ auto genHierIt = m_genHierNames.find(cellp);
                જો (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_ગાંઠ_Module* srcModp = nullptr;
                જો (અચલ AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } નહીંતર જો (અચલ AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    જો (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) ચાલુ;
                } નહીંતર જો (અચલ AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } નહીંતર જો (અચલ AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } નહીંતર {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                જો (!srcModp) ચાલુ;

                // Update path
                અચલ string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                જો (Ast_ગાંઠ_Module* અચલ newModp
                    = m_processor.ગાંઠ_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    જો (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        જો (V3LinkDotIfaceCapture::enabled()) {
                            જો (અચલ AstCell* અચલ modCellp = VN_CAST(cellp, Cell)) {
                                જો (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ઉમેરો(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    જો (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = ખોટું;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    સ્થિર string getRefBaseName(અચલ AstVarXRef* refp) {
        અચલ string dotted = refp->dotted();
        જો (dotted.empty()) પાછા "";
        પાછા dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    શૂન્ય logTemplateLeakRefs(Ast_ગાંઠ_Module* parentModp, Ast_ગાંઠ_Module* templateModp,
                             અચલ અક્ષર* stage, Ast_ગાંઠ* contextp) {
        જો (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) પાછા;
        // LCOV_EXCL_START  // Debug-only diagnostic
        પૂર્ણાંક leak_ગણતરી = 0;
        અચલ auto ancestryOf = [](અચલ Ast_ગાંઠ* nodep) {
            string ancestry;
            માટે (અચલ Ast_ગાંઠ* curp = nodep; curp; curp = curp->backp()) {
                જો (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                જો (VN_IS(curp, ગાંઠ_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    તોડો;
                }
            }
            પાછા ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            જો (refp->typedefp()) {
                Ast_ગાંઠ_Module* અચલ tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                જો (tdOwnerp == templateModp) {
                    ++leak_ગણતરી;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ગાંઠ::ગાંઠ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            જો (refp->refDTypep()) {
                Ast_ગાંઠ_Module* અચલ rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                જો (rdOwnerp == templateModp) {
                    ++leak_ગણતરી;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ગાંઠ::ગાંઠ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            જો (!varrefp->varp()) પાછા;
            Ast_ગાંઠ_Module* અચલ varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            જો (varOwnerp != templateModp) પાછા;
            ++leak_ગણતરી;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ગાંઠ::ગાંઠ_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        જો (leak_ગણતરી > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_ગણતરી);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    શૂન્ય checkParamNotHierRecurse(Ast_ગાંઠ* nodep, તાર્કિક underTypeQuery = ખોટું) {
        માટે (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            અચલ AstAttrOf* અચલ attrp = VN_CAST(nodep, AttrOf);
            અચલ તાર્કિક childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            જો (અચલ AstVarXRef* અચલ refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                તાર્કિક isIfaceRef = ખોટું;
                જો (refp->varp() && refp->varp()->isIfaceParam()) {
                    અચલ string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.ગણતરી(refname) || m_ifaceInstCells.ગણતરી(refname));
                }
                જો (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } નહીંતર જો (અચલ Ast_ગાંઠ_FTask_Ref* અચલ refp = VN_CAST(nodep, ગાંઠ_FTask_Ref)) {
                જો (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op2p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op3p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op4p(), childUnderQuery);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    શૂન્ય specializeNestedIfaceCells(Ast_ગાંઠ_Module* ifaceModp) {
        માટે (Ast_ગાંઠ* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* અચલ nestedCellp = VN_CAST(stmtp, Cell);
            જો (!nestedCellp) ચાલુ;
            જો (!VN_IS(nestedCellp->modp(), Iface)) ચાલુ;
            જો (!nestedCellp->paramsp()) ચાલુ;

            Ast_ગાંઠ_Module* અચલ nestedSrcModp = nestedCellp->modp();
            જો (Ast_ગાંઠ_Module* અચલ nestedNewModp = m_processor.ગાંઠ_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                જો (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    માટે (Ast_ગાંઠ* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        જો (AstVar* અચલ varp = VN_CAST(sp, Var)) {
                            જો (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    જો (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    તાર્કિક cellParamsReferenceIfacePorts(AstCell* cellp) {
        જો (!cellp->paramsp()) પાછા ખોટું;

        માટે (Ast_ગાંઠ* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            જો (AstPin* અચલ pinp = VN_CAST(nodep, Pin)) {
                જો (Ast_ગાંઠ* અચલ exprp = pinp->exprp()) {
                    જો (અચલ AstVarXRef* અચલ refp = VN_CAST(exprp, VarXRef)) {
                        અચલ string refname = getRefBaseName(refp);
                        જો (!refname.empty()
                            && (m_ifacePortNames.ગણતરી(refname)
                                || m_ifaceInstCells.ગણતરી(refname)))
                            પાછા સાચું;
                    }
                }
            }
        }
        પાછા ખોટું;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    શૂન્ય expandIfaceArrayPortDimensions(AstVar* portp, Ast_ગાંઠ_DType* dtypep,
                                        અચલ std::string& suffix) {
        અચલ AstUnpackArrayDType* અચલ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        જો (!arrp) {
            AstVar* અચલ varp = portp->clone_વૃક્ષ(ખોટું);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            જો (Ast_ગાંઠ_DType* અચલ oldp = varp->childDTypep()) {
                VL_DO_DANGLING(ધકેલો_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_વૃક્ષ(ખોટું));
            portp->addNextHere(varp);
            પાછા;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        માટે (અચલ પૂર્ણાંક n : arrp->declRange().seq_જમણું_To_ડાબું()) {
            અચલ std::string s = suffix + "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    શૂન્ય expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   અચલ AstRange* rangep, અચલ std::string& suffix, પૂર્ણાંક idx) {
        // Base case: insert the element when no dimensions left
        જો (!rangep) {
            AstCell* અચલ elemp = arrayedCellp->clone_વૃક્ષ(ખોટું);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            જો (ifaceVarp) {
                AstIfaceRefDType* અચલ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_વૃક્ષ(ખોટું);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* અચલ varp = ifaceVarp->clone_વૃક્ષ(ખોટું);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                જો (Ast_ગાંઠ_DType* અચલ oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(ધકેલો_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            પાછા;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        અચલ AstRange* અચલ subRangep = VN_AS(rangep->nextp(), Range);
        માટે (અચલ પૂર્ણાંક n : rangep->seq_જમણું_To_ડાબું()) {
            અચલ std::string s = suffix + "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    શૂન્ય visitCellOrClassRef(Ast_ગાંઠ* nodep, તાર્કિક isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        જો (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* અચલ cellp = VN_CAST(nodep, Cell);
            જો (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ગાંઠ_Module* અચલ srcModp = cellp->modp();
                Ast_ગાંઠ_Module* અચલ newModp
                    = m_processor.ગાંઠ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                જો (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                જો (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        જો (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->recursiveClone()) nodep->dead(સાચું);  // Fake, made for recursive elimination
        જો (nodep->dead()) પાછા;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        જો (AstClass* અચલ classp = VN_CAST(nodep, Class)) {
            જો (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ધકેલો_back(classp);
                પાછા;
            }
        }

        જો (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            પાછા;
        }

        // Start traversal at root-like things
        જો (nodep->is_ટોચ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    તાર્કિક isCircularType(અચલ AstRefDType* nodep) {
        અચલ auto iter = m_isCircular.emplace(nodep, સાચું);
        જો (!iter.second) પાછા iter.first->second;
        જો (અચલ AstRefDType* અચલ subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            અચલ તાર્કિક ret = isCircularType(subDTypep);
            iter.first->second = ret;
            પાછા ret;
        }
        iter.first->second = ખોટું;
        પાછા ખોટું;
    }

    શૂન્ય visit(AstRefDType* nodep) override {
        અચલ તાર્કિક isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        જો (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        જો (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } નહીંતર જો (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2SetOnce()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCell* nodep) override {
        જો (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            માટે (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* અચલ ifaceVarp = [&]() -> AstVar* {
                જો (!VN_IS(nodep->modp(), Iface)) પાછા nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* અચલ varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                પાછા varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            જો (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(સાચું);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* અચલ firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* અચલ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            પાછા;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        જો (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        જો (nodep->ifacep()) visitCellOrClassRef(nodep, સાચું);
    }
    શૂન્ય visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, ખોટું);
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        જો (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, ખોટું);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    સ્થિર શૂન્ય gatherVars(Ast_ગાંઠ* stmtsp, તાર્કિક (*matchp)(અચલ AstVar*), VarsByName& varsr) {
        માટે (Ast_ગાંઠ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                જો (matchp(varp)) varsr.emplace(varp->name(), varp);
            } નહીંતર જો (AstGenBlock* અચલ genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    અચલ VarsByName& modIfaceRefs() {
        જો (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](અચલ AstVar* varp) { પાછા varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = સાચું;
        }
        પાછા m_modIfaceRefs;
    }
    અચલ VarsByName& ifaceParams(Ast_ગાંઠ_Module* ifacep) {
        અચલ auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        જો (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](અચલ AstVar* varp) { પાછા varp->isParam(); },
                pair.first->second);
        }
        પાછા pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ગાંઠ* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* અચલ lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* અચલ rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        જો (!lhsp || !rhsp) પાછા nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        જો (VL_UNCOVERABLE(!m_modp)) પાછા nullptr;

        અચલ auto& refVars = modIfaceRefs();
        અચલ auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        જો (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) પાછા nullptr;
        AstVar* અચલ ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* અચલ ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        જો (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) પાછા nullptr;

        અચલ auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        અચલ auto targetVarIt = ifaceVars.find(rhsp->name());
        જો (targetVarIt == ifaceVars.end()) પાછા nullptr;
        AstVar* અચલ targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        જો (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        જો (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            પાછા nullptr;  // LCOV_EXCL_LINE
        }
        પાછા targetVarp->valuep()->clone_વૃક્ષ(ખોટું);
    }
    શૂન્ય resolveGenericIfaceDotsIn(Ast_ગાંઠ* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ધકેલો_back(dotp); });
        માટે (AstDot* અચલ dotp : dotps) {
            જો (Ast_ગાંઠ* અચલ newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_વૃક્ષ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    શૂન્ય visit(AstVar* nodep) override {
        જો (nodep->user2SetOnce()) પાછા;  // Process once
        // Interface array port: split it into its elements
        જો (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(સાચું);
        }
        // Build cache of interface port names as we encounter them
        જો (nodep->isIfaceRef()) { m_ifacePortNames.ઉમેરો(nodep->name()); }
        iterateChildren(nodep);
        જો (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            જો (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } નહીંતર જો (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                તાર્કિક hasUnresolvedLparamXRef = ખોટું;
                nodep->valuep()->foreach([&](અચલ AstVarXRef* xrefp) {
                    જો (અચલ AstVar* અચલ varp = xrefp->varp()) {
                        જો (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = સાચું;
                        }
                    }
                });
                જો (hasUnresolvedLparamXRef) પાછા;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                અચલ તાર્કિક hasDot = nodep->valuep()->exists([](AstDot*) { પાછા સાચું; });
                તાર્કિક refsDeferred = ખોટું;
                જો (!hasDot) {
                    અચલ auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](અચલ AstVarRef* refp) {
                        જો (refsDeferred) પાછા;
                        AstVar* અચલ refVarp = refp->varp();
                        જો (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.ગણતરી(refVarp)) {
                            refsDeferred = સાચું;
                        }
                    });
                }
                જો (hasDot || refsDeferred) {
                    v3Global.rootp()->ધકેલો_Deferred_Param_Varp(nodep);
                    પાછા;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        જો (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    શૂન્ય visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        જો (nodep->varp()) iterate(nodep->varp());
    }
    તાર્કિક ifaceParamReplace(AstVarXRef* nodep, Ast_ગાંઠ* candp) {
        માટે (; candp; candp = candp->nextp()) {
            જો (nodep->name() == candp->name()) {
                જો (AstVar* અચલ varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    જો (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    પાછા સાચું;
                } નહીંતર જો (અચલ AstPin* અચલ pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_વૃક્ષ(ખોટું));
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા સાચું;
                }
            }
        }
        પાછા ખોટું;
    }

    શૂન્ય deparamIfaceCellNow(AstCell* cellp) {
        જો (!cellp->paramsp()) પાછા;
        જો (!VN_IS(cellp->modp(), Iface)) પાછા;
        Ast_ગાંઠ_Module* અચલ srcModp = cellp->modp();
        Ast_ગાંઠ_Module* અચલ newModp
            = m_processor.ગાંઠ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        જો (newModp && newModp != srcModp) {
            જો (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        જો (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        જો (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarXRef* nodep) override {
        જો (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            પાછા;
        }
        // Parameters of an interface array are accessed through its first element
        જો (nodep->varp() && nodep->varp()->isParam()) {
            અચલ auto it = m_ifaceInstCells.find(nodep->dotted());
            જો (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        અચલ string dotted = nodep->dotted();
        જો (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            અચલ Ast_ગાંઠ* backp = nodep;
            જ્યારે ((backp = backp->backp())) {
                જો (VN_IS(backp, ગાંઠ_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    તોડો;
                }
                જો (અચલ AstVar* અચલ varp = VN_CAST(backp, Var)) {
                    જો (!varp->isIfaceRef()) ચાલુ;
                    // Through all dimensions of an array
                    અચલ AstIfaceRefDType* અચલ ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    જો (!ifacerefp) ચાલુ;
                    // Interfaces passed in on the port map have ifaces
                    જો (અચલ AstIface* અચલ ifacep = ifacerefp->ifacep()) {
                        જો (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            જો (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                પાછા;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    નહીંતર જો (AstCell* અચલ cellp = ifacerefp->cellp()) {
                        જો (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            જો (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                પાછા;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            અચલ auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            જો (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* અચલ cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    શૂન્ય visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        અચલ AstClass* lhsClassp = nullptr;
        અચલ AstClassOrPackageRef* અચલ classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        જો (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ગાંઠ* rhsDefp = nullptr;
        AstClassOrPackageRef* અચલ rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        જો (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        જો (lhsClassp && rhsDefp) {
            m_state.m_dots.ધકેલો_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } નહીંતર {
            iterate(nodep->rhsp());
        }
    }

    શૂન્ય visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* અચલ varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ગાંઠ_FTask_Ref* અચલ taskrefp = VN_CAST(nodep->refp(), ગાંઠ_FTask_Ref);
        AstEnumItemRef* અચલ enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        જો (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } નહીંતર જો (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } નહીંતર જો (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } નહીંતર {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            પાછા;
        }
        iterate(nodep->cellrefp());

        જો (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } નહીંતર જો (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } નહીંતર {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        માટે (Ast_ગાંઠ *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<પૂર્ણાંક> indices;
        માટે (Ast_ગાંઠ* s = nodep->selp(); s; s = s->nextp()) {
            અચલ AstConst* અચલ constp = VN_CAST(s, Const);
            જો (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                પાછા;
            }
            indices.ધકેલો_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        અચલ string viftopSuffix = "__Viftop";
        અચલ string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        અચલ string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        જો (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            પાછા;
        }
        pos += baseName.length();
        માટે (પૂર્ણાંક idx : indices) {
            અચલ string replacement = "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(idx) + "__KET__";
            જો (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                પાછા;
            }
            m_unlinkedTxt.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    શૂન્ય iterateGenerateCond(Ast_ગાંઠ* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = સાચું;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    શૂન્ય visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep->condp(), Const)) {
            જો (Ast_ગાંઠ* અચલ keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } નહીંતર {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            // Normal edit rules will now recurse the replacement
        } નહીંતર {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    શૂન્ય visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        જો (AstGenFor* અચલ forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3Width::widthParamsEdit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            અચલ string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            જો (Ast_ગાંઠ* અચલ stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } નહીંતર {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    શૂન્ય visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        તાર્કિક hit = ખોટું;
        Ast_ગાંઠ* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        માટે (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            માટે (Ast_ગાંઠ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        અચલ AstConst* અચલ exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        માટે (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            માટે (Ast_ગાંઠ* ep = itemp->condsp(); ep;) {
                Ast_ગાંઠ* અચલ nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        માટે (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            જો (!itemp->isDefault()) {
                માટે (Ast_ગાંઠ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    જો (અચલ AstConst* અચલ ccondp = VN_CAST(ep, Const)) {
                        V3_સંખ્યા match{nodep, 1};
                        match.opEq(ccondp->અંક(), exprp->અંક());
                        જો (!hit && match.isNeqZero()) {
                            hit = સાચું;
                            keepp = itemp->itemsp();
                        }
                    } નહીંતર {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        માટે (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            જો (itemp->isDefault()) {
                જો (!hit) {
                    hit = સાચું;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        જો (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = મૂળભૂત;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

વર્ગ Param_ટોચ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    શૂન્ય relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        માટે (AstDot* અચલ dotp : m_state.m_dots) {
            અચલ AstClassOrPackageRef* અચલ classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            અચલ AstClass* અચલ lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* અચલ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            માટે (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                જો (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    તોડો;
                }
            }
        }
    }

    શૂન્ય fixLevel(Ast_ગાંઠ_Module* modp) {
        // Fix up level of module, based on who instantiates it
        જો (modp->user1SetOnce()) પાછા;  // Already fixed
        જો (m_state.m_parentps[modp].empty()) પાછા;  // Leave top levels alone
        પૂર્ણાંક maxParentLevel = 0;
        માટે (Ast_ગાંઠ_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        જો (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    શૂન્ય resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_ગાંઠ_Module*> modps;
        માટે (Ast_ગાંઠ_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ગાંઠ_Module);
            modp->unlinkFrBack();
            modps.ધકેલો_back(modp);
        }

        // Fix-up levels
        અચલ VNUser1InUse user1InUse;
        માટે (Ast_ગાંઠ_Module* અચલ modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](અચલ Ast_ગાંઠ_Module* ap, અચલ Ast_ગાંઠ_Module* bp) {
                             પાછા ap->level() < bp->level();
                         });

        // Re-insert modules
        માટે (Ast_ગાંઠ_Module* અચલ modp : modps) netlistp->addModulesp(modp);
    }

જાહેર:
    // CONSTRUCTORS
    explicit Param_ટોચ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        અચલ ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        અચલ ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ગાંઠ_FTask_Ref* ftaskrefp) {
            Ast_ગાંઠ_FTask* ftaskp = ftaskrefp->taskp();
            જો (!ftaskp || !ftaskp->classMethod()) પાછા;
            અચલ std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            અચલ auto ancestorClassOf = [](Ast_ગાંઠ* startp) -> AstClass* {
                માટે (Ast_ગાંઠ* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    જો (AstClass* અચલ cp = VN_CAST(np, Class)) પાછા cp;
                }
                પાછા nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            જો (refClassp == ftaskrefp->classOrPackagep())
                પાછા;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            જો (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                પાછા;
            }
            જો (classp->user3p()) પાછા;  // will not get removed, no need to relink
            AstClass* અચલ parametrizedClassp = VN_CAST(classp->user4p(), Class);
            જો (!parametrizedClassp) પાછા;
            Ast_ગાંઠ_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ગાંઠ_FTask* ftaskp) {
                જો (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    પાછા સાચું;
                }
                પાછા ખોટું;
            });
            જો (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        અચલ ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        માટે (AstClass* અચલ classp : m_state.m_paramClasses) {
            જો (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                જો (V3Error::error_ગણતરી()) ચાલુ;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ધકેલો_Deletep(classp->unlinkFrBack()), classp);
            } નહીંતર {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(ખોટું);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ગાંઠ::user3_Clear_વૃક્ષ();
        માટે (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ગાંઠ* અચલ nodep) { nodep->user3(સાચું); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ગાંઠ* અચલ nodep) {
            nodep->foreachLink([&](Ast_ગાંઠ** અચલ linkpp, અચલ અક્ષર* namep) {
                જો (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_ટોચ() = મૂળભૂત;
    VL_UNCOPYABLE(Param_ટોચ);
};

//######################################################################
// Param class functions

શૂન્ય V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    જો (dump_વૃક્ષ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_ટોચ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    જો (dump_વૃક્ષ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_વૃક્ષ("param", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    માટે (AstVar* અચલ varp : rootp->deferredParamVarps()) {
        જો (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
