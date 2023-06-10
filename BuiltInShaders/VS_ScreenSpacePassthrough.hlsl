// a simple vertex shader to passthrough screen-space position data
float4 VSMain(float3 pos : POSITION) : SV_POSITION
{
	return float4(pos, 1.0f);
}