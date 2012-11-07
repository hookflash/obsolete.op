
#include "acm_vorbis.h"
#include "vorbis_interface.h"
#include "acm_codec_database.h"

namespace webrtc {

#ifndef WEBRTC_CODEC_VORBIS
  
ACMVORBIS::ACMVORBIS(WebRtc_Word16 /* codecID*/)
{
    return;
}
  
ACMVORBIS::~ACMVORBIS()
{
    return;
}
  
WebRtc_Word16
ACMVORBIS::InternalEncode(
                          WebRtc_UWord8* /* bitStream        */,
                          WebRtc_Word16* /* bitStreamLenByte */)
{
    return -1;
}
  
WebRtc_Word16
ACMVORBIS::DecodeSafe(
                      WebRtc_UWord8* /* bitStream        */,
                      WebRtc_Word16  /* bitStreamLenByte */,
                      WebRtc_Word16* /* audio            */,
                      WebRtc_Word16* /* audioSamples     */,
                      WebRtc_Word8*  /* speechType       */)
{
    return -1;
}
  
WebRtc_Word16
ACMVORBIS::InternalInitEncoder(
                               WebRtcACMCodecParams* /* codecParams */)
{
    return -1;
}
  
WebRtc_Word16
ACMVORBIS::InternalInitDecoder(
                               WebRtcACMCodecParams* /* codecParams */)
{
    return -1;
}
  
WebRtc_Word32
ACMVORBIS::CodecDef(
                    WebRtcNetEQ_CodecDef& /* codecDef  */,
                    const CodecInst&      /* codecInst */)
{
    return -1;
}
  
ACMGenericCodec*
ACMVORBIS::CreateInstance(void)
{
    return NULL;
}
  
WebRtc_Word16
ACMVORBIS::InternalCreateEncoder()
{
    return -1;
}
  
void
ACMVORBIS::DestructEncoderSafe()
{
    return;
}
  
WebRtc_Word16
ACMVORBIS::InternalCreateDecoder()
{
    return -1;
}
  
void
ACMVORBIS::DestructDecoderSafe()
{
    return;
}
  
WebRtc_Word16
ACMVORBIS::UnregisterFromNetEqSafe(
                                   ACMNetEQ*     /* netEq       */,
                                   WebRtc_Word16 /* payloadType */)
{
    return -1;
}
  
void
ACMVORBIS::InternalDestructEncoderInst(
                                       void* /* ptrInst */)
{
    return;
}
  
#else     //===================== Actual Implementation =======================
  
ACMVORBIS::ACMVORBIS(WebRtc_Word16 codecID) :
_encoderInstPtr(NULL),
_decoderInstPtr(NULL)
{
    _codecID = codecID;
  
    // Set sampling frequency, frame size and rate Speex
    if(_codecID == ACMCodecDB::kVORBIS)
    {
        _samplingFrequency = 16000;
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Wrong codec id for Vorbis.");
    }
}

ACMVORBIS::~ACMVORBIS()
{
    if(_encoderInstPtr != NULL)
    {
        WebRtcVorbis_FreeEnc(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    if(_decoderInstPtr != NULL)
    {
        WebRtcVorbis_FreeDec(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
}

WebRtc_Word16
ACMVORBIS::InternalEncode(
                          WebRtc_UWord8* bitStream,
                          WebRtc_Word16* bitStreamLenByte)
{
    *bitStreamLenByte = WebRtcVorbis_Encode(_encoderInstPtr,
                                            &_inAudio[_inAudioIxRead], _frameLenSmpl, bitStream);
    if (*bitStreamLenByte < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "InternalEncode: error in encode for Vorbis");
        return -1;
    }
    // increment the read index this tell the caller that how far
    // we have gone forward in reading the audio buffer
    _inAudioIxRead += _frameLenSmpl;
    return *bitStreamLenByte;
}

WebRtc_Word16
ACMVORBIS::DecodeSafe(
                      WebRtc_UWord8* /* bitStream        */,
                      WebRtc_Word16  /* bitStreamLenByte */,
                      WebRtc_Word16* /* audio            */,
                      WebRtc_Word16* /* audioSamples     */,
                      WebRtc_Word8*  /* speechType       */)
{
    return 0;
}

WebRtc_Word16
ACMVORBIS::InternalInitEncoder(
                               WebRtcACMCodecParams* codecParams)
{
    // sanity check
    if (_encoderInstPtr == NULL)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Cannot initialize Vorbis encoder, instance does not exist");
        return -1;
    }
  
    WebRtc_Word16 status = SetBitRateSafe((codecParams->codecInstant).rate);
    status += WebRtcVorbis_EncoderInit(_encoderInstPtr);
  
    if (status >= 0) {
        return 0;
    } else {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Error in initialization of Vorbis encoder");
        return -1;
    }
}

WebRtc_Word16
ACMVORBIS::InternalInitDecoder(
                               WebRtcACMCodecParams* codecParams)
{
    WebRtc_Word16 status;
  
    // sanity check
    if (_decoderInstPtr == NULL)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Cannot initialize Speex decoder, instance does not exist");
        return -1;
    }
    status = ((WebRtcVorbis_DecoderInit(_decoderInstPtr) < 0)? -1:0);
  
    if (status >= 0) {
        return 0;
    } else {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Error in initialization of Speex decoder");
        return -1;
    }
}
  
WebRtc_Word32 
ACMVORBIS::EncodeVorbisEndOfStream(
                                   WebRtc_Word8* encodedData,
                                   WebRtc_UWord32& encodedLengthInBytes)
{
    encodedLengthInBytes = 
      WebRtcVorbis_Encode(_encoderInstPtr, NULL, 0, (WebRtc_UWord8*)encodedData);
    return 0;
}

WebRtc_Word32
ACMVORBIS::CodecDef(
                    WebRtcNetEQ_CodecDef& codecDef,
                    const CodecInst& codecInst)
{
    if (!_decoderInitialized)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Error, Speex decoder is not initialized");
        return -1;
    }
  
    // Fill up the structure by calling
    // "SET_CODEC_PAR" & "SET_VORBIS_FUNCTION."
    // Then call NetEQ to add the codec to it's
    // database.
  
    switch(_samplingFrequency)
    {
        case 16000:
        {
//            SET_CODEC_PAR((codecDef), kDecoderSPEEX_16, codecInst.pltype,
//                          _decoderInstPtr, 16000);
            break;
        }
        default:
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                         "Unsupported sampling frequency for Vorbis");
      
            return -1;
        }
    }
  
//  SET_VORBIS_FUNCTIONS((codecDef));
  return 0;
}

ACMGenericCodec*
ACMVORBIS::CreateInstance(void)
{
    return NULL;
}

WebRtc_Word16
ACMVORBIS::InternalCreateEncoder()
{
    return WebRtcVorbis_CreateEnc(&_encoderInstPtr, _samplingFrequency);
}

void
ACMVORBIS::DestructEncoderSafe()
{
    if(_encoderInstPtr != NULL)
    {
        WebRtcVorbis_FreeEnc(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    // there is no encoder set the following
    _encoderExist = false;
    _encoderInitialized = false;
}

WebRtc_Word16
ACMVORBIS::InternalCreateDecoder()
{
    return WebRtcVorbis_CreateDec(&_decoderInstPtr, _samplingFrequency, 1);
}

void
ACMVORBIS::DestructDecoderSafe()
{
    if(_decoderInstPtr != NULL)
    {
        WebRtcVorbis_FreeDec(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
    // there is no encoder instance set the followings
    _decoderExist = false;
    _decoderInitialized = false;
}

WebRtc_Word16
ACMVORBIS::UnregisterFromNetEqSafe(
                                   ACMNetEQ* netEq,
                                   WebRtc_Word16 payloadType)
{
    if(payloadType != _decoderParams.codecInstant.pltype)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "Cannot unregister codec %s given payload-type %d does not match \
                     the stored payload type",
                     _decoderParams.codecInstant.plname,
                     payloadType,
                     _decoderParams.codecInstant.pltype);
        return -1;
    }
  
    switch(_samplingFrequency)
    {
        case 16000:
        {
            return -1; //netEq->RemoveCodec(kDecoderSPEEX_16);
        }
        default:
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                         "Could not unregister Speex from NetEQ. Sampling frequency doesn't match");
            return -1;
        }
    }
}

void
ACMVORBIS::InternalDestructEncoderInst(
                                       void* ptrInst)
{
    if(ptrInst != NULL)
    {
        WebRtcVorbis_FreeEnc((VORBIS_encinst_t_*)ptrInst);
    }
}
  
#endif
  
} // namespace webrtc
