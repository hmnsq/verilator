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

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Options.h"
#endif
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3OptionParser.h"
#अन्तर्भाव "V3String.h"

//######################################################################
// V3OptionParser::Impl
संरचना V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    गणना वर्ग en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        मानम्_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, तार्किक N_Allow_Partial_Match = मिच्छ>
    वर्ग ActionBase VL_NOT_FINAL : सार्वजनिक ActionIfs {
        तार्किक m_notForRerun = मिच्छ;  // This option is not needed for rerun with --dump-inputs
        तार्किक m_undocumented = मिच्छ;  // This option is not documented
    सार्वजनिक:
        तार्किक is_मानम्_Needed() ध्रुव override final { प्रत्यागम N_Mode == en::मानम्_3; }
        तार्किक isFOnOffAllowed() ध्रुव override final { प्रत्यागम N_Mode == en::FONOFF; }
        तार्किक isNotForRerun() ध्रुव override final { प्रत्यागम m_notForRerun; }
        तार्किक isOnOffAllowed() ध्रुव override final { प्रत्यागम N_Mode == en::ONOFF; }
        तार्किक isPartialMatchAllowed() ध्रुव override final { प्रत्यागम N_Allow_Partial_Match; }
        तार्किक isUndocumented() ध्रुव override { प्रत्यागम m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = सच्च;
            प्रत्यागम *एतत्;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = सच्च;
            प्रत्यागम *एतत्;
        }
    };

    // Actual action classes
    template <typename T>
    वर्ग ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    वर्ग ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    वर्ग ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    वर्ग ActionCbCall;  // Callback without argument for "-opt"
    वर्ग ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    वर्ग ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    वर्ग ActionCbVal;  // Callback for "-opt val"
    वर्ग ActionCbPartialMatch;  // Callback "-O3" for "-O"
    वर्ग ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ध्रुव string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    तार्किक m_isFinalized{मिच्छ};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    वर्ग V3OptionParser::Impl::className<type> final : सार्वजनिक ActionBase<enType> { \
        type* ध्रुव m_valp; /* Pointer to an option variable*/ \
\
    सार्वजनिक: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, तार्किक, *m_valp = सच्च, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(सच्च), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, पूर्णाङ्क, *m_valp = std::atoi(argp), en::मानम्_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::मानम्_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, तार्किक, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, तार्किक, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    वर्ग V3OptionParser::Impl::className final : सार्वजनिक ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    सार्वजनिक: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, शून्य(शून्य), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, शून्य(तार्किक), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, शून्य(तार्किक), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, शून्य(ध्रुव वर्ण*), m_cb(optp), en::NONE,
                                 सच्च);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, शून्य(ध्रुव वर्ण*, ध्रुव वर्ण*),
                                 m_cb(optp, argp), en::मानम्_3, सच्च);

template <>
वर्ग V3OptionParser::Impl::ActionCbVal<पूर्णाङ्क> final : सार्वजनिक ActionBase<en::मानम्_3> {
    std::function<शून्य(पूर्णाङ्क)> m_cb;  // Callback function

सार्वजनिक:
    using CbType = std::function<शून्य(पूर्णाङ्क)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { m_cb(std::atoi(argp)); }
};

template <>
वर्ग V3OptionParser::Impl::ActionCbVal<ध्रुव वर्ण*> final : सार्वजनिक ActionBase<en::मानम्_3> {
    std::function<शून्य(ध्रुव वर्ण*)> m_cb;

सार्वजनिक:
    using CbType = std::function<शून्य(ध्रुव वर्ण*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ध्रुव वर्ण* optp) {
    ध्रुव auto it = m_pimpl->m_options.find(optp);
    जइ (it != m_pimpl->m_options.end()) प्रत्यागम it->second.get();  // Exact match
    प्रति (auto&& act : m_pimpl->m_options) {
        जइ (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            जइ (ध्रुव वर्ण* ध्रुव nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                जइ (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    प्रत्यागम act.second.get();
                }
            }
        }
        जइ (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            जइ (ध्रुव वर्ण* ध्रुव nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                जइ (act.first == nop || act.first == ("-"s + nop)) प्रत्यागम act.second.get();
            }
        } अण्णहा जइ (act.second->isPartialMatchAllowed()) {
            जइ (VString::startsWith(optp, act.first)) प्रत्यागम act.second.get();
        }
    }
    प्रत्यागम nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ध्रुव std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{नूतन T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ध्रुव auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    प्रत्यागम *insertedResult.first->second;
}

तार्किक V3OptionParser::hasPrefixFNo(ध्रुव वर्ण* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    जइ (strp[1] == '-') ++strp;
    प्रत्यागम VString::startsWith(strp, "-fno");
}

तार्किक V3OptionParser::hasPrefixNo(ध्रुव वर्ण* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    जइ (strp[1] == '-') ++strp;
    प्रत्यागम VString::startsWith(strp, "-no");
}

std::pair<पूर्णाङ्क, तार्किक> V3OptionParser::parse(पूर्णाङ्क idx, पूर्णाङ्क argc, वर्ण* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ध्रुव वर्ण* optp = argv[idx];
    जइ (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ध्रुव actp = find(optp);
    जइ (!actp) प्रत्यागम {0, मिच्छ};
    जइ (!actp->is_मानम्_Needed()) {
        actp->exec(optp, nullptr);
        प्रत्यागम {1, !actp->isNotForRerun()};
    }
    जइ (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        प्रत्यागम {2, !actp->isNotForRerun()};
    }
    प्रत्यागम {0, मिच्छ};
}

string V3OptionParser::getSuggestion(ध्रुव वर्ण* str) ध्रुव {
    प्रत्यागम m_pimpl->m_spellCheck.best_Candidate_सन्देशः(str);
}

शून्य V3OptionParser::addSuggestionCandidate(ध्रुव string& s) {
    m_pimpl->m_spellCheck.प्रक्षिप_Candidate(s);
}

शून्य V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    प्रति (auto&& opt : m_pimpl->m_options) {
        जइ (opt.second->isUndocumented()) अनुवर्त;
        m_pimpl->m_spellCheck.प्रक्षिप_Candidate(opt.first);
        जइ (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.प्रक्षिप_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        जइ (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.प्रक्षिप_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = सच्च;
}

शून्य V3OptionParser::dumpOptions() {
    प्रति (ध्रुव auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{नूतन Impl{}} {}

V3OptionParser::~V3OptionParser() = स्वभाव;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ध्रुव वर्ण* optp, actKind, argType arg) ध्रुव { \
        प्रत्यागम m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, तार्किक*, ActionSet<तार्किक>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, पूर्णाङ्क*, ActionSet<पूर्णाङ्क>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, तार्किक*, ActionFOnOff<तार्किक>)
V3OPTION_PARSER_DEF_OP(OnOff, तार्किक*, ActionOnOff<तार्किक>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<पूर्णाङ्क>::CbType, ActionCbVal<पूर्णाङ्क>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ध्रुव वर्ण*>::CbType, ActionCbVal<ध्रुव वर्ण*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ध्रुव वर्ण* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ध्रुव {
    ध्रुव size_t prefixLen = std::strlen(optp);
    ध्रुव auto wrap = [prefixLen, cb](ध्रुव वर्ण* optp) { cb(optp + prefixLen); };
    प्रत्यागम m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ध्रुव वर्ण* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ध्रुव {
    ध्रुव size_t prefixLen = std::strlen(optp);
    ध्रुव auto wrap
        = [prefixLen, cb](ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) { cb(optp + prefixLen, argp); };
    प्रत्यागम m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
