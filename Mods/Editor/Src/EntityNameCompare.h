#pragma once

struct EntityNameCompare {
    struct Parsed {
        std::string m_Base;
        bool m_HasNumber = false;
        int m_Number = 0;
    };

    static std::string StripSuffix(const std::string& p_String) {
        const size_t s_Position = p_String.find(" (");

        return (s_Position == std::string::npos) ? p_String : s.substr(0, s_Position);
    }

    static Parsed Parse(const std::string& p_String) {
        Parsed s_Parsed;
        const std::string s_Name = StripSuffix(p_String);

        const size_t s_End = s_Name.size();
        size_t s_Position = s_End;

        while (s_Position > 0 && std::isdigit(static_cast<unsigned char>(s_Name[s_Position - 1]))) {
            --s_Position;
        }

        if (s_Position < s_End) {
            s_Parsed.m_Base = s_Name.substr(0, s_Position);
            s_Parsed.m_Number = std::stoi(s_Name.substr(s_Position));
            s_Parsed.m_HasNumber = true;
        }
        else {
            s_Parsed.m_Base = s_Name;
        }

        return s_Parsed;
    }

    bool operator()(const std::string& p_A, const std::string& p_B) const {
        Parsed s_ParsedA = Parse(p_A);
        Parsed s_ParsedB = Parse(p_B);

        if (s_ParsedA.m_Base == s_ParsedB.m_Base) {
            if (s_ParsedA.m_HasNumber && s_ParsedB.m_HasNumber) {
                return s_ParsedA.m_Number < s_ParsedB.m_Number;
            }

            if (!s_ParsedA.m_HasNumber && s_ParsedB.m_HasNumber) {
                return true;
            }

            if (s_ParsedA.m_HasNumber && !s_ParsedB.m_HasNumber) {
                return false;
            }

            return p_A < p_B;
        }

        return s_ParsedA.m_Base < s_ParsedB.m_Base;
    }
};