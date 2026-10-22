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

#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Options.h"
#endif
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3OptionParser.h"
#અન્તર્ભાવ "V3String.h"

//######################################################################
// V3OptionParser::Impl
સંરચના V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    ગણના વર્ગ en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        કિંમત_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, તાર્કિક N_Allow_Partial_Match = ખોટું>
    વર્ગ ActionBase VL_NOT_FINAL : જાહેર ActionIfs {
        તાર્કિક m_notForRerun = ખોટું;  // This option is not needed for rerun with --dump-inputs
        તાર્કિક m_undocumented = ખોટું;  // This option is not documented
    જાહેર:
        તાર્કિક is_કિંમત_Needed() અચલ override final { પાછા N_Mode == en::કિંમત_3; }
        તાર્કિક isFOnOffAllowed() અચલ override final { પાછા N_Mode == en::FONOFF; }
        તાર્કિક isNotForRerun() અચલ override final { પાછા m_notForRerun; }
        તાર્કિક isOnOffAllowed() અચલ override final { પાછા N_Mode == en::ONOFF; }
        તાર્કિક isPartialMatchAllowed() અચલ override final { પાછા N_Allow_Partial_Match; }
        તાર્કિક isUndocumented() અચલ override { પાછા m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = સાચું;
            પાછા *આ;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = સાચું;
            પાછા *આ;
        }
    };

    // Actual action classes
    template <typename T>
    વર્ગ ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    વર્ગ ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    વર્ગ ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    વર્ગ ActionCbCall;  // Callback without argument for "-opt"
    વર્ગ ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    વર્ગ ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    વર્ગ ActionCbVal;  // Callback for "-opt val"
    વર્ગ ActionCbPartialMatch;  // Callback "-O3" for "-O"
    વર્ગ ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<અચલ string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    તાર્કિક m_isFinalized{ખોટું};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    વર્ગ V3OptionParser::Impl::className<type> final : જાહેર ActionBase<enType> { \
        type* અચલ m_valp; /* Pointer to an option variable*/ \
\
    જાહેર: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        શૂન્ય exec(અચલ અક્ષર* optp, અચલ અક્ષર* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, તાર્કિક, *m_valp = સાચું, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(સાચું), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, પૂર્ણાંક, *m_valp = std::atoi(argp), en::કિંમત_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::કિંમત_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, તાર્કિક, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, તાર્કિક, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    વર્ગ V3OptionParser::Impl::className final : જાહેર ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    જાહેર: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        શૂન્ય exec(અચલ અક્ષર* optp, અચલ અક્ષર* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, શૂન્ય(શૂન્ય), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, શૂન્ય(તાર્કિક), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, શૂન્ય(તાર્કિક), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, શૂન્ય(અચલ અક્ષર*), m_cb(optp), en::NONE,
                                 સાચું);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, શૂન્ય(અચલ અક્ષર*, અચલ અક્ષર*),
                                 m_cb(optp, argp), en::કિંમત_3, સાચું);

template <>
વર્ગ V3OptionParser::Impl::ActionCbVal<પૂર્ણાંક> final : જાહેર ActionBase<en::કિંમત_3> {
    std::function<શૂન્ય(પૂર્ણાંક)> m_cb;  // Callback function

જાહેર:
    using CbType = std::function<શૂન્ય(પૂર્ણાંક)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    શૂન્ય exec(અચલ અક્ષર* optp, અચલ અક્ષર* argp) override { m_cb(std::atoi(argp)); }
};

template <>
વર્ગ V3OptionParser::Impl::ActionCbVal<અચલ અક્ષર*> final : જાહેર ActionBase<en::કિંમત_3> {
    std::function<શૂન્ય(અચલ અક્ષર*)> m_cb;

જાહેર:
    using CbType = std::function<શૂન્ય(અચલ અક્ષર*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    શૂન્ય exec(અચલ અક્ષર* optp, અચલ અક્ષર* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(અચલ અક્ષર* optp) {
    અચલ auto it = m_pimpl->m_options.find(optp);
    જો (it != m_pimpl->m_options.end()) પાછા it->second.get();  // Exact match
    માટે (auto&& act : m_pimpl->m_options) {
        જો (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            જો (અચલ અક્ષર* અચલ nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                જો (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    પાછા act.second.get();
                }
            }
        }
        જો (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            જો (અચલ અક્ષર* અચલ nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                જો (act.first == nop || act.first == ("-"s + nop)) પાછા act.second.get();
            }
        } નહીંતર જો (act.second->isPartialMatchAllowed()) {
            જો (VString::startsWith(optp, act.first)) પાછા act.second.get();
        }
    }
    પાછા nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(અચલ std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{નવું T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    અચલ auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    પાછા *insertedResult.first->second;
}

તાર્કિક V3OptionParser::hasPrefixFNo(અચલ અક્ષર* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    જો (strp[1] == '-') ++strp;
    પાછા VString::startsWith(strp, "-fno");
}

તાર્કિક V3OptionParser::hasPrefixNo(અચલ અક્ષર* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    જો (strp[1] == '-') ++strp;
    પાછા VString::startsWith(strp, "-no");
}

std::pair<પૂર્ણાંક, તાર્કિક> V3OptionParser::parse(પૂર્ણાંક idx, પૂર્ણાંક argc, અક્ષર* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    અચલ અક્ષર* optp = argv[idx];
    જો (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* અચલ actp = find(optp);
    જો (!actp) પાછા {0, ખોટું};
    જો (!actp->is_કિંમત_Needed()) {
        actp->exec(optp, nullptr);
        પાછા {1, !actp->isNotForRerun()};
    }
    જો (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        પાછા {2, !actp->isNotForRerun()};
    }
    પાછા {0, ખોટું};
}

string V3OptionParser::getSuggestion(અચલ અક્ષર* str) અચલ {
    પાછા m_pimpl->m_spellCheck.best_Candidate_સંદેશ(str);
}

શૂન્ય V3OptionParser::addSuggestionCandidate(અચલ string& s) {
    m_pimpl->m_spellCheck.ધકેલો_Candidate(s);
}

શૂન્ય V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    માટે (auto&& opt : m_pimpl->m_options) {
        જો (opt.second->isUndocumented()) ચાલુ;
        m_pimpl->m_spellCheck.ધકેલો_Candidate(opt.first);
        જો (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ધકેલો_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        જો (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ધકેલો_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = સાચું;
}

શૂન્ય V3OptionParser::dumpOptions() {
    માટે (અચલ auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{નવું Impl{}} {}

V3OptionParser::~V3OptionParser() = મૂળભૂત;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        અચલ અક્ષર* optp, actKind, argType arg) અચલ { \
        પાછા m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, તાર્કિક*, ActionSet<તાર્કિક>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, પૂર્ણાંક*, ActionSet<પૂર્ણાંક>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, તાર્કિક*, ActionFOnOff<તાર્કિક>)
V3OPTION_PARSER_DEF_OP(OnOff, તાર્કિક*, ActionOnOff<તાર્કિક>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<પૂર્ણાંક>::CbType, ActionCbVal<પૂર્ણાંક>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<અચલ અક્ષર*>::CbType, ActionCbVal<અચલ અક્ષર*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(અચલ અક્ષર* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) અચલ {
    અચલ size_t prefixLen = std::strlen(optp);
    અચલ auto wrap = [prefixLen, cb](અચલ અક્ષર* optp) { cb(optp + prefixLen); };
    પાછા m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(અચલ અક્ષર* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) અચલ {
    અચલ size_t prefixLen = std::strlen(optp);
    અચલ auto wrap
        = [prefixLen, cb](અચલ અક્ષર* optp, અચલ અક્ષર* argp) { cb(optp + prefixLen, argp); };
    પાછા m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
