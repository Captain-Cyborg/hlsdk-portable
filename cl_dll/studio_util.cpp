//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio_util.h"
#include "build.h"
#if XASH_ARMv8
#define XASH_SIMD_NEON 1
#include <arm_neon.h>
#include "neon_mathfun.h"
#endif

/*
====================
AngleMatrix

====================
*/
#if XASH_SIMD_NEON
const uint32x4_t AngleMatrix_sign0 = vreinterpretq_f32_u32(vsetq_lane_u32(0x80000000, vdupq_n_u32(0), 0));
const uint32x4_t AngleMatrix_sign1 = vreinterpretq_f32_u32(vsetq_lane_u32(0x80000000, vdupq_n_u32(0), 1));
const uint32x4_t AngleMatrix_sign2 = vreinterpretq_f32_u32(vsetq_lane_u32(0x80000000, vdupq_n_u32(0), 2));
#endif
void AngleMatrix( const float *angles, float (*matrix)[4] )
{
#if XASH_SIMD_NEON
	float32x4x3_t out_reg;
	float32x4_t angles_reg = {};
	memcpy(&angles_reg, angles, sizeof(float) * 3);

	float32x4x2_t sp_sy_sr_0_cp_cy_cr_1;
	sincos_ps(vmulq_n_f32(angles_reg, (M_PI * 2 / 360)), &sp_sy_sr_0_cp_cy_cr_1.val[0], &sp_sy_sr_0_cp_cy_cr_1.val[1]);

	float32x4x2_t sp_sr_cp_cr_sy_0_cy_1 = vuzpq_f32(sp_sy_sr_0_cp_cy_cr_1.val[0], sp_sy_sr_0_cp_cy_cr_1.val[1]);
	float32x4x2_t sp_cp_sy_cy_sr_cr_0_1 = vzipq_f32(sp_sy_sr_0_cp_cy_cr_1.val[0], sp_sy_sr_0_cp_cy_cr_1.val[1]);

	float32x4_t _0_sr_cr_0 = vextq_f32(sp_sy_sr_0_cp_cy_cr_1.val[0], sp_cp_sy_cy_sr_cr_0_1.val[1], 3);
	float32x4_t cp_cr_sr_0 = vcombine_f32(vget_high_f32(sp_sr_cp_cr_sy_0_cy_1.val[0]), vget_high_f32(sp_sy_sr_0_cp_cy_cr_1.val[0]));
	float32x4_t cy_sy_sy_0 = vcombine_f32(vrev64_f32(vget_high_f32(sp_cp_sy_cy_sr_cr_0_1.val[0])), vget_low_f32(sp_sr_cp_cr_sy_0_cy_1.val[1]));
	float32x4_t sy_cy_cy_1 = vcombine_f32(vget_high_f32(sp_cp_sy_cy_sr_cr_0_1.val[0]), vget_high_f32(sp_sr_cp_cr_sy_0_cy_1.val[1]));

	float32x4_t _0_srsp_crsp_0 = vmulq_laneq_f32(_0_sr_cr_0, sp_sy_sr_0_cp_cy_cr_1.val[0], 0); // *sp
	out_reg.val[0] = vmulq_laneq_f32(_0_srsp_crsp_0, sp_sy_sr_0_cp_cy_cr_1.val[1], 1); // *cy
	out_reg.val[1] = vmulq_laneq_f32(_0_srsp_crsp_0, sp_sy_sr_0_cp_cy_cr_1.val[0], 1); // *sy

	cy_sy_sy_0 = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(cy_sy_sy_0), AngleMatrix_sign1));
	sy_cy_cy_1 = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(sy_cy_cy_1), AngleMatrix_sign2));
	out_reg.val[0] = vfmaq_f32(out_reg.val[0], cp_cr_sr_0, cy_sy_sy_0);
	out_reg.val[1] = vfmaq_f32(out_reg.val[1], cp_cr_sr_0, sy_cy_cy_1);

	float32x4_t cp_cr_0_1 = vcombine_f32(vget_high_f32(sp_sr_cp_cr_sy_0_cy_1.val[0]), vget_high_f32(sp_cp_sy_cy_sr_cr_0_1.val[1]));
	float32x4_t _1_cp_cr_0 = vextq_f32(cp_cr_0_1, cp_cr_0_1, 3);
	out_reg.val[2] = vmulq_f32(sp_sr_cp_cr_sy_0_cy_1.val[0], _1_cp_cr_0);
	out_reg.val[2] = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(out_reg.val[2]), AngleMatrix_sign0));

	memcpy(matrix, &out_reg, sizeof(float) * 3 * 4);
/*
	matrix[0][0] =		  cp*cy;
	matrix[0][1] = sr*sp*cy-cr*sy;
	matrix[0][2] = cr*sp*cy+sr*sy;
	matrix[0][3] = 0.0;
	matrix[1][0] =		  cp*sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[1][2] = cr*sp*sy-sr*cy;
	matrix[1][3] = 0.0;
	matrix[2][0] = -sp*1;
	matrix[2][1] = sr*cp;
	matrix[2][2] = cp*cr;
	matrix[2][3] = cr*0;
*/
#else
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * ( M_PI_F * 2.0f / 360.0f );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[PITCH] * ( M_PI_F * 2.0f / 360.0f );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[ROLL] * ( M_PI_F * 2.0f / 360.0f );
	sr = sin( angle );
	cr = cos( angle );

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr * sp * cy + cr * -sy;
	matrix[1][1] = sr * sp * sy + cr * cy;
	matrix[2][1] = sr * cp;
	matrix[0][2] = (cr * sp * cy + -sr * -sy);
	matrix[1][2] = (cr * sp * sy + -sr* cy);
	matrix[2][2] = cr * cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
#endif
}

/*
====================
VectorCompare

====================
*/
int VectorCompare( const float *v1, const float *v2 )
{
#if XASH_SIMD_NEON
	// is this really works? 
	float32x4_t v1_reg = {}, v2_reg = {};
	memcpy(&v1_reg, v1, sizeof(float) * 3);
	memcpy(&v2_reg, v2, sizeof(float) * 3);
	return !vaddvq_u32(vceqq_f32(v1_reg, v2_reg));
#else
	int i;

	for( i = 0; i < 3; i++ )
		if( v1[i] != v2[i] )
			return 0;

	return 1;
#endif
}

/*
====================
CrossProduct

====================
*/
void CrossProduct( const float *v1, const float *v2, float *cross )
{
#if XASH_SIMD_NEON
	float32x4_t v1_reg = {}, v2_reg = {};
	memcpy(&v1_reg, v1, sizeof(float) * 3);
	memcpy(&v2_reg, v2, sizeof(float) * 3);

	float32x4_t yzxy_a = vextq_f32(vextq_f32(v1_reg, v1_reg, 3), v1_reg, 2); // [aj, ak, ai, aj]
	float32x4_t yzxy_b = vextq_f32(vextq_f32(v2_reg, v2_reg, 3), v2_reg, 2); // [bj, bk, bi, bj]
	float32x4_t zxyy_a = vextq_f32(yzxy_a, yzxy_a, 1); // [ak, ai, aj, aj]
	float32x4_t zxyy_b = vextq_f32(yzxy_b, yzxy_b, 1); // [bk, ai, bj, bj]
	float32x4_t cross_reg = vfmsq_f32(vmulq_f32(yzxy_a, zxyy_b), zxyy_a, yzxy_b); // [ajbk-akbj, akbi-aibk, aibj-ajbi, 0]

	memcpy(cross, &v2_reg, sizeof(float) * 3);
#else
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
#endif
}

/*
====================
VectorTransform

====================
*/
void VectorTransform( const float *in1, float in2[3][4], float *out )
{
#if XASH_SIMD_NEON
	float32x4_t in1_reg = {};
	memcpy(&in1_reg, &in1, sizeof(float) * 3);

	float32x4x4_t in_t;
	memcpy(&in_t, &in2, sizeof(float) * 3 * 4);
	//memset(&in_t.val[3], 0, sizeof(in_t.val[3]));
	in_t = vld4q_f32((const float*)&in_t);

	float32x4_t out_reg = in_t.val[3];
	out_reg = vfmaq_laneq_f32(out_reg, in_t.val[0], in1_reg, 0);
	out_reg = vfmaq_laneq_f32(out_reg, in_t.val[1], in1_reg, 1);
	out_reg = vfmaq_laneq_f32(out_reg, in_t.val[2], in1_reg, 2);

	memcpy(out, &out_reg, sizeof(float) * 3);
#else
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
#endif
}

/*
================
ConcatTransforms

================
*/
void ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] )
{
#if XASH_SIMD_NEON
	float32x4x3_t in1_reg, in2_reg;
	memcpy(&in1_reg, in1, sizeof(float) * 3 * 4);
	memcpy(&in2_reg, in2, sizeof(float) * 3 * 4);
	float32x4x3_t out_reg = {};

	out_reg.val[0] = vcopyq_laneq_f32(out_reg.val[0], 3, in1_reg.val[0], 3); // out[0][3] = in[0][3]
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], in2_reg.val[0], in1_reg.val[0], 0); // out[0][n] += in2[0][n] * in1[0][0]
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], in2_reg.val[1], in1_reg.val[0], 1); // out[0][n] += in2[1][n] * in1[0][1]
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], in2_reg.val[2], in1_reg.val[0], 2); // out[0][n] += in2[2][n] * in1[0][2]

	out_reg.val[1] = vcopyq_laneq_f32(out_reg.val[1], 3, in1_reg.val[1], 3);
	out_reg.val[1] = vfmaq_laneq_f32(out_reg.val[1], in2_reg.val[0], in1_reg.val[1], 0);
	out_reg.val[1] = vfmaq_laneq_f32(out_reg.val[1], in2_reg.val[1], in1_reg.val[1], 1);
	out_reg.val[1] = vfmaq_laneq_f32(out_reg.val[1], in2_reg.val[2], in1_reg.val[1], 2);

	out_reg.val[2] = vcopyq_laneq_f32(out_reg.val[2], 3, in1_reg.val[2], 3);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], in2_reg.val[0], in1_reg.val[2], 0);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], in2_reg.val[1], in1_reg.val[2], 1);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], in2_reg.val[2], in1_reg.val[2], 2);

	memcpy(&out, &out_reg, sizeof(out));
#else
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
#endif
}

// angles index are not the same as ROLL, PITCH, YAW

/*
====================
AngleQuaternion

====================
*/
#if XASH_SIMD_NEON
const float32x4_t AngleQuaternion_sign2 = vzipq_f32(vdupq_n_u32(0x80000000), vdupq_n_u32(0x00000000)).val[0]; // { 0x80000000, 0x00000000, 0x80000000, 0x00000000 };
#endif
void AngleQuaternion( float *angles, vec4_t quaternion )
{
#if XASH_SIMD_NEON
	float32x4_t angles_reg = {};
	memcpy(&angles_reg, angles, sizeof(float) * 3);
	float32x4x2_t sr_sp_sy_0_cr_cp_cy_1;
	sincos_ps(vmulq_n_f32(angles_reg, 0.5), &sr_sp_sy_0_cr_cp_cy_1.val[0], &sr_sp_sy_0_cr_cp_cy_1.val[1]);

	float32x4x2_t sr_sy_cr_cy_sp_0_cp_1 = vuzpq_f32(sr_sp_sy_0_cr_cp_cy_1.val[0], sr_sp_sy_0_cr_cp_cy_1.val[1]);
	float32x4_t cp_cp_cp_cp = vdupq_laneq_f32(sr_sp_sy_0_cr_cp_cy_1.val[1], 1);
	float32x4_t sp_sp_sp_sp = vdupq_laneq_f32(sr_sp_sy_0_cr_cp_cy_1.val[0], 1);

	float32x4_t sr_sy_cr_cy = sr_sy_cr_cy_sp_0_cp_1.val[0];
	float32x4_t sy_cr_cy_sr = vextq_f32(sr_sy_cr_cy_sp_0_cp_1.val[0], sr_sy_cr_cy_sp_0_cp_1.val[0], 1);
	float32x4_t cr_cy_sr_sy = vextq_f32(sr_sy_cr_cy_sp_0_cp_1.val[0], sr_sy_cr_cy_sp_0_cp_1.val[0], 2);
	float32x4_t cy_sr_sy_cr = vextq_f32(sr_sy_cr_cy_sp_0_cp_1.val[0], sr_sy_cr_cy_sp_0_cp_1.val[0], 3);
	float32x4_t sp_sp_sp_sp_signed = veorq_u32(sp_sp_sp_sp, AngleQuaternion_sign2);

	float32x4_t left = vmulq_f32(vmulq_f32(sr_sy_cr_cy, cp_cp_cp_cp), cy_sr_sy_cr);

	float32x4_t out_reg = vfmaq_f32(left, vmulq_f32(cr_cy_sr_sy, sp_sp_sp_sp_signed), sy_cr_cy_sr);
	memcpy(quaternion, &out_reg, sizeof(float) * 4);
	//quaternion[0] =   sr * cp * cy - cr * sp * sy; // X
	//quaternion[1] =   sy * cp * sr + cy * sp * cr; // Y
	//quaternion[2] =   cr * cp * sy - sr * sp * cy; // Z
	//quaternion[3] =   cy * cp * cr + sy * sp * sr; // W
#else
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5f;
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[1] * 0.5f;
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[0] * 0.5f;
	sr = sin( angle );
	cr = cos( angle );

	quaternion[0] = sr * cp * cy - cr * sp * sy; // X
	quaternion[1] = cr * sp * cy + sr * cp * sy; // Y
	quaternion[2] = cr * cp * sy - sr * sp * cy; // Z
	quaternion[3] = cr * cp * cy + sr * sp * sy; // W
#endif
}

/*
====================
QuaternionSlerp

====================
*/
void QuaternionSlerp( vec4_t p, vec4_t q, float t, vec4_t qt )
{
#if XASH_SIMD_NEON
	float32x4_t p_reg = {}, q_reg = {};
	memcpy(&p_reg, p, sizeof(float) * 4);
	memcpy(&q_reg, q, sizeof(float) * 4);

	// q = (cos(a/2), xsin(a/2), ysin(a/2), zsin(a/2))
	// cos(a-b) = cosacosb+sinasinb
	const uint32x4_t signmask = vdupq_n_u32(0x80000000);
	const float32x4_t one_minus_epsilon = vdupq_n_f32(1.0f - 0.00001f);

	float32x4_t vcosom = vdupq_n_f32(DotProduct(p, q));
	uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(vcosom), signmask);
	q_reg = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(p_reg), sign));
	vcosom = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(vcosom), sign));

	float x[4] = {(1.0f - t), t, 1, 0}; // cosom -> 1, sinom -> 0, sinx ~ x
	float32x4_t x_reg;
	memcpy(&x_reg, x, sizeof(float) * 4);

	// if ((1.0 - cosom) > 0.000001) x = sin(x * omega)
	uint32x4_t cosom_less_then_one = vcltq_f32(vcosom, one_minus_epsilon);
	float32x4_t vomega = acos_ps(vcosom);
	x_reg = vbslq_f32(cosom_less_then_one, x_reg, sin_ps(vmulq_f32(x_reg, vomega)));

	// qt = (x[0] * p + x[1] * q) / x[2];
	float32x4_t qt_reg = vmulq_laneq_f32(p_reg, x_reg, 0);
	qt_reg = vfmaq_laneq_f32(qt_reg, q_reg, x_reg, 1);
	qt_reg = vdivq_f32(qt_reg, vdupq_laneq_f32(x_reg, 2)); // vdivq_laneq_f32 ?

	memcpy(qt, &qt_reg, sizeof(float) * 4);
#else
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;

	for( i = 0; i < 4; i++ )
	{
		a += ( p[i] - q[i] ) * ( p[i] - q[i] );
		b += ( p[i] + q[i] ) * ( p[i] + q[i] );
	}
	if(a > b)
	{
		for( i = 0; i < 4; i++ )
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if( ( 1.0f + cosom ) > 0.000001f )
	{
		if( ( 1.0f - cosom ) > 0.000001f )
		{
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( ( 1.0f - t ) * omega ) / sinom;
			sclq = sin( t * omega ) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}
		for( i = 0; i < 4; i++ )
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sin( ( 1.0f - t ) * ( 0.5f * M_PI_F ) );
		sclq = sin( t * ( 0.5f * M_PI_F ) );
		for( i = 0; i < 3; i++ )
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
#endif
}

/*
====================
QuaternionMatrix

====================
*/
#if XASH_SIMD_NEON
const uint32x4_t QuaternionMatrix_sign1 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0x00000000), 0); // { 0x80000000, 0x00000000, 0x00000000, 0x00000000 };
const uint32x4_t QuaternionMatrix_sign2 = vsetq_lane_u32(0x80000000, vdupq_n_u32(0x00000000), 1); // { 0x00000000, 0x80000000, 0x00000000, 0x00000000 };
const uint32x4_t QuaternionMatrix_sign3 = vsetq_lane_u32(0x00000000, vdupq_n_u32(0x80000000), 2); // { 0x80000000, 0x80000000, 0x00000000, 0x80000000 };
const float32x4_t matrix3x4_identity_0 = vsetq_lane_f32(1, vdupq_n_f32(0), 0); // { 1, 0, 0, 0 }
const float32x4_t matrix3x4_identity_1 = vsetq_lane_f32(1, vdupq_n_f32(0), 1); // { 0, 1, 0, 0 }
const float32x4_t matrix3x4_identity_2 = vsetq_lane_f32(1, vdupq_n_f32(0), 2); // { 0, 0, 1, 0 }
#endif

void QuaternionMatrix( vec4_t quaternion, float (*matrix)[4] )
{
#if XASH_SIMD_NEON
	float32x4_t quaternion_reg;
	memcpy(&quaternion_reg, quaternion, sizeof(float) * 4);

	float32x4_t q1032 = vrev64q_f32(quaternion_reg);
	float32x4_t q1032_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q1032), QuaternionMatrix_sign1));
	float32x4_t q2301 = vextq_f32(quaternion_reg, quaternion_reg, 2);
	float32x4_t q2301_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q2301), QuaternionMatrix_sign3));
	float32x4_t q3210 = vrev64q_f32(q2301);
	float32x4_t q3210_signed = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(q3210), QuaternionMatrix_sign2));

	float32x4x3_t out_reg;

	out_reg.val[0] = vmulq_laneq_f32(q2301_signed, quaternion_reg, 2);
	out_reg.val[0] = vfmaq_laneq_f32(out_reg.val[0], q1032_signed, quaternion_reg, 1);
	out_reg.val[0] = vfmaq_n_f32(matrix3x4_identity_0, out_reg.val[0], 2.0f);

	out_reg.val[1] = vmulq_laneq_f32(q3210_signed, quaternion_reg, 2);
	out_reg.val[1] = vfmsq_laneq_f32(out_reg.val[1], q1032_signed, quaternion_reg, 0);
	out_reg.val[1] = vfmaq_n_f32(matrix3x4_identity_1, out_reg.val[1], 2.0f);

	out_reg.val[2] = vmulq_laneq_f32(q3210_signed, quaternion_reg, 1);
	out_reg.val[2] = vfmaq_laneq_f32(out_reg.val[2], q2301_signed, quaternion_reg, 0);
	out_reg.val[2] = vfmsq_n_f32(matrix3x4_identity_2, out_reg.val[2], 2.0f);

	memcpy(matrix, &out_reg, sizeof(float) * 3 * 4);
/*
	matrix[0][0] = 1.0 + 2.0 * (  quaternion[1] * -quaternion[1] + -quaternion[2] * quaternion[2] );
	matrix[0][1] = 0.0 + 2.0 * (  quaternion[1] *  quaternion[0] + -quaternion[3] * quaternion[2] );
	matrix[0][2] = 0.0 + 2.0 * (  quaternion[1] *  quaternion[3] +  quaternion[0] * quaternion[2] );
	matrix[0][3] = 0.0 + 2.0 * (  quaternion[1] *  quaternion[2] + -quaternion[1] * quaternion[2] );

	matrix[1][0] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[1] +  quaternion[3] * quaternion[2] );
	matrix[1][1] = 1.0 + 2.0 * ( -quaternion[0] *  quaternion[0] + -quaternion[2] * quaternion[2] );
	matrix[1][2] = 0.0 + 2.0 * ( -quaternion[0] *  quaternion[3] +  quaternion[1] * quaternion[2] );
	matrix[1][3] = 0.0 + 2.0 * ( -quaternion[0] *  quaternion[2] +  quaternion[0] * quaternion[2] );
	
	matrix[2][0] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[2] + -quaternion[3] * quaternion[1] );
	matrix[2][1] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[3] +  quaternion[2] * quaternion[1] );
	matrix[2][2] = 1.0 + 2.0 * ( -quaternion[0] *  quaternion[0] + -quaternion[1] * quaternion[1] );
	matrix[2][3] = 0.0 + 2.0 * ( -quaternion[0] * -quaternion[1] + -quaternion[0] * quaternion[1] );
*/
#else
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
#endif
}

/*
====================
MatrixCopy

====================
*/
void MatrixCopy( float in[3][4], float out[3][4] )
{
	memcpy( out, in, sizeof( float ) * 3 * 4 );
}
