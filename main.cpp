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
    AND ANY Eg_xpRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//#define LOCALTEST

#ifdef LOCALTEST
#include "localtest.h"
#else
#include "userosc.h"
#endif

#include "intervals.h"
#include "samples.h"

#define N_VOICES 5

#define N_PARTIALS 2

static float glide[N_VOICES] = {0.01, 0.5, 0.95, 0.5, 0.1};

static wave_table_t *g_wave_tables[8] = {
  &wave_stringbox_8, &wave_sinharm, &wave_flutes, &wave_flutes, &wave_aguitar, &wave_voice, &wave_oboe, &wave_stringbox_8,
};

static wave_table_t *g_wave_tables_all[8] = {
  &wave_stringbox_8, &wave_sinharm, &wave_flutes, &wave_aguitar, &wave_oboe, &wave_voice, &wave_snippets, &wave_overtones,
};

static wave_table_t *g_wave_table_configs[N_VOICES][3][2] = {
  { {&wave_aguitar,       &wave_aguitar},
    {&wave_oboe,          &wave_oboe},
    {&wave_stringbox_8,   &wave_stringbox_8}},

  { {&wave_aguitar,       &wave_aguitar},
    {&wave_oboe,          &wave_oboe},
    {&wave_stringbox_8,   &wave_stringbox_8}},
  
  { {&wave_aguitar,       &wave_aguitar},
    {&wave_oboe,          &wave_oboe},
    {&wave_stringbox_8,   &wave_stringbox_8}},

  { {&wave_aguitar,       &wave_aguitar},
    {&wave_oboe,          &wave_sinharm},
    {&wave_stringbox_8,   &wave_stringbox_8}},

  { {&wave_oboe,          &wave_all_low},
    {&wave_flutes,        &wave_all_low},
    {&wave_voice,         &wave_all_low}},

};

#define N_ENV_NODES 4

struct env_node_t {
  float y;
  float td;  
};

static env_node_t g_envelope_configs_a[N_VOICES][2][N_ENV_NODES] = {
  {
    {{0.f,  0.25},     {0.5, 0.5},     {0.1, 1.5},  {0.f,0.f}},
    {{0.f,  0.1},      {0.75, 0.1},    {0.1, 0.5},  {0.f,0.f}}
  },
  {
    {{0.f,  0.01},     {0.5, 0.25},    {0.1, 0.2},  {0.f,0.f}},
    {{0.f,  0.01},     {1.0, 0.1},     {0.1, 0.5},  {0.f,0.f}}
  },
  {
    {{0.f,  0.05},     {0.5, 0.1},     {0.1, 0.2},  {0.f,0.f}},
    {{0.f,  0.05},     {1.0, 0.1},     {0.1, 0.1},  {0.f,0.f}}
  },
  {
    {{0.f,  0.01},     {0.5, 0.25},    {0.1, 0.2},  {0.f,0.f}},
    {{0.f,  0.01},     {1.0, 0.1},     {0.1, 0.5},  {0.f,0.f}}
  },
  {
    {{0.f,  0.2},     {0.25, 0.2},     {0.1, 0.2},  {0.f,0.f}},
    {{0.f,  0.1},     {1.0, 0.1},      {0.1, 0.1},  {0.f,0.f}}
  }
};

static env_node_t g_envelope_configs_b[N_VOICES][2][N_ENV_NODES] = {
  {
    {{0.f,  0.5},     {0.5, 2.5},      {0.1, 3.0},  {0.f,0.f}},
    {{0.f,  0.05},    {0.75, 0.25},    {0.1, 3.0},  {0.f,0.f}}
  },
  {
    {{0.f,  0.05},     {0.5, 0.5},     {0.1, 3.0},  {0.f,0.f}},
    {{0.f,  0.01},     {1.0, 0.5},     {0.1, 2.0},  {0.f,0.f}}
  },
  {
    {{0.f,  0.01},     {0.5, 0.5},     {0.1, 3.0},  {0.f,0.f}},
    {{0.f,  0.05},     {1.0, 0.1},     {0.1, 5.0},  {0.f,0.f}}
  },
  {
    {{0.f,  0.05},     {0.5, 0.5},     {0.1, 3.0},  {0.f,0.f}},
    {{0.f,  0.01},     {1.0, 0.5},     {0.1, 4.0},  {0.f,0.f}}
  },
  {
    {{0.f,  0.2},     {0.25, 0.5},     {0.1, 0.5},  {0.f,0.f}},
    {{0.f,  0.1},     {1.0, 0.5},      {0.1, 0.3},  {0.f,0.f}}
  }
};


struct partial_t {
  wave_table_t *wave_table;

  int8_t *samples_pending_a, *samples_pending_b;

  float xf_t;
  float xf_c;
 
  float env_t;
  float env_v;
  
  float env_df;
  float env_tf;
  float env_vf;

  float env_c;

  float lfo_a;

  uint8_t env_i;

  env_node_t env_node[N_ENV_NODES];
  
  float env_lfo_t;
  float env_lfo_c;

  float env_lfo_v;
  float env_lfo_v_inv;

  uint8_t repeat_count;

  int8_t xf_d;
  int8_t xf_i;

  float gain;

};

#define PROP_KEY_HOLD 1
#define PROP_RANDOM_GLIDE 2
#define PROP_RANDOM_ATTACK 4

struct voice {
  float t;

  float c;
  float ct;

  float env_cmb0, env_cmb1, env_cmb2;

  uint8_t note_pending;
  uint8_t note;

  uint8_t root_note;

  uint8_t props;

  uint16_t retrigger;

  int8_t *samples_a0, *samples_b0, *samples_a1, *samples_b1;

  float xf_v0, xf_v1;

  partial_t partials[2];
  uint8_t n_playing_partials;

  uint8_t idx;

};

voice g_voices[N_VOICES];

static uint8_t g_voice_trigger_index;

static uint8_t g_root_note;

static uint8_t g_scale;

static uint8_t g_xp;
static uint8_t g_yp;

static uint8_t g_voice_mode;

static bool g_key_hold;

static uint8_t g_trigger_mode;

#define TRIGGER_MODE_DEFAULT 0
#define TRIGGER_MODE_KEY 1

static uint8_t g_voice5_pitch_mode, g_voice5_trigger_mode, g_voice5_trigger_index, g_voice5_trigger_count;

static uint8_t g_wavetable_index;

#define VOICE5_PITCH_DEFAULT 0
#define VOICE5_PITCH_ARP 1

#define VOICE5_TRIGGER_DEFAULT 0
#define VOICE5_TRIGGER_RANDOM 1
#define VOICE5_TRIGGER_X 2

#define VOICE5_IDX 4

#ifdef LOCALTEST
void set_wavetables(uint8_t fixed_partial_a, uint8_t fixed_wavetable_a, uint8_t fixed_partial_b, uint8_t fixed_wavetable_b) {

  uint32_t rand = osc_rand(); 
  uint8_t i, j, t;
  i = 0;
  while (i < N_VOICES) {
    j = 0;
    while (j < 3) {
      t = 0;
      while (t < 2) {
        if (j == fixed_partial_a) {
          g_wave_table_configs[i][j][t] = g_wave_tables[(fixed_wavetable_a) % 8]; 
        } else 
        if (j == fixed_partial_b) {
          g_wave_table_configs[i][j][t] = g_wave_tables[(fixed_wavetable_b) % 8];
        } else {
          g_wave_table_configs[i][j][t] = &wave_snippets; 
        }
        t++;
      }
    j++;
    }
    i++;
  }
}
#endif

inline uint32_t set_wavetables_voice(uint8_t i, uint8_t wt_idx, uint32_t rand) {
  
  if (wt_idx) {
    wt_idx--;
    uint8_t j = 0;
    while (j < (N_ENV_NODES - 1)) {
      g_wave_table_configs[i][j][0] = g_wave_tables_all[wt_idx]; 
      g_wave_table_configs[i][j][1] = g_wave_tables_all[wt_idx]; 
      j++;
    }
  } else {
    uint8_t j = 0;
    while (j < (N_ENV_NODES - 1)) {
      g_wave_table_configs[i][j][0] = g_wave_tables[rand % 4]; 
      rand = rand >> 1;
      g_wave_table_configs[i][j][1] = g_wave_tables[rand % 4 + 4]; 
      rand = rand >> 1;
      j++;
    }
  }
  return (rand << 1) | (rand >> 31);
}


inline void update_total_duration(voice *vce) {
  partial_t *partial = &vce->partials[0];
  partial->env_df += (partial->env_node[0].td + partial->env_node[1].td + partial->env_node[2].td);

  partial = &vce->partials[1];
  partial->env_df += (partial->env_node[0].td + partial->env_node[1].td + partial->env_node[2].td);
}

inline uint32_t set_gain(uint8_t i, uint32_t rand) {
  voice *vce = &g_voices[i]; 

  const float x = (float)(rand & 255) / 255.f * 0.25;

  vce->partials[0].env_node[1].y = 0.75 - x;
  vce->partials[1].env_node[1].y = 0.75 + x;

  return (rand << 1) | (rand >> 31);
}

void init_envelopes() {
  uint8_t i, j, t;
  
  i = 0;
  while (i < N_VOICES) {
    voice *vce = &g_voices[i]; 
    j = 0;
    while (j < N_PARTIALS) {
      partial_t *partial = &vce->partials[j];
      
      partial->env_df = 0.f;

      t = 0;
      while (t < (N_ENV_NODES - 1)) {
        partial->env_node[t].y = g_envelope_configs_a[i][j][t].y;
        partial->env_node[t].td = g_envelope_configs_a[i][j][t].td;

        partial->env_df += partial->env_node[t].td;
        t++;
      }

      partial->env_t = 0.f;
      partial->env_v = 0.f;
      partial->env_vf = 0.f;
      partial->env_tf = 0.f;
      j++;
    }
    i++;
  }
}

inline uint32_t set_envelopes_voice(uint8_t i,uint32_t rand) {
  uint8_t j = 0, t;
  
  float x = (float)(rand & 255) / 255.f;

  voice *vce = &g_voices[i]; 
  while (j < N_PARTIALS) {
    partial_t *partial = &vce->partials[j];
    
    partial->env_df = 0.f;

    t = 0;
    while (t < N_ENV_NODES) {
      partial->env_node[t].y = g_envelope_configs_a[i][j][t].y * (1.f - x) + g_envelope_configs_b[i][j][t].y * x;
      partial->env_node[t].td = g_envelope_configs_a[i][j][t].td * (1.f - x) + g_envelope_configs_b[i][j][t].td * x;
      partial->env_df += partial->env_node[t].td;
      t++;
    }

    j++;
  }
  return (rand << 1) | (rand >> 31);
}

void set_env_lfo(bool reset_timers, float y) {
  uint8_t i, j, t;
  
  i = 0;
  while (i < N_VOICES) {
    voice *vce = &g_voices[i]; 
    j = 0;
    while (j < N_PARTIALS) {
      partial_t *partial = &vce->partials[j];

      partial->env_lfo_c = y * (0.005 + 0.001 * i) + 0.005;

      if (reset_timers) {
        partial->env_t = 0.f;
        partial->env_v = 0.f;
      }
      j++;
    }
    i++;
  }
}

inline void trigger_x(uint8_t x, uint8_t delay) {
   
  if (x <= 127) {
    g_voices[0].retrigger = 1;
    g_voices[0].note_pending = x + chords[g_scale * CHORD_ITEM_SIZE];

    if (g_voice_mode) {
      g_voices[3].retrigger = 1 + delay;
      g_voices[3].note_pending = x + chords[g_scale * CHORD_ITEM_SIZE + 3];
    }

    if (g_voice5_trigger_mode == VOICE5_TRIGGER_X) {
      g_voices[4].retrigger = 16;
      g_voices[4].note_pending = x + chords[g_scale * CHORD_ITEM_SIZE];
    }
  }
}

inline void trigger_voice(uint8_t i, uint8_t x, uint8_t delay) {
  g_voices[i].note_pending = x + chords[g_scale * CHORD_ITEM_SIZE + (i + 1)];  
  g_voices[i].retrigger = delay;
}

inline void trigger_y(uint8_t y, uint8_t delay) {

  g_voices[1].note_pending = y + chords[g_scale * CHORD_ITEM_SIZE + 1];
  g_voices[2].note_pending = y + chords[g_scale * CHORD_ITEM_SIZE + 2];

  if (!g_voice_mode) {
    g_voices[3].note_pending = y + chords[g_scale * CHORD_ITEM_SIZE + 3];
  }

  g_voices[1].retrigger = 1;    
  g_voices[2].retrigger = g_voices[1].retrigger + delay;
  if (!g_voice_mode) {
    g_voices[3].retrigger = g_voices[2].retrigger + delay;
  }
}

void init_voices(bool reset_timers) {
  uint8_t i = 0;
  uint8_t t;

  uint32_t rand = osc_rand();

  while (i < N_VOICES) {
    voice *vce = &g_voices[i]; 

    t = 0;
    while (t < N_PARTIALS) {
      vce->partials[t].env_v = 0.f;
      vce->partials[t].env_i = 127;

      if (reset_timers) {
        vce->partials[t].env_lfo_t = 0.f;
        vce->partials[t].env_lfo_v = 0.f;
        vce->partials[t].env_lfo_c = 0.f;

        vce->partials[t].xf_t = 0.f;
        vce->partials[t].xf_c = 0.f;
        vce->partials[t].xf_d = 0;
        vce->partials[t].xf_i = 0;
    
        vce->partials[t].lfo_a = 1.f;
      }
      t++;
    }

    vce->idx = i;

    vce->partials[0].wave_table = g_wave_table_configs[i][0][0];

    vce->samples_a0 = vce->partials[0].wave_table->samples;
    vce->samples_b0 = vce->samples_a0;
    vce->partials[0].samples_pending_a = vce->samples_a0;
    vce->partials[0].samples_pending_b = vce->samples_b0;
   
    vce->xf_v0 = 0.f;

    vce->partials[1].wave_table = g_wave_table_configs[i][0][1];

    vce->samples_a1 = vce->partials[1].wave_table->samples;
    vce->samples_b1 = vce->samples_a1;
    vce->partials[1].samples_pending_a = vce->samples_a1;
    vce->partials[1].samples_pending_b = vce->samples_b1;
    
    vce->xf_v1 = 0.f;

    vce->root_note = g_root_note;

    vce->n_playing_partials = 0;

    if (reset_timers) {
      vce->c = 0.f;
      vce->ct = 0.f;
      vce->t = 0.f;
    }

    vce->retrigger = 0;

    vce->props = 0;
    if (i == VOICE5_IDX) {
      vce->props = PROP_KEY_HOLD;  
    } 
    if (i != 0) {
      vce->props |= (PROP_RANDOM_GLIDE | PROP_RANDOM_ATTACK);
    }

    i++;
  }
}

void set_voice5_mode() { 
  switch(g_trigger_mode) {
    case 0:
      g_voice5_pitch_mode = VOICE5_PITCH_DEFAULT;
      g_voice5_trigger_mode = VOICE5_TRIGGER_DEFAULT;
      break;
    
    case 1:
      g_voice5_pitch_mode = VOICE5_PITCH_DEFAULT;
      g_voice5_trigger_mode = VOICE5_TRIGGER_RANDOM;
      break;

    case 2:
      g_voice5_pitch_mode = VOICE5_PITCH_ARP;
      g_voice5_trigger_mode = VOICE5_TRIGGER_RANDOM;
      break;

    case 3:
      g_voice5_pitch_mode = VOICE5_PITCH_DEFAULT;
      g_voice5_trigger_mode = VOICE5_TRIGGER_X;
      break;

    default:
      break;
  }
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  #ifndef LOCALTEST
  (void)platform;
  (void)api;
  #endif

  intervals_init();

  g_xp = 0;
  g_yp = 0;

  g_voice_mode = 1;

  g_scale = 0;
  g_root_note = 36;

  g_key_hold = false;

  g_voice_trigger_index = 0;

  g_voice5_pitch_mode = VOICE5_PITCH_ARP;
  g_voice5_trigger_mode = VOICE5_TRIGGER_DEFAULT;
  g_voice5_trigger_index = 0;
  g_voice5_trigger_count = 0;

  g_wavetable_index = 0;

  g_trigger_mode = 0;

  set_voice5_mode();

  init_voices(true);
 
  init_envelopes();
//  set_env_lfo(true, 0.f);
}

inline float get_mixed_wave_sample_noise(uint8_t r, int8_t *wave_a, int8_t *wave_b, float x, float y) {
    r = r & 0x8;
    
    const float p = x - (uint32_t)x;
    
    const float x0f = p * 256.f;
    const uint32_t x0p = (uint32_t)x0f;

    const uint32_t x0 = x0p & 0xff;
    const uint32_t x1 = (x0 + 1) & 0xff;
    
    const float y0 = linintf(x0f - x0p, *(wave_a + x0), *(wave_a + x1)) * (1.f - y);
    const float y1 = linintf(x0f - x0p, *(wave_b + x0), *(wave_b + x1)) * y;

    return (y0 + y1 + r);
}

inline float get_mixed_wave_sample(int8_t *wave_a, int8_t *wave_b, float x, float y) {
    
    const float p = x - (uint32_t)x;
    
    const float x0f = p * 256.f;
    const uint32_t x0p = (uint32_t)x0f;

    const uint32_t x0 = x0p & 0xff;
    const uint32_t x1 = (x0 + 1) & 0xff;
    
    const float y0 = linintf(x0f - x0p, *(wave_a + x0), *(wave_a + x1)) * (1.f - y);
    const float y1 = linintf(x0f - x0p, *(wave_b + x0), *(wave_b + x1)) * y;
   
    return (y0 + y1);
}

inline uint32_t update_wavetables(voice *vce, uint32_t frames, uint32_t rand) {
  uint8_t t = 0;
  while (t < N_PARTIALS) {
    partial_t *partial = &vce->partials[t];
    if (partial->xf_t >= 1.f) {

      const uint32_t xf_t = (uint32_t)partial->xf_t;
      partial->xf_t = (partial->xf_t - xf_t); 

      if (t == 0) {
        vce->samples_a0 = vce->samples_b0;
        if (!partial->xf_d) {
          vce->samples_b0 = partial->wave_table->samples + (rand & partial->wave_table->mask) * 256;
        } else {
          partial->xf_i += partial->xf_d;
          if (partial->xf_i >= partial->wave_table->n_fragments) {
            partial->xf_i = partial->wave_table->n_fragments - 1;
            partial->xf_d = -1;
          } else if (partial->xf_i < 0) {
            partial->xf_i = 0;
            partial->xf_d = 1;
          }
          vce->samples_b0 = partial->wave_table->samples + partial->xf_i * 256;
        }
        rand = (rand << 1) | (rand >> 31);
      }
      if (t == 1) {
        vce->samples_a1 = vce->samples_b1;
        if (!partial->xf_d) {
          vce->samples_b1 = partial->wave_table->samples + (rand & partial->wave_table->mask) * 256;
        } else {
          partial->xf_i += partial->xf_d;
          if (partial->xf_i >= partial->wave_table->n_fragments) {
            partial->xf_i = partial->wave_table->n_fragments - 1;
            partial->xf_d = -1;
          } else if (partial->xf_i < 0) {
            partial->xf_i = 0;
            partial->xf_d = 1;
          }
          vce->samples_b1 = partial->wave_table->samples + partial->xf_i * 256;
        }
        rand = (rand << 1) | (rand >> 31);
      }
     
    }
    t++;
  }
  return rand;
}

inline uint32_t trigger_partials(voice *vce, uint32_t frames, uint32_t rand) {
  uint8_t t = 0;
  while (t < N_PARTIALS) {
    partial_t *partial = &vce->partials[t];

    partial->gain = 1.f;

    partial->env_tf = 0.f;
    partial->env_t = 0.f;
    partial->env_c = (k_samplerate_recipf * frames);
    partial->env_lfo_c = (k_samplerate_recipf * ((float)(rand & 255) / 255.f * 450.f + 50.f));
    partial->env_i = 0;

    partial->env_node[0].y = partial->env_v;

    if (rand & 4) {
      partial->lfo_a = 1.f * ((rand & 1) + 1) * k_samplerate_recipf;
    } else {
      partial->lfo_a = 1.f * k_samplerate_recipf;
    }

    float x = ((rand & 7) + 1);
    partial->xf_c = partial->env_c * x;

    if (rand & 1) {
      partial->xf_d = 0;
    } else if (rand & 2) {
      partial->xf_d = 1;
    }

    if (rand & 1 && vce->idx != VOICE5_IDX) {
      partial->repeat_count = (rand & 3);
    }

    rand = (rand << 1) | (rand >> 31);

    vce->n_playing_partials++;  

    t++;
  }
  return rand;
}

inline void update_partials(voice *vce, uint32_t frames) { 

  uint8_t t = 0;
  while (t < N_PARTIALS) {
    partial_t *partial = &vce->partials[t];

    if (partial->env_i < (N_ENV_NODES - 1)) {
      if (partial->env_t >= partial->env_node[partial->env_i].td) {
        if (g_key_hold && (vce->props & PROP_KEY_HOLD)) {
          partial->env_t = partial->env_node[partial->env_i].td;  
          partial->env_tf = partial->env_t;
        } else {
          partial->env_i++;
          if (partial->env_i >= (N_ENV_NODES - 1) && vce->n_playing_partials > 0) {
            vce->n_playing_partials--;
          } else {
            partial->wave_table = g_wave_table_configs[vce->idx][0][t];
          }
          partial->env_t = 0.f;
        }
      }
    }

    if (partial->env_i < (N_ENV_NODES - 1)) {

      partial->env_vf = 1.f - (partial->env_tf / (partial->env_df));
      if (partial->env_vf < 0) {
        partial->env_vf = 0;
      }

      float env_lfo_t = partial->env_lfo_t;
      env_lfo_t = env_lfo_t - (uint32_t)env_lfo_t;
      partial->env_lfo_t = env_lfo_t + partial->env_lfo_c;

      partial->env_lfo_v_inv = 1.f - (osc_sinf(env_lfo_t) + 1.f) * 0.5;
      partial->env_lfo_v = partial->env_lfo_v_inv * (1.f - partial->env_vf);

      const float f = partial->env_t / partial->env_node[partial->env_i].td;
      partial->env_v = (partial->env_node[partial->env_i].y * (1.f - f) + partial->env_node[partial->env_i + 1].y * f) * partial->gain;    
      
      partial->env_t +=  partial->env_c;
      partial->env_tf += partial->env_c;

    } else {
      if (partial->repeat_count) {
        partial->repeat_count--;

        if (partial->repeat_count) {
          partial->lfo_a *= 10.f;
        }

        partial->xf_d *= -1;

        partial->gain *= 0.5;

        partial->env_t = 0.f;
        partial->env_tf = 0.f;

        partial->env_i = 0;

        partial->env_c = partial->env_c * 0.5;
        partial->xf_c = partial->env_c;

        vce->ct = vce->ct + (k_samplerate_recipf * 0.5);

        partial->env_node[0].y = 0.f;
        partial->env_node[0].td *= 2.f;

      } else {
        partial->env_v = 0.f;
        partial->wave_table = g_wave_table_configs[vce->idx][0][t];
      }
    }

    if (partial->xf_t >= 1.f) {
      partial->xf_t = 1.f;  
    } else {        
      vce->xf_v0 = vce->partials[0].xf_t / 1.f;
      vce->xf_v1 = vce->partials[1].xf_t / 1.f;
      partial->xf_t += partial->xf_c;
    }
    t++;
  }
}

#ifdef LOCALTEST
void OSC_CYCLE(const user_osc_param_t * const params, float *yn, const uint32_t frames)
#else
void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)
#endif
{
  uint8_t i = 0;

  uint32_t rand = osc_rand();

  while (i < N_VOICES) {
    
    voice *vce = &g_voices[i]; 

    if (vce->t > 1.f || !vce->n_playing_partials) {
      vce->t = (vce->t - (uint32_t)vce->t); 

      if (rand & 8) {
        rand = update_wavetables(vce, frames, rand);
      }

      if (vce->retrigger) {
        vce->retrigger--;
        if (!vce->retrigger) {

          rand = set_envelopes_voice(i, rand);

          rand = set_gain(i, rand);

          rand = trigger_partials(vce, frames, rand);  

          vce->root_note = g_root_note;
          vce->note = vce->note_pending;        

          if (i == VOICE5_IDX) {
            vce->ct = k_samplerate_recipf * osc_notehzf(getNote(g_scale, vce->root_note - 12, vce->note));
          } else {
            vce->ct = k_samplerate_recipf * osc_notehzf(getNote(g_scale, vce->root_note, vce->note));
            rand = set_wavetables_voice(i, g_wavetable_index, rand);
          }

          if (g_voice_mode && i == 0 || i == VOICE5_IDX) {
            vce->c = vce->ct;  
          } else if (vce->props & PROP_RANDOM_GLIDE) {
            glide[i] =  (float)(rand & 255) / 255.f * 0.9 + 0.1;
            if (rand & 1) {
              vce->c = vce->ct + (k_samplerate_recipf * (float)(rand & 0x1f));
            }  else if (rand & 2) {
              vce->c = vce->ct - (k_samplerate_recipf * (float)(rand & 0x1f));
            } else {
              vce->c = vce->ct;
            }
            rand = (rand << 1) | (rand >> 31);
          }
        }
      }
    }

    update_partials(vce, frames);

    vce->env_cmb0 = vce->partials[0].env_v + vce->partials[0].env_lfo_v * vce->partials[0].env_v;
    vce->env_cmb1 = vce->partials[1].env_v + vce->partials[1].env_lfo_v * vce->partials[1].env_v;
    vce->env_cmb2 = vce->partials[0].env_v;

    vce->c -= (glide[i] * (vce->c - vce->ct));

    i++;
  }

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  float s;

  voice *vce0 = &g_voices[0];
  voice *vce1 = &g_voices[1];
  voice *vce2 = &g_voices[2];
  voice *vce3 = &g_voices[3];
  voice *vce4 = &g_voices[4];

  float t0 = vce0->t;
  float t1 = vce1->t;
  float t2 = vce2->t;
  float t3 = vce3->t;
  float t4 = vce4->t;

  const float c0 = vce0->c + (vce0->partials[0].env_vf * vce0->partials[0].lfo_a * (vce0->partials[0].env_lfo_v - 0.5));
  const float c1 = vce1->c + (vce1->partials[1].env_vf * vce1->partials[0].lfo_a * (vce1->partials[0].env_lfo_v_inv - 0.5));
  const float c2 = vce2->c + (vce2->partials[1].env_vf * vce2->partials[0].lfo_a * (vce2->partials[0].env_lfo_v_inv - 0.5));
  const float c3 = vce3->c + (vce3->partials[1].env_vf * vce3->partials[0].lfo_a * (vce3->partials[0].env_lfo_v - 0.5));
  const float c4 = vce4->c + (vce4->partials[0].env_vf * vce4->partials[0].lfo_a * (vce4->partials[0].env_lfo_v_inv - 0.5));

  for (; y != y_e; ) {
    s = get_mixed_wave_sample(vce0->samples_a0, vce0->samples_b0, t0, vce0->xf_v0) * vce0->env_cmb0 + get_mixed_wave_sample(vce0->samples_a1, vce0->samples_b1, t0 * 2, vce0->xf_v1) * vce0->env_cmb1;
    t0 += c0;
  
    s += get_mixed_wave_sample(vce1->samples_a0, vce1->samples_b0, t1 * 1, vce1->xf_v0) * vce1->env_cmb0 + get_mixed_wave_sample(vce1->samples_a1, vce1->samples_b1, t1 * 2, vce1->xf_v1) * vce1->env_cmb1;
    t1 += c1;

    s += get_mixed_wave_sample(vce2->samples_a0, vce2->samples_b0, t2 * 2, vce2->xf_v0) * vce2->env_cmb0 + get_mixed_wave_sample(vce2->samples_a1, vce2->samples_b1, t2 * 4, vce2->xf_v1) * vce2->env_cmb1;
    t2 += c2;

    s += get_mixed_wave_sample(vce3->samples_a0, vce3->samples_b0, t3 * 1, vce3->xf_v0) * vce3->env_cmb0 + get_mixed_wave_sample(vce3->samples_a1, vce3->samples_b1, t3 * 4, vce3->xf_v1) * vce3->env_cmb1;
    t3 += c3;

    s += get_mixed_wave_sample(vce4->samples_a0, vce4->samples_b0, t4, vce4->xf_v0) * vce4->env_cmb2;
    t4 += c4;

    s = s / 512;

    *(y++) = f32_to_q31(s);
  }

  vce0->t = t0;
  vce1->t = t1;
  vce2->t = t2;
  vce3->t = t3;
  vce4->t = t4;
}

void trigger_voice5() {

  if (g_voice5_trigger_mode != VOICE5_TRIGGER_X) {
    uint32_t rand = osc_rand();

    switch (g_voice5_pitch_mode) {
      case VOICE5_PITCH_ARP:
        if (g_voice5_trigger_count) {
          g_voice5_trigger_index++;
          if (g_voice5_trigger_index > 3) {
            g_voice5_trigger_index = 1;  
          }
          g_voices[4].note_pending = g_xp + chords[g_scale * CHORD_ITEM_SIZE + g_voice5_trigger_index];  
        }
        break;

      default:
        g_voices[4].note_pending = g_xp + chords[g_scale * CHORD_ITEM_SIZE];  
        break;
      
    }

    switch (g_voice5_trigger_mode) {
      case VOICE5_TRIGGER_RANDOM:
        if (g_voice5_trigger_count) {
          if (rand & 1) {
            if (!g_key_hold) {
              g_voices[4].retrigger = 2;
              g_key_hold ^= true;
            }
          }
        }
        break;

      default:
        g_key_hold = true;
        g_voices[4].retrigger = 1;
        break;  
    }
    g_voice5_trigger_count ^= 1;
  }
}

void OSC_NOTEON(const user_osc_param_t * const params) 
{
  uint8_t note = ((params->pitch)>>8);
  if (note < 48) {
    g_voice_mode ^= 1;
  } 

  if (g_trigger_mode > 0) {
    uint8_t i = osc_rand() & 2 + 1;
    uint8_t delay = 1;
    while (i) {
      if (g_voice_trigger_index < 4) {
        trigger_voice(g_voice_trigger_index, g_yp, delay);
        delay += 8;
        g_voice_trigger_index++;
      } else {
        break;
      }
      i--;
    }
  }

  trigger_voice5();
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  uint32_t rand = osc_rand();

  switch (g_voice5_trigger_mode) {
    case VOICE5_TRIGGER_RANDOM:
      if (g_key_hold && rand & 1) {
        g_key_hold ^= true;
      }
    default:
      g_key_hold = false;
      break;
  }
}

#ifndef LOCALTEST
void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  float valf = param_val_to_f32(value);
  int8_t direction = 0;
  uint8_t intervalSize = getIntervalSize(g_scale);
  uint8_t i = 0, x, y;

  switch (index) {
    
    case k_user_osc_param_id1: 
      g_scale = value;
      break;
  
    case k_user_osc_param_id2:
      g_root_note = value + 24;
      break;

    case k_user_osc_param_id3:
      g_trigger_mode = value % 4;
      set_voice5_mode();
      break;
 
    case k_user_osc_param_id4:
      g_wavetable_index = value;
      break;

    case k_user_osc_param_shape:
      if (valf >= 0 && valf <= 1.0) {
        x = (uint8_t)(valf * intervalSize); 
        if (x != g_xp) {
          trigger_x(x, osc_rand() & 0xff);
          g_xp = x;
        }
      }
      break;

    case k_user_osc_param_shiftshape:
      if (valf >= 0 && valf <= 1.0) {
        y = (uint8_t)(valf * intervalSize); 
        if (y != g_yp) {
          g_voice_trigger_index = 1;
          if (g_trigger_mode < 1) {
            trigger_y(y, 16);
          }
          g_yp = y;
        }
      }
      break;

    default:
      break;
  }  
}
#endif

#ifdef LOCALTEST
int main() {
  user_osc_param_t params;

  float yn[64];

  FILE *fp = fopen("wave.bin", "wb");
  if (fp == NULL){
      exit(1);
  }

  OSC_INIT(0,0);

  uint8_t interval = getIntervalSize(g_scale);
  
  uint16_t i = 0;
  uint16_t i_max = 256;


  uint8_t i_notetrigger = 0;

  g_voice_mode = 1;
  g_trigger_mode = 1;

  uint8_t pitch = 48;

  g_xp = (uint8_t)(0.75 * interval); 

  trigger_x(0,1);

  while(i < i_max) {
    int t = 0;
    while (t < 128) {
      OSC_CYCLE(NULL, &yn[0], 64);
      fwrite(&yn, 4, 64, fp);
      fflush(fp);
      t++;
    }
  
    float v = interval * (sin((float)i / (float)i_max)+ 1.f) * 0.5;
    
    params.pitch = pitch << 8;
    if (i_notetrigger & 1) {
      OSC_NOTEON(&params);
    }

    if (i_notetrigger & 2) {
      OSC_NOTEOFF(&params);

      if (i_notetrigger > 4) {
        g_yp = v + chords[g_scale * CHORD_ITEM_SIZE + 1];
        g_voice_trigger_index = 1;

        i_notetrigger = 0;

        pitch++;
        if (pitch > 49) {
          pitch = 47;
        }

        g_wavetable_index++;
        if (g_wavetable_index > 8) {
          g_wavetable_index = 1;
        }
      }
    }
    i_notetrigger++;

    i++;
  }
  fclose(fp);
}
#endif
