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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Covergroup.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <bitset>
#अन्तर्भाव <cmath>
#अन्तर्भाव <deque>
#अन्तर्भाव <set>
#अन्तर्भाव <tuple>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

वर्ग Covergroup_नियोजय_Valid_Visitor final : सार्वजनिक VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ध्रुव AstVar*, ध्रुव Ast_ग्रन्थिः_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::समुच्चयः<ध्रुव AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ध्रुव Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;
    तार्किक m_collecting = सत्य;
    तार्किक m_valid = सत्य;

    शून्य दर्शय(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = शून्यसूचकः;
        यदि (m_collecting) {
            ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव constructorp
                = VN_CAST(m_memberMap.findMember(nodep, "new"), ग्रन्थिः_FTask);
            प्रति (ध्रुव Ast_ग्रन्थिः* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstVar* ध्रुव varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                यदि (!varp || !varp->isClassMember() || varp->isDeclTyped()) अनुवर्त;
                ध्रुव AstClassRefDType* ध्रुव refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                यदि (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (!m_collecting) {
            ध्रुव AstVar* varp = शून्यसूचकः;
            यदि (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep->lhsp(), ग्रन्थिः_Var_Ref)) {
                varp = refp->varp();
            } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ध्रुव स्वचालित it = m_constructors.find(varp);
            यदि (it != m_constructors.अन्तः_2() && (!m_ftaskp || m_ftaskp != it->second)) {
                m_valid = असत्य;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        यदि (m_collecting && nodep->varp()->name() == "type_option") {
            ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep->backp(), StructSel);
            यदि (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.निवेशय(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्पष्ट Covergroup_नियोजय_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = असत्य;
        यदि (!m_constructors.रिक्तानि()) iterateConst(nodep);
    }
    तार्किक valid() ध्रुव { प्रत्यागम m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ध्रुव std::समुच्चयः<ध्रुव AstVar*>& mergeableTypeOptions() ध्रुव { प्रत्यागम m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

वर्ग Covergroup_व्यञ्जकम्_Valid_Visitor final : सार्वजनिक VNVisitor {
    ध्रुव std::समुच्चयः<ध्रुव AstVar*>& m_sampleMembers;
    ध्रुव std::समुच्चयः<ध्रुव AstVar*>& m_constructorRefMembers;
    तार्किक m_अन्तर्_Coverage_Expression = असत्य;
    तार्किक m_sampleFormalAllowed = असत्य;

    शून्य अवलोकनम्_Sample_Expression(Ast_ग्रन्थिः* nodep) {
        यदि (!nodep) प्रत्यागम;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = सत्य;
        iterate_And_अग्र_Null(nodep);
    }

    शून्य अवलोकनम्_Coverage_Expression(Ast_ग्रन्थिः* nodep) {
        यदि (!nodep) प्रत्यागम;
        VL_RESTORER(m_अन्तर्_Coverage_Expression);
        m_अन्तर्_Coverage_Expression = सत्य;
        iterate_And_अग्र_Null(nodep);
    }

    शून्य दर्शय(AstCoverpoint* nodep) override {
        अवलोकनम्_Sample_Expression(nodep->exprp());
        अवलोकनम्_Sample_Expression(nodep->iffp());
        iterate_And_अग्र_Null(nodep->binsp());
        iterate_And_अग्र_Null(nodep->optionsp());
    }
    शून्य दर्शय(AstCoverCross* nodep) override {
        iterate_And_अग्र_Null(nodep->itemsp());
        अवलोकनम्_Sample_Expression(nodep->iffp());
        iterate_And_अग्र_Null(nodep->optionsp());
        iterate_And_अग्र_Null(nodep->binsp());
    }
    शून्य दर्शय(AstCoverCrossBin* nodep) override {
        iterate_And_अग्र_Null(nodep->selectp());
        अवलोकनम्_Sample_Expression(nodep->iffp());
    }
    शून्य दर्शय(AstCoverCrossWith* nodep) override {
        iterate_And_अग्र_Null(nodep->selectp());
        अवलोकनम्_Coverage_Expression(nodep->filterp());
        अवलोकनम्_Coverage_Expression(nodep->countp());
    }
    शून्य दर्शय(AstCoverBinsof* nodep) override { अवलोकनम्_Coverage_Expression(nodep->rangesp()); }
    शून्य दर्शय(AstCoverBin* nodep) override {
        अवलोकनम्_Coverage_Expression(nodep->rangesp());
        अवलोकनम्_Sample_Expression(nodep->iffp());
        अवलोकनम्_Coverage_Expression(nodep->arraySizep());
        अवलोकनम्_Coverage_Expression(nodep->transp());
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_sampleFormalAllowed && m_sampleMembers.गणनम्(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        यदि (m_अन्तर्_Coverage_Expression && m_constructorRefMembers.गणनम्(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (m_अन्तर्_Coverage_Expression && nodep->taskp()) {
            तार्किक invalidDirection = असत्य;
            प्रति (Ast_ग्रन्थिः* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
                यदि (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = सत्य;
                    भङ्ग;
                }
            }
            यदि (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    Covergroup_व्यञ्जकम्_Valid_Visitor(ध्रुव std::समुच्चयः<ध्रुव AstVar*>& sampleMembers,
                               ध्रुव std::समुच्चयः<ध्रुव AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    शून्य अवलोकनम्(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

वर्ग Bin_सततम् final {
सार्वजनिक:
    // MEMBERS
    uint32_t m_गणनम्;  // Number of bins
    V3_सङ्ख्या m_lo;  // Lowest value of the first bin
    V3_सङ्ख्या m_stride;  // Number of values of each bin but the last
    V3_सङ्ख्या m_hi;  // Highest value of the last bin
    तार्किक m_रिक्तानि = असत्य;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_सततम्(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः, uint32_t गणनम्)
        : m_गणनम्{गणनम्}
        , m_lo{nodep, विस्तारः}
        , m_stride{nodep, विस्तारः, 1}
        , m_hi{nodep, विस्तारः} {}
};

//######################################################################
// Functional coverage visitor

वर्ग FunctionalCoverageVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // STATE
    std::समुच्चयः<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::समुच्चयः<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = शून्यसूचकः;  // Current covergroup being processed
    Ast_ग्रन्थिः_Module* m_unitp = शून्यसूचकः;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = शून्यसूचकः;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = शून्यसूचकः;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = शून्यसूचकः;  // Current sample() function
    AstIf* m_cpGuardp = शून्यसूचकः;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = शून्यसूचकः;  // Current constructor
    std::सदिशम्<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::सदिशम्<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::सदिशम्<Ast_Cg_Option_नियोजय*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cgTypeWeight = 1;  // The covergroup's type_option.weight, a constant
    तार्किक m_cgMergeInstances = असत्य;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    तार्किक m_cgMayMerge = असत्य;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ध्रुव std::समुच्चयः<ध्रुव AstVar*>& m_mergeableTypeOptions;
    पूर्णाङ्क m_cg_At_न्यूनतमम् = 1;  // Covergroup option.at_least

    संरचना EmbeddedEventTrigger final {
        File_पङ्क्तिः* eventFl;  // Clocking-event source location
        AstVar* आधारः_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType edgeType;  // Clocking-event edge qualifier
        AstVar* पूर्व_Varp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_पङ्क्तिः* eventFl, AstVar* आधारः_Varp, AstVar* memberVarp,
                             VEdgeType edgeType)
            : eventFl{eventFl}
            , आधारः_Varp{आधारः_Varp}
            , memberVarp{memberVarp}
            , edgeType{edgeType}
            , पूर्व_Varp{शून्यसूचकः} {}
    };

    std::समुच्चयः<std::string> m_crossed_Cp_नामानि;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    संरचना BinRuns final {
        std::सदिशम्<Bin_सततम्> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t गणनम् = 0;  // Bins across all runs
        तार्किक unsupported = असत्य;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::सदिशम्<std::string> मानानि;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        तार्किक single = असत्य;
    };
    संरचना Cross_Bin_मानानि final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ध्रुव Bin_सततम्* runp = शून्यसूचकः;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    संरचना Bin_विस्तृतिः final {
        uint32_t first = 0;  // First Normal index of the bin declaration
        uint32_t गणनम् = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    संरचना CoverpointBins final {
        uint32_t योग = 0;  // Number of Normal bins
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = शून्यसूचकः;  // Sampled expression, for the value domain
        तार्किक crossed = असत्य;  // Feeds a cross, which needs 'values'
        std::सदिशम्<Cross_Bin_मानानि> मानानि;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_विस्तृतिः> spans;  // Declared bin name -> index span
        Bin_विस्तृतिः implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_सततम्> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ध्रुव AstCoverpoint*, std::सदिशम्<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_नामानि m_sized_नामानि{"__Vsized"};
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> m_detached_मानानि;  // Array-bin values m_cpBins refers to
    std::समुच्चयः<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    उपयुज्य CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, तार्किक>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = शून्यसूचकः;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(तार्किक typeOption) {
        AstVar* ध्रुव varp = VN_AS(
            m_memberMap.findMember(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        प्रत्यागम varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_पङ्क्तिः* fl, AstVar* optionVarp, ध्रुव std::string& member,
                               VAccess access) {
        ध्रुव AstMemberDType* ध्रुव memberp
            = VN_AS(m_memberMap.findMember(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = optionVarp->lifetime().isStatic()
                                       ? नूतन AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ध्रुव selp = नूतन AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_गन्तव्यम्_Enump());
        selp->did_विस्तारः(सत्य);
        प्रत्यागम selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    स्थिर पूर्णाङ्क const_At_न्यूनतमम्(ध्रुव AstConst* constp) {
        ध्रुव पूर्णाङ्क मानम् = constp->गन्तव्यम्_SInt();
        यदि (मानम् >= 0) प्रत्यागम मानम्;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        प्रत्यागम 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    स्थिर ध्रुव AstConst* constOption(ध्रुव Ast_ग्रन्थिः* valuep, VCoverOptionType optType) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) प्रत्यागम constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        प्रत्यागम शून्यसूचकः;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    शून्य अधस्तनम्_Covergroup_Options() {
        तार्किक merge_समुच्चयः = असत्य;  // The covergroup sets type_option.merge_instances
        m_cg_At_न्यूनतमम् = 1;
        प्रति (Ast_Cg_Option_नियोजय* ध्रुव optp : m_cgOptions) {
            यदि (optp->optType() == VCoverOptionType::AT_न्यूनतमम्) {
                यदि (ध्रुव AstConst* ध्रुव constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_न्यूनतमम् = const_At_न्यूनतमम्(constp);
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(optp->unlink_Fr_पृष्ठभागः()), optp);
                अनुवर्त;
            }
            File_पङ्क्तिः* ध्रुव fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ध्रुव AstConst* ध्रुव constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            यदि (optp->optType() == VCoverOptionType::MERGE_INSTANCES) {
                member = "merge_instances";
                m_cgMergeInstances = !constp->अङ्कः().isEqZero();
                merge_समुच्चयः = सत्य;
            } अन्यथा यदि (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } अन्यथा {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::WEIGHT, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                यदि (optp->typeOption()) m_cgTypeWeight = constp->गन्तव्यम्_UInt();
            }
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_पृष्ठभागः()};
            यदि (optp->typeOption()) {
                m_covergroupp->सङ्कलय_Membersp(नूतन AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(प्रक्षिप_Deletep(optp->unlink_Fr_पृष्ठभागः()), optp);
            } अन्यथा {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        यदि (!merge_समुच्चयः && v3Global.opt.coverageMergeInstances()) {
            File_पङ्क्तिः* ध्रुव fl = m_covergroupp->fileline();
            m_covergroupp->सङ्कलय_Membersp(नूतन AstInitialStatic{
                fl, नूतन Ast_नियोजय{
                        fl, newOptionSel(fl, optionVar(सत्य), "merge_instances", VAccess::WRITE),
                        नूतन AstConst{fl, AstConst::BitTrue{}}}});
            m_cgMergeInstances = सत्य;
        }
        m_cgMayMerge = m_cgMergeInstances || m_mergeableTypeOptions.गणनम्(optionVar(सत्य));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t itemDatabaseWeight(Ast_ग्रन्थिः* optionsp) ध्रुव {
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* weightp = शून्यसूचकः;  // The option.weight in effect
        uint32_t typeWeight = 1;
        प्रति (Ast_ग्रन्थिः* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverOption* ध्रुव optp = VN_AS(nodep, CoverOption);
            यदि (!(optp->optType() == VCoverOptionType::WEIGHT)) अनुवर्त;
            // V3Width left type_option.weight a non-negative constant
            यदि (optp->typeOption()) {
                typeWeight = VN_AS(optp->valuep(), Const)->गन्तव्यम्_UInt();
            } अन्यथा {
                weightp = optp->valuep();
            }
        }
        यदि (m_cgMergeInstances) प्रत्यागम typeWeight;
        यदि (!weightp) प्रत्यागम 1;
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(weightp, Const)) प्रत्यागम constp->गन्तव्यम्_UInt();
        प्रत्यागम typeWeight;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    शून्य generateItemWeight(File_पङ्क्तिः* fl, AstVar* itemVarp, Ast_ग्रन्थिः* optionsp) {
        प्रति (Ast_ग्रन्थिः* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverOption* ध्रुव optp = VN_AS(nodep, CoverOption);
            यदि (!(optp->optType() == VCoverOptionType::WEIGHT)) अनुवर्त;
            यदि (!optp->typeOption()) {
                m_constructorp->सङ्कलय_Stmtsp(
                    itemCall(fl, itemVarp, VCMethod::COVERGROUP_WEIGHT,
                             {optp->valuep()->clone_वृक्षः(असत्य), file_पङ्क्तिः_Debug(optp->fileline())})
                        ->makeStmt());
            } अन्यथा यदि (m_cgMayMerge) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, itemVarp, VCMethod::COVERGROUP_TYPE_WEIGHT,
                                                   {optp->valuep()->clone_वृक्षः(असत्य)})
                                              ->makeStmt());
            }
        }
    }

    शून्य processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.आकार() << " coverpoints and "
                                           << m_coverCrosses.आकार() << " crosses");

        m_crossed_Cp_नामानि.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_नामानि.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = शून्यसूचकः;
        m_cgTypeWeight = 1;
        m_cgMergeInstances = असत्य;
        m_cgMayMerge = असत्य;

        अधस्तनम्_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        प्रति (AstCoverCross* crossp : m_coverCrosses) {
            प्रति (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                यदि (refp->exprp()) अनुवर्त;  // hierarchical ref: dropped in generateCrossCode
                यदि (m_coverpointMap.find(refp->name()) == m_coverpointMap.अन्तः_2()) {
                    m_droppedCrosses.निवेशय(crossp);  // bare variable: drop this cross only
                } अन्यथा {
                    m_crossed_Cp_नामानि.निवेशय(refp->name());
                }
            }
        }

        std::सदिशम्<Ast_ग्रन्थिः*> pending;
        std::map<AstCoverpoint*, std::सदिशम्<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::सदिशम्<AstCoverpoint*>> inputs;
        प्रति (AstCoverpoint* ध्रुव cpp : m_coverpoints) {
            check_Bin_नामानि(cpp);
            checkConstructedBins(cpp);
            यदि (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) अनुवर्त;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            यदि (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_रिक्तानि_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                अनुवर्त;
            }
            m_runtimePoints.निवेशय(cpp);
            pending.प्रक्षिप_पृष्ठभागः(cpp);
        }
        प्रति (AstCoverCross* ध्रुव crossp : m_coverCrosses) {
            यदि (m_droppedCrosses.गणनम्(crossp)) अनुवर्त;
            प्रति (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                यदि (refp->exprp()) अनुवर्त;
                ध्रुव स्वचालित point = m_coverpointMap.find(refp->name());
                यदि (point != m_coverpointMap.अन्तः_2()) {
                    consumers[point->second].प्रक्षिप_पृष्ठभागः(crossp);
                    inputs[crossp].प्रक्षिप_पृष्ठभागः(point->second);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            यदि (crossp->exists([](ध्रुव AstCoverCrossWith*) { प्रत्यागम सत्य; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.प्रक्षिप_पृष्ठभागः(crossp);
            }
        }
        प्रति (आकार_t अग्र = 0; अग्र < pending.आकार(); ++अग्र) {
            यदि (AstCoverpoint* ध्रुव pointp = VN_CAST(pending[अग्र], Coverpoint)) {
                प्रति (AstCoverCross* ध्रुव crossp : consumers[pointp]) {
                    यदि (m_runtimeCrosses.emplace(crossp).second) pending.प्रक्षिप_पृष्ठभागः(crossp);
                }
            } अन्यथा {
                प्रति (AstCoverpoint* ध्रुव pointp : inputs[VN_AS(pending[अग्र], CoverCross)]) {
                    यदि (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).second) {
                        pending.प्रक्षिप_पृष्ठभागः(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        प्रति (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_संहिता(cpp);

        // For each cross, generate sampling code
        प्रति (AstCoverCross* crossp : m_coverCrosses) generate_Cross_संहिता(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        प्रति (AstCoverpoint* ध्रुव cpp : m_coverpoints) {
            यदि (!m_runtimePoints.गणनम्(cpp)) अनुवर्त;
            m_constructorp->सङ्कलय_Stmtsp(itemCall(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_मानम्_RELEASE)
                                          ->makeStmt());
        }
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* valuep : m_detached_मानानि) VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
        m_detached_मानानि.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_संहिता();
    }

    स्थिर constexpr आकार_t मानम्_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    स्थिर int64_t const_गणनम्(ध्रुव AstConst* constp) {
        ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
        यदि (constp->isSigned() && अङ्कः.isNegative()) प्रत्यागम -1;
        प्रत्यागम अङ्कः.most_समुच्चयः_Bit_P1() > 32 ? INT64_MAX : static_cast<int64_t>(अङ्कः.गन्तव्यम्_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    स्थिर तार्किक check_गणनम्(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* countp, ध्रुव वर्ण* whatp, ध्रुव वर्ण* clausep) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(countp, Const);
        यदि (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            प्रत्यागम असत्य;
        } अन्यथा यदि (constp && (constp->अङ्कः().isFourState() || const_गणनम्(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->अङ्कः().isFourState() ? constp->अङ्कः().ascii(असत्य)
                                                                  : constp->अङ्कः().गन्तव्यम्_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    स्थिर uint32_t autoBinsRequested(ध्रुव AstCoverBin* binp) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(binp->arraySizep(), Const);
        यदि (!constp) प्रत्यागम 0;
        ध्रुव int64_t गणनम् = const_गणनम्(constp);
        प्रत्यागम गणनम् < 1 || गणनम् > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(गणनम्);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    स्थिर तार्किक isAutoBins(ध्रुव AstCoverBin* binp) {
        प्रत्यागम binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    स्थिर तार्किक isSizedArray(ध्रुव AstCoverBin* binp) {
        प्रत्यागम binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    स्थिर AstCoverWith* binWith(ध्रुव AstCoverBin* binp) {
        प्रत्यागम VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    स्थिर तार्किक isConstructedBins(ध्रुव AstCoverBin* binp) {
        प्रत्यागम isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    स्थिर Ast_ग्रन्थिः* binRangesp(ध्रुव AstCoverBin* binp) {
        ध्रुव AstCoverWith* ध्रुव withp = binWith(binp);
        यदि (!withp) प्रत्यागम binp->rangesp();
        प्रत्यागम VN_IS(withp->subp(), CoverpointRef) ? शून्यसूचकः : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    शून्य check_Bin_नामानि(AstCoverpoint* coverpointp) {
        std::समुच्चयः<std::string> नामानि;
        प्रति (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            यदि (नामानि.emplace(binp->name()).second) अनुवर्त;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(binp->unlink_Fr_पृष्ठभागः()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    शून्य dropBins(ध्रुव AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].प्रक्षिप_पृष्ठभागः(binp->name());
        VL_DO_DANGLING(प्रक्षिप_Deletep(binp->unlink_Fr_पृष्ठभागः()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    तार्किक check_Bins_Array_आकार(ध्रुव AstCoverpoint* coverpointp, AstCoverBin* binp, तार्किक integral) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizep = binp->arraySizep();
        यदि (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } अन्यथा यदि (check_गणनम्(sizep, "Bins array size", "19.5.1")) {
            यदि (integral) प्रत्यागम सत्य;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(प्रक्षिप_Deletep(sizep->unlink_Fr_पृष्ठभागः()), sizep);
            प्रत्यागम असत्य;
        }
        dropBins(coverpointp, binp);
        प्रत्यागम असत्य;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    शून्य checkConstructedBins(AstCoverpoint* coverpointp) {
        ध्रुव तार्किक integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        प्रति (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            यदि (!isConstructedBins(binp)) अनुवर्त;
            यदि (isSizedArray(binp) && !check_Bins_Array_आकार(coverpointp, binp, integral)) अनुवर्त;
            यदि (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                यदि (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    यदि (binp->binsType().bin_Is_प्रसामान्यम्()) coverpointp->user2(सत्य);
                    dropBins(coverpointp, binp);
                }
                अनुवर्त;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ध्रुव तार्किक single = !binp->binsType().bin_Is_प्रसामान्यम्() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_अधिकम्()
                                         << "... Suggest a larger --coverage-max-bins");
            यदि (single) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizep = binp->arraySizep();
                VL_DO_DANGLING(प्रक्षिप_Deletep(sizep->unlink_Fr_पृष्ठभागः()), sizep);
                binp->isArray(असत्य);
                अनुवर्त;
            }
            यदि (binp->binsType().bin_Is_प्रसामान्यम्()) coverpointp->user2(सत्य);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    स्थिर तार्किक withCandidatesOver(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव पूर्णाङ्क विस्तारः = सततम्_विस्तारः(exprp);
        std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        यदि (!binRangesp(binp)) runs = coverpoint_मानानि(binp, exprp);
        ध्रुव स्वचालित constant
            = [](ध्रुव Ast_ग्रन्थिः* nodep) { प्रत्यागम VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        प्रति (Ast_ग्रन्थिः* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            यदि (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                प्रत्यागम असत्य;
            }
            Cross_मानम्_Range range{rangep, resolve_विस्तारः(rangep, exprp)};
            यदि (!resolve_मानम्(rangep, exprp, सत्य, binp->isWildcard(), range)
                || cross_Range_रिक्तानि(range)) {
                अनुवर्त;
            }
            std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> प्राप्तम्{{range.lo, range.hi}};
            यदि (range.wildcard) {  // checkConstructedBins bounded the runs
                प्राप्तम्.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), प्राप्तम्);
            }
            प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& सततम् : प्राप्तम्) {
                runs.emplace_पृष्ठभागः(V3_सङ्ख्या{rangep, विस्तारः, सततम्.first},
                                  V3_सङ्ख्या{rangep, विस्तारः, सततम्.second});
            }
        }
        यदि (!isSizedArray(binp)) {  // The union of the values
            std::क्रमय(runs.begin(), runs.अन्तः_2(), [](ध्रुव स्वचालित& lhs, ध्रुव स्वचालित& rhs) {
                प्रत्यागम cross_मानम्_Less(lhs.first, rhs.first);
            });
            std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> merged;
            प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& सततम् : runs) {
                यदि (merged.रिक्तानि() || cross_मानम्_Less(merged.पृष्ठभागः().second, सततम्.first)) {
                    merged.प्रक्षिप_पृष्ठभागः(सततम्);
                } अन्यथा यदि (cross_मानम्_Less(merged.पृष्ठभागः().second, सततम्.second)) {
                    merged.पृष्ठभागः().second = सततम्.second;
                }
            }
            runs = std::move(merged);
        }
        uint64_t गणनम् = 0;
        प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& सततम् : runs) {
            V3_सङ्ख्या विस्तृतिः{exprp, विस्तारः};
            विस्तृतिः.opSub(सततम्.second, सततम्.first);
            यदि (विस्तृतिः.most_समुच्चयः_Bit_P1() > 32) प्रत्यागम सत्य;  // More than 2**32 values
            गणनम् += विस्तृतिः.गन्तव्यम्_UQuad() + 1;
            यदि (गणनम् > (uint64_t{1} << 32)) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    स्थिर आकार_t sizedWildcardRuns(ध्रुव AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        प्रति (Ast_ग्रन्थिः* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            यदि (!VN_IS(rangep, Const)) अनुवर्त;  // A range, or a value known at construction
            Cross_मानम्_Range range{rangep, resolve_विस्तारः(rangep, exprp)};
            यदि (resolve_मानम्(rangep, exprp, सत्य, सत्य, range) && !cross_Range_रिक्तानि(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        प्रत्यागम runs.आकार();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    शून्य checkAutomaticBins(AstCoverpoint* coverpointp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        प्रति (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            यदि (cbinp->binsType() != VCoverBinsType::BINS_AUTO) अनुवर्त;
            ध्रुव AstConst* ध्रुव constp = VN_CAST(cbinp->arraySizep(), Const);
            यदि (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } अन्यथा यदि (const_गणनम्(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->अङ्कः().गन्तव्यम्_Decimal_S());
            } अन्यथा यदि (const_गणनम्(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->अङ्कः().गन्तव्यम्_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_अधिकम्() << "... Suggest a larger --coverage-max-bins");
            } अन्यथा यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    शून्य extractItemOptions(Ast_ग्रन्थिः* optionsp, पूर्णाङ्क& at_न्यूनतमम्_बहिर्गमनम्, पूर्णाङ्क& auto_Bin_Max_बहिर्गमनम्) {
        at_न्यूनतमम्_बहिर्गमनम् = m_cg_At_न्यूनतमम्;
        auto_Bin_Max_बहिर्गमनम् = -1;  // -1 = not set at coverpoint level
        प्रति (Ast_ग्रन्थिः* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ध्रुव optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            यदि (!(optp->optType() == VCoverOptionType::AT_न्यूनतमम्
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                अनुवर्त;
            }
            ध्रुव AstConst* ध्रुव constp = constOption(optp->valuep(), optp->optType());
            यदि (!constp) अनुवर्त;
            यदि (optp->optType() == VCoverOptionType::AT_न्यूनतमम्) {
                at_न्यूनतमम्_बहिर्गमनम् = const_At_न्यूनतमम्(constp);
            } अन्यथा {
                auto_Bin_Max_बहिर्गमनम् = constp->गन्तव्यम्_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        यदि (auto_Bin_Max_बहिर्गमनम् < 0) {
            यदि (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_बहिर्गमनम् = m_covergroupp->cgAutoBinMax();
            } अन्यथा {
                auto_Bin_Max_बहिर्गमनम् = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    शून्य createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                            ध्रुव AstEnumDType* enump) {
        File_पङ्क्तिः* ध्रुव fl = coverpointp->fileline();
        प्रति (ध्रुव AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ध्रुव lop = new_मानम्_Const(fl, VN_AS(itemp->valuep(), Const)->अङ्कः(), exprp);
            AstInsideRange* ध्रुव rangep = नूतन AstInsideRange{fl, lop, lop->clone_वृक्षः(असत्य)};
            rangep->dtypeFrom(exprp);
            coverpointp->सङ्कलय_Binsp(
                नूतन AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, असत्य, असत्य});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    शून्य createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क autoBinMax) {
        यदि (coverpointp->user2()) प्रत्यागम;  // Declared bins, ignored, leave no bins
        प्रति (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव VCoverBinsType विधा = VN_AS(nodep, CoverBin)->binsType();
            यदि (विधा != VCoverBinsType::BINS_IGNORE && विधा != VCoverBinsType::BINS_ILLEGAL)
                प्रत्यागम;
        }
        यदि (ध्रुव AstEnumDType* ध्रुव enump
            = VN_CAST(exprp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            प्रत्यागम;
        }
        ध्रुव पूर्णाङ्क विस्तारः = exprp->विस्तारः();
        uint32_t गणनम् = विस्तारः < 31 ? std::min<uint32_t>(uint32_t{1} << विस्तारः, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        यदि (!गणनम्) प्रत्यागम;
        यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            प्रत्यागम;
        }
        यदि (गणनम् > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_अधिकम्()
                                              << "... Suggest a larger --coverage-max-bins");
            गणनम् = v3Global.opt.coverageMaxBins();
        }
        File_पङ्क्तिः* ध्रुव fl = coverpointp->fileline();
        coverpointp->सङ्कलय_Binsp(नूतन AstCoverBin{fl, "auto", नूतन AstConst{fl, गणनम्},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    स्थिर string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.अन्तः_2(), '[', '_');
        std::replace(name.begin(), name.अन्तः_2(), ']', '_');
        प्रत्यागम name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_गन्तव्यम्_अस्थायी(Ast_ग्रन्थिः_व्यञ्जकम्* iffp, ध्रुव string& अस्थायी_Name) {
        File_पङ्क्तिः* ध्रुव fl = iffp->fileline();
        AstVar* ध्रुव iffVarp
            = नूतन AstVar{fl, VVarType::BLOCKTEMP, अस्थायी_Name, iffp->findBitDType()};
        iffVarp->funcLocal(सत्य);
        m_sampleFuncp->सङ्कलय_Stmtsp(iffVarp);
        iffp->unlink_Fr_पृष्ठभागः();
        m_sampleFuncp->सङ्कलय_Stmtsp(
            नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        प्रत्यागम नूतन AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    शून्य सङ्कलय_Sample_Stmt(Ast_ग्रन्थिः* stmtp) {
        यदि (m_cpGuardp) {
            m_cpGuardp->सङ्कलय_Thensp(stmtp);
        } अन्यथा {
            m_sampleFuncp->सङ्कलय_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_पूर्व_मानम्_Var(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Check if already created
        यदि (AstVar* ध्रुव पूर्व_Varp = VN_CAST(coverpointp->user1p(), Var)) प्रत्यागम पूर्व_Varp;

        // Create variable to store previous sampled value
        ध्रुव string varName = "__Vprev_" + coverpointp->name();
        AstVar* पूर्व_Varp
            = नूतन AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->सङ्कलय_Membersp(पूर्व_Varp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव initExprp
            = नूतन AstConst{पूर्व_Varp->fileline(), AstConst::Widthed_मानम्{}, पूर्व_Varp->विस्तारः(), 0};
        Ast_ग्रन्थिः_Stmt* ध्रुव initStmtp = नूतन Ast_नियोजय{
            पूर्व_Varp->fileline(), नूतन AstVarRef{पूर्व_Varp->fileline(), पूर्व_Varp, VAccess::WRITE},
            initExprp};
        m_constructorp->सङ्कलय_Stmtsp(initStmtp);

        coverpointp->user1p(पूर्व_Varp);
        प्रत्यागम पूर्व_Varp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ध्रुव string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = नूतन AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->सङ्कलय_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ग्रन्थिः_Stmt* ध्रुव initStmtp = नूतन Ast_नियोजय{
            stateVarp->fileline(), नूतन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            नूतन AstConst{stateVarp->fileline(), AstConst::Widthed_मानम्{}, 8, 0}};
        m_constructorp->सङ्कलय_Stmtsp(initStmtp);

        प्रत्यागम stateVarp;
    }

    शून्य generate_Coverpoint_संहिता(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        पूर्णाङ्क at_न्यूनतमम्_मानम्;
        पूर्णाङ्क autoBinMax;
        extractItemOptions(coverpointp->optionsp(), at_न्यूनतमम्_मानम्, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_न्यूनतमम्_मानम् << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iffp = coverpointp->iffp()) {
            m_cpGuardp = नूतन AstIf{iffp->fileline(), iffp->unlink_Fr_पृष्ठभागः()};
        }

        AstVar* ध्रुव मानम्_Varp = नूतन AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        मानम्_Varp->funcLocal(सत्य);
        m_sampleFuncp->सङ्कलय_Stmtsp(मानम्_Varp);
        exprp->unlink_Fr_पृष्ठभागः();
        सङ्कलय_Sample_Stmt(नूतन Ast_नियोजय{
            coverpointp->fileline(),
            नूतन AstVarRef{coverpointp->fileline(), मानम्_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(नूतन AstVarRef{coverpointp->fileline(), मानम्_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_न्यूनतमम्_मानम्);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        यदि (m_cpGuardp) m_sampleFuncp->सङ्कलय_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ग्रन्थिः_व्यञ्जकम्* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                       File_पङ्क्तिः* fl) {
        Ast_ग्रन्थिः_व्यञ्जकम्* anyBinMatchp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            यदि (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                अनुवर्त;
            // The values of constructed bins are known at construction; see emitSizedSample
            यदि (isConstructedBins(cbinp)) अनुवर्त;
            यदि (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                यदि (anyBinMatchp) VL_DO_DANGLING(प्रक्षिप_Deletep(anyBinMatchp), anyBinMatchp);
                प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? नूतन AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        प्रत्यागम anyBinMatchp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstConst{fl, AstConst::BitTrue{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    स्थिर तार्किक coverpointHasStateExclusions(ध्रुव AstCoverpoint* coverpointp) {
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            यदि (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक coverpointHasConstructedBins(ध्रुव AstCoverpoint* coverpointp) {
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            यदि (isConstructedBins(VN_AS(nodep, CoverBin))) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    स्थिर तार्किक coverpoint_Has_रिक्तानि_Bins(ध्रुव AstCoverpoint* coverpointp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = coverpointp->exprp();
        प्रति (Ast_ग्रन्थिः* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstCoverBin* ध्रुव binp = VN_AS(nodep, CoverBin);
            यदि (!binp->binsType().bin_Is_प्रसामान्यम्() || binp->transp() || !binp->rangesp()) अनुवर्त;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            यदि ((binp->isArray() && binp->isWildcard()) || binWith(binp)) अनुवर्त;
            तार्किक रिक्तानि = सत्य;
            प्रति (Ast_ग्रन्थिः* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                यदि (binp->isArray() && VN_IS(valuep, InsideRange)) अनुवर्त;
                Cross_मानम्_Range range{valuep, resolve_विस्तारः(valuep, exprp)};
                ध्रुव तार्किक none = resolve_मानम्(valuep, exprp, सत्य, binp->isWildcard(), range)
                                  && cross_Range_रिक्तानि(range);
                यदि (binp->isArray() && none) प्रत्यागम सत्य;
                रिक्तानि &= none;
            }
            यदि (रिक्तानि && !binp->isArray()) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    स्थिर तार्किक coverpointHasTransition(AstCoverpoint* coverpointp) {
        प्रति (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            यदि (VN_AS(binp, CoverBin)->transp()) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    स्थिर AstBasicDType* basicDType(File_पङ्क्तिः* fl, VBasicDTypeKwd kwd) {
        प्रत्यागम v3Global.rootp()->typeTablep()->findBasicDType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_पङ्क्तिः* fl, uint32_t लब्धम्_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[लब्धम्_Bound];
        यदि (!typep) {
            typep = नूतन AstCoverpointDType{fl, लब्धम्_Bound};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(typep);
        }
        प्रत्यागम typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ध्रुव {
        प्रत्यागम VIdProtect::protect_शब्दाः_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ध्रुव {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ध्रुव std::string& libname = m_unitp->libname();
        ध्रुव std::string name = m_covergroupp->dtypeName(सत्य);
        प्रत्यागम libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    शून्य generateInstanceAttach() {
        File_पङ्क्तिः* ध्रुव fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = नूतन AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->सङ्कलय_Membersp(m_cgInstVarp);

        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cgMayMerge ? ", true" : ", false") + ")")},
                     /*usePtr=*/असत्य)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ध्रुव weightAddrp = नूतन AstCExpr{fl, "&"};
        weightAddrp->सङ्कलय(newOptionSel(fl, optionVar(असत्य), "weight", VAccess::READ));
        m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_WEIGHT,
                                           {weightAddrp, file_पङ्क्तिः_Debug(fl)}, /*usePtr=*/असत्य)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_पङ्क्तिः* fl, VCMethod विधानम्) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ध्रुव instp
            = नूतन AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(असत्य);
        instp->dtype_समुच्चयः_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{fl, instp, विधानम्};
        callp->usePtr(सत्य);
        प्रत्यागम callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_नियोजय* makeItemCreate(File_पङ्क्तिः* fl, AstVar* itemVarp, VCMethod विधानम्) {
        AstCMethodHard* ध्रुव createp = instanceCall(fl, विधानम्);
        createp->dtypep(itemVarp->dtypep());
        प्रत्यागम नूतन Ast_नियोजय{fl, memberRef(fl, itemVarp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    संरचना RangeBounds final {
        Ast_ग्रन्थिः* loNodep = शून्यसूचकः;  // low bound: AstConst or AstUnbounded
        Ast_ग्रन्थिः* hiNodep = शून्यसूचकः;  // high bound: AstConst or AstUnbounded
        तार्किक loUnbounded() ध्रुव { प्रत्यागम VN_IS(loNodep, Unbounded); }
        तार्किक hiUnbounded() ध्रुव { प्रत्यागम VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ध्रुव { प्रत्यागम VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ध्रुव { प्रत्यागम VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    स्थिर तार्किक constRangeBounds(Ast_ग्रन्थिः* rp, RangeBounds& rb) {
        यदि (AstInsideRange* ध्रुव irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } अन्यथा यदि (AstConst* ध्रुव cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } अन्यथा {
            प्रत्यागम असत्य;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ध्रुव lc = rb.loConstp();
        AstConst* ध्रुव hc = rb.hiConstp();
        यदि ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) प्रत्यागम असत्य;
        यदि ((lc && lc->अङ्कः().isFourState()) || (hc && hc->अङ्कः().isFourState())) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    स्थिर तार्किक extractRangeIntervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::सदिशम्<std::pair<uint64_t, uint64_t>>& बहिर्गमनम्) {
        यदि (!cbinp->rangesp()) प्रत्यागम असत्य;
        प्रति (Ast_ग्रन्थिः* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            यदि (!constRangeBounds(rp, rb)) प्रत्यागम असत्य;
            यदि ((rb.loConstp() && rb.loConstp()->विस्तारः() > 64)
                || (rb.hiConstp() && rb.hiConstp()->विस्तारः() > 64)) {
                प्रत्यागम असत्य;  // Use the safe slot-count bound for wide values.
            }
            ध्रुव uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->गन्तव्यम्_UQuad();
            ध्रुव uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->गन्तव्यम्_UQuad();
            यदि (lo > hi) प्रत्यागम असत्य;
            बहिर्गमनम्.emplace_पृष्ठभागः(lo, hi);
        }
        प्रत्यागम सत्य;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    तार्किक appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                             std::सदिशम्<std::सदिशम्<std::pair<uint64_t, uint64_t>>>& bins,
                             पूर्णाङ्क& slot_गणनम्) {
        यदि (isAutoBins(cbinp)) {
            ++slot_गणनम्;
            bins.प्रक्षिप_पृष्ठभागः({{0, maxVal}});
            प्रत्यागम exprp->विस्तारः() <= 64;
        }
        यदि (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            पूर्णाङ्क elements = 0;
            प्रति (ध्रुव Ast_ग्रन्थिः* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            slot_गणनम् += std::max(1, elements);
            प्रत्यागम असत्य;
        }
        यदि (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++slot_गणनम्;
            यदि (exprp->isSigned() || exprp->विस्तारः() > 64) प्रत्यागम असत्य;
            ध्रुव BinRuns runs = wildcardBinRuns(cbinp, exprp, असत्य);
            यदि (runs.unsupported) प्रत्यागम असत्य;
            std::सदिशम्<std::pair<uint64_t, uint64_t>> ivs;
            प्रति (ध्रुव Bin_सततम्& सततम् : runs.runs) {
                ivs.emplace_पृष्ठभागः(सततम्.m_lo.गन्तव्यम्_UQuad(), सततम्.m_hi.गन्तव्यम्_UQuad());
            }
            bins.प्रक्षिप_पृष्ठभागः(std::move(ivs));
            प्रत्यागम सत्य;
        }
        यदि (cbinp->isArray()) प्रत्यागम appendArrayBinCrossSlots(cbinp, exprp, bins, slot_गणनम्);
        // Non-array bin: one slot covering the union of its intervals.
        ++slot_गणनम्;
        std::सदिशम्<std::pair<uint64_t, uint64_t>> ivs;
        यदि (cbinp->isWildcard() || !extractRangeIntervals(cbinp, maxVal, ivs)) प्रत्यागम असत्य;
        bins.प्रक्षिप_पृष्ठभागः(std::move(ivs));
        प्रत्यागम सत्य;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    तार्किक appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                  std::सदिशम्<std::सदिशम्<std::pair<uint64_t, uint64_t>>>& bins,
                                  पूर्णाङ्क& slot_गणनम्) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        तार्किक exact = !exprp->isSigned() && !cbinp->isWildcard();
        प्रति (Ast_ग्रन्थिः* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++slot_गणनम्;
            RangeBounds rb;
            Cross_मानम्_Range range{rp, resolve_विस्तारः(rp, exprp)};
            यदि (!exact || !constRangeBounds(rp, rb)
                || !resolve_मानम्(rp, exprp, सत्य, असत्य, range)) {
                exact = असत्य;
                अनुवर्त;
            }
            यदि (cross_Range_रिक्तानि(range)) अनुवर्त;  // Its bin, if any, holds no value
            यदि (range.hi.most_समुच्चयः_Bit_P1() > 64) {
                exact = असत्य;
                अनुवर्त;
            }
            bins.प्रक्षिप_पृष्ठभागः({{range.lo.गन्तव्यम्_UQuad(), range.hi.गन्तव्यम्_UQuad()}});
        }
        प्रत्यागम exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    पूर्णाङ्क compute_लब्धम्_List_Bound(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक crossFed) {
        यदि (!crossFed) प्रत्यागम 1;
        ध्रुव पूर्णाङ्क विस्तारः = exprp->विस्तारः();
        ध्रुव uint64_t maxVal = (विस्तारः >= 64) ? UINT64_MAX : ((1ULL << विस्तारः) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::सदिशम्<std::सदिशम्<std::pair<uint64_t, uint64_t>>> bins;
        पूर्णाङ्क slot_गणनम् = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        तार्किक exact = !exprp->isSigned();
        प्रति (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            यदि (!cbinp->binsType().bin_Is_प्रसामान्यम्())
                अनुवर्त;  // ignore/illegal/default: not hit-listed
            यदि (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, slot_गणनम्)) exact = असत्य;
        }
        यदि (!exact) प्रत्यागम std::max(1, slot_गणनम्);
        यदि (bins.रिक्तानि()) प्रत्यागम 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::सदिशम्<uint64_t> pts;
        प्रति (ध्रुव स्वचालित& b : bins)
            प्रति (ध्रुव स्वचालित& iv : b) pts.प्रक्षिप_पृष्ठभागः(iv.first);
        std::क्रमय(pts.begin(), pts.अन्तः_2());
        pts.erase(std::unique(pts.begin(), pts.अन्तः_2()), pts.अन्तः_2());
        पूर्णाङ्क maxOverlap = 1;
        प्रति (ध्रुव uint64_t p : pts) {
            पूर्णाङ्क cnt = 0;
            प्रति (ध्रुव स्वचालित& b : bins) {
                प्रति (ध्रुव स्वचालित& iv : b) {
                    यदि (iv.first <= p && p <= iv.second) {
                        ++cnt;
                        भङ्ग;  // count each bin at most once
                    }
                }
            }
            यदि (cnt > maxOverlap) maxOverlap = cnt;
        }
        प्रत्यागम maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_पङ्क्तिः* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ध्रुव refp = नूतन AstVarRef{fl, varp, access};
        refp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
        प्रत्यागम refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* itemCall(File_पङ्क्तिः* fl, AstVar* varp, VCMethod विधानम्,
                             ध्रुव std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& तर्काः = {}, तार्किक usePtr = सत्य) {
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{fl, memberRef(fl, varp), विधानम्};
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp : तर्काः) callp->सङ्कलय_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_समुच्चयः_Void();
        प्रत्यागम callp;
    }

    // An unsigned integer argument.
    स्थिर AstConst* cnum(File_पङ्क्तिः* fl, uint32_t मानम्) { प्रत्यागम नूतन AstConst{fl, मानम्}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    स्थिर AstCExpr* ctext(File_पङ्क्तिः* fl, ध्रुव std::string& text) {
        प्रत्यागम नूतन AstCExpr{fl, text};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    स्थिर std::string quoted(ध्रुव std::string& text) {
        प्रत्यागम "\"" + V3_बहिर्गमनम्_Formatter::quoteNameControls(text) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    स्थिर AstCExpr* file_पङ्क्तिः_Debug(File_पङ्क्तिः* fl) {
        ध्रुव std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        प्रत्यागम ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::गन्तव्यम्_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    स्थिर तार्किक checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ग्रन्थिः* rangep, तार्किक report = सत्य) {
        यदि (ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange)) {
            ध्रुव AstConst* ध्रुव minp = VN_CAST(irp->lhsp(), Const);
            ध्रुव AstConst* ध्रुव maxp = VN_CAST(irp->rhsp(), Const);
            यदि ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                यदि (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                प्रत्यागम असत्य;
            }
            यदि ((minp && minp->अङ्कः().isFourState()) || (maxp && maxp->अङ्कः().isFourState())) {
                यदि (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                प्रत्यागम असत्य;
            }
        } अन्यथा यदि (!VN_IS(rangep, Const)) {
            यदि (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> extract_Array_मानानि(AstCoverBin* arrayBinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                                 तार्किक& unsupported_बहिर्गमनम्) {
        unsupported_बहिर्गमनम् = असत्य;
        ध्रुव पूर्णाङ्क विस्तारः = exprp->विस्तारः();
        ध्रुव uint64_t maxVal = (विस्तारः >= 64) ? UINT64_MAX : ((1ULL << विस्तारः) - 1);
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> मानानि;
        प्रति (Ast_ग्रन्थिः* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            यदि (!checkArrayBinElement(arrayBinp, rangep)) प्रत्यागम मानानि;
            यदि (AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange)) {
                ध्रुव तार्किक loUnb = VN_IS(irp->lhsp(), Unbounded);
                ध्रुव तार्किक hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ध्रुव uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->गन्तव्यम्_UQuad();
                ध्रुव uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->गन्तव्यम्_UQuad();
                यदि (hi < lo) अनुवर्त;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ध्रुव uint64_t विस्तृतिः = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                यदि (विस्तृतिः >= v3Global.opt.coverageMaxRealBins()
                    || मानानि.आकार() + विस्तृतिः + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_अधिकम्()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_बहिर्गमनम् = सत्य;
                    प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव vp : मानानि) VL_DO_DANGLING(प्रक्षिप_Deletep(vp), vp);
                    मानानि.clear();
                    प्रत्यागम मानानि;
                }
                प्रति (uint64_t v = lo; v <= hi; ++v)
                    मानानि.प्रक्षिप_पृष्ठभागः(नूतन AstConst{irp->fileline(), AstConst::Widthed_मानम्{}, विस्तारः,
                                                  static_cast<uint32_t>(v)});
            } अन्यथा {
                मानानि.प्रक्षिप_पृष्ठभागः(VN_AS(rangep->clone_वृक्षः(असत्य), ग्रन्थिः_व्यञ्जकम्));
            }
        }
        प्रत्यागम मानानि;
    }

    स्थिर पूर्णाङ्क सततम्_विस्तारः(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) { प्रत्यागम exprp->विस्तारः() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    तार्किक autoBinRuns(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, BinRuns& बहिर्गमनम्) {
        ध्रुव uint32_t requested = autoBinsRequested(binp);
        यदि (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) प्रत्यागम असत्य;
        ध्रुव पूर्णाङ्क विस्तारः = exprp->विस्तारः();
        ध्रुव पूर्णाङ्क arithmetic_विस्तारः = सततम्_विस्तारः(exprp);
        ध्रुव uint32_t गणनम्
            = विस्तारः < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << विस्तारः, requested))
                  : requested;
        Bin_सततम् सततम्{binp, arithmetic_विस्तारः, गणनम्};
        V3_सङ्ख्या योग{binp, arithmetic_विस्तारः};
        योग.समुच्चयः_Bit(विस्तारः, 1);
        सततम्.m_stride.opDiv(योग, V3_सङ्ख्या{binp, arithmetic_विस्तारः, गणनम्});
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(binp, विस्तारः, exprp->isSigned(), arithmetic_विस्तारः);
        सततम्.m_lo = domain.lo;
        सततम्.m_hi = domain.hi;
        बहिर्गमनम्.runs.प्रक्षिप_पृष्ठभागः(std::move(सततम्));
        बहिर्गमनम्.गणनम् = गणनम्;
        प्रत्यागम सत्य;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        BinRuns बहिर्गमनम्;
        ध्रुव पूर्णाङ्क विस्तारः = सततम्_विस्तारः(exprp);
        प्रति (Ast_ग्रन्थिः* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            यदि (!checkArrayBinElement(arrayBinp, rangep)) प्रत्यागम बहिर्गमनम्;
            ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange);
            Cross_मानम्_Range range{rangep, resolve_विस्तारः(rangep, exprp)};
            तार्किक रिक्तानि = सत्य;
            यदि (!resolve_मानम्(rangep, exprp, सत्य, असत्य, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अन्यथा {
                रिक्तानि = cross_Range_रिक्तानि(range);
            }
            यदि (रिक्तानि && irp) अनुवर्त;  // A range without values contributes no bins
            Bin_सततम् सततम्{rangep, विस्तारः, 1};
            सततम्.m_रिक्तानि = रिक्तानि;
            uint64_t गणनम् = 1;
            यदि (!रिक्तानि) {
                सततम्.m_lo.op_नियोजय(range.lo);
                सततम्.m_hi.op_नियोजय(range.hi);
                V3_सङ्ख्या विस्तृतिः{rangep, विस्तारः};
                विस्तृतिः.opSub(सततम्.m_hi, सततम्.m_lo);
                // Wider spans exceed any limit
                गणनम् = विस्तृतिः.most_समुच्चयः_Bit_P1() > 32 ? UINT64_MAX : विस्तृतिः.गन्तव्यम्_UQuad() + 1;
            }
            यदि (गणनम् > v3Global.opt.coverageMaxBins() - बहिर्गमनम्.गणनम्) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_अधिकम्()
                                                << "... Suggest a larger --coverage-max-bins");
                बहिर्गमनम्.runs.clear();
                बहिर्गमनम्.गणनम् = 0;
                बहिर्गमनम्.unsupported = सत्य;
                प्रत्यागम बहिर्गमनम्;
            }
            सततम्.m_गणनम् = static_cast<uint32_t>(गणनम्);
            बहिर्गमनम्.गणनम् += सततम्.m_गणनम्;
            बहिर्गमनम्.runs.प्रक्षिप_पृष्ठभागः(std::move(सततम्));
        }
        प्रत्यागम बहिर्गमनम्;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    स्थिर BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक report) {
        BinRuns बहिर्गमनम्;
        ध्रुव पूर्णाङ्क विस्तारः = सततम्_विस्तारः(exprp);
        ध्रुव V3_सङ्ख्या one{arrayBinp, विस्तारः, 1};
        // Disjoint runs of the values, in value order, each value once
        std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> spans;
        uint64_t गणनम् = 0;  // Values of 'spans'
        प्रति (Ast_ग्रन्थिः* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            यदि (!checkArrayBinElement(arrayBinp, rangep, report)) {
                बहिर्गमनम्.unsupported = सत्य;
                प्रत्यागम बहिर्गमनम्;
            }
            Cross_मानम्_Range range{rangep, resolve_विस्तारः(rangep, exprp)};
            यदि (!resolve_मानम्(rangep, exprp, सत्य, सत्य, range)) {
                यदि (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                अनुवर्त;
            }
            यदि (cross_Range_रिक्तानि(range)) अनुवर्त;
            std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> प्राप्तम्;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), प्राप्तम्);
            प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& सततम् : प्राप्तम्) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_पृष्ठभागः(V3_सङ्ख्या{rangep, विस्तारः, सततम्.first},
                                   V3_सङ्ख्या{rangep, विस्तारः, सततम्.second});
            }
            std::क्रमय(spans.begin(), spans.अन्तः_2(), [](ध्रुव स्वचालित& lhs, ध्रुव स्वचालित& rhs) {
                प्रत्यागम cross_मानम्_Less(lhs.first, rhs.first);
            });
            std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> merged;
            प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& विस्तृतिः : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_सङ्ख्या before{rangep, विस्तारः};
                before.opSub(विस्तृतिः.first, one);
                यदि (merged.रिक्तानि() || cross_मानम्_Less(merged.पृष्ठभागः().second, before)) {
                    merged.प्रक्षिप_पृष्ठभागः(विस्तृतिः);
                } अन्यथा यदि (cross_मानम्_Less(merged.पृष्ठभागः().second, विस्तृतिः.second)) {
                    merged.पृष्ठभागः().second = विस्तृतिः.second;
                }
            }
            गणनम् = 0;
            प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& विस्तृतिः : merged) {
                V3_सङ्ख्या आकार{rangep, विस्तारः};
                आकार.opSub(विस्तृतिः.second, विस्तृतिः.first);
                // Beyond 2^32 values exceed any limit
                गणनम् += आकार.most_समुच्चयः_Bit_P1() > 32 ? uint64_t{1} << 33 : आकार.गन्तव्यम्_UQuad() + 1;
                यदि (गणनम् > v3Global.opt.coverageMaxBins()) भङ्ग;
            }
            spans = std::move(merged);
            यदि (गणनम् > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                बहिर्गमनम्.single = !arrayBinp->binsType().bin_Is_प्रसामान्यम्();
                यदि (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (बहिर्गमनम्.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_अधिकम्()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                बहिर्गमनम्.unsupported = सत्य;
                प्रत्यागम बहिर्गमनम्;
            }
        }
        प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& विस्तृतिः : spans) {
            बहिर्गमनम्.runs.emplace_पृष्ठभागः(arrayBinp, विस्तारः, 0);
            Bin_सततम्& सततम् = बहिर्गमनम्.runs.पृष्ठभागः();
            सततम्.m_lo = विस्तृतिः.first;
            सततम्.m_hi = विस्तृतिः.second;
            // A bin for each value, named by it in the coverpoint's type
            V3_सङ्ख्या मानम् = विस्तृतिः.first;
            यावत् (सत्य) {
                ध्रुव V3_सङ्ख्या typed{arrayBinp, exprp->विस्तारः(), मानम्};
                बहिर्गमनम्.मानानि.प्रक्षिप_पृष्ठभागः(exprp->isSigned() ? typed.गन्तव्यम्_Decimal_S() : typed.गन्तव्यम्_Decimal_U());
                ++सततम्.m_गणनम्;
                यदि (मानम्.isCaseEq(विस्तृतिः.second)) भङ्ग;
                V3_सङ्ख्या अग्र{arrayBinp, विस्तारः};
                मानम् = अग्र.op_सङ्कलय(मानम्, one);
            }
        }
        बहिर्गमनम्.गणनम् = static_cast<uint32_t>(गणनम्);
        प्रत्यागम बहिर्गमनम्;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    तार्किक binRunsFor(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, BinRuns& बहिर्गमनम्) {
        यदि (isAutoBins(binp)) {
            यदि (!autoBinRuns(binp, exprp, बहिर्गमनम्)) बहिर्गमनम्.unsupported = सत्य;
            प्रत्यागम सत्य;
        }
        यदि (binp->isArray() && binp->isWildcard()) {
            यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(falsep), falsep);
                बहिर्गमनम्.unsupported = सत्य;
            } अन्यथा {
                बहिर्गमनम् = wildcardBinRuns(binp, exprp, सत्य);
                यदि (बहिर्गमनम्.single) {
                    binp->isArray(असत्य);  // Generates as one bin
                    प्रत्यागम असत्य;
                }
            }
            प्रत्यागम सत्य;
        }
        यदि (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            प्रत्यागम असत्य;
        }
        बहिर्गमनम् = arrayBinRuns(binp, exprp);
        प्रत्यागम सत्य;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ग्रन्थिः_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t गणनम्, uint32_t declared,
                           ध्रुव std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& मानानि = {},
                           ध्रुव std::सदिशम्<std::string>& मानम्_नामानि = {}) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ध्रुव uint32_t प्रसामान्यम्_गणनम्
            = binp->binsType().bin_Is_प्रसामान्यम्() ? static_cast<uint32_t>(गणनम् < 0 ? 1 : गणनम्) : 0;
        ध्रुव Bin_विस्तृतिः विस्तृतिः{bins.योग, प्रसामान्यम्_गणनम्, declared};
        यदि (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = विस्तृतिः;  // Selected by bin, see implicitAutoBinSpan
        } अन्यथा {
            bins.spans.emplace(binp->name(), विस्तृतिः);
        }
        bins.योग += प्रसामान्यम्_गणनम्;
        प्रति (uint32_t i = 0; bins.crossed && i < प्रसामान्यम्_गणनम्; ++i) {
            bins.मानानि.प्रक्षिप_पृष्ठभागः({binp, मानानि.रिक्तानि() ? शून्यसूचकः : मानानि[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        ध्रुव तार्किक single = गणनम् < 0;
        यदि (!मानम्_नामानि.रिक्तानि()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ग्रन्थिः_Stmt* stmtsp = शून्यसूचकः;
            प्रति (ध्रुव std::string& मानम् : मानम्_नामानि) {
                ध्रुव std::string name
                    = VIdProtect::protect_शब्दाः_If(binp->name(), prot) + "[" + मानम् + "]";
                stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(
                    stmtsp,
                    itemCall(fl, cpVarp, VCMethod::COVERGROUP_सङ्कलय_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_समुच्चयः_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                        ->makeStmt());
            }
            प्रत्यागम stmtsp;
        }
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> तर्काः{ctext(fl, binp->binsType().bin_समुच्चयः_Enum())};
        यदि (!single) तर्काः.प्रक्षिप_पृष्ठभागः(cnum(fl, static_cast<uint32_t>(गणनम्)));  // value array bin
        तर्काः.प्रक्षिप_पृष्ठभागः(ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))));
        तर्काः.प्रक्षिप_पृष्ठभागः(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        तर्काः.प्रक्षिप_पृष्ठभागः(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        तर्काः.प्रक्षिप_पृष्ठभागः(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        प्रत्यागम itemCall(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_सङ्कलय_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_सङ्कलय_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_सङ्कलय_ARRAY_NAMER,
                        तर्काः)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    संरचना ConvBinTarget final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        तार्किक is_प्रसामान्यम्;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ग्रन्थिः_Stmt* make_Runtime_Bin_लब्धम्(File_पङ्क्तिः* fl, AstVar* cpVarp, Ast_ग्रन्थिः_व्यञ्जकम्* idxp,
                                   तार्किक is_प्रसामान्यम्) {
        प्रत्यागम itemCall(fl, cpVarp,
                        is_प्रसामान्यम् ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_लेखः_लब्धम्,
                        {idxp})
            ->makeStmt();
    }
    Ast_ग्रन्थिः_Stmt* make_Runtime_Bin_लब्धम्(File_पङ्क्तिः* fl, ध्रुव ConvBinTarget& tgt) {
        प्रत्यागम make_Runtime_Bin_लब्धम्(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_प्रसामान्यम्);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ग्रन्थिः_व्यञ्जकम्* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        यदि (binp->iffp()) condp = नूतन AstLogAnd{fl, binp->iffp()->clone_वृक्षः(असत्य), condp};
        ध्रुव स्वचालित excluded = m_excludedVars.find(cpVarp);
        यदि (excluded != m_excludedVars.अन्तः_2() && !binp->transp()
            && (binp->binsType().bin_Is_प्रसामान्यम्()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = नूतन AstLogAnd{
                fl, नूतन AstNot{fl, नूतन AstVarRef{fl, excluded->second, VAccess::READ}}, condp};
        }
        प्रत्यागम condp;
    }

    शून्य emit_Conv_लब्धम्_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ग्रन्थिः_व्यञ्जकम्* idxp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        Ast_ग्रन्थिः* actionp = make_Runtime_Bin_लब्धम्(fl, cpVarp, idxp, binp->binsType().bin_Is_प्रसामान्यम्());
        यदि (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->सङ्कलय_अग्र(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        सङ्कलय_Sample_Stmt(नूतन AstIf{fl, binCondition(binp, cpVarp, condp), actionp, शून्यसूचकः});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    शून्य emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ग्रन्थिः_व्यञ्जकम्* exprp, uint32_t sized, AstVar* matchedp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* enabledp = binCondition(binp, cpVarp, नूतन AstConst{fl, AstConst::BitTrue{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ध्रुव तार्किक illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        यदि (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ध्रुव varp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_गन्तव्यम्_Str(sized),
                             binp->findBitDType()};
            varp->funcLocal(सत्य);
            m_sampleFuncp->सङ्कलय_Stmtsp(varp);
            सङ्कलय_Sample_Stmt(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = नूतन AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ध्रुव callp
            = itemCall(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_वृक्षः(असत्य), enabledp});
        callp->dtype_समुच्चयः_Bit();
        यदि (illegal) {
            सङ्कलय_Sample_Stmt(नूतन AstIf{fl, नूतन AstLogAnd{fl, callp, enabledp->clone_वृक्षः(असत्य)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } अन्यथा यदि (matchedp && binp->binsType().bin_Is_प्रसामान्यम्()) {
            सङ्कलय_Sample_Stmt(
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, matchedp, VAccess::WRITE},
                              नूतन AstOr{fl, नूतन AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } अन्यथा {
            सङ्कलय_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_अस्थायी(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_DType* dtypep) {
        AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(सत्य);
        m_constructorp->सङ्कलय_Stmtsp(varp);
        प्रत्यागम varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* resize_मानम्(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, Ast_ग्रन्थिः_DType* dtypep) {
        File_पङ्क्तिः* ध्रुव fl = valuep->fileline();
        यदि (valuep->विस्तारः() > dtypep->विस्तारः()) {
            valuep = नूतन AstSel{fl, valuep, 0, dtypep->विस्तारः()};
        } अन्यथा यदि (valuep->विस्तारः() < dtypep->विस्तारः()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{fl, valuep, dtypep->विस्तारः()})
                         : नूतन AstExtend{fl, valuep, dtypep->विस्तारः()};
        }
        valuep->dtypep(dtypep);
        प्रत्यागम valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> new_गणनम्_तर्काः(File_पङ्क्तिः* fl, ध्रुव string& name,
                                                       Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        AstVar* ध्रुव countp = constructor_अस्थायी(fl, name, exprp->dtypep());
        m_constructorp->सङ्कलय_Stmtsp(
            नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_वृक्षः(असत्य)});
        ध्रुव स्वचालित गणनम्_Ref = [&]() { प्रत्यागम नूतन AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ध्रुव zerop = नूतन AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव positivep
            = countp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstGtS{fl, गणनम्_Ref(), zerop})
                                 : नूतन AstNeq{fl, गणनम्_Ref(), zerop};
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = resize_मानम्(गणनम्_Ref(), countp->findUInt64DType());
        यदि (countp->विस्तारः() > VL_QUADSIZE) {
            valuep = नूतन AstCond{
                fl,
                नूतन AstRedOr{
                    fl, नूतन AstSel{fl, गणनम्_Ref(), VL_QUADSIZE, countp->विस्तारः() - VL_QUADSIZE}},
                नूतन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_समुच्चयः_UInt64();
        }
        प्रत्यागम {valuep, positivep};
    }
    // The runtime arguments of a constant count
    स्थिर std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> new_गणनम्_तर्काः(File_पङ्क्तिः* fl, uint64_t गणनम्) {
        प्रत्यागम {नूतन AstConst{fl, AstConst::Unsized64{}, गणनम्},
                नूतन AstConst{fl, AstConst::BitTrue{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    स्थिर std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> coverpoint_मानानि(Ast_ग्रन्थिः* nodep,
                                                                       Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव पूर्णाङ्क विस्तारः = सततम्_विस्तारः(exprp);
        std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs;
        ध्रुव AstEnumDType* ध्रुव enump = VN_CAST(exprp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType);
        यदि (!enump) {
            ध्रुव Cross_मानम्_Range domain
                = cross_मानम्_Domain(nodep, exprp->विस्तारः(), exprp->isSigned(), विस्तारः);
            runs.emplace_पृष्ठभागः(domain.lo, domain.hi);
            प्रत्यागम runs;
        }
        std::सदिशम्<V3_सङ्ख्या> मानानि;
        प्रति (ध्रुव AstEnumItem* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ध्रुव V3_सङ्ख्या& अङ्कः = VN_AS(itemp->valuep(), Const)->अङ्कः();
            यदि (अङ्कः.isFourState()) अनुवर्त;  // Not a coverpoint value (19.5.7)
            मानानि.emplace_पृष्ठभागः(nodep, विस्तारः);
            यदि (exprp->isSigned()) {
                मानानि.पृष्ठभागः().opExtendS(अङ्कः, अङ्कः.विस्तारः());
            } अन्यथा {
                मानानि.पृष्ठभागः().op_नियोजय(अङ्कः);
            }
        }
        std::क्रमय(मानानि.begin(), मानानि.अन्तः_2(), cross_मानम्_Less);
        ध्रुव V3_सङ्ख्या one{nodep, विस्तारः, 1};
        प्रति (ध्रुव V3_सङ्ख्या& मानम् : मानानि) {
            V3_सङ्ख्या अग्र{nodep, विस्तारः};
            यदि (!runs.रिक्तानि() && अग्र.op_सङ्कलय(runs.पृष्ठभागः().second, one).isCaseEq(मानम्)) {
                runs.पृष्ठभागः().second = मानम्;
            } अन्यथा {
                runs.emplace_पृष्ठभागः(मानम्, मानम्);
            }
        }
        प्रत्यागम runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    शून्य generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        ध्रुव string उपसर्गः
            = m_sized_नामानि.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ध्रुव std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> गणनम्
            = binp->arraySizep() ? new_गणनम्_तर्काः(fl, उपसर्गः + "_count", binp->arraySizep())
                                 : new_गणनम्_तर्काः(fl, 1);
        AstCoverWith* ध्रुव withp = binWith(binp);
        यदि (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& सततम् : coverpoint_मानानि(binp, exprp)) {
                m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_मानम्_Const(fl, सततम्.first, exprp),
                                                    new_मानम्_Const(fl, सततम्.second, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        प्रति (Ast_ग्रन्थिः* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            यदि (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                अनुवर्त;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, उपसर्गः + "_" + cvt_गन्तव्यम्_Str(element++));
        }
        यदि (withp) generateWithFilter(binp, withp, cpVarp, exprp, उपसर्गः);
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> तर्काः{ctext(fl, binp->binsType().bin_समुच्चयः_Enum()), गणनम्.first,
                                       गणनम्.second};
        // A filter's bins have the limit it began with
        यदि (!withp) तर्काः.प्रक्षिप_पृष्ठभागः(cnum(fl, v3Global.opt.coverageMaxBins()));
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        तर्काः.प्रक्षिप_पृष्ठभागः(ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))));
        तर्काः.प्रक्षिप_पृष्ठभागः(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        तर्काः.प्रक्षिप_पृष्ठभागः(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        तर्काः.प्रक्षिप_पृष्ठभागः(cnum(fl, static_cast<uint32_t>(fl->firstColumn())));
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_FINISH : VCMethod::COVERGROUP_SIZED_FINISH,
                     तर्काः)
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
    शून्य generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव string& उपसर्गः) {
        File_पङ्क्तिः* ध्रुव fl = withp->fileline();
        ध्रुव तार्किक wide = exprp->isWide();
        ध्रुव string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ग्रन्थिः_DType* ध्रुव मानम्_DTypep = exprp->dtypep();
        AstVar* ध्रुव valuep = constructor_अस्थायी(fl, उपसर्गः + "_value", मानम्_DTypep);
        AstVar* ध्रुव lastp = constructor_अस्थायी(fl, उपसर्गः + "_last", मानम्_DTypep);
        AstVar* ध्रुव firstp = constructor_अस्थायी(fl, उपसर्गः + "_first", मानम्_DTypep);
        AstVar* ध्रुव runp = constructor_अस्थायी(fl, उपसर्गः + "_run", binp->findBitDType());
        AstVar* ध्रुव morep = constructor_अस्थायी(fl, उपसर्गः + "_more", binp->findBitDType());
        AstVar* ध्रुव itemp = withp->itemp()->unlink_Fr_पृष्ठभागः();
        itemp->name(उपसर्गः + "_item");
        m_constructorp->सङ्कलय_Stmtsp(itemp);
        ध्रुव स्वचालित ref = [&](AstVar* varp) { प्रत्यागम नूतन AstVarRef{fl, varp, VAccess::READ}; };
        ध्रुव स्वचालित नियोजय_2 = [&](AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) -> Ast_ग्रन्थिः* {
            प्रत्यागम नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ध्रुव स्वचालित flag = [&](AstVar* varp, तार्किक मानम्) {
            प्रत्यागम नियोजय_2(varp, मानम् ? नूतन AstConst{fl, AstConst::BitTrue{}}
                                      : नूतन AstConst{fl, AstConst::BitFalse{}});
        };
        ध्रुव स्वचालित bound = [&](VCMethod narrow, VCMethod wide_विधानम्, AstVar* varp) -> Ast_ग्रन्थिः* {
            यदि (wide) {
                प्रत्यागम itemCall(fl, cpVarp, wide_विधानम्, {नूतन AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ध्रुव callp = itemCall(fl, cpVarp, narrow);
            callp->dtype_समुच्चयः_UInt64();
            प्रत्यागम नियोजय_2(varp, resize_मानम्(callp, मानम्_DTypep));
        };
        ध्रुव स्वचालित keep = [&](Ast_ग्रन्थिः_व्यञ्जकम्* lop, Ast_ग्रन्थिः_व्यञ्जकम्* hip) {
            AstCMethodHard* ध्रुव callp = itemCall(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_सततम्_W : VCMethod::COVERGROUP_WITH_सततम्,
                {lop, hip});
            callp->dtype_समुच्चयः_Bit();
            प्रत्यागम नियोजय_2(morep, callp);
        };
        ध्रुव स्वचालित चरणः = [&](तार्किक ऊर्ध्वम्) {
            AstConst* ध्रुव onep = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, exprp->विस्तारः(), 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव stepp
                = ऊर्ध्वम् ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन Ast_सङ्कलय{fl, ref(valuep), onep})
                     : नूतन AstSub{fl, ref(valuep), onep};
            stepp->dtypep(मानम्_DTypep);
            प्रत्यागम stepp;
        };
        AstLoop* ध्रुव innerp = नूतन AstLoop{fl};
        innerp->सङ्कलय_Stmtsp(नियोजय_2(itemp, ref(valuep)));
        innerp->सङ्कलय_Stmtsp(नूतन AstIf{
            fl, withp->filterp()->unlink_Fr_पृष्ठभागः(),
            नूतन AstIf{fl, नूतन AstNot{fl, ref(runp)},
                      नियोजय_2(firstp, ref(valuep))->सङ्कलय_अग्र(flag(runp, सत्य))},
            नूतन AstIf{fl, ref(runp), keep(ref(firstp), चरणः(असत्य))->सङ्कलय_अग्र(flag(runp, असत्य))}});
        innerp->सङ्कलय_Stmtsp(नूतन AstLoopTest{
            fl, innerp, नूतन AstLogAnd{fl, ref(morep), नूतन AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->सङ्कलय_Stmtsp(नियोजय_2(valuep, चरणः(सत्य)));
        AstLoop* ध्रुव outerp = नूतन AstLoop{fl};
        AstCMethodHard* ध्रुव nextp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_WITH_अग्र);
        nextp->dtype_समुच्चयः_Bit();
        outerp->सङ्कलय_Stmtsp(नूतन AstLoopTest{fl, outerp, nextp});
        outerp->सङ्कलय_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->सङ्कलय_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->सङ्कलय_Stmtsp(flag(runp, असत्य));
        outerp->सङ्कलय_Stmtsp(innerp);
        outerp->सङ्कलय_Stmtsp(नूतन AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->सङ्कलय_Stmtsp(flag(morep, सत्य));
        m_constructorp->सङ्कलय_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    शून्य generateSizedElement(AstVar* cpVarp, ध्रुव AstCoverBin* binp, Ast_ग्रन्थिः* rangep,
                              Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव string& उपसर्गः) {
        File_पङ्क्तिः* ध्रुव fl = rangep->fileline();
        ध्रुव VCMethod विधानम् = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lowp = irp ? irp->lhsp() : VN_AS(rangep, ग्रन्थिः_व्यञ्जकम्);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव highp = irp ? irp->rhsp() : शून्यसूचकः;
        ध्रुव स्वचालित unbounded
            = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) { प्रत्यागम !boundp || VN_IS(boundp, Unbounded); };
        ध्रुव स्वचालित constant
            = [&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) { प्रत्यागम unbounded(boundp) || VN_IS(boundp, Const); };
        ध्रुव स्वचालित integral = [&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) {
            प्रत्यागम unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        यदि (constant(lowp) && constant(highp)) {
            ध्रुव स्वचालित fourState = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) {
                ध्रुव AstConst* ध्रुव constp = VN_CAST(boundp, Const);
                प्रत्यागम constp && constp->अङ्कः().isFourState();
            };
            यदि (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                प्रत्यागम;
            }
            Cross_मानम्_Range range{rangep, resolve_विस्तारः(rangep, exprp)};
            यदि (!resolve_मानम्(rangep, exprp, सत्य, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } अन्यथा यदि (!cross_Range_रिक्तानि(range)) {
                std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>> runs{{range.lo, range.hi}};
                यदि (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                प्रति (ध्रुव std::pair<V3_सङ्ख्या, V3_सङ्ख्या>& सततम् : runs) {
                    m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, cpVarp, विधानम्,
                                                       {new_मानम्_Const(fl, सततम्.first, exprp),
                                                        new_मानम्_Const(fl, सततम्.second, exprp)})
                                                  ->makeStmt());
                }
            }
            प्रत्यागम;
        }
        यदि (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            प्रत्यागम;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ध्रुव स्वचालित bound_विस्तारः
            = [&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* boundp) { प्रत्यागम unbounded(boundp) ? 0 : boundp->विस्तारः(); };
        ध्रुव पूर्णाङ्क विस्तारः = std::max({exprp->विस्तारः(), bound_विस्तारः(lowp), bound_विस्तारः(highp)}) + 1;
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(rangep, exprp->विस्तारः(), exprp->isSigned(), विस्तारः);
        AstVar* ध्रुव lop = constructor_अस्थायी(fl, उपसर्गः + "_lo", exprp->dtypep());
        lop->dtype_समुच्चयः_Bit_Sized(विस्तारः, VSigning::SIGNED);
        AstVar* ध्रुव hip = constructor_अस्थायी(fl, उपसर्गः + "_hi", lop->dtypep());
        ध्रुव स्वचालित bound = [&](Ast_ग्रन्थिः_व्यञ्जकम्* boundp, ध्रुव V3_सङ्ख्या& सीमा) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            यदि (unbounded(boundp)) {
                AstConst* ध्रुव limitp = नूतन AstConst{fl, सीमा};
                limitp->dtypeFrom(lop);
                प्रत्यागम limitp;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = boundp->clone_वृक्षः(असत्य);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            यदि (exprp->isSigned() && boundp->विस्तारः() <= exprp->विस्तारः()) {
                valuep = resize_मानम्(valuep, exprp->dtypep());
            }
            प्रत्यागम resize_मानम्(valuep, lop->dtypep());
        };
        ध्रुव स्वचालित ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            प्रत्यागम नूतन AstVarRef{fl, varp, access};
        };
        m_constructorp->सङ्कलय_Stmtsp(
            नूतन Ast_नियोजय{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->सङ्कलय_Stmtsp(
            नूतन Ast_नियोजय{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ध्रुव minp = नूतन AstConst{fl, domain.lo};
        AstConst* ध्रुव maxp = नूतन AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ध्रुव lowerp
            = नूतन AstCond{fl, नूतन AstLtS{fl, ref(lop), minp}, minp->clone_वृक्षः(असत्य), ref(lop)};
        AstCond* ध्रुव upperp
            = नूतन AstCond{fl, नूतन AstGtS{fl, ref(hip), maxp}, maxp->clone_वृक्षः(असत्य), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->सङ्कलय_Stmtsp(नूतन AstIf{fl, नूतन AstLteS{fl, ref(lop), ref(hip)},
                                            itemCall(fl, cpVarp, विधानम्,
                                                     {resize_मानम्(ref(lop), exprp->dtypep()),
                                                      resize_मानम्(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* सततम्_Bin_सूचकांक(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव Bin_सततम्& सततम्) {
        यदि (सततम्.m_गणनम् == 1) प्रत्यागम cnum(fl, सततम्.m_declared);
        ध्रुव पूर्णाङ्क विस्तारः = exprp->विस्तारः();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ग्रन्थिः_व्यञ्जकम्* indexp
            = नूतन AstSub{fl, exprp->clone_वृक्षः(असत्य), new_मानम्_Const(fl, सततम्.m_lo, exprp)};
        indexp->dtype_समुच्चयः_Bit_Sized(विस्तारः, VSigning::UNSIGNED);
        V3_सङ्ख्या stride{fl, विस्तारः, 0};
        stride.op_नियोजय(सततम्.m_stride);
        यदि (stride.गणनम्_Ones() != 1) {
            indexp = नूतन AstDiv{fl, indexp, नूतन AstConst{fl, stride}};
        } अन्यथा यदि (!stride.isEqOne()) {
            indexp = नूतन Ast_सरणम्_R{fl, indexp, नूतन AstConst{fl, stride.most_समुच्चयः_Bit_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ध्रुव पूर्णाङ्क ext_विस्तारः = सततम्.m_lo.विस्तारः() + 1;
        V3_सङ्ख्या lo{fl, ext_विस्तारः, 0};
        lo.opExtendS(सततम्.m_lo, सततम्.m_lo.विस्तारः());
        V3_सङ्ख्या विस्तृतिः{fl, ext_विस्तारः, 0};
        विस्तृतिः.opExtendS(सततम्.m_hi, सततम्.m_hi.विस्तारः());
        विस्तृतिः.opSub(V3_सङ्ख्या{विस्तृतिः}, lo);
        V3_सङ्ख्या covered{fl, ext_विस्तारः, 0};
        covered.op_नियोजय(सततम्.m_stride);
        covered.opMul(V3_सङ्ख्या{covered}, V3_सङ्ख्या{fl, ext_विस्तारः, सततम्.m_गणनम्});
        V3_सङ्ख्या hasRemainder{fl, 1, 0};
        यदि (!hasRemainder.opGte(विस्तृतिः, covered).isEqZero()) {
            AstConst* ध्रुव lastp = नूतन AstConst{fl, V3_सङ्ख्या{fl, विस्तारः, सततम्.m_गणनम् - 1}};
            indexp = नूतन AstCond{fl, नूतन AstGt{fl, indexp->clone_वृक्षः(असत्य), lastp},
                                 lastp->clone_वृक्षः(असत्य), indexp};
        }
        यदि (विस्तारः < VL_IDATASIZE) {
            indexp = नूतन AstExtend{fl, indexp, VL_IDATASIZE};
        } अन्यथा यदि (विस्तारः > VL_IDATASIZE) {
            indexp = नूतन AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        प्रत्यागम नूतन Ast_सङ्कलय{fl, cnum(fl, सततम्.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    शून्य emit_सततम्_लब्धम्(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव Bin_सततम्& सततम्) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        AstConst* ध्रुव lop = new_मानम्_Const(fl, सततम्.m_lo, exprp);
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
        यदि (सततम्.m_lo.isCaseEq(सततम्.m_hi)) {
            condp = नूतन AstEq{fl, exprp->clone_वृक्षः(असत्य), lop};
        } अन्यथा {
            AstConst* ध्रुव hip = new_मानम्_Const(fl, सततम्.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lop), lop);
            VL_DO_DANGLING(प्रक्षिप_Deletep(hip), hip);
        }
        emit_Conv_लब्धम्_If(coverpointp, binp, cpVarp, सततम्_Bin_सूचकांक(fl, exprp, सततम्), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    शून्य सङ्कलय_Conv_Trans_लब्धम्_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ध्रुव ConvBinTarget& tgt,
                           Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();
        Ast_ग्रन्थिः* actionp = make_Runtime_Bin_लब्धम्(fl, tgt);
        यदि (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->सङ्कलय_अग्र(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        सङ्कलय_Sample_Stmt(नूतन AstIf{fl, condp, actionp, शून्यसूचकः});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    शून्य generateCoverpoint(AstCoverpoint* coverpointp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क at_न्यूनतमम्_मानम्) {
        File_पङ्क्तिः* ध्रुव fl = coverpointp->fileline();
        ध्रुव तार्किक dynamic = m_runtimePoints.गणनम्(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ध्रुव तार्किक crossFed = m_crossed_Cp_नामानि.गणनम्(coverpointp->name()) != 0;
        ध्रुव पूर्णाङ्क लब्धम्_Bound = compute_लब्धम्_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << लब्धम्_Bound);
        AstVar* ध्रुव cpVarp = नूतन AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(लब्धम्_Bound))};
        m_covergroupp->सङ्कलय_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->सङ्कलय_Stmtsp(makeItemCreate(fl, cpVarp, VCMethod::COVERGROUP_सङ्कलय_COVERPOINT));
        generateItemWeight(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        यदि (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->सङ्कलय_Stmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_लब्धम्_LIST)->makeStmt());
        }
        यदि (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ध्रुव excludedp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->findBitDType()};
            excludedp->funcLocal(सत्य);
            m_sampleFuncp->सङ्कलय_Stmtsp(excludedp);
            AstCMethodHard* ध्रुव callp
                = itemCall(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_मानम्_EXCLUDED_W
                                           : VCMethod::COVERGROUP_मानम्_EXCLUDED,
                           {exprp->clone_वृक्षः(असत्य)});
            callp->dtype_समुच्चयः_Bit();
            सङ्कलय_Sample_Stmt(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::सदिशम्<Ast_ग्रन्थिः_Stmt*> namerStmts;
        std::सदिशम्<AstCoverBin*> defaultBins;
        std::सदिशम्<AstCoverBin*> sizedBins;
        std::सदिशम्<std::tuple<AstCoverBin*, uint32_t, Ast_ग्रन्थिः_व्यञ्जकम्*>> metadata;
        std::सदिशम्<ध्रुव Bin_सततम्*> सततम्_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        प्रति (Ast_ग्रन्थिः* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ध्रुव cbinp = VN_AS(binp, CoverBin);
            ध्रुव पूर्णाङ्क errorsBefore = dynamic ? V3Error::error_गणनम्() : 0;
            यदि (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.प्रक्षिप_पृष्ठभागः(cbinp);
                अनुवर्त;
            }
            यदि (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_विस्तृतिः विस्तृतिः;
                विस्तृतिः.sized = static_cast<int32_t>(sizedBins.आकार());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), विस्तृतिः);
                sizedBins.प्रक्षिप_पृष्ठभागः(cbinp);
                अनुवर्त;
            }
            यदि (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.प्रक्षिप_पृष्ठभागः(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ध्रुव ConvBinTarget tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_प्रसामान्यम्()};
                प्रति (Ast_ग्रन्थिः* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_संहिता(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_समुच्चयः));
                यदि (dynamic && V3Error::error_गणनम्() == errorsBefore) {
                    metadata.emplace_पृष्ठभागः(cbinp, static_cast<uint32_t>(idx), शून्यसूचकः);
                }
                ++idx;
                अनुवर्त;
            }
            BinRuns plan;
            यदि (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                यदि (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].प्रक्षिप_पृष्ठभागः(cbinp->name());
                    अनुवर्त;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ध्रुव uint32_t first_मानम् = bins.योग;
                ध्रुव uint32_t firstDeclared = static_cast<uint32_t>(idx);
                namerStmts.प्रक्षिप_पृष्ठभागः(
                    makeNamer(cpVarp, cbinp, plan.गणनम्, firstDeclared, {}, plan.मानानि));
                प्रति (Bin_सततम्& सततम् : plan.runs) {
                    सततम्.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.प्रक्षिप_पृष्ठभागः(std::move(सततम्));
                    ध्रुव Bin_सततम्& stored = bins.runs.पृष्ठभागः();
                    यदि (bins.crossed && cbinp->binsType().bin_Is_प्रसामान्यम्()) {
                        ध्रुव uint32_t first = first_मानम् + stored.m_declared - firstDeclared;
                        प्रति (uint32_t element = 0; element < stored.m_गणनम्; ++element) {
                            bins.मानानि[first + element].runp = &stored;
                            bins.मानानि[first + element].element = element;
                        }
                    }
                    यदि (!stored.m_रिक्तानि) {
                        emit_सततम्_लब्धम्(coverpointp, cbinp, cpVarp, exprp, stored);
                        यदि (dynamic && V3Error::error_गणनम्() == errorsBefore) {
                            सततम्_Metadata.प्रक्षिप_पृष्ठभागः(&stored);
                        }
                    }
                    idx += stored.m_गणनम्;
                }
                अनुवर्त;
            }
            यदि (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                तार्किक unsupported = असत्य;
                std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> मानानि = extract_Array_मानानि(cbinp, exprp, unsupported);
                यदि (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].प्रक्षिप_पृष्ठभागः(cbinp->name());
                    अनुवर्त;
                }
                namerStmts.प्रक्षिप_पृष्ठभागः(makeNamer(cpVarp, cbinp, static_cast<int64_t>(मानानि.आकार()),
                                               static_cast<uint32_t>(idx), मानानि));
                प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* valuep : मानानि) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_मानानि.प्रक्षिप_पृष्ठभागः(valuep);
                    emit_Conv_लब्धम्_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_मानम्_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } अन्यथा {
                namerStmts.प्रक्षिप_पृष्ठभागः(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_लब्धम्_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                यदि (dynamic && V3Error::error_गणनम्() == errorsBefore) {
                    metadata.emplace_पृष्ठभागः(cbinp, static_cast<uint32_t>(idx), शून्यसूचकः);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        प्रति (ध्रुव std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_विस्तृतिः{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = शून्यसूचकः;
        यदि (!defaultBins.रिक्तानि()
            && std::any_सम्बन्धि(sizedBins.begin(), sizedBins.अन्तः_2(), [](ध्रुव AstCoverBin* binp) {
                   प्रत्यागम binp->binsType().bin_Is_प्रसामान्यम्();
               })) {
            sizedMatchedp = नूतन AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->findBitDType()};
            sizedMatchedp->funcLocal(सत्य);
            m_sampleFuncp->सङ्कलय_Stmtsp(sizedMatchedp);
            सङ्कलय_Sample_Stmt(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        नूतन AstConst{fl, AstConst::BitFalse{}}});
        }
        प्रति (uint32_t sized = 0; sized < sizedBins.आकार(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        प्रति (AstCoverBin* ध्रुव defBinp : defaultBins) {
            File_पङ्क्तिः* ध्रुव dfl = defBinp->fileline();
            namerStmts.प्रक्षिप_पृष्ठभागः(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            यदि (sizedMatchedp) {
                condp = नूतन AstLogAnd{
                    dfl, नूतन AstNot{dfl, नूतन AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_लब्धम्_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        यदि (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        यदि (coverpointHasTransition(coverpointp)) {
            AstVar* ध्रुव पूर्व_Varp = VN_AS(coverpointp->user1p(), Var);
            सङ्कलय_Sample_Stmt(
                नूतन Ast_नियोजय{coverpointp->fileline(),
                              नूतन AstVarRef{पूर्व_Varp->fileline(), पूर्व_Varp, VAccess::WRITE},
                              exprp->clone_वृक्षः(असत्य)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        ध्रुव std::string hier
            = VIdProtect::protect_शब्दाः_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_न्यूनतमम्_मानम्)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव ns : namerStmts) m_constructorp->सङ्कलय_Stmtsp(ns);
        यदि (dynamic) {
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मानम्_TYPE,
                         {cnum(fl, exprp->विस्तारः()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            मानम्_Lists lists;
            प्रति (ध्रुव स्वचालित& प्रविष्टिः : metadata) {
                collect_मानम्_Metadata(lists, exprp, std::get<0>(प्रविष्टिः), std::get<1>(प्रविष्टिः),
                                     std::get<2>(प्रविष्टिः));
            }
            प्रति (ध्रुव Bin_सततम्* ध्रुव runp : सततम्_Metadata) collect_सततम्_Metadata(lists, exprp, *runp);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_RANGES, lists.m_ranges);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_RUNS, lists.m_runs);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_PATTERNS, lists.m_patterns);
            emit_मानम्_List(fl, cpVarp, VCMethod::COVERGROUP_मानम्_TRANSITIONS, lists.m_transitions);
            प्रति (AstCoverBin* ध्रुव binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_मानम्_FINALIZE)->makeStmt());
        }
        यदि (v3Global.opt.coverage()) {
            ध्रुव std::string page
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cpVarp, VCMethod::COVERGROUP_पञ्जिकाकोशः_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(page)),
                          cnum(fl, itemDatabaseWeight(coverpointp->optionsp())),
                          cnum(fl, m_cgTypeWeight)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    शून्य generate_Multi_मानम्_Transition_संहिता(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव ConvBinTarget& tgt,
                                          ध्रुव std::सदिशम्<AstCoverTransItem*>& वस्तूनि) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << वस्तूनि.आकार() << " items");

        // Create state position variable
        AstVar* ध्रुव stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ध्रुव casep
            = नूतन AstCase{binp->fileline(), VCaseType::CT_CASE,
                          नूतन AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, शून्यसूचकः};

        // Generate each case item in the switch statement
        प्रति (आकार_t state = 0; state < वस्तूनि.आकार(); ++state) {
            AstCaseItem* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, वस्तूनि, state);
            casep->सङ्कलय_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        AstCaseItem* ध्रुव defaultItemp = नूतन AstCaseItem{
            binp->fileline(), शून्यसूचकः,
            नूतन Ast_नियोजय{binp->fileline(),
                          नूतन AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          नूतन AstConst{binp->fileline(), AstConst::Widthed_मानम्{}, 8, 0}}};
        casep->सङ्कलय_Itemsp(defaultItemp);

        सङ्कलय_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    AstCaseItem* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव ConvBinTarget& tgt,
                                             AstVar* stateVarp,
                                             ध्रुव std::सदिशम्<AstCoverTransItem*>& वस्तूनि,
                                             आकार_t state) {
        File_पङ्क्तिः* ध्रुव fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_Condp = buildTransitionItemCondition(वस्तूनि[state], exprp);

        Ast_ग्रन्थिः_Stmt* मेलनम्_Actionp = शून्यसूचकः;

        यदि (state == वस्तूनि.आकार() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            मेलनम्_Actionp = make_Runtime_Bin_लब्धम्(fl, tgt);

            // For illegal_bins, add error message
            यदि (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ध्रुव string err_सन्देशः = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                मेलनम्_Actionp = मेलनम्_Actionp->सङ्कलय_अग्र(makeIllegalBinAction(fl, err_सन्देशः));
            }

            // Reset state to 0
            मेलनम्_Actionp = मेलनम्_Actionp->सङ्कलय_अग्र(
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, 0}});
        } अन्यथा {
            // Intermediate state: advance to next state
            मेलनम्_Actionp = नूतन Ast_नियोजय{
                fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ग्रन्थिः_Stmt* no_मेलनम्_Actionp = शून्यसूचकः;
        यदि (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव restartCondp = buildTransitionItemCondition(वस्तूनि[0], exprp);

            UASSERT_OBJ(restartCondp, वस्तूनि[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ग्रन्थिः_Stmt* restartActionp
                = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ग्रन्थिः_Stmt* resetActionp
                = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, stateVarp, VAccess::WRITE},
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, 0}};

            no_मेलनम्_Actionp = नूतन AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = नूतन AstIf{fl, मेलनम्_Condp, मेलनम्_Actionp, no_मेलनम्_Actionp};

        // Create case item for this state value
        AstCaseItem* ध्रुव caseItemp = नूतन AstCaseItem{
            fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        प्रत्यागम caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ग्रन्थिः_Stmt* makeIllegalBinAction(File_पङ्क्तिः* fl, ध्रुव string& err_सन्देशः) {
        AstDisplay* ध्रुव errorp
            = नूतन AstDisplay{fl, VDisplayType::DT_ERROR, err_सन्देशः, शून्यसूचकः, शून्यसूचकः};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ग्रन्थिः*>(errorp)->सङ्कलय_अग्र(नूतन AstStop{fl, सत्य});
        प्रत्यागम errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    स्थिर AstConst* new_मानम्_Const(File_पङ्क्तिः* fl, ध्रुव V3_सङ्ख्या& मानम्, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        V3_सङ्ख्या narrowed{fl, exprp->विस्तारः(), 0};
        narrowed.op_नियोजय(मानम्);
        AstConst* ध्रुव constp = नूतन AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        प्रत्यागम constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    स्थिर AstConst* newRealConst(AstConst* constp) {
        यदि (constp->अङ्कः().isDouble()) प्रत्यागम constp->clone_वृक्षः(असत्य);
        V3_सङ्ख्या real{&constp->अङ्कः(), 64};
        real.opIToRD(constp->अङ्कः(), constp->isSigned());
        प्रत्यागम नूतन AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    स्थिर AstConst* widenConst(File_पङ्क्तिः* fl, AstConst* constp, पूर्णाङ्क target_विस्तारः) {
        यदि (constp->विस्तारः() == target_विस्तारः) प्रत्यागम constp->clone_वृक्षः(असत्य);
        V3_सङ्ख्या अङ्कः{fl, target_विस्तारः, 0};
        अङ्कः.op_नियोजय(constp->अङ्कः());
        प्रत्यागम नूतन AstConst{fl, अङ्कः};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ग्रन्थिः_व्यञ्जकम्* makeRangeCondition(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* minp,
                                    Ast_ग्रन्थिः_व्यञ्जकम्* maxp) {
        ध्रुव पूर्णाङ्क व्यञ्जकम्_विस्तारः = exprp->विस्तारः_Min();
        AstConst* ध्रुव minConstp = VN_AS(minp, Const);
        AstConst* ध्रुव maxConstp = VN_AS(maxp, Const);
        यदि (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            प्रत्यागम नूतन AstAnd{fl,
                              नूतन AstGteD{fl, exprp->clone_वृक्षः(असत्य), newRealConst(minConstp)},
                              नूतन AstLteD{fl, exprp->clone_वृक्षः(असत्य), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ध्रुव minWidep = widenConst(fl, minConstp, व्यञ्जकम्_विस्तारः);
        AstConst* ध्रुव maxWidep = widenConst(fl, maxConstp, व्यञ्जकम्_विस्तारः);
        V3_सङ्ख्या minimum{fl, व्यञ्जकम्_विस्तारः, 0};
        V3_सङ्ख्या अधिकतमम्{fl, व्यञ्जकम्_विस्तारः, 0};
        अधिकतमम्.समुच्चयः_All_Bits1();
        यदि (exprp->isSigned()) {
            minimum.समुच्चयः_Bit(व्यञ्जकम्_विस्तारः - 1, 1);
            अधिकतमम्.समुच्चयः_Bit(व्यञ्जकम्_विस्तारः - 1, 0);
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* lowerp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* upperp = शून्यसूचकः;
        यदि (minWidep->अङ्कः().isCaseEq(minimum)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(minWidep), minWidep);
        } अन्यथा {
            lowerp = exprp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstGteS{fl, exprp->clone_वृक्षः(असत्य), minWidep})
                                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstGte{fl, exprp->clone_वृक्षः(असत्य), minWidep});
        }
        यदि (maxWidep->अङ्कः().isCaseEq(अधिकतमम्)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(maxWidep), maxWidep);
        } अन्यथा {
            upperp = exprp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstLteS{fl, exprp->clone_वृक्षः(असत्य), maxWidep})
                                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                             नूतन AstLte{fl, exprp->clone_वृक्षः(असत्य), maxWidep});
        }
        यदि (lowerp && upperp) प्रत्यागम नूतन AstAnd{fl, lowerp, upperp};
        यदि (lowerp) प्रत्यागम lowerp;
        यदि (upperp) प्रत्यागम upperp;
        प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ग्रन्थिः_व्यञ्जकम्* makeOpenRangeCondition(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstConst* boundp,
                                        तार्किक is_अधस्तनम्_Bound) {
        AstConst* ध्रुव widep = widenConst(fl, boundp, exprp->विस्तारः_Min());
        यदि (is_अधस्तनम्_Bound) {
            यदि (exprp->isSigned()) प्रत्यागम नूतन AstGteS{fl, exprp->clone_वृक्षः(असत्य), widep};
            प्रत्यागम नूतन AstGte{fl, exprp->clone_वृक्षः(असत्य), widep};
        }
        यदि (exprp->isSigned()) प्रत्यागम नूतन AstLteS{fl, exprp->clone_वृक्षः(असत्य), widep};
        प्रत्यागम नूतन AstLte{fl, exprp->clone_वृक्षः(असत्य), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ग्रन्थिः_व्यञ्जकम्* buildTransitionItemCondition(AstCoverTransItem* itemp, AstVar* varp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* varRefp = नूतन AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = buildTransitionItemCondition(itemp, varRefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(varRefp), varRefp);
        प्रत्यागम condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ग्रन्थिः_व्यञ्जकम्* buildTransitionItemCondition(AstCoverTransItem* itemp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;

        प्रति (Ast_ग्रन्थिः* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* singleCondp = शून्यसूचकः;
            valp = V3Const::constifyEdit(valp);
            AstConst* ध्रुव constp = VN_CAST(valp, Const);
            यदि (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                प्रत्यागम नूतन AstConst{valp->fileline(), AstConst::BitFalseErroring{}};
            }
            singleCondp
                = नूतन AstEq{constp->fileline(), exprp->clone_वृक्षः(असत्य), constp->clone_वृक्षः(असत्य)};
            यदि (condp) {
                condp = नूतन AstOr{itemp->fileline(), condp, singleCondp};
            } अन्यथा {
                condp = singleCondp;
            }
        }

        प्रत्यागम condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    शून्य generate_Single_Transition_संहिता(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव ConvBinTarget& tgt,
                                      Ast_Cover_Trans_समुच्चयः* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ध्रुव पूर्व_Varp = create_पूर्व_मानम्_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::सदिशम्<AstCoverTransItem*> वस्तूनि;
        प्रति (Ast_ग्रन्थिः* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            वस्तूनि.प्रक्षिप_पृष्ठभागः(VN_AS(itemp, CoverTransItem));

        यदि (वस्तूनि.रिक्तानि()) {
            binp->v3error("Transition set without items");
            प्रत्यागम;
        }

        यदि (वस्तूनि.आकार() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            प्रत्यागम;
        } अन्यथा यदि (वस्तूनि.आकार() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cond1p = buildTransitionItemCondition(वस्तूनि[0], पूर्व_Varp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cond2p = buildTransitionItemCondition(वस्तूनि[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ग्रन्थिः_व्यञ्जकम्* पूर्णम्_Condp = नूतन AstAnd{binp->fileline(), cond1p, cond2p};

            सङ्कलय_Conv_Trans_लब्धम्_If(coverpointp, binp, tgt, पूर्णम्_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } अन्यथा {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_मानम्_Transition_संहिता(coverpointp, binp, exprp, tgt, वस्तूनि);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_पङ्क्तिः* fl, ध्रुव std::सदिशम्<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ध्रुव cs = नूतन AstCStmt{fl};
        cs->सङ्कलय("{ VlCoverpoint* __Vcx_cps[] = {");
        प्रति (आकार_t d = 0; d < cpVars.आकार(); ++d) {
            यदि (d != 0) cs->सङ्कलय(", ");
            cs->सङ्कलय(memberRef(fl, cpVars[d]));
        }
        cs->सङ्कलय("}; ");
        cs->सङ्कलय(callp);
        cs->सङ्कलय("; }");
        प्रत्यागम cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_पङ्क्तिः* fl, ध्रुव std::सदिशम्<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ध्रुव cs = नूतन AstCStmt{fl};
        cs->सङ्कलय("{ bool __Vcx_iffs[" + cvt_गन्तव्यम्_Str(bins.आकार()) + "]; ");
        प्रति (आकार_t i = 0; i < bins.आकार(); ++i) {
            ध्रुव AstCoverCrossBin* ध्रुव binp = bins[i];
            cs->सङ्कलय("__Vcx_iffs[" + cvt_गन्तव्यम्_Str(i) + "] = ");
            cs->सङ्कलय(binp->iffp() ? binp->iffp()->clone_वृक्षः(असत्य)
                                 : नूतन AstConst{fl, AstConst::BitTrue{}});
            cs->सङ्कलय("; ");
        }
        cs->सङ्कलय(callp);
        cs->सङ्कलय("; }");
        प्रत्यागम cs;
    }

    उपयुज्य CrossSelection = std::सदिशम्<uint64_t>;
    संरचना ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    संरचना CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_शब्दाः = 0;
        तार्किक valid = सत्य;
        std::सदिशम्<ResolvedCrossBin> bins;
    };
    संरचना CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ध्रुव std::सदिशम्<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ध्रुव std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::सदिशम्<uint32_t> strides;  // Flat-index stride per dimension
        तार्किक valid = सत्य;  // False if this explicit bin cannot be implemented
    };
    संरचना CrossBinsofTarget final {
        ध्रुव CoverpointBins* m_binsp = शून्यसूचकः;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_first = 0;  // First declared normal bin in the selected span
        uint32_t m_गणनम् = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declaredFirst = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_अन्तः = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    संरचना Cross_मानम्_Range final {
        V3_सङ्ख्या lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_सङ्ख्या hi;  // Inclusive upper bound
        V3_सङ्ख्या आदर्शः_2;  // Allowed bit values; all X for an ordinary interval
        तार्किक wildcard = असत्य;  // A wildcard singleton rather than an exact four-state value

        Cross_मानम्_Range(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः)
            : lo{nodep, विस्तारः}
            , hi{nodep, विस्तारः}
            , आदर्शः_2{nodep, विस्तारः} {
            आदर्शः_2.समुच्चयः_All_द्वयङ्काः_X();
        }
    };

    स्थिर std::सदिशम्<Ast_ग्रन्थिः*> cross_Bin_मानानि(ध्रुव Cross_Bin_मानानि& bin) {
        यदि (bin.valuep) प्रत्यागम {bin.valuep};
        std::सदिशम्<Ast_ग्रन्थिः*> मानानि;
        यदि (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            प्रति (Ast_ग्रन्थिः* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                AstCoverTransItem* lastp = VN_AS(setp, Cover_Trans_समुच्चयः)->itemsp();
                यावत् (lastp->nextp()) lastp = VN_AS(lastp->nextp(), CoverTransItem);
                प्रति (Ast_ग्रन्थिः* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    मानानि.प्रक्षिप_पृष्ठभागः(valuep);
                }
            }
        } अन्यथा {
            प्रति (Ast_ग्रन्थिः* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                मानानि.प्रक्षिप_पृष्ठभागः(valuep);
            }
        }
        प्रत्यागम मानानि;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    स्थिर Cross_मानम्_Range सततम्_Bin_Range(Ast_ग्रन्थिः* nodep, ध्रुव Bin_सततम्& सततम्, uint32_t element,
                                       पूर्णाङ्क विस्तारः) {
        ध्रुव पूर्णाङ्क runw = सततम्.m_lo.विस्तारः();
        V3_सङ्ख्या विस्थापनम्{nodep, runw};
        विस्थापनम्.opMul(सततम्.m_stride, V3_सङ्ख्या{nodep, runw, element});
        V3_सङ्ख्या lo{nodep, runw};
        lo.op_सङ्कलय(सततम्.m_lo, विस्थापनम्);
        V3_सङ्ख्या hi = सततम्.m_hi;
        यदि (element + 1 < सततम्.m_गणनम्) {
            विस्थापनम्.opSub(सततम्.m_stride, V3_सङ्ख्या{nodep, runw, 1});
            hi.op_सङ्कलय(lo, विस्थापनम्);
        }
        Cross_मानम्_Range range{nodep, विस्तारः};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        प्रत्यागम range;
    }

    स्थिर पूर्णाङ्क cross_Range_विस्तारः(Ast_ग्रन्थिः* nodep) {
        यदि (ध्रुव AstInsideRange* ध्रुव rangep = VN_CAST(nodep, InsideRange)) {
            प्रत्यागम std::max(rangep->lhsp()->विस्तारः(), rangep->rhsp()->विस्तारः());
        }
        प्रत्यागम nodep->विस्तारः();
    }

    स्थिर तार्किक cross_मानम्_Less(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
        V3_सङ्ख्या परिणाम{&lhs};
        प्रत्यागम !परिणाम.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    स्थिर शून्य crossRealBound(ध्रुव AstConst* constp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक ऊर्ध्वस्थम्,
                               ध्रुव Cross_मानम्_Range& domain, V3_सङ्ख्या& परिणाम, तार्किक& रिक्तानि) {
        ध्रुव द्विगुण मानम् = constp->अङ्कः().गन्तव्यम्_Double();
        ध्रुव द्विगुण bound = ऊर्ध्वस्थम् ? std::floor(मानम्) : std::ceil(मानम्);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ध्रुव द्विगुण सीमा
            = std::ldexp(1.0, exprp->isSigned() ? exprp->विस्तारः() - 1 : exprp->विस्तारः());
        ध्रुव द्विगुण minimum = exprp->isSigned() ? -सीमा : 0.0;
        यदि (std::isnan(bound) || (ऊर्ध्वस्थम् ? bound < minimum : bound >= सीमा)) {
            रिक्तानि = सत्य;
            परिणाम = domain.lo;
        } अन्यथा यदि (bound < minimum) {
            परिणाम = domain.lo;
        } अन्यथा यदि (bound >= सीमा) {
            परिणाम = domain.hi;
        } अन्यथा {
            V3_सङ्ख्या real{&परिणाम, 64};
            real.समुच्चयः_Double(bound);
            परिणाम.opRToIRoundS(real);
        }
    }

    स्थिर तार्किक crossRangeBound(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक ऊर्ध्वस्थम्, तार्किक bin_मानम्,
                                ध्रुव Cross_मानम्_Range& domain, V3_सङ्ख्या& परिणाम, तार्किक& रिक्तानि) {
        यदि (VN_IS(nodep, Unbounded)) {
            V3_सङ्ख्या सीमा{nodep, exprp->विस्तारः()};
            यदि (ऊर्ध्वस्थम्) सीमा.समुच्चयः_All_Bits1();
            यदि (exprp->isSigned()) {
                सीमा.समुच्चयः_Bit(exprp->विस्तारः() - 1, !ऊर्ध्वस्थम्);
                परिणाम.opExtendS(सीमा, सीमा.विस्तारः());
            } अन्यथा {
                परिणाम.op_नियोजय(सीमा);
            }
            प्रत्यागम सत्य;
        }
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        यदि (!constp) प्रत्यागम असत्य;
        यदि (constp->अङ्कः().isDouble()) {
            crossRealBound(constp, exprp, ऊर्ध्वस्थम्, domain, परिणाम, रिक्तानि);
            प्रत्यागम सत्य;
        }
        यदि (constp->अङ्कः().isString()) प्रत्यागम असत्य;
        यदि (bin_मानम् && exprp->isSigned() && constp->विस्तारः() <= exprp->विस्तारः()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_सङ्ख्या मानम्{nodep, exprp->विस्तारः()};
            यदि (constp->isSigned()) {
                मानम्.opExtendS(constp->अङ्कः(), constp->विस्तारः());
            } अन्यथा {
                मानम्.op_नियोजय(constp->अङ्कः());
            }
            परिणाम.opExtendS(मानम्, मानम्.विस्तारः());
        } अन्यथा यदि (constp->isSigned()) {
            परिणाम.opExtendS(constp->अङ्कः(), constp->विस्तारः());
        } अन्यथा {
            परिणाम.op_नियोजय(constp->अङ्कः());
        }
        प्रत्यागम सत्य;
    }

    स्थिर Cross_मानम्_Range cross_मानम्_Domain(Ast_ग्रन्थिः* nodep, पूर्णाङ्क मानम्_विस्तारः, तार्किक isSigned,
                                            पूर्णाङ्क विस्तारः) {
        Cross_मानम्_Range domain{nodep, विस्तारः};
        V3_सङ्ख्या lo{nodep, मानम्_विस्तारः};
        V3_सङ्ख्या hi{nodep, मानम्_विस्तारः};
        hi.समुच्चयः_All_Bits1();
        यदि (isSigned) {
            lo.समुच्चयः_Bit(मानम्_विस्तारः - 1, 1);
            hi.समुच्चयः_Bit(मानम्_विस्तारः - 1, 0);
            domain.lo.opExtendS(lo, मानम्_विस्तारः);
            domain.hi.opExtendS(hi, मानम्_विस्तारः);
        } अन्यथा {
            domain.lo.op_नियोजय(lo);
            domain.hi.op_नियोजय(hi);
        }
        प्रत्यागम domain;
    }

    स्थिर शून्य intersectCrossRange(Cross_मानम्_Range& range, ध्रुव Cross_मानम्_Range& other) {
        यदि (cross_मानम्_Less(range.lo, other.lo)) range.lo = other.lo;
        यदि (cross_मानम्_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    स्थिर तार्किक cross_मानम्_Range(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक bin_मानम्, तार्किक wildcard,
                                ध्रुव Cross_मानम्_Range& domain, Cross_मानम्_Range& range) {
        तार्किक रिक्तानि = असत्य;
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        यदि (ध्रुव AstInsideRange* ध्रुव rangep = VN_CAST(nodep, InsideRange)) {
            यदि (!crossRangeBound(rangep->lhsp(), exprp, असत्य, bin_मानम्, domain, range.lo, रिक्तानि)
                || !crossRangeBound(rangep->rhsp(), exprp, सत्य, bin_मानम्, domain, range.hi, रिक्तानि)
                || range.lo.isFourState() || range.hi.isFourState()) {
                प्रत्यागम असत्य;
            }
        } अन्यथा यदि (constp && constp->अङ्कः().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, असत्य, domain, range.lo, रिक्तानि);
            crossRealBound(constp, exprp, सत्य, domain, range.hi, रिक्तानि);
        } अन्यथा {
            यदि (!crossRangeBound(nodep, exprp, असत्य, bin_मानम्, domain, range.lo, रिक्तानि)) {
                प्रत्यागम असत्य;
            }
            यदि (bin_मानम् && exprp->isSigned() && constp && !constp->isSigned()
                && constp->विस्तारः() > exprp->विस्तारः()) {
                तार्किक representable = सत्य;
                प्रति (पूर्णाङ्क bit = exprp->विस्तारः(); bit < constp->विस्तारः(); ++bit) {
                    representable &= !constp->अङ्कः().bitIs1(bit);
                }
                यदि (representable) {
                    V3_सङ्ख्या मानम्{nodep, exprp->विस्तारः()};
                    मानम्.op_नियोजय(constp->अङ्कः());
                    range.lo.opExtendS(मानम्, मानम्.विस्तारः());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            यदि (wildcard) {
                range.आदर्शः_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                यदि (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_मानम्_Domain(nodep, constp->विस्तारः(), सत्य, domain.lo.विस्तारः()));
                }
            }
        }
        यदि (रिक्तानि) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            प्रत्यागम सत्य;
        }
        यदि (!range.lo.isFourState()) intersectCrossRange(range, domain);
        यदि (range.wildcard && exprp->isSigned()) {
            ध्रुव पूर्णाङ्क sign = exprp->विस्तारः() - 1;
            यदि (constp && !constp->isSigned() && constp->विस्तारः() > exprp->विस्तारः()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                प्रति (पूर्णाङ्क bit = exprp->विस्तारः(); bit < constp->विस्तारः(); ++bit) {
                    यदि (constp->अङ्कः().bitIs1(bit)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        प्रत्यागम सत्य;
                    }
                }
                प्रति (पूर्णाङ्क bit = exprp->विस्तारः(); bit < range.आदर्शः_2.विस्तारः(); ++bit) {
                    यदि (range.आदर्शः_2.bitIsXZ(sign))
                        range.आदर्शः_2.समुच्चयः_Bit(bit, 'x');
                    अन्यथा
                        range.आदर्शः_2.समुच्चयः_Bit(bit, range.आदर्शः_2.bitIs1(sign));
                }
                प्रत्यागम सत्य;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            प्रति (पूर्णाङ्क bit = exprp->विस्तारः(); bit < range.आदर्शः_2.विस्तारः(); ++bit) {
                यदि (range.आदर्शः_2.bitIsXZ(bit)) अनुवर्त;
                ध्रुव तार्किक मानम् = range.आदर्शः_2.bitIs1(bit);
                यदि (!range.आदर्शः_2.bitIsXZ(sign) && range.आदर्शः_2.bitIs1(sign) != मानम्) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    भङ्ग;
                }
                range.आदर्शः_2.समुच्चयः_Bit(sign, मानम्);
            }
        }
        प्रत्यागम सत्य;
    }

    गणना CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_अधस्तनम् = 1,
        CROSS_AT_ऊर्ध्वस्थम् = 2,
        CROSS_AT_BOUNDS = CROSS_AT_अधस्तनम् | CROSS_AT_ऊर्ध्वस्थम्,
        CROSS_NO_मेलनम् = 4  // Prefix cannot match the interval/pattern
    };

    स्थिर CrossRangeState cross_Range_चरणः(ध्रुव Cross_मानम्_Range& range, CrossRangeState state,
                                          पूर्णाङ्क bit, पूर्णाङ्क मानम्) {
        यदि (state == CROSS_NO_मेलनम्) प्रत्यागम CROSS_NO_मेलनम्;
        // Flipping the sign bit makes signed order lexicographic.
        ध्रुव तार्किक sign = bit == range.आदर्शः_2.विस्तारः() - 1;
        यदि (!range.आदर्शः_2.bitIsXZ(bit) && मानम् != (range.आदर्शः_2.bitIs1(bit) ^ sign)) {
            प्रत्यागम CROSS_NO_मेलनम्;
        }
        ध्रुव पूर्णाङ्क नीचः = range.lo.bitIs1(bit) ^ sign;
        ध्रुव पूर्णाङ्क उच्चः = range.hi.bitIs1(bit) ^ sign;
        यदि (((state & CROSS_AT_अधस्तनम्) && मानम् < नीचः)
            || ((state & CROSS_AT_ऊर्ध्वस्थम्) && मानम् > उच्चः)) {
            प्रत्यागम CROSS_NO_मेलनम्;
        }
        प्रत्यागम static_cast<CrossRangeState>(
            ((state & CROSS_AT_अधस्तनम्) && मानम् == नीचः ? CROSS_AT_अधस्तनम् : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_ऊर्ध्वस्थम्) && मानम् == उच्चः ? CROSS_AT_ऊर्ध्वस्थम् : CROSS_INSIDE_BOUNDS));
    }

    स्थिर तार्किक crossWildcardIntersects(ध्रुव Cross_मानम्_Range& range) {
        अचिह्नित अवस्थाः = 1U << CROSS_AT_BOUNDS;
        प्रति (पूर्णाङ्क bit = range.आदर्शः_2.विस्तारः() - 1; bit >= 0 && अवस्थाः; --bit) {
            अचिह्नित अग्र = 0;
            प्रति (ध्रुव CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_अधस्तनम्, CROSS_AT_ऊर्ध्वस्थम्, CROSS_AT_BOUNDS}) {
                यदि (!(अवस्थाः & (1U << state))) अनुवर्त;
                प्रति (पूर्णाङ्क मानम् = 0; मानम् < 2; ++मानम्) {
                    ध्रुव CrossRangeState equal = cross_Range_चरणः(range, state, bit, मानम्);
                    यदि (equal != CROSS_NO_मेलनम्) अग्र |= 1U << equal;
                }
            }
            अवस्थाः = अग्र;
        }
        प्रत्यागम अवस्थाः != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    स्थिर तार्किक अग्र_आदर्शः_मानम्(ध्रुव V3_सङ्ख्या& आदर्शः_2, ध्रुव V3_सङ्ख्या& from, V3_सङ्ख्या& परिणाम) {
        परिणाम.op_नियोजय(from);
        पूर्णाङ्क carry = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        प्रति (पूर्णाङ्क bit = from.विस्तारः() - 1; bit >= 0; --bit) {
            यदि (आदर्शः_2.bitIsXZ(bit)) {
                यदि (!from.bitIs1(bit)) carry = bit;
                अनुवर्त;
            }
            यदि (आदर्शः_2.bitIs1(bit) == from.bitIs1(bit)) अनुवर्त;
            यदि (from.bitIs1(bit)) {  // Only a greater prefix, an x bit above raised, can match
                यदि (carry < 0) प्रत्यागम असत्य;
                bit = carry;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            परिणाम.समुच्चयः_Bit(bit, 1);
            यावत् (--bit >= 0) परिणाम.समुच्चयः_Bit(bit, आदर्शः_2.bitIs1(bit));
            प्रत्यागम सत्य;
        }
        प्रत्यागम सत्य;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    स्थिर शून्य crossRangeRuns(ध्रुव Cross_मानम्_Range& range, आकार_t सीमा,
                               std::सदिशम्<std::pair<V3_सङ्ख्या, V3_सङ्ख्या>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ध्रुव पूर्णाङ्क sign = range.lo.विस्तारः() - 1;
        ध्रुव स्वचालित flipped = [sign](V3_सङ्ख्या मानम्) {
            यदि (!मानम्.bitIsXZ(sign)) मानम्.समुच्चयः_Bit(sign, !मानम्.bitIs1(sign));
            प्रत्यागम मानम्;
        };
        ध्रुव V3_सङ्ख्या आदर्शः_2 = flipped(range.आदर्शः_2);
        ध्रुव V3_सङ्ख्या hi = flipped(range.hi);
        ध्रुव V3_सङ्ख्या one{&hi, hi.विस्तारः(), 1};
        V3_सङ्ख्या trailing{&hi, hi.विस्तारः()};
        प्रति (पूर्णाङ्क bit = 0; bit <= sign && आदर्शः_2.bitIsXZ(bit); ++bit) trailing.समुच्चयः_Bit(bit, 1);
        V3_सङ्ख्या from = flipped(range.lo);
        V3_सङ्ख्या first{&hi, hi.विस्तारः()};
        V3_सङ्ख्या चरमः{&hi, hi.विस्तारः()};
        V3_सङ्ख्या less{&hi};
        यावत् (runs.आकार() <= सीमा && अग्र_आदर्शः_मानम्(आदर्शः_2, from, first)
               && less.opLt(hi, first).isEqZero()) {
            चरमः.opOr(first, trailing);
            यदि (!less.opLt(hi, चरमः).isEqZero()) चरमः = hi;
            runs.emplace_पृष्ठभागः(flipped(first), flipped(चरमः));
            यदि (चरमः.isCaseEq(hi)) भङ्ग;
            from.op_सङ्कलय(चरमः, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    स्थिर तार्किक cross_Range_रिक्तानि(ध्रुव Cross_मानम्_Range& range) {
        यदि (range.lo.isFourState()) प्रत्यागम सत्य;
        प्रत्यागम cross_मानम्_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    स्थिर पूर्णाङ्क resolve_विस्तारः(Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        प्रत्यागम std::max(exprp->विस्तारः(), cross_Range_विस्तारः(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    स्थिर तार्किक resolve_मानम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक bin_मानम्, तार्किक wildcard,
                             Cross_मानम्_Range& range) {
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(nodep, exprp->विस्तारः(), exprp->isSigned(), range.lo.विस्तारः());
        प्रत्यागम cross_मानम्_Range(nodep, exprp, bin_मानम्, wildcard, domain, range);
    }

    स्थिर तार्किक crossRangesIntersect(ध्रुव Cross_मानम्_Range& bin, ध्रुव Cross_मानम्_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        यदि (bin.lo.isFourState() || filter.lo.isFourState()) प्रत्यागम असत्य;
        Cross_मानम्_Range मेलनम् = bin;
        intersectCrossRange(मेलनम्, filter);
        प्रत्यागम !cross_मानम्_Less(मेलनम्.hi, मेलनम्.lo)
               && (!bin.wildcard || crossWildcardIntersects(मेलनम्));
    }

    स्थिर शून्य unsupportedCrossRange(AstCoverBinsof* selectp, तार्किक& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = असत्य;
    }

    स्थिर तार्किक cross_मानम्_Matches_Filters(AstCoverBinsof* selectp, Ast_ग्रन्थिः* valuep,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव AstCoverBin* binp,
                                         ध्रुव Cross_मानम्_Range& domain,
                                         ध्रुव std::सदिशम्<Cross_मानम्_Range>& filters,
                                         तार्किक& valid) {
        Cross_मानम्_Range range{valuep, domain.lo.विस्तारः()};
        यदि (!cross_मानम्_Range(valuep, exprp, सत्य, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            प्रत्यागम असत्य;
        }
        प्रति (ध्रुव Cross_मानम्_Range& filter : filters) {
            यदि (crossRangesIntersect(range, filter)) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    std::सदिशम्<तार्किक> selectCoverpointBins(AstCoverBinsof* selectp, ध्रुव CoverpointBins& bins,
                                           uint32_t first, uint32_t गणनम्, तार्किक& valid) {
        यदि (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            प्रत्यागम {};
        }
        std::सदिशम्<तार्किक> selected(bins.योग, असत्य);
        std::सदिशम्<std::सदिशम्<Ast_ग्रन्थिः*>> मानानि;
        पूर्णाङ्क विस्तारः = bins.exprp->विस्तारः();
        प्रति (Ast_ग्रन्थिः* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            विस्तारः = std::max(विस्तारः, cross_Range_विस्तारः(rangep));
        }
        यदि (selectp->rangesp()) {
            मानानि.reserve(गणनम्);
            प्रति (uint32_t i = first; i < first + गणनम्; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                मानानि.प्रक्षिप_पृष्ठभागः(bins.मानानि[i].runp ? std::सदिशम्<Ast_ग्रन्थिः*>{}
                                                     : cross_Bin_मानानि(bins.मानानि[i]));
                प्रति (Ast_ग्रन्थिः* ध्रुव valuep : मानानि.पृष्ठभागः()) {
                    विस्तारः = std::max(विस्तारः, cross_Range_विस्तारः(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++विस्तारः;
        ध्रुव Cross_मानम्_Range domain
            = cross_मानम्_Domain(selectp, bins.exprp->विस्तारः(), bins.exprp->isSigned(), विस्तारः);
        std::सदिशम्<Cross_मानम्_Range> filters;
        प्रति (Ast_ग्रन्थिः* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मानम्_Range filter{rangep, विस्तारः};
            यदि (!cross_मानम्_Range(rangep, bins.exprp, असत्य, असत्य, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                प्रत्यागम {};
            }
            filters.प्रक्षिप_पृष्ठभागः(std::move(filter));
        }
        प्रति (uint32_t i = first; i < first + गणनम्; ++i) {
            यदि (!selectp->rangesp()) {
                selected[i] = सत्य;
                अनुवर्त;
            }
            यदि (ध्रुव Bin_सततम्* ध्रुव runp = bins.मानानि[i].runp) {
                ध्रुव Cross_मानम्_Range range
                    = सततम्_Bin_Range(selectp, *runp, bins.मानानि[i].element, विस्तारः);
                selected[i] = std::any_सम्बन्धि(filters.begin(), filters.अन्तः_2(),
                                          [&](ध्रुव Cross_मानम्_Range& filter) {
                                              प्रत्यागम crossRangesIntersect(range, filter);
                                          });
                अनुवर्त;
            }
            प्रति (Ast_ग्रन्थिः* ध्रुव valuep : मानानि[i - first]) {
                selected[i] = cross_मानम्_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.मानानि[i].binp, domain, filters, valid);
                यदि (!valid) प्रत्यागम {};
                यदि (selected[i]) भङ्ग;
            }
        }
        यदि (selectp->isNegated()) {
            प्रति (uint32_t i = 0; i < bins.योग; ++i) selected[i] = !selected[i];
        }
        प्रत्यागम selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    संरचना मानम्_Lists final {
        std::सदिशम्<std::string> m_ranges;  // Bin, then low and high words
        std::सदिशम्<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::सदिशम्<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::सदिशम्<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    स्थिर शून्य append_शब्दाः(std::string& text, ध्रुव V3_सङ्ख्या& मानम्, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        V3_सङ्ख्या narrowed{&मानम्, exprp->विस्तारः()};
        narrowed.op_नियोजय(मानम्);
        प्रति (पूर्णाङ्क word = 0; word < exprp->विस्तारः_शब्दाः(); ++word) {
            text += ", " + cvt_गन्तव्यम्_Str(narrowed.edataWord(word)) + "U";
        }
    }

    शून्य collect_मानम्_Metadata(मानम्_Lists& lists, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstCoverBin* binp,
                              uint32_t सूचकांक, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        ध्रुव std::string bin = cvt_गन्तव्यम्_Str(सूचकांक) + "U";
        यदि (binp->transp()) lists.m_transitions.प्रक्षिप_पृष्ठभागः(bin);
        प्रति (Ast_ग्रन्थिः* ध्रुव sourcep : cross_Bin_मानानि({binp, valuep})) {
            Cross_मानम्_Range range{sourcep, resolve_विस्तारः(sourcep, exprp)};
            यदि (!resolve_मानम्(sourcep, exprp, सत्य, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                अनुवर्त;
            }
            यदि (cross_Range_रिक्तानि(range)) अनुवर्त;
            std::string प्रविष्टिः = bin;
            यदि (range.wildcard) {
                V3_सङ्ख्या मानम्{sourcep, exprp->विस्तारः()};
                V3_सङ्ख्या आवरणम्{sourcep, exprp->विस्तारः()};
                आवरणम्.op_द्वयङ्काः_Non_XZ(range.आदर्शः_2);
                मानम्.op_द्वयङ्काः_One(range.आदर्शः_2);
                append_शब्दाः(प्रविष्टिः, मानम्, exprp);
                append_शब्दाः(प्रविष्टिः, आवरणम्, exprp);
            }
            append_शब्दाः(प्रविष्टिः, range.lo, exprp);
            append_शब्दाः(प्रविष्टिः, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).प्रक्षिप_पृष्ठभागः(प्रविष्टिः);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    स्थिर शून्य collect_सततम्_Metadata(मानम्_Lists& lists, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव Bin_सततम्& सततम्) {
        V3_सङ्ख्या विस्तृतिः{exprp, सततम्.m_stride.विस्तारः(), 0};
        विस्तृतिः.opSub(सततम्.m_stride, V3_सङ्ख्या{exprp, सततम्.m_stride.विस्तारः(), 1});
        std::string प्रविष्टिः = cvt_गन्तव्यम्_Str(सततम्.m_declared) + "U, " + cvt_गन्तव्यम्_Str(सततम्.m_गणनम्) + "U";
        append_शब्दाः(प्रविष्टिः, सततम्.m_lo, exprp);
        append_शब्दाः(प्रविष्टिः, विस्तृतिः, exprp);
        append_शब्दाः(प्रविष्टिः, सततम्.m_hi, exprp);
        lists.m_runs.प्रक्षिप_पृष्ठभागः(प्रविष्टिः);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    शून्य emit_मानम्_List(File_पङ्क्तिः* fl, AstVar* cpVarp, VCMethod विधानम्,
                       ध्रुव std::सदिशम्<std::string>& entries) {
        प्रति (आकार_t first = 0; first < entries.आकार(); first += मानम्_LIST_ENTRIES) {
            ध्रुव आकार_t अन्तः_2 = std::min(entries.आकार(), first + मानम्_LIST_ENTRIES);
            std::string text = "{" + entries[first];
            प्रति (आकार_t i = first + 1; i < अन्तः_2; ++i) text += ", " + entries[i];
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cpVarp, विधानम्, {ctext(fl, text + "}")})->makeStmt());
        }
    }

    स्थिर तार्किक checkCrossRef(ध्रुव AstCoverCrossRef* refp, ध्रुव AstCoverCross* crossp) {
        यदि (refp->name() == crossp->name()) प्रत्यागम सत्य;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक checkCrossBinName(ध्रुव AstCoverCrossBin* binp, std::समुच्चयः<std::string>& नामानि) {
        यदि (नामानि.emplace(binp->name()).second) प्रत्यागम सत्य;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        प्रत्यागम असत्य;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    स्थिर तार्किक implicit_Auto_Bin_विस्तृतिः(ध्रुव CoverpointBins& bins, ध्रुव std::string& name,
                                    Bin_विस्तृतिः& विस्तृतिः) {
        ध्रुव std::string उपसर्गः = "auto_";
        यदि (!VString::startsWith(name, उपसर्गः)) प्रत्यागम असत्य;
        ध्रुव std::string digits = name.substr(उपसर्गः.आकार());
        ध्रुव अचिह्नित दीर्घ सूचकांक = std::strtoul(digits.c_str(), शून्यसूचकः, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        यदि (सूचकांक >= bins.implicitAuto.गणनम् || digits != cvt_गन्तव्यम्_Str(सूचकांक)) प्रत्यागम असत्य;
        ध्रुव uint32_t विस्थापनम् = static_cast<uint32_t>(सूचकांक);
        विस्तृतिः = Bin_विस्तृतिः{bins.implicitAuto.first + विस्थापनम्, 1, bins.implicitAuto.declared + विस्थापनम्};
        प्रत्यागम सत्य;
    }

    CrossBinsofTarget
    resolveBinsofTarget(ध्रुव AstCoverBinsof* selectp, ध्रुव AstCoverCross* crossp,
                        ध्रुव std::सदिशम्<AstVar*>& cpVars,
                        ध्रुव std::map<std::string, uint32_t>& dimensions) ध्रुव {
        ध्रुव स्वचालित dim = dimensions.find(selectp->pointp()->name());
        यदि (dim == dimensions.अन्तः_2()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            प्रत्यागम {};
        }
        ध्रुव CoverpointBins& bins = m_cpBins.at(cpVars[dim->second]);
        CrossBinsofTarget target{&bins, dim->second, 0, bins.योग};
        यदि (!selectp->name().रिक्तानि()) {
            ध्रुव स्वचालित bin = bins.spans.find(selectp->name());
            Bin_विस्तृतिः विस्तृतिः{0, 0, 0};
            यदि (bin != bins.spans.अन्तः_2()) {
                विस्तृतिः = bin->second;
            } अन्यथा यदि (!implicit_Auto_Bin_विस्तृतिः(bins, selectp->name(), विस्तृतिः)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                प्रत्यागम {};
            }
            target.m_first = विस्तृतिः.first;
            target.m_गणनम् = विस्तृतिः.गणनम्;
            target.m_declaredFirst = विस्तृतिः.declared;
            target.m_declared_अन्तः = विस्तृतिः.declared + विस्तृतिः.गणनम्;
            target.m_sized = विस्तृतिः.sized;
        }
        प्रत्यागम target;
    }

    // The variables a 'with' filter reads, each by its first reference
    उपयुज्य CrossFilterReads = std::map<ध्रुव AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    स्थिर तार्किक counts_मानम्_Tuples(ध्रुव AstCoverCrossWith* withp) {
        प्रत्यागम withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    स्थिर तार्किक crossFilterSupported(ध्रुव AstCoverCrossWith* withp,
                                     ध्रुव CrossFilterReads& reads) {
        प्रति (ध्रुव AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            यदि (varp->dtypep()->skipRefp()->isIntegralOrPacked()) अनुवर्त;
            ध्रुव स्वचालित it = reads.find(varp);
            यदि (it != reads.अन्तः_2()) {
                it->second->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->second->prettyNameQ());
                प्रत्यागम असत्य;
            }
            यदि (counts_मानम्_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                प्रत्यागम असत्य;
            }
        }
        प्रत्यागम सत्य;
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
    शून्य generateCrossFilter(ध्रुव AstCoverCrossWith* withp, ध्रुव CrossFilterReads& reads,
                             ध्रुव AstCoverCrossBin* binp, ध्रुव AstCoverCross* crossp,
                             AstVar* cxp, ध्रुव std::map<string, uint32_t>& dimensions,
                             uint32_t वस्तूनि) {
        File_पङ्क्तिः* ध्रुव fl = withp->fileline();
        ध्रुव string उपसर्गः
            = m_sized_नामानि.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ध्रुव तार्किक counting = counts_मानम्_Tuples(withp);
        // The count, 0 for all
        ध्रुव std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*> गणनम्
            = counting ? new_गणनम्_तर्काः(fl, उपसर्गः + "_count", withp->countp())
                       : new_गणनम्_तर्काः(fl, withp->countp() ? 0 : 1);
        AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
        AstCMethodHard* ध्रुव nextp = itemCall(fl, cxp, VCMethod::COVERGROUP_WITH_अग्र);
        nextp->dtype_समुच्चयः_Bit();
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{fl, loopp, nextp});
        string dims;
        प्रति (uint32_t dim = 0; counting && dim < वस्तूनि; ++dim) {
            dims += (dims.रिक्तानि() ? "" : ", ") + cvt_गन्तव्यम्_Str(dim) + "U";
        }
        यावत् (AstVar* ध्रुव varp = withp->varsp()) {
            varp->unlink_Fr_पृष्ठभागः();
            यदि (!reads.गणनम्(varp)) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(varp), varp);
                अनुवर्त;
            }
            ध्रुव स्वचालित dimIt = dimensions.find(varp->name());
            UASSERT_OBJ(dimIt != dimensions.अन्तः_2(), varp, "Cross 'with' candidate not of an item");
            ध्रुव uint32_t dim = dimIt->second;
            यदि (!counting) dims += (dims.रिक्तानि() ? "" : ", ") + cvt_गन्तव्यम्_Str(dim) + "U";
            varp->name(उपसर्गः + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->सङ्कलय_Stmtsp(varp);
            यदि (varp->isWide()) {
                loopp->सङ्कलय_Stmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मानम्_W,
                                          {cnum(fl, dim), नूतन AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } अन्यथा {
                AstCMethodHard* ध्रुव valuep
                    = itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_मानम्, {cnum(fl, dim)});
                valuep->dtype_समुच्चयः_UInt64();
                Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->findLogicDType(
                    varp->विस्तारः(), varp->विस्तारः(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_मानम्(valuep, dtypep)});
            }
        }
        loopp->सङ्कलय_Stmtsp(itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_पृष्ठभागः()})
                             ->makeStmt());
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), गणनम्.first, गणनम्.second,
                      ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->सङ्कलय_Stmtsp(loopp);
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_अन्तः)->makeStmt());
    }

    तार्किक generateRuntimeSelection(Ast_ग्रन्थिः* nodep, ध्रुव AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ध्रुव std::सदिशम्<AstVar*>& cpVars,
                                  ध्रुव std::map<string, uint32_t>& dimensions) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (ध्रुव AstCoverCrossRef* ध्रुव refp = VN_CAST(nodep, CoverCrossRef)) {
            यदि (!checkCrossRef(refp, crossp)) प्रत्यागम असत्य;
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            प्रत्यागम सत्य;
        }
        यदि (ध्रुव AstCoverCrossSelect* ध्रुव opp = VN_CAST(nodep, CoverCrossSelect)) {
            यदि (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                प्रत्यागम असत्य;
            }
            m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            प्रत्यागम सत्य;
        }
        यदि (ध्रुव AstCoverCrossWith* ध्रुव withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            यदि ((counts_मानम्_Tuples(withp)
                 && !check_गणनम्(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                प्रत्यागम असत्य;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.आकार()));
            प्रत्यागम सत्य;
        }
        AstCoverBinsof* ध्रुव selectp = VN_AS(nodep, CoverBinsof);
        ध्रुव CrossBinsofTarget target = resolveBinsofTarget(selectp, crossp, cpVars, dimensions);
        यदि (!target.m_binsp) प्रत्यागम असत्य;
        ध्रुव CoverpointBins& bins = *target.m_binsp;
        यदि (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            तार्किक valid = सत्य;
            unsupportedCrossRange(selectp, valid);
            प्रत्यागम असत्य;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* firstp;
        Ast_ग्रन्थिः_व्यञ्जकम्* endp;
        यदि (target.m_sized < 0) {
            firstp = cnum(fl, target.m_declaredFirst);
            endp = cnum(fl, target.m_declared_अन्तः);
        } अन्यथा {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ध्रुव cpVarp = cpVars[target.m_dimension];
            ध्रुव uint32_t sized = static_cast<uint32_t>(target.m_sized);
            firstp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_SIZED_FIRST, {cnum(fl, sized)});
            endp = itemCall(fl, cpVarp, VCMethod::COVERGROUP_SIZED_अन्तः, {cnum(fl, sized)});
            firstp->dtype_समुच्चयः_UInt32();
            endp->dtype_समुच्चयः_UInt32();
        }
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, target.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != शून्यसूचकः)})
                ->makeStmt());
        प्रति (Ast_ग्रन्थिः* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_मानम्_Range range{rangep, resolve_विस्तारः(rangep, bins.exprp)};
            यदि (!resolve_मानम्(rangep, bins.exprp, असत्य, असत्य, range)) {
                तार्किक valid = सत्य;
                unsupportedCrossRange(selectp, valid);
                प्रत्यागम असत्य;
            }
            यदि (cross_Range_रिक्तानि(range)) अनुवर्त;
            m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_मानम्_Const(fl, range.lo, bins.exprp),
                                                new_मानम्_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_अन्तः)->makeStmt());
        प्रत्यागम सत्य;
    }

    std::सदिशम्<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ध्रुव std::सदिशम्<AstVar*>& cpVars,
                             ध्रुव std::map<string, uint32_t>& dimensions) {
        std::सदिशम्<AstCoverCrossBin*> bins;
        std::समुच्चयः<string> नामानि;
        प्रति (Ast_ग्रन्थिः* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ध्रुव binp = VN_AS(itemp, CoverCrossBin);
            यदि (!checkCrossBinName(binp, नामानि)) अनुवर्त;
            File_पङ्क्तिः* ध्रुव fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ध्रुव selectionp = नूतन AstFunc{fl, "", शून्यसूचकः, शून्यसूचकः};
            तार्किक supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            यदि (supported) m_constructorp->सङ्कलय_Stmtsp(selectionp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            VL_DO_DANGLING(प्रक्षिप_Deletep(selectionp), selectionp);
            यदि (!supported) अनुवर्त;
            ध्रुव तार्किक protect = v3Global.opt.protectIds();
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_समुच्चयः_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->firstColumn()),
                          cnum(fl, static_cast<uint32_t>(bins.आकार()))})
                    ->makeStmt());
            bins.प्रक्षिप_पृष्ठभागः(binp);
        }
        m_constructorp->सङ्कलय_Stmtsp(
            itemCall(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        प्रत्यागम bins;
    }

    स्थिर शून्य समुच्चयः_Cross_Selection_Range(CrossSelection& selection, uint64_t first, uint64_t अन्तः_2) {
        यावत् (first < अन्तः_2) {
            ध्रुव अचिह्नित bit = first % 64;
            ध्रुव अचिह्नित द्वयङ्काः = std::min<uint64_t>(64 - bit, अन्तः_2 - first);
            selection[VL_BITWORD_Q(first)]
                |= (द्वयङ्काः == 64 ? ~uint64_t{0} : (uint64_t{1} << द्वयङ्काः) - 1) << bit;
            first += द्वयङ्काः;
        }
    }

    CrossSelection crossSelection(Ast_ग्रन्थिः* nodep, CrossSelectionContext& ctx) {
        यदि (ध्रुव AstCoverCrossRef* ध्रुव refp = VN_CAST(nodep, CoverCrossRef)) {
            यदि (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = असत्य;
                प्रत्यागम {};
            }
            CrossSelection परिणाम(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            समुच्चयः_Cross_Selection_Range(परिणाम, 0, ctx.tuples);
            प्रत्यागम परिणाम;
        }
        यदि (AstCoverCrossSelect* ध्रुव opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ध्रुव CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            यदि (!ctx.valid) प्रत्यागम {};
            प्रति (आकार_t i = 0; i < lhs.आकार(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            प्रत्यागम lhs;
        }
        AstCoverBinsof* ध्रुव selectp = VN_AS(nodep, CoverBinsof);
        ध्रुव CrossBinsofTarget target
            = resolveBinsofTarget(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        यदि (!target.m_binsp) {
            ctx.valid = असत्य;
            प्रत्यागम {};
        }
        ध्रुव CoverpointBins& bins = *target.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(target.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ध्रुव std::सदिशम्<तार्किक> selected
            = selectCoverpointBins(selectp, bins, target.m_first, target.m_गणनम्, ctx.valid);
        यदि (!ctx.valid) प्रत्यागम {};
        CrossSelection परिणाम(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ध्रुव uint64_t stride = ctx.strides[target.m_dimension];
        ध्रुव uint64_t period = stride * bins.योग;
        प्रति (uint64_t आधारः = 0; आधारः < ctx.tuples; आधारः += period) {
            प्रति (uint32_t i = 0; i < bins.योग;) {
                यदि (!selected[i]) {
                    ++i;
                    अनुवर्त;
                }
                ध्रुव uint32_t begin = i++;
                यावत् (i < bins.योग && selected[i]) ++i;
                समुच्चयः_Cross_Selection_Range(परिणाम, आधारः + begin * stride, आधारः + i * stride);
            }
        }
        प्रत्यागम परिणाम;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    तार्किक crossShape(AstCoverCross* crossp, ध्रुव std::सदिशम्<AstVar*>& cpVars,
                    std::सदिशम्<uint32_t>& strides, uint32_t& tuples) ध्रुव {
        uint64_t गुणनफलम् = std::any_सम्बन्धि(cpVars.begin(), cpVars.अन्तः_2(),
                                       [एतत्](AstVar* varp) { प्रत्यागम !m_cpBins.at(varp).योग; })
                               ? 0
                               : 1;
        strides.resize(cpVars.आकार());
        प्रति (आकार_t d = cpVars.आकार(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(गुणनफलम्);
            गुणनफलम् *= m_cpBins.at(cpVars[d - 1]).योग;
            यदि (गुणनफलम् > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                प्रत्यागम असत्य;
            }
        }
        tuples = static_cast<uint32_t>(गुणनफलम्);
        प्रत्यागम सत्य;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ध्रुव std::सदिशम्<AstVar*>& cpVars,
                                   ध्रुव std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        यदि (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = असत्य;
            प्रत्यागम layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::समुच्चयः<std::string> नामानि;
        प्रति (Ast_ग्रन्थिः* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ध्रुव binp = VN_AS(itemp, CoverCrossBin);
            यदि (!checkCrossBinName(binp, नामानि)) अनुवर्त;
            ctx.valid = सत्य;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            यदि (!ctx.valid || std::all_सम्बन्धि(selection.begin(), selection.अन्तः_2(), [](uint64_t word) {
                    प्रत्यागम word == 0;
                })) {
                अनुवर्त;
            }
            यदि (occupied.रिक्तानि()) {
                occupied.resize(selection.आकार(), 0);
                excluded.resize(selection.आकार(), 0);
            }
            प्रति (आकार_t i = 0; i < selection.आकार(); ++i) {
                occupied[i] |= selection[i];
                यदि (!binp->binsType().bin_Is_प्रसामान्यम्()) excluded[i] |= selection[i];
            }
            layout.bins.प्रक्षिप_पृष्ठभागः({binp, std::move(selection)});
        }
        यदि (!layout.bins.रिक्तानि()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            प्रति (ResolvedCrossBin& resolved : layout.bins) {
                प्रति (आकार_t i = 0; i < resolved.selection.आकार(); ++i) {
                    यदि (resolved.binp->binsType().bin_Is_प्रसामान्यम्()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    यदि (resolved.selection[i]) ++layout.bin_शब्दाः;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.अन्तः_2(),
                                             [](ध्रुव ResolvedCrossBin& resolved) {
                                                 प्रत्यागम std::all_सम्बन्धि(
                                                     resolved.selection.begin(),
                                                     resolved.selection.अन्तः_2(),
                                                     [](uint64_t word) { प्रत्यागम word == 0; });
                                             }),
                              layout.bins.अन्तः_2());
            layout.autoBins = layout.tuples;
            प्रति (ध्रुव uint64_t word : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{word}.गणनम्());
            }
        }
        प्रत्यागम layout;
    }

    AstCoverCrossDType* crossDType(File_पङ्क्तिः* fl, uint32_t dimensions, ध्रुव CrossLayout& layout,
                                   तार्किक dynamic = असत्य) {
        ध्रुव uint32_t bins = static_cast<uint32_t>(layout.bins.आकार());
        ध्रुव CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_शब्दाः, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        यदि (!typep) {
            typep = नूतन AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_शब्दाः, dynamic};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(typep);
        }
        प्रत्यागम typep;
    }

    std::सदिशम्<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ध्रुव CrossLayout& layout) {
        std::सदिशम्<AstCoverCrossBin*> bins;
        प्रति (ध्रुव ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ध्रुव binp = resolved.binp;
            ध्रुव CrossSelection& selection = resolved.selection;
            File_पङ्क्तिः* ध्रुव fl = binp->fileline();
            ध्रुव तार्किक prot = v3Global.opt.protectIds();
            std::string आवरणम् = "{";
            प्रति (आकार_t i = 0; i < selection.आकार(); ++i) {
                यदि (i) आवरणम् += ", ";
                आवरणम् += std::गन्तव्यम्_string(selection[i]) + "ULL";
            }
            आवरणम् += "}";
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(fl, cxVarp, VCMethod::COVERGROUP_सङ्कलय_BIN,
                         {ctext(fl, binp->binsType().bin_समुच्चयः_Enum()), ctext(fl, आवरणम्),
                          ctext(fl, quoted(VIdProtect::protect_शब्दाः_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})
                    ->makeStmt());
            bins.प्रक्षिप_पृष्ठभागः(binp);
        }
        यदि (!bins.रिक्तानि()) {
            m_constructorp->सङ्कलय_Stmtsp(
                itemCall(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        प्रत्यागम bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    शून्य generateCross(AstCoverCross* crossp) {
        File_पङ्क्तिः* ध्रुव fl = crossp->fileline();
        ध्रुव तार्किक dynamic = m_runtimeCrosses.गणनम्(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_गन्तव्यम्_अस्थायी(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::सदिशम्<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        प्रति (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = itemp->nextp();
            AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
            ध्रुव स्वचालित it = m_cpVarMap.find(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.अन्तः_2(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.आकार()));
            cpVars.प्रक्षिप_पृष्ठभागः(it->second);
            VL_DO_DANGLING(प्रक्षिप_Deletep(refp->unlink_Fr_पृष्ठभागः()), refp);
            itemp = nextp;
        }
        ध्रुव पूर्णाङ्क dims = static_cast<पूर्णाङ्क>(cpVars.आकार());
        CrossLayout layout;
        यदि (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::सदिशम्<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } अन्यथा {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        यदि (!layout.valid) प्रत्यागम;

        AstVar* ध्रुव cxVarp
            = नूतन AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->सङ्कलय_Membersp(cxVarp);
        m_constructorp->सङ्कलय_Stmtsp(makeItemCreate(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_सङ्कलय_CROSS_DYN
                                                         : VCMethod::COVERGROUP_सङ्कलय_CROSS));
        generateItemWeight(fl, cxVarp, crossp->optionsp());
        पूर्णाङ्क at_न्यूनतमम्_मानम्;
        पूर्णाङ्क autoBinMax;  // Not a cross option
        extractItemOptions(crossp->optionsp(), at_न्यूनतमम्_मानम्, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ध्रुव तार्किक prot = v3Global.opt.protectIds();
        ध्रुव std::string hier
            = VIdProtect::protect_शब्दाः_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->सङ्कलय_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            itemCall(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_न्यूनतमम्_मानम्)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->firstColumn()))})));
        ध्रुव std::सदिशम्<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        यदि (v3Global.opt.coverage()) {
            ध्रुव std::string page
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->सङ्कलय_Stmtsp(itemCall(fl, cxVarp, VCMethod::COVERGROUP_पञ्जिकाकोशः_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(page)),
                                                cnum(fl, itemDatabaseWeight(crossp->optionsp())),
                                                cnum(fl, m_cgTypeWeight)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ध्रुव तार्किक hasIffs
            = std::any_सम्बन्धि(bins.begin(), bins.अन्तः_2(),
                          [](ध्रुव AstCoverCrossBin* binp) { प्रत्यागम binp->iffp() != शून्यसूचकः; });
        Ast_ग्रन्थिः_Stmt* ध्रुव samplep
            = !hasIffs ? static_cast<Ast_ग्रन्थिः_Stmt*>(
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ग्रन्थिः_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             itemCall(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iffp = crossp->iffp()) {
            m_sampleFuncp->सङ्कलय_Stmtsp(नूतन AstIf{fl, iffp->clone_वृक्षः(असत्य), samplep});
        } अन्यथा {
            m_sampleFuncp->सङ्कलय_Stmtsp(samplep);
        }
    }

    शून्य generate_Cross_संहिता(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        प्रति (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
            यदि (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                प्रत्यागम;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        यदि (m_droppedCrosses.गणनम्(crossp)) {
            प्रति (Ast_ग्रन्थिः* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                यदि (m_coverpointMap.find(refp->name()) == m_coverpointMap.अन्तः_2()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    भङ्ग;
                }
            }
            प्रत्यागम;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* buildBinCondition(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Get the range list from the bin
        Ast_ग्रन्थिः* ध्रुव rangep = binp->rangesp();
        यदि (!rangep) प्रत्यागम शून्यसूचकः;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ध्रुव तार्किक integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        यदि (binp->isWildcard() && !integral) प्रत्यागम wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ग्रन्थिः_व्यञ्जकम्* पूर्णम्_Condp = शून्यसूचकः;

        प्रति (Ast_ग्रन्थिः* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* rangeCondp = शून्यसूचकः;
            currRangep = V3Const::constifyEdit(currRangep);

            यदि (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव minExprp = irp->lhsp();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maxExprp = irp->rhsp();
                AstConst* ध्रुव minConstp = VN_CAST(minExprp, Const);
                AstConst* ध्रुव maxConstp = VN_CAST(maxExprp, Const);
                ध्रुव तार्किक loUnbounded = VN_IS(minExprp, Unbounded);
                ध्रुव तार्किक hiUnbounded = VN_IS(maxExprp, Unbounded);
                यदि (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ध्रुव boundp = hiUnbounded ? minConstp : maxConstp;
                    यदि (loUnbounded && hiUnbounded) {
                        rangeCondp = नूतन AstConst{irp->fileline(), AstConst::BitTrue{}};
                    } अन्यथा यदि (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        यदि (पूर्णम्_Condp) VL_DO_DANGLING(प्रक्षिप_Deletep(पूर्णम्_Condp), पूर्णम्_Condp);
                        प्रत्यागम शून्यसूचकः;
                    } अन्यथा यदि (boundp->अङ्कः().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        यदि (पूर्णम्_Condp) VL_DO_DANGLING(प्रक्षिप_Deletep(पूर्णम्_Condp), पूर्णम्_Condp);
                        प्रत्यागम शून्यसूचकः;
                    } अन्यथा यदि (integral) {
                        rangeCondp = build_मानम्_Condition(binp, exprp, irp);
                    } अन्यथा {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } अन्यथा यदि (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    यदि (पूर्णम्_Condp) VL_DO_DANGLING(प्रक्षिप_Deletep(पूर्णम्_Condp), पूर्णम्_Condp);
                    प्रत्यागम शून्यसूचकः;
                } अन्यथा यदि (minConstp->अङ्कः().isFourState() || maxConstp->अङ्कः().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    यदि (पूर्णम्_Condp) VL_DO_DANGLING(प्रक्षिप_Deletep(पूर्णम्_Condp), पूर्णम्_Condp);
                    प्रत्यागम शून्यसूचकः;
                } अन्यथा यदि (integral) {
                    rangeCondp = build_मानम्_Condition(binp, exprp, irp);
                } अन्यथा {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } अन्यथा यदि (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_मानम्_Condition(binp, exprp, constp);
            } अन्यथा {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                यदि (पूर्णम्_Condp) VL_DO_DANGLING(प्रक्षिप_Deletep(पूर्णम्_Condp), पूर्णम्_Condp);
                प्रत्यागम शून्यसूचकः;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            पूर्णम्_Condp
                = पूर्णम्_Condp ? नूतन AstOr{binp->fileline(), पूर्णम्_Condp, rangeCondp} : rangeCondp;
        }

        प्रत्यागम पूर्णम्_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* wildcardTypeError(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        प्रत्यागम नूतन AstConst{binp->fileline(), AstConst::BitFalse{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ग्रन्थिः_व्यञ्जकम्* build_मानम्_Condition(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः* valuep) {
        File_पङ्क्तिः* ध्रुव fl = valuep->fileline();
        यदि (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            प्रत्यागम नूतन AstEq{fl, exprp->clone_वृक्षः(असत्य),
                             VN_AS(valuep, ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः(असत्य)};
        }
        Cross_मानम्_Range range{valuep, resolve_विस्तारः(valuep, exprp)};
        यदि (!resolve_मानम्(valuep, exprp, सत्य, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        यदि (cross_Range_रिक्तानि(range)) प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        AstConst* ध्रुव lop = new_मानम्_Const(fl, range.lo, exprp);
        AstConst* ध्रुव hip = new_मानम्_Const(fl, range.hi, exprp);
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
        यदि (lop->अङ्कः().isCaseEq(hip->अङ्कः())) {
            condp = नूतन AstEq{fl, exprp->clone_वृक्षः(असत्य), lop};
        } अन्यथा {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lop), lop);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(hip), hip);
        यदि (!range.wildcard) प्रत्यागम condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_सङ्ख्या आवरणम्{valuep, exprp->विस्तारः()};
        V3_सङ्ख्या मानम्{valuep, exprp->विस्तारः()};
        आवरणम्.op_द्वयङ्काः_Non_XZ(range.आदर्शः_2);
        मानम्.op_द्वयङ्काः_One(range.आदर्शः_2);
        AstConst* ध्रुव आवरणम्_Constp = new_मानम्_Const(fl, आवरणम्, exprp);
        AstConst* ध्रुव मानम्_Constp = new_मानम्_Const(fl, मानम्, exprp);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम्_Masked = नूतन AstAnd{fl, exprp->clone_वृक्षः(असत्य), आवरणम्_Constp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मानम्_Masked = नूतन AstAnd{fl, मानम्_Constp, आवरणम्_Constp->clone_वृक्षः(असत्य)};
        प्रत्यागम नूतन AstLogAnd{fl, condp, नूतन AstEq{fl, व्यञ्जकम्_Masked, मानम्_Masked}};
    }

    शून्य generate_Coverage_Computation_संहिता() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ध्रुव getInstCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_inst_coverage"), Func);
        File_पङ्क्तिः* ध्रुव instFl = getInstCoveragep->fileline();
        AstCMethodHard* ध्रुव instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_समुच्चयः_Double();
        Ast_ग्रन्थिः_व्यञ्जकम्* instValuep = instCallp;
        यदि (m_cgMayMerge) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव mergedp = नूतन AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(सत्य), "merge_instances", VAccess::READ),
                नूतन AstLogNot{instFl, newOptionSel(instFl, optionVar(असत्य), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = नूतन AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
            instFl, नूतन AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ध्रुव getCoveragep
            = VN_AS(m_memberMap.findMember(m_covergroupp, "get_coverage"), Func);
        File_पङ्क्तिः* ध्रुव typeFl = getCoveragep->fileline();
        getCoveragep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
            typeFl, नूतन AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_पङ्क्तिः* fl) {
        AstCExpr* ध्रुव registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_समुच्चयः_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ध्रुव callp
            = नूतन AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->सङ्कलय_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->सङ्कलय_Pinsp(newOptionSel(fl, optionVar(सत्य), "weight", VAccess::READ));
        callp->सङ्कलय_Pinsp(newOptionSel(fl, optionVar(सत्य), "merge_instances", VAccess::READ));
        callp->सङ्कलय_Pinsp(file_पङ्क्तिः_Debug(m_covergroupp->fileline()));
        callp->usePtr(सत्य);
        callp->dtype_समुच्चयः_Double();
        प्रत्यागम callp;
    }

    // VISITORS
    स्थिर तार्किक isEnclosingInstanceVar(ध्रुव AstVar* varp) {
        प्रत्यागम varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    शून्य rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ध्रुव AstClassRefDType* ध्रुव refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    शून्य rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_पङ्क्तिः* ध्रुव fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ध्रुव selp
            = नूतन AstMemberSel{fl, नूतन AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    शून्य rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_पङ्क्तिः* ध्रुव fl = refp->fileline();
        AstArg* ध्रुव argsp = refp->argsp() ? refp->argsp()->unlink_Fr_पृष्ठभागः_With_अग्र() : शून्यसूचकः;
        Ast_विधानम्_Call* ध्रुव callp = नूतन Ast_विधानम्_Call{
            fl, नूतन AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    तार्किक isEnclosingInstanceFunc(ध्रुव Ast_ग्रन्थिः_FTask* funcp) ध्रुव {
        यदि (!funcp->class_विधानम्() || funcp->isStatic()) प्रत्यागम असत्य;
        प्रत्यागम AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    तार्किक isEmbeddedCovergroupVar(ध्रुव AstVar* varp) ध्रुव {
        यदि (!varp || !varp->isClassMember() || varp->isDeclTyped()) प्रत्यागम असत्य;
        ध्रुव AstClassRefDType* ध्रुव refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        प्रत्यागम refp && refp->classp() == m_covergroupp;
    }

    AstVar* findEmbeddedCovergroupVar() ध्रुव {
        यदि (!m_enclosingClassp) प्रत्यागम शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(itemp, Var)) {
                यदि (isEmbeddedCovergroupVar(varp)) प्रत्यागम varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        प्रत्यागम शून्यसूचकः;  // LCOV_EXCL_LINE
    }

    std::सदिशम्<Ast_ग्रन्थिः_नियोजय*> findCovergroupConstructions() {
        std::सदिशम्<Ast_ग्रन्थिः_नियोजय*> foundps;
        यदि (!m_embeddedVarp) प्रत्यागम foundps;
        AstFunc* ध्रुव enclosingNewp
            = VN_CAST(m_memberMap.findMember(m_enclosingClassp, "new"), Func);
        यदि (!enclosingNewp) प्रत्यागम foundps;
        enclosingNewp->foreach([&](Ast_ग्रन्थिः_नियोजय* asgnp) {
            ध्रुव AstNew* ध्रुव newp = VN_CAST(asgnp->rhsp(), New);
            ध्रुव AstVarRef* ध्रुव lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            यदि (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) प्रत्यागम;
            ध्रुव AstClassRefDType* ध्रुव refp = VN_CAST(newp->dtypep(), ClassRefDType);
            यदि (refp && refp->classp() == m_covergroupp) foundps.प्रक्षिप_पृष्ठभागः(asgnp);
        });
        प्रत्यागम foundps;
    }

    std::समुच्चयः<ध्रुव AstVar*> enclosingInstanceVars() ध्रुव {
        std::समुच्चयः<ध्रुव AstVar*> vars;
        यदि (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ध्रुव, AstVar* ध्रुव varp) {
                यदि (isEnclosingInstanceVar(varp)) vars.निवेशय(varp);
            });
        }
        प्रत्यागम vars;
    }

    तार्किक hasEnclosingEventRef(AstCovergroup* cgp) ध्रुव {
        यदि (!m_embeddedVarp || !cgp->eventp()) प्रत्यागम असत्य;
        ध्रुव std::समुच्चयः<ध्रुव AstVar*> enclosingVars = enclosingInstanceVars();
        तार्किक प्राप्तम् = असत्य;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            यदि (enclosingVars.गणनम्(refp->varp())) प्राप्तम् = सत्य;
        });
        प्रत्यागम प्राप्तम्;
    }

    स्थिर तार्किक parse_Embedded_Event_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar*& आधारः_Varp,
                                       AstVar*& memberVarp) {
        यदि (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) {
            आधारः_Varp = refp->varp();
            memberVarp = शून्यसूचकः;
            प्रत्यागम सत्य;
        }
        AstMemberSel* ध्रुव selp = VN_CAST(exprp, MemberSel);
        यदि (!selp) प्रत्यागम असत्य;
        AstVarRef* ध्रुव आधारः_Refp = VN_CAST(selp->fromp(), VarRef);
        यदि (!आधारः_Refp) प्रत्यागम असत्य;
        आधारः_Varp = आधारः_Refp->varp();
        memberVarp = selp->varp();
        प्रत्यागम सत्य;
    }

    तार्किक isEventLvalue(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव EmbeddedEventTrigger& trigger) ध्रुव {
        यदि (AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* आधारः_Varp = शून्यसूचकः;
        AstVar* memberVarp = शून्यसूचकः;
        यदि (!parse_Embedded_Event_व्यञ्जकम्(exprp, आधारः_Varp, memberVarp)) प्रत्यागम असत्य;
        प्रत्यागम आधारः_Varp == trigger.आधारः_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEventRead(File_पङ्क्तिः* fl, ध्रुव EmbeddedEventTrigger& trigger) ध्रुव {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव basep = नूतन AstVarRef{fl, trigger.आधारः_Varp, VAccess::READ};
        यदि (!trigger.memberVarp) प्रत्यागम basep;
        AstMemberSel* ध्रुव selp = नूतन AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        प्रत्यागम selp;
    }

    string event_पूर्व_Name(ध्रुव EmbeddedEventTrigger& trigger, आकार_t trigger_सूचकांक) ध्रुव {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::गन्तव्यम्_string(trigger_सूचकांक)
                      + "_" + trigger.आधारः_Varp->name();
        यदि (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        प्रत्यागम name;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEmbeddedVarNonNull(File_पङ्क्तिः* fl) ध्रुव {
        प्रत्यागम नूतन AstNeq{fl, नूतन AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          नूतन AstConst{fl, AstConst::Null{}}};
    }

    Ast_ग्रन्थिः_Stmt* newSampleStmt(File_पङ्क्तिः* fl) ध्रुव {
        Ast_विधानम्_Call* ध्रुव callp = नूतन Ast_विधानम्_Call{
            fl, नूतन AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", शून्यसूचकः};
        callp->taskp(m_sampleFuncp);
        callp->dtype_समुच्चयः_Void();
        प्रत्यागम callp->makeStmt();
    }

    शून्य installEmbeddedEventFork(Ast_Sen_वृक्षः* eventp,
                                  ध्रुव std::सदिशम्<Ast_ग्रन्थिः_नियोजय*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        प्रति (Ast_ग्रन्थिः_नियोजय* ध्रुव constructp : constructps) {
            File_पङ्क्तिः* ध्रुव fl = constructp->fileline();
            AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
            loopp->सङ्कलय_Stmtsp(नूतन AstEventControl{fl, eventp->clone_वृक्षः(असत्य), शून्यसूचकः});
            loopp->सङ्कलय_Stmtsp(नूतन AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ध्रुव forkp = नूतन AstFork{fl, VJoinType::JOIN_NONE};
            forkp->तात्कालिकम्_Start(सत्य);
            forkp->सङ्कलय_Forksp(नूतन AstBegin{fl, "", loopp, सत्य});
            constructp->सङ्कलय_अग्र_अत्र(forkp);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(eventp), eventp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEventReadyCondition(File_पङ्क्तिः* fl, ध्रुव EmbeddedEventTrigger& trigger) ध्रुव {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव curp = newEventRead(fl, trigger);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prevp = नूतन AstVarRef{fl, trigger.पूर्व_Varp, VAccess::READ};
        Ast_ग्रन्थिः_व्यञ्जकम्* edgep = शून्यसूचकः;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        यदि (trigger.edgeType == VEdgeType::ET_POSEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstAnd{fl, curp, नूतन AstNot{fl, prevp}}, 0, 1};
        } अन्यथा यदि (trigger.edgeType == VEdgeType::ET_NEGEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstAnd{fl, नूतन AstNot{fl, curp}, prevp}, 0, 1};
        } अन्यथा यदि (trigger.edgeType == VEdgeType::ET_BOTHEDGE) {
            edgep = नूतन AstSel{fl, नूतन AstXor{fl, curp, prevp}, 0, 1};
        } अन्यथा {
            edgep = नूतन AstNeq{fl, curp, prevp};
        }
        प्रत्यागम नूतन AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::सदिशम्<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::सदिशम्<EmbeddedEventTrigger> triggers;
        ध्रुव std::समुच्चयः<ध्रुव AstVar*> enclosingVars = enclosingInstanceVars();
        प्रति (Ast_ग्रन्थिः* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            AstSenItem* ध्रुव itemp = VN_AS(senp, SenItem);
            AstVar* आधारः_Varp = शून्यसूचकः;
            AstVar* memberVarp = शून्यसूचकः;
            यदि (!parse_Embedded_Event_व्यञ्जकम्(itemp->sensp(), आधारः_Varp, memberVarp)
                || !enclosingVars.गणनम्(आधारः_Varp)) {
                प्रत्यागम {};
            }
            triggers.emplace_पृष्ठभागः(itemp->fileline(), आधारः_Varp, memberVarp, itemp->edgeType());
        }
        प्रत्यागम triggers;
    }

    शून्य installEmbeddedEventTriggers(std::सदिशम्<EmbeddedEventTrigger>& triggers,
                                      ध्रुव std::सदिशम्<Ast_ग्रन्थिः_नियोजय*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        यदि (constructps.रिक्तानि()) प्रत्यागम;
        प्रति (आकार_t trigger_सूचकांक = 0; trigger_सूचकांक < triggers.आकार(); ++trigger_सूचकांक) {
            EmbeddedEventTrigger& trigger = triggers[trigger_सूचकांक];
            std::सदिशम्<Ast_ग्रन्थिः_नियोजय*> assignps;
            m_enclosingClassp->foreach([&](Ast_ग्रन्थिः_नियोजय* asgnp) {
                यदि (isEventLvalue(asgnp->lhsp(), trigger)) assignps.प्रक्षिप_पृष्ठभागः(asgnp);
            });
            यदि (assignps.रिक्तानि()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                अनुवर्त;
            }
            Ast_ग्रन्थिः_DType* ध्रुव dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.आधारः_Varp->dtypep();
            AstVar* ध्रुव पूर्व_Varp = नूतन AstVar{trigger.eventFl, VVarType::MEMBER,
                                                event_पूर्व_Name(trigger, trigger_सूचकांक), dtypep};
            m_enclosingClassp->सङ्कलय_Membersp(पूर्व_Varp);
            trigger.पूर्व_Varp = पूर्व_Varp;
            प्रति (Ast_ग्रन्थिः_नियोजय* ध्रुव asgnp : assignps) {
                File_पङ्क्तिः* ध्रुव fl = asgnp->fileline();
                AstIf* ध्रुव ifp
                    = नूतन AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->सङ्कलय_अग्र_अत्र(नूतन Ast_नियोजय{fl,
                                               नूतन AstVarRef{fl, trigger.पूर्व_Varp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->सङ्कलय_अग्र_अत्र(ifp);
            }
        }
    }

    शून्य delete_Coverage_वस्तूनि() {
        प्रति (AstCoverpoint* ध्रुव cpp : m_coverpoints) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(cpp->unlink_Fr_पृष्ठभागः()), cpp);
        }
        प्रति (AstCoverCross* ध्रुव crossp : m_coverCrosses) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(crossp->unlink_Fr_पृष्ठभागः()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        प्रति (Ast_Cg_Option_नियोजय* ध्रुव optp : m_cgOptions) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(optp->unlink_Fr_पृष्ठभागः()), optp);
        }
        m_cgOptions.clear();
    }

    वर्ग FormalRefVisitor final : सार्वजनिक VNVisitor {
        ध्रुव std::map<ध्रुव AstVar*, AstVar*>& m_replacements;

        शून्य दर्शय(AstVarRef* nodep) override {
            ध्रुव स्वचालित it = m_replacements.find(nodep->varp());
            यदि (it == m_replacements.अन्तः_2()) प्रत्यागम;
            nodep->varp(it->second);
        }
        शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        स्पष्ट FormalRefVisitor(ध्रुव std::map<ध्रुव AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        शून्य अवलोकनम्(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
    };

    शून्य validateCovergroupExpressions() {
        std::समुच्चयः<ध्रुव AstVar*> sampleMembers;
        std::समुच्चयः<ध्रुव AstVar*> constructorRefMembers;
        प्रति (Ast_ग्रन्थिः* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            यदि (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) अनुवर्त;
            ध्रुव AstVar* ध्रुव memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.निवेशय(varp);
            constructorRefMembers.निवेशय(memberp);
        }
        प्रति (Ast_ग्रन्थिः* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            यदि (!varp || !varp->isIO()) अनुवर्त;
            ध्रुव AstVar* ध्रुव memberp
                = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.निवेशय(memberp);
        }
        Covergroup_व्यञ्जकम्_Valid_Visitor{sampleMembers, constructorRefMembers}.अवलोकनम्(m_constructorp);
    }

    शून्य rebindFormalRefs() {
        std::map<ध्रुव AstVar*, AstVar*> replacements;
        प्रति (Ast_ग्रन्थिः* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isIO()) अनुवर्त;
                AstVar* ध्रुव memberp
                    = VN_CAST(m_memberMap.findMember(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        आकार_t expectedBindings = 0;
        प्रति (ध्रुव स्वचालित& pair : replacements) {
            यदि (pair.first->isRef() || pair.first->isConstRef()) ++expectedBindings;
        }
        आकार_t rewrittenBindings = 0;
        प्रति (Ast_ग्रन्थिः* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
            यदि (Ast_नियोजय* ध्रुव assignp = VN_CAST(stmtp, नियोजय)) {
                AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ध्रुव rhsp = VN_CAST(assignp->rhsp(), VarRef);
                यदि (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ध्रुव स्वचालित it = replacements.find(rhsp->varp());
                    UASSERT_OBJ(it != replacements.अन्तः_2() && it->second == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ध्रुव bindp = नूतन AstCExpr{assignp->fileline(), ""};
                    bindp->सङ्कलय(lhsp->unlink_Fr_पृष्ठभागः());
                    bindp->सङ्कलय(" = &");
                    bindp->सङ्कलय(rhsp->unlink_Fr_पृष्ठभागः());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        प्रति (AstCoverpoint* ध्रुव cpp : m_coverpoints) visitor.अवलोकनम्(cpp);
        प्रति (AstCoverCross* ध्रुव crossp : m_coverCrosses) visitor.अवलोकनम्(crossp);
    }

    AstVarRef* install_Enclosing_पृष्ठभागः_Pointer(ध्रुव std::सदिशम्<Ast_ग्रन्थिः_नियोजय*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        यदि (!m_enclosingClassp) प्रत्यागम शून्यसूचकः;  // Offending refs require an enclosing class

        AstVarRef* invalidp = शून्यसूचकः;
        Ast_ग्रन्थिः* offenderp = शून्यसूचकः;
        std::समुच्चयः<ध्रुव AstVar*> ownVars;
        प्रति (Ast_ग्रन्थिः* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(itemp, Var)) ownVars.निवेशय(varp);
        }
        ध्रुव std::समुच्चयः<ध्रुव AstVar*> enclosingVars = enclosingInstanceVars();
        std::सदिशम्<AstVarRef*> refs_गन्तव्यम्_Rewrite;
        std::सदिशम्<AstThisRef*> this_Refs_गन्तव्यम्_Rewrite;
        std::सदिशम्<AstFuncRef*> func_Refs_गन्तव्यम्_Rewrite;
        ध्रुव स्वचालित अवलोकनम् = [&](Ast_ग्रन्थिः* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                यदि (invalidp) प्रत्यागम;
                ध्रुव AstVar* ध्रुव varp = refp->varp();
                यदि (!isEnclosingInstanceVar(varp) || ownVars.गणनम्(varp)) प्रत्यागम;
                यदि (!enclosingVars.गणनम्(varp)) {
                    invalidp = refp;
                    प्रत्यागम;
                }
                refs_गन्तव्यम्_Rewrite.प्रक्षिप_पृष्ठभागः(refp);
                यदि (!offenderp) offenderp = refp;
            });
            यदि (invalidp) प्रत्यागम;
            rootp->foreach([&](AstThisRef* refp) {
                ध्रुव AstClassRefDType* ध्रुव refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                यदि (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_गन्तव्यम्_Rewrite.प्रक्षिप_पृष्ठभागः(refp);
                    यदि (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                यदि (!isEnclosingInstanceFunc(refp->taskp())) प्रत्यागम;
                func_Refs_गन्तव्यम्_Rewrite.प्रक्षिप_पृष्ठभागः(refp);
                यदि (!offenderp) offenderp = refp;
            });
        };
        प्रति (AstCoverpoint* ध्रुव cpp : m_coverpoints) अवलोकनम्(cpp);
        प्रति (AstCoverCross* ध्रुव crossp : m_coverCrosses) अवलोकनम्(crossp);
        प्रति (Ast_Cg_Option_नियोजय* ध्रुव optp : m_cgOptions) अवलोकनम्(optp);
        यदि (invalidp || !offenderp) प्रत्यागम invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_पङ्क्तिः* ध्रुव fl = m_covergroupp->fileline();
        AstClassRefDType* ध्रुव enclDTypep = नूतन AstClassRefDType{fl, m_enclosingClassp, शून्यसूचकः};
        enclDTypep->rawPointer(सत्य);
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(enclDTypep);
        AstVar* ध्रुव handleVarp
            = नूतन AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->सङ्कलय_Membersp(handleVarp);
        AstVar* ध्रुव argumentp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::निवेशः);
        argumentp->declDirection(VDirection::निवेशः);
        argumentp->funcLocal(सत्य);
        argumentp->noReset(सत्य);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->सङ्कलय_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(नूतन Ast_नियोजय{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    नूतन AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        प्रति (AstVarRef* ध्रुव refp : refs_गन्तव्यम्_Rewrite) { rewriteVarRef(refp, handleVarp); }
        प्रति (AstThisRef* ध्रुव refp : this_Refs_गन्तव्यम्_Rewrite) { rewriteThisRef(refp, handleVarp); }
        प्रति (AstFuncRef* ध्रुव refp : func_Refs_गन्तव्यम्_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        प्रति (Ast_ग्रन्थिः_नियोजय* ध्रुव constructp : constructps) {
            File_पङ्क्तिः* ध्रुव cfl = constructp->fileline();
            AstCExpr* ध्रुव thisp = नूतन AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->सङ्कलय_Argsp(नूतन AstArg{cfl, argumentp->name(), thisp});
        }
        प्रत्यागम शून्यसूचकः;
    }

    शून्य दर्शय(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        यदि (nodep->isCovergroup()) {
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
            m_embeddedVarp = findEmbeddedCovergroupVar();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = शून्यसूचकः;
            m_constructorp = शून्यसूचकः;
            std::सदिशम्<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_वृक्षः* embeddedEventForkp = शून्यसूचकः;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            तार्किक hasUnsupportedEvent = असत्य;
            प्रति (Ast_ग्रन्थिः* itemp = nodep->membersp(); itemp;) {
                Ast_ग्रन्थिः* ध्रुव nextp = itemp->nextp();
                यदि (AstCovergroup* ध्रुव cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    यदि (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_पृष्ठभागः();
                        } अन्यथा {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            यदि (embeddedEventTriggers.रिक्तानि()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = सत्य;
                            }
                        }
                        VL_DO_DANGLING(प्रक्षिप_Deletep(cgp->unlink_Fr_पृष्ठभागः()), cgp);
                        itemp = nextp;
                        अनुवर्त;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    अनुवर्त;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.findMember(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(सत्य);
            m_constructorp = VN_CAST(m_memberMap.findMember(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            यदि (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_वस्तूनि();
                यदि (embeddedEventForkp) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                प्रत्यागम;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ध्रुव std::सदिशम्<Ast_ग्रन्थिः_नियोजय*> constructps = findCovergroupConstructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            यदि (AstVarRef* ध्रुव invalidp = install_Enclosing_पृष्ठभागः_Pointer(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_वस्तूनि();
                यदि (embeddedEventForkp) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                प्रत्यागम;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            यदि (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_वस्तूनि();
        } अन्यथा {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    शून्य दर्शय(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.प्रक्षिप_पृष्ठभागः(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.प्रक्षिप_पृष्ठभागः(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    शून्य दर्शय(Ast_Cg_Option_नियोजय* nodep) override { m_cgOptions.प्रक्षिप_पृष्ठभागः(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ध्रुव std::समुच्चयः<ध्रुव AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = स्वभाव;
};

//######################################################################
// Functional coverage class functions

शून्य V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ध्रुव Covergroup_नियोजय_Valid_Visitor validVisitor{nodep};
    यदि (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_वृक्षः("coveragefunc", 0, dump_वृक्षः_Either_Level() >= 3);
}
