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

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Options.h"
#endif
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3OptionParser.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

//######################################################################
// V3OptionParser::Impl
ସଂରଚନା V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    ଗଣନା ଶ୍ରେଣୀ en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        ମୂଲ୍ୟ_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, ତାର୍କିକ N_Allow_Partial_Match = ମିଥ୍ୟା>
    ଶ୍ରେଣୀ ActionBase VL_NOT_FINAL : ସର୍ବସାଧାରଣ ActionIfs {
        ତାର୍କିକ m_notForRerun = ମିଥ୍ୟା;  // This option is not needed for rerun with --dump-inputs
        ତାର୍କିକ m_undocumented = ମିଥ୍ୟା;  // This option is not documented
    ସର୍ବସାଧାରଣ:
        ତାର୍କିକ is_ମୂଲ୍ୟ_Needed() ଅଚଳ override final { ଫେରନ୍ତୁ N_Mode == en::ମୂଲ୍ୟ_3; }
        ତାର୍କିକ isFOnOffAllowed() ଅଚଳ override final { ଫେରନ୍ତୁ N_Mode == en::FONOFF; }
        ତାର୍କିକ isNotForRerun() ଅଚଳ override final { ଫେରନ୍ତୁ m_notForRerun; }
        ତାର୍କିକ isOnOffAllowed() ଅଚଳ override final { ଫେରନ୍ତୁ N_Mode == en::ONOFF; }
        ତାର୍କିକ isPartialMatchAllowed() ଅଚଳ override final { ଫେରନ୍ତୁ N_Allow_Partial_Match; }
        ତାର୍କିକ isUndocumented() ଅଚଳ override { ଫେରନ୍ତୁ m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = ସତ୍ୟ;
            ଫେରନ୍ତୁ *ଏହି;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = ସତ୍ୟ;
            ଫେରନ୍ତୁ *ଏହି;
        }
    };

    // Actual action classes
    template <typename T>
    ଶ୍ରେଣୀ ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    ଶ୍ରେଣୀ ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    ଶ୍ରେଣୀ ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    ଶ୍ରେଣୀ ActionCbCall;  // Callback without argument for "-opt"
    ଶ୍ରେଣୀ ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    ଶ୍ରେଣୀ ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    ଶ୍ରେଣୀ ActionCbVal;  // Callback for "-opt val"
    ଶ୍ରେଣୀ ActionCbPartialMatch;  // Callback "-O3" for "-O"
    ଶ୍ରେଣୀ ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ଅଚଳ string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    ତାର୍କିକ m_isFinalized{ମିଥ୍ୟା};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    ଶ୍ରେଣୀ V3OptionParser::Impl::className<type> final : ସର୍ବସାଧାରଣ ActionBase<enType> { \
        type* ଅଚଳ m_valp; /* Pointer to an option variable*/ \
\
    ସର୍ବସାଧାରଣ: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        ଶୂନ୍ୟ exec(ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, ତାର୍କିକ, *m_valp = ସତ୍ୟ, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(ସତ୍ୟ), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, ପୂର୍ଣ୍ଣସଂଖ୍ୟା, *m_valp = std::atoi(argp), en::ମୂଲ୍ୟ_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::ମୂଲ୍ୟ_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, ତାର୍କିକ, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, ତାର୍କିକ, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    ଶ୍ରେଣୀ V3OptionParser::Impl::className final : ସର୍ବସାଧାରଣ ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    ସର୍ବସାଧାରଣ: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        ଶୂନ୍ୟ exec(ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, ଶୂନ୍ୟ(ଶୂନ୍ୟ), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, ଶୂନ୍ୟ(ତାର୍କିକ), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, ଶୂନ୍ୟ(ତାର୍କିକ), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*), m_cb(optp), en::NONE,
                                 ସତ୍ୟ);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*, ଅଚଳ ଅକ୍ଷର*),
                                 m_cb(optp, argp), en::ମୂଲ୍ୟ_3, ସତ୍ୟ);

template <>
ଶ୍ରେଣୀ V3OptionParser::Impl::ActionCbVal<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> final : ସର୍ବସାଧାରଣ ActionBase<en::ମୂଲ୍ୟ_3> {
    std::function<ଶୂନ୍ୟ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା)> m_cb;  // Callback function

ସର୍ବସାଧାରଣ:
    using CbType = std::function<ଶୂନ୍ୟ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ଶୂନ୍ୟ exec(ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* argp) override { m_cb(std::atoi(argp)); }
};

template <>
ଶ୍ରେଣୀ V3OptionParser::Impl::ActionCbVal<ଅଚଳ ଅକ୍ଷର*> final : ସର୍ବସାଧାରଣ ActionBase<en::ମୂଲ୍ୟ_3> {
    std::function<ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*)> m_cb;

ସର୍ବସାଧାରଣ:
    using CbType = std::function<ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    ଶୂନ୍ୟ exec(ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ଅଚଳ ଅକ୍ଷର* optp) {
    ଅଚଳ auto it = m_pimpl->m_options.find(optp);
    ଯଦି (it != m_pimpl->m_options.end()) ଫେରନ୍ତୁ it->second.get();  // Exact match
    ପାଇଁ (auto&& act : m_pimpl->m_options) {
        ଯଦି (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            ଯଦି (ଅଚଳ ଅକ୍ଷର* ଅଚଳ nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                ଯଦି (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    ଫେରନ୍ତୁ act.second.get();
                }
            }
        }
        ଯଦି (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            ଯଦି (ଅଚଳ ଅକ୍ଷର* ଅଚଳ nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                ଯଦି (act.first == nop || act.first == ("-"s + nop)) ଫେରନ୍ତୁ act.second.get();
            }
        } ଅନ୍ୟଥା ଯଦି (act.second->isPartialMatchAllowed()) {
            ଯଦି (VString::startsWith(optp, act.first)) ଫେରନ୍ତୁ act.second.get();
        }
    }
    ଫେରନ୍ତୁ nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(ଅଚଳ std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{ନୂତନ T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ଅଚଳ auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    ଫେରନ୍ତୁ *insertedResult.first->second;
}

ତାର୍କିକ V3OptionParser::hasPrefixFNo(ଅଚଳ ଅକ୍ଷର* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ଯଦି (strp[1] == '-') ++strp;
    ଫେରନ୍ତୁ VString::startsWith(strp, "-fno");
}

ତାର୍କିକ V3OptionParser::hasPrefixNo(ଅଚଳ ଅକ୍ଷର* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    ଯଦି (strp[1] == '-') ++strp;
    ଫେରନ୍ତୁ VString::startsWith(strp, "-no");
}

std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ତାର୍କିକ> V3OptionParser::parse(ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx, ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ଅଚଳ ଅକ୍ଷର* optp = argv[idx];
    ଯଦି (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ଅଚଳ actp = find(optp);
    ଯଦି (!actp) ଫେରନ୍ତୁ {0, ମିଥ୍ୟା};
    ଯଦି (!actp->is_ମୂଲ୍ୟ_Needed()) {
        actp->exec(optp, nullptr);
        ଫେରନ୍ତୁ {1, !actp->isNotForRerun()};
    }
    ଯଦି (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        ଫେରନ୍ତୁ {2, !actp->isNotForRerun()};
    }
    ଫେରନ୍ତୁ {0, ମିଥ୍ୟା};
}

string V3OptionParser::getSuggestion(ଅଚଳ ଅକ୍ଷର* str) ଅଚଳ {
    ଫେରନ୍ତୁ m_pimpl->m_spellCheck.best_Candidate_ସନ୍ଦେଶ(str);
}

ଶୂନ୍ୟ V3OptionParser::addSuggestionCandidate(ଅଚଳ string& s) {
    m_pimpl->m_spellCheck.ଠେଲ_Candidate(s);
}

ଶୂନ୍ୟ V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    ପାଇଁ (auto&& opt : m_pimpl->m_options) {
        ଯଦି (opt.second->isUndocumented()) ଜାରି;
        m_pimpl->m_spellCheck.ଠେଲ_Candidate(opt.first);
        ଯଦି (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ଠେଲ_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        ଯଦି (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ଠେଲ_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = ସତ୍ୟ;
}

ଶୂନ୍ୟ V3OptionParser::dumpOptions() {
    ପାଇଁ (ଅଚଳ auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{ନୂତନ Impl{}} {}

V3OptionParser::~V3OptionParser() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        ଅଚଳ ଅକ୍ଷର* optp, actKind, argType arg) ଅଚଳ { \
        ଫେରନ୍ତୁ m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, ତାର୍କିକ*, ActionSet<ତାର୍କିକ>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, ପୂର୍ଣ୍ଣସଂଖ୍ୟା*, ActionSet<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, ତାର୍କିକ*, ActionFOnOff<ତାର୍କିକ>)
V3OPTION_PARSER_DEF_OP(OnOff, ତାର୍କିକ*, ActionOnOff<ତାର୍କିକ>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::CbType, ActionCbVal<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ଅଚଳ ଅକ୍ଷର*>::CbType, ActionCbVal<ଅଚଳ ଅକ୍ଷର*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ଅଚଳ ଅକ୍ଷର* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) ଅଚଳ {
    ଅଚଳ size_t prefixLen = std::strlen(optp);
    ଅଚଳ auto wrap = [prefixLen, cb](ଅଚଳ ଅକ୍ଷର* optp) { cb(optp + prefixLen); };
    ଫେରନ୍ତୁ m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(ଅଚଳ ଅକ୍ଷର* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) ଅଚଳ {
    ଅଚଳ size_t prefixLen = std::strlen(optp);
    ଅଚଳ auto wrap
        = [prefixLen, cb](ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* argp) { cb(optp + prefixLen, argp); };
    ଫେରନ୍ତୁ m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
