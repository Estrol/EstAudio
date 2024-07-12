#include "Unknown.h"

const char *EST_UnknownTypeToString(EST_UnknownType type)
{
    switch (type) {
        case EST_UNKNOWN_NONE:
            return "EST_UNKNOWN_NONE";
        case EST_UNKNOWN_CHANNEL:
            return "EST_UNKNOWN_CHANNEL";
        case EST_UNKNOWN_SAMPLE:
            return "EST_UNKNOWN_SAMPLE";
        case EST_UNKNOWN_DEVICE:
            return "EST_UNKNOWN_DEVICE";
        case EST_UNKNOWN_ENCODER:
            return "EST_UNKNOWN_ENCODER";
        default:
            return "EST_UNKNOWN_INVALID";
    }
}