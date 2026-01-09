// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

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
	}
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );

		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black

		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		

		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	}                                           
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );

		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	}
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	}                                           
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
	}
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
	}                                           
	vOut.a = 1;                                 
	return vOut;                                
}                                               

float4 PSWB3D( VS_OUT vIn ) : SV_Target
{
	uint w,h;
	texWarp.GetDimensions( w, h );
	float4 tex = texWarp.Load( int3( w * vIn.tex.x, h * vIn.tex.y, 0 ) );     
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
		vOut = texContent.Sample( samLin, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		float4 vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
	}                                           
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
	}
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
	}                                           
	vOut.a = 1;                                 
	return vOut;                                
}                                               
                                                
float4 PSWB3DBC( VS_OUT vIn ) : SV_Target             
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
	}                                           
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
	}
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
		vOut = texContent.Sample( samLin, ( tex.xy - offsScale.xy ) * offsScale.zw );  
		vCur = texCur.Sample( samLin, ( tex.xy - offsScaleCur.xy ) * offsScaleCur.zw );  
		vOut.rgb = vCur.a * vCur.rgb + vOut.rgb * ( 1.0 - vCur.a );
		if( bBorder.y > 0.5 )                      
			vOut.rgb*= blend.rgb;			        
	}                                           
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
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
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
	}
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
	}                                           
	vOut.a = 1;                                 
	return vOut;                                
}                                               
                                                
float4 PSWB3DBC( VS_OUT vIn ) : SV_Target             
{                                               
	float4 tex = texWarp.Sample( samWarp, vIn.tex );     
	float4 blend = texBlend.Sample( samWarp, vIn.tex );  
	float4 black = texBlack.Sample( samWarp, vIn.tex ) * blackBias.x;   
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
	}                                           
	if( bBorder.z > 0.5 )                      
	{                                           
		// degamma
		vOut = pow( vOut, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		black = pow( black, float4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vOut *= float4(1,1,1,1) - blackBias.z * black; // scale down to avoid clipping vOut
		vOut += blackBias.y * black;// offset color to get min average black
		// regamma
		vOut = pow( vOut, float4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		
		vOut = max( vOut, black ); // do lower clamp to stay above common black, upper is done anyways
	}                                           
	vOut.a = 1;                                
	return vOut;                                
}
)END";

static const char* s_dpShaderDX4 = R"XX(\
cbuffer cbConstData : register(b0)
{
	float4 cb[4];
	matrix camera_mvp;
	float4 camera_position;
};
// the params are used to avoid branching in the shader, so whenever use as if( param[0] ) or param[0] ? a : b, so the GPU can shortcut
// otherwise it will execute both, if and else parts, and blend the result
// 0: gamma, // gamma linearization for mapping textures
// 1: warping, // 0: no warping, 1: warping
// 2: blending, // 0: no blending, 1: blending
// 3: bla, // 0: no black level adjustment, otherwise it is multiplied to the sampled value
// 4: secondary blending, // 0: no secondary blending, 1: secondary blending
// 5: input gamma, // input gamma for content
// 6: output gamma reciprocal, // 1 / output gamma for content
// 7: color correction, // 0: no color correction, otherwise it is multiplied to the sampled value
// 8: flip_v, // 0 : no flip, 1: flip vertically
// 9: true: use LINEAR + UV ADDRESS WRAP sampling in content, LINEAR + BORDER otherwise
// 10-15: reserved
static float params[16] = (float[16])cb;

// Define quad vertices as constants
static const float4 vertices[4] = {
    float4(-1.0f,  1.0f, 0.0f, 1.0f),   // Top-left
    float4( 1.0f,  1.0f, 0.0f, 1.0f),    // Top-right
    float4(-1.0f, -1.0f, 0.0f, 1.0f),  // Bottom-left
    float4( 1.0f, -1.0f, 0.0f, 1.0f)    // Bottom-right
};
static const float2 uvs[4] = {
    float2( 0.0f, 0.0f ),   // Top-left
    float2( 1.0f, 0.0f ),    // Top-right
    float2( 0.0f, 1.0f ),  // Bottom-left
    float2( 1.0f, 1.0f )    // Bottom-right
};

// Vertex shader input structure
struct VSInput
{
    uint vertexID : SV_VertexID;
};

struct VS_INPUT
{
	float3 Pos : POSITION0; // the 3D position of the mesh, the actual screen
	float2 Tex      : TEXCOORD0; // the pixel uv of the display
	float3 Normal    : NORMAL; // the screen normal
	float3 Tangent   : TANGENT; // the screen tangent
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION; // the vertex position in screen space, z is 0..1 depth, normally this is same like tex0
	float2 Tex0     : TEXCOORD0; // the lookup coordinate in screen-sized mapping texture, for blend and black level correction
	float2 Tex1		: TEXCOORD1; // the lookup coordinate for the content
	float2 Tex2     : TEXCOORD2; // lookup in directional shading map
};

Texture2D txDirectionalShading : register(t0); // the directional shading texture
Texture2D txBlending : register(t1); // the blending texture
Texture2D txBlackLevel : register(t2); // the black level uplift alias beta texture
Texture2D txSecondaryBlending : register(t3); // the secondary blending texture
Texture2D texContent : register(t4); // the content texture
SamplerState samLinear : register(s0); // UV LINEAR + BORDER
SamplerState samPoint : register(s1); // UV POINT + BORDER
SamplerState samLinWrap : register(s2); // UV LINEAR + WRAP

struct PS_OUTPUT
{
	float4 Color : SV_Target;
};

VS_OUTPUT VSMESH(in VS_INPUT In)
{
	VS_OUTPUT Out;
	// compute the position in clip space
	float4 pos = mul( float4( In.Pos, 1 ), camera_mvp );
	// pass through texture coordinate, to sample from mappings
	Out.Tex0 = float2( In.Tex.x, params[8] > 0.0 ? 1.0 - In.Tex.y : In.Tex.y );
	Out.Position = float4( Out.Tex0.x * 2 - 1.0, 1.0 - Out.Tex0.y * 2, 0, 1.0 );

	// get content uv from normalized device coordinates
	Out.Tex1 = pos.xy;
	Out.Tex1 /= pos.w;
	Out.Tex1.x += 1.0;
	Out.Tex1.x *= 0.5;
	if( params[8] > 0.0 ) {
		Out.Tex1.y += 1.0;
		Out.Tex1.y *= 0.5;
	} else {
		Out.Tex1.y -= 1.0;
		Out.Tex1.y *= -0.5;
	}
		
	// calculate the color correction look up
	float3 dir = camera_position.xyz - In.Pos;
	// if direction is too flat, or no normals given, use a default direction
	if( params[7] && length(dir) > 0.0001 && ( In.Normal.x != 0 || In.Normal.y != 0 && In.Normal.z != 0 ) )  {
		dir = normalize( dir );
		float3 bitan = normalize( cross( In.Normal, In.Tangent ) );
		// projecting eye by tangent and bitangent effectively gives a perspective mapping
		float2 dvec = float2( dot( dir, In.Tangent ), dot( dir, bitan ) );
		// go from perspective to spherical mapping
		//dvec *= acos( clamp( dot( dir, In.Normal ), -1.0, 1.0 ) ) / 1.57079632679489661923;
		//Out.Tex2 = float2( ( dvec.x + 1.0 ) / 2.0, 1.0 - ( dvec.y + 1.0 ) / 2.0 );
		Out.Tex2 = float2( ( dir.x + 1.0 ) / 2.0, 1.0 - ( dir.y + 1.0 ) / 2.0 );
	} else {
		Out.Tex2 = float2( 0.5, 0.5 ); // neutral direction
	}
	return Out;
}

PS_OUTPUT PSDP(in VS_OUTPUT In)
{
	float3 gamma = float3(params[0], params[0], params[0]);
	float3 inputGamma = float3(params[5], params[5], params[5]);
	float3 outputGamma = float3(params[6], params[6], params[6]);

	// sample content
	float3 output;
	if( params[9] > 0.0 )
		output = texContent.Sample( samLinWrap, lerp( In.Tex0, In.Tex1, params[1] ) ).rgb;
	else
		output = texContent.Sample( samLinear, lerp( In.Tex0, In.Tex1, params[1] ) ).rgb;
	output = pow( output, inputGamma ); // linearize content

	// apply directional shading, TODO: move linearization to texture loader
	if( params[7] > 0.0 ) {
		float3 clcrt = txDirectionalShading.Sample( samLinWrap, In.Tex2 ).rgb * params[7];
		clcrt = pow( clcrt, gamma );
		output *= clcrt;
	}

	// apply blending, blend texture is already linearized
	if( params[2] > 0.0 ) {
		float3 blend1 = txBlending.Sample( samLinear, In.Tex0 ).rgb;
		output *= blend1;
	}

	// apply secondary blending, TODO: move linearization to texture loader
	if( params[4] > 0.0 ) {
		float3 blend2 = txSecondaryBlending.Sample( samLinear, In.Tex0 ).rgb;
		blend2 = pow( blend2, gamma );  // linearize
		output *= blend2;
	}

	// apply black level uplift, TODO: move linearization to texture loader
	if( params[3] > 0.0 ) {
		float3 bla = txBlackLevel.Sample( samLinear, In.Tex0 ).rgb * params[3];
	    bla = pow( bla, gamma ); // linearize
		output = output * ( 1.0 - bla ) + bla;
	}

	PS_OUTPUT Out;
	Out.Color = float4( pow( output, outputGamma ).rgb, 1.0 );
	return Out;
}

PS_OUTPUT PSDP_DBG(in VS_OUTPUT In)
{

	PS_OUTPUT Out;
	Out.Color = float4(  In.Tex2, 0.0, 1.0 );
	return Out;
}

PS_OUTPUT PS(in VS_OUTPUT In)
{
	PS_OUTPUT Out;
	Out.Color = float4( texContent.Sample( samLinear, In.Tex0 ).rgb, 1 );
	return Out;
}

)XX";
