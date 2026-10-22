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

#شامل "V3Global.h"
#شامل "V3Options.h"
#endif
#شامل "V3Error.h"
#شامل "V3OptionParser.h"
#شامل "V3String.h"

//######################################################################
// V3OptionParser::Impl
ساختار V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    شمارش رده en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        مقدار_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, منطقی N_Allow_Partial_Match = نادرست>
    رده ActionBase VL_NOT_FINAL : عمومی ActionIfs {
        منطقی m_notForRerun = نادرست;  // This option is not needed for rerun with --dump-inputs
        منطقی m_undocumented = نادرست;  // This option is not documented
    عمومی:
        منطقی is_مقدار_Needed() ثابت override final { بازگشت N_Mode == en::مقدار_3; }
        منطقی isFOnOffAllowed() ثابت override final { بازگشت N_Mode == en::FONOFF; }
        منطقی isNotForRerun() ثابت override final { بازگشت m_notForRerun; }
        منطقی isOnOffAllowed() ثابت override final { بازگشت N_Mode == en::ONOFF; }
        منطقی isPartialMatchAllowed() ثابت override final { بازگشت N_Allow_Partial_Match; }
        منطقی isUndocumented() ثابت override { بازگشت m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = درست;
            بازگشت *این;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = درست;
            بازگشت *این;
        }
    };

    // Actual action classes
    template <typename T>
    رده ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    رده ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    رده ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    رده ActionCbCall;  // Callback without argument for "-opt"
    رده ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    رده ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    رده ActionCbVal;  // Callback for "-opt val"
    رده ActionCbPartialMatch;  // Callback "-O3" for "-O"
    رده ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقی m_isFinalized{نادرست};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    رده V3OptionParser::Impl::className<type> final : عمومی ActionBase<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عمومی: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        تهی exec(ثابت نویسه* optp, ثابت نویسه* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, منطقی, *m_valp = درست, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(درست), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, صحیح, *m_valp = std::atoi(argp), en::مقدار_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::مقدار_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقی, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقی, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    رده V3OptionParser::Impl::className final : عمومی ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عمومی: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        تهی exec(ثابت نویسه* optp, ثابت نویسه* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, تهی(تهی), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, تهی(منطقی), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, تهی(منطقی), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, تهی(ثابت نویسه*), m_cb(optp), en::NONE,
                                 درست);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, تهی(ثابت نویسه*, ثابت نویسه*),
                                 m_cb(optp, argp), en::مقدار_3, درست);

template <>
رده V3OptionParser::Impl::ActionCbVal<صحیح> final : عمومی ActionBase<en::مقدار_3> {
    std::function<تهی(صحیح)> m_cb;  // Callback function

عمومی:
    using CbType = std::function<تهی(صحیح)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    تهی exec(ثابت نویسه* optp, ثابت نویسه* argp) override { m_cb(std::atoi(argp)); }
};

template <>
رده V3OptionParser::Impl::ActionCbVal<ثابت نویسه*> final : عمومی ActionBase<en::مقدار_3> {
    std::function<تهی(ثابت نویسه*)> m_cb;

عمومی:
    using CbType = std::function<تهی(ثابت نویسه*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    تهی exec(ثابت نویسه* optp, ثابت نویسه* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ثابت نویسه* optp) {
    ثابت auto it = m_pimpl->m_options.find(optp);
    اگر (it != m_pimpl->m_options.end()) بازگشت it->second.get();  // Exact match
    برای (auto&& act : m_pimpl->m_options) {
        اگر (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            اگر (ثابت نویسه* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                اگر (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    بازگشت act.second.get();
                }
            }
        }
        اگر (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            اگر (ثابت نویسه* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                اگر (act.first == nop || act.first == ("-"s + nop)) بازگشت act.second.get();
            }
        } وگرنه اگر (act.second->isPartialMatchAllowed()) {
            اگر (VString::startsWith(optp, act.first)) بازگشت act.second.get();
        }
    }
    بازگشت nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{جدید T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    بازگشت *insertedResult.first->second;
}

منطقی V3OptionParser::hasPrefixFNo(ثابت نویسه* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    اگر (strp[1] == '-') ++strp;
    بازگشت VString::startsWith(strp, "-fno");
}

منطقی V3OptionParser::hasPrefixNo(ثابت نویسه* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    اگر (strp[1] == '-') ++strp;
    بازگشت VString::startsWith(strp, "-no");
}

std::pair<صحیح, منطقی> V3OptionParser::parse(صحیح idx, صحیح argc, نویسه* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت نویسه* optp = argv[idx];
    اگر (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = find(optp);
    اگر (!actp) بازگشت {0, نادرست};
    اگر (!actp->is_مقدار_Needed()) {
        actp->exec(optp, nullptr);
        بازگشت {1, !actp->isNotForRerun()};
    }
    اگر (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        بازگشت {2, !actp->isNotForRerun()};
    }
    بازگشت {0, نادرست};
}

string V3OptionParser::getSuggestion(ثابت نویسه* str) ثابت {
    بازگشت m_pimpl->m_spellCheck.best_Candidate_پیام(str);
}

تهی V3OptionParser::addSuggestionCandidate(ثابت string& s) {
    m_pimpl->m_spellCheck.افزودن_Candidate(s);
}

تهی V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    برای (auto&& opt : m_pimpl->m_options) {
        اگر (opt.second->isUndocumented()) ادامه;
        m_pimpl->m_spellCheck.افزودن_Candidate(opt.first);
        اگر (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.افزودن_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        اگر (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.افزودن_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = درست;
}

تهی V3OptionParser::dumpOptions() {
    برای (ثابت auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{جدید Impl{}} {}

V3OptionParser::~V3OptionParser() = پیشفرض;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ثابت نویسه* optp, actKind, argType arg) ثابت { \
        بازگشت m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, منطقی*, ActionSet<منطقی>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, صحیح*, ActionSet<صحیح>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقی*, ActionFOnOff<منطقی>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقی*, ActionOnOff<منطقی>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحیح>::CbType, ActionCbVal<صحیح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت نویسه*>::CbType, ActionCbVal<ثابت نویسه*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ثابت نویسه* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ثابت {
    ثابت size_t prefixLen = std::strlen(optp);
    ثابت auto wrap = [prefixLen, cb](ثابت نویسه* optp) { cb(optp + prefixLen); };
    بازگشت m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ثابت نویسه* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ثابت {
    ثابت size_t prefixLen = std::strlen(optp);
    ثابت auto wrap
        = [prefixLen, cb](ثابت نویسه* optp, ثابت نویسه* argp) { cb(optp + prefixLen, argp); };
    بازگشت m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
