#include "mmath.h"
bool test_mmath()
{
	using namespace DirectX;
	struct { VWB_VEC3d x, y; } d[] =
	{
		{ VWB_VEC3d(1, 0, 0), VWB_VEC3d(0, 1, 0) },  // ( 0, 0, 0 )

		{ VWB_VEC3d(1, 1, 0), VWB_VEC3d(-1, 1, 0) }, // ( 0, 0, -45 )
		{ VWB_VEC3d(1, -1, 0), VWB_VEC3d(1, 1, 0) }, // ( 0, 0, 45 )

		{ VWB_VEC3d(1, 0, -1), VWB_VEC3d(0, 1, 0) },  // ( 0, -45, 0 )
		{ VWB_VEC3d(1, 0, 1), VWB_VEC3d(0, 1, 0) }, // ( 0, 45, 0 )

		{ VWB_VEC3d(1, 0, 0), VWB_VEC3d(0, 1, -1) },  // ( -45, 0 , 0 )
		{ VWB_VEC3d(1, 0, 0), VWB_VEC3d(0, 1, 1) }, // ( 45, 0, 0 )

		{ VWB_VEC3d(12, 22, 3), VWB_VEC3d(-18, 10, -5) },

		{ VWB_VEC3d(-12, 2, -3), VWB_VEC3d(2, -10, -5) },
	};

	for (int i = 0; i != ARRAYSIZE(d); i++)
	{
		VWB_MAT33d M = VWB_MAT33d::Base(d[i].x, d[i].y);
		VWB_VEC3d r = M.GetR();
		VWB_VEC3d rDeg = r * 180 / PI;

		VWB_MAT33d R = VWB_MAT33d::R(r);

		VWB_MAT33d Rx = VWB_MAT33d::Rx(r.x);
		VWB_MAT33d Ry = VWB_MAT33d::Ry(r.y);
		VWB_MAT33d Rz = VWB_MAT33d::Rz(r.z);
		VWB_MAT33d RM = Rz * Rx * Ry;
		VWB_MAT33d D = R - RM;
		VWB_MAT33d D2 = M - R;
		VWB_MAT33d D3 = M - RM;
		if (!D.IsZero())
			logStr(1, "Rotation Matrix test 1 (combined vs multipiled) failed, difference norm is %f", D.Norm());
		if (!D2.IsZero())
			logStr(1, "Rotation Matrix test 2 (base vs combined) failed, difference norm is %f", D2.Norm());
		if (!D3.IsZero())
			logStr(1, "Rotation Matrix test 3 (base vs multiplied) failed, difference norm is %f", D3.Norm());
		XMMATRIX MMR = XMMatrixRotationRollPitchYaw((FLOAT)r.x, (FLOAT)-r.y, (FLOAT)-r.z);
		VWB_MAT44f MRX; XMStoreFloat4x4((XMFLOAT4X4*)&MRX, MMR);

		VWB_MAT44f RR = VWB_MAT44f::R(VWB_VEC3f(r));

		MRX -= RR;
		if (!MRX.IsZero())
			logStr(1, "Rotation Matrix test 4 (combined vs XN Math) failed, difference norm is %f", MRX.Norm());

		VWB_MAT33d L = VWB_MAT33d::R_LH(r);
		VWB_MAT33d Lx = VWB_MAT33d::Rx_LH(r.x);
		VWB_MAT33d Ly = VWB_MAT33d::Ry_LH(r.y);
		VWB_MAT33d Lz = VWB_MAT33d::Rz_LH(r.z);
		VWB_MAT33d LM = Lz * Lx * Ly;
		VWB_MAT33d E = L - LM;
		if (!E.IsZero())
			logStr(1, "Rotation Matrix test 5 (LH combined vs LH multiplied) failed, difference norm is %f", E.Norm());

		XMMATRIX MML = XMMatrixRotationRollPitchYaw((FLOAT)-r.x, (FLOAT)r.y, (FLOAT)r.z);
		VWB_MAT44f MLX;
		XMStoreFloat4x4((XMFLOAT4X4*)&MLX, MML);
		MLX -= VWB_MAT44f(L);
		if (!MLX.IsZero())
			logStr(1, "Rotation Matrix test 6 ( LH combined vs XN Math) failed, difference norm is %f", MLX.Norm());
		XMStoreFloat4x4((XMFLOAT4X4*)&MLX, MML);
		MLX -= VWB_MAT44f(LM);
		if (!MLX.IsZero())
			logStr(1, "Rotation Matrix test 7 ( LH multiplied vs XN Math) failed, difference norm is %f", MLX.Norm());
	}
	{
		float  clips[][6] =
		{
			{ 1.6f, 1.0f, 1.6f, 1.0f, 1.0f, 10.0f },
			{ 3.2f, 1.0f, 0.0f, 1.0f, 1.0f, 10.0f },
			{ 0.0f, 1.0f, 3.2f, 1.0f, 1.0f, 10.0f },
			{ 1.6f, 0.0f, 1.6f, 2.0f, 1.0f, 10.0f },
			{ 1.6f, 2.0f, 1.6f, 0.0f, 1.0f, 10.0f },
		};
		for (auto clip = *clips, clipE = *(clips + ARRAYSIZE(clips)); clip != clipE; clip += ARRAYSIZE(clips[0]))
		{
			VWB_MAT44f P = VWB_MAT44f::DXFrustumLH(clip);
			XMMATRIX MP = XMMatrixPerspectiveOffCenterLH(-clip[0], clip[2], -clip[3], clip[1], clip[4], clip[5]);
			VWB_MAT44f PX; XMStoreFloat4x4A((XMFLOAT4X4A*)&PX, MP);
			PX -= P;
			if (!PX.IsZero())
				logStr(1, "Frustum Matrix test 1 failed, difference norm is %f", PX.Norm());

		}
	}
	{
		double clips[][6] = {
			{ 0.1, 0.2, 0.5, 0.4, 1, 1024 },
			{ 0.5, 0.4, 0.1, 0.2, 1, 1024 },
			{ -0.1, 0.2, 0.5, 0.4, 1, 1024 },
			{ 0.1, 0.5, 0.5, -0.2, 1, 1024 },
		};
		for (int i = 0; i != ARRAYSIZE(clips); i++)
		{
			VWB_MAT44d V = VWB_MAT44d::I();
			MakeSymmetricLH(V, clips[i]);
		}
	}
	return true;
}

