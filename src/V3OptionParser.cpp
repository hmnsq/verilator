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
সংরচনা V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    গণনা বর্গ en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        মান_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, তার্কিক N_Allow_Partial_Match = অসত্য>
    বর্গ ActionBase VL_NOT_FINAL : সার্বজনিক ActionIfs {
        তার্কিক m_notForRerun = অসত্য;  // This option is not needed for rerun with --dump-inputs
        তার্কিক m_undocumented = অসত্য;  // This option is not documented
    সার্বজনিক:
        তার্কিক is_মান_Needed() ধ্রুব override final { প্রত্যাগম N_Mode == en::মান_3; }
        তার্কিক isFOnOffAllowed() ধ্রুব override final { প্রত্যাগম N_Mode == en::FONOFF; }
        তার্কিক isNotForRerun() ধ্রুব override final { প্রত্যাগম m_notForRerun; }
        তার্কিক isOnOffAllowed() ধ্রুব override final { প্রত্যাগম N_Mode == en::ONOFF; }
        তার্কিক isPartialMatchAllowed() ধ্রুব override final { প্রত্যাগম N_Allow_Partial_Match; }
        তার্কিক isUndocumented() ধ্রুব override { প্রত্যাগম m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = সত্য;
            প্রত্যাগম *এতত্;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = সত্য;
            প্রত্যাগম *এতত্;
        }
    };

    // Actual action classes
    template <typename T>
    বর্গ ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    বর্গ ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    বর্গ ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    বর্গ ActionCbCall;  // Callback without argument for "-opt"
    বর্গ ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    বর্গ ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    বর্গ ActionCbVal;  // Callback for "-opt val"
    বর্গ ActionCbPartialMatch;  // Callback "-O3" for "-O"
    বর্গ ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ধ্রুব string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    তার্কিক m_isFinalized{অসত্য};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    বর্গ V3OptionParser::Impl::className<type> final : সার্বজনিক ActionBase<enType> { \
        type* ধ্রুব m_valp; /* Pointer to an option variable*/ \
\
    সার্বজনিক: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        শূন্য exec(ধ্রুব বর্ণ* optp, ধ্রুব বর্ণ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, তার্কিক, *m_valp = সত্য, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(সত্য), en::NONE);
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
    বর্গ V3OptionParser::Impl::className final : সার্বজনিক ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    সার্বজনিক: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        শূন্য exec(ধ্রুব বর্ণ* optp, ধ্রুব বর্ণ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, শূন্য(শূন্য), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, শূন্য(তার্কিক), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, শূন্য(তার্কিক), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, শূন্য(ধ্রুব বর্ণ*), m_cb(optp), en::NONE,
                                 সত্য);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, শূন্য(ধ্রুব বর্ণ*, ধ্রুব বর্ণ*),
                                 m_cb(optp, argp), en::মান_3, সত্য);

template <>
বর্গ V3OptionParser::Impl::ActionCbVal<পূর্ণাঙ্ক> final : সার্বজনিক ActionBase<en::মান_3> {
    std::function<শূন্য(পূর্ণাঙ্ক)> m_cb;  // Callback function

সার্বজনিক:
    using CbType = std::function<শূন্য(পূর্ণাঙ্ক)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    শূন্য exec(ধ্রুব বর্ণ* optp, ধ্রুব বর্ণ* argp) override { m_cb(std::atoi(argp)); }
};

template <>
বর্গ V3OptionParser::Impl::ActionCbVal<ধ্রুব বর্ণ*> final : সার্বজনিক ActionBase<en::মান_3> {
    std::function<শূন্য(ধ্রুব বর্ণ*)> m_cb;

সার্বজনিক:
    using CbType = std::function<শূন্য(ধ্রুব বর্ণ*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    শূন্য exec(ধ্রুব বর্ণ* optp, ধ্রুব বর্ণ* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ধ্রুব বর্ণ* optp) {
    ধ্রুব auto it = m_pimpl->m_options.find(optp);
    যদি (it != m_pimpl->m_options.end()) প্রত্যাগম it->second.get();  // Exact match
    প্রতি (auto&& act : m_pimpl->m_options) {
        যদি (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            যদি (ধ্রুব বর্ণ* ধ্রুব nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                যদি (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    প্রত্যাগম act.second.get();
                }
            }
        }
        যদি (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            যদি (ধ্রুব বর্ণ* ধ্রুব nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                যদি (act.first == nop || act.first == ("-"s + nop)) প্রত্যাগম act.second.get();
            }
        } অন্যথা যদি (act.second->isPartialMatchAllowed()) {
            যদি (VString::startsWith(optp, act.first)) প্রত্যাগম act.second.get();
        }
    }
    প্রত্যাগম nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ধ্রুব std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{নূতন T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ধ্রুব auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    প্রত্যাগম *insertedResult.first->second;
}

তার্কিক V3OptionParser::hasPrefixFNo(ধ্রুব বর্ণ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    যদি (strp[1] == '-') ++strp;
    প্রত্যাগম VString::startsWith(strp, "-fno");
}

তার্কিক V3OptionParser::hasPrefixNo(ধ্রুব বর্ণ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    যদি (strp[1] == '-') ++strp;
    প্রত্যাগম VString::startsWith(strp, "-no");
}

std::pair<পূর্ণাঙ্ক, তার্কিক> V3OptionParser::parse(পূর্ণাঙ্ক idx, পূর্ণাঙ্ক argc, বর্ণ* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ধ্রুব বর্ণ* optp = argv[idx];
    যদি (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ধ্রুব actp = find(optp);
    যদি (!actp) প্রত্যাগম {0, অসত্য};
    যদি (!actp->is_মান_Needed()) {
        actp->exec(optp, nullptr);
        প্রত্যাগম {1, !actp->isNotForRerun()};
    }
    যদি (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        প্রত্যাগম {2, !actp->isNotForRerun()};
    }
    প্রত্যাগম {0, অসত্য};
}

string V3OptionParser::getSuggestion(ধ্রুব বর্ণ* str) ধ্রুব {
    প্রত্যাগম m_pimpl->m_spellCheck.best_Candidate_বার্তা(str);
}

শূন্য V3OptionParser::addSuggestionCandidate(ধ্রুব string& s) {
    m_pimpl->m_spellCheck.ঠেলো_Candidate(s);
}

শূন্য V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    প্রতি (auto&& opt : m_pimpl->m_options) {
        যদি (opt.second->isUndocumented()) অনুবর্ত;
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
    প্রতি (ধ্রুব auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{নূতন Impl{}} {}

V3OptionParser::~V3OptionParser() = স্বভাব;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ধ্রুব বর্ণ* optp, actKind, argType arg) ধ্রুব { \
        প্রত্যাগম m_parser.add<Impl::actType>(optp, arg); \
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
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ধ্রুব বর্ণ*>::CbType, ActionCbVal<ধ্রুব বর্ণ*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ধ্রুব বর্ণ* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ধ্রুব {
    ধ্রুব size_t prefixLen = std::strlen(optp);
    ধ্রুব auto wrap = [prefixLen, cb](ধ্রুব বর্ণ* optp) { cb(optp + prefixLen); };
    প্রত্যাগম m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ধ্রুব বর্ণ* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ধ্রুব {
    ধ্রুব size_t prefixLen = std::strlen(optp);
    ধ্রুব auto wrap
        = [prefixLen, cb](ধ্রুব বর্ণ* optp, ধ্রুব বর্ণ* argp) { cb(optp + prefixLen, argp); };
    প্রত্যাগম m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
