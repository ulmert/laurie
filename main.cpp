/*
    BSD 3-Clause License

    Copyright (c) 2023, Jacob Ulmert
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "userosc.h"
#include "intervals.h"

//#define CROSSMOD

static float glide[3][4] = {
  {0.05, 1.f, 0.7, 0.5},
  {0.5, 0.05, 0.09, 0.07},
  {1.f, 1.f, 1.f, 1.f}
};

static uint8_t arp[3][4] = {
  {1,64,32,16},
  {1,3,2,1},
  {1,1,1,1}
};
#define ARP_LAST 2

struct voice {
  float t;
  float c;
  float ct;

  float f;

  float env_a0, env_r0, env_t0, env_v0;
  float env_a1, env_r1, env_t1, env_v1;
 
  float lfo_t0, lfo_c0, lfo_v0, lfo_vc0;
  float lfo_t1, lfo_c1, lfo_v1, lfo_vc1;

  float env_cmb0, env_cmb1, env_cmb2;

  float fo;

  float glide_f;

  uint8_t note_pending;
  uint8_t note;

  uint8_t root_note;

  uint8_t retrigger;

};

voice voices[4];

uint8_t arp_index;

uint8_t root_note;

uint8_t scale;

uint8_t xp;
uint8_t yp;

uint8_t voice_mode;
uint8_t arp_mode;

uint8_t pendingTrigger;

bool hold;

inline void triggerX(uint8_t x) {
  if (arp_mode == ARP_LAST) {
    voices[0].retrigger = osc_rand() & 0xff | 1;
  } else {
    voices[0].retrigger = arp[arp_mode][0];
  }
  voices[0].note_pending = x + chords[scale * CHORD_ITEM_SIZE];

  if (voice_mode) {
    if (arp_mode == ARP_LAST) {
      voices[3].retrigger = osc_rand() & 0xff | 1;
    } else {
      voices[3].retrigger = arp[arp_mode][3];
    }
    voices[3].note_pending = x + chords[scale * CHORD_ITEM_SIZE + 3];
  }
}

inline void triggerY(uint8_t y) {
  voices[1].note_pending = y + chords[scale * CHORD_ITEM_SIZE + 1];
  voices[2].note_pending = y + chords[scale * CHORD_ITEM_SIZE + 2];

  if (!voice_mode) {
    voices[3].note_pending = y + chords[scale * CHORD_ITEM_SIZE + 3];
  }

  if (arp_mode == ARP_LAST) {
    voices[1].retrigger = osc_rand() & 0xff | 1;
    voices[2].retrigger = osc_rand() & 0xff | 1;
    if (!voice_mode) {
      voices[3].retrigger = osc_rand() & 0xff | 1;
    }
  } else {
    voices[1].retrigger = arp[arp_mode][1];
    voices[2].retrigger = arp[arp_mode][2];
    if (!voice_mode) {
      voices[3].retrigger = arp[arp_mode][3];
    }
  }
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;

  intervals_init();

  voices[0].t = 0.f;
  voices[1].t = 0.25;
  voices[2].t = 0.5;
  voices[3].t = 0.75;

  uint8_t i = 0;
  while (i < 4) {
    
    voices[i].root_note = 36;

    voices[i].glide_f = glide[0][i];

    voices[i].env_a0 = k_samplerate_recipf * (i + 1);
    voices[i].env_r0 = k_samplerate_recipf * ((i + 1) / 10.f);
    voices[i].env_t0 = 0.f;
    voices[i].env_v0 = 0.f;

    voices[i].env_a1 = k_samplerate_recipf * ((i + 1) * 4.f);
    voices[i].env_r1 = k_samplerate_recipf / ((i + 1) * 2.f);
    voices[i].env_t1 = 0.f;
    voices[i].env_v1 = 0.f;

    voices[i].lfo_c0 = k_samplerate_recipf * (i + 1);
    voices[i].lfo_t0 = 0.f;
    voices[i].lfo_v0 = 0.f;

    voices[i].lfo_c1 = k_samplerate_recipf * (4.f - i);
    voices[i].lfo_t1 = 0.f;
    voices[i].lfo_v1 = 0.f;

    voices[i].fo = (i + 4) * 2;

    voices[i].c = 0.5;
    voices[i].ct = 0.5;
    
    voices[i].retrigger = 1;

    i++;
  }

  xp = 0.5;
  yp = 0.5;

  voice_mode = 0;

  arp_index = 1;
  arp_mode = 0;

  scale = 0;

  root_note = 36;

  hold = false;
  pendingTrigger = 0;
}

inline void update_env(voice *vce, uint32_t frames) {
  if (vce->env_t0 >= 0.5 && vce->env_t0 < 1.f) {
    vce->env_v0 = 2.f - 2 * vce->env_t0;
    vce->env_t0 += (vce->env_r0 * frames);
  } else if (vce->env_t0 < 0.5) {
    vce->env_v0 = (vce->env_t0 / 0.5);
    vce->env_t0 += (vce->env_a0 * frames);
  }

  if (vce->env_t1 >= 0.5 && vce->env_t1 < 1.f) {
    vce->env_v1 = 2.f - 2 * vce->env_t1;
    vce->env_t1 += (vce->env_r1 * frames);
  } else if (vce->env_t1 < 0.5) {
    vce->env_v1 = (vce->env_t1 / 0.5);
    vce->env_t1 += (vce->env_a1 * frames);
  }
}

void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)
{
  uint8_t i = 0;

  while (i < 4) {
    voice *vce = &voices[i];

    if (vce->t > 1.f) {
      vce->t = vce->t - 1.f;

      if (vce->retrigger) {
        vce->retrigger--;
        if (!vce->retrigger) {
          vce->root_note = root_note;
          vce->note = vce->note_pending;

          vce->f = osc_notehzf(getNote(scale, vce->root_note, vce->note));

          vce->ct = k_samplerate_recipf * vce->f;

          if (vce->env_t0 >= 0.5) {
              vce->env_t0 = 0.5 - (vce->env_t0 - 0.5);
              if (vce->env_t0 < 0.f) {
                vce->env_t0 = 0.f;
              }
            }
          if (vce->env_t1 >= 0.5) {
            vce->env_t1 = 0.5 - (vce->env_t1 - 0.5);
            if (vce->env_t1 < 0.f) {
              vce->env_t1 = 0.f;
            }
          }
        }
      }
    }

    update_env(vce, frames);

    vce->lfo_v0 = (osc_sinf(vce->lfo_t0) + 1.f) * (osc_sinf(vce->lfo_t0 * 0.03) + 1.f) * 0.25;
    vce->lfo_v0 = 0.5 + (0.5 * vce->lfo_v0);
    vce->lfo_t0 += (vce->lfo_c0 * frames);

    vce->lfo_v1 = (osc_sinf(vce->lfo_t1) + 1.f) * 0.5;
    vce->lfo_t1 += (vce->lfo_c1 * frames);

    vce->env_cmb0 = vce->lfo_v0 * vce->env_v0 * 0.15;
    vce->env_cmb1 = vce->lfo_v1 * vce->env_v1 * 0.2;
    vce->env_cmb2 = vce->lfo_v0 * vce->env_v0 * 0.3;
   
    i++;
  }

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  float s;

  voice *vce0 =  &voices[0];
  voice *vce1 =  &voices[1];
  voice *vce2 =  &voices[2];
  voice *vce3 =  &voices[3];

  vce0->c -= (vce0->glide_f * (vce0->c - vce0->ct));
  vce1->c -= (vce1->glide_f * (vce1->c - vce1->ct));
  vce2->c -= (vce2->glide_f * (vce2->c - vce2->ct));
  vce3->c -= (vce3->glide_f * (vce3->c - vce3->ct));

  float t0 = vce0->t;
  float t1 = vce1->t;
  float t2 = vce2->t;
  float t3 = vce3->t;

  for (; y != y_e; ) {
#ifdef CROSSMOD    
    s = (osc_parf(t0) + osc_parf(t0 + vce0->env_cmb2)) * vce0->env_cmb0 + (osc_sinf(t1 * vce0->fo + vce0->lfo_v0) * osc_sinf(t0 * 16.f)) * vce0->env_cmb1;
    t0 += vce0->c;

    s += osc_parf(t1 + vce1->env_cmb2) * vce1->env_cmb2 + (osc_sinf(t0 * vce1->fo + vce1->lfo_v0) * osc_sinf(t1 * 16.f)) * vce1->env_cmb1;
    t1 += vce1->c;

    s += (osc_parf(t2) + osc_parf(t2 + vce2->env_cmb2)) * vce2->env_cmb0 + (osc_sinf(t3 * vce2->fo + vce2->lfo_v0) * osc_sinf(t2 * 16.f)) * vce2->env_cmb1;
    t2 += vce2->c;

    s += osc_parf(t3 + vce3->env_cmb2) * vce3->env_cmb2 + (osc_sinf(t3 * vce3->fo + vce3->lfo_v0) * osc_sinf(t2 * 16.f)) * vce3->env_cmb1;
    t3 += vce3->c;
#else
    s = (osc_parf(t0) + osc_parf(t0 + vce0->env_cmb2)) * vce0->env_cmb0 + (osc_sinf(t0 * vce0->fo + vce0->lfo_v0) * osc_sinf(t0 * 16.f)) * vce0->env_cmb1;
    t0 += vce0->c;

    s += osc_parf(t1 + vce1->env_cmb2) * vce1->env_cmb2 + (osc_sinf(t1 * vce1->fo + vce1->lfo_v0) * osc_sinf(t1 * 16.f)) * vce1->env_cmb1;
    t1 += vce1->c;

    s += (osc_parf(t2) + osc_parf(t2 + vce2->env_cmb2)) * vce2->env_cmb0 + (osc_sinf(t2 * vce2->fo + vce2->lfo_v0) * osc_sinf(t2 * 16.f)) * vce2->env_cmb1;
    t2 += vce2->c;

    s += osc_parf(t3 + vce3->env_cmb2) * vce3->env_cmb2 + (osc_sinf(t3 * vce3->fo + vce3->lfo_v0) * osc_sinf(t2 * 16.f)) * vce3->env_cmb1;
    t3 += vce3->c;
#endif

    *(y++) = f32_to_q31(s);
  }

  vce0->t = t0;
  vce1->t = t1;
  vce2->t = t2;
  vce3->t = t3;

}

void OSC_NOTEON(const user_osc_param_t * const params) 
{
  uint8_t note = ((params->pitch)>>8);
  if (note < 48) {
    hold = true;
  } else {
    root_note = note % 24 + 36;

    voices[arp_index].retrigger = 1.f;
    arp_index++;
    if (arp_index > 3) {
      arp_index = 1;
    }
  }
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  if (hold) {
    if (pendingTrigger & 1) {
      triggerX(xp);
      pendingTrigger ^= 1;
    }
    if (pendingTrigger & 2) {
      triggerY(yp);
      pendingTrigger ^= 2;
    }
    hold = false;
  }
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  float valf = param_val_to_f32(value);
  float valp = (float)value / 100.f;
  if (valp < 0.1) {
    valp = 0.1;
  }
  uint8_t intervalSize = getIntervalSize(scale);
  uint8_t i = 0, x, y;
  switch (index) {
    
    case k_user_osc_param_id1: 
      scale = value;
      break;
  
    case k_user_osc_param_id2:
      while(i < 4) {
        voices[i].glide_f = glide[value][i];
        i++;
      }
      break;

    case k_user_osc_param_id3:
      voice_mode = value;
      break;
 
    case k_user_osc_param_id4:
      arp_mode = value;
      break;

    case k_user_osc_param_shape:
      x = (uint8_t)(valf * intervalSize); 
      if (x != xp) {
        if (!hold) {
          triggerX(x);
        } else {
          pendingTrigger |= 1;
        }
        xp = x;
      }
      break;

    case k_user_osc_param_shiftshape:
      y = (uint8_t)(valf * intervalSize); 
      if (y != yp) {
        if (!hold) {
          triggerY(y);
        } else {
          pendingTrigger |= 2;
        }
        yp = y;
      }
      break;

    default:
      break;
  }  
}