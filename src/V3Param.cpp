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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Param.h"

#ضمن "V3Case.h"
#ضمن "V3Const.h"
#ضمن "V3EmitV.h"
#ضمن "V3HierBlock.h"
#ضمن "V3LinkDotIfaceCapture.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Os.h"
#ضمن "V3Parse.h"
#ضمن "V3Simulate.h"
#ضمن "V3Stats.h"
#ضمن "V3Unroll.h"
#ضمن "V3Width.h"

#ضمن <cctype>
#ضمن <cstdlib>
#ضمن <deque>
#ضمن <map>
#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ساكن AstClassRefDType* class_Ref_DType_د_غوټه(Ast_غوټه* nodep) {
    طالما (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_غوټه_DType* ثابت dtp = VN_CAST(nodep, غوټه_DType);
    أرجع dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : تش_اشاره;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

صنف Parameterized_Hier_بلاکونه final {
    کارول Hier_بلاک_Opts_By_Orig_Name = std::multimap<std::string, ثابت V3_پوړیز_بلاک_Option*>;
    کارول HierMapIt = Hier_بلاک_Opts_By_Orig_Name::const_iterator;
    کارول Hier_بلاک_Mod_Map = std::map<ثابت std::string, Ast_غوټه_Module*>;
    کارول ParamConstMap = std::map<ثابت std::string, std::unique_ptr<AstConst>>;
    کارول GParamsMap = std::map<ثابت std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ثابت منطقي m_hier_Sub_پرلپسې;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    Hier_بلاک_Opts_By_Orig_Name m_hier_بلاک_Opts_By_Orig_Name;
    // key:mangled module name, value:AstNodeModule*
    Hier_بلاک_Mod_Map m_hier_بلاک_Mod;
    // Overridden parameters of the hierarchical block
    std::map<ثابت V3_پوړیز_بلاک_Option*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ثابت std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::سېټ<ثابت AstVar*> m_untypedParams;

    // METHODS

عام:
    Parameterized_Hier_بلاکونه(ثابت V3_Hier_بلاک_Opt_سېټ& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hier_Sub_پرلپسې{!v3Global.opt.hier_بلاکونه().تشه() || v3Global.opt.hier_ماشوم()} {
        لكل (ثابت خپلکاری& hierOpt : hierOpts) {
            m_hier_بلاک_Opts_By_Orig_Name.emplace(hierOpt.دوهم.origName(), &hierOpt.دوهم);
            ثابت V3_پوړیز_بلاک_Option::ParamStrMap& params = hierOpt.دوهم.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.دوهم];
            لكل (V3_پوړیز_بلاک_Option::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.پای_ټکی(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    جديد File_کرښه{File_کرښه::built_دننه_Filename()}, pIt->دوهم)};
                UASSERT(constp, pIt->دوهم << " is not a valid parameter literal");
                ثابت منطقي inserted = consts.emplace(pIt->لومړی, std::move(constp)).دوهم;
                UASSERT(inserted, pIt->لومړی << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ورننباسه({hierOpt.دوهم.origName(), {}});
        }
        لكل (Ast_غوټه_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), غوټه_Module)) {
            إذا (hierOpts.پیدا_کړه(modp->prettyName()) != hierOpts.پای_ټکی()) {
                m_hier_بلاک_Mod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ثابت string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ثابت خپلکاری defParamIt = m_modParams.پیدا_کړه(actualModName);
            إذا (defParamIt != m_modParams.پای_ټکی()) {
                // modp is the original of parameterized hierarchical block
                لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                        إذا (varp->isGParam()) defParamIt->دوهم.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ثابت AstBasicDType* ثابت dtypep
                            = VN_CAST(varp->ماشوم_DTypep(), BasicDType);
                        إذا (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    منطقي hier_Sub_پرلپسې() ثابت { أرجع m_hier_Sub_پرلپسې; }
    منطقي is_Hier_بلاک(ثابت string& origName) ثابت {
        أرجع m_hier_بلاک_Opts_By_Orig_Name.پیدا_کړه(origName) != m_hier_بلاک_Opts_By_Orig_Name.پای_ټکی();
    }
    Ast_غوټه_Module* پیدا_کړه_By_Params(ثابت string& origName, AstPin* لومړی_Pinp,
                                ثابت Ast_غوټه_Module* modp) {
        UASSERT(is_Hier_بلاک(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ثابت std::pair<HierMapIt, HierMapIt> candidates
            = m_hier_بلاک_Opts_By_Orig_Name.equal_range(origName);
        ثابت خپلکاری paramsIt = m_modParams.پیدا_کړه(origName);
        UASSERT_OBJ(paramsIt != m_modParams.پای_ټکی(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ثابت AstPin* typeMismatchPinp = تش_اشاره;
        HierMapIt hierIt;
        لكل (hierIt = candidates.لومړی; hierIt != candidates.دوهم; ++hierIt) {
            منطقي وموندل = ریښتیا;
            اندازه_t paramIdx = 0;
            ثابت ParamConstMap& params = m_hierParams[hierIt->دوهم];
            UASSERT(params.اندازه() == hierIt->دوهم->params().اندازه(), "not match");
            لكل (AstPin* pinp = لومړی_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->exprp()) استمر;
                إذا (ثابت AstVar* ثابت modvarp = pinp->modVarp()) {
                    AstConst* ثابت constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ثابت خپلکاری paramIt = paramsIt->دوهم.پیدا_کړه(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->دوهم.پای_ټکی(), modvarp, "must be registered");
                    ثابت منطقي untyped = m_untypedParams.شمېر(paramIt->دوهم);
                    AstConst* ثابت defValuep = VN_CAST(paramIt->دوهم->valuep(), Const);
                    إذا (defValuep && areSame(constp, defValuep)) {
                        إذا (!untyped || constp->same_ارزښت_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            استمر;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ثابت خپلکاری pIt = vlstd::as_const(params).پیدا_کړه(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    إذا (pIt == params.پای_ټکی() || paramIdx >= params.اندازه()
                        || !areSame(constp, pIt->دوهم.get())) {
                        وموندل = خطأ;
                        اكسر;
                    }
                    إذا (untyped && !constp->same_ارزښت_Type(pIt->دوهم.get())) {
                        typeMismatchPinp = pinp;
                        وموندل = خطأ;
                        اكسر;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->دوهم.get());
                    ++paramIdx;
                }
            }
            إذا (وموندل && paramIdx == hierIt->دوهم->params().اندازه()) اكسر;
        }
        إذا (hierIt == candidates.دوهم && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_غوټه::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            أرجع تش_اشاره;
        }
        UASSERT_OBJ(hierIt != candidates.دوهم, لومړی_Pinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ثابت Hier_بلاک_Mod_Map::const_iterator modIt
            = m_hier_بلاک_Mod.پیدا_کړه(hierIt->دوهم->mangledName());
        UASSERT_OBJ(modIt != m_hier_بلاک_Mod.پای_ټکی(), لومړی_Pinp,
                    hierIt->دوهم->mangledName() << " is not found");

        ثابت خپلکاری it = vlstd::as_const(m_hier_بلاک_Mod).پیدا_کړه(hierIt->دوهم->mangledName());
        إذا (it == m_hier_بلاک_Mod.پای_ټکی()) أرجع تش_اشاره;
        أرجع it->دوهم;
    }
    ساكن منطقي areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        إذا (pinValuep->isString()) {
            أرجع pinValuep->عدد().منزل_String() == hierOptParamp->عدد().منزل_String();
        }

        إذا (hierOptParamp->isDouble()) {
            مزدوج var;
            إذا (pinValuep->isDouble()) {
                var = pinValuep->عدد().منزل_Double();
            } وإلا {  // Cast from integer to real
                V3_شمېره var_عدد{pinValuep, V3_شمېره::Double{}, 0.0};
                var_عدد.opIToRD(pinValuep->عدد());
                var = var_عدد.منزل_Double();
            }
            أرجع V3_شمېره::epsilonEqual(var, hierOptParamp->عدد().منزل_Double());
        } وإلا {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_شمېره var_عدد{pinValuep, hierOptParamp->عدد().پلنوالی()};
            إذا (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_عدد.opRToIRoundS(pinValuep->عدد());
            } وإلا إذا (pinValuep->isSigned()) {
                var_عدد.opExtendS(pinValuep->عدد(), pinValuep->عدد().پلنوالی());
            } وإلا {
                var_عدد.op_ټاکل(pinValuep->عدد());
            }
            V3_شمېره isEq{pinValuep, 1};
            isEq.opEq(var_عدد, hierOptParamp->عدد());
            أرجع isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

صنف ParamProcessor final {
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
    ثابت VNUser2_دننه_Use m_inuser2;
    ثابت VNUser3_دننه_Use m_inuser3;
    ثابت VNUser4_دننه_Use m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    کارول IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    کارول CloneMap = std::unordered_map<ثابت Ast_غوټه*, Ast_غوټه*>;
    بنية ModInfo final {
        Ast_غوټه_Module* ثابت m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        څرګند ModInfo(Ast_غوټه_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ثابت std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ثابت std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::سېټ<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VString_سېټ m_all_Module_نومونه;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ثابت std::string, std::string> m_ارزښت_نومونه;  // Parameter value text to its name
    std::سېټ<std::string> m_ارزښت_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ساكن constexpr std::string::اندازه_type DIGEST_DIGITS = 8;

    ثابت Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    Parameterized_Hier_بلاکونه m_hier_بلاکونه;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    کارول Default_ارزښت_Map = std::map<std::string, Ast_غوټه*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_غوټه_Module*, Default_ارزښت_Map> m_default_Parameter_ارزښتونه;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ثابت Ast_غوټه_Module*, ثابت AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::ویکټور<std::pair<AstParamTypeDType*, صحيح>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, صحيح> m_param_شاخص;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_سېټ<ثابت AstClass*> m_default_Clone_دننه_پرمختګ;

    std::ویکټور<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ساكن فراغ make_Small_نومونه(Ast_غوټه_Module* modp) {
        std::ویکټور<صحيح> کارول_شوی_Letter;
        کارول_شوی_Letter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isGParam() || varp->isIfaceRef()) {
                    حرف ch = varp->name()[0];
                    ch = std::لوی_توری(ch);
                    إذا (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(کارول_شوی_Letter[static_cast<صحيح>(ch)] * 256 + ch);
                    کارول_شوی_Letter[static_cast<صحيح>(ch)]++;
                }
            } وإلا إذا (AstParamTypeDType* ثابت typep = VN_CAST(stmtp, ParamTypeDType)) {
                ثابت حرف ch = 'T';
                typep->user3(کارول_شوی_Letter[static_cast<صحيح>(ch)] * 256 + ch);
                کارول_شوی_Letter[static_cast<صحيح>(ch)]++;
            }
        }
    }
    ساكن string paramSmallName(Ast_غوټه_Module* modp, Ast_غوټه* varp) {
        إذا (varp->user3() <= 1) make_Small_نومونه(modp);
        صحيح شاخص = varp->user3() / 256;
        ثابت حرف ch = varp->user3() & 255;
        string st = cvt_منزل_Str(ch);
        طالما (شاخص) {
            st += cvt_منزل_Str(static_cast<حرف>((شاخص % 25) + 'A'));
            شاخص /= 26;
        }
        أرجع st;
    }

    ساكن منطقي is_Aggregate_Param_ارزښت(ثابت Ast_غوټه* nodep) {
        أرجع VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ساكن string param_ارزښت_String(ثابت Ast_غوټه* nodep) {
        إذا (ثابت AstRefDType* ثابت refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skip_Ref_منزل_Non_Refp();
        }
        string کلی = nodep->name();
        إذا (ثابت AstIfaceRefDType* ثابت ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            إذا (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                کلی = ifrtp->cellp()->modp()->name();
            } وإلا إذا (ifrtp->ifacep()) {
                کلی = ifrtp->ifacep()->name();
            } وإلا {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } وإلا إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت dtypep
                   = VN_CAST(nodep, غوټه_UOr_Struct_DType)) {
            کلی += " ";
            کلی += dtypep->verilogKwd();
            کلی += " {";
            لكل (ثابت Ast_غوټه* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                کلی += param_ارزښت_String(memberp);
                کلی += ";";
            }
            کلی += "}";
        } وإلا إذا (ثابت AstMemberDType* ثابت dtypep = VN_CAST(nodep, MemberDType)) {
            کلی += " ";
            کلی += param_ارزښت_String(dtypep->subDTypep());
        } وإلا إذا (ثابت AstBasicDType* ثابت dtypep = VN_CAST(nodep, BasicDType)) {
            إذا (dtypep->isSigned()) کلی += " signed";
            إذا (dtypep->isRanged()) {
                کلی += "[" + cvt_منزل_Str(dtypep->کیڼ()) + ":" + cvt_منزل_Str(dtypep->ښی()) + "]";
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت dtypep = VN_CAST(nodep, PackArrayDType)) {
            کلی += "[";
            کلی += cvt_منزل_Str(dtypep->کیڼ());
            کلی += ":";
            کلی += cvt_منزل_Str(dtypep->ښی());
            کلی += "] ";
            کلی += param_ارزښت_String(dtypep->subDTypep());
        } وإلا إذا (ثابت Ast_نمونه* ثابت patp = VN_CAST(nodep, نمونه)) {
            // A parameter override pattern, keyed before the specialized module types it
            کلی += "'{";
            لكل (ثابت AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                إذا (memp->isDefault()) کلی += "default";
                إذا (ثابت Ast_متن* ثابت textp = VN_CAST(memp->keyp(), متن_2)) {
                    کلی += textp->متن();
                } وإلا إذا (memp->keyp()) {
                    کلی += param_ارزښت_String(memp->keyp());
                }
                کلی += ":";
                إذا (memp->repp()) کلی += param_ارزښت_String(memp->repp()) + "x";
                لكل (ثابت Ast_غوټه_بیان* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), غوټه_بیان)) {
                    کلی += param_ارزښت_String(valuep) + ";";
                }
                کلی += ",";
            }
            کلی += "}";
        } وإلا إذا (ثابت AstConsArray* ثابت consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            کلی += "{";
            لكل (ثابت Ast_Cons_Array_توکی* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Array_توکی)) {
                کلی += cvt_منزل_Str(itemp->شاخص()) + ":" + param_ارزښت_String(itemp->valuep()) + ",";
            }
            إذا (consArrayp->defaultp()) {
                کلی += "default:" + param_ارزښت_String(consArrayp->defaultp()) + ",";
            }
            کلی += "}";
        } وإلا إذا (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep, ConsStruct)) {
            کلی += "{";
            لكل (ثابت Ast_Cons_Struct_توکی* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_توکی)) {
                کلی += param_ارزښت_String(itemp->valuep());
                کلی += ",";
            }
            کلی += "}";
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            إذا (classRefp->classp()) {
                ثابت string& className = classRefp->classp()->name();
                ثابت string& origName = classRefp->classp()->origName();
                ثابت منطقي isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                إذا (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } وإلا إذا (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    کلی = className;
                } وإلا {
                    // Unspecialized class with no params - use origName
                    کلی = origName;
                }
            } وإلا {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            کلی = paramElemString(dtypep->subDTypep()) + "$" + cvt_منزل_Str(dtypep->declRange());
        } وإلا إذا (ثابت Ast_کتار_DType* ثابت dtypep = VN_CAST(nodep, کتار_DType)) {
            کلی = paramElemString(dtypep->subDTypep()) + "$[$";
            إذا (dtypep->boundConst()) کلی += ":" + cvt_منزل_Str(dtypep->boundConst());
            کلی += "]";
        } وإلا إذا (ثابت AstDynArrayDType* ثابت dtypep = VN_CAST(nodep, DynArrayDType)) {
            کلی = paramElemString(dtypep->subDTypep()) + "$[]";
        } وإلا إذا (ثابت AstAssocArrayDType* ثابت dtypep = VN_CAST(nodep, AssocArrayDType)) {
            کلی = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->کلی_DTypep()) + "]";
        } وإلا إذا (ثابت AstWildcardArrayDType* ثابت dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            کلی = paramElemString(dtypep->subDTypep()) + "$[*]";
        } وإلا إذا (ثابت AstEnumDType* ثابت dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            کلی += " enum {";
            لكل (ثابت Ast_Enum_توکی* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
                ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const);
                کلی += itemp->name() + "=" + (constp ? constp->عدد().ascii() : "?") + ";";
            }
            کلی += "}";
        } وإلا إذا (ثابت Ast_غوټه_DType* ثابت dtypep = VN_CAST(nodep, غوټه_DType)) {
            کلی += dtypep->prettyDTypeName(ریښتیا);
        }
        UASSERT_OBJ(!کلی.تشه(), nodep, "Parameter yielded no value string");
        أرجع کلی;
    }
    // As paramValueString(), for the element or key type of a container
    ساكن string paramElemString(ثابت Ast_غوټه_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ثابت AstClassRefDType* ثابت classRefp
            = VN_CAST(dtypep->skip_Ref_منزل_Non_Refp(), ClassRefDType);
        إذا (classRefp && classRefp->paramsp()) أرجع classRefp->prettyDTypeName(ریښتیا);
        أرجع param_ارزښت_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ساكن string digestSuffix(ثابت string& متن, std::سېټ<string>& usedr) {
        ثابت string hex = VHashSha512{متن}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::اندازه_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        طالما (digits < hex.اندازه() && !usedr.ورننباسه(hex.substr(0, digits)).دوهم) ++digits;
        أرجع hex.substr(0, digits);
    }
    string param_ارزښت_شمېره(Ast_غوټه* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        إذا (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ثابت string متن = param_ارزښت_String(nodep);
        ثابت خپلکاری pair = m_ارزښت_نومونه.emplace(متن, "");
        إذا (pair.دوهم) pair.لومړی->دوهم = "z" + digestSuffix(متن, m_ارزښت_Suffixes);
        أرجع pair.لومړی->دوهم;
    }
    string moduleCalcName(ثابت Ast_غوټه_Module* srcModp, ثابت string& longname) {
        string newname = longname;
        إذا (longname.اوږدتیا() > 30) {
            ثابت خپلکاری pair = m_longMap.emplace(longname, "");
            إذا (pair.دوهم) {
                // We use all upper case above, so lower here can't conflict
                pair.لومړی->دوهم
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.لومړی->دوهم;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        أرجع newname;
    }
    ساكن Ast_غوټه_DType* arraySubDTypep(Ast_غوټه_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            أرجع adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        إذا (ثابت AstBracketArrayDType* ثابت adtypep = VN_CAST(nodep, BracketArrayDType)) {
            أرجع adtypep->subDTypep();
        }
        أرجع تش_اشاره;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ساكن Ast_غوټه_DType* arraySubDTypeDeepp(Ast_غوټه_DType* nodep) {
        طالما (Ast_غوټه_DType* ثابت subp = arraySubDTypep(nodep)) nodep = subp;
        أرجع nodep;
    }
    ساكن منطقي isString(Ast_غوټه_DType* nodep) {
        إذا (AstBasicDType* ثابت basicp = VN_CAST(nodep->skip_Ref_منزل_Non_Refp(), BasicDType))
            أرجع basicp->isString();
        أرجع خطأ;
    }
    فراغ collectPins(CloneMap* clonemapp, Ast_غوټه_Module* modp, منطقي originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت Ast_غوټه* originalParamp = تش_اشاره;
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } وإلا إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            إذا (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            إذا (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    فراغ relinkPins(ثابت CloneMap* clonemapp, AstPin* startpinp) {
        لكل (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ثابت خپلکاری cloneiter = clonemapp->پیدا_کړه(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->پای_ټکی(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->دوهم, Var));
            } وإلا إذا (pinp->modPTypep()) {
                ثابت خپلکاری cloneiter = clonemapp->پیدا_کړه(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->پای_ټکی(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->دوهم, ParamTypeDType));
            } وإلا {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    فراغ relinkPinsByName(AstPin* startpinp, Ast_غوټه_Module* modp) {
        std::map<ثابت string, AstVar*> name_منزل_Pin;
        لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    name_منزل_Pin.emplace(varp->name(), varp);
                }
            }
        }
        لكل (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                ثابت خپلکاری varIt = vlstd::as_const(name_منزل_Pin).پیدا_کړه(varp->name());
                UASSERT_OBJ(varIt != name_منزل_Pin.پای_ټکی(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->دوهم);
            }
        }
    }
    ساكن منطقي hasDescendantDefparams(ثابت Ast_غوټه_Module* modp,
                                       std::سېټ<ثابت Ast_غوټه_Module*>& لیدل_شوی) {
        إذا (!لیدل_شوی.ورننباسه(modp).دوهم) أرجع خطأ;
        لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت cellp = VN_CAST(stmtp, Cell);
            إذا (!cellp) استمر;
            لكل (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->param_لار().تشه()) {
                    لیدل_شوی.erase(modp);
                    أرجع ریښتیا;
                }
            }
            إذا (Ast_غوټه_Module* ثابت ماشوم_Modp = cellp->modp()) {
                إذا (hasDescendantDefparams(ماشوم_Modp, لیدل_شوی)) {
                    لیدل_شوی.erase(modp);
                    أرجع ریښتیا;
                }
            }
        }
        لیدل_شوی.erase(modp);
        أرجع خطأ;
    }
    ساكن منطقي hasDescendantDefparams(ثابت Ast_غوټه_Module* modp) {
        std::سېټ<ثابت Ast_غوټه_Module*> لیدل_شوی;
        أرجع hasDescendantDefparams(modp, لیدل_شوی);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ثابت AstPin* nestedDefparamp(ثابت Ast_غوټه_Module* modp) {
        ثابت خپلکاری pair = m_nestedDefparamps.emplace(modp, تش_اشاره);
        إذا (!pair.دوهم) أرجع pair.لومړی->دوهم;  // Searched, or being searched
        ثابت AstPin* foundp = تش_اشاره;
        modp->exists([&](ثابت AstCell* cellp) {
            لكل (ثابت AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->param_لار().تشه()) foundp = pinp;
            }
            إذا (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            أرجع foundp != تش_اشاره;
        });
        pair.لومړی->دوهم = foundp;
        أرجع foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    فراغ checkSupportedParam(Ast_غوټه_Module* modp, AstPin* pinp) ثابت {
        // ConsArray is not supported because that can not be set via -G
        // option.
        إذا (pinp->modVarp()) {
            منطقي supported = خطأ;
            إذا (ثابت AstConst* ثابت constp = VN_CAST(pinp->exprp(), Const)) {
                supported = ریښتیا;
                إذا (constp->عدد().isString()
                    && !V3_Hier_بلاک::stringParamPassable(constp->عدد().منزل_String())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            إذا (!supported) {
                pinp->v3error(
                    Ast_غوټه::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } وإلا إذا (ثابت AstParamTypeDType* ثابت typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_غوټه::prettyNameQ(modp->origName()));
        }
    }
    منطقي moduleExists(ثابت string& modName) ثابت {
        إذا (m_all_Module_نومونه.پیدا_کړه(modName) != m_all_Module_نومونه.پای_ټکی()) أرجع ریښتیا;
        إذا (m_modNameMap.پیدا_کړه(modName) != m_modNameMap.پای_ټکی()) أرجع ریښتیا;
        أرجع خطأ;
    }

    string parameterized_Hier_بلاک_Name(Ast_غوټه_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hier_بلاک(), modp, "should be used for hierarchical block");

        std::map<string, Ast_غوټه*> pins;

        AstPin* pinp = paramPinsp;
        طالما (pinp) {
            checkSupportedParam(modp, pinp);
            إذا (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                إذا (!pinp->exprp()) استمر;
                إذا (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } وإلا إذا (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), غوټه_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ثابت خپلکاری pair = m_default_Parameter_ارزښتونه.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        إذا (pair.دوهم) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            إذا (ثابت AstPin* ثابت defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_غوټه::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_ارزښت_Map params;
            لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isGParam()) {
                        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } وإلا إذا (AstParamTypeDType* ثابت p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_غوټه* ثابت dtypep = static_cast<Ast_غوټه*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.لومړی->دوهم = std::move(params);
        }
        ثابت خپلکاری paramsIt = pair.لومړی;
        إذا (paramsIt->دوهم.تشه()) أرجع modp->origName();  // modp has no parameter

        string longname = modp->origName();
        لكل (خپلکاری&& default_ارزښت : paramsIt->دوهم) {
            ثابت خپلکاری pinIt = pins.پیدا_کړه(default_ارزښت.لومړی);
            // If the pin does not have a value assigned, use the default one.
            ثابت Ast_غوټه* ثابت nodep = pinIt == pins.پای_ټکی() ? default_ارزښت.دوهم : pinIt->دوهم;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_ارزښت.لومړی + "=";
            // constp can be nullptr

            إذا (ثابت AstConst* ثابت p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->عدد().ascii(خطأ);
            } وإلا إذا (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_ونه(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ثابت خپلکاری iter = m_longMap.پیدا_کړه(longname);
        إذا (iter != m_longMap.پای_ټکی()) أرجع iter->دوهم;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ورننباسه(longname);
        طالما (ریښتیا) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ثابت string hashStr = hashStrGen.digestHex();
            لكل (string::اندازه_type i = 1; i < hashStr.اندازه(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                إذا (newName.at(newName.اندازه() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                إذا (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    أرجع newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ورننباسه(V3Os::trueRandom(64));
        }
    }
    فراغ replaceRefsRecurse(Ast_غوټه* ثابت nodep, ثابت AstClass* ثابت oldClassp,
                            AstClass* ثابت newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        إذا (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            إذا (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } وإلا إذا (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            إذا (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            إذا (typedefp->subDTypep()) {
                إذا (AstClassRefDType* ثابت classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    إذا (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } وإلا إذا (Ast_غوټه_FTask_Ref* ثابت ftaskRefp = VN_CAST(nodep, غوټه_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            إذا (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            إذا (Ast_غوټه_FTask* ثابت oldTaskp = ftaskRefp->taskp()) {
                إذا (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        إذا (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        إذا (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        إذا (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        إذا (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        إذا (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    صنف VarXRefRelinkVisitor final : عام VNVisitor {
        Ast_غوټه_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_غوټه_Module*> m_varModuleMap;  // Cache var->module lookups

    عام:
        څرګند VarXRefRelinkVisitor(Ast_غوټه_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    خاص:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_غوټه_Module* پیدا_کړه_Var_Module(AstVar* varp) {
            ثابت خپلکاری it = m_varModuleMap.پیدا_کړه(varp);
            إذا (it != m_varModuleMap.پای_ټکی()) أرجع it->دوهم;
            Ast_غوټه_Module* varModp = تش_اشاره;
            لكل (Ast_غوټه* np = varp; np; np = np->backp()) {
                إذا (Ast_غوټه_Module* ثابت modp = VN_CAST(np, غوټه_Module)) {
                    varModp = modp;
                    اكسر;
                }
            }
            m_varModuleMap[varp] = varModp;
            أرجع varModp;
        }

        فراغ لیدنه(AstVarXRef* nodep) override {
            AstVar* ثابت varp = nodep->varp();
            إذا (!varp) {
                iterateChildren(nodep);
                أرجع;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ثابت string& dotted = nodep->dotted();
            إذا (dotted.تشه()) {
                iterateChildren(nodep);
                أرجع;
            }

            ثابت اندازه_t dotPos = dotted.پیدا_کړه('.');
            ثابت string پورټ_Name = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            إذا (پورټ_Name.تشه()) {
                iterateChildren(nodep);
                أرجع;
            }

            // Find the interface port variable in the cloned module
            AstVar* پورټ_Varp = تش_اشاره;
            لكل (Ast_غوټه* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varChkp = VN_CAST(stmtp, Var)) {
                    إذا (varChkp->name() == پورټ_Name && varChkp->isIfaceRef()) {
                        پورټ_Varp = varChkp;
                        اكسر;
                    }
                }
            }
            إذا (!پورټ_Varp) {
                iterateChildren(nodep);
                أرجع;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ثابت irefp = VN_CAST(پورټ_Varp->subDTypep(), IfaceRefDType);
            إذا (!irefp) {
                iterateChildren(nodep);
                أرجع;
            }

            Ast_غوټه_Module* ثابت newIfacep = irefp->iface_له_لارې_Cellp();
            إذا (!newIfacep) {
                iterateChildren(nodep);
                أرجع;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_غوټه_Module* ثابت varModp = پیدا_کړه_Var_Module(varp);

            // If variable is in a different module than the port's interface, remap it
            إذا (varModp && varModp != newIfacep) {
                لكل (Ast_غوټه* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت newVarp = VN_CAST(stmtp, Var)) {
                        إذا (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            اكسر;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    فراغ fixupCrossModuleRefDTypes(Ast_غوټه_Module* newModp) {
        إذا (!V3LinkDotIfaceCapture::enabled()) أرجع;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        صحيح fixed_شمېر = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ثابت VIface_Capture_نښان* ثابت tagp = V3LinkDotIfaceCapture::capture_نښان(refp);
            إذا (!tagp) أرجع;

            Ast_غوټه_Module* ثابت correctModp
                = V3LinkDotIfaceCapture::follow_Cell_لار(newModp, tagp->m_cell_لار);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cell_لار << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            إذا (!correctModp || correctModp->dead()) أرجع;
            إذا (correctModp->parameterizedTemplate()) أرجع;

            منطقي fixed = خطأ;
            إذا (refp->typedefp()) {
                إذا (AstTypedef* ثابت newTdp = V3LinkDotIfaceCapture::پیدا_کړه_Typedef_دننه_Module(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    إذا (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = ریښتیا;
                }
            }
            إذا (!fixed && refp->refDTypep()) {
                إذا (Ast_غوټه_DType* ثابت newDtp = V3LinkDotIfaceCapture::پیدا_کړه_DType_دننه_Module(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = ریښتیا;
                }
            }
            إذا (fixed) ++fixed_شمېر;
        });
        V3Stats::جمع_کړه_Stat_مجموعه("IfaceCapture, Ledger fixups in V3Param", fixed_شمېر);
    }

    // Return true on success, false on error
    منطقي deepCloneModule(Ast_غوټه_Module* srcModp, Ast_غوټه* ifErrorp, AstPin* paramsp,
                         ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_غوټه_Module* newModp;
        إذا (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_ونه(خطأ), غوټه_Module);
        } وإلا {
            newModp = srcModp->clone_ونه(خطأ);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        إذا (AstIface* ثابت newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(ریښتیا);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(خطأ);

        newModp->name(newname);
        newModp->user2(خطأ);  // We need to re-recurse this module once changed
        newModp->recursive(خطأ);
        newModp->recursiveClone(خطأ);
        newModp->hasGParam(خطأ);
        // Recursion may need level cleanups
        إذا (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        إذا ((newModp->level() - srcModp->level()) >= (v3Global.opt.module_Recursion_ژوروالی() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.module_Recursion_ژوروالی());
            VL_DO_DANGLING(newModp->delete_ونه(), newModp);
            أرجع خطأ;
        }

        إذا (AstClass* ثابت newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_غوټه* insertp = srcModp;
        طالما (VN_IS(insertp->nextp(), غوټه_Module)
               && VN_AS(insertp->nextp(), غوټه_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->جمع_کړه_بل_دلته(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ثابت خپلکاری iter = m_modNameMap.پیدا_کړه(newname);
        CloneMap* ثابت clonemapp = &(iter->دوهم.m_cloneMap);
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
        منطقي needRelinkByName = خطأ;
        إذا (paramsp) {
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (pinp->modVarp() && clonemapp->پیدا_کړه(pinp->modVarp()) == clonemapp->پای_ټکی()) {
                    needRelinkByName = ریښتیا;
                    اكسر;
                }
            }
        }
        إذا (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } وإلا {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        لكل (خپلکاری it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ثابت AstIfaceRefDType* ثابت پورټ_Irefp = it->لومړی;
            ثابت AstIfaceRefDType* ثابت pinIrefp = it->دوهم;
            AstIfaceRefDType* ثابت cloneIrefp = پورټ_Irefp->clonep();
            UINFO(8, "     IfaceOld " << پورټ_Irefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, پورټ_Irefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->iface_له_لارې_Cellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        إذا (!ifaceRefRefs.تشه()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->exprp()) {
                إذا (AstVar* ثابت modvarp = pinp->modVarp()) {
                    Ast_غوټه* ثابت newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ثابت exprp = VN_CAST(newp, Const);
                    AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                    ثابت منطقي overridden
                        = !(origp && exprp && Parameterized_Hier_بلاکونه::areSame(exprp, origp));
                    // Remove any existing parameter
                    إذا (modvarp->valuep()) modvarp->valuep()->unlink_Fr_شا()->delete_ونه();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_ونه(خطأ));
                    modvarp->overriddenParam(overridden);
                } وإلا إذا (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    Ast_غوټه_DType* ثابت dtypep = VN_AS(pinp->exprp(), غوټه_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    إذا (modptp->ماشوم_DTypep()) modptp->ماشوم_DTypep()->unlink_Fr_شا()->delete_ونه();
                    // Set this parameter to value requested by cell
                    modptp->ماشوم_DTypep(dtypep->clone_ونه(خطأ));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        أرجع ریښتیا;
    }
    ثابت ModInfo* module_پیدا_کړه_Or_Clone(Ast_غوټه_Module* srcModp, Ast_غوټه* ifErrorp, AstPin* paramsp,
                                     ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        خپلکاری it = m_modNameMap.پیدا_کړه(newname);
        إذا (it != m_modNameMap.پای_ټکی()) {
            UINFO(4, "     De-parameterize to prev: " << it->دوهم.m_modp);
        } وإلا {
            إذا (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                أرجع تش_اشاره;
            }
            it = m_modNameMap.پیدا_کړه(newname);
            UASSERT(it != m_modNameMap.پای_ټکی(), "should find just-made module");
        }
        ثابت ModInfo* ثابت modInfop = &(it->دوهم);
        أرجع modInfop;
    }

    فراغ convert_منزل_Stringp(Ast_غوټه* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        إذا (constp && !constp->عدد().isString()) {
            constp->replaceWith(
                جديد AstConst{constp->fileline(), AstConst::String{}, constp->عدد().منزل_String()});
            VL_DO_DANGLING(constp->delete_ونه(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    فراغ constify_Member_ارزښت(AstVar* varp, std::سېټ<AstVar*>& دننه_پرمختګ) {
        إذا (!varp->valuep() || VN_IS(varp->valuep(), Const)) أرجع;
        إذا (!دننه_پرمختګ.ورننباسه(varp).دوهم) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlink_Fr_شا()->delete_ونه();
            varp->valuep(جديد AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            أرجع;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::سېټ<AstVar*> siblings;
        varp->valuep()->foreach([&](ثابت AstVarRef* refp) {
            AstVar* ثابت refVarp = refp->varp();
            إذا (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ورننباسه(refVarp);
            }
        });
        لكل (AstVar* ثابت sibp : siblings) constify_Member_ارزښت(sibp, دننه_پرمختګ);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        دننه_پرمختګ.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ساكن AstParseRef* memberParseRef(Ast_غوټه* nodep) {
        إذا (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) أرجع refp;
        Ast_غوټه_Pre_Sel* ثابت selp = VN_CAST(nodep, غوټه_Pre_Sel);
        أرجع selp ? VN_CAST(selp->fromp(), ParseRef) : تش_اشاره;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ساكن فراغ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_غوټه_بیان* valuep) {
        إذا (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } وإلا {
            Ast_غوټه_Pre_Sel* ثابت selp = VN_AS(dotp->rhsp(), غوټه_Pre_Sel);
            إذا (Ast_Attr_د* ثابت attrp = selp->attrp()) {
                إذا (Ast_غوټه* ثابت oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_ونه(خطأ));
                    VL_DO_DANGLING(oldFromp->delete_ونه(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_ونه(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlink_Fr_شا());
        }
        VL_DO_DANGLING(dotp->delete_ونه(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ساكن منطقي is_Deferred_Member_بنسټ(ثابت Ast_غوټه_بیان* nodep) {
        إذا (VN_IS(nodep, MemberSel)) أرجع ریښتیا;
        إذا (ثابت Ast_غوټه_Pre_Sel* ثابت selp = VN_CAST(nodep, غوټه_Pre_Sel)) {
            ثابت Ast_غوټه_DType* ثابت fromDTypep = selp->fromp()->dtypep();
            ثابت Ast_غوټه_Array_DType* ثابت arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), غوټه_Array_DType) : تش_اشاره;
            إذا (arrayDTypep) {
                ثابت Ast_غوټه_DType* ثابت elemDTypep = arrayDTypep->subDTypep();
                أرجع elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), غوټه_UOr_Struct_DType);
            }
            أرجع is_Deferred_Member_بنسټ(selp->fromp());
        }
        ثابت Ast_غوټه_DType* ثابت dtypep = nodep->dtypep();
        أرجع dtypep && VN_IS(dtypep->skipRefOrNullp(), غوټه_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ساكن فراغ resolveMemberDot(AstDot* dotp) {
        Ast_غوټه_بیان* ثابت lhsp = VN_CAST(dotp->lhsp(), غوټه_بیان);
        AstParseRef* ثابت memberRefp = memberParseRef(dotp->rhsp());
        إذا (!lhsp || !memberRefp || !is_Deferred_Member_بنسټ(lhsp)) أرجع;
        AstMemberSel* ثابت newp = جديد AstMemberSel{memberRefp->fileline(), lhsp->unlink_Fr_شا(),
                                                    VFlag_ماشوم_DType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    فراغ resolve_Dot_منزل_Typedef(Ast_غوټه* exprp) {
        AstDot* ثابت dotp = VN_CAST(exprp, Dot);
        إذا (!dotp) أرجع;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ثابت classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        إذا (!classRefp) {
            resolveMemberDot(dotp);
            أرجع;
        }
        AstParseRef* ثابت parseRefp = memberParseRef(dotp->rhsp());
        إذا (!parseRefp) أرجع;

        ثابت AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ثابت paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        إذا (paramTypep) {
            Ast_غوټه_DType* ثابت dtypep = paramTypep->subDTypep();
            AstClassRefDType* ثابت classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : تش_اشاره;
            إذا (classRefDTypep) {
                AstClass* ثابت srcClassp = classRefDTypep->classp();
                إذا (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    إذا (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        إذا (classRefp->paramsp()) {
            AstClass* ثابت srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            إذا (srcClassp && srcClassp->hasGParam()) {
                إذا (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        إذا (AstClassRefDType* ثابت crp = class_Ref_DType_د_غوټه(classRefp->classOrPackageNodep())) {
            إذا (AstClass* ثابت srcClassp = crp->classp()) {
                إذا (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    إذا (AstClass* ثابت newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        إذا (!lhsClassp) أرجع;

        Ast_غوټه* ثابت memberp = m_memberMap.پیدا_کړه_Member(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        إذا (AstTypedef* ثابت tdefp = VN_CAST(memberp, Typedef)) {
            إذا (parseRefp != dotp->rhsp()) أرجع;
            AstRefDType* ثابت refp = جديد AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_ونه(), dotp);
        } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    فراغ substituteParamMember(AstDot* ثابت dotp, AstParseRef* ثابت memberRefp,
                               AstVar* ثابت varp) {
        إذا (!varp->isParam() || !varp->valuep()) أرجع;
        إذا (!VN_IS(varp->valuep(), Const)) {
            std::سېټ<AstVar*> دننه_پرمختګ;
            constify_Member_ارزښت(varp, دننه_پرمختګ);
        }
        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
        إذا (!constp) أرجع;
        AstConst* ثابت newp = constp->clone_ونه(خطأ);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    فراغ resolveParamClassRefDType(AstRefDType* refp) {
        إذا (refp->typedefp() || refp->refDTypep()) أرجع;

        AstClassOrPackageRef* ثابت classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        إذا (!classRefp) أرجع;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        إذا (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            أرجع;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_غوټه* targetp = classRefp->classOrPackageNodep();
        طالما (ثابت AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        إذا (Ast_غوټه_DType* ثابت dtargetp = VN_CAST(targetp, غوټه_DType))
            targetp = dtargetp->skipRefOrNullp();
        إذا (!targetp) أرجع;
        AstClassRefDType* ثابت classRefDTypep = VN_CAST(targetp, ClassRefDType);
        إذا (!classRefDTypep) أرجع;
        srcClassp = classRefDTypep->classp();
        إذا (!srcClassp) أرجع;

        AstClass* newClassp = srcClassp;
        إذا (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ثابت typedefp
            = VN_CAST(m_memberMap.پیدا_کړه_Member(newClassp, refp->name()), Typedef);
        إذا (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ساكن منطقي defaultParamsHaveTypeQueryOnParamType(ثابت AstClassRefDType* nodep) {
        منطقي وموندل = خطأ;
        nodep->foreach([&](ثابت Ast_Attr_د* attrp) {
            إذا (وموندل || !attrp->attrType().isTypeQuery()) أرجع;
            ثابت AstRefDType* ثابت refp = VN_CAST(attrp->fromp(), RefDType);
            إذا (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) وموندل = ریښتیا;
        });
        أرجع وموندل;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    منطقي classTypeMatchesDefaultClone(ثابت Ast_غوټه_DType* exprp, ثابت Ast_غوټه_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ثابت خپلکاری* ثابت بیان_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ثابت خپلکاری* ثابت origClassRefp = VN_CAST(origp, ClassRefDType);
        إذا (!بیان_Class_Refp || !origClassRefp) أرجع خطأ;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ثابت Ast_غوټه_Module* ثابت defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        إذا (defaultClonep && defaultClonep == بیان_Class_Refp->classp()) أرجع ریښتیا;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        إذا (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) أرجع خطأ;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        إذا (!origClassRefp->classp()->hasGParam()) أرجع خطأ;
        إذا (origClassRefp->classp()->origName() != بیان_Class_Refp->classp()->origName())
            أرجع خطأ;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        إذا (!m_default_Clone_دننه_پرمختګ.ورننباسه(origClassRefp->classp()).دوهم) أرجع خطأ;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ثابت origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_ونه(خطأ));
        Ast_غوټه_Module* ثابت resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ثابت منطقي سمون = resolvedModp && VN_CAST(resolvedModp, Class) == بیان_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_ونه(), origClonep);
        m_default_Clone_دننه_پرمختګ.erase(origClassRefp->classp());
        أرجع سمون;
    }

    ساكن منطقي param_Consts_Equal_At_Max_پلنوالی(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        إذا (exprp->عدد().پلنوالی() == origp->عدد().پلنوالی()) أرجع خطأ;
        إذا (exprp->عدد().isOpaque()) أرجع خطأ;
        إذا (origp->عدد().isOpaque()) أرجع خطأ;
        ثابت صحيح max_پلنوالی = std::max(exprp->عدد().پلنوالی(), origp->عدد().پلنوالی());
        V3_شمېره بیان_عدد{exprp, max_پلنوالی};
        إذا (exprp->عدد().isSigned()) {
            بیان_عدد.opExtendS(exprp->عدد(), exprp->عدد().پلنوالی());
        } وإلا {
            بیان_عدد.op_ټاکل(exprp->عدد());
        }
        V3_شمېره orig_عدد{origp, max_پلنوالی};
        إذا (origp->عدد().isSigned()) {
            orig_عدد.opExtendS(origp->عدد(), origp->عدد().پلنوالی());
        } وإلا {
            orig_عدد.op_ټاکل(origp->عدد());
        }
        V3_شمېره isEq{exprp, 1};
        isEq.opEq(بیان_عدد, orig_عدد);
        أرجع isEq.isNeqZero();
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
    فراغ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (!pinp->exprp()) استمر;
            إذا (!VString::startsWith(pinp->name(), "__VGIfaceParam")) استمر;
            ثابت Ast_غوټه_Var_Ref* ثابت fwdRefp = VN_CAST(pinp->exprp(), غوټه_Var_Ref);
            إذا (!fwdRefp) استمر;
            ثابت AstIfaceRefDType* ثابت resolvedp
                = VN_CAST(fwdRefp->varp()->ماشوم_DTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ثابت newIrefp = جديد AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            إذا (resolvedp->paramsp()) {
                newIrefp->جمع_کړه_Paramsp(resolvedp->paramsp()->clone_ونه(ریښتیا));
            }
            pinp->exprp()->unlink_Fr_شا()->delete_ونه();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    فراغ name_By_Pin_ارزښت(AstPin* pinp, Ast_غوټه_Module* srcModp, AstVar* modvarp, string& longnamer,
                        منطقي& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_ارزښت_شمېره(pinp->exprp());
        any_overridesr = ریښتیا;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    فراغ cellPinCleanup(Ast_غوټه* nodep, AstPin* pinp, AstPin* paramsp, Ast_غوټه_Module* srcModp,
                        string& longnamer, منطقي& any_overridesr,
                        std::unordered_سېټ<ثابت AstPin*>& overridingTypePinsr) {
        إذا (!pinp->exprp()) أرجع;  // No-connect
        إذا (AstVar* ثابت modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            إذا (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وإلا إذا (VN_IS(pinp->exprp(), نمونه)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_ارزښت(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } وإلا {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                إذا (is_Aggregate_Param_ارزښت(pinp->exprp())) {
                    name_By_Pin_ارزښت(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    أرجع;
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
                إذا (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_اندازه))) {
                    ثابت منطقي dependent = modvarp->valuep()->exists(
                        [](Ast_غوټه* np) { أرجع VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    إذا (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                إذا (isString(modvarp->subDTypep())) {
                    convert_منزل_Stringp(pinp->exprp());
                    convert_منزل_Stringp(modvarp->valuep());
                }
                AstConst* ثابت exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = تش_اشاره;
                إذا (exprp && !exprp->عدد().isDouble() && !exprp->عدد().isString()) {
                    AstVar* cloneVarp = modvarp->clone_ونه(خطأ);
                    منطقي cloneVarpUnresolved = خطأ;
                    إذا (Ast_غوټه* ثابت oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlink_Fr_شا();
                        VL_DO_DANGLING(oldValuep->delete_ونه(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_ونه(خطأ));
                    إذا (Ast_غوټه_DType* ثابت origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        إذا (cloneVarp->ماشوم_DTypep())
                            cloneVarp->ماشوم_DTypep()->unlink_Fr_شا()->delete_ونه();
                        cloneVarp->ماشوم_DTypep(origDTypep->clone_ونه(خطأ));
                        cloneVarp->dtypep(تش_اشاره);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr صحيح maxSubstIters = 1000;
                        لكل (صحيح it = 0; it < maxSubstIters; ++it) {
                            منطقي any = خطأ;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ثابت targetp = varrefp->varp();
                                Ast_غوټه* replacep = تش_اشاره;
                                لكل (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    إذا (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        إذا (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), نمونه)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_ونه(خطأ);
                                        }
                                        اكسر;
                                    }
                                }
                                إذا (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_ونه(خطأ);
                                }
                                // An inlined pattern takes the type of its parameter
                                Ast_نمونه* ثابت patp = VN_CAST(replacep, نمونه);
                                إذا (patp && !patp->ماشوم_DTypep() && targetp->ماشوم_DTypep()) {
                                    patp->ماشوم_DTypep(targetp->ماشوم_DTypep()->clone_ونه(خطأ));
                                }
                                إذا (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_ونه(), varrefp);
                                    any = ریښتیا;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::ویکټور<std::pair<AstRefDType*, Ast_غوټه_DType*>> منزل_Replace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ثابت ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                إذا (!ptdp) أرجع;
                                AstPin* overridePinp = تش_اشاره;
                                لكل (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    إذا (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        اكسر;
                                    }
                                }
                                Ast_غوټه_DType* ثابت substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), غوټه_DType)
                                                   : ptdp->subDTypep();
                                إذا (substp) منزل_Replace.emplace_شا(refp, substp);
                            });
                            لكل (خپلکاری it = منزل_Replace.rbegin(); it != منزل_Replace.rend(); ++it) {
                                AstRefDType* ثابت refp = it->لومړی;
                                refp->replaceWith(it->دوهم->clone_ونه(خطأ));
                                VL_DO_DANGLING(refp->delete_ونه(), refp);
                                any = ریښتیا;
                            }
                            إذا (!any) اكسر;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_غوټه* np) {
                            إذا (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = ریښتیا;
                                أرجع;
                            }
                            إذا (Ast_Enum_توکی_Ref* ثابت توکی_Refp = VN_CAST(np, Enum_توکی_Ref)) {
                                إذا (V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(توکی_Refp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = ریښتیا;
                                }
                                أرجع;
                            }
                            AstRefDType* ثابت refp = VN_CAST(np, RefDType);
                            إذا (!refp) أرجع;
                            إذا (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = ریښتیا;
                            }
                            // A type declared in the template must not be widthed there
                            إذا (V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = ریښتیا;
                            }
                        });
                    }
                    إذا (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        إذا (AstConst* ثابت widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            إذا (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlink_Fr_شا();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_ونه(), cloneVarp);
                }
                AstConst* ثابت namingExprp = normedNamep ? normedNamep : exprp;
                إذا (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_غوټه* ثابت pinExprp = pinp->exprp();
                    pinExprp->replaceWith(جديد AstConst{pinp->fileline(), AstConst::Widthed_ارزښت{},
                                                       modvarp->پلنوالی(), 0});
                    VL_DO_DANGLING(pinExprp->delete_ونه(), pinExprp);
                } وإلا إذا (origp
                           && (exprp->same_ونه(origp)
                               || (exprp->عدد().پلنوالی() == origp->عدد().پلنوالی()
                                   && Parameterized_Hier_بلاکونه::areSame(exprp, origp))
                               || param_Consts_Equal_At_Max_پلنوالی(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } وإلا إذا (namingExprp->عدد().isDouble() || namingExprp->عدد().isString()
                           || namingExprp->عدد().isFourState()
                           || namingExprp->عدد().پلنوالی() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_ارزښت_شمېره(namingExprp));
                    any_overridesr = ریښتیا;
                } وإلا {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->عدد().ascii(خطأ));
                    any_overridesr = ریښتیا;
                }
                إذا (normedNamep) VL_DO_DANGLING(normedNamep->delete_ونه(), normedNamep);
            }
        } وإلا إذا (AstParamTypeDType* ثابت modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_غوټه_DType* rawTypep = VN_CAST(pinp->exprp(), غوټه_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            منطقي skip_پلنوالی_For_Template_Struct = خطأ;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_غوټه_DType* ثابت resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : تش_اشاره;
                إذا (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_غوټه_Module* ثابت ownerModp
                        = V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    إذا (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skip_پلنوالی_For_Template_Struct = ریښتیا;
                        V3Stats::جمع_کړه_Stat_مجموعه("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                إذا (rawTypep && !skip_پلنوالی_For_Template_Struct) V3_پلنوالی::پلنوالی_Params_Edit(rawTypep);
            }
            Ast_غوټه_DType* exprp = rawTypep ? rawTypep->skip_Ref_منزل_Non_Refp() : تش_اشاره;
            ثابت Ast_غوټه_DType* origp = modvarp->skip_Ref_منزل_Non_Refp();
            إذا (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وإلا إذا (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } وإلا {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                إذا (!skip_پلنوالی_For_Template_Struct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), غوټه_DType);
                    exprp = rawTypep->skip_Ref_منزل_Non_Refp();
                }
                إذا (!modvarp->fwdType().is_غوټه_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                إذا (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } وإلا {
                    إذا (!skip_پلنوالی_For_Template_Struct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), غوټه_DType);
                        exprp = rawTypep ? rawTypep->skip_Ref_منزل_Non_Refp() : تش_اشاره;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    إذا (AstClassRefDType* ثابت classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        إذا (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } وإلا {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    فراغ cellInterfaceCleanup(AstPin* pinsp, Ast_غوټه_Module* srcModp, string& longnamer,
                              منطقي& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        لكل (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ثابت AstVar* ثابت modvarp = pinp->modVarp();
            إذا (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                استمر;
            }
            إذا (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ثابت پورټ_Irefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ثابت Ast_غوټه* ثابت exprp = pinp->exprp();
                إذا (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    استمر;
                }
                AstIfaceRefDType* pinIrefp = تش_اشاره;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                إذا (ثابت Ast_غوټه_Var_Ref* ثابت vrp = VN_CAST(exprp, غوټه_Var_Ref)) {
                    إذا (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                إذا (!pinIrefp && exprp) {
                    ثابت Ast_غوټه* basep = exprp;
                    طالما (ثابت Ast_غوټه_Pre_Sel* ثابت selp = VN_CAST(basep, غوټه_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(basep, VarRef)) {
                        إذا (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << پورټ_Irefp);

                إذا (!پورټ_Irefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } وإلا إذا (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } وإلا {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    إذا (پورټ_Irefp->iface_له_لارې_Cellp() != pinIrefp->iface_له_لارې_Cellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_ارزښت_شمېره(pinIrefp));
                        any_overridesr = ریښتیا;
                        ifaceRefRefs.emplace_شا(پورټ_Irefp, pinIrefp);
                        إذا (پورټ_Irefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && پورټ_Irefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_غوټه::prettyNameQ(پورټ_Irefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_غوټه::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    فراغ storeOriginalParams(Ast_غوټه_Module* ثابت classp) {
        لكل (Ast_غوټه* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_غوټه* originalParamp = تش_اشاره;
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isGParam()) originalParamp = varp->clonep();
            } وإلا إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            إذا (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    فراغ generic_Interface_Var_تیاری(ثابت AstPin* ثابت paramsp, ثابت AstPin* ثابت pinsp) {
        std::unordered_map<string, ثابت AstPin*> paramspMap;
        لكل (ثابت AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ورننباسه({pinp->name().substr(std::مزي_اوږدوالی("__VGIfaceParam")), pinp});
            }
        }

        إذا (paramspMap.تشه()) أرجع;

        لكل (ثابت Ast_غوټه* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                إذا (AstVar* ثابت varp = pinp->modVarp()) {
                    Ast_غوټه_DType* dtypep = varp->ماشوم_DTypep()->elemDTypep();
                    إذا (AstIfaceGenericDType* ثابت ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ثابت خپلکاری iter = paramspMap.پیدا_کړه(varp->name());
                        إذا (iter == paramspMap.پای_ټکی()) استمر;
                        Ast_غوټه* ثابت backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlink_Fr_شا();
                        ثابت AstPin* ثابت paramp = iter->دوهم;
                        paramspMap.erase(iter);
                        ثابت AstIfaceRefDType* ثابت ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ثابت newIfacerefp = جديد AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        إذا (خپلکاری* ثابت arrDtp = VN_CAST(backp, غوټه_Array_DType)) {
                            arrDtp->ماشوم_DTypep(newIfacerefp);
                        } وإلا إذا (خپلکاری* ثابت arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->ماشوم_DTypep(newIfacerefp);
                        } وإلا {
                            varp->ماشوم_DTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ټېل_Deletep(ifaceGDTypep), ifaceGDTypep);
                        إذا (paramspMap.تشه()) أرجع;
                    }
                }
            }
        }

        UASSERT(paramspMap.تشه(), "Not every generic interface implicit param is used");
    }

    فراغ resolveDefaultParams(Ast_غوټه* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = تش_اشاره;
        AstPin* paramsp = تش_اشاره;

        إذا (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            إذا (!classp) أرجع;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } وإلا إذا (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } وإلا {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_شاخص.clear();
        لكل (Ast_غوټه* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                إذا (!paramTypep->isGParam()) استمر;
                m_param_شاخص.emplace(paramTypep, static_cast<صحيح>(m_classTypeParams.اندازه()));
                m_classTypeParams.emplace_شا(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::ویکټور<Ast_غوټه_DType*> defaultTypeNodes(m_classTypeParams.اندازه(), تش_اشاره);
        لكل (اندازه_t i = 0; i < m_classTypeParams.اندازه(); ++i) {
            AstParamTypeDType* ثابت paramTypep = m_classTypeParams[i].لومړی;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ثابت reqDtp = VN_CAST(paramTypep->get_ماشوم_DTypep(), RequireDType);
            إذا (!reqDtp) استمر;

            // If default is a reference to another param type, record dependency
            إذا (AstRefDType* ثابت refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                إذا (AstParamTypeDType* ثابت سرچینه_Paramp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    خپلکاری it = m_param_شاخص.پیدا_کړه(سرچینه_Paramp);
                    إذا (it != m_param_شاخص.پای_ټکی()) { m_classTypeParams[i].دوهم = it->دوهم; }
                    استمر;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            إذا (Ast_غوټه_DType* ثابت dtp = VN_CAST(reqDtp->lhsp(), غوټه_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::ویکټور<AstPin*> pins_By_شاخص;
        pins_By_شاخص.resize(m_classTypeParams.اندازه(), تش_اشاره);
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ثابت typep = pinp->modPTypep();
            ثابت خپلکاری it = typep ? m_param_شاخص.پیدا_کړه(typep) : m_param_شاخص.پای_ټکی();
            إذا (it != m_param_شاخص.پای_ټکی()) {
                pins_By_شاخص[it->دوهم] = pinp;
            } وإلا {
                pins_By_شاخص.ټېل_شا(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        لكل (منطقي resolvedSome = ریښتیا; resolvedSome;) {
            resolvedSome = خطأ;
            لكل (اندازه_t paramIdx = 0; paramIdx < m_classTypeParams.اندازه(); paramIdx++) {
                إذا (pins_By_شاخص[paramIdx]) استمر;
                ثابت صحيح سرچینه_Param_Idx = m_classTypeParams[paramIdx].دوهم;

                AstPin* newPinp = تش_اشاره;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                إذا (سرچینه_Param_Idx >= 0 && pins_By_شاخص[سرچینه_Param_Idx]) {
                    newPinp = pins_By_شاخص[سرچینه_Param_Idx]->clone_ونه(خطأ);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                إذا (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_غوټه_DType* ثابت dtypep = defaultTypeNodes[paramIdx];
                    newPinp = جديد AstPin{dtypep->fileline(), static_cast<صحيح>(paramIdx) + 1,
                                         "__paramNumber" + cvt_منزل_Str(paramIdx + 1),
                                         dtypep->clone_ونه(خطأ)};
                }

                إذا (newPinp) {
                    newPinp->name("__paramNumber" + cvt_منزل_Str(paramIdx + 1));
                    newPinp->param(ریښتیا);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].لومړی);
                    إذا (classOrPackageRef) {
                        classOrPackageRef->جمع_کړه_Paramsp(newPinp);
                    } وإلا إذا (classRefDType) {
                        classRefDType->جمع_کړه_Paramsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_شاخص[paramIdx] = newPinp;
                    إذا (!paramsp) paramsp = newPinp;
                    resolvedSome = ریښتیا;
                }
            }
        }
    }

    Ast_غوټه_Module* غوټه_Deparam_Common(Ast_غوټه* nodep, Ast_غوټه_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, منطقي any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        إذا (debug() >= 9 && paramsp) paramsp->dump_ونه_And_بل(وتنه, "-  cellparams: ");

        إذا (srcModp->hier_بلاک()) {
            longname = parameterized_Hier_بلاک_Name(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } وإلا {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_سېټ<ثابت AstPin*> overridingTypePins;
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->exprp()) استمر;  // An empty override keeps the default
                إذا (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    استمر;
                }
                AstVar* ثابت modvarp = pinp->modVarp();
                إذا (!modvarp || !modvarp->isGParam()) استمر;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ثابت Ast_نمونه* ثابت patternp = VN_CAST(pinp->exprp(), نمونه);
                إذا (patternp && !patternp->ماشوم_DTypep()) استمر;
                إذا (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_ارزښت(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    إذا (!overridingTypePins.شمېر(pinp)) استمر;
                    Ast_غوټه_DType* ثابت typep = VN_AS(pinp->exprp(), غوټه_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_ارزښت_شمېره(typep->skip_Ref_منزل_Non_Refp());
                    any_overrides = ریښتیا;
                    استمر;
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
        منطقي defaultsResolved = خطأ;
        إذا (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (pinp->modPTypep()) {
                    any_overrides = ریښتیا;
                    defaultsResolved = ریښتیا;
                    اكسر;
                }
            }
        }
        إذا (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.منزل_String();
            any_overrides = ریښتیا;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_غوټه_Module* newModp = تش_اشاره;
        إذا (m_hier_بلاکونه.hier_Sub_پرلپسې() && m_hier_بلاکونه.is_Hier_بلاک(srcModp->origName())) {
            Ast_غوټه_Module* ثابت paramedModp
                = m_hier_بلاکونه.پیدا_کړه_By_Params(srcModp->origName(), paramsp, m_modp);
            إذا (!paramedModp) أرجع تش_اشاره;  // Unsupported, already reported
            paramedModp->dead(خطأ);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } وإلا إذا (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            إذا (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_غوټه_Module* غوټه_Copyp = srcModp->clone_ونه(خطأ);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << غوټه_Copyp);
                m_deleter.ټېل_Deletep(غوټه_Copyp);  // nodeCopyp used past here
                srcModp->user3p(غوټه_Copyp);
                storeOriginalParams(غوټه_Copyp);
            }
            newModp = srcModp;
        } وإلا {
            ثابت string newname
                = srcModp->hier_بلاک() ? longname : moduleCalcName(srcModp, longname);
            ثابت ModInfo* ثابت modInfop
                = module_پیدا_کړه_Or_Clone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            إذا (!modInfop) أرجع تش_اشاره;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ثابت منطقي cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ثابت newClassp = VN_CAST(newModp, Class);
        إذا (newClassp && !newClassp->user2()) m_specializedClassps.ټېل_شا(newClassp);

        // Link source class to its specialized version for later relinking of method references
        إذا (defaultsResolved) srcModp->user4p(newModp);

        لكل (خپلکاری* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        generic_Interface_Var_تیاری(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        إذا (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_شا_With_بل()->delete_ونه(), paramsp);
        أرجع newModp;
    }

    Ast_غوټه_Module* cellDeparam(AstCell* nodep, Ast_غوټه_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_غوټه_Module* ثابت newModp = غوټه_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        إذا (!newModp) أرجع تش_اشاره;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(خطأ);
        أرجع newModp;
    }
    Ast_غوټه_Module* ifaceRefDeparam(AstIfaceRefDType* ثابت nodep, Ast_غوټه_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        إذا (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ثابت string ownerOrigName
                = ownerIfacep->origName().تشه() ? ownerIfacep->name() : ownerIfacep->origName();
            ثابت string srcOrigName
                = srcModp->origName().تشه() ? srcModp->name() : srcModp->origName();
            string owner_بنسټ_Name = ownerOrigName;
            ثابت اندازه_t ownerPos = owner_بنسټ_Name.پیدا_کړه("__");
            إذا (ownerPos != string::npos) owner_بنسټ_Name = owner_بنسټ_Name.substr(0, ownerPos);
            string src_بنسټ_Name = srcOrigName;
            ثابت اندازه_t srcPos = src_بنسټ_Name.پیدا_کړه("__");
            إذا (srcPos != string::npos) src_بنسټ_Name = src_بنسټ_Name.substr(0, srcPos);

            إذا (owner_بنسټ_Name == src_بنسټ_Name) {
                منطقي allOwnParams = ریښتیا;
                لكل (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    إذا (AstRefDType* ثابت refp = VN_CAST(pinp->exprp(), RefDType)) {
                        إذا (AstParamTypeDType* ثابت ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_غوټه_Module* ثابت ptdOwnerp
                                = V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(ptdp);
                            إذا (ptdOwnerp != m_modp) allOwnParams = خطأ;
                        } وإلا {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } وإلا {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                إذا (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::جمع_کړه_Stat_مجموعه("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    إذا (nodep->paramsp()) nodep->paramsp()->unlink_Fr_شا_With_بل()->delete_ونه();
                    أرجع ownerIfacep;
                }
            }
        }

        Ast_غوټه_Module* ثابت newModp
            = غوټه_Deparam_Common(nodep, srcModp, nodep->paramsp(), تش_اشاره, خطأ);
        إذا (!newModp) أرجع تش_اشاره;
        nodep->ifacep(VN_AS(newModp, Iface));
        أرجع newModp;
    }
    Ast_غوټه_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_غوټه_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_غوټه_Module* ثابت newModp
            = غوټه_Deparam_Common(nodep, srcModp, nodep->paramsp(), تش_اشاره, خطأ);
        إذا (!newModp) أرجع تش_اشاره;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ثابت refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ثابت newClassp = refDTypep ? VN_CAST(newModp, Class) : تش_اشاره;
        إذا (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            إذا (AstTypedef* ثابت typedefp
                = VN_CAST(m_memberMap.پیدا_کړه_Member(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        أرجع newModp;
    }
    Ast_غوټه_Module* classRefDeparam(AstClassRefDType* nodep, Ast_غوټه_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_غوټه_Module* ثابت newModp
            = غوټه_Deparam_Common(nodep, srcModp, nodep->paramsp(), تش_اشاره, خطأ);
        إذا (!newModp) أرجع تش_اشاره;
        AstClass* ثابت newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        أرجع newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    صنف DeferredResolverVisitor final : عام VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::سېټ<ثابت Ast_غوټه*> m_reachedDecls;

        منطقي لومړی_رسېدنه(ثابت Ast_غوټه* ثابت declp) { أرجع m_reachedDecls.ورننباسه(declp).دوهم; }

        فراغ iterateParamType(AstParamTypeDType* ثابت paramTypep) {
            إذا (لومړی_رسېدنه(paramTypep)) iterate(paramTypep);
        }
        فراغ iterateTypedef(AstTypedef* ثابت tdefp) {
            إذا (لومړی_رسېدنه(tdefp)) iterate(tdefp->subDTypep());
        }

        فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }
        فراغ لیدنه(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_غوټه* ثابت targetp = nodep->classOrPackageNodep();
            إذا (AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } وإلا إذا (AstParamTypeDType* ثابت paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        فراغ لیدنه(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolve_Dot_منزل_Typedef(nodep);
        }
        فراغ لیدنه(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ثابت tdefp = nodep->typedefp();
            إذا (tdefp) {
                iterateTypedef(tdefp);
            } وإلا إذا (AstParamTypeDType* ثابت paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        فراغ لیدنه(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ثابت varp = nodep->varp();
            إذا (لومړی_رسېدنه(varp)) {
                iterate(varp->subDTypep());
                ثابت خپلکاری& deferredVarps = v3Global.rootp()->deferredParamVarps();
                إذا (varp->varType() == VVarType::LPARAM && deferredVarps.شمېر(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    عام:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_غوټه* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

عام:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    فراغ retargetIfaceRefs(Ast_غوټه_Module* پلار_Modp, ثابت string& cellName) {
        Ast_غوټه_Module* ثابت correctModp
            = V3LinkDotIfaceCapture::follow_Cell_لار(پلار_Modp, cellName);
        إذا (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) أرجع;
        پلار_Modp->foreach([&](AstRefDType* refp) {
            ثابت VIface_Capture_نښان* ثابت tagp = V3LinkDotIfaceCapture::capture_نښان(refp);
            إذا (!tagp || tagp->m_cell_لار != cellName) أرجع;
            // Only retarget REFDTYPEs owned by parentModp.
            إذا (V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(refp) != پلار_Modp) أرجع;
            إذا (V3LinkDotIfaceCapture::retarget_Ref_منزل_Module(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    فراغ resolveDeferredDotsReachableFrom(Ast_غوټه* rootp, ثابت Ast_غوټه_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*هذا, rootp};
    }

    Ast_غوټه_Module* غوټه_Deparam(Ast_غوټه* nodep, Ast_غوټه_Module* srcModp, Ast_غوټه_Module* modp,
                               ثابت string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_غوټه::غوټه_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string غوټه_Name = nodep->name();
        إذا (AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            إذا (غوټه_Name.تشه()) غوټه_Name = ifaceRefp->cellName();
        }
        ثابت string instanceName
            = غوټه_Name.تشه() ? someInstanceName : (someInstanceName + "." + غوټه_Name);

        إذا (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            لكل (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ثابت nextp = VN_AS(pinp->nextp(), Pin);
                إذا (!pinp->param_لار().تشه() && instanceName != pinp->param_لار()
                    && !VString::پایونه_With(instanceName, "." + pinp->param_لار())) {
                    VL_DO_DANGLING(pinp->unlink_Fr_شا()->delete_ونه(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_غوټه::غوټه_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_غوټه_Module* newModp = تش_اشاره;
        إذا (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } وإلا إذا (AstIfaceRefDType* ثابت ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } وإلا إذا (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وإلا إذا (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وإلا {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        إذا (!newModp) أرجع تش_اشاره;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_غوټه::غوټه_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        أرجع newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::ویکټور<AstClass*> takeSpecializedClassps() {
        std::ویکټور<AstClass*> اخیستل_شوی;
        اخیستل_شوی.بدلون(m_specializedClassps);
        أرجع اخیستل_شوی;
    }

    // CONSTRUCTORS
    څرګند ParamProcessor(AstNetlist* nodep)
        : m_hier_بلاکونه{v3Global.opt.hier_بلاکونه(), nodep} {
        لكل (Ast_غوټه_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), غوټه_Module)) {
            m_all_Module_نومونه.ورننباسه(modp->name());
        }
    }
    ~ParamProcessor() = افتراضي;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

صنف ClassRefUnlinkerVisitor final : عام VNVisitor {

عام:
    څرګند ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {
        إذا (nodep->paramsp()) {
            إذا (AstClass* ثابت classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                إذا (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ثابت dotp = VN_CAST(nodep->backp(), Dot);
                    إذا (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlink_Fr_شا());
                        VL_DO_DANGLING2(ټېل_Deletep(dotp), dotp, nodep);
                    } وإلا {
                        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                    }
                }
            }
        }
    }
    فراغ لیدنه(AstClass* nodep) override {}  // don't iterate inside classes
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

صنف ParamClassRefDTypeRelinkVisitor final : عام VNVisitor {

    AstClass* m_classp = تش_اشاره;
    std::unordered_map<ثابت AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_غوټه_Module* m_ownerModp = تش_اشاره;
    Ast_غوټه_Module* m_ownerMapForp = تش_اشاره;
    std::unordered_map<std::string, AstClass*> m_orig_Name_منزل_Clone;
    std::unordered_سېټ<std::string> m_ambiguous;

    فراغ ensureOwnerMap() {
        إذا (m_ownerMapForp == m_ownerModp) أرجع;
        m_ownerMapForp = m_ownerModp;
        m_orig_Name_منزل_Clone.clear();
        m_ambiguous.clear();
        إذا (!m_ownerModp) أرجع;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        إذا (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            إذا (ownerClassp->hasGParam()) أرجع;
        }
        لكل (Ast_غوټه* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ثابت tdp = VN_CAST(sp, Typedef);
            إذا (!tdp) استمر;
            AstClassRefDType* ثابت crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            إذا (!crdtp || !crdtp->classp()) استمر;
            AstClass* ثابت targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            إذا (targetp->hasGParam()) استمر;
            ثابت std::string origName
                = targetp->origName().تشه() ? targetp->name() : targetp->origName();
            ثابت خپلکاری pair = m_orig_Name_منزل_Clone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            إذا (!pair.دوهم && pair.لومړی->دوهم != targetp) m_ambiguous.ورننباسه(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ساكن string classOrigName(ثابت AstClass* classp) {
        أرجع classp->origName().تشه() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ساكن AstTypedef* پیدا_کړه_Typedef_With_Bases(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : تش_اشاره) {
            إذا (AstTypedef* ثابت tdp = V3LinkDotIfaceCapture::پیدا_کړه_Typedef_دننه_Module(cp, name)) {
                إذا (tdp->subDTypep()) أرجع tdp;
            }
        }
        أرجع تش_اشاره;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    فراغ retargetRefDType(AstRefDType* refp) {
        إذا (!m_ownerModp) أرجع;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        إذا (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            إذا (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ثابت AstClass* refClassp = تش_اشاره;
                إذا (AstTypedef* ثابت tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(tdp), Class);
                } وإلا إذا (ثابت AstClassOrPackageRef* ثابت classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                إذا (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ثابت selfTdp = پیدا_کړه_Typedef_With_Bases(ownerClassp, refp->name());
                    إذا (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        أرجع;
                    }
                }
            }
        }

        AstTypedef* ثابت oldTdp = refp->typedefp();
        إذا (!oldTdp) أرجع;
        AstClass* ثابت oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(oldTdp), Class);
        إذا (!oldOwnerp) أرجع;

        ensureOwnerMap();
        إذا (m_orig_Name_منزل_Clone.تشه()) أرجع;
        ثابت std::string origName
            = oldOwnerp->origName().تشه() ? oldOwnerp->name() : oldOwnerp->origName();
        إذا (m_ambiguous.شمېر(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            أرجع;
        }
        ثابت خپلکاری it = m_orig_Name_منزل_Clone.پیدا_کړه(origName);
        إذا (it == m_orig_Name_منزل_Clone.پای_ټکی()) أرجع;
        AstClass* ثابت correctClassp = it->دوهم;
        إذا (correctClassp == oldOwnerp) أرجع;
        AstTypedef* ثابت newTdp
            = V3LinkDotIfaceCapture::پیدا_کړه_Typedef_دننه_Module(correctClassp, refp->name());
        إذا (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            أرجع;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        إذا (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            أرجع;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

عام:
    څرګند ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        إذا (AstParamTypeDType* ثابت paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ثابت خپلکاری it = m_paramTypeClassMap.پیدا_کړه(paramtypep);
            إذا (it != m_paramTypeClassMap.پای_ټکی()) {
                AstClass* ثابت origClassp = it->دوهم;
                إذا (origClassp->hasGParam() && !origClassp->user3p()) {
                    إذا (AstClass* ثابت parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ثابت string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = تش_اشاره;
                        لكل (Ast_غوټه* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                إذا (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    اكسر;
                                }
                            }
                        }
                        إذا (newParamTypep) {
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
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

صنف ParamState final {
عام:
    // STATE - across all visitors
    std::ویکټور<AstClass*> m_paramClasses;  // Parameterized classes
    std::ویکټور<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    کارول WQKey = std::pair<منطقي, صحيح>;
    std::multimap<WQKey, Ast_غوټه_Module*> m_work_کتار_بل;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_غوټه_Module*, std::unordered_سېټ<Ast_غوټه_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

صنف ParamVisitor final : عام VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    منطقي m_iterateModule = خطأ;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<منطقي, Ast_غوټه*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ثابت Ast_غوټه*, std::string>
        m_gen_Hier_نومونه;  // Maps ast nodes to generated hierarchy names
    std::map<ثابت AstRefDType*, منطقي>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    کارول VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ثابت Ast_غوټه_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Module iterating
    std::unordered_سېټ<std::string> m_iface_پورټ_نومونه;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    منطقي m_mod_Iface_Refs_بشپړ = خطأ;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    منطقي m_دننه_Generate_Cond = خطأ;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    فراغ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_غوټه_Module*> work_کتار;
        m_generateHierName = "";
        m_iterateModule = ریښتیا;

        // Visit all cells under module, recursively
        طالما (ریښتیا) {
            // Classes specialized since the last pass still need their bodies elaborated
            لكل (AstClass* ثابت classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_کتار_بل.emplace(ParamState::WQKey{ریښتیا, classp->level()}, classp);
            }
            إذا (work_کتار.تشه()) std::بدلون(work_کتار, m_state.m_work_کتار_بل);
            إذا (work_کتار.تشه()) اكسر;

            ثابت خپلکاری itm = work_کتار.cbegin();
            Ast_غوټه_Module* ثابت modp = itm->دوهم;
            work_کتار.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            إذا (!modp->user2_سېټ_Once()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                إذا (modp->someInstanceName().تشه()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_iface_پورټ_نومونه);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_mod_Iface_Refs_بشپړ);
                    m_modp = modp;
                    m_mod_Iface_Refs_بشپړ = خطأ;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            طالما (!m_cellps.تشه()) {
                ثابت خپلکاری itim = m_cellps.cbegin();
                Ast_غوټه* ثابت cellp = itim->دوهم;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ثابت خپلکاری genHierIt = m_gen_Hier_نومونه.پیدا_کړه(cellp);
                إذا (genHierIt != m_gen_Hier_نومونه.پای_ټکی()) {
                    genHierName = std::move(genHierIt->دوهم);
                    m_gen_Hier_نومونه.erase(genHierIt);
                }

                Ast_غوټه_Module* srcModp = تش_اشاره;
                إذا (ثابت AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } وإلا إذا (ثابت AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    إذا (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) استمر;
                } وإلا إذا (ثابت AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } وإلا إذا (ثابت AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } وإلا {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                إذا (!srcModp) استمر;

                // Update path
                ثابت string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                إذا (Ast_غوټه_Module* ثابت newModp
                    = m_processor.غوټه_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    إذا (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        إذا (V3LinkDotIfaceCapture::enabled()) {
                            إذا (ثابت AstCell* ثابت modCellp = VN_CAST(cellp, Cell)) {
                                إذا (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    work_کتار.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ورننباسه(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    إذا (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = خطأ;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ساكن string get_Ref_بنسټ_Name(ثابت AstVarXRef* refp) {
        ثابت string dotted = refp->dotted();
        إذا (dotted.تشه()) أرجع "";
        أرجع dotted.substr(0, dotted.پیدا_کړه('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    فراغ logTemplateLeakRefs(Ast_غوټه_Module* پلار_Modp, Ast_غوټه_Module* templateModp,
                             ثابت حرف* stage, Ast_غوټه* contextp) {
        إذا (debug() < 9 || !پلار_Modp || !templateModp || !VN_IS(templateModp, Iface)) أرجع;
        // LCOV_EXCL_START  // Debug-only diagnostic
        صحيح leak_شمېر = 0;
        ثابت خپلکاری ancestry_د = [](ثابت Ast_غوټه* nodep) {
            string ancestry;
            لكل (ثابت Ast_غوټه* curp = nodep; curp; curp = curp->backp()) {
                إذا (!ancestry.تشه()) ancestry += "<-";
                ancestry += curp->typeName();
                إذا (VN_IS(curp, غوټه_Module) || VN_IS(curp, Netlist) || VN_IS(curp, Type_جدول)) {
                    اكسر;
                }
            }
            أرجع ancestry;
        };

        پلار_Modp->foreach([&](AstRefDType* refp) {
            إذا (refp->typedefp()) {
                Ast_غوټه_Module* ثابت tdOwnerp
                    = V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(refp->typedefp());
                إذا (tdOwnerp == templateModp) {
                    ++leak_شمېر;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << پلار_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_غوټه::غوټه_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_د(refp));
                }
            }
            إذا (refp->refDTypep()) {
                Ast_غوټه_Module* ثابت rdOwnerp
                    = V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(refp->refDTypep());
                إذا (rdOwnerp == templateModp) {
                    ++leak_شمېر;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << پلار_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_غوټه::غوټه_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_د(refp));
                }
            }
        });

        پلار_Modp->foreach([&](AstVarRef* varrefp) {
            إذا (!varrefp->varp()) أرجع;
            Ast_غوټه_Module* ثابت varOwnerp
                = V3LinkDotIfaceCapture::پیدا_کړه_Owner_Module(varrefp->varp());
            إذا (varOwnerp != templateModp) أرجع;
            ++leak_شمېر;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << پلار_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_غوټه::غوټه_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestry_د(varrefp));
        });

        إذا (leak_شمېر > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << پلار_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_شمېر);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    فراغ checkParamNotHierRecurse(Ast_غوټه* nodep, منطقي underTypeQuery = خطأ) {
        لكل (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ثابت Ast_Attr_د* ثابت attrp = VN_CAST(nodep, Attr_د);
            ثابت منطقي ماشوم_Under_Query
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            إذا (ثابت AstVarXRef* ثابت refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                منطقي isIfaceRef = خطأ;
                إذا (refp->varp() && refp->varp()->isIfaceParam()) {
                    ثابت string refname = get_Ref_بنسټ_Name(refp);
                    isIfaceRef
                        = !refname.تشه()
                          && (m_iface_پورټ_نومونه.شمېر(refname) || m_ifaceInstCells.شمېر(refname));
                }
                إذا (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } وإلا إذا (ثابت Ast_غوټه_FTask_Ref* ثابت refp = VN_CAST(nodep, غوټه_FTask_Ref)) {
                إذا (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), ماشوم_Under_Query);
            checkParamNotHierRecurse(nodep->op2p(), ماشوم_Under_Query);
            checkParamNotHierRecurse(nodep->op3p(), ماشوم_Under_Query);
            checkParamNotHierRecurse(nodep->op4p(), ماشوم_Under_Query);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    فراغ specializeNestedIfaceCells(Ast_غوټه_Module* ifaceModp) {
        لكل (Ast_غوټه* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت nestedCellp = VN_CAST(stmtp, Cell);
            إذا (!nestedCellp) استمر;
            إذا (!VN_IS(nestedCellp->modp(), Iface)) استمر;
            إذا (!nestedCellp->paramsp()) استمر;

            Ast_غوټه_Module* ثابت nestedSrcModp = nestedCellp->modp();
            إذا (Ast_غوټه_Module* ثابت nestedNewModp = m_processor.غوټه_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                إذا (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    لكل (Ast_غوټه* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        إذا (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                            إذا (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    إذا (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    منطقي cellParamsReferenceIfacePorts(AstCell* cellp) {
        إذا (!cellp->paramsp()) أرجع خطأ;

        لكل (Ast_غوټه* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            إذا (AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                إذا (Ast_غوټه* ثابت exprp = pinp->exprp()) {
                    إذا (ثابت AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
                        ثابت string refname = get_Ref_بنسټ_Name(refp);
                        إذا (!refname.تشه()
                            && (m_iface_پورټ_نومونه.شمېر(refname)
                                || m_ifaceInstCells.شمېر(refname)))
                            أرجع ریښتیا;
                    }
                }
            }
        }
        أرجع خطأ;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    فراغ expand_Iface_Array_پورټ_Dimensions(AstVar* portp, Ast_غوټه_DType* dtypep,
                                        ثابت std::string& suffix) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        إذا (!arrp) {
            AstVar* ثابت varp = portp->clone_ونه(خطأ);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            إذا (Ast_غوټه_DType* ثابت oldp = varp->ماشوم_DTypep()) {
                VL_DO_DANGLING(ټېل_Deletep(oldp->unlink_Fr_شا()), oldp);
            }
            varp->dtypep(تش_اشاره);
            varp->ماشوم_DTypep(portp->subDTypep()->elemDTypep()->clone_ونه(خطأ));
            portp->جمع_کړه_بل_دلته(varp);
            أرجع;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        لكل (ثابت صحيح n : arrp->declRange().seq_ښی_منزل_کیڼ()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_غوټه::encode_شمېره(n) + "__KET__";
            expand_Iface_Array_پورټ_Dimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    فراغ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ثابت AstRange* rangep, ثابت std::string& suffix, صحيح idx) {
        // Base case: insert the element when no dimensions left
        إذا (!rangep) {
            AstCell* ثابت elemp = arrayedCellp->clone_ونه(خطأ);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->جمع_کړه_بل_دلته(elemp);
            إذا (ifaceVarp) {
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_ونه(خطأ);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ثابت varp = ifaceVarp->clone_ونه(خطأ);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                إذا (Ast_غوټه_DType* ثابت oldp = varp->ماشوم_DTypep()) {
                    VL_DO_DANGLING(ټېل_Deletep(oldp->unlink_Fr_شا()), oldp);
                }
                varp->dtypep(تش_اشاره);
                varp->ماشوم_DTypep(irefp);
                ifaceVarp->جمع_کړه_بل_دلته(varp);
            }
            أرجع;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);
        لكل (ثابت صحيح n : rangep->seq_ښی_منزل_کیڼ()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_غوټه::encode_شمېره(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    فراغ لیدنه_Cell_Or_Class_Ref(Ast_غوټه* nodep, منطقي isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_gen_Hier_نومونه.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        إذا (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ثابت cellp = VN_CAST(nodep, Cell);
            إذا (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_غوټه_Module* ثابت srcModp = cellp->modp();
                Ast_غوټه_Module* ثابت newModp
                    = m_processor.غوټه_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                إذا (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                إذا (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        إذا (!m_دننه_Generate_Cond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (nodep->recursiveClone()) nodep->dead(ریښتیا);  // Fake, made for recursive elimination
        إذا (nodep->dead()) أرجع;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            إذا (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ټېل_شا(classp);
                أرجع;
            }
        }

        إذا (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_کتار_بل.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            أرجع;
        }

        // Start traversal at root-like things
        إذا (nodep->is_سر()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_کتار_بل.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    منطقي isCircularType(ثابت AstRefDType* nodep) {
        ثابت خپلکاری iter = m_isCircular.emplace(nodep, ریښتیا);
        إذا (!iter.دوهم) أرجع iter.لومړی->دوهم;
        إذا (ثابت AstRefDType* ثابت subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ثابت منطقي ret = isCircularType(subDTypep);
            iter.لومړی->دوهم = ret;
            أرجع ret;
        }
        iter.لومړی->دوهم = خطأ;
        أرجع خطأ;
    }

    فراغ لیدنه(AstRefDType* nodep) override {
        ثابت منطقي isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        إذا (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        إذا (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } وإلا إذا (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2_سېټ_Once()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCell* nodep) override {
        إذا (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            لكل (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3_پلنوالی::پلنوالی_Params_Edit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ثابت ifaceVarp = [&]() -> AstVar* {
                إذا (!VN_IS(nodep->modp(), Iface)) أرجع تش_اشاره;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ثابت varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                أرجع varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            إذا (ifaceVarp) {
                ifaceVarp->is_Iface_Array_وېشل(ریښتیا);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ثابت firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            أرجع;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        إذا (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        لیدنه_Cell_Or_Class_Ref(nodep, VN_IS(nodep->modp(), Iface));
    }
    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2_سېټ_Once();
        إذا (nodep->ifacep()) لیدنه_Cell_Or_Class_Ref(nodep, ریښتیا);
    }
    فراغ لیدنه(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2_سېټ_Once();
        checkParamNotHierRecurse(nodep->paramsp());
        لیدنه_Cell_Or_Class_Ref(nodep, خطأ);
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        إذا (!VN_IS(nodep->classOrPackageNodep(), Typedef)) لیدنه_Cell_Or_Class_Ref(nodep, خطأ);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ساكن فراغ gatherVars(Ast_غوټه* stmtsp, منطقي (*matchp)(ثابت AstVar*), VarsByName& varsr) {
        لكل (Ast_غوټه* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                إذا (matchp(varp)) varsr.emplace(varp->name(), varp);
            } وإلا إذا (Ast_Gen_بلاک* ثابت genp = VN_CAST(nodep, Gen_بلاک)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ثابت VarsByName& modIfaceRefs() {
        إذا (!m_mod_Iface_Refs_بشپړ) {
            gatherVars(
                m_modp->stmtsp(), [](ثابت AstVar* varp) { أرجع varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_mod_Iface_Refs_بشپړ = ریښتیا;
        }
        أرجع m_modIfaceRefs;
    }
    ثابت VarsByName& ifaceParams(Ast_غوټه_Module* ifacep) {
        ثابت خپلکاری pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        إذا (pair.دوهم) {
            gatherVars(
                ifacep->stmtsp(), [](ثابت AstVar* varp) { أرجع varp->isParam(); },
                pair.لومړی->دوهم);
        }
        أرجع pair.لومړی->دوهم;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_غوټه* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ثابت lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ثابت rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        إذا (!lhsp || !rhsp) أرجع تش_اشاره;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        إذا (VL_UNCOVERABLE(!m_modp)) أرجع تش_اشاره;

        ثابت خپلکاری& refVars = modIfaceRefs();
        ثابت خپلکاری ifaceVarIt = refVars.پیدا_کړه(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        إذا (VL_UNCOVERABLE(ifaceVarIt == refVars.پای_ټکی())) أرجع تش_اشاره;
        AstVar* ثابت ifaceVarp = ifaceVarIt->دوهم;
        AstIfaceRefDType* ثابت ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        إذا (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) أرجع تش_اشاره;

        ثابت خپلکاری& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ثابت خپلکاری موخه_Var_It = ifaceVars.پیدا_کړه(rhsp->name());
        إذا (موخه_Var_It == ifaceVars.پای_ټکی()) أرجع تش_اشاره;
        AstVar* ثابت موخه_Varp = موخه_Var_It->دوهم;
        // processWorkQ() visits interface cells first, so the interface is already constified
        إذا (VL_UNCOVERABLE(!VN_IS(موخه_Varp->valuep(), Const))) iterate(موخه_Varp);
        إذا (VL_UNCOVERABLE(!موخه_Varp->valuep() || !VN_IS(موخه_Varp->valuep(), Const))) {
            أرجع تش_اشاره;  // LCOV_EXCL_LINE
        }
        أرجع موخه_Varp->valuep()->clone_ونه(خطأ);
    }
    فراغ resolve_Generic_Iface_Dots_دننه(Ast_غوټه* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::ویکټور<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ټېل_شا(dotp); });
        لكل (AstDot* ثابت dotp : dotps) {
            إذا (Ast_غوټه* ثابت newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_ونه(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    فراغ لیدنه(AstVar* nodep) override {
        إذا (nodep->user2_سېټ_Once()) أرجع;  // Process once
        // Interface array port: split it into its elements
        إذا (nodep->isIfaceRef() && !nodep->is_Iface_پلار()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3_پلنوالی::پلنوالی_Params_Edit(nodep->subDTypep());
            expand_Iface_Array_پورټ_Dimensions(nodep, nodep->subDTypep(), "");
            nodep->is_Iface_Array_وېشل(ریښتیا);
        }
        // Build cache of interface port names as we encounter them
        إذا (nodep->isIfaceRef()) { m_iface_پورټ_نومونه.ورننباسه(nodep->name()); }
        iterateChildren(nodep);
        إذا (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            إذا (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } وإلا إذا (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolve_Generic_Iface_Dots_دننه(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                منطقي hasUnresolvedLparamXRef = خطأ;
                nodep->valuep()->foreach([&](ثابت AstVarXRef* xrefp) {
                    إذا (ثابت AstVar* ثابت varp = xrefp->varp()) {
                        إذا (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = ریښتیا;
                        }
                    }
                });
                إذا (hasUnresolvedLparamXRef) أرجع;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ثابت منطقي hasDot = nodep->valuep()->exists([](AstDot*) { أرجع ریښتیا; });
                منطقي refsDeferred = خطأ;
                إذا (!hasDot) {
                    ثابت خپلکاری& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ثابت AstVarRef* refp) {
                        إذا (refsDeferred) أرجع;
                        AstVar* ثابت refVarp = refp->varp();
                        إذا (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.شمېر(refVarp)) {
                            refsDeferred = ریښتیا;
                        }
                    });
                }
                إذا (hasDot || refsDeferred) {
                    v3Global.rootp()->ټېل_Deferred_Param_Varp(nodep);
                    أرجع;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    فراغ لیدنه(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        إذا (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    فراغ لیدنه(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        إذا (nodep->varp()) iterate(nodep->varp());
    }
    منطقي ifaceParamReplace(AstVarXRef* nodep, Ast_غوټه* candp) {
        لكل (; candp; candp = candp->nextp()) {
            إذا (nodep->name() == candp->name()) {
                إذا (AstVar* ثابت varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    إذا (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    أرجع ریښتیا;
                } وإلا إذا (ثابت AstPin* ثابت pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_ونه(خطأ));
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع ریښتیا;
                }
            }
        }
        أرجع خطأ;
    }

    فراغ deparamIfaceCellNow(AstCell* cellp) {
        إذا (!cellp->paramsp()) أرجع;
        إذا (!VN_IS(cellp->modp(), Iface)) أرجع;
        Ast_غوټه_Module* ثابت srcModp = cellp->modp();
        Ast_غوټه_Module* ثابت newModp
            = m_processor.غوټه_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        إذا (newModp && newModp != srcModp) {
            إذا (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (nodep->لري_Gen_بلاک()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(تش_اشاره);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Enum_توکی_Ref* nodep) override {
        // Needs relink, as may remove pointed-to item
        إذا (nodep->لري_Gen_بلاک()) nodep->itemp(تش_اشاره);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarXRef* nodep) override {
        إذا (nodep->لري_Gen_بلاک()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(تش_اشاره);
            أرجع;
        }
        // Parameters of an interface array are accessed through its first element
        إذا (nodep->varp() && nodep->varp()->isParam()) {
            ثابت خپلکاری it = m_ifaceInstCells.پیدا_کړه(nodep->dotted());
            إذا (it != m_ifaceInstCells.پای_ټکی()) nodep->dotted(it->دوهم->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ثابت string dotted = nodep->dotted();
        إذا (!dotted.تشه() && nodep->varp() && nodep->varp()->isParam()) {
            ثابت Ast_غوټه* backp = nodep;
            طالما ((backp = backp->backp())) {
                إذا (VN_IS(backp, غوټه_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    اكسر;
                }
                إذا (ثابت AstVar* ثابت varp = VN_CAST(backp, Var)) {
                    إذا (!varp->isIfaceRef()) استمر;
                    // Through all dimensions of an array
                    ثابت AstIfaceRefDType* ثابت ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    إذا (!ifacerefp) استمر;
                    // Interfaces passed in on the port map have ifaces
                    إذا (ثابت AstIface* ثابت ifacep = ifacerefp->ifacep()) {
                        إذا (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            إذا (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                أرجع;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    وإلا إذا (AstCell* ثابت cellp = ifacerefp->cellp()) {
                        إذا (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            إذا (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                أرجع;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ثابت خپلکاری ifaceCellIt = m_ifaceInstCells.پیدا_کړه(dotted);
            إذا (ifaceCellIt != m_ifaceInstCells.پای_ټکی()) {
                AstCell* ثابت cellp = ifaceCellIt->دوهم;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    فراغ لیدنه(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ثابت AstClass* lhsClassp = تش_اشاره;
        ثابت AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        إذا (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_غوټه* rhsDefp = تش_اشاره;
        AstClassOrPackageRef* ثابت rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        إذا (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        إذا (lhsClassp && rhsDefp) {
            m_state.m_dots.ټېل_شا(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } وإلا {
            iterate(nodep->rhsp());
        }
    }

    فراغ لیدنه(AstUnlinkedRef* nodep) override {
        AstVarXRef* ثابت varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_غوټه_FTask_Ref* ثابت taskrefp = VN_CAST(nodep->refp(), غوټه_FTask_Ref);
        Ast_Enum_توکی_Ref* ثابت enumrefp = VN_CAST(nodep->refp(), Enum_توکی_Ref);
        إذا (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } وإلا إذا (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } وإلا إذا (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } وإلا {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            أرجع;
        }
        iterate(nodep->cellrefp());

        إذا (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } وإلا إذا (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } وإلا {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlink_Fr_شا());
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        لكل (Ast_غوټه *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::ویکټور<صحيح> indices;
        لكل (Ast_غوټه* s = nodep->selp(); s; s = s->nextp()) {
            ثابت AstConst* ثابت constp = VN_CAST(s, Const);
            إذا (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                أرجع;
            }
            indices.ټېل_شا(constp->منزل_SInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ثابت string viftopSuffix = "__Viftop";
        ثابت string بنسټ_Name
            = VString::پایونه_With(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().اندازه() - viftopSuffix.اندازه())
                  : nodep->name();
        ثابت string placeholder = "__BRA__??__KET__";
        اندازه_t pos = m_unlinkedTxt.پیدا_کړه(بنسټ_Name + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        إذا (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            أرجع;
        }
        pos += بنسټ_Name.اوږدتیا();
        لكل (صحيح idx : indices) {
            ثابت string replacement = "__BRA__" + Ast_غوټه::encode_شمېره(idx) + "__KET__";
            إذا (m_unlinkedTxt.compare(pos, placeholder.اوږدتیا(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                أرجع;
            }
            m_unlinkedTxt.replace(pos, placeholder.اوږدتیا(), replacement);
            pos += replacement.اوږدتیا();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    فراغ iterateGenerateCond(Ast_غوټه* nodep) {
        VL_RESTORER(m_دننه_Generate_Cond);
        m_دننه_Generate_Cond = ریښتیا;
        iterate_And_بل_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    فراغ لیدنه(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3_پلنوالی::پلنوالی_Generate_Params_Edit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
            إذا (Ast_غوټه* ثابت keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_شا_With_بل();
                nodep->replaceWith(keepp);
            } وإلا {
                nodep->unlink_Fr_شا();
            }
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            // Normal edit rules will now recurse the replacement
        } وإلا {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        إذا (AstGenFor* ثابت forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3_پلنوالی::پلنوالی_Params_Edit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            ثابت string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            إذا (Ast_غوټه* ثابت stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_شا_With_بل();
                nodep->جمع_کړه_بل_دلته(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } وإلا {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    فراغ لیدنه(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        منطقي موندنه = خطأ;
        Ast_غوټه* keepp = تش_اشاره;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        لكل (Ast_Gen_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_توکی)) {
            لكل (Ast_غوټه* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3_پلنوالی::پلنوالی_Params_Edit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ثابت AstConst* ثابت exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        لكل (Ast_Gen_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_توکی)) {
            لكل (Ast_غوټه* ep = itemp->condsp(); ep;) {
                Ast_غوټه* ثابت nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        لكل (Ast_Gen_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_توکی)) {
            إذا (!itemp->isDefault()) {
                لكل (Ast_غوټه* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    إذا (ثابت AstConst* ثابت ccondp = VN_CAST(ep, Const)) {
                        V3_شمېره سمون{nodep, 1};
                        سمون.opEq(ccondp->عدد(), exprp->عدد());
                        إذا (!موندنه && سمون.isNeqZero()) {
                            موندنه = ریښتیا;
                            keepp = itemp->itemsp();
                        }
                    } وإلا {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        لكل (Ast_Gen_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_توکی)) {
            إذا (itemp->isDefault()) {
                إذا (!موندنه) {
                    موندنه = ریښتیا;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        إذا (keepp) {
            keepp->unlink_Fr_شا_With_بل();
            nodep->replaceWith(keepp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = افتراضي;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

صنف Param_سر final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    فراغ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        لكل (AstDot* ثابت dotp : m_state.m_dots) {
            ثابت AstClassOrPackageRef* ثابت classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ثابت AstClass* ثابت lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            لكل (خپلکاری* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    اكسر;
                }
            }
        }
    }

    فراغ fixLevel(Ast_غوټه_Module* modp) {
        // Fix up level of module, based on who instantiates it
        إذا (modp->user1_سېټ_Once()) أرجع;  // Already fixed
        إذا (m_state.m_parentps[modp].تشه()) أرجع;  // Leave top levels alone
        صحيح max_پلار_Level = 0;
        لكل (Ast_غوټه_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            max_پلار_Level = std::max(max_پلار_Level, parentp->level());
        }
        إذا (modp->level() <= max_پلار_Level) modp->level(max_پلار_Level + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->ژوروالی(modp->level());
    }

    فراغ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::ویکټور<Ast_غوټه_Module*> modps;
        لكل (Ast_غوټه_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), غوټه_Module);
            modp->unlink_Fr_شا();
            modps.ټېل_شا(modp);
        }

        // Fix-up levels
        ثابت VNUser1_دننه_Use user1_دننه_Use;
        لكل (Ast_غوټه_Module* ثابت modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_ترتیبول(modps.begin(), modps.پای_ټکی(),
                         [](ثابت Ast_غوټه_Module* ap, ثابت Ast_غوټه_Module* bp) {
                             أرجع ap->level() < bp->level();
                         });

        // Re-insert modules
        لكل (Ast_غوټه_Module* ثابت modp : modps) netlistp->جمع_کړه_Modulesp(modp);
    }

عام:
    // CONSTRUCTORS
    څرګند Param_سر(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ثابت ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ثابت ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_غوټه_FTask_Ref* ftaskrefp) {
            Ast_غوټه_FTask* ftaskp = ftaskrefp->taskp();
            إذا (!ftaskp || !ftaskp->class_لاره()) أرجع;
            ثابت std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ثابت خپلکاری ancestor_Class_د = [](Ast_غوټه* startp) -> AstClass* {
                لكل (Ast_غوټه* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    إذا (AstClass* ثابت cp = VN_CAST(np, Class)) أرجع cp;
                }
                أرجع تش_اشاره;
            };
            AstClass* refClassp = ancestor_Class_د(ftaskrefp);
            إذا (refClassp == ftaskrefp->classOrPackagep())
                أرجع;  // task is in the same class as reference
            AstClass* classp = ancestor_Class_د(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            إذا (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                أرجع;
            }
            إذا (classp->user3p()) أرجع;  // will not get removed, no need to relink
            AstClass* ثابت parametrizedClassp = VN_CAST(classp->user4p(), Class);
            إذا (!parametrizedClassp) أرجع;
            Ast_غوټه_FTask* newFuncp = تش_اشاره;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_غوټه_FTask* ftaskp) {
                إذا (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    أرجع ریښتیا;
                }
                أرجع خطأ;
            });
            إذا (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ثابت ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_سېټ<AstClass*> removedClassps;
        لكل (AstClass* ثابت classp : m_state.m_paramClasses) {
            إذا (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                إذا (V3Error::error_شمېر()) استمر;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ټېل_Deletep(classp->unlink_Fr_شا()), classp);
            } وإلا {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(خطأ);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_غوټه::user3_Clear_ونه();
        لكل (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_غوټه* ثابت nodep) { nodep->user3(ریښتیا); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_غوټه* ثابت nodep) {
            nodep->foreachLink([&](Ast_غوټه** ثابت linkpp, ثابت حرف* namep) {
                إذا (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = تش_اشاره;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_سر() = افتراضي;
    VL_UNCOPYABLE(Param_سر);
};

//######################################################################
// Param class functions

فراغ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    إذا (dump_ونه_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_سر{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    إذا (dump_ونه_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_ونه("param", 0, dump_ونه_Either_Level() >= 3);
}

فراغ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    لكل (AstVar* ثابت varp : rootp->deferredParamVarps()) {
        إذا (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
