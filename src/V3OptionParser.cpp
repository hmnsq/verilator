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

#ضمن "V3Global.h"
#ضمن "V3Options.h"
#endif
#ضمن "V3Error.h"
#ضمن "V3OptionParser.h"
#ضمن "V3String.h"

//######################################################################
// V3OptionParser::Impl
بنية V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    تعداد صنف en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        قىممەت_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, منطقي N_Allow_Partial_Match = خطأ>
    صنف ActionBase VL_NOT_FINAL : عام ActionIfs {
        منطقي m_notForRerun = خطأ;  // This option is not needed for rerun with --dump-inputs
        منطقي m_undocumented = خطأ;  // This option is not documented
    عام:
        منطقي is_قىممەت_Needed() ثابت override final { أرجع N_Mode == en::قىممەت_3; }
        منطقي isFOnOffAllowed() ثابت override final { أرجع N_Mode == en::FONOFF; }
        منطقي isNotForRerun() ثابت override final { أرجع m_notForRerun; }
        منطقي isOnOffAllowed() ثابت override final { أرجع N_Mode == en::ONOFF; }
        منطقي isPartialMatchAllowed() ثابت override final { أرجع N_Allow_Partial_Match; }
        منطقي isUndocumented() ثابت override { أرجع m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = true;
            أرجع *هذا;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = true;
            أرجع *هذا;
        }
    };

    // Actual action classes
    template <typename T>
    صنف ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    صنف ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    صنف ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    صنف ActionCbCall;  // Callback without argument for "-opt"
    صنف ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    صنف ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    صنف ActionCbVal;  // Callback for "-opt val"
    صنف ActionCbPartialMatch;  // Callback "-O3" for "-O"
    صنف ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ثابت string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    منطقي m_isFinalized{خطأ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    صنف V3OptionParser::Impl::className<type> final : عام ActionBase<enType> { \
        type* ثابت m_valp; /* Pointer to an option variable*/ \
\
    عام: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, منطقي, *m_valp = true, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(true), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, صحيح, *m_valp = std::atoi(argp), en::قىممەت_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::قىممەت_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, منطقي, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, منطقي, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    صنف V3OptionParser::Impl::className final : عام ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    عام: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, فراغ(فراغ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, فراغ(منطقي), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, فراغ(منطقي), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, فراغ(ثابت حرف*), m_cb(optp), en::NONE,
                                 true);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, فراغ(ثابت حرف*, ثابت حرف*),
                                 m_cb(optp, argp), en::قىممەت_3, true);

template <>
صنف V3OptionParser::Impl::ActionCbVal<صحيح> final : عام ActionBase<en::قىممەت_3> {
    std::function<فراغ(صحيح)> m_cb;  // Callback function

عام:
    using CbType = std::function<فراغ(صحيح)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(std::atoi(argp)); }
};

template <>
صنف V3OptionParser::Impl::ActionCbVal<ثابت حرف*> final : عام ActionBase<en::قىممەت_3> {
    std::function<فراغ(ثابت حرف*)> m_cb;

عام:
    using CbType = std::function<فراغ(ثابت حرف*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    فراغ exec(ثابت حرف* optp, ثابت حرف* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ثابت حرف* optp) {
    ثابت auto it = m_pimpl->m_options.find(optp);
    إذا (it != m_pimpl->m_options.end()) أرجع it->second.get();  // Exact match
    لكل (auto&& act : m_pimpl->m_options) {
        إذا (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                إذا (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    أرجع act.second.get();
                }
            }
        }
        إذا (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            إذا (ثابت حرف* ثابت nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                إذا (act.first == nop || act.first == ("-"s + nop)) أرجع act.second.get();
            }
        } وإلا إذا (act.second->isPartialMatchAllowed()) {
            إذا (VString::startsWith(optp, act.first)) أرجع act.second.get();
        }
    }
    أرجع nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ثابت std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{جديد T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ثابت auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    أرجع *insertedResult.first->second;
}

منطقي V3OptionParser::hasPrefixFNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-fno");
}

منطقي V3OptionParser::hasPrefixNo(ثابت حرف* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    إذا (strp[1] == '-') ++strp;
    أرجع VString::startsWith(strp, "-no");
}

std::pair<صحيح, منطقي> V3OptionParser::parse(صحيح idx, صحيح argc, حرف* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ثابت حرف* optp = argv[idx];
    إذا (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ثابت actp = find(optp);
    إذا (!actp) أرجع {0, خطأ};
    إذا (!actp->is_قىممەت_Needed()) {
        actp->exec(optp, nullptr);
        أرجع {1, !actp->isNotForRerun()};
    }
    إذا (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        أرجع {2, !actp->isNotForRerun()};
    }
    أرجع {0, خطأ};
}

string V3OptionParser::getSuggestion(ثابت حرف* str) ثابت {
    أرجع m_pimpl->m_spellCheck.best_Candidate_ئۇچۇر(str);
}

فراغ V3OptionParser::addSuggestionCandidate(ثابت string& s) {
    m_pimpl->m_spellCheck.ئىتتىرىش_Candidate(s);
}

فراغ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    لكل (auto&& opt : m_pimpl->m_options) {
        إذا (opt.second->isUndocumented()) استمر;
        m_pimpl->m_spellCheck.ئىتتىرىش_Candidate(opt.first);
        إذا (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ئىتتىرىش_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        إذا (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ئىتتىرىش_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = true;
}

فراغ V3OptionParser::dumpOptions() {
    لكل (ثابت auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{جديد Impl{}} {}

V3OptionParser::~V3OptionParser() = افتراضي;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ثابت حرف* optp, actKind, argType arg) ثابت { \
        أرجع m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, منطقي*, ActionSet<منطقي>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, صحيح*, ActionSet<صحيح>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, منطقي*, ActionFOnOff<منطقي>)
V3OPTION_PARSER_DEF_OP(OnOff, منطقي*, ActionOnOff<منطقي>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<صحيح>::CbType, ActionCbVal<صحيح>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ثابت حرف*>::CbType, ActionCbVal<ثابت حرف*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ثابت حرف* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ثابت {
    ثابت size_t prefixLen = std::strlen(optp);
    ثابت auto wrap = [prefixLen, cb](ثابت حرف* optp) { cb(optp + prefixLen); };
    أرجع m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ثابت حرف* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ثابت {
    ثابت size_t prefixLen = std::strlen(optp);
    ثابت auto wrap
        = [prefixLen, cb](ثابت حرف* optp, ثابت حرف* argp) { cb(optp + prefixLen, argp); };
    أرجع m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
