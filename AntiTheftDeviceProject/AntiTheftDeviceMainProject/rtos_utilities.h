#ifndef RTOS_UTILITIES_H
#define RTOS_UTILITIES_H

static const BaseType_t app_cpu = 0;

static TimerHandle_t one_shot_timer = NULL;

static const uint8_t handle_state_msg_queue_len = 1;
static QueueHandle_t handle_state_msg_queue;
static const uint8_t distance_sensor_msg_queue_len = 1;
static QueueHandle_t distance_sensor_msg_queue;
static const uint8_t ontime_secu_msg_queue_len = 1;
static QueueHandle_t ontime_secu_msg_queue;
// static const uint8_t invoke_queue_len = 1;
// static QueueHandle_t invoke_queue;
// static TimerHandle_t auto_reload_timer = NULL;

// Task handler
static TaskHandle_t task_1_handler = NULL;
static TaskHandle_t task_2_handler = NULL;
static TaskHandle_t task_3_handler = NULL;
static TaskHandle_t task_4_handler = NULL;
static TaskHandle_t task_5_handler = NULL;

bool freeRTOS_task_create_pinned_to_core(TaskFunction_t pv_task_code, const char *name, const uint32_t stack_size, 
                                        void *const pv_parameters, UBaseType_t ux_priority, TaskHandle_t *const pv_created_task,
                                        const BaseType_t core_id)
{
    BaseType_t task_creation_result = xTaskCreatePinnedToCore(
                                  pv_task_code,                      // Function to be called
                                  name,                              // Name of task
                                  stack_size,                        // Stack size (bytes in ESP32, word in FreeRTOS)
                                  pv_parameters,                     // parameter to pass function
                                  ux_priority,                       // Task priority ( 0 to configMAX_PRIORITIES - 1)
                                  pv_created_task,                   // Task handle
                                  core_id);

  if (task_creation_result == pdPASS) 
  {
    return true;
  }

  return false;
}

#endif
