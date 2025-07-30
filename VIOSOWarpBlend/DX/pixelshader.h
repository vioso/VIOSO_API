static char s_pixelShaderDX2a[] = R"END(
sampler samContent : register(s0);
sampler samWarp : register(s1);               
sampler samBlend : register(s2);              
sampler samCur : register(s3);               // cur texture, rgb for color; if alpha<=0,5 2*alpha is for blending, bigger will invert pixel
sampler samBlack : register(s4);			// this is the black level uplift alias beta texture
                                                
float4x4 matView : register(c0);                
float4 bBorder : register(c4);                   // bBorder.x > 0.5 = border on, else off; bBorder.y > 0.5 = blend on, else off, bBorder.z > 0.5 black-level correction on, else off
float4 params : register(c5);					   //x.. content width, y .. content height, z = 1/content width, w = 1/content height
float4 offsScale : register(c6);				   //x.. offset x, y .. offset y, z = scale X, w = scale Y  (u',v')=( (u-x)*z, (v-y)*w )
float4 offsScaleCur : register(c7);				  //x.. offset x, y .. offset y, z = scale X, w = scale Y  (u',v')=( (u-x)*z, (v-y)*w )
float4 blackBias : register(c8); 			// a bias value, serves as a scale of the black texture; thus the texture can have RGB8 and will be up-scaled to fill whole definition range, but has a fine resolution in low intensity values
                                                
struct VS_OUT {                                 
    float4 pos : POSITION;                   
    float2 tex : TEXCOORD0;                     
};                  
																
                                                
//-------------------------------------------------------------
// Pixel Shaders												
//-------------------------------------------------------------
                                                
float4 tex2DBC(uniform sampler   texCnt,
               float2            vPos)
{
	vPos*= params.xy;
	float2 t = floor( vPos - 0.5 ) + 0.5; // the nearest pixel
	float2 w0 = 1;
	float2 w1 = vPos - t;
	float2 w2 = w1 * w1;
	float2 w3 = w2 * w1;

	w0 = w2 - 0.5 * (w3 + w1);
	w1 = 1.5 * w3 - 2.5 * w2 + 1.0;
	w3 = 0.5 * (w3 - w2);
	w2 = 1.0 - w0 - w1 - w3;

	float2 s0 = w0 + w1;
	float2 s1 = w2 + w3;
	float2 f0 = w1 / s0;
	float2 f1 = w3 / s1;

	float2 t0 = t - 1 + f0;
	float2 t1 = t + 1 + f1;
	t0*= params.zw;
	t1*= params.zw;

	return
		( tex2D( texCnt, t0 ) * s0.x +
		  tex2D( texCnt, float2( t1.x, t0.y ) ) * s1.x ) * s0.y +
		( tex2D( texCnt, float2( t0.x, t1.y ) ) * s0.x +
		  tex2D( texCnt, t1 ) * s1.x ) * s1.y;
}
                                                
float4 PS( VS_OUT vIn ) : COLOR                 
{                                               
    float4 color = tex2D( samContent, ( vIn.tex.xy + offsScale.xy ) * offsScale.zw ); 
    return color;                               
}                                               
                                                
float4 PSWB( VS_OUT vIn ) : COLOR               
{                                               
	float4 tex = tex2D( samWarp, vIn.tex );      
	float4 blend = tex2D( samBlend, vIn.tex );   
	float4 black = tex2D( samBlack, vIn.tex ) * blackBias.x;   
	float4 vOut = 0;
	float4 vCur = 0;
	if( 0.1 < blend.a )
	{
		if( bBorder.x > 0.5 )                      
		{                                           
		    tex.x*= 1.02;                           
		    tex.x-= 0.01;                           
		    tex.y*= 1.02;                           
		    tex.y-= 0.01;                           
		}                                           
		tex.xy/= blend.a;
       vOut = tex2D( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw ); 
		vCur = tex2D( samCur, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}
	vOut.a = 1;                                  
	return vOut;                                 
}                                               
                                                
float4 PSWB3D( VS_OUT vIn ) : COLOR             
{                                               
	float4 tex = tex2D( samWarp, vIn.tex );      
	float4 blend = tex2D( samBlend, vIn.tex );   
	float4 black = tex2D( samBlack, vIn.tex ) * blackBias.x;   
	float4 vOut = float4( 0,0,0,1);              
	float4 vCur = 0;
	if( 0.1 < blend.a )                            
	{                                            
		tex/= blend.a;                            
		tex.w = 1;                            
		tex = mul( tex, matView );               
		tex.xy/= tex.w;                          
		tex.x/=2;                                
		tex.y/=-2;                               
		tex.xy+= 0.5;                           
       vOut = tex2D( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw ); 
		vCur = tex2D( samCur, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}                                               

float4 PSBC( VS_OUT vIn ) : COLOR                 
{                                               
    float4 color = tex2DBC(samContent, ( vIn.tex.xy - offsScale.xy ) * offsScale.zw ); 
    return color;                               
}                                               
                                                
float4 PSWBBC( VS_OUT vIn ) : COLOR               
{                                               
	vIn.tex-= offsScale.xy;                      
	vIn.tex*= offsScale.zw;                      
	float4 tex = tex2D( samWarp, vIn.tex );     
	float4 blend = tex2D( samBlend, vIn.tex );  
	float4 black = tex2D( samBlack, vIn.tex ) * blackBias.x;   
	float4 vOut = 0;
	float4 vCur = 0;
	if( 0.1 < blend.a )
	{
		if( bBorder.x > 0.5 )                      
		{                                           
		    tex.x*= 1.02;                           
		    tex.x-= 0.01;                           
		    tex.y*= 1.02;                           
		    tex.y-= 0.01;                           
		}                                           
		tex.xy/= blend.a;
       vOut = tex2DBC( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw ); 
		vCur = tex2D( samCur, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}
	vOut.a = 1;                                 
	return vOut;                                
}                                               
                                                
float4 PSWB3DBC( VS_OUT vIn ) : COLOR             
{                                               
	float4 tex = tex2D( samWarp, vIn.tex );     
	float4 blend = tex2D( samBlend, vIn.tex );  
	float4 black = tex2D( samBlack, vIn.tex ) * blackBias.x;   
	float4 vOut = float4( 0,0,0,1);             
	float4 vCur = 0;
	if( 0.1 < blend.a )                            
	{                                           
		tex/= blend.a;                            
		tex.w = 1;                            
		tex = mul( tex, matView );              
		tex.xy/= tex.w;                         
		tex.x/=2;                              
		tex.y/=-2;                               
		tex.xy+= 0.5;                           
       vOut = tex2DBC( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw ); 
		vCur = tex2D( samCur, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}                                               
)END";

static char s_pixelShaderDX4[] = R"END(
Texture2D texWarp : register(t0);               
Texture2D texBlend : register(t1);              
Texture2D texCur : register(t2);              
Texture2D texBlack : register(t3);			// this is the black level uplift alias beta texture
Texture2D texContent : register(t4);            
                                                
cbuffer ConstantBuffer : register( b0 )                     
{																
	float4x4 matView;							
	float4 bBorder;								  // bBorder.x > 0.5 = border on, else off; bBorder.y > 0.5 = blend on, else off, bBorder.z > 0.5 black-level correction on, else off
	float4 params;								  //x.. content width, y .. content height, z = 1/content width, w = 1/content height
	float4 offsScale;	            			  //x.. offset x, y .. offset y, z = scale X, w = scale Y  (u',v')=( (u-x)*z, (v-y)*w )
	float4 offsScaleCur;						  //x.. offset x, y .. offset y, z = scale X, w = scale Y  (u',v')=( (u-x)*z, (v-y)*w )
	float4 blackBias;							 // a bias value, serves as a scale of the black texture; thus the texture can have RGB8 and will be up-scaled to fill whole definition range, but has a fine resolution in low intensity values
};												
                                                
SamplerState samLin : register( s0 );
SamplerState samWarp : register( s1 );
SamplerState samContent : register( s2 );
                                                
//-------------------------------------------------------------
struct VS_INPUT												
{																
    float4 Pos : POSITION;										
    float2 Tex : TEXCOORD0;									
};																
																
struct VS_OUT {                                 
    float4 pos : SV_Position;                   
    float2 tex : TEXCOORD0;                     
};                  
																
//-------------------------------------------------------------
// Vertex Shader												
//-------------------------------------------------------------
VS_OUT VS( VS_INPUT input )									
{																
    VS_OUT output = (VS_OUT)0;								
    output.pos = input.Pos;									
    output.tex = input.Tex;								    
    return output;												
}																
                                                
//-------------------------------------------------------------
// Pixel Shaders												
//-------------------------------------------------------------
float4 tex2DBC(uniform Texture2D texCnt,
               float2            vPos)
{
	vPos*= params.xy;
	float2 t = floor( vPos - 0.5 ) + 0.5; // the nearest pixel
	float2 w0 = 1;
	float2 w1 = vPos - t;
	float2 w2 = w1 * w1;
	float2 w3 = w2 * w1;

	w0 = w2 - 0.5 * (w3 + w1);
	w1 = 1.5 * w3 - 2.5 * w2 + 1.0;
	w3 = 0.5 * (w3 - w2);
	w2 = 1.0 - w0 - w1 - w3;

	float2 s0 = w0 + w1;
	float2 s1 = w2 + w3;
	float2 f0 = w1 / s0;
	float2 f1 = w3 / s1;

	float2 t0 = t - 1 + f0;
	float2 t1 = t + 1 + f1;
	t0*= params.zw;
	t1*= params.zw;

	return
		( texCnt.Sample( samContent, t0 ) * s0.x +
		  texCnt.Sample( samContent, float2( t1.x, t0.y ) ) * s1.x ) * s0.y +
		( texCnt.Sample( samContent, float2( t0.x, t1.y ) ) * s0.x +
		  texCnt.Sample( samContent, t1 ) * s1.x ) * s1.y;
}
                                                
float4 PS( VS_OUT vIn ) : SV_Target                 
{                                               
	 float4 color = texContent.Sample( samLin, ( vIn.tex - offsScale.xy ) * offsScale.zw ); 
    return color;                               
}                                               
                                                
float4 PSWB( VS_OUT vIn ) : SV_Target               
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = 0;
	float4 vCur = 0;
	if( 0.1 < blend.a )
	{
		if( bBorder.x > 0.5 )                      
		{                                           
		    tex.x*= 1.02;                           
		    tex.x-= 0.01;                           
		    tex.y*= 1.02;                           
		    tex.y-= 0.01;                           
		}                                           
		tex.xy/= blend.a;
		vOut = texContent.Sample( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}
	vOut.a = 1;                                 
	return vOut;                                
}                                               

float4 PSWB3D( VS_OUT vIn ) : SV_Target
{
	uint w,h;
	texWarp.GetDimensions( w, h );
	float4 tex = texWarp.Load( int3( w * vIn.tex.x, h * vIn.tex.y, 0 ) );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = float4( 0,0,0,1);             
	if( 0.01 < blend.a )                            
	{                                           
		tex/= blend.a;                            
		tex.w = 1;                            
		tex = mul( tex, matView );              
		tex.xy/= tex.w;                         
		tex.x/=2;                              
		tex.y/=-2;                               
		tex.xy+= 0.5;    
//		// test mappings and border fit
//		if( 0.01 <= tex.x && tex.x <= 0.99 && 0.01 <= tex.y && tex.y <= 0.99 )                    
//			vOut = float4( tex.x, tex.y, 1, 1 );
//		else if( 0 <= tex.x && tex.x <= 1 && 0 <= tex.y && tex.y <= 1 )  
//			vOut = float4( tex.x, tex.y, 0, 1 );
//		else
//			vOut = float4( 0, 0, 0, 1 );
		vOut = texContent.Sample( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		float4 vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}                                               

float4 PSBC( VS_OUT vIn ) : SV_Target                 
{                                               
    float4 color = tex2DBC( texContent, ( vIn.tex - offsScale.xy ) * offsScale.zw ); 
    return color;                               
}                                               
                                                
float4 PSWBBC( VS_OUT vIn ) : SV_Target               
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = 0;
	float4 vCur = 0;
	if( 0.1 < blend.a )
	{
		if( bBorder.x > 0.5 )                      
		{                                           
		    tex.x*= 1.02;                           
		    tex.x-= 0.01;                           
		    tex.y*= 1.02;                           
		    tex.y-= 0.01;                           
		}                                           
		tex.xy/= blend.a;
		vOut = tex2DBC( texContent, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}
	vOut.a = 1;                                 
	return vOut;                                
}                                               
                                                
float4 PSWB3DBC( VS_OUT vIn ) : SV_Target             
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = float4( 0,0,0,1);             
	float4 vCur = 0;
	if( 0.1 < blend.a )                            
	{                                           
		tex/= blend.a;                            
		tex.w = 1;                            
		tex = mul( tex, matView );              
		tex.xy/= tex.w;                         
		tex.x/=2;                              
		tex.y/=-2;                               
		tex.xy+= 0.5;                           
		vOut = tex2DBC( texContent, ( tex.xy - offsScale.xy ) * offsScale.zw ); 
		vCur = texCur.Sample( samLin, (tex.xy - offsScaleCur.xy) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}                                               
)END";

static char s_pixelShaderDX4_vFlip[] = R"END(
Texture2D texWarp : register(t0);               
Texture2D texBlend : register(t1);              
Texture2D texCur : register(t2);              
Texture2D texBlack : register(t3);			// this is the black level uplift alias beta texture
Texture2D texContent : register(t4);            
                                                
cbuffer ConstantBuffer : register( b0 )                     
{																
	float4x4 matView;							
	float4 bBorder;								  // bBorder.x > 0.5 = border on, else off; bBorder.y > 0.5 = blend on, else off, bBorder.z > 0.5 black-level correction on, else off
	float4 params;								  //x.. content width, y .. content height, z = 1/content width, w = 1/content height
	float4 offsScale;	            			  //x.. offset x, y .. offset y, z = scale X, w = scale Y  (u',v')=( (u-x)*z, (v-y)*w )
	float4 offsScaleCur;						  //x.. offset x, y .. offset y, z = scale X, w = scale Y  (u',v')=( (u-x)*z, (v-y)*w )
	float4 blackBias;							 // a bias value, serves as a scale of the black texture; thus the texture can have RGB8 and will be up-scaled to fill whole definition range, but has a fine resolution in low intensity values
}												
                                                
sampler samLin : register( s0 );               
sampler samWarp : register( s1 );               
sampler samContent : register( s2 );
                                                
//-------------------------------------------------------------
struct VS_INPUT												
{																
    float4 Pos : POSITION;										
    float2 Tex : TEXCOORD0;									
};																
																
struct VS_OUT {                                 
    float4 pos : SV_Position;                   
    float2 tex : TEXCOORD0;                     
};                  
																
//-------------------------------------------------------------
// Vertex Shader												
//-------------------------------------------------------------
VS_OUT VS( VS_INPUT input )									
{																
    VS_OUT output = (VS_OUT)0;								
    output.pos = input.Pos;									
    output.tex = input.Tex;	    
    return output;												
}																
                                                
//-------------------------------------------------------------
// Pixel Shaders												
//-------------------------------------------------------------
float4 tex2DBC(uniform Texture2D texCnt,
               float2            vPos)
{
	vPos*= params.xy;
	float2 t = floor( vPos - 0.5 ) + 0.5; // the nearest pixel
	float2 w0 = 1;
	float2 w1 = vPos - t;
	float2 w2 = w1 * w1;
	float2 w3 = w2 * w1;

	w0 = w2 - 0.5 * (w3 + w1);
	w1 = 1.5 * w3 - 2.5 * w2 + 1.0;
	w3 = 0.5 * (w3 - w2);
	w2 = 1.0 - w0 - w1 - w3;

	float2 s0 = w0 + w1;
	float2 s1 = w2 + w3;
	float2 f0 = w1 / s0;
	float2 f1 = w3 / s1;

	float2 t0 = t - 1 + f0;
	float2 t1 = t + 1 + f1;
	t0*= params.zw;
	t1*= params.zw;

	return
		( texCnt.Sample( samContent, t0 ) * s0.x +
		  texCnt.Sample( samContent, float2( t1.x, t0.y ) ) * s1.x ) * s0.y +
		( texCnt.Sample( samContent, float2( t0.x, t1.y ) ) * s0.x +
		  texCnt.Sample( samContent, t1 ) * s1.x ) * s1.y;
}
                                                
float4 PS( VS_OUT vIn ) : SV_Target                 
{                                               
	 float4 color = texContent.Sample( samContent, ( vIn.tex - offsScale.xy ) * offsScale.zw ); 
    return color;                               
}                                               
                                                
float4 PSWB( VS_OUT vIn ) : SV_Target               
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = 0;
	float4 vCur = 0;
	if( 0.1 < blend.a )
	{
		if( bBorder.x > 0.5 )                      
		{                                           
		    tex.x*= 1.02;                           
		    tex.x-= 0.01;                           
		    tex.y*= 1.02;                           
		    tex.y-= 0.01;                           
		}                                           
		tex.xy/= blend.a;
		tex.y = 1 - tex.y;
		vOut = texContent.Sample( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}
	vOut.a = 1;                                 
	return vOut;                                
}                                               
                                                
float4 PSWB3D_( VS_OUT vIn ) : SV_Target             
{                                               
	float4 vOut = texContent.Sample( samWarp, vIn.tex );     
	vOut.a = 1;                                
	return vOut;                                
}                                               

float4 PSWB3D( VS_OUT vIn ) : SV_Target             
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = float4( 0,0,0,1);             
	float4 vCur = 0;
	if( 0.1 < blend.a )                            
	{                                           
		tex/= blend.a;                            
		tex.w = 1;                            
		tex = mul( tex, matView );              
		tex.xy/= tex.w;                         
		tex.x/=2;                              
		tex.y/=2;                               
		tex.xy+= 0.5;                           
		vOut = texContent.Sample( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}                                               

float4 PSBC( VS_OUT vIn ) : SV_Target                 
{                                               
    float4 color = tex2DBC( texContent, ( vIn.tex - offsScale.xy ) * offsScale.zw ); 
    return color;                               
}                                               
                                                
float4 PSWBBC( VS_OUT vIn ) : SV_Target               
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = 0;
	float4 vCur = 0;
	if( 0.1 < blend.a )
	{
		if( bBorder.x > 0.5 )                      
		{                                           
		    tex.x*= 1.02;                           
		    tex.x-= 0.01;                           
		    tex.y*= 1.02;                           
		    tex.y-= 0.01;                           
		}                                           
		tex.xy/= blend.a;
		tex.y = 1 - tex.y;
		vOut = tex2DBC( texContent, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}
	vOut.a = 1;                                 
	return vOut;                                
}                                               
                                                
float4 PSWB3DBC( VS_OUT vIn ) : SV_Target             
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samLin, vIn.tex );  
	float4 black = texBlack.Sample( samLin, vIn.tex ) * blackBias.x;   
	float4 vOut = float4( 0,0,0,1);             
	float4 vCur = 0;
	if( 0.1 < blend.a )                            
	{                                           
		tex/= blend.a;                            
		tex.w = 1;                            
		tex = mul( tex, matView );              
		tex.xy/= tex.w;                         
		tex.x/=2;                              
		tex.y/=2;                               
		tex.xy+= 0.5;                           
		vOut = tex2DBC( texContent, ( tex.xy - offsScale.xy ) * offsScale.zw ); 
		vCur = texCur.Sample( samLin, (tex.xy - offsScaleCur.xy) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
		if( bBorder.z > 0.5 )                      
		{                                           
			vOut += blackBias.y * black;// offset color to get min average black
			vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping } vOut
			vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
		}                                           
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}
)END";