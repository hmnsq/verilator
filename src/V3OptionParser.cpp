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

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Options.h"
#endif
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3OptionParser.h"
#அந்தர்பாவ "V3String.h"

//######################################################################
// V3OptionParser::Impl
கட்டமைப்பு V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    எண்ணிக்கை வகுப்பு en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        மதிப்பு_3  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, தருக்க N_Allow_Partial_Match = பொய்>
    வகுப்பு ActionBase VL_NOT_FINAL : பொது ActionIfs {
        தருக்க m_notForRerun = பொய்;  // This option is not needed for rerun with --dump-inputs
        தருக்க m_undocumented = பொய்;  // This option is not documented
    பொது:
        தருக்க is_மதிப்பு_Needed() மாறிலி override final { திரும்பு N_Mode == en::மதிப்பு_3; }
        தருக்க isFOnOffAllowed() மாறிலி override final { திரும்பு N_Mode == en::FONOFF; }
        தருக்க isNotForRerun() மாறிலி override final { திரும்பு m_notForRerun; }
        தருக்க isOnOffAllowed() மாறிலி override final { திரும்பு N_Mode == en::ONOFF; }
        தருக்க isPartialMatchAllowed() மாறிலி override final { திரும்பு N_Allow_Partial_Match; }
        தருக்க isUndocumented() மாறிலி override { திரும்பு m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = உண்மை;
            திரும்பு *இது;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = உண்மை;
            திரும்பு *இது;
        }
    };

    // Actual action classes
    template <typename T>
    வகுப்பு ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    வகுப்பு ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    வகுப்பு ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    வகுப்பு ActionCbCall;  // Callback without argument for "-opt"
    வகுப்பு ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    வகுப்பு ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    வகுப்பு ActionCbVal;  // Callback for "-opt val"
    வகுப்பு ActionCbPartialMatch;  // Callback "-O3" for "-O"
    வகுப்பு ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<மாறிலி string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    தருக்க m_isFinalized{பொய்};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    வகுப்பு V3OptionParser::Impl::className<type> final : பொது ActionBase<enType> { \
        type* மாறிலி m_valp; /* Pointer to an option variable*/ \
\
    பொது: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        வெறுமை exec(மாறிலி எழுத்து* optp, மாறிலி எழுத்து* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, தருக்க, *m_valp = உண்மை, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(உண்மை), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, முழு, *m_valp = std::atoi(argp), en::மதிப்பு_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::மதிப்பு_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, தருக்க, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, தருக்க, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    வகுப்பு V3OptionParser::Impl::className final : பொது ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    பொது: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        வெறுமை exec(மாறிலி எழுத்து* optp, மாறிலி எழுத்து* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, வெறுமை(வெறுமை), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, வெறுமை(தருக்க), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, வெறுமை(தருக்க), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, வெறுமை(மாறிலி எழுத்து*), m_cb(optp), en::NONE,
                                 உண்மை);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, வெறுமை(மாறிலி எழுத்து*, மாறிலி எழுத்து*),
                                 m_cb(optp, argp), en::மதிப்பு_3, உண்மை);

template <>
வகுப்பு V3OptionParser::Impl::ActionCbVal<முழு> final : பொது ActionBase<en::மதிப்பு_3> {
    std::function<வெறுமை(முழு)> m_cb;  // Callback function

பொது:
    using CbType = std::function<வெறுமை(முழு)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    வெறுமை exec(மாறிலி எழுத்து* optp, மாறிலி எழுத்து* argp) override { m_cb(std::atoi(argp)); }
};

template <>
வகுப்பு V3OptionParser::Impl::ActionCbVal<மாறிலி எழுத்து*> final : பொது ActionBase<en::மதிப்பு_3> {
    std::function<வெறுமை(மாறிலி எழுத்து*)> m_cb;

பொது:
    using CbType = std::function<வெறுமை(மாறிலி எழுத்து*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    வெறுமை exec(மாறிலி எழுத்து* optp, மாறிலி எழுத்து* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(மாறிலி எழுத்து* optp) {
    மாறிலி auto it = m_pimpl->m_options.find(optp);
    எனில் (it != m_pimpl->m_options.end()) திரும்பு it->second.get();  // Exact match
    க்கு (auto&& act : m_pimpl->m_options) {
        எனில் (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            எனில் (மாறிலி எழுத்து* மாறிலி nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                எனில் (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    திரும்பு act.second.get();
                }
            }
        }
        எனில் (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            எனில் (மாறிலி எழுத்து* மாறிலி nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                எனில் (act.first == nop || act.first == ("-"s + nop)) திரும்பு act.second.get();
            }
        } இல்லையெனில் எனில் (act.second->isPartialMatchAllowed()) {
            எனில் (VString::startsWith(optp, act.first)) திரும்பு act.second.get();
        }
    }
    திரும்பு nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(மாறிலி std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{புதிய T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    மாறிலி auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    திரும்பு *insertedResult.first->second;
}

தருக்க V3OptionParser::hasPrefixFNo(மாறிலி எழுத்து* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    எனில் (strp[1] == '-') ++strp;
    திரும்பு VString::startsWith(strp, "-fno");
}

தருக்க V3OptionParser::hasPrefixNo(மாறிலி எழுத்து* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    எனில் (strp[1] == '-') ++strp;
    திரும்பு VString::startsWith(strp, "-no");
}

std::pair<முழு, தருக்க> V3OptionParser::parse(முழு idx, முழு argc, எழுத்து* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    மாறிலி எழுத்து* optp = argv[idx];
    எனில் (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* மாறிலி actp = find(optp);
    எனில் (!actp) திரும்பு {0, பொய்};
    எனில் (!actp->is_மதிப்பு_Needed()) {
        actp->exec(optp, nullptr);
        திரும்பு {1, !actp->isNotForRerun()};
    }
    எனில் (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        திரும்பு {2, !actp->isNotForRerun()};
    }
    திரும்பு {0, பொய்};
}

string V3OptionParser::getSuggestion(மாறிலி எழுத்து* str) மாறிலி {
    திரும்பு m_pimpl->m_spellCheck.best_Candidate_செய்தி(str);
}

வெறுமை V3OptionParser::addSuggestionCandidate(மாறிலி string& s) {
    m_pimpl->m_spellCheck.தள்ளு_Candidate(s);
}

வெறுமை V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    க்கு (auto&& opt : m_pimpl->m_options) {
        எனில் (opt.second->isUndocumented()) தொடர்;
        m_pimpl->m_spellCheck.தள்ளு_Candidate(opt.first);
        எனில் (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.தள்ளு_Candidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        எனில் (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.தள்ளு_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = உண்மை;
}

வெறுமை V3OptionParser::dumpOptions() {
    க்கு (மாறிலி auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{புதிய Impl{}} {}

V3OptionParser::~V3OptionParser() = இயல்பு;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        மாறிலி எழுத்து* optp, actKind, argType arg) மாறிலி { \
        திரும்பு m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, தருக்க*, ActionSet<தருக்க>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, முழு*, ActionSet<முழு>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, தருக்க*, ActionFOnOff<தருக்க>)
V3OPTION_PARSER_DEF_OP(OnOff, தருக்க*, ActionOnOff<தருக்க>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<முழு>::CbType, ActionCbVal<முழு>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<மாறிலி எழுத்து*>::CbType, ActionCbVal<மாறிலி எழுத்து*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(மாறிலி எழுத்து* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) மாறிலி {
    மாறிலி size_t prefixLen = std::strlen(optp);
    மாறிலி auto wrap = [prefixLen, cb](மாறிலி எழுத்து* optp) { cb(optp + prefixLen); };
    திரும்பு m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(மாறிலி எழுத்து* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) மாறிலி {
    மாறிலி size_t prefixLen = std::strlen(optp);
    மாறிலி auto wrap
        = [prefixLen, cb](மாறிலி எழுத்து* optp, மாறிலி எழுத்து* argp) { cb(optp + prefixLen, argp); };
    திரும்பு m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
