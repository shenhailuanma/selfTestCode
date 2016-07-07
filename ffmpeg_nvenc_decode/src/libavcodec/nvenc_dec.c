#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "libavutil/common.h"
#include "libavutil/fifo.h"
#include "libavutil/opt.h"

#include "avcodec.h"
#include "internal.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "nvEncodeAPI.h"

#if defined(_WIN32)
#define CUDAAPI __stdcall
#else
#define CUDAAPI
#endif

#if defined(_WIN32)
#define LOAD_FUNC(l, s) GetProcAddress(l, s)
#define DL_CLOSE_FUNC(l) FreeLibrary(l)
#else
#define LOAD_FUNC(l, s) dlsym(l, s)
#define DL_CLOSE_FUNC(l) dlclose(l)
#endif

typedef enum cudaError_enum {
    CUDA_SUCCESS = 0
} CUresult;

typedef int CUdevice;
typedef void* CUcontext;
typedef void *CUvideodecoder;
typedef void *CUvideoparser;
typedef void *CUstream;
typedef struct _CUcontextlock_st *CUvideoctxlock;

typedef enum cudaVideoCodec_enum {
    cudaVideoCodec_MPEG1=0,
    cudaVideoCodec_MPEG2,
    cudaVideoCodec_MPEG4,
    cudaVideoCodec_VC1,
    cudaVideoCodec_H264,
    cudaVideoCodec_JPEG,
    cudaVideoCodec_H264_SVC,
    cudaVideoCodec_H264_MVC,
    cudaVideoCodec_HEVC,
    cudaVideoCodec_NumCodecs,
    // Uncompressed YUV
    cudaVideoCodec_YUV420 = (('I'<<24)|('Y'<<16)|('U'<<8)|('V')),   // Y,U,V (4:2:0)
    cudaVideoCodec_YV12   = (('Y'<<24)|('V'<<16)|('1'<<8)|('2')),   // Y,V,U (4:2:0)
    cudaVideoCodec_NV12   = (('N'<<24)|('V'<<16)|('1'<<8)|('2')),   // Y,U,V (4:2:0)
    cudaVideoCodec_YUYV   = (('Y'<<24)|('U'<<16)|('Y'<<8)|('V')),   // YUYV/YUY2 (4:2:2)
    cudaVideoCodec_UYVY   = (('U'<<24)|('Y'<<16)|('V'<<8)|('Y'))    // UYVY (4:2:2)
} cudaVideoCodec;

typedef enum cudaVideoChromaFormat_enum {
    cudaVideoChromaFormat_Monochrome=0,
    cudaVideoChromaFormat_420,
    cudaVideoChromaFormat_422,
    cudaVideoChromaFormat_444
} cudaVideoChromaFormat;

typedef enum cudaVideoSurfaceFormat_enum {
    cudaVideoSurfaceFormat_NV12=0   // NV12 (currently the only supported output format)
} cudaVideoSurfaceFormat;

typedef enum cudaVideoDeinterlaceMode_enum {
    cudaVideoDeinterlaceMode_Weave=0,   // Weave both fields (no deinterlacing)
    cudaVideoDeinterlaceMode_Bob,       // Drop one field
    cudaVideoDeinterlaceMode_Adaptive   // Adaptive deinterlacing
} cudaVideoDeinterlaceMode;

typedef enum cudaVideoCreateFlags_enum {
    cudaVideoCreate_Default = 0x00,     // Default operation mode: use dedicated video engines
    cudaVideoCreate_PreferCUDA = 0x01,  // Use a CUDA-based decoder if faster than dedicated engines (requires a valid vidLock object for multi-threading)
    cudaVideoCreate_PreferDXVA = 0x02,  // Go through DXVA internally if possible (requires D3D9 interop)
    cudaVideoCreate_PreferCUVID = 0x04  // Use dedicated video engines directly
} cudaVideoCreateFlags;

// Video format
typedef struct
{
    cudaVideoCodec codec;           // Compression format
    struct {
        unsigned int numerator;     // frame rate numerator   (0 = unspecified or variable frame rate)
        unsigned int denominator;   // frame rate denominator (0 = unspecified or variable frame rate)
    } frame_rate;                   // frame rate = numerator / denominator (for example: 30000/1001)
    int progressive_sequence;       // 0=interlaced, 1=progressive
    unsigned int coded_width;       // coded frame width
    unsigned int coded_height;      // coded frame height 
    struct {                        // area of the frame that should be displayed
        int left;                   // typical example:
        int top;                    //   coded_width = 1920, coded_height = 1088
        int right;                  //   display_area = { 0,0,1920,1080 }
        int bottom;
    } display_area;
    cudaVideoChromaFormat chroma_format;    // Chroma format
    unsigned int bitrate;           // video bitrate (bps, 0=unknown)
    struct {                        // Display Aspect Ratio = x:y (4:3, 16:9, etc)
        int x;
        int y;
    } display_aspect_ratio;
    struct {
        unsigned char video_format;
        unsigned char color_primaries;
        unsigned char transfer_characteristics;
        unsigned char matrix_coefficients;
    } video_signal_description;
    unsigned int seqhdr_data_length;          // Additional bytes following (CUVIDEOFORMATEX)
} CUVIDEOFORMAT;


typedef struct _CUVIDDECODECREATEINFO
{
    // Decoding
    unsigned long ulWidth;          // Coded Sequence Width
    unsigned long ulHeight;         // Coded Sequence Height
    unsigned long ulNumDecodeSurfaces;  // Maximum number of internal decode surfaces
    cudaVideoCodec CodecType;        // cudaVideoCodec_XXX
    cudaVideoChromaFormat ChromaFormat; // cudaVideoChromaFormat_XXX (only 4:2:0 is currently supported)
    unsigned long ulCreationFlags;  // Decoder creation flags (cudaVideoCreateFlags_XXX)
    unsigned long Reserved1[5];     // Reserved for future use - set to zero
    struct {                        // area of the frame that should be displayed
        short left;
        short top;
        short right;
        short bottom;
    } display_area;
    // Output format
    cudaVideoSurfaceFormat OutputFormat;       // cudaVideoSurfaceFormat_XXX
    cudaVideoDeinterlaceMode DeinterlaceMode;  // cudaVideoDeinterlaceMode_XXX
    unsigned long ulTargetWidth;    // Post-processed Output Width 
    unsigned long ulTargetHeight;   // Post-processed Output Height
    unsigned long ulNumOutputSurfaces; // Maximum number of output surfaces simultaneously mapped
    CUvideoctxlock vidLock;         // If non-NULL, context lock used for synchronizing ownership of the cuda context
    struct {                        // target rectangle in the output frame (for aspect ratio conversion)
        short left;
        short top;
        short right;
        short bottom;
    } target_rect;                  // if a null rectangle is specified, {0,0,ulTargetWidth,ulTargetHeight} will be used
    unsigned long Reserved2[5];     // Reserved for future use - set to zero
} CUVIDDECODECREATEINFO;


typedef long long CUvideotimestamp;

// Video format including raw sequence header information
typedef struct
{
    CUVIDEOFORMAT format;
    unsigned char raw_seqhdr_data[1024];
} CUVIDEOFORMATEX;

// Video parser
//
typedef struct _CUVIDPARSERDISPINFO
{
    int picture_index;
    int progressive_frame;
    int top_field_first;
    int repeat_first_field; // Number of additional fields (1=ivtc, 2=frame doubling, 4=frame tripling, -1=unpaired field)
    CUvideotimestamp timestamp;
} CUVIDPARSERDISPINFO;

// MPEG-2 Picture Parameters
typedef struct _CUVIDMPEG2PICPARAMS
{
    int ForwardRefIdx;          // Picture index of forward reference (P/B-frames)
    int BackwardRefIdx;         // Picture index of backward reference (B-frames)
    int picture_coding_type;
    int full_pel_forward_vector;
    int full_pel_backward_vector;
    int f_code[2][2];
    int intra_dc_precision;
    int frame_pred_frame_dct;
    int concealment_motion_vectors;
    int q_scale_type;
    int intra_vlc_format;
    int alternate_scan;
    int top_field_first;
    // Quantization matrices (raster order)
    unsigned char QuantMatrixIntra[64];
    unsigned char QuantMatrixInter[64];
} CUVIDMPEG2PICPARAMS;


// H.264 Picture Parameters
typedef struct _CUVIDH264DPBENTRY
{
    int PicIdx;             // picture index of reference frame
    int FrameIdx;           // frame_num(short-term) or LongTermFrameIdx(long-term)
    int is_long_term;       // 0=short term reference, 1=long term reference
    int not_existing;       // non-existing reference frame (corresponding PicIdx should be set to -1)
    int used_for_reference; // 0=unused, 1=top_field, 2=bottom_field, 3=both_fields
    int FieldOrderCnt[2];   // field order count of top and bottom fields
} CUVIDH264DPBENTRY;

typedef struct _CUVIDH264MVCEXT
{
    int num_views_minus1;
    int view_id;
    unsigned char inter_view_flag;
    unsigned char num_inter_view_refs_l0;
    unsigned char num_inter_view_refs_l1;
    unsigned char MVCReserved8Bits;
    int InterViewRefsL0[16];
    int InterViewRefsL1[16];
} CUVIDH264MVCEXT;

typedef struct _CUVIDH264SVCEXT
{
    unsigned char profile_idc;
    unsigned char level_idc;
    unsigned char DQId;
    unsigned char DQIdMax;
    unsigned char disable_inter_layer_deblocking_filter_idc;
    unsigned char ref_layer_chroma_phase_y_plus1;
    signed char   inter_layer_slice_alpha_c0_offset_div2;
    signed char   inter_layer_slice_beta_offset_div2;
    
    unsigned short DPBEntryValidFlag;
    unsigned char inter_layer_deblocking_filter_control_present_flag;
    unsigned char extended_spatial_scalability_idc;
    unsigned char adaptive_tcoeff_level_prediction_flag;
    unsigned char slice_header_restriction_flag;
    unsigned char chroma_phase_x_plus1_flag;
    unsigned char chroma_phase_y_plus1;
    
    unsigned char tcoeff_level_prediction_flag;
    unsigned char constrained_intra_resampling_flag;
    unsigned char ref_layer_chroma_phase_x_plus1_flag;
    unsigned char store_ref_base_pic_flag;
    unsigned char Reserved8BitsA; 
    unsigned char Reserved8BitsB; 
    // For the 4 scaled_ref_layer_XX fields below,
    // if (extended_spatial_scalability_idc == 1), SPS field, G.7.3.2.1.4, add prefix "seq_"
    // if (extended_spatial_scalability_idc == 2), SLH field, G.7.3.3.4, 
    short scaled_ref_layer_left_offset;
    short scaled_ref_layer_top_offset;
    short scaled_ref_layer_right_offset;
    short scaled_ref_layer_bottom_offset;
    unsigned short Reserved16Bits;
    struct _CUVIDPICPARAMS *pNextLayer; // Points to the picparams for the next layer to be decoded. Linked list ends at the target layer.
    int bRefBaseLayer;                 // whether to store ref base pic
} CUVIDH264SVCEXT;

typedef struct _CUVIDH264PICPARAMS
{
    // SPS
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int delta_pic_order_always_zero_flag;
    int frame_mbs_only_flag;
    int direct_8x8_inference_flag;
    int num_ref_frames;             // NOTE: shall meet level 4.1 restrictions
    unsigned char residual_colour_transform_flag;
    unsigned char bit_depth_luma_minus8;    // Must be 0 (only 8-bit supported)
    unsigned char bit_depth_chroma_minus8;  // Must be 0 (only 8-bit supported)
    unsigned char qpprime_y_zero_transform_bypass_flag;
    // PPS
    int entropy_coding_mode_flag;
    int pic_order_present_flag;
    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int deblocking_filter_control_present_flag;
    int redundant_pic_cnt_present_flag;
    int transform_8x8_mode_flag;
    int MbaffFrameFlag;
    int constrained_intra_pred_flag;
    int chroma_qp_index_offset;
    int second_chroma_qp_index_offset;
    int ref_pic_flag;
    int frame_num;
    int CurrFieldOrderCnt[2];
    // DPB
    CUVIDH264DPBENTRY dpb[16];          // List of reference frames within the DPB
    // Quantization Matrices (raster-order)
    unsigned char WeightScale4x4[6][16];
    unsigned char WeightScale8x8[2][64];
    // FMO/ASO
    unsigned char fmo_aso_enable;
    unsigned char num_slice_groups_minus1;
    unsigned char slice_group_map_type;
    signed char pic_init_qs_minus26;
    unsigned int slice_group_change_rate_minus1;
    union
    {
        unsigned long long slice_group_map_addr;
        const unsigned char *pMb2SliceGroupMap;
    } fmo;
    unsigned int  Reserved[12];
    // SVC/MVC
    union
    {
        CUVIDH264MVCEXT mvcext;
        CUVIDH264SVCEXT svcext;
    };
} CUVIDH264PICPARAMS;

typedef struct _CUVIDMPEG4PICPARAMS
{
    int ForwardRefIdx;          // Picture index of forward reference (P/B-frames)
    int BackwardRefIdx;         // Picture index of backward reference (B-frames)
    // VOL
    int video_object_layer_width;
    int video_object_layer_height;
    int vop_time_increment_bitcount;
    int top_field_first;
    int resync_marker_disable;
    int quant_type;
    int quarter_sample;
    int short_video_header;
    int divx_flags;
    // VOP
    int vop_coding_type;
    int vop_coded;
    int vop_rounding_type;
    int alternate_vertical_scan_flag;
    int interlaced;
    int vop_fcode_forward;
    int vop_fcode_backward;
    int trd[2];
    int trb[2];
    // Quantization matrices (raster order)
    unsigned char QuantMatrixIntra[64];
    unsigned char QuantMatrixInter[64];
    int gmc_enabled;
} CUVIDMPEG4PICPARAMS;

// HEVC Picture Parameters
typedef struct _CUVIDHEVCPICPARAMS
{
    // sps
    int pic_width_in_luma_samples;
    int pic_height_in_luma_samples;
    unsigned char log2_min_luma_coding_block_size_minus3;
    unsigned char log2_diff_max_min_luma_coding_block_size;
    unsigned char log2_min_transform_block_size_minus2;
    unsigned char log2_diff_max_min_transform_block_size;
    unsigned char pcm_enabled_flag;
    unsigned char log2_min_pcm_luma_coding_block_size_minus3;
    unsigned char log2_diff_max_min_pcm_luma_coding_block_size;
    unsigned char pcm_sample_bit_depth_luma_minus1;
    
    unsigned char pcm_sample_bit_depth_chroma_minus1;
    unsigned char pcm_loop_filter_disabled_flag;
    unsigned char strong_intra_smoothing_enabled_flag;
    unsigned char max_transform_hierarchy_depth_intra;
    unsigned char max_transform_hierarchy_depth_inter;
    unsigned char amp_enabled_flag;
    unsigned char separate_colour_plane_flag;
    unsigned char log2_max_pic_order_cnt_lsb_minus4;

    unsigned char num_short_term_ref_pic_sets;
    unsigned char long_term_ref_pics_present_flag;
    unsigned char num_long_term_ref_pics_sps;
    unsigned char sps_temporal_mvp_enabled_flag;
    unsigned char sample_adaptive_offset_enabled_flag;
    unsigned char scaling_list_enable_flag;
    unsigned char IrapPicFlag;
    unsigned char IdrPicFlag;
    unsigned char reserved1[16];
    
    // pps
    unsigned char dependent_slice_segments_enabled_flag;
    unsigned char slice_segment_header_extension_present_flag;
    unsigned char sign_data_hiding_enabled_flag;
    unsigned char cu_qp_delta_enabled_flag;
    unsigned char diff_cu_qp_delta_depth;
    signed char init_qp_minus26;
    signed char pps_cb_qp_offset;
    signed char pps_cr_qp_offset;
    
    unsigned char constrained_intra_pred_flag;
    unsigned char weighted_pred_flag;
    unsigned char weighted_bipred_flag;
    unsigned char transform_skip_enabled_flag;
    unsigned char transquant_bypass_enabled_flag;
    unsigned char entropy_coding_sync_enabled_flag;
    unsigned char log2_parallel_merge_level_minus2;
    unsigned char num_extra_slice_header_bits;
    
    unsigned char loop_filter_across_tiles_enabled_flag;
    unsigned char loop_filter_across_slices_enabled_flag;
    unsigned char output_flag_present_flag;
    unsigned char num_ref_idx_l0_default_active_minus1;
    unsigned char num_ref_idx_l1_default_active_minus1;
    unsigned char lists_modification_present_flag;
    unsigned char cabac_init_present_flag;
    unsigned char pps_slice_chroma_qp_offsets_present_flag;
    
    unsigned char deblocking_filter_override_enabled_flag;
    unsigned char pps_deblocking_filter_disabled_flag;
    signed char pps_beta_offset_div2;
    signed char pps_tc_offset_div2;
    unsigned char tiles_enabled_flag;
    unsigned char uniform_spacing_flag;
    unsigned char num_tile_columns_minus1;
    unsigned char num_tile_rows_minus1;
    
    unsigned short column_width_minus1[21];
    unsigned short row_height_minus1[19];
    unsigned int reserved3[16];

    // RefPicSets
    int NumBitsForShortTermRPSInSlice;
    int NumDeltaPocsOfRefRpsIdx;
    int NumPocTotalCurr;
    int NumPocStCurrBefore;
    int NumPocStCurrAfter;
    int NumPocLtCurr;
    int CurrPicOrderCntVal;
    int RefPicIdx[16];                  // [refpic] Indices of valid reference pictures (-1 if unused for reference)
    int PicOrderCntVal[16];             // [refpic]
    unsigned char IsLongTerm[16];       // [refpic] 0=not a long-term reference, 1=long-term reference
    unsigned char RefPicSetStCurrBefore[8]; // [0..NumPocStCurrBefore-1] -> refpic (0..15)
    unsigned char RefPicSetStCurrAfter[8];  // [0..NumPocStCurrAfter-1] -> refpic (0..15)
    unsigned char RefPicSetLtCurr[8];       // [0..NumPocLtCurr-1] -> refpic (0..15)
    unsigned int reserved4[16];

    // scaling lists (diag order)
    unsigned char ScalingList4x4[6][16];       // [matrixId][i]
    unsigned char ScalingList8x8[6][64];       // [matrixId][i]
    unsigned char ScalingList16x16[6][64];     // [matrixId][i]
    unsigned char ScalingList32x32[2][64];     // [matrixId][i]
    unsigned char ScalingListDCCoeff16x16[6];  // [matrixId]
    unsigned char ScalingListDCCoeff32x32[2];  // [matrixId]
} CUVIDHEVCPICPARAMS;


// Picture Parameters for Decoding 
typedef struct _CUVIDPICPARAMS
{
    int PicWidthInMbs;      // Coded Frame Size
    int FrameHeightInMbs;   // Coded Frame Height
    int CurrPicIdx;         // Output index of the current picture
    int field_pic_flag;     // 0=frame picture, 1=field picture
    int bottom_field_flag;  // 0=top field, 1=bottom field (ignored if field_pic_flag=0)
    int second_field;       // Second field of a complementary field pair
    // Bitstream data
    unsigned int nBitstreamDataLen;        // Number of bytes in bitstream data buffer
    const unsigned char *pBitstreamData;   // Ptr to bitstream data for this picture (slice-layer)
    unsigned int nNumSlices;               // Number of slices in this picture
    const unsigned int *pSliceDataOffsets; // nNumSlices entries, contains offset of each slice within the bitstream data buffer
    int ref_pic_flag;       // This picture is a reference picture
    int intra_pic_flag;     // This picture is entirely intra coded
    unsigned int Reserved[30];             // Reserved for future use
    // Codec-specific data
    union {
        CUVIDMPEG2PICPARAMS mpeg2;          // Also used for MPEG-1
        CUVIDH264PICPARAMS h264;
        //CUVIDVC1PICPARAMS vc1; // fixme: not to suport it
        CUVIDMPEG4PICPARAMS mpeg4;
        //CUVIDJPEGPICPARAMS jpeg; // fixme: not to suport it
        CUVIDHEVCPICPARAMS hevc;
        unsigned int CodecReserved[1024];
    } CodecSpecific;
} CUVIDPICPARAMS;


// Post-processing
typedef struct _CUVIDPROCPARAMS
{
    int progressive_frame;  // Input is progressive (deinterlace_mode will be ignored)
    int second_field;       // Output the second field (ignored if deinterlace mode is Weave)
    int top_field_first;    // Input frame is top field first (1st field is top, 2nd field is bottom)
    int unpaired_field;     // Input only contains one field (2nd field is invalid)
    // The fields below are used for raw YUV input
    unsigned int reserved_flags;        // Reserved for future use (set to zero)
    unsigned int reserved_zero;         // Reserved (set to zero)
    unsigned long long raw_input_dptr;  // Input CUdeviceptr for raw YUV extensions
    unsigned int raw_input_pitch;       // pitch in bytes of raw YUV input (should be aligned appropriately)
    unsigned int raw_input_format;      // Reserved for future use (set to zero)
    unsigned long long raw_output_dptr; // Reserved for future use (set to zero)
    unsigned int raw_output_pitch;      // Reserved for future use (set to zero)
    unsigned int Reserved[48];
    void *Reserved3[3];
} CUVIDPROCPARAMS;

typedef struct _CUVIDSOURCEDATAPACKET
{
    unsigned long flags;            // Combination of CUVID_PKT_XXX flags
    unsigned long payload_size;     // number of bytes in the payload (may be zero if EOS flag is set)
    const unsigned char *payload;   // Pointer to packet payload data (may be NULL if EOS flag is set)
    CUvideotimestamp timestamp;     // Presentation timestamp (10MHz clock), only valid if CUVID_PKT_TIMESTAMP flag is set
} CUVIDSOURCEDATAPACKET;

//
// Parser callbacks
// The parser will call these synchronously from within cuvidParseVideoData(), whenever a picture is ready to
// be decoded and/or displayed.
//
typedef int (CUDAAPI *PFNVIDSEQUENCECALLBACK)(void *, CUVIDEOFORMAT *);
typedef int (CUDAAPI *PFNVIDDECODECALLBACK)(void *, CUVIDPICPARAMS *);
typedef int (CUDAAPI *PFNVIDDISPLAYCALLBACK)(void *, CUVIDPARSERDISPINFO *);

typedef struct _CUVIDPARSERPARAMS
{
    cudaVideoCodec CodecType;       // cudaVideoCodec_XXX
    unsigned int ulMaxNumDecodeSurfaces;    // Max # of decode surfaces (parser will cycle through these)
    unsigned int ulClockRate;       // Timestamp units in Hz (0=default=10000000Hz)
    unsigned int ulErrorThreshold;  // % Error threshold (0-100) for calling pfnDecodePicture (100=always call pfnDecodePicture even if picture bitstream is fully corrupted)
    unsigned int ulMaxDisplayDelay; // Max display queue delay (improves pipelining of decode with display) - 0=no delay (recommended values: 2..4)
    unsigned int uReserved1[5]; // Reserved for future use - set to 0
    void *pUserData;        // User data for callbacks
    PFNVIDSEQUENCECALLBACK pfnSequenceCallback; // Called before decoding frames and/or whenever there is a format change
    PFNVIDDECODECALLBACK pfnDecodePicture;      // Called when a picture is ready to be decoded (decode order)
    PFNVIDDISPLAYCALLBACK pfnDisplayPicture;    // Called whenever a picture is ready to be displayed (display order)
    void *pvReserved2[7];           // Reserved for future use - set to NULL
    CUVIDEOFORMATEX *pExtVideoInfo; // [Optional] sequence header data from system layer
} CUVIDPARSERPARAMS;


typedef void * CUdeviceptr;

//#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64) || defined(__aarch64__)
//    typedef unsigned long long CUdeviceptr;
//#else
//    typedef unsigned int CUdeviceptr;
//#endif

typedef CUresult(CUDAAPI *PCUINIT)(unsigned int Flags);
typedef CUresult(CUDAAPI *PCUDEVICEGETCOUNT)(int *count);
typedef CUresult(CUDAAPI *PCUDEVICEGET)(CUdevice *device, int ordinal);
typedef CUresult(CUDAAPI *PCUDEVICEGETNAME)(char *name, int len, CUdevice dev);
typedef CUresult(CUDAAPI *PCUDEVICECOMPUTECAPABILITY)(int *major, int *minor, CUdevice dev);
typedef CUresult(CUDAAPI *PCUCTXCREATE)(CUcontext *pctx, unsigned int flags, CUdevice dev);
typedef CUresult(CUDAAPI *PCUCTXPUSHCURRENT)(CUcontext ctx);// cuCtxPushCurrent
typedef CUresult(CUDAAPI *PCUCTXPOPCURRENT)(CUcontext *pctx); // cuCtxPopCurrent
typedef CUresult(CUDAAPI *PCUCTXDESTROY)(CUcontext ctx);
typedef CUresult(CUDAAPI *PCUMEMALLOCHOST)(void ** pp, size_t bytesize); 
typedef CUresult(CUDAAPI *PCUMEMFREEHOST)(void * p);
typedef CUresult(CUDAAPI *PCUSTREAMCREATE)(void ** phStream, unsigned int  Flags); // cuStreamCreate
typedef CUresult(CUDAAPI *PCUSTREAMDESTROY)(void * phStream, unsigned int  Flags); // cuStreamDestroy
typedef CUresult(CUDAAPI *PCUMEMCPYDTOHASYNC)(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount, void * hStream);// cuMemcpyDtoHAsync
typedef CUresult(CUDAAPI *PCUMEMCPYDTOH)(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount);// cuMemcpyDtoH



typedef CUresult(CUDAAPI *PCUVIDCREATEDECODER)(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci); // cuvidCreateDecoder
typedef CUresult(CUDAAPI *PCUVIDDESTROYDECODER)(CUvideodecoder hDecoder); // cuvidDestroyDecoder
typedef CUresult(CUDAAPI *PCUVIDDECODEPICTURE)(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);// cuvidDecodePicture
typedef CUresult(CUDAAPI *PCUVIDMAPVIDEOFRAME)(CUvideodecoder hDecoder, int nPicIdx,CUdeviceptr *pDevPtr, 
                                                unsigned int *pPitch, CUVIDPROCPARAMS *pVPP);   // cuvidMapVideoFrame
typedef CUresult(CUDAAPI *PCUVIDUNMAPVIDEOFRAME)(CUvideodecoder hDecoder, CUdeviceptr DevPtr); // cuvidUnmapVideoFrame
typedef CUresult(CUDAAPI *PCUVIDCTXLOCKCREATE)(CUvideoctxlock *pLock, CUcontext ctx); // cuvidCtxLockCreate
typedef CUresult(CUDAAPI *PCUVIDCTXLOCKDESTROY)(CUvideoctxlock lck); // cuvidCtxLockDestroy
typedef CUresult(CUDAAPI *PCUVIDCTXLOCK)(CUvideoctxlock lck, unsigned int reserved_flags); // cuvidCtxLock
typedef CUresult(CUDAAPI *PCUVIDCTXUNLOCK)(CUvideoctxlock lck, unsigned int reserved_flags); // cuvidCtxUnlock
typedef CUresult(CUDAAPI *PCUVIDCREATEVIDEOPARSER)(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams); // cuvidCreateVideoParser
typedef CUresult(CUDAAPI *PCUVIDPARSEVIDEODATA)(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);// cuvidParseVideoData
typedef CUresult(CUDAAPI *PCUVIDDESTROYVIDEOPARSER)(CUvideoparser obj);// cuvidDestroyVideoParser

typedef NVENCSTATUS (NVENCAPI* PNVENCODEAPICREATEINSTANCE)(NV_ENCODE_API_FUNCTION_LIST *functionList);

typedef struct NvencInputSurface
{
    NV_ENC_INPUT_PTR input_surface;
    int width;
    int height;

    int lockCount;

    NV_ENC_BUFFER_FORMAT format;
} NvencInputSurface;

typedef struct NvencOutputSurface
{
    NV_ENC_OUTPUT_PTR output_surface;
    int size;

    NvencInputSurface* input_surface;

    int busy;
} NvencOutputSurface;

typedef struct NvencData
{
    union {
        int64_t timestamp;
        NvencOutputSurface *surface;
    } u;
} NvencData;

typedef struct NvencDataList
{
    NvencData* data;

    uint32_t pos;
    uint32_t count;
    uint32_t size;
} NvencDataList;

typedef struct NvencDynLoadFunctions
{
    PCUINIT cu_init;
    PCUDEVICEGETCOUNT           cu_device_get_count;
    PCUDEVICEGET                cu_device_get;
    PCUDEVICEGETNAME            cu_device_get_name;
    PCUDEVICECOMPUTECAPABILITY  cu_device_compute_capability;
    PCUCTXCREATE                cu_ctx_create;
    PCUCTXPUSHCURRENT           cu_ctx_push_current;
    PCUCTXPOPCURRENT            cu_ctx_pop_current;
    PCUCTXDESTROY               cu_ctx_destroy;
    PCUMEMALLOCHOST             cu_mem_alloc_host;
    PCUMEMFREEHOST              cu_mem_free_host;
    PCUSTREAMCREATE             cu_stream_create;
    PCUSTREAMDESTROY            cu_stream_destory;
    PCUMEMCPYDTOHASYNC          cu_memcpy_dtoh_async;
    PCUMEMCPYDTOH               cu_memcpy_dtoh;

    // nvcuvid
    PCUVIDCREATEDECODER         cuvid_create_decoder;
    PCUVIDDESTROYDECODER        cuvid_destroy_decoder;
    PCUVIDDECODEPICTURE         cuvid_decode_picture;
    PCUVIDCTXLOCKCREATE         cuvid_lock_create;
    PCUVIDCTXLOCKDESTROY        cuvid_lock_destroy;
    PCUVIDCTXLOCK               cuvid_lock;
    PCUVIDCTXUNLOCK             cuvid_unlock;
    PCUVIDMAPVIDEOFRAME         cuvid_map_video_frame;
    PCUVIDUNMAPVIDEOFRAME       cuvid_unmap_video_frame;
    PCUVIDCREATEVIDEOPARSER     cuvid_create_video_parser;
    PCUVIDPARSEVIDEODATA        cuvid_parse_video_data;
    PCUVIDDESTROYVIDEOPARSER    cuvid_destroy_video_parser;

    NV_ENCODE_API_FUNCTION_LIST nvenc_funcs;
    int nvenc_device_count;
    CUdevice nvenc_devices[16];

#if defined(_WIN32)
    HMODULE cuda_lib;
    HMODULE nvenc_lib;
    HMODULE nvenc_dec_lib;
#else
    void* cuda_lib;
    void* nvenc_lib;
    void* nvenc_dec_lib;
#endif
} NvencDynLoadFunctions;


#define NVDEC_MAX_QUEUE_SIZE  (20)

typedef struct NvencDecContext {
    AVClass *class;

    NvencDynLoadFunctions nvenc_dload_funcs;
    int device_count; // cuda device count
    CUcontext cu_context; // cuda context

    CUvideodecoder          h_decoder; // video decoder handle
    CUvideoparser           h_parser;  // video parser handle
    CUstream                h_stream;

    CUVIDDECODECREATEINFO   video_decode_create_info; 
    cudaVideoCreateFlags    video_create_flags;
    CUvideoctxlock          vid_ctx_lock;
    CUVIDPARSERPARAMS       video_parser_params;


    // the filter for converting to Annex B
    AVBitStreamFilterContext *bsf;

    // runtime
    int decode_frame_cont;
    CUdeviceptr  pDecodedFrame[2]; 
    unsigned char *pFrameYUV[4];
    int             nReadPosition_;
    int             nFramesInQueue_;
    CUVIDPARSERDISPINFO aDisplayQueue_[NVDEC_MAX_QUEUE_SIZE];
    int        aIsFrameInUse_[NVDEC_MAX_QUEUE_SIZE];


    AVFrame tmp_frame; // fixme: 
    int have_decoded_frames; // fixme: should delete after test

    /* Options */
    int gpu;


    // for test
    FILE *fpWriteYUV ;

} NvencDecContext;





static void print_hex(unsigned char * data, int len)
{
    int i;

    for(i = 0; i < len; i++)
        printf("%2x ", data[i]);
    printf("\n");
}


#define CHECK_LOAD_FUNC(t, f, s) \
do { \
    (f) = (t)LOAD_FUNC(dl_fn->cuda_lib, s); \
    if (!(f)) { \
        av_log(avctx, AV_LOG_FATAL, "Failed loading %s from CUDA library\n", s); \
        goto cuda_error; \
    } \
} while (0)

static av_cold int nvenc_dyload_cuda(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    if (dl_fn->cuda_lib)
        return 1;

#if defined(_WIN32)
    dl_fn->cuda_lib = LoadLibrary(TEXT("nvcuda.dll"));
#else
    dl_fn->cuda_lib = dlopen("libcuda.so", RTLD_LAZY);
#endif

    if (!dl_fn->cuda_lib) {
        av_log(avctx, AV_LOG_FATAL, "Failed loading CUDA library\n");
        goto cuda_error;
    }

    CHECK_LOAD_FUNC(PCUINIT, dl_fn->cu_init, "cuInit");
    CHECK_LOAD_FUNC(PCUDEVICEGETCOUNT, dl_fn->cu_device_get_count, "cuDeviceGetCount");
    CHECK_LOAD_FUNC(PCUDEVICEGET, dl_fn->cu_device_get, "cuDeviceGet");
    CHECK_LOAD_FUNC(PCUDEVICEGETNAME, dl_fn->cu_device_get_name, "cuDeviceGetName");
    CHECK_LOAD_FUNC(PCUDEVICECOMPUTECAPABILITY, dl_fn->cu_device_compute_capability, "cuDeviceComputeCapability");
    //CHECK_LOAD_FUNC(PCUCTXCREATE, dl_fn->cu_ctx_create, "cuCtxCreate");
    CHECK_LOAD_FUNC(PCUCTXCREATE, dl_fn->cu_ctx_create, "cuCtxCreate_v2");
    CHECK_LOAD_FUNC(PCUCTXPUSHCURRENT, dl_fn->cu_ctx_push_current, "cuCtxPushCurrent_v2");
    CHECK_LOAD_FUNC(PCUCTXPOPCURRENT, dl_fn->cu_ctx_pop_current, "cuCtxPopCurrent_v2");
    CHECK_LOAD_FUNC(PCUCTXDESTROY, dl_fn->cu_ctx_destroy, "cuCtxDestroy_v2");

    CHECK_LOAD_FUNC(PCUMEMALLOCHOST, dl_fn->cu_mem_alloc_host, "cuMemAllocHost_v2");
    CHECK_LOAD_FUNC(PCUMEMFREEHOST, dl_fn->cu_mem_free_host, "cuMemFreeHost");
    CHECK_LOAD_FUNC(PCUSTREAMCREATE, dl_fn->cu_stream_create, "cuStreamCreate");
    CHECK_LOAD_FUNC(PCUSTREAMDESTROY, dl_fn->cu_stream_destory, "cuStreamDestroy");
    CHECK_LOAD_FUNC(PCUMEMCPYDTOHASYNC, dl_fn->cu_memcpy_dtoh_async, "cuMemcpyDtoHAsync_v2");
    CHECK_LOAD_FUNC(PCUMEMCPYDTOH, dl_fn->cu_memcpy_dtoh, "cuMemcpyDtoH_v2");

    return 1;

cuda_error:

    if (dl_fn->cuda_lib)
        DL_CLOSE_FUNC(dl_fn->cuda_lib);

    dl_fn->cuda_lib = NULL;

    return 0;
}


static av_cold int nvenc_decode_close(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_close begin\n");

    //ff_qsv_decode_close(&s->qsv);

    av_bitstream_filter_close(ctx->bsf);

    // free the cu video decoder

    // free the cu video parser

    // free the cu memory



    return 0;
}



static int CUDAAPI HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat)
{
    NvencDecContext *ctx = (NvencDecContext *)pUserData;

    av_log(NULL, AV_LOG_VERBOSE, "[HandleVideoSequence] begin\n");

    if ((pFormat->codec         != ctx->video_decode_create_info.CodecType)         // codec-type
        || (pFormat->coded_width   != ctx->video_decode_create_info.ulWidth)
        || (pFormat->coded_height  != ctx->video_decode_create_info.ulHeight)
        || (pFormat->chroma_format != ctx->video_decode_create_info.ChromaFormat))
    {
        // We don't deal with dynamic changes in video format
        return 0;
    }

    return 1;
}

static int CUDAAPI HandlePictureDecode(void *pUserData, CUVIDPICPARAMS *pPicParams)
{
    NvencDecContext *ctx = (NvencDecContext *)pUserData;
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDecode] begin\n");
    CUresult oResult = dl_fn->cuvid_decode_picture(ctx->h_decoder, pPicParams);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDecode] cuvidDecodePicture return:%d\n", oResult);

    return 1;
}

static int CUDAAPI HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pPicParams)
{
    NvencDecContext *ctx = (NvencDecContext *)pUserData;
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    int ret;


    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] begin\n");

    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] picture_index=%d, timestamp=%lld\n", pPicParams->picture_index, pPicParams->timestamp);

    ret = dl_fn->cu_ctx_push_current(ctx->cu_context);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cu_ctx_push_current ret=%d\n", ret);

    // map 
    CUVIDPROCPARAMS oVideoProcessingParameters;
    memset(&oVideoProcessingParameters, 0, sizeof(CUVIDPROCPARAMS));

    oVideoProcessingParameters.progressive_frame = pPicParams->progressive_frame;
    oVideoProcessingParameters.second_field      = 0;
    oVideoProcessingParameters.top_field_first   = pPicParams->top_field_first;
    oVideoProcessingParameters.unpaired_field    = (pPicParams->progressive_frame == 1 || pPicParams->repeat_first_field <= 1);

    unsigned int nDecodedPitch = 0;
    unsigned int nWidth = 0;
    unsigned int nHeight = 0;
    CUdeviceptr  pDecodedFrame[2] = { 0, 0 };

    //ret = dl_fn->cuvid_map_video_frame(ctx->h_decoder, pPicParams->picture_index, &ctx->pDecodedFrame[0], &nDecodedPitch, &oVideoProcessingParameters);
    ret = dl_fn->cuvid_map_video_frame(ctx->h_decoder, pPicParams->picture_index, &pDecodedFrame[0], &nDecodedPitch, &oVideoProcessingParameters);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cuvid_map_video_frame ret=%d, CUdeviceptr=%ld size=%d\n", ret, pDecodedFrame[0], sizeof(pDecodedFrame[0]));

    // memcpy from device to host async
    nHeight = ctx->video_decode_create_info.ulHeight;
    //ret = dl_fn->cu_memcpy_dtoh_async(ctx->pFrameYUV[0], pDecodedFrame[0], (nDecodedPitch * nHeight * 3 / 2), ctx->h_stream);
    ret = dl_fn->cu_memcpy_dtoh(ctx->pFrameYUV[0], pDecodedFrame[0], (nDecodedPitch * nHeight * 3 / 2));
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cu_memcpy_dtoh_async ret=%d, nDecodedPitch=%d, nHeight=%d ,stream=%lld, dest=%lld\n", 
            ret, nDecodedPitch, nHeight, ctx->h_stream, ctx->pFrameYUV[0]);

    // copy the data to frame
    print_hex(ctx->pFrameYUV[0], 30);
    static write_cnt = 0;

    if(write_cnt > 250)
        fwrite(ctx->pFrameYUV[0], 1, (nDecodedPitch * nHeight * 3 / 2), ctx->fpWriteYUV);
    write_cnt ++;
    // unmap
    //ret = dl_fn->cuvid_unmap_video_frame(ctx->h_decoder, ctx->pDecodedFrame[0]);
    ret = dl_fn->cuvid_unmap_video_frame(ctx->h_decoder, pDecodedFrame[0]);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cuvid_unmap_video_frame ret=%d\n", ret);

    ret = dl_fn->cu_ctx_pop_current(NULL);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cu_ctx_pop_current ret=%d\n", ret);

    ctx->have_decoded_frames = 1;
    return 1;
}


static av_cold int check_cuda_errors(AVCodecContext *avctx, CUresult err, const char *func)
{
    if (err != CUDA_SUCCESS) {
        av_log(avctx, AV_LOG_FATAL, ">> %s - failed with error code 0x%x\n", func, err);
        return 0;
    }
    return 1;
}
#define check_cuda_errors(f) if (!check_cuda_errors(avctx, f, #f)) goto nvenc_decode_init_fail

#define CHECK_LOAD_CUVID_FUNC(t, f, s) \
do { \
    (f) = (t)LOAD_FUNC(dl_fn->nvenc_dec_lib, s); \
    if (!(f)) { \
        av_log(avctx, AV_LOG_FATAL, "Failed loading %s from CUVID library\n", s); \
        goto nvenc_decode_init_fail; \
    } \
} while (0)

static av_cold int nvenc_decode_init(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init begin\n");
    av_log(NULL, AV_LOG_VERBOSE, "nvenc_decode_init test null\n");

    int ret = -1;
    CUresult cu_res;

    enum AVPixelFormat pix_fmts[2] = { AV_PIX_FMT_NV12,
                                       AV_PIX_FMT_NONE };

    ret = ff_get_format(avctx, pix_fmts);
    if (ret < 0)
        return ret;

    avctx->pix_fmt      = ret;


    /* init the cuda */
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    if(!nvenc_dyload_cuda(avctx)){
        av_log(avctx, AV_LOG_ERROR, "nvenc_decode_init: nvenc_dyload_cuda failed.\n");
        return -1;
    }
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init: after nvenc_dyload_cuda()\n");

    check_cuda_errors(dl_fn->cu_init(0));
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init: after cu_init(0);\n");

    check_cuda_errors(dl_fn->cu_device_get_count(&ctx->device_count));
    if (!ctx->device_count) {
        av_log(avctx, AV_LOG_FATAL, "No CUDA capable devices found\n");
        goto nvenc_decode_init_fail;
    }
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init: after cu_device_get_count, device_count=%d, select gpu:%d.\n", ctx->device_count, ctx->gpu);


    av_log(avctx, AV_LOG_VERBOSE, "video width=%d, height=%d, pix_fmt=%d\n", avctx->width, avctx->height, avctx->pix_fmt);


    CUdevice cu_device = 0;
    char gpu_name[128];
    int smminor = 0, smmajor = 0;
    int i, smver, target_smver;
    dl_fn->nvenc_device_count = 0;

    for (i = 0; i < ctx->device_count; ++i) {
        check_cuda_errors(dl_fn->cu_device_get(&cu_device, i));
        check_cuda_errors(dl_fn->cu_device_get_name(gpu_name, sizeof(gpu_name), cu_device));
        check_cuda_errors(dl_fn->cu_device_compute_capability(&smmajor, &smminor, cu_device));

        smver = (smmajor << 4) | smminor;

        av_log(avctx, AV_LOG_VERBOSE, "[ GPU #%d - < %s > has Compute SM %d.%d, NVENC %s ]\n", i, gpu_name, smmajor, smminor, (smver >= target_smver) ? "Available" : "Not Available");

        if (smver >= target_smver)
            dl_fn->nvenc_devices[dl_fn->nvenc_device_count++] = cu_device;
    }

    if (!dl_fn->nvenc_device_count) {
        av_log(avctx, AV_LOG_FATAL, "No NVENC capable devices found\n");
        goto nvenc_decode_init_fail;
    }


    // open the decoder lib
    #if defined(_WIN32)
        if (sizeof(void*) == 8) {
            dl_fn->nvenc_dec_lib = LoadLibrary(TEXT("nvcuvid.dll"));
        } else {
            dl_fn->nvenc_dec_lib = LoadLibrary(TEXT("nvcuvid.dll"));
        }
    #else
        dl_fn->nvenc_dec_lib = dlopen("libnvcuvid.so", RTLD_LAZY);
    #endif

    if (dl_fn->nvenc_dec_lib == NULL){
        av_log(avctx, AV_LOG_FATAL, "Failed loading the nvcuvid library\n");
        goto nvenc_decode_init_fail;
    }
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after load libnvcuvid.\n");

    CHECK_LOAD_CUVID_FUNC(PCUVIDCREATEDECODER, dl_fn->cuvid_create_decoder, "cuvidCreateDecoder");
    CHECK_LOAD_CUVID_FUNC(PCUVIDDESTROYDECODER, dl_fn->cuvid_destroy_decoder, "cuvidDestroyDecoder");
    CHECK_LOAD_CUVID_FUNC(PCUVIDDECODEPICTURE, dl_fn->cuvid_decode_picture, "cuvidDecodePicture");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXLOCKCREATE, dl_fn->cuvid_lock_create, "cuvidCtxLockCreate");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXLOCKDESTROY, dl_fn->cuvid_lock_destroy, "cuvidCtxLockDestroy");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXLOCK, dl_fn->cuvid_lock, "cuvidCtxLock");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXUNLOCK, dl_fn->cuvid_unlock, "cuvidCtxUnlock");
    //CHECK_LOAD_CUVID_FUNC(PCUVIDMAPVIDEOFRAME, dl_fn->cuvid_map_video_frame, "cuvidMapVideoFrame");
    CHECK_LOAD_CUVID_FUNC(PCUVIDMAPVIDEOFRAME, dl_fn->cuvid_map_video_frame, "cuvidMapVideoFrame64");
    //CHECK_LOAD_CUVID_FUNC(PCUVIDUNMAPVIDEOFRAME, dl_fn->cuvid_unmap_video_frame, "cuvidUnmapVideoFrame");    
    CHECK_LOAD_CUVID_FUNC(PCUVIDUNMAPVIDEOFRAME, dl_fn->cuvid_unmap_video_frame, "cuvidUnmapVideoFrame64");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCREATEVIDEOPARSER, dl_fn->cuvid_create_video_parser, "cuvidCreateVideoParser");
    CHECK_LOAD_CUVID_FUNC(PCUVIDPARSEVIDEODATA, dl_fn->cuvid_parse_video_data, "cuvidParseVideoData");
    CHECK_LOAD_CUVID_FUNC(PCUVIDDESTROYVIDEOPARSER, dl_fn->cuvid_destroy_video_parser, "cuvidDestroyVideoParser");

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after load libnvcuvid functions.\n");


    // create cuda context
    ctx->cu_context = NULL;
    check_cuda_errors(dl_fn->cu_ctx_create(&ctx->cu_context, 4, dl_fn->nvenc_devices[ctx->gpu]));  // CU_CTX_SCHED_BLOCKING_SYNC=4, avoid CPU spins


    /* to open the decoder */
    memset(&ctx->video_decode_create_info, 0, sizeof(ctx->video_decode_create_info));
    ctx->video_decode_create_info.CodecType = cudaVideoCodec_H264; // fixme: should support other codec in feature
    ctx->video_decode_create_info.ulWidth   = avctx->width;
    ctx->video_decode_create_info.ulHeight  = avctx->height;
    ctx->video_decode_create_info.ulNumDecodeSurfaces = 6; // fixme
    ctx->video_decode_create_info.ChromaFormat        = cudaVideoChromaFormat_420; // fixme
    ctx->video_decode_create_info.OutputFormat        = cudaVideoSurfaceFormat_NV12;
    ctx->video_decode_create_info.DeinterlaceMode     = cudaVideoDeinterlaceMode_Adaptive;

    // No scaling
    ctx->video_decode_create_info.ulTargetWidth       = ctx->video_decode_create_info.ulWidth;
    ctx->video_decode_create_info.ulTargetHeight      = ctx->video_decode_create_info.ulHeight;
    ctx->video_decode_create_info.ulNumOutputSurfaces = 6;  // fixme
    ctx->video_decode_create_info.ulCreationFlags     = cudaVideoCreate_Default; // fixme cudaVideoCreate_Default cudaVideoCreate_PreferCUVID

    check_cuda_errors(dl_fn->cuvid_lock_create(&ctx->vid_ctx_lock, ctx->cu_context));
    ctx->video_decode_create_info.vidLock             = ctx->vid_ctx_lock; // fixme

    check_cuda_errors(dl_fn->cuvid_create_decoder(&ctx->h_decoder, &ctx->video_decode_create_info));
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after create video decoder.\n");

    /* create parser */
    memset(&ctx->video_parser_params, 0, sizeof(ctx->video_parser_params));
    ctx->video_parser_params.CodecType              = cudaVideoCodec_H264; // fixme: should support other codec in feature
    ctx->video_parser_params.ulMaxNumDecodeSurfaces = ctx->video_decode_create_info.ulNumDecodeSurfaces; // same as decoder 
    ctx->video_parser_params.ulMaxDisplayDelay      = 1;  // this flag is needed so the parser will push frames out to the decoder as quickly as it can
    ctx->video_parser_params.pUserData              = ctx; // user data we just send the ctx
    ctx->video_parser_params.pfnSequenceCallback    = HandleVideoSequence;    // Called before decoding frames and/or whenever there is a format change
    ctx->video_parser_params.pfnDecodePicture       = HandlePictureDecode;    // Called when a picture is ready to be decoded (decode order)
    ctx->video_parser_params.pfnDisplayPicture      = HandlePictureDisplay;   // Called whenever a picture is ready to be displayed (display order)

    check_cuda_errors(dl_fn->cuvid_create_video_parser(&ctx->h_parser, &ctx->video_parser_params)); 
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after create video parser.\n");


    // alloc the memory host
    ctx->pFrameYUV[0] = NULL;
    ctx->pFrameYUV[1] = NULL;
    ctx->pFrameYUV[2] = NULL;
    ctx->pFrameYUV[3] = NULL;
    check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[0], 2048*1080*3/2)); // fixme: need cuMemFreeHost
    check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[1], 1920*1080*3/2));
    check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[2], 1920*1080*3/2));
    check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[3], 1920*1080*3/2));

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after alloc the memory host.\n");

    // Create a Stream ID for handling Readback
    check_cuda_errors(dl_fn->cu_stream_create(&ctx->h_stream, 0));
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after cu_stream_create, stream=%ld.\n", ctx->h_stream);

    // others
    if (avctx->codec_id == AV_CODEC_ID_H264)
        ctx->bsf = av_bitstream_filter_init("h264_mp4toannexb");
    else
        ctx->bsf = av_bitstream_filter_init("hevc_mp4toannexb");
    if (!ctx->bsf) {
        ret = AVERROR(ENOMEM);
        goto nvenc_decode_init_fail;
    }

    //sleep(6);
    // for test
    ctx->fpWriteYUV = fopen("/home/write.yuv","w+");

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init over\n");
    return 0;

nvenc_decode_init_fail:

    nvenc_decode_close(avctx);
    return -1;
}



static int nvenc_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame, AVPacket *avpkt)
{
    NvencDecContext *ctx = avctx->priv_data;

    //av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame begin\n");
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    AVFrame *frame    = data;
    int ret;
    int cu_ret;
    uint8_t *p_filtered = NULL;
    int      n_filtered = NULL;
    AVPacket pkt_filtered = { 0 };


    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame packet size=%d, stream_index=%d, pts=%lld, dts=%lld\n", 
        avpkt->size, avpkt->stream_index, avpkt->pts, avpkt->dts);

    // get the picture that has been decoded
    if(ctx->have_decoded_frames){
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame get frame\n");
        //*got_frame = 1;
    }

    // fill the data to decode
    if(avpkt->size > 0){
        if (avpkt->size > 3 && !avpkt->data[0] &&
            !avpkt->data[1] && !avpkt->data[2] && 1==avpkt->data[3]) {
            /* we already have annex-b prefix */
            av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame we already have annex-b prefix\n");

        } else {
            /* no annex-b prefix. try to restore: */
            av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame no annex-b prefix. try to restore\n");

            ret = av_bitstream_filter_filter(ctx->bsf, avctx, "private_spspps_buf",
                                         &p_filtered, &n_filtered,
                                         avpkt->data, avpkt->size, 0);
            if (ret >= 0) {
                pkt_filtered.pts  = avpkt->pts;
                pkt_filtered.data = p_filtered;
                pkt_filtered.size = n_filtered;

                // fill the packet to video parser
                CUVIDSOURCEDATAPACKET cu_pkt;
                cu_pkt.flags = 0;
                cu_pkt.payload_size = (unsigned long)pkt_filtered.size;
                cu_pkt.payload = pkt_filtered.data;
                cu_pkt.timestamp = pkt_filtered.pts;

                cu_ret = dl_fn->cuvid_parse_video_data(ctx->h_parser, &cu_pkt);
                av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame cuvid_parse_video_data cu_ret=%d\n", cu_ret);

                if (p_filtered != avpkt->data)
                    av_free(p_filtered);
                return ret > 0 ? avpkt->size : ret;
            }
        }
    }



    /*

    unsigned char * buffer = avpkt->data;
    int data_size = avpkt->size;

    print_hex(buffer, 10);
    if((avpkt->flags & AV_PKT_FLAG_KEY) && (avctx->extradata_size > 0) ){
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame this is key frame, extradata_size=%d\n", avctx->extradata_size);
        data_size += avctx->extradata_size;
        buffer = av_malloc(data_size);
        memcpy(buffer, avctx->extradata, avctx->extradata_size);
        memcpy(buffer+avctx->extradata_size, avpkt->data, avpkt->size);
        print_hex(buffer, 10+avctx->extradata_size);
    }


    static int frame_cnt = 0;
    // fill the packet to video parser
    CUVIDSOURCEDATAPACKET cu_pkt;
    cu_pkt.flags = 0;
    cu_pkt.payload_size = (unsigned long)data_size;
    cu_pkt.payload = buffer;
    cu_pkt.timestamp = avpkt->dts;

    ret = dl_fn->cuvid_parse_video_data(ctx->h_parser, &cu_pkt);
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame cuvid_parse_video_data ret=%d\n", ret);
    */

    ret = avpkt->size;  // fixme:

    return ret;
}

static void nvenc_decode_flush(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_flush begin\n");

}



#define OFFSET(x) offsetof(NvencDecContext, x)
#define VD AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    { "gpu", "Selects which NVENC capable GPU to use. First GPU is 0, second is 1, and so on.", OFFSET(gpu), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, INT_MAX, VD },
    { NULL },
};

static const AVClass nvenc_h264_class = {
    .class_name = "nvenc_h264",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_nvenc_h264_decoder = {
    .name           = "nvenc_h264",
    .long_name      = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (Nvidia nvenc)"),
    .priv_data_size = sizeof(NvencDecContext),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .init           = nvenc_decode_init,
    .decode         = nvenc_decode_frame,
    .flush          = nvenc_decode_flush,
    .close          = nvenc_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1,
    .priv_class     = &nvenc_h264_class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12, AV_PIX_FMT_NONE },
};

