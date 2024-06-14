#include "Internal.h"

namespace {
    static std::string s_errorMessage;
} // namespace

const char *EST_ErrorGetMessage()
{
    return s_errorMessage.c_str();
}

void EST_ErrorSetMessage(const char *message)
{
    s_errorMessage = message;
}

const char *EST_ErrorTranslateMessage(enum EST_RESULT code)
{
    switch (code) {
        case EST_OK:
            return "No error";

        case EST_ERROR:
            return "Unknown error";

        case EST_ERROR_OUT_OF_MEMORY:
            return "Out of memory";

        case EST_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";

        case EST_ERROR_INVALID_STATE:
            return "Invalid state";

        case EST_ERROR_INVALID_OPERATION:
            return "Invalid operation";

        case EST_ERROR_INVALID_FORMAT:
            return "Invalid format";

        case EST_ERROR_INVALID_DATA:
            return "Invalid data";

        case EST_ERROR_TIMEDOUT:
            return "Timed out";

        case EST_ERROR_ENCODER_EMPTY:
            return "Encoder is empty";

        case EST_ERROR_ENCODER_UNSUPPORTED:
            return "Encoder operation is unsupported";

        case EST_ERROR_ENCODER_INVALID_WRITE:
            return "Invalid write operation on encoder";

        case EST_ERROR_ENCODER_INVALID_READ:
            return "Invalid read operation on encoder";

        case EST_ERROR_ENCODER_INVALID_OPERATION:
            return "Invalid operation on encoder";

        default:
            return "Unknown error code";
    }
}