/*****************************************************************************
 * @file    osal.c
 *
 * @brief   Implements the interface defined in "osal.h" needed by the stack.
 *          Actually, only memset, memcpy and memcmp wrappers are implemented.
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2018-2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *****************************************************************************
 */

#include <string.h>
#include "osal.h"


/**
 * Osal_MemCpy
 *
 */

void* Osal_MemCpy( void *dest, const void *src, unsigned int size )
{
  return memcpy( dest, src, size );
}

/**
 * Osal_MemSet
 *
 */

void* Osal_MemSet( void *ptr, int value, unsigned int size )
{
  return memset( ptr, value, size );
}

/**
 * Osal_MemCmp
 *
 */
int Osal_MemCmp( const void *s1, const void *s2, unsigned int size )
{
  return memcmp( s1, s2, size );
}
