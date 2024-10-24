/***********************************************************************
Copyright (c) 2006-2011, Skype Limited. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of Internet Society, IETF or IETF Trust, nor the
names of specific contributors, may be used to endorse or promote
products derived from this software without specific prior written
permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"

/* Wrappers. Calls flp / fix code */

/* Convert AR filter coefficients to NLSF parameters */
void silk_A2NLSF_FLP(
    opus_int16                      *NLSF_Q15,                          /* O    NLSF vector      [ LPC_order ]              */
    const silk_float                *pAR,                               /* I    LPC coefficients [ LPC_order ]              */
    const opus_int                  LPC_order                           /* I    LPC order                                   */
)
{
    opus_int   i;
    opus_int32 a_fix_Q16[ MAX_LPC_ORDER ];

    for( i = 0; i < LPC_order; i++ ) {
        a_fix_Q16[ i ] = silk_float2int( pAR[ i ] * 65536.0f );
    }

    silk_A2NLSF( NLSF_Q15, a_fix_Q16, LPC_order );
}

/* Convert LSF parameters to AR prediction filter coefficients */
void silk_NLSF2A_FLP(
    silk_float                      *pAR,                               /* O    LPC coefficients [ LPC_order ]              */
    const opus_int16                *NLSF_Q15,                          /* I    NLSF vector      [ LPC_order ]              */
    const opus_int                  LPC_order                           /* I    LPC order                                   */
)
{
    opus_int   i;
    opus_int16 a_fix_Q12[ MAX_LPC_ORDER ];

    silk_NLSF2A( a_fix_Q12, NLSF_Q15, LPC_order );

    for( i = 0; i < LPC_order; i++ ) {
        pAR[ i ] = ( silk_float )a_fix_Q12[ i ] * ( 1.0f / 4096.0f );
    }
}

/***********************************************/
/* Floating-point Silk LTP quantiation wrapper */
/***********************************************/
void silk_quant_LTP_gains_FLP(
    silk_float                      B[ MAX_NB_SUBFR * LTP_ORDER ],      /* I/O  (Un-)quantized LTP gains                    */
    opus_int8                       cbk_index[ MAX_NB_SUBFR ],          /* O    Codebook index                              */
    opus_int8                       *periodicity_index,                 /* O    Periodicity index                           */
    opus_int32                      *sum_log_gain_Q7,                   /* I/O  Cumulative max prediction gain  */
    const silk_float                W[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ], /* I    Error weights                        */
    const opus_int                  mu_Q10,                             /* I    Mu value (R/D tradeoff)                     */
    const opus_int                  lowComplexity,                      /* I    Flag for low complexity                     */
    const opus_int                  nb_subfr                            /* I    number of subframes                         */
)
{
    opus_int   i;
    opus_int16 B_Q14[ MAX_NB_SUBFR * LTP_ORDER ];
    opus_int32 W_Q18[ MAX_NB_SUBFR*LTP_ORDER*LTP_ORDER ];

    for( i = 0; i < nb_subfr * LTP_ORDER; i++ ) {
        B_Q14[ i ] = (opus_int16)silk_float2int( B[ i ] * 16384.0f );
    }
    for( i = 0; i < nb_subfr * LTP_ORDER * LTP_ORDER; i++ ) {
        W_Q18[ i ] = (opus_int32)silk_float2int( W[ i ] * 262144.0f );
    }

    silk_quant_LTP_gains( B_Q14, cbk_index, periodicity_index, sum_log_gain_Q7, W_Q18, mu_Q10, lowComplexity, nb_subfr );

    for( i = 0; i < nb_subfr * LTP_ORDER; i++ ) {
        B[ i ] = (silk_float)B_Q14[ i ] * ( 1.0f / 16384.0f );
    }
}
