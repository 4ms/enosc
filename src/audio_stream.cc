/*
 * audio_stream.c
 *
 * Author: Dan Green (danngreen1@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 *
 * -----------------------------------------------------------------------------
 */

#include "globals.h"

#include "audio_stream.hh"
#include "codec.hh"


#define MAX_CODEC_DAC_VAL 8388607
#define MIN_CODEC_DAC_VAL (-8388608)

int32_t average_L, average_R;

int32_t tri_L=0, tri_R=0;
int32_t tri_L_dir=1, tri_R_dir=1;

void process_audio_block_codec(int32_t *src, int32_t *dst)
{
  uint32_t 	i_sample;
	int32_t		in_L, in_R;
	int32_t 	sum_L=0, sum_R=0;

  if (do_audio_passthrough_test)
	{

		for ( i_sample = 0; i_sample < codec_HT_CHAN_LEN; i_sample++)
		{
			in_L = *src++;
			in_R = *src++;

			sum_L += ((int32_t)(in_L<<8))/256;
			sum_R += ((int32_t)(in_R<<8))/256;

			*dst++ = in_L;
			*dst++ = in_R;
		}

		average_L = ((sum_L/codec_HT_CHAN_LEN) << 8);
		average_R = ((sum_R/codec_HT_CHAN_LEN) << 8);

	} 
	else //triangle wave test
	{

		for ( i_sample = 0; i_sample < codec_HT_CHAN_LEN; i_sample++)
		{
			if (tri_L_dir==1)	tri_L+=0x1000;
			else				tri_L-=0x2000;

			if (tri_L >= MAX_CODEC_DAC_VAL) 	{ tri_L_dir = 1 - tri_L_dir; tri_L = MAX_CODEC_DAC_VAL; }
			if (tri_L <= MIN_CODEC_DAC_VAL) 	{ tri_L_dir = 1 - tri_L_dir; tri_L = MIN_CODEC_DAC_VAL; }

			if (tri_R_dir==1)	tri_R+=0x100000;
			else				tri_R-=0x200000;

			if (tri_R >= MAX_CODEC_DAC_VAL) 	{ tri_R_dir = 1 - tri_R_dir; tri_R = MAX_CODEC_DAC_VAL; }
			if (tri_R <= MIN_CODEC_DAC_VAL) 	{ tri_R_dir = 1 - tri_R_dir; tri_R = MIN_CODEC_DAC_VAL; }

			*dst++ = tri_L;
			*dst++ = tri_R;
		}

	}
}
