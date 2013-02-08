
#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_VORBIS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_VORBIS_H_

#include "acm_generic_codec.h"

// forward declaration
struct VORBIS_encinst_t_;
struct VORBIS_decinst_t_;

namespace webrtc {
  
class ACMVORBIS : public ACMGenericCodec
{
public:
    ACMVORBIS(WebRtc_Word16 codecID);
    ~ACMVORBIS();

    // for FEC
    ACMGenericCodec* CreateInstance(void);
    
    WebRtc_Word16 InternalEncode(
                                 WebRtc_UWord8* bitstream,
                                 WebRtc_Word16* bitStreamLenByte);
    
    WebRtc_Word16 InternalInitEncoder(
                                      WebRtcACMCodecParams *codecParams);
    
    WebRtc_Word16 InternalInitDecoder(
                                      WebRtcACMCodecParams *codecParams);
  
    WebRtc_Word32 EncodeVorbisEndOfStream(
                                          WebRtc_Word8* encodedData,
                                          WebRtc_UWord32& encodedLengthInBytes);

    
protected:
    WebRtc_Word16 DecodeSafe(
                             WebRtc_UWord8* bitStream,
                             WebRtc_Word16  bitStreamLenByte,
                             WebRtc_Word16* audio,
                             WebRtc_Word16* audioSamples,
                             WebRtc_Word8*  speechType);
    
    WebRtc_Word32 CodecDef(
                           WebRtcNetEQ_CodecDef& codecDef,
                           const CodecInst&      codecInst);
    
    void DestructEncoderSafe();
    
    void DestructDecoderSafe();
    
    WebRtc_Word16 InternalCreateEncoder();
    
    WebRtc_Word16 InternalCreateDecoder();
    
    WebRtc_Word16 UnregisterFromNetEqSafe(
                                          ACMNetEQ*     netEq,
                                          WebRtc_Word16 payloadType);
    
    void InternalDestructEncoderInst(
                                     void* ptrInst);
    
    VORBIS_encinst_t_* _encoderInstPtr;
    VORBIS_decinst_t_* _decoderInstPtr;
    WebRtc_Word16      _samplingFrequency;
};

} // namespace webrtc

#endif  // WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_VORBIS_H_
