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

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Options.h"
#endif
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3OptionParser.h"
#അന്തര്ഭാവ "V3String.h"

//######################################################################
// V3OptionParser::Impl
ഘടന V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    എണ്ണൽ വർഗ്ഗം en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        മൂല്യം_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, തർക്കം N_Allow_Partial_Match = അസത്യം>
    വർഗ്ഗം ActionBase VL_NOT_FINAL : പൊതു ActionIfs {
        തർക്കം m_notForRerun = അസത്യം;  // This option is not needed for rerun with --dump-inputs
        തർക്കം m_undocumented = അസത്യം;  // This option is not documented
    പൊതു:
        തർക്കം is_മൂല്യം_Needed() സ്ഥിരം override final { തിരികെ N_Mode == en::മൂല്യം_3; }
        തർക്കം isFOnOffAllowed() സ്ഥിരം override final { തിരികെ N_Mode == en::FONOFF; }
        തർക്കം isNotForRerun() സ്ഥിരം override final { തിരികെ m_notForRerun; }
        തർക്കം isOnOffAllowed() സ്ഥിരം override final { തിരികെ N_Mode == en::ONOFF; }
        തർക്കം isPartialMatchAllowed() സ്ഥിരം override final { തിരികെ N_Allow_Partial_Match; }
        തർക്കം isUndocumented() സ്ഥിരം override { തിരികെ m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = സത്യം;
            തിരികെ *ഇത്;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = സത്യം;
            തിരികെ *ഇത്;
        }
    };

    // Actual action classes
    template <typename T>
    വർഗ്ഗം ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    വർഗ്ഗം ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    വർഗ്ഗം ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    വർഗ്ഗം ActionCbCall;  // Callback without argument for "-opt"
    വർഗ്ഗം ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    വർഗ്ഗം ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    വർഗ്ഗം ActionCbVal;  // Callback for "-opt val"
    വർഗ്ഗം ActionCbPartialMatch;  // Callback "-O3" for "-O"
    വർഗ്ഗം ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<സ്ഥിരം string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    തർക്കം m_isFinalized{അസത്യം};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    വർഗ്ഗം V3OptionParser::Impl::className<type> final : പൊതു ActionBase<enType> { \
        type* സ്ഥിരം m_valp; /* Pointer to an option variable*/ \
\
    പൊതു: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        ശൂന്യം exec(സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, തർക്കം, *m_valp = സത്യം, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(സത്യം), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, പൂർണ്ണസംഖ്യ, *m_valp = std::atoi(argp), en::മൂല്യം_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::മൂല്യം_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, തർക്കം, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, തർക്കം, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    വർഗ്ഗം V3OptionParser::Impl::className final : പൊതു ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    പൊതു: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        ശൂന്യം exec(സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, ശൂന്യം(ശൂന്യം), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, ശൂന്യം(തർക്കം), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, ശൂന്യം(തർക്കം), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, ശൂന്യം(സ്ഥിരം അക്ഷരം*), m_cb(optp), en::NONE,
                                 സത്യം);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, ശൂന്യം(സ്ഥിരം അക്ഷരം*, സ്ഥിരം അക്ഷരം*),
                                 m_cb(optp, argp), en::മൂല്യം_3, സത്യം);

template <>
വർഗ്ഗം V3OptionParser::Impl::ActionCbVal<പൂർണ്ണസംഖ്യ> final : പൊതു ActionBase<en::മൂല്യം_3> {
    std::function<ശൂന്യം(പൂർണ്ണസംഖ്യ)> m_cb;  // Callback function

പൊതു:
    using CbType = std::function<ശൂന്യം(പൂർണ്ണസംഖ്യ)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ശൂന്യം exec(സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* argp) override { m_cb(std::atoi(argp)); }
};

template <>
വർഗ്ഗം V3OptionParser::Impl::ActionCbVal<സ്ഥിരം അക്ഷരം*> final : പൊതു ActionBase<en::മൂല്യം_3> {
    std::function<ശൂന്യം(സ്ഥിരം അക്ഷരം*)> m_cb;

പൊതു:
    using CbType = std::function<ശൂന്യം(സ്ഥിരം അക്ഷരം*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ശൂന്യം exec(സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(സ്ഥിരം അക്ഷരം* optp) {
    സ്ഥിരം auto it = m_pimpl->m_options.find(optp);
    എങ്കിൽ (it != m_pimpl->m_options.end()) തിരികെ it->second.get();  // Exact match
    വേണ്ടി (auto&& act : m_pimpl->m_options) {
        എങ്കിൽ (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            എങ്കിൽ (സ്ഥിരം അക്ഷരം* സ്ഥിരം nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                എങ്കിൽ (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    തിരികെ act.second.get();
                }
            }
        }
        എങ്കിൽ (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            എങ്കിൽ (സ്ഥിരം അക്ഷരം* സ്ഥിരം nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                എങ്കിൽ (act.first == nop || act.first == ("-"s + nop)) തിരികെ act.second.get();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (act.second->isPartialMatchAllowed()) {
            എങ്കിൽ (VString::startsWith(optp, act.first)) തിരികെ act.second.get();
        }
    }
    തിരികെ nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(സ്ഥിരം std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{പുതിയ T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    സ്ഥിരം auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    തിരികെ *insertedResult.first->second;
}

തർക്കം V3OptionParser::hasPrefixFNo(സ്ഥിരം അക്ഷരം* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    എങ്കിൽ (strp[1] == '-') ++strp;
    തിരികെ VString::startsWith(strp, "-fno");
}

തർക്കം V3OptionParser::hasPrefixNo(സ്ഥിരം അക്ഷരം* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    എങ്കിൽ (strp[1] == '-') ++strp;
    തിരികെ VString::startsWith(strp, "-no");
}

std::pair<പൂർണ്ണസംഖ്യ, തർക്കം> V3OptionParser::parse(പൂർണ്ണസംഖ്യ idx, പൂർണ്ണസംഖ്യ argc, അക്ഷരം* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    സ്ഥിരം അക്ഷരം* optp = argv[idx];
    എങ്കിൽ (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* സ്ഥിരം actp = find(optp);
    എങ്കിൽ (!actp) തിരികെ {0, അസത്യം};
    എങ്കിൽ (!actp->is_മൂല്യം_Needed()) {
        actp->exec(optp, nullptr);
        തിരികെ {1, !actp->isNotForRerun()};
    }
    എങ്കിൽ (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        തിരികെ {2, !actp->isNotForRerun()};
    }
    തിരികെ {0, അസത്യം};
}

string V3OptionParser::getSuggestion(സ്ഥിരം അക്ഷരം* str) സ്ഥിരം {
    തിരികെ m_pimpl->m_spellCheck.best_Candidate_സന്ദേശം(str);
}

ശൂന്യം V3OptionParser::addSuggestionCandidate(സ്ഥിരം string& s) {
    m_pimpl->m_spellCheck.തള്ളുക_Candidate(s);
}

ശൂന്യം V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    വേണ്ടി (auto&& opt : m_pimpl->m_options) {
        എങ്കിൽ (opt.second->isUndocumented()) തുടരുക;
        m_pimpl->m_spellCheck.തള്ളുക_Candidate(opt.first);
        എങ്കിൽ (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.തള്ളുക_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        എങ്കിൽ (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.തള്ളുക_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = സത്യം;
}

ശൂന്യം V3OptionParser::dumpOptions() {
    വേണ്ടി (സ്ഥിരം auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{പുതിയ Impl{}} {}

V3OptionParser::~V3OptionParser() = സ്ഥിരസ്ഥിതി;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        സ്ഥിരം അക്ഷരം* optp, actKind, argType arg) സ്ഥിരം { \
        തിരികെ m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, തർക്കം*, ActionSet<തർക്കം>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, പൂർണ്ണസംഖ്യ*, ActionSet<പൂർണ്ണസംഖ്യ>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, തർക്കം*, ActionFOnOff<തർക്കം>)
V3OPTION_PARSER_DEF_OP(OnOff, തർക്കം*, ActionOnOff<തർക്കം>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<പൂർണ്ണസംഖ്യ>::CbType, ActionCbVal<പൂർണ്ണസംഖ്യ>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<സ്ഥിരം അക്ഷരം*>::CbType, ActionCbVal<സ്ഥിരം അക്ഷരം*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(സ്ഥിരം അക്ഷരം* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) സ്ഥിരം {
    സ്ഥിരം size_t prefixLen = std::strlen(optp);
    സ്ഥിരം auto wrap = [prefixLen, cb](സ്ഥിരം അക്ഷരം* optp) { cb(optp + prefixLen); };
    തിരികെ m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(സ്ഥിരം അക്ഷരം* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) സ്ഥിരം {
    സ്ഥിരം size_t prefixLen = std::strlen(optp);
    സ്ഥിരം auto wrap
        = [prefixLen, cb](സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* argp) { cb(optp + prefixLen, argp); };
    തിരികെ m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
