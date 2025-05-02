/*
	EPOCH Super Cassette Vision Emulator 'eSCV'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ uPD1771C ]
*/

#include <math.h>
#include "sound.h"
#include "sound_tbl.h"
#include "../upd7801.h"

//#define SOUND_DEBUG

void SOUND::reset()
{
	clear_channel(&tone);
	clear_channel(&noise0);
	clear_channel(&noise1);
	clear_channel(&pcm);
	
	_memset(params, 0, sizeof(params));
	param_cnt = param_ptr = 0;
	noise1_enb = true;
	regist_id = -1;
	cmd_addr = 0;
}

void SOUND::write_data8(uint32 addr, uint32 data)
{
	if(!param_cnt) {
		// new command
		if(data == 0)			// note off
			param_cnt = 1;
		else if(data == 1)		// noise ???
			param_cnt = 10;
		else if(data == 2)		// note on
			param_cnt = 4;
		else if(data == 0x1f)		// pcm
			param_cnt = MAX_PARAM;
		param_ptr = 0;
		cmd_addr = dev->get_prv_pc();
#ifdef SOUND_DEBUG
		emu->out_debug(_T("PC=%4x\tSOUND\t"), cmd_addr);
#endif
	}
#ifdef SOUND_DEBUG
	emu->out_debug("%2x ", data);
#endif
	if(param_cnt) {
		params[param_ptr++] = data;
		if(params[0] == 0x1f) {
			// pcm command
			if(param_ptr == 6) {
				_memset(pcm_table, 0, sizeof(pcm_table));
				pcm_len = pcm.ptr = 0;
			}
			else if(param_ptr >= 7) {
				// 0xfe,0x00 : end of pcm, intf1 must not be done except star speeder
				if(params[param_ptr - 2] == 0xfe && data == 0x00 && cmd_addr != 0xa765)
					param_cnt = 1;
				else
					process_pcm(params[param_ptr - 2]);
			}
		}
		if(--param_cnt) {
			if(regist_id == -1)
				vm->regist_event(this, 0, 100, false, &regist_id);
		}
	}
	if(!param_cnt) {
		// process command
		process_cmd();
#ifdef SOUND_DEBUG
		emu->out_debug("\n");
#endif
	}
}

void SOUND::write_io8(uint32 addr, uint32 data)
{
	// PC3 : L->H
	if(data & 0x8) {
		// note off
		clear_channel(&tone);
		clear_channel(&noise0);
		clear_channel(&noise1);
		
		if(cmd_addr == 0x8402) {
			// y2 monster land
			bool pause = (dev->get_prv_pc() == 0x96c) ? true : false;
			if(pause || !(params[0] == 0x1f && param_ptr > 5)) {
				// terminate command
				if(regist_id != -1)
					vm->cancel_event(regist_id);
				_memset(params, 0, sizeof(params));
				param_cnt = param_ptr = 0;
				
				// terminate pcm when pause
				if(pause)
					clear_channel(&pcm);
			}
//			else if(regist_id == -1)
//				vm->regist_callback(this, 0, 100, false, &regist_id);
		}
		else {
			if(params[0]) {
				// terminate command
				_memset(params, 0, sizeof(params));
				param_cnt = param_ptr = 0;
//				clear_channel(&pcm);
			}
//			if(regist_id == -1)
//				vm->regist_callback(this, 0, 100, false, &regist_id);
		}
#ifdef SOUND_DEBUG
		emu->out_debug("PC3\n");
#endif
	}
}

void SOUND::event_callback(int id)
{
	if(pcm.count && param_ptr == 5 && params[0] == 0x1f && params[1] == 0x04 && params[2] == 0x64) {
		// wait previous pcm
		vm->regist_event(this, 0, 100, false, &regist_id);
		return;
	}
	dev->write_signal(SIG_UPD7801_INTF1, 0xffffffff, 1);
	regist_id = -1;
}

void SOUND::init(int rate)
{
	tone.diff = (int)((SOUND_CLOCK / rate) * 128.0 * 16.0 + 0.5);
	noise0.diff = (int)((NOISE0_CLOCK / rate) * 128.0 * 16.0 + 0.5);
	noise1.diff = (int)((NOISE1_CLOCK / rate) * 128.0 * 16.0 + 0.5);
	pcm.diff = (int)((SOUND_CLOCK / rate) * 128.0 * 16.0 + 0.5);
	
	// create volume table
	double vol = MAX_VOLUME;
	for(int i = 0; i < 32; i++) {
		volume_table[31 - i] = (int)vol;
		vol /= 1.12201845439369;//1.258925412;
	}
	volume_table[0] = 0;
	
	// create detune table
	for(int i = 0; i < 32; i++)
		detune_table[i] = (int)(detune_rate[i] * 256 / 100 + 0.5);
	
	// create noise #1 table
	for(int i = 0; i < 0x80; i++) {
		for(int j = 0; j < 128; j++)
			noise1_table[i][j] = (int)(128.0 - (1.0 - i / 128.0) * j + 0.5);
		for(int j = 0; j < 128; j++)
			noise1_table[i][j + 128] = (int)((1.0 - i / 128.0) * j - 128.0 + 0.5);
	}
	// for lupin3
	for(int i = 0; i < 256; i++)
		noise1_table[128][i] = lupin3_table[i];
	
	// reset device
//	reset();
}

void SOUND::process_cmd()
{
	if(params[0] == 0) {
		// note/noise off
		clear_channel(&tone);
		clear_channel(&noise0);
		clear_channel(&noise1);
		noise1_enb = true;
	}
	else if(params[0] == 1) {
		// preset noise ?
		noise0.period = params[2] << 8;
		noise0.volume = (MAX_NOISE * (params[3] & 0x3f)) >> 6;
		noise0.output = (noise0_table[noise0.ptr] * noise0.volume) >> 8;
		
		if(!params[7] && noise1_enb) {
			noise1.timbre = (params[5] == 8) ? 128 : (params[5] > 0x7f || !params[5]) ? 0x7f : params[5];
			noise1.period = (params[5] == 8) ? 8432 : (params[5] << 8);
			noise1.volume = (MAX_NOISE * 0x2f) >> 7;
			noise1.output = (noise1_table[noise1.timbre][noise1.ptr] * noise1.volume) >> 8;
		}
		else {
			noise1.timbre = (params[5] > 0x7f || !params[5]) ? 0x7f : params[5];
			noise1.period = params[4] << 8;
			noise1.volume = (MAX_NOISE * (params[7] & 0x7f)) >> 7;
			noise1.output = (noise1_table[noise1.timbre][noise1.ptr] * noise1.volume) >> 8;
			noise1_enb = false;
		}
		
		// note off
		clear_channel(&tone);
	}
	else if(params[0] == 2) {
		// note on : $02, timbre, period, volume ?
		tone.timbre = params[1] >> 5;
		tone.period = (params[2] * detune_table[params[1] & 0x1f]);
		tone.volume = volume_table[params[3] & 0x1f];
		tone.output = (timbre_table[tone.timbre][tone.ptr] * tone.volume) >> 8;
		
		// noise off
		clear_channel(&noise0);
		clear_channel(&noise1);
	}
	
	// clear command buffer
	_memset(params, 0, sizeof(params));
	param_cnt = param_ptr = 0;
}

void SOUND::process_pcm(uint8 data)
{
	// add pcm wave to buffer
	pcm_table[pcm_len++] = (data & 0x80) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x40) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x20) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x10) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x08) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x04) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x02) ? MAX_PCM : 0;
	pcm_table[pcm_len++] = (data & 0x01) ? MAX_PCM : 0;
	
	if(!pcm.count) {
		pcm.count = PCM_PERIOD;
		pcm.output = pcm_table[pcm_len - 8];
	}
}

void SOUND::clear_channel(channel_t *ch)
{
	ch->count = 0;
	ch->volume = 0;
	ch->ptr = 0;
	ch->output = 0;
}

void SOUND::mix(int32* buffer, int cnt)
{
	// create sound buffer
	for(int i = 0; i < cnt; i++) {
		int vol = 0;
		// mix pcm
		if(pcm.count) {
			pcm.count -= pcm.diff;
			while(pcm.count <= 0) {
				pcm.count += PCM_PERIOD;
				// low-pass filter for the next sample
				if(++pcm.ptr < pcm_len)
					pcm.output =  (pcm_table[pcm.ptr] + pcm_table[pcm.ptr + 1] + pcm_table[pcm.ptr + 2] + pcm_table[pcm.ptr + 3]) >> 2;
				else {
					pcm.count = 0;
					break;
				}
			}
			vol = pcm.output;
		}
		else {
			// mix tone
			if(tone.volume && tone.period) {
				tone.count -= tone.diff;
				while(tone.count <= 0) {
					tone.count += tone.period;
					tone.ptr = (tone.ptr + 1) & 0xff;
					tone.output = (timbre_table[tone.timbre][tone.ptr] * tone.volume) >> 8;
				}
				vol += tone.output;
			}
			if(noise0.volume && noise0.period) {
				noise0.count -= noise0.diff;
				while(noise0.count <= 0) {
					noise0.count += noise0.period;
					noise0.ptr = (noise0.ptr + 1) & 0xff;
					noise0.output = (noise0_table[noise0.ptr] * noise0.volume) >> 8;
				}
				vol += noise0.output;
			}
			if(noise1.volume && noise1.period) {
				noise1.count -= noise1.diff;
				while(noise1.count <= 0) {
					noise1.count += noise1.period;
					noise1.ptr = (noise1.ptr + 1) & 0xff;
					noise1.output = (noise1_table[noise1.timbre][noise1.ptr] * noise1.volume) >> 8;
				}
				vol += noise1.output;
			}
		}
		buffer[i] = vol;
	}
}
