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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Param.h"

#അന്തര്ഭാവ "V3Case.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3HierBlock.h"
#അന്തര്ഭാവ "V3LinkDotIfaceCapture.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Parse.h"
#അന്തര്ഭാവ "V3Simulate.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3Unroll.h"
#അന്തര്ഭാവ "V3Width.h"

#അന്തര്ഭാവ <cctype>
#അന്തര്ഭാവ <cstdlib>
#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
സ്ഥിതം AstClassRefDType* class_Ref_DType_Of_കണ്ണി(Ast_കണ്ണി* nodep) {
    ആകുമ്പോൾ (AstTypedef* സ്ഥിരം tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_കണ്ണി_DType* സ്ഥിരം dtp = VN_CAST(nodep, കണ്ണി_DType);
    തിരികെ dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

വർഗ്ഗം ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, സ്ഥിരം V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<സ്ഥിരം std::string, Ast_കണ്ണി_Module*>;
    using ParamConstMap = std::map<സ്ഥിരം std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<സ്ഥിരം std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    സ്ഥിരം തർക്കം m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<സ്ഥിരം V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<സ്ഥിരം std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<സ്ഥിരം AstVar*> m_untypedParams;

    // METHODS

പൊതു:
    ParameterizedHierBlocks(സ്ഥിരം V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        വേണ്ടി (സ്ഥിരം auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            സ്ഥിരം V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            വേണ്ടി (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    പുതിയ File_വരി{File_വരി::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                സ്ഥിരം തർക്കം inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.തിരുകുക({hierOpt.second.origName(), {}});
        }
        വേണ്ടി (Ast_കണ്ണി_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
            എങ്കിൽ (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            സ്ഥിരം string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            സ്ഥിരം auto defParamIt = m_modParams.find(actualModName);
            എങ്കിൽ (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                        എങ്കിൽ (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        സ്ഥിരം AstBasicDType* സ്ഥിരം dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        എങ്കിൽ (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    തർക്കം hierSubRun() സ്ഥിരം { തിരികെ m_hierSubRun; }
    തർക്കം isHierBlock(സ്ഥിരം string& origName) സ്ഥിരം {
        തിരികെ m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_കണ്ണി_Module* findByParams(സ്ഥിരം string& origName, AstPin* firstPinp,
                                സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        സ്ഥിരം std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        സ്ഥിരം auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        സ്ഥിരം AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        വേണ്ടി (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            തർക്കം found = സത്യം;
            size_t paramIdx = 0;
            സ്ഥിരം ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            വേണ്ടി (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (!pinp->exprp()) തുടരുക;
                എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം modvarp = pinp->modVarp()) {
                    AstConst* സ്ഥിരം constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    സ്ഥിരം auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    സ്ഥിരം തർക്കം untyped = m_untypedParams.എണ്ണം(paramIt->second);
                    AstConst* സ്ഥിരം defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    എങ്കിൽ (defValuep && areSame(constp, defValuep)) {
                        എങ്കിൽ (!untyped || constp->same_മൂല്യം_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            തുടരുക;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    സ്ഥിരം auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    എങ്കിൽ (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = അസത്യം;
                        നിർത്തുക;
                    }
                    എങ്കിൽ (untyped && !constp->same_മൂല്യം_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = അസത്യം;
                        നിർത്തുക;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            എങ്കിൽ (found && paramIdx == hierIt->second->params().size()) നിർത്തുക;
        }
        എങ്കിൽ (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_കണ്ണി::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            തിരികെ nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        സ്ഥിരം HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        സ്ഥിരം auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        എങ്കിൽ (it == m_hierBlockMod.end()) തിരികെ nullptr;
        തിരികെ it->second;
    }
    സ്ഥിതം തർക്കം areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        എങ്കിൽ (pinValuep->isString()) {
            തിരികെ pinValuep->അക്കം().toString() == hierOptParamp->അക്കം().toString();
        }

        എങ്കിൽ (hierOptParamp->isDouble()) {
            ഇരട്ട var;
            എങ്കിൽ (pinValuep->isDouble()) {
                var = pinValuep->അക്കം().toDouble();
            } അല്ലെങ്കിൽ {  // Cast from integer to real
                V3_സംഖ്യ var_അക്കം{pinValuep, V3_സംഖ്യ::Double{}, 0.0};
                var_അക്കം.opIToRD(pinValuep->അക്കം());
                var = var_അക്കം.toDouble();
            }
            തിരികെ V3_സംഖ്യ::epsilonEqual(var, hierOptParamp->അക്കം().toDouble());
        } അല്ലെങ്കിൽ {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_സംഖ്യ var_അക്കം{pinValuep, hierOptParamp->അക്കം().width()};
            എങ്കിൽ (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_അക്കം.opRToIRoundS(pinValuep->അക്കം());
            } അല്ലെങ്കിൽ എങ്കിൽ (pinValuep->isSigned()) {
                var_അക്കം.opExtendS(pinValuep->അക്കം(), pinValuep->അക്കം().width());
            } അല്ലെങ്കിൽ {
                var_അക്കം.opAssign(pinValuep->അക്കം());
            }
            V3_സംഖ്യ isEq{pinValuep, 1};
            isEq.opEq(var_അക്കം, hierOptParamp->അക്കം());
            തിരികെ isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

വർഗ്ഗം ParamProcessor final {
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
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser3InUse m_inuser3;
    സ്ഥിരം VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, Ast_കണ്ണി*>;
    ഘടന ModInfo final {
        Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_കണ്ണി_Module* modp)
            : m_modp{modp} {}
    };
    std::map<സ്ഥിരം std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<സ്ഥിരം std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<സ്ഥിരം std::string, std::string> m_മൂല്യം_Names;  // Parameter value text to its name
    std::set<std::string> m_മൂല്യം_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    സ്ഥിതം constexpr std::string::size_type DIGEST_DIGITS = 8;

    സ്ഥിരം Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_മൂല്യം_Map = std::map<std::string, Ast_കണ്ണി*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_കണ്ണി_Module*, Default_മൂല്യം_Map> m_default_Parameter_മൂല്യങ്ങൾ;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<സ്ഥിരം Ast_കണ്ണി_Module*, സ്ഥിരം AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, പൂർണ്ണസംഖ്യ>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, പൂർണ്ണസംഖ്യ> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<സ്ഥിരം AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    സ്ഥിതം ശൂന്യം makeSmallNames(Ast_കണ്ണി_Module* modp) {
        std::vector<പൂർണ്ണസംഖ്യ> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (varp->isGParam() || varp->isIfaceRef()) {
                    അക്ഷരം ch = varp->name()[0];
                    ch = std::toupper(ch);
                    എങ്കിൽ (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<പൂർണ്ണസംഖ്യ>(ch)] * 256 + ch);
                    usedLetter[static_cast<പൂർണ്ണസംഖ്യ>(ch)]++;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം typep = VN_CAST(stmtp, ParamTypeDType)) {
                സ്ഥിരം അക്ഷരം ch = 'T';
                typep->user3(usedLetter[static_cast<പൂർണ്ണസംഖ്യ>(ch)] * 256 + ch);
                usedLetter[static_cast<പൂർണ്ണസംഖ്യ>(ch)]++;
            }
        }
    }
    സ്ഥിതം string paramSmallName(Ast_കണ്ണി_Module* modp, Ast_കണ്ണി* varp) {
        എങ്കിൽ (varp->user3() <= 1) makeSmallNames(modp);
        പൂർണ്ണസംഖ്യ index = varp->user3() / 256;
        സ്ഥിരം അക്ഷരം ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        ആകുമ്പോൾ (index) {
            st += cvtToStr(static_cast<അക്ഷരം>((index % 25) + 'A'));
            index /= 26;
        }
        തിരികെ st;
    }

    സ്ഥിതം തർക്കം is_Aggregate_Param_മൂല്യം(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    സ്ഥിതം string param_മൂല്യം_String(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (സ്ഥിരം AstRefDType* സ്ഥിരം refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string താക്കോൽ = nodep->name();
        എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            എങ്കിൽ (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                താക്കോൽ = ifrtp->cellp()->modp()->name();
            } അല്ലെങ്കിൽ എങ്കിൽ (ifrtp->ifacep()) {
                താക്കോൽ = ifrtp->ifacep()->name();
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം dtypep
                   = VN_CAST(nodep, കണ്ണി_UOr_Struct_DType)) {
            താക്കോൽ += " ";
            താക്കോൽ += dtypep->verilogKwd();
            താക്കോൽ += " {";
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                താക്കോൽ += param_മൂല്യം_String(memberp);
                താക്കോൽ += ";";
            }
            താക്കോൽ += "}";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberDType* സ്ഥിരം dtypep = VN_CAST(nodep, MemberDType)) {
            താക്കോൽ += " ";
            താക്കോൽ += param_മൂല്യം_String(dtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം dtypep = VN_CAST(nodep, BasicDType)) {
            എങ്കിൽ (dtypep->isSigned()) താക്കോൽ += " signed";
            എങ്കിൽ (dtypep->isRanged()) {
                താക്കോൽ += "[" + cvtToStr(dtypep->ഇടത്()) + ":" + cvtToStr(dtypep->വലത്()) + "]";
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം dtypep = VN_CAST(nodep, PackArrayDType)) {
            താക്കോൽ += "[";
            താക്കോൽ += cvtToStr(dtypep->ഇടത്());
            താക്കോൽ += ":";
            താക്കോൽ += cvtToStr(dtypep->വലത്());
            താക്കോൽ += "] ";
            താക്കോൽ += param_മൂല്യം_String(dtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPattern* സ്ഥിരം patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            താക്കോൽ += "'{";
            വേണ്ടി (സ്ഥിരം AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                എങ്കിൽ (memp->isDefault()) താക്കോൽ += "default";
                എങ്കിൽ (സ്ഥിരം AstText* സ്ഥിരം textp = VN_CAST(memp->keyp(), Text)) {
                    താക്കോൽ += textp->text();
                } അല്ലെങ്കിൽ എങ്കിൽ (memp->keyp()) {
                    താക്കോൽ += param_മൂല്യം_String(memp->keyp());
                }
                താക്കോൽ += ":";
                എങ്കിൽ (memp->repp()) താക്കോൽ += param_മൂല്യം_String(memp->repp()) + "x";
                വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
                    താക്കോൽ += param_മൂല്യം_String(valuep) + ";";
                }
                താക്കോൽ += ",";
            }
            താക്കോൽ += "}";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            താക്കോൽ += "{";
            വേണ്ടി (സ്ഥിരം AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                താക്കോൽ += cvtToStr(itemp->index()) + ":" + param_മൂല്യം_String(itemp->valuep()) + ",";
            }
            എങ്കിൽ (consArrayp->defaultp()) {
                താക്കോൽ += "default:" + param_മൂല്യം_String(consArrayp->defaultp()) + ",";
            }
            താക്കോൽ += "}";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConsStruct* സ്ഥിരം consp = VN_CAST(nodep, ConsStruct)) {
            താക്കോൽ += "{";
            വേണ്ടി (സ്ഥിരം AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                താക്കോൽ += param_മൂല്യം_String(itemp->valuep());
                താക്കോൽ += ",";
            }
            താക്കോൽ += "}";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            എങ്കിൽ (classRefp->classp()) {
                സ്ഥിരം string& className = classRefp->classp()->name();
                സ്ഥിരം string& origName = classRefp->classp()->origName();
                സ്ഥിരം തർക്കം isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                എങ്കിൽ (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } അല്ലെങ്കിൽ എങ്കിൽ (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    താക്കോൽ = className;
                } അല്ലെങ്കിൽ {
                    // Unspecialized class with no params - use origName
                    താക്കോൽ = origName;
                }
            } അല്ലെങ്കിൽ {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            താക്കോൽ = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstQueueDType* സ്ഥിരം dtypep = VN_CAST(nodep, QueueDType)) {
            താക്കോൽ = paramElemString(dtypep->subDTypep()) + "$[$";
            എങ്കിൽ (dtypep->boundConst()) താക്കോൽ += ":" + cvtToStr(dtypep->boundConst());
            താക്കോൽ += "]";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം dtypep = VN_CAST(nodep, DynArrayDType)) {
            താക്കോൽ = paramElemString(dtypep->subDTypep()) + "$[]";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstAssocArrayDType* സ്ഥിരം dtypep = VN_CAST(nodep, AssocArrayDType)) {
            താക്കോൽ = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->താക്കോൽ_DTypep()) + "]";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstWildcardArrayDType* സ്ഥിരം dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            താക്കോൽ = paramElemString(dtypep->subDTypep()) + "$[*]";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstEnumDType* സ്ഥിരം dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            താക്കോൽ += " enum {";
            വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(itemp->valuep(), Const);
                താക്കോൽ += itemp->name() + "=" + (constp ? constp->അക്കം().ascii() : "?") + ";";
            }
            താക്കോൽ += "}";
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = VN_CAST(nodep, കണ്ണി_DType)) {
            താക്കോൽ += dtypep->prettyDTypeName(സത്യം);
        }
        UASSERT_OBJ(!താക്കോൽ.empty(), nodep, "Parameter yielded no value string");
        തിരികെ താക്കോൽ;
    }
    // As paramValueString(), for the element or key type of a container
    സ്ഥിതം string paramElemString(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        എങ്കിൽ (classRefp && classRefp->paramsp()) തിരികെ classRefp->prettyDTypeName(സത്യം);
        തിരികെ param_മൂല്യം_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    സ്ഥിതം string digestSuffix(സ്ഥിരം string& text, std::set<string>& usedr) {
        സ്ഥിരം string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        ആകുമ്പോൾ (digits < hex.size() && !usedr.തിരുകുക(hex.substr(0, digits)).second) ++digits;
        തിരികെ hex.substr(0, digits);
    }
    string param_മൂല്യം_സംഖ്യ(Ast_കണ്ണി* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        എങ്കിൽ (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        സ്ഥിരം string text = param_മൂല്യം_String(nodep);
        സ്ഥിരം auto pair = m_മൂല്യം_Names.emplace(text, "");
        എങ്കിൽ (pair.second) pair.first->second = "z" + digestSuffix(text, m_മൂല്യം_Suffixes);
        തിരികെ pair.first->second;
    }
    string moduleCalcName(സ്ഥിരം Ast_കണ്ണി_Module* srcModp, സ്ഥിരം string& longname) {
        string newname = longname;
        എങ്കിൽ (longname.length() > 30) {
            സ്ഥിരം auto pair = m_longMap.emplace(longname, "");
            എങ്കിൽ (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        തിരികെ newname;
    }
    സ്ഥിതം Ast_കണ്ണി_DType* arraySubDTypep(Ast_കണ്ണി_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            തിരികെ adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        എങ്കിൽ (സ്ഥിരം AstBracketArrayDType* സ്ഥിരം adtypep = VN_CAST(nodep, BracketArrayDType)) {
            തിരികെ adtypep->subDTypep();
        }
        തിരികെ nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    സ്ഥിതം Ast_കണ്ണി_DType* arraySubDTypeDeepp(Ast_കണ്ണി_DType* nodep) {
        ആകുമ്പോൾ (Ast_കണ്ണി_DType* സ്ഥിരം subp = arraySubDTypep(nodep)) nodep = subp;
        തിരികെ nodep;
    }
    സ്ഥിതം തർക്കം isString(Ast_കണ്ണി_DType* nodep) {
        എങ്കിൽ (AstBasicDType* സ്ഥിരം basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            തിരികെ basicp->isString();
        തിരികെ അസത്യം;
    }
    ശൂന്യം collectPins(CloneMap* clonemapp, Ast_കണ്ണി_Module* modp, തർക്കം originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            സ്ഥിരം Ast_കണ്ണി* originalParamp = nullptr;
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം ptp = VN_CAST(stmtp, ParamTypeDType)) {
                എങ്കിൽ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            എങ്കിൽ (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            എങ്കിൽ (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    ശൂന്യം relinkPins(സ്ഥിരം CloneMap* clonemapp, AstPin* startpinp) {
        വേണ്ടി (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                സ്ഥിരം auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } അല്ലെങ്കിൽ എങ്കിൽ (pinp->modPTypep()) {
                സ്ഥിരം auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } അല്ലെങ്കിൽ {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    ശൂന്യം relinkPinsByName(AstPin* startpinp, Ast_കണ്ണി_Module* modp) {
        std::map<സ്ഥിരം string, AstVar*> nameToPin;
        വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        വേണ്ടി (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = pinp->modVarp()) {
                സ്ഥിരം auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    സ്ഥിതം തർക്കം hasDescendantDefparams(സ്ഥിരം Ast_കണ്ണി_Module* modp,
                                       std::set<സ്ഥിരം Ast_കണ്ണി_Module*>& visited) {
        എങ്കിൽ (!visited.തിരുകുക(modp).second) തിരികെ അസത്യം;
        വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* സ്ഥിരം cellp = VN_CAST(stmtp, Cell);
            എങ്കിൽ (!cellp) തുടരുക;
            വേണ്ടി (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    തിരികെ സത്യം;
                }
            }
            എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം childModp = cellp->modp()) {
                എങ്കിൽ (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    തിരികെ സത്യം;
                }
            }
        }
        visited.erase(modp);
        തിരികെ അസത്യം;
    }
    സ്ഥിതം തർക്കം hasDescendantDefparams(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        std::set<സ്ഥിരം Ast_കണ്ണി_Module*> visited;
        തിരികെ hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    സ്ഥിരം AstPin* nestedDefparamp(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        സ്ഥിരം auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        എങ്കിൽ (!pair.second) തിരികെ pair.first->second;  // Searched, or being searched
        സ്ഥിരം AstPin* foundp = nullptr;
        modp->exists([&](സ്ഥിരം AstCell* cellp) {
            വേണ്ടി (സ്ഥിരം AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (!pinp->paramPath().empty()) foundp = pinp;
            }
            എങ്കിൽ (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            തിരികെ foundp != nullptr;
        });
        pair.first->second = foundp;
        തിരികെ foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    ശൂന്യം checkSupportedParam(Ast_കണ്ണി_Module* modp, AstPin* pinp) സ്ഥിരം {
        // ConsArray is not supported because that can not be set via -G
        // option.
        എങ്കിൽ (pinp->modVarp()) {
            തർക്കം supported = അസത്യം;
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(pinp->exprp(), Const)) {
                supported = സത്യം;
                എങ്കിൽ (constp->അക്കം().isString()
                    && !V3HierBlock::stringParamPassable(constp->അക്കം().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            എങ്കിൽ (!supported) {
                pinp->v3error(
                    Ast_കണ്ണി::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstParamTypeDType* സ്ഥിരം typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_കണ്ണി::prettyNameQ(modp->origName()));
        }
    }
    തർക്കം moduleExists(സ്ഥിരം string& modName) സ്ഥിരം {
        എങ്കിൽ (m_allModuleNames.find(modName) != m_allModuleNames.end()) തിരികെ സത്യം;
        എങ്കിൽ (m_modNameMap.find(modName) != m_modNameMap.end()) തിരികെ സത്യം;
        തിരികെ അസത്യം;
    }

    string parameterizedHierBlockName(Ast_കണ്ണി_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_കണ്ണി*> pins;

        AstPin* pinp = paramPinsp;
        ആകുമ്പോൾ (pinp) {
            checkSupportedParam(modp, pinp);
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = pinp->modVarp()) {
                എങ്കിൽ (!pinp->exprp()) തുടരുക;
                എങ്കിൽ (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), കണ്ണി_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        സ്ഥിരം auto pair = m_default_Parameter_മൂല്യങ്ങൾ.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        എങ്കിൽ (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            എങ്കിൽ (സ്ഥിരം AstPin* സ്ഥിരം defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_കണ്ണി::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_മൂല്യം_Map params;
            വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                    എങ്കിൽ (varp->isGParam()) {
                        AstConst* സ്ഥിരം constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_കണ്ണി* സ്ഥിരം dtypep = static_cast<Ast_കണ്ണി*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        സ്ഥിരം auto paramsIt = pair.first;
        എങ്കിൽ (paramsIt->second.empty()) തിരികെ modp->origName();  // modp has no parameter

        string longname = modp->origName();
        വേണ്ടി (auto&& default_മൂല്യം : paramsIt->second) {
            സ്ഥിരം auto pinIt = pins.find(default_മൂല്യം.first);
            // If the pin does not have a value assigned, use the default one.
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep = pinIt == pins.end() ? default_മൂല്യം.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_മൂല്യം.first + "=";
            // constp can be nullptr

            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->അക്കം().ascii(അസത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_വൃക്ഷം(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        സ്ഥിരം auto iter = m_longMap.find(longname);
        എങ്കിൽ (iter != m_longMap.end()) തിരികെ iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.തിരുകുക(longname);
        ആകുമ്പോൾ (സത്യം) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            സ്ഥിരം string hashStr = hashStrGen.digestHex();
            വേണ്ടി (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                എങ്കിൽ (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                എങ്കിൽ (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    തിരികെ newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.തിരുകുക(V3Os::trueRandom(64));
        }
    }
    ശൂന്യം replaceRefsRecurse(Ast_കണ്ണി* സ്ഥിരം nodep, സ്ഥിരം AstClass* സ്ഥിരം oldClassp,
                            AstClass* സ്ഥിരം newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(nodep, ClassRefDType)) {
            എങ്കിൽ (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            എങ്കിൽ (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstTypedef* സ്ഥിരം typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            എങ്കിൽ (typedefp->subDTypep()) {
                എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    എങ്കിൽ (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_FTask_Ref* സ്ഥിരം ftaskRefp = VN_CAST(nodep, കണ്ണി_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            എങ്കിൽ (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം oldTaskp = ftaskRefp->taskp()) {
                എങ്കിൽ (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        എങ്കിൽ (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        എങ്കിൽ (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        എങ്കിൽ (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        എങ്കിൽ (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        എങ്കിൽ (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    വർഗ്ഗം VarXRefRelinkVisitor final : പൊതു VNVisitor {
        Ast_കണ്ണി_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_കണ്ണി_Module*> m_varModuleMap;  // Cache var->module lookups

    പൊതു:
        explicit VarXRefRelinkVisitor(Ast_കണ്ണി_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    സ്വകാര്യ:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_കണ്ണി_Module* findVarModule(AstVar* varp) {
            സ്ഥിരം auto it = m_varModuleMap.find(varp);
            എങ്കിൽ (it != m_varModuleMap.end()) തിരികെ it->second;
            Ast_കണ്ണി_Module* varModp = nullptr;
            വേണ്ടി (Ast_കണ്ണി* np = varp; np; np = np->backp()) {
                എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(np, കണ്ണി_Module)) {
                    varModp = modp;
                    നിർത്തുക;
                }
            }
            m_varModuleMap[varp] = varModp;
            തിരികെ varModp;
        }

        ശൂന്യം visit(AstVarXRef* nodep) override {
            AstVar* സ്ഥിരം varp = nodep->varp();
            എങ്കിൽ (!varp) {
                iterateChildren(nodep);
                തിരികെ;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            സ്ഥിരം string& dotted = nodep->dotted();
            എങ്കിൽ (dotted.empty()) {
                iterateChildren(nodep);
                തിരികെ;
            }

            സ്ഥിരം size_t dotPos = dotted.find('.');
            സ്ഥിരം string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            എങ്കിൽ (portName.empty()) {
                iterateChildren(nodep);
                തിരികെ;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            വേണ്ടി (Ast_കണ്ണി* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (AstVar* സ്ഥിരം varChkp = VN_CAST(stmtp, Var)) {
                    എങ്കിൽ (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        നിർത്തുക;
                    }
                }
            }
            എങ്കിൽ (!portVarp) {
                iterateChildren(nodep);
                തിരികെ;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* സ്ഥിരം irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            എങ്കിൽ (!irefp) {
                iterateChildren(nodep);
                തിരികെ;
            }

            Ast_കണ്ണി_Module* സ്ഥിരം newIfacep = irefp->ifaceViaCellp();
            എങ്കിൽ (!newIfacep) {
                iterateChildren(nodep);
                തിരികെ;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_കണ്ണി_Module* സ്ഥിരം varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            എങ്കിൽ (varModp && varModp != newIfacep) {
                വേണ്ടി (Ast_കണ്ണി* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    എങ്കിൽ (AstVar* സ്ഥിരം newVarp = VN_CAST(stmtp, Var)) {
                        എങ്കിൽ (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            നിർത്തുക;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    ശൂന്യം fixupCrossModuleRefDTypes(Ast_കണ്ണി_Module* newModp) {
        എങ്കിൽ (!V3LinkDotIfaceCapture::enabled()) തിരികെ;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        പൂർണ്ണസംഖ്യ fixed_എണ്ണം = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            സ്ഥിരം VIfaceCaptureTag* സ്ഥിരം tagp = V3LinkDotIfaceCapture::captureTag(refp);
            എങ്കിൽ (!tagp) തിരികെ;

            Ast_കണ്ണി_Module* സ്ഥിരം correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            എങ്കിൽ (!correctModp || correctModp->dead()) തിരികെ;
            എങ്കിൽ (correctModp->parameterizedTemplate()) തിരികെ;

            തർക്കം fixed = അസത്യം;
            എങ്കിൽ (refp->typedefp()) {
                എങ്കിൽ (AstTypedef* സ്ഥിരം newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    എങ്കിൽ (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = സത്യം;
                }
            }
            എങ്കിൽ (!fixed && refp->refDTypep()) {
                എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = സത്യം;
                }
            }
            എങ്കിൽ (fixed) ++fixed_എണ്ണം;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_എണ്ണം);
    }

    // Return true on success, false on error
    തർക്കം deepCloneModule(Ast_കണ്ണി_Module* srcModp, Ast_കണ്ണി* ifErrorp, AstPin* paramsp,
                         സ്ഥിരം string& newname, സ്ഥിരം IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_കണ്ണി_Module* newModp;
        എങ്കിൽ (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_വൃക്ഷം(അസത്യം), കണ്ണി_Module);
        } അല്ലെങ്കിൽ {
            newModp = srcModp->clone_വൃക്ഷം(അസത്യം);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        എങ്കിൽ (AstIface* സ്ഥിരം newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(സത്യം);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(അസത്യം);

        newModp->name(newname);
        newModp->user2(അസത്യം);  // We need to re-recurse this module once changed
        newModp->recursive(അസത്യം);
        newModp->recursiveClone(അസത്യം);
        newModp->hasGParam(അസത്യം);
        // Recursion may need level cleanups
        എങ്കിൽ (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        എങ്കിൽ ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_വൃക്ഷം(), newModp);
            തിരികെ അസത്യം;
        }

        എങ്കിൽ (AstClass* സ്ഥിരം newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_കണ്ണി* insertp = srcModp;
        ആകുമ്പോൾ (VN_IS(insertp->nextp(), കണ്ണി_Module)
               && VN_AS(insertp->nextp(), കണ്ണി_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        സ്ഥിരം auto iter = m_modNameMap.find(newname);
        CloneMap* സ്ഥിരം clonemapp = &(iter->second.m_cloneMap);
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
        തർക്കം needRelinkByName = അസത്യം;
        എങ്കിൽ (paramsp) {
            വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = സത്യം;
                    നിർത്തുക;
                }
            }
        }
        എങ്കിൽ (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } അല്ലെങ്കിൽ {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        വേണ്ടി (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            സ്ഥിരം AstIfaceRefDType* സ്ഥിരം portIrefp = it->first;
            സ്ഥിരം AstIfaceRefDType* സ്ഥിരം pinIrefp = it->second;
            AstIfaceRefDType* സ്ഥിരം cloneIrefp = portIrefp->clonep();
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
        എങ്കിൽ (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (pinp->exprp()) {
                എങ്കിൽ (AstVar* സ്ഥിരം modvarp = pinp->modVarp()) {
                    Ast_കണ്ണി* സ്ഥിരം newp = pinp->exprp();  // Const or ConsArray
                    AstConst* സ്ഥിരം exprp = VN_CAST(newp, Const);
                    AstConst* സ്ഥിരം origp = VN_CAST(modvarp->valuep(), Const);
                    സ്ഥിരം തർക്കം overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    എങ്കിൽ (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_വൃക്ഷം();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_വൃക്ഷം(അസത്യം));
                    modvarp->overriddenParam(overridden);
                } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം modptp = pinp->modPTypep()) {
                    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = VN_AS(pinp->exprp(), കണ്ണി_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    എങ്കിൽ (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_വൃക്ഷം();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_വൃക്ഷം(അസത്യം));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        തിരികെ സത്യം;
    }
    സ്ഥിരം ModInfo* moduleFindOrClone(Ast_കണ്ണി_Module* srcModp, Ast_കണ്ണി* ifErrorp, AstPin* paramsp,
                                     സ്ഥിരം string& newname, സ്ഥിരം IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        എങ്കിൽ (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                തിരികെ nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        സ്ഥിരം ModInfo* സ്ഥിരം modInfop = &(it->second);
        തിരികെ modInfop;
    }

    ശൂന്യം convertToStringp(Ast_കണ്ണി* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        എങ്കിൽ (constp && !constp->അക്കം().isString()) {
            constp->replaceWith(
                പുതിയ AstConst{constp->fileline(), AstConst::String{}, constp->അക്കം().toString()});
            VL_DO_DANGLING(constp->delete_വൃക്ഷം(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    ശൂന്യം constify_Member_മൂല്യം(AstVar* varp, std::set<AstVar*>& inProgress) {
        എങ്കിൽ (!varp->valuep() || VN_IS(varp->valuep(), Const)) തിരികെ;
        എങ്കിൽ (!inProgress.തിരുകുക(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_വൃക്ഷം();
            varp->valuep(പുതിയ AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            തിരികെ;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](സ്ഥിരം AstVarRef* refp) {
            AstVar* സ്ഥിരം refVarp = refp->varp();
            എങ്കിൽ (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.തിരുകുക(refVarp);
            }
        });
        വേണ്ടി (AstVar* സ്ഥിരം sibp : siblings) constify_Member_മൂല്യം(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    സ്ഥിതം AstParseRef* memberParseRef(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (AstParseRef* സ്ഥിരം refp = VN_CAST(nodep, ParseRef)) തിരികെ refp;
        Ast_കണ്ണി_Pre_Sel* സ്ഥിരം selp = VN_CAST(nodep, കണ്ണി_Pre_Sel);
        തിരികെ selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    സ്ഥിതം ശൂന്യം replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        എങ്കിൽ (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_Pre_Sel* സ്ഥിരം selp = VN_AS(dotp->rhsp(), കണ്ണി_Pre_Sel);
            എങ്കിൽ (AstAttrOf* സ്ഥിരം attrp = selp->attrp()) {
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_വൃക്ഷം(അസത്യം));
                    VL_DO_DANGLING(oldFromp->delete_വൃക്ഷം(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_വൃക്ഷം(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_വൃക്ഷം(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    സ്ഥിതം തർക്കം isDeferredMemberBase(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (VN_IS(nodep, MemberSel)) തിരികെ സത്യം;
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Pre_Sel* സ്ഥിരം selp = VN_CAST(nodep, കണ്ണി_Pre_Sel)) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDTypep = selp->fromp()->dtypep();
            സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), കണ്ണി_Array_DType) : nullptr;
            എങ്കിൽ (arrayDTypep) {
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDTypep = arrayDTypep->subDTypep();
                തിരികെ elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), കണ്ണി_UOr_Struct_DType);
            }
            തിരികെ isDeferredMemberBase(selp->fromp());
        }
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->dtypep();
        തിരികെ dtypep && VN_IS(dtypep->skipRefOrNullp(), കണ്ണി_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    സ്ഥിതം ശൂന്യം resolveMemberDot(AstDot* dotp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = VN_CAST(dotp->lhsp(), കണ്ണി_വ്യഞ്ജകം);
        AstParseRef* സ്ഥിരം memberRefp = memberParseRef(dotp->rhsp());
        എങ്കിൽ (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) തിരികെ;
        AstMemberSel* സ്ഥിരം newp = പുതിയ AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    ശൂന്യം resolveDotToTypedef(Ast_കണ്ണി* exprp) {
        AstDot* സ്ഥിരം dotp = VN_CAST(exprp, Dot);
        എങ്കിൽ (!dotp) തിരികെ;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* സ്ഥിരം classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        എങ്കിൽ (!classRefp) {
            resolveMemberDot(dotp);
            തിരികെ;
        }
        AstParseRef* സ്ഥിരം parseRefp = memberParseRef(dotp->rhsp());
        എങ്കിൽ (!parseRefp) തിരികെ;

        സ്ഥിരം AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* സ്ഥിരം paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        എങ്കിൽ (paramTypep) {
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = paramTypep->subDTypep();
            AstClassRefDType* സ്ഥിരം classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            എങ്കിൽ (classRefDTypep) {
                AstClass* സ്ഥിരം srcClassp = classRefDTypep->classp();
                എങ്കിൽ (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    എങ്കിൽ (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        എങ്കിൽ (classRefp->paramsp()) {
            AstClass* സ്ഥിരം srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            എങ്കിൽ (srcClassp && srcClassp->hasGParam()) {
                എങ്കിൽ (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        എങ്കിൽ (AstClassRefDType* സ്ഥിരം crp = class_Ref_DType_Of_കണ്ണി(classRefp->classOrPackageNodep())) {
            എങ്കിൽ (AstClass* സ്ഥിരം srcClassp = crp->classp()) {
                എങ്കിൽ (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    എങ്കിൽ (AstClass* സ്ഥിരം newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        എങ്കിൽ (!lhsClassp) തിരികെ;

        Ast_കണ്ണി* സ്ഥിരം memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        എങ്കിൽ (AstTypedef* സ്ഥിരം tdefp = VN_CAST(memberp, Typedef)) {
            എങ്കിൽ (parseRefp != dotp->rhsp()) തിരികെ;
            AstRefDType* സ്ഥിരം refp = പുതിയ AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_വൃക്ഷം(), dotp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    ശൂന്യം substituteParamMember(AstDot* സ്ഥിരം dotp, AstParseRef* സ്ഥിരം memberRefp,
                               AstVar* സ്ഥിരം varp) {
        എങ്കിൽ (!varp->isParam() || !varp->valuep()) തിരികെ;
        എങ്കിൽ (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_മൂല്യം(varp, inProgress);
        }
        AstConst* സ്ഥിരം constp = VN_CAST(varp->valuep(), Const);
        എങ്കിൽ (!constp) തിരികെ;
        AstConst* സ്ഥിരം newp = constp->clone_വൃക്ഷം(അസത്യം);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    ശൂന്യം resolveParamClassRefDType(AstRefDType* refp) {
        എങ്കിൽ (refp->typedefp() || refp->refDTypep()) തിരികെ;

        AstClassOrPackageRef* സ്ഥിരം classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        എങ്കിൽ (!classRefp) തിരികെ;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        എങ്കിൽ (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            തിരികെ;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_കണ്ണി* targetp = classRefp->classOrPackageNodep();
        ആകുമ്പോൾ (സ്ഥിരം AstTypedef* സ്ഥിരം tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtargetp = VN_CAST(targetp, കണ്ണി_DType))
            targetp = dtargetp->skipRefOrNullp();
        എങ്കിൽ (!targetp) തിരികെ;
        AstClassRefDType* സ്ഥിരം classRefDTypep = VN_CAST(targetp, ClassRefDType);
        എങ്കിൽ (!classRefDTypep) തിരികെ;
        srcClassp = classRefDTypep->classp();
        എങ്കിൽ (!srcClassp) തിരികെ;

        AstClass* newClassp = srcClassp;
        എങ്കിൽ (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* സ്ഥിരം typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        എങ്കിൽ (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    സ്ഥിതം തർക്കം defaultParamsHaveTypeQueryOnParamType(സ്ഥിരം AstClassRefDType* nodep) {
        തർക്കം found = അസത്യം;
        nodep->foreach([&](സ്ഥിരം AstAttrOf* attrp) {
            എങ്കിൽ (found || !attrp->attrType().isTypeQuery()) തിരികെ;
            സ്ഥിരം AstRefDType* സ്ഥിരം refp = VN_CAST(attrp->fromp(), RefDType);
            എങ്കിൽ (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = സത്യം;
        });
        തിരികെ found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    തർക്കം classTypeMatchesDefaultClone(സ്ഥിരം Ast_കണ്ണി_DType* exprp, സ്ഥിരം Ast_കണ്ണി_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        സ്ഥിരം auto* സ്ഥിരം വ്യഞ്ജകം_Class_Refp = VN_CAST(exprp, ClassRefDType);
        സ്ഥിരം auto* സ്ഥിരം origClassRefp = VN_CAST(origp, ClassRefDType);
        എങ്കിൽ (!വ്യഞ്ജകം_Class_Refp || !origClassRefp) തിരികെ അസത്യം;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        എങ്കിൽ (defaultClonep && defaultClonep == വ്യഞ്ജകം_Class_Refp->classp()) തിരികെ സത്യം;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        എങ്കിൽ (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) തിരികെ അസത്യം;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        എങ്കിൽ (!origClassRefp->classp()->hasGParam()) തിരികെ അസത്യം;
        എങ്കിൽ (origClassRefp->classp()->origName() != വ്യഞ്ജകം_Class_Refp->classp()->origName())
            തിരികെ അസത്യം;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        എങ്കിൽ (!m_defaultCloneInProgress.തിരുകുക(origClassRefp->classp()).second) തിരികെ അസത്യം;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* സ്ഥിരം origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_വൃക്ഷം(അസത്യം));
        Ast_കണ്ണി_Module* സ്ഥിരം resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        സ്ഥിരം തർക്കം match = resolvedModp && VN_CAST(resolvedModp, Class) == വ്യഞ്ജകം_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_വൃക്ഷം(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        തിരികെ match;
    }

    സ്ഥിതം തർക്കം paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        എങ്കിൽ (exprp->അക്കം().width() == origp->അക്കം().width()) തിരികെ അസത്യം;
        എങ്കിൽ (exprp->അക്കം().isOpaque()) തിരികെ അസത്യം;
        എങ്കിൽ (origp->അക്കം().isOpaque()) തിരികെ അസത്യം;
        സ്ഥിരം പൂർണ്ണസംഖ്യ maxWidth = std::max(exprp->അക്കം().width(), origp->അക്കം().width());
        V3_സംഖ്യ വ്യഞ്ജകം_അക്കം{exprp, maxWidth};
        എങ്കിൽ (exprp->അക്കം().isSigned()) {
            വ്യഞ്ജകം_അക്കം.opExtendS(exprp->അക്കം(), exprp->അക്കം().width());
        } അല്ലെങ്കിൽ {
            വ്യഞ്ജകം_അക്കം.opAssign(exprp->അക്കം());
        }
        V3_സംഖ്യ orig_അക്കം{origp, maxWidth};
        എങ്കിൽ (origp->അക്കം().isSigned()) {
            orig_അക്കം.opExtendS(origp->അക്കം(), origp->അക്കം().width());
        } അല്ലെങ്കിൽ {
            orig_അക്കം.opAssign(origp->അക്കം());
        }
        V3_സംഖ്യ isEq{exprp, 1};
        isEq.opEq(വ്യഞ്ജകം_അക്കം, orig_അക്കം);
        തിരികെ isEq.isNeqZero();
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
    ശൂന്യം resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (!pinp->exprp()) തുടരുക;
            എങ്കിൽ (!VString::startsWith(pinp->name(), "__VGIfaceParam")) തുടരുക;
            സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം fwdRefp = VN_CAST(pinp->exprp(), കണ്ണി_Var_Ref);
            എങ്കിൽ (!fwdRefp) തുടരുക;
            സ്ഥിരം AstIfaceRefDType* സ്ഥിരം resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* സ്ഥിരം newIrefp = പുതിയ AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            എങ്കിൽ (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_വൃക്ഷം(സത്യം));
            }
            pinp->exprp()->unlinkFrBack()->delete_വൃക്ഷം();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    ശൂന്യം name_By_Pin_മൂല്യം(AstPin* pinp, Ast_കണ്ണി_Module* srcModp, AstVar* modvarp, string& longnamer,
                        തർക്കം& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_മൂല്യം_സംഖ്യ(pinp->exprp());
        any_overridesr = സത്യം;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    ശൂന്യം cellPinCleanup(Ast_കണ്ണി* nodep, AstPin* pinp, AstPin* paramsp, Ast_കണ്ണി_Module* srcModp,
                        string& longnamer, തർക്കം& any_overridesr,
                        std::unordered_set<സ്ഥിരം AstPin*>& overridingTypePinsr) {
        എങ്കിൽ (!pinp->exprp()) തിരികെ;  // No-connect
        എങ്കിൽ (AstVar* സ്ഥിരം modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            എങ്കിൽ (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_മൂല്യം(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } അല്ലെങ്കിൽ {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                എങ്കിൽ (is_Aggregate_Param_മൂല്യം(pinp->exprp())) {
                    name_By_Pin_മൂല്യം(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    തിരികെ;
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
                എങ്കിൽ (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    സ്ഥിരം തർക്കം dependent = modvarp->valuep()->exists(
                        [](Ast_കണ്ണി* np) { തിരികെ VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    എങ്കിൽ (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                എങ്കിൽ (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* സ്ഥിരം exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* സ്ഥിരം origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                എങ്കിൽ (exprp && !exprp->അക്കം().isDouble() && !exprp->അക്കം().isString()) {
                    AstVar* cloneVarp = modvarp->clone_വൃക്ഷം(അസത്യം);
                    തർക്കം cloneVarpUnresolved = അസത്യം;
                    എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_വൃക്ഷം(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_വൃക്ഷം(അസത്യം));
                    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        എങ്കിൽ (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_വൃക്ഷം();
                        cloneVarp->childDTypep(origDTypep->clone_വൃക്ഷം(അസത്യം));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr പൂർണ്ണസംഖ്യ maxSubstIters = 1000;
                        വേണ്ടി (പൂർണ്ണസംഖ്യ it = 0; it < maxSubstIters; ++it) {
                            തർക്കം any = അസത്യം;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* സ്ഥിരം targetp = varrefp->varp();
                                Ast_കണ്ണി* replacep = nullptr;
                                വേണ്ടി (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    എങ്കിൽ (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        എങ്കിൽ (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_വൃക്ഷം(അസത്യം);
                                        }
                                        നിർത്തുക;
                                    }
                                }
                                എങ്കിൽ (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_വൃക്ഷം(അസത്യം);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* സ്ഥിരം patp = VN_CAST(replacep, Pattern);
                                എങ്കിൽ (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_വൃക്ഷം(അസത്യം));
                                }
                                എങ്കിൽ (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_വൃക്ഷം(), varrefp);
                                    any = സത്യം;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_കണ്ണി_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* സ്ഥിരം ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                എങ്കിൽ (!ptdp) തിരികെ;
                                AstPin* overridePinp = nullptr;
                                വേണ്ടി (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    എങ്കിൽ (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        നിർത്തുക;
                                    }
                                }
                                Ast_കണ്ണി_DType* സ്ഥിരം substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), കണ്ണി_DType)
                                                   : ptdp->subDTypep();
                                എങ്കിൽ (substp) toReplace.emplace_back(refp, substp);
                            });
                            വേണ്ടി (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* സ്ഥിരം refp = it->first;
                                refp->replaceWith(it->second->clone_വൃക്ഷം(അസത്യം));
                                VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
                                any = സത്യം;
                            }
                            എങ്കിൽ (!any) നിർത്തുക;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_കണ്ണി* np) {
                            എങ്കിൽ (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = സത്യം;
                                തിരികെ;
                            }
                            എങ്കിൽ (AstEnumItemRef* സ്ഥിരം itemRefp = VN_CAST(np, EnumItemRef)) {
                                എങ്കിൽ (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = സത്യം;
                                }
                                തിരികെ;
                            }
                            AstRefDType* സ്ഥിരം refp = VN_CAST(np, RefDType);
                            എങ്കിൽ (!refp) തിരികെ;
                            എങ്കിൽ (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = സത്യം;
                            }
                            // A type declared in the template must not be widthed there
                            എങ്കിൽ (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = സത്യം;
                            }
                        });
                    }
                    എങ്കിൽ (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        എങ്കിൽ (AstConst* സ്ഥിരം widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            എങ്കിൽ (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_വൃക്ഷം(), cloneVarp);
                }
                AstConst* സ്ഥിരം namingExprp = normedNamep ? normedNamep : exprp;
                എങ്കിൽ (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_കണ്ണി* സ്ഥിരം pinExprp = pinp->exprp();
                    pinExprp->replaceWith(പുതിയ AstConst{pinp->fileline(), AstConst::Widthed_മൂല്യം{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_വൃക്ഷം(), pinExprp);
                } അല്ലെങ്കിൽ എങ്കിൽ (origp
                           && (exprp->same_വൃക്ഷം(origp)
                               || (exprp->അക്കം().width() == origp->അക്കം().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } അല്ലെങ്കിൽ എങ്കിൽ (namingExprp->അക്കം().isDouble() || namingExprp->അക്കം().isString()
                           || namingExprp->അക്കം().isFourState()
                           || namingExprp->അക്കം().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_മൂല്യം_സംഖ്യ(namingExprp));
                    any_overridesr = സത്യം;
                } അല്ലെങ്കിൽ {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->അക്കം().ascii(അസത്യം));
                    any_overridesr = സത്യം;
                }
                എങ്കിൽ (normedNamep) VL_DO_DANGLING(normedNamep->delete_വൃക്ഷം(), normedNamep);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_കണ്ണി_DType* rawTypep = VN_CAST(pinp->exprp(), കണ്ണി_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            തർക്കം skipWidthForTemplateStruct = അസത്യം;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_കണ്ണി_DType* സ്ഥിരം resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                എങ്കിൽ (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_കണ്ണി_Module* സ്ഥിരം ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    എങ്കിൽ (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = സത്യം;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                എങ്കിൽ (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_കണ്ണി_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            സ്ഥിരം Ast_കണ്ണി_DType* origp = modvarp->skipRefToNonRefp();
            എങ്കിൽ (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } അല്ലെങ്കിൽ എങ്കിൽ (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } അല്ലെങ്കിൽ {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                എങ്കിൽ (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), കണ്ണി_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                എങ്കിൽ (!modvarp->fwdType().is_കണ്ണി_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                എങ്കിൽ (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), കണ്ണി_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        എങ്കിൽ (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } അല്ലെങ്കിൽ {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    ശൂന്യം cellInterfaceCleanup(AstPin* pinsp, Ast_കണ്ണി_Module* srcModp, string& longnamer,
                              തർക്കം& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        വേണ്ടി (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            സ്ഥിരം AstVar* സ്ഥിരം modvarp = pinp->modVarp();
            എങ്കിൽ (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                തുടരുക;
            }
            എങ്കിൽ (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* സ്ഥിരം portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം exprp = pinp->exprp();
                എങ്കിൽ (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    തുടരുക;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം vrp = VN_CAST(exprp, കണ്ണി_Var_Ref)) {
                    എങ്കിൽ (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                എങ്കിൽ (!pinIrefp && exprp) {
                    സ്ഥിരം Ast_കണ്ണി* basep = exprp;
                    ആകുമ്പോൾ (സ്ഥിരം Ast_കണ്ണി_Pre_Sel* സ്ഥിരം selp = VN_CAST(basep, കണ്ണി_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vrp = VN_CAST(basep, VarRef)) {
                        എങ്കിൽ (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                എങ്കിൽ (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } അല്ലെങ്കിൽ എങ്കിൽ (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } അല്ലെങ്കിൽ {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    എങ്കിൽ (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_മൂല്യം_സംഖ്യ(pinIrefp));
                        any_overridesr = സത്യം;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        എങ്കിൽ (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_കണ്ണി::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_കണ്ണി::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    ശൂന്യം storeOriginalParams(Ast_കണ്ണി_Module* സ്ഥിരം classp) {
        വേണ്ടി (Ast_കണ്ണി* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_കണ്ണി* originalParamp = nullptr;
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (varp->isGParam()) originalParamp = varp->clonep();
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം ptp = VN_CAST(stmtp, ParamTypeDType)) {
                എങ്കിൽ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            എങ്കിൽ (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    ശൂന്യം genericInterfaceVarSetup(സ്ഥിരം AstPin* സ്ഥിരം paramsp, സ്ഥിരം AstPin* സ്ഥിരം pinsp) {
        std::unordered_map<string, സ്ഥിരം AstPin*> paramspMap;
        വേണ്ടി (സ്ഥിരം AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.തിരുകുക({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        എങ്കിൽ (paramspMap.empty()) തിരികെ;

        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstPin* സ്ഥിരം pinp = VN_CAST(nodep, Pin)) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = pinp->modVarp()) {
                    Ast_കണ്ണി_DType* dtypep = varp->childDTypep()->elemDTypep();
                    എങ്കിൽ (AstIfaceGenericDType* സ്ഥിരം ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        സ്ഥിരം auto iter = paramspMap.find(varp->name());
                        എങ്കിൽ (iter == paramspMap.end()) തുടരുക;
                        Ast_കണ്ണി* സ്ഥിരം backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        സ്ഥിരം AstPin* സ്ഥിരം paramp = iter->second;
                        paramspMap.erase(iter);
                        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* സ്ഥിരം newIfacerefp = പുതിയ AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        എങ്കിൽ (auto* സ്ഥിരം arrDtp = VN_CAST(backp, കണ്ണി_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } അല്ലെങ്കിൽ എങ്കിൽ (auto* സ്ഥിരം arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } അല്ലെങ്കിൽ {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.തള്ളുക_Deletep(ifaceGDTypep), ifaceGDTypep);
                        എങ്കിൽ (paramspMap.empty()) തിരികെ;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    ശൂന്യം resolveDefaultParams(Ast_കണ്ണി* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        എങ്കിൽ (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            എങ്കിൽ (!classp) തിരികെ;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        വേണ്ടി (Ast_കണ്ണി* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                എങ്കിൽ (!paramTypep->isGParam()) തുടരുക;
                m_paramIndex.emplace(paramTypep, static_cast<പൂർണ്ണസംഖ്യ>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_കണ്ണി_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        വേണ്ടി (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* സ്ഥിരം paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* സ്ഥിരം reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            എങ്കിൽ (!reqDtp) തുടരുക;

            // If default is a reference to another param type, record dependency
            എങ്കിൽ (AstRefDType* സ്ഥിരം refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                എങ്കിൽ (AstParamTypeDType* സ്ഥിരം sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    എങ്കിൽ (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    തുടരുക;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtp = VN_CAST(reqDtp->lhsp(), കണ്ണി_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* സ്ഥിരം typep = pinp->modPTypep();
            സ്ഥിരം auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            എങ്കിൽ (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } അല്ലെങ്കിൽ {
                pinsByIndex.തള്ളുക_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        വേണ്ടി (തർക്കം resolvedSome = സത്യം; resolvedSome;) {
            resolvedSome = അസത്യം;
            വേണ്ടി (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                എങ്കിൽ (pinsByIndex[paramIdx]) തുടരുക;
                സ്ഥിരം പൂർണ്ണസംഖ്യ sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                എങ്കിൽ (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_വൃക്ഷം(അസത്യം);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                എങ്കിൽ (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = defaultTypeNodes[paramIdx];
                    newPinp = പുതിയ AstPin{dtypep->fileline(), static_cast<പൂർണ്ണസംഖ്യ>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_വൃക്ഷം(അസത്യം)};
                }

                എങ്കിൽ (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(സത്യം);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    എങ്കിൽ (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } അല്ലെങ്കിൽ എങ്കിൽ (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    എങ്കിൽ (!paramsp) paramsp = newPinp;
                    resolvedSome = സത്യം;
                }
            }
        }
    }

    Ast_കണ്ണി_Module* കണ്ണി_Deparam_Common(Ast_കണ്ണി* nodep, Ast_കണ്ണി_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, തർക്കം any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        എങ്കിൽ (debug() >= 9 && paramsp) paramsp->dump_വൃക്ഷം_And_Next(cout, "-  cellparams: ");

        എങ്കിൽ (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } അല്ലെങ്കിൽ {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<സ്ഥിരം AstPin*> overridingTypePins;
            വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (!pinp->exprp()) തുടരുക;  // An empty override keeps the default
                എങ്കിൽ (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    തുടരുക;
                }
                AstVar* സ്ഥിരം modvarp = pinp->modVarp();
                എങ്കിൽ (!modvarp || !modvarp->isGParam()) തുടരുക;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                സ്ഥിരം AstPattern* സ്ഥിരം patternp = VN_CAST(pinp->exprp(), Pattern);
                എങ്കിൽ (patternp && !patternp->childDTypep()) തുടരുക;
                എങ്കിൽ (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_മൂല്യം(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (AstParamTypeDType* സ്ഥിരം modptp = pinp->modPTypep()) {
                    എങ്കിൽ (!overridingTypePins.എണ്ണം(pinp)) തുടരുക;
                    Ast_കണ്ണി_DType* സ്ഥിരം typep = VN_AS(pinp->exprp(), കണ്ണി_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_മൂല്യം_സംഖ്യ(typep->skipRefToNonRefp());
                    any_overrides = സത്യം;
                    തുടരുക;
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
        തർക്കം defaultsResolved = അസത്യം;
        എങ്കിൽ (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            വേണ്ടി (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                എങ്കിൽ (pinp->modPTypep()) {
                    any_overrides = സത്യം;
                    defaultsResolved = സത്യം;
                    നിർത്തുക;
                }
            }
        }
        എങ്കിൽ (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = സത്യം;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_കണ്ണി_Module* newModp = nullptr;
        എങ്കിൽ (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_കണ്ണി_Module* സ്ഥിരം paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            എങ്കിൽ (!paramedModp) തിരികെ nullptr;  // Unsupported, already reported
            paramedModp->dead(അസത്യം);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } അല്ലെങ്കിൽ എങ്കിൽ (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            എങ്കിൽ (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_കണ്ണി_Module* കണ്ണി_Copyp = srcModp->clone_വൃക്ഷം(അസത്യം);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << കണ്ണി_Copyp);
                m_deleter.തള്ളുക_Deletep(കണ്ണി_Copyp);  // nodeCopyp used past here
                srcModp->user3p(കണ്ണി_Copyp);
                storeOriginalParams(കണ്ണി_Copyp);
            }
            newModp = srcModp;
        } അല്ലെങ്കിൽ {
            സ്ഥിരം string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            സ്ഥിരം ModInfo* സ്ഥിരം modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            എങ്കിൽ (!modInfop) തിരികെ nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        സ്ഥിരം തർക്കം cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* സ്ഥിരം newClassp = VN_CAST(newModp, Class);
        എങ്കിൽ (newClassp && !newClassp->user2()) m_specializedClassps.തള്ളുക_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        എങ്കിൽ (defaultsResolved) srcModp->user4p(newModp);

        വേണ്ടി (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                എങ്കിൽ (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        എങ്കിൽ (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_വൃക്ഷം(), paramsp);
        തിരികെ newModp;
    }

    Ast_കണ്ണി_Module* cellDeparam(AstCell* nodep, Ast_കണ്ണി_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_കണ്ണി_Module* സ്ഥിരം newModp = കണ്ണി_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        എങ്കിൽ (!newModp) തിരികെ nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(അസത്യം);
        തിരികെ newModp;
    }
    Ast_കണ്ണി_Module* ifaceRefDeparam(AstIfaceRefDType* സ്ഥിരം nodep, Ast_കണ്ണി_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        എങ്കിൽ (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            സ്ഥിരം string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            സ്ഥിരം string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            സ്ഥിരം size_t ownerPos = ownerBaseName.find("__");
            എങ്കിൽ (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            സ്ഥിരം size_t srcPos = srcBaseName.find("__");
            എങ്കിൽ (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            എങ്കിൽ (ownerBaseName == srcBaseName) {
                തർക്കം allOwnParams = സത്യം;
                വേണ്ടി (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    എങ്കിൽ (AstRefDType* സ്ഥിരം refp = VN_CAST(pinp->exprp(), RefDType)) {
                        എങ്കിൽ (AstParamTypeDType* സ്ഥിരം ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_കണ്ണി_Module* സ്ഥിരം ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            എങ്കിൽ (ptdOwnerp != m_modp) allOwnParams = അസത്യം;
                        } അല്ലെങ്കിൽ {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } അല്ലെങ്കിൽ {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                എങ്കിൽ (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    എങ്കിൽ (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
                    തിരികെ ownerIfacep;
                }
            }
        }

        Ast_കണ്ണി_Module* സ്ഥിരം newModp
            = കണ്ണി_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, അസത്യം);
        എങ്കിൽ (!newModp) തിരികെ nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        തിരികെ newModp;
    }
    Ast_കണ്ണി_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_കണ്ണി_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_കണ്ണി_Module* സ്ഥിരം newModp
            = കണ്ണി_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, അസത്യം);
        എങ്കിൽ (!newModp) തിരികെ nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* സ്ഥിരം refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* സ്ഥിരം newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        എങ്കിൽ (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            എങ്കിൽ (AstTypedef* സ്ഥിരം typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        തിരികെ newModp;
    }
    Ast_കണ്ണി_Module* classRefDeparam(AstClassRefDType* nodep, Ast_കണ്ണി_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_കണ്ണി_Module* സ്ഥിരം newModp
            = കണ്ണി_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, അസത്യം);
        എങ്കിൽ (!newModp) തിരികെ nullptr;
        AstClass* സ്ഥിരം newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        തിരികെ newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    വർഗ്ഗം DeferredResolverVisitor final : പൊതു VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<സ്ഥിരം Ast_കണ്ണി*> m_reachedDecls;

        തർക്കം firstReach(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം declp) { തിരികെ m_reachedDecls.തിരുകുക(declp).second; }

        ശൂന്യം iterateParamType(AstParamTypeDType* സ്ഥിരം paramTypep) {
            എങ്കിൽ (firstReach(paramTypep)) iterate(paramTypep);
        }
        ശൂന്യം iterateTypedef(AstTypedef* സ്ഥിരം tdefp) {
            എങ്കിൽ (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }
        ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_കണ്ണി* സ്ഥിരം targetp = nodep->classOrPackageNodep();
            എങ്കിൽ (AstTypedef* സ്ഥിരം tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        ശൂന്യം visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        ശൂന്യം visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* സ്ഥിരം tdefp = nodep->typedefp();
            എങ്കിൽ (tdefp) {
                iterateTypedef(tdefp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        ശൂന്യം visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* സ്ഥിരം varp = nodep->varp();
            എങ്കിൽ (firstReach(varp)) {
                iterate(varp->subDTypep());
                സ്ഥിരം auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                എങ്കിൽ (varp->varType() == VVarType::LPARAM && deferredVarps.എണ്ണം(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    പൊതു:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_കണ്ണി* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

പൊതു:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    ശൂന്യം retargetIfaceRefs(Ast_കണ്ണി_Module* parentModp, സ്ഥിരം string& cellName) {
        Ast_കണ്ണി_Module* സ്ഥിരം correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        എങ്കിൽ (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) തിരികെ;
        parentModp->foreach([&](AstRefDType* refp) {
            സ്ഥിരം VIfaceCaptureTag* സ്ഥിരം tagp = V3LinkDotIfaceCapture::captureTag(refp);
            എങ്കിൽ (!tagp || tagp->m_cellPath != cellName) തിരികെ;
            // Only retarget REFDTYPEs owned by parentModp.
            എങ്കിൽ (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) തിരികെ;
            എങ്കിൽ (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    ശൂന്യം resolveDeferredDotsReachableFrom(Ast_കണ്ണി* rootp, സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*ഇത്, rootp};
    }

    Ast_കണ്ണി_Module* കണ്ണി_Deparam(Ast_കണ്ണി* nodep, Ast_കണ്ണി_Module* srcModp, Ast_കണ്ണി_Module* modp,
                               സ്ഥിരം string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_കണ്ണി::കണ്ണി_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string കണ്ണി_Name = nodep->name();
        എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            എങ്കിൽ (കണ്ണി_Name.empty()) കണ്ണി_Name = ifaceRefp->cellName();
        }
        സ്ഥിരം string instanceName
            = കണ്ണി_Name.empty() ? someInstanceName : (someInstanceName + "." + കണ്ണി_Name);

        എങ്കിൽ (AstCell* സ്ഥിരം cellp = VN_CAST(nodep, Cell)) {
            വേണ്ടി (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* സ്ഥിരം nextp = VN_AS(pinp->nextp(), Pin);
                എങ്കിൽ (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_വൃക്ഷം(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_കണ്ണി::കണ്ണി_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_കണ്ണി_Module* newModp = nullptr;
        എങ്കിൽ (AstCell* സ്ഥിരം cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        എങ്കിൽ (!newModp) തിരികെ nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_കണ്ണി::കണ്ണി_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        തിരികെ newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        തിരികെ taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        വേണ്ടി (Ast_കണ്ണി_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
            m_allModuleNames.തിരുകുക(modp->name());
        }
    }
    ~ParamProcessor() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

വർഗ്ഗം ClassRefUnlinkerVisitor final : പൊതു VNVisitor {

പൊതു:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
        എങ്കിൽ (nodep->paramsp()) {
            എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                എങ്കിൽ (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* സ്ഥിരം dotp = VN_CAST(nodep->backp(), Dot);
                    എങ്കിൽ (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(തള്ളുക_Deletep(dotp), dotp, nodep);
                    } അല്ലെങ്കിൽ {
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    ശൂന്യം visit(AstClass* nodep) override {}  // don't iterate inside classes
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

വർഗ്ഗം ParamClassRefDTypeRelinkVisitor final : പൊതു VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<സ്ഥിരം AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_കണ്ണി_Module* m_ownerModp = nullptr;
    Ast_കണ്ണി_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    ശൂന്യം ensureOwnerMap() {
        എങ്കിൽ (m_ownerMapForp == m_ownerModp) തിരികെ;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        എങ്കിൽ (!m_ownerModp) തിരികെ;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        എങ്കിൽ (AstClass* സ്ഥിരം ownerClassp = VN_CAST(m_ownerModp, Class)) {
            എങ്കിൽ (ownerClassp->hasGParam()) തിരികെ;
        }
        വേണ്ടി (Ast_കണ്ണി* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* സ്ഥിരം tdp = VN_CAST(sp, Typedef);
            എങ്കിൽ (!tdp) തുടരുക;
            AstClassRefDType* സ്ഥിരം crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            എങ്കിൽ (!crdtp || !crdtp->classp()) തുടരുക;
            AstClass* സ്ഥിരം targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            എങ്കിൽ (targetp->hasGParam()) തുടരുക;
            സ്ഥിരം std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            സ്ഥിരം auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            എങ്കിൽ (!pair.second && pair.first->second != targetp) m_ambiguous.തിരുകുക(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    സ്ഥിതം string classOrigName(സ്ഥിരം AstClass* classp) {
        തിരികെ classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    സ്ഥിതം AstTypedef* findTypedefWithBases(AstClass* classp, സ്ഥിരം string& name) {
        വേണ്ടി (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            എങ്കിൽ (AstTypedef* സ്ഥിരം tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                എങ്കിൽ (tdp->subDTypep()) തിരികെ tdp;
            }
        }
        തിരികെ nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    ശൂന്യം retargetRefDType(AstRefDType* refp) {
        എങ്കിൽ (!m_ownerModp) തിരികെ;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        എങ്കിൽ (AstClass* സ്ഥിരം ownerClassp = VN_CAST(m_ownerModp, Class)) {
            എങ്കിൽ (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                സ്ഥിരം AstClass* refClassp = nullptr;
                എങ്കിൽ (AstTypedef* സ്ഥിരം tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassOrPackageRef* സ്ഥിരം classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                എങ്കിൽ (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* സ്ഥിരം selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    എങ്കിൽ (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        തിരികെ;
                    }
                }
            }
        }

        AstTypedef* സ്ഥിരം oldTdp = refp->typedefp();
        എങ്കിൽ (!oldTdp) തിരികെ;
        AstClass* സ്ഥിരം oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        എങ്കിൽ (!oldOwnerp) തിരികെ;

        ensureOwnerMap();
        എങ്കിൽ (m_origNameToClone.empty()) തിരികെ;
        സ്ഥിരം std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        എങ്കിൽ (m_ambiguous.എണ്ണം(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            തിരികെ;
        }
        സ്ഥിരം auto it = m_origNameToClone.find(origName);
        എങ്കിൽ (it == m_origNameToClone.end()) തിരികെ;
        AstClass* സ്ഥിരം correctClassp = it->second;
        എങ്കിൽ (correctClassp == oldOwnerp) തിരികെ;
        AstTypedef* സ്ഥിരം newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        എങ്കിൽ (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            തിരികെ;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        എങ്കിൽ (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            തിരികെ;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

പൊതു:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstParamTypeDType* സ്ഥിരം ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        എങ്കിൽ (AstParamTypeDType* സ്ഥിരം paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            സ്ഥിരം auto it = m_paramTypeClassMap.find(paramtypep);
            എങ്കിൽ (it != m_paramTypeClassMap.end()) {
                AstClass* സ്ഥിരം origClassp = it->second;
                എങ്കിൽ (origClassp->hasGParam() && !origClassp->user3p()) {
                    എങ്കിൽ (AstClass* സ്ഥിരം parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        സ്ഥിരം string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        വേണ്ടി (Ast_കണ്ണി* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            എങ്കിൽ (AstParamTypeDType* സ്ഥിരം ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                എങ്കിൽ (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    നിർത്തുക;
                                }
                            }
                        }
                        എങ്കിൽ (newParamTypep) {
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
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

വർഗ്ഗം ParamState final {
പൊതു:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<തർക്കം, പൂർണ്ണസംഖ്യ>;
    std::multimap<WQKey, Ast_കണ്ണി_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_കണ്ണി_Module*, std::unordered_set<Ast_കണ്ണി_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

വർഗ്ഗം ParamVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    തർക്കം m_iterateModule = അസത്യം;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<തർക്കം, Ast_കണ്ണി*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<സ്ഥിരം AstRefDType*, തർക്കം>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    തർക്കം m_modIfaceRefsDone = അസത്യം;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    തർക്കം m_inGenerateCond = അസത്യം;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    ശൂന്യം processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_കണ്ണി_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = സത്യം;

        // Visit all cells under module, recursively
        ആകുമ്പോൾ (സത്യം) {
            // Classes specialized since the last pass still need their bodies elaborated
            വേണ്ടി (AstClass* സ്ഥിരം classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{സത്യം, classp->level()}, classp);
            }
            എങ്കിൽ (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            എങ്കിൽ (workQueue.empty()) നിർത്തുക;

            സ്ഥിരം auto itm = workQueue.cbegin();
            Ast_കണ്ണി_Module* സ്ഥിരം modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            എങ്കിൽ (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                എങ്കിൽ (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = അസത്യം;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            ആകുമ്പോൾ (!m_cellps.empty()) {
                സ്ഥിരം auto itim = m_cellps.cbegin();
                Ast_കണ്ണി* സ്ഥിരം cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                സ്ഥിരം auto genHierIt = m_genHierNames.find(cellp);
                എങ്കിൽ (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_കണ്ണി_Module* srcModp = nullptr;
                എങ്കിൽ (സ്ഥിരം AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    എങ്കിൽ (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } അല്ലെങ്കിൽ {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                എങ്കിൽ (!srcModp) തുടരുക;

                // Update path
                സ്ഥിരം string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം newModp
                    = m_processor.കണ്ണി_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    എങ്കിൽ (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        എങ്കിൽ (V3LinkDotIfaceCapture::enabled()) {
                            എങ്കിൽ (സ്ഥിരം AstCell* സ്ഥിരം modCellp = VN_CAST(cellp, Cell)) {
                                എങ്കിൽ (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].തിരുകുക(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    എങ്കിൽ (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = അസത്യം;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    സ്ഥിതം string getRefBaseName(സ്ഥിരം AstVarXRef* refp) {
        സ്ഥിരം string dotted = refp->dotted();
        എങ്കിൽ (dotted.empty()) തിരികെ "";
        തിരികെ dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    ശൂന്യം logTemplateLeakRefs(Ast_കണ്ണി_Module* parentModp, Ast_കണ്ണി_Module* templateModp,
                             സ്ഥിരം അക്ഷരം* stage, Ast_കണ്ണി* contextp) {
        എങ്കിൽ (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) തിരികെ;
        // LCOV_EXCL_START  // Debug-only diagnostic
        പൂർണ്ണസംഖ്യ leak_എണ്ണം = 0;
        സ്ഥിരം auto ancestryOf = [](സ്ഥിരം Ast_കണ്ണി* nodep) {
            string ancestry;
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* curp = nodep; curp; curp = curp->backp()) {
                എങ്കിൽ (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                എങ്കിൽ (VN_IS(curp, കണ്ണി_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    നിർത്തുക;
                }
            }
            തിരികെ ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            എങ്കിൽ (refp->typedefp()) {
                Ast_കണ്ണി_Module* സ്ഥിരം tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                എങ്കിൽ (tdOwnerp == templateModp) {
                    ++leak_എണ്ണം;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_കണ്ണി::കണ്ണി_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            എങ്കിൽ (refp->refDTypep()) {
                Ast_കണ്ണി_Module* സ്ഥിരം rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                എങ്കിൽ (rdOwnerp == templateModp) {
                    ++leak_എണ്ണം;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_കണ്ണി::കണ്ണി_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            എങ്കിൽ (!varrefp->varp()) തിരികെ;
            Ast_കണ്ണി_Module* സ്ഥിരം varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            എങ്കിൽ (varOwnerp != templateModp) തിരികെ;
            ++leak_എണ്ണം;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_കണ്ണി::കണ്ണി_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        എങ്കിൽ (leak_എണ്ണം > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_എണ്ണം);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    ശൂന്യം checkParamNotHierRecurse(Ast_കണ്ണി* nodep, തർക്കം underTypeQuery = അസത്യം) {
        വേണ്ടി (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            സ്ഥിരം AstAttrOf* സ്ഥിരം attrp = VN_CAST(nodep, AttrOf);
            സ്ഥിരം തർക്കം childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            എങ്കിൽ (സ്ഥിരം AstVarXRef* സ്ഥിരം refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                തർക്കം isIfaceRef = അസത്യം;
                എങ്കിൽ (refp->varp() && refp->varp()->isIfaceParam()) {
                    സ്ഥിരം string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.എണ്ണം(refname) || m_ifaceInstCells.എണ്ണം(refname));
                }
                എങ്കിൽ (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_FTask_Ref* സ്ഥിരം refp = VN_CAST(nodep, കണ്ണി_FTask_Ref)) {
                എങ്കിൽ (refp->dotted() != "") {
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
    ശൂന്യം specializeNestedIfaceCells(Ast_കണ്ണി_Module* ifaceModp) {
        വേണ്ടി (Ast_കണ്ണി* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* സ്ഥിരം nestedCellp = VN_CAST(stmtp, Cell);
            എങ്കിൽ (!nestedCellp) തുടരുക;
            എങ്കിൽ (!VN_IS(nestedCellp->modp(), Iface)) തുടരുക;
            എങ്കിൽ (!nestedCellp->paramsp()) തുടരുക;

            Ast_കണ്ണി_Module* സ്ഥിരം nestedSrcModp = nestedCellp->modp();
            എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം nestedNewModp = m_processor.കണ്ണി_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                എങ്കിൽ (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    വേണ്ടി (Ast_കണ്ണി* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(sp, Var)) {
                            എങ്കിൽ (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    എങ്കിൽ (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    തർക്കം cellParamsReferenceIfacePorts(AstCell* cellp) {
        എങ്കിൽ (!cellp->paramsp()) തിരികെ അസത്യം;

        വേണ്ടി (Ast_കണ്ണി* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (AstPin* സ്ഥിരം pinp = VN_CAST(nodep, Pin)) {
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം exprp = pinp->exprp()) {
                    എങ്കിൽ (സ്ഥിരം AstVarXRef* സ്ഥിരം refp = VN_CAST(exprp, VarXRef)) {
                        സ്ഥിരം string refname = getRefBaseName(refp);
                        എങ്കിൽ (!refname.empty()
                            && (m_ifacePortNames.എണ്ണം(refname)
                                || m_ifaceInstCells.എണ്ണം(refname)))
                            തിരികെ സത്യം;
                    }
                }
            }
        }
        തിരികെ അസത്യം;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    ശൂന്യം expandIfaceArrayPortDimensions(AstVar* portp, Ast_കണ്ണി_DType* dtypep,
                                        സ്ഥിരം std::string& suffix) {
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        എങ്കിൽ (!arrp) {
            AstVar* സ്ഥിരം varp = portp->clone_വൃക്ഷം(അസത്യം);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം oldp = varp->childDTypep()) {
                VL_DO_DANGLING(തള്ളുക_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_വൃക്ഷം(അസത്യം));
            portp->addNextHere(varp);
            തിരികെ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ n : arrp->declRange().seq_വലത്_To_ഇടത്()) {
            സ്ഥിരം std::string s = suffix + "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    ശൂന്യം expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   സ്ഥിരം AstRange* rangep, സ്ഥിരം std::string& suffix, പൂർണ്ണസംഖ്യ idx) {
        // Base case: insert the element when no dimensions left
        എങ്കിൽ (!rangep) {
            AstCell* സ്ഥിരം elemp = arrayedCellp->clone_വൃക്ഷം(അസത്യം);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            എങ്കിൽ (ifaceVarp) {
                AstIfaceRefDType* സ്ഥിരം irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_വൃക്ഷം(അസത്യം);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* സ്ഥിരം varp = ifaceVarp->clone_വൃക്ഷം(അസത്യം);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            തിരികെ;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        സ്ഥിരം AstRange* സ്ഥിരം subRangep = VN_AS(rangep->nextp(), Range);
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ n : rangep->seq_വലത്_To_ഇടത്()) {
            സ്ഥിരം std::string s = suffix + "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    ശൂന്യം visitCellOrClassRef(Ast_കണ്ണി* nodep, തർക്കം isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        എങ്കിൽ (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* സ്ഥിരം cellp = VN_CAST(nodep, Cell);
            എങ്കിൽ (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_കണ്ണി_Module* സ്ഥിരം srcModp = cellp->modp();
                Ast_കണ്ണി_Module* സ്ഥിരം newModp
                    = m_processor.കണ്ണി_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                എങ്കിൽ (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                എങ്കിൽ (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        എങ്കിൽ (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->recursiveClone()) nodep->dead(സത്യം);  // Fake, made for recursive elimination
        എങ്കിൽ (nodep->dead()) തിരികെ;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) {
            എങ്കിൽ (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.തള്ളുക_back(classp);
                തിരികെ;
            }
        }

        എങ്കിൽ (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            തിരികെ;
        }

        // Start traversal at root-like things
        എങ്കിൽ (nodep->is_മുകൾ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    തർക്കം isCircularType(സ്ഥിരം AstRefDType* nodep) {
        സ്ഥിരം auto iter = m_isCircular.emplace(nodep, സത്യം);
        എങ്കിൽ (!iter.second) തിരികെ iter.first->second;
        എങ്കിൽ (സ്ഥിരം AstRefDType* സ്ഥിരം subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            സ്ഥിരം തർക്കം ret = isCircularType(subDTypep);
            iter.first->second = ret;
            തിരികെ ret;
        }
        iter.first->second = അസത്യം;
        തിരികെ അസത്യം;
    }

    ശൂന്യം visit(AstRefDType* nodep) override {
        സ്ഥിരം തർക്കം isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        എങ്കിൽ (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        എങ്കിൽ (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->typedefp() && nodep->subDTypep()
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
    ശൂന്യം visit(AstCell* nodep) override {
        എങ്കിൽ (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            വേണ്ടി (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* സ്ഥിരം ifaceVarp = [&]() -> AstVar* {
                എങ്കിൽ (!VN_IS(nodep->modp(), Iface)) തിരികെ nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* സ്ഥിരം varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                തിരികെ varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            എങ്കിൽ (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(സത്യം);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* സ്ഥിരം firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* സ്ഥിരം irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            തിരികെ;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        എങ്കിൽ (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        എങ്കിൽ (nodep->ifacep()) visitCellOrClassRef(nodep, സത്യം);
    }
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, അസത്യം);
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        എങ്കിൽ (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, അസത്യം);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    സ്ഥിതം ശൂന്യം gatherVars(Ast_കണ്ണി* stmtsp, തർക്കം (*matchp)(സ്ഥിരം AstVar*), VarsByName& varsr) {
        വേണ്ടി (Ast_കണ്ണി* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                എങ്കിൽ (matchp(varp)) varsr.emplace(varp->name(), varp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstGenBlock* സ്ഥിരം genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    സ്ഥിരം VarsByName& modIfaceRefs() {
        എങ്കിൽ (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](സ്ഥിരം AstVar* varp) { തിരികെ varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = സത്യം;
        }
        തിരികെ m_modIfaceRefs;
    }
    സ്ഥിരം VarsByName& ifaceParams(Ast_കണ്ണി_Module* ifacep) {
        സ്ഥിരം auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        എങ്കിൽ (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](സ്ഥിരം AstVar* varp) { തിരികെ varp->isParam(); },
                pair.first->second);
        }
        തിരികെ pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_കണ്ണി* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* സ്ഥിരം lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* സ്ഥിരം rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        എങ്കിൽ (!lhsp || !rhsp) തിരികെ nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        എങ്കിൽ (VL_UNCOVERABLE(!m_modp)) തിരികെ nullptr;

        സ്ഥിരം auto& refVars = modIfaceRefs();
        സ്ഥിരം auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        എങ്കിൽ (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) തിരികെ nullptr;
        AstVar* സ്ഥിരം ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* സ്ഥിരം ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        എങ്കിൽ (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) തിരികെ nullptr;

        സ്ഥിരം auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        സ്ഥിരം auto targetVarIt = ifaceVars.find(rhsp->name());
        എങ്കിൽ (targetVarIt == ifaceVars.end()) തിരികെ nullptr;
        AstVar* സ്ഥിരം targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        എങ്കിൽ (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        എങ്കിൽ (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            തിരികെ nullptr;  // LCOV_EXCL_LINE
        }
        തിരികെ targetVarp->valuep()->clone_വൃക്ഷം(അസത്യം);
    }
    ശൂന്യം resolveGenericIfaceDotsIn(Ast_കണ്ണി* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.തള്ളുക_back(dotp); });
        വേണ്ടി (AstDot* സ്ഥിരം dotp : dotps) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_വൃക്ഷം(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (nodep->user2SetOnce()) തിരികെ;  // Process once
        // Interface array port: split it into its elements
        എങ്കിൽ (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(സത്യം);
        }
        // Build cache of interface port names as we encounter them
        എങ്കിൽ (nodep->isIfaceRef()) { m_ifacePortNames.തിരുകുക(nodep->name()); }
        iterateChildren(nodep);
        എങ്കിൽ (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            എങ്കിൽ (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                തർക്കം hasUnresolvedLparamXRef = അസത്യം;
                nodep->valuep()->foreach([&](സ്ഥിരം AstVarXRef* xrefp) {
                    എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = xrefp->varp()) {
                        എങ്കിൽ (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = സത്യം;
                        }
                    }
                });
                എങ്കിൽ (hasUnresolvedLparamXRef) തിരികെ;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                സ്ഥിരം തർക്കം hasDot = nodep->valuep()->exists([](AstDot*) { തിരികെ സത്യം; });
                തർക്കം refsDeferred = അസത്യം;
                എങ്കിൽ (!hasDot) {
                    സ്ഥിരം auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](സ്ഥിരം AstVarRef* refp) {
                        എങ്കിൽ (refsDeferred) തിരികെ;
                        AstVar* സ്ഥിരം refVarp = refp->varp();
                        എങ്കിൽ (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.എണ്ണം(refVarp)) {
                            refsDeferred = സത്യം;
                        }
                    });
                }
                എങ്കിൽ (hasDot || refsDeferred) {
                    v3Global.rootp()->തള്ളുക_Deferred_Param_Varp(nodep);
                    തിരികെ;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    ശൂന്യം visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        എങ്കിൽ (nodep->varp()) iterate(nodep->varp());
    }
    തർക്കം ifaceParamReplace(AstVarXRef* nodep, Ast_കണ്ണി* candp) {
        വേണ്ടി (; candp; candp = candp->nextp()) {
            എങ്കിൽ (nodep->name() == candp->name()) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    എങ്കിൽ (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    തിരികെ സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPin* സ്ഥിരം pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_വൃക്ഷം(അസത്യം));
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ സത്യം;
                }
            }
        }
        തിരികെ അസത്യം;
    }

    ശൂന്യം deparamIfaceCellNow(AstCell* cellp) {
        എങ്കിൽ (!cellp->paramsp()) തിരികെ;
        എങ്കിൽ (!VN_IS(cellp->modp(), Iface)) തിരികെ;
        Ast_കണ്ണി_Module* സ്ഥിരം srcModp = cellp->modp();
        Ast_കണ്ണി_Module* സ്ഥിരം newModp
            = m_processor.കണ്ണി_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        എങ്കിൽ (newModp && newModp != srcModp) {
            എങ്കിൽ (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        എങ്കിൽ (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarXRef* nodep) override {
        എങ്കിൽ (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            തിരികെ;
        }
        // Parameters of an interface array are accessed through its first element
        എങ്കിൽ (nodep->varp() && nodep->varp()->isParam()) {
            സ്ഥിരം auto it = m_ifaceInstCells.find(nodep->dotted());
            എങ്കിൽ (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        സ്ഥിരം string dotted = nodep->dotted();
        എങ്കിൽ (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            സ്ഥിരം Ast_കണ്ണി* backp = nodep;
            ആകുമ്പോൾ ((backp = backp->backp())) {
                എങ്കിൽ (VN_IS(backp, കണ്ണി_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    നിർത്തുക;
                }
                എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(backp, Var)) {
                    എങ്കിൽ (!varp->isIfaceRef()) തുടരുക;
                    // Through all dimensions of an array
                    സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    എങ്കിൽ (!ifacerefp) തുടരുക;
                    // Interfaces passed in on the port map have ifaces
                    എങ്കിൽ (സ്ഥിരം AstIface* സ്ഥിരം ifacep = ifacerefp->ifacep()) {
                        എങ്കിൽ (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            എങ്കിൽ (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                തിരികെ;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    അല്ലെങ്കിൽ എങ്കിൽ (AstCell* സ്ഥിരം cellp = ifacerefp->cellp()) {
                        എങ്കിൽ (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            എങ്കിൽ (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                തിരികെ;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            സ്ഥിരം auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            എങ്കിൽ (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* സ്ഥിരം cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    ശൂന്യം visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        സ്ഥിരം AstClass* lhsClassp = nullptr;
        സ്ഥിരം AstClassOrPackageRef* സ്ഥിരം classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        എങ്കിൽ (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_കണ്ണി* rhsDefp = nullptr;
        AstClassOrPackageRef* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        എങ്കിൽ (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        എങ്കിൽ (lhsClassp && rhsDefp) {
            m_state.m_dots.തള്ളുക_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } അല്ലെങ്കിൽ {
            iterate(nodep->rhsp());
        }
    }

    ശൂന്യം visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* സ്ഥിരം varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_കണ്ണി_FTask_Ref* സ്ഥിരം taskrefp = VN_CAST(nodep->refp(), കണ്ണി_FTask_Ref);
        AstEnumItemRef* സ്ഥിരം enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        എങ്കിൽ (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } അല്ലെങ്കിൽ എങ്കിൽ (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } അല്ലെങ്കിൽ എങ്കിൽ (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            തിരികെ;
        }
        iterate(nodep->cellrefp());

        എങ്കിൽ (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } അല്ലെങ്കിൽ എങ്കിൽ (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } അല്ലെങ്കിൽ {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        വേണ്ടി (Ast_കണ്ണി *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<പൂർണ്ണസംഖ്യ> indices;
        വേണ്ടി (Ast_കണ്ണി* s = nodep->selp(); s; s = s->nextp()) {
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(s, Const);
            എങ്കിൽ (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                തിരികെ;
            }
            indices.തള്ളുക_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        സ്ഥിരം string viftopSuffix = "__Viftop";
        സ്ഥിരം string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        സ്ഥിരം string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        എങ്കിൽ (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            തിരികെ;
        }
        pos += baseName.length();
        വേണ്ടി (പൂർണ്ണസംഖ്യ idx : indices) {
            സ്ഥിരം string replacement = "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(idx) + "__KET__";
            എങ്കിൽ (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                തിരികെ;
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
    ശൂന്യം iterateGenerateCond(Ast_കണ്ണി* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = സത്യം;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    ശൂന്യം visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->condp(), Const)) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } അല്ലെങ്കിൽ {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            // Normal edit rules will now recurse the replacement
        } അല്ലെങ്കിൽ {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    ശൂന്യം visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        എങ്കിൽ (AstGenFor* സ്ഥിരം forp = VN_AS(nodep->genforp(), GenFor)) {
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
            സ്ഥിരം string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } അല്ലെങ്കിൽ {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    ശൂന്യം visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        തർക്കം hit = അസത്യം;
        Ast_കണ്ണി* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        വേണ്ടി (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            വേണ്ടി (Ast_കണ്ണി* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        സ്ഥിരം AstConst* സ്ഥിരം exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        വേണ്ടി (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            വേണ്ടി (Ast_കണ്ണി* ep = itemp->condsp(); ep;) {
                Ast_കണ്ണി* സ്ഥിരം nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        വേണ്ടി (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            എങ്കിൽ (!itemp->isDefault()) {
                വേണ്ടി (Ast_കണ്ണി* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം ccondp = VN_CAST(ep, Const)) {
                        V3_സംഖ്യ match{nodep, 1};
                        match.opEq(ccondp->അക്കം(), exprp->അക്കം());
                        എങ്കിൽ (!hit && match.isNeqZero()) {
                            hit = സത്യം;
                            keepp = itemp->itemsp();
                        }
                    } അല്ലെങ്കിൽ {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        വേണ്ടി (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            എങ്കിൽ (itemp->isDefault()) {
                എങ്കിൽ (!hit) {
                    hit = സത്യം;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        എങ്കിൽ (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

വർഗ്ഗം Param_മുകൾ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    ശൂന്യം relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        വേണ്ടി (AstDot* സ്ഥിരം dotp : m_state.m_dots) {
            സ്ഥിരം AstClassOrPackageRef* സ്ഥിരം classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            സ്ഥിരം AstClass* സ്ഥിരം lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* സ്ഥിരം rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            വേണ്ടി (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                എങ്കിൽ (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    നിർത്തുക;
                }
            }
        }
    }

    ശൂന്യം fixLevel(Ast_കണ്ണി_Module* modp) {
        // Fix up level of module, based on who instantiates it
        എങ്കിൽ (modp->user1SetOnce()) തിരികെ;  // Already fixed
        എങ്കിൽ (m_state.m_parentps[modp].empty()) തിരികെ;  // Leave top levels alone
        പൂർണ്ണസംഖ്യ maxParentLevel = 0;
        വേണ്ടി (Ast_കണ്ണി_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        എങ്കിൽ (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    ശൂന്യം resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_കണ്ണി_Module*> modps;
        വേണ്ടി (Ast_കണ്ണി_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), കണ്ണി_Module);
            modp->unlinkFrBack();
            modps.തള്ളുക_back(modp);
        }

        // Fix-up levels
        സ്ഥിരം VNUser1InUse user1InUse;
        വേണ്ടി (Ast_കണ്ണി_Module* സ്ഥിരം modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](സ്ഥിരം Ast_കണ്ണി_Module* ap, സ്ഥിരം Ast_കണ്ണി_Module* bp) {
                             തിരികെ ap->level() < bp->level();
                         });

        // Re-insert modules
        വേണ്ടി (Ast_കണ്ണി_Module* സ്ഥിരം modp : modps) netlistp->addModulesp(modp);
    }

പൊതു:
    // CONSTRUCTORS
    explicit Param_മുകൾ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        സ്ഥിരം ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        സ്ഥിരം ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_കണ്ണി_FTask_Ref* ftaskrefp) {
            Ast_കണ്ണി_FTask* ftaskp = ftaskrefp->taskp();
            എങ്കിൽ (!ftaskp || !ftaskp->classMethod()) തിരികെ;
            സ്ഥിരം std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            സ്ഥിരം auto ancestorClassOf = [](Ast_കണ്ണി* startp) -> AstClass* {
                വേണ്ടി (Ast_കണ്ണി* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    എങ്കിൽ (AstClass* സ്ഥിരം cp = VN_CAST(np, Class)) തിരികെ cp;
                }
                തിരികെ nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            എങ്കിൽ (refClassp == ftaskrefp->classOrPackagep())
                തിരികെ;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            എങ്കിൽ (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                തിരികെ;
            }
            എങ്കിൽ (classp->user3p()) തിരികെ;  // will not get removed, no need to relink
            AstClass* സ്ഥിരം parametrizedClassp = VN_CAST(classp->user4p(), Class);
            എങ്കിൽ (!parametrizedClassp) തിരികെ;
            Ast_കണ്ണി_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_കണ്ണി_FTask* ftaskp) {
                എങ്കിൽ (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    തിരികെ സത്യം;
                }
                തിരികെ അസത്യം;
            });
            എങ്കിൽ (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        സ്ഥിരം ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        വേണ്ടി (AstClass* സ്ഥിരം classp : m_state.m_paramClasses) {
            എങ്കിൽ (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                എങ്കിൽ (V3Error::error_എണ്ണം()) തുടരുക;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(തള്ളുക_Deletep(classp->unlinkFrBack()), classp);
            } അല്ലെങ്കിൽ {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(അസത്യം);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_കണ്ണി::user3_Clear_വൃക്ഷം();
        വേണ്ടി (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_കണ്ണി* സ്ഥിരം nodep) { nodep->user3(സത്യം); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_കണ്ണി* സ്ഥിരം nodep) {
            nodep->foreachLink([&](Ast_കണ്ണി** സ്ഥിരം linkpp, സ്ഥിരം അക്ഷരം* namep) {
                എങ്കിൽ (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_മുകൾ() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(Param_മുകൾ);
};

//######################################################################
// Param class functions

ശൂന്യം V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    എങ്കിൽ (dump_വൃക്ഷം_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_മുകൾ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    എങ്കിൽ (dump_വൃക്ഷം_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_വൃക്ഷം("param", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    വേണ്ടി (AstVar* സ്ഥിരം varp : rootp->deferredParamVarps()) {
        എങ്കിൽ (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
