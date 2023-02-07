/*
 * Allwinner SoCs hdmi driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "hdmi_core.h"

static s32 is_hdmi;
static s32 is_yuv;
u32 is_exp;
u32 rgb_only;
static u8 EDID_Buf[HDMI_EDID_LEN];
u8 Device_Support_VIC[516];

static u8 exp0[16] = {

	0x36, 0x74, 0x4d, 0x53, 0x74, 0x61, 0x72, 0x20,
	0x44, 0x65, 0x6d, 0x6f, 0x0a, 0x20, 0x20, 0x38
};

static u8 exp1[16] = {

	0x2d, 0xee, 0x4b, 0x4f, 0x4e, 0x41, 0x4b, 0x20,
	0x54, 0x56, 0x0a, 0x20, 0x20, 0x20, 0x20, 0xa5
};

static void ddc_init(void)
{

}

static void edid_read_data(u8 block, u8 *buf)
{
	u8 i;
	u8 *pbuf = buf + 128*block;
	u8 offset = (block&0x01) ? 128:0;

	bsp_hdmi_ddc_read(Explicit_Offset_Address_E_DDC_Read, block>>1,
			offset, 128, pbuf);

	/* ////////////////////////////////////////////////////////////////// */
	hdmi_inf("Sink : EDID bank %d:\n", block);

	hdmi_inf(
	    " 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");
	hdmi_inf(
	    " =============================================================\n");

	for (i = 0; i < 8; i++) {
		hdmi_inf(" %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  "
			 "%2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  "
			 "%2.2x  %2.2x\n",
			 pbuf[i * 16 + 0], pbuf[i * 16 + 1], pbuf[i * 16 + 2],
			 pbuf[i * 16 + 3], pbuf[i * 16 + 4], pbuf[i * 16 + 5],
			 pbuf[i * 16 + 6], pbuf[i * 16 + 7], pbuf[i * 16 + 8],
			 pbuf[i * 16 + 9], pbuf[i * 16 + 10], pbuf[i * 16 + 11],
			 pbuf[i * 16 + 12], pbuf[i * 16 + 13],
			 pbuf[i * 16 + 14], pbuf[i * 16 + 15]);
	}
	hdmi_inf(
	    " =============================================================\n");
}

/* /////////////////////////////////////////////////////////////////// */
/* hdmi_edid_parse() */
/* Check EDID check sum and EDID 1.3 extended segment. */
/* /////////////////////////////////////////////////////////////////// */
static s32 edid_check_sum(u8 block, u8 *buf)
{
	s32 i = 0, CheckSum = 0;
	u8 *pbuf = buf + 128*block;

	for (i = 0, CheckSum = 0 ; i < 128 ; i++) {
		CheckSum += pbuf[i];
		CheckSum &= 0xFF;
	}
	if (CheckSum != 0) {
		hdmi_inf("EDID block %d checksum error\n", block);
		return -1;
	}

	return 0;
}
static s32 edid_check_header(u8 *pbuf)
{
	if (pbuf[0] != 0x00 ||
		pbuf[1] != 0xFF ||
		pbuf[2] != 0xFF ||
		pbuf[3] != 0xFF ||
		pbuf[4] != 0xFF ||
		pbuf[5] != 0xFF ||
		pbuf[6] != 0xFF ||
		pbuf[7] != 0x00) {
		hdmi_inf("EDID block0 header error\n");
		return -1;
	}

	return 0;
}

static s32 edid_check_version(u8 *pbuf)
{
	hdmi_inf("EDID version: %d.%d ", pbuf[0x12], pbuf[0x13]);
	if ((pbuf[0x12] != 0x01) || (pbuf[0x13] != 0x03)) {
		hdmi_inf("Unsupport EDID format,EDID parsing exit\n");
		return -1;
	}

	return 0;
}

static s32 edid_parse_dtd_block(u8 *pbuf)
{
	/*Hsync_offset,Hsync_plus,Vsync_offset,Vsync_plus,H_image_size,
	 *V_image_size,H_Border,V_Border,
	 */
	u32	pclk, sizex, Hblanking, sizey, Vblanking;
	u32	pixels_total, frame_rate;

	pclk		= ((u32)pbuf[1]	<< 8) + pbuf[0];
	sizex		= (((u32)pbuf[4]	<< 4) & 0x0f00) + pbuf[2];
	Hblanking	= (((u32)pbuf[4]	<< 8) & 0x0f00) + pbuf[3];
	sizey		= (((u32)pbuf[7]	<< 4) & 0x0f00) + pbuf[5];
	Vblanking	= (((u32)pbuf[7]	<< 8) & 0x0f00) + pbuf[6];
/* Hsync_offset= (((u32)pbuf[11] << 2) & 0x0300) + pbuf[8]; */
/* Hsync_plus	= (((u32)pbuf[11] << 4) & 0x0300) + pbuf[9]; */
/* Vsync_offset= (((u32)pbuf[11] << 2) & 0x0030) + (pbuf[10] >> 4); */
/* Vsync_plus	= (((u32)pbuf[11] << 4) & 0x0030) + (pbuf[8] & 0x0f); */
/* H_image_size= (((u32)pbuf[14] << 4) & 0x0f00) + pbuf[12]; */
/* V_image_size= (((u32)pbuf[14] << 8) & 0x0f00) + pbuf[13]; */
/* H_Border	=  pbuf[15]; */
/* V_Border	=  pbuf[16]; */

	pixels_total = (sizex + Hblanking) * (sizey + Vblanking);

	if ((pbuf[0] == 0) && (pbuf[1] == 0) && (pbuf[2] == 0))
		return 0;

	if (pixels_total == 0)
		return 0;
	frame_rate = (pclk * 10000) / pixels_total;

	if ((frame_rate == 59) || (frame_rate == 60))	{
		if ((sizex == 720) && (sizey == 240))
			Device_Support_VIC[HDMI1440_480I] = 1;
		if ((sizex == 720) && (sizey == 480))
			/* Device_Support_VIC[HDMI480P] = 1; */
		if ((sizex == 1280) && (sizey == 720))
			Device_Support_VIC[HDMI720P_60] = 1;
		if ((sizex == 1920) && (sizey == 540))
			Device_Support_VIC[HDMI1080I_60] = 1;
		if ((sizex == 1920) && (sizey == 1080))
			Device_Support_VIC[HDMI1080P_60] = 1;
		if ((sizex == 1280) && (sizey == 1024))
			Device_Support_VIC[HDMI1280_1024P_60] = 1;
	} else if ((frame_rate == 49) || (frame_rate == 50)) {
		if ((sizex == 720) && (sizey == 288))
			Device_Support_VIC[HDMI1440_576I] = 1;
		if ((sizex == 720) && (sizey == 576))
			Device_Support_VIC[HDMI576P] = 1;
		if ((sizex == 1280) && (sizey == 720))
			Device_Support_VIC[HDMI720P_50] = 1;
		if ((sizex == 1920) && (sizey == 540))
			Device_Support_VIC[HDMI1080I_50] = 1;
		if ((sizex == 1920) && (sizey == 1080))
			Device_Support_VIC[HDMI1080P_50] = 1;
	} else if ((frame_rate == 23) || (frame_rate == 24)) {
		if ((sizex == 1920) && (sizey == 1080))
			Device_Support_VIC[HDMI1080P_24] = 1;
	}
	hdmi_inf("PCLK=%d\tXsize=%d\tYsize=%d\tFrame_rate=%d\n",
	pclk*10000, sizex, sizey, frame_rate);

	return 0;
}

static s32 edid_parse_videodata_block(u8 *pbuf, u8 size)
{
	int i = 0;
	u8 vic_data = 0;

	while (i < size) {
		vic_data = pbuf[i] & 0x7f;
		if ((vic_data == 93) ||
		    (vic_data == 94) ||
		    (vic_data == 95))
			Device_Support_VIC[96 - vic_data + 0x100] = 1;
		else if (vic_data == 98)
			Device_Support_VIC[0x104] = 1;
		else
			Device_Support_VIC[vic_data] = 1;

		if (pbuf[i] & 0x80)
			hdmi_inf
	("edid_parse_videodata_block: VIC %d(native) support\n", pbuf[i]&0x7f);
		else
			hdmi_inf("edid_parse_videodata_block: VIC %d support\n",
				pbuf[i]);

		i++;
	}

	return 0;
}

static s32 edid_parse_audiodata_block(u8 *pbuf, u8 size)
{
	u8 sum = 0;

	while (sum < size) {
		if ((pbuf[sum]&0xf8) == 0x08) {
			hdmi_inf("edid_parse_audiodata_block: max channel=%d\n",
				 (pbuf[sum] & 0x7) + 1);
			hdmi_inf(
			    "edid_parse_audiodata_block: SampleRate code=%x\n",
			    pbuf[sum + 1]);
			hdmi_inf(
			    "edid_parse_audiodata_block: WordLen code=%x\n",
			    pbuf[sum + 2]);
		}
		sum += 3;
	}
	return 0;
}

static s32 edid_parse_vsdb(u8 *pbuf, u8 size)
{
	u8 index = 8;
	u8 vic_len = 0;
	u8 vsbd_vic = 0;
	u8 i;

	/* check if it's HDMI VSDB */
	if ((pbuf[0] == 0x03) && (pbuf[1] == 0x0c) && (pbuf[2] == 0x00)) {
		is_hdmi = 1;
		hdmi_inf("Find HDMI1.4 Vendor Specific DataBlock\n");
	} else if ((pbuf[0] == 0xd8) && (pbuf[1] == 0x5d) && (pbuf[2] == 0xc4)) {
		is_hdmi = 1;
		hdmi_inf("Find HDMI2.0 Vendor Specific DataBlock\n");
	} else {
		hdmi_inf("%s:wrong input!\n", __func__);
		is_hdmi = 0;
		is_yuv = 0;
		return 0;
	}

	if (size <= 8)
		return 0;

	if ((pbuf[7]&0x20) == 0)
		return 0;
	if ((pbuf[7]&0x40) == 0x40)
		index = index + 2;
	if ((pbuf[7]&0x80) == 0x80)
		index = index + 2;

	/* mandatary format support */
	if (pbuf[index]&0x80) {
		Device_Support_VIC[HDMI1080P_24_3D_FP] = 1;
		Device_Support_VIC[HDMI720P_50_3D_FP] = 1;
		Device_Support_VIC[HDMI720P_60_3D_FP] = 1;
		hdmi_inf("3D_present\n");
	}

	if (((pbuf[index]&0x60) == 1) || ((pbuf[index]&0x60) == 2))
		hdmi_inf("3D_multi_present\n");

	vic_len = pbuf[index+1]>>5;
	hdmi_inf("vsdb_vic_len = %d\n", vic_len);
	for (i = 0; i < vic_len; i++) {
		/* HDMI_VIC for extended resolution transmission */
		vsbd_vic = pbuf[index+2+i];
		if ((vsbd_vic > 0) && (vsbd_vic < 0x05)) {
			Device_Support_VIC[vsbd_vic + 0x100] = 1;
			hdmi_inf("edid_parse_vsdb: VIC %d support\n", vsbd_vic);
		} else {
			hdmi_inf("edid_parse_vsdb: VIC %d is a reserved VIC\n",
				vsbd_vic);
		}
	}

	index += (pbuf[index+1]&0xe0) + 2;
	if (index > (size+1))
		return 0;

	hdmi_inf("3D_multi_present byte(%2.2x,%2.2x)\n", pbuf[index],
			pbuf[index+1]);

	return 0;
}

static s32 edid_check_special(u8 *buf_src, u8 *buf_dst)
{
	u32 i;

	for (i = 0; i < 2; i++) {
		if (buf_dst[i] != buf_src[8+i])
			return -1;
	}
	for (i = 0; i < 13; i++) {
		if (buf_dst[2+i] != buf_src[0x5f+i])
			return -1;
	}
	if (buf_dst[15] != buf_src[0x7f])
		return -1;

	return 0;
}


s32 hdmi_edid_parse(void)
{
	/* collect the EDID ucdata of segment 0 */
	u8 BlockCount;
	u32 i, offset;

	hdmi_inf("hdmi_edid_parse\n");

	memset(Device_Support_VIC, 0, sizeof(Device_Support_VIC));
	memset(EDID_Buf, 0, sizeof(EDID_Buf));
	is_hdmi = 1;
	is_yuv = 1;
	is_exp = 0;
	ddc_init();

	edid_read_data(0, EDID_Buf);

	if (edid_check_sum(0, EDID_Buf) != 0)
		return 0;

	if (edid_check_header(EDID_Buf) != 0)
		return 0;

	if (edid_check_version(EDID_Buf) != 0)
		return 0;

	edid_parse_dtd_block(EDID_Buf + 0x36);

	edid_parse_dtd_block(EDID_Buf + 0x48);

	BlockCount = EDID_Buf[0x7E];
	/* back list some TV edids are bad,so go default RGB */
	if ((edid_check_special(EDID_Buf, exp0) == 0) ||
		(edid_check_special(EDID_Buf, exp1) == 0)) {
		pr_info("*****************is_exp*****************\n");
		is_exp = 1;
	}

	if (BlockCount == 0) {
		hdmi_inf("[%s]:BlockCount is 0\n", __func__);
		is_hdmi = 1;
		is_yuv = 0;
		return 0;
	}

	if (BlockCount > 0) {
		bool find_audio = false;

		if (BlockCount > 4)
			BlockCount = 4;

		for (i = 1 ; i <= BlockCount ; i++) {
			edid_read_data(i, EDID_Buf);
			if (edid_check_sum(i, EDID_Buf) != 0)
				return 0;
			/*&&(EDID_Buf[0x80*i+1]==1)*/
			if ((EDID_Buf[0x80*i+0] == 2)) {
				if ((EDID_Buf[0x80*i+1] >= 1)) {
					if (EDID_Buf[0x80*i+3]&0x20) {
						is_yuv = 1;
						hdmi_inf("support YCbCr44\n");
						if (rgb_only == 1) {
							hdmi_inf("rgb only\n");
							is_yuv = 0;
						}
					} else
						is_yuv = 0;
				}

				offset = EDID_Buf[0x80*i+2];
				/* deal with reserved data block */
				if (offset > 4) {
					u8 bsum = 4;

					while (bsum < offset) {
						u8 tag = EDID_Buf[0x80*i+bsum]>>5;
						u8 len = EDID_Buf[0x80*i+bsum]&0x1f;

						if ((len > 0) &&
						((bsum + len + 1) > offset)) {
							hdmi_inf
							("len & bsum error\n");
							return 0;
						}
						if (tag == 1) {
							/* ADB */
							edid_parse_audiodata_block(EDID_Buf+0x80*i+bsum+1, len);
							find_audio = true;
						} else if (tag == 2) {
							/* VDB */
							edid_parse_videodata_block(EDID_Buf+0x80*i+bsum+1, len);
						} else if (tag == 3) {
							/* vendor specific */
							edid_parse_vsdb(EDID_Buf+0x80*i+bsum+1, len);
						}

						bsum += (len + 1);
					}
				} else
					hdmi_inf(
					    "no data in reserved block%d\n", i);

				/* deal with 18-byte timing block */
				if (offset >= 4) {
					while (offset < (0x80-18)) {
						edid_parse_dtd_block(EDID_Buf +
							0x80*i + offset);
						offset += 18;
					}
				} else
					hdmi_inf(
					    "no datail timing in block%d\n", i);
			}
		}

		if ((!is_yuv) && (!find_audio)) {
			pr_info("[%s]:set DVI mode\n", __func__);
			is_hdmi = 0;
		}
	}

	return 0;

}

u32 hdmi_edid_is_hdmi(void)
{
	return is_hdmi;
}

u32 hdmi_edid_is_yuv(void)
{
	return is_yuv;
}

uintptr_t hdmi_edid_get_data(void)
{
	return (uintptr_t)EDID_Buf;
}
