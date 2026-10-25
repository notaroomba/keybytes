/**
  ******************************************************************************
  * @file    stm32_seq.c
  * @author  MCD Application Team
  * @brief   Simple sequencer implementation
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_seq.h"
#include "utilities_conf.h"

/** @addtogroup SEQUENCER
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/** @defgroup SEQUENCER_Private_type SEQUENCER private type
  *  @{
  */

/**
  * @brief structure used to manage task scheduling
  */
typedef struct
{
    uint32_t priority;    /*!<bit field of the enabled task.          */
    uint32_t round_robin; /*!<mask on the allowed task to be running. */
} UTIL_SEQ_Priority_t;

/**
  * @}
  */

/* Private defines -----------------------------------------------------------*/

/** @defgroup SEQUENCER_Private_define SEQUENCER private defines
  *  @{
  */

/**
  * @brief macro used to enter the critical section before calling the IDLE function
  * @note  in a basic configuration shall be identical to the macro
  *        UTIL_SEQ_ENTER_CRITICAL_SECTION. The redefinition of this macro will allow
  *        to perform specific operation

  */
#ifndef UTIL_SEQ_ENTER_CRITICAL_SECTION_IDLE
#define UTIL_SEQ_ENTER_CRITICAL_SECTION_IDLE( )    UTIL_SEQ_ENTER_CRITICAL_SECTION( )
#endif /* UTIL_SEQ_ENTER_CRITICAL_SECTION_IDLE */

/**
  * @brief macro used to exit the critical section when exiting the IDLE function
  * @note  the behavior of the macro shall be symmetrical with the macro
  *        UTIL_SEQ_ENTER_CRITICAL_SECTION_IDLE
  */
#ifndef UTIL_SEQ_EXIT_CRITICAL_SECTION_IDLE
#define UTIL_SEQ_EXIT_CRITICAL_SECTION_IDLE( )     UTIL_SEQ_EXIT_CRITICAL_SECTION( )
#endif /* UTIL_SEQ_EXIT_CRITICAL_SECTION_IDLE */

/**
  * @brief define to represent no task running
  */
#define UTIL_SEQ_NOTASKRUNNING       (0xFFFFFFFFU)

/**
  * @brief define to represent no bit set inside uint32_t mapping
  */
#define UTIL_SEQ_NO_BIT_SET     (0U)

/**
  * @brief define to represent all bits set inside uint32_t mapping
  */
#define UTIL_SEQ_ALL_BIT_SET    (~0U)

/**
  * @brief default number of task is default 32 (maximum), can be reduced by redefining in utilities_conf.h
  */
#ifndef UTIL_SEQ_CONF_TASK_NBR
#define UTIL_SEQ_CONF_TASK_NBR  (32)
#endif /* UTIL_SEQ_CONF_TASK_NBR */

#if UTIL_SEQ_CONF_TASK_NBR > 32
#error "UTIL_SEQ_CONF_TASK_NBR must be less than or equal to 32"
#endif /* UTIL_SEQ_CONF_TASK_NBR */

/**
  * @brief default value of priority number.
  */
#ifndef UTIL_SEQ_CONF_PRIO_NBR
#define UTIL_SEQ_CONF_PRIO_NBR  (2)
#endif /* UTIL_SEQ_CONF_PRIO_NBR */

/**
  * @brief default memset function.
  */
#ifndef UTIL_SEQ_MEMSET8
#define UTIL_SEQ_MEMSET8( dest, value, size )   UTILS_MEMSET8( dest, value, size )
#endif /* UTIL_SEQ_MEMSET8 */

/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/

/** @defgroup SEQUENCER_Private_varaible SEQUENCER private variables
  *  @{
  */

/**
  * @brief task set.
  */
static volatile UTIL_SEQ_bm_t TaskSet;

/**
  * @brief task mask.
  */
static volatile UTIL_SEQ_bm_t TaskMask = UTIL_SEQ_ALL_BIT_SET;

/**
  * @brief super mask.
  */
static UTIL_SEQ_bm_t SuperMask = UTIL_SEQ_ALL_BIT_SET;

/**
  * @brief evt set mask.
  */
static volatile UTIL_SEQ_bm_t EvtSet = UTIL_SEQ_NO_BIT_SET;

/**
  * @brief evt expected mask.
  */
static volatile UTIL_SEQ_bm_t EvtWaited = UTIL_SEQ_NO_BIT_SET;

/**
  * @brief current task id.
  */
static uint32_t CurrentTaskIdx = 0U;

/**
  * @brief task function registered.
  */
static void (*TaskCb[UTIL_SEQ_CONF_TASK_NBR])( void );

/**
  * @brief task prio management.
  */
static volatile UTIL_SEQ_Priority_t TaskPrio[UTIL_SEQ_CONF_PRIO_NBR];


/**
  * @brief List of the cleared task
  */
static UTIL_SEQ_bm_t  TaskClearList = 0;

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup SEQUENCER_Private_function SEQUENCER private functions
  *  @{
  */
uint8_t SEQ_BitPosition(uint32_t Value);

/**
  * @}
  */

/* Functions Definition ------------------------------------------------------*/


/** @addtogroup SEQUENCER_Exported_function SEQUENCER exported functions
  *  @{
  */
void UTIL_SEQ_Init( void )
{
    TaskSet = UTIL_SEQ_NO_BIT_SET;
    TaskMask = UTIL_SEQ_ALL_BIT_SET;
    SuperMask = UTIL_SEQ_ALL_BIT_SET;
    EvtSet = UTIL_SEQ_NO_BIT_SET;
    EvtWaited = UTIL_SEQ_NO_BIT_SET;
    CurrentTaskIdx = 0U;
    (void)UTIL_SEQ_MEMSET8((uint8_t *)TaskCb, 0, sizeof(TaskCb));
    for(uint32_t index = 0; index < UTIL_SEQ_CONF_PRIO_NBR; index++)
    {
        TaskPrio[index].priority = 0;
        TaskPrio[index].round_robin = 0;
    }
    UTIL_SEQ_INIT_CRITICAL_SECTION( );
    TaskClearList = 0;
}

void UTIL_SEQ_DeInit( void )
{
}

/**
  * This function can be nested.
  * That is the reason why many variables that are used only in that function are declared static.
  * Note: These variables could have been declared static in the function.
  *
  */
void UTIL_SEQ_Run( UTIL_SEQ_bm_t Mask_bm )
{
    uint32_t counter;
    UTIL_SEQ_bm_t current_task_set;
    UTIL_SEQ_bm_t super_mask_backup;
    UTIL_SEQ_bm_t local_taskset;
    UTIL_SEQ_bm_t local_evtset;
    UTIL_SEQ_bm_t local_taskmask;
    UTIL_SEQ_bm_t local_evtwaited;
    uint32_t round_robin[UTIL_SEQ_CONF_PRIO_NBR];
    UTIL_SEQ_bm_t  task_starving_list;

    /*
     * When this function is nested, the mask to be applied cannot be larger than the first call
     * The mask is always getting smaller and smaller
     * A copy is made of the mask set by UTIL_SEQ_Run() in case it is called again in the task
     */
    super_mask_backup = SuperMask;
    SuperMask &= Mask_bm;

    /*
     * There are two independent mask to check:
     * TaskMask that comes from UTIL_SEQ_PauseTask() / UTIL_SEQ_ResumeTask
     * SuperMask that comes from UTIL_SEQ_Run
     * If the waited event is there, exit from  UTIL_SEQ_Run() to return to the
     * waiting task
     */
    local_taskset = TaskSet;
    local_evtset = EvtSet;
    local_taskmask = TaskMask;
    local_evtwaited =  EvtWaited;
    while(((local_taskset & local_taskmask & SuperMask) != 0U) && ((local_evtset & local_evtwaited)==0U))
    {
        counter = 0U;
        /*
         * When a flag is set, the associated bit is set in TaskPrio[counter].priority mask depending
         * on the priority parameter given from UTIL_SEQ_SetTask()
         * The while loop is looking for a flag set from the highest priority maskr to the lower
         */
        while((TaskPrio[counter].priority & local_taskmask & SuperMask)== 0U)
        {
            counter++;
        }

        current_task_set = TaskPrio[counter].priority & local_taskmask & SuperMask;

        /*
         * The round_robin register is a mask of allowed flags to be evaluated.
         * The concept is to make sure that on each round on UTIL_SEQ_Run(), if two same flags are always set,
         * the sequencer does not run always only the first one.
         * When a task has been executed, The flag is removed from the round_robin mask.
         * If on the next UTIL_SEQ_RUN(), the two same flags are set again, the round_robin mask will
         * mask out the first flag so that the second one can be executed.
         * Note that the first flag is not removed from the list of pending task but just masked by
         * the round_robin mask
         *
         * In the check below, the round_robin mask is reinitialize in case all pending
         * tasks haven been executed at least once
         */
        if ((TaskPrio[counter].round_robin & current_task_set) == 0U)
        {
            TaskPrio[counter].round_robin = UTIL_SEQ_ALL_BIT_SET;
        }

        /*
         * Compute the Stack Startving List
         * This is the list of the task that have been set at least once minus the one that have been cleared ar least once
         */
        task_starving_list = TaskSet;

        /*
         * Due to the concept of TaskPrio[counter].round_robin and TaskClearList, it could be that at some points in time,
         * (when using UTIL_SEQ_WaitEvt()), that there is a situation where at the same time, a bit is set in TaskPrio[counter].round_robin
         * and reset in TaskClearList and another bit is set in TaskClearList and reset in TaskPrio[counter].round_robin.
         * Such situation shall not happen when evaluating task_starving_list
         * At any time, there should not be any bit reset in TaskPrio[counter].round_robin and reset in TaskClearList
         * It is correct with regard to the Sequencer Architecture to set in TaskClearList all tasks that are said to be executed from TaskPrio[counter].round_robin
         * This synchronizes both information before calculating the CurrentTaskIdx
         */
        TaskClearList |= (~TaskPrio[counter].round_robin);

        task_starving_list &= (~TaskClearList);

        /*
         * Consider first the starving list and update current_task_set accordingly
         */
        if ((task_starving_list & current_task_set) != 0U)
        {
          current_task_set = (task_starving_list & current_task_set);
        }
        else
        {
          /* nothing to do */
        }

        /*
         * Reinitialize the Starving List if required
         */
        if(task_starving_list == 0)
        {
          TaskClearList = 0;
        }

        /*
         * Read the flag index of the task to be executed
         * Once the index is read, the associated task will be executed even though a higher priority stack is requested
         * before task execution.
         */
        CurrentTaskIdx = (SEQ_BitPosition(current_task_set & TaskPrio[counter].round_robin));

        UTIL_SEQ_ENTER_CRITICAL_SECTION( );
        /* remove from the list or pending task the one that has been selected to be executed */
        TaskSet &= ~(1U << CurrentTaskIdx);

        /*
         * remove from all priority mask the task that has been selected to be executed
         */
        for (counter = UTIL_SEQ_CONF_PRIO_NBR; counter != 0U; counter--)
        {
          TaskPrio[counter - 1u].priority    &= ~(1U << CurrentTaskIdx);
        }
        UTIL_SEQ_EXIT_CRITICAL_SECTION( );

        UTIL_SEQ_PreTask(CurrentTaskIdx);

        /*
         * Check that function exists before calling it
         */
        if ((CurrentTaskIdx < UTIL_SEQ_CONF_TASK_NBR) && (TaskCb[CurrentTaskIdx] != NULL))
        {
          /*
           * save the round-robin value to take into account the operation done in UTIL_SEQ_WaitEvt
           */
          for (uint32_t index = 0; index < UTIL_SEQ_CONF_PRIO_NBR; index++)
          {
            TaskPrio[index].round_robin &= ~(1U << CurrentTaskIdx);
            round_robin[index] = TaskPrio[index].round_robin;
          }

          /* Execute the task */
          TaskCb[CurrentTaskIdx]( );

          /*
           * restore the round-robin context
           */
          for (uint32_t index = 0; index < UTIL_SEQ_CONF_PRIO_NBR; index++)
          {
            TaskPrio[index].round_robin &= round_robin[index];
          }

          UTIL_SEQ_PostTask(CurrentTaskIdx);

          local_taskset = TaskSet;
          local_evtset = EvtSet;
          local_taskmask = TaskMask;
          local_evtwaited = EvtWaited;

          /*
           * Update the two list for next round
           */
          TaskClearList |= (1U << CurrentTaskIdx);
        }
        else
        {
          /*
           * must never occurs, it means there is a warning in the system
           */
          UTIL_SEQ_CatchWarning(UTIL_SEQ_WARNING_INVALIDTASKID);
        }
    }

    /* the set of CurrentTaskIdx to no task running allows to call WaitEvt in the Pre/Post ilde context */
    CurrentTaskIdx = UTIL_SEQ_NOTASKRUNNING;
    /* if a waited event is present, ignore the IDLE sequence */
    if ((local_evtset & EvtWaited)== 0U)
    {
        UTIL_SEQ_PreIdle( );

        UTIL_SEQ_ENTER_CRITICAL_SECTION_IDLE( );
        local_taskset = TaskSet;
        local_evtset = EvtSet;
        local_taskmask = TaskMask;
        if ((local_taskset & local_taskmask & SuperMask) == 0U)
        {
            if ((local_evtset & EvtWaited)== 0U)
            {
                UTIL_SEQ_Idle( );
            }
        }
        UTIL_SEQ_EXIT_CRITICAL_SECTION_IDLE( );

        UTIL_SEQ_PostIdle( );
    }

    /* restore the mask from UTIL_SEQ_Run() */
    SuperMask = super_mask_backup;

    return;
}

void UTIL_SEQ_RegTask(UTIL_SEQ_bm_t TaskId_bm, uint32_t Flags, void (*Task)( void ))
{
    (void)Flags;
    UTIL_SEQ_ENTER_CRITICAL_SECTION();

    TaskCb[SEQ_BitPosition(TaskId_bm)] = Task;

    UTIL_SEQ_EXIT_CRITICAL_SECTION();

    return;
}

uint32_t UTIL_SEQ_IsRegisteredTask(UTIL_SEQ_bm_t TaskId_bm )
{
    uint32_t _status = 0;
    UTIL_SEQ_ENTER_CRITICAL_SECTION();

    if ( TaskCb[SEQ_BitPosition(TaskId_bm)] != NULL )
    {
        _status = 1;
    }

    UTIL_SEQ_EXIT_CRITICAL_SECTION();
    return _status;
}

void UTIL_SEQ_SetTask( UTIL_SEQ_bm_t TaskId_bm, uint32_t Task_Prio )
{
    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    TaskSet |= TaskId_bm;
    TaskPrio[Task_Prio].priority |= TaskId_bm;

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );

    return;
}

uint32_t UTIL_SEQ_IsSchedulableTask( UTIL_SEQ_bm_t TaskId_bm)
{
    uint32_t _status;
    UTIL_SEQ_bm_t local_taskset;

    UTIL_SEQ_ENTER_CRITICAL_SECTION();

    local_taskset = TaskSet;
    _status = ((local_taskset & TaskMask & SuperMask & TaskId_bm) == TaskId_bm)? 1U: 0U;

    UTIL_SEQ_EXIT_CRITICAL_SECTION();
    return _status;
}

void UTIL_SEQ_PauseTask( UTIL_SEQ_bm_t TaskId_bm )
{
    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    TaskMask &= (~TaskId_bm);

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );

    return;
}

uint32_t UTIL_SEQ_IsPauseTask( UTIL_SEQ_bm_t TaskId_bm )
{
    uint32_t _status;
    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    _status = ((TaskMask & TaskId_bm) == TaskId_bm) ? 0u:1u;

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );
    return _status;
}

void UTIL_SEQ_ResumeTask( UTIL_SEQ_bm_t TaskId_bm )
{
    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    TaskMask |= TaskId_bm;

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );

    return;
}

void UTIL_SEQ_SetEvt( UTIL_SEQ_bm_t EvtId_bm )
{
    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    EvtSet |= EvtId_bm;

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );

    return;
}

void UTIL_SEQ_ClrEvt( UTIL_SEQ_bm_t EvtId_bm )
{
    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    EvtSet &= (~EvtId_bm);

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );

    return;
}

void UTIL_SEQ_WaitEvt(UTIL_SEQ_bm_t EvtId_bm)
{
    UTIL_SEQ_bm_t event_waited_id_backup;
    UTIL_SEQ_bm_t current_task_idx;
    UTIL_SEQ_bm_t wait_task_idx;
    /*
     * store in local the current_task_id_bm as the global variable CurrentTaskIdx
     * may be overwritten in case there are nested call of UTIL_SEQ_Run()
     */
    current_task_idx = CurrentTaskIdx;
    if(UTIL_SEQ_NOTASKRUNNING == CurrentTaskIdx)
    {
        wait_task_idx = 0u;
    }
    else
    {
        wait_task_idx = (uint32_t)1u << CurrentTaskIdx;
    }

    /* backup the event id that was currently waited */
    event_waited_id_backup = EvtWaited;
    EvtWaited = EvtId_bm;
    /*
     * wait for the new event
     * note: that means that if the previous waited event occurs, it will not exit
     * the while loop below.
     * The system is waiting only for the last waited event.
     * When it will go out, it will wait again from the previous one.
     * It case it occurs while waiting for the second one, the while loop will exit immediately
     */

    while ((EvtSet & EvtId_bm) == 0U)
    {
        UTIL_SEQ_EvtIdle(wait_task_idx, EvtId_bm);
    }

    /*
     * Restore the CurrentTaskIdx that may have been modified by call of UTIL_SEQ_Run()
     * from UTIL_SEQ_EvtIdle(). This is required so that a second call of UTIL_SEQ_WaitEvt()
     * in the same process pass the correct current_task_id_bm in the call of UTIL_SEQ_EvtIdle()
     */
    CurrentTaskIdx = current_task_idx;

    UTIL_SEQ_ENTER_CRITICAL_SECTION( );

    EvtSet &= (~EvtId_bm);

    UTIL_SEQ_EXIT_CRITICAL_SECTION( );

    EvtWaited = event_waited_id_backup;
    return;
}

UTIL_SEQ_bm_t UTIL_SEQ_IsEvtPend( void )
{
    UTIL_SEQ_bm_t local_evtwaited = EvtWaited;
    return (EvtSet & local_evtwaited);
}

__WEAK void UTIL_SEQ_EvtIdle( UTIL_SEQ_bm_t TaskId_bm, UTIL_SEQ_bm_t EvtWaited_bm )
{
    (void)EvtWaited_bm;
    UTIL_SEQ_Run(~TaskId_bm);
    return;
}

__WEAK void UTIL_SEQ_Idle( void )
{
    return;
}

__WEAK void UTIL_SEQ_PreIdle( void )
{
    /*
     * Unless specified by the application, there is nothing to be done
     */
    return;
}

__WEAK void UTIL_SEQ_PostIdle( void )
{
    /*
     * Unless specified by the application, there is nothing to be done
     */
    return;
}

__WEAK void UTIL_SEQ_PreTask( uint32_t TaskId )
{
    (void)TaskId;
    return;
}

__WEAK void UTIL_SEQ_PostTask( uint32_t TaskId )
{
    (void)TaskId;
    return;
}

__WEAK void UTIL_SEQ_CatchWarning(UTIL_SEQ_WARNING WarningId)
{
    (void)WarningId;
    return;
}

/**
  * @}
  */

/** @addtogroup SEQUENCER_Private_function
  *  @{
  */

#if( __CORTEX_M == 0)
const uint8_t SEQ_clz_table_4bit[16U] = { 4U, 3U, 2U, 2U, 1U, 1U, 1U, 1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U };
/**
  * @brief return the position of the first bit set to 1
  * @param Value 32 bit value
  * @retval bit position
  */
uint8_t SEQ_BitPosition(uint32_t Value)
{
    uint8_t position = 0U;
    uint32_t lvalue = Value;

    if ((lvalue & 0xFFFF0000U) == 0U)
    {
        position  = 16U;
        lvalue <<= 16U;
    }
    if ((lvalue & 0xFF000000U) == 0U)
    {
        position +=  8U;
        lvalue <<=  8U;
    }
    if ((lvalue & 0xF0000000U) == 0U)
    {
        position +=  4U;
        lvalue <<=  4U;
    }

    position += SEQ_clz_table_4bit[lvalue >> (32-4)];

    return (uint8_t)(31U-position);
}
#else
/**
  * @brief return the position of the first bit set to 1
  * @param Value 32 bit value
  * @retval bit position
  */
uint8_t SEQ_BitPosition(uint32_t Value)
{
    return (uint8_t)(31 -__CLZ( Value ));
}
#endif /* __CORTEX_M == 0 */

/**
  * @}
  */

/**
  * @}
  */

