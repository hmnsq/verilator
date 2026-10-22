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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Param.h"

#ಅನ್ತರ್ಭಾವ "V3Case.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"
#ಅನ್ತರ್ಭಾವ "V3LinkDotIfaceCapture.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3Simulate.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3Unroll.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <cstdlib>
#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ಸ್ಥಿರ AstClassRefDType* class_Ref_DType_ದ_ಗಂಟು(Ast_ಗಂಟು* nodep) {
    ಆಗ (AstTypedef* ಸ್ಥಿರಾಂಕ tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, ಗಂಟು_DType);
    ಹಿಂತಿರುಗಿ dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : ಶೂನ್ಯಸೂಚಕ;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

ವರ್ಗ Parameterized_Hier_ಖಂಡಗಳು final {
    ಬಳಸಿ Hier_ಖಂಡ_Opts_By_Orig_Name = std::multimap<std::string, ಸ್ಥಿರಾಂಕ V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option*>;
    ಬಳಸಿ HierMapIt = Hier_ಖಂಡ_Opts_By_Orig_Name::const_iterator;
    ಬಳಸಿ Hier_ಖಂಡ_Mod_Map = std::map<ಸ್ಥಿರಾಂಕ std::string, Ast_ಗಂಟು_Module*>;
    ಬಳಸಿ ParamConstMap = std::map<ಸ್ಥಿರಾಂಕ std::string, std::unique_ptr<AstConst>>;
    ಬಳಸಿ GParamsMap = std::map<ಸ್ಥಿರಾಂಕ std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_hier_Sub_ಸತತ;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    Hier_ಖಂಡ_Opts_By_Orig_Name m_hier_ಖಂಡ_Opts_By_Orig_Name;
    // key:mangled module name, value:AstNodeModule*
    Hier_ಖಂಡ_Mod_Map m_hier_ಖಂಡ_Mod;
    // Overridden parameters of the hierarchical block
    std::map<ಸ್ಥಿರಾಂಕ V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ಸ್ಥಿರಾಂಕ std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_untypedParams;

    // METHODS

ಸಾರ್ವಜನಿಕ:
    Parameterized_Hier_ಖಂಡಗಳು(ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ_Opt_ಗಣ& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hier_Sub_ಸತತ{!v3Global.opt.hier_ಖಂಡಗಳು().ಖಾಲಿಗಳು() || v3Global.opt.hier_ಮಗು()} {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& hierOpt : hierOpts) {
            m_hier_ಖಂಡ_Opts_By_Orig_Name.emplace(hierOpt.ಎರಡನೇ.origName(), &hierOpt.ಎರಡನೇ);
            ಸ್ಥಿರಾಂಕ V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option::ParamStrMap& params = hierOpt.ಎರಡನೇ.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.ಎರಡನೇ];
            ಗಾಗಿ (V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.ಕೊನೆ(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::built_ಒಳ_Filename()}, pIt->ಎರಡನೇ)};
                UASSERT(constp, pIt->ಎರಡನೇ << " is not a valid parameter literal");
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ inserted = consts.emplace(pIt->ಮೊದಲ, std::move(constp)).ಎರಡನೇ;
                UASSERT(inserted, pIt->ಮೊದಲ << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ಸೇರಿಸು({hierOpt.ಎರಡನೇ.origName(), {}});
        }
        ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            ಆದರೆ (hierOpts.ಕಂಡುಹಿಡಿ(modp->prettyName()) != hierOpts.ಕೊನೆ()) {
                m_hier_ಖಂಡ_Mod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ಸ್ಥಿರಾಂಕ string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ defParamIt = m_modParams.ಕಂಡುಹಿಡಿ(actualModName);
            ಆದರೆ (defParamIt != m_modParams.ಕೊನೆ()) {
                // modp is the original of parameterized hierarchical block
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                        ಆದರೆ (varp->isGParam()) defParamIt->ಎರಡನೇ.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ dtypep
                            = VN_CAST(varp->ಮಗು_DTypep(), BasicDType);
                        ಆದರೆ (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    ತಾರ್ಕಿಕ hier_Sub_ಸತತ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hier_Sub_ಸತತ; }
    ತಾರ್ಕಿಕ is_Hier_ಖಂಡ(ಸ್ಥಿರಾಂಕ string& origName) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_hier_ಖಂಡ_Opts_By_Orig_Name.ಕಂಡುಹಿಡಿ(origName) != m_hier_ಖಂಡ_Opts_By_Orig_Name.ಕೊನೆ();
    }
    Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_By_Params(ಸ್ಥಿರಾಂಕ string& origName, AstPin* ಮೊದಲ_Pinp,
                                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        UASSERT(is_Hier_ಖಂಡ(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ಸ್ಥಿರಾಂಕ std::pair<HierMapIt, HierMapIt> candidates
            = m_hier_ಖಂಡ_Opts_By_Orig_Name.equal_range(origName);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ paramsIt = m_modParams.ಕಂಡುಹಿಡಿ(origName);
        UASSERT_OBJ(paramsIt != m_modParams.ಕೊನೆ(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ಸ್ಥಿರಾಂಕ AstPin* typeMismatchPinp = ಶೂನ್ಯಸೂಚಕ;
        HierMapIt hierIt;
        ಗಾಗಿ (hierIt = candidates.ಮೊದಲ; hierIt != candidates.ಎರಡನೇ; ++hierIt) {
            ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು = ನಿಜ;
            ಗಾತ್ರ_t paramIdx = 0;
            ಸ್ಥಿರಾಂಕ ParamConstMap& params = m_hierParams[hierIt->ಎರಡನೇ];
            UASSERT(params.ಗಾತ್ರ_2() == hierIt->ಎರಡನೇ->params().ಗಾತ್ರ_2(), "not match");
            ಗಾಗಿ (AstPin* pinp = ಮೊದಲ_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (!pinp->exprp()) ಮುಂದುವರಿ;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ modvarp = pinp->modVarp()) {
                    AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ paramIt = paramsIt->ಎರಡನೇ.ಕಂಡುಹಿಡಿ(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->ಎರಡನೇ.ಕೊನೆ(), modvarp, "must be registered");
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ untyped = m_untypedParams.ಎಣಿಕೆ_2(paramIt->ಎರಡನೇ);
                    AstConst* ಸ್ಥಿರಾಂಕ defValuep = VN_CAST(paramIt->ಎರಡನೇ->valuep(), Const);
                    ಆದರೆ (defValuep && areSame(constp, defValuep)) {
                        ಆದರೆ (!untyped || constp->same_ಮೌಲ್ಯ_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            ಮುಂದುವರಿ;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pIt = vlstd::as_const(params).ಕಂಡುಹಿಡಿ(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    ಆದರೆ (pIt == params.ಕೊನೆ() || paramIdx >= params.ಗಾತ್ರ_2()
                        || !areSame(constp, pIt->ಎರಡನೇ.get())) {
                        ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
                        ನಿಲ್ಲಿಸು;
                    }
                    ಆದರೆ (untyped && !constp->same_ಮೌಲ್ಯ_Type(pIt->ಎರಡನೇ.get())) {
                        typeMismatchPinp = pinp;
                        ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
                        ನಿಲ್ಲಿಸು;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->ಎರಡನೇ.get());
                    ++paramIdx;
                }
            }
            ಆದರೆ (ಸಿಕ್ಕಿತು && paramIdx == hierIt->ಎರಡನೇ->params().ಗಾತ್ರ_2()) ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (hierIt == candidates.ಎರಡನೇ && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ಗಂಟು::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        UASSERT_OBJ(hierIt != candidates.ಎರಡನೇ, ಮೊದಲ_Pinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ಸ್ಥಿರಾಂಕ Hier_ಖಂಡ_Mod_Map::const_iterator modIt
            = m_hier_ಖಂಡ_Mod.ಕಂಡುಹಿಡಿ(hierIt->ಎರಡನೇ->mangledName());
        UASSERT_OBJ(modIt != m_hier_ಖಂಡ_Mod.ಕೊನೆ(), ಮೊದಲ_Pinp,
                    hierIt->ಎರಡನೇ->mangledName() << " is not found");

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = vlstd::as_const(m_hier_ಖಂಡ_Mod).ಕಂಡುಹಿಡಿ(hierIt->ಎರಡನೇ->mangledName());
        ಆದರೆ (it == m_hier_ಖಂಡ_Mod.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        ಆದರೆ (pinValuep->isString()) {
            ಹಿಂತಿರುಗಿ pinValuep->ಅಂಕೆ().ಗಮ್ಯ_String() == hierOptParamp->ಅಂಕೆ().ಗಮ್ಯ_String();
        }

        ಆದರೆ (hierOptParamp->isDouble()) {
            ದ್ವಿಗುಣ var;
            ಆದರೆ (pinValuep->isDouble()) {
                var = pinValuep->ಅಂಕೆ().ಗಮ್ಯ_Double();
            } ಇಲ್ಲದಿದ್ದರೆ {  // Cast from integer to real
                V3_ಸಂಖ್ಯೆ var_ಅಂಕೆ{pinValuep, V3_ಸಂಖ್ಯೆ::Double{}, 0.0};
                var_ಅಂಕೆ.opIToRD(pinValuep->ಅಂಕೆ());
                var = var_ಅಂಕೆ.ಗಮ್ಯ_Double();
            }
            ಹಿಂತಿರುಗಿ V3_ಸಂಖ್ಯೆ::epsilonEqual(var, hierOptParamp->ಅಂಕೆ().ಗಮ್ಯ_Double());
        } ಇಲ್ಲದಿದ್ದರೆ {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_ಸಂಖ್ಯೆ var_ಅಂಕೆ{pinValuep, hierOptParamp->ಅಂಕೆ().ಅಗಲ()};
            ಆದರೆ (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_ಅಂಕೆ.opRToIRoundS(pinValuep->ಅಂಕೆ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pinValuep->isSigned()) {
                var_ಅಂಕೆ.opExtendS(pinValuep->ಅಂಕೆ(), pinValuep->ಅಂಕೆ().ಅಗಲ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                var_ಅಂಕೆ.op_ನಿಯೋಜಿಸು(pinValuep->ಅಂಕೆ());
            }
            V3_ಸಂಖ್ಯೆ isEq{pinValuep, 1};
            isEq.opEq(var_ಅಂಕೆ, hierOptParamp->ಅಂಕೆ());
            ಹಿಂತಿರುಗಿ isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

ವರ್ಗ ParamProcessor final {
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
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    ಬಳಸಿ IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    ಬಳಸಿ CloneMap = std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, Ast_ಗಂಟು*>;
    ರಚನೆ ModInfo final {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        ಸ್ಪಷ್ಟ ModInfo(Ast_ಗಂಟು_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ಸ್ಥಿರಾಂಕ std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ಸ್ಥಿರಾಂಕ std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::ಗಣ<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VString_ಗಣ m_all_Module_ಹೆಸರುಗಳು;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ಸ್ಥಿರಾಂಕ std::string, std::string> m_ಮೌಲ್ಯ_ಹೆಸರುಗಳು;  // Parameter value text to its name
    std::ಗಣ<std::string> m_ಮೌಲ್ಯ_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ಸ್ಥಿರ constexpr std::string::ಗಾತ್ರ_type DIGEST_DIGITS = 8;

    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    Parameterized_Hier_ಖಂಡಗಳು m_hier_ಖಂಡಗಳು;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    ಬಳಸಿ Default_ಮೌಲ್ಯ_Map = std::map<std::string, Ast_ಗಂಟು*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ಗಂಟು_Module*, Default_ಮೌಲ್ಯ_Map> m_default_Parameter_ಮೌಲ್ಯಗಳು;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*, ಸ್ಥಿರಾಂಕ AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::ಸದಿಶ<std::pair<AstParamTypeDType*, ಪೂರ್ಣಾಂಕ>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, ಪೂರ್ಣಾಂಕ> m_param_ಸೂಚ್ಯಂಕ;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstClass*> m_default_Clone_ಒಳ_ಪ್ರಗತಿ;

    std::ಸದಿಶ<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ಸ್ಥಿರ ಶೂನ್ಯ make_Small_ಹೆಸರುಗಳು(Ast_ಗಂಟು_Module* modp) {
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> ಬಳಸಿದ_Letter;
        ಬಳಸಿದ_Letter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->isGParam() || varp->isIfaceRef()) {
                    ಅಕ್ಷರ ch = varp->name()[0];
                    ch = std::ದೊಡ್ಡ_ಅಕ್ಷರ(ch);
                    ಆದರೆ (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(ಬಳಸಿದ_Letter[static_cast<ಪೂರ್ಣಾಂಕ>(ch)] * 256 + ch);
                    ಬಳಸಿದ_Letter[static_cast<ಪೂರ್ಣಾಂಕ>(ch)]++;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(stmtp, ParamTypeDType)) {
                ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ch = 'T';
                typep->user3(ಬಳಸಿದ_Letter[static_cast<ಪೂರ್ಣಾಂಕ>(ch)] * 256 + ch);
                ಬಳಸಿದ_Letter[static_cast<ಪೂರ್ಣಾಂಕ>(ch)]++;
            }
        }
    }
    ಸ್ಥಿರ string paramSmallName(Ast_ಗಂಟು_Module* modp, Ast_ಗಂಟು* varp) {
        ಆದರೆ (varp->user3() <= 1) make_Small_ಹೆಸರುಗಳು(modp);
        ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ = varp->user3() / 256;
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ch = varp->user3() & 255;
        string st = cvt_ಗಮ್ಯ_Str(ch);
        ಆಗ (ಸೂಚ್ಯಂಕ) {
            st += cvt_ಗಮ್ಯ_Str(static_cast<ಅಕ್ಷರ>((ಸೂಚ್ಯಂಕ % 25) + 'A'));
            ಸೂಚ್ಯಂಕ /= 26;
        }
        ಹಿಂತಿರುಗಿ st;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Aggregate_Param_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ಸ್ಥಿರ string param_ಮೌಲ್ಯ_String(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skip_Ref_ಗಮ್ಯ_Non_Refp();
        }
        string ಕೀಲಿ = nodep->name();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            ಆದರೆ (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                ಕೀಲಿ = ifrtp->cellp()->modp()->name();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ifrtp->ifacep()) {
                ಕೀಲಿ = ifrtp->ifacep()->name();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ dtypep
                   = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಕೀಲಿ += " ";
            ಕೀಲಿ += dtypep->verilogKwd();
            ಕೀಲಿ += " {";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                ಕೀಲಿ += param_ಮೌಲ್ಯ_String(memberp);
                ಕೀಲಿ += ";";
            }
            ಕೀಲಿ += "}";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, MemberDType)) {
            ಕೀಲಿ += " ";
            ಕೀಲಿ += param_ಮೌಲ್ಯ_String(dtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, BasicDType)) {
            ಆದರೆ (dtypep->isSigned()) ಕೀಲಿ += " signed";
            ಆದರೆ (dtypep->isRanged()) {
                ಕೀಲಿ += "[" + cvt_ಗಮ್ಯ_Str(dtypep->ಎಡ()) + ":" + cvt_ಗಮ್ಯ_Str(dtypep->ಬಲ()) + "]";
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, PackArrayDType)) {
            ಕೀಲಿ += "[";
            ಕೀಲಿ += cvt_ಗಮ್ಯ_Str(dtypep->ಎಡ());
            ಕೀಲಿ += ":";
            ಕೀಲಿ += cvt_ಗಮ್ಯ_Str(dtypep->ಬಲ());
            ಕೀಲಿ += "] ";
            ಕೀಲಿ += param_ಮೌಲ್ಯ_String(dtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ patp = VN_CAST(nodep, ನಮೂನೆ)) {
            // A parameter override pattern, keyed before the specialized module types it
            ಕೀಲಿ += "'{";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                ಆದರೆ (memp->isDefault()) ಕೀಲಿ += "default";
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಪಠ್ಯ* ಸ್ಥಿರಾಂಕ textp = VN_CAST(memp->keyp(), ಪಠ್ಯ_2)) {
                    ಕೀಲಿ += textp->ಪಠ್ಯ();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (memp->keyp()) {
                    ಕೀಲಿ += param_ಮೌಲ್ಯ_String(memp->keyp());
                }
                ಕೀಲಿ += ":";
                ಆದರೆ (memp->repp()) ಕೀಲಿ += param_ಮೌಲ್ಯ_String(memp->repp()) + "x";
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                    ಕೀಲಿ += param_ಮೌಲ್ಯ_String(valuep) + ";";
                }
                ಕೀಲಿ += ",";
            }
            ಕೀಲಿ += "}";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            ಕೀಲಿ += "{";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Array_ಅಂಶ* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Array_ಅಂಶ)) {
                ಕೀಲಿ += cvt_ಗಮ್ಯ_Str(itemp->ಸೂಚ್ಯಂಕ()) + ":" + param_ಮೌಲ್ಯ_String(itemp->valuep()) + ",";
            }
            ಆದರೆ (consArrayp->defaultp()) {
                ಕೀಲಿ += "default:" + param_ಮೌಲ್ಯ_String(consArrayp->defaultp()) + ",";
            }
            ಕೀಲಿ += "}";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsStruct* ಸ್ಥಿರಾಂಕ consp = VN_CAST(nodep, ConsStruct)) {
            ಕೀಲಿ += "{";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Struct_ಅಂಶ* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_ಅಂಶ)) {
                ಕೀಲಿ += param_ಮೌಲ್ಯ_String(itemp->valuep());
                ಕೀಲಿ += ",";
            }
            ಕೀಲಿ += "}";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            ಆದರೆ (classRefp->classp()) {
                ಸ್ಥಿರಾಂಕ string& className = classRefp->classp()->name();
                ಸ್ಥಿರಾಂಕ string& origName = classRefp->classp()->origName();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                ಆದರೆ (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    ಕೀಲಿ = className;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Unspecialized class with no params - use origName
                    ಕೀಲಿ = origName;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$" + cvt_ಗಮ್ಯ_Str(dtypep->declRange());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, ಸರತಿ_DType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$[$";
            ಆದರೆ (dtypep->boundConst()) ಕೀಲಿ += ":" + cvt_ಗಮ್ಯ_Str(dtypep->boundConst());
            ಕೀಲಿ += "]";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, DynArrayDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$[]";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, AssocArrayDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->ಕೀಲಿ_DTypep()) + "]";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$[*]";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            ಕೀಲಿ += " enum {";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(itemp->valuep(), Const);
                ಕೀಲಿ += itemp->name() + "=" + (constp ? constp->ಅಂಕೆ().ascii() : "?") + ";";
            }
            ಕೀಲಿ += "}";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, ಗಂಟು_DType)) {
            ಕೀಲಿ += dtypep->prettyDTypeName(ನಿಜ);
        }
        UASSERT_OBJ(!ಕೀಲಿ.ಖಾಲಿಗಳು(), nodep, "Parameter yielded no value string");
        ಹಿಂತಿರುಗಿ ಕೀಲಿ;
    }
    // As paramValueString(), for the element or key type of a container
    ಸ್ಥಿರ string paramElemString(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp
            = VN_CAST(dtypep->skip_Ref_ಗಮ್ಯ_Non_Refp(), ClassRefDType);
        ಆದರೆ (classRefp && classRefp->paramsp()) ಹಿಂತಿರುಗಿ classRefp->prettyDTypeName(ನಿಜ);
        ಹಿಂತಿರುಗಿ param_ಮೌಲ್ಯ_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ಸ್ಥಿರ string digestSuffix(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, std::ಗಣ<string>& usedr) {
        ಸ್ಥಿರಾಂಕ string hex = VHashSha512{ಪಠ್ಯ}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::ಗಾತ್ರ_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        ಆಗ (digits < hex.ಗಾತ್ರ_2() && !usedr.ಸೇರಿಸು(hex.substr(0, digits)).ಎರಡನೇ) ++digits;
        ಹಿಂತಿರುಗಿ hex.substr(0, digits);
    }
    string param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        ಆದರೆ (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ಸ್ಥಿರಾಂಕ string ಪಠ್ಯ = param_ಮೌಲ್ಯ_String(nodep);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_ಮೌಲ್ಯ_ಹೆಸರುಗಳು.emplace(ಪಠ್ಯ, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ = "z" + digestSuffix(ಪಠ್ಯ, m_ಮೌಲ್ಯ_Suffixes);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }
    string moduleCalcName(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* srcModp, ಸ್ಥಿರಾಂಕ string& longname) {
        string newname = longname;
        ಆದರೆ (longname.ಉದ್ದಳತೆ() > 30) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_longMap.emplace(longname, "");
            ಆದರೆ (pair.ಎರಡನೇ) {
                // We use all upper case above, so lower here can't conflict
                pair.ಮೊದಲ->ಎರಡನೇ
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.ಮೊದಲ->ಎರಡನೇ;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        ಹಿಂತಿರುಗಿ newname;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* arraySubDTypep(Ast_ಗಂಟು_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            ಹಿಂತಿರುಗಿ adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBracketArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(nodep, BracketArrayDType)) {
            ಹಿಂತಿರುಗಿ adtypep->subDTypep();
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* arraySubDTypeDeepp(Ast_ಗಂಟು_DType* nodep) {
        ಆಗ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subp = arraySubDTypep(nodep)) nodep = subp;
        ಹಿಂತಿರುಗಿ nodep;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isString(Ast_ಗಂಟು_DType* nodep) {
        ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(nodep->skip_Ref_ಗಮ್ಯ_Non_Refp(), BasicDType))
            ಹಿಂತಿರುಗಿ basicp->isString();
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ collectPins(CloneMap* clonemapp, Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* originalParamp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ಆದರೆ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ಆದರೆ (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            ಆದರೆ (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    ಶೂನ್ಯ relinkPins(ಸ್ಥಿರಾಂಕ CloneMap* clonemapp, AstPin* startpinp) {
        ಗಾಗಿ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ cloneiter = clonemapp->ಕಂಡುಹಿಡಿ(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->ಕೊನೆ(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->ಎರಡನೇ, Var));
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pinp->modPTypep()) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ cloneiter = clonemapp->ಕಂಡುಹಿಡಿ(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->ಕೊನೆ(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->ಎರಡನೇ, ParamTypeDType));
            } ಇಲ್ಲದಿದ್ದರೆ {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    ಶೂನ್ಯ relinkPinsByName(AstPin* startpinp, Ast_ಗಂಟು_Module* modp) {
        std::map<ಸ್ಥಿರಾಂಕ string, AstVar*> name_ಗಮ್ಯ_Pin;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    name_ಗಮ್ಯ_Pin.emplace(varp->name(), varp);
                }
            }
        }
        ಗಾಗಿ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = pinp->modVarp()) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ varIt = vlstd::as_const(name_ಗಮ್ಯ_Pin).ಕಂಡುಹಿಡಿ(varp->name());
                UASSERT_OBJ(varIt != name_ಗಮ್ಯ_Pin.ಕೊನೆ(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->ಎರಡನೇ);
            }
        }
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasDescendantDefparams(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp,
                                       std::ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*>& ಭೇಟಿಯಾದ) {
        ಆದರೆ (!ಭೇಟಿಯಾದ.ಸೇರಿಸು(modp).ಎರಡನೇ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(stmtp, Cell);
            ಆದರೆ (!cellp) ಮುಂದುವರಿ;
            ಗಾಗಿ (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (!pinp->param_ಮಾರ್ಗ().ಖಾಲಿಗಳು()) {
                    ಭೇಟಿಯಾದ.erase(modp);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
            ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ಮಗು_Modp = cellp->modp()) {
                ಆದರೆ (hasDescendantDefparams(ಮಗು_Modp, ಭೇಟಿಯಾದ)) {
                    ಭೇಟಿಯಾದ.erase(modp);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }
        ಭೇಟಿಯಾದ.erase(modp);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasDescendantDefparams(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        std::ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*> ಭೇಟಿಯಾದ;
        ಹಿಂತಿರುಗಿ hasDescendantDefparams(modp, ಭೇಟಿಯಾದ);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ಸ್ಥಿರಾಂಕ AstPin* nestedDefparamp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_nestedDefparamps.emplace(modp, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (!pair.ಎರಡನೇ) ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;  // Searched, or being searched
        ಸ್ಥಿರಾಂಕ AstPin* foundp = ಶೂನ್ಯಸೂಚಕ;
        modp->exists([&](ಸ್ಥಿರಾಂಕ AstCell* cellp) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (!pinp->param_ಮಾರ್ಗ().ಖಾಲಿಗಳು()) foundp = pinp;
            }
            ಆದರೆ (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            ಹಿಂತಿರುಗಿ foundp != ಶೂನ್ಯಸೂಚಕ;
        });
        pair.ಮೊದಲ->ಎರಡನೇ = foundp;
        ಹಿಂತಿರುಗಿ foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    ಶೂನ್ಯ checkSupportedParam(Ast_ಗಂಟು_Module* modp, AstPin* pinp) ಸ್ಥಿರಾಂಕ {
        // ConsArray is not supported because that can not be set via -G
        // option.
        ಆದರೆ (pinp->modVarp()) {
            ತಾರ್ಕಿಕ supported = ಸುಳ್ಳು;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(pinp->exprp(), Const)) {
                supported = ನಿಜ;
                ಆದರೆ (constp->ಅಂಕೆ().isString()
                    && !V3_Hier_ಖಂಡ::stringParamPassable(constp->ಅಂಕೆ().ಗಮ್ಯ_String())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            ಆದರೆ (!supported) {
                pinp->v3error(
                    Ast_ಗಂಟು::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstParamTypeDType* ಸ್ಥಿರಾಂಕ typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ಗಂಟು::prettyNameQ(modp->origName()));
        }
    }
    ತಾರ್ಕಿಕ moduleExists(ಸ್ಥಿರಾಂಕ string& modName) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_all_Module_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(modName) != m_all_Module_ಹೆಸರುಗಳು.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (m_modNameMap.ಕಂಡುಹಿಡಿ(modName) != m_modNameMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    string parameterized_Hier_ಖಂಡ_Name(Ast_ಗಂಟು_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hier_ಖಂಡ(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ಗಂಟು*> pins;

        AstPin* pinp = paramPinsp;
        ಆಗ (pinp) {
            checkSupportedParam(modp, pinp);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = pinp->modVarp()) {
                ಆದರೆ (!pinp->exprp()) ಮುಂದುವರಿ;
                ಆದರೆ (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ಗಂಟು_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_default_Parameter_ಮೌಲ್ಯಗಳು.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        ಆದರೆ (pair.ಎರಡನೇ) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPin* ಸ್ಥಿರಾಂಕ defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ಗಂಟು::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_ಮೌಲ್ಯ_Map params;
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                    ಆದರೆ (varp->isGParam()) {
                        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ dtypep = static_cast<Ast_ಗಂಟು*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.ಮೊದಲ->ಎರಡನೇ = std::move(params);
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ paramsIt = pair.ಮೊದಲ;
        ಆದರೆ (paramsIt->ಎರಡನೇ.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ modp->origName();  // modp has no parameter

        string longname = modp->origName();
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ&& default_ಮೌಲ್ಯ : paramsIt->ಎರಡನೇ) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pinIt = pins.ಕಂಡುಹಿಡಿ(default_ಮೌಲ್ಯ.ಮೊದಲ);
            // If the pin does not have a value assigned, use the default one.
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = pinIt == pins.ಕೊನೆ() ? default_ಮೌಲ್ಯ.ಎರಡನೇ : pinIt->ಎರಡನೇ;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_ಮೌಲ್ಯ.ಮೊದಲ + "=";
            // constp can be nullptr

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->ಅಂಕೆ().ascii(ಸುಳ್ಳು);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_ವೃಕ್ಷ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_longMap.ಕಂಡುಹಿಡಿ(longname);
        ಆದರೆ (iter != m_longMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ಸೇರಿಸು(longname);
        ಆಗ (ನಿಜ) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ಸ್ಥಿರಾಂಕ string hashStr = hashStrGen.digestHex();
            ಗಾಗಿ (string::ಗಾತ್ರ_type i = 1; i < hashStr.ಗಾತ್ರ_2(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                ಆದರೆ (newName.at(newName.ಗಾತ್ರ_2() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                ಆದರೆ (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    ಹಿಂತಿರುಗಿ newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ಸೇರಿಸು(V3Os::trueRandom(64));
        }
    }
    ಶೂನ್ಯ replaceRefsRecurse(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ oldClassp,
                            AstClass* ಸ್ಥಿರಾಂಕ newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(nodep, ClassRefDType)) {
            ಆದರೆ (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ಆದರೆ (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            ಆದರೆ (typedefp->subDTypep()) {
                ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    ಆದರೆ (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ ftaskRefp = VN_CAST(nodep, ಗಂಟು_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            ಆದರೆ (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ oldTaskp = ftaskRefp->taskp()) {
                ಆದರೆ (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        ಆದರೆ (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        ಆದರೆ (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        ಆದರೆ (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        ಆದರೆ (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        ಆದರೆ (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    ವರ್ಗ VarXRefRelinkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
        Ast_ಗಂಟು_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ಗಂಟು_Module*> m_varModuleMap;  // Cache var->module lookups

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ VarXRefRelinkVisitor(Ast_ಗಂಟು_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    ಖಾಸಗಿ:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Var_Module(AstVar* varp) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_varModuleMap.ಕಂಡುಹಿಡಿ(varp);
            ಆದರೆ (it != m_varModuleMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
            Ast_ಗಂಟು_Module* varModp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು* np = varp; np; np = np->backp()) {
                ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(np, ಗಂಟು_Module)) {
                    varModp = modp;
                    ನಿಲ್ಲಿಸು;
                }
            }
            m_varModuleMap[varp] = varModp;
            ಹಿಂತಿರುಗಿ varModp;
        }

        ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
            AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
            ಆದರೆ (!varp) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ಸ್ಥಿರಾಂಕ string& dotted = nodep->dotted();
            ಆದರೆ (dotted.ಖಾಲಿಗಳು()) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t dotPos = dotted.ಕಂಡುಹಿಡಿ('.');
            ಸ್ಥಿರಾಂಕ string ಪೋರ್ಟ್_Name = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            ಆದರೆ (ಪೋರ್ಟ್_Name.ಖಾಲಿಗಳು()) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            // Find the interface port variable in the cloned module
            AstVar* ಪೋರ್ಟ್_Varp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varChkp = VN_CAST(stmtp, Var)) {
                    ಆದರೆ (varChkp->name() == ಪೋರ್ಟ್_Name && varChkp->isIfaceRef()) {
                        ಪೋರ್ಟ್_Varp = varChkp;
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
            ಆದರೆ (!ಪೋರ್ಟ್_Varp) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp = VN_CAST(ಪೋರ್ಟ್_Varp->subDTypep(), IfaceRefDType);
            ಆದರೆ (!irefp) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newIfacep = irefp->iface_ಮೂಲಕ_Cellp();
            ಆದರೆ (!newIfacep) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varModp = ಕಂಡುಹಿಡಿ_Var_Module(varp);

            // If variable is in a different module than the port's interface, remap it
            ಆದರೆ (varModp && varModp != newIfacep) {
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ newVarp = VN_CAST(stmtp, Var)) {
                        ಆದರೆ (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    ಶೂನ್ಯ fixupCrossModuleRefDTypes(Ast_ಗಂಟು_Module* newModp) {
        ಆದರೆ (!V3LinkDotIfaceCapture::enabled()) ಹಿಂತಿರುಗಿ;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        ಪೂರ್ಣಾಂಕ fixed_ಎಣಿಕೆ = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* ಸ್ಥಿರಾಂಕ tagp = V3LinkDotIfaceCapture::capture_ಟ್ಯಾಗ್(refp);
            ಆದರೆ (!tagp) ಹಿಂತಿರುಗಿ;

            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ correctModp
                = V3LinkDotIfaceCapture::follow_Cell_ಮಾರ್ಗ(newModp, tagp->m_cell_ಮಾರ್ಗ);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cell_ಮಾರ್ಗ << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            ಆದರೆ (!correctModp || correctModp->dead()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (correctModp->parameterizedTemplate()) ಹಿಂತಿರುಗಿ;

            ತಾರ್ಕಿಕ fixed = ಸುಳ್ಳು;
            ಆದರೆ (refp->typedefp()) {
                ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ newTdp = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Typedef_ಒಳ_Module(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    ಆದರೆ (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = ನಿಜ;
                }
            }
            ಆದರೆ (!fixed && refp->refDTypep()) {
                ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ newDtp = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_DType_ಒಳ_Module(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = ನಿಜ;
                }
            }
            ಆದರೆ (fixed) ++fixed_ಎಣಿಕೆ;
        });
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("IfaceCapture, Ledger fixups in V3Param", fixed_ಎಣಿಕೆ);
    }

    // Return true on success, false on error
    ತಾರ್ಕಿಕ deepCloneModule(Ast_ಗಂಟು_Module* srcModp, Ast_ಗಂಟು* ifErrorp, AstPin* paramsp,
                         ಸ್ಥಿರಾಂಕ string& newname, ಸ್ಥಿರಾಂಕ IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ಗಂಟು_Module* newModp;
        ಆದರೆ (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಗಂಟು_Module);
        } ಇಲ್ಲದಿದ್ದರೆ {
            newModp = srcModp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(ನಿಜ);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(ಸುಳ್ಳು);

        newModp->name(newname);
        newModp->user2(ಸುಳ್ಳು);  // We need to re-recurse this module once changed
        newModp->recursive(ಸುಳ್ಳು);
        newModp->recursiveClone(ಸುಳ್ಳು);
        newModp->hasGParam(ಸುಳ್ಳು);
        // Recursion may need level cleanups
        ಆದರೆ (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        ಆದರೆ ((newModp->level() - srcModp->level()) >= (v3Global.opt.module_Recursion_ಆಳ() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.module_Recursion_ಆಳ());
            VL_DO_DANGLING(newModp->delete_ವೃಕ್ಷ(), newModp);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ಗಂಟು* insertp = srcModp;
        ಆಗ (VN_IS(insertp->nextp(), ಗಂಟು_Module)
               && VN_AS(insertp->nextp(), ಗಂಟು_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_modNameMap.ಕಂಡುಹಿಡಿ(newname);
        CloneMap* ಸ್ಥಿರಾಂಕ clonemapp = &(iter->ಎರಡನೇ.m_cloneMap);
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
        ತಾರ್ಕಿಕ needRelinkByName = ಸುಳ್ಳು;
        ಆದರೆ (paramsp) {
            ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (pinp->modVarp() && clonemapp->ಕಂಡುಹಿಡಿ(pinp->modVarp()) == clonemapp->ಕೊನೆ()) {
                    needRelinkByName = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಆದರೆ (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Irefp = it->ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ pinIrefp = it->ಎರಡನೇ;
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ cloneIrefp = ಪೋರ್ಟ್_Irefp->clonep();
            UINFO(8, "     IfaceOld " << ಪೋರ್ಟ್_Irefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, ಪೋರ್ಟ್_Irefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->iface_ಮೂಲಕ_Cellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        ಆದರೆ (!ifaceRefRefs.ಖಾಲಿಗಳು()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (pinp->exprp()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ modvarp = pinp->modVarp()) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(newp, Const);
                    AstConst* ಸ್ಥಿರಾಂಕ origp = VN_CAST(modvarp->valuep(), Const);
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ overridden
                        = !(origp && exprp && Parameterized_Hier_ಖಂಡಗಳು::areSame(exprp, origp));
                    // Remove any existing parameter
                    ಆದರೆ (modvarp->valuep()) modvarp->valuep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    modvarp->overriddenParam(overridden);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ modptp = pinp->modPTypep()) {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = VN_AS(pinp->exprp(), ಗಂಟು_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    ಆದರೆ (modptp->ಮಗು_DTypep()) modptp->ಮಗು_DTypep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                    // Set this parameter to value requested by cell
                    modptp->ಮಗು_DTypep(dtypep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಸ್ಥಿರಾಂಕ ModInfo* module_ಕಂಡುಹಿಡಿ_Or_Clone(Ast_ಗಂಟು_Module* srcModp, Ast_ಗಂಟು* ifErrorp, AstPin* paramsp,
                                     ಸ್ಥಿರಾಂಕ string& newname, ಸ್ಥಿರಾಂಕ IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        ಸ್ವಯಂಚಾಲಿತ it = m_modNameMap.ಕಂಡುಹಿಡಿ(newname);
        ಆದರೆ (it != m_modNameMap.ಕೊನೆ()) {
            UINFO(4, "     De-parameterize to prev: " << it->ಎರಡನೇ.m_modp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            it = m_modNameMap.ಕಂಡುಹಿಡಿ(newname);
            UASSERT(it != m_modNameMap.ಕೊನೆ(), "should find just-made module");
        }
        ಸ್ಥಿರಾಂಕ ModInfo* ಸ್ಥಿರಾಂಕ modInfop = &(it->ಎರಡನೇ);
        ಹಿಂತಿರುಗಿ modInfop;
    }

    ಶೂನ್ಯ convert_ಗಮ್ಯ_Stringp(Ast_ಗಂಟು* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        ಆದರೆ (constp && !constp->ಅಂಕೆ().isString()) {
            constp->replaceWith(
                ಹೊಸ AstConst{constp->fileline(), AstConst::String{}, constp->ಅಂಕೆ().ಗಮ್ಯ_String()});
            VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    ಶೂನ್ಯ constify_Member_ಮೌಲ್ಯ(AstVar* varp, std::ಗಣ<AstVar*>& ಒಳ_ಪ್ರಗತಿ) {
        ಆದರೆ (!varp->valuep() || VN_IS(varp->valuep(), Const)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!ಒಳ_ಪ್ರಗತಿ.ಸೇರಿಸು(varp).ಎರಡನೇ) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            varp->valuep(ಹೊಸ AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            ಹಿಂತಿರುಗಿ;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::ಗಣ<AstVar*> siblings;
        varp->valuep()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            AstVar* ಸ್ಥಿರಾಂಕ refVarp = refp->varp();
            ಆದರೆ (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ಸೇರಿಸು(refVarp);
            }
        });
        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ sibp : siblings) constify_Member_ಮೌಲ್ಯ(sibp, ಒಳ_ಪ್ರಗತಿ);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        ಒಳ_ಪ್ರಗತಿ.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ಸ್ಥಿರ AstParseRef* memberParseRef(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (AstParseRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ParseRef)) ಹಿಂತಿರುಗಿ refp;
        Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ಗಂಟು_Pre_Sel);
        ಹಿಂತಿರುಗಿ selp ? VN_CAST(selp->fromp(), ParseRef) : ಶೂನ್ಯಸೂಚಕ;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ಸ್ಥಿರ ಶೂನ್ಯ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಆದರೆ (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ selp = VN_AS(dotp->rhsp(), ಗಂಟು_Pre_Sel);
            ಆದರೆ (Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ attrp = selp->attrp()) {
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    VL_DO_DANGLING(oldFromp->delete_ವೃಕ್ಷ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_ವೃಕ್ಷ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Deferred_Member_ತಳ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (VN_IS(nodep, MemberSel)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ಗಂಟು_Pre_Sel)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDTypep = selp->fromp()->dtypep();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ಗಂಟು_Array_DType) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (arrayDTypep) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypep = arrayDTypep->subDTypep();
                ಹಿಂತಿರುಗಿ elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ಗಂಟು_UOr_Struct_DType);
            }
            ಹಿಂತಿರುಗಿ is_Deferred_Member_ತಳ(selp->fromp());
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->dtypep();
        ಹಿಂತಿರುಗಿ dtypep && VN_IS(dtypep->skipRefOrNullp(), ಗಂಟು_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ಸ್ಥಿರ ಶೂನ್ಯ resolveMemberDot(AstDot* dotp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(dotp->lhsp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        AstParseRef* ಸ್ಥಿರಾಂಕ memberRefp = memberParseRef(dotp->rhsp());
        ಆದರೆ (!lhsp || !memberRefp || !is_Deferred_Member_ತಳ(lhsp)) ಹಿಂತಿರುಗಿ;
        AstMemberSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstMemberSel{memberRefp->fileline(), lhsp->unlink_Fr_ಹಿಂಭಾಗ(),
                                                    VFlag_ಮಗು_DType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    ಶೂನ್ಯ resolve_Dot_ಗಮ್ಯ_Typedef(Ast_ಗಂಟು* exprp) {
        AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(exprp, Dot);
        ಆದರೆ (!dotp) ಹಿಂತಿರುಗಿ;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        ಆದರೆ (!classRefp) {
            resolveMemberDot(dotp);
            ಹಿಂತಿರುಗಿ;
        }
        AstParseRef* ಸ್ಥಿರಾಂಕ parseRefp = memberParseRef(dotp->rhsp());
        ಆದರೆ (!parseRefp) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        ಆದರೆ (paramTypep) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = paramTypep->subDTypep();
            AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (classRefDTypep) {
                AstClass* ಸ್ಥಿರಾಂಕ srcClassp = classRefDTypep->classp();
                ಆದರೆ (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    ಆದರೆ (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        ಆದರೆ (classRefp->paramsp()) {
            AstClass* ಸ್ಥಿರಾಂಕ srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            ಆದರೆ (srcClassp && srcClassp->hasGParam()) {
                ಆದರೆ (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ crp = class_Ref_DType_ದ_ಗಂಟು(classRefp->classOrPackageNodep())) {
            ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ srcClassp = crp->classp()) {
                ಆದರೆ (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        ಆದರೆ (!lhsClassp) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ memberp = m_memberMap.ಕಂಡುಹಿಡಿ_Member(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ tdefp = VN_CAST(memberp, Typedef)) {
            ಆದರೆ (parseRefp != dotp->rhsp()) ಹಿಂತಿರುಗಿ;
            AstRefDType* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    ಶೂನ್ಯ substituteParamMember(AstDot* ಸ್ಥಿರಾಂಕ dotp, AstParseRef* ಸ್ಥಿರಾಂಕ memberRefp,
                               AstVar* ಸ್ಥಿರಾಂಕ varp) {
        ಆದರೆ (!varp->isParam() || !varp->valuep()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!VN_IS(varp->valuep(), Const)) {
            std::ಗಣ<AstVar*> ಒಳ_ಪ್ರಗತಿ;
            constify_Member_ಮೌಲ್ಯ(varp, ಒಳ_ಪ್ರಗತಿ);
        }
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(varp->valuep(), Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ;
        AstConst* ಸ್ಥಿರಾಂಕ newp = constp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    ಶೂನ್ಯ resolveParamClassRefDType(AstRefDType* refp) {
        ಆದರೆ (refp->typedefp() || refp->refDTypep()) ಹಿಂತಿರುಗಿ;

        AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        ಆದರೆ (!classRefp) ಹಿಂತಿರುಗಿ;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        ಆದರೆ (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            ಹಿಂತಿರುಗಿ;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ಗಂಟು* targetp = classRefp->classOrPackageNodep();
        ಆಗ (ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtargetp = VN_CAST(targetp, ಗಂಟು_DType))
            targetp = dtargetp->skipRefOrNullp();
        ಆದರೆ (!targetp) ಹಿಂತಿರುಗಿ;
        AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefDTypep = VN_CAST(targetp, ClassRefDType);
        ಆದರೆ (!classRefDTypep) ಹಿಂತಿರುಗಿ;
        srcClassp = classRefDTypep->classp();
        ಆದರೆ (!srcClassp) ಹಿಂತಿರುಗಿ;

        AstClass* newClassp = srcClassp;
        ಆದರೆ (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ಸ್ಥಿರಾಂಕ typedefp
            = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(newClassp, refp->name()), Typedef);
        ಆದರೆ (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ defaultParamsHaveTypeQueryOnParamType(ಸ್ಥಿರಾಂಕ AstClassRefDType* nodep) {
        ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
        nodep->foreach([&](ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* attrp) {
            ಆದರೆ (ಸಿಕ್ಕಿತು || !attrp->attrType().isTypeQuery()) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(attrp->fromp(), RefDType);
            ಆದರೆ (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) ಸಿಕ್ಕಿತು = ನಿಜ;
        });
        ಹಿಂತಿರುಗಿ ಸಿಕ್ಕಿತು;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    ತಾರ್ಕಿಕ classTypeMatchesDefaultClone(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* exprp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ origClassRefp = VN_CAST(origp, ClassRefDType);
        ಆದರೆ (!ಅಭಿವ್ಯಕ್ತಿ_Class_Refp || !origClassRefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        ಆದರೆ (defaultClonep && defaultClonep == ಅಭಿವ್ಯಕ್ತಿ_Class_Refp->classp()) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        ಆದರೆ (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        ಆದರೆ (!origClassRefp->classp()->hasGParam()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (origClassRefp->classp()->origName() != ಅಭಿವ್ಯಕ್ತಿ_Class_Refp->classp()->origName())
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        ಆದರೆ (!m_default_Clone_ಒಳ_ಪ್ರಗತಿ.ಸೇರಿಸು(origClassRefp->classp()).ಎರಡನೇ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ಸ್ಥಿರಾಂಕ origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ = resolvedModp && VN_CAST(resolvedModp, Class) == ಅಭಿವ್ಯಕ್ತಿ_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_ವೃಕ್ಷ(), origClonep);
        m_default_Clone_ಒಳ_ಪ್ರಗತಿ.erase(origClassRefp->classp());
        ಹಿಂತಿರುಗಿ ಹೊಂದಿಕೆ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ param_Consts_Equal_At_Max_ಅಗಲ(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        ಆದರೆ (exprp->ಅಂಕೆ().ಅಗಲ() == origp->ಅಂಕೆ().ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (exprp->ಅಂಕೆ().isOpaque()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (origp->ಅಂಕೆ().isOpaque()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ max_ಅಗಲ = std::max(exprp->ಅಂಕೆ().ಅಗಲ(), origp->ಅಂಕೆ().ಅಗಲ());
        V3_ಸಂಖ್ಯೆ ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ{exprp, max_ಅಗಲ};
        ಆದರೆ (exprp->ಅಂಕೆ().isSigned()) {
            ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ.opExtendS(exprp->ಅಂಕೆ(), exprp->ಅಂಕೆ().ಅಗಲ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ.op_ನಿಯೋಜಿಸು(exprp->ಅಂಕೆ());
        }
        V3_ಸಂಖ್ಯೆ orig_ಅಂಕೆ{origp, max_ಅಗಲ};
        ಆದರೆ (origp->ಅಂಕೆ().isSigned()) {
            orig_ಅಂಕೆ.opExtendS(origp->ಅಂಕೆ(), origp->ಅಂಕೆ().ಅಗಲ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            orig_ಅಂಕೆ.op_ನಿಯೋಜಿಸು(origp->ಅಂಕೆ());
        }
        V3_ಸಂಖ್ಯೆ isEq{exprp, 1};
        isEq.opEq(ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ, orig_ಅಂಕೆ);
        ಹಿಂತಿರುಗಿ isEq.isNeqZero();
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
    ಶೂನ್ಯ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (!pinp->exprp()) ಮುಂದುವರಿ;
            ಆದರೆ (!VString::startsWith(pinp->name(), "__VGIfaceParam")) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ fwdRefp = VN_CAST(pinp->exprp(), ಗಂಟು_Var_Ref);
            ಆದರೆ (!fwdRefp) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ resolvedp
                = VN_CAST(fwdRefp->varp()->ಮಗು_DTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ newIrefp = ಹೊಸ AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            ಆದರೆ (resolvedp->paramsp()) {
                newIrefp->ಕೂಡಿಸು_Paramsp(resolvedp->paramsp()->clone_ವೃಕ್ಷ(ನಿಜ));
            }
            pinp->exprp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    ಶೂನ್ಯ name_By_Pin_ಮೌಲ್ಯ(AstPin* pinp, Ast_ಗಂಟು_Module* srcModp, AstVar* modvarp, string& longnamer,
                        ತಾರ್ಕಿಕ& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(pinp->exprp());
        any_overridesr = ನಿಜ;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    ಶೂನ್ಯ cellPinCleanup(Ast_ಗಂಟು* nodep, AstPin* pinp, AstPin* paramsp, Ast_ಗಂಟು_Module* srcModp,
                        string& longnamer, ತಾರ್ಕಿಕ& any_overridesr,
                        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstPin*>& overridingTypePinsr) {
        ಆದರೆ (!pinp->exprp()) ಹಿಂತಿರುಗಿ;  // No-connect
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            ಆದರೆ (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(pinp->exprp(), ನಮೂನೆ)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_ಮೌಲ್ಯ(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                ಆದರೆ (is_Aggregate_Param_ಮೌಲ್ಯ(pinp->exprp())) {
                    name_By_Pin_ಮೌಲ್ಯ(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    ಹಿಂತಿರುಗಿ;
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
                ಆದರೆ (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_ಗಾತ್ರ))) {
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ dependent = modvarp->valuep()->exists(
                        [](Ast_ಗಂಟು* np) { ಹಿಂತಿರುಗಿ VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    ಆದರೆ (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                ಆದರೆ (isString(modvarp->subDTypep())) {
                    convert_ಗಮ್ಯ_Stringp(pinp->exprp());
                    convert_ಗಮ್ಯ_Stringp(modvarp->valuep());
                }
                AstConst* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ಸ್ಥಿರಾಂಕ origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (exprp && !exprp->ಅಂಕೆ().isDouble() && !exprp->ಅಂಕೆ().isString()) {
                    AstVar* cloneVarp = modvarp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    ತಾರ್ಕಿಕ cloneVarpUnresolved = ಸುಳ್ಳು;
                    ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlink_Fr_ಹಿಂಭಾಗ();
                        VL_DO_DANGLING(oldValuep->delete_ವೃಕ್ಷ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        ಆದರೆ (cloneVarp->ಮಗು_DTypep())
                            cloneVarp->ಮಗು_DTypep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                        cloneVarp->ಮಗು_DTypep(origDTypep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                        cloneVarp->dtypep(ಶೂನ್ಯಸೂಚಕ);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr ಪೂರ್ಣಾಂಕ maxSubstIters = 1000;
                        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ it = 0; it < maxSubstIters; ++it) {
                            ತಾರ್ಕಿಕ any = ಸುಳ್ಳು;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ಸ್ಥಿರಾಂಕ targetp = varrefp->varp();
                                Ast_ಗಂಟು* replacep = ಶೂನ್ಯಸೂಚಕ;
                                ಗಾಗಿ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ಆದರೆ (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        ಆದರೆ (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), ನಮೂನೆ)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                                        }
                                        ನಿಲ್ಲಿಸು;
                                    }
                                }
                                ಆದರೆ (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                                }
                                // An inlined pattern takes the type of its parameter
                                Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ patp = VN_CAST(replacep, ನಮೂನೆ);
                                ಆದರೆ (patp && !patp->ಮಗು_DTypep() && targetp->ಮಗು_DTypep()) {
                                    patp->ಮಗು_DTypep(targetp->ಮಗು_DTypep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                                }
                                ಆದರೆ (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_ವೃಕ್ಷ(), varrefp);
                                    any = ನಿಜ;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::ಸದಿಶ<std::pair<AstRefDType*, Ast_ಗಂಟು_DType*>> ಗಮ್ಯ_Replace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                ಆದರೆ (!ptdp) ಹಿಂತಿರುಗಿ;
                                AstPin* overridePinp = ಶೂನ್ಯಸೂಚಕ;
                                ಗಾಗಿ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ಆದರೆ (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        ನಿಲ್ಲಿಸು;
                                    }
                                }
                                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ಗಂಟು_DType)
                                                   : ptdp->subDTypep();
                                ಆದರೆ (substp) ಗಮ್ಯ_Replace.emplace_ಹಿಂಭಾಗ(refp, substp);
                            });
                            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ಗಮ್ಯ_Replace.rbegin(); it != ಗಮ್ಯ_Replace.rend(); ++it) {
                                AstRefDType* ಸ್ಥಿರಾಂಕ refp = it->ಮೊದಲ;
                                refp->replaceWith(it->ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                                VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
                                any = ನಿಜ;
                            }
                            ಆದರೆ (!any) ನಿಲ್ಲಿಸು;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ಗಂಟು* np) {
                            ಆದರೆ (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = ನಿಜ;
                                ಹಿಂತಿರುಗಿ;
                            }
                            ಆದರೆ (Ast_Enum_ಅಂಶ_Ref* ಸ್ಥಿರಾಂಕ ಅಂಶ_Refp = VN_CAST(np, Enum_ಅಂಶ_Ref)) {
                                ಆದರೆ (V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(ಅಂಶ_Refp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = ನಿಜ;
                                }
                                ಹಿಂತಿರುಗಿ;
                            }
                            AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(np, RefDType);
                            ಆದರೆ (!refp) ಹಿಂತಿರುಗಿ;
                            ಆದರೆ (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = ನಿಜ;
                            }
                            // A type declared in the template must not be widthed there
                            ಆದರೆ (V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = ನಿಜ;
                            }
                        });
                    }
                    ಆದರೆ (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            ಆದರೆ (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlink_Fr_ಹಿಂಭಾಗ();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_ವೃಕ್ಷ(), cloneVarp);
                }
                AstConst* ಸ್ಥಿರಾಂಕ namingExprp = normedNamep ? normedNamep : exprp;
                ಆದರೆ (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ pinExprp = pinp->exprp();
                    pinExprp->replaceWith(ಹೊಸ AstConst{pinp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                                       modvarp->ಅಗಲ(), 0});
                    VL_DO_DANGLING(pinExprp->delete_ವೃಕ್ಷ(), pinExprp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (origp
                           && (exprp->same_ವೃಕ್ಷ(origp)
                               || (exprp->ಅಂಕೆ().ಅಗಲ() == origp->ಅಂಕೆ().ಅಗಲ()
                                   && Parameterized_Hier_ಖಂಡಗಳು::areSame(exprp, origp))
                               || param_Consts_Equal_At_Max_ಅಗಲ(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (namingExprp->ಅಂಕೆ().isDouble() || namingExprp->ಅಂಕೆ().isString()
                           || namingExprp->ಅಂಕೆ().isFourState()
                           || namingExprp->ಅಂಕೆ().ಅಗಲ() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(namingExprp));
                    any_overridesr = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->ಅಂಕೆ().ascii(ಸುಳ್ಳು));
                    any_overridesr = ನಿಜ;
                }
                ಆದರೆ (normedNamep) VL_DO_DANGLING(normedNamep->delete_ವೃಕ್ಷ(), normedNamep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ಗಂಟು_DType* rawTypep = VN_CAST(pinp->exprp(), ಗಂಟು_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            ತಾರ್ಕಿಕ skip_ಅಗಲ_For_Template_Struct = ಸುಳ್ಳು;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp
                        = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    ಆದರೆ (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skip_ಅಗಲ_For_Template_Struct = ನಿಜ;
                        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                ಆದರೆ (rawTypep && !skip_ಅಗಲ_For_Template_Struct) V3_ಅಗಲ::ಅಗಲ_Params_Edit(rawTypep);
            }
            Ast_ಗಂಟು_DType* exprp = rawTypep ? rawTypep->skip_Ref_ಗಮ್ಯ_Non_Refp() : ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* origp = modvarp->skip_Ref_ಗಮ್ಯ_Non_Refp();
            ಆದರೆ (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                ಆದರೆ (!skip_ಅಗಲ_For_Template_Struct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ಗಂಟು_DType);
                    exprp = rawTypep->skip_Ref_ಗಮ್ಯ_Non_Refp();
                }
                ಆದರೆ (!modvarp->fwdType().is_ಗಂಟು_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                ಆದರೆ (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (!skip_ಅಗಲ_For_Template_Struct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ಗಂಟು_DType);
                        exprp = rawTypep ? rawTypep->skip_Ref_ಗಮ್ಯ_Non_Refp() : ಶೂನ್ಯಸೂಚಕ;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        ಆದರೆ (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    ಶೂನ್ಯ cellInterfaceCleanup(AstPin* pinsp, Ast_ಗಂಟು_Module* srcModp, string& longnamer,
                              ತಾರ್ಕಿಕ& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        ಗಾಗಿ (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ modvarp = pinp->modVarp();
            ಆದರೆ (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Irefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ exprp = pinp->exprp();
                ಆದರೆ (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    ಮುಂದುವರಿ;
                }
                AstIfaceRefDType* pinIrefp = ಶೂನ್ಯಸೂಚಕ;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ vrp = VN_CAST(exprp, ಗಂಟು_Var_Ref)) {
                    ಆದರೆ (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                ಆದರೆ (!pinIrefp && exprp) {
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* basep = exprp;
                    ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(basep, ಗಂಟು_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrp = VN_CAST(basep, VarRef)) {
                        ಆದರೆ (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << ಪೋರ್ಟ್_Irefp);

                ಆದರೆ (!ಪೋರ್ಟ್_Irefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    ಆದರೆ (ಪೋರ್ಟ್_Irefp->iface_ಮೂಲಕ_Cellp() != pinIrefp->iface_ಮೂಲಕ_Cellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(pinIrefp));
                        any_overridesr = ನಿಜ;
                        ifaceRefRefs.emplace_ಹಿಂಭಾಗ(ಪೋರ್ಟ್_Irefp, pinIrefp);
                        ಆದರೆ (ಪೋರ್ಟ್_Irefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && ಪೋರ್ಟ್_Irefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ಗಂಟು::prettyNameQ(ಪೋರ್ಟ್_Irefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ಗಂಟು::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ storeOriginalParams(Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classp) {
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ಗಂಟು* originalParamp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->isGParam()) originalParamp = varp->clonep();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ಆದರೆ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ಆದರೆ (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    ಶೂನ್ಯ generic_Interface_Var_ಸಿದ್ಧತೆ(ಸ್ಥಿರಾಂಕ AstPin* ಸ್ಥಿರಾಂಕ paramsp, ಸ್ಥಿರಾಂಕ AstPin* ಸ್ಥಿರಾಂಕ pinsp) {
        std::unordered_map<string, ಸ್ಥಿರಾಂಕ AstPin*> paramspMap;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ಸೇರಿಸು({pinp->name().substr(std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__VGIfaceParam")), pinp});
            }
        }

        ಆದರೆ (paramspMap.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPin* ಸ್ಥಿರಾಂಕ pinp = VN_CAST(nodep, Pin)) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = pinp->modVarp()) {
                    Ast_ಗಂಟು_DType* dtypep = varp->ಮಗು_DTypep()->elemDTypep();
                    ಆದರೆ (AstIfaceGenericDType* ಸ್ಥಿರಾಂಕ ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = paramspMap.ಕಂಡುಹಿಡಿ(varp->name());
                        ಆದರೆ (iter == paramspMap.ಕೊನೆ()) ಮುಂದುವರಿ;
                        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlink_Fr_ಹಿಂಭಾಗ();
                        ಸ್ಥಿರಾಂಕ AstPin* ಸ್ಥಿರಾಂಕ paramp = iter->ಎರಡನೇ;
                        paramspMap.erase(iter);
                        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ಸ್ಥಿರಾಂಕ newIfacerefp = ಹೊಸ AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ arrDtp = VN_CAST(backp, ಗಂಟು_Array_DType)) {
                            arrDtp->ಮಗು_DTypep(newIfacerefp);
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->ಮಗು_DTypep(newIfacerefp);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            varp->ಮಗು_DTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ತಳ್ಳು_Deletep(ifaceGDTypep), ifaceGDTypep);
                        ಆದರೆ (paramspMap.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        UASSERT(paramspMap.ಖಾಲಿಗಳು(), "Not every generic interface implicit param is used");
    }

    ಶೂನ್ಯ resolveDefaultParams(Ast_ಗಂಟು* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = ಶೂನ್ಯಸೂಚಕ;
        AstPin* paramsp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            ಆದರೆ (!classp) ಹಿಂತಿರುಗಿ;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_ಸೂಚ್ಯಂಕ.clear();
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                ಆದರೆ (!paramTypep->isGParam()) ಮುಂದುವರಿ;
                m_param_ಸೂಚ್ಯಂಕ.emplace(paramTypep, static_cast<ಪೂರ್ಣಾಂಕ>(m_classTypeParams.ಗಾತ್ರ_2()));
                m_classTypeParams.emplace_ಹಿಂಭಾಗ(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::ಸದಿಶ<Ast_ಗಂಟು_DType*> defaultTypeNodes(m_classTypeParams.ಗಾತ್ರ_2(), ಶೂನ್ಯಸೂಚಕ);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < m_classTypeParams.ಗಾತ್ರ_2(); ++i) {
            AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep = m_classTypeParams[i].ಮೊದಲ;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ಸ್ಥಿರಾಂಕ reqDtp = VN_CAST(paramTypep->get_ಮಗು_DTypep(), RequireDType);
            ಆದರೆ (!reqDtp) ಮುಂದುವರಿ;

            // If default is a reference to another param type, record dependency
            ಆದರೆ (AstRefDType* ಸ್ಥಿರಾಂಕ refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ಮೂಲ_Paramp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    ಸ್ವಯಂಚಾಲಿತ it = m_param_ಸೂಚ್ಯಂಕ.ಕಂಡುಹಿಡಿ(ಮೂಲ_Paramp);
                    ಆದರೆ (it != m_param_ಸೂಚ್ಯಂಕ.ಕೊನೆ()) { m_classTypeParams[i].ಎರಡನೇ = it->ಎರಡನೇ; }
                    ಮುಂದುವರಿ;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(reqDtp->lhsp(), ಗಂಟು_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::ಸದಿಶ<AstPin*> pins_By_ಸೂಚ್ಯಂಕ;
        pins_By_ಸೂಚ್ಯಂಕ.resize(m_classTypeParams.ಗಾತ್ರ_2(), ಶೂನ್ಯಸೂಚಕ);
        ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ಸ್ಥಿರಾಂಕ typep = pinp->modPTypep();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = typep ? m_param_ಸೂಚ್ಯಂಕ.ಕಂಡುಹಿಡಿ(typep) : m_param_ಸೂಚ್ಯಂಕ.ಕೊನೆ();
            ಆದರೆ (it != m_param_ಸೂಚ್ಯಂಕ.ಕೊನೆ()) {
                pins_By_ಸೂಚ್ಯಂಕ[it->ಎರಡನೇ] = pinp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                pins_By_ಸೂಚ್ಯಂಕ.ತಳ್ಳು_ಹಿಂಭಾಗ(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        ಗಾಗಿ (ತಾರ್ಕಿಕ resolvedSome = ನಿಜ; resolvedSome;) {
            resolvedSome = ಸುಳ್ಳು;
            ಗಾಗಿ (ಗಾತ್ರ_t paramIdx = 0; paramIdx < m_classTypeParams.ಗಾತ್ರ_2(); paramIdx++) {
                ಆದರೆ (pins_By_ಸೂಚ್ಯಂಕ[paramIdx]) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೂಲ_Param_Idx = m_classTypeParams[paramIdx].ಎರಡನೇ;

                AstPin* newPinp = ಶೂನ್ಯಸೂಚಕ;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                ಆದರೆ (ಮೂಲ_Param_Idx >= 0 && pins_By_ಸೂಚ್ಯಂಕ[ಮೂಲ_Param_Idx]) {
                    newPinp = pins_By_ಸೂಚ್ಯಂಕ[ಮೂಲ_Param_Idx]->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                ಆದರೆ (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = defaultTypeNodes[paramIdx];
                    newPinp = ಹೊಸ AstPin{dtypep->fileline(), static_cast<ಪೂರ್ಣಾಂಕ>(paramIdx) + 1,
                                         "__paramNumber" + cvt_ಗಮ್ಯ_Str(paramIdx + 1),
                                         dtypep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
                }

                ಆದರೆ (newPinp) {
                    newPinp->name("__paramNumber" + cvt_ಗಮ್ಯ_Str(paramIdx + 1));
                    newPinp->param(ನಿಜ);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].ಮೊದಲ);
                    ಆದರೆ (classOrPackageRef) {
                        classOrPackageRef->ಕೂಡಿಸು_Paramsp(newPinp);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (classRefDType) {
                        classRefDType->ಕೂಡಿಸು_Paramsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_ಸೂಚ್ಯಂಕ[paramIdx] = newPinp;
                    ಆದರೆ (!paramsp) paramsp = newPinp;
                    resolvedSome = ನಿಜ;
                }
            }
        }
    }

    Ast_ಗಂಟು_Module* ಗಂಟು_Deparam_Common(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, ತಾರ್ಕಿಕ any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        ಆದರೆ (debug() >= 9 && paramsp) paramsp->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(ಹೊರಗೆ, "-  cellparams: ");

        ಆದರೆ (srcModp->hier_ಖಂಡ()) {
            longname = parameterized_Hier_ಖಂಡ_Name(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstPin*> overridingTypePins;
            ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (!pinp->exprp()) ಮುಂದುವರಿ;  // An empty override keeps the default
                ಆದರೆ (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    ಮುಂದುವರಿ;
                }
                AstVar* ಸ್ಥಿರಾಂಕ modvarp = pinp->modVarp();
                ಆದರೆ (!modvarp || !modvarp->isGParam()) ಮುಂದುವರಿ;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ಸ್ಥಿರಾಂಕ Ast_ನಮೂನೆ* ಸ್ಥಿರಾಂಕ patternp = VN_CAST(pinp->exprp(), ನಮೂನೆ);
                ಆದರೆ (patternp && !patternp->ಮಗು_DTypep()) ಮುಂದುವರಿ;
                ಆದರೆ (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_ಮೌಲ್ಯ(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ modptp = pinp->modPTypep()) {
                    ಆದರೆ (!overridingTypePins.ಎಣಿಕೆ_2(pinp)) ಮುಂದುವರಿ;
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ typep = VN_AS(pinp->exprp(), ಗಂಟು_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(typep->skip_Ref_ಗಮ್ಯ_Non_Refp());
                    any_overrides = ನಿಜ;
                    ಮುಂದುವರಿ;
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
        ತಾರ್ಕಿಕ defaultsResolved = ಸುಳ್ಳು;
        ಆದರೆ (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            ಗಾಗಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಆದರೆ (pinp->modPTypep()) {
                    any_overrides = ನಿಜ;
                    defaultsResolved = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಆದರೆ (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.ಗಮ್ಯ_String();
            any_overrides = ನಿಜ;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ಗಂಟು_Module* newModp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (m_hier_ಖಂಡಗಳು.hier_Sub_ಸತತ() && m_hier_ಖಂಡಗಳು.is_Hier_ಖಂಡ(srcModp->origName())) {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ paramedModp
                = m_hier_ಖಂಡಗಳು.ಕಂಡುಹಿಡಿ_By_Params(srcModp->origName(), paramsp, m_modp);
            ಆದರೆ (!paramedModp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Unsupported, already reported
            paramedModp->dead(ಸುಳ್ಳು);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            ಆದರೆ (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ಗಂಟು_Module* ಗಂಟು_Copyp = srcModp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ಗಂಟು_Copyp);
                m_deleter.ತಳ್ಳು_Deletep(ಗಂಟು_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ಗಂಟು_Copyp);
                storeOriginalParams(ಗಂಟು_Copyp);
            }
            newModp = srcModp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ string newname
                = srcModp->hier_ಖಂಡ() ? longname : moduleCalcName(srcModp, longname);
            ಸ್ಥಿರಾಂಕ ModInfo* ಸ್ಥಿರಾಂಕ modInfop
                = module_ಕಂಡುಹಿಡಿ_Or_Clone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            ಆದರೆ (!modInfop) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ಸ್ಥಿರಾಂಕ newClassp = VN_CAST(newModp, Class);
        ಆದರೆ (newClassp && !newClassp->user2()) m_specializedClassps.ತಳ್ಳು_ಹಿಂಭಾಗ(newClassp);

        // Link source class to its specialized version for later relinking of method references
        ಆದರೆ (defaultsResolved) srcModp->user4p(newModp);

        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                ಆದರೆ (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        generic_Interface_Var_ಸಿದ್ಧತೆ(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        ಆದರೆ (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ(), paramsp);
        ಹಿಂತಿರುಗಿ newModp;
    }

    Ast_ಗಂಟು_Module* cellDeparam(AstCell* nodep, Ast_ಗಂಟು_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        ಆದರೆ (!newModp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(ಸುಳ್ಳು);
        ಹಿಂತಿರುಗಿ newModp;
    }
    Ast_ಗಂಟು_Module* ifaceRefDeparam(AstIfaceRefDType* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        ಆದರೆ (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ಸ್ಥಿರಾಂಕ string ownerOrigName
                = ownerIfacep->origName().ಖಾಲಿಗಳು() ? ownerIfacep->name() : ownerIfacep->origName();
            ಸ್ಥಿರಾಂಕ string srcOrigName
                = srcModp->origName().ಖಾಲಿಗಳು() ? srcModp->name() : srcModp->origName();
            string owner_ತಳ_Name = ownerOrigName;
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ownerPos = owner_ತಳ_Name.ಕಂಡುಹಿಡಿ("__");
            ಆದರೆ (ownerPos != string::npos) owner_ತಳ_Name = owner_ತಳ_Name.substr(0, ownerPos);
            string src_ತಳ_Name = srcOrigName;
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t srcPos = src_ತಳ_Name.ಕಂಡುಹಿಡಿ("__");
            ಆದರೆ (srcPos != string::npos) src_ತಳ_Name = src_ತಳ_Name.substr(0, srcPos);

            ಆದರೆ (owner_ತಳ_Name == src_ತಳ_Name) {
                ತಾರ್ಕಿಕ allOwnParams = ನಿಜ;
                ಗಾಗಿ (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    ಆದರೆ (AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(pinp->exprp(), RefDType)) {
                        ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ptdOwnerp
                                = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(ptdp);
                            ಆದರೆ (ptdOwnerp != m_modp) allOwnParams = ಸುಳ್ಳು;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                ಆದರೆ (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    ಆದರೆ (nodep->paramsp()) nodep->paramsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
                    ಹಿಂತಿರುಗಿ ownerIfacep;
                }
            }
        }

        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp
            = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(), ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು);
        ಆದರೆ (!newModp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        nodep->ifacep(VN_AS(newModp, Iface));
        ಹಿಂತಿರುಗಿ newModp;
    }
    Ast_ಗಂಟು_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ಗಂಟು_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp
            = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(), ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು);
        ಆದರೆ (!newModp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ಸ್ಥಿರಾಂಕ refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ಸ್ಥಿರಾಂಕ newClassp = refDTypep ? VN_CAST(newModp, Class) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ typedefp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        ಹಿಂತಿರುಗಿ newModp;
    }
    Ast_ಗಂಟು_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ಗಂಟು_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp
            = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(), ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು);
        ಆದರೆ (!newModp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstClass* ಸ್ಥಿರಾಂಕ newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        ಹಿಂತಿರುಗಿ newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    ವರ್ಗ DeferredResolverVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*> m_reachedDecls;

        ತಾರ್ಕಿಕ ಮೊದಲ_ವ್ಯಾಪ್ತಿ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declp) { ಹಿಂತಿರುಗಿ m_reachedDecls.ಸೇರಿಸು(declp).ಎರಡನೇ; }

        ಶೂನ್ಯ iterateParamType(AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep) {
            ಆದರೆ (ಮೊದಲ_ವ್ಯಾಪ್ತಿ(paramTypep)) iterate(paramTypep);
        }
        ಶೂನ್ಯ iterateTypedef(AstTypedef* ಸ್ಥಿರಾಂಕ tdefp) {
            ಆದರೆ (ಮೊದಲ_ವ್ಯಾಪ್ತಿ(tdefp)) iterate(tdefp->subDTypep());
        }

        ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
        ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ targetp = nodep->classOrPackageNodep();
            ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        ಶೂನ್ಯ ಭೇಟಿ(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolve_Dot_ಗಮ್ಯ_Typedef(nodep);
        }
        ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ಸ್ಥಿರಾಂಕ tdefp = nodep->typedefp();
            ಆದರೆ (tdefp) {
                iterateTypedef(tdefp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
            ಆದರೆ (ಮೊದಲ_ವ್ಯಾಪ್ತಿ(varp)) {
                iterate(varp->subDTypep());
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& deferredVarps = v3Global.rootp()->deferredParamVarps();
                ಆದರೆ (varp->varType() == VVarType::LPARAM && deferredVarps.ಎಣಿಕೆ_2(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    ಸಾರ್ವಜನಿಕ:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ಗಂಟು* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

ಸಾರ್ವಜನಿಕ:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    ಶೂನ್ಯ retargetIfaceRefs(Ast_ಗಂಟು_Module* ಪೋಷಕ_Modp, ಸ್ಥಿರಾಂಕ string& cellName) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ correctModp
            = V3LinkDotIfaceCapture::follow_Cell_ಮಾರ್ಗ(ಪೋಷಕ_Modp, cellName);
        ಆದರೆ (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) ಹಿಂತಿರುಗಿ;
        ಪೋಷಕ_Modp->foreach([&](AstRefDType* refp) {
            ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* ಸ್ಥಿರಾಂಕ tagp = V3LinkDotIfaceCapture::capture_ಟ್ಯಾಗ್(refp);
            ಆದರೆ (!tagp || tagp->m_cell_ಮಾರ್ಗ != cellName) ಹಿಂತಿರುಗಿ;
            // Only retarget REFDTYPEs owned by parentModp.
            ಆದರೆ (V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(refp) != ಪೋಷಕ_Modp) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (V3LinkDotIfaceCapture::retarget_Ref_ಗಮ್ಯ_Module(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    ಶೂನ್ಯ resolveDeferredDotsReachableFrom(Ast_ಗಂಟು* rootp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*ಇದು, rootp};
    }

    Ast_ಗಂಟು_Module* ಗಂಟು_Deparam(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_Module* srcModp, Ast_ಗಂಟು_Module* modp,
                               ಸ್ಥಿರಾಂಕ string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ಗಂಟು::ಗಂಟು_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ಗಂಟು_Name = nodep->name();
        ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            ಆದರೆ (ಗಂಟು_Name.ಖಾಲಿಗಳು()) ಗಂಟು_Name = ifaceRefp->cellName();
        }
        ಸ್ಥಿರಾಂಕ string instanceName
            = ಗಂಟು_Name.ಖಾಲಿಗಳು() ? someInstanceName : (someInstanceName + "." + ಗಂಟು_Name);

        ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell)) {
            ಗಾಗಿ (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ಸ್ಥಿರಾಂಕ nextp = VN_AS(pinp->nextp(), Pin);
                ಆದರೆ (!pinp->param_ಮಾರ್ಗ().ಖಾಲಿಗಳು() && instanceName != pinp->param_ಮಾರ್ಗ()
                    && !VString::ತುದಿಗಳು_With(instanceName, "." + pinp->param_ಮಾರ್ಗ())) {
                    VL_DO_DANGLING(pinp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ಗಂಟು::ಗಂಟು_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ಗಂಟು_Module* newModp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        ಆದರೆ (!newModp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ಗಂಟು::ಗಂಟು_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        ಹಿಂತಿರುಗಿ newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::ಸದಿಶ<AstClass*> takeSpecializedClassps() {
        std::ಸದಿಶ<AstClass*> ತೆಗೆದುಕೊಂಡ;
        ತೆಗೆದುಕೊಂಡ.ಅದಲುಬದಲು(m_specializedClassps);
        ಹಿಂತಿರುಗಿ ತೆಗೆದುಕೊಂಡ;
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ParamProcessor(AstNetlist* nodep)
        : m_hier_ಖಂಡಗಳು{v3Global.opt.hier_ಖಂಡಗಳು(), nodep} {
        ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            m_all_Module_ಹೆಸರುಗಳು.ಸೇರಿಸು(modp->name());
        }
    }
    ~ParamProcessor() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

ವರ್ಗ ClassRefUnlinkerVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        ಆದರೆ (nodep->paramsp()) {
            ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                ಆದರೆ (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep->backp(), Dot);
                    ಆದರೆ (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
                        VL_DO_DANGLING2(ತಳ್ಳು_Deletep(dotp), dotp, nodep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {}  // don't iterate inside classes
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

ವರ್ಗ ParamClassRefDTypeRelinkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {

    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ಗಂಟು_Module* m_ownerModp = ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು_Module* m_ownerMapForp = ಶೂನ್ಯಸೂಚಕ;
    std::unordered_map<std::string, AstClass*> m_orig_Name_ಗಮ್ಯ_Clone;
    std::unordered_ಗಣ<std::string> m_ambiguous;

    ಶೂನ್ಯ ensureOwnerMap() {
        ಆದರೆ (m_ownerMapForp == m_ownerModp) ಹಿಂತಿರುಗಿ;
        m_ownerMapForp = m_ownerModp;
        m_orig_Name_ಗಮ್ಯ_Clone.clear();
        m_ambiguous.clear();
        ಆದರೆ (!m_ownerModp) ಹಿಂತಿರುಗಿ;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ಆದರೆ (ownerClassp->hasGParam()) ಹಿಂತಿರುಗಿ;
        }
        ಗಾಗಿ (Ast_ಗಂಟು* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ಸ್ಥಿರಾಂಕ tdp = VN_CAST(sp, Typedef);
            ಆದರೆ (!tdp) ಮುಂದುವರಿ;
            AstClassRefDType* ಸ್ಥಿರಾಂಕ crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            ಆದರೆ (!crdtp || !crdtp->classp()) ಮುಂದುವರಿ;
            AstClass* ಸ್ಥಿರಾಂಕ targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            ಆದರೆ (targetp->hasGParam()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ std::string origName
                = targetp->origName().ಖಾಲಿಗಳು() ? targetp->name() : targetp->origName();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_orig_Name_ಗಮ್ಯ_Clone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            ಆದರೆ (!pair.ಎರಡನೇ && pair.ಮೊದಲ->ಎರಡನೇ != targetp) m_ambiguous.ಸೇರಿಸು(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ಸ್ಥಿರ string classOrigName(ಸ್ಥಿರಾಂಕ AstClass* classp) {
        ಹಿಂತಿರುಗಿ classp->origName().ಖಾಲಿಗಳು() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ಸ್ಥಿರ AstTypedef* ಕಂಡುಹಿಡಿ_Typedef_With_Bases(AstClass* classp, ಸ್ಥಿರಾಂಕ string& name) {
        ಗಾಗಿ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ) {
            ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ tdp = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Typedef_ಒಳ_Module(cp, name)) {
                ಆದರೆ (tdp->subDTypep()) ಹಿಂತಿರುಗಿ tdp;
            }
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    ಶೂನ್ಯ retargetRefDType(AstRefDType* refp) {
        ಆದರೆ (!m_ownerModp) ಹಿಂತಿರುಗಿ;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ಆದರೆ (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ಸ್ಥಿರಾಂಕ AstClass* refClassp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(tdp), Class);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                ಆದರೆ (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ಸ್ಥಿರಾಂಕ selfTdp = ಕಂಡುಹಿಡಿ_Typedef_With_Bases(ownerClassp, refp->name());
                    ಆದರೆ (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        AstTypedef* ಸ್ಥಿರಾಂಕ oldTdp = refp->typedefp();
        ಆದರೆ (!oldTdp) ಹಿಂತಿರುಗಿ;
        AstClass* ಸ್ಥಿರಾಂಕ oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(oldTdp), Class);
        ಆದರೆ (!oldOwnerp) ಹಿಂತಿರುಗಿ;

        ensureOwnerMap();
        ಆದರೆ (m_orig_Name_ಗಮ್ಯ_Clone.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ std::string origName
            = oldOwnerp->origName().ಖಾಲಿಗಳು() ? oldOwnerp->name() : oldOwnerp->origName();
        ಆದರೆ (m_ambiguous.ಎಣಿಕೆ_2(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_orig_Name_ಗಮ್ಯ_Clone.ಕಂಡುಹಿಡಿ(origName);
        ಆದರೆ (it == m_orig_Name_ಗಮ್ಯ_Clone.ಕೊನೆ()) ಹಿಂತಿರುಗಿ;
        AstClass* ಸ್ಥಿರಾಂಕ correctClassp = it->ಎರಡನೇ;
        ಆದರೆ (correctClassp == oldOwnerp) ಹಿಂತಿರುಗಿ;
        AstTypedef* ಸ್ಥಿರಾಂಕ newTdp
            = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Typedef_ಒಳ_Module(correctClassp, refp->name());
        ಆದರೆ (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            ಹಿಂತಿರುಗಿ;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        ಆದರೆ (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            ಹಿಂತಿರುಗಿ;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_paramTypeClassMap.ಕಂಡುಹಿಡಿ(paramtypep);
            ಆದರೆ (it != m_paramTypeClassMap.ಕೊನೆ()) {
                AstClass* ಸ್ಥಿರಾಂಕ origClassp = it->ಎರಡನೇ;
                ಆದರೆ (origClassp->hasGParam() && !origClassp->user3p()) {
                    ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ಸ್ಥಿರಾಂಕ string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = ಶೂನ್ಯಸೂಚಕ;
                        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                ಆದರೆ (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    ನಿಲ್ಲಿಸು;
                                }
                            }
                        }
                        ಆದರೆ (newParamTypep) {
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
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

ವರ್ಗ ParamState final {
ಸಾರ್ವಜನಿಕ:
    // STATE - across all visitors
    std::ಸದಿಶ<AstClass*> m_paramClasses;  // Parameterized classes
    std::ಸದಿಶ<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    ಬಳಸಿ WQKey = std::pair<ತಾರ್ಕಿಕ, ಪೂರ್ಣಾಂಕ>;
    std::multimap<WQKey, Ast_ಗಂಟು_Module*> m_work_ಸರತಿ_ಮುಂದಿನ;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ಗಂಟು_Module*, std::unordered_ಗಣ<Ast_ಗಂಟು_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

ವರ್ಗ ParamVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    ತಾರ್ಕಿಕ m_iterateModule = ಸುಳ್ಳು;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<ತಾರ್ಕಿಕ, Ast_ಗಂಟು*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, std::string>
        m_gen_Hier_ಹೆಸರುಗಳು;  // Maps ast nodes to generated hierarchy names
    std::map<ಸ್ಥಿರಾಂಕ AstRefDType*, ತಾರ್ಕಿಕ>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    ಬಳಸಿ VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Module iterating
    std::unordered_ಗಣ<std::string> m_iface_ಪೋರ್ಟ್_ಹೆಸರುಗಳು;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    ತಾರ್ಕಿಕ m_mod_Iface_Refs_ಮುಗಿದ = ಸುಳ್ಳು;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    ತಾರ್ಕಿಕ m_ಒಳ_Generate_Cond = ಸುಳ್ಳು;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    ಶೂನ್ಯ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ಗಂಟು_Module*> work_ಸರತಿ;
        m_generateHierName = "";
        m_iterateModule = ನಿಜ;

        // Visit all cells under module, recursively
        ಆಗ (ನಿಜ) {
            // Classes specialized since the last pass still need their bodies elaborated
            ಗಾಗಿ (AstClass* ಸ್ಥಿರಾಂಕ classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_ಸರತಿ_ಮುಂದಿನ.emplace(ParamState::WQKey{ನಿಜ, classp->level()}, classp);
            }
            ಆದರೆ (work_ಸರತಿ.ಖಾಲಿಗಳು()) std::ಅದಲುಬದಲು(work_ಸರತಿ, m_state.m_work_ಸರತಿ_ಮುಂದಿನ);
            ಆದರೆ (work_ಸರತಿ.ಖಾಲಿಗಳು()) ನಿಲ್ಲಿಸು;

            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itm = work_ಸರತಿ.cbegin();
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = itm->ಎರಡನೇ;
            work_ಸರತಿ.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            ಆದರೆ (!modp->user2_ಗಣ_Once()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                ಆದರೆ (modp->someInstanceName().ಖಾಲಿಗಳು()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_iface_ಪೋರ್ಟ್_ಹೆಸರುಗಳು);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_mod_Iface_Refs_ಮುಗಿದ);
                    m_modp = modp;
                    m_mod_Iface_Refs_ಮುಗಿದ = ಸುಳ್ಳು;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            ಆಗ (!m_cellps.ಖಾಲಿಗಳು()) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itim = m_cellps.cbegin();
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ cellp = itim->ಎರಡನೇ;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ genHierIt = m_gen_Hier_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(cellp);
                ಆದರೆ (genHierIt != m_gen_Hier_ಹೆಸರುಗಳು.ಕೊನೆ()) {
                    genHierName = std::move(genHierIt->ಎರಡನೇ);
                    m_gen_Hier_ಹೆಸರುಗಳು.erase(genHierIt);
                }

                Ast_ಗಂಟು_Module* srcModp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    ಆದರೆ (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                ಆದರೆ (!srcModp) ಮುಂದುವರಿ;

                // Update path
                ಸ್ಥಿರಾಂಕ string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp
                    = m_processor.ಗಂಟು_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    ಆದರೆ (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        ಆದರೆ (V3LinkDotIfaceCapture::enabled()) {
                            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ modCellp = VN_CAST(cellp, Cell)) {
                                ಆದರೆ (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    work_ಸರತಿ.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ಸೇರಿಸು(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    ಆದರೆ (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = ಸುಳ್ಳು;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ಸ್ಥಿರ string get_Ref_ತಳ_Name(ಸ್ಥಿರಾಂಕ AstVarXRef* refp) {
        ಸ್ಥಿರಾಂಕ string dotted = refp->dotted();
        ಆದರೆ (dotted.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ "";
        ಹಿಂತಿರುಗಿ dotted.substr(0, dotted.ಕಂಡುಹಿಡಿ('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    ಶೂನ್ಯ logTemplateLeakRefs(Ast_ಗಂಟು_Module* ಪೋಷಕ_Modp, Ast_ಗಂಟು_Module* templateModp,
                             ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* stage, Ast_ಗಂಟು* contextp) {
        ಆದರೆ (debug() < 9 || !ಪೋಷಕ_Modp || !templateModp || !VN_IS(templateModp, Iface)) ಹಿಂತಿರುಗಿ;
        // LCOV_EXCL_START  // Debug-only diagnostic
        ಪೂರ್ಣಾಂಕ leak_ಎಣಿಕೆ = 0;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ancestry_ದ = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
            string ancestry;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* curp = nodep; curp; curp = curp->backp()) {
                ಆದರೆ (!ancestry.ಖಾಲಿಗಳು()) ancestry += "<-";
                ancestry += curp->typeName();
                ಆದರೆ (VN_IS(curp, ಗಂಟು_Module) || VN_IS(curp, Netlist) || VN_IS(curp, Type_ಕೋಷ್ಟಕ)) {
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಹಿಂತಿರುಗಿ ancestry;
        };

        ಪೋಷಕ_Modp->foreach([&](AstRefDType* refp) {
            ಆದರೆ (refp->typedefp()) {
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ tdOwnerp
                    = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(refp->typedefp());
                ಆದರೆ (tdOwnerp == templateModp) {
                    ++leak_ಎಣಿಕೆ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << ಪೋಷಕ_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ಗಂಟು::ಗಂಟು_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_ದ(refp));
                }
            }
            ಆದರೆ (refp->refDTypep()) {
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ rdOwnerp
                    = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(refp->refDTypep());
                ಆದರೆ (rdOwnerp == templateModp) {
                    ++leak_ಎಣಿಕೆ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << ಪೋಷಕ_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ಗಂಟು::ಗಂಟು_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_ದ(refp));
                }
            }
        });

        ಪೋಷಕ_Modp->foreach([&](AstVarRef* varrefp) {
            ಆದರೆ (!varrefp->varp()) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varOwnerp
                = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(varrefp->varp());
            ಆದರೆ (varOwnerp != templateModp) ಹಿಂತಿರುಗಿ;
            ++leak_ಎಣಿಕೆ;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << ಪೋಷಕ_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ಗಂಟು::ಗಂಟು_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestry_ದ(varrefp));
        });

        ಆದರೆ (leak_ಎಣಿಕೆ > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << ಪೋಷಕ_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_ಎಣಿಕೆ);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    ಶೂನ್ಯ checkParamNotHierRecurse(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ underTypeQuery = ಸುಳ್ಳು) {
        ಗಾಗಿ (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ attrp = VN_CAST(nodep, Attr_ದ);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮಗು_Under_Query
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                ತಾರ್ಕಿಕ isIfaceRef = ಸುಳ್ಳು;
                ಆದರೆ (refp->varp() && refp->varp()->isIfaceParam()) {
                    ಸ್ಥಿರಾಂಕ string refname = get_Ref_ತಳ_Name(refp);
                    isIfaceRef
                        = !refname.ಖಾಲಿಗಳು()
                          && (m_iface_ಪೋರ್ಟ್_ಹೆಸರುಗಳು.ಎಣಿಕೆ_2(refname) || m_ifaceInstCells.ಎಣಿಕೆ_2(refname));
                }
                ಆದರೆ (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ಗಂಟು_FTask_Ref)) {
                ಆದರೆ (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), ಮಗು_Under_Query);
            checkParamNotHierRecurse(nodep->op2p(), ಮಗು_Under_Query);
            checkParamNotHierRecurse(nodep->op3p(), ಮಗು_Under_Query);
            checkParamNotHierRecurse(nodep->op4p(), ಮಗು_Under_Query);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    ಶೂನ್ಯ specializeNestedIfaceCells(Ast_ಗಂಟು_Module* ifaceModp) {
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ಸ್ಥಿರಾಂಕ nestedCellp = VN_CAST(stmtp, Cell);
            ಆದರೆ (!nestedCellp) ಮುಂದುವರಿ;
            ಆದರೆ (!VN_IS(nestedCellp->modp(), Iface)) ಮುಂದುವರಿ;
            ಆದರೆ (!nestedCellp->paramsp()) ಮುಂದುವರಿ;

            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ nestedSrcModp = nestedCellp->modp();
            ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ nestedNewModp = m_processor.ಗಂಟು_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                ಆದರೆ (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    ಗಾಗಿ (Ast_ಗಂಟು* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(sp, Var)) {
                            ಆದರೆ (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    ಆದರೆ (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    ತಾರ್ಕಿಕ cellParamsReferenceIfacePorts(AstCell* cellp) {
        ಆದರೆ (!cellp->paramsp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಗಾಗಿ (Ast_ಗಂಟು* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            ಆದರೆ (AstPin* ಸ್ಥಿರಾಂಕ pinp = VN_CAST(nodep, Pin)) {
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ exprp = pinp->exprp()) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarXRef)) {
                        ಸ್ಥಿರಾಂಕ string refname = get_Ref_ತಳ_Name(refp);
                        ಆದರೆ (!refname.ಖಾಲಿಗಳು()
                            && (m_iface_ಪೋರ್ಟ್_ಹೆಸರುಗಳು.ಎಣಿಕೆ_2(refname)
                                || m_ifaceInstCells.ಎಣಿಕೆ_2(refname)))
                            ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    ಶೂನ್ಯ expand_Iface_Array_ಪೋರ್ಟ್_Dimensions(AstVar* portp, Ast_ಗಂಟು_DType* dtypep,
                                        ಸ್ಥಿರಾಂಕ std::string& suffix) {
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        ಆದರೆ (!arrp) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = portp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ oldp = varp->ಮಗು_DTypep()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp->unlink_Fr_ಹಿಂಭಾಗ()), oldp);
            }
            varp->dtypep(ಶೂನ್ಯಸೂಚಕ);
            varp->ಮಗು_DTypep(portp->subDTypep()->elemDTypep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            portp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(varp);
            ಹಿಂತಿರುಗಿ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ n : arrp->declRange().seq_ಬಲ_ಗಮ್ಯ_ಎಡ()) {
            ಸ್ಥಿರಾಂಕ std::string s = suffix + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(n) + "__KET__";
            expand_Iface_Array_ಪೋರ್ಟ್_Dimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    ಶೂನ್ಯ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ಸ್ಥಿರಾಂಕ AstRange* rangep, ಸ್ಥಿರಾಂಕ std::string& suffix, ಪೂರ್ಣಾಂಕ idx) {
        // Base case: insert the element when no dimensions left
        ಆದರೆ (!rangep) {
            AstCell* ಸ್ಥಿರಾಂಕ elemp = arrayedCellp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(elemp);
            ಆದರೆ (ifaceVarp) {
                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ಸ್ಥಿರಾಂಕ varp = ifaceVarp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ oldp = varp->ಮಗು_DTypep()) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp->unlink_Fr_ಹಿಂಭಾಗ()), oldp);
                }
                varp->dtypep(ಶೂನ್ಯಸೂಚಕ);
                varp->ಮಗು_DTypep(irefp);
                ifaceVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(varp);
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ಸ್ಥಿರಾಂಕ AstRange* ಸ್ಥಿರಾಂಕ subRangep = VN_AS(rangep->nextp(), Range);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ n : rangep->seq_ಬಲ_ಗಮ್ಯ_ಎಡ()) {
            ಸ್ಥಿರಾಂಕ std::string s = suffix + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    ಶೂನ್ಯ ಭೇಟಿ_Cell_Or_Class_Ref(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_gen_Hier_ಹೆಸರುಗಳು.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        ಆದರೆ (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell);
            ಆದರೆ (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ srcModp = cellp->modp();
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp
                    = m_processor.ಗಂಟು_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                ಆದರೆ (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                ಆದರೆ (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        ಆದರೆ (!m_ಒಳ_Generate_Cond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->recursiveClone()) nodep->dead(ನಿಜ);  // Fake, made for recursive elimination
        ಆದರೆ (nodep->dead()) ಹಿಂತಿರುಗಿ;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) {
            ಆದರೆ (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ತಳ್ಳು_ಹಿಂಭಾಗ(classp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_ಸರತಿ_ಮುಂದಿನ.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Start traversal at root-like things
        ಆದರೆ (nodep->is_ತುದಿ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_ಸರತಿ_ಮುಂದಿನ.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    ತಾರ್ಕಿಕ isCircularType(ಸ್ಥಿರಾಂಕ AstRefDType* nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_isCircular.emplace(nodep, ನಿಜ);
        ಆದರೆ (!iter.ಎರಡನೇ) ಹಿಂತಿರುಗಿ iter.ಮೊದಲ->ಎರಡನೇ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstRefDType* ಸ್ಥಿರಾಂಕ subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ret = isCircularType(subDTypep);
            iter.ಮೊದಲ->ಎರಡನೇ = ret;
            ಹಿಂತಿರುಗಿ ret;
        }
        iter.ಮೊದಲ->ಎರಡನೇ = ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        ಆದರೆ (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        ಆದರೆ (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2_ಗಣ_Once()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        ಆದರೆ (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            ಗಾಗಿ (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3_ಅಗಲ::ಅಗಲ_Params_Edit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ಸ್ಥಿರಾಂಕ ifaceVarp = [&]() -> AstVar* {
                ಆದರೆ (!VN_IS(nodep->modp(), Iface)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                ಹಿಂತಿರುಗಿ varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            ಆದರೆ (ifaceVarp) {
                ifaceVarp->is_Iface_Array_ವಿಭಜಿಸು(ನಿಜ);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ಸ್ಥಿರಾಂಕ firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            ಹಿಂತಿರುಗಿ;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        ಆದರೆ (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        ಭೇಟಿ_Cell_Or_Class_Ref(nodep, VN_IS(nodep->modp(), Iface));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2_ಗಣ_Once();
        ಆದರೆ (nodep->ifacep()) ಭೇಟಿ_Cell_Or_Class_Ref(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2_ಗಣ_Once();
        checkParamNotHierRecurse(nodep->paramsp());
        ಭೇಟಿ_Cell_Or_Class_Ref(nodep, ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        ಆದರೆ (!VN_IS(nodep->classOrPackageNodep(), Typedef)) ಭೇಟಿ_Cell_Or_Class_Ref(nodep, ಸುಳ್ಳು);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ಸ್ಥಿರ ಶೂನ್ಯ gatherVars(Ast_ಗಂಟು* stmtsp, ತಾರ್ಕಿಕ (*matchp)(ಸ್ಥಿರಾಂಕ AstVar*), VarsByName& varsr) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                ಆದರೆ (matchp(varp)) varsr.emplace(varp->name(), varp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_Gen_ಖಂಡ* ಸ್ಥಿರಾಂಕ genp = VN_CAST(nodep, Gen_ಖಂಡ)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ಸ್ಥಿರಾಂಕ VarsByName& modIfaceRefs() {
        ಆದರೆ (!m_mod_Iface_Refs_ಮುಗಿದ) {
            gatherVars(
                m_modp->stmtsp(), [](ಸ್ಥಿರಾಂಕ AstVar* varp) { ಹಿಂತಿರುಗಿ varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_mod_Iface_Refs_ಮುಗಿದ = ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ m_modIfaceRefs;
    }
    ಸ್ಥಿರಾಂಕ VarsByName& ifaceParams(Ast_ಗಂಟು_Module* ifacep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        ಆದರೆ (pair.ಎರಡನೇ) {
            gatherVars(
                ifacep->stmtsp(), [](ಸ್ಥಿರಾಂಕ AstVar* varp) { ಹಿಂತಿರುಗಿ varp->isParam(); },
                pair.ಮೊದಲ->ಎರಡನೇ);
        }
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ಗಂಟು* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        ಆದರೆ (!lhsp || !rhsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        ಆದರೆ (VL_UNCOVERABLE(!m_modp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& refVars = modIfaceRefs();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ifaceVarIt = refVars.ಕಂಡುಹಿಡಿ(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        ಆದರೆ (VL_UNCOVERABLE(ifaceVarIt == refVars.ಕೊನೆ())) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVar* ಸ್ಥಿರಾಂಕ ifaceVarp = ifaceVarIt->ಎರಡನೇ;
        AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        ಆದರೆ (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗುರಿ_Var_It = ifaceVars.ಕಂಡುಹಿಡಿ(rhsp->name());
        ಆದರೆ (ಗುರಿ_Var_It == ifaceVars.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVar* ಸ್ಥಿರಾಂಕ ಗುರಿ_Varp = ಗುರಿ_Var_It->ಎರಡನೇ;
        // processWorkQ() visits interface cells first, so the interface is already constified
        ಆದರೆ (VL_UNCOVERABLE(!VN_IS(ಗುರಿ_Varp->valuep(), Const))) iterate(ಗುರಿ_Varp);
        ಆದರೆ (VL_UNCOVERABLE(!ಗುರಿ_Varp->valuep() || !VN_IS(ಗುರಿ_Varp->valuep(), Const))) {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // LCOV_EXCL_LINE
        }
        ಹಿಂತಿರುಗಿ ಗುರಿ_Varp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ resolve_Generic_Iface_Dots_ಒಳ(Ast_ಗಂಟು* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::ಸದಿಶ<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ತಳ್ಳು_ಹಿಂಭಾಗ(dotp); });
        ಗಾಗಿ (AstDot* ಸ್ಥಿರಾಂಕ dotp : dotps) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (nodep->user2_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        // Interface array port: split it into its elements
        ಆದರೆ (nodep->isIfaceRef() && !nodep->is_Iface_ಪೋಷಕ()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep->subDTypep());
            expand_Iface_Array_ಪೋರ್ಟ್_Dimensions(nodep, nodep->subDTypep(), "");
            nodep->is_Iface_Array_ವಿಭಜಿಸು(ನಿಜ);
        }
        // Build cache of interface port names as we encounter them
        ಆದರೆ (nodep->isIfaceRef()) { m_iface_ಪೋರ್ಟ್_ಹೆಸರುಗಳು.ಸೇರಿಸು(nodep->name()); }
        iterateChildren(nodep);
        ಆದರೆ (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            ಆದರೆ (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolve_Generic_Iface_Dots_ಒಳ(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                ತಾರ್ಕಿಕ hasUnresolvedLparamXRef = ಸುಳ್ಳು;
                nodep->valuep()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarXRef* xrefp) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = xrefp->varp()) {
                        ಆದರೆ (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = ನಿಜ;
                        }
                    }
                });
                ಆದರೆ (hasUnresolvedLparamXRef) ಹಿಂತಿರುಗಿ;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasDot = nodep->valuep()->exists([](AstDot*) { ಹಿಂತಿರುಗಿ ನಿಜ; });
                ತಾರ್ಕಿಕ refsDeferred = ಸುಳ್ಳು;
                ಆದರೆ (!hasDot) {
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
                        ಆದರೆ (refsDeferred) ಹಿಂತಿರುಗಿ;
                        AstVar* ಸ್ಥಿರಾಂಕ refVarp = refp->varp();
                        ಆದರೆ (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.ಎಣಿಕೆ_2(refVarp)) {
                            refsDeferred = ನಿಜ;
                        }
                    });
                }
                ಆದರೆ (hasDot || refsDeferred) {
                    v3Global.rootp()->ತಳ್ಳು_Deferred_Param_Varp(nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        ಆದರೆ (nodep->varp()) iterate(nodep->varp());
    }
    ತಾರ್ಕಿಕ ifaceParamReplace(AstVarXRef* nodep, Ast_ಗಂಟು* candp) {
        ಗಾಗಿ (; candp; candp = candp->nextp()) {
            ಆದರೆ (nodep->name() == candp->name()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    ಆದರೆ (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPin* ಸ್ಥಿರಾಂಕ pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ deparamIfaceCellNow(AstCell* cellp) {
        ಆದರೆ (!cellp->paramsp()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!VN_IS(cellp->modp(), Iface)) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ srcModp = cellp->modp();
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ newModp
            = m_processor.ಗಂಟು_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        ಆದರೆ (newModp && newModp != srcModp) {
            ಆದರೆ (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (nodep->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(ಶೂನ್ಯಸೂಚಕ);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        // Needs relink, as may remove pointed-to item
        ಆದರೆ (nodep->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ()) nodep->itemp(ಶೂನ್ಯಸೂಚಕ);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
        ಆದರೆ (nodep->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(ಶೂನ್ಯಸೂಚಕ);
            ಹಿಂತಿರುಗಿ;
        }
        // Parameters of an interface array are accessed through its first element
        ಆದರೆ (nodep->varp() && nodep->varp()->isParam()) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ifaceInstCells.ಕಂಡುಹಿಡಿ(nodep->dotted());
            ಆದರೆ (it != m_ifaceInstCells.ಕೊನೆ()) nodep->dotted(it->ಎರಡನೇ->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ಸ್ಥಿರಾಂಕ string dotted = nodep->dotted();
        ಆದರೆ (!dotted.ಖಾಲಿಗಳು() && nodep->varp() && nodep->varp()->isParam()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* backp = nodep;
            ಆಗ ((backp = backp->backp())) {
                ಆದರೆ (VN_IS(backp, ಗಂಟು_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    ನಿಲ್ಲಿಸು;
                }
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(backp, Var)) {
                    ಆದರೆ (!varp->isIfaceRef()) ಮುಂದುವರಿ;
                    // Through all dimensions of an array
                    ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    ಆದರೆ (!ifacerefp) ಮುಂದುವರಿ;
                    // Interfaces passed in on the port map have ifaces
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep = ifacerefp->ifacep()) {
                        ಆದರೆ (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            ಆದರೆ (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                ಹಿಂತಿರುಗಿ;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cellp = ifacerefp->cellp()) {
                        ಆದರೆ (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            ಆದರೆ (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                ಹಿಂತಿರುಗಿ;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ifaceCellIt = m_ifaceInstCells.ಕಂಡುಹಿಡಿ(dotted);
            ಆದರೆ (ifaceCellIt != m_ifaceInstCells.ಕೊನೆ()) {
                AstCell* ಸ್ಥಿರಾಂಕ cellp = ifaceCellIt->ಎರಡನೇ;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ಸ್ಥಿರಾಂಕ AstClass* lhsClassp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        ಆದರೆ (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ಗಂಟು* rhsDefp = ಶೂನ್ಯಸೂಚಕ;
        AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        ಆದರೆ (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        ಆದರೆ (lhsClassp && rhsDefp) {
            m_state.m_dots.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate(nodep->rhsp());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstUnlinkedRef* nodep) override {
        AstVarXRef* ಸ್ಥಿರಾಂಕ varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ taskrefp = VN_CAST(nodep->refp(), ಗಂಟು_FTask_Ref);
        Ast_Enum_ಅಂಶ_Ref* ಸ್ಥಿರಾಂಕ enumrefp = VN_CAST(nodep->refp(), Enum_ಅಂಶ_Ref);
        ಆದರೆ (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            ಹಿಂತಿರುಗಿ;
        }
        iterate(nodep->cellrefp());

        ಆದರೆ (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } ಇಲ್ಲದಿದ್ದರೆ {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        ಗಾಗಿ (Ast_ಗಂಟು *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> indices;
        ಗಾಗಿ (Ast_ಗಂಟು* s = nodep->selp(); s; s = s->nextp()) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(s, Const);
            ಆದರೆ (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                ಹಿಂತಿರುಗಿ;
            }
            indices.ತಳ್ಳು_ಹಿಂಭಾಗ(constp->ಗಮ್ಯ_SInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ಸ್ಥಿರಾಂಕ string viftopSuffix = "__Viftop";
        ಸ್ಥಿರಾಂಕ string ತಳ_Name
            = VString::ತುದಿಗಳು_With(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().ಗಾತ್ರ_2() - viftopSuffix.ಗಾತ್ರ_2())
                  : nodep->name();
        ಸ್ಥಿರಾಂಕ string placeholder = "__BRA__??__KET__";
        ಗಾತ್ರ_t pos = m_unlinkedTxt.ಕಂಡುಹಿಡಿ(ತಳ_Name + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        ಆದರೆ (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            ಹಿಂತಿರುಗಿ;
        }
        pos += ತಳ_Name.ಉದ್ದಳತೆ();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ idx : indices) {
            ಸ್ಥಿರಾಂಕ string replacement = "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(idx) + "__KET__";
            ಆದರೆ (m_unlinkedTxt.compare(pos, placeholder.ಉದ್ದಳತೆ(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                ಹಿಂತಿರುಗಿ;
            }
            m_unlinkedTxt.replace(pos, placeholder.ಉದ್ದಳತೆ(), replacement);
            pos += replacement.ಉದ್ದಳತೆ();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    ಶೂನ್ಯ iterateGenerateCond(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_ಒಳ_Generate_Cond);
        m_ಒಳ_Generate_Cond = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3_ಅಗಲ::ಅಗಲ_Generate_Params_Edit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->condp(), Const)) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                nodep->replaceWith(keepp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
            }
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Normal edit rules will now recurse the replacement
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        ಆದರೆ (AstGenFor* ಸ್ಥಿರಾಂಕ forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3_ಅಗಲ::ಅಗಲ_Params_Edit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            ಸ್ಥಿರಾಂಕ string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        ತಾರ್ಕಿಕ ದೊರಕಿತು = ಸುಳ್ಳು;
        Ast_ಗಂಟು* keepp = ಶೂನ್ಯಸೂಚಕ;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        ಗಾಗಿ (Ast_Gen_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ಅಂಶ)) {
            ಗಾಗಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        ಗಾಗಿ (Ast_Gen_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ಅಂಶ)) {
            ಗಾಗಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep;) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        ಗಾಗಿ (Ast_Gen_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ಅಂಶ)) {
            ಆದರೆ (!itemp->isDefault()) {
                ಗಾಗಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ ccondp = VN_CAST(ep, Const)) {
                        V3_ಸಂಖ್ಯೆ ಹೊಂದಿಕೆ{nodep, 1};
                        ಹೊಂದಿಕೆ.opEq(ccondp->ಅಂಕೆ(), exprp->ಅಂಕೆ());
                        ಆದರೆ (!ದೊರಕಿತು && ಹೊಂದಿಕೆ.isNeqZero()) {
                            ದೊರಕಿತು = ನಿಜ;
                            keepp = itemp->itemsp();
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        ಗಾಗಿ (Ast_Gen_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ಅಂಶ)) {
            ಆದರೆ (itemp->isDefault()) {
                ಆದರೆ (!ದೊರಕಿತು) {
                    ದೊರಕಿತು = ನಿಜ;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        ಆದರೆ (keepp) {
            keepp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            nodep->replaceWith(keepp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

ವರ್ಗ Param_ತುದಿ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    ಶೂನ್ಯ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        ಗಾಗಿ (AstDot* ಸ್ಥಿರಾಂಕ dotp : m_state.m_dots) {
            ಸ್ಥಿರಾಂಕ AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                ಆದರೆ (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
    }

    ಶೂನ್ಯ fixLevel(Ast_ಗಂಟು_Module* modp) {
        // Fix up level of module, based on who instantiates it
        ಆದರೆ (modp->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Already fixed
        ಆದರೆ (m_state.m_parentps[modp].ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;  // Leave top levels alone
        ಪೂರ್ಣಾಂಕ max_ಪೋಷಕ_Level = 0;
        ಗಾಗಿ (Ast_ಗಂಟು_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            max_ಪೋಷಕ_Level = std::max(max_ಪೋಷಕ_Level, parentp->level());
        }
        ಆದರೆ (modp->level() <= max_ಪೋಷಕ_Level) modp->level(max_ಪೋಷಕ_Level + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->ಆಳ(modp->level());
    }

    ಶೂನ್ಯ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::ಸದಿಶ<Ast_ಗಂಟು_Module*> modps;
        ಗಾಗಿ (Ast_ಗಂಟು_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ಗಂಟು_Module);
            modp->unlink_Fr_ಹಿಂಭಾಗ();
            modps.ತಳ್ಳು_ಹಿಂಭಾಗ(modp);
        }

        // Fix-up levels
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        ಗಾಗಿ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_ವಿಂಗಡಿಸು(modps.begin(), modps.ಕೊನೆ(),
                         [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ap, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* bp) {
                             ಹಿಂತಿರುಗಿ ap->level() < bp->level();
                         });

        // Re-insert modules
        ಗಾಗಿ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp : modps) netlistp->ಕೂಡಿಸು_Modulesp(modp);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Param_ತುದಿ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ಸ್ಥಿರಾಂಕ ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ಸ್ಥಿರಾಂಕ ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ಗಂಟು_FTask_Ref* ftaskrefp) {
            Ast_ಗಂಟು_FTask* ftaskp = ftaskrefp->taskp();
            ಆದರೆ (!ftaskp || !ftaskp->class_ವಿಧಾನ()) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ancestor_Class_ದ = [](Ast_ಗಂಟು* startp) -> AstClass* {
                ಗಾಗಿ (Ast_ಗಂಟು* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ cp = VN_CAST(np, Class)) ಹಿಂತಿರುಗಿ cp;
                }
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            };
            AstClass* refClassp = ancestor_Class_ದ(ftaskrefp);
            ಆದರೆ (refClassp == ftaskrefp->classOrPackagep())
                ಹಿಂತಿರುಗಿ;  // task is in the same class as reference
            AstClass* classp = ancestor_Class_ದ(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            ಆದರೆ (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (classp->user3p()) ಹಿಂತಿರುಗಿ;  // will not get removed, no need to relink
            AstClass* ಸ್ಥಿರಾಂಕ parametrizedClassp = VN_CAST(classp->user4p(), Class);
            ಆದರೆ (!parametrizedClassp) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು_FTask* newFuncp = ಶೂನ್ಯಸೂಚಕ;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ಗಂಟು_FTask* ftaskp) {
                ಆದರೆ (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಆದರೆ (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ಸ್ಥಿರಾಂಕ ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_ಗಣ<AstClass*> removedClassps;
        ಗಾಗಿ (AstClass* ಸ್ಥಿರಾಂಕ classp : m_state.m_paramClasses) {
            ಆದರೆ (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                ಆದರೆ (V3Error::error_ಎಣಿಕೆ()) ಮುಂದುವರಿ;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(classp->unlink_Fr_ಹಿಂಭಾಗ()), classp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(ಸುಳ್ಳು);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ಗಂಟು::user3_Clear_ವೃಕ್ಷ();
        ಗಾಗಿ (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) { nodep->user3(ನಿಜ); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
            nodep->foreachLink([&](Ast_ಗಂಟು** ಸ್ಥಿರಾಂಕ linkpp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* namep) {
                ಆದರೆ (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = ಶೂನ್ಯಸೂಚಕ;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_ತುದಿ() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(Param_ತುದಿ);
};

//######################################################################
// Param class functions

ಶೂನ್ಯ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    ಆದರೆ (dump_ವೃಕ್ಷ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_ತುದಿ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    ಆದರೆ (dump_ವೃಕ್ಷ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_ವೃಕ್ಷ("param", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : rootp->deferredParamVarps()) {
        ಆದರೆ (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
