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

#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Options.h"
#endif
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3OptionParser.h"
#অন্তর্ভাৱ "V3String.h"

//######################################################################
// V3OptionParser::Impl
সংরচনা V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    গণনা শ্ৰেণী en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        মান_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, তার্কিক N_Allow_Partial_Match = মিছা>
    শ্ৰেণী ActionBase VL_NOT_FINAL : সার্ৱজনিক ActionIfs {
        তার্কিক m_notForRerun = মিছা;  // This option is not needed for rerun with --dump-inputs
        তার্কিক m_undocumented = মিছা;  // This option is not documented
    সার্ৱজনিক:
        তার্কিক is_মান_Needed() ধ্রুৱ override final { ঘূৰাই N_Mode == en::মান_3; }
        তার্কিক isFOnOffAllowed() ধ্রুৱ override final { ঘূৰাই N_Mode == en::FONOFF; }
        তার্কিক isNotForRerun() ধ্রুৱ override final { ঘূৰাই m_notForRerun; }
        তার্কিক isOnOffAllowed() ধ্রুৱ override final { ঘূৰাই N_Mode == en::ONOFF; }
        তার্কিক isPartialMatchAllowed() ধ্রুৱ override final { ঘূৰাই N_Allow_Partial_Match; }
        তার্কিক isUndocumented() ধ্রুৱ override { ঘূৰাই m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = সঁচা;
            ঘূৰাই *এতত্;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = সঁচা;
            ঘূৰাই *এতত্;
        }
    };

    // Actual action classes
    template <typename T>
    শ্ৰেণী ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    শ্ৰেণী ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    শ্ৰেণী ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    শ্ৰেণী ActionCbCall;  // Callback without argument for "-opt"
    শ্ৰেণী ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    শ্ৰেণী ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    শ্ৰেণী ActionCbVal;  // Callback for "-opt val"
    শ্ৰেণী ActionCbPartialMatch;  // Callback "-O3" for "-O"
    শ্ৰেণী ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ধ্রুৱ string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    তার্কিক m_isFinalized{মিছা};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    শ্ৰেণী V3OptionParser::Impl::className<type> final : সার্ৱজনিক ActionBase<enType> { \
        type* ধ্রুৱ m_valp; /* Pointer to an option variable*/ \
\
    সার্ৱজনিক: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        শূন্য exec(ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, তার্কিক, *m_valp = সঁচা, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(সঁচা), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, পূর্ণাঙ্ক, *m_valp = std::atoi(argp), en::মান_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::মান_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, তার্কিক, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, তার্কিক, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    শ্ৰেণী V3OptionParser::Impl::className final : সার্ৱজনিক ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    সার্ৱজনিক: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        শূন্য exec(ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, শূন্য(শূন্য), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, শূন্য(তার্কিক), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, শূন্য(তার্কিক), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, শূন্য(ধ্রুৱ ৱর্ণ*), m_cb(optp), en::NONE,
                                 সঁচা);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, শূন্য(ধ্রুৱ ৱর্ণ*, ধ্রুৱ ৱর্ণ*),
                                 m_cb(optp, argp), en::মান_3, সঁচা);

template <>
শ্ৰেণী V3OptionParser::Impl::ActionCbVal<পূর্ণাঙ্ক> final : সার্ৱজনিক ActionBase<en::মান_3> {
    std::function<শূন্য(পূর্ণাঙ্ক)> m_cb;  // Callback function

সার্ৱজনিক:
    using CbType = std::function<শূন্য(পূর্ণাঙ্ক)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    শূন্য exec(ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* argp) override { m_cb(std::atoi(argp)); }
};

template <>
শ্ৰেণী V3OptionParser::Impl::ActionCbVal<ধ্রুৱ ৱর্ণ*> final : সার্ৱজনিক ActionBase<en::মান_3> {
    std::function<শূন্য(ধ্রুৱ ৱর্ণ*)> m_cb;

সার্ৱজনিক:
    using CbType = std::function<শূন্য(ধ্রুৱ ৱর্ণ*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    শূন্য exec(ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ধ্রুৱ ৱর্ণ* optp) {
    ধ্রুৱ auto it = m_pimpl->m_options.find(optp);
    যদি (it != m_pimpl->m_options.end()) ঘূৰাই it->second.get();  // Exact match
    বাবে (auto&& act : m_pimpl->m_options) {
        যদি (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            যদি (ধ্রুৱ ৱর্ণ* ধ্রুৱ nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                যদি (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    ঘূৰাই act.second.get();
                }
            }
        }
        যদি (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            যদি (ধ্রুৱ ৱর্ণ* ধ্রুৱ nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                যদি (act.first == nop || act.first == ("-"s + nop)) ঘূৰাই act.second.get();
            }
        } অন্যথা যদি (act.second->isPartialMatchAllowed()) {
            যদি (VString::startsWith(optp, act.first)) ঘূৰাই act.second.get();
        }
    }
    ঘূৰাই nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ধ্রুৱ std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{নূতন T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ধ্রুৱ auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    ঘূৰাই *insertedResult.first->second;
}

তার্কিক V3OptionParser::hasPrefixFNo(ধ্রুৱ ৱর্ণ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    যদি (strp[1] == '-') ++strp;
    ঘূৰাই VString::startsWith(strp, "-fno");
}

তার্কিক V3OptionParser::hasPrefixNo(ধ্রুৱ ৱর্ণ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    যদি (strp[1] == '-') ++strp;
    ঘূৰাই VString::startsWith(strp, "-no");
}

std::pair<পূর্ণাঙ্ক, তার্কিক> V3OptionParser::parse(পূর্ণাঙ্ক idx, পূর্ণাঙ্ক argc, ৱর্ণ* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ধ্রুৱ ৱর্ণ* optp = argv[idx];
    যদি (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ধ্রুৱ actp = find(optp);
    যদি (!actp) ঘূৰাই {0, মিছা};
    যদি (!actp->is_মান_Needed()) {
        actp->exec(optp, nullptr);
        ঘূৰাই {1, !actp->isNotForRerun()};
    }
    যদি (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        ঘূৰাই {2, !actp->isNotForRerun()};
    }
    ঘূৰাই {0, মিছা};
}

string V3OptionParser::getSuggestion(ধ্রুৱ ৱর্ণ* str) ধ্রুৱ {
    ঘূৰাই m_pimpl->m_spellCheck.best_Candidate_বাৰ্তা(str);
}

শূন্য V3OptionParser::addSuggestionCandidate(ধ্রুৱ string& s) {
    m_pimpl->m_spellCheck.ঠেলো_Candidate(s);
}

শূন্য V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    বাবে (auto&& opt : m_pimpl->m_options) {
        যদি (opt.second->isUndocumented()) অনুৱর্ত;
        m_pimpl->m_spellCheck.ঠেলো_Candidate(opt.first);
        যদি (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ঠেলো_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        যদি (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ঠেলো_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = সঁচা;
}

শূন্য V3OptionParser::dumpOptions() {
    বাবে (ধ্রুৱ auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{নূতন Impl{}} {}

V3OptionParser::~V3OptionParser() = স্ৱভাৱ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ধ্রুৱ ৱর্ণ* optp, actKind, argType arg) ধ্রুৱ { \
        ঘূৰাই m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, তার্কিক*, ActionSet<তার্কিক>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, পূর্ণাঙ্ক*, ActionSet<পূর্ণাঙ্ক>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, তার্কিক*, ActionFOnOff<তার্কিক>)
V3OPTION_PARSER_DEF_OP(OnOff, তার্কিক*, ActionOnOff<তার্কিক>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<পূর্ণাঙ্ক>::CbType, ActionCbVal<পূর্ণাঙ্ক>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ধ্রুৱ ৱর্ণ*>::CbType, ActionCbVal<ধ্রুৱ ৱর্ণ*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ধ্রুৱ ৱর্ণ* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ধ্রুৱ {
    ধ্রুৱ size_t prefixLen = std::strlen(optp);
    ধ্রুৱ auto wrap = [prefixLen, cb](ধ্রুৱ ৱর্ণ* optp) { cb(optp + prefixLen); };
    ঘূৰাই m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ধ্রুৱ ৱর্ণ* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ধ্রুৱ {
    ধ্রুৱ size_t prefixLen = std::strlen(optp);
    ধ্রুৱ auto wrap
        = [prefixLen, cb](ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* argp) { cb(optp + prefixLen, argp); };
    ঘূৰাই m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
