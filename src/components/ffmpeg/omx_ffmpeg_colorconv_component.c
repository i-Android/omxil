/**
  @file src/components/ffmpeg/omx_ffmpeg_colorconv_component.c

  This component implements a color converter using the ffmpeg
  software library.

  Originally developed by Peter Littlefield
	Copyright (C) 2007  STMicroelectronics and Agere Systems

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

  $Date: 2007-05-22 14:25:04 +0200 (Tue, 22 May 2007) $
  Revision $Rev: 872 $
  Author $Author: giulio_urlini $
*/

#include <omxcore.h>
#include <omx_ffmpeg_colorconv_component.h>

/** Maximum Number of Video Color Converter Component Instance*/
#define MAX_COMPONENT_VIDEOCOLORCONV 2

/** Counter of Video Component Instance*/
OMX_U32 noVideoColorConvInstance = 0;

/** define the max input buffer size */
#define MAX_VIDEO_INPUT_BUF_SIZE 460800


/**	Figure out equivalent ffmpeg PixelFormat based on OMX_COLOR_FORMATTYPE 
  * @param omx_pxlfmt is the input openmax standard pixel format
  * output is the ffmpeg library supported pixel format corresponding to this input pixel format
  * this output ffmpeg pixel format will be needed in port parameter settings
  */
enum PixelFormat find_ffmpeg_pxlfmt(OMX_COLOR_FORMATTYPE omx_pxlfmt) {
  enum PixelFormat ffmpeg_pxlfmt;

  switch (omx_pxlfmt) {
    case OMX_COLOR_FormatL8:
      ffmpeg_pxlfmt = PIX_FMT_GRAY8;
      break;
    case OMX_COLOR_Format16bitARGB1555:
      ffmpeg_pxlfmt = PIX_FMT_RGB555;
      break;
    case OMX_COLOR_Format16bitRGB565:
    case OMX_COLOR_Format16bitBGR565:
      ffmpeg_pxlfmt = PIX_FMT_RGB565;
      break;
    case OMX_COLOR_Format24bitRGB888:
      ffmpeg_pxlfmt = PIX_FMT_RGB24;
      break;
    case OMX_COLOR_Format24bitBGR888:
      ffmpeg_pxlfmt = PIX_FMT_BGR24;
      break;
    case OMX_COLOR_Format32bitBGRA8888:
    case OMX_COLOR_Format32bitARGB8888:
      ffmpeg_pxlfmt = PIX_FMT_RGBA32;
      break;
    case OMX_COLOR_FormatYUV411Planar:
    case OMX_COLOR_FormatYUV411PackedPlanar:
      ffmpeg_pxlfmt = PIX_FMT_YUV411P;
      break;
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420PackedPlanar:
      ffmpeg_pxlfmt = PIX_FMT_YUV420P;
      break;
    case OMX_COLOR_FormatYUV422Planar:
    case OMX_COLOR_FormatYUV422PackedPlanar:
      ffmpeg_pxlfmt = PIX_FMT_YUV422P;
      break;
    case OMX_COLOR_FormatCbYCrY:
      ffmpeg_pxlfmt = PIX_FMT_UYVY422;
      break;
    case OMX_COLOR_FormatMonochrome:	//	Better hope resolutions are multiples of 8
      ffmpeg_pxlfmt = PIX_FMT_MONOBLACK;
      break;
    case OMX_COLOR_FormatL2:
    case OMX_COLOR_FormatL4:
    case OMX_COLOR_FormatL16:
    case OMX_COLOR_FormatL24:
    case OMX_COLOR_FormatL32:
    case OMX_COLOR_Format8bitRGB332:
    case OMX_COLOR_Format12bitRGB444:
    case OMX_COLOR_Format16bitARGB4444:
    case OMX_COLOR_Format18bitRGB666:
    case OMX_COLOR_Format18bitARGB1665:
    case OMX_COLOR_Format19bitARGB1666:
    case OMX_COLOR_Format24bitARGB1887:
    case OMX_COLOR_Format25bitARGB1888:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV422SemiPlanar:
    case OMX_COLOR_FormatYCbYCr:
    case OMX_COLOR_FormatYCrYCb:
    case OMX_COLOR_FormatCrYCbY:
    case OMX_COLOR_FormatYUV444Interleaved:
    case OMX_COLOR_FormatRawBayer8bit:
    case OMX_COLOR_FormatRawBayer10bit:
    case OMX_COLOR_FormatRawBayer8bitcompressed:		
    case OMX_COLOR_FormatUnused:
    default:
      ffmpeg_pxlfmt = PIX_FMT_NONE;
      break;
  }
  return ffmpeg_pxlfmt;
}

/**	This function takes two inputs - 
  * @param width is the input picture width
  * @param omx_pxlfmt is the input openmax standard pixel format
  * It calculates the byte per pixel needed to display the picture with the input omx_pxlfmt
  * The output stride for display is thus product of input width and byte per pixel
  */
OMX_S32 calcStride(OMX_U32 width, OMX_COLOR_FORMATTYPE omx_pxlfmt) {
  OMX_U32 stride;
  OMX_U32 bpp; // bit per pixel

  switch(omx_pxlfmt) {
    case OMX_COLOR_FormatMonochrome:
      bpp = 1;
      break;
    case OMX_COLOR_FormatL2:
      bpp = 2;
    case OMX_COLOR_FormatL4:
      bpp = 4;
      break;
    case OMX_COLOR_FormatL8:
    case OMX_COLOR_Format8bitRGB332:
    case OMX_COLOR_FormatRawBayer8bit:
    case OMX_COLOR_FormatRawBayer8bitcompressed:
      bpp = 8;	
      break;
    case OMX_COLOR_FormatRawBayer10bit:
      bpp = 10;
      break;
    case OMX_COLOR_FormatYUV411Planar:
    case OMX_COLOR_FormatYUV411PackedPlanar:
    case OMX_COLOR_Format12bitRGB444:
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420PackedPlanar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV444Interleaved:
      bpp = 12;
      break;
    case OMX_COLOR_FormatL16:
    case OMX_COLOR_Format16bitARGB4444:
    case OMX_COLOR_Format16bitARGB1555:
    case OMX_COLOR_Format16bitRGB565:
    case OMX_COLOR_Format16bitBGR565:
    case OMX_COLOR_FormatYUV422Planar:
    case OMX_COLOR_FormatYUV422PackedPlanar:
    case OMX_COLOR_FormatYUV422SemiPlanar:
    case OMX_COLOR_FormatYCbYCr:
    case OMX_COLOR_FormatYCrYCb:
    case OMX_COLOR_FormatCbYCrY:
    case OMX_COLOR_FormatCrYCbY:
      bpp = 16;
      break;
    case OMX_COLOR_Format18bitRGB666:
    case OMX_COLOR_Format18bitARGB1665:
      bpp = 18;
      break;
    case OMX_COLOR_Format19bitARGB1666:
      bpp = 19;
      break;
    case OMX_COLOR_FormatL24:
    case OMX_COLOR_Format24bitRGB888:
    case OMX_COLOR_Format24bitBGR888:
    case OMX_COLOR_Format24bitARGB1887:
      bpp = 24;
      break;
    case OMX_COLOR_Format25bitARGB1888:
      bpp = 25;
      break;
    case OMX_COLOR_FormatL32:
    case OMX_COLOR_Format32bitBGRA8888:
    case OMX_COLOR_Format32bitARGB8888:
      bpp = 32;
      break;
    default:
      bpp = 0;
      break;
  }
  stride = (width * bpp) >> 3;
  return (OMX_S32) stride;
}

/** The Constructor 
  * @param cComponentName is the name of the constructed component
  */
OMX_ERRORTYPE omx_ffmpeg_colorconv_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {
	OMX_ERRORTYPE err = OMX_ErrorNone;	
	omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private;
	omx_ffmpeg_colorconv_component_PortType *inPort,*outPort;
	OMX_S32 i;

  if (!openmaxStandComp->pComponentPrivate) {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, allocating component\n", __func__);
    openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_ffmpeg_colorconv_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate == NULL) {
      return OMX_ErrorInsufficientResources;
    }
  } else {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, Error Component %x Already Allocated\n", __func__, (int)openmaxStandComp->pComponentPrivate);
  }

  omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;

  /** we could create our own port structures here
    * fixme maybe the base class could use a "port factory" function pointer?	
    */
  err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

  /** here we can override whatever defaults the base_component constructor set
    * e.g. we can override the function pointers in the private struct  
    */
  omx_ffmpeg_colorconv_component_Private = (omx_ffmpeg_colorconv_component_PrivateType *)openmaxStandComp->pComponentPrivate;

  /** Allocate Ports and Call base port constructor. */	
  if (omx_ffmpeg_colorconv_component_Private->sPortTypesParam.nPorts && !omx_ffmpeg_colorconv_component_Private->ports) {
    omx_ffmpeg_colorconv_component_Private->ports = calloc(omx_ffmpeg_colorconv_component_Private->sPortTypesParam.nPorts, sizeof (omx_base_PortType *));
    if (!omx_ffmpeg_colorconv_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }
    for (i=0; i < omx_ffmpeg_colorconv_component_Private->sPortTypesParam.nPorts; i++) {
      /** this is the important thing separating this from the base class; size of the struct is for derived class port type
        * this could be refactored as a smarter factory function instead?
        */
      omx_ffmpeg_colorconv_component_Private->ports[i] = calloc(1, sizeof(omx_ffmpeg_colorconv_component_PortType));
      if (!omx_ffmpeg_colorconv_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
    }
  }

  base_port_Constructor(openmaxStandComp, &omx_ffmpeg_colorconv_component_Private->ports[0], 0, OMX_TRUE);
  base_port_Constructor(openmaxStandComp, &omx_ffmpeg_colorconv_component_Private->ports[1], 1, OMX_FALSE);
	
  inPort = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  outPort = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

  /** Domain specific section for the ports. */	
  //input port parameter settings
  inPort->sPortParam.eDomain = OMX_PortDomainVideo;
  inPort->sPortParam.format.video.cMIMEType = (OMX_STRING)malloc(sizeof(char)*128);
  strcpy(inPort->sPortParam.format.video.cMIMEType, "raw");
  inPort->sPortParam.format.video.pNativeRender = NULL;
  inPort->sPortParam.format.video.nFrameWidth = 640;
  inPort->sPortParam.format.video.nFrameHeight = 480;
  inPort->sPortParam.format.video.nStride = 0;
  inPort->sPortParam.format.video.nSliceHeight = 0;
  inPort->sPortParam.format.video.nBitrate = 0;
  inPort->sPortParam.format.video.xFramerate = 25; 
  inPort->sPortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
  inPort->ffmpeg_pxlfmt = PIX_FMT_YUV420P;
  inPort->sPortParam.nBufferSize = MAX_VIDEO_INPUT_BUF_SIZE;
  inPort->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
  inPort->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  inPort->sPortParam.format.video.pNativeWindow = NULL;

  //output port parameter settings
  outPort->sPortParam.eDomain = OMX_PortDomainVideo;
  outPort->sPortParam.format.video.cMIMEType = (OMX_STRING)malloc(sizeof(char)*128);
  strcpy(outPort->sPortParam.format.video.cMIMEType, "raw");
  outPort->sPortParam.format.video.pNativeRender = NULL;
  outPort->sPortParam.format.video.nFrameWidth = 640;
  outPort->sPortParam.format.video.nFrameHeight = 480;
  outPort->sPortParam.format.video.nStride = 0;
  outPort->sPortParam.format.video.nSliceHeight = 0;
  outPort->sPortParam.format.video.nBitrate = 0;
  outPort->sPortParam.format.video.xFramerate = 25; 
  outPort->sPortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
  outPort->ffmpeg_pxlfmt = PIX_FMT_RGB24;
  outPort->sPortParam.nBufferSize = MAX_VIDEO_INPUT_BUF_SIZE * 3;
  outPort->sPortParam.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;
  outPort->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  outPort->sPortParam.format.video.pNativeWindow = NULL;


  setHeader(&inPort->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
  inPort->sVideoParam.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
  inPort->sVideoParam.nIndex = 0;
  inPort->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
  inPort->sVideoParam.eColorFormat = OMX_COLOR_FormatYUV420Planar;

  setHeader(&outPort->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
  outPort->sVideoParam.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
  outPort->sVideoParam.nIndex = 0;
  outPort->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
  outPort->sVideoParam.eColorFormat = OMX_COLOR_Format24bitRGB888;


  setHeader(&inPort->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));	
  inPort->omxConfigCrop.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
  inPort->omxConfigCrop.nLeft = inPort->omxConfigCrop.nTop = 0;
  inPort->omxConfigCrop.nWidth = 640;
  inPort->omxConfigCrop.nHeight = 480;

  setHeader(&inPort->omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));	
  inPort->omxConfigRotate.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
  inPort->omxConfigRotate.nRotation = 0;

  setHeader(&inPort->omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));	
  inPort->omxConfigMirror.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
  inPort->omxConfigMirror.eMirror = OMX_MirrorNone;

  setHeader(&inPort->omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));	
  inPort->omxConfigScale.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
  inPort->omxConfigScale.xWidth = inPort->omxConfigScale.xHeight = 0x10000;

  setHeader(&inPort->omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));	
  inPort->omxConfigOutputPosition.nPortIndex = OMX_BASE_FILTER_INPUTPORT_INDEX;
  inPort->omxConfigOutputPosition.nX = inPort->omxConfigOutputPosition.nY = 0;

  setHeader(&outPort->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));	
  outPort->omxConfigCrop.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
  outPort->omxConfigCrop.nLeft = outPort->omxConfigCrop.nTop = 0;
  outPort->omxConfigCrop.nWidth = 640;
  outPort->omxConfigCrop.nHeight = 480;

  setHeader(&outPort->omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));	
  outPort->omxConfigRotate.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
  outPort->omxConfigRotate.nRotation = 0;

  setHeader(&outPort->omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));	
  outPort->omxConfigMirror.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
  outPort->omxConfigMirror.eMirror = OMX_MirrorNone;

  setHeader(&outPort->omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));	
  outPort->omxConfigScale.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
  outPort->omxConfigScale.xWidth = outPort->omxConfigScale.xHeight = 0x10000;

  setHeader(&outPort->omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));	
  outPort->omxConfigOutputPosition.nPortIndex = OMX_BASE_FILTER_OUTPUTPORT_INDEX;
  outPort->omxConfigOutputPosition.nX = outPort->omxConfigOutputPosition.nY = 0;

  omx_ffmpeg_colorconv_component_Private->in_alloc_size = 0;
  omx_ffmpeg_colorconv_component_Private->conv_alloc_size = 0;
  omx_ffmpeg_colorconv_component_Private->out_alloc_size = 0;

  omx_ffmpeg_colorconv_component_Private->in_buffer = NULL;
  omx_ffmpeg_colorconv_component_Private->conv_buffer = NULL;
  omx_ffmpeg_colorconv_component_Private->out_buffer = NULL;

  omx_ffmpeg_colorconv_component_Private->messageHandler = omx_video_colorconv_MessageHandler;
  omx_ffmpeg_colorconv_component_Private->destructor = omx_ffmpeg_colorconv_component_Destructor;
  omx_ffmpeg_colorconv_component_Private->BufferMgmtCallback = omx_ffmpeg_colorconv_component_BufferMgmtCallback;
  openmaxStandComp->SetParameter = omx_ffmpeg_colorconv_component_SetParameter;
  openmaxStandComp->GetParameter = omx_ffmpeg_colorconv_component_GetParameter;
  openmaxStandComp->SetConfig = omx_ffmpeg_colorconv_component_SetConfig;
  openmaxStandComp->GetConfig = omx_ffmpeg_colorconv_component_GetConfig;

  noVideoColorConvInstance++;

  if(noVideoColorConvInstance > MAX_COMPONENT_VIDEOCOLORCONV) {
    return OMX_ErrorInsufficientResources;
  }
  return err;
}

/** The destructor
 */
OMX_ERRORTYPE omx_ffmpeg_colorconv_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {
  int i;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;

  /** frees the locally dynamic allocated memory */
  if (omx_ffmpeg_colorconv_component_Private->sPortTypesParam.nPorts && omx_ffmpeg_colorconv_component_Private->ports) {
    for (i = 0; i < omx_ffmpeg_colorconv_component_Private->sPortTypesParam.nPorts; i++) {
      if(omx_ffmpeg_colorconv_component_Private->ports[i]) {
        base_port_Destructor(omx_ffmpeg_colorconv_component_Private->ports[i]);
      }
    }
    free(omx_ffmpeg_colorconv_component_Private->ports);
    omx_ffmpeg_colorconv_component_Private->ports = NULL;
  }

  DEBUG(DEB_LEV_FUNCTION_NAME, "Destructor of video color converter component is called\n");

  omx_base_filter_Destructor(openmaxStandComp);
  noVideoColorConvInstance--;

  return OMX_ErrorNone;
}


/** The Initialization function 
  * This function alloates the frames and buffers to store the color converterted output from input yuv
  */
OMX_ERRORTYPE omx_ffmpeg_colorconv_component_Init(OMX_COMPONENTTYPE *openmaxStandComp) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;
  omx_ffmpeg_colorconv_component_PortType *inPort,*outPort;
  OMX_U32 in_width, in_height, out_width, out_height;

  inPort = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  outPort = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

  in_width = inPort->sPortParam.format.video.nFrameWidth;
  in_height = inPort->sPortParam.format.video.nFrameHeight;
  out_width = outPort->sPortParam.format.video.nFrameWidth;
  out_height = outPort->sPortParam.format.video.nFrameHeight;

  if (omx_ffmpeg_colorconv_component_Private->in_buffer) {
    free(omx_ffmpeg_colorconv_component_Private->in_buffer);
  }
  omx_ffmpeg_colorconv_component_Private->in_alloc_size = avpicture_get_size(inPort->ffmpeg_pxlfmt, 
  inPort->sPortParam.format.video.nFrameWidth, inPort->sPortParam.format.video.nFrameHeight);

  omx_ffmpeg_colorconv_component_Private->in_buffer = malloc(omx_ffmpeg_colorconv_component_Private->in_alloc_size);

  if (omx_ffmpeg_colorconv_component_Private->in_buffer == NULL) {
    DEBUG(DEB_LEV_ERR, "\nError allocating internal input buffer!\n");
    return OMX_ErrorInsufficientResources;
  }

  if (omx_ffmpeg_colorconv_component_Private->conv_buffer) {
    free(omx_ffmpeg_colorconv_component_Private->conv_buffer);
  }
  omx_ffmpeg_colorconv_component_Private->conv_alloc_size = avpicture_get_size(outPort->ffmpeg_pxlfmt,
              inPort->sPortParam.format.video.nFrameWidth, inPort->sPortParam.format.video.nFrameHeight);

  omx_ffmpeg_colorconv_component_Private->conv_buffer = malloc(omx_ffmpeg_colorconv_component_Private->conv_alloc_size);

  if (omx_ffmpeg_colorconv_component_Private->conv_buffer == NULL) {
    DEBUG(DEB_LEV_ERR, "\nError allocating internal conversion buffer! size : %d \n", 
    omx_ffmpeg_colorconv_component_Private->conv_alloc_size);
    return OMX_ErrorInsufficientResources;
  } 
  av_register_all();
  omx_ffmpeg_colorconv_component_Private->in_frame = avcodec_alloc_frame();
  omx_ffmpeg_colorconv_component_Private->conv_frame = avcodec_alloc_frame();

  return err;
};

/** The Deinitialization function 
  * This function dealloates the frames and buffers to store the color converterted output from input yuv
  */
OMX_ERRORTYPE omx_ffmpeg_colorconv_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;

  if (omx_ffmpeg_colorconv_component_Private->in_buffer) {
    free(omx_ffmpeg_colorconv_component_Private->in_buffer);
    omx_ffmpeg_colorconv_component_Private->in_buffer = NULL;
  }		
  if (omx_ffmpeg_colorconv_component_Private->conv_buffer) {
    free(omx_ffmpeg_colorconv_component_Private->conv_buffer);	
    omx_ffmpeg_colorconv_component_Private->conv_buffer = NULL;
  }
  if (omx_ffmpeg_colorconv_component_Private->out_buffer) {
    free(omx_ffmpeg_colorconv_component_Private->out_buffer);	
    omx_ffmpeg_colorconv_component_Private->out_buffer = NULL;
  }
  omx_ffmpeg_colorconv_component_Private->in_alloc_size = 0;
  omx_ffmpeg_colorconv_component_Private->conv_alloc_size = 0;
  omx_ffmpeg_colorconv_component_Private->out_alloc_size = 0;
  if (omx_ffmpeg_colorconv_component_Private->in_frame) {
    av_free(omx_ffmpeg_colorconv_component_Private->in_frame);
    omx_ffmpeg_colorconv_component_Private->in_frame = NULL;
  }
  if (omx_ffmpeg_colorconv_component_Private->conv_frame) {
    av_free(omx_ffmpeg_colorconv_component_Private->conv_frame);
    omx_ffmpeg_colorconv_component_Private->conv_frame = NULL;
  }

  return err;
}


/** Check Domain of the Tunneled Component */
OMX_ERRORTYPE omx_ffmpeg_colorconv_component_DomainCheck(OMX_PARAM_PORTDEFINITIONTYPE pDef){
  if(pDef.eDomain != OMX_PortDomainVideo) {
    return OMX_ErrorPortsNotCompatible;
  } else if(pDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMax) {
    return OMX_ErrorPortsNotCompatible;
  }
  return OMX_ErrorNone;
}


/**	This function copies source inmage to destination image of required dimension and color formats 
  * @param src_ptr is the source image strting pointer
  * @param src_stride is the source image stride (src_width * byte_per_pixel)
  * @param src_width is source image width & src_height is source image height
  * @param src_offset_x & src_offset_y are x,y offset values (if any) from starting pointer
  * @param dest_ptr is the destination image strting pointer
  * @param dest_stride is the destination image stride (dest_width * byte_per_pixel)
  * @param dest_width is destination image width & dest_height is destination image height
  * @param dest_offset_x dest_offset_y are x,y offset values (if any) from starting pointer
  * @param cpy_width cpy_height is the source image copy width and height - it determines the portion of 
    source image to be copied from source to destination image
  * @param colorformat is the source image color format
  */
void omx_img_copy(OMX_U8* src_ptr, OMX_S32 src_stride, OMX_U32 src_width, OMX_U32 src_height, 
                  OMX_S32 src_offset_x, OMX_S32 src_offset_y,
                  OMX_U8* dest_ptr, OMX_S32 dest_stride, OMX_U32 dest_width,  OMX_U32 dest_height, 
                  OMX_S32 dest_offset_x, OMX_S32 dest_offset_y, 
                  OMX_S32 cpy_width, OMX_U32 cpy_height, OMX_COLOR_FORMATTYPE colorformat ) {	

  OMX_U32 i;
  //	CAUTION: We don't do any checking of boundaries! (FIXME - see omx_ffmpeg_colorconv_component_BufferMgmtCallback)
  if (colorformat == OMX_COLOR_FormatYUV411Planar ||				//	Input frame is planar, not interleaved
      colorformat == OMX_COLOR_FormatYUV411PackedPlanar || 		//	Feel free to add more formats if implementing them
      colorformat == OMX_COLOR_FormatYUV420Planar ||
      colorformat == OMX_COLOR_FormatYUV420PackedPlanar ||
      colorformat == OMX_COLOR_FormatYUV422Planar ||
      colorformat == OMX_COLOR_FormatYUV422PackedPlanar ) {

    OMX_U32 src_luma_width;			//	Width (in columns) of the source Y plane
    OMX_U32 src_luma_height;		//	Height (in rows) of source Y plane
    OMX_S32 src_luma_stride;		//	Stride in bytes of each source Y row
    OMX_U32 src_luma_offset_x;		//	Horizontal byte offset
    OMX_U32 src_luma_offset_y;		//	Vertical offset in rows from top of plane
    OMX_U32 src_luma_offset;		//	Total byte offset to rectangle

    OMX_U32 src_chroma_width;		//	Width (in columns) of source chroma planes
    OMX_U32 src_chroma_height;		//	Height (in rows) of source chroma planes
    OMX_S32 src_chroma_stride;		//	Stride in bytes of each source chroma row
    OMX_U32 src_chroma_offset_x;	//	Horizontal byte offset
    OMX_U32 src_chroma_offset_y;	//	Vertical offset in rows from top of plane
    OMX_U32 src_chroma_offset;		//	Bytes to crop rectangle from start of chroma plane

    OMX_U32 dest_luma_width;		//	Width (in columns) of the destination Y plane
    OMX_U32 dest_luma_height;		//	Height (in rows) of destination Y plane
    OMX_S32 dest_luma_stride;		//	Stride in bytes of each destination Y row
    OMX_U32 dest_luma_offset_x;		//	Horizontal byte offset
    OMX_U32 dest_luma_offset_y;		//	Vertical offset in rows from top of plane
    OMX_U32 dest_luma_offset;		//	Bytes to crop rectangle from start of Y plane

    OMX_U32 dest_chroma_width;		//	Width (in columns) of destination chroma planes
    OMX_U32 dest_chroma_height;		//	Height (in rows) of destination chroma planes
    OMX_S32 dest_chroma_stride;		//	Stride in bytes of each destination chroma row
    OMX_U32 dest_chroma_offset_x;	//	Horizontal byte offset
    OMX_U32 dest_chroma_offset_y;	//	Vertical offset in rows from top of plane
    OMX_U32 dest_chroma_offset;		//	Bytes to crop rectangle from start of chroma plane

    OMX_U32 luma_crop_width;		//	Width in bytes of a luma row in the crop rectangle
    OMX_U32 luma_crop_height;		//	Number of luma rows in the crop rectangle
    OMX_U32 chroma_crop_width;		//	Width in bytes of a chroma row in the crop rectangle
    OMX_U32 chroma_crop_height;		//	Number of chroma rows in crop rectangle

    switch (colorformat) {
      //	Watch out for odd or non-multiple-of-4 (4:1:1) luma resolutions (I don't check)		
      case OMX_COLOR_FormatYUV411Planar:		//	Planar vs. PackedPlanar will have to be handled differently if/when slicing is implemented
      case OMX_COLOR_FormatYUV411PackedPlanar:
        /**	OpenMAX IL spec says chroma channels are subsampled by 4x horizontally AND vertically in YUV 4:1:1.
          *	Conventional wisdom (wiki) tells us that it is only subsampled horizontally.
          *		Following OpenMAX spec anyway.	Technically I guess this would be YUV 4:1:0.	
          */				
        src_luma_width = src_width;
        src_luma_height = src_height;
        src_luma_stride = (OMX_S32) src_luma_width;
        src_luma_offset_x = src_offset_x;
        src_luma_offset_y = src_offset_y;

        src_chroma_width = src_luma_width  >> 2; 
        src_chroma_height = src_luma_height;
        src_chroma_stride = (OMX_S32) src_chroma_width;
        src_chroma_offset_x = src_luma_offset_x  >> 2; 
        src_chroma_offset_y = src_luma_offset_y;

        dest_luma_width = dest_width;
        dest_luma_height = dest_height;
        dest_luma_stride = (OMX_S32) dest_luma_width;
        dest_luma_offset_x = dest_offset_x;
        dest_luma_offset_y = dest_offset_y;

        dest_chroma_width = dest_luma_width  >> 2;
        dest_chroma_height = dest_luma_height;
        dest_chroma_stride = (OMX_S32) dest_chroma_width;
        dest_chroma_offset_x = dest_luma_offset_x  >> 2; 
        dest_chroma_offset_y = dest_luma_offset_y;

        luma_crop_width = (OMX_U32) abs(cpy_width);
        luma_crop_height = cpy_height;
        chroma_crop_width = luma_crop_width  >> 2; 
        chroma_crop_height = luma_crop_height;
        break;	

      //	Planar vs. PackedPlanar will have to be handled differently if/when slicing is implemented
      case OMX_COLOR_FormatYUV420Planar:		
      case OMX_COLOR_FormatYUV420PackedPlanar:
        src_luma_width = src_width;
        src_luma_height = src_height;
        src_luma_stride = (OMX_S32) src_luma_width;
        src_luma_offset_x = src_offset_x;
        src_luma_offset_y = src_offset_y;

        src_chroma_width = src_luma_width >> 1;
        src_chroma_height = src_luma_height >> 1;
        src_chroma_stride = (OMX_S32) src_chroma_width;
        src_chroma_offset_x = src_luma_offset_x >> 1;
        src_chroma_offset_y = src_luma_offset_y >> 1;

        dest_luma_width = dest_width;
        dest_luma_height = dest_height;
        dest_luma_stride = (OMX_S32) dest_luma_width;
        dest_luma_offset_x = dest_offset_x;
        dest_luma_offset_y = dest_offset_y;

        dest_chroma_width = dest_luma_width >> 1;
        dest_chroma_height = dest_luma_height >> 1;
        dest_chroma_stride = (OMX_S32) dest_chroma_width;
        dest_chroma_offset_x = dest_luma_offset_x >> 1;
        dest_chroma_offset_y = dest_luma_offset_y >> 1;

        luma_crop_width = cpy_width;
        luma_crop_height = cpy_height;
        chroma_crop_width = luma_crop_width >> 1;
        chroma_crop_height = luma_crop_height >> 1;
        break;

      //	Planar vs. PackedPlanar will have to be handled differently if/when slicing is implemented
      case OMX_COLOR_FormatYUV422Planar:		
      case OMX_COLOR_FormatYUV422PackedPlanar:
        src_luma_width = src_width;
        src_luma_height = src_height;
        src_luma_stride = (OMX_S32) src_luma_width;
        src_luma_offset_x = src_offset_x;
        src_luma_offset_y = src_offset_y;

        src_chroma_width = src_luma_width >> 1;
        src_chroma_height = src_luma_height;
        src_chroma_stride = (OMX_S32) src_chroma_width;
        src_chroma_offset_x = src_luma_offset_x >> 1;
        src_chroma_offset_y = src_luma_offset_y;

        dest_luma_width = dest_width;
        dest_luma_height = dest_height;
        dest_luma_stride = (OMX_S32) dest_luma_width;
        dest_luma_offset_x = dest_offset_x;
        dest_luma_offset_y = dest_offset_y;

        dest_chroma_width = dest_luma_width >> 1;
        dest_chroma_height = dest_luma_height;
        dest_chroma_stride = (OMX_S32) dest_chroma_width;
        dest_chroma_offset_x = dest_luma_offset_x >> 1;
        dest_chroma_offset_y = dest_luma_offset_y;

        luma_crop_width = (OMX_U32) abs(cpy_width);
        luma_crop_height = cpy_height;
        chroma_crop_width = luma_crop_width >> 1;
        chroma_crop_height = luma_crop_height;
        break;

      default:
        DEBUG(DEB_LEV_ERR,"\n color format not supported --error \n");
        return;
    }

    /**	Pointers to the start of each plane to make things easier */
    OMX_U8* Y_input_ptr = src_ptr;
    OMX_U8* U_input_ptr = Y_input_ptr + ((OMX_U32) abs(src_luma_stride) * src_luma_height);
    OMX_U8* V_input_ptr = U_input_ptr + ((OMX_U32) abs(src_chroma_stride) * src_chroma_height);

    /**	Figure out total offsets */
    src_luma_offset = (src_luma_offset_y * (OMX_U32) abs(src_luma_stride)) + src_luma_offset_x;
    src_chroma_offset = (src_chroma_offset_y * (OMX_U32) abs(src_chroma_stride)) + src_chroma_offset_x;

    /**	If input stride is negative, reverse source row order */
    if (src_stride < 0) {
      src_luma_offset += ((OMX_U32) abs(src_luma_stride)) * (src_luma_height - 1);
      src_chroma_offset += ((OMX_U32) abs(src_chroma_stride)) * (src_chroma_height - 1);

      if (src_luma_stride > 0) {
        src_luma_stride *= -1;
      }

      if (src_chroma_stride > 0) {
        src_chroma_stride *= -1;	
      }
    }

    /**	Pointers to use with memcpy */
    OMX_U8* src_Y_ptr = Y_input_ptr + src_luma_offset;		
    OMX_U8* src_U_ptr = U_input_ptr + src_chroma_offset;
    OMX_U8*	src_V_ptr = V_input_ptr + src_chroma_offset;

    /**	Pointers to destination planes to make things easier */
    OMX_U8* Y_output_ptr = dest_ptr;
    OMX_U8* U_output_ptr = Y_output_ptr + ((OMX_U32) abs(dest_luma_stride) * dest_luma_height);
    OMX_U8* V_output_ptr = U_output_ptr + ((OMX_U32) abs(dest_chroma_stride) * dest_chroma_height);	

    /**	Figure out total offsets */
    dest_luma_offset = (dest_luma_offset_y * (OMX_U32) abs(dest_luma_stride)) + dest_luma_offset_x;
    dest_chroma_offset = (dest_chroma_offset_y * (OMX_U32) abs(dest_chroma_stride)) + dest_chroma_offset_x;

    /**	If output stride is negative, reverse destination row order */
    if (dest_stride < 0) {
      dest_luma_offset += ((OMX_U32) abs(dest_luma_stride)) * (dest_luma_height - 1);
      dest_chroma_offset += ((OMX_U32) abs(dest_chroma_stride)) * (dest_chroma_height - 1);

      if (dest_luma_stride > 0) {
        dest_luma_stride *= -1;
      }

      if (dest_chroma_stride > 0) {
        dest_chroma_stride *= -1;	
      }
    }

    /**	Pointers to use with memcpy */
    OMX_U8* dest_Y_ptr = Y_output_ptr + dest_luma_offset;		
    OMX_U8* dest_U_ptr = U_output_ptr + dest_chroma_offset;
    OMX_U8*	dest_V_ptr = V_output_ptr + dest_chroma_offset;

    //	Y
    for (i = 0; i < luma_crop_height; ++i, src_Y_ptr += src_luma_stride, dest_Y_ptr += dest_luma_stride) {
      memcpy(dest_Y_ptr, src_Y_ptr, luma_crop_width);	//	Copy Y rows into in_buffer
    }
    //	U
    for (i = 0; i < chroma_crop_height; ++i, src_U_ptr += src_chroma_stride, dest_U_ptr += dest_chroma_stride) {
      memcpy(dest_U_ptr, src_U_ptr, chroma_crop_width);	//	Copy U rows into in_buffer
    }
    //	V
    for (i = 0; i < chroma_crop_height; ++i, src_V_ptr += src_chroma_stride, dest_V_ptr += dest_chroma_stride) {
      memcpy(dest_V_ptr, src_V_ptr, chroma_crop_width);	//	Copy V rows into in_buffer
    }
  } else {	
    OMX_U32 cpy_byte_width = calcStride((OMX_U32) abs(cpy_width), colorformat);	//	Bytes width to copy

    OMX_U32 src_byte_offset_x = calcStride((OMX_U32) abs(src_offset_x), colorformat);
    OMX_U32 dest_byte_offset_x = calcStride((OMX_U32) abs(dest_offset_x), colorformat);

    OMX_U32 src_byte_offset_y = src_offset_y * (OMX_U32) abs(src_stride);
    OMX_U32 dest_byte_offset_y = dest_offset_y * (OMX_U32) abs(dest_stride);

    if (src_stride < 0)	{
      //	If input stride is negative, start from bottom
      src_byte_offset_y += cpy_height * (OMX_U32) abs(src_stride);
    }	
    if (dest_stride < 0) {
      //	If output stride is negative, start from bottom
      dest_byte_offset_y += cpy_height * (OMX_U32) abs(dest_stride);
    }

    OMX_U8* src_cpy_ptr = src_ptr + src_byte_offset_y + src_byte_offset_x;
    OMX_U8* dest_cpy_ptr = dest_ptr + dest_byte_offset_y + dest_byte_offset_x;

    for (i = 0; i < cpy_height; ++i, src_cpy_ptr += src_stride, dest_cpy_ptr += dest_stride) {
      memcpy(dest_cpy_ptr, src_cpy_ptr, cpy_byte_width);	//	Copy rows
    }
  }
}


/** This function is used to process the input buffer and provide one output buffer
  */
void omx_ffmpeg_colorconv_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer) {

  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;
  omx_ffmpeg_colorconv_component_PortType *inPort = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  omx_ffmpeg_colorconv_component_PortType *outPort = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

  OMX_COLOR_FORMATTYPE input_colorformat = inPort->sVideoParam.eColorFormat;
  OMX_S32 input_cpy_width = (OMX_S32) inPort->omxConfigCrop.nWidth;			//	Width (in columns) of the crop rectangle
  OMX_U32 input_cpy_height = inPort->omxConfigCrop.nHeight;					//	Height (in rows) of the crop rectangle

  OMX_U8* input_src_ptr = (OMX_U8*) (pInputBuffer->pBuffer);
  OMX_S32 input_src_stride = inPort->sPortParam.format.video.nStride;			//	Negative means bottom-to-top (think Windows bmp)
  OMX_U32 input_src_width = inPort->sPortParam.format.video.nFrameWidth;
  OMX_U32 input_src_height = inPort->sPortParam.format.video.nSliceHeight;

  /**	FIXME: Configuration values should be clamped to prevent memory trampling and potential segfaults.
    *	It might be best to store clamped AND unclamped values on a per-port basis so that OMX_GetConfig 
    *	can still return the unclamped ones.
    */

  OMX_S32 input_src_offset_x = inPort->omxConfigCrop.nLeft;	//	Offset (in columns) to left side of crop rectangle
  OMX_S32 input_src_offset_y = inPort->omxConfigCrop.nTop;		//	Offset (in rows) from top of the image to crop rectangle

  OMX_U8* input_dest_ptr = (OMX_U8*) omx_ffmpeg_colorconv_component_Private->in_buffer;
  OMX_S32 input_dest_stride = (input_src_stride < 0) ? -1 * calcStride(input_cpy_width, input_colorformat) : calcStride(input_cpy_width, input_colorformat);
  if (inPort->omxConfigMirror.eMirror == OMX_MirrorVertical || inPort->omxConfigMirror.eMirror == OMX_MirrorBoth) {
    input_dest_stride *= -1;
  }
  OMX_U32 input_dest_width = input_cpy_width;
  OMX_U32 input_dest_height = input_cpy_height;
  OMX_U32 input_dest_offset_x = 0;
  OMX_U32 input_dest_offset_y = 0;

  OMX_U8* output_src_ptr = (OMX_U8*) omx_ffmpeg_colorconv_component_Private->conv_buffer;
  OMX_COLOR_FORMATTYPE output_colorformat = outPort->sVideoParam.eColorFormat;
  OMX_U32 output_cpy_width = outPort->omxConfigCrop.nWidth;						//	Width (in columns) of the crop rectangle
  OMX_U32 output_cpy_height = outPort->omxConfigCrop.nHeight;						//	Height (in rows) of the crop rectangle
  OMX_S32 output_dest_stride = outPort->sPortParam.format.video.nStride;			//	Negative means bottom-to-top (think Windows bmp)	
  OMX_S32 output_src_stride = (output_dest_stride < 0) ? -1 * calcStride(input_cpy_width, output_colorformat) : calcStride(input_cpy_width, output_colorformat);
  if (outPort->omxConfigMirror.eMirror == OMX_MirrorVertical || outPort->omxConfigMirror.eMirror == OMX_MirrorBoth) {
    output_src_stride *= -1;
  }
  OMX_U32 output_src_width = input_cpy_width;
  OMX_U32 output_src_height = input_cpy_height;
  OMX_S32 output_src_offset_x = outPort->omxConfigCrop.nLeft;		//	Offset (in columns) to left side of crop rectangle
  OMX_S32 output_src_offset_y = outPort->omxConfigCrop.nTop;		//	Offset (in rows) from top of the image to crop rectangle

  OMX_U8* output_dest_ptr = (OMX_U8*) (pOutputBuffer->pBuffer);
  OMX_U32 output_dest_width = outPort->sPortParam.format.video.nFrameWidth;

  OMX_U32 output_dest_height = outPort->sPortParam.format.video.nSliceHeight;
  OMX_S32 output_dest_offset_x = outPort->omxConfigOutputPosition.nX;
  OMX_S32 output_dest_offset_y = outPort->omxConfigOutputPosition.nY;

  avpicture_fill((AVPicture*) omx_ffmpeg_colorconv_component_Private->in_frame, omx_ffmpeg_colorconv_component_Private->in_buffer, inPort->ffmpeg_pxlfmt, input_dest_width, input_dest_height);
  avpicture_fill((AVPicture*) omx_ffmpeg_colorconv_component_Private->conv_frame, omx_ffmpeg_colorconv_component_Private->conv_buffer, outPort->ffmpeg_pxlfmt, output_src_width, output_src_height);

  //	Copy image data into in_buffer
  omx_img_copy(input_src_ptr, input_src_stride, input_src_width, input_src_height, input_src_offset_x, input_src_offset_y,
                input_dest_ptr, input_dest_stride, input_dest_width, input_dest_height, input_dest_offset_x, input_dest_offset_y,
                input_cpy_width, input_cpy_height, input_colorformat);

  pInputBuffer->nFilledLen = 0;

  //	Use ffmpeg to convert the colors into conv_buffer
  img_convert((AVPicture*) omx_ffmpeg_colorconv_component_Private->conv_frame, outPort->ffmpeg_pxlfmt, 
              (AVPicture*) omx_ffmpeg_colorconv_component_Private->in_frame, inPort->ffmpeg_pxlfmt, 
              input_dest_width, input_dest_height);

  omx_img_copy(output_src_ptr, output_src_stride, output_src_width, output_src_height, 
                output_src_offset_x, output_src_offset_y,output_dest_ptr, output_dest_stride, output_dest_width, 
                output_dest_height, output_dest_offset_x, output_dest_offset_y,
                output_cpy_width, output_cpy_height, output_colorformat);

  pOutputBuffer->nFilledLen = (OMX_U32) abs(output_dest_stride) * output_dest_height;

  DEBUG(DEB_LEV_FULL_SEQ, "in %s One output buffer %x len=%d is full returning in color converter\n", 
          __func__, (int)pOutputBuffer->pBuffer, (int)pOutputBuffer->nFilledLen);
}


OMX_ERRORTYPE omx_ffmpeg_colorconv_component_SetConfig(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nIndex,
  OMX_IN  OMX_PTR pComponentConfigStructure) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_U32 portIndex;
  // Possible configs to set
  OMX_CONFIG_RECTTYPE *omxConfigCrop;
  OMX_CONFIG_ROTATIONTYPE *omxConfigRotate;
  OMX_CONFIG_MIRRORTYPE *omxConfigMirror;
  OMX_CONFIG_SCALEFACTORTYPE *omxConfigScale;
  OMX_CONFIG_POINTTYPE *omxConfigOutputPosition;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;
  omx_ffmpeg_colorconv_component_PortType *port;
  if (pComponentConfigStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Setting configuration %i\n", nIndex);	
  switch (nIndex) {
    case OMX_IndexConfigCommonInputCrop:
    case OMX_IndexConfigCommonOutputCrop:
      omxConfigCrop = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
      portIndex = omxConfigCrop->nPortIndex;
      err = checkHeader(omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
      CHECK_ERROR(err, "Check Header");
      if ( (nIndex == OMX_IndexConfigCommonOutputCrop && portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)  ||
          (nIndex == OMX_IndexConfigCommonInputCrop && portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) ) {
        port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
        port->omxConfigCrop.nLeft = omxConfigCrop->nLeft;
        port->omxConfigCrop.nTop = omxConfigCrop->nTop;
        port->omxConfigCrop.nWidth = omxConfigCrop->nWidth;
        port->omxConfigCrop.nHeight = omxConfigCrop->nHeight;
      } else if (portIndex <= 1) {
        return OMX_ErrorUnsupportedIndex;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexConfigCommonRotate:
      omxConfigRotate = (OMX_CONFIG_ROTATIONTYPE*)pComponentConfigStructure;
      portIndex = omxConfigRotate->nPortIndex;
      err = checkHeader(omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));
      CHECK_ERROR(err, "Check Header");
      if (portIndex <= 1) {
        port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
        if (omxConfigRotate->nRotation != 0) {
          //	Rotation not supported (yet)
          return OMX_ErrorUnsupportedSetting;
        }
        port->omxConfigRotate.nRotation = omxConfigRotate->nRotation;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexConfigCommonMirror:
      omxConfigMirror = (OMX_CONFIG_MIRRORTYPE*)pComponentConfigStructure;
      portIndex = omxConfigMirror->nPortIndex;
      err = checkHeader(omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));
      CHECK_ERROR(err, "Check Header");
      if (portIndex <= 1) {
        if (omxConfigMirror->eMirror == OMX_MirrorBoth || omxConfigMirror->eMirror == OMX_MirrorHorizontal)	{
          //	Horizontal mirroring not yet supported
          return OMX_ErrorUnsupportedSetting;
        }
        port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
        port->omxConfigMirror.eMirror = omxConfigMirror->eMirror;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexConfigCommonScale:
      omxConfigScale = (OMX_CONFIG_SCALEFACTORTYPE*)pComponentConfigStructure;
      portIndex = omxConfigScale->nPortIndex;
      err = checkHeader(omxConfigScale, sizeof(OMX_CONFIG_MIRRORTYPE));
      CHECK_ERROR(err, "Check Header");
      if (portIndex <= 1) {
        if (omxConfigScale->xWidth != 0x10000 || omxConfigScale->xHeight != 0x10000) {
          //	Scaling not yet supported
          return OMX_ErrorUnsupportedSetting;
        }
        port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
        port->omxConfigScale.xWidth = omxConfigScale->xWidth;
        port->omxConfigScale.xHeight = omxConfigScale->xHeight;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexConfigCommonOutputPosition:
      omxConfigOutputPosition = (OMX_CONFIG_POINTTYPE*)pComponentConfigStructure;
      portIndex = omxConfigOutputPosition->nPortIndex;
      err = checkHeader(omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));
      CHECK_ERROR(err, "Check Header");
      if (portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
        port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
        port->omxConfigOutputPosition.nX = omxConfigOutputPosition->nX;
        port->omxConfigOutputPosition.nY = omxConfigOutputPosition->nY;
        //} else if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
      } else if (portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
        return OMX_ErrorUnsupportedIndex;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;
    default: // delegate to superclass
      return omx_base_component_SetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return OMX_ErrorNone;
}



OMX_ERRORTYPE omx_ffmpeg_colorconv_component_GetConfig(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nIndex,
  OMX_INOUT OMX_PTR pComponentConfigStructure) {

  // Possible configs to ask for
  OMX_CONFIG_RECTTYPE *omxConfigCrop;
  OMX_CONFIG_ROTATIONTYPE *omxConfigRotate;
  OMX_CONFIG_MIRRORTYPE *omxConfigMirror;
  OMX_CONFIG_SCALEFACTORTYPE *omxConfigScale;
  OMX_CONFIG_POINTTYPE *omxConfigOutputPosition;

  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;
  omx_ffmpeg_colorconv_component_PortType *port;
  if (pComponentConfigStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Getting configuration %i\n", nIndex);
  /* Check which structure we are being fed and fill its header */
  switch (nIndex) {
    case OMX_IndexConfigCommonInputCrop:
      omxConfigCrop = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
      setHeader(omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
      if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[omxConfigCrop->nPortIndex];
        memcpy(omxConfigCrop, &port->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
      } else if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
        return OMX_ErrorUnsupportedIndex;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    case OMX_IndexConfigCommonOutputCrop:
      omxConfigCrop = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
      setHeader(omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
      if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[omxConfigCrop->nPortIndex];
        memcpy(omxConfigCrop, &port->omxConfigCrop, sizeof(OMX_CONFIG_RECTTYPE));
      } else if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
        return OMX_ErrorUnsupportedIndex;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    case OMX_IndexConfigCommonRotate:
      omxConfigRotate = (OMX_CONFIG_ROTATIONTYPE*)pComponentConfigStructure;
      setHeader(omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));
      if (omxConfigRotate->nPortIndex <= 1) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[omxConfigRotate->nPortIndex];
        memcpy(omxConfigRotate, &port->omxConfigRotate, sizeof(OMX_CONFIG_ROTATIONTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    case OMX_IndexConfigCommonMirror:
      omxConfigMirror = (OMX_CONFIG_MIRRORTYPE*)pComponentConfigStructure;
      setHeader(omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));
      if (omxConfigMirror->nPortIndex <= 1) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[omxConfigMirror->nPortIndex];
        memcpy(omxConfigMirror, &port->omxConfigMirror, sizeof(OMX_CONFIG_MIRRORTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;			
    case OMX_IndexConfigCommonScale:
      omxConfigScale = (OMX_CONFIG_SCALEFACTORTYPE*)pComponentConfigStructure;
      setHeader(omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
      if (omxConfigScale->nPortIndex <= 1) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[omxConfigScale->nPortIndex];
        memcpy(omxConfigScale, &port->omxConfigScale, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    case OMX_IndexConfigCommonOutputPosition:
      omxConfigOutputPosition = (OMX_CONFIG_POINTTYPE*)pComponentConfigStructure;
      setHeader(omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));
      if (omxConfigOutputPosition->nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[omxConfigOutputPosition->nPortIndex];
        memcpy(omxConfigOutputPosition, &port->omxConfigOutputPosition, sizeof(OMX_CONFIG_POINTTYPE));
        //} else if (omxConfigCrop->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
      } else if (omxConfigOutputPosition->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
        return OMX_ErrorUnsupportedIndex;
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    default: // delegate to superclass
      return omx_base_component_GetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_ffmpeg_colorconv_component_SetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
  OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
  OMX_U32 portIndex;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;
  omx_ffmpeg_colorconv_component_PortType *port;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }	
  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Setting parameter %i\n", nParamIndex);
  switch(nParamIndex) {
    case OMX_IndexParamVideoInit:
      /*Check Structure Header*/
      checkHeader(ComponentParameterStructure , sizeof(OMX_PORT_PARAM_TYPE));
      CHECK_ERROR(err, "Check Header");
      memcpy(&omx_ffmpeg_colorconv_component_Private->sPortTypesParam,ComponentParameterStructure,sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamPortDefinition:
      pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
      portIndex = pPortDef->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      CHECK_ERROR(err,"Parameter Check");
      port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
      port->sPortParam.nBufferCountActual = pPortDef->nBufferCountActual;

      //	Copy stuff from OMX_VIDEO_PORTDEFINITIONTYPE structure
      port->sPortParam.format.video.cMIMEType = pPortDef->format.video.cMIMEType;
      port->sPortParam.format.video.nFrameWidth = pPortDef->format.video.nFrameWidth;
      port->sPortParam.format.video.nFrameHeight = pPortDef->format.video.nFrameHeight;
      port->sPortParam.format.video.nBitrate = pPortDef->format.video.nBitrate;
      port->sPortParam.format.video.xFramerate = pPortDef->format.video.xFramerate;
      port->sPortParam.format.video.bFlagErrorConcealment = pPortDef->format.video.bFlagErrorConcealment;	
      //	Figure out stride, slice height, min buffer size
      port->sPortParam.format.video.nStride = calcStride(port->sPortParam.format.video.nFrameWidth, port->sVideoParam.eColorFormat);
      port->sPortParam.format.video.nSliceHeight = port->sPortParam.format.video.nFrameHeight;	//	No support for slices yet
      port->sPortParam.nBufferSize = (OMX_U32) abs(port->sPortParam.format.video.nStride) * port->sPortParam.format.video.nSliceHeight;
      port->omxConfigCrop.nWidth = port->sPortParam.format.video.nFrameWidth;
      port->omxConfigCrop.nHeight = port->sPortParam.format.video.nFrameHeight;
      //	Don't care so much about the other domains
      memcpy(&port->sPortParam.format.audio, &pPortDef->format.audio, sizeof(OMX_AUDIO_PORTDEFINITIONTYPE));
      memcpy(&port->sPortParam.format.image, &pPortDef->format.image, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));
      memcpy(&port->sPortParam.format.other, &pPortDef->format.other, sizeof(OMX_OTHER_PORTDEFINITIONTYPE));
      break;
    case OMX_IndexParamVideoPortFormat:
      //	FIXME: How do we handle the nIndex member?
      pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      portIndex = pVideoPortFormat->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      CHECK_ERROR(err,"Parameter Check");
      port = (omx_ffmpeg_colorconv_component_PortType *) omx_ffmpeg_colorconv_component_Private->ports[portIndex];
      if (pVideoPortFormat->eCompressionFormat != OMX_VIDEO_CodingUnused)	{
        //	No compression allowed
        return OMX_ErrorUnsupportedSetting;
      }
      port->sVideoParam.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
      port->sVideoParam.eColorFormat = pVideoPortFormat->eColorFormat;
      port->ffmpeg_pxlfmt = find_ffmpeg_pxlfmt(port->sVideoParam.eColorFormat);

      if(port->ffmpeg_pxlfmt == PIX_FMT_NONE) {
        /** no real pixel format supported by ffmpeg for this user input color format
          * so return bad parameter error to user application */
        return OMX_ErrorBadParameter;	        
      }
      //	Figure out stride, slice height, min buffer size
      port->sPortParam.format.video.nStride = calcStride(port->sPortParam.format.video.nFrameWidth, port->sVideoParam.eColorFormat);
      port->sPortParam.format.video.nSliceHeight = port->sPortParam.format.video.nFrameHeight;	//	No support for slices yet
      port->sPortParam.nBufferSize = (OMX_U32) abs(port->sPortParam.format.video.nStride) * port->sPortParam.format.video.nSliceHeight;
      break;
    default: /*Call the base component function*/
      return omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_ffmpeg_colorconv_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure) {

  OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
  OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = openmaxStandComp->pComponentPrivate;
  omx_ffmpeg_colorconv_component_PortType *port;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }

  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Getting parameter %i\n", nParamIndex);
  /* Check which structure we are being fed and fill its header */
  switch(nParamIndex) {
    case OMX_IndexParamVideoInit:
      setHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
      memcpy(ComponentParameterStructure, &omx_ffmpeg_colorconv_component_Private->sPortTypesParam, sizeof(OMX_PORT_PARAM_TYPE));
      break;		
    case OMX_IndexParamVideoPortFormat:
      pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      setHeader(pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      if (pVideoPortFormat->nPortIndex <= 1) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[pVideoPortFormat->nPortIndex];
        memcpy(pVideoPortFormat, &port->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    case OMX_IndexParamPortDefinition:
      pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
      setHeader(pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      if (pPortDef->nPortIndex <= 1) {
        port = (omx_ffmpeg_colorconv_component_PortType *)omx_ffmpeg_colorconv_component_Private->ports[pPortDef->nPortIndex];
        memcpy(pPortDef, &port->sPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;		
    default: /*Call the base component function*/
      return omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_video_colorconv_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp,internalRequestMessageType *message) {
  omx_ffmpeg_colorconv_component_PrivateType* omx_ffmpeg_colorconv_component_Private = (omx_ffmpeg_colorconv_component_PrivateType*)openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err;
  OMX_STATETYPE eState;

  DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s\n", __func__);
  eState = omx_ffmpeg_colorconv_component_Private->state; //storing current state

  if (message->messageType == OMX_CommandStateSet) {
    if ((message->messageParam == OMX_StateExecuting ) && (omx_ffmpeg_colorconv_component_Private->state == OMX_StateIdle)) {
      err = omx_ffmpeg_colorconv_component_Init(openmaxStandComp);
      CHECK_ERROR(err,"Video Color Converter Init Failed");
    }
  }
  // Execute the base message handling
  err = omx_base_component_MessageHandler(openmaxStandComp,message);

  if (message->messageType == OMX_CommandStateSet) {
    if ((message->messageParam == OMX_StateIdle ) && (omx_ffmpeg_colorconv_component_Private->state == OMX_StateIdle) && eState == OMX_StateExecuting) {
      err = omx_ffmpeg_colorconv_component_Deinit(openmaxStandComp);
      CHECK_ERROR(err,"Video Color Converter Deinit Failed");
    }
  }
  return err;
}