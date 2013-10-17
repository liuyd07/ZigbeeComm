#ifndef     HAL_CCM_H_
#define     HAL_CCM_H_

void SSP_CCM_Auth (uint8, uint8 *, uint8 *, uint16, uint8 *, uint16, uint8 *, uint8 *);
void SSP_CCM_Encrypt (uint8, uint8 *, uint8 *, uint16, uint8 *, uint8 *);
void SSP_CCM_Decrypt (uint8, uint8 *, uint8 *, uint16, uint8 *, uint8 *);
uint8 SSP_CCM_InvAuth (uint8, uint8 *, uint8 *, uint16, uint8 *, uint16, uint8 *, uint8 *);

#endif  // HAL_CCM_H_

