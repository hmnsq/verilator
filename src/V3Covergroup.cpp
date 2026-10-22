// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Functional coverage implementation
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
// FUNCTIONAL COVERAGE TRANSFORMATIONS:
//      For each covergroup (AstClass with isCovergroup()):
//          For each coverpoint (AstCoverpoint):
//              Generate member variable for VerilatedCoverpoint
//              Generate initialization in constructor
//              Generate sample code in sample() method
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Covergroup.h"

#ضمن "V3Const.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3MemberMap.h"
#ضمن "V3UniqueNames.h"

#ضمن <bitset>
#ضمن <cmath>
#ضمن <deque>
#ضمن <set>
#ضمن <tuple>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

صنف Covergroup_سونپو_Valid_Visitor final : عام VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ثابت AstVar*, ثابت Ast_گرہ_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::سیٹ<ثابت AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ثابت Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;
    منطقي m_collecting = سچ;
    منطقي m_valid = سچ;

    فراغ ویکھو(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = خالی_سوچک;
        إذا (m_collecting) {
            ثابت Ast_گرہ_FTask* ثابت constructorp
                = VN_CAST(m_memberMap.لبھو_Member(nodep, "new"), گرہ_FTask);
            لكل (ثابت Ast_گرہ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                إذا (!varp || !varp->isClassMember() || varp->isDeclTyped()) استمر;
                ثابت AstClassRefDType* ثابت refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                إذا (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (!m_collecting) {
            ثابت AstVar* varp = خالی_سوچک;
            إذا (ثابت Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(nodep->lhsp(), گرہ_Var_Ref)) {
                varp = refp->varp();
            } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ثابت خودکار it = m_constructors.لبھو(varp);
            إذا (it != m_constructors.سرا() && (!m_ftaskp || m_ftaskp != it->دوجا)) {
                m_valid = خطأ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        إذا (m_collecting && nodep->varp()->name() == "type_option") {
            ثابت AstStructSel* ثابت selp = VN_CAST(nodep->backp(), StructSel);
            إذا (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.داخل_کرو(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    واضح Covergroup_سونپو_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = خطأ;
        إذا (!m_constructors.خالی_تھاواں()) iterateConst(nodep);
    }
    منطقي valid() ثابت { أرجع m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ثابت std::سیٹ<ثابت AstVar*>& mergeableTypeOptions() ثابت { أرجع m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

صنف Covergroup_اظہاریہ_Valid_Visitor final : عام VNVisitor {
    ثابت std::سیٹ<ثابت AstVar*>& m_sampleMembers;
    ثابت std::سیٹ<ثابت AstVar*>& m_constructorRefMembers;
    منطقي m_وچ_Coverage_عبارت = خطأ;
    منطقي m_sampleFormalAllowed = خطأ;

    فراغ جانچ_Sample_عبارت(Ast_گرہ* nodep) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = سچ;
        iterate_And_اگلا_Null(nodep);
    }

    فراغ جانچ_Coverage_عبارت(Ast_گرہ* nodep) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_وچ_Coverage_عبارت);
        m_وچ_Coverage_عبارت = سچ;
        iterate_And_اگلا_Null(nodep);
    }

    فراغ ویکھو(AstCoverpoint* nodep) override {
        جانچ_Sample_عبارت(nodep->exprp());
        جانچ_Sample_عبارت(nodep->iffp());
        iterate_And_اگلا_Null(nodep->binsp());
        iterate_And_اگلا_Null(nodep->optionsp());
    }
    فراغ ویکھو(AstCoverCross* nodep) override {
        iterate_And_اگلا_Null(nodep->itemsp());
        جانچ_Sample_عبارت(nodep->iffp());
        iterate_And_اگلا_Null(nodep->optionsp());
        iterate_And_اگلا_Null(nodep->binsp());
    }
    فراغ ویکھو(AstCoverCrossBin* nodep) override {
        iterate_And_اگلا_Null(nodep->selectp());
        جانچ_Sample_عبارت(nodep->iffp());
    }
    فراغ ویکھو(AstCoverCrossWith* nodep) override {
        iterate_And_اگلا_Null(nodep->selectp());
        جانچ_Coverage_عبارت(nodep->filterp());
        جانچ_Coverage_عبارت(nodep->countp());
    }
    فراغ ویکھو(AstCoverBinsof* nodep) override { جانچ_Coverage_عبارت(nodep->rangesp()); }
    فراغ ویکھو(AstCoverBin* nodep) override {
        جانچ_Coverage_عبارت(nodep->rangesp());
        جانچ_Sample_عبارت(nodep->iffp());
        جانچ_Coverage_عبارت(nodep->arraySizep());
        جانچ_Coverage_عبارت(nodep->transp());
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (!m_sampleFormalAllowed && m_sampleMembers.گنتی(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        إذا (m_وچ_Coverage_عبارت && m_constructorRefMembers.گنتی(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        إذا (m_وچ_Coverage_عبارت && nodep->taskp()) {
            منطقي invalidDirection = خطأ;
            لكل (Ast_گرہ* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
                إذا (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = سچ;
                    اكسر;
                }
            }
            إذا (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    Covergroup_اظہاریہ_Valid_Visitor(ثابت std::سیٹ<ثابت AstVar*>& sampleMembers,
                               ثابت std::سیٹ<ثابت AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    فراغ جانچ(Ast_گرہ* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

صنف Bin_لگاتار final {
عام:
    // MEMBERS
    uint32_t m_گنتی;  // Number of bins
    V3_نمبر m_lo;  // Lowest value of the first bin
    V3_نمبر m_stride;  // Number of values of each bin but the last
    V3_نمبر m_hi;  // Highest value of the last bin
    منطقي m_خالی_تھاواں = خطأ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_لگاتار(Ast_گرہ* nodep, صحيح چوڑائی, uint32_t گنتی)
        : m_گنتی{گنتی}
        , m_lo{nodep, چوڑائی}
        , m_stride{nodep, چوڑائی, 1}
        , m_hi{nodep, چوڑائی} {}
};

//######################################################################
// Functional coverage visitor

صنف FunctionalCoverageVisitor final : عام VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE
    std::سیٹ<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::سیٹ<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = خالی_سوچک;  // Current covergroup being processed
    Ast_گرہ_Module* m_unitp = خالی_سوچک;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = خالی_سوچک;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = خالی_سوچک;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = خالی_سوچک;  // Current sample() function
    AstIf* m_cpGuardp = خالی_سوچک;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = خالی_سوچک;  // Current constructor
    std::سمتیہ<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::سمتیہ<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::سمتیہ<Ast_Cg_Option_سونپو*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_بھار = 1;  // The covergroup's type_option.weight, a constant
    منطقي m_cg_ملاؤ_Instances = خطأ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    منطقي m_cg_May_ملاؤ = خطأ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ثابت std::سیٹ<ثابت AstVar*>& m_mergeableTypeOptions;
    صحيح m_cg_At_گھٹ_توں_گھٹ = 1;  // Covergroup option.at_least

    بنية EmbeddedEventTrigger final {
        File_سطر* eventFl;  // Clocking-event source location
        AstVar* ادھار_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType کنارا_Type;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_سطر* eventFl, AstVar* ادھار_Varp, AstVar* memberVarp,
                             VEdgeType کنارا_Type)
            : eventFl{eventFl}
            , ادھار_Varp{ادھار_Varp}
            , memberVarp{memberVarp}
            , کنارا_Type{کنارا_Type}
            , prevVarp{خالی_سوچک} {}
    };

    std::سیٹ<std::string> m_crossed_Cp_ناں;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    بنية BinRuns final {
        std::سمتیہ<Bin_لگاتار> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t گنتی = 0;  // Bins across all runs
        منطقي unsupported = خطأ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::سمتیہ<std::string> اقدار;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        منطقي single = خطأ;
    };
    بنية Cross_Bin_اقدار final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_گرہ_اظہاریہ* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ثابت Bin_لگاتار* runp = خالی_سوچک;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    بنية Bin_وسعت final {
        uint32_t پہلا = 0;  // First Normal index of the bin declaration
        uint32_t گنتی = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    بنية CoverpointBins final {
        uint32_t کل = 0;  // Number of Normal bins
        Ast_گرہ_اظہاریہ* exprp = خالی_سوچک;  // Sampled expression, for the value domain
        منطقي crossed = خطأ;  // Feeds a cross, which needs 'values'
        std::سمتیہ<Cross_Bin_اقدار> اقدار;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_وسعت> spans;  // Declared bin name -> index span
        Bin_وسعت implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_لگاتار> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ثابت AstCoverpoint*, std::سمتیہ<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_ناں m_sized_ناں{"__Vsized"};
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_detached_اقدار;  // Array-bin values m_cpBins refers to
    std::سیٹ<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    ورتوں CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, منطقي>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = خالی_سوچک;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(منطقي typeOption) {
        AstVar* ثابت varp = VN_AS(
            m_memberMap.لبھو_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        أرجع varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_سطر* fl, AstVar* optionVarp, ثابت std::string& member,
                               VAccess access) {
        ثابت AstMemberDType* ثابت memberp
            = VN_AS(m_memberMap.لبھو_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_گرہ_اظہاریہ* ثابت fromp = optionVarp->lifetime().isStatic()
                                       ? جديد AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ثابت selp = جديد AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_منزل_Enump());
        selp->did_چوڑائی(سچ);
        أرجع selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ساكن صحيح const_At_گھٹ_توں_گھٹ(ثابت AstConst* constp) {
        ثابت صحيح قدر = constp->منزل_SInt();
        إذا (قدر >= 0) أرجع قدر;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        أرجع 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ساكن ثابت AstConst* constOption(ثابت Ast_گرہ* valuep, VCoverOptionType optType) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) أرجع constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        أرجع خالی_سوچک;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    فراغ ہیٹھلا_Covergroup_Options() {
        منطقي ملاؤ_سیٹ = خطأ;  // The covergroup sets type_option.merge_instances
        m_cg_At_گھٹ_توں_گھٹ = 1;
        لكل (Ast_Cg_Option_سونپو* ثابت optp : m_cgOptions) {
            إذا (optp->optType() == VCoverOptionType::AT_گھٹ_توں_گھٹ) {
                إذا (ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_گھٹ_توں_گھٹ = const_At_گھٹ_توں_گھٹ(constp);
                }
                VL_DO_DANGLING(ڈالو_Deletep(optp->unlink_Fr_پٹھ()), optp);
                استمر;
            }
            File_سطر* ثابت fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ثابت AstConst* ثابت constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            إذا (optp->optType() == VCoverOptionType::ملاؤ_INSTANCES) {
                member = "merge_instances";
                m_cg_ملاؤ_Instances = !constp->عدد().isEqZero();
                ملاؤ_سیٹ = سچ;
            } وإلا إذا (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } وإلا {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::بھار, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                إذا (optp->typeOption()) m_cg_Type_بھار = constp->منزل_UInt();
            }
            Ast_سونپو* ثابت assignp = جديد Ast_سونپو{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_پٹھ()};
            إذا (optp->typeOption()) {
                m_covergroupp->جوڑو_Membersp(جديد AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ڈالو_Deletep(optp->unlink_Fr_پٹھ()), optp);
            } وإلا {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ڈالو_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        إذا (!ملاؤ_سیٹ && v3Global.opt.coverage_ملاؤ_Instances()) {
            File_سطر* ثابت fl = m_covergroupp->fileline();
            m_covergroupp->جوڑو_Membersp(جديد AstInitialStatic{
                fl, جديد Ast_سونپو{
                        fl, newOptionSel(fl, optionVar(سچ), "merge_instances", VAccess::WRITE),
                        جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}}});
            m_cg_ملاؤ_Instances = سچ;
        }
        m_cg_May_ملاؤ = m_cg_ملاؤ_Instances || m_mergeableTypeOptions.گنتی(optionVar(سچ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t شے_Database_بھار(Ast_گرہ* optionsp) ثابت {
        ثابت Ast_گرہ_اظہاریہ* weightp = خالی_سوچک;  // The option.weight in effect
        uint32_t type_بھار = 1;
        لكل (Ast_گرہ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            إذا (!(optp->optType() == VCoverOptionType::بھار)) استمر;
            // V3Width left type_option.weight a non-negative constant
            إذا (optp->typeOption()) {
                type_بھار = VN_AS(optp->valuep(), Const)->منزل_UInt();
            } وإلا {
                weightp = optp->valuep();
            }
        }
        إذا (m_cg_ملاؤ_Instances) أرجع type_بھار;
        إذا (!weightp) أرجع 1;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(weightp, Const)) أرجع constp->منزل_UInt();
        أرجع type_بھار;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    فراغ generate_شے_بھار(File_سطر* fl, AstVar* شے_Varp, Ast_گرہ* optionsp) {
        لكل (Ast_گرہ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            إذا (!(optp->optType() == VCoverOptionType::بھار)) استمر;
            إذا (!optp->typeOption()) {
                m_constructorp->جوڑو_Stmtsp(
                    شے_Call(fl, شے_Varp, VCMethod::COVERGROUP_بھار,
                             {optp->valuep()->clone_درخت(خطأ), file_سطر_Debug(optp->fileline())})
                        ->makeStmt());
            } وإلا إذا (m_cg_May_ملاؤ) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->جوڑو_Stmtsp(شے_Call(fl, شے_Varp, VCMethod::COVERGROUP_TYPE_بھار,
                                                   {optp->valuep()->clone_درخت(خطأ)})
                                              ->makeStmt());
            }
        }
    }

    فراغ processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.ناپ() << " coverpoints and "
                                           << m_coverCrosses.ناپ() << " crosses");

        m_crossed_Cp_ناں.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_ناں.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = خالی_سوچک;
        m_cg_Type_بھار = 1;
        m_cg_ملاؤ_Instances = خطأ;
        m_cg_May_ملاؤ = خطأ;

        ہیٹھلا_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        لكل (AstCoverCross* crossp : m_coverCrosses) {
            لكل (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (refp->exprp()) استمر;  // hierarchical ref: dropped in generateCrossCode
                إذا (m_coverpointMap.لبھو(refp->name()) == m_coverpointMap.سرا()) {
                    m_droppedCrosses.داخل_کرو(crossp);  // bare variable: drop this cross only
                } وإلا {
                    m_crossed_Cp_ناں.داخل_کرو(refp->name());
                }
            }
        }

        std::سمتیہ<Ast_گرہ*> pending;
        std::map<AstCoverpoint*, std::سمتیہ<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::سمتیہ<AstCoverpoint*>> inputs;
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            check_Bin_ناں(cpp);
            checkConstructedBins(cpp);
            إذا (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            إذا (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_خالی_تھاواں_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                استمر;
            }
            m_runtimePoints.داخل_کرو(cpp);
            pending.ڈالو_پٹھ(cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            إذا (m_droppedCrosses.گنتی(crossp)) استمر;
            لكل (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (refp->exprp()) استمر;
                ثابت خودکار نقطہ = m_coverpointMap.لبھو(refp->name());
                إذا (نقطہ != m_coverpointMap.سرا()) {
                    consumers[نقطہ->دوجا].ڈالو_پٹھ(crossp);
                    inputs[crossp].ڈالو_پٹھ(نقطہ->دوجا);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            إذا (crossp->exists([](ثابت AstCoverCrossWith*) { أرجع سچ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ڈالو_پٹھ(crossp);
            }
        }
        لكل (ناپ_t اگلا = 0; اگلا < pending.ناپ(); ++اگلا) {
            إذا (AstCoverpoint* ثابت pointp = VN_CAST(pending[اگلا], Coverpoint)) {
                لكل (AstCoverCross* ثابت crossp : consumers[pointp]) {
                    إذا (m_runtimeCrosses.emplace(crossp).دوجا) pending.ڈالو_پٹھ(crossp);
                }
            } وإلا {
                لكل (AstCoverpoint* ثابت pointp : inputs[VN_AS(pending[اگلا], CoverCross)]) {
                    إذا (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).دوجا) {
                        pending.ڈالو_پٹھ(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        لكل (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_کوڈ(cpp);

        // For each cross, generate sampling code
        لكل (AstCoverCross* crossp : m_coverCrosses) generate_Cross_کوڈ(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            إذا (!m_runtimePoints.گنتی(cpp)) استمر;
            m_constructorp->جوڑو_Stmtsp(شے_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_قدر_RELEASE)
                                          ->makeStmt());
        }
        لكل (Ast_گرہ_اظہاریہ* valuep : m_detached_اقدار) VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
        m_detached_اقدار.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_کوڈ();
    }

    ساكن constexpr ناپ_t قدر_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ساكن int64_t const_گنتی(ثابت AstConst* constp) {
        ثابت V3_نمبر& عدد = constp->عدد();
        إذا (constp->isSigned() && عدد.isNegative()) أرجع -1;
        أرجع عدد.most_سیٹ_ثنائی_ہندسہ_P1() > 32 ? INT64_MAX : static_cast<int64_t>(عدد.منزل_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ساكن منطقي check_گنتی(ثابت Ast_گرہ_اظہاریہ* countp, ثابت حرف* whatp, ثابت حرف* clausep) {
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        إذا (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        } وإلا إذا (constp && (constp->عدد().isFourState() || const_گنتی(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->عدد().isFourState() ? constp->عدد().ascii(خطأ)
                                                                  : constp->عدد().منزل_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        }
        أرجع سچ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ساكن uint32_t autoBinsRequested(ثابت AstCoverBin* binp) {
        ثابت AstConst* ثابت constp = VN_CAST(binp->arraySizep(), Const);
        إذا (!constp) أرجع 0;
        ثابت int64_t گنتی = const_گنتی(constp);
        أرجع گنتی < 1 || گنتی > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(گنتی);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ساكن منطقي isAutoBins(ثابت AstCoverBin* binp) {
        أرجع binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ساكن منطقي isSizedArray(ثابت AstCoverBin* binp) {
        أرجع binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ساكن AstCoverWith* binWith(ثابت AstCoverBin* binp) {
        أرجع VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ساكن منطقي isConstructedBins(ثابت AstCoverBin* binp) {
        أرجع isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ساكن Ast_گرہ* binRangesp(ثابت AstCoverBin* binp) {
        ثابت AstCoverWith* ثابت withp = binWith(binp);
        إذا (!withp) أرجع binp->rangesp();
        أرجع VN_IS(withp->subp(), CoverpointRef) ? خالی_سوچک : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    فراغ check_Bin_ناں(AstCoverpoint* coverpointp) {
        std::سیٹ<std::string> ناں;
        لكل (Ast_گرہ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            إذا (ناں.emplace(binp->name()).دوجا) استمر;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ڈالو_Deletep(binp->unlink_Fr_پٹھ()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    فراغ dropBins(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ڈالو_پٹھ(binp->name());
        VL_DO_DANGLING(ڈالو_Deletep(binp->unlink_Fr_پٹھ()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    منطقي check_Bins_Array_ناپ(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp, منطقي integral) {
        Ast_گرہ_اظہاریہ* ثابت sizep = binp->arraySizep();
        إذا (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } وإلا إذا (check_گنتی(sizep, "Bins array size", "19.5.1")) {
            إذا (integral) أرجع سچ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ڈالو_Deletep(sizep->unlink_Fr_پٹھ()), sizep);
            أرجع خطأ;
        }
        dropBins(coverpointp, binp);
        أرجع خطأ;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    فراغ checkConstructedBins(AstCoverpoint* coverpointp) {
        ثابت منطقي integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        لكل (Ast_گرہ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            إذا (!isConstructedBins(binp)) استمر;
            إذا (isSizedArray(binp) && !check_Bins_Array_ناپ(coverpointp, binp, integral)) استمر;
            إذا (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                إذا (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    إذا (binp->binsType().bin_Is_عام()) coverpointp->user2(سچ);
                    dropBins(coverpointp, binp);
                }
                استمر;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ثابت منطقي single = !binp->binsType().bin_Is_عام() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_ہور()
                                         << "... Suggest a larger --coverage-max-bins");
            إذا (single) {
                Ast_گرہ_اظہاریہ* ثابت sizep = binp->arraySizep();
                VL_DO_DANGLING(ڈالو_Deletep(sizep->unlink_Fr_پٹھ()), sizep);
                binp->isArray(خطأ);
                استمر;
            }
            إذا (binp->binsType().bin_Is_عام()) coverpointp->user2(سچ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ساكن منطقي withCandidatesOver(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        ثابت صحيح چوڑائی = لگاتار_چوڑائی(exprp);
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs;
        إذا (!binRangesp(binp)) runs = coverpoint_اقدار(binp, exprp);
        ثابت خودکار constant
            = [](ثابت Ast_گرہ* nodep) { أرجع VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        لكل (Ast_گرہ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            إذا (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                أرجع خطأ;
            }
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            إذا (!resolve_قدر(rangep, exprp, سچ, binp->isWildcard(), range)
                || cross_Range_خالی_تھاواں(range)) {
                استمر;
            }
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> لبھیا{{range.lo, range.hi}};
            إذا (range.wildcard) {  // checkConstructedBins bounded the runs
                لبھیا.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), لبھیا);
            }
            لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& لگاتار : لبھیا) {
                runs.emplace_پٹھ(V3_نمبر{rangep, چوڑائی, لگاتار.پہلا},
                                  V3_نمبر{rangep, چوڑائی, لگاتار.دوجا});
            }
        }
        إذا (!isSizedArray(binp)) {  // The union of the values
            std::ترتیب_دیو(runs.begin(), runs.سرا(), [](ثابت خودکار& lhs, ثابت خودکار& rhs) {
                أرجع cross_قدر_Less(lhs.پہلا, rhs.پہلا);
            });
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> merged;
            لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& لگاتار : runs) {
                إذا (merged.خالی_تھاواں() || cross_قدر_Less(merged.پٹھ().دوجا, لگاتار.پہلا)) {
                    merged.ڈالو_پٹھ(لگاتار);
                } وإلا إذا (cross_قدر_Less(merged.پٹھ().دوجا, لگاتار.دوجا)) {
                    merged.پٹھ().دوجا = لگاتار.دوجا;
                }
            }
            runs = std::move(merged);
        }
        uint64_t گنتی = 0;
        لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& لگاتار : runs) {
            V3_نمبر وسعت{exprp, چوڑائی};
            وسعت.opSub(لگاتار.دوجا, لگاتار.پہلا);
            إذا (وسعت.most_سیٹ_ثنائی_ہندسہ_P1() > 32) أرجع سچ;  // More than 2**32 values
            گنتی += وسعت.منزل_UQuad() + 1;
            إذا (گنتی > (uint64_t{1} << 32)) أرجع سچ;
        }
        أرجع خطأ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ساكن ناپ_t sizedWildcardRuns(ثابت AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs;
        لكل (Ast_گرہ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            إذا (!VN_IS(rangep, Const)) استمر;  // A range, or a value known at construction
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            إذا (resolve_قدر(rangep, exprp, سچ, سچ, range) && !cross_Range_خالی_تھاواں(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        أرجع runs.ناپ();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    فراغ checkAutomaticBins(AstCoverpoint* coverpointp, ثابت Ast_گرہ_اظہاریہ* exprp) {
        لكل (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (cbinp->binsType() != VCoverBinsType::BINS_AUTO) استمر;
            ثابت AstConst* ثابت constp = VN_CAST(cbinp->arraySizep(), Const);
            إذا (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } وإلا إذا (const_گنتی(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->عدد().منزل_Decimal_S());
            } وإلا إذا (const_گنتی(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->عدد().منزل_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_ہور() << "... Suggest a larger --coverage-max-bins");
            } وإلا إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    فراغ کڈھو_شے_Options(Ast_گرہ* optionsp, صحيح& at_گھٹ_توں_گھٹ_باہرلا, صحيح& auto_Bin_Max_باہرلا) {
        at_گھٹ_توں_گھٹ_باہرلا = m_cg_At_گھٹ_توں_گھٹ;
        auto_Bin_Max_باہرلا = -1;  // -1 = not set at coverpoint level
        لكل (Ast_گرہ* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ثابت optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            إذا (!(optp->optType() == VCoverOptionType::AT_گھٹ_توں_گھٹ
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                استمر;
            }
            ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType());
            إذا (!constp) استمر;
            إذا (optp->optType() == VCoverOptionType::AT_گھٹ_توں_گھٹ) {
                at_گھٹ_توں_گھٹ_باہرلا = const_At_گھٹ_توں_گھٹ(constp);
            } وإلا {
                auto_Bin_Max_باہرلا = constp->منزل_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        إذا (auto_Bin_Max_باہرلا < 0) {
            إذا (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_باہرلا = m_covergroupp->cgAutoBinMax();
            } وإلا {
                auto_Bin_Max_باہرلا = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    فراغ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp,
                            ثابت AstEnumDType* enump) {
        File_سطر* ثابت fl = coverpointp->fileline();
        لكل (ثابت Ast_Enum_شے* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            AstConst* ثابت lop = new_قدر_Const(fl, VN_AS(itemp->valuep(), Const)->عدد(), exprp);
            AstInsideRange* ثابت rangep = جديد AstInsideRange{fl, lop, lop->clone_درخت(خطأ)};
            rangep->dtypeFrom(exprp);
            coverpointp->جوڑو_Binsp(
                جديد AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, خطأ, خطأ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    فراغ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp, صحيح autoBinMax) {
        إذا (coverpointp->user2()) أرجع;  // Declared bins, ignored, leave no bins
        لكل (Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت VCoverBinsType قسم = VN_AS(nodep, CoverBin)->binsType();
            إذا (قسم != VCoverBinsType::BINS_IGNORE && قسم != VCoverBinsType::BINS_ILLEGAL)
                أرجع;
        }
        إذا (ثابت AstEnumDType* ثابت enump
            = VN_CAST(exprp->dtypep()->skip_Ref_منزل_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            أرجع;
        }
        ثابت صحيح چوڑائی = exprp->چوڑائی();
        uint32_t گنتی = چوڑائی < 31 ? std::min<uint32_t>(uint32_t{1} << چوڑائی, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        إذا (!گنتی) أرجع;
        إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            أرجع;
        }
        إذا (گنتی > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_ہور()
                                              << "... Suggest a larger --coverage-max-bins");
            گنتی = v3Global.opt.coverageMaxBins();
        }
        File_سطر* ثابت fl = coverpointp->fileline();
        coverpointp->جوڑو_Binsp(جديد AstCoverBin{fl, "auto", جديد AstConst{fl, گنتی},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ساكن string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.سرا(), '[', '_');
        std::replace(name.begin(), name.سرا(), ']', '_');
        أرجع name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_منزل_عارضی(Ast_گرہ_اظہاریہ* iffp, ثابت string& عارضی_Name) {
        File_سطر* ثابت fl = iffp->fileline();
        AstVar* ثابت iffVarp
            = جديد AstVar{fl, VVarType::BLOCKTEMP, عارضی_Name, iffp->لبھو_ثنائی_ہندسہ_DType()};
        iffVarp->funcLocal(سچ);
        m_sampleFuncp->جوڑو_Stmtsp(iffVarp);
        iffp->unlink_Fr_پٹھ();
        m_sampleFuncp->جوڑو_Stmtsp(
            جديد Ast_سونپو{fl, جديد AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        أرجع جديد AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    فراغ جوڑو_Sample_Stmt(Ast_گرہ* stmtp) {
        إذا (m_cpGuardp) {
            m_cpGuardp->جوڑو_Thensp(stmtp);
        } وإلا {
            m_sampleFuncp->جوڑو_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_قدر_Var(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp) {
        // Check if already created
        إذا (AstVar* ثابت prevVarp = VN_CAST(coverpointp->user1p(), Var)) أرجع prevVarp;

        // Create variable to store previous sampled value
        ثابت string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = جديد AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->جوڑو_Membersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_گرہ_اظہاریہ* ثابت initExprp
            = جديد AstConst{prevVarp->fileline(), AstConst::Widthed_قدر{}, prevVarp->چوڑائی(), 0};
        Ast_گرہ_Stmt* ثابت initStmtp = جديد Ast_سونپو{
            prevVarp->fileline(), جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->جوڑو_Stmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        أرجع prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ثابت string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = جديد AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->جوڑو_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_گرہ_Stmt* ثابت initStmtp = جديد Ast_سونپو{
            stateVarp->fileline(), جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            جديد AstConst{stateVarp->fileline(), AstConst::Widthed_قدر{}, 8, 0}};
        m_constructorp->جوڑو_Stmtsp(initStmtp);

        أرجع stateVarp;
    }

    فراغ generate_Coverpoint_کوڈ(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_گرہ_اظہاریہ* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        صحيح at_گھٹ_توں_گھٹ_قدر;
        صحيح autoBinMax;
        کڈھو_شے_Options(coverpointp->optionsp(), at_گھٹ_توں_گھٹ_قدر, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_گھٹ_توں_گھٹ_قدر << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        إذا (Ast_گرہ_اظہاریہ* ثابت iffp = coverpointp->iffp()) {
            m_cpGuardp = جديد AstIf{iffp->fileline(), iffp->unlink_Fr_پٹھ()};
        }

        AstVar* ثابت قدر_Varp = جديد AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        قدر_Varp->funcLocal(سچ);
        m_sampleFuncp->جوڑو_Stmtsp(قدر_Varp);
        exprp->unlink_Fr_پٹھ();
        جوڑو_Sample_Stmt(جديد Ast_سونپو{
            coverpointp->fileline(),
            جديد AstVarRef{coverpointp->fileline(), قدر_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(جديد AstVarRef{coverpointp->fileline(), قدر_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_گھٹ_توں_گھٹ_قدر);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        إذا (m_cpGuardp) m_sampleFuncp->جوڑو_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_گرہ_اظہاریہ* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp,
                                       File_سطر* fl) {
        Ast_گرہ_اظہاریہ* anyBinMatchp = خالی_سوچک;
        لكل (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                استمر;
            // The values of constructed bins are known at construction; see emitSizedSample
            إذا (isConstructedBins(cbinp)) استمر;
            إذا (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                إذا (anyBinMatchp) VL_DO_DANGLING(ڈالو_Deletep(anyBinMatchp), anyBinMatchp);
                أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}};
            }
            Ast_گرہ_اظہاریہ* ثابت binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? جديد AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        أرجع anyBinMatchp ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_گرہ_اظہاریہ*>(جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ساكن منطقي coverpointHasStateExclusions(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            إذا (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    ساكن منطقي coverpointHasConstructedBins(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            إذا (isConstructedBins(VN_AS(nodep, CoverBin))) أرجع سچ;
        }
        أرجع خطأ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ساكن منطقي coverpoint_Has_خالی_تھاواں_Bins(ثابت AstCoverpoint* coverpointp) {
        Ast_گرہ_اظہاریہ* ثابت exprp = coverpointp->exprp();
        لكل (Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            إذا (!binp->binsType().bin_Is_عام() || binp->transp() || !binp->rangesp()) استمر;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            إذا ((binp->isArray() && binp->isWildcard()) || binWith(binp)) استمر;
            منطقي خالی_تھاواں = سچ;
            لكل (Ast_گرہ* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                إذا (binp->isArray() && VN_IS(valuep, InsideRange)) استمر;
                Cross_قدر_Range range{valuep, resolve_چوڑائی(valuep, exprp)};
                ثابت منطقي none = resolve_قدر(valuep, exprp, سچ, binp->isWildcard(), range)
                                  && cross_Range_خالی_تھاواں(range);
                إذا (binp->isArray() && none) أرجع سچ;
                خالی_تھاواں &= none;
            }
            إذا (خالی_تھاواں && !binp->isArray()) أرجع سچ;
        }
        أرجع خطأ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ساكن منطقي coverpointHasTransition(AstCoverpoint* coverpointp) {
        لكل (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            إذا (VN_AS(binp, CoverBin)->transp()) أرجع سچ;
        }
        أرجع خطأ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ساكن AstBasicDType* basicDType(File_سطر* fl, VBasicDTypeKwd kwd) {
        أرجع v3Global.rootp()->typeTablep()->لبھو_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_سطر* fl, uint32_t کامیابی_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[کامیابی_Bound];
        إذا (!typep) {
            typep = جديد AstCoverpointDType{fl, کامیابی_Bound};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(typep);
        }
        أرجع typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ثابت {
        أرجع VIdProtect::protect_الفاظ_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ثابت {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ثابت std::string& libname = m_unitp->libname();
        ثابت std::string name = m_covergroupp->dtypeName(سچ);
        أرجع libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    فراغ generateInstanceAttach() {
        File_سطر* ثابت fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->جوڑو_Membersp(m_cgInstVarp);

        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_ملاؤ ? ", true" : ", false") + ")")},
                     /*usePtr=*/خطأ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ثابت بھار_Addrp = جديد AstCExpr{fl, "&"};
        بھار_Addrp->جوڑو(newOptionSel(fl, optionVar(خطأ), "weight", VAccess::READ));
        m_constructorp->جوڑو_Stmtsp(شے_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_بھار,
                                           {بھار_Addrp, file_سطر_Debug(fl)}, /*usePtr=*/خطأ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_سطر* fl, VCMethod طریقہ) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ثابت instp
            = جديد AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(خطأ);
        instp->dtype_سیٹ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, instp, طریقہ};
        callp->usePtr(سچ);
        أرجع callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_سونپو* make_شے_Create(File_سطر* fl, AstVar* شے_Varp, VCMethod طریقہ) {
        AstCMethodHard* ثابت createp = instanceCall(fl, طریقہ);
        createp->dtypep(شے_Varp->dtypep());
        أرجع جديد Ast_سونپو{fl, memberRef(fl, شے_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    بنية RangeBounds final {
        Ast_گرہ* loNodep = خالی_سوچک;  // low bound: AstConst or AstUnbounded
        Ast_گرہ* hiNodep = خالی_سوچک;  // high bound: AstConst or AstUnbounded
        منطقي loUnbounded() ثابت { أرجع VN_IS(loNodep, Unbounded); }
        منطقي hiUnbounded() ثابت { أرجع VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ثابت { أرجع VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ثابت { أرجع VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ساكن منطقي constRangeBounds(Ast_گرہ* rp, RangeBounds& rb) {
        إذا (AstInsideRange* ثابت irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } وإلا إذا (AstConst* ثابت cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } وإلا {
            أرجع خطأ;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ثابت lc = rb.loConstp();
        AstConst* ثابت hc = rb.hiConstp();
        إذا ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) أرجع خطأ;
        إذا ((lc && lc->عدد().isFourState()) || (hc && hc->عدد().isFourState())) أرجع خطأ;
        أرجع سچ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ساكن منطقي کڈھو_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::سمتیہ<std::pair<uint64_t, uint64_t>>& باہرلا) {
        إذا (!cbinp->rangesp()) أرجع خطأ;
        لكل (Ast_گرہ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            إذا (!constRangeBounds(rp, rb)) أرجع خطأ;
            إذا ((rb.loConstp() && rb.loConstp()->چوڑائی() > 64)
                || (rb.hiConstp() && rb.hiConstp()->چوڑائی() > 64)) {
                أرجع خطأ;  // Use the safe slot-count bound for wide values.
            }
            ثابت uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->منزل_UQuad();
            ثابت uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->منزل_UQuad();
            إذا (lo > hi) أرجع خطأ;
            باہرلا.emplace_پٹھ(lo, hi);
        }
        أرجع سچ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    منطقي appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_گرہ_اظہاریہ* exprp,
                             std::سمتیہ<std::سمتیہ<std::pair<uint64_t, uint64_t>>>& bins,
                             صحيح& تھاں_گنتی) {
        إذا (isAutoBins(cbinp)) {
            ++تھاں_گنتی;
            bins.ڈالو_پٹھ({{0, maxVal}});
            أرجع exprp->چوڑائی() <= 64;
        }
        إذا (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            صحيح elements = 0;
            لكل (ثابت Ast_گرہ* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            تھاں_گنتی += std::max(1, elements);
            أرجع خطأ;
        }
        إذا (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++تھاں_گنتی;
            إذا (exprp->isSigned() || exprp->چوڑائی() > 64) أرجع خطأ;
            ثابت BinRuns runs = wildcardBinRuns(cbinp, exprp, خطأ);
            إذا (runs.unsupported) أرجع خطأ;
            std::سمتیہ<std::pair<uint64_t, uint64_t>> ivs;
            لكل (ثابت Bin_لگاتار& لگاتار : runs.runs) {
                ivs.emplace_پٹھ(لگاتار.m_lo.منزل_UQuad(), لگاتار.m_hi.منزل_UQuad());
            }
            bins.ڈالو_پٹھ(std::move(ivs));
            أرجع سچ;
        }
        إذا (cbinp->isArray()) أرجع appendArrayBinCrossSlots(cbinp, exprp, bins, تھاں_گنتی);
        // Non-array bin: one slot covering the union of its intervals.
        ++تھاں_گنتی;
        std::سمتیہ<std::pair<uint64_t, uint64_t>> ivs;
        إذا (cbinp->isWildcard() || !کڈھو_Range_Intervals(cbinp, maxVal, ivs)) أرجع خطأ;
        bins.ڈالو_پٹھ(std::move(ivs));
        أرجع سچ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    منطقي appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_گرہ_اظہاریہ* exprp,
                                  std::سمتیہ<std::سمتیہ<std::pair<uint64_t, uint64_t>>>& bins,
                                  صحيح& تھاں_گنتی) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        منطقي exact = !exprp->isSigned() && !cbinp->isWildcard();
        لكل (Ast_گرہ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++تھاں_گنتی;
            RangeBounds rb;
            Cross_قدر_Range range{rp, resolve_چوڑائی(rp, exprp)};
            إذا (!exact || !constRangeBounds(rp, rb)
                || !resolve_قدر(rp, exprp, سچ, خطأ, range)) {
                exact = خطأ;
                استمر;
            }
            إذا (cross_Range_خالی_تھاواں(range)) استمر;  // Its bin, if any, holds no value
            إذا (range.hi.most_سیٹ_ثنائی_ہندسہ_P1() > 64) {
                exact = خطأ;
                استمر;
            }
            bins.ڈالو_پٹھ({{range.lo.منزل_UQuad(), range.hi.منزل_UQuad()}});
        }
        أرجع exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    صحيح compute_کامیابی_List_Bound(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp, منطقي crossFed) {
        إذا (!crossFed) أرجع 1;
        ثابت صحيح چوڑائی = exprp->چوڑائی();
        ثابت uint64_t maxVal = (چوڑائی >= 64) ? UINT64_MAX : ((1ULL << چوڑائی) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::سمتیہ<std::سمتیہ<std::pair<uint64_t, uint64_t>>> bins;
        صحيح تھاں_گنتی = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        منطقي exact = !exprp->isSigned();
        لكل (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (!cbinp->binsType().bin_Is_عام())
                استمر;  // ignore/illegal/default: not hit-listed
            إذا (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, تھاں_گنتی)) exact = خطأ;
        }
        إذا (!exact) أرجع std::max(1, تھاں_گنتی);
        إذا (bins.خالی_تھاواں()) أرجع 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::سمتیہ<uint64_t> pts;
        لكل (ثابت خودکار& b : bins)
            لكل (ثابت خودکار& iv : b) pts.ڈالو_پٹھ(iv.پہلا);
        std::ترتیب_دیو(pts.begin(), pts.سرا());
        pts.erase(std::unique(pts.begin(), pts.سرا()), pts.سرا());
        صحيح maxOverlap = 1;
        لكل (ثابت uint64_t p : pts) {
            صحيح cnt = 0;
            لكل (ثابت خودکار& b : bins) {
                لكل (ثابت خودکار& iv : b) {
                    إذا (iv.پہلا <= p && p <= iv.دوجا) {
                        ++cnt;
                        اكسر;  // count each bin at most once
                    }
                }
            }
            إذا (cnt > maxOverlap) maxOverlap = cnt;
        }
        أرجع maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_سطر* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, access};
        refp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}});
        أرجع refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* شے_Call(File_سطر* fl, AstVar* varp, VCMethod طریقہ,
                             ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*>& دلیلاں = {}, منطقي usePtr = سچ) {
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, memberRef(fl, varp), طریقہ};
        لكل (Ast_گرہ_اظہاریہ* ثابت argp : دلیلاں) callp->جوڑو_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_سیٹ_Void();
        أرجع callp;
    }

    // An unsigned integer argument.
    ساكن AstConst* cnum(File_سطر* fl, uint32_t قدر) { أرجع جديد AstConst{fl, قدر}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ساكن AstCExpr* ctext(File_سطر* fl, ثابت std::string& متن) {
        أرجع جديد AstCExpr{fl, متن};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ساكن std::string quoted(ثابت std::string& متن) {
        أرجع "\"" + V3_باہرلا_Formatter::quoteNameControls(متن) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ساكن AstCExpr* file_سطر_Debug(File_سطر* fl) {
        ثابت std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        أرجع ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::منزل_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ساكن منطقي checkArrayBinElement(AstCoverBin* arrayBinp, Ast_گرہ* rangep, منطقي report = سچ) {
        إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            ثابت AstConst* ثابت minp = VN_CAST(irp->lhsp(), Const);
            ثابت AstConst* ثابت maxp = VN_CAST(irp->rhsp(), Const);
            إذا ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                إذا (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                أرجع خطأ;
            }
            إذا ((minp && minp->عدد().isFourState()) || (maxp && maxp->عدد().isFourState())) {
                إذا (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                أرجع خطأ;
            }
        } وإلا إذا (!VN_IS(rangep, Const)) {
            إذا (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            أرجع خطأ;
        }
        أرجع سچ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::سمتیہ<Ast_گرہ_اظہاریہ*> کڈھو_Array_اقدار(AstCoverBin* arrayBinp, Ast_گرہ_اظہاریہ* exprp,
                                                 منطقي& unsupported_باہرلا) {
        unsupported_باہرلا = خطأ;
        ثابت صحيح چوڑائی = exprp->چوڑائی();
        ثابت uint64_t maxVal = (چوڑائی >= 64) ? UINT64_MAX : ((1ULL << چوڑائی) - 1);
        std::سمتیہ<Ast_گرہ_اظہاریہ*> اقدار;
        لكل (Ast_گرہ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep)) أرجع اقدار;
            إذا (AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
                ثابت منطقي loUnb = VN_IS(irp->lhsp(), Unbounded);
                ثابت منطقي hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ثابت uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->منزل_UQuad();
                ثابت uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->منزل_UQuad();
                إذا (hi < lo) استمر;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ثابت uint64_t وسعت = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                إذا (وسعت >= v3Global.opt.coverageMaxRealBins()
                    || اقدار.ناپ() + وسعت + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_ہور()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_باہرلا = سچ;
                    لكل (Ast_گرہ_اظہاریہ* ثابت vp : اقدار) VL_DO_DANGLING(ڈالو_Deletep(vp), vp);
                    اقدار.clear();
                    أرجع اقدار;
                }
                لكل (uint64_t v = lo; v <= hi; ++v)
                    اقدار.ڈالو_پٹھ(جديد AstConst{irp->fileline(), AstConst::Widthed_قدر{}, چوڑائی,
                                                  static_cast<uint32_t>(v)});
            } وإلا {
                اقدار.ڈالو_پٹھ(VN_AS(rangep->clone_درخت(خطأ), گرہ_اظہاریہ));
            }
        }
        أرجع اقدار;
    }

    ساكن صحيح لگاتار_چوڑائی(ثابت Ast_گرہ_اظہاریہ* exprp) { أرجع exprp->چوڑائی() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    منطقي autoBinRuns(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp, BinRuns& باہرلا) {
        ثابت uint32_t requested = autoBinsRequested(binp);
        إذا (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع خطأ;
        ثابت صحيح چوڑائی = exprp->چوڑائی();
        ثابت صحيح arithmetic_چوڑائی = لگاتار_چوڑائی(exprp);
        ثابت uint32_t گنتی
            = چوڑائی < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << چوڑائی, requested))
                  : requested;
        Bin_لگاتار لگاتار{binp, arithmetic_چوڑائی, گنتی};
        V3_نمبر کل{binp, arithmetic_چوڑائی};
        کل.سیٹ_ثنائی_ہندسہ(چوڑائی, 1);
        لگاتار.m_stride.opDiv(کل, V3_نمبر{binp, arithmetic_چوڑائی, گنتی});
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(binp, چوڑائی, exprp->isSigned(), arithmetic_چوڑائی);
        لگاتار.m_lo = domain.lo;
        لگاتار.m_hi = domain.hi;
        باہرلا.runs.ڈالو_پٹھ(std::move(لگاتار));
        باہرلا.گنتی = گنتی;
        أرجع سچ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_گرہ_اظہاریہ* exprp) {
        BinRuns باہرلا;
        ثابت صحيح چوڑائی = لگاتار_چوڑائی(exprp);
        لكل (Ast_گرہ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep)) أرجع باہرلا;
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            منطقي خالی_تھاواں = سچ;
            إذا (!resolve_قدر(rangep, exprp, سچ, خطأ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا {
                خالی_تھاواں = cross_Range_خالی_تھاواں(range);
            }
            إذا (خالی_تھاواں && irp) استمر;  // A range without values contributes no bins
            Bin_لگاتار لگاتار{rangep, چوڑائی, 1};
            لگاتار.m_خالی_تھاواں = خالی_تھاواں;
            uint64_t گنتی = 1;
            إذا (!خالی_تھاواں) {
                لگاتار.m_lo.op_سونپو(range.lo);
                لگاتار.m_hi.op_سونپو(range.hi);
                V3_نمبر وسعت{rangep, چوڑائی};
                وسعت.opSub(لگاتار.m_hi, لگاتار.m_lo);
                // Wider spans exceed any limit
                گنتی = وسعت.most_سیٹ_ثنائی_ہندسہ_P1() > 32 ? UINT64_MAX : وسعت.منزل_UQuad() + 1;
            }
            إذا (گنتی > v3Global.opt.coverageMaxBins() - باہرلا.گنتی) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_ہور()
                                                << "... Suggest a larger --coverage-max-bins");
                باہرلا.runs.clear();
                باہرلا.گنتی = 0;
                باہرلا.unsupported = سچ;
                أرجع باہرلا;
            }
            لگاتار.m_گنتی = static_cast<uint32_t>(گنتی);
            باہرلا.گنتی += لگاتار.m_گنتی;
            باہرلا.runs.ڈالو_پٹھ(std::move(لگاتار));
        }
        أرجع باہرلا;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ساكن BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_گرہ_اظہاریہ* exprp, منطقي report) {
        BinRuns باہرلا;
        ثابت صحيح چوڑائی = لگاتار_چوڑائی(exprp);
        ثابت V3_نمبر one{arrayBinp, چوڑائی, 1};
        // Disjoint runs of the values, in value order, each value once
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> spans;
        uint64_t گنتی = 0;  // Values of 'spans'
        لكل (Ast_گرہ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep, report)) {
                باہرلا.unsupported = سچ;
                أرجع باہرلا;
            }
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            إذا (!resolve_قدر(rangep, exprp, سچ, سچ, range)) {
                إذا (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                استمر;
            }
            إذا (cross_Range_خالی_تھاواں(range)) استمر;
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> لبھیا;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), لبھیا);
            لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& لگاتار : لبھیا) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_پٹھ(V3_نمبر{rangep, چوڑائی, لگاتار.پہلا},
                                   V3_نمبر{rangep, چوڑائی, لگاتار.دوجا});
            }
            std::ترتیب_دیو(spans.begin(), spans.سرا(), [](ثابت خودکار& lhs, ثابت خودکار& rhs) {
                أرجع cross_قدر_Less(lhs.پہلا, rhs.پہلا);
            });
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> merged;
            لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& وسعت : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_نمبر before{rangep, چوڑائی};
                before.opSub(وسعت.پہلا, one);
                إذا (merged.خالی_تھاواں() || cross_قدر_Less(merged.پٹھ().دوجا, before)) {
                    merged.ڈالو_پٹھ(وسعت);
                } وإلا إذا (cross_قدر_Less(merged.پٹھ().دوجا, وسعت.دوجا)) {
                    merged.پٹھ().دوجا = وسعت.دوجا;
                }
            }
            گنتی = 0;
            لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& وسعت : merged) {
                V3_نمبر ناپ{rangep, چوڑائی};
                ناپ.opSub(وسعت.دوجا, وسعت.پہلا);
                // Beyond 2^32 values exceed any limit
                گنتی += ناپ.most_سیٹ_ثنائی_ہندسہ_P1() > 32 ? uint64_t{1} << 33 : ناپ.منزل_UQuad() + 1;
                إذا (گنتی > v3Global.opt.coverageMaxBins()) اكسر;
            }
            spans = std::move(merged);
            إذا (گنتی > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                باہرلا.single = !arrayBinp->binsType().bin_Is_عام();
                إذا (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (باہرلا.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_ہور()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                باہرلا.unsupported = سچ;
                أرجع باہرلا;
            }
        }
        لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& وسعت : spans) {
            باہرلا.runs.emplace_پٹھ(arrayBinp, چوڑائی, 0);
            Bin_لگاتار& لگاتار = باہرلا.runs.پٹھ();
            لگاتار.m_lo = وسعت.پہلا;
            لگاتار.m_hi = وسعت.دوجا;
            // A bin for each value, named by it in the coverpoint's type
            V3_نمبر قدر = وسعت.پہلا;
            طالما (سچ) {
                ثابت V3_نمبر typed{arrayBinp, exprp->چوڑائی(), قدر};
                باہرلا.اقدار.ڈالو_پٹھ(exprp->isSigned() ? typed.منزل_Decimal_S() : typed.منزل_Decimal_U());
                ++لگاتار.m_گنتی;
                إذا (قدر.isCaseEq(وسعت.دوجا)) اكسر;
                V3_نمبر اگلا{arrayBinp, چوڑائی};
                قدر = اگلا.op_جوڑو(قدر, one);
            }
        }
        باہرلا.گنتی = static_cast<uint32_t>(گنتی);
        أرجع باہرلا;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    منطقي binRunsFor(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp, BinRuns& باہرلا) {
        إذا (isAutoBins(binp)) {
            إذا (!autoBinRuns(binp, exprp, باہرلا)) باہرلا.unsupported = سچ;
            أرجع سچ;
        }
        إذا (binp->isArray() && binp->isWildcard()) {
            إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_گرہ_اظہاریہ* ثابت falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ڈالو_Deletep(falsep), falsep);
                باہرلا.unsupported = سچ;
            } وإلا {
                باہرلا = wildcardBinRuns(binp, exprp, سچ);
                إذا (باہرلا.single) {
                    binp->isArray(خطأ);  // Generates as one bin
                    أرجع خطأ;
                }
            }
            أرجع سچ;
        }
        إذا (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع خطأ;
        }
        باہرلا = arrayBinRuns(binp, exprp);
        أرجع سچ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_گرہ_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t گنتی, uint32_t declared,
                           ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*>& اقدار = {},
                           ثابت std::سمتیہ<std::string>& قدر_ناں = {}) {
        File_سطر* ثابت fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ثابت uint32_t عام_گنتی
            = binp->binsType().bin_Is_عام() ? static_cast<uint32_t>(گنتی < 0 ? 1 : گنتی) : 0;
        ثابت Bin_وسعت وسعت{bins.کل, عام_گنتی, declared};
        إذا (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = وسعت;  // Selected by bin, see implicitAutoBinSpan
        } وإلا {
            bins.spans.emplace(binp->name(), وسعت);
        }
        bins.کل += عام_گنتی;
        لكل (uint32_t i = 0; bins.crossed && i < عام_گنتی; ++i) {
            bins.اقدار.ڈالو_پٹھ({binp, اقدار.خالی_تھاواں() ? خالی_سوچک : اقدار[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت منطقي single = گنتی < 0;
        إذا (!قدر_ناں.خالی_تھاواں()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_گرہ_Stmt* stmtsp = خالی_سوچک;
            لكل (ثابت std::string& قدر : قدر_ناں) {
                ثابت std::string name
                    = VIdProtect::protect_الفاظ_If(binp->name(), prot) + "[" + قدر + "]";
                stmtsp = Ast_گرہ::جوڑو_اگلا(
                    stmtsp,
                    شے_Call(fl, cpVarp, VCMethod::COVERGROUP_جوڑو_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_سیٹ_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->پہلا_کالم()))})
                        ->makeStmt());
            }
            أرجع stmtsp;
        }
        std::سمتیہ<Ast_گرہ_اظہاریہ*> دلیلاں{ctext(fl, binp->binsType().bin_سیٹ_Enum())};
        إذا (!single) دلیلاں.ڈالو_پٹھ(cnum(fl, static_cast<uint32_t>(گنتی)));  // value array bin
        دلیلاں.ڈالو_پٹھ(ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))));
        دلیلاں.ڈالو_پٹھ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        دلیلاں.ڈالو_پٹھ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        دلیلاں.ڈالو_پٹھ(cnum(fl, static_cast<uint32_t>(fl->پہلا_کالم())));
        أرجع شے_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_جوڑو_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_جوڑو_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_جوڑو_ARRAY_NAMER,
                        دلیلاں)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    بنية Conv_Bin_نشانہ final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        منطقي is_عام;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_گرہ_Stmt* make_Runtime_Bin_کامیابی(File_سطر* fl, AstVar* cpVarp, Ast_گرہ_اظہاریہ* idxp,
                                   منطقي is_عام) {
        أرجع شے_Call(fl, cpVarp,
                        is_عام ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_ریکارڈ_کامیابی,
                        {idxp})
            ->makeStmt();
    }
    Ast_گرہ_Stmt* make_Runtime_Bin_کامیابی(File_سطر* fl, ثابت Conv_Bin_نشانہ& tgt) {
        أرجع make_Runtime_Bin_کامیابی(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_عام);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_گرہ_اظہاریہ* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_گرہ_اظہاریہ* condp) {
        File_سطر* ثابت fl = binp->fileline();
        إذا (binp->iffp()) condp = جديد AstLogAnd{fl, binp->iffp()->clone_درخت(خطأ), condp};
        ثابت خودکار excluded = m_excludedVars.لبھو(cpVarp);
        إذا (excluded != m_excludedVars.سرا() && !binp->transp()
            && (binp->binsType().bin_Is_عام()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = جديد AstLogAnd{
                fl, جديد AstNot{fl, جديد AstVarRef{fl, excluded->دوجا, VAccess::READ}}, condp};
        }
        أرجع condp;
    }

    فراغ emit_Conv_کامیابی_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_گرہ_اظہاریہ* idxp, Ast_گرہ_اظہاریہ* condp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گرہ* actionp = make_Runtime_Bin_کامیابی(fl, cpVarp, idxp, binp->binsType().bin_Is_عام());
        إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جوڑو_اگلا(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        جوڑو_Sample_Stmt(جديد AstIf{fl, binCondition(binp, cpVarp, condp), actionp, خالی_سوچک});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    فراغ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_گرہ_اظہاریہ* exprp, uint32_t sized, AstVar* matchedp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گرہ_اظہاریہ* enabledp = binCondition(binp, cpVarp, جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ثابت منطقي illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        إذا (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_منزل_Str(sized),
                             binp->لبھو_ثنائی_ہندسہ_DType()};
            varp->funcLocal(سچ);
            m_sampleFuncp->جوڑو_Stmtsp(varp);
            جوڑو_Sample_Stmt(جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = جديد AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ثابت callp
            = شے_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_درخت(خطأ), enabledp});
        callp->dtype_سیٹ_ثنائی_ہندسہ();
        إذا (illegal) {
            جوڑو_Sample_Stmt(جديد AstIf{fl, جديد AstLogAnd{fl, callp, enabledp->clone_درخت(خطأ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } وإلا إذا (matchedp && binp->binsType().bin_Is_عام()) {
            جوڑو_Sample_Stmt(
                جديد Ast_سونپو{fl, جديد AstVarRef{fl, matchedp, VAccess::WRITE},
                              جديد AstOr{fl, جديد AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } وإلا {
            جوڑو_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_عارضی(File_سطر* fl, ثابت string& name, Ast_گرہ_DType* dtypep) {
        AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(سچ);
        m_constructorp->جوڑو_Stmtsp(varp);
        أرجع varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ساكن Ast_گرہ_اظہاریہ* resize_قدر(Ast_گرہ_اظہاریہ* valuep, Ast_گرہ_DType* dtypep) {
        File_سطر* ثابت fl = valuep->fileline();
        إذا (valuep->چوڑائی() > dtypep->چوڑائی()) {
            valuep = جديد AstSel{fl, valuep, 0, dtypep->چوڑائی()};
        } وإلا إذا (valuep->چوڑائی() < dtypep->چوڑائی()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstExtendS{fl, valuep, dtypep->چوڑائی()})
                         : جديد AstExtend{fl, valuep, dtypep->چوڑائی()};
        }
        valuep->dtypep(dtypep);
        أرجع valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> new_گنتی_دلیلاں(File_سطر* fl, ثابت string& name,
                                                       Ast_گرہ_اظہاریہ* exprp) {
        AstVar* ثابت countp = constructor_عارضی(fl, name, exprp->dtypep());
        m_constructorp->جوڑو_Stmtsp(
            جديد Ast_سونپو{fl, جديد AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_درخت(خطأ)});
        ثابت خودکار گنتی_Ref = [&]() { أرجع جديد AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ثابت zerop = جديد AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_گرہ_اظہاریہ* ثابت positivep
            = countp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstGtS{fl, گنتی_Ref(), zerop})
                                 : جديد AstNeq{fl, گنتی_Ref(), zerop};
        Ast_گرہ_اظہاریہ* valuep = resize_قدر(گنتی_Ref(), countp->لبھو_UInt64_DType());
        إذا (countp->چوڑائی() > VL_QUADSIZE) {
            valuep = جديد AstCond{
                fl,
                جديد AstRedOr{
                    fl, جديد AstSel{fl, گنتی_Ref(), VL_QUADSIZE, countp->چوڑائی() - VL_QUADSIZE}},
                جديد AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_سیٹ_UInt64();
        }
        أرجع {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ساكن std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> new_گنتی_دلیلاں(File_سطر* fl, uint64_t گنتی) {
        أرجع {جديد AstConst{fl, AstConst::Unsized64{}, گنتی},
                جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ساكن std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> coverpoint_اقدار(Ast_گرہ* nodep,
                                                                       Ast_گرہ_اظہاریہ* exprp) {
        ثابت صحيح چوڑائی = لگاتار_چوڑائی(exprp);
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs;
        ثابت AstEnumDType* ثابت enump = VN_CAST(exprp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (!enump) {
            ثابت Cross_قدر_Range domain
                = cross_قدر_Domain(nodep, exprp->چوڑائی(), exprp->isSigned(), چوڑائی);
            runs.emplace_پٹھ(domain.lo, domain.hi);
            أرجع runs;
        }
        std::سمتیہ<V3_نمبر> اقدار;
        لكل (ثابت Ast_Enum_شے* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            ثابت V3_نمبر& عدد = VN_AS(itemp->valuep(), Const)->عدد();
            إذا (عدد.isFourState()) استمر;  // Not a coverpoint value (19.5.7)
            اقدار.emplace_پٹھ(nodep, چوڑائی);
            إذا (exprp->isSigned()) {
                اقدار.پٹھ().opExtendS(عدد, عدد.چوڑائی());
            } وإلا {
                اقدار.پٹھ().op_سونپو(عدد);
            }
        }
        std::ترتیب_دیو(اقدار.begin(), اقدار.سرا(), cross_قدر_Less);
        ثابت V3_نمبر one{nodep, چوڑائی, 1};
        لكل (ثابت V3_نمبر& قدر : اقدار) {
            V3_نمبر اگلا{nodep, چوڑائی};
            إذا (!runs.خالی_تھاواں() && اگلا.op_جوڑو(runs.پٹھ().دوجا, one).isCaseEq(قدر)) {
                runs.پٹھ().دوجا = قدر;
            } وإلا {
                runs.emplace_پٹھ(قدر, قدر);
            }
        }
        أرجع runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    فراغ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_گرہ_اظہاریہ* exprp) {
        File_سطر* ثابت fl = binp->fileline();
        ثابت string اگیتر
            = m_sized_ناں.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ثابت std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> گنتی
            = binp->arraySizep() ? new_گنتی_دلیلاں(fl, اگیتر + "_count", binp->arraySizep())
                                 : new_گنتی_دلیلاں(fl, 1);
        AstCoverWith* ثابت withp = binWith(binp);
        إذا (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& لگاتار : coverpoint_اقدار(binp, exprp)) {
                m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_قدر_Const(fl, لگاتار.پہلا, exprp),
                                                    new_قدر_Const(fl, لگاتار.دوجا, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        لكل (Ast_گرہ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            إذا (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                استمر;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, اگیتر + "_" + cvt_منزل_Str(element++));
        }
        إذا (withp) generateWithFilter(binp, withp, cpVarp, exprp, اگیتر);
        std::سمتیہ<Ast_گرہ_اظہاریہ*> دلیلاں{ctext(fl, binp->binsType().bin_سیٹ_Enum()), گنتی.پہلا,
                                       گنتی.دوجا};
        // A filter's bins have the limit it began with
        إذا (!withp) دلیلاں.ڈالو_پٹھ(cnum(fl, v3Global.opt.coverageMaxBins()));
        ثابت منطقي prot = v3Global.opt.protectIds();
        دلیلاں.ڈالو_پٹھ(ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))));
        دلیلاں.ڈالو_پٹھ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        دلیلاں.ڈالو_پٹھ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        دلیلاں.ڈالو_پٹھ(cnum(fl, static_cast<uint32_t>(fl->پہلا_کالم())));
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_انت : VCMethod::COVERGROUP_SIZED_انت,
                     دلیلاں)
                ->makeStmt());
    }

    // Emit the constructor code evaluating the 'with' filter of 'binp' for each of its candidate
    // values, which sizedRange() added, passing the runs of values it keeps (IEEE 1800-2023
    // 19.5.1.1).  The filter is evaluated in a loop, whose code does not grow with the elements:
    //   withBegin(grouping, limit);
    //   more = 1;
    //   while (withNext()) {
    //       value = withLo(); last = withHi(); run = 0;
    //       while (true) {
    //           item = value;
    //           if (filter) { if (!run) { first = value; run = 1; } }
    //           else if (run) { more = withRun(first, value - 1); run = 0; }
    //           if (!more || value == last) break;
    //           ++value;
    //       }
    //       if (run) more = withRun(first, last);
    //   }
    فراغ generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_گرہ_اظہاریہ* exprp, ثابت string& اگیتر) {
        File_سطر* ثابت fl = withp->fileline();
        ثابت منطقي wide = exprp->isWide();
        ثابت string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_گرہ_DType* ثابت قدر_DTypep = exprp->dtypep();
        AstVar* ثابت valuep = constructor_عارضی(fl, اگیتر + "_value", قدر_DTypep);
        AstVar* ثابت lastp = constructor_عارضی(fl, اگیتر + "_last", قدر_DTypep);
        AstVar* ثابت firstp = constructor_عارضی(fl, اگیتر + "_first", قدر_DTypep);
        AstVar* ثابت runp = constructor_عارضی(fl, اگیتر + "_run", binp->لبھو_ثنائی_ہندسہ_DType());
        AstVar* ثابت morep = constructor_عارضی(fl, اگیتر + "_more", binp->لبھو_ثنائی_ہندسہ_DType());
        AstVar* ثابت itemp = withp->itemp()->unlink_Fr_پٹھ();
        itemp->name(اگیتر + "_item");
        m_constructorp->جوڑو_Stmtsp(itemp);
        ثابت خودکار ref = [&](AstVar* varp) { أرجع جديد AstVarRef{fl, varp, VAccess::READ}; };
        ثابت خودکار سونپو_2 = [&](AstVar* varp, Ast_گرہ_اظہاریہ* rhsp) -> Ast_گرہ* {
            أرجع جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ثابت خودکار flag = [&](AstVar* varp, منطقي قدر) {
            أرجع سونپو_2(varp, قدر ? جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}
                                      : جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}});
        };
        ثابت خودکار bound = [&](VCMethod narrow, VCMethod wide_طریقہ, AstVar* varp) -> Ast_گرہ* {
            إذا (wide) {
                أرجع شے_Call(fl, cpVarp, wide_طریقہ, {جديد AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ثابت callp = شے_Call(fl, cpVarp, narrow);
            callp->dtype_سیٹ_UInt64();
            أرجع سونپو_2(varp, resize_قدر(callp, قدر_DTypep));
        };
        ثابت خودکار keep = [&](Ast_گرہ_اظہاریہ* lop, Ast_گرہ_اظہاریہ* hip) {
            AstCMethodHard* ثابت callp = شے_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_لگاتار_W : VCMethod::COVERGROUP_WITH_لگاتار,
                {lop, hip});
            callp->dtype_سیٹ_ثنائی_ہندسہ();
            أرجع سونپو_2(morep, callp);
        };
        ثابت خودکار قدم = [&](منطقي اتے) {
            AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_قدر{}, exprp->چوڑائی(), 1};
            Ast_گرہ_اظہاریہ* ثابت stepp
                = اتے ? static_cast<Ast_گرہ_اظہاریہ*>(جديد Ast_جوڑو{fl, ref(valuep), onep})
                     : جديد AstSub{fl, ref(valuep), onep};
            stepp->dtypep(قدر_DTypep);
            أرجع stepp;
        };
        AstLoop* ثابت innerp = جديد AstLoop{fl};
        innerp->جوڑو_Stmtsp(سونپو_2(itemp, ref(valuep)));
        innerp->جوڑو_Stmtsp(جديد AstIf{
            fl, withp->filterp()->unlink_Fr_پٹھ(),
            جديد AstIf{fl, جديد AstNot{fl, ref(runp)},
                      سونپو_2(firstp, ref(valuep))->جوڑو_اگلا(flag(runp, سچ))},
            جديد AstIf{fl, ref(runp), keep(ref(firstp), قدم(خطأ))->جوڑو_اگلا(flag(runp, خطأ))}});
        innerp->جوڑو_Stmtsp(جديد AstLoopTest{
            fl, innerp, جديد AstLogAnd{fl, ref(morep), جديد AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->جوڑو_Stmtsp(سونپو_2(valuep, قدم(سچ)));
        AstLoop* ثابت outerp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = شے_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_اگلا);
        nextp->dtype_سیٹ_ثنائی_ہندسہ();
        outerp->جوڑو_Stmtsp(جديد AstLoopTest{fl, outerp, nextp});
        outerp->جوڑو_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->جوڑو_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->جوڑو_Stmtsp(flag(runp, خطأ));
        outerp->جوڑو_Stmtsp(innerp);
        outerp->جوڑو_Stmtsp(جديد AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->جوڑو_Stmtsp(flag(morep, سچ));
        m_constructorp->جوڑو_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    فراغ generateSizedElement(AstVar* cpVarp, ثابت AstCoverBin* binp, Ast_گرہ* rangep,
                              Ast_گرہ_اظہاریہ* exprp, ثابت string& اگیتر) {
        File_سطر* ثابت fl = rangep->fileline();
        ثابت VCMethod طریقہ = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
        Ast_گرہ_اظہاریہ* ثابت lowp = irp ? irp->lhsp() : VN_AS(rangep, گرہ_اظہاریہ);
        Ast_گرہ_اظہاریہ* ثابت highp = irp ? irp->rhsp() : خالی_سوچک;
        ثابت خودکار unbounded
            = [](ثابت Ast_گرہ_اظہاریہ* boundp) { أرجع !boundp || VN_IS(boundp, Unbounded); };
        ثابت خودکار constant
            = [&](ثابت Ast_گرہ_اظہاریہ* boundp) { أرجع unbounded(boundp) || VN_IS(boundp, Const); };
        ثابت خودکار integral = [&](ثابت Ast_گرہ_اظہاریہ* boundp) {
            أرجع unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        إذا (constant(lowp) && constant(highp)) {
            ثابت خودکار fourState = [](ثابت Ast_گرہ_اظہاریہ* boundp) {
                ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
                أرجع constp && constp->عدد().isFourState();
            };
            إذا (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                أرجع;
            }
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            إذا (!resolve_قدر(rangep, exprp, سچ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا إذا (!cross_Range_خالی_تھاواں(range)) {
                std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs{{range.lo, range.hi}};
                إذا (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                لكل (ثابت std::pair<V3_نمبر, V3_نمبر>& لگاتار : runs) {
                    m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cpVarp, طریقہ,
                                                       {new_قدر_Const(fl, لگاتار.پہلا, exprp),
                                                        new_قدر_Const(fl, لگاتار.دوجا, exprp)})
                                                  ->makeStmt());
                }
            }
            أرجع;
        }
        إذا (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            أرجع;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ثابت خودکار bound_چوڑائی
            = [&](ثابت Ast_گرہ_اظہاریہ* boundp) { أرجع unbounded(boundp) ? 0 : boundp->چوڑائی(); };
        ثابت صحيح چوڑائی = std::max({exprp->چوڑائی(), bound_چوڑائی(lowp), bound_چوڑائی(highp)}) + 1;
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(rangep, exprp->چوڑائی(), exprp->isSigned(), چوڑائی);
        AstVar* ثابت lop = constructor_عارضی(fl, اگیتر + "_lo", exprp->dtypep());
        lop->dtype_سیٹ_ثنائی_ہندسہ_Sized(چوڑائی, VSigning::SIGNED);
        AstVar* ثابت hip = constructor_عارضی(fl, اگیتر + "_hi", lop->dtypep());
        ثابت خودکار bound = [&](Ast_گرہ_اظہاریہ* boundp, ثابت V3_نمبر& حد_بندی) -> Ast_گرہ_اظہاریہ* {
            إذا (unbounded(boundp)) {
                AstConst* ثابت limitp = جديد AstConst{fl, حد_بندی};
                limitp->dtypeFrom(lop);
                أرجع limitp;
            }
            Ast_گرہ_اظہاریہ* valuep = boundp->clone_درخت(خطأ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            إذا (exprp->isSigned() && boundp->چوڑائی() <= exprp->چوڑائی()) {
                valuep = resize_قدر(valuep, exprp->dtypep());
            }
            أرجع resize_قدر(valuep, lop->dtypep());
        };
        ثابت خودکار ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            أرجع جديد AstVarRef{fl, varp, access};
        };
        m_constructorp->جوڑو_Stmtsp(
            جديد Ast_سونپو{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->جوڑو_Stmtsp(
            جديد Ast_سونپو{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ثابت minp = جديد AstConst{fl, domain.lo};
        AstConst* ثابت maxp = جديد AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ثابت lowerp
            = جديد AstCond{fl, جديد AstLtS{fl, ref(lop), minp}, minp->clone_درخت(خطأ), ref(lop)};
        AstCond* ثابت upperp
            = جديد AstCond{fl, جديد AstGtS{fl, ref(hip), maxp}, maxp->clone_درخت(خطأ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->جوڑو_Stmtsp(جديد AstIf{fl, جديد AstLteS{fl, ref(lop), ref(hip)},
                                            شے_Call(fl, cpVarp, طریقہ,
                                                     {resize_قدر(ref(lop), exprp->dtypep()),
                                                      resize_قدر(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ساكن Ast_گرہ_اظہاریہ* لگاتار_Bin_اشاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, ثابت Bin_لگاتار& لگاتار) {
        إذا (لگاتار.m_گنتی == 1) أرجع cnum(fl, لگاتار.m_declared);
        ثابت صحيح چوڑائی = exprp->چوڑائی();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_گرہ_اظہاریہ* indexp
            = جديد AstSub{fl, exprp->clone_درخت(خطأ), new_قدر_Const(fl, لگاتار.m_lo, exprp)};
        indexp->dtype_سیٹ_ثنائی_ہندسہ_Sized(چوڑائی, VSigning::UNSIGNED);
        V3_نمبر stride{fl, چوڑائی, 0};
        stride.op_سونپو(لگاتار.m_stride);
        إذا (stride.گنتی_Ones() != 1) {
            indexp = جديد AstDiv{fl, indexp, جديد AstConst{fl, stride}};
        } وإلا إذا (!stride.isEqOne()) {
            indexp = جديد Ast_کھسکاؤ_R{fl, indexp, جديد AstConst{fl, stride.most_سیٹ_ثنائی_ہندسہ_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ثابت صحيح ext_چوڑائی = لگاتار.m_lo.چوڑائی() + 1;
        V3_نمبر lo{fl, ext_چوڑائی, 0};
        lo.opExtendS(لگاتار.m_lo, لگاتار.m_lo.چوڑائی());
        V3_نمبر وسعت{fl, ext_چوڑائی, 0};
        وسعت.opExtendS(لگاتار.m_hi, لگاتار.m_hi.چوڑائی());
        وسعت.opSub(V3_نمبر{وسعت}, lo);
        V3_نمبر covered{fl, ext_چوڑائی, 0};
        covered.op_سونپو(لگاتار.m_stride);
        covered.opMul(V3_نمبر{covered}, V3_نمبر{fl, ext_چوڑائی, لگاتار.m_گنتی});
        V3_نمبر hasRemainder{fl, 1, 0};
        إذا (!hasRemainder.opGte(وسعت, covered).isEqZero()) {
            AstConst* ثابت lastp = جديد AstConst{fl, V3_نمبر{fl, چوڑائی, لگاتار.m_گنتی - 1}};
            indexp = جديد AstCond{fl, جديد AstGt{fl, indexp->clone_درخت(خطأ), lastp},
                                 lastp->clone_درخت(خطأ), indexp};
        }
        إذا (چوڑائی < VL_IDATASIZE) {
            indexp = جديد AstExtend{fl, indexp, VL_IDATASIZE};
        } وإلا إذا (چوڑائی > VL_IDATASIZE) {
            indexp = جديد AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        أرجع جديد Ast_جوڑو{fl, cnum(fl, لگاتار.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    فراغ emit_لگاتار_کامیابی(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_گرہ_اظہاریہ* exprp, ثابت Bin_لگاتار& لگاتار) {
        File_سطر* ثابت fl = binp->fileline();
        AstConst* ثابت lop = new_قدر_Const(fl, لگاتار.m_lo, exprp);
        Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
        إذا (لگاتار.m_lo.isCaseEq(لگاتار.m_hi)) {
            condp = جديد AstEq{fl, exprp->clone_درخت(خطأ), lop};
        } وإلا {
            AstConst* ثابت hip = new_قدر_Const(fl, لگاتار.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ڈالو_Deletep(lop), lop);
            VL_DO_DANGLING(ڈالو_Deletep(hip), hip);
        }
        emit_Conv_کامیابی_If(coverpointp, binp, cpVarp, لگاتار_Bin_اشاریہ(fl, exprp, لگاتار), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    فراغ جوڑو_Conv_Trans_کامیابی_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ثابت Conv_Bin_نشانہ& tgt,
                           Ast_گرہ_اظہاریہ* condp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گرہ* actionp = make_Runtime_Bin_کامیابی(fl, tgt);
        إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جوڑو_اگلا(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        جوڑو_Sample_Stmt(جديد AstIf{fl, condp, actionp, خالی_سوچک});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    فراغ generateCoverpoint(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp, صحيح at_گھٹ_توں_گھٹ_قدر) {
        File_سطر* ثابت fl = coverpointp->fileline();
        ثابت منطقي dynamic = m_runtimePoints.گنتی(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ثابت منطقي crossFed = m_crossed_Cp_ناں.گنتی(coverpointp->name()) != 0;
        ثابت صحيح کامیابی_Bound = compute_کامیابی_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << کامیابی_Bound);
        AstVar* ثابت cpVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(کامیابی_Bound))};
        m_covergroupp->جوڑو_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->جوڑو_Stmtsp(make_شے_Create(fl, cpVarp, VCMethod::COVERGROUP_جوڑو_COVERPOINT));
        generate_شے_بھار(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        إذا (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_کامیابی_LIST)->makeStmt());
        }
        إذا (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ثابت excludedp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->لبھو_ثنائی_ہندسہ_DType()};
            excludedp->funcLocal(سچ);
            m_sampleFuncp->جوڑو_Stmtsp(excludedp);
            AstCMethodHard* ثابت callp
                = شے_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_قدر_EXCLUDED_W
                                           : VCMethod::COVERGROUP_قدر_EXCLUDED,
                           {exprp->clone_درخت(خطأ)});
            callp->dtype_سیٹ_ثنائی_ہندسہ();
            جوڑو_Sample_Stmt(جديد Ast_سونپو{fl, جديد AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::سمتیہ<Ast_گرہ_Stmt*> namerStmts;
        std::سمتیہ<AstCoverBin*> defaultBins;
        std::سمتیہ<AstCoverBin*> sizedBins;
        std::سمتیہ<std::tuple<AstCoverBin*, uint32_t, Ast_گرہ_اظہاریہ*>> metadata;
        std::سمتیہ<ثابت Bin_لگاتار*> لگاتار_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        لكل (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            ثابت صحيح errorsBefore = dynamic ? V3Error::error_گنتی() : 0;
            إذا (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ڈالو_پٹھ(cbinp);
                استمر;
            }
            إذا (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_وسعت وسعت;
                وسعت.sized = static_cast<int32_t>(sizedBins.ناپ());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), وسعت);
                sizedBins.ڈالو_پٹھ(cbinp);
                استمر;
            }
            إذا (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ڈالو_پٹھ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ثابت Conv_Bin_نشانہ tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_عام()};
                لكل (Ast_گرہ* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_کوڈ(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_سیٹ));
                إذا (dynamic && V3Error::error_گنتی() == errorsBefore) {
                    metadata.emplace_پٹھ(cbinp, static_cast<uint32_t>(idx), خالی_سوچک);
                }
                ++idx;
                استمر;
            }
            BinRuns plan;
            إذا (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                إذا (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ڈالو_پٹھ(cbinp->name());
                    استمر;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ثابت uint32_t پہلا_قدر = bins.کل;
                ثابت uint32_t پہلا_Declared = static_cast<uint32_t>(idx);
                namerStmts.ڈالو_پٹھ(
                    makeNamer(cpVarp, cbinp, plan.گنتی, پہلا_Declared, {}, plan.اقدار));
                لكل (Bin_لگاتار& لگاتار : plan.runs) {
                    لگاتار.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ڈالو_پٹھ(std::move(لگاتار));
                    ثابت Bin_لگاتار& stored = bins.runs.پٹھ();
                    إذا (bins.crossed && cbinp->binsType().bin_Is_عام()) {
                        ثابت uint32_t پہلا = پہلا_قدر + stored.m_declared - پہلا_Declared;
                        لكل (uint32_t element = 0; element < stored.m_گنتی; ++element) {
                            bins.اقدار[پہلا + element].runp = &stored;
                            bins.اقدار[پہلا + element].element = element;
                        }
                    }
                    إذا (!stored.m_خالی_تھاواں) {
                        emit_لگاتار_کامیابی(coverpointp, cbinp, cpVarp, exprp, stored);
                        إذا (dynamic && V3Error::error_گنتی() == errorsBefore) {
                            لگاتار_Metadata.ڈالو_پٹھ(&stored);
                        }
                    }
                    idx += stored.m_گنتی;
                }
                استمر;
            }
            إذا (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                منطقي unsupported = خطأ;
                std::سمتیہ<Ast_گرہ_اظہاریہ*> اقدار = کڈھو_Array_اقدار(cbinp, exprp, unsupported);
                إذا (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ڈالو_پٹھ(cbinp->name());
                    استمر;
                }
                namerStmts.ڈالو_پٹھ(makeNamer(cpVarp, cbinp, static_cast<int64_t>(اقدار.ناپ()),
                                               static_cast<uint32_t>(idx), اقدار));
                لكل (Ast_گرہ_اظہاریہ* valuep : اقدار) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_اقدار.ڈالو_پٹھ(valuep);
                    emit_Conv_کامیابی_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_قدر_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } وإلا {
                namerStmts.ڈالو_پٹھ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                إذا (Ast_گرہ_اظہاریہ* ثابت condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_کامیابی_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                إذا (dynamic && V3Error::error_گنتی() == errorsBefore) {
                    metadata.emplace_پٹھ(cbinp, static_cast<uint32_t>(idx), خالی_سوچک);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        لكل (ثابت std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_وسعت{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = خالی_سوچک;
        إذا (!defaultBins.خالی_تھاواں()
            && std::any_دا(sizedBins.begin(), sizedBins.سرا(), [](ثابت AstCoverBin* binp) {
                   أرجع binp->binsType().bin_Is_عام();
               })) {
            sizedMatchedp = جديد AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->لبھو_ثنائی_ہندسہ_DType()};
            sizedMatchedp->funcLocal(سچ);
            m_sampleFuncp->جوڑو_Stmtsp(sizedMatchedp);
            جوڑو_Sample_Stmt(جديد Ast_سونپو{fl, جديد AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}});
        }
        لكل (uint32_t sized = 0; sized < sizedBins.ناپ(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        لكل (AstCoverBin* ثابت defBinp : defaultBins) {
            File_سطر* ثابت dfl = defBinp->fileline();
            namerStmts.ڈالو_پٹھ(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_گرہ_اظہاریہ* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            إذا (sizedMatchedp) {
                condp = جديد AstLogAnd{
                    dfl, جديد AstNot{dfl, جديد AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_کامیابی_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        إذا (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        إذا (coverpointHasTransition(coverpointp)) {
            AstVar* ثابت prevVarp = VN_AS(coverpointp->user1p(), Var);
            جوڑو_Sample_Stmt(
                جديد Ast_سونپو{coverpointp->fileline(),
                              جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_درخت(خطأ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_الفاظ_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_گھٹ_توں_گھٹ_قدر)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        لكل (Ast_گرہ_Stmt* ثابت ns : namerStmts) m_constructorp->جوڑو_Stmtsp(ns);
        إذا (dynamic) {
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_قدر_TYPE,
                         {cnum(fl, exprp->چوڑائی()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            قدر_Lists lists;
            لكل (ثابت خودکار& اندراج : metadata) {
                collect_قدر_Metadata(lists, exprp, std::get<0>(اندراج), std::get<1>(اندراج),
                                     std::get<2>(اندراج));
            }
            لكل (ثابت Bin_لگاتار* ثابت runp : لگاتار_Metadata) collect_لگاتار_Metadata(lists, exprp, *runp);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_RANGES, lists.m_ranges);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_RUNS, lists.m_runs);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_PATTERNS, lists.m_patterns);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_TRANSITIONS, lists.m_transitions);
            لكل (AstCoverBin* ثابت binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_قدر_FINALIZE)->makeStmt());
        }
        إذا (v3Global.opt.coverage()) {
            ثابت std::string ورقہ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_رجسٹر_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(ورقہ)),
                          cnum(fl, شے_Database_بھار(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_بھار)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    فراغ generate_Multi_قدر_Transition_کوڈ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_گرہ_اظہاریہ* exprp, ثابت Conv_Bin_نشانہ& tgt,
                                          ثابت std::سمتیہ<Ast_Cover_Trans_شے*>& اشیاء) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << اشیاء.ناپ() << " items");

        // Create state position variable
        AstVar* ثابت stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ثابت casep
            = جديد AstCase{binp->fileline(), VCaseType::CT_CASE,
                          جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, خالی_سوچک};

        // Generate each case item in the switch statement
        لكل (ناپ_t state = 0; state < اشیاء.ناپ(); ++state) {
            Ast_Case_شے* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, اشیاء, state);
            casep->جوڑو_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_شے* ثابت defaultItemp = جديد Ast_Case_شے{
            binp->fileline(), خالی_سوچک,
            جديد Ast_سونپو{binp->fileline(),
                          جديد AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          جديد AstConst{binp->fileline(), AstConst::Widthed_قدر{}, 8, 0}}};
        casep->جوڑو_Itemsp(defaultItemp);

        جوڑو_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_شے* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_گرہ_اظہاریہ* exprp, ثابت Conv_Bin_نشانہ& tgt,
                                             AstVar* stateVarp,
                                             ثابت std::سمتیہ<Ast_Cover_Trans_شے*>& اشیاء,
                                             ناپ_t state) {
        File_سطر* ثابت fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_گرہ_اظہاریہ* ثابت میل_Condp = build_Transition_شے_Condition(اشیاء[state], exprp);

        Ast_گرہ_Stmt* میل_Actionp = خالی_سوچک;

        إذا (state == اشیاء.ناپ() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            میل_Actionp = make_Runtime_Bin_کامیابی(fl, tgt);

            // For illegal_bins, add error message
            إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ثابت string err_پیغام = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                میل_Actionp = میل_Actionp->جوڑو_اگلا(makeIllegalBinAction(fl, err_پیغام));
            }

            // Reset state to 0
            میل_Actionp = میل_Actionp->جوڑو_اگلا(
                جديد Ast_سونپو{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, 0}});
        } وإلا {
            // Intermediate state: advance to next state
            میل_Actionp = جديد Ast_سونپو{
                fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_گرہ_Stmt* no_میل_Actionp = خالی_سوچک;
        إذا (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_گرہ_اظہاریہ* ثابت restartCondp = build_Transition_شے_Condition(اشیاء[0], exprp);

            UASSERT_OBJ(restartCondp, اشیاء[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_گرہ_Stmt* restartActionp
                = جديد Ast_سونپو{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_گرہ_Stmt* resetActionp
                = جديد Ast_سونپو{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, 0}};

            no_میل_Actionp = جديد AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_گرہ_Stmt* ثابت stmtp = جديد AstIf{fl, میل_Condp, میل_Actionp, no_میل_Actionp};

        // Create case item for this state value
        Ast_Case_شے* ثابت caseItemp = جديد Ast_Case_شے{
            fl, جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        أرجع caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_گرہ_Stmt* makeIllegalBinAction(File_سطر* fl, ثابت string& err_پیغام) {
        AstDisplay* ثابت errorp
            = جديد AstDisplay{fl, VDisplayType::DT_ERROR, err_پیغام, خالی_سوچک, خالی_سوچک};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_گرہ*>(errorp)->جوڑو_اگلا(جديد AstStop{fl, سچ});
        أرجع errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ساكن AstConst* new_قدر_Const(File_سطر* fl, ثابت V3_نمبر& قدر, ثابت Ast_گرہ_اظہاریہ* exprp) {
        V3_نمبر narrowed{fl, exprp->چوڑائی(), 0};
        narrowed.op_سونپو(قدر);
        AstConst* ثابت constp = جديد AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        أرجع constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ساكن AstConst* newRealConst(AstConst* constp) {
        إذا (constp->عدد().isDouble()) أرجع constp->clone_درخت(خطأ);
        V3_نمبر real{&constp->عدد(), 64};
        real.opIToRD(constp->عدد(), constp->isSigned());
        أرجع جديد AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ساكن AstConst* widenConst(File_سطر* fl, AstConst* constp, صحيح نشانہ_چوڑائی) {
        إذا (constp->چوڑائی() == نشانہ_چوڑائی) أرجع constp->clone_درخت(خطأ);
        V3_نمبر عدد{fl, نشانہ_چوڑائی, 0};
        عدد.op_سونپو(constp->عدد());
        أرجع جديد AstConst{fl, عدد};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_گرہ_اظہاریہ* makeRangeCondition(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* minp,
                                    Ast_گرہ_اظہاریہ* maxp) {
        ثابت صحيح اظہاریہ_چوڑائی = exprp->چوڑائی_Min();
        AstConst* ثابت minConstp = VN_AS(minp, Const);
        AstConst* ثابت maxConstp = VN_AS(maxp, Const);
        إذا (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            أرجع جديد AstAnd{fl,
                              جديد AstGteD{fl, exprp->clone_درخت(خطأ), newRealConst(minConstp)},
                              جديد AstLteD{fl, exprp->clone_درخت(خطأ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ثابت minWidep = widenConst(fl, minConstp, اظہاریہ_چوڑائی);
        AstConst* ثابت maxWidep = widenConst(fl, maxConstp, اظہاریہ_چوڑائی);
        V3_نمبر minimum{fl, اظہاریہ_چوڑائی, 0};
        V3_نمبر ودھ_توں_ودھ{fl, اظہاریہ_چوڑائی, 0};
        ودھ_توں_ودھ.سیٹ_All_Bits1();
        إذا (exprp->isSigned()) {
            minimum.سیٹ_ثنائی_ہندسہ(اظہاریہ_چوڑائی - 1, 1);
            ودھ_توں_ودھ.سیٹ_ثنائی_ہندسہ(اظہاریہ_چوڑائی - 1, 0);
        }
        Ast_گرہ_اظہاریہ* lowerp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* upperp = خالی_سوچک;
        إذا (minWidep->عدد().isCaseEq(minimum)) {
            VL_DO_DANGLING(ڈالو_Deletep(minWidep), minWidep);
        } وإلا {
            lowerp = exprp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(
                                             جديد AstGteS{fl, exprp->clone_درخت(خطأ), minWidep})
                                       : static_cast<Ast_گرہ_اظہاریہ*>(
                                             جديد AstGte{fl, exprp->clone_درخت(خطأ), minWidep});
        }
        إذا (maxWidep->عدد().isCaseEq(ودھ_توں_ودھ)) {
            VL_DO_DANGLING(ڈالو_Deletep(maxWidep), maxWidep);
        } وإلا {
            upperp = exprp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(
                                             جديد AstLteS{fl, exprp->clone_درخت(خطأ), maxWidep})
                                       : static_cast<Ast_گرہ_اظہاریہ*>(
                                             جديد AstLte{fl, exprp->clone_درخت(خطأ), maxWidep});
        }
        إذا (lowerp && upperp) أرجع جديد AstAnd{fl, lowerp, upperp};
        إذا (lowerp) أرجع lowerp;
        إذا (upperp) أرجع upperp;
        أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_گرہ_اظہاریہ* makeOpenRangeCondition(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, AstConst* boundp,
                                        منطقي is_ہیٹھلا_Bound) {
        AstConst* ثابت widep = widenConst(fl, boundp, exprp->چوڑائی_Min());
        إذا (is_ہیٹھلا_Bound) {
            إذا (exprp->isSigned()) أرجع جديد AstGteS{fl, exprp->clone_درخت(خطأ), widep};
            أرجع جديد AstGte{fl, exprp->clone_درخت(خطأ), widep};
        }
        إذا (exprp->isSigned()) أرجع جديد AstLteS{fl, exprp->clone_درخت(خطأ), widep};
        أرجع جديد AstLte{fl, exprp->clone_درخت(خطأ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_گرہ_اظہاریہ* build_Transition_شے_Condition(Ast_Cover_Trans_شے* itemp, AstVar* varp) {
        Ast_گرہ_اظہاریہ* varRefp = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_گرہ_اظہاریہ* ثابت condp = build_Transition_شے_Condition(itemp, varRefp);
        VL_DO_DANGLING(ڈالو_Deletep(varRefp), varRefp);
        أرجع condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_گرہ_اظہاریہ* build_Transition_شے_Condition(Ast_Cover_Trans_شے* itemp, Ast_گرہ_اظہاریہ* exprp) {
        Ast_گرہ_اظہاریہ* condp = خالی_سوچک;

        لكل (Ast_گرہ* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_گرہ_اظہاریہ* singleCondp = خالی_سوچک;
            valp = V3Const::constifyEdit(valp);
            AstConst* ثابت constp = VN_CAST(valp, Const);
            إذا (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                أرجع جديد AstConst{valp->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
            }
            singleCondp
                = جديد AstEq{constp->fileline(), exprp->clone_درخت(خطأ), constp->clone_درخت(خطأ)};
            إذا (condp) {
                condp = جديد AstOr{itemp->fileline(), condp, singleCondp};
            } وإلا {
                condp = singleCondp;
            }
        }

        أرجع condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    فراغ generate_Single_Transition_کوڈ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_گرہ_اظہاریہ* exprp, ثابت Conv_Bin_نشانہ& tgt,
                                      Ast_Cover_Trans_سیٹ* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ثابت prevVarp = create_Prev_قدر_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::سمتیہ<Ast_Cover_Trans_شے*> اشیاء;
        لكل (Ast_گرہ* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            اشیاء.ڈالو_پٹھ(VN_AS(itemp, Cover_Trans_شے));

        إذا (اشیاء.خالی_تھاواں()) {
            binp->v3error("Transition set without items");
            أرجع;
        }

        إذا (اشیاء.ناپ() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            أرجع;
        } وإلا إذا (اشیاء.ناپ() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_گرہ_اظہاریہ* ثابت cond1p = build_Transition_شے_Condition(اشیاء[0], prevVarp);
            Ast_گرہ_اظہاریہ* ثابت cond2p = build_Transition_شے_Condition(اشیاء[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_گرہ_اظہاریہ* بھریا_Condp = جديد AstAnd{binp->fileline(), cond1p, cond2p};

            جوڑو_Conv_Trans_کامیابی_If(coverpointp, binp, tgt, بھریا_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } وإلا {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_قدر_Transition_کوڈ(coverpointp, binp, exprp, tgt, اشیاء);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_سطر* fl, ثابت std::سمتیہ<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->جوڑو("{ VlCoverpoint* __Vcx_cps[] = {");
        لكل (ناپ_t d = 0; d < cpVars.ناپ(); ++d) {
            إذا (d != 0) cs->جوڑو(", ");
            cs->جوڑو(memberRef(fl, cpVars[d]));
        }
        cs->جوڑو("}; ");
        cs->جوڑو(callp);
        cs->جوڑو("; }");
        أرجع cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_سطر* fl, ثابت std::سمتیہ<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->جوڑو("{ bool __Vcx_iffs[" + cvt_منزل_Str(bins.ناپ()) + "]; ");
        لكل (ناپ_t i = 0; i < bins.ناپ(); ++i) {
            ثابت AstCoverCrossBin* ثابت binp = bins[i];
            cs->جوڑو("__Vcx_iffs[" + cvt_منزل_Str(i) + "] = ");
            cs->جوڑو(binp->iffp() ? binp->iffp()->clone_درخت(خطأ)
                                 : جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}});
            cs->جوڑو("; ");
        }
        cs->جوڑو(callp);
        cs->جوڑو("; }");
        أرجع cs;
    }

    ورتوں CrossSelection = std::سمتیہ<uint64_t>;
    بنية ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    بنية CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_الفاظ = 0;
        منطقي valid = سچ;
        std::سمتیہ<ResolvedCrossBin> bins;
    };
    بنية CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ثابت std::سمتیہ<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ثابت std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::سمتیہ<uint32_t> strides;  // Flat-index stride per dimension
        منطقي valid = سچ;  // False if this explicit bin cannot be implemented
    };
    بنية Cross_Binsof_نشانہ final {
        ثابت CoverpointBins* m_binsp = خالی_سوچک;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_پہلا = 0;  // First declared normal bin in the selected span
        uint32_t m_گنتی = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_پہلا = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_سرا = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    بنية Cross_قدر_Range final {
        V3_نمبر lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_نمبر hi;  // Inclusive upper bound
        V3_نمبر نمونہ_2;  // Allowed bit values; all X for an ordinary interval
        منطقي wildcard = خطأ;  // A wildcard singleton rather than an exact four-state value

        Cross_قدر_Range(Ast_گرہ* nodep, صحيح چوڑائی)
            : lo{nodep, چوڑائی}
            , hi{nodep, چوڑائی}
            , نمونہ_2{nodep, چوڑائی} {
            نمونہ_2.سیٹ_All_بٹ_X();
        }
    };

    ساكن std::سمتیہ<Ast_گرہ*> cross_Bin_اقدار(ثابت Cross_Bin_اقدار& bin) {
        إذا (bin.valuep) أرجع {bin.valuep};
        std::سمتیہ<Ast_گرہ*> اقدار;
        إذا (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            لكل (Ast_گرہ* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_شے* lastp = VN_AS(setp, Cover_Trans_سیٹ)->itemsp();
                طالما (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_شے);
                لكل (Ast_گرہ* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    اقدار.ڈالو_پٹھ(valuep);
                }
            }
        } وإلا {
            لكل (Ast_گرہ* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                اقدار.ڈالو_پٹھ(valuep);
            }
        }
        أرجع اقدار;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ساكن Cross_قدر_Range لگاتار_Bin_Range(Ast_گرہ* nodep, ثابت Bin_لگاتار& لگاتار, uint32_t element,
                                       صحيح چوڑائی) {
        ثابت صحيح runw = لگاتار.m_lo.چوڑائی();
        V3_نمبر وتھ{nodep, runw};
        وتھ.opMul(لگاتار.m_stride, V3_نمبر{nodep, runw, element});
        V3_نمبر lo{nodep, runw};
        lo.op_جوڑو(لگاتار.m_lo, وتھ);
        V3_نمبر hi = لگاتار.m_hi;
        إذا (element + 1 < لگاتار.m_گنتی) {
            وتھ.opSub(لگاتار.m_stride, V3_نمبر{nodep, runw, 1});
            hi.op_جوڑو(lo, وتھ);
        }
        Cross_قدر_Range range{nodep, چوڑائی};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        أرجع range;
    }

    ساكن صحيح cross_Range_چوڑائی(Ast_گرہ* nodep) {
        إذا (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            أرجع std::max(rangep->lhsp()->چوڑائی(), rangep->rhsp()->چوڑائی());
        }
        أرجع nodep->چوڑائی();
    }

    ساكن منطقي cross_قدر_Less(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
        V3_نمبر نتیجہ{&lhs};
        أرجع !نتیجہ.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ساكن فراغ crossRealBound(ثابت AstConst* constp, Ast_گرہ_اظہاریہ* exprp, منطقي اتلا,
                               ثابت Cross_قدر_Range& domain, V3_نمبر& نتیجہ, منطقي& خالی_تھاواں) {
        ثابت مزدوج قدر = constp->عدد().منزل_Double();
        ثابت مزدوج bound = اتلا ? std::floor(قدر) : std::ceil(قدر);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ثابت مزدوج حد_بندی
            = std::ldexp(1.0, exprp->isSigned() ? exprp->چوڑائی() - 1 : exprp->چوڑائی());
        ثابت مزدوج minimum = exprp->isSigned() ? -حد_بندی : 0.0;
        إذا (std::isnan(bound) || (اتلا ? bound < minimum : bound >= حد_بندی)) {
            خالی_تھاواں = سچ;
            نتیجہ = domain.lo;
        } وإلا إذا (bound < minimum) {
            نتیجہ = domain.lo;
        } وإلا إذا (bound >= حد_بندی) {
            نتیجہ = domain.hi;
        } وإلا {
            V3_نمبر real{&نتیجہ, 64};
            real.سیٹ_Double(bound);
            نتیجہ.opRToIRoundS(real);
        }
    }

    ساكن منطقي crossRangeBound(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, منطقي اتلا, منطقي bin_قدر,
                                ثابت Cross_قدر_Range& domain, V3_نمبر& نتیجہ, منطقي& خالی_تھاواں) {
        إذا (VN_IS(nodep, Unbounded)) {
            V3_نمبر حد_بندی{nodep, exprp->چوڑائی()};
            إذا (اتلا) حد_بندی.سیٹ_All_Bits1();
            إذا (exprp->isSigned()) {
                حد_بندی.سیٹ_ثنائی_ہندسہ(exprp->چوڑائی() - 1, !اتلا);
                نتیجہ.opExtendS(حد_بندی, حد_بندی.چوڑائی());
            } وإلا {
                نتیجہ.op_سونپو(حد_بندی);
            }
            أرجع سچ;
        }
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp) أرجع خطأ;
        إذا (constp->عدد().isDouble()) {
            crossRealBound(constp, exprp, اتلا, domain, نتیجہ, خالی_تھاواں);
            أرجع سچ;
        }
        إذا (constp->عدد().isString()) أرجع خطأ;
        إذا (bin_قدر && exprp->isSigned() && constp->چوڑائی() <= exprp->چوڑائی()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_نمبر قدر{nodep, exprp->چوڑائی()};
            إذا (constp->isSigned()) {
                قدر.opExtendS(constp->عدد(), constp->چوڑائی());
            } وإلا {
                قدر.op_سونپو(constp->عدد());
            }
            نتیجہ.opExtendS(قدر, قدر.چوڑائی());
        } وإلا إذا (constp->isSigned()) {
            نتیجہ.opExtendS(constp->عدد(), constp->چوڑائی());
        } وإلا {
            نتیجہ.op_سونپو(constp->عدد());
        }
        أرجع سچ;
    }

    ساكن Cross_قدر_Range cross_قدر_Domain(Ast_گرہ* nodep, صحيح قدر_چوڑائی, منطقي isSigned,
                                            صحيح چوڑائی) {
        Cross_قدر_Range domain{nodep, چوڑائی};
        V3_نمبر lo{nodep, قدر_چوڑائی};
        V3_نمبر hi{nodep, قدر_چوڑائی};
        hi.سیٹ_All_Bits1();
        إذا (isSigned) {
            lo.سیٹ_ثنائی_ہندسہ(قدر_چوڑائی - 1, 1);
            hi.سیٹ_ثنائی_ہندسہ(قدر_چوڑائی - 1, 0);
            domain.lo.opExtendS(lo, قدر_چوڑائی);
            domain.hi.opExtendS(hi, قدر_چوڑائی);
        } وإلا {
            domain.lo.op_سونپو(lo);
            domain.hi.op_سونپو(hi);
        }
        أرجع domain;
    }

    ساكن فراغ intersectCrossRange(Cross_قدر_Range& range, ثابت Cross_قدر_Range& other) {
        إذا (cross_قدر_Less(range.lo, other.lo)) range.lo = other.lo;
        إذا (cross_قدر_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ساكن منطقي cross_قدر_Range(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, منطقي bin_قدر, منطقي wildcard,
                                ثابت Cross_قدر_Range& domain, Cross_قدر_Range& range) {
        منطقي خالی_تھاواں = خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            إذا (!crossRangeBound(rangep->lhsp(), exprp, خطأ, bin_قدر, domain, range.lo, خالی_تھاواں)
                || !crossRangeBound(rangep->rhsp(), exprp, سچ, bin_قدر, domain, range.hi, خالی_تھاواں)
                || range.lo.isFourState() || range.hi.isFourState()) {
                أرجع خطأ;
            }
        } وإلا إذا (constp && constp->عدد().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, خطأ, domain, range.lo, خالی_تھاواں);
            crossRealBound(constp, exprp, سچ, domain, range.hi, خالی_تھاواں);
        } وإلا {
            إذا (!crossRangeBound(nodep, exprp, خطأ, bin_قدر, domain, range.lo, خالی_تھاواں)) {
                أرجع خطأ;
            }
            إذا (bin_قدر && exprp->isSigned() && constp && !constp->isSigned()
                && constp->چوڑائی() > exprp->چوڑائی()) {
                منطقي representable = سچ;
                لكل (صحيح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < constp->چوڑائی(); ++ثنائی_ہندسہ_2) {
                    representable &= !constp->عدد().ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2);
                }
                إذا (representable) {
                    V3_نمبر قدر{nodep, exprp->چوڑائی()};
                    قدر.op_سونپو(constp->عدد());
                    range.lo.opExtendS(قدر, قدر.چوڑائی());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            إذا (wildcard) {
                range.نمونہ_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                إذا (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_قدر_Domain(nodep, constp->چوڑائی(), سچ, domain.lo.چوڑائی()));
                }
            }
        }
        إذا (خالی_تھاواں) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            أرجع سچ;
        }
        إذا (!range.lo.isFourState()) intersectCrossRange(range, domain);
        إذا (range.wildcard && exprp->isSigned()) {
            ثابت صحيح sign = exprp->چوڑائی() - 1;
            إذا (constp && !constp->isSigned() && constp->چوڑائی() > exprp->چوڑائی()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                لكل (صحيح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < constp->چوڑائی(); ++ثنائی_ہندسہ_2) {
                    إذا (constp->عدد().ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        أرجع سچ;
                    }
                }
                لكل (صحيح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < range.نمونہ_2.چوڑائی(); ++ثنائی_ہندسہ_2) {
                    إذا (range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(sign))
                        range.نمونہ_2.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
                    وإلا
                        range.نمونہ_2.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, range.نمونہ_2.ثنائی_ہندسہ_Is1(sign));
                }
                أرجع سچ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            لكل (صحيح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < range.نمونہ_2.چوڑائی(); ++ثنائی_ہندسہ_2) {
                إذا (range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) استمر;
                ثابت منطقي قدر = range.نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2);
                إذا (!range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(sign) && range.نمونہ_2.ثنائی_ہندسہ_Is1(sign) != قدر) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    اكسر;
                }
                range.نمونہ_2.سیٹ_ثنائی_ہندسہ(sign, قدر);
            }
        }
        أرجع سچ;
    }

    تعداد CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_ہیٹھلا = 1,
        CROSS_AT_اتلا = 2,
        CROSS_AT_BOUNDS = CROSS_AT_ہیٹھلا | CROSS_AT_اتلا,
        CROSS_NO_میل = 4  // Prefix cannot match the interval/pattern
    };

    ساكن CrossRangeState cross_Range_قدم(ثابت Cross_قدر_Range& range, CrossRangeState state,
                                          صحيح ثنائی_ہندسہ_2, صحيح قدر) {
        إذا (state == CROSS_NO_میل) أرجع CROSS_NO_میل;
        // Flipping the sign bit makes signed order lexicographic.
        ثابت منطقي sign = ثنائی_ہندسہ_2 == range.نمونہ_2.چوڑائی() - 1;
        إذا (!range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && قدر != (range.نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) ^ sign)) {
            أرجع CROSS_NO_میل;
        }
        ثابت صحيح کم = range.lo.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) ^ sign;
        ثابت صحيح بلند = range.hi.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) ^ sign;
        إذا (((state & CROSS_AT_ہیٹھلا) && قدر < کم)
            || ((state & CROSS_AT_اتلا) && قدر > بلند)) {
            أرجع CROSS_NO_میل;
        }
        أرجع static_cast<CrossRangeState>(
            ((state & CROSS_AT_ہیٹھلا) && قدر == کم ? CROSS_AT_ہیٹھلا : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_اتلا) && قدر == بلند ? CROSS_AT_اتلا : CROSS_INSIDE_BOUNDS));
    }

    ساكن منطقي crossWildcardIntersects(ثابت Cross_قدر_Range& range) {
        بنانشانی حالتاں = 1U << CROSS_AT_BOUNDS;
        لكل (صحيح ثنائی_ہندسہ_2 = range.نمونہ_2.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0 && حالتاں; --ثنائی_ہندسہ_2) {
            بنانشانی اگلا = 0;
            لكل (ثابت CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_ہیٹھلا, CROSS_AT_اتلا, CROSS_AT_BOUNDS}) {
                إذا (!(حالتاں & (1U << state))) استمر;
                لكل (صحيح قدر = 0; قدر < 2; ++قدر) {
                    ثابت CrossRangeState equal = cross_Range_قدم(range, state, ثنائی_ہندسہ_2, قدر);
                    إذا (equal != CROSS_NO_میل) اگلا |= 1U << equal;
                }
            }
            حالتاں = اگلا;
        }
        أرجع حالتاں != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ساكن منطقي اگلا_نمونہ_قدر(ثابت V3_نمبر& نمونہ_2, ثابت V3_نمبر& from, V3_نمبر& نتیجہ) {
        نتیجہ.op_سونپو(from);
        صحيح حاصل = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        لكل (صحيح ثنائی_ہندسہ_2 = from.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; --ثنائی_ہندسہ_2) {
            إذا (نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
                إذا (!from.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) حاصل = ثنائی_ہندسہ_2;
                استمر;
            }
            إذا (نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) == from.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) استمر;
            إذا (from.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {  // Only a greater prefix, an x bit above raised, can match
                إذا (حاصل < 0) أرجع خطأ;
                ثنائی_ہندسہ_2 = حاصل;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            نتیجہ.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
            طالما (--ثنائی_ہندسہ_2 >= 0) نتیجہ.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2));
            أرجع سچ;
        }
        أرجع سچ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ساكن فراغ crossRangeRuns(ثابت Cross_قدر_Range& range, ناپ_t حد_بندی,
                               std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ثابت صحيح sign = range.lo.چوڑائی() - 1;
        ثابت خودکار flipped = [sign](V3_نمبر قدر) {
            إذا (!قدر.ثنائی_ہندسہ_Is_XZ(sign)) قدر.سیٹ_ثنائی_ہندسہ(sign, !قدر.ثنائی_ہندسہ_Is1(sign));
            أرجع قدر;
        };
        ثابت V3_نمبر نمونہ_2 = flipped(range.نمونہ_2);
        ثابت V3_نمبر hi = flipped(range.hi);
        ثابت V3_نمبر one{&hi, hi.چوڑائی(), 1};
        V3_نمبر trailing{&hi, hi.چوڑائی()};
        لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 <= sign && نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2); ++ثنائی_ہندسہ_2) trailing.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        V3_نمبر from = flipped(range.lo);
        V3_نمبر پہلا{&hi, hi.چوڑائی()};
        V3_نمبر آخری{&hi, hi.چوڑائی()};
        V3_نمبر less{&hi};
        طالما (runs.ناپ() <= حد_بندی && اگلا_نمونہ_قدر(نمونہ_2, from, پہلا)
               && less.opLt(hi, پہلا).isEqZero()) {
            آخری.opOr(پہلا, trailing);
            إذا (!less.opLt(hi, آخری).isEqZero()) آخری = hi;
            runs.emplace_پٹھ(flipped(پہلا), flipped(آخری));
            إذا (آخری.isCaseEq(hi)) اكسر;
            from.op_جوڑو(آخری, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ساكن منطقي cross_Range_خالی_تھاواں(ثابت Cross_قدر_Range& range) {
        إذا (range.lo.isFourState()) أرجع سچ;
        أرجع cross_قدر_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ساكن صحيح resolve_چوڑائی(Ast_گرہ* nodep, ثابت Ast_گرہ_اظہاریہ* exprp) {
        أرجع std::max(exprp->چوڑائی(), cross_Range_چوڑائی(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ساكن منطقي resolve_قدر(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, منطقي bin_قدر, منطقي wildcard,
                             Cross_قدر_Range& range) {
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(nodep, exprp->چوڑائی(), exprp->isSigned(), range.lo.چوڑائی());
        أرجع cross_قدر_Range(nodep, exprp, bin_قدر, wildcard, domain, range);
    }

    ساكن منطقي crossRangesIntersect(ثابت Cross_قدر_Range& bin, ثابت Cross_قدر_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        إذا (bin.lo.isFourState() || filter.lo.isFourState()) أرجع خطأ;
        Cross_قدر_Range میل = bin;
        intersectCrossRange(میل, filter);
        أرجع !cross_قدر_Less(میل.hi, میل.lo)
               && (!bin.wildcard || crossWildcardIntersects(میل));
    }

    ساكن فراغ unsupportedCrossRange(AstCoverBinsof* selectp, منطقي& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = خطأ;
    }

    ساكن منطقي cross_قدر_Matches_Filters(AstCoverBinsof* selectp, Ast_گرہ* valuep,
                                         Ast_گرہ_اظہاریہ* exprp, ثابت AstCoverBin* binp,
                                         ثابت Cross_قدر_Range& domain,
                                         ثابت std::سمتیہ<Cross_قدر_Range>& filters,
                                         منطقي& valid) {
        Cross_قدر_Range range{valuep, domain.lo.چوڑائی()};
        إذا (!cross_قدر_Range(valuep, exprp, سچ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        لكل (ثابت Cross_قدر_Range& filter : filters) {
            إذا (crossRangesIntersect(range, filter)) أرجع سچ;
        }
        أرجع خطأ;
    }

    std::سمتیہ<منطقي> selectCoverpointBins(AstCoverBinsof* selectp, ثابت CoverpointBins& bins,
                                           uint32_t پہلا, uint32_t گنتی, منطقي& valid) {
        إذا (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            أرجع {};
        }
        std::سمتیہ<منطقي> selected(bins.کل, خطأ);
        std::سمتیہ<std::سمتیہ<Ast_گرہ*>> اقدار;
        صحيح چوڑائی = bins.exprp->چوڑائی();
        لكل (Ast_گرہ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            چوڑائی = std::max(چوڑائی, cross_Range_چوڑائی(rangep));
        }
        إذا (selectp->rangesp()) {
            اقدار.reserve(گنتی);
            لكل (uint32_t i = پہلا; i < پہلا + گنتی; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                اقدار.ڈالو_پٹھ(bins.اقدار[i].runp ? std::سمتیہ<Ast_گرہ*>{}
                                                     : cross_Bin_اقدار(bins.اقدار[i]));
                لكل (Ast_گرہ* ثابت valuep : اقدار.پٹھ()) {
                    چوڑائی = std::max(چوڑائی, cross_Range_چوڑائی(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++چوڑائی;
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(selectp, bins.exprp->چوڑائی(), bins.exprp->isSigned(), چوڑائی);
        std::سمتیہ<Cross_قدر_Range> filters;
        لكل (Ast_گرہ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_قدر_Range filter{rangep, چوڑائی};
            إذا (!cross_قدر_Range(rangep, bins.exprp, خطأ, خطأ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                أرجع {};
            }
            filters.ڈالو_پٹھ(std::move(filter));
        }
        لكل (uint32_t i = پہلا; i < پہلا + گنتی; ++i) {
            إذا (!selectp->rangesp()) {
                selected[i] = سچ;
                استمر;
            }
            إذا (ثابت Bin_لگاتار* ثابت runp = bins.اقدار[i].runp) {
                ثابت Cross_قدر_Range range
                    = لگاتار_Bin_Range(selectp, *runp, bins.اقدار[i].element, چوڑائی);
                selected[i] = std::any_دا(filters.begin(), filters.سرا(),
                                          [&](ثابت Cross_قدر_Range& filter) {
                                              أرجع crossRangesIntersect(range, filter);
                                          });
                استمر;
            }
            لكل (Ast_گرہ* ثابت valuep : اقدار[i - پہلا]) {
                selected[i] = cross_قدر_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.اقدار[i].binp, domain, filters, valid);
                إذا (!valid) أرجع {};
                إذا (selected[i]) اكسر;
            }
        }
        إذا (selectp->isNegated()) {
            لكل (uint32_t i = 0; i < bins.کل; ++i) selected[i] = !selected[i];
        }
        أرجع selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    بنية قدر_Lists final {
        std::سمتیہ<std::string> m_ranges;  // Bin, then low and high words
        std::سمتیہ<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::سمتیہ<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::سمتیہ<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ساكن فراغ append_الفاظ(std::string& متن, ثابت V3_نمبر& قدر, ثابت Ast_گرہ_اظہاریہ* exprp) {
        V3_نمبر narrowed{&قدر, exprp->چوڑائی()};
        narrowed.op_سونپو(قدر);
        لكل (صحيح لفظ = 0; لفظ < exprp->چوڑائی_الفاظ(); ++لفظ) {
            متن += ", " + cvt_منزل_Str(narrowed.edata_لفظ(لفظ)) + "U";
        }
    }

    فراغ collect_قدر_Metadata(قدر_Lists& lists, Ast_گرہ_اظہاریہ* exprp, AstCoverBin* binp,
                              uint32_t اشاریہ, Ast_گرہ_اظہاریہ* valuep) {
        ثابت std::string bin = cvt_منزل_Str(اشاریہ) + "U";
        إذا (binp->transp()) lists.m_transitions.ڈالو_پٹھ(bin);
        لكل (Ast_گرہ* ثابت sourcep : cross_Bin_اقدار({binp, valuep})) {
            Cross_قدر_Range range{sourcep, resolve_چوڑائی(sourcep, exprp)};
            إذا (!resolve_قدر(sourcep, exprp, سچ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                استمر;
            }
            إذا (cross_Range_خالی_تھاواں(range)) استمر;
            std::string اندراج = bin;
            إذا (range.wildcard) {
                V3_نمبر قدر{sourcep, exprp->چوڑائی()};
                V3_نمبر نقاب{sourcep, exprp->چوڑائی()};
                نقاب.op_بٹ_Non_XZ(range.نمونہ_2);
                قدر.op_بٹ_One(range.نمونہ_2);
                append_الفاظ(اندراج, قدر, exprp);
                append_الفاظ(اندراج, نقاب, exprp);
            }
            append_الفاظ(اندراج, range.lo, exprp);
            append_الفاظ(اندراج, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ڈالو_پٹھ(اندراج);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ساكن فراغ collect_لگاتار_Metadata(قدر_Lists& lists, Ast_گرہ_اظہاریہ* exprp, ثابت Bin_لگاتار& لگاتار) {
        V3_نمبر وسعت{exprp, لگاتار.m_stride.چوڑائی(), 0};
        وسعت.opSub(لگاتار.m_stride, V3_نمبر{exprp, لگاتار.m_stride.چوڑائی(), 1});
        std::string اندراج = cvt_منزل_Str(لگاتار.m_declared) + "U, " + cvt_منزل_Str(لگاتار.m_گنتی) + "U";
        append_الفاظ(اندراج, لگاتار.m_lo, exprp);
        append_الفاظ(اندراج, وسعت, exprp);
        append_الفاظ(اندراج, لگاتار.m_hi, exprp);
        lists.m_runs.ڈالو_پٹھ(اندراج);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    فراغ emit_قدر_List(File_سطر* fl, AstVar* cpVarp, VCMethod طریقہ,
                       ثابت std::سمتیہ<std::string>& entries) {
        لكل (ناپ_t پہلا = 0; پہلا < entries.ناپ(); پہلا += قدر_LIST_ENTRIES) {
            ثابت ناپ_t سرا = std::min(entries.ناپ(), پہلا + قدر_LIST_ENTRIES);
            std::string متن = "{" + entries[پہلا];
            لكل (ناپ_t i = پہلا + 1; i < سرا; ++i) متن += ", " + entries[i];
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, طریقہ, {ctext(fl, متن + "}")})->makeStmt());
        }
    }

    ساكن منطقي checkCrossRef(ثابت AstCoverCrossRef* refp, ثابت AstCoverCross* crossp) {
        إذا (refp->name() == crossp->name()) أرجع سچ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        أرجع خطأ;
    }

    ساكن منطقي checkCrossBinName(ثابت AstCoverCrossBin* binp, std::سیٹ<std::string>& ناں) {
        إذا (ناں.emplace(binp->name()).دوجا) أرجع سچ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        أرجع خطأ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ساكن منطقي implicit_Auto_Bin_وسعت(ثابت CoverpointBins& bins, ثابت std::string& name,
                                    Bin_وسعت& وسعت) {
        ثابت std::string اگیتر = "auto_";
        إذا (!VString::startsWith(name, اگیتر)) أرجع خطأ;
        ثابت std::string digits = name.substr(اگیتر.ناپ());
        ثابت بنانشانی لما اشاریہ = std::strtoul(digits.c_str(), خالی_سوچک, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        إذا (اشاریہ >= bins.implicitAuto.گنتی || digits != cvt_منزل_Str(اشاریہ)) أرجع خطأ;
        ثابت uint32_t وتھ = static_cast<uint32_t>(اشاریہ);
        وسعت = Bin_وسعت{bins.implicitAuto.پہلا + وتھ, 1, bins.implicitAuto.declared + وتھ};
        أرجع سچ;
    }

    Cross_Binsof_نشانہ
    resolve_Binsof_نشانہ(ثابت AstCoverBinsof* selectp, ثابت AstCoverCross* crossp,
                        ثابت std::سمتیہ<AstVar*>& cpVars,
                        ثابت std::map<std::string, uint32_t>& dimensions) ثابت {
        ثابت خودکار dim = dimensions.لبھو(selectp->pointp()->name());
        إذا (dim == dimensions.سرا()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            أرجع {};
        }
        ثابت CoverpointBins& bins = m_cpBins.at(cpVars[dim->دوجا]);
        Cross_Binsof_نشانہ نشانہ{&bins, dim->دوجا, 0, bins.کل};
        إذا (!selectp->name().خالی_تھاواں()) {
            ثابت خودکار bin = bins.spans.لبھو(selectp->name());
            Bin_وسعت وسعت{0, 0, 0};
            إذا (bin != bins.spans.سرا()) {
                وسعت = bin->دوجا;
            } وإلا إذا (!implicit_Auto_Bin_وسعت(bins, selectp->name(), وسعت)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                أرجع {};
            }
            نشانہ.m_پہلا = وسعت.پہلا;
            نشانہ.m_گنتی = وسعت.گنتی;
            نشانہ.m_declared_پہلا = وسعت.declared;
            نشانہ.m_declared_سرا = وسعت.declared + وسعت.گنتی;
            نشانہ.m_sized = وسعت.sized;
        }
        أرجع نشانہ;
    }

    // The variables a 'with' filter reads, each by its first reference
    ورتوں CrossFilterReads = std::map<ثابت AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ساكن منطقي counts_قدر_Tuples(ثابت AstCoverCrossWith* withp) {
        أرجع withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ساكن منطقي crossFilterSupported(ثابت AstCoverCrossWith* withp,
                                     ثابت CrossFilterReads& reads) {
        لكل (ثابت AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            ثابت خودکار it = reads.لبھو(varp);
            إذا (it != reads.سرا()) {
                it->دوجا->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->دوجا->prettyNameQ());
                أرجع خطأ;
            }
            إذا (counts_قدر_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                أرجع خطأ;
            }
        }
        أرجع سچ;
    }

    // Emit the constructor code selecting, of the bin tuples of the selection, those with enough
    // value tuples for which the 'with' filter is true: one, all for 'matches $', or a count
    // (IEEE 1800-2023 19.6.1.2).  The runtime gives the values of the cross items the filter
    // reads, of each value tuple it may evaluate:
    //   withBegin({dimensions}, count, positive, bin, file, line);
    //   while (withNext()) {
    //       item = withValue(dimension);  // Of each cross item read
    //       withTest(filter);
    //   }
    //   withEnd();
    // The dimensions are those of the items read, or to count value tuples, of all items: the
    // filter's result for a value tuple of the items it reads repeats for each of the others'.
    فراغ generateCrossFilter(ثابت AstCoverCrossWith* withp, ثابت CrossFilterReads& reads,
                             ثابت AstCoverCrossBin* binp, ثابت AstCoverCross* crossp,
                             AstVar* cxp, ثابت std::map<string, uint32_t>& dimensions,
                             uint32_t اشیاء) {
        File_سطر* ثابت fl = withp->fileline();
        ثابت string اگیتر
            = m_sized_ناں.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ثابت منطقي counting = counts_قدر_Tuples(withp);
        // The count, 0 for all
        ثابت std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> گنتی
            = counting ? new_گنتی_دلیلاں(fl, اگیتر + "_count", withp->countp())
                       : new_گنتی_دلیلاں(fl, withp->countp() ? 0 : 1);
        AstLoop* ثابت loopp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = شے_Call(fl, cxp, VCMethod::COVERGROUP_WITH_اگلا);
        nextp->dtype_سیٹ_ثنائی_ہندسہ();
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{fl, loopp, nextp});
        string dims;
        لكل (uint32_t dim = 0; counting && dim < اشیاء; ++dim) {
            dims += (dims.خالی_تھاواں() ? "" : ", ") + cvt_منزل_Str(dim) + "U";
        }
        طالما (AstVar* ثابت varp = withp->varsp()) {
            varp->unlink_Fr_پٹھ();
            إذا (!reads.گنتی(varp)) {
                VL_DO_DANGLING(ڈالو_Deletep(varp), varp);
                استمر;
            }
            ثابت خودکار dimIt = dimensions.لبھو(varp->name());
            UASSERT_OBJ(dimIt != dimensions.سرا(), varp, "Cross 'with' candidate not of an item");
            ثابت uint32_t dim = dimIt->دوجا;
            إذا (!counting) dims += (dims.خالی_تھاواں() ? "" : ", ") + cvt_منزل_Str(dim) + "U";
            varp->name(اگیتر + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->جوڑو_Stmtsp(varp);
            إذا (varp->isWide()) {
                loopp->جوڑو_Stmtsp(شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_قدر_W,
                                          {cnum(fl, dim), جديد AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } وإلا {
                AstCMethodHard* ثابت valuep
                    = شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_قدر, {cnum(fl, dim)});
                valuep->dtype_سیٹ_UInt64();
                Ast_گرہ_DType* ثابت dtypep = varp->لبھو_Logic_DType(
                    varp->چوڑائی(), varp->چوڑائی(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_قدر(valuep, dtypep)});
            }
        }
        loopp->جوڑو_Stmtsp(شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_پٹھ()})
                             ->makeStmt());
        ثابت منطقي prot = v3Global.opt.protectIds();
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), گنتی.پہلا, گنتی.دوجا,
                      ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->جوڑو_Stmtsp(loopp);
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_سرا)->makeStmt());
    }

    منطقي generateRuntimeSelection(Ast_گرہ* nodep, ثابت AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ثابت std::سمتیہ<AstVar*>& cpVars,
                                  ثابت std::map<string, uint32_t>& dimensions) {
        File_سطر* ثابت fl = nodep->fileline();
        إذا (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            إذا (!checkCrossRef(refp, crossp)) أرجع خطأ;
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            أرجع سچ;
        }
        إذا (ثابت AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            إذا (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                أرجع خطأ;
            }
            m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            أرجع سچ;
        }
        إذا (ثابت AstCoverCrossWith* ثابت withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            إذا ((counts_قدر_Tuples(withp)
                 && !check_گنتی(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                أرجع خطأ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.ناپ()));
            أرجع سچ;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_نشانہ نشانہ = resolve_Binsof_نشانہ(selectp, crossp, cpVars, dimensions);
        إذا (!نشانہ.m_binsp) أرجع خطأ;
        ثابت CoverpointBins& bins = *نشانہ.m_binsp;
        إذا (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            منطقي valid = سچ;
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        Ast_گرہ_اظہاریہ* firstp;
        Ast_گرہ_اظہاریہ* endp;
        إذا (نشانہ.m_sized < 0) {
            firstp = cnum(fl, نشانہ.m_declared_پہلا);
            endp = cnum(fl, نشانہ.m_declared_سرا);
        } وإلا {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ثابت cpVarp = cpVars[نشانہ.m_dimension];
            ثابت uint32_t sized = static_cast<uint32_t>(نشانہ.m_sized);
            firstp = شے_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_پہلا, {cnum(fl, sized)});
            endp = شے_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_سرا, {cnum(fl, sized)});
            firstp->dtype_سیٹ_UInt32();
            endp->dtype_سیٹ_UInt32();
        }
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, نشانہ.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != خالی_سوچک)})
                ->makeStmt());
        لكل (Ast_گرہ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, bins.exprp)};
            إذا (!resolve_قدر(rangep, bins.exprp, خطأ, خطأ, range)) {
                منطقي valid = سچ;
                unsupportedCrossRange(selectp, valid);
                أرجع خطأ;
            }
            إذا (cross_Range_خالی_تھاواں(range)) استمر;
            m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_قدر_Const(fl, range.lo, bins.exprp),
                                                new_قدر_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_سرا)->makeStmt());
        أرجع سچ;
    }

    std::سمتیہ<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ثابت std::سمتیہ<AstVar*>& cpVars,
                             ثابت std::map<string, uint32_t>& dimensions) {
        std::سمتیہ<AstCoverCrossBin*> bins;
        std::سیٹ<string> ناں;
        لكل (Ast_گرہ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            إذا (!checkCrossBinName(binp, ناں)) استمر;
            File_سطر* ثابت fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ثابت selectionp = جديد AstFunc{fl, "", خالی_سوچک, خالی_سوچک};
            منطقي supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            إذا (supported) m_constructorp->جوڑو_Stmtsp(selectionp->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
            VL_DO_DANGLING(ڈالو_Deletep(selectionp), selectionp);
            إذا (!supported) استمر;
            ثابت منطقي protect = v3Global.opt.protectIds();
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_سیٹ_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->پہلا_کالم()),
                          cnum(fl, static_cast<uint32_t>(bins.ناپ()))})
                    ->makeStmt());
            bins.ڈالو_پٹھ(binp);
        }
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        أرجع bins;
    }

    ساكن فراغ سیٹ_Cross_Selection_Range(CrossSelection& selection, uint64_t پہلا, uint64_t سرا) {
        طالما (پہلا < سرا) {
            ثابت بنانشانی ثنائی_ہندسہ_2 = پہلا % 64;
            ثابت بنانشانی بٹ = std::min<uint64_t>(64 - ثنائی_ہندسہ_2, سرا - پہلا);
            selection[VL_BITWORD_Q(پہلا)]
                |= (بٹ == 64 ? ~uint64_t{0} : (uint64_t{1} << بٹ) - 1) << ثنائی_ہندسہ_2;
            پہلا += بٹ;
        }
    }

    CrossSelection crossSelection(Ast_گرہ* nodep, CrossSelectionContext& ctx) {
        إذا (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            إذا (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = خطأ;
                أرجع {};
            }
            CrossSelection نتیجہ(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            سیٹ_Cross_Selection_Range(نتیجہ, 0, ctx.tuples);
            أرجع نتیجہ;
        }
        إذا (AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ثابت CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            إذا (!ctx.valid) أرجع {};
            لكل (ناپ_t i = 0; i < lhs.ناپ(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            أرجع lhs;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_نشانہ نشانہ
            = resolve_Binsof_نشانہ(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        إذا (!نشانہ.m_binsp) {
            ctx.valid = خطأ;
            أرجع {};
        }
        ثابت CoverpointBins& bins = *نشانہ.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(نشانہ.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ثابت std::سمتیہ<منطقي> selected
            = selectCoverpointBins(selectp, bins, نشانہ.m_پہلا, نشانہ.m_گنتی, ctx.valid);
        إذا (!ctx.valid) أرجع {};
        CrossSelection نتیجہ(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ثابت uint64_t stride = ctx.strides[نشانہ.m_dimension];
        ثابت uint64_t period = stride * bins.کل;
        لكل (uint64_t ادھار = 0; ادھار < ctx.tuples; ادھار += period) {
            لكل (uint32_t i = 0; i < bins.کل;) {
                إذا (!selected[i]) {
                    ++i;
                    استمر;
                }
                ثابت uint32_t begin = i++;
                طالما (i < bins.کل && selected[i]) ++i;
                سیٹ_Cross_Selection_Range(نتیجہ, ادھار + begin * stride, ادھار + i * stride);
            }
        }
        أرجع نتیجہ;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    منطقي crossShape(AstCoverCross* crossp, ثابت std::سمتیہ<AstVar*>& cpVars,
                    std::سمتیہ<uint32_t>& strides, uint32_t& tuples) ثابت {
        uint64_t ضرب_حاصل = std::any_دا(cpVars.begin(), cpVars.سرا(),
                                       [هذا](AstVar* varp) { أرجع !m_cpBins.at(varp).کل; })
                               ? 0
                               : 1;
        strides.resize(cpVars.ناپ());
        لكل (ناپ_t d = cpVars.ناپ(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(ضرب_حاصل);
            ضرب_حاصل *= m_cpBins.at(cpVars[d - 1]).کل;
            إذا (ضرب_حاصل > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                أرجع خطأ;
            }
        }
        tuples = static_cast<uint32_t>(ضرب_حاصل);
        أرجع سچ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ثابت std::سمتیہ<AstVar*>& cpVars,
                                   ثابت std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        إذا (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = خطأ;
            أرجع layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::سیٹ<std::string> ناں;
        لكل (Ast_گرہ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            إذا (!checkCrossBinName(binp, ناں)) استمر;
            ctx.valid = سچ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            إذا (!ctx.valid || std::all_دا(selection.begin(), selection.سرا(), [](uint64_t لفظ) {
                    أرجع لفظ == 0;
                })) {
                استمر;
            }
            إذا (occupied.خالی_تھاواں()) {
                occupied.resize(selection.ناپ(), 0);
                excluded.resize(selection.ناپ(), 0);
            }
            لكل (ناپ_t i = 0; i < selection.ناپ(); ++i) {
                occupied[i] |= selection[i];
                إذا (!binp->binsType().bin_Is_عام()) excluded[i] |= selection[i];
            }
            layout.bins.ڈالو_پٹھ({binp, std::move(selection)});
        }
        إذا (!layout.bins.خالی_تھاواں()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            لكل (ResolvedCrossBin& resolved : layout.bins) {
                لكل (ناپ_t i = 0; i < resolved.selection.ناپ(); ++i) {
                    إذا (resolved.binp->binsType().bin_Is_عام()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    إذا (resolved.selection[i]) ++layout.bin_الفاظ;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.سرا(),
                                             [](ثابت ResolvedCrossBin& resolved) {
                                                 أرجع std::all_دا(
                                                     resolved.selection.begin(),
                                                     resolved.selection.سرا(),
                                                     [](uint64_t لفظ) { أرجع لفظ == 0; });
                                             }),
                              layout.bins.سرا());
            layout.autoBins = layout.tuples;
            لكل (ثابت uint64_t لفظ : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{لفظ}.گنتی());
            }
        }
        أرجع layout;
    }

    AstCoverCrossDType* crossDType(File_سطر* fl, uint32_t dimensions, ثابت CrossLayout& layout,
                                   منطقي dynamic = خطأ) {
        ثابت uint32_t bins = static_cast<uint32_t>(layout.bins.ناپ());
        ثابت CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_الفاظ, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        إذا (!typep) {
            typep = جديد AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_الفاظ, dynamic};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(typep);
        }
        أرجع typep;
    }

    std::سمتیہ<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ثابت CrossLayout& layout) {
        std::سمتیہ<AstCoverCrossBin*> bins;
        لكل (ثابت ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ثابت binp = resolved.binp;
            ثابت CrossSelection& selection = resolved.selection;
            File_سطر* ثابت fl = binp->fileline();
            ثابت منطقي prot = v3Global.opt.protectIds();
            std::string نقاب = "{";
            لكل (ناپ_t i = 0; i < selection.ناپ(); ++i) {
                إذا (i) نقاب += ", ";
                نقاب += std::منزل_string(selection[i]) + "ULL";
            }
            نقاب += "}";
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cxVarp, VCMethod::COVERGROUP_جوڑو_BIN,
                         {ctext(fl, binp->binsType().bin_سیٹ_Enum()), ctext(fl, نقاب),
                          ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->پہلا_کالم()))})
                    ->makeStmt());
            bins.ڈالو_پٹھ(binp);
        }
        إذا (!bins.خالی_تھاواں()) {
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        أرجع bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    فراغ generateCross(AstCoverCross* crossp) {
        File_سطر* ثابت fl = crossp->fileline();
        ثابت منطقي dynamic = m_runtimeCrosses.گنتی(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        إذا (Ast_گرہ_اظہاریہ* ثابت iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_منزل_عارضی(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::سمتیہ<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        لكل (Ast_گرہ* itemp = crossp->itemsp(); itemp;) {
            Ast_گرہ* ثابت nextp = itemp->nextp();
            AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            ثابت خودکار it = m_cpVarMap.لبھو(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.سرا(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.ناپ()));
            cpVars.ڈالو_پٹھ(it->دوجا);
            VL_DO_DANGLING(ڈالو_Deletep(refp->unlink_Fr_پٹھ()), refp);
            itemp = nextp;
        }
        ثابت صحيح dims = static_cast<صحيح>(cpVars.ناپ());
        CrossLayout layout;
        إذا (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::سمتیہ<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } وإلا {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        إذا (!layout.valid) أرجع;

        AstVar* ثابت cxVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->جوڑو_Membersp(cxVarp);
        m_constructorp->جوڑو_Stmtsp(make_شے_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_جوڑو_CROSS_DYN
                                                         : VCMethod::COVERGROUP_جوڑو_CROSS));
        generate_شے_بھار(fl, cxVarp, crossp->optionsp());
        صحيح at_گھٹ_توں_گھٹ_قدر;
        صحيح autoBinMax;  // Not a cross option
        کڈھو_شے_Options(crossp->optionsp(), at_گھٹ_توں_گھٹ_قدر, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_الفاظ_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->جوڑو_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            شے_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_گھٹ_توں_گھٹ_قدر)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->پہلا_کالم()))})));
        ثابت std::سمتیہ<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        إذا (v3Global.opt.coverage()) {
            ثابت std::string ورقہ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cxVarp, VCMethod::COVERGROUP_رجسٹر_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(ورقہ)),
                                                cnum(fl, شے_Database_بھار(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_بھار)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ثابت منطقي hasIffs
            = std::any_دا(bins.begin(), bins.سرا(),
                          [](ثابت AstCoverCrossBin* binp) { أرجع binp->iffp() != خالی_سوچک; });
        Ast_گرہ_Stmt* ثابت samplep
            = !hasIffs ? static_cast<Ast_گرہ_Stmt*>(
                             شے_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_گرہ_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             شے_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        إذا (Ast_گرہ_اظہاریہ* ثابت iffp = crossp->iffp()) {
            m_sampleFuncp->جوڑو_Stmtsp(جديد AstIf{fl, iffp->clone_درخت(خطأ), samplep});
        } وإلا {
            m_sampleFuncp->جوڑو_Stmtsp(samplep);
        }
    }

    فراغ generate_Cross_کوڈ(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        لكل (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            إذا (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                أرجع;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        إذا (m_droppedCrosses.گنتی(crossp)) {
            لكل (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (m_coverpointMap.لبھو(refp->name()) == m_coverpointMap.سرا()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    اكسر;
                }
            }
            أرجع;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_گرہ_اظہاریہ* buildBinCondition(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        // Get the range list from the bin
        Ast_گرہ* ثابت rangep = binp->rangesp();
        إذا (!rangep) أرجع خالی_سوچک;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ثابت منطقي integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        إذا (binp->isWildcard() && !integral) أرجع wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_گرہ_اظہاریہ* بھریا_Condp = خالی_سوچک;

        لكل (Ast_گرہ* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_گرہ_اظہاریہ* rangeCondp = خالی_سوچک;
            currRangep = V3Const::constifyEdit(currRangep);

            إذا (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_گرہ_اظہاریہ* ثابت minExprp = irp->lhsp();
                Ast_گرہ_اظہاریہ* ثابت maxExprp = irp->rhsp();
                AstConst* ثابت minConstp = VN_CAST(minExprp, Const);
                AstConst* ثابت maxConstp = VN_CAST(maxExprp, Const);
                ثابت منطقي loUnbounded = VN_IS(minExprp, Unbounded);
                ثابت منطقي hiUnbounded = VN_IS(maxExprp, Unbounded);
                إذا (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ثابت boundp = hiUnbounded ? minConstp : maxConstp;
                    إذا (loUnbounded && hiUnbounded) {
                        rangeCondp = جديد AstConst{irp->fileline(), AstConst::ثنائی_ہندسہ_True{}};
                    } وإلا إذا (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        إذا (بھریا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھریا_Condp), بھریا_Condp);
                        أرجع خالی_سوچک;
                    } وإلا إذا (boundp->عدد().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        إذا (بھریا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھریا_Condp), بھریا_Condp);
                        أرجع خالی_سوچک;
                    } وإلا إذا (integral) {
                        rangeCondp = build_قدر_Condition(binp, exprp, irp);
                    } وإلا {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } وإلا إذا (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    إذا (بھریا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھریا_Condp), بھریا_Condp);
                    أرجع خالی_سوچک;
                } وإلا إذا (minConstp->عدد().isFourState() || maxConstp->عدد().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    إذا (بھریا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھریا_Condp), بھریا_Condp);
                    أرجع خالی_سوچک;
                } وإلا إذا (integral) {
                    rangeCondp = build_قدر_Condition(binp, exprp, irp);
                } وإلا {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } وإلا إذا (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_قدر_Condition(binp, exprp, constp);
            } وإلا {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                إذا (بھریا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھریا_Condp), بھریا_Condp);
                أرجع خالی_سوچک;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            بھریا_Condp
                = بھریا_Condp ? جديد AstOr{binp->fileline(), بھریا_Condp, rangeCondp} : rangeCondp;
        }

        أرجع بھریا_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ساكن Ast_گرہ_اظہاریہ* wildcardTypeError(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        أرجع جديد AstConst{binp->fileline(), AstConst::ثنائی_ہندسہ_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_گرہ_اظہاریہ* build_قدر_Condition(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ* valuep) {
        File_سطر* ثابت fl = valuep->fileline();
        إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع جديد AstEq{fl, exprp->clone_درخت(خطأ),
                             VN_AS(valuep, گرہ_اظہاریہ)->clone_درخت(خطأ)};
        }
        Cross_قدر_Range range{valuep, resolve_چوڑائی(valuep, exprp)};
        إذا (!resolve_قدر(valuep, exprp, سچ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}};
        }
        إذا (cross_Range_خالی_تھاواں(range)) أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}};
        AstConst* ثابت lop = new_قدر_Const(fl, range.lo, exprp);
        AstConst* ثابت hip = new_قدر_Const(fl, range.hi, exprp);
        Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
        إذا (lop->عدد().isCaseEq(hip->عدد())) {
            condp = جديد AstEq{fl, exprp->clone_درخت(خطأ), lop};
        } وإلا {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ڈالو_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ڈالو_Deletep(hip), hip);
        إذا (!range.wildcard) أرجع condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_نمبر نقاب{valuep, exprp->چوڑائی()};
        V3_نمبر قدر{valuep, exprp->چوڑائی()};
        نقاب.op_بٹ_Non_XZ(range.نمونہ_2);
        قدر.op_بٹ_One(range.نمونہ_2);
        AstConst* ثابت نقاب_Constp = new_قدر_Const(fl, نقاب, exprp);
        AstConst* ثابت قدر_Constp = new_قدر_Const(fl, قدر, exprp);
        Ast_گرہ_اظہاریہ* ثابت اظہاریہ_Masked = جديد AstAnd{fl, exprp->clone_درخت(خطأ), نقاب_Constp};
        Ast_گرہ_اظہاریہ* ثابت قدر_Masked = جديد AstAnd{fl, قدر_Constp, نقاب_Constp->clone_درخت(خطأ)};
        أرجع جديد AstLogAnd{fl, condp, جديد AstEq{fl, اظہاریہ_Masked, قدر_Masked}};
    }

    فراغ generate_Coverage_Computation_کوڈ() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ثابت getInstCoveragep
            = VN_AS(m_memberMap.لبھو_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_سطر* ثابت instFl = getInstCoveragep->fileline();
        AstCMethodHard* ثابت instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_سیٹ_Double();
        Ast_گرہ_اظہاریہ* instValuep = instCallp;
        إذا (m_cg_May_ملاؤ) {
            Ast_گرہ_اظہاریہ* ثابت mergedp = جديد AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(سچ), "merge_instances", VAccess::READ),
                جديد AstLogNot{instFl, newOptionSel(instFl, optionVar(خطأ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = جديد AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->جوڑو_Stmtsp(جديد Ast_سونپو{
            instFl, جديد AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ثابت getCoveragep
            = VN_AS(m_memberMap.لبھو_Member(m_covergroupp, "get_coverage"), Func);
        File_سطر* ثابت typeFl = getCoveragep->fileline();
        getCoveragep->جوڑو_Stmtsp(جديد Ast_سونپو{
            typeFl, جديد AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_سطر* fl) {
        AstCExpr* ثابت registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_سیٹ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp
            = جديد AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->جوڑو_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->جوڑو_Pinsp(newOptionSel(fl, optionVar(سچ), "weight", VAccess::READ));
        callp->جوڑو_Pinsp(newOptionSel(fl, optionVar(سچ), "merge_instances", VAccess::READ));
        callp->جوڑو_Pinsp(file_سطر_Debug(m_covergroupp->fileline()));
        callp->usePtr(سچ);
        callp->dtype_سیٹ_Double();
        أرجع callp;
    }

    // VISITORS
    ساكن منطقي isEnclosingInstanceVar(ثابت AstVar* varp) {
        أرجع varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    فراغ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ثابت AstClassRefDType* ثابت refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_گرہ_اظہاریہ* ثابت newp = جديد AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    فراغ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_سطر* ثابت fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    فراغ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_سطر* ثابت fl = refp->fileline();
        AstArg* ثابت argsp = refp->argsp() ? refp->argsp()->unlink_Fr_پٹھ_With_اگلا() : خالی_سوچک;
        Ast_طریقہ_Call* ثابت callp = جديد Ast_طریقہ_Call{
            fl, جديد AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    منطقي isEnclosingInstanceFunc(ثابت Ast_گرہ_FTask* funcp) ثابت {
        إذا (!funcp->class_طریقہ() || funcp->isStatic()) أرجع خطأ;
        أرجع AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    منطقي isEmbeddedCovergroupVar(ثابت AstVar* varp) ثابت {
        إذا (!varp || !varp->isClassMember() || varp->isDeclTyped()) أرجع خطأ;
        ثابت AstClassRefDType* ثابت refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        أرجع refp && refp->classp() == m_covergroupp;
    }

    AstVar* لبھو_Embedded_Covergroup_Var() ثابت {
        إذا (!m_enclosingClassp) أرجع خالی_سوچک;
        لكل (Ast_گرہ* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(itemp, Var)) {
                إذا (isEmbeddedCovergroupVar(varp)) أرجع varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        أرجع خالی_سوچک;  // LCOV_EXCL_LINE
    }

    std::سمتیہ<Ast_گرہ_سونپو*> لبھو_Covergroup_Constructions() {
        std::سمتیہ<Ast_گرہ_سونپو*> foundps;
        إذا (!m_embeddedVarp) أرجع foundps;
        AstFunc* ثابت enclosingNewp
            = VN_CAST(m_memberMap.لبھو_Member(m_enclosingClassp, "new"), Func);
        إذا (!enclosingNewp) أرجع foundps;
        enclosingNewp->foreach([&](Ast_گرہ_سونپو* asgnp) {
            ثابت AstNew* ثابت newp = VN_CAST(asgnp->rhsp(), New);
            ثابت AstVarRef* ثابت lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            إذا (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) أرجع;
            ثابت AstClassRefDType* ثابت refp = VN_CAST(newp->dtypep(), ClassRefDType);
            إذا (refp && refp->classp() == m_covergroupp) foundps.ڈالو_پٹھ(asgnp);
        });
        أرجع foundps;
    }

    std::سیٹ<ثابت AstVar*> enclosingInstanceVars() ثابت {
        std::سیٹ<ثابت AstVar*> vars;
        إذا (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {
                إذا (isEnclosingInstanceVar(varp)) vars.داخل_کرو(varp);
            });
        }
        أرجع vars;
    }

    منطقي hasEnclosingEventRef(AstCovergroup* cgp) ثابت {
        إذا (!m_embeddedVarp || !cgp->eventp()) أرجع خطأ;
        ثابت std::سیٹ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        منطقي لبھیا = خطأ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            إذا (enclosingVars.گنتی(refp->varp())) لبھیا = سچ;
        });
        أرجع لبھیا;
    }

    ساكن منطقي parse_Embedded_Event_اظہاریہ(Ast_گرہ_اظہاریہ* exprp, AstVar*& ادھار_Varp,
                                       AstVar*& memberVarp) {
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            ادھار_Varp = refp->varp();
            memberVarp = خالی_سوچک;
            أرجع سچ;
        }
        AstMemberSel* ثابت selp = VN_CAST(exprp, MemberSel);
        إذا (!selp) أرجع خطأ;
        AstVarRef* ثابت ادھار_Refp = VN_CAST(selp->fromp(), VarRef);
        إذا (!ادھار_Refp) أرجع خطأ;
        ادھار_Varp = ادھار_Refp->varp();
        memberVarp = selp->varp();
        أرجع سچ;
    }

    منطقي isEventLvalue(Ast_گرہ_اظہاریہ* exprp, ثابت EmbeddedEventTrigger& trigger) ثابت {
        إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* ادھار_Varp = خالی_سوچک;
        AstVar* memberVarp = خالی_سوچک;
        إذا (!parse_Embedded_Event_اظہاریہ(exprp, ادھار_Varp, memberVarp)) أرجع خطأ;
        أرجع ادھار_Varp == trigger.ادھار_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_گرہ_اظہاریہ* newEventRead(File_سطر* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_گرہ_اظہاریہ* ثابت basep = جديد AstVarRef{fl, trigger.ادھار_Varp, VAccess::READ};
        إذا (!trigger.memberVarp) أرجع basep;
        AstMemberSel* ثابت selp = جديد AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        أرجع selp;
    }

    string eventPrevName(ثابت EmbeddedEventTrigger& trigger, ناپ_t trigger_اشاریہ) ثابت {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::منزل_string(trigger_اشاریہ)
                      + "_" + trigger.ادھار_Varp->name();
        إذا (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        أرجع name;
    }

    Ast_گرہ_اظہاریہ* newEmbeddedVarNonNull(File_سطر* fl) ثابت {
        أرجع جديد AstNeq{fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          جديد AstConst{fl, AstConst::Null{}}};
    }

    Ast_گرہ_Stmt* newSampleStmt(File_سطر* fl) ثابت {
        Ast_طریقہ_Call* ثابت callp = جديد Ast_طریقہ_Call{
            fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", خالی_سوچک};
        callp->taskp(m_sampleFuncp);
        callp->dtype_سیٹ_Void();
        أرجع callp->makeStmt();
    }

    فراغ installEmbeddedEventFork(Ast_Sen_درخت* eventp,
                                  ثابت std::سمتیہ<Ast_گرہ_سونپو*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        لكل (Ast_گرہ_سونپو* ثابت constructp : constructps) {
            File_سطر* ثابت fl = constructp->fileline();
            AstLoop* ثابت loopp = جديد AstLoop{fl};
            loopp->جوڑو_Stmtsp(جديد AstEventControl{fl, eventp->clone_درخت(خطأ), خالی_سوچک});
            loopp->جوڑو_Stmtsp(جديد AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN_NONE};
            forkp->فوری_شروع(سچ);
            forkp->جوڑو_Forksp(جديد AstBegin{fl, "", loopp, سچ});
            constructp->جوڑو_اگلا_ایتھے(forkp);
        }
        VL_DO_DANGLING(ڈالو_Deletep(eventp), eventp);
    }

    Ast_گرہ_اظہاریہ* newEventReadyCondition(File_سطر* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_گرہ_اظہاریہ* ثابت curp = newEventRead(fl, trigger);
        Ast_گرہ_اظہاریہ* ثابت prevp = جديد AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_گرہ_اظہاریہ* edgep = خالی_سوچک;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        إذا (trigger.کنارا_Type == VEdgeType::ET_POSEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, curp, جديد AstNot{fl, prevp}}, 0, 1};
        } وإلا إذا (trigger.کنارا_Type == VEdgeType::ET_NEGEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, جديد AstNot{fl, curp}, prevp}, 0, 1};
        } وإلا إذا (trigger.کنارا_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = جديد AstSel{fl, جديد AstXor{fl, curp, prevp}, 0, 1};
        } وإلا {
            edgep = جديد AstNeq{fl, curp, prevp};
        }
        أرجع جديد AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::سمتیہ<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::سمتیہ<EmbeddedEventTrigger> triggers;
        ثابت std::سیٹ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        لكل (Ast_گرہ* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_شے* ثابت itemp = VN_AS(senp, Sen_شے);
            AstVar* ادھار_Varp = خالی_سوچک;
            AstVar* memberVarp = خالی_سوچک;
            إذا (!parse_Embedded_Event_اظہاریہ(itemp->sensp(), ادھار_Varp, memberVarp)
                || !enclosingVars.گنتی(ادھار_Varp)) {
                أرجع {};
            }
            triggers.emplace_پٹھ(itemp->fileline(), ادھار_Varp, memberVarp, itemp->کنارا_Type());
        }
        أرجع triggers;
    }

    فراغ installEmbeddedEventTriggers(std::سمتیہ<EmbeddedEventTrigger>& triggers,
                                      ثابت std::سمتیہ<Ast_گرہ_سونپو*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        إذا (constructps.خالی_تھاواں()) أرجع;
        لكل (ناپ_t trigger_اشاریہ = 0; trigger_اشاریہ < triggers.ناپ(); ++trigger_اشاریہ) {
            EmbeddedEventTrigger& trigger = triggers[trigger_اشاریہ];
            std::سمتیہ<Ast_گرہ_سونپو*> assignps;
            m_enclosingClassp->foreach([&](Ast_گرہ_سونپو* asgnp) {
                إذا (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ڈالو_پٹھ(asgnp);
            });
            إذا (assignps.خالی_تھاواں()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                استمر;
            }
            Ast_گرہ_DType* ثابت dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.ادھار_Varp->dtypep();
            AstVar* ثابت prevVarp = جديد AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, trigger_اشاریہ), dtypep};
            m_enclosingClassp->جوڑو_Membersp(prevVarp);
            trigger.prevVarp = prevVarp;
            لكل (Ast_گرہ_سونپو* ثابت asgnp : assignps) {
                File_سطر* ثابت fl = asgnp->fileline();
                AstIf* ثابت ifp
                    = جديد AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->جوڑو_اگلا_ایتھے(جديد Ast_سونپو{fl,
                                               جديد AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->جوڑو_اگلا_ایتھے(ifp);
            }
        }
    }

    فراغ delete_Coverage_اشیاء() {
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            VL_DO_DANGLING(ڈالو_Deletep(cpp->unlink_Fr_پٹھ()), cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            VL_DO_DANGLING(ڈالو_Deletep(crossp->unlink_Fr_پٹھ()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        لكل (Ast_Cg_Option_سونپو* ثابت optp : m_cgOptions) {
            VL_DO_DANGLING(ڈالو_Deletep(optp->unlink_Fr_پٹھ()), optp);
        }
        m_cgOptions.clear();
    }

    صنف FormalRefVisitor final : عام VNVisitor {
        ثابت std::map<ثابت AstVar*, AstVar*>& m_replacements;

        فراغ ویکھو(AstVarRef* nodep) override {
            ثابت خودکار it = m_replacements.لبھو(nodep->varp());
            إذا (it == m_replacements.سرا()) أرجع;
            nodep->varp(it->دوجا);
        }
        فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    عام:
        واضح FormalRefVisitor(ثابت std::map<ثابت AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        فراغ جانچ(Ast_گرہ* nodep) { iterate(nodep); }
    };

    فراغ validateCovergroupExpressions() {
        std::سیٹ<ثابت AstVar*> sampleMembers;
        std::سیٹ<ثابت AstVar*> constructorRefMembers;
        لكل (Ast_گرہ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.لبھو_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.داخل_کرو(varp);
            constructorRefMembers.داخل_کرو(memberp);
        }
        لكل (Ast_گرہ* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.لبھو_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.داخل_کرو(memberp);
        }
        Covergroup_اظہاریہ_Valid_Visitor{sampleMembers, constructorRefMembers}.جانچ(m_constructorp);
    }

    فراغ rebindFormalRefs() {
        std::map<ثابت AstVar*, AstVar*> replacements;
        لكل (Ast_گرہ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) استمر;
                AstVar* ثابت memberp
                    = VN_CAST(m_memberMap.لبھو_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        ناپ_t expectedBindings = 0;
        لكل (ثابت خودکار& pair : replacements) {
            إذا (pair.پہلا->isRef() || pair.پہلا->isConstRef()) ++expectedBindings;
        }
        ناپ_t rewrittenBindings = 0;
        لكل (Ast_گرہ* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_گرہ* ثابت nextp = stmtp->nextp();
            إذا (Ast_سونپو* ثابت assignp = VN_CAST(stmtp, سونپو)) {
                AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assignp->rhsp(), VarRef);
                إذا (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ثابت خودکار it = replacements.لبھو(rhsp->varp());
                    UASSERT_OBJ(it != replacements.سرا() && it->دوجا == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ثابت bindp = جديد AstCExpr{assignp->fileline(), ""};
                    bindp->جوڑو(lhsp->unlink_Fr_پٹھ());
                    bindp->جوڑو(" = &");
                    bindp->جوڑو(rhsp->unlink_Fr_پٹھ());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ڈالو_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) visitor.جانچ(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) visitor.جانچ(crossp);
    }

    AstVarRef* install_Enclosing_پٹھ_اشارہ_گر(ثابت std::سمتیہ<Ast_گرہ_سونپو*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        إذا (!m_enclosingClassp) أرجع خالی_سوچک;  // Offending refs require an enclosing class

        AstVarRef* invalidp = خالی_سوچک;
        Ast_گرہ* offenderp = خالی_سوچک;
        std::سیٹ<ثابت AstVar*> ownVars;
        لكل (Ast_گرہ* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) ownVars.داخل_کرو(varp);
        }
        ثابت std::سیٹ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        std::سمتیہ<AstVarRef*> refs_منزل_Rewrite;
        std::سمتیہ<AstThisRef*> this_Refs_منزل_Rewrite;
        std::سمتیہ<AstFuncRef*> func_Refs_منزل_Rewrite;
        ثابت خودکار جانچ = [&](Ast_گرہ* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                إذا (invalidp) أرجع;
                ثابت AstVar* ثابت varp = refp->varp();
                إذا (!isEnclosingInstanceVar(varp) || ownVars.گنتی(varp)) أرجع;
                إذا (!enclosingVars.گنتی(varp)) {
                    invalidp = refp;
                    أرجع;
                }
                refs_منزل_Rewrite.ڈالو_پٹھ(refp);
                إذا (!offenderp) offenderp = refp;
            });
            إذا (invalidp) أرجع;
            rootp->foreach([&](AstThisRef* refp) {
                ثابت AstClassRefDType* ثابت refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                إذا (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_منزل_Rewrite.ڈالو_پٹھ(refp);
                    إذا (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                إذا (!isEnclosingInstanceFunc(refp->taskp())) أرجع;
                func_Refs_منزل_Rewrite.ڈالو_پٹھ(refp);
                إذا (!offenderp) offenderp = refp;
            });
        };
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) جانچ(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) جانچ(crossp);
        لكل (Ast_Cg_Option_سونپو* ثابت optp : m_cgOptions) جانچ(optp);
        إذا (invalidp || !offenderp) أرجع invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_سطر* ثابت fl = m_covergroupp->fileline();
        AstClassRefDType* ثابت enclDTypep = جديد AstClassRefDType{fl, m_enclosingClassp, خالی_سوچک};
        enclDTypep->raw_اشارہ_گر(سچ);
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(enclDTypep);
        AstVar* ثابت handleVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->جوڑو_Membersp(handleVarp);
        AstVar* ثابت argumentp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::داخلہ);
        argumentp->declDirection(VDirection::داخلہ);
        argumentp->funcLocal(سچ);
        argumentp->noReset(سچ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->جوڑو_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(جديد Ast_سونپو{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    جديد AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        لكل (AstVarRef* ثابت refp : refs_منزل_Rewrite) { rewriteVarRef(refp, handleVarp); }
        لكل (AstThisRef* ثابت refp : this_Refs_منزل_Rewrite) { rewriteThisRef(refp, handleVarp); }
        لكل (AstFuncRef* ثابت refp : func_Refs_منزل_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        لكل (Ast_گرہ_سونپو* ثابت constructp : constructps) {
            File_سطر* ثابت cfl = constructp->fileline();
            AstCExpr* ثابت thisp = جديد AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->جوڑو_Argsp(جديد AstArg{cfl, argumentp->name(), thisp});
        }
        أرجع خالی_سوچک;
    }

    فراغ ویکھو(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        إذا (nodep->isCovergroup()) {
            VL_RESTORER(m_covergroupp);
            VL_RESTORER(m_embeddedVarp);
            VL_RESTORER(m_sampleFuncp);
            VL_RESTORER(m_constructorp);
            VL_RESTORER_CLEAR(m_coverpoints);
            VL_RESTORER_CLEAR(m_coverpointMap);
            VL_RESTORER_CLEAR(m_coverCrosses);
            VL_RESTORER_CLEAR(m_cgOptions);
            VL_RESTORER_CLEAR(m_covergroupName);
            m_covergroupp = nodep;
            m_embeddedVarp = لبھو_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = خالی_سوچک;
            m_constructorp = خالی_سوچک;
            std::سمتیہ<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_درخت* embeddedEventForkp = خالی_سوچک;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            منطقي hasUnsupportedEvent = خطأ;
            لكل (Ast_گرہ* itemp = nodep->membersp(); itemp;) {
                Ast_گرہ* ثابت nextp = itemp->nextp();
                إذا (AstCovergroup* ثابت cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    إذا (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        إذا (v3Global.opt.timing().is_سیٹ_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_پٹھ();
                        } وإلا {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            إذا (embeddedEventTriggers.خالی_تھاواں()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = سچ;
                            }
                        }
                        VL_DO_DANGLING(ڈالو_Deletep(cgp->unlink_Fr_پٹھ()), cgp);
                        itemp = nextp;
                        استمر;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    استمر;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.لبھو_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(سچ);
            m_constructorp = VN_CAST(m_memberMap.لبھو_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            إذا (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_اشیاء();
                إذا (embeddedEventForkp) {
                    VL_DO_DANGLING(ڈالو_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ثابت std::سمتیہ<Ast_گرہ_سونپو*> constructps = لبھو_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            إذا (AstVarRef* ثابت invalidp = install_Enclosing_پٹھ_اشارہ_گر(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_اشیاء();
                إذا (embeddedEventForkp) {
                    VL_DO_DANGLING(ڈالو_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            إذا (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_اشیاء();
        } وإلا {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    فراغ ویکھو(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ڈالو_پٹھ(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ڈالو_پٹھ(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    فراغ ویکھو(Ast_Cg_Option_سونپو* nodep) override { m_cgOptions.ڈالو_پٹھ(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ثابت std::سیٹ<ثابت AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = افتراضي;
};

//######################################################################
// Functional coverage class functions

فراغ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ثابت Covergroup_سونپو_Valid_Visitor validVisitor{nodep};
    إذا (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_درخت("coveragefunc", 0, dump_درخت_Either_Level() >= 3);
}
