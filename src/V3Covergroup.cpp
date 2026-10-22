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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Covergroup.h"

#شامل "V3Const.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3MemberMap.h"
#شامل "V3UniqueNames.h"

#شامل <bitset>
#شامل <cmath>
#شامل <deque>
#شامل <set>
#شامل <tuple>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

جماعت Covergroup_تفویض_Valid_Visitor final : عوامی VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ثابت AstVar*, ثابت Ast_گرہ_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::سیٹ<ثابت AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ثابت Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;
    منطقی m_collecting = سچ;
    منطقی m_valid = سچ;

    خالی دورہ(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = خالی_اشاریہ;
        اگر (m_collecting) {
            ثابت Ast_گرہ_FTask* ثابت constructorp
                = VN_CAST(m_memberMap.ڈھونڈو_Member(nodep, "new"), گرہ_FTask);
            برائے (ثابت Ast_گرہ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                اگر (!varp || !varp->isClassMember() || varp->isDeclTyped()) جاری;
                ثابت AstClassRefDType* ثابت refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                اگر (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (!m_collecting) {
            ثابت AstVar* varp = خالی_اشاریہ;
            اگر (ثابت Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(nodep->lhsp(), گرہ_Var_Ref)) {
                varp = refp->varp();
            } ورنہ اگر (ثابت AstMemberSel* ثابت selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ثابت خودکار it = m_constructors.ڈھونڈو(varp);
            اگر (it != m_constructors.انتہا() && (!m_ftaskp || m_ftaskp != it->دوسرا)) {
                m_valid = جھوٹ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        اگر (m_collecting && nodep->varp()->name() == "type_option") {
            ثابت AstStructSel* ثابت selp = VN_CAST(nodep->backp(), StructSel);
            اگر (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.داخل_کرو(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    صریح Covergroup_تفویض_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = جھوٹ;
        اگر (!m_constructors.خالی_خانے()) iterateConst(nodep);
    }
    منطقی valid() ثابت { واپس m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ثابت std::سیٹ<ثابت AstVar*>& mergeableTypeOptions() ثابت { واپس m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

جماعت Covergroup_اظہاریہ_Valid_Visitor final : عوامی VNVisitor {
    ثابت std::سیٹ<ثابت AstVar*>& m_sampleMembers;
    ثابت std::سیٹ<ثابت AstVar*>& m_constructorRefMembers;
    منطقی m_اندر_Coverage_عبارت = جھوٹ;
    منطقی m_sampleFormalAllowed = جھوٹ;

    خالی جائزہ_Sample_عبارت(Ast_گرہ* nodep) {
        اگر (!nodep) واپس;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = سچ;
        iterate_And_اگلا_Null(nodep);
    }

    خالی جائزہ_Coverage_عبارت(Ast_گرہ* nodep) {
        اگر (!nodep) واپس;
        VL_RESTORER(m_اندر_Coverage_عبارت);
        m_اندر_Coverage_عبارت = سچ;
        iterate_And_اگلا_Null(nodep);
    }

    خالی دورہ(AstCoverpoint* nodep) override {
        جائزہ_Sample_عبارت(nodep->exprp());
        جائزہ_Sample_عبارت(nodep->iffp());
        iterate_And_اگلا_Null(nodep->binsp());
        iterate_And_اگلا_Null(nodep->optionsp());
    }
    خالی دورہ(AstCoverCross* nodep) override {
        iterate_And_اگلا_Null(nodep->itemsp());
        جائزہ_Sample_عبارت(nodep->iffp());
        iterate_And_اگلا_Null(nodep->optionsp());
        iterate_And_اگلا_Null(nodep->binsp());
    }
    خالی دورہ(AstCoverCrossBin* nodep) override {
        iterate_And_اگلا_Null(nodep->selectp());
        جائزہ_Sample_عبارت(nodep->iffp());
    }
    خالی دورہ(AstCoverCrossWith* nodep) override {
        iterate_And_اگلا_Null(nodep->selectp());
        جائزہ_Coverage_عبارت(nodep->filterp());
        جائزہ_Coverage_عبارت(nodep->countp());
    }
    خالی دورہ(AstCoverBinsof* nodep) override { جائزہ_Coverage_عبارت(nodep->rangesp()); }
    خالی دورہ(AstCoverBin* nodep) override {
        جائزہ_Coverage_عبارت(nodep->rangesp());
        جائزہ_Sample_عبارت(nodep->iffp());
        جائزہ_Coverage_عبارت(nodep->arraySizep());
        جائزہ_Coverage_عبارت(nodep->transp());
    }
    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_sampleFormalAllowed && m_sampleMembers.گنتی(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        اگر (m_اندر_Coverage_عبارت && m_constructorRefMembers.گنتی(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        اگر (m_اندر_Coverage_عبارت && nodep->taskp()) {
            منطقی invalidDirection = جھوٹ;
            برائے (Ast_گرہ* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
                اگر (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = سچ;
                    توڑو;
                }
            }
            اگر (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    Covergroup_اظہاریہ_Valid_Visitor(ثابت std::سیٹ<ثابت AstVar*>& sampleMembers,
                               ثابت std::سیٹ<ثابت AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    خالی جائزہ(Ast_گرہ* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

جماعت Bin_تسلسل final {
عوامی:
    // MEMBERS
    uint32_t m_گنتی;  // Number of bins
    V3_نمبر m_lo;  // Lowest value of the first bin
    V3_نمبر m_stride;  // Number of values of each bin but the last
    V3_نمبر m_hi;  // Highest value of the last bin
    منطقی m_خالی_خانے = جھوٹ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_تسلسل(Ast_گرہ* nodep, صحیح چوڑائی, uint32_t گنتی)
        : m_گنتی{گنتی}
        , m_lo{nodep, چوڑائی}
        , m_stride{nodep, چوڑائی, 1}
        , m_hi{nodep, چوڑائی} {}
};

//######################################################################
// Functional coverage visitor

جماعت FunctionalCoverageVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    std::سیٹ<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::سیٹ<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = خالی_اشاریہ;  // Current covergroup being processed
    Ast_گرہ_Module* m_unitp = خالی_اشاریہ;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = خالی_اشاریہ;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = خالی_اشاریہ;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = خالی_اشاریہ;  // Current sample() function
    AstIf* m_cpGuardp = خالی_اشاریہ;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = خالی_اشاریہ;  // Current constructor
    std::سمتیہ<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::سمتیہ<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::سمتیہ<Ast_Cg_Option_تفویض*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_وزن = 1;  // The covergroup's type_option.weight, a constant
    منطقی m_cg_ملاؤ_Instances = جھوٹ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    منطقی m_cg_May_ملاؤ = جھوٹ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ثابت std::سیٹ<ثابت AstVar*>& m_mergeableTypeOptions;
    صحیح m_cg_At_کم_سے_کم = 1;  // Covergroup option.at_least

    ساخت EmbeddedEventTrigger final {
        File_سطر* eventFl;  // Clocking-event source location
        AstVar* بنیاد_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType کنارہ_Type;  // Clocking-event edge qualifier
        AstVar* پچھلا_Varp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_سطر* eventFl, AstVar* بنیاد_Varp, AstVar* memberVarp,
                             VEdgeType کنارہ_Type)
            : eventFl{eventFl}
            , بنیاد_Varp{بنیاد_Varp}
            , memberVarp{memberVarp}
            , کنارہ_Type{کنارہ_Type}
            , پچھلا_Varp{خالی_اشاریہ} {}
    };

    std::سیٹ<std::string> m_crossed_Cp_نام;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    ساخت BinRuns final {
        std::سمتیہ<Bin_تسلسل> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t گنتی = 0;  // Bins across all runs
        منطقی unsupported = جھوٹ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::سمتیہ<std::string> اقدار;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        منطقی single = جھوٹ;
    };
    ساخت Cross_Bin_اقدار final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_گرہ_اظہاریہ* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ثابت Bin_تسلسل* runp = خالی_اشاریہ;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    ساخت Bin_وسعت final {
        uint32_t پہلا = 0;  // First Normal index of the bin declaration
        uint32_t گنتی = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    ساخت CoverpointBins final {
        uint32_t کل = 0;  // Number of Normal bins
        Ast_گرہ_اظہاریہ* exprp = خالی_اشاریہ;  // Sampled expression, for the value domain
        منطقی crossed = جھوٹ;  // Feeds a cross, which needs 'values'
        std::سمتیہ<Cross_Bin_اقدار> اقدار;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_وسعت> spans;  // Declared bin name -> index span
        Bin_وسعت implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_تسلسل> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ثابت AstCoverpoint*, std::سمتیہ<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_نام m_sized_نام{"__Vsized"};
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_detached_اقدار;  // Array-bin values m_cpBins refers to
    std::سیٹ<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    استعمال CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, منطقی>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = خالی_اشاریہ;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(منطقی typeOption) {
        AstVar* ثابت varp = VN_AS(
            m_memberMap.ڈھونڈو_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        واپس varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_سطر* fl, AstVar* optionVarp, ثابت std::string& member,
                               VAccess access) {
        ثابت AstMemberDType* ثابت memberp
            = VN_AS(m_memberMap.ڈھونڈو_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_گرہ_اظہاریہ* ثابت fromp = optionVarp->lifetime().isStatic()
                                       ? نیا AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ثابت selp = نیا AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_مقصد_Enump());
        selp->did_چوڑائی(سچ);
        واپس selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ساکن صحیح const_At_کم_سے_کم(ثابت AstConst* constp) {
        ثابت صحیح قدر = constp->مقصد_SInt();
        اگر (قدر >= 0) واپس قدر;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        واپس 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ساکن ثابت AstConst* constOption(ثابت Ast_گرہ* valuep, VCoverOptionType optType) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) واپس constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        واپس خالی_اشاریہ;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    خالی زیریں_Covergroup_Options() {
        منطقی ملاؤ_سیٹ = جھوٹ;  // The covergroup sets type_option.merge_instances
        m_cg_At_کم_سے_کم = 1;
        برائے (Ast_Cg_Option_تفویض* ثابت optp : m_cgOptions) {
            اگر (optp->optType() == VCoverOptionType::AT_کم_سے_کم) {
                اگر (ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_کم_سے_کم = const_At_کم_سے_کم(constp);
                }
                VL_DO_DANGLING(ڈالو_Deletep(optp->unlink_Fr_عقب()), optp);
                جاری;
            }
            File_سطر* ثابت fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ثابت AstConst* ثابت constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            اگر (optp->optType() == VCoverOptionType::ملاؤ_INSTANCES) {
                member = "merge_instances";
                m_cg_ملاؤ_Instances = !constp->عدد().isEqZero();
                ملاؤ_سیٹ = سچ;
            } ورنہ اگر (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } ورنہ {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::وزن, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                اگر (optp->typeOption()) m_cg_Type_وزن = constp->مقصد_UInt();
            }
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_عقب()};
            اگر (optp->typeOption()) {
                m_covergroupp->جوڑو_Membersp(نیا AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ڈالو_Deletep(optp->unlink_Fr_عقب()), optp);
            } ورنہ {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ڈالو_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        اگر (!ملاؤ_سیٹ && v3Global.opt.coverage_ملاؤ_Instances()) {
            File_سطر* ثابت fl = m_covergroupp->fileline();
            m_covergroupp->جوڑو_Membersp(نیا AstInitialStatic{
                fl, نیا Ast_تفویض{
                        fl, newOptionSel(fl, optionVar(سچ), "merge_instances", VAccess::WRITE),
                        نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}}});
            m_cg_ملاؤ_Instances = سچ;
        }
        m_cg_May_ملاؤ = m_cg_ملاؤ_Instances || m_mergeableTypeOptions.گنتی(optionVar(سچ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t شے_Database_وزن(Ast_گرہ* optionsp) ثابت {
        ثابت Ast_گرہ_اظہاریہ* weightp = خالی_اشاریہ;  // The option.weight in effect
        uint32_t type_وزن = 1;
        برائے (Ast_گرہ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            اگر (!(optp->optType() == VCoverOptionType::وزن)) جاری;
            // V3Width left type_option.weight a non-negative constant
            اگر (optp->typeOption()) {
                type_وزن = VN_AS(optp->valuep(), Const)->مقصد_UInt();
            } ورنہ {
                weightp = optp->valuep();
            }
        }
        اگر (m_cg_ملاؤ_Instances) واپس type_وزن;
        اگر (!weightp) واپس 1;
        اگر (ثابت AstConst* ثابت constp = VN_CAST(weightp, Const)) واپس constp->مقصد_UInt();
        واپس type_وزن;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    خالی generate_شے_وزن(File_سطر* fl, AstVar* شے_Varp, Ast_گرہ* optionsp) {
        برائے (Ast_گرہ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            اگر (!(optp->optType() == VCoverOptionType::وزن)) جاری;
            اگر (!optp->typeOption()) {
                m_constructorp->جوڑو_Stmtsp(
                    شے_Call(fl, شے_Varp, VCMethod::COVERGROUP_وزن,
                             {optp->valuep()->clone_درخت(جھوٹ), file_سطر_Debug(optp->fileline())})
                        ->makeStmt());
            } ورنہ اگر (m_cg_May_ملاؤ) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->جوڑو_Stmtsp(شے_Call(fl, شے_Varp, VCMethod::COVERGROUP_TYPE_وزن,
                                                   {optp->valuep()->clone_درخت(جھوٹ)})
                                              ->makeStmt());
            }
        }
    }

    خالی processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.سائز() << " coverpoints and "
                                           << m_coverCrosses.سائز() << " crosses");

        m_crossed_Cp_نام.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_نام.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = خالی_اشاریہ;
        m_cg_Type_وزن = 1;
        m_cg_ملاؤ_Instances = جھوٹ;
        m_cg_May_ملاؤ = جھوٹ;

        زیریں_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        برائے (AstCoverCross* crossp : m_coverCrosses) {
            برائے (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                اگر (refp->exprp()) جاری;  // hierarchical ref: dropped in generateCrossCode
                اگر (m_coverpointMap.ڈھونڈو(refp->name()) == m_coverpointMap.انتہا()) {
                    m_droppedCrosses.داخل_کرو(crossp);  // bare variable: drop this cross only
                } ورنہ {
                    m_crossed_Cp_نام.داخل_کرو(refp->name());
                }
            }
        }

        std::سمتیہ<Ast_گرہ*> pending;
        std::map<AstCoverpoint*, std::سمتیہ<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::سمتیہ<AstCoverpoint*>> inputs;
        برائے (AstCoverpoint* ثابت cpp : m_coverpoints) {
            check_Bin_نام(cpp);
            checkConstructedBins(cpp);
            اگر (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) جاری;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            اگر (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_خالی_خانے_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                جاری;
            }
            m_runtimePoints.داخل_کرو(cpp);
            pending.ڈالو_عقب(cpp);
        }
        برائے (AstCoverCross* ثابت crossp : m_coverCrosses) {
            اگر (m_droppedCrosses.گنتی(crossp)) جاری;
            برائے (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                اگر (refp->exprp()) جاری;
                ثابت خودکار نقطہ = m_coverpointMap.ڈھونڈو(refp->name());
                اگر (نقطہ != m_coverpointMap.انتہا()) {
                    consumers[نقطہ->دوسرا].ڈالو_عقب(crossp);
                    inputs[crossp].ڈالو_عقب(نقطہ->دوسرا);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            اگر (crossp->exists([](ثابت AstCoverCrossWith*) { واپس سچ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ڈالو_عقب(crossp);
            }
        }
        برائے (سائز_t اگلا = 0; اگلا < pending.سائز(); ++اگلا) {
            اگر (AstCoverpoint* ثابت pointp = VN_CAST(pending[اگلا], Coverpoint)) {
                برائے (AstCoverCross* ثابت crossp : consumers[pointp]) {
                    اگر (m_runtimeCrosses.emplace(crossp).دوسرا) pending.ڈالو_عقب(crossp);
                }
            } ورنہ {
                برائے (AstCoverpoint* ثابت pointp : inputs[VN_AS(pending[اگلا], CoverCross)]) {
                    اگر (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).دوسرا) {
                        pending.ڈالو_عقب(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        برائے (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_رمز(cpp);

        // For each cross, generate sampling code
        برائے (AstCoverCross* crossp : m_coverCrosses) generate_Cross_رمز(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        برائے (AstCoverpoint* ثابت cpp : m_coverpoints) {
            اگر (!m_runtimePoints.گنتی(cpp)) جاری;
            m_constructorp->جوڑو_Stmtsp(شے_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_قدر_RELEASE)
                                          ->makeStmt());
        }
        برائے (Ast_گرہ_اظہاریہ* valuep : m_detached_اقدار) VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
        m_detached_اقدار.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_رمز();
    }

    ساکن constexpr سائز_t قدر_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ساکن int64_t const_گنتی(ثابت AstConst* constp) {
        ثابت V3_نمبر& عدد = constp->عدد();
        اگر (constp->isSigned() && عدد.isNegative()) واپس -1;
        واپس عدد.most_سیٹ_ثنائی_ہندسہ_P1() > 32 ? INT64_MAX : static_cast<int64_t>(عدد.مقصد_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ساکن منطقی check_گنتی(ثابت Ast_گرہ_اظہاریہ* countp, ثابت حرف* whatp, ثابت حرف* clausep) {
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        اگر (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            واپس جھوٹ;
        } ورنہ اگر (constp && (constp->عدد().isFourState() || const_گنتی(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->عدد().isFourState() ? constp->عدد().ascii(جھوٹ)
                                                                  : constp->عدد().مقصد_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            واپس جھوٹ;
        }
        واپس سچ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ساکن uint32_t autoBinsRequested(ثابت AstCoverBin* binp) {
        ثابت AstConst* ثابت constp = VN_CAST(binp->arraySizep(), Const);
        اگر (!constp) واپس 0;
        ثابت int64_t گنتی = const_گنتی(constp);
        واپس گنتی < 1 || گنتی > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(گنتی);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ساکن منطقی isAutoBins(ثابت AstCoverBin* binp) {
        واپس binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ساکن منطقی isSizedArray(ثابت AstCoverBin* binp) {
        واپس binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ساکن AstCoverWith* binWith(ثابت AstCoverBin* binp) {
        واپس VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ساکن منطقی isConstructedBins(ثابت AstCoverBin* binp) {
        واپس isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ساکن Ast_گرہ* binRangesp(ثابت AstCoverBin* binp) {
        ثابت AstCoverWith* ثابت withp = binWith(binp);
        اگر (!withp) واپس binp->rangesp();
        واپس VN_IS(withp->subp(), CoverpointRef) ? خالی_اشاریہ : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    خالی check_Bin_نام(AstCoverpoint* coverpointp) {
        std::سیٹ<std::string> نام;
        برائے (Ast_گرہ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            اگر (نام.emplace(binp->name()).دوسرا) جاری;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ڈالو_Deletep(binp->unlink_Fr_عقب()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    خالی dropBins(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ڈالو_عقب(binp->name());
        VL_DO_DANGLING(ڈالو_Deletep(binp->unlink_Fr_عقب()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    منطقی check_Bins_Array_سائز(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp, منطقی integral) {
        Ast_گرہ_اظہاریہ* ثابت sizep = binp->arraySizep();
        اگر (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } ورنہ اگر (check_گنتی(sizep, "Bins array size", "19.5.1")) {
            اگر (integral) واپس سچ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ڈالو_Deletep(sizep->unlink_Fr_عقب()), sizep);
            واپس جھوٹ;
        }
        dropBins(coverpointp, binp);
        واپس جھوٹ;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    خالی checkConstructedBins(AstCoverpoint* coverpointp) {
        ثابت منطقی integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        برائے (Ast_گرہ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            اگر (!isConstructedBins(binp)) جاری;
            اگر (isSizedArray(binp) && !check_Bins_Array_سائز(coverpointp, binp, integral)) جاری;
            اگر (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                اگر (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    اگر (binp->binsType().bin_Is_معمول()) coverpointp->user2(سچ);
                    dropBins(coverpointp, binp);
                }
                جاری;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ثابت منطقی single = !binp->binsType().bin_Is_معمول() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_مزید()
                                         << "... Suggest a larger --coverage-max-bins");
            اگر (single) {
                Ast_گرہ_اظہاریہ* ثابت sizep = binp->arraySizep();
                VL_DO_DANGLING(ڈالو_Deletep(sizep->unlink_Fr_عقب()), sizep);
                binp->isArray(جھوٹ);
                جاری;
            }
            اگر (binp->binsType().bin_Is_معمول()) coverpointp->user2(سچ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ساکن منطقی withCandidatesOver(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        ثابت صحیح چوڑائی = تسلسل_چوڑائی(exprp);
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs;
        اگر (!binRangesp(binp)) runs = coverpoint_اقدار(binp, exprp);
        ثابت خودکار constant
            = [](ثابت Ast_گرہ* nodep) { واپس VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        برائے (Ast_گرہ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            اگر (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                واپس جھوٹ;
            }
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            اگر (!resolve_قدر(rangep, exprp, سچ, binp->isWildcard(), range)
                || cross_Range_خالی_خانے(range)) {
                جاری;
            }
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> ملا{{range.lo, range.hi}};
            اگر (range.wildcard) {  // checkConstructedBins bounded the runs
                ملا.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), ملا);
            }
            برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& تسلسل : ملا) {
                runs.emplace_عقب(V3_نمبر{rangep, چوڑائی, تسلسل.پہلا},
                                  V3_نمبر{rangep, چوڑائی, تسلسل.دوسرا});
            }
        }
        اگر (!isSizedArray(binp)) {  // The union of the values
            std::ترتیب_دو(runs.begin(), runs.انتہا(), [](ثابت خودکار& lhs, ثابت خودکار& rhs) {
                واپس cross_قدر_Less(lhs.پہلا, rhs.پہلا);
            });
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> merged;
            برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& تسلسل : runs) {
                اگر (merged.خالی_خانے() || cross_قدر_Less(merged.عقب().دوسرا, تسلسل.پہلا)) {
                    merged.ڈالو_عقب(تسلسل);
                } ورنہ اگر (cross_قدر_Less(merged.عقب().دوسرا, تسلسل.دوسرا)) {
                    merged.عقب().دوسرا = تسلسل.دوسرا;
                }
            }
            runs = std::move(merged);
        }
        uint64_t گنتی = 0;
        برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& تسلسل : runs) {
            V3_نمبر وسعت{exprp, چوڑائی};
            وسعت.opSub(تسلسل.دوسرا, تسلسل.پہلا);
            اگر (وسعت.most_سیٹ_ثنائی_ہندسہ_P1() > 32) واپس سچ;  // More than 2**32 values
            گنتی += وسعت.مقصد_UQuad() + 1;
            اگر (گنتی > (uint64_t{1} << 32)) واپس سچ;
        }
        واپس جھوٹ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ساکن سائز_t sizedWildcardRuns(ثابت AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs;
        برائے (Ast_گرہ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            اگر (!VN_IS(rangep, Const)) جاری;  // A range, or a value known at construction
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            اگر (resolve_قدر(rangep, exprp, سچ, سچ, range) && !cross_Range_خالی_خانے(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        واپس runs.سائز();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    خالی checkAutomaticBins(AstCoverpoint* coverpointp, ثابت Ast_گرہ_اظہاریہ* exprp) {
        برائے (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            اگر (cbinp->binsType() != VCoverBinsType::BINS_AUTO) جاری;
            ثابت AstConst* ثابت constp = VN_CAST(cbinp->arraySizep(), Const);
            اگر (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } ورنہ اگر (const_گنتی(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->عدد().مقصد_Decimal_S());
            } ورنہ اگر (const_گنتی(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->عدد().مقصد_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_مزید() << "... Suggest a larger --coverage-max-bins");
            } ورنہ اگر (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    خالی نکالو_شے_Options(Ast_گرہ* optionsp, صحیح& at_کم_سے_کم_برآمد, صحیح& auto_Bin_Max_برآمد) {
        at_کم_سے_کم_برآمد = m_cg_At_کم_سے_کم;
        auto_Bin_Max_برآمد = -1;  // -1 = not set at coverpoint level
        برائے (Ast_گرہ* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ثابت optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            اگر (!(optp->optType() == VCoverOptionType::AT_کم_سے_کم
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                جاری;
            }
            ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType());
            اگر (!constp) جاری;
            اگر (optp->optType() == VCoverOptionType::AT_کم_سے_کم) {
                at_کم_سے_کم_برآمد = const_At_کم_سے_کم(constp);
            } ورنہ {
                auto_Bin_Max_برآمد = constp->مقصد_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        اگر (auto_Bin_Max_برآمد < 0) {
            اگر (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_برآمد = m_covergroupp->cgAutoBinMax();
            } ورنہ {
                auto_Bin_Max_برآمد = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    خالی createEnumAutoBins(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp,
                            ثابت AstEnumDType* enump) {
        File_سطر* ثابت fl = coverpointp->fileline();
        برائے (ثابت Ast_Enum_شے* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            AstConst* ثابت lop = new_قدر_Const(fl, VN_AS(itemp->valuep(), Const)->عدد(), exprp);
            AstInsideRange* ثابت rangep = نیا AstInsideRange{fl, lop, lop->clone_درخت(جھوٹ)};
            rangep->dtypeFrom(exprp);
            coverpointp->جوڑو_Binsp(
                نیا AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, جھوٹ, جھوٹ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    خالی createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp, صحیح autoBinMax) {
        اگر (coverpointp->user2()) واپس;  // Declared bins, ignored, leave no bins
        برائے (Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت VCoverBinsType قسم = VN_AS(nodep, CoverBin)->binsType();
            اگر (قسم != VCoverBinsType::BINS_IGNORE && قسم != VCoverBinsType::BINS_ILLEGAL)
                واپس;
        }
        اگر (ثابت AstEnumDType* ثابت enump
            = VN_CAST(exprp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            واپس;
        }
        ثابت صحیح چوڑائی = exprp->چوڑائی();
        uint32_t گنتی = چوڑائی < 31 ? std::min<uint32_t>(uint32_t{1} << چوڑائی, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        اگر (!گنتی) واپس;
        اگر (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            واپس;
        }
        اگر (گنتی > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_مزید()
                                              << "... Suggest a larger --coverage-max-bins");
            گنتی = v3Global.opt.coverageMaxBins();
        }
        File_سطر* ثابت fl = coverpointp->fileline();
        coverpointp->جوڑو_Binsp(نیا AstCoverBin{fl, "auto", نیا AstConst{fl, گنتی},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ساکن string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.انتہا(), '[', '_');
        std::replace(name.begin(), name.انتہا(), ']', '_');
        واپس name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_مقصد_عارضی(Ast_گرہ_اظہاریہ* iffp, ثابت string& عارضی_Name) {
        File_سطر* ثابت fl = iffp->fileline();
        AstVar* ثابت iffVarp
            = نیا AstVar{fl, VVarType::BLOCKTEMP, عارضی_Name, iffp->ڈھونڈو_ثنائی_ہندسہ_DType()};
        iffVarp->funcLocal(سچ);
        m_sampleFuncp->جوڑو_Stmtsp(iffVarp);
        iffp->unlink_Fr_عقب();
        m_sampleFuncp->جوڑو_Stmtsp(
            نیا Ast_تفویض{fl, نیا AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        واپس نیا AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    خالی جوڑو_Sample_Stmt(Ast_گرہ* stmtp) {
        اگر (m_cpGuardp) {
            m_cpGuardp->جوڑو_Thensp(stmtp);
        } ورنہ {
            m_sampleFuncp->جوڑو_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_پچھلا_قدر_Var(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp) {
        // Check if already created
        اگر (AstVar* ثابت پچھلا_Varp = VN_CAST(coverpointp->user1p(), Var)) واپس پچھلا_Varp;

        // Create variable to store previous sampled value
        ثابت string varName = "__Vprev_" + coverpointp->name();
        AstVar* پچھلا_Varp
            = نیا AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->جوڑو_Membersp(پچھلا_Varp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_گرہ_اظہاریہ* ثابت initExprp
            = نیا AstConst{پچھلا_Varp->fileline(), AstConst::Widthed_قدر{}, پچھلا_Varp->چوڑائی(), 0};
        Ast_گرہ_Stmt* ثابت initStmtp = نیا Ast_تفویض{
            پچھلا_Varp->fileline(), نیا AstVarRef{پچھلا_Varp->fileline(), پچھلا_Varp, VAccess::WRITE},
            initExprp};
        m_constructorp->جوڑو_Stmtsp(initStmtp);

        coverpointp->user1p(پچھلا_Varp);
        واپس پچھلا_Varp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ثابت string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = نیا AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->جوڑو_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_گرہ_Stmt* ثابت initStmtp = نیا Ast_تفویض{
            stateVarp->fileline(), نیا AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            نیا AstConst{stateVarp->fileline(), AstConst::Widthed_قدر{}, 8, 0}};
        m_constructorp->جوڑو_Stmtsp(initStmtp);

        واپس stateVarp;
    }

    خالی generate_Coverpoint_رمز(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_گرہ_اظہاریہ* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        صحیح at_کم_سے_کم_قدر;
        صحیح autoBinMax;
        نکالو_شے_Options(coverpointp->optionsp(), at_کم_سے_کم_قدر, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_کم_سے_کم_قدر << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        اگر (Ast_گرہ_اظہاریہ* ثابت iffp = coverpointp->iffp()) {
            m_cpGuardp = نیا AstIf{iffp->fileline(), iffp->unlink_Fr_عقب()};
        }

        AstVar* ثابت قدر_Varp = نیا AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        قدر_Varp->funcLocal(سچ);
        m_sampleFuncp->جوڑو_Stmtsp(قدر_Varp);
        exprp->unlink_Fr_عقب();
        جوڑو_Sample_Stmt(نیا Ast_تفویض{
            coverpointp->fileline(),
            نیا AstVarRef{coverpointp->fileline(), قدر_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(نیا AstVarRef{coverpointp->fileline(), قدر_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_کم_سے_کم_قدر);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        اگر (m_cpGuardp) m_sampleFuncp->جوڑو_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_گرہ_اظہاریہ* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp,
                                       File_سطر* fl) {
        Ast_گرہ_اظہاریہ* anyBinMatchp = خالی_اشاریہ;
        برائے (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            اگر (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                جاری;
            // The values of constructed bins are known at construction; see emitSizedSample
            اگر (isConstructedBins(cbinp)) جاری;
            اگر (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                اگر (anyBinMatchp) VL_DO_DANGLING(ڈالو_Deletep(anyBinMatchp), anyBinMatchp);
                واپس نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}};
            }
            Ast_گرہ_اظہاریہ* ثابت binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? نیا AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        واپس anyBinMatchp ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_گرہ_اظہاریہ*>(نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ساکن منطقی coverpointHasStateExclusions(ثابت AstCoverpoint* coverpointp) {
        برائے (ثابت Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            اگر (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    ساکن منطقی coverpointHasConstructedBins(ثابت AstCoverpoint* coverpointp) {
        برائے (ثابت Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            اگر (isConstructedBins(VN_AS(nodep, CoverBin))) واپس سچ;
        }
        واپس جھوٹ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ساکن منطقی coverpoint_Has_خالی_خانے_Bins(ثابت AstCoverpoint* coverpointp) {
        Ast_گرہ_اظہاریہ* ثابت exprp = coverpointp->exprp();
        برائے (Ast_گرہ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            اگر (!binp->binsType().bin_Is_معمول() || binp->transp() || !binp->rangesp()) جاری;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            اگر ((binp->isArray() && binp->isWildcard()) || binWith(binp)) جاری;
            منطقی خالی_خانے = سچ;
            برائے (Ast_گرہ* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                اگر (binp->isArray() && VN_IS(valuep, InsideRange)) جاری;
                Cross_قدر_Range range{valuep, resolve_چوڑائی(valuep, exprp)};
                ثابت منطقی none = resolve_قدر(valuep, exprp, سچ, binp->isWildcard(), range)
                                  && cross_Range_خالی_خانے(range);
                اگر (binp->isArray() && none) واپس سچ;
                خالی_خانے &= none;
            }
            اگر (خالی_خانے && !binp->isArray()) واپس سچ;
        }
        واپس جھوٹ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ساکن منطقی coverpointHasTransition(AstCoverpoint* coverpointp) {
        برائے (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            اگر (VN_AS(binp, CoverBin)->transp()) واپس سچ;
        }
        واپس جھوٹ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ساکن AstBasicDType* basicDType(File_سطر* fl, VBasicDTypeKwd kwd) {
        واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_سطر* fl, uint32_t کامیابی_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[کامیابی_Bound];
        اگر (!typep) {
            typep = نیا AstCoverpointDType{fl, کامیابی_Bound};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(typep);
        }
        واپس typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ثابت {
        واپس VIdProtect::protect_الفاظ_If(m_covergroupName, v3Global.opt.protectIds());
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
        واپس libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    خالی generateInstanceAttach() {
        File_سطر* ثابت fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = نیا AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->جوڑو_Membersp(m_cgInstVarp);

        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_ملاؤ ? ", true" : ", false") + ")")},
                     /*usePtr=*/جھوٹ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ثابت وزن_Addrp = نیا AstCExpr{fl, "&"};
        وزن_Addrp->جوڑو(newOptionSel(fl, optionVar(جھوٹ), "weight", VAccess::READ));
        m_constructorp->جوڑو_Stmtsp(شے_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_وزن,
                                           {وزن_Addrp, file_سطر_Debug(fl)}, /*usePtr=*/جھوٹ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_سطر* fl, VCMethod طریقہ) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ثابت instp
            = نیا AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(جھوٹ);
        instp->dtype_سیٹ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp = نیا AstCMethodHard{fl, instp, طریقہ};
        callp->usePtr(سچ);
        واپس callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_تفویض* make_شے_Create(File_سطر* fl, AstVar* شے_Varp, VCMethod طریقہ) {
        AstCMethodHard* ثابت createp = instanceCall(fl, طریقہ);
        createp->dtypep(شے_Varp->dtypep());
        واپس نیا Ast_تفویض{fl, memberRef(fl, شے_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    ساخت RangeBounds final {
        Ast_گرہ* loNodep = خالی_اشاریہ;  // low bound: AstConst or AstUnbounded
        Ast_گرہ* hiNodep = خالی_اشاریہ;  // high bound: AstConst or AstUnbounded
        منطقی loUnbounded() ثابت { واپس VN_IS(loNodep, Unbounded); }
        منطقی hiUnbounded() ثابت { واپس VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ثابت { واپس VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ثابت { واپس VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ساکن منطقی constRangeBounds(Ast_گرہ* rp, RangeBounds& rb) {
        اگر (AstInsideRange* ثابت irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } ورنہ اگر (AstConst* ثابت cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } ورنہ {
            واپس جھوٹ;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ثابت lc = rb.loConstp();
        AstConst* ثابت hc = rb.hiConstp();
        اگر ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) واپس جھوٹ;
        اگر ((lc && lc->عدد().isFourState()) || (hc && hc->عدد().isFourState())) واپس جھوٹ;
        واپس سچ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ساکن منطقی نکالو_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::سمتیہ<std::pair<uint64_t, uint64_t>>& برآمد) {
        اگر (!cbinp->rangesp()) واپس جھوٹ;
        برائے (Ast_گرہ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            اگر (!constRangeBounds(rp, rb)) واپس جھوٹ;
            اگر ((rb.loConstp() && rb.loConstp()->چوڑائی() > 64)
                || (rb.hiConstp() && rb.hiConstp()->چوڑائی() > 64)) {
                واپس جھوٹ;  // Use the safe slot-count bound for wide values.
            }
            ثابت uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->مقصد_UQuad();
            ثابت uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->مقصد_UQuad();
            اگر (lo > hi) واپس جھوٹ;
            برآمد.emplace_عقب(lo, hi);
        }
        واپس سچ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    منطقی appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_گرہ_اظہاریہ* exprp,
                             std::سمتیہ<std::سمتیہ<std::pair<uint64_t, uint64_t>>>& bins,
                             صحیح& خانہ_گنتی) {
        اگر (isAutoBins(cbinp)) {
            ++خانہ_گنتی;
            bins.ڈالو_عقب({{0, maxVal}});
            واپس exprp->چوڑائی() <= 64;
        }
        اگر (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            صحیح elements = 0;
            برائے (ثابت Ast_گرہ* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            خانہ_گنتی += std::max(1, elements);
            واپس جھوٹ;
        }
        اگر (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++خانہ_گنتی;
            اگر (exprp->isSigned() || exprp->چوڑائی() > 64) واپس جھوٹ;
            ثابت BinRuns runs = wildcardBinRuns(cbinp, exprp, جھوٹ);
            اگر (runs.unsupported) واپس جھوٹ;
            std::سمتیہ<std::pair<uint64_t, uint64_t>> ivs;
            برائے (ثابت Bin_تسلسل& تسلسل : runs.runs) {
                ivs.emplace_عقب(تسلسل.m_lo.مقصد_UQuad(), تسلسل.m_hi.مقصد_UQuad());
            }
            bins.ڈالو_عقب(std::move(ivs));
            واپس سچ;
        }
        اگر (cbinp->isArray()) واپس appendArrayBinCrossSlots(cbinp, exprp, bins, خانہ_گنتی);
        // Non-array bin: one slot covering the union of its intervals.
        ++خانہ_گنتی;
        std::سمتیہ<std::pair<uint64_t, uint64_t>> ivs;
        اگر (cbinp->isWildcard() || !نکالو_Range_Intervals(cbinp, maxVal, ivs)) واپس جھوٹ;
        bins.ڈالو_عقب(std::move(ivs));
        واپس سچ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    منطقی appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_گرہ_اظہاریہ* exprp,
                                  std::سمتیہ<std::سمتیہ<std::pair<uint64_t, uint64_t>>>& bins,
                                  صحیح& خانہ_گنتی) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        منطقی exact = !exprp->isSigned() && !cbinp->isWildcard();
        برائے (Ast_گرہ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++خانہ_گنتی;
            RangeBounds rb;
            Cross_قدر_Range range{rp, resolve_چوڑائی(rp, exprp)};
            اگر (!exact || !constRangeBounds(rp, rb)
                || !resolve_قدر(rp, exprp, سچ, جھوٹ, range)) {
                exact = جھوٹ;
                جاری;
            }
            اگر (cross_Range_خالی_خانے(range)) جاری;  // Its bin, if any, holds no value
            اگر (range.hi.most_سیٹ_ثنائی_ہندسہ_P1() > 64) {
                exact = جھوٹ;
                جاری;
            }
            bins.ڈالو_عقب({{range.lo.مقصد_UQuad(), range.hi.مقصد_UQuad()}});
        }
        واپس exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    صحیح compute_کامیابی_List_Bound(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp, منطقی crossFed) {
        اگر (!crossFed) واپس 1;
        ثابت صحیح چوڑائی = exprp->چوڑائی();
        ثابت uint64_t maxVal = (چوڑائی >= 64) ? UINT64_MAX : ((1ULL << چوڑائی) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::سمتیہ<std::سمتیہ<std::pair<uint64_t, uint64_t>>> bins;
        صحیح خانہ_گنتی = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        منطقی exact = !exprp->isSigned();
        برائے (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            اگر (!cbinp->binsType().bin_Is_معمول())
                جاری;  // ignore/illegal/default: not hit-listed
            اگر (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, خانہ_گنتی)) exact = جھوٹ;
        }
        اگر (!exact) واپس std::max(1, خانہ_گنتی);
        اگر (bins.خالی_خانے()) واپس 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::سمتیہ<uint64_t> pts;
        برائے (ثابت خودکار& b : bins)
            برائے (ثابت خودکار& iv : b) pts.ڈالو_عقب(iv.پہلا);
        std::ترتیب_دو(pts.begin(), pts.انتہا());
        pts.erase(std::unique(pts.begin(), pts.انتہا()), pts.انتہا());
        صحیح maxOverlap = 1;
        برائے (ثابت uint64_t p : pts) {
            صحیح cnt = 0;
            برائے (ثابت خودکار& b : bins) {
                برائے (ثابت خودکار& iv : b) {
                    اگر (iv.پہلا <= p && p <= iv.دوسرا) {
                        ++cnt;
                        توڑو;  // count each bin at most once
                    }
                }
            }
            اگر (cnt > maxOverlap) maxOverlap = cnt;
        }
        واپس maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_سطر* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ثابت refp = نیا AstVarRef{fl, varp, access};
        refp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}});
        واپس refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* شے_Call(File_سطر* fl, AstVar* varp, VCMethod طریقہ,
                             ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*>& دلائل = {}, منطقی usePtr = سچ) {
        AstCMethodHard* ثابت callp = نیا AstCMethodHard{fl, memberRef(fl, varp), طریقہ};
        برائے (Ast_گرہ_اظہاریہ* ثابت argp : دلائل) callp->جوڑو_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_سیٹ_Void();
        واپس callp;
    }

    // An unsigned integer argument.
    ساکن AstConst* cnum(File_سطر* fl, uint32_t قدر) { واپس نیا AstConst{fl, قدر}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ساکن AstCExpr* ctext(File_سطر* fl, ثابت std::string& متن) {
        واپس نیا AstCExpr{fl, متن};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ساکن std::string quoted(ثابت std::string& متن) {
        واپس "\"" + V3_برآمد_Formatter::quoteNameControls(متن) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ساکن AstCExpr* file_سطر_Debug(File_سطر* fl) {
        ثابت std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        واپس ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::مقصد_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ساکن منطقی checkArrayBinElement(AstCoverBin* arrayBinp, Ast_گرہ* rangep, منطقی report = سچ) {
        اگر (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            ثابت AstConst* ثابت minp = VN_CAST(irp->lhsp(), Const);
            ثابت AstConst* ثابت maxp = VN_CAST(irp->rhsp(), Const);
            اگر ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                اگر (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                واپس جھوٹ;
            }
            اگر ((minp && minp->عدد().isFourState()) || (maxp && maxp->عدد().isFourState())) {
                اگر (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                واپس جھوٹ;
            }
        } ورنہ اگر (!VN_IS(rangep, Const)) {
            اگر (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            واپس جھوٹ;
        }
        واپس سچ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::سمتیہ<Ast_گرہ_اظہاریہ*> نکالو_Array_اقدار(AstCoverBin* arrayBinp, Ast_گرہ_اظہاریہ* exprp,
                                                 منطقی& unsupported_برآمد) {
        unsupported_برآمد = جھوٹ;
        ثابت صحیح چوڑائی = exprp->چوڑائی();
        ثابت uint64_t maxVal = (چوڑائی >= 64) ? UINT64_MAX : ((1ULL << چوڑائی) - 1);
        std::سمتیہ<Ast_گرہ_اظہاریہ*> اقدار;
        برائے (Ast_گرہ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            اگر (!checkArrayBinElement(arrayBinp, rangep)) واپس اقدار;
            اگر (AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
                ثابت منطقی loUnb = VN_IS(irp->lhsp(), Unbounded);
                ثابت منطقی hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ثابت uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->مقصد_UQuad();
                ثابت uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->مقصد_UQuad();
                اگر (hi < lo) جاری;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ثابت uint64_t وسعت = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                اگر (وسعت >= v3Global.opt.coverageMaxRealBins()
                    || اقدار.سائز() + وسعت + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_مزید()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_برآمد = سچ;
                    برائے (Ast_گرہ_اظہاریہ* ثابت vp : اقدار) VL_DO_DANGLING(ڈالو_Deletep(vp), vp);
                    اقدار.clear();
                    واپس اقدار;
                }
                برائے (uint64_t v = lo; v <= hi; ++v)
                    اقدار.ڈالو_عقب(نیا AstConst{irp->fileline(), AstConst::Widthed_قدر{}, چوڑائی,
                                                  static_cast<uint32_t>(v)});
            } ورنہ {
                اقدار.ڈالو_عقب(VN_AS(rangep->clone_درخت(جھوٹ), گرہ_اظہاریہ));
            }
        }
        واپس اقدار;
    }

    ساکن صحیح تسلسل_چوڑائی(ثابت Ast_گرہ_اظہاریہ* exprp) { واپس exprp->چوڑائی() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    منطقی autoBinRuns(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp, BinRuns& برآمد) {
        ثابت uint32_t requested = autoBinsRequested(binp);
        اگر (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) واپس جھوٹ;
        ثابت صحیح چوڑائی = exprp->چوڑائی();
        ثابت صحیح arithmetic_چوڑائی = تسلسل_چوڑائی(exprp);
        ثابت uint32_t گنتی
            = چوڑائی < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << چوڑائی, requested))
                  : requested;
        Bin_تسلسل تسلسل{binp, arithmetic_چوڑائی, گنتی};
        V3_نمبر کل{binp, arithmetic_چوڑائی};
        کل.سیٹ_ثنائی_ہندسہ(چوڑائی, 1);
        تسلسل.m_stride.opDiv(کل, V3_نمبر{binp, arithmetic_چوڑائی, گنتی});
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(binp, چوڑائی, exprp->isSigned(), arithmetic_چوڑائی);
        تسلسل.m_lo = domain.lo;
        تسلسل.m_hi = domain.hi;
        برآمد.runs.ڈالو_عقب(std::move(تسلسل));
        برآمد.گنتی = گنتی;
        واپس سچ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_گرہ_اظہاریہ* exprp) {
        BinRuns برآمد;
        ثابت صحیح چوڑائی = تسلسل_چوڑائی(exprp);
        برائے (Ast_گرہ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            اگر (!checkArrayBinElement(arrayBinp, rangep)) واپس برآمد;
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            منطقی خالی_خانے = سچ;
            اگر (!resolve_قدر(rangep, exprp, سچ, جھوٹ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ورنہ {
                خالی_خانے = cross_Range_خالی_خانے(range);
            }
            اگر (خالی_خانے && irp) جاری;  // A range without values contributes no bins
            Bin_تسلسل تسلسل{rangep, چوڑائی, 1};
            تسلسل.m_خالی_خانے = خالی_خانے;
            uint64_t گنتی = 1;
            اگر (!خالی_خانے) {
                تسلسل.m_lo.op_تفویض(range.lo);
                تسلسل.m_hi.op_تفویض(range.hi);
                V3_نمبر وسعت{rangep, چوڑائی};
                وسعت.opSub(تسلسل.m_hi, تسلسل.m_lo);
                // Wider spans exceed any limit
                گنتی = وسعت.most_سیٹ_ثنائی_ہندسہ_P1() > 32 ? UINT64_MAX : وسعت.مقصد_UQuad() + 1;
            }
            اگر (گنتی > v3Global.opt.coverageMaxBins() - برآمد.گنتی) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_مزید()
                                                << "... Suggest a larger --coverage-max-bins");
                برآمد.runs.clear();
                برآمد.گنتی = 0;
                برآمد.unsupported = سچ;
                واپس برآمد;
            }
            تسلسل.m_گنتی = static_cast<uint32_t>(گنتی);
            برآمد.گنتی += تسلسل.m_گنتی;
            برآمد.runs.ڈالو_عقب(std::move(تسلسل));
        }
        واپس برآمد;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ساکن BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_گرہ_اظہاریہ* exprp, منطقی report) {
        BinRuns برآمد;
        ثابت صحیح چوڑائی = تسلسل_چوڑائی(exprp);
        ثابت V3_نمبر one{arrayBinp, چوڑائی, 1};
        // Disjoint runs of the values, in value order, each value once
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> spans;
        uint64_t گنتی = 0;  // Values of 'spans'
        برائے (Ast_گرہ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            اگر (!checkArrayBinElement(arrayBinp, rangep, report)) {
                برآمد.unsupported = سچ;
                واپس برآمد;
            }
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            اگر (!resolve_قدر(rangep, exprp, سچ, سچ, range)) {
                اگر (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                جاری;
            }
            اگر (cross_Range_خالی_خانے(range)) جاری;
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> ملا;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), ملا);
            برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& تسلسل : ملا) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_عقب(V3_نمبر{rangep, چوڑائی, تسلسل.پہلا},
                                   V3_نمبر{rangep, چوڑائی, تسلسل.دوسرا});
            }
            std::ترتیب_دو(spans.begin(), spans.انتہا(), [](ثابت خودکار& lhs, ثابت خودکار& rhs) {
                واپس cross_قدر_Less(lhs.پہلا, rhs.پہلا);
            });
            std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> merged;
            برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& وسعت : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_نمبر before{rangep, چوڑائی};
                before.opSub(وسعت.پہلا, one);
                اگر (merged.خالی_خانے() || cross_قدر_Less(merged.عقب().دوسرا, before)) {
                    merged.ڈالو_عقب(وسعت);
                } ورنہ اگر (cross_قدر_Less(merged.عقب().دوسرا, وسعت.دوسرا)) {
                    merged.عقب().دوسرا = وسعت.دوسرا;
                }
            }
            گنتی = 0;
            برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& وسعت : merged) {
                V3_نمبر سائز{rangep, چوڑائی};
                سائز.opSub(وسعت.دوسرا, وسعت.پہلا);
                // Beyond 2^32 values exceed any limit
                گنتی += سائز.most_سیٹ_ثنائی_ہندسہ_P1() > 32 ? uint64_t{1} << 33 : سائز.مقصد_UQuad() + 1;
                اگر (گنتی > v3Global.opt.coverageMaxBins()) توڑو;
            }
            spans = std::move(merged);
            اگر (گنتی > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                برآمد.single = !arrayBinp->binsType().bin_Is_معمول();
                اگر (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (برآمد.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_مزید()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                برآمد.unsupported = سچ;
                واپس برآمد;
            }
        }
        برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& وسعت : spans) {
            برآمد.runs.emplace_عقب(arrayBinp, چوڑائی, 0);
            Bin_تسلسل& تسلسل = برآمد.runs.عقب();
            تسلسل.m_lo = وسعت.پہلا;
            تسلسل.m_hi = وسعت.دوسرا;
            // A bin for each value, named by it in the coverpoint's type
            V3_نمبر قدر = وسعت.پہلا;
            جب (سچ) {
                ثابت V3_نمبر typed{arrayBinp, exprp->چوڑائی(), قدر};
                برآمد.اقدار.ڈالو_عقب(exprp->isSigned() ? typed.مقصد_Decimal_S() : typed.مقصد_Decimal_U());
                ++تسلسل.m_گنتی;
                اگر (قدر.isCaseEq(وسعت.دوسرا)) توڑو;
                V3_نمبر اگلا{arrayBinp, چوڑائی};
                قدر = اگلا.op_جوڑو(قدر, one);
            }
        }
        برآمد.گنتی = static_cast<uint32_t>(گنتی);
        واپس برآمد;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    منطقی binRunsFor(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp, BinRuns& برآمد) {
        اگر (isAutoBins(binp)) {
            اگر (!autoBinRuns(binp, exprp, برآمد)) برآمد.unsupported = سچ;
            واپس سچ;
        }
        اگر (binp->isArray() && binp->isWildcard()) {
            اگر (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_گرہ_اظہاریہ* ثابت falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ڈالو_Deletep(falsep), falsep);
                برآمد.unsupported = سچ;
            } ورنہ {
                برآمد = wildcardBinRuns(binp, exprp, سچ);
                اگر (برآمد.single) {
                    binp->isArray(جھوٹ);  // Generates as one bin
                    واپس جھوٹ;
                }
            }
            واپس سچ;
        }
        اگر (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            واپس جھوٹ;
        }
        برآمد = arrayBinRuns(binp, exprp);
        واپس سچ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_گرہ_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t گنتی, uint32_t declared,
                           ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*>& اقدار = {},
                           ثابت std::سمتیہ<std::string>& قدر_نام = {}) {
        File_سطر* ثابت fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ثابت uint32_t معمول_گنتی
            = binp->binsType().bin_Is_معمول() ? static_cast<uint32_t>(گنتی < 0 ? 1 : گنتی) : 0;
        ثابت Bin_وسعت وسعت{bins.کل, معمول_گنتی, declared};
        اگر (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = وسعت;  // Selected by bin, see implicitAutoBinSpan
        } ورنہ {
            bins.spans.emplace(binp->name(), وسعت);
        }
        bins.کل += معمول_گنتی;
        برائے (uint32_t i = 0; bins.crossed && i < معمول_گنتی; ++i) {
            bins.اقدار.ڈالو_عقب({binp, اقدار.خالی_خانے() ? خالی_اشاریہ : اقدار[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ثابت منطقی prot = v3Global.opt.protectIds();
        ثابت منطقی single = گنتی < 0;
        اگر (!قدر_نام.خالی_خانے()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_گرہ_Stmt* stmtsp = خالی_اشاریہ;
            برائے (ثابت std::string& قدر : قدر_نام) {
                ثابت std::string name
                    = VIdProtect::protect_الفاظ_If(binp->name(), prot) + "[" + قدر + "]";
                stmtsp = Ast_گرہ::جوڑو_اگلا(
                    stmtsp,
                    شے_Call(fl, cpVarp, VCMethod::COVERGROUP_جوڑو_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_سیٹ_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->پہلا_عمود()))})
                        ->makeStmt());
            }
            واپس stmtsp;
        }
        std::سمتیہ<Ast_گرہ_اظہاریہ*> دلائل{ctext(fl, binp->binsType().bin_سیٹ_Enum())};
        اگر (!single) دلائل.ڈالو_عقب(cnum(fl, static_cast<uint32_t>(گنتی)));  // value array bin
        دلائل.ڈالو_عقب(ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))));
        دلائل.ڈالو_عقب(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        دلائل.ڈالو_عقب(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        دلائل.ڈالو_عقب(cnum(fl, static_cast<uint32_t>(fl->پہلا_عمود())));
        واپس شے_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_جوڑو_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_جوڑو_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_جوڑو_ARRAY_NAMER,
                        دلائل)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    ساخت Conv_Bin_ہدف final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        منطقی is_معمول;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_گرہ_Stmt* make_Runtime_Bin_کامیابی(File_سطر* fl, AstVar* cpVarp, Ast_گرہ_اظہاریہ* idxp,
                                   منطقی is_معمول) {
        واپس شے_Call(fl, cpVarp,
                        is_معمول ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_ریکارڈ_کامیابی,
                        {idxp})
            ->makeStmt();
    }
    Ast_گرہ_Stmt* make_Runtime_Bin_کامیابی(File_سطر* fl, ثابت Conv_Bin_ہدف& tgt) {
        واپس make_Runtime_Bin_کامیابی(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_معمول);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_گرہ_اظہاریہ* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_گرہ_اظہاریہ* condp) {
        File_سطر* ثابت fl = binp->fileline();
        اگر (binp->iffp()) condp = نیا AstLogAnd{fl, binp->iffp()->clone_درخت(جھوٹ), condp};
        ثابت خودکار excluded = m_excludedVars.ڈھونڈو(cpVarp);
        اگر (excluded != m_excludedVars.انتہا() && !binp->transp()
            && (binp->binsType().bin_Is_معمول()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = نیا AstLogAnd{
                fl, نیا AstNot{fl, نیا AstVarRef{fl, excluded->دوسرا, VAccess::READ}}, condp};
        }
        واپس condp;
    }

    خالی emit_Conv_کامیابی_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_گرہ_اظہاریہ* idxp, Ast_گرہ_اظہاریہ* condp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گرہ* actionp = make_Runtime_Bin_کامیابی(fl, cpVarp, idxp, binp->binsType().bin_Is_معمول());
        اگر (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جوڑو_اگلا(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        جوڑو_Sample_Stmt(نیا AstIf{fl, binCondition(binp, cpVarp, condp), actionp, خالی_اشاریہ});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    خالی emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_گرہ_اظہاریہ* exprp, uint32_t sized, AstVar* matchedp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گرہ_اظہاریہ* enabledp = binCondition(binp, cpVarp, نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ثابت منطقی illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        اگر (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ثابت varp
                = نیا AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_مقصد_Str(sized),
                             binp->ڈھونڈو_ثنائی_ہندسہ_DType()};
            varp->funcLocal(سچ);
            m_sampleFuncp->جوڑو_Stmtsp(varp);
            جوڑو_Sample_Stmt(نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = نیا AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ثابت callp
            = شے_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_درخت(جھوٹ), enabledp});
        callp->dtype_سیٹ_ثنائی_ہندسہ();
        اگر (illegal) {
            جوڑو_Sample_Stmt(نیا AstIf{fl, نیا AstLogAnd{fl, callp, enabledp->clone_درخت(جھوٹ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } ورنہ اگر (matchedp && binp->binsType().bin_Is_معمول()) {
            جوڑو_Sample_Stmt(
                نیا Ast_تفویض{fl, نیا AstVarRef{fl, matchedp, VAccess::WRITE},
                              نیا AstOr{fl, نیا AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } ورنہ {
            جوڑو_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_عارضی(File_سطر* fl, ثابت string& name, Ast_گرہ_DType* dtypep) {
        AstVar* ثابت varp = نیا AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(سچ);
        m_constructorp->جوڑو_Stmtsp(varp);
        واپس varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ساکن Ast_گرہ_اظہاریہ* resize_قدر(Ast_گرہ_اظہاریہ* valuep, Ast_گرہ_DType* dtypep) {
        File_سطر* ثابت fl = valuep->fileline();
        اگر (valuep->چوڑائی() > dtypep->چوڑائی()) {
            valuep = نیا AstSel{fl, valuep, 0, dtypep->چوڑائی()};
        } ورنہ اگر (valuep->چوڑائی() < dtypep->چوڑائی()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstExtendS{fl, valuep, dtypep->چوڑائی()})
                         : نیا AstExtend{fl, valuep, dtypep->چوڑائی()};
        }
        valuep->dtypep(dtypep);
        واپس valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> new_گنتی_دلائل(File_سطر* fl, ثابت string& name,
                                                       Ast_گرہ_اظہاریہ* exprp) {
        AstVar* ثابت countp = constructor_عارضی(fl, name, exprp->dtypep());
        m_constructorp->جوڑو_Stmtsp(
            نیا Ast_تفویض{fl, نیا AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_درخت(جھوٹ)});
        ثابت خودکار گنتی_Ref = [&]() { واپس نیا AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ثابت zerop = نیا AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_گرہ_اظہاریہ* ثابت positivep
            = countp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstGtS{fl, گنتی_Ref(), zerop})
                                 : نیا AstNeq{fl, گنتی_Ref(), zerop};
        Ast_گرہ_اظہاریہ* valuep = resize_قدر(گنتی_Ref(), countp->ڈھونڈو_UInt64_DType());
        اگر (countp->چوڑائی() > VL_QUADSIZE) {
            valuep = نیا AstCond{
                fl,
                نیا AstRedOr{
                    fl, نیا AstSel{fl, گنتی_Ref(), VL_QUADSIZE, countp->چوڑائی() - VL_QUADSIZE}},
                نیا AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_سیٹ_UInt64();
        }
        واپس {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ساکن std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> new_گنتی_دلائل(File_سطر* fl, uint64_t گنتی) {
        واپس {نیا AstConst{fl, AstConst::Unsized64{}, گنتی},
                نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ساکن std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> coverpoint_اقدار(Ast_گرہ* nodep,
                                                                       Ast_گرہ_اظہاریہ* exprp) {
        ثابت صحیح چوڑائی = تسلسل_چوڑائی(exprp);
        std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs;
        ثابت AstEnumDType* ثابت enump = VN_CAST(exprp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        اگر (!enump) {
            ثابت Cross_قدر_Range domain
                = cross_قدر_Domain(nodep, exprp->چوڑائی(), exprp->isSigned(), چوڑائی);
            runs.emplace_عقب(domain.lo, domain.hi);
            واپس runs;
        }
        std::سمتیہ<V3_نمبر> اقدار;
        برائے (ثابت Ast_Enum_شے* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            ثابت V3_نمبر& عدد = VN_AS(itemp->valuep(), Const)->عدد();
            اگر (عدد.isFourState()) جاری;  // Not a coverpoint value (19.5.7)
            اقدار.emplace_عقب(nodep, چوڑائی);
            اگر (exprp->isSigned()) {
                اقدار.عقب().opExtendS(عدد, عدد.چوڑائی());
            } ورنہ {
                اقدار.عقب().op_تفویض(عدد);
            }
        }
        std::ترتیب_دو(اقدار.begin(), اقدار.انتہا(), cross_قدر_Less);
        ثابت V3_نمبر one{nodep, چوڑائی, 1};
        برائے (ثابت V3_نمبر& قدر : اقدار) {
            V3_نمبر اگلا{nodep, چوڑائی};
            اگر (!runs.خالی_خانے() && اگلا.op_جوڑو(runs.عقب().دوسرا, one).isCaseEq(قدر)) {
                runs.عقب().دوسرا = قدر;
            } ورنہ {
                runs.emplace_عقب(قدر, قدر);
            }
        }
        واپس runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    خالی generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_گرہ_اظہاریہ* exprp) {
        File_سطر* ثابت fl = binp->fileline();
        ثابت string سابقہ
            = m_sized_نام.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ثابت std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> گنتی
            = binp->arraySizep() ? new_گنتی_دلائل(fl, سابقہ + "_count", binp->arraySizep())
                                 : new_گنتی_دلائل(fl, 1);
        AstCoverWith* ثابت withp = binWith(binp);
        اگر (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& تسلسل : coverpoint_اقدار(binp, exprp)) {
                m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_قدر_Const(fl, تسلسل.پہلا, exprp),
                                                    new_قدر_Const(fl, تسلسل.دوسرا, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        برائے (Ast_گرہ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            اگر (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                جاری;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, سابقہ + "_" + cvt_مقصد_Str(element++));
        }
        اگر (withp) generateWithFilter(binp, withp, cpVarp, exprp, سابقہ);
        std::سمتیہ<Ast_گرہ_اظہاریہ*> دلائل{ctext(fl, binp->binsType().bin_سیٹ_Enum()), گنتی.پہلا,
                                       گنتی.دوسرا};
        // A filter's bins have the limit it began with
        اگر (!withp) دلائل.ڈالو_عقب(cnum(fl, v3Global.opt.coverageMaxBins()));
        ثابت منطقی prot = v3Global.opt.protectIds();
        دلائل.ڈالو_عقب(ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))));
        دلائل.ڈالو_عقب(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        دلائل.ڈالو_عقب(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        دلائل.ڈالو_عقب(cnum(fl, static_cast<uint32_t>(fl->پہلا_عمود())));
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_اختتام : VCMethod::COVERGROUP_SIZED_اختتام,
                     دلائل)
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
    خالی generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_گرہ_اظہاریہ* exprp, ثابت string& سابقہ) {
        File_سطر* ثابت fl = withp->fileline();
        ثابت منطقی wide = exprp->isWide();
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
        AstVar* ثابت valuep = constructor_عارضی(fl, سابقہ + "_value", قدر_DTypep);
        AstVar* ثابت lastp = constructor_عارضی(fl, سابقہ + "_last", قدر_DTypep);
        AstVar* ثابت firstp = constructor_عارضی(fl, سابقہ + "_first", قدر_DTypep);
        AstVar* ثابت runp = constructor_عارضی(fl, سابقہ + "_run", binp->ڈھونڈو_ثنائی_ہندسہ_DType());
        AstVar* ثابت morep = constructor_عارضی(fl, سابقہ + "_more", binp->ڈھونڈو_ثنائی_ہندسہ_DType());
        AstVar* ثابت itemp = withp->itemp()->unlink_Fr_عقب();
        itemp->name(سابقہ + "_item");
        m_constructorp->جوڑو_Stmtsp(itemp);
        ثابت خودکار ref = [&](AstVar* varp) { واپس نیا AstVarRef{fl, varp, VAccess::READ}; };
        ثابت خودکار تفویض_2 = [&](AstVar* varp, Ast_گرہ_اظہاریہ* rhsp) -> Ast_گرہ* {
            واپس نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ثابت خودکار flag = [&](AstVar* varp, منطقی قدر) {
            واپس تفویض_2(varp, قدر ? نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}
                                      : نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}});
        };
        ثابت خودکار bound = [&](VCMethod narrow, VCMethod wide_طریقہ, AstVar* varp) -> Ast_گرہ* {
            اگر (wide) {
                واپس شے_Call(fl, cpVarp, wide_طریقہ, {نیا AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ثابت callp = شے_Call(fl, cpVarp, narrow);
            callp->dtype_سیٹ_UInt64();
            واپس تفویض_2(varp, resize_قدر(callp, قدر_DTypep));
        };
        ثابت خودکار keep = [&](Ast_گرہ_اظہاریہ* lop, Ast_گرہ_اظہاریہ* hip) {
            AstCMethodHard* ثابت callp = شے_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_تسلسل_W : VCMethod::COVERGROUP_WITH_تسلسل,
                {lop, hip});
            callp->dtype_سیٹ_ثنائی_ہندسہ();
            واپس تفویض_2(morep, callp);
        };
        ثابت خودکار قدم = [&](منطقی اوپر) {
            AstConst* ثابت onep = نیا AstConst{fl, AstConst::Widthed_قدر{}, exprp->چوڑائی(), 1};
            Ast_گرہ_اظہاریہ* ثابت stepp
                = اوپر ? static_cast<Ast_گرہ_اظہاریہ*>(نیا Ast_جوڑو{fl, ref(valuep), onep})
                     : نیا AstSub{fl, ref(valuep), onep};
            stepp->dtypep(قدر_DTypep);
            واپس stepp;
        };
        AstLoop* ثابت innerp = نیا AstLoop{fl};
        innerp->جوڑو_Stmtsp(تفویض_2(itemp, ref(valuep)));
        innerp->جوڑو_Stmtsp(نیا AstIf{
            fl, withp->filterp()->unlink_Fr_عقب(),
            نیا AstIf{fl, نیا AstNot{fl, ref(runp)},
                      تفویض_2(firstp, ref(valuep))->جوڑو_اگلا(flag(runp, سچ))},
            نیا AstIf{fl, ref(runp), keep(ref(firstp), قدم(جھوٹ))->جوڑو_اگلا(flag(runp, جھوٹ))}});
        innerp->جوڑو_Stmtsp(نیا AstLoopTest{
            fl, innerp, نیا AstLogAnd{fl, ref(morep), نیا AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->جوڑو_Stmtsp(تفویض_2(valuep, قدم(سچ)));
        AstLoop* ثابت outerp = نیا AstLoop{fl};
        AstCMethodHard* ثابت nextp = شے_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_اگلا);
        nextp->dtype_سیٹ_ثنائی_ہندسہ();
        outerp->جوڑو_Stmtsp(نیا AstLoopTest{fl, outerp, nextp});
        outerp->جوڑو_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->جوڑو_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->جوڑو_Stmtsp(flag(runp, جھوٹ));
        outerp->جوڑو_Stmtsp(innerp);
        outerp->جوڑو_Stmtsp(نیا AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->جوڑو_Stmtsp(flag(morep, سچ));
        m_constructorp->جوڑو_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    خالی generateSizedElement(AstVar* cpVarp, ثابت AstCoverBin* binp, Ast_گرہ* rangep,
                              Ast_گرہ_اظہاریہ* exprp, ثابت string& سابقہ) {
        File_سطر* ثابت fl = rangep->fileline();
        ثابت VCMethod طریقہ = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
        Ast_گرہ_اظہاریہ* ثابت lowp = irp ? irp->lhsp() : VN_AS(rangep, گرہ_اظہاریہ);
        Ast_گرہ_اظہاریہ* ثابت highp = irp ? irp->rhsp() : خالی_اشاریہ;
        ثابت خودکار unbounded
            = [](ثابت Ast_گرہ_اظہاریہ* boundp) { واپس !boundp || VN_IS(boundp, Unbounded); };
        ثابت خودکار constant
            = [&](ثابت Ast_گرہ_اظہاریہ* boundp) { واپس unbounded(boundp) || VN_IS(boundp, Const); };
        ثابت خودکار integral = [&](ثابت Ast_گرہ_اظہاریہ* boundp) {
            واپس unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        اگر (constant(lowp) && constant(highp)) {
            ثابت خودکار fourState = [](ثابت Ast_گرہ_اظہاریہ* boundp) {
                ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
                واپس constp && constp->عدد().isFourState();
            };
            اگر (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                واپس;
            }
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, exprp)};
            اگر (!resolve_قدر(rangep, exprp, سچ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } ورنہ اگر (!cross_Range_خالی_خانے(range)) {
                std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>> runs{{range.lo, range.hi}};
                اگر (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                برائے (ثابت std::pair<V3_نمبر, V3_نمبر>& تسلسل : runs) {
                    m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cpVarp, طریقہ,
                                                       {new_قدر_Const(fl, تسلسل.پہلا, exprp),
                                                        new_قدر_Const(fl, تسلسل.دوسرا, exprp)})
                                                  ->makeStmt());
                }
            }
            واپس;
        }
        اگر (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            واپس;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ثابت خودکار bound_چوڑائی
            = [&](ثابت Ast_گرہ_اظہاریہ* boundp) { واپس unbounded(boundp) ? 0 : boundp->چوڑائی(); };
        ثابت صحیح چوڑائی = std::max({exprp->چوڑائی(), bound_چوڑائی(lowp), bound_چوڑائی(highp)}) + 1;
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(rangep, exprp->چوڑائی(), exprp->isSigned(), چوڑائی);
        AstVar* ثابت lop = constructor_عارضی(fl, سابقہ + "_lo", exprp->dtypep());
        lop->dtype_سیٹ_ثنائی_ہندسہ_Sized(چوڑائی, VSigning::SIGNED);
        AstVar* ثابت hip = constructor_عارضی(fl, سابقہ + "_hi", lop->dtypep());
        ثابت خودکار bound = [&](Ast_گرہ_اظہاریہ* boundp, ثابت V3_نمبر& حد_بندی) -> Ast_گرہ_اظہاریہ* {
            اگر (unbounded(boundp)) {
                AstConst* ثابت limitp = نیا AstConst{fl, حد_بندی};
                limitp->dtypeFrom(lop);
                واپس limitp;
            }
            Ast_گرہ_اظہاریہ* valuep = boundp->clone_درخت(جھوٹ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            اگر (exprp->isSigned() && boundp->چوڑائی() <= exprp->چوڑائی()) {
                valuep = resize_قدر(valuep, exprp->dtypep());
            }
            واپس resize_قدر(valuep, lop->dtypep());
        };
        ثابت خودکار ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            واپس نیا AstVarRef{fl, varp, access};
        };
        m_constructorp->جوڑو_Stmtsp(
            نیا Ast_تفویض{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->جوڑو_Stmtsp(
            نیا Ast_تفویض{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ثابت minp = نیا AstConst{fl, domain.lo};
        AstConst* ثابت maxp = نیا AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ثابت lowerp
            = نیا AstCond{fl, نیا AstLtS{fl, ref(lop), minp}, minp->clone_درخت(جھوٹ), ref(lop)};
        AstCond* ثابت upperp
            = نیا AstCond{fl, نیا AstGtS{fl, ref(hip), maxp}, maxp->clone_درخت(جھوٹ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->جوڑو_Stmtsp(نیا Ast_تفویض{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->جوڑو_Stmtsp(نیا Ast_تفویض{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->جوڑو_Stmtsp(نیا AstIf{fl, نیا AstLteS{fl, ref(lop), ref(hip)},
                                            شے_Call(fl, cpVarp, طریقہ,
                                                     {resize_قدر(ref(lop), exprp->dtypep()),
                                                      resize_قدر(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ساکن Ast_گرہ_اظہاریہ* تسلسل_Bin_اشاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, ثابت Bin_تسلسل& تسلسل) {
        اگر (تسلسل.m_گنتی == 1) واپس cnum(fl, تسلسل.m_declared);
        ثابت صحیح چوڑائی = exprp->چوڑائی();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_گرہ_اظہاریہ* indexp
            = نیا AstSub{fl, exprp->clone_درخت(جھوٹ), new_قدر_Const(fl, تسلسل.m_lo, exprp)};
        indexp->dtype_سیٹ_ثنائی_ہندسہ_Sized(چوڑائی, VSigning::UNSIGNED);
        V3_نمبر stride{fl, چوڑائی, 0};
        stride.op_تفویض(تسلسل.m_stride);
        اگر (stride.گنتی_Ones() != 1) {
            indexp = نیا AstDiv{fl, indexp, نیا AstConst{fl, stride}};
        } ورنہ اگر (!stride.isEqOne()) {
            indexp = نیا Ast_سرکاؤ_R{fl, indexp, نیا AstConst{fl, stride.most_سیٹ_ثنائی_ہندسہ_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ثابت صحیح ext_چوڑائی = تسلسل.m_lo.چوڑائی() + 1;
        V3_نمبر lo{fl, ext_چوڑائی, 0};
        lo.opExtendS(تسلسل.m_lo, تسلسل.m_lo.چوڑائی());
        V3_نمبر وسعت{fl, ext_چوڑائی, 0};
        وسعت.opExtendS(تسلسل.m_hi, تسلسل.m_hi.چوڑائی());
        وسعت.opSub(V3_نمبر{وسعت}, lo);
        V3_نمبر covered{fl, ext_چوڑائی, 0};
        covered.op_تفویض(تسلسل.m_stride);
        covered.opMul(V3_نمبر{covered}, V3_نمبر{fl, ext_چوڑائی, تسلسل.m_گنتی});
        V3_نمبر hasRemainder{fl, 1, 0};
        اگر (!hasRemainder.opGte(وسعت, covered).isEqZero()) {
            AstConst* ثابت lastp = نیا AstConst{fl, V3_نمبر{fl, چوڑائی, تسلسل.m_گنتی - 1}};
            indexp = نیا AstCond{fl, نیا AstGt{fl, indexp->clone_درخت(جھوٹ), lastp},
                                 lastp->clone_درخت(جھوٹ), indexp};
        }
        اگر (چوڑائی < VL_IDATASIZE) {
            indexp = نیا AstExtend{fl, indexp, VL_IDATASIZE};
        } ورنہ اگر (چوڑائی > VL_IDATASIZE) {
            indexp = نیا AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        واپس نیا Ast_جوڑو{fl, cnum(fl, تسلسل.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    خالی emit_تسلسل_کامیابی(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_گرہ_اظہاریہ* exprp, ثابت Bin_تسلسل& تسلسل) {
        File_سطر* ثابت fl = binp->fileline();
        AstConst* ثابت lop = new_قدر_Const(fl, تسلسل.m_lo, exprp);
        Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
        اگر (تسلسل.m_lo.isCaseEq(تسلسل.m_hi)) {
            condp = نیا AstEq{fl, exprp->clone_درخت(جھوٹ), lop};
        } ورنہ {
            AstConst* ثابت hip = new_قدر_Const(fl, تسلسل.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ڈالو_Deletep(lop), lop);
            VL_DO_DANGLING(ڈالو_Deletep(hip), hip);
        }
        emit_Conv_کامیابی_If(coverpointp, binp, cpVarp, تسلسل_Bin_اشاریہ(fl, exprp, تسلسل), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    خالی جوڑو_Conv_Trans_کامیابی_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ثابت Conv_Bin_ہدف& tgt,
                           Ast_گرہ_اظہاریہ* condp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گرہ* actionp = make_Runtime_Bin_کامیابی(fl, tgt);
        اگر (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جوڑو_اگلا(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        جوڑو_Sample_Stmt(نیا AstIf{fl, condp, actionp, خالی_اشاریہ});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    خالی generateCoverpoint(AstCoverpoint* coverpointp, Ast_گرہ_اظہاریہ* exprp, صحیح at_کم_سے_کم_قدر) {
        File_سطر* ثابت fl = coverpointp->fileline();
        ثابت منطقی dynamic = m_runtimePoints.گنتی(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ثابت منطقی crossFed = m_crossed_Cp_نام.گنتی(coverpointp->name()) != 0;
        ثابت صحیح کامیابی_Bound = compute_کامیابی_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << کامیابی_Bound);
        AstVar* ثابت cpVarp = نیا AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(کامیابی_Bound))};
        m_covergroupp->جوڑو_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->جوڑو_Stmtsp(make_شے_Create(fl, cpVarp, VCMethod::COVERGROUP_جوڑو_COVERPOINT));
        generate_شے_وزن(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        اگر (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_کامیابی_LIST)->makeStmt());
        }
        اگر (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ثابت excludedp
                = نیا AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->ڈھونڈو_ثنائی_ہندسہ_DType()};
            excludedp->funcLocal(سچ);
            m_sampleFuncp->جوڑو_Stmtsp(excludedp);
            AstCMethodHard* ثابت callp
                = شے_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_قدر_EXCLUDED_W
                                           : VCMethod::COVERGROUP_قدر_EXCLUDED,
                           {exprp->clone_درخت(جھوٹ)});
            callp->dtype_سیٹ_ثنائی_ہندسہ();
            جوڑو_Sample_Stmt(نیا Ast_تفویض{fl, نیا AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::سمتیہ<Ast_گرہ_Stmt*> namerStmts;
        std::سمتیہ<AstCoverBin*> defaultBins;
        std::سمتیہ<AstCoverBin*> sizedBins;
        std::سمتیہ<std::tuple<AstCoverBin*, uint32_t, Ast_گرہ_اظہاریہ*>> metadata;
        std::سمتیہ<ثابت Bin_تسلسل*> تسلسل_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        برائے (Ast_گرہ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            ثابت صحیح errorsBefore = dynamic ? V3Error::error_گنتی() : 0;
            اگر (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ڈالو_عقب(cbinp);
                جاری;
            }
            اگر (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_وسعت وسعت;
                وسعت.sized = static_cast<int32_t>(sizedBins.سائز());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), وسعت);
                sizedBins.ڈالو_عقب(cbinp);
                جاری;
            }
            اگر (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ڈالو_عقب(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ثابت Conv_Bin_ہدف tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_معمول()};
                برائے (Ast_گرہ* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_رمز(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_سیٹ));
                اگر (dynamic && V3Error::error_گنتی() == errorsBefore) {
                    metadata.emplace_عقب(cbinp, static_cast<uint32_t>(idx), خالی_اشاریہ);
                }
                ++idx;
                جاری;
            }
            BinRuns plan;
            اگر (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                اگر (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ڈالو_عقب(cbinp->name());
                    جاری;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ثابت uint32_t پہلا_قدر = bins.کل;
                ثابت uint32_t پہلا_Declared = static_cast<uint32_t>(idx);
                namerStmts.ڈالو_عقب(
                    makeNamer(cpVarp, cbinp, plan.گنتی, پہلا_Declared, {}, plan.اقدار));
                برائے (Bin_تسلسل& تسلسل : plan.runs) {
                    تسلسل.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ڈالو_عقب(std::move(تسلسل));
                    ثابت Bin_تسلسل& stored = bins.runs.عقب();
                    اگر (bins.crossed && cbinp->binsType().bin_Is_معمول()) {
                        ثابت uint32_t پہلا = پہلا_قدر + stored.m_declared - پہلا_Declared;
                        برائے (uint32_t element = 0; element < stored.m_گنتی; ++element) {
                            bins.اقدار[پہلا + element].runp = &stored;
                            bins.اقدار[پہلا + element].element = element;
                        }
                    }
                    اگر (!stored.m_خالی_خانے) {
                        emit_تسلسل_کامیابی(coverpointp, cbinp, cpVarp, exprp, stored);
                        اگر (dynamic && V3Error::error_گنتی() == errorsBefore) {
                            تسلسل_Metadata.ڈالو_عقب(&stored);
                        }
                    }
                    idx += stored.m_گنتی;
                }
                جاری;
            }
            اگر (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                منطقی unsupported = جھوٹ;
                std::سمتیہ<Ast_گرہ_اظہاریہ*> اقدار = نکالو_Array_اقدار(cbinp, exprp, unsupported);
                اگر (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ڈالو_عقب(cbinp->name());
                    جاری;
                }
                namerStmts.ڈالو_عقب(makeNamer(cpVarp, cbinp, static_cast<int64_t>(اقدار.سائز()),
                                               static_cast<uint32_t>(idx), اقدار));
                برائے (Ast_گرہ_اظہاریہ* valuep : اقدار) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_اقدار.ڈالو_عقب(valuep);
                    emit_Conv_کامیابی_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_قدر_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } ورنہ {
                namerStmts.ڈالو_عقب(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                اگر (Ast_گرہ_اظہاریہ* ثابت condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_کامیابی_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                اگر (dynamic && V3Error::error_گنتی() == errorsBefore) {
                    metadata.emplace_عقب(cbinp, static_cast<uint32_t>(idx), خالی_اشاریہ);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        برائے (ثابت std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_وسعت{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = خالی_اشاریہ;
        اگر (!defaultBins.خالی_خانے()
            && std::any_کا(sizedBins.begin(), sizedBins.انتہا(), [](ثابت AstCoverBin* binp) {
                   واپس binp->binsType().bin_Is_معمول();
               })) {
            sizedMatchedp = نیا AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->ڈھونڈو_ثنائی_ہندسہ_DType()};
            sizedMatchedp->funcLocal(سچ);
            m_sampleFuncp->جوڑو_Stmtsp(sizedMatchedp);
            جوڑو_Sample_Stmt(نیا Ast_تفویض{fl, نیا AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}});
        }
        برائے (uint32_t sized = 0; sized < sizedBins.سائز(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        برائے (AstCoverBin* ثابت defBinp : defaultBins) {
            File_سطر* ثابت dfl = defBinp->fileline();
            namerStmts.ڈالو_عقب(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_گرہ_اظہاریہ* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            اگر (sizedMatchedp) {
                condp = نیا AstLogAnd{
                    dfl, نیا AstNot{dfl, نیا AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_کامیابی_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        اگر (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        اگر (coverpointHasTransition(coverpointp)) {
            AstVar* ثابت پچھلا_Varp = VN_AS(coverpointp->user1p(), Var);
            جوڑو_Sample_Stmt(
                نیا Ast_تفویض{coverpointp->fileline(),
                              نیا AstVarRef{پچھلا_Varp->fileline(), پچھلا_Varp, VAccess::WRITE},
                              exprp->clone_درخت(جھوٹ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ثابت منطقی prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_الفاظ_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_کم_سے_کم_قدر)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        برائے (Ast_گرہ_Stmt* ثابت ns : namerStmts) m_constructorp->جوڑو_Stmtsp(ns);
        اگر (dynamic) {
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_قدر_TYPE,
                         {cnum(fl, exprp->چوڑائی()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            قدر_Lists lists;
            برائے (ثابت خودکار& اندراج_2 : metadata) {
                collect_قدر_Metadata(lists, exprp, std::get<0>(اندراج_2), std::get<1>(اندراج_2),
                                     std::get<2>(اندراج_2));
            }
            برائے (ثابت Bin_تسلسل* ثابت runp : تسلسل_Metadata) collect_تسلسل_Metadata(lists, exprp, *runp);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_RANGES, lists.m_ranges);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_RUNS, lists.m_runs);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_PATTERNS, lists.m_patterns);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_TRANSITIONS, lists.m_transitions);
            برائے (AstCoverBin* ثابت binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_قدر_FINALIZE)->makeStmt());
        }
        اگر (v3Global.opt.coverage()) {
            ثابت std::string صفحہ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, VCMethod::COVERGROUP_رجسٹر_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(صفحہ)),
                          cnum(fl, شے_Database_وزن(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_وزن)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    خالی generate_Multi_قدر_Transition_رمز(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_گرہ_اظہاریہ* exprp, ثابت Conv_Bin_ہدف& tgt,
                                          ثابت std::سمتیہ<Ast_Cover_Trans_شے*>& اشیاء) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << اشیاء.سائز() << " items");

        // Create state position variable
        AstVar* ثابت stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ثابت casep
            = نیا AstCase{binp->fileline(), VCaseType::CT_CASE,
                          نیا AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, خالی_اشاریہ};

        // Generate each case item in the switch statement
        برائے (سائز_t state = 0; state < اشیاء.سائز(); ++state) {
            Ast_Case_شے* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, اشیاء, state);
            casep->جوڑو_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_شے* ثابت defaultItemp = نیا Ast_Case_شے{
            binp->fileline(), خالی_اشاریہ,
            نیا Ast_تفویض{binp->fileline(),
                          نیا AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          نیا AstConst{binp->fileline(), AstConst::Widthed_قدر{}, 8, 0}}};
        casep->جوڑو_Itemsp(defaultItemp);

        جوڑو_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_شے* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_گرہ_اظہاریہ* exprp, ثابت Conv_Bin_ہدف& tgt,
                                             AstVar* stateVarp,
                                             ثابت std::سمتیہ<Ast_Cover_Trans_شے*>& اشیاء,
                                             سائز_t state) {
        File_سطر* ثابت fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_گرہ_اظہاریہ* ثابت مطابقت_Condp = build_Transition_شے_Condition(اشیاء[state], exprp);

        Ast_گرہ_Stmt* مطابقت_Actionp = خالی_اشاریہ;

        اگر (state == اشیاء.سائز() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            مطابقت_Actionp = make_Runtime_Bin_کامیابی(fl, tgt);

            // For illegal_bins, add error message
            اگر (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ثابت string err_پیغام = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                مطابقت_Actionp = مطابقت_Actionp->جوڑو_اگلا(makeIllegalBinAction(fl, err_پیغام));
            }

            // Reset state to 0
            مطابقت_Actionp = مطابقت_Actionp->جوڑو_اگلا(
                نیا Ast_تفویض{fl, نیا AstVarRef{fl, stateVarp, VAccess::WRITE},
                              نیا AstConst{fl, AstConst::Widthed_قدر{}, 8, 0}});
        } ورنہ {
            // Intermediate state: advance to next state
            مطابقت_Actionp = نیا Ast_تفویض{
                fl, نیا AstVarRef{fl, stateVarp, VAccess::WRITE},
                نیا AstConst{fl, AstConst::Widthed_قدر{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_گرہ_Stmt* no_مطابقت_Actionp = خالی_اشاریہ;
        اگر (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_گرہ_اظہاریہ* ثابت restartCondp = build_Transition_شے_Condition(اشیاء[0], exprp);

            UASSERT_OBJ(restartCondp, اشیاء[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_گرہ_Stmt* restartActionp
                = نیا Ast_تفویض{fl, نیا AstVarRef{fl, stateVarp, VAccess::WRITE},
                                نیا AstConst{fl, AstConst::Widthed_قدر{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_گرہ_Stmt* resetActionp
                = نیا Ast_تفویض{fl, نیا AstVarRef{fl, stateVarp, VAccess::WRITE},
                                نیا AstConst{fl, AstConst::Widthed_قدر{}, 8, 0}};

            no_مطابقت_Actionp = نیا AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_گرہ_Stmt* ثابت stmtp = نیا AstIf{fl, مطابقت_Condp, مطابقت_Actionp, no_مطابقت_Actionp};

        // Create case item for this state value
        Ast_Case_شے* ثابت caseItemp = نیا Ast_Case_شے{
            fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        واپس caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_گرہ_Stmt* makeIllegalBinAction(File_سطر* fl, ثابت string& err_پیغام) {
        AstDisplay* ثابت errorp
            = نیا AstDisplay{fl, VDisplayType::DT_ERROR, err_پیغام, خالی_اشاریہ, خالی_اشاریہ};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_گرہ*>(errorp)->جوڑو_اگلا(نیا AstStop{fl, سچ});
        واپس errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ساکن AstConst* new_قدر_Const(File_سطر* fl, ثابت V3_نمبر& قدر, ثابت Ast_گرہ_اظہاریہ* exprp) {
        V3_نمبر narrowed{fl, exprp->چوڑائی(), 0};
        narrowed.op_تفویض(قدر);
        AstConst* ثابت constp = نیا AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        واپس constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ساکن AstConst* newRealConst(AstConst* constp) {
        اگر (constp->عدد().isDouble()) واپس constp->clone_درخت(جھوٹ);
        V3_نمبر real{&constp->عدد(), 64};
        real.opIToRD(constp->عدد(), constp->isSigned());
        واپس نیا AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ساکن AstConst* widenConst(File_سطر* fl, AstConst* constp, صحیح ہدف_چوڑائی) {
        اگر (constp->چوڑائی() == ہدف_چوڑائی) واپس constp->clone_درخت(جھوٹ);
        V3_نمبر عدد{fl, ہدف_چوڑائی, 0};
        عدد.op_تفویض(constp->عدد());
        واپس نیا AstConst{fl, عدد};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_گرہ_اظہاریہ* makeRangeCondition(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* minp,
                                    Ast_گرہ_اظہاریہ* maxp) {
        ثابت صحیح اظہاریہ_چوڑائی = exprp->چوڑائی_Min();
        AstConst* ثابت minConstp = VN_AS(minp, Const);
        AstConst* ثابت maxConstp = VN_AS(maxp, Const);
        اگر (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            واپس نیا AstAnd{fl,
                              نیا AstGteD{fl, exprp->clone_درخت(جھوٹ), newRealConst(minConstp)},
                              نیا AstLteD{fl, exprp->clone_درخت(جھوٹ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ثابت minWidep = widenConst(fl, minConstp, اظہاریہ_چوڑائی);
        AstConst* ثابت maxWidep = widenConst(fl, maxConstp, اظہاریہ_چوڑائی);
        V3_نمبر minimum{fl, اظہاریہ_چوڑائی, 0};
        V3_نمبر زیادہ_سے_زیادہ{fl, اظہاریہ_چوڑائی, 0};
        زیادہ_سے_زیادہ.سیٹ_All_Bits1();
        اگر (exprp->isSigned()) {
            minimum.سیٹ_ثنائی_ہندسہ(اظہاریہ_چوڑائی - 1, 1);
            زیادہ_سے_زیادہ.سیٹ_ثنائی_ہندسہ(اظہاریہ_چوڑائی - 1, 0);
        }
        Ast_گرہ_اظہاریہ* lowerp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* upperp = خالی_اشاریہ;
        اگر (minWidep->عدد().isCaseEq(minimum)) {
            VL_DO_DANGLING(ڈالو_Deletep(minWidep), minWidep);
        } ورنہ {
            lowerp = exprp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(
                                             نیا AstGteS{fl, exprp->clone_درخت(جھوٹ), minWidep})
                                       : static_cast<Ast_گرہ_اظہاریہ*>(
                                             نیا AstGte{fl, exprp->clone_درخت(جھوٹ), minWidep});
        }
        اگر (maxWidep->عدد().isCaseEq(زیادہ_سے_زیادہ)) {
            VL_DO_DANGLING(ڈالو_Deletep(maxWidep), maxWidep);
        } ورنہ {
            upperp = exprp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(
                                             نیا AstLteS{fl, exprp->clone_درخت(جھوٹ), maxWidep})
                                       : static_cast<Ast_گرہ_اظہاریہ*>(
                                             نیا AstLte{fl, exprp->clone_درخت(جھوٹ), maxWidep});
        }
        اگر (lowerp && upperp) واپس نیا AstAnd{fl, lowerp, upperp};
        اگر (lowerp) واپس lowerp;
        اگر (upperp) واپس upperp;
        واپس نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_گرہ_اظہاریہ* makeOpenRangeCondition(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, AstConst* boundp,
                                        منطقی is_زیریں_Bound) {
        AstConst* ثابت widep = widenConst(fl, boundp, exprp->چوڑائی_Min());
        اگر (is_زیریں_Bound) {
            اگر (exprp->isSigned()) واپس نیا AstGteS{fl, exprp->clone_درخت(جھوٹ), widep};
            واپس نیا AstGte{fl, exprp->clone_درخت(جھوٹ), widep};
        }
        اگر (exprp->isSigned()) واپس نیا AstLteS{fl, exprp->clone_درخت(جھوٹ), widep};
        واپس نیا AstLte{fl, exprp->clone_درخت(جھوٹ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_گرہ_اظہاریہ* build_Transition_شے_Condition(Ast_Cover_Trans_شے* itemp, AstVar* varp) {
        Ast_گرہ_اظہاریہ* varRefp = نیا AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_گرہ_اظہاریہ* ثابت condp = build_Transition_شے_Condition(itemp, varRefp);
        VL_DO_DANGLING(ڈالو_Deletep(varRefp), varRefp);
        واپس condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_گرہ_اظہاریہ* build_Transition_شے_Condition(Ast_Cover_Trans_شے* itemp, Ast_گرہ_اظہاریہ* exprp) {
        Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;

        برائے (Ast_گرہ* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_گرہ_اظہاریہ* singleCondp = خالی_اشاریہ;
            valp = V3Const::constifyEdit(valp);
            AstConst* ثابت constp = VN_CAST(valp, Const);
            اگر (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                واپس نیا AstConst{valp->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}};
            }
            singleCondp
                = نیا AstEq{constp->fileline(), exprp->clone_درخت(جھوٹ), constp->clone_درخت(جھوٹ)};
            اگر (condp) {
                condp = نیا AstOr{itemp->fileline(), condp, singleCondp};
            } ورنہ {
                condp = singleCondp;
            }
        }

        واپس condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    خالی generate_Single_Transition_رمز(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_گرہ_اظہاریہ* exprp, ثابت Conv_Bin_ہدف& tgt,
                                      Ast_Cover_Trans_سیٹ* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ثابت پچھلا_Varp = create_پچھلا_قدر_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::سمتیہ<Ast_Cover_Trans_شے*> اشیاء;
        برائے (Ast_گرہ* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            اشیاء.ڈالو_عقب(VN_AS(itemp, Cover_Trans_شے));

        اگر (اشیاء.خالی_خانے()) {
            binp->v3error("Transition set without items");
            واپس;
        }

        اگر (اشیاء.سائز() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            واپس;
        } ورنہ اگر (اشیاء.سائز() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_گرہ_اظہاریہ* ثابت cond1p = build_Transition_شے_Condition(اشیاء[0], پچھلا_Varp);
            Ast_گرہ_اظہاریہ* ثابت cond2p = build_Transition_شے_Condition(اشیاء[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_گرہ_اظہاریہ* بھرا_Condp = نیا AstAnd{binp->fileline(), cond1p, cond2p};

            جوڑو_Conv_Trans_کامیابی_If(coverpointp, binp, tgt, بھرا_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } ورنہ {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_قدر_Transition_رمز(coverpointp, binp, exprp, tgt, اشیاء);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_سطر* fl, ثابت std::سمتیہ<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ثابت cs = نیا AstCStmt{fl};
        cs->جوڑو("{ VlCoverpoint* __Vcx_cps[] = {");
        برائے (سائز_t d = 0; d < cpVars.سائز(); ++d) {
            اگر (d != 0) cs->جوڑو(", ");
            cs->جوڑو(memberRef(fl, cpVars[d]));
        }
        cs->جوڑو("}; ");
        cs->جوڑو(callp);
        cs->جوڑو("; }");
        واپس cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_سطر* fl, ثابت std::سمتیہ<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ثابت cs = نیا AstCStmt{fl};
        cs->جوڑو("{ bool __Vcx_iffs[" + cvt_مقصد_Str(bins.سائز()) + "]; ");
        برائے (سائز_t i = 0; i < bins.سائز(); ++i) {
            ثابت AstCoverCrossBin* ثابت binp = bins[i];
            cs->جوڑو("__Vcx_iffs[" + cvt_مقصد_Str(i) + "] = ");
            cs->جوڑو(binp->iffp() ? binp->iffp()->clone_درخت(جھوٹ)
                                 : نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}});
            cs->جوڑو("; ");
        }
        cs->جوڑو(callp);
        cs->جوڑو("; }");
        واپس cs;
    }

    استعمال CrossSelection = std::سمتیہ<uint64_t>;
    ساخت ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    ساخت CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_الفاظ = 0;
        منطقی valid = سچ;
        std::سمتیہ<ResolvedCrossBin> bins;
    };
    ساخت CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ثابت std::سمتیہ<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ثابت std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::سمتیہ<uint32_t> strides;  // Flat-index stride per dimension
        منطقی valid = سچ;  // False if this explicit bin cannot be implemented
    };
    ساخت Cross_Binsof_ہدف final {
        ثابت CoverpointBins* m_binsp = خالی_اشاریہ;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_پہلا = 0;  // First declared normal bin in the selected span
        uint32_t m_گنتی = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_پہلا = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_انتہا = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    ساخت Cross_قدر_Range final {
        V3_نمبر lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_نمبر hi;  // Inclusive upper bound
        V3_نمبر نمونہ_2;  // Allowed bit values; all X for an ordinary interval
        منطقی wildcard = جھوٹ;  // A wildcard singleton rather than an exact four-state value

        Cross_قدر_Range(Ast_گرہ* nodep, صحیح چوڑائی)
            : lo{nodep, چوڑائی}
            , hi{nodep, چوڑائی}
            , نمونہ_2{nodep, چوڑائی} {
            نمونہ_2.سیٹ_All_بٹ_X();
        }
    };

    ساکن std::سمتیہ<Ast_گرہ*> cross_Bin_اقدار(ثابت Cross_Bin_اقدار& bin) {
        اگر (bin.valuep) واپس {bin.valuep};
        std::سمتیہ<Ast_گرہ*> اقدار;
        اگر (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            برائے (Ast_گرہ* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_شے* lastp = VN_AS(setp, Cover_Trans_سیٹ)->itemsp();
                جب (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_شے);
                برائے (Ast_گرہ* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    اقدار.ڈالو_عقب(valuep);
                }
            }
        } ورنہ {
            برائے (Ast_گرہ* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                اقدار.ڈالو_عقب(valuep);
            }
        }
        واپس اقدار;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ساکن Cross_قدر_Range تسلسل_Bin_Range(Ast_گرہ* nodep, ثابت Bin_تسلسل& تسلسل, uint32_t element,
                                       صحیح چوڑائی) {
        ثابت صحیح runw = تسلسل.m_lo.چوڑائی();
        V3_نمبر انحراف{nodep, runw};
        انحراف.opMul(تسلسل.m_stride, V3_نمبر{nodep, runw, element});
        V3_نمبر lo{nodep, runw};
        lo.op_جوڑو(تسلسل.m_lo, انحراف);
        V3_نمبر hi = تسلسل.m_hi;
        اگر (element + 1 < تسلسل.m_گنتی) {
            انحراف.opSub(تسلسل.m_stride, V3_نمبر{nodep, runw, 1});
            hi.op_جوڑو(lo, انحراف);
        }
        Cross_قدر_Range range{nodep, چوڑائی};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        واپس range;
    }

    ساکن صحیح cross_Range_چوڑائی(Ast_گرہ* nodep) {
        اگر (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            واپس std::max(rangep->lhsp()->چوڑائی(), rangep->rhsp()->چوڑائی());
        }
        واپس nodep->چوڑائی();
    }

    ساکن منطقی cross_قدر_Less(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
        V3_نمبر نتیجہ{&lhs};
        واپس !نتیجہ.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ساکن خالی crossRealBound(ثابت AstConst* constp, Ast_گرہ_اظہاریہ* exprp, منطقی بالائی,
                               ثابت Cross_قدر_Range& domain, V3_نمبر& نتیجہ, منطقی& خالی_خانے) {
        ثابت دوہرا قدر = constp->عدد().مقصد_Double();
        ثابت دوہرا bound = بالائی ? std::floor(قدر) : std::ceil(قدر);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ثابت دوہرا حد_بندی
            = std::ldexp(1.0, exprp->isSigned() ? exprp->چوڑائی() - 1 : exprp->چوڑائی());
        ثابت دوہرا minimum = exprp->isSigned() ? -حد_بندی : 0.0;
        اگر (std::isnan(bound) || (بالائی ? bound < minimum : bound >= حد_بندی)) {
            خالی_خانے = سچ;
            نتیجہ = domain.lo;
        } ورنہ اگر (bound < minimum) {
            نتیجہ = domain.lo;
        } ورنہ اگر (bound >= حد_بندی) {
            نتیجہ = domain.hi;
        } ورنہ {
            V3_نمبر real{&نتیجہ, 64};
            real.سیٹ_Double(bound);
            نتیجہ.opRToIRoundS(real);
        }
    }

    ساکن منطقی crossRangeBound(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, منطقی بالائی, منطقی bin_قدر,
                                ثابت Cross_قدر_Range& domain, V3_نمبر& نتیجہ, منطقی& خالی_خانے) {
        اگر (VN_IS(nodep, Unbounded)) {
            V3_نمبر حد_بندی{nodep, exprp->چوڑائی()};
            اگر (بالائی) حد_بندی.سیٹ_All_Bits1();
            اگر (exprp->isSigned()) {
                حد_بندی.سیٹ_ثنائی_ہندسہ(exprp->چوڑائی() - 1, !بالائی);
                نتیجہ.opExtendS(حد_بندی, حد_بندی.چوڑائی());
            } ورنہ {
                نتیجہ.op_تفویض(حد_بندی);
            }
            واپس سچ;
        }
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (!constp) واپس جھوٹ;
        اگر (constp->عدد().isDouble()) {
            crossRealBound(constp, exprp, بالائی, domain, نتیجہ, خالی_خانے);
            واپس سچ;
        }
        اگر (constp->عدد().isString()) واپس جھوٹ;
        اگر (bin_قدر && exprp->isSigned() && constp->چوڑائی() <= exprp->چوڑائی()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_نمبر قدر{nodep, exprp->چوڑائی()};
            اگر (constp->isSigned()) {
                قدر.opExtendS(constp->عدد(), constp->چوڑائی());
            } ورنہ {
                قدر.op_تفویض(constp->عدد());
            }
            نتیجہ.opExtendS(قدر, قدر.چوڑائی());
        } ورنہ اگر (constp->isSigned()) {
            نتیجہ.opExtendS(constp->عدد(), constp->چوڑائی());
        } ورنہ {
            نتیجہ.op_تفویض(constp->عدد());
        }
        واپس سچ;
    }

    ساکن Cross_قدر_Range cross_قدر_Domain(Ast_گرہ* nodep, صحیح قدر_چوڑائی, منطقی isSigned,
                                            صحیح چوڑائی) {
        Cross_قدر_Range domain{nodep, چوڑائی};
        V3_نمبر lo{nodep, قدر_چوڑائی};
        V3_نمبر hi{nodep, قدر_چوڑائی};
        hi.سیٹ_All_Bits1();
        اگر (isSigned) {
            lo.سیٹ_ثنائی_ہندسہ(قدر_چوڑائی - 1, 1);
            hi.سیٹ_ثنائی_ہندسہ(قدر_چوڑائی - 1, 0);
            domain.lo.opExtendS(lo, قدر_چوڑائی);
            domain.hi.opExtendS(hi, قدر_چوڑائی);
        } ورنہ {
            domain.lo.op_تفویض(lo);
            domain.hi.op_تفویض(hi);
        }
        واپس domain;
    }

    ساکن خالی intersectCrossRange(Cross_قدر_Range& range, ثابت Cross_قدر_Range& other) {
        اگر (cross_قدر_Less(range.lo, other.lo)) range.lo = other.lo;
        اگر (cross_قدر_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ساکن منطقی cross_قدر_Range(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, منطقی bin_قدر, منطقی wildcard,
                                ثابت Cross_قدر_Range& domain, Cross_قدر_Range& range) {
        منطقی خالی_خانے = جھوٹ;
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            اگر (!crossRangeBound(rangep->lhsp(), exprp, جھوٹ, bin_قدر, domain, range.lo, خالی_خانے)
                || !crossRangeBound(rangep->rhsp(), exprp, سچ, bin_قدر, domain, range.hi, خالی_خانے)
                || range.lo.isFourState() || range.hi.isFourState()) {
                واپس جھوٹ;
            }
        } ورنہ اگر (constp && constp->عدد().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, جھوٹ, domain, range.lo, خالی_خانے);
            crossRealBound(constp, exprp, سچ, domain, range.hi, خالی_خانے);
        } ورنہ {
            اگر (!crossRangeBound(nodep, exprp, جھوٹ, bin_قدر, domain, range.lo, خالی_خانے)) {
                واپس جھوٹ;
            }
            اگر (bin_قدر && exprp->isSigned() && constp && !constp->isSigned()
                && constp->چوڑائی() > exprp->چوڑائی()) {
                منطقی representable = سچ;
                برائے (صحیح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < constp->چوڑائی(); ++ثنائی_ہندسہ_2) {
                    representable &= !constp->عدد().ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2);
                }
                اگر (representable) {
                    V3_نمبر قدر{nodep, exprp->چوڑائی()};
                    قدر.op_تفویض(constp->عدد());
                    range.lo.opExtendS(قدر, قدر.چوڑائی());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            اگر (wildcard) {
                range.نمونہ_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                اگر (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_قدر_Domain(nodep, constp->چوڑائی(), سچ, domain.lo.چوڑائی()));
                }
            }
        }
        اگر (خالی_خانے) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            واپس سچ;
        }
        اگر (!range.lo.isFourState()) intersectCrossRange(range, domain);
        اگر (range.wildcard && exprp->isSigned()) {
            ثابت صحیح sign = exprp->چوڑائی() - 1;
            اگر (constp && !constp->isSigned() && constp->چوڑائی() > exprp->چوڑائی()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                برائے (صحیح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < constp->چوڑائی(); ++ثنائی_ہندسہ_2) {
                    اگر (constp->عدد().ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        واپس سچ;
                    }
                }
                برائے (صحیح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < range.نمونہ_2.چوڑائی(); ++ثنائی_ہندسہ_2) {
                    اگر (range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(sign))
                        range.نمونہ_2.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
                    ورنہ
                        range.نمونہ_2.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, range.نمونہ_2.ثنائی_ہندسہ_Is1(sign));
                }
                واپس سچ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            برائے (صحیح ثنائی_ہندسہ_2 = exprp->چوڑائی(); ثنائی_ہندسہ_2 < range.نمونہ_2.چوڑائی(); ++ثنائی_ہندسہ_2) {
                اگر (range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) جاری;
                ثابت منطقی قدر = range.نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2);
                اگر (!range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(sign) && range.نمونہ_2.ثنائی_ہندسہ_Is1(sign) != قدر) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    توڑو;
                }
                range.نمونہ_2.سیٹ_ثنائی_ہندسہ(sign, قدر);
            }
        }
        واپس سچ;
    }

    شمار CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_زیریں = 1,
        CROSS_AT_بالائی = 2,
        CROSS_AT_BOUNDS = CROSS_AT_زیریں | CROSS_AT_بالائی,
        CROSS_NO_مطابقت = 4  // Prefix cannot match the interval/pattern
    };

    ساکن CrossRangeState cross_Range_قدم(ثابت Cross_قدر_Range& range, CrossRangeState state,
                                          صحیح ثنائی_ہندسہ_2, صحیح قدر) {
        اگر (state == CROSS_NO_مطابقت) واپس CROSS_NO_مطابقت;
        // Flipping the sign bit makes signed order lexicographic.
        ثابت منطقی sign = ثنائی_ہندسہ_2 == range.نمونہ_2.چوڑائی() - 1;
        اگر (!range.نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && قدر != (range.نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) ^ sign)) {
            واپس CROSS_NO_مطابقت;
        }
        ثابت صحیح کم = range.lo.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) ^ sign;
        ثابت صحیح بلند = range.hi.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) ^ sign;
        اگر (((state & CROSS_AT_زیریں) && قدر < کم)
            || ((state & CROSS_AT_بالائی) && قدر > بلند)) {
            واپس CROSS_NO_مطابقت;
        }
        واپس static_cast<CrossRangeState>(
            ((state & CROSS_AT_زیریں) && قدر == کم ? CROSS_AT_زیریں : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_بالائی) && قدر == بلند ? CROSS_AT_بالائی : CROSS_INSIDE_BOUNDS));
    }

    ساکن منطقی crossWildcardIntersects(ثابت Cross_قدر_Range& range) {
        غیرعلامتی حالتیں = 1U << CROSS_AT_BOUNDS;
        برائے (صحیح ثنائی_ہندسہ_2 = range.نمونہ_2.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0 && حالتیں; --ثنائی_ہندسہ_2) {
            غیرعلامتی اگلا = 0;
            برائے (ثابت CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_زیریں, CROSS_AT_بالائی, CROSS_AT_BOUNDS}) {
                اگر (!(حالتیں & (1U << state))) جاری;
                برائے (صحیح قدر = 0; قدر < 2; ++قدر) {
                    ثابت CrossRangeState equal = cross_Range_قدم(range, state, ثنائی_ہندسہ_2, قدر);
                    اگر (equal != CROSS_NO_مطابقت) اگلا |= 1U << equal;
                }
            }
            حالتیں = اگلا;
        }
        واپس حالتیں != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ساکن منطقی اگلا_نمونہ_قدر(ثابت V3_نمبر& نمونہ_2, ثابت V3_نمبر& from, V3_نمبر& نتیجہ) {
        نتیجہ.op_تفویض(from);
        صحیح حاصل = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        برائے (صحیح ثنائی_ہندسہ_2 = from.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; --ثنائی_ہندسہ_2) {
            اگر (نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
                اگر (!from.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) حاصل = ثنائی_ہندسہ_2;
                جاری;
            }
            اگر (نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) == from.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) جاری;
            اگر (from.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {  // Only a greater prefix, an x bit above raised, can match
                اگر (حاصل < 0) واپس جھوٹ;
                ثنائی_ہندسہ_2 = حاصل;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            نتیجہ.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
            جب (--ثنائی_ہندسہ_2 >= 0) نتیجہ.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, نمونہ_2.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2));
            واپس سچ;
        }
        واپس سچ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ساکن خالی crossRangeRuns(ثابت Cross_قدر_Range& range, سائز_t حد_بندی,
                               std::سمتیہ<std::pair<V3_نمبر, V3_نمبر>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ثابت صحیح sign = range.lo.چوڑائی() - 1;
        ثابت خودکار flipped = [sign](V3_نمبر قدر) {
            اگر (!قدر.ثنائی_ہندسہ_Is_XZ(sign)) قدر.سیٹ_ثنائی_ہندسہ(sign, !قدر.ثنائی_ہندسہ_Is1(sign));
            واپس قدر;
        };
        ثابت V3_نمبر نمونہ_2 = flipped(range.نمونہ_2);
        ثابت V3_نمبر hi = flipped(range.hi);
        ثابت V3_نمبر one{&hi, hi.چوڑائی(), 1};
        V3_نمبر trailing{&hi, hi.چوڑائی()};
        برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 <= sign && نمونہ_2.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2); ++ثنائی_ہندسہ_2) trailing.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        V3_نمبر from = flipped(range.lo);
        V3_نمبر پہلا{&hi, hi.چوڑائی()};
        V3_نمبر آخری{&hi, hi.چوڑائی()};
        V3_نمبر less{&hi};
        جب (runs.سائز() <= حد_بندی && اگلا_نمونہ_قدر(نمونہ_2, from, پہلا)
               && less.opLt(hi, پہلا).isEqZero()) {
            آخری.opOr(پہلا, trailing);
            اگر (!less.opLt(hi, آخری).isEqZero()) آخری = hi;
            runs.emplace_عقب(flipped(پہلا), flipped(آخری));
            اگر (آخری.isCaseEq(hi)) توڑو;
            from.op_جوڑو(آخری, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ساکن منطقی cross_Range_خالی_خانے(ثابت Cross_قدر_Range& range) {
        اگر (range.lo.isFourState()) واپس سچ;
        واپس cross_قدر_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ساکن صحیح resolve_چوڑائی(Ast_گرہ* nodep, ثابت Ast_گرہ_اظہاریہ* exprp) {
        واپس std::max(exprp->چوڑائی(), cross_Range_چوڑائی(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ساکن منطقی resolve_قدر(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* exprp, منطقی bin_قدر, منطقی wildcard,
                             Cross_قدر_Range& range) {
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(nodep, exprp->چوڑائی(), exprp->isSigned(), range.lo.چوڑائی());
        واپس cross_قدر_Range(nodep, exprp, bin_قدر, wildcard, domain, range);
    }

    ساکن منطقی crossRangesIntersect(ثابت Cross_قدر_Range& bin, ثابت Cross_قدر_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        اگر (bin.lo.isFourState() || filter.lo.isFourState()) واپس جھوٹ;
        Cross_قدر_Range مطابقت = bin;
        intersectCrossRange(مطابقت, filter);
        واپس !cross_قدر_Less(مطابقت.hi, مطابقت.lo)
               && (!bin.wildcard || crossWildcardIntersects(مطابقت));
    }

    ساکن خالی unsupportedCrossRange(AstCoverBinsof* selectp, منطقی& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = جھوٹ;
    }

    ساکن منطقی cross_قدر_Matches_Filters(AstCoverBinsof* selectp, Ast_گرہ* valuep,
                                         Ast_گرہ_اظہاریہ* exprp, ثابت AstCoverBin* binp,
                                         ثابت Cross_قدر_Range& domain,
                                         ثابت std::سمتیہ<Cross_قدر_Range>& filters,
                                         منطقی& valid) {
        Cross_قدر_Range range{valuep, domain.lo.چوڑائی()};
        اگر (!cross_قدر_Range(valuep, exprp, سچ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            واپس جھوٹ;
        }
        برائے (ثابت Cross_قدر_Range& filter : filters) {
            اگر (crossRangesIntersect(range, filter)) واپس سچ;
        }
        واپس جھوٹ;
    }

    std::سمتیہ<منطقی> selectCoverpointBins(AstCoverBinsof* selectp, ثابت CoverpointBins& bins,
                                           uint32_t پہلا, uint32_t گنتی, منطقی& valid) {
        اگر (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            واپس {};
        }
        std::سمتیہ<منطقی> selected(bins.کل, جھوٹ);
        std::سمتیہ<std::سمتیہ<Ast_گرہ*>> اقدار;
        صحیح چوڑائی = bins.exprp->چوڑائی();
        برائے (Ast_گرہ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            چوڑائی = std::max(چوڑائی, cross_Range_چوڑائی(rangep));
        }
        اگر (selectp->rangesp()) {
            اقدار.reserve(گنتی);
            برائے (uint32_t i = پہلا; i < پہلا + گنتی; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                اقدار.ڈالو_عقب(bins.اقدار[i].runp ? std::سمتیہ<Ast_گرہ*>{}
                                                     : cross_Bin_اقدار(bins.اقدار[i]));
                برائے (Ast_گرہ* ثابت valuep : اقدار.عقب()) {
                    چوڑائی = std::max(چوڑائی, cross_Range_چوڑائی(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++چوڑائی;
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(selectp, bins.exprp->چوڑائی(), bins.exprp->isSigned(), چوڑائی);
        std::سمتیہ<Cross_قدر_Range> filters;
        برائے (Ast_گرہ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_قدر_Range filter{rangep, چوڑائی};
            اگر (!cross_قدر_Range(rangep, bins.exprp, جھوٹ, جھوٹ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                واپس {};
            }
            filters.ڈالو_عقب(std::move(filter));
        }
        برائے (uint32_t i = پہلا; i < پہلا + گنتی; ++i) {
            اگر (!selectp->rangesp()) {
                selected[i] = سچ;
                جاری;
            }
            اگر (ثابت Bin_تسلسل* ثابت runp = bins.اقدار[i].runp) {
                ثابت Cross_قدر_Range range
                    = تسلسل_Bin_Range(selectp, *runp, bins.اقدار[i].element, چوڑائی);
                selected[i] = std::any_کا(filters.begin(), filters.انتہا(),
                                          [&](ثابت Cross_قدر_Range& filter) {
                                              واپس crossRangesIntersect(range, filter);
                                          });
                جاری;
            }
            برائے (Ast_گرہ* ثابت valuep : اقدار[i - پہلا]) {
                selected[i] = cross_قدر_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.اقدار[i].binp, domain, filters, valid);
                اگر (!valid) واپس {};
                اگر (selected[i]) توڑو;
            }
        }
        اگر (selectp->isNegated()) {
            برائے (uint32_t i = 0; i < bins.کل; ++i) selected[i] = !selected[i];
        }
        واپس selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    ساخت قدر_Lists final {
        std::سمتیہ<std::string> m_ranges;  // Bin, then low and high words
        std::سمتیہ<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::سمتیہ<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::سمتیہ<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ساکن خالی append_الفاظ(std::string& متن, ثابت V3_نمبر& قدر, ثابت Ast_گرہ_اظہاریہ* exprp) {
        V3_نمبر narrowed{&قدر, exprp->چوڑائی()};
        narrowed.op_تفویض(قدر);
        برائے (صحیح لفظ = 0; لفظ < exprp->چوڑائی_الفاظ(); ++لفظ) {
            متن += ", " + cvt_مقصد_Str(narrowed.edata_لفظ(لفظ)) + "U";
        }
    }

    خالی collect_قدر_Metadata(قدر_Lists& lists, Ast_گرہ_اظہاریہ* exprp, AstCoverBin* binp,
                              uint32_t اشاریہ, Ast_گرہ_اظہاریہ* valuep) {
        ثابت std::string bin = cvt_مقصد_Str(اشاریہ) + "U";
        اگر (binp->transp()) lists.m_transitions.ڈالو_عقب(bin);
        برائے (Ast_گرہ* ثابت sourcep : cross_Bin_اقدار({binp, valuep})) {
            Cross_قدر_Range range{sourcep, resolve_چوڑائی(sourcep, exprp)};
            اگر (!resolve_قدر(sourcep, exprp, سچ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                جاری;
            }
            اگر (cross_Range_خالی_خانے(range)) جاری;
            std::string اندراج_2 = bin;
            اگر (range.wildcard) {
                V3_نمبر قدر{sourcep, exprp->چوڑائی()};
                V3_نمبر نقاب{sourcep, exprp->چوڑائی()};
                نقاب.op_بٹ_Non_XZ(range.نمونہ_2);
                قدر.op_بٹ_One(range.نمونہ_2);
                append_الفاظ(اندراج_2, قدر, exprp);
                append_الفاظ(اندراج_2, نقاب, exprp);
            }
            append_الفاظ(اندراج_2, range.lo, exprp);
            append_الفاظ(اندراج_2, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ڈالو_عقب(اندراج_2);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ساکن خالی collect_تسلسل_Metadata(قدر_Lists& lists, Ast_گرہ_اظہاریہ* exprp, ثابت Bin_تسلسل& تسلسل) {
        V3_نمبر وسعت{exprp, تسلسل.m_stride.چوڑائی(), 0};
        وسعت.opSub(تسلسل.m_stride, V3_نمبر{exprp, تسلسل.m_stride.چوڑائی(), 1});
        std::string اندراج_2 = cvt_مقصد_Str(تسلسل.m_declared) + "U, " + cvt_مقصد_Str(تسلسل.m_گنتی) + "U";
        append_الفاظ(اندراج_2, تسلسل.m_lo, exprp);
        append_الفاظ(اندراج_2, وسعت, exprp);
        append_الفاظ(اندراج_2, تسلسل.m_hi, exprp);
        lists.m_runs.ڈالو_عقب(اندراج_2);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    خالی emit_قدر_List(File_سطر* fl, AstVar* cpVarp, VCMethod طریقہ,
                       ثابت std::سمتیہ<std::string>& entries) {
        برائے (سائز_t پہلا = 0; پہلا < entries.سائز(); پہلا += قدر_LIST_ENTRIES) {
            ثابت سائز_t انتہا = std::min(entries.سائز(), پہلا + قدر_LIST_ENTRIES);
            std::string متن = "{" + entries[پہلا];
            برائے (سائز_t i = پہلا + 1; i < انتہا; ++i) متن += ", " + entries[i];
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cpVarp, طریقہ, {ctext(fl, متن + "}")})->makeStmt());
        }
    }

    ساکن منطقی checkCrossRef(ثابت AstCoverCrossRef* refp, ثابت AstCoverCross* crossp) {
        اگر (refp->name() == crossp->name()) واپس سچ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        واپس جھوٹ;
    }

    ساکن منطقی checkCrossBinName(ثابت AstCoverCrossBin* binp, std::سیٹ<std::string>& نام) {
        اگر (نام.emplace(binp->name()).دوسرا) واپس سچ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        واپس جھوٹ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ساکن منطقی implicit_Auto_Bin_وسعت(ثابت CoverpointBins& bins, ثابت std::string& name,
                                    Bin_وسعت& وسعت) {
        ثابت std::string سابقہ = "auto_";
        اگر (!VString::startsWith(name, سابقہ)) واپس جھوٹ;
        ثابت std::string digits = name.substr(سابقہ.سائز());
        ثابت غیرعلامتی طویل اشاریہ = std::strtoul(digits.c_str(), خالی_اشاریہ, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        اگر (اشاریہ >= bins.implicitAuto.گنتی || digits != cvt_مقصد_Str(اشاریہ)) واپس جھوٹ;
        ثابت uint32_t انحراف = static_cast<uint32_t>(اشاریہ);
        وسعت = Bin_وسعت{bins.implicitAuto.پہلا + انحراف, 1, bins.implicitAuto.declared + انحراف};
        واپس سچ;
    }

    Cross_Binsof_ہدف
    resolve_Binsof_ہدف(ثابت AstCoverBinsof* selectp, ثابت AstCoverCross* crossp,
                        ثابت std::سمتیہ<AstVar*>& cpVars,
                        ثابت std::map<std::string, uint32_t>& dimensions) ثابت {
        ثابت خودکار dim = dimensions.ڈھونڈو(selectp->pointp()->name());
        اگر (dim == dimensions.انتہا()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            واپس {};
        }
        ثابت CoverpointBins& bins = m_cpBins.at(cpVars[dim->دوسرا]);
        Cross_Binsof_ہدف ہدف{&bins, dim->دوسرا, 0, bins.کل};
        اگر (!selectp->name().خالی_خانے()) {
            ثابت خودکار bin = bins.spans.ڈھونڈو(selectp->name());
            Bin_وسعت وسعت{0, 0, 0};
            اگر (bin != bins.spans.انتہا()) {
                وسعت = bin->دوسرا;
            } ورنہ اگر (!implicit_Auto_Bin_وسعت(bins, selectp->name(), وسعت)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                واپس {};
            }
            ہدف.m_پہلا = وسعت.پہلا;
            ہدف.m_گنتی = وسعت.گنتی;
            ہدف.m_declared_پہلا = وسعت.declared;
            ہدف.m_declared_انتہا = وسعت.declared + وسعت.گنتی;
            ہدف.m_sized = وسعت.sized;
        }
        واپس ہدف;
    }

    // The variables a 'with' filter reads, each by its first reference
    استعمال CrossFilterReads = std::map<ثابت AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ساکن منطقی counts_قدر_Tuples(ثابت AstCoverCrossWith* withp) {
        واپس withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ساکن منطقی crossFilterSupported(ثابت AstCoverCrossWith* withp,
                                     ثابت CrossFilterReads& reads) {
        برائے (ثابت AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            اگر (varp->dtypep()->skipRefp()->isIntegralOrPacked()) جاری;
            ثابت خودکار it = reads.ڈھونڈو(varp);
            اگر (it != reads.انتہا()) {
                it->دوسرا->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->دوسرا->prettyNameQ());
                واپس جھوٹ;
            }
            اگر (counts_قدر_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                واپس جھوٹ;
            }
        }
        واپس سچ;
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
    خالی generateCrossFilter(ثابت AstCoverCrossWith* withp, ثابت CrossFilterReads& reads,
                             ثابت AstCoverCrossBin* binp, ثابت AstCoverCross* crossp,
                             AstVar* cxp, ثابت std::map<string, uint32_t>& dimensions,
                             uint32_t اشیاء) {
        File_سطر* ثابت fl = withp->fileline();
        ثابت string سابقہ
            = m_sized_نام.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ثابت منطقی counting = counts_قدر_Tuples(withp);
        // The count, 0 for all
        ثابت std::pair<Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*> گنتی
            = counting ? new_گنتی_دلائل(fl, سابقہ + "_count", withp->countp())
                       : new_گنتی_دلائل(fl, withp->countp() ? 0 : 1);
        AstLoop* ثابت loopp = نیا AstLoop{fl};
        AstCMethodHard* ثابت nextp = شے_Call(fl, cxp, VCMethod::COVERGROUP_WITH_اگلا);
        nextp->dtype_سیٹ_ثنائی_ہندسہ();
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{fl, loopp, nextp});
        string dims;
        برائے (uint32_t dim = 0; counting && dim < اشیاء; ++dim) {
            dims += (dims.خالی_خانے() ? "" : ", ") + cvt_مقصد_Str(dim) + "U";
        }
        جب (AstVar* ثابت varp = withp->varsp()) {
            varp->unlink_Fr_عقب();
            اگر (!reads.گنتی(varp)) {
                VL_DO_DANGLING(ڈالو_Deletep(varp), varp);
                جاری;
            }
            ثابت خودکار dimIt = dimensions.ڈھونڈو(varp->name());
            UASSERT_OBJ(dimIt != dimensions.انتہا(), varp, "Cross 'with' candidate not of an item");
            ثابت uint32_t dim = dimIt->دوسرا;
            اگر (!counting) dims += (dims.خالی_خانے() ? "" : ", ") + cvt_مقصد_Str(dim) + "U";
            varp->name(سابقہ + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->جوڑو_Stmtsp(varp);
            اگر (varp->isWide()) {
                loopp->جوڑو_Stmtsp(شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_قدر_W,
                                          {cnum(fl, dim), نیا AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } ورنہ {
                AstCMethodHard* ثابت valuep
                    = شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_قدر, {cnum(fl, dim)});
                valuep->dtype_سیٹ_UInt64();
                Ast_گرہ_DType* ثابت dtypep = varp->ڈھونڈو_Logic_DType(
                    varp->چوڑائی(), varp->چوڑائی(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->جوڑو_Stmtsp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_قدر(valuep, dtypep)});
            }
        }
        loopp->جوڑو_Stmtsp(شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_عقب()})
                             ->makeStmt());
        ثابت منطقی prot = v3Global.opt.protectIds();
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), گنتی.پہلا, گنتی.دوسرا,
                      ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->جوڑو_Stmtsp(loopp);
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_انتہا)->makeStmt());
    }

    منطقی generateRuntimeSelection(Ast_گرہ* nodep, ثابت AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ثابت std::سمتیہ<AstVar*>& cpVars,
                                  ثابت std::map<string, uint32_t>& dimensions) {
        File_سطر* ثابت fl = nodep->fileline();
        اگر (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            اگر (!checkCrossRef(refp, crossp)) واپس جھوٹ;
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            واپس سچ;
        }
        اگر (ثابت AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            اگر (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                واپس جھوٹ;
            }
            m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            واپس سچ;
        }
        اگر (ثابت AstCoverCrossWith* ثابت withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            اگر ((counts_قدر_Tuples(withp)
                 && !check_گنتی(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                واپس جھوٹ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.سائز()));
            واپس سچ;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_ہدف ہدف = resolve_Binsof_ہدف(selectp, crossp, cpVars, dimensions);
        اگر (!ہدف.m_binsp) واپس جھوٹ;
        ثابت CoverpointBins& bins = *ہدف.m_binsp;
        اگر (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            منطقی valid = سچ;
            unsupportedCrossRange(selectp, valid);
            واپس جھوٹ;
        }
        Ast_گرہ_اظہاریہ* firstp;
        Ast_گرہ_اظہاریہ* endp;
        اگر (ہدف.m_sized < 0) {
            firstp = cnum(fl, ہدف.m_declared_پہلا);
            endp = cnum(fl, ہدف.m_declared_انتہا);
        } ورنہ {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ثابت cpVarp = cpVars[ہدف.m_dimension];
            ثابت uint32_t sized = static_cast<uint32_t>(ہدف.m_sized);
            firstp = شے_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_پہلا, {cnum(fl, sized)});
            endp = شے_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_انتہا, {cnum(fl, sized)});
            firstp->dtype_سیٹ_UInt32();
            endp->dtype_سیٹ_UInt32();
        }
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, ہدف.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != خالی_اشاریہ)})
                ->makeStmt());
        برائے (Ast_گرہ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_قدر_Range range{rangep, resolve_چوڑائی(rangep, bins.exprp)};
            اگر (!resolve_قدر(rangep, bins.exprp, جھوٹ, جھوٹ, range)) {
                منطقی valid = سچ;
                unsupportedCrossRange(selectp, valid);
                واپس جھوٹ;
            }
            اگر (cross_Range_خالی_خانے(range)) جاری;
            m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_قدر_Const(fl, range.lo, bins.exprp),
                                                new_قدر_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_انتہا)->makeStmt());
        واپس سچ;
    }

    std::سمتیہ<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ثابت std::سمتیہ<AstVar*>& cpVars,
                             ثابت std::map<string, uint32_t>& dimensions) {
        std::سمتیہ<AstCoverCrossBin*> bins;
        std::سیٹ<string> نام;
        برائے (Ast_گرہ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            اگر (!checkCrossBinName(binp, نام)) جاری;
            File_سطر* ثابت fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ثابت selectionp = نیا AstFunc{fl, "", خالی_اشاریہ, خالی_اشاریہ};
            منطقی supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            اگر (supported) m_constructorp->جوڑو_Stmtsp(selectionp->stmtsp()->unlink_Fr_عقب_With_اگلا());
            VL_DO_DANGLING(ڈالو_Deletep(selectionp), selectionp);
            اگر (!supported) جاری;
            ثابت منطقی protect = v3Global.opt.protectIds();
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_سیٹ_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->پہلا_عمود()),
                          cnum(fl, static_cast<uint32_t>(bins.سائز()))})
                    ->makeStmt());
            bins.ڈالو_عقب(binp);
        }
        m_constructorp->جوڑو_Stmtsp(
            شے_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        واپس bins;
    }

    ساکن خالی سیٹ_Cross_Selection_Range(CrossSelection& selection, uint64_t پہلا, uint64_t انتہا) {
        جب (پہلا < انتہا) {
            ثابت غیرعلامتی ثنائی_ہندسہ_2 = پہلا % 64;
            ثابت غیرعلامتی بٹ = std::min<uint64_t>(64 - ثنائی_ہندسہ_2, انتہا - پہلا);
            selection[VL_BITWORD_Q(پہلا)]
                |= (بٹ == 64 ? ~uint64_t{0} : (uint64_t{1} << بٹ) - 1) << ثنائی_ہندسہ_2;
            پہلا += بٹ;
        }
    }

    CrossSelection crossSelection(Ast_گرہ* nodep, CrossSelectionContext& ctx) {
        اگر (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            اگر (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = جھوٹ;
                واپس {};
            }
            CrossSelection نتیجہ(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            سیٹ_Cross_Selection_Range(نتیجہ, 0, ctx.tuples);
            واپس نتیجہ;
        }
        اگر (AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ثابت CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            اگر (!ctx.valid) واپس {};
            برائے (سائز_t i = 0; i < lhs.سائز(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            واپس lhs;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_ہدف ہدف
            = resolve_Binsof_ہدف(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        اگر (!ہدف.m_binsp) {
            ctx.valid = جھوٹ;
            واپس {};
        }
        ثابت CoverpointBins& bins = *ہدف.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(ہدف.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ثابت std::سمتیہ<منطقی> selected
            = selectCoverpointBins(selectp, bins, ہدف.m_پہلا, ہدف.m_گنتی, ctx.valid);
        اگر (!ctx.valid) واپس {};
        CrossSelection نتیجہ(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ثابت uint64_t stride = ctx.strides[ہدف.m_dimension];
        ثابت uint64_t period = stride * bins.کل;
        برائے (uint64_t بنیاد = 0; بنیاد < ctx.tuples; بنیاد += period) {
            برائے (uint32_t i = 0; i < bins.کل;) {
                اگر (!selected[i]) {
                    ++i;
                    جاری;
                }
                ثابت uint32_t begin = i++;
                جب (i < bins.کل && selected[i]) ++i;
                سیٹ_Cross_Selection_Range(نتیجہ, بنیاد + begin * stride, بنیاد + i * stride);
            }
        }
        واپس نتیجہ;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    منطقی crossShape(AstCoverCross* crossp, ثابت std::سمتیہ<AstVar*>& cpVars,
                    std::سمتیہ<uint32_t>& strides, uint32_t& tuples) ثابت {
        uint64_t حاصل_ضرب = std::any_کا(cpVars.begin(), cpVars.انتہا(),
                                       [یہ](AstVar* varp) { واپس !m_cpBins.at(varp).کل; })
                               ? 0
                               : 1;
        strides.resize(cpVars.سائز());
        برائے (سائز_t d = cpVars.سائز(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(حاصل_ضرب);
            حاصل_ضرب *= m_cpBins.at(cpVars[d - 1]).کل;
            اگر (حاصل_ضرب > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                واپس جھوٹ;
            }
        }
        tuples = static_cast<uint32_t>(حاصل_ضرب);
        واپس سچ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ثابت std::سمتیہ<AstVar*>& cpVars,
                                   ثابت std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        اگر (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = جھوٹ;
            واپس layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::سیٹ<std::string> نام;
        برائے (Ast_گرہ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            اگر (!checkCrossBinName(binp, نام)) جاری;
            ctx.valid = سچ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            اگر (!ctx.valid || std::all_کا(selection.begin(), selection.انتہا(), [](uint64_t لفظ) {
                    واپس لفظ == 0;
                })) {
                جاری;
            }
            اگر (occupied.خالی_خانے()) {
                occupied.resize(selection.سائز(), 0);
                excluded.resize(selection.سائز(), 0);
            }
            برائے (سائز_t i = 0; i < selection.سائز(); ++i) {
                occupied[i] |= selection[i];
                اگر (!binp->binsType().bin_Is_معمول()) excluded[i] |= selection[i];
            }
            layout.bins.ڈالو_عقب({binp, std::move(selection)});
        }
        اگر (!layout.bins.خالی_خانے()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            برائے (ResolvedCrossBin& resolved : layout.bins) {
                برائے (سائز_t i = 0; i < resolved.selection.سائز(); ++i) {
                    اگر (resolved.binp->binsType().bin_Is_معمول()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    اگر (resolved.selection[i]) ++layout.bin_الفاظ;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.انتہا(),
                                             [](ثابت ResolvedCrossBin& resolved) {
                                                 واپس std::all_کا(
                                                     resolved.selection.begin(),
                                                     resolved.selection.انتہا(),
                                                     [](uint64_t لفظ) { واپس لفظ == 0; });
                                             }),
                              layout.bins.انتہا());
            layout.autoBins = layout.tuples;
            برائے (ثابت uint64_t لفظ : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{لفظ}.گنتی());
            }
        }
        واپس layout;
    }

    AstCoverCrossDType* crossDType(File_سطر* fl, uint32_t dimensions, ثابت CrossLayout& layout,
                                   منطقی dynamic = جھوٹ) {
        ثابت uint32_t bins = static_cast<uint32_t>(layout.bins.سائز());
        ثابت CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_الفاظ, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        اگر (!typep) {
            typep = نیا AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_الفاظ, dynamic};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(typep);
        }
        واپس typep;
    }

    std::سمتیہ<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ثابت CrossLayout& layout) {
        std::سمتیہ<AstCoverCrossBin*> bins;
        برائے (ثابت ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ثابت binp = resolved.binp;
            ثابت CrossSelection& selection = resolved.selection;
            File_سطر* ثابت fl = binp->fileline();
            ثابت منطقی prot = v3Global.opt.protectIds();
            std::string نقاب = "{";
            برائے (سائز_t i = 0; i < selection.سائز(); ++i) {
                اگر (i) نقاب += ", ";
                نقاب += std::مقصد_string(selection[i]) + "ULL";
            }
            نقاب += "}";
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(fl, cxVarp, VCMethod::COVERGROUP_جوڑو_BIN,
                         {ctext(fl, binp->binsType().bin_سیٹ_Enum()), ctext(fl, نقاب),
                          ctext(fl, quoted(VIdProtect::protect_الفاظ_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->پہلا_عمود()))})
                    ->makeStmt());
            bins.ڈالو_عقب(binp);
        }
        اگر (!bins.خالی_خانے()) {
            m_constructorp->جوڑو_Stmtsp(
                شے_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        واپس bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    خالی generateCross(AstCoverCross* crossp) {
        File_سطر* ثابت fl = crossp->fileline();
        ثابت منطقی dynamic = m_runtimeCrosses.گنتی(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        اگر (Ast_گرہ_اظہاریہ* ثابت iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_مقصد_عارضی(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::سمتیہ<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        برائے (Ast_گرہ* itemp = crossp->itemsp(); itemp;) {
            Ast_گرہ* ثابت nextp = itemp->nextp();
            AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            ثابت خودکار it = m_cpVarMap.ڈھونڈو(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.انتہا(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.سائز()));
            cpVars.ڈالو_عقب(it->دوسرا);
            VL_DO_DANGLING(ڈالو_Deletep(refp->unlink_Fr_عقب()), refp);
            itemp = nextp;
        }
        ثابت صحیح dims = static_cast<صحیح>(cpVars.سائز());
        CrossLayout layout;
        اگر (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::سمتیہ<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } ورنہ {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        اگر (!layout.valid) واپس;

        AstVar* ثابت cxVarp
            = نیا AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->جوڑو_Membersp(cxVarp);
        m_constructorp->جوڑو_Stmtsp(make_شے_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_جوڑو_CROSS_DYN
                                                         : VCMethod::COVERGROUP_جوڑو_CROSS));
        generate_شے_وزن(fl, cxVarp, crossp->optionsp());
        صحیح at_کم_سے_کم_قدر;
        صحیح autoBinMax;  // Not a cross option
        نکالو_شے_Options(crossp->optionsp(), at_کم_سے_کم_قدر, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ثابت منطقی prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_الفاظ_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->جوڑو_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            شے_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_کم_سے_کم_قدر)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->پہلا_عمود()))})));
        ثابت std::سمتیہ<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        اگر (v3Global.opt.coverage()) {
            ثابت std::string صفحہ
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جوڑو_Stmtsp(شے_Call(fl, cxVarp, VCMethod::COVERGROUP_رجسٹر_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(صفحہ)),
                                                cnum(fl, شے_Database_وزن(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_وزن)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ثابت منطقی hasIffs
            = std::any_کا(bins.begin(), bins.انتہا(),
                          [](ثابت AstCoverCrossBin* binp) { واپس binp->iffp() != خالی_اشاریہ; });
        Ast_گرہ_Stmt* ثابت samplep
            = !hasIffs ? static_cast<Ast_گرہ_Stmt*>(
                             شے_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_گرہ_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             شے_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        اگر (Ast_گرہ_اظہاریہ* ثابت iffp = crossp->iffp()) {
            m_sampleFuncp->جوڑو_Stmtsp(نیا AstIf{fl, iffp->clone_درخت(جھوٹ), samplep});
        } ورنہ {
            m_sampleFuncp->جوڑو_Stmtsp(samplep);
        }
    }

    خالی generate_Cross_رمز(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        برائے (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            اگر (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                واپس;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        اگر (m_droppedCrosses.گنتی(crossp)) {
            برائے (Ast_گرہ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                اگر (m_coverpointMap.ڈھونڈو(refp->name()) == m_coverpointMap.انتہا()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    توڑو;
                }
            }
            واپس;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_گرہ_اظہاریہ* buildBinCondition(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        // Get the range list from the bin
        Ast_گرہ* ثابت rangep = binp->rangesp();
        اگر (!rangep) واپس خالی_اشاریہ;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ثابت منطقی integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        اگر (binp->isWildcard() && !integral) واپس wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_گرہ_اظہاریہ* بھرا_Condp = خالی_اشاریہ;

        برائے (Ast_گرہ* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_گرہ_اظہاریہ* rangeCondp = خالی_اشاریہ;
            currRangep = V3Const::constifyEdit(currRangep);

            اگر (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_گرہ_اظہاریہ* ثابت minExprp = irp->lhsp();
                Ast_گرہ_اظہاریہ* ثابت maxExprp = irp->rhsp();
                AstConst* ثابت minConstp = VN_CAST(minExprp, Const);
                AstConst* ثابت maxConstp = VN_CAST(maxExprp, Const);
                ثابت منطقی loUnbounded = VN_IS(minExprp, Unbounded);
                ثابت منطقی hiUnbounded = VN_IS(maxExprp, Unbounded);
                اگر (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ثابت boundp = hiUnbounded ? minConstp : maxConstp;
                    اگر (loUnbounded && hiUnbounded) {
                        rangeCondp = نیا AstConst{irp->fileline(), AstConst::ثنائی_ہندسہ_True{}};
                    } ورنہ اگر (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        اگر (بھرا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھرا_Condp), بھرا_Condp);
                        واپس خالی_اشاریہ;
                    } ورنہ اگر (boundp->عدد().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        اگر (بھرا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھرا_Condp), بھرا_Condp);
                        واپس خالی_اشاریہ;
                    } ورنہ اگر (integral) {
                        rangeCondp = build_قدر_Condition(binp, exprp, irp);
                    } ورنہ {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } ورنہ اگر (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    اگر (بھرا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھرا_Condp), بھرا_Condp);
                    واپس خالی_اشاریہ;
                } ورنہ اگر (minConstp->عدد().isFourState() || maxConstp->عدد().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    اگر (بھرا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھرا_Condp), بھرا_Condp);
                    واپس خالی_اشاریہ;
                } ورنہ اگر (integral) {
                    rangeCondp = build_قدر_Condition(binp, exprp, irp);
                } ورنہ {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } ورنہ اگر (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_قدر_Condition(binp, exprp, constp);
            } ورنہ {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                اگر (بھرا_Condp) VL_DO_DANGLING(ڈالو_Deletep(بھرا_Condp), بھرا_Condp);
                واپس خالی_اشاریہ;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            بھرا_Condp
                = بھرا_Condp ? نیا AstOr{binp->fileline(), بھرا_Condp, rangeCondp} : rangeCondp;
        }

        واپس بھرا_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ساکن Ast_گرہ_اظہاریہ* wildcardTypeError(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        واپس نیا AstConst{binp->fileline(), AstConst::ثنائی_ہندسہ_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_گرہ_اظہاریہ* build_قدر_Condition(AstCoverBin* binp, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ* valuep) {
        File_سطر* ثابت fl = valuep->fileline();
        اگر (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            واپس نیا AstEq{fl, exprp->clone_درخت(جھوٹ),
                             VN_AS(valuep, گرہ_اظہاریہ)->clone_درخت(جھوٹ)};
        }
        Cross_قدر_Range range{valuep, resolve_چوڑائی(valuep, exprp)};
        اگر (!resolve_قدر(valuep, exprp, سچ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            واپس نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}};
        }
        اگر (cross_Range_خالی_خانے(range)) واپس نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}};
        AstConst* ثابت lop = new_قدر_Const(fl, range.lo, exprp);
        AstConst* ثابت hip = new_قدر_Const(fl, range.hi, exprp);
        Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
        اگر (lop->عدد().isCaseEq(hip->عدد())) {
            condp = نیا AstEq{fl, exprp->clone_درخت(جھوٹ), lop};
        } ورنہ {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ڈالو_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ڈالو_Deletep(hip), hip);
        اگر (!range.wildcard) واپس condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_نمبر نقاب{valuep, exprp->چوڑائی()};
        V3_نمبر قدر{valuep, exprp->چوڑائی()};
        نقاب.op_بٹ_Non_XZ(range.نمونہ_2);
        قدر.op_بٹ_One(range.نمونہ_2);
        AstConst* ثابت نقاب_Constp = new_قدر_Const(fl, نقاب, exprp);
        AstConst* ثابت قدر_Constp = new_قدر_Const(fl, قدر, exprp);
        Ast_گرہ_اظہاریہ* ثابت اظہاریہ_Masked = نیا AstAnd{fl, exprp->clone_درخت(جھوٹ), نقاب_Constp};
        Ast_گرہ_اظہاریہ* ثابت قدر_Masked = نیا AstAnd{fl, قدر_Constp, نقاب_Constp->clone_درخت(جھوٹ)};
        واپس نیا AstLogAnd{fl, condp, نیا AstEq{fl, اظہاریہ_Masked, قدر_Masked}};
    }

    خالی generate_Coverage_Computation_رمز() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ثابت getInstCoveragep
            = VN_AS(m_memberMap.ڈھونڈو_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_سطر* ثابت instFl = getInstCoveragep->fileline();
        AstCMethodHard* ثابت instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_سیٹ_Double();
        Ast_گرہ_اظہاریہ* instValuep = instCallp;
        اگر (m_cg_May_ملاؤ) {
            Ast_گرہ_اظہاریہ* ثابت mergedp = نیا AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(سچ), "merge_instances", VAccess::READ),
                نیا AstLogNot{instFl, newOptionSel(instFl, optionVar(جھوٹ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = نیا AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->جوڑو_Stmtsp(نیا Ast_تفویض{
            instFl, نیا AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ثابت getCoveragep
            = VN_AS(m_memberMap.ڈھونڈو_Member(m_covergroupp, "get_coverage"), Func);
        File_سطر* ثابت typeFl = getCoveragep->fileline();
        getCoveragep->جوڑو_Stmtsp(نیا Ast_تفویض{
            typeFl, نیا AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_سطر* fl) {
        AstCExpr* ثابت registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_سیٹ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp
            = نیا AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->جوڑو_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->جوڑو_Pinsp(newOptionSel(fl, optionVar(سچ), "weight", VAccess::READ));
        callp->جوڑو_Pinsp(newOptionSel(fl, optionVar(سچ), "merge_instances", VAccess::READ));
        callp->جوڑو_Pinsp(file_سطر_Debug(m_covergroupp->fileline()));
        callp->usePtr(سچ);
        callp->dtype_سیٹ_Double();
        واپس callp;
    }

    // VISITORS
    ساکن منطقی isEnclosingInstanceVar(ثابت AstVar* varp) {
        واپس varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    خالی rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ثابت AstClassRefDType* ثابت refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_گرہ_اظہاریہ* ثابت newp = نیا AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    خالی rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_سطر* ثابت fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ثابت selp
            = نیا AstMemberSel{fl, نیا AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    خالی rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_سطر* ثابت fl = refp->fileline();
        AstArg* ثابت argsp = refp->argsp() ? refp->argsp()->unlink_Fr_عقب_With_اگلا() : خالی_اشاریہ;
        Ast_طریقہ_Call* ثابت callp = نیا Ast_طریقہ_Call{
            fl, نیا AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    منطقی isEnclosingInstanceFunc(ثابت Ast_گرہ_FTask* funcp) ثابت {
        اگر (!funcp->class_طریقہ() || funcp->isStatic()) واپس جھوٹ;
        واپس AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    منطقی isEmbeddedCovergroupVar(ثابت AstVar* varp) ثابت {
        اگر (!varp || !varp->isClassMember() || varp->isDeclTyped()) واپس جھوٹ;
        ثابت AstClassRefDType* ثابت refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        واپس refp && refp->classp() == m_covergroupp;
    }

    AstVar* ڈھونڈو_Embedded_Covergroup_Var() ثابت {
        اگر (!m_enclosingClassp) واپس خالی_اشاریہ;
        برائے (Ast_گرہ* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(itemp, Var)) {
                اگر (isEmbeddedCovergroupVar(varp)) واپس varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        واپس خالی_اشاریہ;  // LCOV_EXCL_LINE
    }

    std::سمتیہ<Ast_گرہ_تفویض*> ڈھونڈو_Covergroup_Constructions() {
        std::سمتیہ<Ast_گرہ_تفویض*> foundps;
        اگر (!m_embeddedVarp) واپس foundps;
        AstFunc* ثابت enclosingNewp
            = VN_CAST(m_memberMap.ڈھونڈو_Member(m_enclosingClassp, "new"), Func);
        اگر (!enclosingNewp) واپس foundps;
        enclosingNewp->foreach([&](Ast_گرہ_تفویض* asgnp) {
            ثابت AstNew* ثابت newp = VN_CAST(asgnp->rhsp(), New);
            ثابت AstVarRef* ثابت lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            اگر (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) واپس;
            ثابت AstClassRefDType* ثابت refp = VN_CAST(newp->dtypep(), ClassRefDType);
            اگر (refp && refp->classp() == m_covergroupp) foundps.ڈالو_عقب(asgnp);
        });
        واپس foundps;
    }

    std::سیٹ<ثابت AstVar*> enclosingInstanceVars() ثابت {
        std::سیٹ<ثابت AstVar*> vars;
        اگر (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {
                اگر (isEnclosingInstanceVar(varp)) vars.داخل_کرو(varp);
            });
        }
        واپس vars;
    }

    منطقی hasEnclosingEventRef(AstCovergroup* cgp) ثابت {
        اگر (!m_embeddedVarp || !cgp->eventp()) واپس جھوٹ;
        ثابت std::سیٹ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        منطقی ملا = جھوٹ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            اگر (enclosingVars.گنتی(refp->varp())) ملا = سچ;
        });
        واپس ملا;
    }

    ساکن منطقی parse_Embedded_Event_اظہاریہ(Ast_گرہ_اظہاریہ* exprp, AstVar*& بنیاد_Varp,
                                       AstVar*& memberVarp) {
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            بنیاد_Varp = refp->varp();
            memberVarp = خالی_اشاریہ;
            واپس سچ;
        }
        AstMemberSel* ثابت selp = VN_CAST(exprp, MemberSel);
        اگر (!selp) واپس جھوٹ;
        AstVarRef* ثابت بنیاد_Refp = VN_CAST(selp->fromp(), VarRef);
        اگر (!بنیاد_Refp) واپس جھوٹ;
        بنیاد_Varp = بنیاد_Refp->varp();
        memberVarp = selp->varp();
        واپس سچ;
    }

    منطقی isEventLvalue(Ast_گرہ_اظہاریہ* exprp, ثابت EmbeddedEventTrigger& trigger) ثابت {
        اگر (AstSel* ثابت selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* بنیاد_Varp = خالی_اشاریہ;
        AstVar* memberVarp = خالی_اشاریہ;
        اگر (!parse_Embedded_Event_اظہاریہ(exprp, بنیاد_Varp, memberVarp)) واپس جھوٹ;
        واپس بنیاد_Varp == trigger.بنیاد_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_گرہ_اظہاریہ* newEventRead(File_سطر* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_گرہ_اظہاریہ* ثابت basep = نیا AstVarRef{fl, trigger.بنیاد_Varp, VAccess::READ};
        اگر (!trigger.memberVarp) واپس basep;
        AstMemberSel* ثابت selp = نیا AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        واپس selp;
    }

    string event_پچھلا_Name(ثابت EmbeddedEventTrigger& trigger, سائز_t trigger_اشاریہ) ثابت {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::مقصد_string(trigger_اشاریہ)
                      + "_" + trigger.بنیاد_Varp->name();
        اگر (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        واپس name;
    }

    Ast_گرہ_اظہاریہ* newEmbeddedVarNonNull(File_سطر* fl) ثابت {
        واپس نیا AstNeq{fl, نیا AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          نیا AstConst{fl, AstConst::Null{}}};
    }

    Ast_گرہ_Stmt* newSampleStmt(File_سطر* fl) ثابت {
        Ast_طریقہ_Call* ثابت callp = نیا Ast_طریقہ_Call{
            fl, نیا AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", خالی_اشاریہ};
        callp->taskp(m_sampleFuncp);
        callp->dtype_سیٹ_Void();
        واپس callp->makeStmt();
    }

    خالی installEmbeddedEventFork(Ast_Sen_درخت* eventp,
                                  ثابت std::سمتیہ<Ast_گرہ_تفویض*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        برائے (Ast_گرہ_تفویض* ثابت constructp : constructps) {
            File_سطر* ثابت fl = constructp->fileline();
            AstLoop* ثابت loopp = نیا AstLoop{fl};
            loopp->جوڑو_Stmtsp(نیا AstEventControl{fl, eventp->clone_درخت(جھوٹ), خالی_اشاریہ});
            loopp->جوڑو_Stmtsp(نیا AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ثابت forkp = نیا AstFork{fl, VJoinType::JOIN_NONE};
            forkp->فوری_آغاز(سچ);
            forkp->جوڑو_Forksp(نیا AstBegin{fl, "", loopp, سچ});
            constructp->جوڑو_اگلا_یہاں(forkp);
        }
        VL_DO_DANGLING(ڈالو_Deletep(eventp), eventp);
    }

    Ast_گرہ_اظہاریہ* newEventReadyCondition(File_سطر* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_گرہ_اظہاریہ* ثابت curp = newEventRead(fl, trigger);
        Ast_گرہ_اظہاریہ* ثابت prevp = نیا AstVarRef{fl, trigger.پچھلا_Varp, VAccess::READ};
        Ast_گرہ_اظہاریہ* edgep = خالی_اشاریہ;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        اگر (trigger.کنارہ_Type == VEdgeType::ET_POSEDGE) {
            edgep = نیا AstSel{fl, نیا AstAnd{fl, curp, نیا AstNot{fl, prevp}}, 0, 1};
        } ورنہ اگر (trigger.کنارہ_Type == VEdgeType::ET_NEGEDGE) {
            edgep = نیا AstSel{fl, نیا AstAnd{fl, نیا AstNot{fl, curp}, prevp}, 0, 1};
        } ورنہ اگر (trigger.کنارہ_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = نیا AstSel{fl, نیا AstXor{fl, curp, prevp}, 0, 1};
        } ورنہ {
            edgep = نیا AstNeq{fl, curp, prevp};
        }
        واپس نیا AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::سمتیہ<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::سمتیہ<EmbeddedEventTrigger> triggers;
        ثابت std::سیٹ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        برائے (Ast_گرہ* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_شے* ثابت itemp = VN_AS(senp, Sen_شے);
            AstVar* بنیاد_Varp = خالی_اشاریہ;
            AstVar* memberVarp = خالی_اشاریہ;
            اگر (!parse_Embedded_Event_اظہاریہ(itemp->sensp(), بنیاد_Varp, memberVarp)
                || !enclosingVars.گنتی(بنیاد_Varp)) {
                واپس {};
            }
            triggers.emplace_عقب(itemp->fileline(), بنیاد_Varp, memberVarp, itemp->کنارہ_Type());
        }
        واپس triggers;
    }

    خالی installEmbeddedEventTriggers(std::سمتیہ<EmbeddedEventTrigger>& triggers,
                                      ثابت std::سمتیہ<Ast_گرہ_تفویض*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        اگر (constructps.خالی_خانے()) واپس;
        برائے (سائز_t trigger_اشاریہ = 0; trigger_اشاریہ < triggers.سائز(); ++trigger_اشاریہ) {
            EmbeddedEventTrigger& trigger = triggers[trigger_اشاریہ];
            std::سمتیہ<Ast_گرہ_تفویض*> assignps;
            m_enclosingClassp->foreach([&](Ast_گرہ_تفویض* asgnp) {
                اگر (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ڈالو_عقب(asgnp);
            });
            اگر (assignps.خالی_خانے()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                جاری;
            }
            Ast_گرہ_DType* ثابت dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.بنیاد_Varp->dtypep();
            AstVar* ثابت پچھلا_Varp = نیا AstVar{trigger.eventFl, VVarType::MEMBER,
                                                event_پچھلا_Name(trigger, trigger_اشاریہ), dtypep};
            m_enclosingClassp->جوڑو_Membersp(پچھلا_Varp);
            trigger.پچھلا_Varp = پچھلا_Varp;
            برائے (Ast_گرہ_تفویض* ثابت asgnp : assignps) {
                File_سطر* ثابت fl = asgnp->fileline();
                AstIf* ثابت ifp
                    = نیا AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->جوڑو_اگلا_یہاں(نیا Ast_تفویض{fl,
                                               نیا AstVarRef{fl, trigger.پچھلا_Varp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->جوڑو_اگلا_یہاں(ifp);
            }
        }
    }

    خالی delete_Coverage_اشیاء() {
        برائے (AstCoverpoint* ثابت cpp : m_coverpoints) {
            VL_DO_DANGLING(ڈالو_Deletep(cpp->unlink_Fr_عقب()), cpp);
        }
        برائے (AstCoverCross* ثابت crossp : m_coverCrosses) {
            VL_DO_DANGLING(ڈالو_Deletep(crossp->unlink_Fr_عقب()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        برائے (Ast_Cg_Option_تفویض* ثابت optp : m_cgOptions) {
            VL_DO_DANGLING(ڈالو_Deletep(optp->unlink_Fr_عقب()), optp);
        }
        m_cgOptions.clear();
    }

    جماعت FormalRefVisitor final : عوامی VNVisitor {
        ثابت std::map<ثابت AstVar*, AstVar*>& m_replacements;

        خالی دورہ(AstVarRef* nodep) override {
            ثابت خودکار it = m_replacements.ڈھونڈو(nodep->varp());
            اگر (it == m_replacements.انتہا()) واپس;
            nodep->varp(it->دوسرا);
        }
        خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    عوامی:
        صریح FormalRefVisitor(ثابت std::map<ثابت AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        خالی جائزہ(Ast_گرہ* nodep) { iterate(nodep); }
    };

    خالی validateCovergroupExpressions() {
        std::سیٹ<ثابت AstVar*> sampleMembers;
        std::سیٹ<ثابت AstVar*> constructorRefMembers;
        برائے (Ast_گرہ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            اگر (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) جاری;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.ڈھونڈو_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.داخل_کرو(varp);
            constructorRefMembers.داخل_کرو(memberp);
        }
        برائے (Ast_گرہ* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            اگر (!varp || !varp->isIO()) جاری;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.ڈھونڈو_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.داخل_کرو(memberp);
        }
        Covergroup_اظہاریہ_Valid_Visitor{sampleMembers, constructorRefMembers}.جائزہ(m_constructorp);
    }

    خالی rebindFormalRefs() {
        std::map<ثابت AstVar*, AstVar*> replacements;
        برائے (Ast_گرہ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isIO()) جاری;
                AstVar* ثابت memberp
                    = VN_CAST(m_memberMap.ڈھونڈو_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        سائز_t expectedBindings = 0;
        برائے (ثابت خودکار& pair : replacements) {
            اگر (pair.پہلا->isRef() || pair.پہلا->isConstRef()) ++expectedBindings;
        }
        سائز_t rewrittenBindings = 0;
        برائے (Ast_گرہ* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_گرہ* ثابت nextp = stmtp->nextp();
            اگر (Ast_تفویض* ثابت assignp = VN_CAST(stmtp, تفویض)) {
                AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assignp->rhsp(), VarRef);
                اگر (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ثابت خودکار it = replacements.ڈھونڈو(rhsp->varp());
                    UASSERT_OBJ(it != replacements.انتہا() && it->دوسرا == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ثابت bindp = نیا AstCExpr{assignp->fileline(), ""};
                    bindp->جوڑو(lhsp->unlink_Fr_عقب());
                    bindp->جوڑو(" = &");
                    bindp->جوڑو(rhsp->unlink_Fr_عقب());
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
        برائے (AstCoverpoint* ثابت cpp : m_coverpoints) visitor.جائزہ(cpp);
        برائے (AstCoverCross* ثابت crossp : m_coverCrosses) visitor.جائزہ(crossp);
    }

    AstVarRef* install_Enclosing_عقب_اشارہ_گر(ثابت std::سمتیہ<Ast_گرہ_تفویض*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        اگر (!m_enclosingClassp) واپس خالی_اشاریہ;  // Offending refs require an enclosing class

        AstVarRef* invalidp = خالی_اشاریہ;
        Ast_گرہ* offenderp = خالی_اشاریہ;
        std::سیٹ<ثابت AstVar*> ownVars;
        برائے (Ast_گرہ* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) ownVars.داخل_کرو(varp);
        }
        ثابت std::سیٹ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        std::سمتیہ<AstVarRef*> refs_مقصد_Rewrite;
        std::سمتیہ<AstThisRef*> this_Refs_مقصد_Rewrite;
        std::سمتیہ<AstFuncRef*> func_Refs_مقصد_Rewrite;
        ثابت خودکار جائزہ = [&](Ast_گرہ* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                اگر (invalidp) واپس;
                ثابت AstVar* ثابت varp = refp->varp();
                اگر (!isEnclosingInstanceVar(varp) || ownVars.گنتی(varp)) واپس;
                اگر (!enclosingVars.گنتی(varp)) {
                    invalidp = refp;
                    واپس;
                }
                refs_مقصد_Rewrite.ڈالو_عقب(refp);
                اگر (!offenderp) offenderp = refp;
            });
            اگر (invalidp) واپس;
            rootp->foreach([&](AstThisRef* refp) {
                ثابت AstClassRefDType* ثابت refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                اگر (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_مقصد_Rewrite.ڈالو_عقب(refp);
                    اگر (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                اگر (!isEnclosingInstanceFunc(refp->taskp())) واپس;
                func_Refs_مقصد_Rewrite.ڈالو_عقب(refp);
                اگر (!offenderp) offenderp = refp;
            });
        };
        برائے (AstCoverpoint* ثابت cpp : m_coverpoints) جائزہ(cpp);
        برائے (AstCoverCross* ثابت crossp : m_coverCrosses) جائزہ(crossp);
        برائے (Ast_Cg_Option_تفویض* ثابت optp : m_cgOptions) جائزہ(optp);
        اگر (invalidp || !offenderp) واپس invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_سطر* ثابت fl = m_covergroupp->fileline();
        AstClassRefDType* ثابت enclDTypep = نیا AstClassRefDType{fl, m_enclosingClassp, خالی_اشاریہ};
        enclDTypep->raw_اشارہ_گر(سچ);
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(enclDTypep);
        AstVar* ثابت handleVarp
            = نیا AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->جوڑو_Membersp(handleVarp);
        AstVar* ثابت argumentp = نیا AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::داخلہ);
        argumentp->declDirection(VDirection::داخلہ);
        argumentp->funcLocal(سچ);
        argumentp->noReset(سچ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->جوڑو_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(نیا Ast_تفویض{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    نیا AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        برائے (AstVarRef* ثابت refp : refs_مقصد_Rewrite) { rewriteVarRef(refp, handleVarp); }
        برائے (AstThisRef* ثابت refp : this_Refs_مقصد_Rewrite) { rewriteThisRef(refp, handleVarp); }
        برائے (AstFuncRef* ثابت refp : func_Refs_مقصد_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        برائے (Ast_گرہ_تفویض* ثابت constructp : constructps) {
            File_سطر* ثابت cfl = constructp->fileline();
            AstCExpr* ثابت thisp = نیا AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->جوڑو_Argsp(نیا AstArg{cfl, argumentp->name(), thisp});
        }
        واپس خالی_اشاریہ;
    }

    خالی دورہ(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        اگر (nodep->isCovergroup()) {
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
            m_embeddedVarp = ڈھونڈو_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = خالی_اشاریہ;
            m_constructorp = خالی_اشاریہ;
            std::سمتیہ<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_درخت* embeddedEventForkp = خالی_اشاریہ;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            منطقی hasUnsupportedEvent = جھوٹ;
            برائے (Ast_گرہ* itemp = nodep->membersp(); itemp;) {
                Ast_گرہ* ثابت nextp = itemp->nextp();
                اگر (AstCovergroup* ثابت cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    اگر (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        اگر (v3Global.opt.timing().is_سیٹ_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_عقب();
                        } ورنہ {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            اگر (embeddedEventTriggers.خالی_خانے()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = سچ;
                            }
                        }
                        VL_DO_DANGLING(ڈالو_Deletep(cgp->unlink_Fr_عقب()), cgp);
                        itemp = nextp;
                        جاری;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    جاری;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.ڈھونڈو_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(سچ);
            m_constructorp = VN_CAST(m_memberMap.ڈھونڈو_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            اگر (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_اشیاء();
                اگر (embeddedEventForkp) {
                    VL_DO_DANGLING(ڈالو_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                واپس;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ثابت std::سمتیہ<Ast_گرہ_تفویض*> constructps = ڈھونڈو_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            اگر (AstVarRef* ثابت invalidp = install_Enclosing_عقب_اشارہ_گر(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_اشیاء();
                اگر (embeddedEventForkp) {
                    VL_DO_DANGLING(ڈالو_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                واپس;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            اگر (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_اشیاء();
        } ورنہ {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    خالی دورہ(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ڈالو_عقب(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    خالی دورہ(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ڈالو_عقب(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    خالی دورہ(Ast_Cg_Option_تفویض* nodep) override { m_cgOptions.ڈالو_عقب(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ثابت std::سیٹ<ثابت AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = طےشدہ;
};

//######################################################################
// Functional coverage class functions

خالی V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ثابت Covergroup_تفویض_Valid_Visitor validVisitor{nodep};
    اگر (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_درخت("coveragefunc", 0, dump_درخت_Either_Level() >= 3);
}
