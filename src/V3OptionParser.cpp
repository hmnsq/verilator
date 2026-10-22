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

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"
#endif
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3OptionParser.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

//######################################################################
// V3OptionParser::Impl
ಸಂರಚನಾ V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    ಗಣನಾ ವರ್ಗ en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        ಮೌಲ್ಯ_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, ತಾರ್ಕಿಕ N_Allow_Partial_Match = ಅಸತ್ಯ>
    ವರ್ಗ ActionBase VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ ActionIfs {
        ತಾರ್ಕಿಕ m_notForRerun = ಅಸತ್ಯ;  // This option is not needed for rerun with --dump-inputs
        ತಾರ್ಕಿಕ m_undocumented = ಅಸತ್ಯ;  // This option is not documented
    ಸಾರ್ವಜನಿಕ:
        ತಾರ್ಕಿಕ is_ಮೌಲ್ಯ_Needed() ಧ್ರುವ override final { ಪ್ರತ್ಯಾಗಮ N_Mode == en::ಮೌಲ್ಯ_3; }
        ತಾರ್ಕಿಕ isFOnOffAllowed() ಧ್ರುವ override final { ಪ್ರತ್ಯಾಗಮ N_Mode == en::FONOFF; }
        ತಾರ್ಕಿಕ isNotForRerun() ಧ್ರುವ override final { ಪ್ರತ್ಯಾಗಮ m_notForRerun; }
        ತಾರ್ಕಿಕ isOnOffAllowed() ಧ್ರುವ override final { ಪ್ರತ್ಯಾಗಮ N_Mode == en::ONOFF; }
        ತಾರ್ಕಿಕ isPartialMatchAllowed() ಧ್ರುವ override final { ಪ್ರತ್ಯಾಗಮ N_Allow_Partial_Match; }
        ತಾರ್ಕಿಕ isUndocumented() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
    };

    // Actual action classes
    template <typename T>
    ವರ್ಗ ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    ವರ್ಗ ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    ವರ್ಗ ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    ವರ್ಗ ActionCbCall;  // Callback without argument for "-opt"
    ವರ್ಗ ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    ವರ್ಗ ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    ವರ್ಗ ActionCbVal;  // Callback for "-opt val"
    ವರ್ಗ ActionCbPartialMatch;  // Callback "-O3" for "-O"
    ವರ್ಗ ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ಧ್ರುವ string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    ತಾರ್ಕಿಕ m_isFinalized{ಅಸತ್ಯ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    ವರ್ಗ V3OptionParser::Impl::className<type> final : ಸಾರ್ವಜನಿಕ ActionBase<enType> { \
        type* ಧ್ರುವ m_valp; /* Pointer to an option variable*/ \
\
    ಸಾರ್ವಜನಿಕ: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        ಶೂನ್ಯ exec(ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, ತಾರ್ಕಿಕ, *m_valp = ಸತ್ಯ, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(ಸತ್ಯ), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, ಪೂರ್ಣಾಙ್ಕ, *m_valp = std::atoi(argp), en::ಮೌಲ್ಯ_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::ಮೌಲ್ಯ_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, ತಾರ್ಕಿಕ, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, ತಾರ್ಕಿಕ, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    ವರ್ಗ V3OptionParser::Impl::className final : ಸಾರ್ವಜನಿಕ ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    ಸಾರ್ವಜನಿಕ: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        ಶೂನ್ಯ exec(ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, ಶೂನ್ಯ(ಶೂನ್ಯ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, ಶೂನ್ಯ(ತಾರ್ಕಿಕ), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, ಶೂನ್ಯ(ತಾರ್ಕಿಕ), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*), m_cb(optp), en::NONE,
                                 ಸತ್ಯ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*, ಧ್ರುವ ವರ್ಣ*),
                                 m_cb(optp, argp), en::ಮೌಲ್ಯ_3, ಸತ್ಯ);

template <>
ವರ್ಗ V3OptionParser::Impl::ActionCbVal<ಪೂರ್ಣಾಙ್ಕ> final : ಸಾರ್ವಜನಿಕ ActionBase<en::ಮೌಲ್ಯ_3> {
    std::function<ಶೂನ್ಯ(ಪೂರ್ಣಾಙ್ಕ)> m_cb;  // Callback function

ಸಾರ್ವಜನಿಕ:
    using CbType = std::function<ಶೂನ್ಯ(ಪೂರ್ಣಾಙ್ಕ)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ಶೂನ್ಯ exec(ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* argp) override { m_cb(std::atoi(argp)); }
};

template <>
ವರ್ಗ V3OptionParser::Impl::ActionCbVal<ಧ್ರುವ ವರ್ಣ*> final : ಸಾರ್ವಜನಿಕ ActionBase<en::ಮೌಲ್ಯ_3> {
    std::function<ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*)> m_cb;

ಸಾರ್ವಜನಿಕ:
    using CbType = std::function<ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ಶೂನ್ಯ exec(ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ಧ್ರುವ ವರ್ಣ* optp) {
    ಧ್ರುವ auto it = m_pimpl->m_options.find(optp);
    ಯದಿ (it != m_pimpl->m_options.end()) ಪ್ರತ್ಯಾಗಮ it->second.get();  // Exact match
    ಪ್ರತಿ (auto&& act : m_pimpl->m_options) {
        ಯದಿ (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            ಯದಿ (ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                ಯದಿ (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    ಪ್ರತ್ಯಾಗಮ act.second.get();
                }
            }
        }
        ಯದಿ (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            ಯದಿ (ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                ಯದಿ (act.first == nop || act.first == ("-"s + nop)) ಪ್ರತ್ಯಾಗಮ act.second.get();
            }
        } ಅನ್ಯಥಾ ಯದಿ (act.second->isPartialMatchAllowed()) {
            ಯದಿ (VString::startsWith(optp, act.first)) ಪ್ರತ್ಯಾಗಮ act.second.get();
        }
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ಧ್ರುವ std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{ನೂತನ T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ಧ್ರುವ auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    ಪ್ರತ್ಯಾಗಮ *insertedResult.first->second;
}

ತಾರ್ಕಿಕ V3OptionParser::hasPrefixFNo(ಧ್ರುವ ವರ್ಣ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ಯದಿ (strp[1] == '-') ++strp;
    ಪ್ರತ್ಯಾಗಮ VString::startsWith(strp, "-fno");
}

ತಾರ್ಕಿಕ V3OptionParser::hasPrefixNo(ಧ್ರುವ ವರ್ಣ* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ಯದಿ (strp[1] == '-') ++strp;
    ಪ್ರತ್ಯಾಗಮ VString::startsWith(strp, "-no");
}

std::pair<ಪೂರ್ಣಾಙ್ಕ, ತಾರ್ಕಿಕ> V3OptionParser::parse(ಪೂರ್ಣಾಙ್ಕ idx, ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ಧ್ರುವ ವರ್ಣ* optp = argv[idx];
    ಯದಿ (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ಧ್ರುವ actp = find(optp);
    ಯದಿ (!actp) ಪ್ರತ್ಯಾಗಮ {0, ಅಸತ್ಯ};
    ಯದಿ (!actp->is_ಮೌಲ್ಯ_Needed()) {
        actp->exec(optp, nullptr);
        ಪ್ರತ್ಯಾಗಮ {1, !actp->isNotForRerun()};
    }
    ಯದಿ (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        ಪ್ರತ್ಯಾಗಮ {2, !actp->isNotForRerun()};
    }
    ಪ್ರತ್ಯಾಗಮ {0, ಅಸತ್ಯ};
}

string V3OptionParser::getSuggestion(ಧ್ರುವ ವರ್ಣ* str) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_pimpl->m_spellCheck.best_Candidate_ಸಂದೇಶ(str);
}

ಶೂನ್ಯ V3OptionParser::addSuggestionCandidate(ಧ್ರುವ string& s) {
    m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate(s);
}

ಶೂನ್ಯ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    ಪ್ರತಿ (auto&& opt : m_pimpl->m_options) {
        ಯದಿ (opt.second->isUndocumented()) ಅನುವರ್ತ;
        m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate(opt.first);
        ಯದಿ (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        ಯದಿ (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ತಳ್ಳು_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = ಸತ್ಯ;
}

ಶೂನ್ಯ V3OptionParser::dumpOptions() {
    ಪ್ರತಿ (ಧ್ರುವ auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{ನೂತನ Impl{}} {}

V3OptionParser::~V3OptionParser() = ಸ್ವಭಾವ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ಧ್ರುವ ವರ್ಣ* optp, actKind, argType arg) ಧ್ರುವ { \
        ಪ್ರತ್ಯಾಗಮ m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, ತಾರ್ಕಿಕ*, ActionSet<ತಾರ್ಕಿಕ>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, ಪೂರ್ಣಾಙ್ಕ*, ActionSet<ಪೂರ್ಣಾಙ್ಕ>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, ತಾರ್ಕಿಕ*, ActionFOnOff<ತಾರ್ಕಿಕ>)
V3OPTION_PARSER_DEF_OP(OnOff, ತಾರ್ಕಿಕ*, ActionOnOff<ತಾರ್ಕಿಕ>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ಪೂರ್ಣಾಙ್ಕ>::CbType, ActionCbVal<ಪೂರ್ಣಾಙ್ಕ>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ಧ್ರುವ ವರ್ಣ*>::CbType, ActionCbVal<ಧ್ರುವ ವರ್ಣ*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ಧ್ರುವ ವರ್ಣ* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ಧ್ರುವ {
    ಧ್ರುವ size_t prefixLen = std::strlen(optp);
    ಧ್ರುವ auto wrap = [prefixLen, cb](ಧ್ರುವ ವರ್ಣ* optp) { cb(optp + prefixLen); };
    ಪ್ರತ್ಯಾಗಮ m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ಧ್ರುವ ವರ್ಣ* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ಧ್ರುವ {
    ಧ್ರುವ size_t prefixLen = std::strlen(optp);
    ಧ್ರುವ auto wrap
        = [prefixLen, cb](ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* argp) { cb(optp + prefixLen, argp); };
    ಪ್ರತ್ಯಾಗಮ m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
