// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Command line option parser
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

#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
#define VL_MT_DISABLED_CODE_UNIT 1

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Options.h"
#endif
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3OptionParser.h"
#অন্তর্ভাব "V3String.h"

//######################################################################
// V3OptionParser::Impl
কাঠামো V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    গণনা শ্রেণী en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        মান_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, যৌক্তিক N_Allow_Partial_Match = মিথ্যা>
    শ্রেণী ActionBase VL_NOT_FINAL : সর্বজনীন ActionIfs {
        যৌক্তিক m_notForRerun = মিথ্যা;  // This option is not needed for rerun with --dump-inputs
        যৌক্তিক m_undocumented = মিথ্যা;  // This option is not documented
    সর্বজনীন:
        যৌক্তিক is_মান_Needed() ধ্রুবক override final { ফেরত N_Mode == en::মান_3; }
        যৌক্তিক isFOnOffAllowed() ধ্রুবক override final { ফেরত N_Mode == en::FONOFF; }
        যৌক্তিক isNotForRerun() ধ্রুবক override final { ফেরত m_notForRerun; }
        যৌক্তিক isOnOffAllowed() ধ্রুবক override final { ফেরত N_Mode == en::ONOFF; }
        যৌক্তিক isPartialMatchAllowed() ধ্রুবক override final { ফেরত N_Allow_Partial_Match; }
        যৌক্তিক isUndocumented() ধ্রুবক override { ফেরত m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = সত্য;
            ফেরত *এই;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = সত্য;
            ফেরত *এই;
        }
    };

    // Actual action classes
    template <typename T>
    শ্রেণী ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    শ্রেণী ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    শ্রেণী ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    শ্রেণী ActionCbCall;  // Callback without argument for "-opt"
    শ্রেণী ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    শ্রেণী ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    শ্রেণী ActionCbVal;  // Callback for "-opt val"
    শ্রেণী ActionCbPartialMatch;  // Callback "-O3" for "-O"
    শ্রেণী ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ধ্রুবক string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    যৌক্তিক m_isFinalized{মিথ্যা};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    শ্রেণী V3OptionParser::Impl::className<type> final : সর্বজনীন ActionBase<enType> { \
        type* ধ্রুবক m_valp; /* Pointer to an option variable*/ \
\
    সর্বজনীন: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        শূন্য exec(ধ্রুবক অক্ষর* optp, ধ্রুবক অক্ষর* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, যৌক্তিক, *m_valp = সত্য, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(সত্য), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, পূর্ণসংখ্যা, *m_valp = std::atoi(argp), en::মান_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::মান_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, যৌক্তিক, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, যৌক্তিক, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    শ্রেণী V3OptionParser::Impl::className final : সর্বজনীন ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    সর্বজনীন: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        শূন্য exec(ধ্রুবক অক্ষর* optp, ধ্রুবক অক্ষর* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, শূন্য(শূন্য), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, শূন্য(যৌক্তিক), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, শূন্য(যৌক্তিক), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, শূন্য(ধ্রুবক অক্ষর*), m_cb(optp), en::NONE,
                                 সত্য);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, শূন্য(ধ্রুবক অক্ষর*, ধ্রুবক অক্ষর*),
                                 m_cb(optp, argp), en::মান_3, সত্য);

template <>
শ্রেণী V3OptionParser::Impl::ActionCbVal<পূর্ণসংখ্যা> final : সর্বজনীন ActionBase<en::মান_3> {
    std::function<শূন্য(পূর্ণসংখ্যা)> m_cb;  // Callback function

সর্বজনীন:
    using CbType = std::function<শূন্য(পূর্ণসংখ্যা)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    শূন্য exec(ধ্রুবক অক্ষর* optp, ধ্রুবক অক্ষর* argp) override { m_cb(std::atoi(argp)); }
};

template <>
শ্রেণী V3OptionParser::Impl::ActionCbVal<ধ্রুবক অক্ষর*> final : সর্বজনীন ActionBase<en::মান_3> {
    std::function<শূন্য(ধ্রুবক অক্ষর*)> m_cb;

সর্বজনীন:
    using CbType = std::function<শূন্য(ধ্রুবক অক্ষর*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    শূন্য exec(ধ্রুবক অক্ষর* optp, ধ্রুবক অক্ষর* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ধ্রুবক অক্ষর* optp) {
    ধ্রুবক auto it = m_pimpl->m_options.find(optp);
    যদি (it != m_pimpl->m_options.end()) ফেরত it->second.get();  // Exact match
    জন্য (auto&& act : m_pimpl->m_options) {
        যদি (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            যদি (ধ্রুবক অক্ষর* ধ্রুবক nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                যদি (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    ফেরত act.second.get();
                }
            }
        }
        যদি (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            যদি (ধ্রুবক অক্ষর* ধ্রুবক nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                যদি (act.first == nop || act.first == ("-"s + nop)) ফেরত act.second.get();
            }
        } অন্যথা যদি (act.second->isPartialMatchAllowed()) {
            যদি (VString::startsWith(optp, act.first)) ফেরত act.second.get();
        }
    }
    ফেরত nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ধ্রুবক std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{নতুন T_Act{std::move(arg)}};
    UASSERT(opt.আকার_2() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ধ্রুবক auto inserted_ফলাফল = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_ফলাফল.second, opt << " is already registered");
    ফেরত *inserted_ফলাফল.first->second;
}

যৌক্তিক V3OptionParser::hasPrefixFNo(ধ্রুবক অক্ষর* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    যদি (strp[1] == '-') ++strp;
    ফেরত VString::startsWith(strp, "-fno");
}

যৌক্তিক V3OptionParser::hasPrefixNo(ধ্রুবক অক্ষর* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    যদি (strp[1] == '-') ++strp;
    ফেরত VString::startsWith(strp, "-no");
}

std::pair<পূর্ণসংখ্যা, যৌক্তিক> V3OptionParser::parse(পূর্ণসংখ্যা idx, পূর্ণসংখ্যা argc, অক্ষর* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ধ্রুবক অক্ষর* optp = argv[idx];
    যদি (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ধ্রুবক actp = find(optp);
    যদি (!actp) ফেরত {0, মিথ্যা};
    যদি (!actp->is_মান_Needed()) {
        actp->exec(optp, nullptr);
        ফেরত {1, !actp->isNotForRerun()};
    }
    যদি (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        ফেরত {2, !actp->isNotForRerun()};
    }
    ফেরত {0, মিথ্যা};
}

string V3OptionParser::getSuggestion(ধ্রুবক অক্ষর* str) ধ্রুবক {
    ফেরত m_pimpl->m_spellCheck.best_Candidate_বার্তা(str);
}

শূন্য V3OptionParser::addSuggestionCandidate(ধ্রুবক string& s) {
    m_pimpl->m_spellCheck.ঠেলো_Candidate(s);
}

শূন্য V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    জন্য (auto&& opt : m_pimpl->m_options) {
        যদি (opt.second->isUndocumented()) চালু;
        m_pimpl->m_spellCheck.ঠেলো_Candidate(opt.first);
        যদি (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ঠেলো_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        যদি (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ঠেলো_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = সত্য;
}

শূন্য V3OptionParser::dumpOptions() {
    জন্য (ধ্রুবক auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{নতুন Impl{}} {}

V3OptionParser::~V3OptionParser() = পূর্বনির্ধারিত;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ধ্রুবক অক্ষর* optp, actKind, argType arg) ধ্রুবক { \
        ফেরত m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, যৌক্তিক*, ActionSet<যৌক্তিক>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, পূর্ণসংখ্যা*, ActionSet<পূর্ণসংখ্যা>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, যৌক্তিক*, ActionFOnOff<যৌক্তিক>)
V3OPTION_PARSER_DEF_OP(OnOff, যৌক্তিক*, ActionOnOff<যৌক্তিক>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<পূর্ণসংখ্যা>::CbType, ActionCbVal<পূর্ণসংখ্যা>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ধ্রুবক অক্ষর*>::CbType, ActionCbVal<ধ্রুবক অক্ষর*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ধ্রুবক অক্ষর* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ধ্রুবক {
    ধ্রুবক আকার_t prefixLen = std::strlen(optp);
    ধ্রুবক auto wrap = [prefixLen, cb](ধ্রুবক অক্ষর* optp) { cb(optp + prefixLen); };
    ফেরত m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ধ্রুবক অক্ষর* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ধ্রুবক {
    ধ্রুবক আকার_t prefixLen = std::strlen(optp);
    ধ্রুবক auto wrap
        = [prefixLen, cb](ধ্রুবক অক্ষর* optp, ধ্রুবক অক্ষর* argp) { cb(optp + prefixLen, argp); };
    ফেরত m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
