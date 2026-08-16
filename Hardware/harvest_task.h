#ifndef __HARVEST_TASK_H__
#define __HARVEST_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ai_postprocess.h"

/*
 * Change this value to switch the full harvest behavior.
 * Ground: search right, grab from ground, search left, grab from ground, then home.
 * Tree:   drive, align while approaching, grab fruit from tree.
 */
#define HARVEST_TASK_GROUND        (0U)
#define HARVEST_TASK_TREE          (1U)
#define HARVEST_TASK_SELECT        HARVEST_TASK_GROUND

void HarvestTask_Init(void);
void HarvestTask_Service(const ai_detection_t * p_dets, uint32_t num_dets);

#ifdef __cplusplus
}
#endif

#endif /* __HARVEST_TASK_H__ */
