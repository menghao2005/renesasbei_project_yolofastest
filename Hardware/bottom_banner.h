/* TrustZone Non-Secure address alias for logo data in NS flash */
#define NS_ALIAS(p) ((uint16_t *)((uint32_t)(p) + 0x10000000U))
#ifndef BOTTOM_BANNER_H_
#define BOTTOM_BANNER_H_

#include <stdint.h>

void bottom_banner_draw(uint16_t *fb, uint32_t stride);

#endif
