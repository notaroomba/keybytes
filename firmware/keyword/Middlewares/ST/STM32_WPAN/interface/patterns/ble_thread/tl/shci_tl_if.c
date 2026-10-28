/**
 ******************************************************************************
 * @file    shci_tl_if.c
 * @author  MCD Application Team
 * @brief   Transport layer interface to the system channel
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2018-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "shci_tl.h"
#include "tl.h"


void shci_register_io_bus(tSHciIO* fops)
{
  /* Register IO bus services */
  fops->Init    = TL_SYS_Init;
  fops->Send    = TL_SYS_SendCmd;

  return;
}
