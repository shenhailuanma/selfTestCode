#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "libavutil/common.h"
#include "libavutil/fifo.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"

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

// Data packet
typedef enum {
    CUVID_PKT_ENDOFSTREAM = 0x01,   // Set when this is the last packet for this stream
    CUVID_PKT_TIMESTAMP = 0x02,   // Timestamp is valid
    CUVID_PKT_DISCONTINUITY = 0x04    // Set when a discontinuity has to be signalled
} CUvideopacketflags;

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


typedef CUresult(CUDAAPI *PCUINIT)(unsigned int Flags); // cuInit
typedef CUresult(CUDAAPI *PCUDEVICEGETCOUNT)(int *count); // cuDeviceGetCount
typedef CUresult(CUDAAPI *PCUDEVICEGET)(CUdevice *device, int ordinal); // cuDeviceGet
typedef CUresult(CUDAAPI *PCUDEVICEGETNAME)(char *name, int len, CUdevice dev); // cuDeviceGetName
typedef CUresult(CUDAAPI *PCUDEVICECOMPUTECAPABILITY)(int *major, int *minor, CUdevice dev); // cuDeviceComputeCapability
typedef CUresult(CUDAAPI *PCUCTXCREATE)(CUcontext *pctx, unsigned int flags, CUdevice dev); // cuCtxCreate
typedef CUresult(CUDAAPI *PCUCTXPUSHCURRENT)(CUcontext ctx);// cuCtxPushCurrent
typedef CUresult(CUDAAPI *PCUCTXPOPCURRENT)(CUcontext *pctx); // cuCtxPopCurrent
typedef CUresult(CUDAAPI *PCUCTXDESTROY)(CUcontext ctx); // cuCtxDestroy
typedef CUresult(CUDAAPI *PCUMEMALLOCHOST)(void ** pp, size_t bytesize);  // cuMemAllocHost
typedef CUresult(CUDAAPI *PCUMEMFREEHOST)(void * p); // cuMemFreeHost
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


/** 
 * Return values for NVML API calls. 
 */
typedef enum nvmlReturn_enum 
{
    NVML_SUCCESS = 0,                   //!< The operation was successful
    NVML_ERROR_UNINITIALIZED = 1,       //!< NVML was not first initialized with nvmlInit()
    NVML_ERROR_INVALID_ARGUMENT = 2,    //!< A supplied argument is invalid
    NVML_ERROR_NOT_SUPPORTED = 3,       //!< The requested operation is not available on target device
    NVML_ERROR_NO_PERMISSION = 4,       //!< The current user does not have permission for operation
    NVML_ERROR_ALREADY_INITIALIZED = 5, //!< Deprecated: Multiple initializations are now allowed through ref counting
    NVML_ERROR_NOT_FOUND = 6,           //!< A query to find an object was unsuccessful
    NVML_ERROR_INSUFFICIENT_SIZE = 7,   //!< An input argument is not large enough
    NVML_ERROR_INSUFFICIENT_POWER = 8,  //!< A device's external power cables are not properly attached
    NVML_ERROR_DRIVER_NOT_LOADED = 9,   //!< NVIDIA driver is not loaded
    NVML_ERROR_TIMEOUT = 10,            //!< User provided timeout passed
    NVML_ERROR_UNKNOWN = 999            //!< An internal driver error occurred
} nvmlReturn_t;

typedef void * nvmlDevice_t;

/* Memory allocation information for a device. */
typedef struct nvmlMemory_st 
{
    unsigned long long total;        //!< Total installed FB memory (in bytes)
    unsigned long long free;         //!< Unallocated FB memory (in bytes)
    unsigned long long used;         //!< Allocated FB memory (in bytes). Note that the driver/GPU always sets aside a small amount of memory for bookkeeping
} nvmlMemory_t;


/* Information about running compute processes on the GPU */
typedef struct nvmlProcessInfo_st
{
    unsigned int pid;                 //!< Process ID
    unsigned long long usedGpuMemory; //!< Amount of used GPU memory in bytes.
                                      //!< Under WDDM, \ref NVML_VALUE_NOT_AVAILABLE is always reported
                                      //!< because Windows KMD manages all the memory and not the NVIDIA driver
} nvmlProcessInfo_t;

/* Utilization information for a device. */
typedef struct nvmlUtilization_st 
{
    unsigned int gpu;                //!< Percent of time over the past second during which one or more kernels was executing on the GPU
    unsigned int memory;             //!< Percent of time over the past second during which global (device) memory was being read or written
} nvmlUtilization_t;

typedef nvmlReturn_t(CUDAAPI *NVMLINIT)(void);  // nvmlInit
typedef nvmlReturn_t(CUDAAPI *NVMLSHUTDOWN)(void);  // nvmlShutdown 
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETCOUNT)(unsigned int *deviceCount); // nvmlDeviceGetCount
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETHANDLEBYINDEX)(unsigned int index, nvmlDevice_t *device); // nvmlDeviceGetHandleByIndex
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETDECODERUTILIZATION)(nvmlDevice_t device, unsigned int *utilization,unsigned int *samplingPeriodUs); // nvmlDeviceGetDecoderUtilization
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETENCODERUTILIZATION)(nvmlDevice_t device, unsigned int *utilization,unsigned int *samplingPeriodUs); // nvmlDeviceGetEncoderUtilization 
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETMEMORYINFO)(nvmlDevice_t device, nvmlMemory_t *memory); // nvmlDeviceGetMemoryInfo
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETRUNNINGPROCESSES)(nvmlDevice_t device, unsigned int *infoCount,nvmlProcessInfo_t *infos);// nvmlDeviceGetComputeRunningProcesses
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETPPROCESSNAME)(unsigned int pid, char *name, unsigned int length); // nvmlSystemGetProcessName
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETUTILIZATIONRATES)(nvmlDevice_t device, nvmlUtilization_t *utilization); // nvmlDeviceGetUtilizationRates
typedef nvmlReturn_t(CUDAAPI *NVMLDEVICEGETTEMPERATURE)(nvmlDevice_t device, int sensorType, unsigned int *temp); // nvmlDeviceGetTemperature


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


    NVMLINIT                    nvml_init;
    NVMLSHUTDOWN                nvml_shutdown;
    NVMLDEVICEGETCOUNT          nvml_device_get_count;
    NVMLDEVICEGETHANDLEBYINDEX  nvml_device_get_handle_by_index;
    NVMLDEVICEGETDECODERUTILIZATION     nvml_device_get_decoder_utilization;
    NVMLDEVICEGETENCODERUTILIZATION     nvml_device_get_encoder_utilization;
    NVMLDEVICEGETMEMORYINFO     nvml_device_get_memory_info;
    NVMLDEVICEGETRUNNINGPROCESSES       nvml_device_get_running_processes;
    NVMLDEVICEGETPPROCESSNAME   nvml_device_get_process_name;
    NVMLDEVICEGETUTILIZATIONRATES       nvml_device_get_utilization_rates;
    NVMLDEVICEGETTEMPERATURE    nvml_device_get_temperature;

    NV_ENCODE_API_FUNCTION_LIST nvenc_funcs;
    int nvenc_device_count;
    CUdevice nvenc_devices[16];

#if defined(_WIN32)
    HMODULE cuda_lib;
    HMODULE nvenc_lib;
    HMODULE nvenc_dec_lib;
    HMODULE nvml_lib;
#else
    void* cuda_lib;
    void* nvenc_lib;
    void* nvenc_dec_lib;
    void* nvml_lib;
#endif
} NvencDynLoadFunctions;



typedef struct {
    int decode_width;
    int decode_height;
    int decode_linesize;

    unsigned char *data;
    int data_size;
    int64_t pts;
}DecodeFrame;


#define NVDEC_MAX_QUEUE_SIZE  (20)
#define NVDEC_MEMORY_NUMBER   (4)

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
    int parser_width;
    int parser_height;


    DecodeFrame         aFrameQueue[NVDEC_MAX_QUEUE_SIZE];
    //AVFrame   *         aFrameQueue[NVDEC_MAX_QUEUE_SIZE];
    volatile int        nFrameQueueReadPosition;
    volatile int        nFramesInFrameQueue;

    CUdeviceptr  pDecodedFrame[2]; 
    unsigned char *pFrameYUV[NVDEC_MEMORY_NUMBER];
    volatile int        nReadPosition;
    volatile int        nFramesInQueue;
    CUVIDPARSERDISPINFO aDisplayQueue[NVDEC_MAX_QUEUE_SIZE];
    volatile int        aIsFrameInUse[NVDEC_MAX_QUEUE_SIZE]; 
    volatile int        bEndOfDecode;




    AVFrame tmp_frame; // fixme: 
    int have_decoded_frames; // fixme: should delete after test

    /* Options */
    int gpu;


    // for test
    FILE *fpWriteYUV ;
    FILE *fpWriteES ;

} NvencDecContext;





static void print_hex(unsigned char * data, int len)
{
    int i;

    for(i = 0; i < len; i++)
        printf("%2x ", data[i]);
    printf("\n");
}
/*
static void frame_enqueue1(NvencDecContext *ctx, AVFrame * pFrame)
{
    int bPlacedFrame = 0;
    int iWritePosition = 0;

    do{
        if (ctx->nFramesInFrameQueue < NVDEC_MAX_QUEUE_SIZE)
        {
            iWritePosition = (ctx->nFrameQueueReadPosition + ctx->nFramesInFrameQueue) % NVDEC_MAX_QUEUE_SIZE;
            ctx->aFrameQueue[iWritePosition] = pFrame;
            ctx->nFramesInFrameQueue++;
            bPlacedFrame = 1;
        }


        if (bPlacedFrame) // Done
            break;

        sleep(1);   // Wait a bit

    }while(!ctx->bEndOfDecode);
}

static int frame_dequeue1(NvencDecContext *ctx, AVFrame ** pFrame)
{
    int bHaveNewFrame = 0;


    if (ctx->nFramesInFrameQueue > 0)
    {
        int iEntry = ctx->nFrameQueueReadPosition;
        *pFrame = ctx->aFrameQueue[iEntry];
        ctx->nFrameQueueReadPosition = (iEntry+1) % NVDEC_MAX_QUEUE_SIZE;
        ctx->nFramesInFrameQueue--;
        bHaveNewFrame = 1;
        ctx->aFrameQueue[iEntry] = NULL;
    }


    return bHaveNewFrame;
}
*/
static void frame_enqueue(NvencDecContext *ctx, const DecodeFrame * pFrame)
{
    int bPlacedFrame = 0;
    int iWritePosition = 0;

    do{
        if (ctx->nFramesInFrameQueue < NVDEC_MAX_QUEUE_SIZE)
        {
            iWritePosition = (ctx->nFrameQueueReadPosition + ctx->nFramesInFrameQueue) % NVDEC_MAX_QUEUE_SIZE;
            ctx->aFrameQueue[iWritePosition] = *pFrame;
            ctx->nFramesInFrameQueue++;
            bPlacedFrame = 1;
        }


        if (bPlacedFrame) // Done
            break;

        sleep(1);   // Wait a bit

    }while(!ctx->bEndOfDecode);
}

static int frame_dequeue(NvencDecContext *ctx, DecodeFrame * pFrame)
{
    int bHaveNewFrame = 0;


    if (ctx->nFramesInFrameQueue > 0)
    {
        int iEntry = ctx->nFrameQueueReadPosition;
        *pFrame = ctx->aFrameQueue[iEntry];
        ctx->nFrameQueueReadPosition = (iEntry+1) % NVDEC_MAX_QUEUE_SIZE;
        ctx->nFramesInFrameQueue--;
        bHaveNewFrame = 1;
    }


    return bHaveNewFrame;
}

static void pic_enqueue(NvencDecContext *ctx, const CUVIDPARSERDISPINFO *pPicParams)
{
    ctx->aIsFrameInUse[pPicParams->picture_index] = 1;

    int bPlacedFrame = 0;
    int iWritePosition = 0;
    do{
        if (ctx->nFramesInQueue < NVDEC_MAX_QUEUE_SIZE)
        {
            iWritePosition = (ctx->nReadPosition + ctx->nFramesInQueue) % NVDEC_MAX_QUEUE_SIZE;
            ctx->aDisplayQueue[iWritePosition] = *pPicParams;
            ctx->nFramesInQueue++;
            bPlacedFrame = 1;
        }


        if (bPlacedFrame) // Done
            break;

        sleep(1);   // Wait a bit

    }while(!ctx->bEndOfDecode);

}

static int  pic_dequeue(NvencDecContext *ctx, CUVIDPARSERDISPINFO *pPicParams)
{
    pPicParams->picture_index = -1;
    int bHaveNewFrame = 0;


    if (ctx->nFramesInQueue > 0)
    {
        int iEntry = ctx->nReadPosition;
        *pPicParams = ctx->aDisplayQueue[iEntry];
        ctx->nReadPosition = (iEntry+1) % NVDEC_MAX_QUEUE_SIZE;
        ctx->nFramesInQueue--;
        bHaveNewFrame = 1;
    }


    return bHaveNewFrame;
}

static void pic_release(NvencDecContext *ctx, const CUVIDPARSERDISPINFO *pPicParams)
{
    ctx->aIsFrameInUse[pPicParams->picture_index] = 0;
}

static int pic_wait_frame_available(NvencDecContext *ctx, int nPictureIndex)
{
    while (0 != ctx->aIsFrameInUse[nPictureIndex])
    {
        sleep(1);   // Decoder is getting too far ahead from display

        if (0 != ctx->bEndOfDecode)
            return 0;
    }

    return 1;
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
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;
    int ret;
    int i;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_close begin\n");

    //ff_qsv_decode_close(&s->qsv);

    av_bitstream_filter_close(ctx->bsf);

    // free the cu video decoder
    if(ctx->h_decoder){
        dl_fn->cuvid_destroy_decoder(ctx->h_decoder);
        ctx->h_decoder = NULL;
    }

    // free the cu video parser
    if(ctx->h_parser){
        dl_fn->cuvid_destroy_video_parser(ctx->h_parser);
        ctx->h_parser = NULL;
    }

    // free lock
    if(ctx->vid_ctx_lock){
        dl_fn->cuvid_lock_destroy(ctx->vid_ctx_lock);
        ctx->vid_ctx_lock = NULL;
    }

    // free context
    if(ctx->cu_context){
        dl_fn->cu_ctx_destroy(ctx->cu_context);
        ctx->cu_context = NULL;
    }


    // free the cu memory
    for(i = 0; i < NVDEC_MEMORY_NUMBER; i++){
        if(ctx->pFrameYUV[i]){
            dl_fn->cu_mem_free_host(ctx->pFrameYUV[i]);
            ctx->pFrameYUV[i] = NULL;
        }
    }




    return 0;
}

static int get_suitable_gpu(void);

static int nvenc_open_decoder(NvencDecContext *ctx, int width, int height)
{
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;
    int ret ;

    ctx->gpu = get_suitable_gpu();

    // create cuda context
    ctx->cu_context = NULL;
    ret = dl_fn->cu_ctx_create(&ctx->cu_context, 4, dl_fn->nvenc_devices[ctx->gpu]);  //CU_CTX_SCHED_AUTO=0, CU_CTX_SCHED_BLOCKING_SYNC=4, avoid CPU spins
    if (ret != CUDA_SUCCESS) {
        av_log(NULL, AV_LOG_FATAL, ">> cu_ctx_create - failed with error code %d\n", ret);
        return -1;
    }

    // alloc the memory host
    if(ctx->pFrameYUV[0] == NULL){
        ret = dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[0], 2048*1088*3/2); 
        if (ret != CUDA_SUCCESS) {
            av_log(NULL, AV_LOG_FATAL, ">> cu_mem_alloc_host - failed with error code %d\n", ret);
            return -1;
        }
    }
    //check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[1], 2048*1088*3/2));
    //check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[2], 1920*1080*3/2));
    //check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[3], 1920*1080*3/2));

    av_log(NULL, AV_LOG_VERBOSE, "[nvenc_open_decoder] after alloc the memory host.\n");


    memset(&ctx->video_decode_create_info, 0, sizeof(ctx->video_decode_create_info));
    ctx->video_decode_create_info.CodecType = cudaVideoCodec_H264; // fixme: should support other codec in feature
    ctx->video_decode_create_info.ulWidth   = FFALIGN(width, 16);
    ctx->video_decode_create_info.ulHeight  = FFALIGN(height, 16);
    ctx->video_decode_create_info.ulNumDecodeSurfaces = 20; // fixme
    // Limit decode memory to 24MB (16M pixels at 4:2:0 = 24M bytes)
    // Keep atleast 6 DecodeSurfaces
    while (ctx->video_decode_create_info.ulNumDecodeSurfaces > 6 && 
        ctx->video_decode_create_info.ulNumDecodeSurfaces * ctx->video_decode_create_info.ulWidth * ctx->video_decode_create_info.ulHeight > 16 * 1024 * 1024)
    {
        ctx->video_decode_create_info.ulNumDecodeSurfaces--;
    }
    av_log(NULL, AV_LOG_INFO, "[nvenc_open_decoder] ulWidth=%d, ulHeight=%d, ulNumDecodeSurfaces=%d.\n", 
        ctx->video_decode_create_info.ulWidth, ctx->video_decode_create_info.ulHeight, ctx->video_decode_create_info.ulNumDecodeSurfaces);

    ctx->video_decode_create_info.ChromaFormat        = cudaVideoChromaFormat_420; 
    ctx->video_decode_create_info.OutputFormat        = cudaVideoSurfaceFormat_NV12;
    ctx->video_decode_create_info.DeinterlaceMode     = cudaVideoDeinterlaceMode_Weave; // cudaVideoDeinterlaceMode_Weave  cudaVideoDeinterlaceMode_Adaptive

    // No scaling
    ctx->video_decode_create_info.ulTargetWidth       = ctx->video_decode_create_info.ulWidth;
    ctx->video_decode_create_info.ulTargetHeight      = ctx->video_decode_create_info.ulHeight;
    ctx->video_decode_create_info.ulNumOutputSurfaces = 2;  // fixme
    ctx->video_decode_create_info.ulCreationFlags     = cudaVideoCreate_PreferCUVID; // fixme cudaVideoCreate_Default cudaVideoCreate_PreferCUVID cudaVideoCreate_PreferCUDA

    ret = dl_fn->cuvid_lock_create(&ctx->vid_ctx_lock, ctx->cu_context);
    if (ret != CUDA_SUCCESS) {
        av_log(NULL, AV_LOG_FATAL, ">> cuvid_lock_create - failed with error code %d\n", ret);
        return -1;
    }
    ctx->video_decode_create_info.vidLock             = ctx->vid_ctx_lock; // fixme

    ret = dl_fn->cuvid_create_decoder(&ctx->h_decoder, &ctx->video_decode_create_info);
    if (ret != CUDA_SUCCESS) {
        av_log(NULL, AV_LOG_FATAL, ">> cuvid_create_decoder - failed with error code %d\n",  ret);
        return -1;
    }

    av_log(NULL, AV_LOG_VERBOSE, "[nvenc_open_decoder] after create video decoder.\n");

    return 0;   
}



static int CUDAAPI HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat)
{
    NvencDecContext *ctx = (NvencDecContext *)pUserData;

    av_log(NULL, AV_LOG_INFO, "[HandleVideoSequence] codec=%d, coded_width=%d, coded_height=%d\n",
        pFormat->codec, pFormat->coded_width, pFormat->coded_height);

    if(ctx->h_decoder == NULL){

        ctx->parser_width = pFormat->coded_width;
        ctx->parser_height = pFormat->coded_height;

        nvenc_open_decoder(ctx, pFormat->coded_width, pFormat->coded_height);
    }

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



    //av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDecode] begin\n");

    if(ctx->h_decoder){
        int bFrameAvailable = pic_wait_frame_available(ctx, pPicParams->CurrPicIdx);

        if (!bFrameAvailable)
            return 0;

        CUresult oResult = dl_fn->cuvid_decode_picture(ctx->h_decoder, pPicParams);
        //av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDecode] cuvidDecodePicture return:%d\n", oResult);
    }

    return 1;
}

static int CUDAAPI HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pPicParams)
{
    NvencDecContext *ctx = (NvencDecContext *)pUserData;
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    int ret;

    //av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] picture_index=%d, timestamp=%lld\n", pPicParams->picture_index, pPicParams->timestamp);
    pic_enqueue(ctx, pPicParams);

    CUVIDPARSERDISPINFO oDisplayInfo;
    pic_dequeue(ctx, &oDisplayInfo);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] dequeue picture_index=%d, timestamp=%lld\n", oDisplayInfo.picture_index, oDisplayInfo.timestamp);


    ret = dl_fn->cu_ctx_push_current(ctx->cu_context);
    //av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cu_ctx_push_current ret=%d\n", ret);

    // map 
    CUVIDPROCPARAMS oVideoProcessingParameters;
    memset(&oVideoProcessingParameters, 0, sizeof(CUVIDPROCPARAMS));

    //oVideoProcessingParameters.progressive_frame = pPicParams->progressive_frame;
    oVideoProcessingParameters.progressive_frame = oDisplayInfo.progressive_frame;
    oVideoProcessingParameters.second_field      = 0;
    //oVideoProcessingParameters.top_field_first   = pPicParams->top_field_first;
    oVideoProcessingParameters.top_field_first   = oDisplayInfo.top_field_first;
    //oVideoProcessingParameters.unpaired_field    = (pPicParams->progressive_frame == 1 || pPicParams->repeat_first_field <= 1);
    oVideoProcessingParameters.unpaired_field    = (oDisplayInfo.progressive_frame == 1 || oDisplayInfo.repeat_first_field <= 1);

    unsigned int nDecodedPitch = 0;
    unsigned int nWidth = 0;
    unsigned int nHeight = 0;
    unsigned int nPicSize = 0;
    CUdeviceptr  pDecodedFrame[2] = { 0, 0 };

    //ret = dl_fn->cuvid_map_video_frame(ctx->h_decoder, pPicParams->picture_index, &ctx->pDecodedFrame[0], &nDecodedPitch, &oVideoProcessingParameters);
    ret = dl_fn->cuvid_map_video_frame(ctx->h_decoder, oDisplayInfo.picture_index, &pDecodedFrame[0], &nDecodedPitch, &oVideoProcessingParameters);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cuvid_map_video_frame ret=%d, CUdeviceptr=%ld size=%d\n", ret, pDecodedFrame[0], sizeof(pDecodedFrame[0]));

    // memcpy from device to host async
    nWidth  = ctx->video_decode_create_info.ulWidth;
    nHeight = ctx->video_decode_create_info.ulHeight;
    nPicSize = nDecodedPitch * nHeight * 3 / 2; 

    //ret = dl_fn->cu_memcpy_dtoh_async(ctx->pFrameYUV[0], pDecodedFrame[0], nPicSize, ctx->h_stream);
    ret = dl_fn->cu_memcpy_dtoh(ctx->pFrameYUV[0], pDecodedFrame[0], nPicSize);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cu_memcpy_dtoh_async ret=%d, nDecodedPitch=%d, nHeight=%d ,stream=%lld, dest=%lld\n", 
            ret, nDecodedPitch, nHeight, ctx->h_stream, ctx->pFrameYUV[0]);

    if(ret != 0){
        av_log(NULL, AV_LOG_ERROR, "[HandlePictureDisplay] cu_memcpy_dtoh failed ret=%d\n", ret);
    }
    // copy the data to frame
    /*
    print_hex(ctx->pFrameYUV[0], 30);
    static write_cnt = 0;
    if(write_cnt > 250 && write_cnt < 500)
        fwrite(ctx->pFrameYUV[0], 1, nPicSize, ctx->fpWriteYUV);
    write_cnt ++;
    */

    DecodeFrame decodedFrame;

    decodedFrame.decode_width = nWidth;
    decodedFrame.decode_height = nHeight;
    decodedFrame.decode_linesize = nWidth;
    decodedFrame.pts = pPicParams->timestamp;
    decodedFrame.data_size = nWidth*nHeight*3/2;
    decodedFrame.data = av_malloc(decodedFrame.data_size);
    if(decodedFrame.data){
        //memcpy(decodedFrame.data, ctx->pFrameYUV[0], nPicSize);

        int i;
        unsigned char *buffer_dst = decodedFrame.data;
        unsigned char *buffer_src = ctx->pFrameYUV[0];
        for(i = 0; i < nHeight; i++){
            memcpy(buffer_dst, buffer_src, nWidth);
            buffer_dst += nWidth;
            buffer_src += nDecodedPitch;
        }

        for(i = 0; i < nHeight/2; i++){
            memcpy(buffer_dst, buffer_src, nWidth);
            buffer_dst += nWidth;
            buffer_src += nDecodedPitch;
        }

        frame_enqueue(ctx, &decodedFrame);
    }

    /*
    AVFrame *decoded_frame = av_frame_alloc();
    if(decoded_frame){
        decoded_frame->width    = nWidth;
        decoded_frame->Height   = nHeight;


    }
    */


    // unmap
    //ret = dl_fn->cuvid_unmap_video_frame(ctx->h_decoder, ctx->pDecodedFrame[0]);
    ret = dl_fn->cuvid_unmap_video_frame(ctx->h_decoder, pDecodedFrame[0]);
    av_log(NULL, AV_LOG_VERBOSE, "[HandlePictureDisplay] cuvid_unmap_video_frame ret=%d\n", ret);

    // release the picture
    pic_release(ctx, &oDisplayInfo);

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

#define CHECK_LOAD_NVML_FUNC(t, f, s) \
do { \
    (f) = (t)LOAD_FUNC(nvml_lib, s); \
    if (!(f)) { \
        av_log(NULL, AV_LOG_FATAL, "Failed loading %s from NVML library\n", s); \
        goto get_suitable_gpu_fail; \
    } \
} while (0)

static av_cold int check_nvml_error(AVCodecContext *avctx, CUresult err, const char *func)
{
    if (err != CUDA_SUCCESS) {
        av_log(avctx, AV_LOG_FATAL, ">> %s - failed with error code:%d\n", func, err);
        return 0;
    }
    return 1;
}
#define check_nvml_errors(f) if (!check_nvml_error(NULL, f, #f)) goto get_suitable_gpu_fail

static int get_suitable_gpu(void)
{
    void* nvml_lib;
    NVMLINIT                    nvml_init;
    NVMLSHUTDOWN                nvml_shutdown;
    NVMLDEVICEGETCOUNT          nvml_device_get_count;
    NVMLDEVICEGETHANDLEBYINDEX  nvml_device_get_handle_by_index;
    NVMLDEVICEGETDECODERUTILIZATION     nvml_device_get_decoder_utilization;
    NVMLDEVICEGETENCODERUTILIZATION     nvml_device_get_encoder_utilization;
    NVMLDEVICEGETMEMORYINFO     nvml_device_get_memory_info;
    NVMLDEVICEGETRUNNINGPROCESSES       nvml_device_get_running_processes;
    NVMLDEVICEGETPPROCESSNAME   nvml_device_get_process_name;
    NVMLDEVICEGETUTILIZATIONRATES       nvml_device_get_utilization_rates;
    NVMLDEVICEGETTEMPERATURE    nvml_device_get_temperature;

    nvmlDevice_t device_handel;
    unsigned int utilization_value = 0;
    unsigned int utilization_sample = 0;
    int best_gpu = 0;
    unsigned int decoder_used = 100;

    // open the libnvidia-ml.so
    #if defined(_WIN32)
        if (sizeof(void*) == 8) {
            nvml_lib = LoadLibrary(TEXT("nvidia-ml.dll"));
        } else {
            nvml_lib = LoadLibrary(TEXT("nvidia-ml.dll"));
        }
    #else
        nvml_lib = dlopen("libnvidia-ml.so", RTLD_LAZY);
    #endif

    av_log(NULL, AV_LOG_VERBOSE, "[get_suitable_gpu] after load libnvidia-ml.\n");

    CHECK_LOAD_NVML_FUNC(NVMLINIT, nvml_init, "nvmlInit");
    CHECK_LOAD_NVML_FUNC(NVMLSHUTDOWN, nvml_shutdown, "nvmlShutdown");

    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETCOUNT, nvml_device_get_count, "nvmlDeviceGetCount");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETHANDLEBYINDEX, nvml_device_get_handle_by_index, "nvmlDeviceGetHandleByIndex");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETDECODERUTILIZATION, nvml_device_get_decoder_utilization, "nvmlDeviceGetDecoderUtilization");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETENCODERUTILIZATION, nvml_device_get_encoder_utilization, "nvmlDeviceGetEncoderUtilization");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETMEMORYINFO, nvml_device_get_memory_info, "nvmlDeviceGetMemoryInfo");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETRUNNINGPROCESSES, nvml_device_get_running_processes, "nvmlDeviceGetComputeRunningProcesses");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETPPROCESSNAME, nvml_device_get_process_name, "nvmlSystemGetProcessName");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETUTILIZATIONRATES, nvml_device_get_utilization_rates, "nvmlDeviceGetUtilizationRates");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETTEMPERATURE, nvml_device_get_temperature, "nvmlDeviceGetTemperature");


    av_log(NULL, AV_LOG_VERBOSE, "[get_suitable_gpu] after load libnvidia-ml functions.\n");

    // get gpu info
    check_nvml_errors(nvml_init());
    unsigned int device_count = 0;
    check_nvml_errors(nvml_device_get_count(&device_count));
    av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] device_count:%u.\n", device_count);



    nvmlMemory_t memory_info;
    unsigned int process_buf_size = 100;
    nvmlProcessInfo_t process_buf[100];
    char process_name[256];
    int  min_processes = 1000;
    nvmlUtilization_t gpu_utilization;
    unsigned int temperature;

    unsigned int local_pid = getpid();
    srand(local_pid);
    //unsigned int sleep_time = rand()%200000 + rand()%300000 + rand()%500000; // random in 1 s
    unsigned int sleep_time = rand()%1000000; // random in 1 s

    av_log(NULL, AV_LOG_ERROR, "[get_suitable_gpu] sleep_time:%u.\n", sleep_time);
    av_usleep(sleep_time); // sleep 

    for(int i = 0; i < device_count; i++){
        check_nvml_errors(nvml_device_get_handle_by_index(i, &device_handel));
        //av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] sleep_time=%u, device:%d, device_handel:%lld.\n", sleep_time, i, device_handel);
        check_nvml_errors(nvml_device_get_decoder_utilization(device_handel, &utilization_value, &utilization_sample));
        av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, decoder_utilization:%u, utilization_sample:%u.\n", local_pid, i, utilization_value, utilization_sample);



        check_nvml_errors(nvml_device_get_encoder_utilization(device_handel, &utilization_value, &utilization_sample));
        av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, encoder_utilization:%u, utilization_sample:%u.\n", local_pid, i, utilization_value, utilization_sample);
        //if(decoder_used >  utilization_value){
        //    decoder_used = utilization_value;
        //    best_gpu = i;
        //}
        check_nvml_errors(nvml_device_get_memory_info(device_handel, &memory_info));
        av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, memory_info: total=%llu, free=%llu, used=%llu.\n", local_pid, i, memory_info.total, memory_info.free, memory_info.used);

        check_nvml_errors(nvml_device_get_utilization_rates(device_handel, &gpu_utilization));
        av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, gpu_utilization: gpu=%u, memory=%u.\n", local_pid, i, gpu_utilization.gpu, gpu_utilization.memory);

        check_nvml_errors(nvml_device_get_temperature(device_handel, 0, &temperature));
        av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, temperature: %u.\n", local_pid, i, temperature);


        // get processes info
        process_buf_size = 100;
        memset(process_buf, 0, sizeof(nvmlProcessInfo_t)*100);
        memset(process_name, 0, sizeof(process_name));
        check_nvml_errors(nvml_device_get_running_processes(device_handel, &process_buf_size, process_buf));
        av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, process number:%u.\n", local_pid, i, process_buf_size);
        if(process_buf_size < min_processes){
            min_processes = process_buf_size;
            best_gpu = i;
        }

        while(process_buf_size > 0){
            process_buf_size--;

            //check_nvml_errors(nvml_device_get_process_name(process_buf[process_buf_size].pid, process_name, sizeof(process_name)));
            nvml_device_get_process_name(process_buf[process_buf_size].pid, process_name, sizeof(process_name));

            av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, device:%d, process id:%u, name:%s memory:%llu.\n", 
                local_pid, i, process_buf[process_buf_size].pid, process_name, process_buf[process_buf_size].usedGpuMemory);
        }




    }
    av_log(NULL, AV_LOG_INFO, "[get_suitable_gpu] local_pid=%u, get the best_gpu:%d.\n", local_pid, best_gpu);


get_suitable_gpu_fail:

    nvml_shutdown();

    return best_gpu;
}

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
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init: after nvenc_dyload_cuda()\n");

    check_cuda_errors(dl_fn->cu_init(0));
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init: after cu_init(0);\n");

    check_cuda_errors(dl_fn->cu_device_get_count(&ctx->device_count));
    if (!ctx->device_count) {
        av_log(avctx, AV_LOG_FATAL, "No CUDA capable devices found\n");
        goto nvenc_decode_init_fail;
    }
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init: after cu_device_get_count, device_count=%d, select gpu:%d.\n", ctx->device_count, ctx->gpu);


    av_log(avctx, AV_LOG_VERBOSE, "video width=%d, height=%d, pix_fmt=%d, extradata_size=%d\n", 
        avctx->width, avctx->height, avctx->pix_fmt, avctx->extradata_size);

    /*
    if(avctx->width == 0 && avctx->height == 0){  // fixme:
        avctx->width = 1920;
        avctx->height = 1080;
    }
    */

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

        av_log(avctx, AV_LOG_INFO, "[ GPU #%d - < %s > has Compute SM %d.%d, NVENC %s ]\n", i, gpu_name, smmajor, smminor, (smver >= target_smver) ? "Available" : "Not Available");

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
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init after load libnvcuvid.\n");

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

    /*
    // open the libnvidia-ml.so
    #if defined(_WIN32)
        if (sizeof(void*) == 8) {
            dl_fn->nvml_lib = LoadLibrary(TEXT("nvidia-ml.dll"));
        } else {
            dl_fn->nvml_lib = LoadLibrary(TEXT("nvidia-ml.dll"));
        }
    #else
        dl_fn->nvml_lib = dlopen("libnvidia-ml.so", RTLD_LAZY);
    #endif

    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init after load libnvidia-ml.\n");

    CHECK_LOAD_NVML_FUNC(NVMLINIT, dl_fn->nvml_init, "nvmlInit");
    CHECK_LOAD_NVML_FUNC(NVMLSHUTDOWN, dl_fn->nvml_shutdown, "nvmlShutdown");

    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETCOUNT, dl_fn->nvml_device_get_count, "nvmlDeviceGetCount");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETHANDLEBYINDEX, dl_fn->nvml_device_get_handle_by_index, "nvmlDeviceGetHandleByIndex");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETDECODERUTILIZATION, dl_fn->nvml_device_get_decoder_utilization, "nvmlDeviceGetDecoderUtilization");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETENCODERUTILIZATION, dl_fn->nvml_device_get_encoder_utilization, "nvmlDeviceGetEncoderUtilization");
    CHECK_LOAD_NVML_FUNC(NVMLDEVICEGETMEMORYINFO, dl_fn->nvml_device_get_memory_info, "nvmlDeviceGetMemoryInfo");

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after load libnvidia-ml functions.\n");

    // get gpu info
    check_cuda_errors(dl_fn->nvml_init());
    unsigned int device_count = 0;
    check_cuda_errors(dl_fn->nvml_device_get_count(&device_count));
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init device_count:%u.\n", device_count);

    nvmlDevice_t device_handel;
    unsigned int utilization_value = 0;
    unsigned int utilization_sample = 0;
    int best_gpu = 0;
    unsigned int decoder_used = 100;

    nvmlMemory_t memory_info;
    for(i = 0; i < device_count; i++){
        check_cuda_errors(dl_fn->nvml_device_get_handle_by_index(i, &device_handel));
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init device:%d, device_handel:%lld.\n", i, device_handel);
        check_cuda_errors(dl_fn->nvml_device_get_decoder_utilization(device_handel, &utilization_value, &utilization_sample));
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init device:%d, decoder_utilization:%u, utilization_sample:%u.\n", i, utilization_value, utilization_sample);



        check_cuda_errors(dl_fn->nvml_device_get_encoder_utilization(device_handel, &utilization_value, &utilization_sample));
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init device:%d, encoder_utilization:%u, utilization_sample:%u.\n", i, utilization_value, utilization_sample);
        if(decoder_used >  utilization_value){
            decoder_used = utilization_value;
            best_gpu = i;
        }
        check_cuda_errors(dl_fn->nvml_device_get_memory_info(device_handel, &memory_info));
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init device:%d, memory_info: total=%llu, free=%llu, used=%llu.\n", i, memory_info.total, memory_info.free, memory_info.used);


    }
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init get the best_gpu:%d.\n", best_gpu);
    ctx->gpu = best_gpu;


    check_cuda_errors(dl_fn->nvml_shutdown());
    */

    //ctx->gpu = get_suitable_gpu(avctx);

    // create cuda context
    ctx->cu_context = NULL;
    //check_cuda_errors(dl_fn->cu_ctx_create(&ctx->cu_context, 4, dl_fn->nvenc_devices[ctx->gpu]));  //CU_CTX_SCHED_AUTO=0, CU_CTX_SCHED_BLOCKING_SYNC=4, avoid CPU spins

    
    /* the handle of decoder */
    ctx->h_decoder = NULL;


    /* create parser */
    memset(&ctx->video_parser_params, 0, sizeof(ctx->video_parser_params));
    ctx->video_parser_params.CodecType              = cudaVideoCodec_H264; // fixme: should support other codec in feature
    //ctx->video_parser_params.ulMaxNumDecodeSurfaces = ctx->video_decode_create_info.ulNumDecodeSurfaces; // same as decoder 
    ctx->video_parser_params.ulMaxNumDecodeSurfaces = 6; // same as decoder 
    ctx->video_parser_params.ulMaxDisplayDelay      = 1;  // this flag is needed so the parser will push frames out to the decoder as quickly as it can
    ctx->video_parser_params.pUserData              = ctx; // user data we just send the ctx
    ctx->video_parser_params.pfnSequenceCallback    = HandleVideoSequence;    // Called before decoding frames and/or whenever there is a format change
    ctx->video_parser_params.pfnDecodePicture       = HandlePictureDecode;    // Called when a picture is ready to be decoded (decode order)
    ctx->video_parser_params.pfnDisplayPicture      = HandlePictureDisplay;   // Called whenever a picture is ready to be displayed (display order)

    check_cuda_errors(dl_fn->cuvid_create_video_parser(&ctx->h_parser, &ctx->video_parser_params)); 
    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init after create video parser.\n");
    ctx->parser_width  = 0;
    ctx->parser_height = 0;

    
    // alloc the memory host
    ctx->pFrameYUV[0] = NULL;
    ctx->pFrameYUV[1] = NULL;
    ctx->pFrameYUV[2] = NULL;
    ctx->pFrameYUV[3] = NULL;
    /*
    check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[0], 2048*1088*3/2)); 
    check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[1], 2048*1088*3/2));
    //check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[2], 1920*1080*3/2));
    //check_cuda_errors(dl_fn->cu_mem_alloc_host((void **)&ctx->pFrameYUV[3], 1920*1080*3/2));

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after alloc the memory host.\n");
    */

    // Create a Stream ID for handling Readback
    ctx->h_stream = NULL;
    //check_cuda_errors(dl_fn->cu_stream_create(&ctx->h_stream, 0));
    //av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after cu_stream_create, stream=%ld.\n", ctx->h_stream);




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
    //ctx->fpWriteYUV = fopen("/home/write.yuv","w+");
    //ctx->fpWriteES  = fopen("/home/write.es", "w+");

    av_log(avctx, AV_LOG_INFO, "nvenc_decode_init over\n");
    return 0;

nvenc_decode_init_fail:

    nvenc_decode_close(avctx);
    return -1;
}

static void buffer_default_free(void *opaque, uint8_t *data)
{
    
    static cnt = 0;
    av_log(NULL, AV_LOG_VERBOSE, "buffer_default_free cnt=%d, data=%lld\n", cnt++, data);
    av_free(data);

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

    //av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame frame, data[0]:%lld, linesize[0]=%d, buf[0]=%lld\n", frame->data[0], frame->linesize[0], frame->buf[0]);

    // get the picture that has been decoded
    if(ctx->have_decoded_frames){
        av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame get frame\n");

        DecodeFrame decodedFrame;

        if(frame_dequeue(ctx, &decodedFrame)){
            frame->linesize[0] = decodedFrame.decode_linesize;
            frame->linesize[1] = decodedFrame.decode_linesize;
            //frame->linesize[2] = decodedFrame.decode_linesize/2;

            frame->data[0] = decodedFrame.data;
            frame->data[1] = frame->data[0] + decodedFrame.decode_linesize*decodedFrame.decode_height;
            //frame->data[2] = frame->data[1] + decodedFrame.decode_linesize*decodedFrame.decode_height/4;

            frame->extended_data = frame->data;
            frame->width = decodedFrame.decode_width;
            frame->height = decodedFrame.decode_height;
            frame->format = AV_PIX_FMT_NV12;


            frame->pts = decodedFrame.pts;

            frame->buf[0] = av_buffer_create(decodedFrame.data, decodedFrame.decode_width*decodedFrame.decode_linesize*3/2, buffer_default_free, NULL, 0);

            av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame frame, data[0]:%lld, linesize[0]=%d, buf[0]=%lld\n", frame->data[0], frame->linesize[0], frame->buf[0]);
            av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame decodedFrame, width=%d, height=%d\n", frame->width, frame->height);

            /*
            static write_cnt = 0;
            if(write_cnt > 250 && write_cnt < 500){
                fwrite(frame->data[0], 1, decodedFrame.data_size, ctx->fpWriteYUV);
            }
            write_cnt ++;
            */

            

            *got_frame = 1;
        }
    }

    // fill the data to decode
    CUVIDSOURCEDATAPACKET cu_pkt;
    if(avpkt && avpkt->size > 0){
        if (avpkt->size > 3 && !avpkt->data[0] &&
            !avpkt->data[1] && !avpkt->data[2] && 1==avpkt->data[3]) {
            /* we already have annex-b prefix */
            av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame we already have annex-b prefix\n");

            ret = avpkt->size;

            if (ret >= 0) {

                // fill the packet to video parser
                cu_pkt.flags = CUVID_PKT_TIMESTAMP;
                cu_pkt.payload_size = avpkt->size;
                cu_pkt.payload = avpkt->data;
                cu_pkt.timestamp = avpkt->pts;

                //fwrite(pkt_filtered.data, 1, pkt_filtered.size, ctx->fpWriteES);

                cu_ret = dl_fn->cuvid_parse_video_data(ctx->h_parser, &cu_pkt);
                av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame cuvid_parse_video_data cu_ret=%d\n", cu_ret);

                if (p_filtered != avpkt->data)
                    av_free(p_filtered);
                return ret > 0 ? avpkt->size : ret;
            }
        } else {
            /* no annex-b prefix. try to restore: */
            av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame no annex-b prefix. try to restore\n");

            ret = av_bitstream_filter_filter(ctx->bsf, avctx, "private_spspps_buf",
                                         &p_filtered, &n_filtered,
                                         avpkt->data, avpkt->size, 0);
            if (ret >= 0) {
                pkt_filtered.pts  = avpkt->pts;
                pkt_filtered.dts  = avpkt->dts;
                pkt_filtered.data = p_filtered;
                pkt_filtered.size = n_filtered;

                // fill the packet to video parser
                cu_pkt.flags = CUVID_PKT_TIMESTAMP;
                cu_pkt.payload_size = pkt_filtered.size;
                cu_pkt.payload = pkt_filtered.data;
                cu_pkt.timestamp = pkt_filtered.pts;

                //fwrite(pkt_filtered.data, 1, pkt_filtered.size, ctx->fpWriteES);

                cu_ret = dl_fn->cuvid_parse_video_data(ctx->h_parser, &cu_pkt);
                av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame cuvid_parse_video_data cu_ret=%d\n", cu_ret);

                if (p_filtered != avpkt->data)
                    av_free(p_filtered);
                return ret > 0 ? avpkt->size : ret;
            }
        }
    }


    if(avctx->width <= 0 || avctx->height <= 0){
        avctx->width = ctx->parser_width;
        avctx->height = ctx->parser_height;
    }


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
    { "gpu", "Selects which NVENC capable GPU to use (decode). First GPU is 0, second is 1, and so on.", OFFSET(gpu), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, INT_MAX, VD },
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

