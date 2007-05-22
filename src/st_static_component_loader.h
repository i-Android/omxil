/**
  @file src/st_omx_component_loader.h

  ST specific component loader for local components.

  Copyright (C) 2007  STMicroelectronics and Nokia

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

  $Date: 2007-04-27 14:41:59 +0200 (Fri, 27 Apr 2007) $
  Revision $Rev: 802 $
  Author $Author: giulio_urlini $
*/

#ifndef __ST_OMX_COMPONENT_LOADER_H__
#define __ST_OMX_COMPONENT_LOADER_H__

#include "omxcore.h"
struct BOSA_COMPONENTLOADER st_static_loader;

/** @brief the private data structure handled by the ST static loader that described
 * an openMAX component
 * 
 * This structure contains all the fields that the loader must use to support 
 * the loading  unloading functions of the component, that are not described by the
 * standard.
 */
typedef struct stLoaderComponentType{
  OMX_VERSIONTYPE componentVersion; /**< the verison of the component in the OpenMAX standard format */
  char* name; /**< String that represents the name of the component, ruled by the standard */
  int name_specific_length;/**< this field contains the number of roles of the component */
  char** name_specific; /**< Strings those represent the names of the specifc format components */
  char** role_specific; /**< Strings those represent the names of the specifc format components */
  char* name_requested; /**< This parameter is used to send to the component the string requested by the IL Client */
  OMX_ERRORTYPE (*constructor)(OMX_COMPONENTTYPE*,OMX_STRING cComponentName); /**< constructor function pointer for each Linux ST OpenMAX component */
  OMX_ERRORTYPE (*destructor)(OMX_COMPONENTTYPE*); /**< constructor function pointer for each Linux ST OpenMAX component */
} stLoaderComponentType;

/** @brief The initialization of the ST specific component loader. 
 */
void st_static_InitComponentLoader();

/** @brief The constructor of the ST specific component loader. 
 * 
 * It is the component loader developed under linux by ST, for local libraries.
 * It is based on a registry file, like in the case of Gstreamer. It reads the 
 * registry file, and allows the components to register themself to the 
 * main list templateList.
 */
OMX_ERRORTYPE BOSA_ST_CreateComponentLoader(BOSA_ComponentLoaderHandle *loaderHandle);

/** @brief The destructor of the ST specific component loader. 
 * 
 * This function deallocates the list of available components.
 */
OMX_ERRORTYPE BOSA_ST_DestroyComponentLoader(BOSA_ComponentLoaderHandle loaderHandle);

/** @brief creator of the requested openmax component
 * 
 * This function searches for the requested component in the internal list.
 * If the copmponent is found, its constructor is called,
 * and the standard callback are assigned.
 * A pointer to a standard openmax component is returned.
 */
OMX_ERRORTYPE BOSA_ST_CreateComponent(
    BOSA_ComponentLoaderHandle loaderHandle,
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);

/** @brief This function search for the index from 0 to end of the list
 * 
 * This function searches in the list of ST static components and enumerates
 * both the class names and the role specific components.
 */ 
OMX_ERRORTYPE BOSA_ST_ComponentNameEnum(
    BOSA_ComponentLoaderHandle loaderHandle,
    OMX_STRING cComponentName,
    OMX_U32 nNameLength,
    OMX_U32 nIndex);

/** @brief The specific version of OMX_GetRolesOfComponent 
 * 
 * This function replicates exactly the behavior of the 
 * standard OMX_GetRolesOfComponent function for the ST static
 * component loader 
 */
OMX_ERRORTYPE BOSA_ST_GetRolesOfComponent( 
    BOSA_ComponentLoaderHandle loaderHandle,
    OMX_STRING compName,
    OMX_U32 *pNumRoles,
    OMX_U8 **roles);

/** @brief The specific version of OMX_GetComponentsOfRole 
 * 
 * This function replicates exactly the behavior of the 
 * standard OMX_GetComponentsOfRole function for the ST static
 * component loader 
 */
OMX_API OMX_ERRORTYPE BOSA_ST_GetComponentsOfRole ( 
    BOSA_ComponentLoaderHandle loaderHandle,
    OMX_STRING role,
    OMX_U32 *pNumComps,
    OMX_U8  **compNames);
 
#endif