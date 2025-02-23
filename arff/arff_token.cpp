#include <arff_token.h>



std::string arff_token2str(ArffTokenEnum type) {
    switch(type) {
    case RELATION:      return "RELATION";
    case ATTRIBUTE:     return "ATTRIBUTE";
    case DATA_TOKEN:    return "DATA_TOKEN";
    case MISSING_TOKEN: return "MISSING_TOKEN";
    case NUMERIC_TOKEN: return "NUMERIC_TOKEN";
    case REAL_TOKEN:	return "REAL_TOKEN";
    case STRING_TOKEN:  return "STRING_TOKEN";
    case DATE_TOKEN:    return "DATE_TOKEN";
    case VALUE_TOKEN:   return "VALUE_TOKEN";
    case BRKT_OPEN:     return "BRKT_OPEN";
    case BRKT_CLOSE:    return "BRKT_CLOSE";
    case END_OF_FILE:   return "END_OF_FILE";
    default:            return "UNKNOWN";
    }
}



ArffToken::ArffToken(const std::string& _str, ArffTokenEnum _token) :
    m_str(_str), m_enum(_token) {
}

ArffToken::ArffToken(const ArffToken& _src) :
    m_str(_src.m_str), m_enum(_src.m_enum) {
}

ArffToken::~ArffToken() {
}

std::string ArffToken::token_str() const {
    return m_str;
}

ArffTokenEnum ArffToken::token_enum() const {
    return m_enum;
}

int32 ArffToken::token_int32() const {
    return (int32)token_int64();
}

int64 ArffToken::token_int64() const {
    if(m_enum != VALUE_TOKEN) {
        THROW("ArffToken::token_int64 token is not '%s', it's '%s'!",
              "VALUE_TOKEN", arff_token2str(m_enum).c_str());
    }
    int64 num;
    str2num<int64>(m_str, num);
    return num;
}

float ArffToken::token_float() const {
    return (float)token_double();
}

double ArffToken::token_double() const {
    if(m_enum != VALUE_TOKEN) {
        THROW("ArffToken::token_double token is not '%s', it's '%s'!",
              "VALUE_TOKEN", arff_token2str(m_enum).c_str());
    }
    double num;
    str2num<double>(m_str, num);
    return num;
}
