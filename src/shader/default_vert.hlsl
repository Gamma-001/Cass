struct PSInput {
    float4 position : SV_POSITION;
	float4 color : COLOR;
};

PSInput main(float4 pos : POSITION, float4 color: COLOR) {
    PSInput result;
    
    result.position = pos;
    result.color = color;
    
    return result;
}