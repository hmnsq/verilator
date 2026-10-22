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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Param.h"

#अन्तर्भाव "V3Case.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3HierBlock.h"
#अन्तर्भाव "V3LinkDotIfaceCapture.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Parse.h"
#अन्तर्भाव "V3Simulate.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3Unroll.h"
#अन्तर्भाव "V3Width.h"

#अन्तर्भाव <cctype>
#अन्तर्भाव <cstdlib>
#अन्तर्भाव <deque>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
स्थिर AstClassRefDType* class_Ref_DType_Of_गाठ(Ast_गाठ* nodep) {
    पर्यंत (AstTypedef* स्थिरांक tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_गाठ_DType* स्थिरांक dtp = VN_CAST(nodep, गाठ_DType);
    परत dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

वर्ग ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, स्थिरांक V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<स्थिरांक std::string, Ast_गाठ_Module*>;
    using ParamConstMap = std::map<स्थिरांक std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<स्थिरांक std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    स्थिरांक तार्किक m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<स्थिरांक V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<स्थिरांक std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<स्थिरांक AstVar*> m_untypedParams;

    // METHODS

सार्वजनिक:
    ParameterizedHierBlocks(स्थिरांक V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        साठी (स्थिरांक स्वतः& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            स्थिरांक V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            साठी (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    नवीन File_ओळ{File_ओळ::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                स्थिरांक तार्किक inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.घाला({hierOpt.second.origName(), {}});
        }
        साठी (Ast_गाठ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाठ_Module)) {
            जर (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            स्थिरांक string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            स्थिरांक स्वतः defParamIt = m_modParams.find(actualModName);
            जर (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                        जर (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        स्थिरांक AstBasicDType* स्थिरांक dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        जर (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    तार्किक hierSubRun() स्थिरांक { परत m_hierSubRun; }
    तार्किक isHierBlock(स्थिरांक string& origName) स्थिरांक {
        परत m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_गाठ_Module* findByParams(स्थिरांक string& origName, AstPin* firstPinp,
                                स्थिरांक Ast_गाठ_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        स्थिरांक std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        स्थिरांक स्वतः paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        स्थिरांक AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        साठी (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            तार्किक found = सत्य;
            माप_प्रकार paramIdx = 0;
            स्थिरांक ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            साठी (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (!pinp->exprp()) सुरूठेवा;
                जर (स्थिरांक AstVar* स्थिरांक modvarp = pinp->modVarp()) {
                    AstConst* स्थिरांक constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    स्थिरांक स्वतः paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    स्थिरांक तार्किक untyped = m_untypedParams.मोजणी(paramIt->second);
                    AstConst* स्थिरांक defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    जर (defValuep && areSame(constp, defValuep)) {
                        जर (!untyped || constp->same_मूल्य_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            सुरूठेवा;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    स्थिरांक स्वतः pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    जर (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = असत्य;
                        थांबा;
                    }
                    जर (untyped && !constp->same_मूल्य_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = असत्य;
                        थांबा;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            जर (found && paramIdx == hierIt->second->params().size()) थांबा;
        }
        जर (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_गाठ::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            परत nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        स्थिरांक HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        स्थिरांक स्वतः it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        जर (it == m_hierBlockMod.end()) परत nullptr;
        परत it->second;
    }
    स्थिर तार्किक areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        जर (pinValuep->isString()) {
            परत pinValuep->अंक().toString() == hierOptParamp->अंक().toString();
        }

        जर (hierOptParamp->isDouble()) {
            द्विगुण var;
            जर (pinValuep->isDouble()) {
                var = pinValuep->अंक().toDouble();
            } अन्यथा {  // Cast from integer to real
                V3_संख्या var_अंक{pinValuep, V3_संख्या::Double{}, 0.0};
                var_अंक.opIToRD(pinValuep->अंक());
                var = var_अंक.toDouble();
            }
            परत V3_संख्या::epsilonEqual(var, hierOptParamp->अंक().toDouble());
        } अन्यथा {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_संख्या var_अंक{pinValuep, hierOptParamp->अंक().width()};
            जर (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_अंक.opRToIRoundS(pinValuep->अंक());
            } अन्यथा जर (pinValuep->isSigned()) {
                var_अंक.opExtendS(pinValuep->अंक(), pinValuep->अंक().width());
            } अन्यथा {
                var_अंक.opAssign(pinValuep->अंक());
            }
            V3_संख्या isEq{pinValuep, 1};
            isEq.opEq(var_अंक, hierOptParamp->अंक());
            परत isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

वर्ग ParamProcessor final {
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
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिरांक VNUser3InUse m_inuser3;
    स्थिरांक VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<स्थिरांक Ast_गाठ*, Ast_गाठ*>;
    संरचना ModInfo final {
        Ast_गाठ_Module* स्थिरांक m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_गाठ_Module* modp)
            : m_modp{modp} {}
    };
    std::map<स्थिरांक std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<स्थिरांक std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<स्थिरांक std::string, std::string> m_मूल्य_Names;  // Parameter value text to its name
    std::set<std::string> m_मूल्य_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    स्थिर constexpr std::string::size_type DIGEST_DIGITS = 8;

    स्थिरांक Ast_गाठ_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_मूल्य_Map = std::map<std::string, Ast_गाठ*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_गाठ_Module*, Default_मूल्य_Map> m_default_Parameter_मूल्ये;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<स्थिरांक Ast_गाठ_Module*, स्थिरांक AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, पूर्णांक>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, पूर्णांक> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<स्थिरांक AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    स्थिर शून्य makeSmallNames(Ast_गाठ_Module* modp) {
        std::vector<पूर्णांक> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (varp->isGParam() || varp->isIfaceRef()) {
                    अक्षर ch = varp->name()[0];
                    ch = std::बड़े(ch);
                    जर (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<पूर्णांक>(ch)] * 256 + ch);
                    usedLetter[static_cast<पूर्णांक>(ch)]++;
                }
            } अन्यथा जर (AstParamTypeDType* स्थिरांक typep = VN_CAST(stmtp, ParamTypeDType)) {
                स्थिरांक अक्षर ch = 'T';
                typep->user3(usedLetter[static_cast<पूर्णांक>(ch)] * 256 + ch);
                usedLetter[static_cast<पूर्णांक>(ch)]++;
            }
        }
    }
    स्थिर string paramSmallName(Ast_गाठ_Module* modp, Ast_गाठ* varp) {
        जर (varp->user3() <= 1) makeSmallNames(modp);
        पूर्णांक index = varp->user3() / 256;
        स्थिरांक अक्षर ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        पर्यंत (index) {
            st += cvtToStr(static_cast<अक्षर>((index % 25) + 'A'));
            index /= 26;
        }
        परत st;
    }

    स्थिर तार्किक is_Aggregate_Param_मूल्य(स्थिरांक Ast_गाठ* nodep) {
        परत VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    स्थिर string param_मूल्य_String(स्थिरांक Ast_गाठ* nodep) {
        जर (स्थिरांक AstRefDType* स्थिरांक refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string किल्ली = nodep->name();
        जर (स्थिरांक AstIfaceRefDType* स्थिरांक ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            जर (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                किल्ली = ifrtp->cellp()->modp()->name();
            } अन्यथा जर (ifrtp->ifacep()) {
                किल्ली = ifrtp->ifacep()->name();
            } अन्यथा {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } अन्यथा जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक dtypep
                   = VN_CAST(nodep, गाठ_UOr_Struct_DType)) {
            किल्ली += " ";
            किल्ली += dtypep->verilogKwd();
            किल्ली += " {";
            साठी (स्थिरांक Ast_गाठ* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                किल्ली += param_मूल्य_String(memberp);
                किल्ली += ";";
            }
            किल्ली += "}";
        } अन्यथा जर (स्थिरांक AstMemberDType* स्थिरांक dtypep = VN_CAST(nodep, MemberDType)) {
            किल्ली += " ";
            किल्ली += param_मूल्य_String(dtypep->subDTypep());
        } अन्यथा जर (स्थिरांक AstBasicDType* स्थिरांक dtypep = VN_CAST(nodep, BasicDType)) {
            जर (dtypep->isSigned()) किल्ली += " signed";
            जर (dtypep->isRanged()) {
                किल्ली += "[" + cvtToStr(dtypep->डावा()) + ":" + cvtToStr(dtypep->उजवा()) + "]";
            }
        } अन्यथा जर (स्थिरांक AstPackArrayDType* स्थिरांक dtypep = VN_CAST(nodep, PackArrayDType)) {
            किल्ली += "[";
            किल्ली += cvtToStr(dtypep->डावा());
            किल्ली += ":";
            किल्ली += cvtToStr(dtypep->उजवा());
            किल्ली += "] ";
            किल्ली += param_मूल्य_String(dtypep->subDTypep());
        } अन्यथा जर (स्थिरांक AstPattern* स्थिरांक patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            किल्ली += "'{";
            साठी (स्थिरांक AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                जर (memp->isDefault()) किल्ली += "default";
                जर (स्थिरांक AstText* स्थिरांक textp = VN_CAST(memp->keyp(), Text)) {
                    किल्ली += textp->text();
                } अन्यथा जर (memp->keyp()) {
                    किल्ली += param_मूल्य_String(memp->keyp());
                }
                किल्ली += ":";
                जर (memp->repp()) किल्ली += param_मूल्य_String(memp->repp()) + "x";
                साठी (स्थिरांक Ast_गाठ_अभिव्यक्ती* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), गाठ_अभिव्यक्ती)) {
                    किल्ली += param_मूल्य_String(valuep) + ";";
                }
                किल्ली += ",";
            }
            किल्ली += "}";
        } अन्यथा जर (स्थिरांक AstConsArray* स्थिरांक consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            किल्ली += "{";
            साठी (स्थिरांक AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                किल्ली += cvtToStr(itemp->index()) + ":" + param_मूल्य_String(itemp->valuep()) + ",";
            }
            जर (consArrayp->defaultp()) {
                किल्ली += "default:" + param_मूल्य_String(consArrayp->defaultp()) + ",";
            }
            किल्ली += "}";
        } अन्यथा जर (स्थिरांक AstConsStruct* स्थिरांक consp = VN_CAST(nodep, ConsStruct)) {
            किल्ली += "{";
            साठी (स्थिरांक AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                किल्ली += param_मूल्य_String(itemp->valuep());
                किल्ली += ",";
            }
            किल्ली += "}";
        } अन्यथा जर (स्थिरांक AstClassRefDType* स्थिरांक classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            जर (classRefp->classp()) {
                स्थिरांक string& className = classRefp->classp()->name();
                स्थिरांक string& origName = classRefp->classp()->origName();
                स्थिरांक तार्किक isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                जर (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } अन्यथा जर (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    किल्ली = className;
                } अन्यथा {
                    // Unspecialized class with no params - use origName
                    किल्ली = origName;
                }
            } अन्यथा {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            किल्ली = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } अन्यथा जर (स्थिरांक AstQueueDType* स्थिरांक dtypep = VN_CAST(nodep, QueueDType)) {
            किल्ली = paramElemString(dtypep->subDTypep()) + "$[$";
            जर (dtypep->boundConst()) किल्ली += ":" + cvtToStr(dtypep->boundConst());
            किल्ली += "]";
        } अन्यथा जर (स्थिरांक AstDynArrayDType* स्थिरांक dtypep = VN_CAST(nodep, DynArrayDType)) {
            किल्ली = paramElemString(dtypep->subDTypep()) + "$[]";
        } अन्यथा जर (स्थिरांक AstAssocArrayDType* स्थिरांक dtypep = VN_CAST(nodep, AssocArrayDType)) {
            किल्ली = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->किल्ली_DTypep()) + "]";
        } अन्यथा जर (स्थिरांक AstWildcardArrayDType* स्थिरांक dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            किल्ली = paramElemString(dtypep->subDTypep()) + "$[*]";
        } अन्यथा जर (स्थिरांक AstEnumDType* स्थिरांक dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            किल्ली += " enum {";
            साठी (स्थिरांक AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                स्थिरांक AstConst* स्थिरांक constp = VN_CAST(itemp->valuep(), Const);
                किल्ली += itemp->name() + "=" + (constp ? constp->अंक().ascii() : "?") + ";";
            }
            किल्ली += "}";
        } अन्यथा जर (स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = VN_CAST(nodep, गाठ_DType)) {
            किल्ली += dtypep->prettyDTypeName(सत्य);
        }
        UASSERT_OBJ(!किल्ली.empty(), nodep, "Parameter yielded no value string");
        परत किल्ली;
    }
    // As paramValueString(), for the element or key type of a container
    स्थिर string paramElemString(स्थिरांक Ast_गाठ_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        स्थिरांक AstClassRefDType* स्थिरांक classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        जर (classRefp && classRefp->paramsp()) परत classRefp->prettyDTypeName(सत्य);
        परत param_मूल्य_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    स्थिर string digestSuffix(स्थिरांक string& text, std::set<string>& usedr) {
        स्थिरांक string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        पर्यंत (digits < hex.size() && !usedr.घाला(hex.substr(0, digits)).second) ++digits;
        परत hex.substr(0, digits);
    }
    string param_मूल्य_संख्या(Ast_गाठ* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        जर (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        स्थिरांक string text = param_मूल्य_String(nodep);
        स्थिरांक स्वतः pair = m_मूल्य_Names.emplace(text, "");
        जर (pair.second) pair.first->second = "z" + digestSuffix(text, m_मूल्य_Suffixes);
        परत pair.first->second;
    }
    string moduleCalcName(स्थिरांक Ast_गाठ_Module* srcModp, स्थिरांक string& longname) {
        string newname = longname;
        जर (longname.length() > 30) {
            स्थिरांक स्वतः pair = m_longMap.emplace(longname, "");
            जर (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        परत newname;
    }
    स्थिर Ast_गाठ_DType* arraySubDTypep(Ast_गाठ_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            परत adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        जर (स्थिरांक AstBracketArrayDType* स्थिरांक adtypep = VN_CAST(nodep, BracketArrayDType)) {
            परत adtypep->subDTypep();
        }
        परत nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    स्थिर Ast_गाठ_DType* arraySubDTypeDeepp(Ast_गाठ_DType* nodep) {
        पर्यंत (Ast_गाठ_DType* स्थिरांक subp = arraySubDTypep(nodep)) nodep = subp;
        परत nodep;
    }
    स्थिर तार्किक isString(Ast_गाठ_DType* nodep) {
        जर (AstBasicDType* स्थिरांक basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            परत basicp->isString();
        परत असत्य;
    }
    शून्य collectPins(CloneMap* clonemapp, Ast_गाठ_Module* modp, तार्किक originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            स्थिरांक Ast_गाठ* originalParamp = nullptr;
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } अन्यथा जर (AstParamTypeDType* स्थिरांक ptp = VN_CAST(stmtp, ParamTypeDType)) {
                जर (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            जर (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            जर (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    शून्य relinkPins(स्थिरांक CloneMap* clonemapp, AstPin* startpinp) {
        साठी (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जर (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                स्थिरांक स्वतः cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } अन्यथा जर (pinp->modPTypep()) {
                स्थिरांक स्वतः cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } अन्यथा {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    शून्य relinkPinsByName(AstPin* startpinp, Ast_गाठ_Module* modp) {
        std::map<स्थिरांक string, AstVar*> nameToPin;
        साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        साठी (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जर (स्थिरांक AstVar* स्थिरांक varp = pinp->modVarp()) {
                स्थिरांक स्वतः varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    स्थिर तार्किक hasDescendantDefparams(स्थिरांक Ast_गाठ_Module* modp,
                                       std::set<स्थिरांक Ast_गाठ_Module*>& visited) {
        जर (!visited.घाला(modp).second) परत असत्य;
        साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* स्थिरांक cellp = VN_CAST(stmtp, Cell);
            जर (!cellp) सुरूठेवा;
            साठी (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    परत सत्य;
                }
            }
            जर (Ast_गाठ_Module* स्थिरांक childModp = cellp->modp()) {
                जर (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    परत सत्य;
                }
            }
        }
        visited.erase(modp);
        परत असत्य;
    }
    स्थिर तार्किक hasDescendantDefparams(स्थिरांक Ast_गाठ_Module* modp) {
        std::set<स्थिरांक Ast_गाठ_Module*> visited;
        परत hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    स्थिरांक AstPin* nestedDefparamp(स्थिरांक Ast_गाठ_Module* modp) {
        स्थिरांक स्वतः pair = m_nestedDefparamps.emplace(modp, nullptr);
        जर (!pair.second) परत pair.first->second;  // Searched, or being searched
        स्थिरांक AstPin* foundp = nullptr;
        modp->exists([&](स्थिरांक AstCell* cellp) {
            साठी (स्थिरांक AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (!pinp->paramPath().empty()) foundp = pinp;
            }
            जर (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            परत foundp != nullptr;
        });
        pair.first->second = foundp;
        परत foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    शून्य checkSupportedParam(Ast_गाठ_Module* modp, AstPin* pinp) स्थिरांक {
        // ConsArray is not supported because that can not be set via -G
        // option.
        जर (pinp->modVarp()) {
            तार्किक supported = असत्य;
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(pinp->exprp(), Const)) {
                supported = सत्य;
                जर (constp->अंक().isString()
                    && !V3HierBlock::stringParamPassable(constp->अंक().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            जर (!supported) {
                pinp->v3error(
                    Ast_गाठ::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } अन्यथा जर (स्थिरांक AstParamTypeDType* स्थिरांक typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_गाठ::prettyNameQ(modp->origName()));
        }
    }
    तार्किक moduleExists(स्थिरांक string& modName) स्थिरांक {
        जर (m_allModuleNames.find(modName) != m_allModuleNames.end()) परत सत्य;
        जर (m_modNameMap.find(modName) != m_modNameMap.end()) परत सत्य;
        परत असत्य;
    }

    string parameterizedHierBlockName(Ast_गाठ_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_गाठ*> pins;

        AstPin* pinp = paramPinsp;
        पर्यंत (pinp) {
            checkSupportedParam(modp, pinp);
            जर (स्थिरांक AstVar* स्थिरांक varp = pinp->modVarp()) {
                जर (!pinp->exprp()) सुरूठेवा;
                जर (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } अन्यथा जर (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), गाठ_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        स्थिरांक स्वतः pair = m_default_Parameter_मूल्ये.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        जर (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            जर (स्थिरांक AstPin* स्थिरांक defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_गाठ::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_मूल्य_Map params;
            साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                    जर (varp->isGParam()) {
                        AstConst* स्थिरांक constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } अन्यथा जर (AstParamTypeDType* स्थिरांक p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_गाठ* स्थिरांक dtypep = static_cast<Ast_गाठ*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        स्थिरांक स्वतः paramsIt = pair.first;
        जर (paramsIt->second.empty()) परत modp->origName();  // modp has no parameter

        string longname = modp->origName();
        साठी (स्वतः&& default_मूल्य : paramsIt->second) {
            स्थिरांक स्वतः pinIt = pins.find(default_मूल्य.first);
            // If the pin does not have a value assigned, use the default one.
            स्थिरांक Ast_गाठ* स्थिरांक nodep = pinIt == pins.end() ? default_मूल्य.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_मूल्य.first + "=";
            // constp can be nullptr

            जर (स्थिरांक AstConst* स्थिरांक p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->अंक().ascii(असत्य);
            } अन्यथा जर (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_वृक्ष(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        स्थिरांक स्वतः iter = m_longMap.find(longname);
        जर (iter != m_longMap.end()) परत iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.घाला(longname);
        पर्यंत (सत्य) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            स्थिरांक string hashStr = hashStrGen.digestHex();
            साठी (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                जर (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                जर (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    परत newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.घाला(V3Os::trueRandom(64));
        }
    }
    शून्य replaceRefsRecurse(Ast_गाठ* स्थिरांक nodep, स्थिरांक AstClass* स्थिरांक oldClassp,
                            AstClass* स्थिरांक newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        जर (AstClassRefDType* स्थिरांक classRefp = VN_CAST(nodep, ClassRefDType)) {
            जर (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } अन्यथा जर (AstClassOrPackageRef* स्थिरांक classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            जर (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } अन्यथा जर (AstTypedef* स्थिरांक typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            जर (typedefp->subDTypep()) {
                जर (AstClassRefDType* स्थिरांक classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    जर (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } अन्यथा जर (Ast_गाठ_FTask_Ref* स्थिरांक ftaskRefp = VN_CAST(nodep, गाठ_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            जर (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            जर (Ast_गाठ_FTask* स्थिरांक oldTaskp = ftaskRefp->taskp()) {
                जर (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        जर (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        जर (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        जर (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        जर (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        जर (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    वर्ग VarXRefRelinkVisitor final : सार्वजनिक VNVisitor {
        Ast_गाठ_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_गाठ_Module*> m_varModuleMap;  // Cache var->module lookups

    सार्वजनिक:
        explicit VarXRefRelinkVisitor(Ast_गाठ_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    खाजगी:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_गाठ_Module* findVarModule(AstVar* varp) {
            स्थिरांक स्वतः it = m_varModuleMap.find(varp);
            जर (it != m_varModuleMap.end()) परत it->second;
            Ast_गाठ_Module* varModp = nullptr;
            साठी (Ast_गाठ* np = varp; np; np = np->backp()) {
                जर (Ast_गाठ_Module* स्थिरांक modp = VN_CAST(np, गाठ_Module)) {
                    varModp = modp;
                    थांबा;
                }
            }
            m_varModuleMap[varp] = varModp;
            परत varModp;
        }

        शून्य visit(AstVarXRef* nodep) override {
            AstVar* स्थिरांक varp = nodep->varp();
            जर (!varp) {
                iterateChildren(nodep);
                परत;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            स्थिरांक string& dotted = nodep->dotted();
            जर (dotted.empty()) {
                iterateChildren(nodep);
                परत;
            }

            स्थिरांक माप_प्रकार dotPos = dotted.find('.');
            स्थिरांक string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            जर (portName.empty()) {
                iterateChildren(nodep);
                परत;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            साठी (Ast_गाठ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                जर (AstVar* स्थिरांक varChkp = VN_CAST(stmtp, Var)) {
                    जर (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        थांबा;
                    }
                }
            }
            जर (!portVarp) {
                iterateChildren(nodep);
                परत;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* स्थिरांक irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            जर (!irefp) {
                iterateChildren(nodep);
                परत;
            }

            Ast_गाठ_Module* स्थिरांक newIfacep = irefp->ifaceViaCellp();
            जर (!newIfacep) {
                iterateChildren(nodep);
                परत;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_गाठ_Module* स्थिरांक varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            जर (varModp && varModp != newIfacep) {
                साठी (Ast_गाठ* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    जर (AstVar* स्थिरांक newVarp = VN_CAST(stmtp, Var)) {
                        जर (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            थांबा;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    शून्य fixupCrossModuleRefDTypes(Ast_गाठ_Module* newModp) {
        जर (!V3LinkDotIfaceCapture::enabled()) परत;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        पूर्णांक fixed_मोजणी = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            स्थिरांक VIfaceCaptureTag* स्थिरांक tagp = V3LinkDotIfaceCapture::captureTag(refp);
            जर (!tagp) परत;

            Ast_गाठ_Module* स्थिरांक correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            जर (!correctModp || correctModp->dead()) परत;
            जर (correctModp->parameterizedTemplate()) परत;

            तार्किक fixed = असत्य;
            जर (refp->typedefp()) {
                जर (AstTypedef* स्थिरांक newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    जर (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = सत्य;
                }
            }
            जर (!fixed && refp->refDTypep()) {
                जर (Ast_गाठ_DType* स्थिरांक newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = सत्य;
                }
            }
            जर (fixed) ++fixed_मोजणी;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_मोजणी);
    }

    // Return true on success, false on error
    तार्किक deepCloneModule(Ast_गाठ_Module* srcModp, Ast_गाठ* ifErrorp, AstPin* paramsp,
                         स्थिरांक string& newname, स्थिरांक IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_गाठ_Module* newModp;
        जर (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_वृक्ष(असत्य), गाठ_Module);
        } अन्यथा {
            newModp = srcModp->clone_वृक्ष(असत्य);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        जर (AstIface* स्थिरांक newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(सत्य);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(असत्य);

        newModp->name(newname);
        newModp->user2(असत्य);  // We need to re-recurse this module once changed
        newModp->recursive(असत्य);
        newModp->recursiveClone(असत्य);
        newModp->hasGParam(असत्य);
        // Recursion may need level cleanups
        जर (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        जर ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_वृक्ष(), newModp);
            परत असत्य;
        }

        जर (AstClass* स्थिरांक newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_गाठ* insertp = srcModp;
        पर्यंत (VN_IS(insertp->nextp(), गाठ_Module)
               && VN_AS(insertp->nextp(), गाठ_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        स्थिरांक स्वतः iter = m_modNameMap.find(newname);
        CloneMap* स्थिरांक clonemapp = &(iter->second.m_cloneMap);
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
        तार्किक needRelinkByName = असत्य;
        जर (paramsp) {
            साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = सत्य;
                    थांबा;
                }
            }
        }
        जर (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } अन्यथा {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        साठी (स्वतः it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            स्थिरांक AstIfaceRefDType* स्थिरांक portIrefp = it->first;
            स्थिरांक AstIfaceRefDType* स्थिरांक pinIrefp = it->second;
            AstIfaceRefDType* स्थिरांक cloneIrefp = portIrefp->clonep();
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
        जर (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जर (pinp->exprp()) {
                जर (AstVar* स्थिरांक modvarp = pinp->modVarp()) {
                    Ast_गाठ* स्थिरांक newp = pinp->exprp();  // Const or ConsArray
                    AstConst* स्थिरांक exprp = VN_CAST(newp, Const);
                    AstConst* स्थिरांक origp = VN_CAST(modvarp->valuep(), Const);
                    स्थिरांक तार्किक overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    जर (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_वृक्ष();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_वृक्ष(असत्य));
                    modvarp->overriddenParam(overridden);
                } अन्यथा जर (AstParamTypeDType* स्थिरांक modptp = pinp->modPTypep()) {
                    Ast_गाठ_DType* स्थिरांक dtypep = VN_AS(pinp->exprp(), गाठ_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    जर (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_वृक्ष();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_वृक्ष(असत्य));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        परत सत्य;
    }
    स्थिरांक ModInfo* moduleFindOrClone(Ast_गाठ_Module* srcModp, Ast_गाठ* ifErrorp, AstPin* paramsp,
                                     स्थिरांक string& newname, स्थिरांक IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        स्वतः it = m_modNameMap.find(newname);
        जर (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } अन्यथा {
            जर (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                परत nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        स्थिरांक ModInfo* स्थिरांक modInfop = &(it->second);
        परत modInfop;
    }

    शून्य convertToStringp(Ast_गाठ* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        जर (constp && !constp->अंक().isString()) {
            constp->replaceWith(
                नवीन AstConst{constp->fileline(), AstConst::String{}, constp->अंक().toString()});
            VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    शून्य constify_Member_मूल्य(AstVar* varp, std::set<AstVar*>& inProgress) {
        जर (!varp->valuep() || VN_IS(varp->valuep(), Const)) परत;
        जर (!inProgress.घाला(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_वृक्ष();
            varp->valuep(नवीन AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            परत;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](स्थिरांक AstVarRef* refp) {
            AstVar* स्थिरांक refVarp = refp->varp();
            जर (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.घाला(refVarp);
            }
        });
        साठी (AstVar* स्थिरांक sibp : siblings) constify_Member_मूल्य(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    स्थिर AstParseRef* memberParseRef(Ast_गाठ* nodep) {
        जर (AstParseRef* स्थिरांक refp = VN_CAST(nodep, ParseRef)) परत refp;
        Ast_गाठ_Pre_Sel* स्थिरांक selp = VN_CAST(nodep, गाठ_Pre_Sel);
        परत selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    स्थिर शून्य replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_गाठ_अभिव्यक्ती* valuep) {
        जर (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } अन्यथा {
            Ast_गाठ_Pre_Sel* स्थिरांक selp = VN_AS(dotp->rhsp(), गाठ_Pre_Sel);
            जर (AstAttrOf* स्थिरांक attrp = selp->attrp()) {
                जर (Ast_गाठ* स्थिरांक oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_वृक्ष(असत्य));
                    VL_DO_DANGLING(oldFromp->delete_वृक्ष(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_वृक्ष(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    स्थिर तार्किक isDeferredMemberBase(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (VN_IS(nodep, MemberSel)) परत सत्य;
        जर (स्थिरांक Ast_गाठ_Pre_Sel* स्थिरांक selp = VN_CAST(nodep, गाठ_Pre_Sel)) {
            स्थिरांक Ast_गाठ_DType* स्थिरांक fromDTypep = selp->fromp()->dtypep();
            स्थिरांक Ast_गाठ_Array_DType* स्थिरांक arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), गाठ_Array_DType) : nullptr;
            जर (arrayDTypep) {
                स्थिरांक Ast_गाठ_DType* स्थिरांक elemDTypep = arrayDTypep->subDTypep();
                परत elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), गाठ_UOr_Struct_DType);
            }
            परत isDeferredMemberBase(selp->fromp());
        }
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = nodep->dtypep();
        परत dtypep && VN_IS(dtypep->skipRefOrNullp(), गाठ_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    स्थिर शून्य resolveMemberDot(AstDot* dotp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = VN_CAST(dotp->lhsp(), गाठ_अभिव्यक्ती);
        AstParseRef* स्थिरांक memberRefp = memberParseRef(dotp->rhsp());
        जर (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) परत;
        AstMemberSel* स्थिरांक newp = नवीन AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    शून्य resolveDotToTypedef(Ast_गाठ* exprp) {
        AstDot* स्थिरांक dotp = VN_CAST(exprp, Dot);
        जर (!dotp) परत;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* स्थिरांक classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        जर (!classRefp) {
            resolveMemberDot(dotp);
            परत;
        }
        AstParseRef* स्थिरांक parseRefp = memberParseRef(dotp->rhsp());
        जर (!parseRefp) परत;

        स्थिरांक AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* स्थिरांक paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        जर (paramTypep) {
            Ast_गाठ_DType* स्थिरांक dtypep = paramTypep->subDTypep();
            AstClassRefDType* स्थिरांक classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            जर (classRefDTypep) {
                AstClass* स्थिरांक srcClassp = classRefDTypep->classp();
                जर (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    जर (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        जर (classRefp->paramsp()) {
            AstClass* स्थिरांक srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            जर (srcClassp && srcClassp->hasGParam()) {
                जर (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        जर (AstClassRefDType* स्थिरांक crp = class_Ref_DType_Of_गाठ(classRefp->classOrPackageNodep())) {
            जर (AstClass* स्थिरांक srcClassp = crp->classp()) {
                जर (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    जर (AstClass* स्थिरांक newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        जर (!lhsClassp) परत;

        Ast_गाठ* स्थिरांक memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        जर (AstTypedef* स्थिरांक tdefp = VN_CAST(memberp, Typedef)) {
            जर (parseRefp != dotp->rhsp()) परत;
            AstRefDType* स्थिरांक refp = नवीन AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
        } अन्यथा जर (AstVar* स्थिरांक varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    शून्य substituteParamMember(AstDot* स्थिरांक dotp, AstParseRef* स्थिरांक memberRefp,
                               AstVar* स्थिरांक varp) {
        जर (!varp->isParam() || !varp->valuep()) परत;
        जर (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_मूल्य(varp, inProgress);
        }
        AstConst* स्थिरांक constp = VN_CAST(varp->valuep(), Const);
        जर (!constp) परत;
        AstConst* स्थिरांक newp = constp->clone_वृक्ष(असत्य);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    शून्य resolveParamClassRefDType(AstRefDType* refp) {
        जर (refp->typedefp() || refp->refDTypep()) परत;

        AstClassOrPackageRef* स्थिरांक classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        जर (!classRefp) परत;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        जर (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            परत;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_गाठ* targetp = classRefp->classOrPackageNodep();
        पर्यंत (स्थिरांक AstTypedef* स्थिरांक tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        जर (Ast_गाठ_DType* स्थिरांक dtargetp = VN_CAST(targetp, गाठ_DType))
            targetp = dtargetp->skipRefOrNullp();
        जर (!targetp) परत;
        AstClassRefDType* स्थिरांक classRefDTypep = VN_CAST(targetp, ClassRefDType);
        जर (!classRefDTypep) परत;
        srcClassp = classRefDTypep->classp();
        जर (!srcClassp) परत;

        AstClass* newClassp = srcClassp;
        जर (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* स्थिरांक typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        जर (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    स्थिर तार्किक defaultParamsHaveTypeQueryOnParamType(स्थिरांक AstClassRefDType* nodep) {
        तार्किक found = असत्य;
        nodep->foreach([&](स्थिरांक AstAttrOf* attrp) {
            जर (found || !attrp->attrType().isTypeQuery()) परत;
            स्थिरांक AstRefDType* स्थिरांक refp = VN_CAST(attrp->fromp(), RefDType);
            जर (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = सत्य;
        });
        परत found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    तार्किक classTypeMatchesDefaultClone(स्थिरांक Ast_गाठ_DType* exprp, स्थिरांक Ast_गाठ_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        स्थिरांक स्वतः* स्थिरांक अभिव्यक्ती_Class_Refp = VN_CAST(exprp, ClassRefDType);
        स्थिरांक स्वतः* स्थिरांक origClassRefp = VN_CAST(origp, ClassRefDType);
        जर (!अभिव्यक्ती_Class_Refp || !origClassRefp) परत असत्य;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        स्थिरांक Ast_गाठ_Module* स्थिरांक defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        जर (defaultClonep && defaultClonep == अभिव्यक्ती_Class_Refp->classp()) परत सत्य;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        जर (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) परत असत्य;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        जर (!origClassRefp->classp()->hasGParam()) परत असत्य;
        जर (origClassRefp->classp()->origName() != अभिव्यक्ती_Class_Refp->classp()->origName())
            परत असत्य;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        जर (!m_defaultCloneInProgress.घाला(origClassRefp->classp()).second) परत असत्य;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* स्थिरांक origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_वृक्ष(असत्य));
        Ast_गाठ_Module* स्थिरांक resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        स्थिरांक तार्किक match = resolvedModp && VN_CAST(resolvedModp, Class) == अभिव्यक्ती_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_वृक्ष(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        परत match;
    }

    स्थिर तार्किक paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        जर (exprp->अंक().width() == origp->अंक().width()) परत असत्य;
        जर (exprp->अंक().isOpaque()) परत असत्य;
        जर (origp->अंक().isOpaque()) परत असत्य;
        स्थिरांक पूर्णांक maxWidth = std::max(exprp->अंक().width(), origp->अंक().width());
        V3_संख्या अभिव्यक्ती_अंक{exprp, maxWidth};
        जर (exprp->अंक().isSigned()) {
            अभिव्यक्ती_अंक.opExtendS(exprp->अंक(), exprp->अंक().width());
        } अन्यथा {
            अभिव्यक्ती_अंक.opAssign(exprp->अंक());
        }
        V3_संख्या orig_अंक{origp, maxWidth};
        जर (origp->अंक().isSigned()) {
            orig_अंक.opExtendS(origp->अंक(), origp->अंक().width());
        } अन्यथा {
            orig_अंक.opAssign(origp->अंक());
        }
        V3_संख्या isEq{exprp, 1};
        isEq.opEq(अभिव्यक्ती_अंक, orig_अंक);
        परत isEq.isNeqZero();
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
    शून्य resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जर (!pinp->exprp()) सुरूठेवा;
            जर (!VString::startsWith(pinp->name(), "__VGIfaceParam")) सुरूठेवा;
            स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक fwdRefp = VN_CAST(pinp->exprp(), गाठ_Var_Ref);
            जर (!fwdRefp) सुरूठेवा;
            स्थिरांक AstIfaceRefDType* स्थिरांक resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* स्थिरांक newIrefp = नवीन AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            जर (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_वृक्ष(सत्य));
            }
            pinp->exprp()->unlinkFrBack()->delete_वृक्ष();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    शून्य name_By_Pin_मूल्य(AstPin* pinp, Ast_गाठ_Module* srcModp, AstVar* modvarp, string& longnamer,
                        तार्किक& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_मूल्य_संख्या(pinp->exprp());
        any_overridesr = सत्य;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    शून्य cellPinCleanup(Ast_गाठ* nodep, AstPin* pinp, AstPin* paramsp, Ast_गाठ_Module* srcModp,
                        string& longnamer, तार्किक& any_overridesr,
                        std::unordered_set<स्थिरांक AstPin*>& overridingTypePinsr) {
        जर (!pinp->exprp()) परत;  // No-connect
        जर (AstVar* स्थिरांक modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            जर (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अन्यथा जर (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_मूल्य(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } अन्यथा {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                जर (is_Aggregate_Param_मूल्य(pinp->exprp())) {
                    name_By_Pin_मूल्य(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    परत;
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
                जर (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    स्थिरांक तार्किक dependent = modvarp->valuep()->exists(
                        [](Ast_गाठ* np) { परत VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    जर (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                जर (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* स्थिरांक exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* स्थिरांक origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                जर (exprp && !exprp->अंक().isDouble() && !exprp->अंक().isString()) {
                    AstVar* cloneVarp = modvarp->clone_वृक्ष(असत्य);
                    तार्किक cloneVarpUnresolved = असत्य;
                    जर (Ast_गाठ* स्थिरांक oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_वृक्ष(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_वृक्ष(असत्य));
                    जर (Ast_गाठ_DType* स्थिरांक origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        जर (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_वृक्ष();
                        cloneVarp->childDTypep(origDTypep->clone_वृक्ष(असत्य));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr पूर्णांक maxSubstIters = 1000;
                        साठी (पूर्णांक it = 0; it < maxSubstIters; ++it) {
                            तार्किक any = असत्य;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* स्थिरांक targetp = varrefp->varp();
                                Ast_गाठ* replacep = nullptr;
                                साठी (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    जर (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        जर (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_वृक्ष(असत्य);
                                        }
                                        थांबा;
                                    }
                                }
                                जर (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_वृक्ष(असत्य);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* स्थिरांक patp = VN_CAST(replacep, Pattern);
                                जर (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_वृक्ष(असत्य));
                                }
                                जर (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_वृक्ष(), varrefp);
                                    any = सत्य;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_गाठ_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* स्थिरांक ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                जर (!ptdp) परत;
                                AstPin* overridePinp = nullptr;
                                साठी (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    जर (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        थांबा;
                                    }
                                }
                                Ast_गाठ_DType* स्थिरांक substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), गाठ_DType)
                                                   : ptdp->subDTypep();
                                जर (substp) toReplace.emplace_back(refp, substp);
                            });
                            साठी (स्वतः it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* स्थिरांक refp = it->first;
                                refp->replaceWith(it->second->clone_वृक्ष(असत्य));
                                VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
                                any = सत्य;
                            }
                            जर (!any) थांबा;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_गाठ* np) {
                            जर (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = सत्य;
                                परत;
                            }
                            जर (AstEnumItemRef* स्थिरांक itemRefp = VN_CAST(np, EnumItemRef)) {
                                जर (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = सत्य;
                                }
                                परत;
                            }
                            AstRefDType* स्थिरांक refp = VN_CAST(np, RefDType);
                            जर (!refp) परत;
                            जर (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = सत्य;
                            }
                            // A type declared in the template must not be widthed there
                            जर (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = सत्य;
                            }
                        });
                    }
                    जर (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        जर (AstConst* स्थिरांक widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            जर (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_वृक्ष(), cloneVarp);
                }
                AstConst* स्थिरांक namingExprp = normedNamep ? normedNamep : exprp;
                जर (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_गाठ* स्थिरांक pinExprp = pinp->exprp();
                    pinExprp->replaceWith(नवीन AstConst{pinp->fileline(), AstConst::Widthed_मूल्य{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_वृक्ष(), pinExprp);
                } अन्यथा जर (origp
                           && (exprp->same_वृक्ष(origp)
                               || (exprp->अंक().width() == origp->अंक().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } अन्यथा जर (namingExprp->अंक().isDouble() || namingExprp->अंक().isString()
                           || namingExprp->अंक().isFourState()
                           || namingExprp->अंक().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_मूल्य_संख्या(namingExprp));
                    any_overridesr = सत्य;
                } अन्यथा {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->अंक().ascii(असत्य));
                    any_overridesr = सत्य;
                }
                जर (normedNamep) VL_DO_DANGLING(normedNamep->delete_वृक्ष(), normedNamep);
            }
        } अन्यथा जर (AstParamTypeDType* स्थिरांक modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_गाठ_DType* rawTypep = VN_CAST(pinp->exprp(), गाठ_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            तार्किक skipWidthForTemplateStruct = असत्य;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_गाठ_DType* स्थिरांक resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                जर (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_गाठ_Module* स्थिरांक ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    जर (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = सत्य;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                जर (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_गाठ_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            स्थिरांक Ast_गाठ_DType* origp = modvarp->skipRefToNonRefp();
            जर (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अन्यथा जर (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } अन्यथा {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                जर (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), गाठ_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                जर (!modvarp->fwdType().is_गाठ_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                जर (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } अन्यथा {
                    जर (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), गाठ_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    जर (AstClassRefDType* स्थिरांक classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        जर (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } अन्यथा {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    शून्य cellInterfaceCleanup(AstPin* pinsp, Ast_गाठ_Module* srcModp, string& longnamer,
                              तार्किक& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        साठी (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            स्थिरांक AstVar* स्थिरांक modvarp = pinp->modVarp();
            जर (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                सुरूठेवा;
            }
            जर (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* स्थिरांक portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                स्थिरांक Ast_गाठ* स्थिरांक exprp = pinp->exprp();
                जर (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    सुरूठेवा;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                जर (स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक vrp = VN_CAST(exprp, गाठ_Var_Ref)) {
                    जर (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                जर (!pinIrefp && exprp) {
                    स्थिरांक Ast_गाठ* basep = exprp;
                    पर्यंत (स्थिरांक Ast_गाठ_Pre_Sel* स्थिरांक selp = VN_CAST(basep, गाठ_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    जर (स्थिरांक AstVarRef* स्थिरांक vrp = VN_CAST(basep, VarRef)) {
                        जर (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                जर (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } अन्यथा जर (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } अन्यथा {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    जर (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_मूल्य_संख्या(pinIrefp));
                        any_overridesr = सत्य;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        जर (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_गाठ::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_गाठ::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    शून्य storeOriginalParams(Ast_गाठ_Module* स्थिरांक classp) {
        साठी (Ast_गाठ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_गाठ* originalParamp = nullptr;
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (varp->isGParam()) originalParamp = varp->clonep();
            } अन्यथा जर (AstParamTypeDType* स्थिरांक ptp = VN_CAST(stmtp, ParamTypeDType)) {
                जर (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            जर (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    शून्य genericInterfaceVarSetup(स्थिरांक AstPin* स्थिरांक paramsp, स्थिरांक AstPin* स्थिरांक pinsp) {
        std::unordered_map<string, स्थिरांक AstPin*> paramspMap;
        साठी (स्थिरांक AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जर (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.घाला({pinp->name().substr(std::म_माप("__VGIfaceParam")), pinp});
            }
        }

        जर (paramspMap.empty()) परत;

        साठी (स्थिरांक Ast_गाठ* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstPin* स्थिरांक pinp = VN_CAST(nodep, Pin)) {
                जर (AstVar* स्थिरांक varp = pinp->modVarp()) {
                    Ast_गाठ_DType* dtypep = varp->childDTypep()->elemDTypep();
                    जर (AstIfaceGenericDType* स्थिरांक ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        स्थिरांक स्वतः iter = paramspMap.find(varp->name());
                        जर (iter == paramspMap.end()) सुरूठेवा;
                        Ast_गाठ* स्थिरांक backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        स्थिरांक AstPin* स्थिरांक paramp = iter->second;
                        paramspMap.erase(iter);
                        स्थिरांक AstIfaceRefDType* स्थिरांक ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* स्थिरांक newIfacerefp = नवीन AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        जर (स्वतः* स्थिरांक arrDtp = VN_CAST(backp, गाठ_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अन्यथा जर (स्वतः* स्थिरांक arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अन्यथा {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ढकला_Deletep(ifaceGDTypep), ifaceGDTypep);
                        जर (paramspMap.empty()) परत;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    शून्य resolveDefaultParams(Ast_गाठ* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        जर (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            जर (!classp) परत;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } अन्यथा जर (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } अन्यथा {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        साठी (Ast_गाठ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                जर (!paramTypep->isGParam()) सुरूठेवा;
                m_paramIndex.emplace(paramTypep, static_cast<पूर्णांक>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_गाठ_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        साठी (माप_प्रकार i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* स्थिरांक paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* स्थिरांक reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            जर (!reqDtp) सुरूठेवा;

            // If default is a reference to another param type, record dependency
            जर (AstRefDType* स्थिरांक refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                जर (AstParamTypeDType* स्थिरांक sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    स्वतः it = m_paramIndex.find(sourceParamp);
                    जर (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    सुरूठेवा;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            जर (Ast_गाठ_DType* स्थिरांक dtp = VN_CAST(reqDtp->lhsp(), गाठ_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* स्थिरांक typep = pinp->modPTypep();
            स्थिरांक स्वतः it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            जर (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } अन्यथा {
                pinsByIndex.ढकला_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        साठी (तार्किक resolvedSome = सत्य; resolvedSome;) {
            resolvedSome = असत्य;
            साठी (माप_प्रकार paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                जर (pinsByIndex[paramIdx]) सुरूठेवा;
                स्थिरांक पूर्णांक sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                जर (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_वृक्ष(असत्य);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                जर (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_गाठ_DType* स्थिरांक dtypep = defaultTypeNodes[paramIdx];
                    newPinp = नवीन AstPin{dtypep->fileline(), static_cast<पूर्णांक>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_वृक्ष(असत्य)};
                }

                जर (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(सत्य);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    जर (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } अन्यथा जर (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    जर (!paramsp) paramsp = newPinp;
                    resolvedSome = सत्य;
                }
            }
        }
    }

    Ast_गाठ_Module* गाठ_Deparam_Common(Ast_गाठ* nodep, Ast_गाठ_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, तार्किक any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        जर (debug() >= 9 && paramsp) paramsp->dump_वृक्ष_And_Next(cout, "-  cellparams: ");

        जर (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } अन्यथा {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<स्थिरांक AstPin*> overridingTypePins;
            साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (!pinp->exprp()) सुरूठेवा;  // An empty override keeps the default
                जर (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    सुरूठेवा;
                }
                AstVar* स्थिरांक modvarp = pinp->modVarp();
                जर (!modvarp || !modvarp->isGParam()) सुरूठेवा;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                स्थिरांक AstPattern* स्थिरांक patternp = VN_CAST(pinp->exprp(), Pattern);
                जर (patternp && !patternp->childDTypep()) सुरूठेवा;
                जर (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_मूल्य(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (AstParamTypeDType* स्थिरांक modptp = pinp->modPTypep()) {
                    जर (!overridingTypePins.मोजणी(pinp)) सुरूठेवा;
                    Ast_गाठ_DType* स्थिरांक typep = VN_AS(pinp->exprp(), गाठ_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_मूल्य_संख्या(typep->skipRefToNonRefp());
                    any_overrides = सत्य;
                    सुरूठेवा;
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
        तार्किक defaultsResolved = असत्य;
        जर (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            साठी (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                जर (pinp->modPTypep()) {
                    any_overrides = सत्य;
                    defaultsResolved = सत्य;
                    थांबा;
                }
            }
        }
        जर (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = सत्य;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_गाठ_Module* newModp = nullptr;
        जर (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_गाठ_Module* स्थिरांक paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            जर (!paramedModp) परत nullptr;  // Unsupported, already reported
            paramedModp->dead(असत्य);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } अन्यथा जर (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            जर (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_गाठ_Module* गाठ_Copyp = srcModp->clone_वृक्ष(असत्य);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << गाठ_Copyp);
                m_deleter.ढकला_Deletep(गाठ_Copyp);  // nodeCopyp used past here
                srcModp->user3p(गाठ_Copyp);
                storeOriginalParams(गाठ_Copyp);
            }
            newModp = srcModp;
        } अन्यथा {
            स्थिरांक string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            स्थिरांक ModInfo* स्थिरांक modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            जर (!modInfop) परत nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        स्थिरांक तार्किक cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* स्थिरांक newClassp = VN_CAST(newModp, Class);
        जर (newClassp && !newClassp->user2()) m_specializedClassps.ढकला_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        जर (defaultsResolved) srcModp->user4p(newModp);

        साठी (स्वतः* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                जर (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        जर (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_वृक्ष(), paramsp);
        परत newModp;
    }

    Ast_गाठ_Module* cellDeparam(AstCell* nodep, Ast_गाठ_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_गाठ_Module* स्थिरांक newModp = गाठ_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        जर (!newModp) परत nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(असत्य);
        परत newModp;
    }
    Ast_गाठ_Module* ifaceRefDeparam(AstIfaceRefDType* स्थिरांक nodep, Ast_गाठ_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        जर (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            स्थिरांक string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            स्थिरांक string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            स्थिरांक माप_प्रकार ownerPos = ownerBaseName.find("__");
            जर (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            स्थिरांक माप_प्रकार srcPos = srcBaseName.find("__");
            जर (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            जर (ownerBaseName == srcBaseName) {
                तार्किक allOwnParams = सत्य;
                साठी (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    जर (AstRefDType* स्थिरांक refp = VN_CAST(pinp->exprp(), RefDType)) {
                        जर (AstParamTypeDType* स्थिरांक ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_गाठ_Module* स्थिरांक ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            जर (ptdOwnerp != m_modp) allOwnParams = असत्य;
                        } अन्यथा {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } अन्यथा {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                जर (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    जर (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_वृक्ष();
                    परत ownerIfacep;
                }
            }
        }

        Ast_गाठ_Module* स्थिरांक newModp
            = गाठ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, असत्य);
        जर (!newModp) परत nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        परत newModp;
    }
    Ast_गाठ_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_गाठ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_गाठ_Module* स्थिरांक newModp
            = गाठ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, असत्य);
        जर (!newModp) परत nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* स्थिरांक refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* स्थिरांक newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        जर (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            जर (AstTypedef* स्थिरांक typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        परत newModp;
    }
    Ast_गाठ_Module* classRefDeparam(AstClassRefDType* nodep, Ast_गाठ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_गाठ_Module* स्थिरांक newModp
            = गाठ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, असत्य);
        जर (!newModp) परत nullptr;
        AstClass* स्थिरांक newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        परत newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    वर्ग DeferredResolverVisitor final : सार्वजनिक VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<स्थिरांक Ast_गाठ*> m_reachedDecls;

        तार्किक firstReach(स्थिरांक Ast_गाठ* स्थिरांक declp) { परत m_reachedDecls.घाला(declp).second; }

        शून्य iterateParamType(AstParamTypeDType* स्थिरांक paramTypep) {
            जर (firstReach(paramTypep)) iterate(paramTypep);
        }
        शून्य iterateTypedef(AstTypedef* स्थिरांक tdefp) {
            जर (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }
        शून्य visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_गाठ* स्थिरांक targetp = nodep->classOrPackageNodep();
            जर (AstTypedef* स्थिरांक tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } अन्यथा जर (AstParamTypeDType* स्थिरांक paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        शून्य visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        शून्य visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* स्थिरांक tdefp = nodep->typedefp();
            जर (tdefp) {
                iterateTypedef(tdefp);
            } अन्यथा जर (AstParamTypeDType* स्थिरांक paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        शून्य visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* स्थिरांक varp = nodep->varp();
            जर (firstReach(varp)) {
                iterate(varp->subDTypep());
                स्थिरांक स्वतः& deferredVarps = v3Global.rootp()->deferredParamVarps();
                जर (varp->varType() == VVarType::LPARAM && deferredVarps.मोजणी(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    सार्वजनिक:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_गाठ* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

सार्वजनिक:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    शून्य retargetIfaceRefs(Ast_गाठ_Module* parentModp, स्थिरांक string& cellName) {
        Ast_गाठ_Module* स्थिरांक correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        जर (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) परत;
        parentModp->foreach([&](AstRefDType* refp) {
            स्थिरांक VIfaceCaptureTag* स्थिरांक tagp = V3LinkDotIfaceCapture::captureTag(refp);
            जर (!tagp || tagp->m_cellPath != cellName) परत;
            // Only retarget REFDTYPEs owned by parentModp.
            जर (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) परत;
            जर (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    शून्य resolveDeferredDotsReachableFrom(Ast_गाठ* rootp, स्थिरांक Ast_गाठ_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*हे, rootp};
    }

    Ast_गाठ_Module* गाठ_Deparam(Ast_गाठ* nodep, Ast_गाठ_Module* srcModp, Ast_गाठ_Module* modp,
                               स्थिरांक string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_गाठ::गाठ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string गाठ_Name = nodep->name();
        जर (AstIfaceRefDType* स्थिरांक ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            जर (गाठ_Name.empty()) गाठ_Name = ifaceRefp->cellName();
        }
        स्थिरांक string instanceName
            = गाठ_Name.empty() ? someInstanceName : (someInstanceName + "." + गाठ_Name);

        जर (AstCell* स्थिरांक cellp = VN_CAST(nodep, Cell)) {
            साठी (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* स्थिरांक nextp = VN_AS(pinp->nextp(), Pin);
                जर (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_वृक्ष(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_गाठ::गाठ_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_गाठ_Module* newModp = nullptr;
        जर (AstCell* स्थिरांक cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } अन्यथा जर (AstIfaceRefDType* स्थिरांक ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } अन्यथा जर (AstClassRefDType* स्थिरांक classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अन्यथा जर (AstClassOrPackageRef* स्थिरांक classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अन्यथा {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        जर (!newModp) परत nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_गाठ::गाठ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        परत newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        परत taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        साठी (Ast_गाठ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाठ_Module)) {
            m_allModuleNames.घाला(modp->name());
        }
    }
    ~ParamProcessor() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

वर्ग ClassRefUnlinkerVisitor final : सार्वजनिक VNVisitor {

सार्वजनिक:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        जर (nodep->paramsp()) {
            जर (AstClass* स्थिरांक classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                जर (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* स्थिरांक dotp = VN_CAST(nodep->backp(), Dot);
                    जर (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(ढकला_Deletep(dotp), dotp, nodep);
                    } अन्यथा {
                        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    शून्य visit(AstClass* nodep) override {}  // don't iterate inside classes
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

वर्ग ParamClassRefDTypeRelinkVisitor final : सार्वजनिक VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<स्थिरांक AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_गाठ_Module* m_ownerModp = nullptr;
    Ast_गाठ_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    शून्य ensureOwnerMap() {
        जर (m_ownerMapForp == m_ownerModp) परत;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        जर (!m_ownerModp) परत;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        जर (AstClass* स्थिरांक ownerClassp = VN_CAST(m_ownerModp, Class)) {
            जर (ownerClassp->hasGParam()) परत;
        }
        साठी (Ast_गाठ* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* स्थिरांक tdp = VN_CAST(sp, Typedef);
            जर (!tdp) सुरूठेवा;
            AstClassRefDType* स्थिरांक crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            जर (!crdtp || !crdtp->classp()) सुरूठेवा;
            AstClass* स्थिरांक targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            जर (targetp->hasGParam()) सुरूठेवा;
            स्थिरांक std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            स्थिरांक स्वतः pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            जर (!pair.second && pair.first->second != targetp) m_ambiguous.घाला(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    स्थिर string classOrigName(स्थिरांक AstClass* classp) {
        परत classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    स्थिर AstTypedef* findTypedefWithBases(AstClass* classp, स्थिरांक string& name) {
        साठी (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            जर (AstTypedef* स्थिरांक tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                जर (tdp->subDTypep()) परत tdp;
            }
        }
        परत nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    शून्य retargetRefDType(AstRefDType* refp) {
        जर (!m_ownerModp) परत;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        जर (AstClass* स्थिरांक ownerClassp = VN_CAST(m_ownerModp, Class)) {
            जर (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                स्थिरांक AstClass* refClassp = nullptr;
                जर (AstTypedef* स्थिरांक tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } अन्यथा जर (स्थिरांक AstClassOrPackageRef* स्थिरांक classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                जर (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* स्थिरांक selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    जर (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        परत;
                    }
                }
            }
        }

        AstTypedef* स्थिरांक oldTdp = refp->typedefp();
        जर (!oldTdp) परत;
        AstClass* स्थिरांक oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        जर (!oldOwnerp) परत;

        ensureOwnerMap();
        जर (m_origNameToClone.empty()) परत;
        स्थिरांक std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        जर (m_ambiguous.मोजणी(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            परत;
        }
        स्थिरांक स्वतः it = m_origNameToClone.find(origName);
        जर (it == m_origNameToClone.end()) परत;
        AstClass* स्थिरांक correctClassp = it->second;
        जर (correctClassp == oldOwnerp) परत;
        AstTypedef* स्थिरांक newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        जर (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            परत;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        जर (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            परत;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

सार्वजनिक:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstParamTypeDType* स्थिरांक ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        जर (AstParamTypeDType* स्थिरांक paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            स्थिरांक स्वतः it = m_paramTypeClassMap.find(paramtypep);
            जर (it != m_paramTypeClassMap.end()) {
                AstClass* स्थिरांक origClassp = it->second;
                जर (origClassp->hasGParam() && !origClassp->user3p()) {
                    जर (AstClass* स्थिरांक parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        स्थिरांक string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        साठी (Ast_गाठ* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            जर (AstParamTypeDType* स्थिरांक ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                जर (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    थांबा;
                                }
                            }
                        }
                        जर (newParamTypep) {
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
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

वर्ग ParamState final {
सार्वजनिक:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<तार्किक, पूर्णांक>;
    std::multimap<WQKey, Ast_गाठ_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_गाठ_Module*, std::unordered_set<Ast_गाठ_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

वर्ग ParamVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    तार्किक m_iterateModule = असत्य;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<तार्किक, Ast_गाठ*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<स्थिरांक Ast_गाठ*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<स्थिरांक AstRefDType*, तार्किक>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<स्थिरांक Ast_गाठ_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाठ_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    तार्किक m_modIfaceRefsDone = असत्य;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    तार्किक m_inGenerateCond = असत्य;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    शून्य processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_गाठ_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = सत्य;

        // Visit all cells under module, recursively
        पर्यंत (सत्य) {
            // Classes specialized since the last pass still need their bodies elaborated
            साठी (AstClass* स्थिरांक classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{सत्य, classp->level()}, classp);
            }
            जर (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            जर (workQueue.empty()) थांबा;

            स्थिरांक स्वतः itm = workQueue.cbegin();
            Ast_गाठ_Module* स्थिरांक modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            जर (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                जर (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = असत्य;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            पर्यंत (!m_cellps.empty()) {
                स्थिरांक स्वतः itim = m_cellps.cbegin();
                Ast_गाठ* स्थिरांक cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                स्थिरांक स्वतः genHierIt = m_genHierNames.find(cellp);
                जर (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_गाठ_Module* srcModp = nullptr;
                जर (स्थिरांक AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } अन्यथा जर (स्थिरांक AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    जर (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) सुरूठेवा;
                } अन्यथा जर (स्थिरांक AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } अन्यथा जर (स्थिरांक AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } अन्यथा {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                जर (!srcModp) सुरूठेवा;

                // Update path
                स्थिरांक string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                जर (Ast_गाठ_Module* स्थिरांक newModp
                    = m_processor.गाठ_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    जर (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        जर (V3LinkDotIfaceCapture::enabled()) {
                            जर (स्थिरांक AstCell* स्थिरांक modCellp = VN_CAST(cellp, Cell)) {
                                जर (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].घाला(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    जर (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = असत्य;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    स्थिर string getRefBaseName(स्थिरांक AstVarXRef* refp) {
        स्थिरांक string dotted = refp->dotted();
        जर (dotted.empty()) परत "";
        परत dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    शून्य logTemplateLeakRefs(Ast_गाठ_Module* parentModp, Ast_गाठ_Module* templateModp,
                             स्थिरांक अक्षर* stage, Ast_गाठ* contextp) {
        जर (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) परत;
        // LCOV_EXCL_START  // Debug-only diagnostic
        पूर्णांक leak_मोजणी = 0;
        स्थिरांक स्वतः ancestryOf = [](स्थिरांक Ast_गाठ* nodep) {
            string ancestry;
            साठी (स्थिरांक Ast_गाठ* curp = nodep; curp; curp = curp->backp()) {
                जर (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                जर (VN_IS(curp, गाठ_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    थांबा;
                }
            }
            परत ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            जर (refp->typedefp()) {
                Ast_गाठ_Module* स्थिरांक tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                जर (tdOwnerp == templateModp) {
                    ++leak_मोजणी;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_गाठ::गाठ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            जर (refp->refDTypep()) {
                Ast_गाठ_Module* स्थिरांक rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                जर (rdOwnerp == templateModp) {
                    ++leak_मोजणी;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_गाठ::गाठ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            जर (!varrefp->varp()) परत;
            Ast_गाठ_Module* स्थिरांक varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            जर (varOwnerp != templateModp) परत;
            ++leak_मोजणी;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_गाठ::गाठ_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        जर (leak_मोजणी > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_मोजणी);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    शून्य checkParamNotHierRecurse(Ast_गाठ* nodep, तार्किक underTypeQuery = असत्य) {
        साठी (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            स्थिरांक AstAttrOf* स्थिरांक attrp = VN_CAST(nodep, AttrOf);
            स्थिरांक तार्किक childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            जर (स्थिरांक AstVarXRef* स्थिरांक refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                तार्किक isIfaceRef = असत्य;
                जर (refp->varp() && refp->varp()->isIfaceParam()) {
                    स्थिरांक string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.मोजणी(refname) || m_ifaceInstCells.मोजणी(refname));
                }
                जर (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } अन्यथा जर (स्थिरांक Ast_गाठ_FTask_Ref* स्थिरांक refp = VN_CAST(nodep, गाठ_FTask_Ref)) {
                जर (refp->dotted() != "") {
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
    शून्य specializeNestedIfaceCells(Ast_गाठ_Module* ifaceModp) {
        साठी (Ast_गाठ* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* स्थिरांक nestedCellp = VN_CAST(stmtp, Cell);
            जर (!nestedCellp) सुरूठेवा;
            जर (!VN_IS(nestedCellp->modp(), Iface)) सुरूठेवा;
            जर (!nestedCellp->paramsp()) सुरूठेवा;

            Ast_गाठ_Module* स्थिरांक nestedSrcModp = nestedCellp->modp();
            जर (Ast_गाठ_Module* स्थिरांक nestedNewModp = m_processor.गाठ_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                जर (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    साठी (Ast_गाठ* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        जर (AstVar* स्थिरांक varp = VN_CAST(sp, Var)) {
                            जर (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    जर (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    तार्किक cellParamsReferenceIfacePorts(AstCell* cellp) {
        जर (!cellp->paramsp()) परत असत्य;

        साठी (Ast_गाठ* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            जर (AstPin* स्थिरांक pinp = VN_CAST(nodep, Pin)) {
                जर (Ast_गाठ* स्थिरांक exprp = pinp->exprp()) {
                    जर (स्थिरांक AstVarXRef* स्थिरांक refp = VN_CAST(exprp, VarXRef)) {
                        स्थिरांक string refname = getRefBaseName(refp);
                        जर (!refname.empty()
                            && (m_ifacePortNames.मोजणी(refname)
                                || m_ifaceInstCells.मोजणी(refname)))
                            परत सत्य;
                    }
                }
            }
        }
        परत असत्य;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    शून्य expandIfaceArrayPortDimensions(AstVar* portp, Ast_गाठ_DType* dtypep,
                                        स्थिरांक std::string& suffix) {
        स्थिरांक AstUnpackArrayDType* स्थिरांक arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        जर (!arrp) {
            AstVar* स्थिरांक varp = portp->clone_वृक्ष(असत्य);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            जर (Ast_गाठ_DType* स्थिरांक oldp = varp->childDTypep()) {
                VL_DO_DANGLING(ढकला_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_वृक्ष(असत्य));
            portp->addNextHere(varp);
            परत;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        साठी (स्थिरांक पूर्णांक n : arrp->declRange().seq_उजवा_To_डावा()) {
            स्थिरांक std::string s = suffix + "__BRA__" + Ast_गाठ::encode_संख्या(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    शून्य expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   स्थिरांक AstRange* rangep, स्थिरांक std::string& suffix, पूर्णांक idx) {
        // Base case: insert the element when no dimensions left
        जर (!rangep) {
            AstCell* स्थिरांक elemp = arrayedCellp->clone_वृक्ष(असत्य);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            जर (ifaceVarp) {
                AstIfaceRefDType* स्थिरांक irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_वृक्ष(असत्य);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* स्थिरांक varp = ifaceVarp->clone_वृक्ष(असत्य);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                जर (Ast_गाठ_DType* स्थिरांक oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(ढकला_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            परत;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        स्थिरांक AstRange* स्थिरांक subRangep = VN_AS(rangep->nextp(), Range);
        साठी (स्थिरांक पूर्णांक n : rangep->seq_उजवा_To_डावा()) {
            स्थिरांक std::string s = suffix + "__BRA__" + Ast_गाठ::encode_संख्या(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    शून्य visitCellOrClassRef(Ast_गाठ* nodep, तार्किक isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        जर (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* स्थिरांक cellp = VN_CAST(nodep, Cell);
            जर (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_गाठ_Module* स्थिरांक srcModp = cellp->modp();
                Ast_गाठ_Module* स्थिरांक newModp
                    = m_processor.गाठ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                जर (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                जर (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        जर (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        जर (nodep->recursiveClone()) nodep->dead(सत्य);  // Fake, made for recursive elimination
        जर (nodep->dead()) परत;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        जर (AstClass* स्थिरांक classp = VN_CAST(nodep, Class)) {
            जर (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ढकला_back(classp);
                परत;
            }
        }

        जर (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            परत;
        }

        // Start traversal at root-like things
        जर (nodep->is_शीर्ष()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    तार्किक isCircularType(स्थिरांक AstRefDType* nodep) {
        स्थिरांक स्वतः iter = m_isCircular.emplace(nodep, सत्य);
        जर (!iter.second) परत iter.first->second;
        जर (स्थिरांक AstRefDType* स्थिरांक subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            स्थिरांक तार्किक ret = isCircularType(subDTypep);
            iter.first->second = ret;
            परत ret;
        }
        iter.first->second = असत्य;
        परत असत्य;
    }

    शून्य visit(AstRefDType* nodep) override {
        स्थिरांक तार्किक isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        जर (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        जर (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } अन्यथा जर (nodep->typedefp() && nodep->subDTypep()
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
    शून्य visit(AstCell* nodep) override {
        जर (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            साठी (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* स्थिरांक ifaceVarp = [&]() -> AstVar* {
                जर (!VN_IS(nodep->modp(), Iface)) परत nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* स्थिरांक varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                परत varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            जर (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(सत्य);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* स्थिरांक firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* स्थिरांक irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            परत;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        जर (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        जर (nodep->ifacep()) visitCellOrClassRef(nodep, सत्य);
    }
    शून्य visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, असत्य);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        जर (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, असत्य);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    स्थिर शून्य gatherVars(Ast_गाठ* stmtsp, तार्किक (*matchp)(स्थिरांक AstVar*), VarsByName& varsr) {
        साठी (Ast_गाठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                जर (matchp(varp)) varsr.emplace(varp->name(), varp);
            } अन्यथा जर (AstGenBlock* स्थिरांक genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    स्थिरांक VarsByName& modIfaceRefs() {
        जर (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](स्थिरांक AstVar* varp) { परत varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = सत्य;
        }
        परत m_modIfaceRefs;
    }
    स्थिरांक VarsByName& ifaceParams(Ast_गाठ_Module* ifacep) {
        स्थिरांक स्वतः pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        जर (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](स्थिरांक AstVar* varp) { परत varp->isParam(); },
                pair.first->second);
        }
        परत pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_गाठ* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* स्थिरांक lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* स्थिरांक rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        जर (!lhsp || !rhsp) परत nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        जर (VL_UNCOVERABLE(!m_modp)) परत nullptr;

        स्थिरांक स्वतः& refVars = modIfaceRefs();
        स्थिरांक स्वतः ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        जर (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) परत nullptr;
        AstVar* स्थिरांक ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* स्थिरांक ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        जर (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) परत nullptr;

        स्थिरांक स्वतः& ifaceVars = ifaceParams(ifacerefp->ifacep());
        स्थिरांक स्वतः targetVarIt = ifaceVars.find(rhsp->name());
        जर (targetVarIt == ifaceVars.end()) परत nullptr;
        AstVar* स्थिरांक targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        जर (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        जर (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            परत nullptr;  // LCOV_EXCL_LINE
        }
        परत targetVarp->valuep()->clone_वृक्ष(असत्य);
    }
    शून्य resolveGenericIfaceDotsIn(Ast_गाठ* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ढकला_back(dotp); });
        साठी (AstDot* स्थिरांक dotp : dotps) {
            जर (Ast_गाठ* स्थिरांक newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    शून्य visit(AstVar* nodep) override {
        जर (nodep->user2SetOnce()) परत;  // Process once
        // Interface array port: split it into its elements
        जर (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(सत्य);
        }
        // Build cache of interface port names as we encounter them
        जर (nodep->isIfaceRef()) { m_ifacePortNames.घाला(nodep->name()); }
        iterateChildren(nodep);
        जर (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            जर (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } अन्यथा जर (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                तार्किक hasUnresolvedLparamXRef = असत्य;
                nodep->valuep()->foreach([&](स्थिरांक AstVarXRef* xrefp) {
                    जर (स्थिरांक AstVar* स्थिरांक varp = xrefp->varp()) {
                        जर (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = सत्य;
                        }
                    }
                });
                जर (hasUnresolvedLparamXRef) परत;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                स्थिरांक तार्किक hasDot = nodep->valuep()->exists([](AstDot*) { परत सत्य; });
                तार्किक refsDeferred = असत्य;
                जर (!hasDot) {
                    स्थिरांक स्वतः& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](स्थिरांक AstVarRef* refp) {
                        जर (refsDeferred) परत;
                        AstVar* स्थिरांक refVarp = refp->varp();
                        जर (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.मोजणी(refVarp)) {
                            refsDeferred = सत्य;
                        }
                    });
                }
                जर (hasDot || refsDeferred) {
                    v3Global.rootp()->ढकला_Deferred_Param_Varp(nodep);
                    परत;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        जर (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    शून्य visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        जर (nodep->varp()) iterate(nodep->varp());
    }
    तार्किक ifaceParamReplace(AstVarXRef* nodep, Ast_गाठ* candp) {
        साठी (; candp; candp = candp->nextp()) {
            जर (nodep->name() == candp->name()) {
                जर (AstVar* स्थिरांक varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    जर (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    परत सत्य;
                } अन्यथा जर (स्थिरांक AstPin* स्थिरांक pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_वृक्ष(असत्य));
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत सत्य;
                }
            }
        }
        परत असत्य;
    }

    शून्य deparamIfaceCellNow(AstCell* cellp) {
        जर (!cellp->paramsp()) परत;
        जर (!VN_IS(cellp->modp(), Iface)) परत;
        Ast_गाठ_Module* स्थिरांक srcModp = cellp->modp();
        Ast_गाठ_Module* स्थिरांक newModp
            = m_processor.गाठ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        जर (newModp && newModp != srcModp) {
            जर (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        जर (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            परत;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        जर (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    शून्य visit(AstVarXRef* nodep) override {
        जर (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            परत;
        }
        // Parameters of an interface array are accessed through its first element
        जर (nodep->varp() && nodep->varp()->isParam()) {
            स्थिरांक स्वतः it = m_ifaceInstCells.find(nodep->dotted());
            जर (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        स्थिरांक string dotted = nodep->dotted();
        जर (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            स्थिरांक Ast_गाठ* backp = nodep;
            पर्यंत ((backp = backp->backp())) {
                जर (VN_IS(backp, गाठ_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    थांबा;
                }
                जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(backp, Var)) {
                    जर (!varp->isIfaceRef()) सुरूठेवा;
                    // Through all dimensions of an array
                    स्थिरांक AstIfaceRefDType* स्थिरांक ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    जर (!ifacerefp) सुरूठेवा;
                    // Interfaces passed in on the port map have ifaces
                    जर (स्थिरांक AstIface* स्थिरांक ifacep = ifacerefp->ifacep()) {
                        जर (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            जर (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                परत;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    अन्यथा जर (AstCell* स्थिरांक cellp = ifacerefp->cellp()) {
                        जर (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            जर (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                परत;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            स्थिरांक स्वतः ifaceCellIt = m_ifaceInstCells.find(dotted);
            जर (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* स्थिरांक cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    शून्य visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        स्थिरांक AstClass* lhsClassp = nullptr;
        स्थिरांक AstClassOrPackageRef* स्थिरांक classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        जर (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_गाठ* rhsDefp = nullptr;
        AstClassOrPackageRef* स्थिरांक rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        जर (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        जर (lhsClassp && rhsDefp) {
            m_state.m_dots.ढकला_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } अन्यथा {
            iterate(nodep->rhsp());
        }
    }

    शून्य visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* स्थिरांक varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_गाठ_FTask_Ref* स्थिरांक taskrefp = VN_CAST(nodep->refp(), गाठ_FTask_Ref);
        AstEnumItemRef* स्थिरांक enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        जर (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } अन्यथा जर (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } अन्यथा जर (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } अन्यथा {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            परत;
        }
        iterate(nodep->cellrefp());

        जर (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } अन्यथा जर (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } अन्यथा {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        साठी (Ast_गाठ *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<पूर्णांक> indices;
        साठी (Ast_गाठ* s = nodep->selp(); s; s = s->nextp()) {
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(s, Const);
            जर (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                परत;
            }
            indices.ढकला_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        स्थिरांक string viftopSuffix = "__Viftop";
        स्थिरांक string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        स्थिरांक string placeholder = "__BRA__??__KET__";
        माप_प्रकार pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        जर (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            परत;
        }
        pos += baseName.length();
        साठी (पूर्णांक idx : indices) {
            स्थिरांक string replacement = "__BRA__" + Ast_गाठ::encode_संख्या(idx) + "__KET__";
            जर (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                परत;
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
    शून्य iterateGenerateCond(Ast_गाठ* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = सत्य;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    शून्य visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->condp(), Const)) {
            जर (Ast_गाठ* स्थिरांक keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } अन्यथा {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            // Normal edit rules will now recurse the replacement
        } अन्यथा {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    शून्य visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        जर (AstGenFor* स्थिरांक forp = VN_AS(nodep->genforp(), GenFor)) {
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
            स्थिरांक string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            जर (Ast_गाठ* स्थिरांक stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } अन्यथा {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    शून्य visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        तार्किक hit = असत्य;
        Ast_गाठ* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        साठी (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            साठी (Ast_गाठ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        स्थिरांक AstConst* स्थिरांक exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        साठी (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            साठी (Ast_गाठ* ep = itemp->condsp(); ep;) {
                Ast_गाठ* स्थिरांक nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        साठी (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            जर (!itemp->isDefault()) {
                साठी (Ast_गाठ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    जर (स्थिरांक AstConst* स्थिरांक ccondp = VN_CAST(ep, Const)) {
                        V3_संख्या match{nodep, 1};
                        match.opEq(ccondp->अंक(), exprp->अंक());
                        जर (!hit && match.isNeqZero()) {
                            hit = सत्य;
                            keepp = itemp->itemsp();
                        }
                    } अन्यथा {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        साठी (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            जर (itemp->isDefault()) {
                जर (!hit) {
                    hit = सत्य;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        जर (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = पूर्वनिर्धारित;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

वर्ग Param_शीर्ष final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    शून्य relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        साठी (AstDot* स्थिरांक dotp : m_state.m_dots) {
            स्थिरांक AstClassOrPackageRef* स्थिरांक classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            स्थिरांक AstClass* स्थिरांक lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* स्थिरांक rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            साठी (स्वतः* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                जर (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    थांबा;
                }
            }
        }
    }

    शून्य fixLevel(Ast_गाठ_Module* modp) {
        // Fix up level of module, based on who instantiates it
        जर (modp->user1SetOnce()) परत;  // Already fixed
        जर (m_state.m_parentps[modp].empty()) परत;  // Leave top levels alone
        पूर्णांक maxParentLevel = 0;
        साठी (Ast_गाठ_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        जर (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    शून्य resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_गाठ_Module*> modps;
        साठी (Ast_गाठ_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), गाठ_Module);
            modp->unlinkFrBack();
            modps.ढकला_back(modp);
        }

        // Fix-up levels
        स्थिरांक VNUser1InUse user1InUse;
        साठी (Ast_गाठ_Module* स्थिरांक modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](स्थिरांक Ast_गाठ_Module* ap, स्थिरांक Ast_गाठ_Module* bp) {
                             परत ap->level() < bp->level();
                         });

        // Re-insert modules
        साठी (Ast_गाठ_Module* स्थिरांक modp : modps) netlistp->addModulesp(modp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit Param_शीर्ष(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        स्थिरांक ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        स्थिरांक ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_गाठ_FTask_Ref* ftaskrefp) {
            Ast_गाठ_FTask* ftaskp = ftaskrefp->taskp();
            जर (!ftaskp || !ftaskp->classMethod()) परत;
            स्थिरांक std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            स्थिरांक स्वतः ancestorClassOf = [](Ast_गाठ* startp) -> AstClass* {
                साठी (Ast_गाठ* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    जर (AstClass* स्थिरांक cp = VN_CAST(np, Class)) परत cp;
                }
                परत nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            जर (refClassp == ftaskrefp->classOrPackagep())
                परत;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            जर (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                परत;
            }
            जर (classp->user3p()) परत;  // will not get removed, no need to relink
            AstClass* स्थिरांक parametrizedClassp = VN_CAST(classp->user4p(), Class);
            जर (!parametrizedClassp) परत;
            Ast_गाठ_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_गाठ_FTask* ftaskp) {
                जर (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    परत सत्य;
                }
                परत असत्य;
            });
            जर (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        स्थिरांक ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        साठी (AstClass* स्थिरांक classp : m_state.m_paramClasses) {
            जर (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                जर (V3Error::error_मोजणी()) सुरूठेवा;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ढकला_Deletep(classp->unlinkFrBack()), classp);
            } अन्यथा {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(असत्य);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_गाठ::user3_Clear_वृक्ष();
        साठी (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_गाठ* स्थिरांक nodep) { nodep->user3(सत्य); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_गाठ* स्थिरांक nodep) {
            nodep->foreachLink([&](Ast_गाठ** स्थिरांक linkpp, स्थिरांक अक्षर* namep) {
                जर (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_शीर्ष() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(Param_शीर्ष);
};

//######################################################################
// Param class functions

शून्य V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    जर (dump_वृक्ष_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_शीर्ष{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    जर (dump_वृक्ष_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_वृक्ष("param", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    साठी (AstVar* स्थिरांक varp : rootp->deferredParamVarps()) {
        जर (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
