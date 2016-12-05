/*----------------------------------------------------------------------------
 * Name:    ADC.h
 * Purpose: ADC realted definitions
 * Note(s): 
 *--------------------------------------------------------------------------*/
 
 
#ifndef __ADC_H
#define __ADC_H
 
extern uint16_t AD_last;
extern uint8_t  AD_done;
 
extern void ADC_Init (void);
extern void ADC_IRQHandler( void );
extern void ADC_StartCnv (void );
extern void     ADC_StopCnv (void);
extern uint16_t ADC_GetCnv  (void);

#endif
