/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "Unknown.h"
#include "EstAudio.h"
#include "EstEncoder.h"

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

const void **EST_GetFunctionTable()
{
    static const void *functionTable[] = {
        /**
         * Devices
         */

        EST_DeviceInit,
        EST_GetInfo,
        EST_DeviceAddCallback,
        EST_DeviceRemoveCallback,
        EST_DeviceFree,

        /**
         * Channels
         */

        EST_ChannelLoad,
        EST_ChannelLoadFromMemory,
        EST_SampleGetChannel,
        EST_SampleGetChannels,
        EST_EncoderGetChannel,
        EST_EncoderGetChannels,
        EST_ChannelPlay,
        EST_ChannelPause,
        EST_ChannelStop,
        EST_ChannelFree,
        EST_ChannelSeekPosition,
        EST_ChannelIsPlaying,
        EST_ChannelSetAttribute,
        EST_ChannelGetAttribute,
        EST_ChannelAddCallback,
        EST_ChannelRemoveCallback,
        EST_ChannelGetData,

        /**
         * Samples
         */

        EST_SampleLoad,
        EST_SampleLoadFromMemory,
        EST_SampleLoadFromEncoder,
        EST_SampleSetAttribute,
        EST_SampleGetAttribute,
        EST_SampleFree,

        /**
         * Encoders
         */

        EST_EncoderLoad,
        EST_EncoderLoadMemory,
        EST_EncoderFree,
        EST_EncoderGetInfo,
        EST_EncoderRender,
        EST_EncoderSetAttribute,
        EST_EncoderGetAttribute,
        EST_EncoderGetData,
        EST_EncoderFlushData,
        EST_EncoderGetAvailableDataSize,
        EST_EncoderExportFile,

        /**
         * FX
         */

        EST_FXLoad,
        EST_FXLoadFromMemory,
        EST_FXFree,
        EST_FXCreateSample,
        EST_FXCreateChannel,
        EST_FXSetAttribute,
        EST_FXGetAttribute,
    };

    return functionTable;
}