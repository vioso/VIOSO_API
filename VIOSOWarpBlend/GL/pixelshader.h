GLchar const* s_szPasstrough_vertex_shader_v110 = R"END(
#version 110
uniform vec4 offsScale;
void main()
{
	gl_Position=gl_Vertex;			
	gl_TexCoord[0]=gl_MultiTexCoord0;
	gl_TexCoord[0].x-=offScale.x;	
	gl_TexCoord[0].y-=offScale.y;	
	gl_TexCoord[0].x*=offScale.z;	
	gl_TexCoord[0].x*=offScale.w;	
}
)END";

GLchar const* s_szPasstrough_vertex_shader_v330 = R"END(
#version 330
in vec2 TexCoord;
in vec3 Position;

out vec2 texcoord;

const vec2 pos[4] =
vec2[4](
	vec2(-1.0, 1.0),
	vec2(-1.0,-1.0),
	vec2( 1.0, 1.0),
	vec2( 1.0,-1.0)
);

const vec2 tex[4] =
vec2[4](
	vec2(0,0),
	vec2(0,1),
	vec2(1,0),
	vec2(1,1)
);

uniform vec4 offsScale;
uniform vec4 size;

void main(void) {
	gl_Position = vec4((pos[gl_VertexID].x + pos[gl_VertexID].x * size.x) * size.z, (pos[gl_VertexID].y + pos[gl_VertexID].y * size.y) * size.w, 0.0, 1.0);
	texcoord = tex[gl_VertexID];
	texcoord-= offsScale.xy;
	texcoord*= offsScale.zw;
}
)END";

GLchar const* s_fragment_shader_header_v110 = R"END(
#version 110				
uniform sampler2D samContent, samWarp, samBlend,samBlack;
uniform bool bBorder;
uniform bool bDoNotBlend;
uniform bool bDoNoBlack;		
uniform mat4 matView;		
uniform vec4 blackBias;
#define texcoord gl_TexCoord[0]
#define FragColor gl_FragColor
vec4 _tex2D( sampler2D sam, vec2 tex ){ return texture2D( sam, tex ); }
)END";

GLchar const* s_fragment_shader_header_v330 = R"END(
#version 330
uniform sampler2D samContent, samWarp, samBlend, samBlack;
uniform bool bBorder;	
uniform bool bDoNotBlend;
uniform bool bDoNoBlack;
uniform mat4 matView;	
uniform vec4 blackBias;
in vec2 texcoord;		
out vec4 FragColor;	
vec4 _tex2D( sampler2D sam, vec2 tex ){ return texture( sam, tex ); }
)END";

GLchar const* s_func_tex2D_BC = R"END(
uniform vec4 params;
vec4 _texture2D( sampler2D texCnt,
				   vec2 vPos)		
{									
	vPos*= params.xy;				
	vec2 t = floor( vPos - 0.5 ) + vec2(0.5,0.5); // the nearest pixel
	vec2 w0 = vec2(1,1);
	vec2 w1 = vPos - t;	
	vec2 w2 = w1 * w1;	
	vec2 w3 = w2 * w1;	

	w0 = w2 - 0.5 * (w3 + w1);		
	w1 = 1.5 * w3 - 2.5 * w2 + 1.0;	
	w3 = 0.5 * (w3 - w2);			
	w2 = 1.0 - w0 - w1 - w3;		

	vec2 s0 = w0 + w1;				
	vec2 s1 = w2 + w3;				
	vec2 f0 = w1 / s0;				
	vec2 f1 = w3 / s1;				

	vec2 t0 = t - 1.0 + f0;			
	vec2 t1 = t + 1.0 + f1;			
	t0*= params.zw;					
	t1*= params.zw;					

	return
		( _tex2D( texCnt, t0 ) * s0.x +
		  _tex2D( texCnt, vec2( t1.x, t0.y ) ) * s1.x ) * s0.y +
		( _tex2D( texCnt, vec2( t0.x, t1.y ) ) * s0.x +			
		  _tex2D( texCnt, t1 ) * s1.x ) * s1.y;					
}																
)END";

GLchar const* s_func_tex2D = R"END(
vec4 _texture2D( sampler2D texCnt,								
				   vec2 vPos)									
{																
	return _tex2D( texCnt, vPos );								
}																
)END";

GLchar const* s_bypass_fragment_shader = R"END(
void main()													
{																
	FragColor = _texture2D( samContent,texcoord.st );			
	FragColor.a = 1.0;											
}																
)END";

GLchar const* s_warp_blend_fragment_shader = R"END(
void main()\n"
{																
	vec4 tex = _tex2D( samWarp,texcoord.st );					
	vec4 blend = _tex2D( samBlend, texcoord.st );				
	vec4 black = _tex2D( samBlack, texcoord.st ) * blackBias.x;	
	if( 0.1 < blend.a )											
	{															
		tex.y = 1.0 - tex.y;									
		if( bBorder )											
		{														
		    tex.x*= 1.02;										
		    tex.x-= 0.01;										
		    tex.y*= 1.02;										
		    tex.y-= 0.01;										
		}														
		tex.xy/= blend.a;										"
		FragColor = _texture2D( samContent, tex.xy );			
		if( !bDoNotBlend )					
			FragColor.rgb*= blend.rgb;		
		if( !bDoNoBlack )               
		{
			// offset color to get min average black
			FragColor += blackBias.y * black;					

			// scale down to avoid clipping vOut
			FragColor *= vec4(1,1,1,1) - blackBias.z * black;
	
			// do lower clamp to stay above common black, upper is done anyways
			FragColor = max( FragColor, black );				
		}
		FragColor.a = 1.0;
	}	
	else
	{	
		FragColor = vec4( 0.0,0.0,0.0,1.0 );
	}
}
)END";

GLchar const* s_warp_blend_fragment_shader_3D = R"END(
void main()
{
	vec4 tex = _tex2D( samWarp, texcoord.st );
	vec4 blend = _tex2D( samBlend, texcoord.st );
	vec4 black = _tex2D( samBlack, texcoord.st ) * blackBias.x;
	if( 0.01 < blend.a )
	{
		tex/= blend.a;
		tex.a = 1;
		tex = matView * tex;
		tex.xy/= tex.w;
		tex.x/=2.0;
		tex.y/=2.0;
		tex.xy+= 0.5;
//		// test mappings and border fit
//		if( 0.01 <= tex.x && tex.x <= 0.99 && 0.01 <= tex.y && tex.y <= 0.99 )                    
//			FragColor = vec4( tex.x, 1 - tex.y, 1, 1 );
//		else if( 0 <= tex.x && tex.x <= 1 && 0 <= tex.y && tex.y <= 1 )                    
//			FragColor = vec4( tex.x, 1 - tex.y, 0, 1 );
//		else
//			FragColor = vec4( 0, 0, 0, 1 );
		FragColor = _texture2D( samContent, tex.xy );
		if( !bDoNotBlend )
			FragColor.rgb*= blend.rgb;
		if( !bDoNoBlack )
		{
			// offset color to get min average black
			FragColor += blackBias.y * black;					

			// scale down to avoid clipping vOut
			FragColor *= vec4(1,1,1,1) - blackBias.z * black;
	
			// do lower clamp to stay above common black, upper is done anyways
			FragColor = max( FragColor, black );				
		}
		FragColor.a = 1.0;
	}
	else
	{
		FragColor = vec4( 0.0,0.0,0.0,1.0 );
	}
}
)END";
